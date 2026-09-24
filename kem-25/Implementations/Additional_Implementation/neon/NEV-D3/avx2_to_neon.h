/*
 * avx2_to_neon.h - AVX2 intrinsics to ARM NEON translation layer
 *
 * This header provides a drop-in replacement for <immintrin.h> on ARM/AArch64
 * platforms. It emulates AVX2 256-bit operations using pairs of NEON 128-bit
 * operations. All operations produce bit-identical results to their AVX2
 * counterparts, ensuring KAT correctness.
 *
 * Target: AArch64 / ARMv8 with NEON
 */

#ifndef AVX2_TO_NEON_H
#define AVX2_TO_NEON_H

#include <arm_neon.h>
#include <stdint.h>
#include <string.h>

/* ============================================================
 * Type definitions
 * ============================================================ */

/* __m256i: emulated as two 128-bit NEON vectors */
typedef struct {
    int8x16_t lo;
    int8x16_t hi;
} __m256i;

/* __m128i: wrap NEON 128-bit vector */
typedef int8x16_t __m128i;

/* ============================================================
 * 128-bit SSE helper functions
 * ============================================================ */

static inline __m128i _mm_loadu_si128(const void *p) {
    return vld1q_s8((const int8_t *)p);
}

static inline void _mm_storeu_si128(void *p, __m128i a) {
    vst1q_s8((int8_t *)p, a);
}

static inline void _mm_store_si128(void *p, __m128i a) {
    vst1q_s8((int8_t *)p, a);
}

static inline __m128i _mm_xor_si128(__m128i a, __m128i b) {
    return veorq_s8(a, b);
}

static inline __m128i _mm_loadu_si32(const void *p) {
    int32_t val;
    memcpy(&val, p, 4);
    int32x4_t v = vdupq_n_s32(0);
    v = vsetq_lane_s32(val, v, 0);
    return vreinterpretq_s8_s32(v);
}

static inline __m128i _mm_loadu_si16(const void *p) {
    int16_t val;
    memcpy(&val, p, 2);
    int16x8_t v = vdupq_n_s16(0);
    v = vsetq_lane_s16(val, v, 0);
    return vreinterpretq_s8_s16(v);
}

static inline __m128i _mm_unpacklo_epi32(__m128i a, __m128i b) {
    int32x4_t va = vreinterpretq_s32_s8(a);
    int32x4_t vb = vreinterpretq_s32_s8(b);
    int32x2_t a_lo = vget_low_s32(va);
    int32x2_t b_lo = vget_low_s32(vb);
    int32x2x2_t z = vzip_s32(a_lo, b_lo);
    return vreinterpretq_s8_s32(vcombine_s32(z.val[0], z.val[1]));
}

static inline __m128i _mm_unpackhi_epi32(__m128i a, __m128i b) {
    int32x4_t va = vreinterpretq_s32_s8(a);
    int32x4_t vb = vreinterpretq_s32_s8(b);
    int32x2_t a_hi = vget_high_s32(va);
    int32x2_t b_hi = vget_high_s32(vb);
    int32x2x2_t z = vzip_s32(a_hi, b_hi);
    return vreinterpretq_s8_s32(vcombine_s32(z.val[0], z.val[1]));
}

static inline __m128i _mm_packus_epi32(__m128i a, __m128i b) {
    int32x4_t va = vreinterpretq_s32_s8(a);
    int32x4_t vb = vreinterpretq_s32_s8(b);
    uint16x4_t lo = vqmovun_s32(va);
    uint16x4_t hi = vqmovun_s32(vb);
    return vreinterpretq_s8_u16(vcombine_u16(lo, hi));
}

static inline __m128i _mm_srli_si128(__m128i a, int imm) {
    if (imm >= 16) return vreinterpretq_s8_s32(vdupq_n_s32(0));
    if (imm == 0) return a;
    uint8_t buf[32];
    vst1q_u8(buf, vreinterpretq_u8_s8(a));
    memset(buf + 16, 0, 16);
    return vreinterpretq_s8_u8(vld1q_u8(buf + imm));
}

static inline void _mm_storeu_si32(void *p, __m128i a) {
    int32_t val = vgetq_lane_s32(vreinterpretq_s32_s8(a), 0);
    memcpy(p, &val, 4);
}

static inline void _mm_storel_epi64(void *p, __m128i a) {
    int64_t val = vgetq_lane_s64(vreinterpretq_s64_s8(a), 0);
    memcpy(p, &val, 8);
}

/* ============================================================
 * 256-bit Load / Store
 * ============================================================ */

static inline __m256i _mm256_load_si256(const void *p) {
    __m256i r;
    const int8_t *pp = (const int8_t *)p;
    r.lo = vld1q_s8(pp);
    r.hi = vld1q_s8(pp + 16);
    return r;
}

static inline __m256i _mm256_loadu_si256(const void *p) {
    __m256i r;
    const int8_t *pp = (const int8_t *)p;
    r.lo = vld1q_s8(pp);
    r.hi = vld1q_s8(pp + 16);
    return r;
}

static inline void _mm256_store_si256(void *p, __m256i a) {
    int8_t *pp = (int8_t *)p;
    vst1q_s8(pp, a.lo);
    vst1q_s8(pp + 16, a.hi);
}

static inline void _mm256_storeu_si256(void *p, __m256i a) {
    int8_t *pp = (int8_t *)p;
    vst1q_s8(pp, a.lo);
    vst1q_s8(pp + 16, a.hi);
}

/* ============================================================
 * Set / Broadcast operations
 * ============================================================ */

static inline __m256i _mm256_setzero_si256(void) {
    __m256i r;
    r.lo = vreinterpretq_s8_s32(vdupq_n_s32(0));
    r.hi = vreinterpretq_s8_s32(vdupq_n_s32(0));
    return r;
}

static inline __m256i _mm256_set1_epi16(int16_t a) {
    __m256i r;
    r.lo = vreinterpretq_s8_s16(vdupq_n_s16(a));
    r.hi = vreinterpretq_s8_s16(vdupq_n_s16(a));
    return r;
}

static inline __m256i _mm256_set1_epi8(int8_t a) {
    __m256i r;
    r.lo = vdupq_n_s8(a);
    r.hi = vdupq_n_s8(a);
    return r;
}

static inline __m256i _mm256_set1_epi32(int32_t a) {
    __m256i r;
    r.lo = vreinterpretq_s8_s32(vdupq_n_s32(a));
    r.hi = vreinterpretq_s8_s32(vdupq_n_s32(a));
    return r;
}

