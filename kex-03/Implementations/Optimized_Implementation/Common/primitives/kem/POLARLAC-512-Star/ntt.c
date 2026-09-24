/*
 * Active NTT glue layer for PolarLAC AVX2 builds.
 *
 * This file intentionally keeps only the NTT/INTT/pointwise interfaces used by
 * PKE/KEM. The old scalar base-multiplication NTT and the optional conjugate
 * NTT rejection implementation were removed to avoid dead code paths.
 */

#include <stdint.h>
#include <string.h>
#include <immintrin.h>

#include "params.h"
#include "ntt.h"
#include "ntt_avx2.h"

#if defined(__GNUC__) || defined(__clang__)
#define POLARLAC_NTT_ALIGN64 __attribute__((aligned(64)))
#else
#define POLARLAC_NTT_ALIGN64
#endif

static inline int polarlac_ntt_is_aligned32(const void *p)
{
    return (((uintptr_t)p) & 31u) == 0u;
}

void mq_poly_ntt(int16_t *a)
{
    if (polarlac_ntt_is_aligned32(a)) {
        NTT_AVX2(a);
        return;
    }

    int16_t work[RL_KEM_N] POLARLAC_NTT_ALIGN64;
    memcpy(work, a, RL_KEM_N * sizeof(int16_t));
    NTT_AVX2(work);
    memcpy(a, work, RL_KEM_N * sizeof(int16_t));
}

void mq_poly_intt(int16_t *a)
{
    if (polarlac_ntt_is_aligned32(a)) {
        INTT_AVX2_Lazy(a);
        return;
    }

    int16_t work[RL_KEM_N] POLARLAC_NTT_ALIGN64;
    memcpy(work, a, RL_KEM_N * sizeof(int16_t));
    INTT_AVX2_Lazy(work);
    memcpy(a, work, RL_KEM_N * sizeof(int16_t));
}

void mq_poly_pointwise_mul(int16_t *r, int16_t *a, int16_t *b)
{
    if (polarlac_ntt_is_aligned32(r) &&
        polarlac_ntt_is_aligned32(a) &&
        polarlac_ntt_is_aligned32(b)) {
        lazyincom_mul_mod_avx2(a, b, r);
        return;
    }

    int16_t a_work[RL_KEM_N] POLARLAC_NTT_ALIGN64;
    int16_t b_work[RL_KEM_N] POLARLAC_NTT_ALIGN64;
    int16_t r_work[RL_KEM_N] POLARLAC_NTT_ALIGN64;

    memcpy(a_work, a, RL_KEM_N * sizeof(int16_t));
    memcpy(b_work, b, RL_KEM_N * sizeof(int16_t));
    lazyincom_mul_mod_avx2(a_work, b_work, r_work);
    memcpy(r, r_work, RL_KEM_N * sizeof(int16_t));
}

void mq_poly_pointwise_accumulate(int16_t *acc, const int16_t *a, const int16_t *b)
{
    if (polarlac_ntt_is_aligned32(acc) &&
        polarlac_ntt_is_aligned32(a) &&
        polarlac_ntt_is_aligned32(b)) {
        incom_mul_acc_avx2(acc, a, b);
        return;
    }

    int16_t acc_work[RL_KEM_N] POLARLAC_NTT_ALIGN64;
    int16_t a_work[RL_KEM_N] POLARLAC_NTT_ALIGN64;
    int16_t b_work[RL_KEM_N] POLARLAC_NTT_ALIGN64;

    memcpy(acc_work, acc, RL_KEM_N * sizeof(int16_t));
    memcpy(a_work, a, RL_KEM_N * sizeof(int16_t));
    memcpy(b_work, b, RL_KEM_N * sizeof(int16_t));
    incom_mul_acc_avx2(acc_work, a_work, b_work);
    memcpy(acc, acc_work, RL_KEM_N * sizeof(int16_t));
}

void mq_poly_pointwise_accumulate_aligned(int16_t *acc, const int16_t *a, const int16_t *b)
{
    incom_mul_acc_avx2(acc, a, b);
}

void mq_poly_pointwise_muladd2_aligned(int16_t *out,
                                       const int16_t *a0, const int16_t *b0,
                                       const int16_t *a1, const int16_t *b1)
{
    incom_mul_add2_avx2(out, a0, b0, a1, b1);
}
