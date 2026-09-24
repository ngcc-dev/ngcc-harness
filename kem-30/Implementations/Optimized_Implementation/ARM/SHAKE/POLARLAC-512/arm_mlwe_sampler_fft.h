#ifndef ARM_MLWE_SAMPLER_FFT_H
#define ARM_MLWE_SAMPLER_FFT_H

#include <stddef.h>
#include <stdint.h>

#if defined(POLARLAC_COMPONENT_B_TEST_DISABLE) && !defined(POLARLAC_B_TEST_HOOKS)
#error "POLARLAC_COMPONENT_B_TEST_DISABLE is reserved for B0 test-oracle builds"
#endif

#if defined(POLARLAC_B_TEST_HOOKS) && defined(POLARLAC_COMPONENT_B_TEST_DISABLE)
#define POLARLAC_COMPONENT_B_FFT_SELECTED 0
#define POLARLAC_COMPONENT_B_TERNARY_SELECTED 0
#define POLARLAC_COMPONENT_B_UNIFORM_SELECTED 0
#else
#ifndef POLARLAC_COMPONENT_B_FFT_SELECTED
#define POLARLAC_COMPONENT_B_FFT_SELECTED 1
#endif
#ifndef POLARLAC_COMPONENT_B_TERNARY_SELECTED
#define POLARLAC_COMPONENT_B_TERNARY_SELECTED 0
#endif
#ifndef POLARLAC_COMPONENT_B_UNIFORM_SELECTED
#define POLARLAC_COMPONENT_B_UNIFORM_SELECTED 0
#endif
#endif

/* Backward-compatible alias used only for FFT helper code paths. */
#define POLARLAC_COMPONENT_B_SELECTED POLARLAC_COMPONENT_B_FFT_SELECTED

#if (POLARLAC_COMPONENT_B_FFT_SELECTED || \
     POLARLAC_COMPONENT_B_TERNARY_SELECTED || \
     POLARLAC_COMPONENT_B_UNIFORM_SELECTED) && defined(__aarch64__)
#include <arm_neon.h>
#endif

/*
 * The ternary sampler consumes a byte string, transposes its bits into a
 * plane-major stream, and then combines three or four equally sized logical
 * bit-vectors.  The cursor representation below computes the same bit order
 * directly without materialising t[3N] or t[4N].  Cursor evolution depends
 * only on public parameters and the public loop index.
 */
typedef struct {
    size_t byte_index;
    unsigned int bit_index;
} polarlac_bit_cursor;

static inline polarlac_bit_cursor polarlac_bit_cursor_init(
    size_t flattened_bit_index,
    size_t source_bytes)
{
    polarlac_bit_cursor c;
    c.byte_index = flattened_bit_index % source_bytes;
    c.bit_index = (unsigned int)(flattened_bit_index / source_bytes);
    return c;
}

static inline void polarlac_bit_cursor_advance(
    polarlac_bit_cursor *c,
    size_t amount,
    size_t source_bytes)
{
    c->byte_index += amount;
    if (c->byte_index == source_bytes) {
        c->byte_index = 0;
        c->bit_index++;
    }
}

static inline size_t polarlac_min_size(size_t a, size_t b)
{
    return a < b ? a : b;
}

static inline uint8_t polarlac_cursor_bit(
    const uint8_t *buf,
    polarlac_bit_cursor c)
{
    return (uint8_t)((buf[c.byte_index] >> c.bit_index) & 1u);
}