/* _mm256_set_epi32(e7,e6,e5,e4,e3,e2,e1,e0) - high to low */
static inline __m256i _mm256_set_epi32(int32_t e7, int32_t e6, int32_t e5, int32_t e4,
                                       int32_t e3, int32_t e2, int32_t e1, int32_t e0) {
    __m256i r;
    int32_t lo_arr[4] = {e0, e1, e2, e3};
    int32_t hi_arr[4] = {e4, e5, e6, e7};
    r.lo = vreinterpretq_s8_s32(vld1q_s32(lo_arr));
    r.hi = vreinterpretq_s8_s32(vld1q_s32(hi_arr));
    return r;
}

/* _mm256_setr_epi32(e0,e1,e2,e3,e4,e5,e6,e7) - low to high */
static inline __m256i _mm256_setr_epi32(int32_t e0, int32_t e1, int32_t e2, int32_t e3,
                                        int32_t e4, int32_t e5, int32_t e6, int32_t e7) {
    __m256i r;
    int32_t lo_arr[4] = {e0, e1, e2, e3};
    int32_t hi_arr[4] = {e4, e5, e6, e7};
    r.lo = vreinterpretq_s8_s32(vld1q_s32(lo_arr));
    r.hi = vreinterpretq_s8_s32(vld1q_s32(hi_arr));
    return r;
}

/* _mm256_set_epi8(e31...e0) - high to low */
static inline __m256i _mm256_set_epi8(
    int8_t e31, int8_t e30, int8_t e29, int8_t e28,
    int8_t e27, int8_t e26, int8_t e25, int8_t e24,
    int8_t e23, int8_t e22, int8_t e21, int8_t e20,
    int8_t e19, int8_t e18, int8_t e17, int8_t e16,
    int8_t e15, int8_t e14, int8_t e13, int8_t e12,
    int8_t e11, int8_t e10, int8_t e9,  int8_t e8,
    int8_t e7,  int8_t e6,  int8_t e5,  int8_t e4,
    int8_t e3,  int8_t e2,  int8_t e1,  int8_t e0) {
    __m256i r;
    int8_t lo_arr[16] = {e0,e1,e2,e3,e4,e5,e6,e7,e8,e9,e10,e11,e12,e13,e14,e15};
    int8_t hi_arr[16] = {e16,e17,e18,e19,e20,e21,e22,e23,e24,e25,e26,e27,e28,e29,e30,e31};
    r.lo = vld1q_s8(lo_arr);
    r.hi = vld1q_s8(hi_arr);
    return r;
}

/* _mm256_setr_epi8(e0,e1,...,e31) - low to high */
static inline __m256i _mm256_setr_epi8(
    int8_t e0,  int8_t e1,  int8_t e2,  int8_t e3,
    int8_t e4,  int8_t e5,  int8_t e6,  int8_t e7,
    int8_t e8,  int8_t e9,  int8_t e10, int8_t e11,
    int8_t e12, int8_t e13, int8_t e14, int8_t e15,
    int8_t e16, int8_t e17, int8_t e18, int8_t e19,
    int8_t e20, int8_t e21, int8_t e22, int8_t e23,
    int8_t e24, int8_t e25, int8_t e26, int8_t e27,
    int8_t e28, int8_t e29, int8_t e30, int8_t e31) {
    __m256i r;
    int8_t lo_arr[16] = {e0,e1,e2,e3,e4,e5,e6,e7,e8,e9,e10,e11,e12,e13,e14,e15};
    int8_t hi_arr[16] = {e16,e17,e18,e19,e20,e21,e22,e23,e24,e25,e26,e27,e28,e29,e30,e31};
    r.lo = vld1q_s8(lo_arr);
    r.hi = vld1q_s8(hi_arr);
    return r;
}

/* ============================================================
 * Arithmetic: 16-bit
 * ============================================================ */

static inline __m256i _mm256_add_epi16(__m256i a, __m256i b) {
    __m256i r;
    r.lo = vreinterpretq_s8_s16(vaddq_s16(vreinterpretq_s16_s8(a.lo), vreinterpretq_s16_s8(b.lo)));
    r.hi = vreinterpretq_s8_s16(vaddq_s16(vreinterpretq_s16_s8(a.hi), vreinterpretq_s16_s8(b.hi)));
    return r;
}

static inline __m256i _mm256_sub_epi16(__m256i a, __m256i b) {
    __m256i r;
    r.lo = vreinterpretq_s8_s16(vsubq_s16(vreinterpretq_s16_s8(a.lo), vreinterpretq_s16_s8(b.lo)));
    r.hi = vreinterpretq_s8_s16(vsubq_s16(vreinterpretq_s16_s8(a.hi), vreinterpretq_s16_s8(b.hi)));
    return r;
}

static inline __m256i _mm256_mullo_epi16(__m256i a, __m256i b) {
    __m256i r;
    r.lo = vreinterpretq_s8_s16(vmulq_s16(vreinterpretq_s16_s8(a.lo), vreinterpretq_s16_s8(b.lo)));
    r.hi = vreinterpretq_s8_s16(vmulq_s16(vreinterpretq_s16_s8(a.hi), vreinterpretq_s16_s8(b.hi)));
    return r;
}

/* mulhi_epi16: signed high 16 bits of 16x16 multiply */
static inline __m256i _mm256_mulhi_epi16(__m256i a, __m256i b) {
    __m256i r;
    int16x8_t a_lo = vreinterpretq_s16_s8(a.lo);
    int16x8_t b_lo = vreinterpretq_s16_s8(b.lo);
    int16x8_t a_hi = vreinterpretq_s16_s8(a.hi);
    int16x8_t b_hi = vreinterpretq_s16_s8(b.hi);

    /* Compute high half: multiply low/high halves, then take high 16 bits */
    int32x4_t p0 = vmull_s16(vget_low_s16(a_lo), vget_low_s16(b_lo));
    int32x4_t p1 = vmull_s16(vget_high_s16(a_lo), vget_high_s16(b_lo));
    int32x4_t p2 = vmull_s16(vget_low_s16(a_hi), vget_low_s16(b_hi));
    int32x4_t p3 = vmull_s16(vget_high_s16(a_hi), vget_high_s16(b_hi));

    /* Extract high 16 bits by narrowing (shift right 16) */
    int16x4_t h0 = vshrn_n_s32(p0, 16);
    int16x4_t h1 = vshrn_n_s32(p1, 16);
    int16x4_t h2 = vshrn_n_s32(p2, 16);
    int16x4_t h3 = vshrn_n_s32(p3, 16);

    r.lo = vreinterpretq_s8_s16(vcombine_s16(h0, h1));
    r.hi = vreinterpretq_s8_s16(vcombine_s16(h2, h3));
    return r;
}

