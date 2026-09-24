/*
 * AArch64 NEON parameters and helpers for DKE.
 * Only active when DKE_USE_AARCH64 is defined.
 */
#ifndef DKE_NEON_PARAMS_H
#define DKE_NEON_PARAMS_H

#include "../parameters.h"

#if defined(DKE_USE_AARCH64)

/* DKE_NEON_OPT: enable NEON-specific optimizations for AArch64.
 * Undefine to revert to the original (less optimized) NEON code paths. */
#define DKE_NEON_OPT

#include <arm_neon.h>
#include <stdint.h>

/*
 * Montgomery reduction: given a = x*y (32-bit), compute
 *   t = (int16_t)(a) * QINV
 *   result = (a - (int32_t)t * Q) >> 16
 *
 * NEON vectorized: operates on 8 x int16 pairs (lo, hi).
 *   lo = vmullo(x, y), hi = vmulhi(x, y)  [conceptually]
 *   t = vmul(lo, qinv_vec)
 *   t = vqdmulh(t, q_vec)  -- approximate mulhi via saturating doubling mulhi
 *   result = vsub(hi, t)
 *
 * Actually for NEON we use a different approach:
 *   We have a*b as pairs (lo16, hi16) where a*b = hi16<<16 + lo16.
 *   t = lo16 * QINV (mod 2^16)  = vmul_s16(lo, qinv)
 *   t_hi = mulhi(t, Q)           = vqdmulh then adjust
 *   result = hi - t_hi
 *
 * NEON has vqdmulh which computes (a*b*2) >> 16 (saturating).
 * So mulhi(a,b) ≈ (vqdmulh(a,b) + 1) >> 1, but it's easier to
 * use the ML-KEM NEON approach: use vmull to get 32-bit products.
 */

/* Barrett constant: v = round(2^26 / Q) */
#if DKE_Q == 3329
#define DKE_BARRETT_V  20159
#elif DKE_Q == 7681
#define DKE_BARRETT_V  8737
#endif

/* Montgomery multiply: r = montgomery_reduce(a * b)
 * Using NEON vmull_s16 → 32-bit, then reduce.
 * Processes 8 coefficients at a time (int16x8_t). */
static inline int16x8_t neon_fqmul(int16x8_t a, int16x8_t b,
                                     int16x4_t qinv_lo, int16x4_t q_lo) {
    /* Split into low/high halves for vmull (4-wide → 4x32) */
    int32x4_t prod_lo = vmull_s16(vget_low_s16(a), vget_low_s16(b));
    int32x4_t prod_hi = vmull_s16(vget_high_s16(a), vget_high_s16(b));

    /* t = (int16)(prod) * QINV */
    int16x4_t t_lo = vmul_s16(vmovn_s32(prod_lo), qinv_lo);
    int16x4_t t_hi = vmul_s16(vmovn_s32(prod_hi), qinv_lo);

    /* prod - (int32)t * Q, then >> 16 */
    prod_lo = vmlsl_s16(prod_lo, t_lo, q_lo);
    prod_hi = vmlsl_s16(prod_hi, t_hi, q_lo);

    /* >> 16: take high 16 bits */
    return vuzp2q_s16(vreinterpretq_s16_s32(prod_lo),
                      vreinterpretq_s16_s32(prod_hi));
}

/* Broadcast fqmul: multiply all 8 elements of a by scalar zeta */
static inline int16x8_t neon_fqmul_scalar(int16x8_t a, int16_t zeta,
                                            int16x4_t qinv_lo, int16x4_t q_lo) {
    int16x4_t z = vdup_n_s16(zeta);

    int32x4_t prod_lo = vmull_s16(vget_low_s16(a), z);
    int32x4_t prod_hi = vmull_s16(vget_high_s16(a), z);

    int16x4_t t_lo = vmul_s16(vmovn_s32(prod_lo), qinv_lo);
    int16x4_t t_hi = vmul_s16(vmovn_s32(prod_hi), qinv_lo);

    prod_lo = vmlsl_s16(prod_lo, t_lo, q_lo);
    prod_hi = vmlsl_s16(prod_hi, t_hi, q_lo);

    return vuzp2q_s16(vreinterpretq_s16_s32(prod_lo),
                      vreinterpretq_s16_s32(prod_hi));
}

/* 4-wide Montgomery reduce: given 4 x int32 products, return 4 x int16 reduced.
 * Used by basemul to keep intermediate products in NEON registers. */
static inline int16x4_t neon_mred4(int32x4_t prod,
                                     int16x4_t qinv_lo, int16x4_t q_lo) {
    int16x4_t t = vmul_s16(vmovn_s32(prod), qinv_lo);
    prod = vmlsl_s16(prod, t, q_lo);
    return vshrn_n_s32(prod, 16);
}

/* Barrett reduction: r = a mod Q, for |a| < 2^15 */
static inline int16x8_t neon_barrett_reduce(int16x8_t a, int16x8_t v_vec,
                                             int16x8_t q_vec) {
    /* t = (a * v + 2^25) >> 26
     * Using vqdmulhq: returns (a*v*2) >> 16 (saturating).
     * Then shift right by 10 more to get >> 26 total.
     * But vqdmulh gives (a*v*2+rounding)>>16, so we need:
     *   t = vqdmulh(a, v)  → (a*v*2) >> 16
     *   t = (t + 1) >> 1   → (a*v) >> 16  (approximate)
     * Actually, let's use the simpler approach matching ML-KEM NEON:
     *   t = mulhi(a, v) = vqdmulh(a, v) >> 1 (approximately)
     *   t = (t + (1<<9)) >> 10
     *   t = t * Q
     *   result = a - t
     */
    int16x8_t t = vqdmulhq_s16(a, v_vec);
    /* vqdmulh gives (a*v*2 + rounding) >> 16.
     * We want (a*v) >> 16, so shift right by 1 more: */
    /* Actually for Barrett we want: t = round(a * v / 2^26)
     * = round(a * v >> 26).
     * vqdmulh(a,v) ≈ (2*a*v) >> 16.
     * So (vqdmulh(a,v) + 1) >> 11 ≈ (a*v) >> 26. */
    t = vaddq_s16(t, vdupq_n_s16(1 << 10));
    t = vshrq_n_s16(t, 11);
    t = vmulq_s16(t, q_vec);
    return vsubq_s16(a, t);
}

#endif /* DKE_USE_AARCH64 */
#endif /* DKE_NEON_PARAMS_H */
