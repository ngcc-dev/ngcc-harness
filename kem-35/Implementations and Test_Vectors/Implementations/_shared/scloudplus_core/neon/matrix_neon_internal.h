/**
 * @file matrix_neon_internal.h
 * @brief Shared NEON matrix helpers for Scloud+.
 *
 * This header contains only backend-local helper routines used by the NEON
 * matrix modules.  Each build selects exactly one NEON matrix module, so the
 * non-static mat_add/mat_sub definitions appear in only one translation unit.
 */
#ifndef SCLOUDPLUS_AARCH64_MATRIX_NEON_INTERNAL_H
#define SCLOUDPLUS_AARCH64_MATRIX_NEON_INTERNAL_H

#include "matrix.h"
#include "modarith.h"
#include "scloudplus_param_common.h"
#include <arm_neon.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>

/* ---- alignment macros ---- */
#if defined(_WIN32) || defined(_WIN64)
#define ALIGN_HEADER(N) __declspec(aligned(N))
#define ALIGN_FOOTER(N)
#else
#define ALIGN_HEADER(N)
#define ALIGN_FOOTER(N) __attribute__((aligned(N)))
#endif

#define NEON_LANES_U16 8U

/* ---- mul16 (common scalar helper) ---- */
static inline uint16_t mul16(uint16_t a, uint16_t b)
{
    return (uint16_t)((uint32_t)a * (uint32_t)b);
}

/* ---- neon_mask_q — modulo-q mask via NEON ---- */
static inline uint16x8_t neon_mask_q(uint16x8_t value)
{
    return vandq_u16(value, vdupq_n_u16((uint16_t)scloudplus_q_mask));
}

/* Sum eight uint16 lanes modulo 2^16.  Matrix products in this backend keep
 * the natural uint16 wraparound and apply the q mask only at pack/reduce
 * boundaries, matching the scalar implementation. */
static inline uint16_t neon_hadd_u16_mod_2p16(uint16x8_t value)
{
    uint16x4_t sum4 = vadd_u16(vget_low_u16(value), vget_high_u16(value));
    sum4 = vpadd_u16(sum4, sum4);
    sum4 = vpadd_u16(sum4, sum4);
    return vget_lane_u16(sum4, 0);
}

/* ---- neon_mul_add_rows8_u16 — 8-row × 1-col MAD kernel ---- */
static inline void neon_mul_add_rows8_u16(uint16_t *dst, const uint16_t *rows,
                                          const uint16_t coeffs[8])
{
    size_t q = 0U;
    for (; q + NEON_LANES_U16 <= (size_t)scloudplus_n; q += NEON_LANES_U16) {
        uint16x8_t sum = vld1q_u16(dst + q);
        sum = vaddq_u16(sum, vmulq_u16(vdupq_n_u16(coeffs[0]), vld1q_u16(rows + 0U * (size_t)scloudplus_n + q)));
        sum = vaddq_u16(sum, vmulq_u16(vdupq_n_u16(coeffs[1]), vld1q_u16(rows + 1U * (size_t)scloudplus_n + q)));
        sum = vaddq_u16(sum, vmulq_u16(vdupq_n_u16(coeffs[2]), vld1q_u16(rows + 2U * (size_t)scloudplus_n + q)));
        sum = vaddq_u16(sum, vmulq_u16(vdupq_n_u16(coeffs[3]), vld1q_u16(rows + 3U * (size_t)scloudplus_n + q)));
        sum = vaddq_u16(sum, vmulq_u16(vdupq_n_u16(coeffs[4]), vld1q_u16(rows + 4U * (size_t)scloudplus_n + q)));
        sum = vaddq_u16(sum, vmulq_u16(vdupq_n_u16(coeffs[5]), vld1q_u16(rows + 5U * (size_t)scloudplus_n + q)));
        sum = vaddq_u16(sum, vmulq_u16(vdupq_n_u16(coeffs[6]), vld1q_u16(rows + 6U * (size_t)scloudplus_n + q)));
        sum = vaddq_u16(sum, vmulq_u16(vdupq_n_u16(coeffs[7]), vld1q_u16(rows + 7U * (size_t)scloudplus_n + q)));
        vst1q_u16(dst + q, sum);
    }
    for (; q < (size_t)scloudplus_n; q++) {
        uint16_t acc = dst[q];
        for (size_t row = 0U; row < 8U; row++)
            acc = (uint16_t)(acc + coeffs[row] * rows[row * (size_t)scloudplus_n + q]);
        dst[q] = acc;
    }
}