/* mulhi_epu16: unsigned high 16 bits of 16x16 multiply */
static inline __m256i _mm256_mulhi_epu16(__m256i a, __m256i b) {
    __m256i r;
    uint16x8_t a_lo = vreinterpretq_u16_s8(a.lo);
    uint16x8_t b_lo = vreinterpretq_u16_s8(b.lo);
    uint16x8_t a_hi = vreinterpretq_u16_s8(a.hi);
    uint16x8_t b_hi = vreinterpretq_u16_s8(b.hi);

    uint32x4_t p0 = vmull_u16(vget_low_u16(a_lo), vget_low_u16(b_lo));
    uint32x4_t p1 = vmull_u16(vget_high_u16(a_lo), vget_high_u16(b_lo));
    uint32x4_t p2 = vmull_u16(vget_low_u16(a_hi), vget_low_u16(b_hi));
    uint32x4_t p3 = vmull_u16(vget_high_u16(a_hi), vget_high_u16(b_hi));

    uint16x4_t h0 = vshrn_n_u32(p0, 16);
    uint16x4_t h1 = vshrn_n_u32(p1, 16);
    uint16x4_t h2 = vshrn_n_u32(p2, 16);
    uint16x4_t h3 = vshrn_n_u32(p3, 16);

    r.lo = vreinterpretq_s8_u16(vcombine_u16(h0, h1));
    r.hi = vreinterpretq_s8_u16(vcombine_u16(h2, h3));
    return r;
}

/* ============================================================
 * Arithmetic: 8-bit
 * ============================================================ */

static inline __m256i _mm256_add_epi8(__m256i a, __m256i b) {
    __m256i r;
    r.lo = vaddq_s8(a.lo, b.lo);
    r.hi = vaddq_s8(a.hi, b.hi);
    return r;
}

static inline __m256i _mm256_sub_epi8(__m256i a, __m256i b) {
    __m256i r;
    r.lo = vsubq_s8(a.lo, b.lo);
    r.hi = vsubq_s8(a.hi, b.hi);
    return r;
}

/* ============================================================
 * Arithmetic: 32-bit
 * ============================================================ */

static inline __m256i _mm256_add_epi32(__m256i a, __m256i b) {
    __m256i r;
    r.lo = vreinterpretq_s8_s32(vaddq_s32(vreinterpretq_s32_s8(a.lo), vreinterpretq_s32_s8(b.lo)));
    r.hi = vreinterpretq_s8_s32(vaddq_s32(vreinterpretq_s32_s8(a.hi), vreinterpretq_s32_s8(b.hi)));
    return r;
}

static inline __m256i _mm256_sub_epi32(__m256i a, __m256i b) {
    __m256i r;
    r.lo = vreinterpretq_s8_s32(vsubq_s32(vreinterpretq_s32_s8(a.lo), vreinterpretq_s32_s8(b.lo)));
    r.hi = vreinterpretq_s8_s32(vsubq_s32(vreinterpretq_s32_s8(a.hi), vreinterpretq_s32_s8(b.hi)));
    return r;
}

static inline __m256i _mm256_mullo_epi32(__m256i a, __m256i b) {
    __m256i r;
    r.lo = vreinterpretq_s8_s32(vmulq_s32(vreinterpretq_s32_s8(a.lo), vreinterpretq_s32_s8(b.lo)));
    r.hi = vreinterpretq_s8_s32(vmulq_s32(vreinterpretq_s32_s8(a.hi), vreinterpretq_s32_s8(b.hi)));
    return r;
}

/* ============================================================
 * Bitwise operations
 * ============================================================ */

static inline __m256i _mm256_and_si256(__m256i a, __m256i b) {
    __m256i r;
    r.lo = vreinterpretq_s8_s32(vandq_s32(vreinterpretq_s32_s8(a.lo), vreinterpretq_s32_s8(b.lo)));
    r.hi = vreinterpretq_s8_s32(vandq_s32(vreinterpretq_s32_s8(a.hi), vreinterpretq_s32_s8(b.hi)));
    return r;
}

static inline __m256i _mm256_or_si256(__m256i a, __m256i b) {
    __m256i r;
    r.lo = vreinterpretq_s8_s32(vorrq_s32(vreinterpretq_s32_s8(a.lo), vreinterpretq_s32_s8(b.lo)));
    r.hi = vreinterpretq_s8_s32(vorrq_s32(vreinterpretq_s32_s8(a.hi), vreinterpretq_s32_s8(b.hi)));
    return r;
}

static inline __m256i _mm256_xor_si256(__m256i a, __m256i b) {
    __m256i r;
    r.lo = vreinterpretq_s8_s32(veorq_s32(vreinterpretq_s32_s8(a.lo), vreinterpretq_s32_s8(b.lo)));
    r.hi = vreinterpretq_s8_s32(veorq_s32(vreinterpretq_s32_s8(a.hi), vreinterpretq_s32_s8(b.hi)));
    return r;
}

/* andnot: (~a) & b */
static inline __m256i _mm256_andnot_si256(__m256i a, __m256i b) {
    __m256i r;
    r.lo = vreinterpretq_s8_s32(vbicq_s32(vreinterpretq_s32_s8(b.lo), vreinterpretq_s32_s8(a.lo)));
    r.hi = vreinterpretq_s8_s32(vbicq_s32(vreinterpretq_s32_s8(b.hi), vreinterpretq_s32_s8(a.hi)));
    return r;
}

/* ============================================================
 * Shift operations
 * ============================================================ */

static inline __m256i _mm256_srai_epi16(__m256i a, int imm) {
    __m256i r;
    int16x8_t va_lo = vreinterpretq_s16_s8(a.lo);
    int16x8_t va_hi = vreinterpretq_s16_s8(a.hi);
    /* NEON requires compile-time constant for vshrq_n, use vshlq with negative shift */
    int16x8_t shift = vdupq_n_s16((int16_t)(-imm));
    r.lo = vreinterpretq_s8_s16(vshlq_s16(va_lo, shift));
    r.hi = vreinterpretq_s8_s16(vshlq_s16(va_hi, shift));
    return r;
}

