/**
 * @file gf2x.c
 * @brief x86_64 AVX2/PCLMUL implementation of multiplication in GF(2)[x]/(x^n-1).
 *
 * This optimized file keeps the exact public API and algebraic semantics of the
 * reference gf2x.c: vect_mul(o, a1, a2) computes a1*a2 modulo X^PARAM_N - 1.
 *
 * HARE KR targets use the selected x86 GF2X kernel: an explicit public-
 * parameter plan with a top-level word-aligned Toom-3 layer, dense
 * Karatsuba/PCLMUL submultiplication, and AVX2-assisted X^PARAM_N-1 reduction.
 *
 * The implementation remains dense and public-size. It does not use secret
 * sparse multiplication, secret support indices, or secret-dependent memory
 * addressing.
 */

#include "gf2x.h"
#include <immintrin.h>
#include <stdint.h>
#include <string.h>
#include "parameters.h"
#include "gf2x_kr_plan.h"

/**
 * @brief PCLMUL schoolbook base-case threshold.
 *
 * The value is public and selected only from compile-time instance parameters.
 * KR v1.5 parameters use the public leaf threshold in gf2x_kr_plan.h.
 * This does not use secret support positions, sparse multiplication, or
 * input-dependent control.
 */
#if defined(HARE_BACKEND_KR) && defined(HARE_GF2X_PLAN_LEAF_THRESHOLD)
  #define HARE_GF2X_KARATSUBA_THRESHOLD HARE_GF2X_PLAN_LEAF_THRESHOLD
  #define HARE_GF2X_PARAMETER_SPECIALIZED 1
#else
  #define HARE_GF2X_KARATSUBA_THRESHOLD 32U
#endif

#define HARE_GF2X_TMP_BUFFER_WORDS (16U * VEC_N_SIZE_64)
#define HARE_GF2X_DEC64 (PARAM_N & 0x3FU)
#define HARE_GF2X_D0 (64U - HARE_GF2X_DEC64)

/*
 * Established top-level Toom-3 layer over 64-bit word blocks, followed by the
 * existing dense Karatsuba/PCLMUL sub-multiplier.
 * This is enabled only for KR v1.5 x86 targets, whose parameters are public
 * compile-time constants.  The implementation remains dense and never uses secret support
 * positions for sparse multiplication or memory addressing.
 */
#if defined(HARE_BACKEND_KR) && defined(HARE_GF2X_PARAMETER_SPECIALIZED) && defined(HARE_GF2X_PLAN_USE_TOOM3_TOP)
  #define HARE_GF2X_USE_TOOM3_TOP HARE_GF2X_PLAN_USE_TOOM3_TOP
#else
  #define HARE_GF2X_USE_TOOM3_TOP 0
#endif


#if HARE_GF2X_USE_TOOM3_TOP
  #if (VEC_N_SIZE_64 != HARE_GF2X_PLAN_NWORDS)
    #error "GF2X KR plan does not match VEC_N_SIZE_64"
  #endif
  #define HARE_GF2X_TOOM_T HARE_GF2X_PLAN_TOOM_T
  #define HARE_GF2X_TOOM_E HARE_GF2X_PLAN_TOOM_E
#else
  #define HARE_GF2X_TOOM_T ((VEC_N_SIZE_64 + 2U) / 3U)
  #define HARE_GF2X_TOOM_E (HARE_GF2X_TOOM_T + 2U)
#endif
#define HARE_GF2X_TOOM_PAD_WORDS (3U * HARE_GF2X_TOOM_T)
#define HARE_GF2X_TOOM_2E (2U * HARE_GF2X_TOOM_E)
#define HARE_GF2X_TOOM_RO_WORDS (6U * HARE_GF2X_TOOM_T + 4U)
#define HARE_GF2X_TOOM_KSCRATCH_WORDS (16U * HARE_GF2X_TOOM_E)