static inline void polarlac_ternary_3plane_from_bytes(
    int16_t *out,
    size_t coefficient_count,
    const uint8_t *buf,
    size_t source_bytes)
{
    polarlac_bit_cursor c0 = polarlac_bit_cursor_init(0, source_bytes);
    polarlac_bit_cursor c1 = polarlac_bit_cursor_init(coefficient_count, source_bytes);
    polarlac_bit_cursor c2 = polarlac_bit_cursor_init(2u * coefficient_count, source_bytes);
    size_t produced = 0;

    while (produced < coefficient_count) {
        size_t run = coefficient_count - produced;
        run = polarlac_min_size(run, source_bytes - c0.byte_index);
        run = polarlac_min_size(run, source_bytes - c1.byte_index);
        run = polarlac_min_size(run, source_bytes - c2.byte_index);

#if POLARLAC_COMPONENT_B_TERNARY_SELECTED && defined(__aarch64__)
        while (run >= 16u) {
            const int8x16_t sh0 = vdupq_n_s8((int8_t)-(int)c0.bit_index);
            const int8x16_t sh1 = vdupq_n_s8((int8_t)-(int)c1.bit_index);
            const int8x16_t sh2 = vdupq_n_s8((int8_t)-(int)c2.bit_index);
            const uint8x16_t one = vdupq_n_u8(1u);
            uint8x16_t b0 = vandq_u8(vshlq_u8(vld1q_u8(buf + c0.byte_index), sh0), one);
            uint8x16_t b1 = vandq_u8(vshlq_u8(vld1q_u8(buf + c1.byte_index), sh1), one);
            uint8x16_t b2 = vandq_u8(vshlq_u8(vld1q_u8(buf + c2.byte_index), sh2), one);
            int8x16_t diff = vsubq_s8(vreinterpretq_s8_u8(b0), vreinterpretq_s8_u8(b1));
            int8x16_t value = vmulq_s8(diff, vreinterpretq_s8_u8(b2));

            vst1q_s16(out + produced,
                      vmovl_s8(vget_low_s8(value)));
            vst1q_s16(out + produced + 8u,
                      vmovl_s8(vget_high_s8(value)));

            produced += 16u;
            run -= 16u;
            polarlac_bit_cursor_advance(&c0, 16u, source_bytes);
            polarlac_bit_cursor_advance(&c1, 16u, source_bytes);
            polarlac_bit_cursor_advance(&c2, 16u, source_bytes);
        }
#endif

        while (run != 0u) {
            const int16_t b0 = (int16_t)polarlac_cursor_bit(buf, c0);
            const int16_t b1 = (int16_t)polarlac_cursor_bit(buf, c1);
            const int16_t b2 = (int16_t)polarlac_cursor_bit(buf, c2);
            out[produced++] = (int16_t)((b0 - b1) * b2);
            run--;
            polarlac_bit_cursor_advance(&c0, 1u, source_bytes);
            polarlac_bit_cursor_advance(&c1, 1u, source_bytes);
            polarlac_bit_cursor_advance(&c2, 1u, source_bytes);
        }
    }
}

static inline void polarlac_ternary_4plane_from_bytes(
    int16_t *out,
    size_t coefficient_count,
    const uint8_t *buf,
    size_t source_bytes)
{
    polarlac_bit_cursor c0 = polarlac_bit_cursor_init(0, source_bytes);
    polarlac_bit_cursor c1 = polarlac_bit_cursor_init(coefficient_count, source_bytes);
    polarlac_bit_cursor c2 = polarlac_bit_cursor_init(2u * coefficient_count, source_bytes);
    polarlac_bit_cursor c3 = polarlac_bit_cursor_init(3u * coefficient_count, source_bytes);
    size_t produced = 0;

    while (produced < coefficient_count) {
        size_t run = coefficient_count - produced;
        run = polarlac_min_size(run, source_bytes - c0.byte_index);
        run = polarlac_min_size(run, source_bytes - c1.byte_index);
        run = polarlac_min_size(run, source_bytes - c2.byte_index);
        run = polarlac_min_size(run, source_bytes - c3.byte_index);

#if POLARLAC_COMPONENT_B_TERNARY_SELECTED && defined(__aarch64__)
        while (run >= 16u) {
            const int8x16_t sh0 = vdupq_n_s8((int8_t)-(int)c0.bit_index);
            const int8x16_t sh1 = vdupq_n_s8((int8_t)-(int)c1.bit_index);
            const int8x16_t sh2 = vdupq_n_s8((int8_t)-(int)c2.bit_index);
            const int8x16_t sh3 = vdupq_n_s8((int8_t)-(int)c3.bit_index);
            const uint8x16_t one = vdupq_n_u8(1u);
            uint8x16_t b0 = vandq_u8(vshlq_u8(vld1q_u8(buf + c0.byte_index), sh0), one);
            uint8x16_t b1 = vandq_u8(vshlq_u8(vld1q_u8(buf + c1.byte_index), sh1), one);
            uint8x16_t b2 = vandq_u8(vshlq_u8(vld1q_u8(buf + c2.byte_index), sh2), one);
            uint8x16_t b3 = vandq_u8(vshlq_u8(vld1q_u8(buf + c3.byte_index), sh3), one);
            int8x16_t lhs = vreinterpretq_s8_u8(vandq_u8(b0, b1));
            int8x16_t rhs = vreinterpretq_s8_u8(vandq_u8(b2, b3));
            int8x16_t value = vsubq_s8(lhs, rhs);

            vst1q_s16(out + produced,
                      vmovl_s8(vget_low_s8(value)));
            vst1q_s16(out + produced + 8u,
                      vmovl_s8(vget_high_s8(value)));

            produced += 16u;
            run -= 16u;
            polarlac_bit_cursor_advance(&c0, 16u, source_bytes);
            polarlac_bit_cursor_advance(&c1, 16u, source_bytes);
            polarlac_bit_cursor_advance(&c2, 16u, source_bytes);
            polarlac_bit_cursor_advance(&c3, 16u, source_bytes);
        }
#endif

        while (run != 0u) {
            const int16_t b0 = (int16_t)polarlac_cursor_bit(buf, c0);
            const int16_t b1 = (int16_t)polarlac_cursor_bit(buf, c1);
            const int16_t b2 = (int16_t)polarlac_cursor_bit(buf, c2);
            const int16_t b3 = (int16_t)polarlac_cursor_bit(buf, c3);
            out[produced++] = (int16_t)((b0 & b1) - (b2 & b3));
            run--;
            polarlac_bit_cursor_advance(&c0, 1u, source_bytes);
            polarlac_bit_cursor_advance(&c1, 1u, source_bytes);
            polarlac_bit_cursor_advance(&c2, 1u, source_bytes);
            polarlac_bit_cursor_advance(&c3, 1u, source_bytes);
        }
    }
}


