/*
 * AArch64 NEON vectorized polynomial arithmetic for DKE.
 * Provides: poly_reduce, poly_add, poly_sub, poly_tomont, poly_scale2,
 *           poly_basemul_montgomery.
 * Only compiled when DKE_USE_AARCH64 is defined.
 */
#include "../parameters.h"

#if defined(DKE_USE_AARCH64)

#include <arm_neon.h>
#include <stdint.h>
#include "../poly.h"
#include "../polyvec.h"
#include "../ntt.h"
#include "../reduce.h"
#include "neon_params.h"

#if defined(DKE_USE_AARCH64_NATIVE) && DKE_N == 256 && DKE_Q == 3329
#include "../aarch64-native/dke_native.h"
#endif

/* ---- Constants ---- */
static const int16_t NEON_Q    = (int16_t)DKE_Q;
static const int16_t NEON_QINV = (int16_t)DKE_QINV;

void DKE_poly_reduce(poly *pol) {
#if 0 /* native reduce outputs [0,Q), DKE signal/compress expect signed [-Q/2,Q/2] */
    dke_poly_reduce_native(pol->coeffs);
    return;
#endif
    unsigned int i;
    int16x8_t v_vec = vdupq_n_s16((int16_t)DKE_BARRETT_V);
    int16x8_t q_vec = vdupq_n_s16(NEON_Q);

    for (i = 0; i < DKE_N; i += 8) {
        int16x8_t a = vld1q_s16(&pol->coeffs[i]);
        a = neon_barrett_reduce(a, v_vec, q_vec);
        vst1q_s16(&pol->coeffs[i], a);
    }
}

void DKE_poly_add(poly *r, const poly *a, const poly *b) {
    unsigned int i;
    for (i = 0; i < DKE_N; i += 8) {
        int16x8_t va = vld1q_s16(&a->coeffs[i]);
        int16x8_t vb = vld1q_s16(&b->coeffs[i]);
        vst1q_s16(&r->coeffs[i], vaddq_s16(va, vb));
    }
}

void DKE_poly_sub(poly *r, const poly *a, const poly *b) {
    unsigned int i;
    for (i = 0; i < DKE_N; i += 8) {
        int16x8_t va = vld1q_s16(&a->coeffs[i]);
        int16x8_t vb = vld1q_s16(&b->coeffs[i]);
        vst1q_s16(&r->coeffs[i], vsubq_s16(va, vb));
    }
}

void DKE_poly_tomont(poly *pol) {
#if defined(DKE_USE_AARCH64_NATIVE) && DKE_N == 256 && DKE_Q == 3329
    dke_poly_tomont_native(pol->coeffs);
    return;
#endif
    unsigned int i;
    int16x8_t f = vdupq_n_s16((int16_t)((1ULL << 32) % DKE_Q));
    int16x4_t qinv_lo = vdup_n_s16(NEON_QINV);
    int16x4_t q_lo    = vdup_n_s16(NEON_Q);

    for (i = 0; i < DKE_N; i += 8) {
        int16x8_t v = vld1q_s16(&pol->coeffs[i]);
        v = neon_fqmul(v, f, qinv_lo, q_lo);
        vst1q_s16(&pol->coeffs[i], v);
    }
}

/*
 * Basemul: r = a * b in NTT domain, fully NEON vectorized.
 * Process 2 groups of (a0,a1)*(b0,b1) per iteration (8 coefficients).
 *
 * Group 1: mod (X^2 - zeta)
 *   r0 = a0*b0 + a1*b1*zeta
 *   r1 = a0*b1 + a1*b0
 * Group 2: mod (X^2 + zeta)  [negate zeta]
 *   r2 = a2*b2 + a3*b3*(-zeta)
 *   r3 = a2*b3 + a3*b2
 *
 * Strategy (matching AVX2 poly_avx2.c):
 *   - Use vtrn_s16 to separate even/odd coefficients
 *   - Build zeta vector [z, z, -z, -z] for the two groups
 *   - Compute all products in NEON, no scalar lane extraction
 *   - Use vtrn_s16 to interleave results back
 */
