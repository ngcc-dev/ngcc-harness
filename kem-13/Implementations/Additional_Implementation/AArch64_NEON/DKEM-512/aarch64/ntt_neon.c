/*
 * AArch64 NEON NTT / INVNTT for DKE.
 * Uses NEON 128-bit SIMD (8 x int16 per vector).
 *
 * NTT: Cooley-Tukey butterflies, top-down (len = N/2 .. 2).
 * INVNTT: Gentleman-Sande butterflies, bottom-up (len = 2 .. N/2),
 *         then multiply by f = INVNTT_F.
 *
 * Only compiled when DKE_USE_AARCH64 is defined.
 */
#include "../parameters.h"

#if defined(DKE_USE_AARCH64)

#include <arm_neon.h>
#include <stdint.h>
#include "../ntt.h"
#include "../reduce.h"
#include "neon_params.h"

#if defined(DKE_USE_AARCH64_NATIVE)
#include "../aarch64-native/dke_native.h"
#endif

static const int16_t NEON_Q    = (int16_t)DKE_Q;
static const int16_t NEON_QINV = (int16_t)DKE_QINV;

/* Scalar fqmul for small-layer fallback */
static inline int16_t fqmul_s(int16_t a, int16_t b) {
    return DKE_montgomery_reduce((int32_t)a * b);
}

/*
 * NTT: Cooley-Tukey, in-place.
 * Layers with len >= 8: NEON vectorized (8 coefficients per vector).
 * Layers with len < 8: scalar fallback.
 */