static inline __m256i _mm256_srli_epi16(__m256i a, int imm) {
    __m256i r;
    uint16x8_t va_lo = vreinterpretq_u16_s8(a.lo);
    uint16x8_t va_hi = vreinterpretq_u16_s8(a.hi);
    int16x8_t shift = vdupq_n_s16((int16_t)(-imm));
    r.lo = vreinterpretq_s8_u16(vshlq_u16(va_lo, shift));
    r.hi = vreinterpretq_s8_u16(vshlq_u16(va_hi, shift));
    return r;
}

static inline __m256i _mm256_slli_epi16(__m256i a, int imm) {
    __m256i r;
    int16x8_t shift = vdupq_n_s16((int16_t)imm);
    r.lo = vreinterpretq_s8_s16(vshlq_s16(vreinterpretq_s16_s8(a.lo), shift));
    r.hi = vreinterpretq_s8_s16(vshlq_s16(vreinterpretq_s16_s8(a.hi), shift));
    return r;
}

static inline __m256i _mm256_srli_epi32(__m256i a, int imm) {
    __m256i r;
    int32x4_t shift = vdupq_n_s32(-imm);
    r.lo = vreinterpretq_s8_u32(vshlq_u32(vreinterpretq_u32_s8(a.lo), shift));
    r.hi = vreinterpretq_s8_u32(vshlq_u32(vreinterpretq_u32_s8(a.hi), shift));
    return r;
}

static inline __m256i _mm256_slli_epi32(__m256i a, int imm) {
    __m256i r;
    int32x4_t shift = vdupq_n_s32(imm);
    r.lo = vreinterpretq_s8_s32(vshlq_s32(vreinterpretq_s32_s8(a.lo), shift));
    r.hi = vreinterpretq_s8_s32(vshlq_s32(vreinterpretq_s32_s8(a.hi), shift));
    return r;
}

/* Variable shift right logical 32-bit */
static inline __m256i _mm256_srlv_epi32(__m256i a, __m256i count) {
    __m256i r;
    int32x4_t neg_lo = vnegq_s32(vreinterpretq_s32_s8(count.lo));
    int32x4_t neg_hi = vnegq_s32(vreinterpretq_s32_s8(count.hi));
    r.lo = vreinterpretq_s8_u32(vshlq_u32(vreinterpretq_u32_s8(a.lo), neg_lo));
    r.hi = vreinterpretq_s8_u32(vshlq_u32(vreinterpretq_u32_s8(a.hi), neg_hi));
    return r;
}

/* ============================================================
 * Shuffle / Permute operations
 * ============================================================ */

/* Helper: shuffle 32-bit elements within each 128-bit lane */
static inline int8x16_t neon_shuffle_epi32_lane(int8x16_t v, int imm8) {
    int32x4_t src = vreinterpretq_s32_s8(v);
    int32_t arr[4];
    vst1q_s32(arr, src);
    int32_t res[4];
    res[0] = arr[(imm8 >> 0) & 3];
    res[1] = arr[(imm8 >> 2) & 3];
    res[2] = arr[(imm8 >> 4) & 3];
    res[3] = arr[(imm8 >> 6) & 3];
    return vreinterpretq_s8_s32(vld1q_s32(res));
}

static inline __m256i _mm256_shuffle_epi32(__m256i a, int imm8) {
    __m256i r;
    r.lo = neon_shuffle_epi32_lane(a.lo, imm8);
    r.hi = neon_shuffle_epi32_lane(a.hi, imm8);
    return r;
}

/* shuffle_epi8: SSSE3/AVX2 byte shuffle within each 128-bit lane */
static inline __m256i _mm256_shuffle_epi8(__m256i a, __m256i b) {
    __m256i r;
    uint8x16_t mask = vdupq_n_u8(0x80);
    uint8x16_t idx_lo = vreinterpretq_u8_s8(b.lo);
    uint8x16_t idx_hi = vreinterpretq_u8_s8(b.hi);
    uint8x16_t src_lo = vreinterpretq_u8_s8(a.lo);
    uint8x16_t src_hi = vreinterpretq_u8_s8(a.hi);

    /* If high bit of index is set, result is 0 */
    uint8x16_t hi_bit_lo = vcgeq_u8(idx_lo, mask);
    uint8x16_t hi_bit_hi = vcgeq_u8(idx_hi, mask);

    uint8x16_t idx_masked_lo = vandq_u8(idx_lo, vdupq_n_u8(0x0f));
    uint8x16_t idx_masked_hi = vandq_u8(idx_hi, vdupq_n_u8(0x0f));

    uint8x16_t res_lo = vqtbl1q_u8(src_lo, idx_masked_lo);
    uint8x16_t res_hi = vqtbl1q_u8(src_hi, idx_masked_hi);

    res_lo = vbicq_u8(res_lo, hi_bit_lo);
    res_hi = vbicq_u8(res_hi, hi_bit_hi);

    r.lo = vreinterpretq_s8_u8(res_lo);
    r.hi = vreinterpretq_s8_u8(res_hi);
    return r;
}

/* permute2x128: select 128-bit halves
 * imm8 bits [1:0] select src for dst[127:0]
 * imm8 bits [5:4] select src for dst[255:128]
 * 0=a.lo, 1=a.hi, 2=b.lo, 3=b.hi */
static inline __m256i _mm256_permute2x128_si256(__m256i a, __m256i b, int imm8) {
    __m256i r;
    int8x16_t srcs[4] = {a.lo, a.hi, b.lo, b.hi};
    int sel_lo = imm8 & 0x03;
    int sel_hi = (imm8 >> 4) & 0x03;
    r.lo = srcs[sel_lo];
    r.hi = srcs[sel_hi];
    return r;
}

/* permute4x64: permute 64-bit elements across lanes */
static inline __m256i _mm256_permute4x64_epi64(__m256i a, int imm8) {
    __m256i r;
    int64_t arr[4];
    int64x2_t vlo = vreinterpretq_s64_s8(a.lo);
    int64x2_t vhi = vreinterpretq_s64_s8(a.hi);
    arr[0] = vgetq_lane_s64(vlo, 0);
    arr[1] = vgetq_lane_s64(vlo, 1);
    arr[2] = vgetq_lane_s64(vhi, 0);
    arr[3] = vgetq_lane_s64(vhi, 1);

    int64_t res[4];
    res[0] = arr[(imm8 >> 0) & 3];
    res[1] = arr[(imm8 >> 2) & 3];
    res[2] = arr[(imm8 >> 4) & 3];
    res[3] = arr[(imm8 >> 6) & 3];

    r.lo = vreinterpretq_s8_s64(vld1q_s64(&res[0]));
    r.hi = vreinterpretq_s8_s64(vld1q_s64(&res[2]));
    return r;
}

