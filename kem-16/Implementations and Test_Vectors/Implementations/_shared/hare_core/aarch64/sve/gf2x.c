/**
 * @file gf2x.c
 * @brief AArch64/SVE dense GF(2)[x]/(x^n-1) multiplication path.
 *
 * The ARM/SVE Additional implementation keeps the algebraic semantics of the
 * reference implementation: vect_mul(o, a1, a2) computes a1*a2 modulo
 * X^PARAM_N - 1 over GF(2).  It remains a dense public-size multiplication and
 * never uses secret support positions as sparse multiplication indices.
 *
 * For KR instances, this file uses a public-parameter GF2X schedule aligned
 * with the selected x86 structure: a word-aligned top-level Toom-3 layer,
 * dense Karatsuba submultiplication, NEON PMULL 64x64 base products when the
 * target is built with HARE_ARM_PMULL_GF2X, and SVE-assisted X^PARAM_N-1
 * reduction.  PMULL can still be disabled at build time, in which case the
 * same dense schedule uses a fixed 64-iteration software carry-less base case.
 *
 * All split sizes, thresholds, loop bounds, and reduction offsets are public
 * compile-time parameters.  No PKE/KEM, parameter, KAT, RM, RS, or FO behavior
 * is changed by this implementation.
 */

#include "gf2x.h"

#include <arm_sve.h>
#include <stdint.h>
#include <string.h>
#if defined(HARE_ARM_PMULL_GF2X)
#define HARE_ARM_GF2X_USE_PMULL 1
#include <arm_neon.h>
#else
#define HARE_ARM_GF2X_USE_PMULL 0
#endif
#include "parameters.h"

#if defined(HARE_BACKEND_KR)
#if (PARAM_N == 20899)
#define HARE_ARM_GF2X_PLAN_NWORDS 327U
#define HARE_ARM_GF2X_PLAN_TOOM_T 109U
#define HARE_ARM_GF2X_PLAN_TOOM_E 111U
#define HARE_ARM_GF2X_PLAN_LEAF_THRESHOLD 16U
#define HARE_ARM_GF2X_PLAN_USE_TOOM3_TOP 1
#elif (PARAM_N == 52379)
#define HARE_ARM_GF2X_PLAN_NWORDS 819U
#define HARE_ARM_GF2X_PLAN_TOOM_T 273U
#define HARE_ARM_GF2X_PLAN_TOOM_E 275U
#define HARE_ARM_GF2X_PLAN_LEAF_THRESHOLD 16U
#define HARE_ARM_GF2X_PLAN_USE_TOOM3_TOP 1
#elif (PARAM_N == 104869)
#define HARE_ARM_GF2X_PLAN_NWORDS 1639U
#define HARE_ARM_GF2X_PLAN_TOOM_T 547U
#define HARE_ARM_GF2X_PLAN_TOOM_E 549U
#define HARE_ARM_GF2X_PLAN_LEAF_THRESHOLD 16U
#define HARE_ARM_GF2X_PLAN_USE_TOOM3_TOP 1
#elif (PARAM_N == 173981)
#define HARE_ARM_GF2X_PLAN_NWORDS 2719U
#define HARE_ARM_GF2X_PLAN_TOOM_T 907U
#define HARE_ARM_GF2X_PLAN_TOOM_E 909U
#define HARE_ARM_GF2X_PLAN_LEAF_THRESHOLD 16U
#define HARE_ARM_GF2X_PLAN_USE_TOOM3_TOP 1
#endif
#endif

#if defined(HARE_ARM_GF2X_PLAN_LEAF_THRESHOLD)
#define HARE_ARM_GF2X_KARATSUBA_THRESHOLD HARE_ARM_GF2X_PLAN_LEAF_THRESHOLD
#else
#define HARE_ARM_GF2X_KARATSUBA_THRESHOLD 16U
#endif

#define HARE_ARM_GF2X_TMP_BUFFER_WORDS (16U * VEC_N_SIZE_64)
#define HARE_ARM_GF2X_DEC64 (PARAM_N & 0x3FU)
#define HARE_ARM_GF2X_D0 (64U - HARE_ARM_GF2X_DEC64)

#if defined(HARE_ARM_GF2X_PLAN_USE_TOOM3_TOP)
#define HARE_ARM_GF2X_USE_TOOM3_TOP HARE_ARM_GF2X_PLAN_USE_TOOM3_TOP
#else
#define HARE_ARM_GF2X_USE_TOOM3_TOP 0
#endif