void DKE_ntt(int16_t r[DKE_N]) {
#if defined(DKE_USE_AARCH64_NATIVE) && DKE_N == 256 && DKE_Q == 3329
    dke_ntt_asm(r);
    return;
#elif defined(DKE_USE_AARCH64_NATIVE) && DKE_N == 512 && DKE_Q == 7681
    dke_ntt512_compiler(r);
    return;
#endif
    unsigned int len, start, j, k;
    int16x4_t qinv_lo = vdup_n_s16(NEON_QINV);
    int16x4_t q_lo    = vdup_n_s16(NEON_Q);

    k = 1;

    /* NEON layers: len = N/2 down to 8 */
    for (len = DKE_N / 2; len >= 8; len >>= 1) {
        for (start = 0; start < DKE_N; start += 2 * len) {
            int16_t zeta = DKE_zetas[k++];
            for (j = start; j < start + len; j += 8) {
                int16x8_t a = vld1q_s16(&r[j]);
                int16x8_t b = vld1q_s16(&r[j + len]);

                /* t = fqmul(zeta, b) */
                int16x8_t t = neon_fqmul_scalar(b, zeta, qinv_lo, q_lo);

                /* CT butterfly: a' = a + t, b' = a - t */
                vst1q_s16(&r[j],       vaddq_s16(a, t));
                vst1q_s16(&r[j + len], vsubq_s16(a, t));
            }
        }
    }

#if defined(DKE_NEON_OPT)
    /* Vectorized layer: len = 4.
     * Each block: 8 coeffs [a0..a3 | b0..b3] with one zeta per block.
     * Process 2 adjacent blocks (16 coeffs) per iteration. */
    {
        for (start = 0; start < DKE_N; start += 16) {
            int16_t z0 = DKE_zetas[k++];
            int16_t z1 = DKE_zetas[k++];
            /* Block 0: r[start..start+3] (a), r[start+4..start+7] (b), zeta=z0
             * Block 1: r[start+8..start+11] (a), r[start+12..start+15] (b), zeta=z1 */
            int16x4_t a0 = vld1_s16(&r[start]);
            int16x4_t b0 = vld1_s16(&r[start + 4]);
            int16x4_t a1 = vld1_s16(&r[start + 8]);
            int16x4_t b1 = vld1_s16(&r[start + 12]);
            /* fqmul(z, b) for each half */
            int32x4_t p0 = vmull_s16(b0, vdup_n_s16(z0));
            int32x4_t p1 = vmull_s16(b1, vdup_n_s16(z1));
            int16x4_t t0 = neon_mred4(p0, qinv_lo, q_lo);
            int16x4_t t1 = neon_mred4(p1, qinv_lo, q_lo);
            vst1_s16(&r[start],      vadd_s16(a0, t0));
            vst1_s16(&r[start + 4],  vsub_s16(a0, t0));
            vst1_s16(&r[start + 8],  vadd_s16(a1, t1));
            vst1_s16(&r[start + 12], vsub_s16(a1, t1));
        }

        /* len = 2: each block is 4 coeffs [a0,a1 | b0,b1], one zeta per block.
         * NEON vectorized: process 8 coeffs (2 blocks) per iteration.
         * Use vtrn (reinterpret as s32) to separate even/odd 32-bit words:
         *   v = [c0,c1, c2,c3, c4,c5, c6,c7]
         *   a = [c0,c1, c0,c1, c4,c5, c4,c5]  (even 32-bit words)
         *   b = [c2,c3, c2,c3, c6,c7, c6,c7]  (odd 32-bit words)
         * Then CT butterfly with per-block zeta. */
        for (start = 0; start < DKE_N; start += 8) {
            int16_t z0 = DKE_zetas[k++];
            int16_t z1 = DKE_zetas[k++];
            int16x8_t v = vld1q_s16(&r[start]);

            /* Separate even/odd 32-bit words using vtrn on s32 view */
            int32x4_t v32 = vreinterpretq_s32_s16(v);
            int32x2_t v32_lo = vget_low_s32(v32);
            int32x2_t v32_hi = vget_high_s32(v32);
            int32x2x2_t trn = vtrn_s32(v32_lo, v32_hi);
            /* trn.val[0] = [c0c1, c4c5], trn.val[1] = [c2c3, c6c7] */

            int16x4_t a_part = vreinterpret_s16_s32(trn.val[0]);
            int16x4_t b_part = vreinterpret_s16_s32(trn.val[1]);

            /* zeta vector: [z0, z0, z1, z1] */
            int16x4_t zv = {z0, z0, z1, z1};

            /* t = fqmul(zeta, b) */
            int16x4_t t = neon_mred4(vmull_s16(zv, b_part), qinv_lo, q_lo);

            /* CT butterfly: a' = a + t, b' = a - t */
            int16x4_t r_a = vadd_s16(a_part, t);
            int16x4_t r_b = vsub_s16(a_part, t);

            /* Interleave back using vtrn on s32 view */
            int32x2x2_t r_trn = vtrn_s32(vreinterpret_s32_s16(r_a),
                                          vreinterpret_s32_s16(r_b));
            int16x8_t result = vreinterpretq_s16_s32(
                vcombine_s32(r_trn.val[0], r_trn.val[1]));
            vst1q_s16(&r[start], result);
        }
    }
#else
    /* Scalar layers: len = 4, 2 */
    for (; len >= 2; len >>= 1) {
        for (start = 0; start < DKE_N; start += 2 * len) {
            int16_t zeta = DKE_zetas[k++];
            for (j = start; j < start + len; j++) {
                int16_t t = fqmul_s(zeta, r[j + len]);
                r[j + len] = r[j] - t;
                r[j]       = r[j] + t;
            }
        }
    }
#endif
}

/*
 * INVNTT: Gentleman-Sande, in-place.
 * Layers with len >= 8: NEON vectorized.
 * Layers with len < 8: scalar.
 * Final multiply by INVNTT_F.
 */