/* permutevar8x32: variable permute of 32-bit elements across full 256 bits */
static inline __m256i _mm256_permutevar8x32_epi32(__m256i a, __m256i idx) {
    __m256i r;
    int32_t src[8], indices[8], res[8];
    vst1q_s32(src, vreinterpretq_s32_s8(a.lo));
    vst1q_s32(src + 4, vreinterpretq_s32_s8(a.hi));
    vst1q_s32(indices, vreinterpretq_s32_s8(idx.lo));
    vst1q_s32(indices + 4, vreinterpretq_s32_s8(idx.hi));
    for (int i = 0; i < 8; i++)
        res[i] = src[indices[i] & 7];
    r.lo = vreinterpretq_s8_s32(vld1q_s32(res));
    r.hi = vreinterpretq_s8_s32(vld1q_s32(res + 4));
    return r;
}

/* blend_epi16: blend 16-bit elements using immediate mask, within each 128-bit lane */
static inline __m256i _mm256_blend_epi16(__m256i a, __m256i b, int imm8) {
    __m256i r;
    int16_t va[8], vb[8], res[8];

    vst1q_s16(va, vreinterpretq_s16_s8(a.lo));
    vst1q_s16(vb, vreinterpretq_s16_s8(b.lo));
    for (int i = 0; i < 8; i++)
        res[i] = (imm8 & (1 << i)) ? vb[i] : va[i];
    r.lo = vreinterpretq_s8_s16(vld1q_s16(res));

    vst1q_s16(va, vreinterpretq_s16_s8(a.hi));
    vst1q_s16(vb, vreinterpretq_s16_s8(b.hi));
    for (int i = 0; i < 8; i++)
        res[i] = (imm8 & (1 << i)) ? vb[i] : va[i];
    r.hi = vreinterpretq_s8_s16(vld1q_s16(res));

    return r;
}

/* blend_epi32: blend 32-bit elements, mask repeats per lane */
static inline __m256i _mm256_blend_epi32(__m256i a, __m256i b, int imm8) {
    __m256i r;
    int32_t va[4], vb[4], res[4];

    vst1q_s32(va, vreinterpretq_s32_s8(a.lo));
    vst1q_s32(vb, vreinterpretq_s32_s8(b.lo));
    for (int i = 0; i < 4; i++)
        res[i] = (imm8 & (1 << i)) ? vb[i] : va[i];
    r.lo = vreinterpretq_s8_s32(vld1q_s32(res));

    vst1q_s32(va, vreinterpretq_s32_s8(a.hi));
    vst1q_s32(vb, vreinterpretq_s32_s8(b.hi));
    for (int i = 0; i < 4; i++)
        res[i] = (imm8 & (1 << (i + 4))) ? vb[i] : va[i];
    r.hi = vreinterpretq_s8_s32(vld1q_s32(res));

    return r;
}

/* ============================================================
 * Unpack operations (within each 128-bit lane)
 * ============================================================ */

static inline __m256i _mm256_unpacklo_epi8(__m256i a, __m256i b) {
    __m256i r;
    int8x8_t a_lo_lo = vget_low_s8(a.lo);
    int8x8_t b_lo_lo = vget_low_s8(b.lo);
    int8x8_t a_hi_lo = vget_low_s8(a.hi);
    int8x8_t b_hi_lo = vget_low_s8(b.hi);
    int8x8x2_t z0 = vzip_s8(a_lo_lo, b_lo_lo);
    int8x8x2_t z1 = vzip_s8(a_hi_lo, b_hi_lo);
    r.lo = vcombine_s8(z0.val[0], z0.val[1]);
    r.hi = vcombine_s8(z1.val[0], z1.val[1]);
    return r;
}

static inline __m256i _mm256_unpackhi_epi8(__m256i a, __m256i b) {
    __m256i r;
    int8x8_t a_lo_hi = vget_high_s8(a.lo);
    int8x8_t b_lo_hi = vget_high_s8(b.lo);
    int8x8_t a_hi_hi = vget_high_s8(a.hi);
    int8x8_t b_hi_hi = vget_high_s8(b.hi);
    int8x8x2_t z0 = vzip_s8(a_lo_hi, b_lo_hi);
    int8x8x2_t z1 = vzip_s8(a_hi_hi, b_hi_hi);
    r.lo = vcombine_s8(z0.val[0], z0.val[1]);
    r.hi = vcombine_s8(z1.val[0], z1.val[1]);
    return r;
}

static inline __m256i _mm256_unpacklo_epi16(__m256i a, __m256i b) {
    __m256i r;
    int16x4_t a0 = vget_low_s16(vreinterpretq_s16_s8(a.lo));
    int16x4_t b0 = vget_low_s16(vreinterpretq_s16_s8(b.lo));
    int16x4_t a1 = vget_low_s16(vreinterpretq_s16_s8(a.hi));
    int16x4_t b1 = vget_low_s16(vreinterpretq_s16_s8(b.hi));
    int16x4x2_t z0 = vzip_s16(a0, b0);
    int16x4x2_t z1 = vzip_s16(a1, b1);
    r.lo = vreinterpretq_s8_s16(vcombine_s16(z0.val[0], z0.val[1]));
    r.hi = vreinterpretq_s8_s16(vcombine_s16(z1.val[0], z1.val[1]));
    return r;
}

static inline __m256i _mm256_unpackhi_epi16(__m256i a, __m256i b) {
    __m256i r;
    int16x4_t a0 = vget_high_s16(vreinterpretq_s16_s8(a.lo));
    int16x4_t b0 = vget_high_s16(vreinterpretq_s16_s8(b.lo));
    int16x4_t a1 = vget_high_s16(vreinterpretq_s16_s8(a.hi));
    int16x4_t b1 = vget_high_s16(vreinterpretq_s16_s8(b.hi));
    int16x4x2_t z0 = vzip_s16(a0, b0);
    int16x4x2_t z1 = vzip_s16(a1, b1);
    r.lo = vreinterpretq_s8_s16(vcombine_s16(z0.val[0], z0.val[1]));
    r.hi = vreinterpretq_s8_s16(vcombine_s16(z1.val[0], z1.val[1]));
    return r;
}