static inline int16_t polarlac_ternary_11_106_11_map(uint8_t k)
{
    const uint16_t mneg = (uint16_t)(0u - (((uint16_t)k - 11u) >> 15));
    const uint16_t mpos = (uint16_t)(0u - (((uint16_t)116u - (uint16_t)k) >> 15));
    return (int16_t)((mpos & 1u) - (mneg & 1u));
}

static inline void polarlac_ternary_7byte_groups_from_bytes(
    int16_t *out,
    size_t coefficient_count,
    const uint8_t *buf,
    size_t source_bytes)
{
    size_t produced = 0;
    size_t i = 0;
    (void)source_bytes;

    while (produced < coefficient_count) {
        const uint8_t x0 = buf[i++];
        const uint8_t x1 = buf[i++];
        const uint8_t x2 = buf[i++];
        const uint8_t x3 = buf[i++];
        const uint8_t x4 = buf[i++];
        const uint8_t x5 = buf[i++];
        const uint8_t x6 = buf[i++];
        const uint8_t k7 = (uint8_t)((((uint16_t)(x0 >> 7) & 1u) << 6) |
                                     (((uint16_t)(x1 >> 7) & 1u) << 5) |
                                     (((uint16_t)(x2 >> 7) & 1u) << 4) |
                                     (((uint16_t)(x3 >> 7) & 1u) << 3) |
                                     (((uint16_t)(x4 >> 7) & 1u) << 2) |
                                     (((uint16_t)(x5 >> 7) & 1u) << 1) |
                                      ((uint16_t)(x6 >> 7) & 1u));

        out[produced++] = polarlac_ternary_11_106_11_map((uint8_t)(x0 & 127u));
        out[produced++] = polarlac_ternary_11_106_11_map((uint8_t)(x1 & 127u));
        out[produced++] = polarlac_ternary_11_106_11_map((uint8_t)(x2 & 127u));
        out[produced++] = polarlac_ternary_11_106_11_map((uint8_t)(x3 & 127u));
        out[produced++] = polarlac_ternary_11_106_11_map((uint8_t)(x4 & 127u));
        out[produced++] = polarlac_ternary_11_106_11_map((uint8_t)(x5 & 127u));
        out[produced++] = polarlac_ternary_11_106_11_map((uint8_t)(x6 & 127u));
        out[produced++] = polarlac_ternary_11_106_11_map(k7);
    }
}

static inline void polarlac_expand_u8_to_i16(
    int16_t *out,
    const uint8_t *in,
    size_t count)
{
    size_t i = 0;
#if POLARLAC_COMPONENT_B_UNIFORM_SELECTED && defined(__aarch64__)
    const size_t count16 = count & ~(size_t)15u;
    for (; i < count16; i += 16u) {
        uint8x16_t v = vld1q_u8(in + i);
        vst1q_s16(out + i,
                  vreinterpretq_s16_u16(vmovl_u8(vget_low_u8(v))));
        vst1q_s16(out + i + 8u,
                  vreinterpretq_s16_u16(vmovl_u8(vget_high_u8(v))));
    }
#endif
    for (; i < count; i++) {
        out[i] = (int16_t)in[i];
    }
}

#if defined(POLARLAC_ARM_MLWE_FFT_HELPERS) && \
    POLARLAC_COMPONENT_B_SELECTED && defined(__aarch64__)

/*
 * FFT products are bounded by the int16 input/twiddle ranges and remain far
 * from INT32_MIN, so vabsq_s32 implements the scalar symmetric rounding
 * exactly on every reachable lane value.
 */