void DKE_invntt(int16_t r[DKE_N]) {
#if defined(DKE_USE_AARCH64_NATIVE) && DKE_N == 256 && DKE_Q == 3329
    dke_intt_asm(r);
    return;
#elif defined(DKE_USE_AARCH64_NATIVE) && DKE_N == 512 && DKE_Q == 7681
    dke_intt512_compiler(r);
    return;
#endif
    unsigned int start, len, j, k;
    int16x4_t qinv_lo = vdup_n_s16(NEON_QINV);
    int16x4_t q_lo    = vdup_n_s16(NEON_Q);
    int16x8_t v_vec   = vdupq_n_s16((int16_t)DKE_BARRETT_V);
    int16x8_t q_vec   = vdupq_n_s16(NEON_Q);

    k = DKE_NTT_ZETAS_LEN - 1;

#if defined(DKE_NEON_OPT)
    /* Vectorized INVNTT small layers */
    {
        /* len = 2: each block = 4 coeffs [a0,a1 | b0,b1], ONE zeta per block.
         * GS butterfly: a' = barrett(a+b), b' = fqmul(zeta, b-a)
         * NEON vectorized: process 2 blocks (8 coeffs) per iteration.
         * Use vtrn_s32 to separate even/odd 32-bit words (same as forward NTT). */
        for (start = 0; start < DKE_N; start += 8) {
            int16_t z0 = DKE_zetas[k--];
            int16_t z1 = DKE_zetas[k--];
            int16x8_t v = vld1q_s16(&r[start]);

            /* Separate even/odd 32-bit words */
            int32x4_t v32 = vreinterpretq_s32_s16(v);
            int32x2_t v32_lo = vget_low_s32(v32);
            int32x2_t v32_hi = vget_high_s32(v32);
            int32x2x2_t trn = vtrn_s32(v32_lo, v32_hi);
            int16x4_t a_part = vreinterpret_s16_s32(trn.val[0]); /* [c0,c1, c4,c5] */
            int16x4_t b_part = vreinterpret_s16_s32(trn.val[1]); /* [c2,c3, c6,c7] */

            /* GS butterfly: sum = barrett(a+b), diff = fqmul(zeta, b-a) */
            int16x8_t sum8 = vcombine_s16(vadd_s16(a_part, b_part),
                                          vadd_s16(a_part, b_part));
            int16x8_t sum_br = neon_barrett_reduce(sum8, v_vec, q_vec);
            int16x4_t r_a = vget_low_s16(sum_br);

            int16x4_t diff = vsub_s16(b_part, a_part);
            int16x4_t zv = {z0, z0, z1, z1};
            int16x4_t r_b = neon_mred4(vmull_s16(zv, diff), qinv_lo, q_lo);

            /* Interleave back */
            int32x2x2_t r_trn = vtrn_s32(vreinterpret_s32_s16(r_a),
                                          vreinterpret_s32_s16(r_b));
            int16x8_t result = vreinterpretq_s16_s32(
                vcombine_s32(r_trn.val[0], r_trn.val[1]));
            vst1q_s16(&r[start], result);
        }

        /* len = 4: each block is 8 coeffs [a0..a3 | b0..b3], one zeta.
         * Process 2 blocks (16 coeffs) per iteration. */
        for (start = 0; start < DKE_N; start += 16) {
            int16_t z0 = DKE_zetas[k--];  /* block 0 consumed first */
            int16_t z1 = DKE_zetas[k--];  /* block 1 */
            int16x4_t a0 = vld1_s16(&r[start]);
            int16x4_t b0 = vld1_s16(&r[start + 4]);
            int16x4_t a1 = vld1_s16(&r[start + 8]);
            int16x4_t b1 = vld1_s16(&r[start + 12]);
            /* GS butterfly: sum = barrett(a+b), diff = fqmul(zeta, b-a) */
            int16x8_t sum0 = vcombine_s16(vadd_s16(a0, b0), vadd_s16(a1, b1));
            sum0 = neon_barrett_reduce(sum0, v_vec, q_vec);
            int16x4_t d0 = vsub_s16(b0, a0);
            int16x4_t d1 = vsub_s16(b1, a1);
            int16x4_t rd0 = neon_mred4(vmull_s16(d0, vdup_n_s16(z0)), qinv_lo, q_lo);
            int16x4_t rd1 = neon_mred4(vmull_s16(d1, vdup_n_s16(z1)), qinv_lo, q_lo);
            vst1_s16(&r[start],      vget_low_s16(sum0));
            vst1_s16(&r[start + 4],  rd0);
            vst1_s16(&r[start + 8],  vget_high_s16(sum0));
            vst1_s16(&r[start + 12], rd1);
        }
        len = 8;
    }
#else
    /* Scalar layers: len = 2, 4 */
    for (len = 2; len < 8; len <<= 1) {
        for (start = 0; start < DKE_N; start += 2 * len) {
            int16_t zeta = DKE_zetas[k--];
            for (j = start; j < start + len; j++) {
                int16_t t = r[j];
                r[j]       = DKE_barrett_reduce(t + r[j + len]);
                r[j + len] = fqmul_s(zeta, r[j + len] - t);
            }
        }
    }
#endif

#if defined(DKE_NEON_OPT)
    /* NEON layers: len = 8 up to N/4 (non-final layers) */
    for (; len <= DKE_N / 4; len <<= 1) {
        for (start = 0; start < DKE_N; start += 2 * len) {
            int16_t zeta = DKE_zetas[k--];
            for (j = start; j < start + len; j += 8) {
                int16x8_t a = vld1q_s16(&r[j]);
                int16x8_t b = vld1q_s16(&r[j + len]);
                int16x8_t sum  = neon_barrett_reduce(vaddq_s16(a, b), v_vec, q_vec);
                int16x8_t diff = neon_fqmul_scalar(vsubq_s16(b, a), zeta, qinv_lo, q_lo);
                vst1q_s16(&r[j],       sum);
                vst1q_s16(&r[j + len], diff);
            }
        }
    }

    /* Final layer (len = N/2): fuse INVNTT_F multiply into the butterfly.
     * Instead of: sum = barrett(a+b), diff = fqmul(zeta, b-a), then separate fqmul(f, *)
     * Do:         sum = fqmul(f, barrett(a+b)), diff = fqmul(zeta_f, b-a)
     * where zeta_f = fqmul(zeta, f) — precomputed, saves one full pass over all N coeffs. */
    {
        const int16_t f = DKE_INVNTT_F;
        int16_t zeta = DKE_zetas[k--];
        int16_t zeta_f = fqmul_s(zeta, f);
        for (j = 0; j < DKE_N / 2; j += 8) {
            int16x8_t a = vld1q_s16(&r[j]);
            int16x8_t b = vld1q_s16(&r[j + DKE_N / 2]);
            int16x8_t sum  = neon_barrett_reduce(vaddq_s16(a, b), v_vec, q_vec);
            int16x8_t diff = vsubq_s16(b, a);
            sum  = neon_fqmul_scalar(sum, f, qinv_lo, q_lo);
            diff = neon_fqmul_scalar(diff, zeta_f, qinv_lo, q_lo);
            vst1q_s16(&r[j],              sum);
            vst1q_s16(&r[j + DKE_N / 2],  diff);
        }
    }
#else
    /* NEON layers: len = 8 up to N/2 */
    for (; len <= DKE_N / 2; len <<= 1) {
        for (start = 0; start < DKE_N; start += 2 * len) {
            int16_t zeta = DKE_zetas[k--];
            for (j = start; j < start + len; j += 8) {
                int16x8_t a = vld1q_s16(&r[j]);
                int16x8_t b = vld1q_s16(&r[j + len]);
                int16x8_t sum  = neon_barrett_reduce(vaddq_s16(a, b), v_vec, q_vec);
                int16x8_t diff = neon_fqmul_scalar(vsubq_s16(b, a), zeta, qinv_lo, q_lo);
                vst1q_s16(&r[j],       sum);
                vst1q_s16(&r[j + len], diff);
            }
        }
    }
    /* Final multiply by INVNTT_F */
    {
        int16_t f = DKE_INVNTT_F;
        for (j = 0; j < DKE_N; j += 8) {
            int16x8_t a = vld1q_s16(&r[j]);
            a = neon_fqmul_scalar(a, f, qinv_lo, q_lo);
            vst1q_s16(&r[j], a);
        }
    }
#endif
}

/*
 * Basemul: provided by poly_neon.c (scalar with NEON reduce/add/sub).
 * The basemul itself is hard to vectorize with NEON 128-bit due to
 * the zeta-per-group structure. Scalar basemul with NEON helpers
 * is the standard approach (same as mlkem-native).
 */

#endif /* DKE_USE_AARCH64 */