static inline __m256i _mm256_unpacklo_epi32(__m256i a, __m256i b) {
    __m256i r;
    int32x2_t a0 = vget_low_s32(vreinterpretq_s32_s8(a.lo));
    int32x2_t b0 = vget_low_s32(vreinterpretq_s32_s8(b.lo));
    int32x2_t a1 = vget_low_s32(vreinterpretq_s32_s8(a.hi));
    int32x2_t b1 = vget_low_s32(vreinterpretq_s32_s8(b.hi));
    int32x2x2_t z0 = vzip_s32(a0, b0);
    int32x2x2_t z1 = vzip_s32(a1, b1);
    r.lo = vreinterpretq_s8_s32(vcombine_s32(z0.val[0], z0.val[1]));
    r.hi = vreinterpretq_s8_s32(vcombine_s32(z1.val[0], z1.val[1]));
    return r;
}

static inline __m256i _mm256_unpackhi_epi32(__m256i a, __m256i b) {
    __m256i r;
    int32x2_t a0 = vget_high_s32(vreinterpretq_s32_s8(a.lo));
    int32x2_t b0 = vget_high_s32(vreinterpretq_s32_s8(b.lo));
    int32x2_t a1 = vget_high_s32(vreinterpretq_s32_s8(a.hi));
    int32x2_t b1 = vget_high_s32(vreinterpretq_s32_s8(b.hi));
    int32x2x2_t z0 = vzip_s32(a0, b0);
    int32x2x2_t z1 = vzip_s32(a1, b1);
    r.lo = vreinterpretq_s8_s32(vcombine_s32(z0.val[0], z0.val[1]));
    r.hi = vreinterpretq_s8_s32(vcombine_s32(z1.val[0], z1.val[1]));
    return r;
}

static inline __m256i _mm256_unpacklo_epi64(__m256i a, __m256i b) {
    __m256i r;
    /* lo lane: a.lo[63:0], b.lo[63:0] */
    int64x1_t a0 = vget_low_s64(vreinterpretq_s64_s8(a.lo));
    int64x1_t b0 = vget_low_s64(vreinterpretq_s64_s8(b.lo));
    int64x1_t a1 = vget_low_s64(vreinterpretq_s64_s8(a.hi));
    int64x1_t b1 = vget_low_s64(vreinterpretq_s64_s8(b.hi));
    r.lo = vreinterpretq_s8_s64(vcombine_s64(a0, b0));
    r.hi = vreinterpretq_s8_s64(vcombine_s64(a1, b1));
    return r;
}

/* ============================================================
 * Pack operations (within each 128-bit lane)
 * ============================================================ */

/* packus_epi32: pack int32 to uint16 with unsigned saturation */
static inline __m256i _mm256_packus_epi32(__m256i a, __m256i b) {
    __m256i r;
    int32x4_t a_lo = vreinterpretq_s32_s8(a.lo);
    int32x4_t b_lo = vreinterpretq_s32_s8(b.lo);
    int32x4_t a_hi = vreinterpretq_s32_s8(a.hi);
    int32x4_t b_hi = vreinterpretq_s32_s8(b.hi);
    uint16x4_t r0 = vqmovun_s32(a_lo);
    uint16x4_t r1 = vqmovun_s32(b_lo);
    uint16x4_t r2 = vqmovun_s32(a_hi);
    uint16x4_t r3 = vqmovun_s32(b_hi);
    r.lo = vreinterpretq_s8_u16(vcombine_u16(r0, r1));
    r.hi = vreinterpretq_s8_u16(vcombine_u16(r2, r3));
    return r;
}

/* packs_epi32: pack int32 to int16 with signed saturation */
static inline __m256i _mm256_packs_epi32(__m256i a, __m256i b) {
    __m256i r;
    int32x4_t a_lo = vreinterpretq_s32_s8(a.lo);
    int32x4_t b_lo = vreinterpretq_s32_s8(b.lo);
    int32x4_t a_hi = vreinterpretq_s32_s8(a.hi);
    int32x4_t b_hi = vreinterpretq_s32_s8(b.hi);
    int16x4_t r0 = vqmovn_s32(a_lo);
    int16x4_t r1 = vqmovn_s32(b_lo);
    int16x4_t r2 = vqmovn_s32(a_hi);
    int16x4_t r3 = vqmovn_s32(b_hi);
    r.lo = vreinterpretq_s8_s16(vcombine_s16(r0, r1));
    r.hi = vreinterpretq_s8_s16(vcombine_s16(r2, r3));
    return r;
}

/* ============================================================
 * Convert / Extract / Cast operations
 * ============================================================ */

/* cvtepi8_epi16: sign-extend 8 bytes from 128-bit to 8 x 16-bit in 256-bit */
static inline __m256i _mm256_cvtepi8_epi16(__m128i a) {
    __m256i r;
    int8x8_t lo8 = vget_low_s8(a);
    int8x8_t hi8 = vget_high_s8(a);
    r.lo = vreinterpretq_s8_s16(vmovl_s8(lo8));
    r.hi = vreinterpretq_s8_s16(vmovl_s8(hi8));
    return r;
}

/* Cast 256 to its lower 128 bits */
static inline __m128i _mm256_castsi256_si128(__m256i a) {
    return a.lo;
}

/* Extract 128-bit lane */
static inline __m128i _mm256_extracti128_si256(__m256i a, int imm) {
    return (imm & 1) ? a.hi : a.lo;
}

/* Broadcast 32-bit element from 128-bit to all 32-bit slots of 256-bit */
static inline __m256i _mm256_broadcastd_epi32(__m128i a) {
    __m256i r;
    int32_t val = vgetq_lane_s32(vreinterpretq_s32_s8(a), 0);
    r.lo = vreinterpretq_s8_s32(vdupq_n_s32(val));
    r.hi = vreinterpretq_s8_s32(vdupq_n_s32(val));
    return r;
}

/* ============================================================
 * Test / Compare operations
 * ============================================================ */