void DKE_poly_basemul_montgomery(poly *res, const poly *a, const poly *b) {
    unsigned int i;
    int16x4_t qinv_v = vdup_n_s16(NEON_QINV);
    int16x4_t q_v    = vdup_n_s16(NEON_Q);
    const int16_t *zp = DKE_zetas + DKE_NTT_ZETAS_LEN / 2;

    for (i = 0; i < DKE_N / 4; i += 2) {
        /* Load 8 coefficients (2 groups of 4) */
        int16x8_t av = vld1q_s16(&a->coeffs[4 * i]);
        int16x8_t bv = vld1q_s16(&b->coeffs[4 * i]);

        /* Separate even/odd: a_even = [a0,a2,a4,a6], a_odd = [a1,a3,a5,a7]
         * vtrn interleaves pairs: trn1=[a0,b0,a2,b2,...], trn2=[a1,b1,a3,b3,...]
         * Applied to the same vector with itself, it extracts even/odd. */
        int16x4x2_t a_lo_trn = vtrn_s16(vget_low_s16(av), vget_high_s16(av));
        int16x4x2_t b_lo_trn = vtrn_s16(vget_low_s16(bv), vget_high_s16(bv));
        /* a_lo_trn.val[0] = [a0, a4, a2, a6] — not quite what we want.
         * Actually vtrn_s16([a0,a1,a2,a3], [a4,a5,a6,a7]) gives:
         *   val[0] = [a0, a4, a2, a6]  (even indices interleaved)
         *   val[1] = [a1, a5, a3, a7]  (odd indices interleaved)
         * This doesn't separate cleanly. Instead, use uzp (unzip). */

        /* Use vuzp to deinterleave: given [a0,a1,a2,a3] and [a4,a5,a6,a7],
         * vuzp1 = [a0,a2,a4,a6] (even), vuzp2 = [a1,a3,a5,a7] (odd). */
        int16x8_t a_even = vuzp1q_s16(av, av);  /* [a0,a2,a4,a6, a0,a2,a4,a6] */
        int16x8_t a_odd  = vuzp2q_s16(av, av);  /* [a1,a3,a5,a7, a1,a3,a5,a7] */
        int16x8_t b_even = vuzp1q_s16(bv, bv);
        int16x8_t b_odd  = vuzp2q_s16(bv, bv);

        /* We only need the low 4 elements from each */
        int16x4_t ae = vget_low_s16(a_even);  /* [a0, a2, a4, a6] */
        int16x4_t ao = vget_low_s16(a_odd);   /* [a1, a3, a5, a7] */
        int16x4_t be = vget_low_s16(b_even);
        int16x4_t bo = vget_low_s16(b_odd);

        /* Build zeta vector: [z0, -z0, z1, -z1]
         * where z0 = zetas[base + i], z1 = zetas[base + i + 1]
         * Group 0 uses +zeta, Group 1 uses -zeta */
        int16_t z0 = zp[i];
        int16_t z1 = zp[i + 1];
        int16x4_t zv = {z0, (int16_t)-z0, z1, (int16_t)-z1};

        /* Compute products:
         * ab11 = montgomery_reduce(a_odd * b_odd)  → [a1*b1, a3*b3, a5*b5, a7*b7] */
        int16x4_t ab11 = neon_mred4(vmull_s16(ao, bo), qinv_v, q_v);

        /* ab11z = montgomery_reduce(ab11 * zeta_vec) → [a1b1*z0, a3b3*(-z0), a5b5*z1, a7b7*(-z1)] */
        int16x4_t ab11z = neon_mred4(vmull_s16(ab11, zv), qinv_v, q_v);

        /* ab00 = montgomery_reduce(a_even * b_even) → [a0*b0, a2*b2, a4*b4, a6*b6] */
        int16x4_t ab00 = neon_mred4(vmull_s16(ae, be), qinv_v, q_v);

        /* r_even = ab00 + ab11z → [r0, r2, r4, r6] */
        int16x4_t r_even = vadd_s16(ab00, ab11z);

        /* ab01 = montgomery_reduce(a_even * b_odd) */
        int16x4_t ab01 = neon_mred4(vmull_s16(ae, bo), qinv_v, q_v);

        /* ab10 = montgomery_reduce(a_odd * b_even) */
        int16x4_t ab10 = neon_mred4(vmull_s16(ao, be), qinv_v, q_v);

        /* r_odd = ab01 + ab10 → [r1, r3, r5, r7] */
        int16x4_t r_odd = vadd_s16(ab01, ab10);

        /* Interleave even/odd back: [r0,r1, r2,r3, r4,r5, r6,r7] */
        int16x4x2_t r_trn = vzip_s16(r_even, r_odd);
        vst1q_s16(&res->coeffs[4 * i], vcombine_s16(r_trn.val[0], r_trn.val[1]));
    }
}