/* ---- neon_mul_add_rows8_2x_u16 — 8-row × 2-col MAD kernel ---- */
static inline void neon_mul_add_rows8_2x_u16(uint16_t *dst0, uint16_t *dst1,
                                             const uint16_t *rows,
                                             const uint16_t coeffs0[8],
                                             const uint16_t coeffs1[8])
{
    size_t q = 0U;
    for (; q + 2U * NEON_LANES_U16 <= (size_t)scloudplus_n;
         q += 2U * NEON_LANES_U16) {
        uint16x8_t sum00 = vld1q_u16(dst0 + q);
        uint16x8_t sum01 = vld1q_u16(dst0 + q + NEON_LANES_U16);
        uint16x8_t sum10 = vld1q_u16(dst1 + q);
        uint16x8_t sum11 = vld1q_u16(dst1 + q + NEON_LANES_U16);

        for (size_t row = 0U; row < 8U; row++) {
            const uint16_t *rowp = rows + row * (size_t)scloudplus_n + q;
            const uint16x8_t a0 = vld1q_u16(rowp);
            const uint16x8_t a1 = vld1q_u16(rowp + NEON_LANES_U16);
            const uint16x8_t c0 = vdupq_n_u16(coeffs0[row]);
            const uint16x8_t c1 = vdupq_n_u16(coeffs1[row]);
            sum00 = vaddq_u16(sum00, vmulq_u16(a0, c0));
            sum01 = vaddq_u16(sum01, vmulq_u16(a1, c0));
            sum10 = vaddq_u16(sum10, vmulq_u16(a0, c1));
            sum11 = vaddq_u16(sum11, vmulq_u16(a1, c1));
        }
        vst1q_u16(dst0 + q, sum00);
        vst1q_u16(dst0 + q + NEON_LANES_U16, sum01);
        vst1q_u16(dst1 + q, sum10);
        vst1q_u16(dst1 + q + NEON_LANES_U16, sum11);
    }
    for (; q + NEON_LANES_U16 <= (size_t)scloudplus_n; q += NEON_LANES_U16) {
        uint16x8_t sum0 = vld1q_u16(dst0 + q);
        uint16x8_t sum1 = vld1q_u16(dst1 + q);
        for (size_t row = 0U; row < 8U; row++) {
            const uint16x8_t a = vld1q_u16(rows + row * (size_t)scloudplus_n + q);
            sum0 = vaddq_u16(sum0, vmulq_u16(a, vdupq_n_u16(coeffs0[row])));
            sum1 = vaddq_u16(sum1, vmulq_u16(a, vdupq_n_u16(coeffs1[row])));
        }
        vst1q_u16(dst0 + q, sum0);
        vst1q_u16(dst1 + q, sum1);
    }
    for (; q < (size_t)scloudplus_n; q++) {
        uint16_t acc0 = dst0[q];
        uint16_t acc1 = dst1[q];
        for (size_t row = 0U; row < 8U; row++) {
            const uint16_t a = rows[row * (size_t)scloudplus_n + q];
            acc0 = (uint16_t)(acc0 + coeffs0[row] * a);
            acc1 = (uint16_t)(acc1 + coeffs1[row] * a);
        }
        dst0[q] = acc0;
        dst1[q] = acc1;
    }
}