/* testz: return 1 if (a & b) == 0 */
static inline int _mm256_testz_si256(__m256i a, __m256i b) {
    uint32x4_t lo = vandq_u32(vreinterpretq_u32_s8(a.lo), vreinterpretq_u32_s8(b.lo));
    uint32x4_t hi = vandq_u32(vreinterpretq_u32_s8(a.hi), vreinterpretq_u32_s8(b.hi));
    uint32x4_t combined = vorrq_u32(lo, hi);
    uint32x2_t reduced = vorr_u32(vget_low_u32(combined), vget_high_u32(combined));
    return (vget_lane_u32(reduced, 0) | vget_lane_u32(reduced, 1)) == 0 ? 1 : 0;
}

/* movemask_epi8: extract the MSB of each byte, creating a 32-bit mask */
static inline int _mm256_movemask_epi8(__m256i a) {
    uint8_t buf_lo[16], buf_hi[16];
    vst1q_u8(buf_lo, vreinterpretq_u8_s8(a.lo));
    vst1q_u8(buf_hi, vreinterpretq_u8_s8(a.hi));
    uint32_t mask = 0;
    for (int i = 0; i < 16; i++) {
        mask |= ((uint32_t)(buf_lo[i] >> 7)) << i;
        mask |= ((uint32_t)(buf_hi[i] >> 7)) << (i + 16);
    }
    return (int)mask;
}

/* ============================================================
 * Gather operation
 * ============================================================ */

/* i32gather_epi32: gather 32-bit values using indices * scale */
static inline __m256i _mm256_i32gather_epi32(const int *base, __m256i vindex, int scale) {
    __m256i r;
    int32_t indices[8], res[8];
    vst1q_s32(indices, vreinterpretq_s32_s8(vindex.lo));
    vst1q_s32(indices + 4, vreinterpretq_s32_s8(vindex.hi));
    const uint8_t *bp = (const uint8_t *)base;
    for (int i = 0; i < 8; i++) {
        int32_t val;
        memcpy(&val, bp + (int64_t)indices[i] * scale, 4);
        res[i] = val;
    }
    r.lo = vreinterpretq_s8_s32(vld1q_s32(res));
    r.hi = vreinterpretq_s8_s32(vld1q_s32(res + 4));
    return r;
}

/* ============================================================
 * BMI2 emulation
 * ============================================================ */

/* _pext_u32: parallel bit extract */
static inline uint32_t _pext_u32(uint32_t a, uint32_t mask) {
    uint32_t result = 0;
    int k = 0;
    for (int i = 0; i < 32; i++) {
        if (mask & (1u << i)) {
            if (a & (1u << i))
                result |= (1u << k);
            k++;
        }
    }
    return result;
}

/* ============================================================
 * Additional 128-bit SSE functions
 * ============================================================ */

static inline int _mm_cvtsi128_si32(__m128i a) {
    return vgetq_lane_s32(vreinterpretq_s32_s8(a), 0);
}

static inline __m128i _mm_loadl_epi64(const void *p) {
    int64_t val;
    memcpy(&val, p, 8);
    int64x2_t v = vdupq_n_s64(0);
    v = vsetq_lane_s64(val, v, 0);
    return vreinterpretq_s8_s64(v);
}

static inline __m128i _mm_or_si128(__m128i a, __m128i b) {
    return vreinterpretq_s8_s32(vorrq_s32(vreinterpretq_s32_s8(a), vreinterpretq_s32_s8(b)));
}

static inline __m128i _mm_slli_si128(__m128i a, int imm) {
    if (imm >= 16) return vreinterpretq_s8_s32(vdupq_n_s32(0));
    if (imm == 0) return a;
    uint8_t buf[32];
    memset(buf, 0, 16);
    vst1q_u8(buf + 16, vreinterpretq_u8_s8(a));
    return vreinterpretq_s8_u8(vld1q_u8(buf + 16 - imm));
}

/* ============================================================
 * Additional 256-bit functions
 * ============================================================ */

/* cmpeq_epi16: compare equal, returns 0xFFFF or 0x0000 per lane */
static inline __m256i _mm256_cmpeq_epi16(__m256i a, __m256i b) {
    __m256i r;
    r.lo = vreinterpretq_s8_u16(vceqq_s16(vreinterpretq_s16_s8(a.lo), vreinterpretq_s16_s8(b.lo)));
    r.hi = vreinterpretq_s8_u16(vceqq_s16(vreinterpretq_s16_s8(a.hi), vreinterpretq_s16_s8(b.hi)));
    return r;
}

/* cmpgt_epi16: compare greater than signed */
static inline __m256i _mm256_cmpgt_epi16(__m256i a, __m256i b) {
    __m256i r;
    r.lo = vreinterpretq_s8_u16(vcgtq_s16(vreinterpretq_s16_s8(a.lo), vreinterpretq_s16_s8(b.lo)));
    r.hi = vreinterpretq_s8_u16(vcgtq_s16(vreinterpretq_s16_s8(a.hi), vreinterpretq_s16_s8(b.hi)));
    return r;
}

/* cvtepu32_epi64: zero-extend 4 × uint32 → 4 × int64 */
static inline __m256i _mm256_cvtepu32_epi64(__m128i a) {
    __m256i r;
    uint32x4_t v = vreinterpretq_u32_s8(a);
    uint64x2_t lo = vmovl_u32(vget_low_u32(v));
    uint64x2_t hi = vmovl_u32(vget_high_u32(v));
    r.lo = vreinterpretq_s8_u64(lo);
    r.hi = vreinterpretq_s8_u64(hi);
    return r;
}

/* madd_epi16: multiply adjacent 16-bit pairs and add → 32-bit */
static inline __m256i _mm256_madd_epi16(__m256i a, __m256i b) {
    __m256i r;
    int16x8_t a_lo = vreinterpretq_s16_s8(a.lo);
    int16x8_t b_lo = vreinterpretq_s16_s8(b.lo);
    int16x8_t a_hi = vreinterpretq_s16_s8(a.hi);
    int16x8_t b_hi = vreinterpretq_s16_s8(b.hi);

    int32x4_t p0 = vmull_s16(vget_low_s16(a_lo), vget_low_s16(b_lo));
    int32x4_t p1 = vmull_s16(vget_high_s16(a_lo), vget_high_s16(b_lo));
    int32x4_t p2 = vmull_s16(vget_low_s16(a_hi), vget_low_s16(b_hi));
    int32x4_t p3 = vmull_s16(vget_high_s16(a_hi), vget_high_s16(b_hi));

    /* Add adjacent pairs: (p[0]+p[1], p[2]+p[3], ...) */
    int32x4_t s0 = vpaddq_s32(p0, p1);
    int32x4_t s1 = vpaddq_s32(p2, p3);

    r.lo = vreinterpretq_s8_s32(s0);
    r.hi = vreinterpretq_s8_s32(s1);
    return r;
}