#if HARE_ARM_GF2X_USE_TOOM3_TOP
#if (VEC_N_SIZE_64 != HARE_ARM_GF2X_PLAN_NWORDS)
#error "ARM GF2X KR plan does not match VEC_N_SIZE_64"
#endif
#define HARE_ARM_GF2X_TOOM_T HARE_ARM_GF2X_PLAN_TOOM_T
#define HARE_ARM_GF2X_TOOM_E HARE_ARM_GF2X_PLAN_TOOM_E
#else
#define HARE_ARM_GF2X_TOOM_T ((VEC_N_SIZE_64 + 2U) / 3U)
#define HARE_ARM_GF2X_TOOM_E (HARE_ARM_GF2X_TOOM_T + 2U)
#endif

#define HARE_ARM_GF2X_TOOM_2E (2U * HARE_ARM_GF2X_TOOM_E)
#define HARE_ARM_GF2X_TOOM_RO_WORDS (6U * HARE_ARM_GF2X_TOOM_T + 4U)
#define HARE_ARM_GF2X_TOOM_KSCRATCH_WORDS (16U * HARE_ARM_GF2X_TOOM_E)

static inline void clmul_u64(uint64_t *lo, uint64_t *hi, uint64_t a, uint64_t b) {
#if HARE_ARM_GF2X_USE_PMULL
    const poly64_t pa = (poly64_t)a;
    const poly64_t pb = (poly64_t)b;
    const poly128_t pc = vmull_p64(pa, pb);
    const uint64x2_t out = vreinterpretq_u64_p128(pc);
    *lo = vgetq_lane_u64(out, 0);
    *hi = vgetq_lane_u64(out, 1);
#else
    uint64_t r0 = 0;
    uint64_t r1 = 0;

    for (unsigned i = 0; i < 64u; ++i) {
        const uint64_t mask = UINT64_C(0) - ((b >> i) & 1u);
        if (i == 0u) {
            r0 ^= a & mask;
        } else {
            r0 ^= (a << i) & mask;
            r1 ^= (a >> (64u - i)) & mask;
        }
    }

    *lo = r0;
    *hi = r1;
#endif
}

static void clmul_schoolbook_mul(uint64_t *r, const uint64_t *a, const uint64_t *b, size_t n) {
    memset(r, 0, 2U * n * sizeof(uint64_t));

    for (size_t i = 0; i < n; ++i) {
        const uint64_t ai = a[i];
        for (size_t j = 0; j < n; ++j) {
            uint64_t lo = 0;
            uint64_t hi = 0;
            clmul_u64(&lo, &hi, ai, b[j]);
            r[i + j] ^= lo;
            r[i + j + 1U] ^= hi;
        }
    }
}

static void karatsuba_mul(uint64_t *r, const uint64_t *a, const uint64_t *b, size_t n, uint64_t *tmp_buffer) {
    if (n <= HARE_ARM_GF2X_KARATSUBA_THRESHOLD) {
        clmul_schoolbook_mul(r, a, b, n);
        return;
    }

    const size_t m = n >> 1;
    const size_t n0 = m;
    const size_t n1 = n - m;

    uint64_t *z0 = tmp_buffer;
    uint64_t *z2 = z0 + 2U * n;
    uint64_t *zmid = z2 + 2U * n;
    uint64_t *ta = zmid + 2U * n;
    uint64_t *tb = ta + n;
    uint64_t *child_buffer = tmp_buffer + 8U * n;

    karatsuba_mul(z0, a, b, n0, child_buffer);
    karatsuba_mul(z2, a + m, b + m, n1, child_buffer);

    for (size_t i = 0; i < n1; ++i) {
        const uint64_t loa = (i < n0) ? a[i] : 0U;
        const uint64_t lob = (i < n0) ? b[i] : 0U;
        ta[i] = loa ^ a[m + i];
        tb[i] = lob ^ b[m + i];
    }

    karatsuba_mul(zmid, ta, tb, n1, child_buffer);

    memset(r, 0, 2U * n * sizeof(uint64_t));

    for (size_t i = 0; i < 2U * n0; ++i) {
        r[i] ^= z0[i];
    }
    for (size_t i = 0; i < 2U * n1; ++i) {
        r[2U * m + i] ^= z2[i];
    }
    for (size_t i = 0; i < 2U * n1; ++i) {
        const uint64_t z0i = (i < 2U * n0) ? z0[i] : 0U;
        const uint64_t z2i = (i < 2U * n1) ? z2[i] : 0U;
        const uint64_t mid = zmid[i] ^ z0i ^ z2i;
        r[m + i] ^= mid;
    }
}

#if HARE_ARM_GF2X_USE_TOOM3_TOP
static void divide_by_y_plus_one(uint64_t *out, const uint64_t *in) {
    out[0] = in[0];
    for (size_t i = 1; i < HARE_ARM_GF2X_TOOM_2E; ++i) {
        out[i] = out[i - 1U] ^ in[i];
    }
}