/*
 * Mulcache-based polyvec_basemul_acc for DKE-512 (K=4, N=512).
 *
 * Precomputes cache[i] = montgomery_reduce(b[2i+1] * zeta) for each pair,
 * then the basemul per pair reduces from 5 to 3 Montgomery multiplies:
 *   r0 = a0*b0 + a1*cache    (2 mul-acc + 1 mred)
 *   r1 = a0*b1 + a1*b0       (2 mul-acc + 1 mred)
 *
 * Accumulates K=4 results in 32-bit registers (smlal), single final reduce.
 * For DKE-128/256, the native mlkem-native ASM version is used instead.
 */
#if DKE_N == 512 && DKE_K == 4

/* Compute mulcache for one polynomial: cache[i] = mred(b[2i+1] * zeta[i]) */
static void poly_mulcache_512(int16_t cache[DKE_N / 2], const int16_t b[DKE_N]) {
    unsigned int i;
    int16x4_t qinv_v = vdup_n_s16(NEON_QINV);
    int16x4_t q_v    = vdup_n_s16(NEON_Q);
    const int16_t *zp = DKE_zetas + DKE_NTT_ZETAS_LEN / 2;

    /* Process 4 pairs (8 coefficients) at a time.
     * For pairs (b0,b1),(b2,b3),(b4,b5),(b6,b7):
     * cache = [mred(b1*z0), mred(b3*(-z0)), mred(b5*z1), mred(b7*(-z1))] */
    for (i = 0; i < DKE_N / 4; i += 2) {
        int16x8_t bv = vld1q_s16(&b[4 * i]);
        int16x4_t bo = vget_low_s16(vuzp2q_s16(bv, bv)); /* [b1,b3,b5,b7] */

        int16_t z0 = zp[i];
        int16_t z1 = zp[i + 1];
        int16x4_t zv = {z0, (int16_t)-z0, z1, (int16_t)-z1};

        int16x4_t c = neon_mred4(vmull_s16(bo, zv), qinv_v, q_v);
        vst1_s16(&cache[2 * i], c);
    }
}

void DKE_polyvec_basemul_acc_montgomery_neon(poly *res,
                                              const polyvec *a, const polyvec *b) {
    unsigned int i, k;
    int16x4_t qinv_v = vdup_n_s16(NEON_QINV);
    int16x4_t q_v    = vdup_n_s16(NEON_Q);

    /* Precompute mulcache for all K polynomials in b */
    int16_t b_cache[DKE_K * (DKE_N / 2)];
    for (k = 0; k < DKE_K; k++)
        poly_mulcache_512(b_cache + k * (DKE_N / 2), b->vec[k].coeffs);

    for (i = 0; i < DKE_N / 4; i += 2) {
        /* Accumulate K=4 basemul results in 32-bit */
        int32x4_t acc_r0 = vdupq_n_s32(0);
        int32x4_t acc_r1 = vdupq_n_s32(0);

        for (k = 0; k < DKE_K; k++) {
            int16x8_t av = vld1q_s16(&a->vec[k].coeffs[4 * i]);
            int16x8_t bv = vld1q_s16(&b->vec[k].coeffs[4 * i]);
            int16x4_t cv = vld1_s16(&b_cache[k * (DKE_N / 2) + 2 * i]);

            int16x4_t ae = vget_low_s16(vuzp1q_s16(av, av)); /* [a0,a2,a4,a6] */
            int16x4_t ao = vget_low_s16(vuzp2q_s16(av, av)); /* [a1,a3,a5,a7] */
            int16x4_t be = vget_low_s16(vuzp1q_s16(bv, bv)); /* [b0,b2,b4,b6] */
            int16x4_t bo = vget_low_s16(vuzp2q_s16(bv, bv)); /* [b1,b3,b5,b7] */

            /* r0 = a0*b0 + a1*cache (cache = b1*zeta already reduced)
             * Accumulate in 32-bit: acc += a0*b0 + a1*cache */
            acc_r0 = vmlal_s16(acc_r0, ae, be);   /* += a0*b0 */
            acc_r0 = vmlal_s16(acc_r0, ao, cv);   /* += a1*cache */

            /* r1 = a0*b1 + a1*b0 */
            acc_r1 = vmlal_s16(acc_r1, ae, bo);   /* += a0*b1 */
            acc_r1 = vmlal_s16(acc_r1, ao, be);   /* += a1*b0 */
        }

        /* Single Montgomery reduce at the end (instead of per-iteration) */
        int16x4_t re = neon_mred4(acc_r0, qinv_v, q_v);
        int16x4_t ro = neon_mred4(acc_r1, qinv_v, q_v);

        /* Interleave and store */
        int16x4x2_t r_trn = vzip_s16(re, ro);
        vst1q_s16(&res->coeffs[4 * i], vcombine_s16(r_trn.val[0], r_trn.val[1]));
    }
}
#endif

#endif /* DKE_USE_AARCH64 */