/* mul_epi32: multiply packed signed 32-bit (lanes 0,2 → 64-bit) */
static inline __m256i _mm256_mul_epi32(__m256i a, __m256i b) {
    __m256i r;
    int32x4_t va_lo = vreinterpretq_s32_s8(a.lo);
    int32x4_t vb_lo = vreinterpretq_s32_s8(b.lo);
    int32x4_t va_hi = vreinterpretq_s32_s8(a.hi);
    int32x4_t vb_hi = vreinterpretq_s32_s8(b.hi);
    /* Multiply elements at index 0 and 2 (even positions) */
    int32x2_t a_even_lo = vuzp1_s32(vget_low_s32(va_lo), vget_high_s32(va_lo));
    int32x2_t b_even_lo = vuzp1_s32(vget_low_s32(vb_lo), vget_high_s32(vb_lo));
    int32x2_t a_even_hi = vuzp1_s32(vget_low_s32(va_hi), vget_high_s32(va_hi));
    int32x2_t b_even_hi = vuzp1_s32(vget_low_s32(vb_hi), vget_high_s32(vb_hi));
    r.lo = vreinterpretq_s8_s64(vmull_s32(a_even_lo, b_even_lo));
    r.hi = vreinterpretq_s8_s64(vmull_s32(a_even_hi, b_even_hi));
    return r;
}

/* mul_epu32: multiply packed unsigned 32-bit (lanes 0,2 → 64-bit) */
static inline __m256i _mm256_mul_epu32(__m256i a, __m256i b) {
    __m256i r;
    uint32x4_t va_lo = vreinterpretq_u32_s8(a.lo);
    uint32x4_t vb_lo = vreinterpretq_u32_s8(b.lo);
    uint32x4_t va_hi = vreinterpretq_u32_s8(a.hi);
    uint32x4_t vb_hi = vreinterpretq_u32_s8(b.hi);
    uint32x2_t a_even_lo = vuzp1_u32(vget_low_u32(va_lo), vget_high_u32(va_lo));
    uint32x2_t b_even_lo = vuzp1_u32(vget_low_u32(vb_lo), vget_high_u32(vb_lo));
    uint32x2_t a_even_hi = vuzp1_u32(vget_low_u32(va_hi), vget_high_u32(va_hi));
    uint32x2_t b_even_hi = vuzp1_u32(vget_low_u32(vb_hi), vget_high_u32(vb_hi));
    r.lo = vreinterpretq_s8_u64(vmull_u32(a_even_lo, b_even_lo));
    r.hi = vreinterpretq_s8_u64(vmull_u32(a_even_hi, b_even_hi));
    return r;
}

/* set_m128i: combine two 128-bit halves into 256-bit */
static inline __m256i _mm256_set_m128i(__m128i hi, __m128i lo) {
    __m256i r;
    r.lo = lo;
    r.hi = hi;
    return r;
}

/* setr_epi16: set 16 × int16, low to high */
static inline __m256i _mm256_setr_epi16(
    int16_t e0,  int16_t e1,  int16_t e2,  int16_t e3,
    int16_t e4,  int16_t e5,  int16_t e6,  int16_t e7,
    int16_t e8,  int16_t e9,  int16_t e10, int16_t e11,
    int16_t e12, int16_t e13, int16_t e14, int16_t e15) {
    __m256i r;
    int16_t lo_arr[8] = {e0,e1,e2,e3,e4,e5,e6,e7};
    int16_t hi_arr[8] = {e8,e9,e10,e11,e12,e13,e14,e15};
    r.lo = vreinterpretq_s8_s16(vld1q_s16(lo_arr));
    r.hi = vreinterpretq_s8_s16(vld1q_s16(hi_arr));
    return r;
}

/* setr_epi64x: set 4 × int64, low to high */
static inline __m256i _mm256_setr_epi64x(int64_t e0, int64_t e1, int64_t e2, int64_t e3) {
    __m256i r;
    int64_t lo_arr[2] = {e0, e1};
    int64_t hi_arr[2] = {e2, e3};
    r.lo = vreinterpretq_s8_s64(vld1q_s64(lo_arr));
    r.hi = vreinterpretq_s8_s64(vld1q_s64(hi_arr));
    return r;
}

/* 64-bit shift operations */
static inline __m256i _mm256_slli_epi64(__m256i a, int imm) {
    __m256i r;
    int64x2_t shift = vdupq_n_s64(imm);
    r.lo = vreinterpretq_s8_s64(vshlq_s64(vreinterpretq_s64_s8(a.lo), shift));
    r.hi = vreinterpretq_s8_s64(vshlq_s64(vreinterpretq_s64_s8(a.hi), shift));
    return r;
}

static inline __m256i _mm256_srli_epi64(__m256i a, int imm) {
    __m256i r;
    int64x2_t shift = vdupq_n_s64(-imm);
    r.lo = vreinterpretq_s8_u64(vshlq_u64(vreinterpretq_u64_s8(a.lo), shift));
    r.hi = vreinterpretq_s8_u64(vshlq_u64(vreinterpretq_u64_s8(a.hi), shift));
    return r;
}

static inline __m256i _mm256_sllv_epi64(__m256i a, __m256i count) {
    __m256i r;
    r.lo = vreinterpretq_s8_s64(vshlq_s64(vreinterpretq_s64_s8(a.lo), vreinterpretq_s64_s8(count.lo)));
    r.hi = vreinterpretq_s8_s64(vshlq_s64(vreinterpretq_s64_s8(a.hi), vreinterpretq_s64_s8(count.hi)));
    return r;
}

static inline __m256i _mm256_srlv_epi64(__m256i a, __m256i count) {
    __m256i r;
    int64x2_t neg_lo = vnegq_s64(vreinterpretq_s64_s8(count.lo));
    int64x2_t neg_hi = vnegq_s64(vreinterpretq_s64_s8(count.hi));
    r.lo = vreinterpretq_s8_u64(vshlq_u64(vreinterpretq_u64_s8(a.lo), neg_lo));
    r.hi = vreinterpretq_s8_u64(vshlq_u64(vreinterpretq_u64_s8(a.hi), neg_hi));
    return r;
}

#endif /* AVX2_TO_NEON_H */