static void karatsuba_mul_toom_e(uint64_t *r, const uint64_t *a, const uint64_t *b, uint64_t *scratch) {
    karatsuba_mul(r, a, b, HARE_ARM_GF2X_TOOM_E, scratch);
}

static void toom3_top_mul(uint64_t *r, const uint64_t *a, const uint64_t *b) {
    uint64_t U0[HARE_ARM_GF2X_TOOM_E] = {0};
    uint64_t U1[HARE_ARM_GF2X_TOOM_E] = {0};
    uint64_t U2[HARE_ARM_GF2X_TOOM_E] = {0};
    uint64_t V0[HARE_ARM_GF2X_TOOM_E] = {0};
    uint64_t V1[HARE_ARM_GF2X_TOOM_E] = {0};
    uint64_t V2[HARE_ARM_GF2X_TOOM_E] = {0};

    uint64_t W0[HARE_ARM_GF2X_TOOM_2E] = {0};
    uint64_t W1[HARE_ARM_GF2X_TOOM_2E] = {0};
    uint64_t W2[HARE_ARM_GF2X_TOOM_2E] = {0};
    uint64_t W3[HARE_ARM_GF2X_TOOM_2E] = {0};
    uint64_t W4[HARE_ARM_GF2X_TOOM_2E] = {0};
    uint64_t tmp[HARE_ARM_GF2X_TOOM_2E + 3U] = {0};
    uint64_t ro[HARE_ARM_GF2X_TOOM_RO_WORDS] = {0};
    uint64_t scratch[HARE_ARM_GF2X_TOOM_KSCRATCH_WORDS];

    for (size_t i = 0; i < HARE_ARM_GF2X_TOOM_T; ++i) {
        const size_t i1 = i + HARE_ARM_GF2X_TOOM_T;
        const size_t i2 = i + 2U * HARE_ARM_GF2X_TOOM_T;
        U0[i] = (i < VEC_N_SIZE_64) ? a[i] : 0U;
        V0[i] = (i < VEC_N_SIZE_64) ? b[i] : 0U;
        U1[i] = (i1 < VEC_N_SIZE_64) ? a[i1] : 0U;
        V1[i] = (i1 < VEC_N_SIZE_64) ? b[i1] : 0U;
        U2[i] = (i2 < VEC_N_SIZE_64) ? a[i2] : 0U;
        V2[i] = (i2 < VEC_N_SIZE_64) ? b[i2] : 0U;
    }

    for (size_t i = 0; i < HARE_ARM_GF2X_TOOM_T; ++i) {
        W3[i] = U0[i] ^ U1[i] ^ U2[i];
        W2[i] = V0[i] ^ V1[i] ^ V2[i];
    }
    karatsuba_mul_toom_e(W1, W3, W2, scratch);

    W0[0] = 0U;
    W4[0] = 0U;
    W0[1] = U1[0];
    W4[1] = V1[0];
    for (size_t i = 1; i < HARE_ARM_GF2X_TOOM_T + 1U; ++i) {
        W0[i + 1U] = U1[i] ^ U2[i - 1U];
        W4[i + 1U] = V1[i] ^ V2[i - 1U];
    }
    W0[HARE_ARM_GF2X_TOOM_T + 1U] = U2[HARE_ARM_GF2X_TOOM_T - 1U];
    W4[HARE_ARM_GF2X_TOOM_T + 1U] = V2[HARE_ARM_GF2X_TOOM_T - 1U];

    for (size_t i = 0; i < HARE_ARM_GF2X_TOOM_E; ++i) {
        W3[i] ^= W0[i];
        W2[i] ^= W4[i];
        W0[i] ^= U0[i];
        W4[i] ^= V0[i];
    }

    karatsuba_mul_toom_e(tmp, W3, W2, scratch);
    memcpy(W3, tmp, HARE_ARM_GF2X_TOOM_2E * sizeof(uint64_t));
    karatsuba_mul_toom_e(W2, W0, W4, scratch);

    karatsuba_mul_toom_e(W4, U2, V2, scratch);
    karatsuba_mul_toom_e(W0, U0, V0, scratch);

    for (size_t i = 0; i < HARE_ARM_GF2X_TOOM_2E; ++i) {
        W3[i] ^= W2[i];
    }
    for (size_t i = 0; i < 2U * HARE_ARM_GF2X_TOOM_T; ++i) {
        W1[i] ^= W0[i];
    }

    for (size_t i = 0; i < HARE_ARM_GF2X_TOOM_2E - 1U; ++i) {
        W2[i] = W2[i + 1U] ^ W0[i + 1U];
    }
    W2[HARE_ARM_GF2X_TOOM_2E - 1U] = 0U;

    memset(tmp, 0, sizeof(tmp));
    for (size_t i = 0; i < HARE_ARM_GF2X_TOOM_2E; ++i) {
        tmp[i] = W2[i] ^ W3[i] ^ W4[i];
    }
    for (size_t i = 0; i < 2U * HARE_ARM_GF2X_TOOM_T; ++i) {
        tmp[i + 3U] ^= W4[i];
    }
    divide_by_y_plus_one(W2, tmp);

    memset(tmp, 0, sizeof(tmp));
    for (size_t i = 0; i < HARE_ARM_GF2X_TOOM_2E - 1U; ++i) {
        tmp[i] = W3[i + 1U] ^ W1[i + 1U];
    }
    divide_by_y_plus_one(W3, tmp);

    for (size_t i = 0; i < HARE_ARM_GF2X_TOOM_2E; ++i) {
        W1[i] ^= W2[i] ^ W4[i];
        W2[i] ^= W3[i];
    }

    for (size_t i = 0; i < HARE_ARM_GF2X_TOOM_T; ++i) {
        ro[i] = W0[i];
        ro[i + HARE_ARM_GF2X_TOOM_T] = W0[i + HARE_ARM_GF2X_TOOM_T] ^ W1[i];
        ro[i + 2U * HARE_ARM_GF2X_TOOM_T] = W1[i + HARE_ARM_GF2X_TOOM_T] ^ W2[i];
        ro[i + 3U * HARE_ARM_GF2X_TOOM_T] = W2[i + HARE_ARM_GF2X_TOOM_T] ^ W3[i];
        ro[i + 4U * HARE_ARM_GF2X_TOOM_T] = W3[i + HARE_ARM_GF2X_TOOM_T] ^ W4[i];
        ro[i + 5U * HARE_ARM_GF2X_TOOM_T] = W4[i + HARE_ARM_GF2X_TOOM_T];
    }
    for (size_t j = 0; j < 4U; ++j) {
        ro[4U * HARE_ARM_GF2X_TOOM_T + j] ^= W2[2U * HARE_ARM_GF2X_TOOM_T + j];
        ro[5U * HARE_ARM_GF2X_TOOM_T + j] ^= W3[2U * HARE_ARM_GF2X_TOOM_T + j];
    }

    for (size_t i = 0; i < 2U * VEC_N_SIZE_64; ++i) {
        r[i] = ro[i];
    }
}
#endif