/* ---- unpack10 helpers (NEON vtbl version — used by AES/SHAKE/SM3 packed10) ---- */
static inline uint16x8_t unpack10_x8_u16(const uint8_t *in)
{
    static const uint8_t gather_data[16] = {
        0, 1, 1, 2, 2, 3, 3, 4, 5, 6, 6, 7, 7, 8, 8, 9};
    static const int16_t shift_data[8] = {
        0, -2, -4, -6, 0, -2, -4, -6};
    const uint8x16_t bytes = vld1q_u8(in);
    const uint8x16_t gather = vld1q_u8(gather_data);
    const uint16x8_t windows = vreinterpretq_u16_u8(vqtbl1q_u8(bytes, gather));
    const uint16x8_t mask10 = vdupq_n_u16(0x03FF);

    return vandq_u16(vshlq_u16(windows, vld1q_s16(shift_data)), mask10);
}

static inline void unpack10_x16(const uint8_t *in, uint16_t *out)
{
    vst1q_u16(out, unpack10_x8_u16(in));
    vst1q_u16(out + 8U, unpack10_x8_u16(in + 10U));
}

/* ---- unpack10 helpers (portable fallback used by packed10 row decoding) ---- */
static inline void unpack10_x4(const uint8_t *in, uint16_t *out)
{
    out[0] = (uint16_t)(in[0] | ((uint16_t)(in[1] & 0x03U) << 8U));
    out[1] = (uint16_t)((in[1] >> 2U) | ((uint16_t)(in[2] & 0x0FU) << 6U));
    out[2] = (uint16_t)((in[2] >> 4U) | ((uint16_t)(in[3] & 0x3FU) << 4U));
    out[3] = (uint16_t)((in[3] >> 6U) | ((uint16_t)in[4] << 2U));
}

static inline void unpack10_x16_s(const uint8_t *in, uint16_t *out)
{
    unpack10_x4(in + 0U, out + 0U);
    unpack10_x4(in + 5U, out + 4U);
    unpack10_x4(in + 10U, out + 8U);
    unpack10_x4(in + 15U, out + 12U);
}

/* ---- set_row_input_counter (used by XOF-based SHAKE/SM3 families) ---- */
#if defined(SCLOUDPLUS_NEON_FAMILY_SHAKE) || \
    defined(SCLOUDPLUS_NEON_FAMILY_SM3)
#define A_INPUT_BYTES (scloudplus_seedA_bytes + 4U)

static inline void set_row_input_counter(uint8_t in[A_INPUT_BYTES], uint32_t row)
{
    in[scloudplus_seedA_bytes + 0U] = (uint8_t)(row >> 0U);
    in[scloudplus_seedA_bytes + 1U] = (uint8_t)(row >> 8U);
    in[scloudplus_seedA_bytes + 2U] = (uint8_t)(row >> 16U);
    in[scloudplus_seedA_bytes + 3U] = (uint8_t)(row >> 24U);
}
#endif

/* ---- mat_add / mat_sub
 *
 * These helpers are shared by all NEON families.  They use the public modulus
 * q = 2^10 and do not depend on a specific security level or primitive.
 */
void mat_add(uint16_t *lhs, uint16_t *rhs, int len, uint16_t *out)
{
    int i = 0;
    for (; i + (int)NEON_LANES_U16 <= len; i += (int)NEON_LANES_U16) {
        const uint16x8_t l = vld1q_u16(lhs + i);
        const uint16x8_t r = vld1q_u16(rhs + i);
        vst1q_u16(out + i, neon_mask_q(vaddq_u16(l, r)));
    }
    for (; i < len; i++)
        out[i] = scloudplus_mod_q((uint32_t)lhs[i] + (uint32_t)rhs[i]);
}

void mat_sub(uint16_t *lhs, uint16_t *rhs, int len, uint16_t *out)
{
    int i = 0;
    for (; i + (int)NEON_LANES_U16 <= len; i += (int)NEON_LANES_U16) {
        const uint16x8_t l = vld1q_u16(lhs + i);
        const uint16x8_t r = vld1q_u16(rhs + i);
        vst1q_u16(out + i, neon_mask_q(vsubq_u16(l, r)));
    }
    for (; i < len; i++)
        out[i] = scloudplus_mod_q((uint32_t)lhs[i] - (uint32_t)rhs[i]);
}

#endif /* SCLOUDPLUS_AARCH64_MATRIX_COMMON_H */