/**
 * @brief Compute a carry-less 64x64 -> 128-bit product with PCLMULQDQ.
 */
static inline void clmul_u64(uint64_t *lo, uint64_t *hi, uint64_t a, uint64_t b) {
    const __m128i aa = _mm_cvtsi64_si128((long long)a);
    const __m128i bb = _mm_cvtsi64_si128((long long)b);
    const __m128i pp = _mm_clmulepi64_si128(aa, bb, 0x00);
    *lo = (uint64_t)_mm_cvtsi128_si64(pp);
    *hi = (uint64_t)_mm_cvtsi128_si64(_mm_srli_si128(pp, 8));
}

/**
 * @brief PCLMUL schoolbook multiplication over GF(2).
 *
 * Computes r = a*b where a and b contain n 64-bit limbs. The output has 2*n
 * limbs. This routine is used only as a public-size base case inside Karatsuba;
 * loop counts depend on parameter sizes, not secrets.
 */
static void pclmul_schoolbook_mul(uint64_t *r, const uint64_t *a, const uint64_t *b, size_t n) {
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

/**
 * @brief Recursive Karatsuba multiplication using caller-supplied workspace.
 */
static void karatsuba_mul(uint64_t *r, const uint64_t *a, const uint64_t *b, size_t n, uint64_t *tmp_buffer) {
    if (n <= HARE_GF2X_KARATSUBA_THRESHOLD) {
        pclmul_schoolbook_mul(r, a, b, n);
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

#if HARE_GF2X_USE_TOOM3_TOP
static void divide_by_y_plus_one(uint64_t *out, const uint64_t *in) {
    out[0] = in[0];
    for (size_t i = 1; i < HARE_GF2X_TOOM_2E; ++i) {
        out[i] = out[i - 1U] ^ in[i];
    }
}

static void karatsuba_mul_toom_e(uint64_t *r, const uint64_t *a, const uint64_t *b, uint64_t *scratch) {
    karatsuba_mul(r, a, b, HARE_GF2X_TOOM_E, scratch);
}

/**
 * @brief Top-level word-aligned Toom-3 multiplication.
 *
 * The split variable is Y=X^(64*T).  Inputs are zero-padded to 3*T words.
 * Evaluation/interpolation follows the same GF(2) Toom-3 structure used by
 * mature HQC-style implementations, but delegates sub-products to the existing
 * dense Karatsuba/PCLMUL routine.  All loop bounds are public compile-time
 * parameters.  The first 2*VEC_N_SIZE_64 words of the padded product are
 * copied to @p r, which is sufficient because input words beyond VEC_N_SIZE_64
 * are zero.
 */
static void toom3_top_mul(uint64_t *r, const uint64_t *a, const uint64_t *b) {
    uint64_t U0[HARE_GF2X_TOOM_E] = {0};
    uint64_t U1[HARE_GF2X_TOOM_E] = {0};
    uint64_t U2[HARE_GF2X_TOOM_E] = {0};
    uint64_t V0[HARE_GF2X_TOOM_E] = {0};
    uint64_t V1[HARE_GF2X_TOOM_E] = {0};
    uint64_t V2[HARE_GF2X_TOOM_E] = {0};

    uint64_t W0[HARE_GF2X_TOOM_2E] = {0};
    uint64_t W1[HARE_GF2X_TOOM_2E] = {0};
    uint64_t W2[HARE_GF2X_TOOM_2E] = {0};
    uint64_t W3[HARE_GF2X_TOOM_2E] = {0};
    uint64_t W4[HARE_GF2X_TOOM_2E] = {0};
    uint64_t tmp[HARE_GF2X_TOOM_2E + 3U] = {0};
    uint64_t ro[HARE_GF2X_TOOM_RO_WORDS] = {0};
    uint64_t scratch[HARE_GF2X_TOOM_KSCRATCH_WORDS];

    for (size_t i = 0; i < HARE_GF2X_TOOM_T; ++i) {
        const size_t i1 = i + HARE_GF2X_TOOM_T;
        const size_t i2 = i + 2U * HARE_GF2X_TOOM_T;
        U0[i] = (i  < VEC_N_SIZE_64) ? a[i]  : 0U;
        V0[i] = (i  < VEC_N_SIZE_64) ? b[i]  : 0U;
        U1[i] = (i1 < VEC_N_SIZE_64) ? a[i1] : 0U;
        V1[i] = (i1 < VEC_N_SIZE_64) ? b[i1] : 0U;
        U2[i] = (i2 < VEC_N_SIZE_64) ? a[i2] : 0U;
        V2[i] = (i2 < VEC_N_SIZE_64) ? b[i2] : 0U;
    }

    /* Evaluation at 1: W1=(U0+U1+U2)*(V0+V1+V2). */
    for (size_t i = 0; i < HARE_GF2X_TOOM_T; ++i) {
        W3[i] = U0[i] ^ U1[i] ^ U2[i];
        W2[i] = V0[i] ^ V1[i] ^ V2[i];
    }
    karatsuba_mul_toom_e(W1, W3, W2, scratch);

    /* Evaluation at Y and Y+1, word-shifted by one Y coefficient. */
    W0[0] = 0U;
    W4[0] = 0U;
    W0[1] = U1[0];
    W4[1] = V1[0];
    for (size_t i = 1; i < HARE_GF2X_TOOM_T + 1U; ++i) {
        W0[i + 1U] = U1[i] ^ U2[i - 1U];
        W4[i + 1U] = V1[i] ^ V2[i - 1U];
    }
    W0[HARE_GF2X_TOOM_T + 1U] = U2[HARE_GF2X_TOOM_T - 1U];
    W4[HARE_GF2X_TOOM_T + 1U] = V2[HARE_GF2X_TOOM_T - 1U];

    for (size_t i = 0; i < HARE_GF2X_TOOM_E; ++i) {
        W3[i] ^= W0[i];
        W2[i] ^= W4[i];
        W0[i] ^= U0[i];
        W4[i] ^= V0[i];
    }

    karatsuba_mul_toom_e(tmp, W3, W2, scratch);
    memcpy(W3, tmp, HARE_GF2X_TOOM_2E * sizeof(uint64_t));
    karatsuba_mul_toom_e(W2, W0, W4, scratch);

    karatsuba_mul_toom_e(W4, U2, V2, scratch);
    karatsuba_mul_toom_e(W0, U0, V0, scratch);

    /* Interpolation. */
    for (size_t i = 0; i < HARE_GF2X_TOOM_2E; ++i) {
        W3[i] ^= W2[i];
    }
    for (size_t i = 0; i < 2U * HARE_GF2X_TOOM_T; ++i) {
        W1[i] ^= W0[i];
    }

    for (size_t i = 0; i < HARE_GF2X_TOOM_2E - 1U; ++i) {
        W2[i] = W2[i + 1U] ^ W0[i + 1U];
    }
    W2[HARE_GF2X_TOOM_2E - 1U] = 0U;

    memset(tmp, 0, sizeof(tmp));
    for (size_t i = 0; i < HARE_GF2X_TOOM_2E; ++i) {
        tmp[i] = W2[i] ^ W3[i] ^ W4[i];
    }
    for (size_t i = 0; i < 2U * HARE_GF2X_TOOM_T; ++i) {
        tmp[i + 3U] ^= W4[i];
    }
    divide_by_y_plus_one(W2, tmp);

    memset(tmp, 0, sizeof(tmp));
    for (size_t i = 0; i < HARE_GF2X_TOOM_2E - 1U; ++i) {
        tmp[i] = W3[i + 1U] ^ W1[i + 1U];
    }
    divide_by_y_plus_one(W3, tmp);

    for (size_t i = 0; i < HARE_GF2X_TOOM_2E; ++i) {
        W1[i] ^= W2[i] ^ W4[i];
        W2[i] ^= W3[i];
    }

    /* Recomposition W0 + W1*Y + W2*Y^2 + W3*Y^3 + W4*Y^4. */
    for (size_t i = 0; i < HARE_GF2X_TOOM_T; ++i) {
        ro[i] = W0[i];
        ro[i + HARE_GF2X_TOOM_T] = W0[i + HARE_GF2X_TOOM_T] ^ W1[i];
        ro[i + 2U * HARE_GF2X_TOOM_T] = W1[i + HARE_GF2X_TOOM_T] ^ W2[i];
        ro[i + 3U * HARE_GF2X_TOOM_T] = W2[i + HARE_GF2X_TOOM_T] ^ W3[i];
        ro[i + 4U * HARE_GF2X_TOOM_T] = W3[i + HARE_GF2X_TOOM_T] ^ W4[i];
        ro[i + 5U * HARE_GF2X_TOOM_T] = W4[i + HARE_GF2X_TOOM_T];
    }
    for (size_t j = 0; j < 4U; ++j) {
        ro[4U * HARE_GF2X_TOOM_T + j] ^= W2[2U * HARE_GF2X_TOOM_T + j];
        ro[5U * HARE_GF2X_TOOM_T + j] ^= W3[2U * HARE_GF2X_TOOM_T + j];
    }

    for (size_t i = 0; i < 2U * VEC_N_SIZE_64; ++i) {
        r[i] = ro[i];
    }
}
#endif

/**
 * @brief Reduce a degree < 2*PARAM_N product modulo X^PARAM_N - 1.
 */
static void reduce_avx2(uint64_t *o, const uint64_t *a) {
#if HARE_GF2X_DEC64 == 0
    for (size_t i = 0; i < VEC_N_SIZE_64; ++i) {
        o[i] = a[i] ^ a[i + VEC_N_SIZE_64];
    }
#else
    size_t i = 0;
    for (; i + 4U <= VEC_N_SIZE_64; i += 4U) {
        __m256i lo = _mm256_loadu_si256((const __m256i *)(const void *)&a[i]);
        __m256i hi0 = _mm256_loadu_si256((const __m256i *)(const void *)&a[i + VEC_N_SIZE_64 - 1U]);
        __m256i hi1 = _mm256_loadu_si256((const __m256i *)(const void *)&a[i + VEC_N_SIZE_64]);
        __m256i folded = _mm256_xor_si256(
            _mm256_srli_epi64(hi0, HARE_GF2X_DEC64),
            _mm256_slli_epi64(hi1, HARE_GF2X_D0));
        __m256i out = _mm256_xor_si256(lo, folded);
        _mm256_storeu_si256((__m256i *)(void *)&o[i], out);
    }

    for (; i < VEC_N_SIZE_64; ++i) {
        const uint64_t r = a[i + VEC_N_SIZE_64 - 1U] >> HARE_GF2X_DEC64;
        const uint64_t carry = a[i + VEC_N_SIZE_64] << HARE_GF2X_D0;
        o[i] = a[i] ^ r ^ carry;
    }
#endif

    o[VEC_N_SIZE_64 - 1U] &= BITMASK(PARAM_N, 64);
}

/**
 * @brief Carry-less multiplication modulo X^PARAM_N - 1.
 */
void vect_mul(uint64_t *o, const uint64_t *a1, const uint64_t *a2) {
    uint64_t unreduced[2U * VEC_N_SIZE_64];

#if HARE_GF2X_USE_TOOM3_TOP
    toom3_top_mul(unreduced, a1, a2);
#else
    uint64_t tmp_buffer[HARE_GF2X_TMP_BUFFER_WORDS];
    karatsuba_mul(unreduced, a1, a2, VEC_N_SIZE_64, tmp_buffer);
#endif
    reduce_avx2(o, unreduced);
}