static inline int32x4_t polarlac_fft_rsh32x4(int32x4_t x)
{
    const int32x4_t zero = vdupq_n_s32(0);
    const uint32x4_t negative = vcltq_s32(x, zero);
    const int32x4_t magnitude = vabsq_s32(x);
    const int32x4_t rounded = vshrq_n_s32(
        vaddq_s32(magnitude, vdupq_n_s32(1 << (FFT_I16_SHIFT - 1))),
        FFT_I16_SHIFT);
    const int32x4_t negated = vsubq_s32(zero, rounded);
    return vbslq_s32(negative, negated, rounded);
}

static inline int16x4_t polarlac_wrap_add_s16x4(int16x4_t a, int16x4_t b)
{
    return vreinterpret_s16_u16(
        vadd_u16(vreinterpret_u16_s16(a), vreinterpret_u16_s16(b)));
}

static inline int16x4_t polarlac_wrap_sub_s16x4(int16x4_t a, int16x4_t b)
{
    return vreinterpret_s16_u16(
        vsub_u16(vreinterpret_u16_s16(a), vreinterpret_u16_s16(b)));
}

/* Process a multiple of four combine elements and return the processed count. */
static inline size_t polarlac_fft_combine_neon(
    ci16_t *out,
    const ci16_t *even_unique,
    const ci16_t *odd_unique,
    const ci16_t *twiddle,
    size_t count)
{
    size_t k = 0;
    const size_t count4 = count & ~(size_t)3u;

    for (; k < count4; k += 4u) {
        int16x4x2_t ov = vld2_s16((const int16_t *)(const void *)(odd_unique + k));
        int16x4x2_t tv = vld2_s16((const int16_t *)(const void *)(twiddle + k));
        int16x4x2_t ev = vld2_s16((const int16_t *)(const void *)(even_unique + k));
        int32x4_t ore = vmovl_s16(ov.val[0]);
        int32x4_t oim = vmovl_s16(ov.val[1]);
        int32x4_t tre = vmovl_s16(tv.val[0]);
        int32x4_t tim = vmovl_s16(tv.val[1]);
        int32x4_t t0 = vmulq_s32(tre, ore);
        int32x4_t t2 = vmulq_s32(tim, oim);
        int32x4_t t1 = vmulq_s32(vaddq_s32(tre, tim),
                                  vaddq_s32(ore, oim));
        int16x4_t mr = vmovn_s32(polarlac_fft_rsh32x4(vsubq_s32(t0, t2)));
        int16x4_t mi = vmovn_s32(polarlac_fft_rsh32x4(
            vsubq_s32(vsubq_s32(t1, t0), t2)));
        int16x4_t plus_re = polarlac_wrap_add_s16x4(ev.val[0], mr);
        int16x4_t plus_im = polarlac_wrap_add_s16x4(ev.val[1], mi);
        int16x4_t minus_re = polarlac_wrap_sub_s16x4(ev.val[0], mr);
        int16x4_t minus_im = polarlac_wrap_sub_s16x4(ev.val[1], mi);
        int16x4x2_t re_zip = vzip_s16(plus_re, minus_re);
        int16x4x2_t im_zip = vzip_s16(plus_im, minus_im);
        int16x4x2_t store0 = {{ re_zip.val[0], im_zip.val[0] }};
        int16x4x2_t store1 = {{ re_zip.val[1], im_zip.val[1] }};

        vst2_s16((int16_t *)(void *)(out + (k << 1)), store0);
        vst2_s16((int16_t *)(void *)(out + (k << 1) + 4u), store1);
    }

    return k;
}

static inline size_t polarlac_fft_bound_neon(
    const ci16_t *uniq,
    size_t count,
    int32_t bound,
    uint32_t *reject_mask)
{
    size_t i = 0;
    const size_t count4 = count & ~(size_t)3u;
    uint32x4_t rejected = vdupq_n_u32(0u);
    const int32x4_t vb = vdupq_n_s32(bound);

    for (; i < count4; i += 4u) {
        int16x4x2_t v = vld2_s16((const int16_t *)(const void *)(uniq + i));
        int32x4_t re = polarlac_fft_rsh32x4(vmovl_s16(v.val[0]));
        int32x4_t im = polarlac_fft_rsh32x4(vmovl_s16(v.val[1]));
        int32x4_t comp = vaddq_s32(vmulq_s32(re, re), vmulq_s32(im, im));
        rejected = vorrq_u32(rejected, vcgeq_s32(comp, vb));
    }

    *reject_mask |= (vmaxvq_u32(rejected) != 0u);
    return i;
}

#endif /* B2 && AArch64 */

#endif /* ARM_MLWE_SAMPLER_FFT_H */