static void reduce_arm(uint64_t *o, const uint64_t *a) {
#if HARE_ARM_GF2X_DEC64 == 0
    size_t i = 0;
    const uint64_t vl = svcntd();
    for (; i < VEC_N_SIZE_64; i += vl) {
        svbool_t pg = svwhilelt_b64((uint64_t)i, (uint64_t)VEC_N_SIZE_64);
        svuint64_t lo = svld1_u64(pg, a + i);
        svuint64_t hi = svld1_u64(pg, a + i + VEC_N_SIZE_64);
        svst1_u64(pg, o + i, sveor_u64_x(pg, lo, hi));
    }
#else
    size_t i = 0;
    const uint64_t vl = svcntd();
    for (; i < VEC_N_SIZE_64; i += vl) {
        svbool_t pg = svwhilelt_b64((uint64_t)i, (uint64_t)VEC_N_SIZE_64);
        svuint64_t lo = svld1_u64(pg, a + i);
        svuint64_t hi0 = svld1_u64(pg, a + i + VEC_N_SIZE_64 - 1U);
        svuint64_t hi1 = svld1_u64(pg, a + i + VEC_N_SIZE_64);
        svuint64_t folded = sveor_u64_x(pg,
                                        svlsr_n_u64_z(pg, hi0, HARE_ARM_GF2X_DEC64),
                                        svlsl_n_u64_z(pg, hi1, HARE_ARM_GF2X_D0));
        svst1_u64(pg, o + i, sveor_u64_x(pg, lo, folded));
    }
#endif
    o[VEC_N_SIZE_64 - 1U] &= BITMASK(PARAM_N, 64);
}

void vect_mul(uint64_t *o, const uint64_t *a1, const uint64_t *a2) {
    uint64_t unreduced[2U * VEC_N_SIZE_64];

#if HARE_ARM_GF2X_USE_TOOM3_TOP
    toom3_top_mul(unreduced, a1, a2);
#else
    uint64_t tmp_buffer[HARE_ARM_GF2X_TMP_BUFFER_WORDS];
    karatsuba_mul(unreduced, a1, a2, VEC_N_SIZE_64, tmp_buffer);
#endif
    reduce_arm(o, unreduced);
}
