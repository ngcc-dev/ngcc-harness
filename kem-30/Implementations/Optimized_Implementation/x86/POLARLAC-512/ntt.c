/*
Copyright (c) 2026 Ying Liu.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
File Description: Implements the active NTT glue layer for the optimized POLARLAC-512 instance.
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

static inline __m256i avx2_mod_q_epi32_ntt(__m256i a)
{
    const __m256i q = _mm256_set1_epi32((int)RL_KEM_Q);
    const __m256i qinv = _mm256_set1_epi32((int)QINV);
    __m256i u = _mm256_mullo_epi32(a, qinv);
    u = _mm256_srai_epi32(_mm256_slli_epi32(u, 16), 16);
    __m256i t = _mm256_sub_epi32(a, _mm256_mullo_epi32(u, q));
    __m256i r = _mm256_srai_epi32(t, 16);
    r = _mm256_add_epi32(r, _mm256_and_si256(_mm256_srai_epi32(r, 31), q));
    __m256i r_sub = _mm256_sub_epi32(r, q);
    __m256i geq = _mm256_cmpgt_epi32(r_sub, _mm256_set1_epi32(-1));
    return _mm256_blendv_epi8(r, r_sub, geq);
}

static inline __m256i pack_i32_pair_to_i16x16_ntt(__m256i x0, __m256i x1)
{
    __m128i p0 = _mm_packs_epi32(_mm256_castsi256_si128(x0), _mm256_extracti128_si256(x0, 1));
    __m128i p1 = _mm_packs_epi32(_mm256_castsi256_si128(x1), _mm256_extracti128_si256(x1, 1));
    return _mm256_inserti128_si256(_mm256_castsi128_si256(p0), p1, 1);
}

void mq_poly_pointwise_accumulate_aligned(int16_t *acc, const int16_t *a, const int16_t *b)
{
    int16_t prod[RL_KEM_N] POLARLAC_NTT_ALIGN64;

    lazyincom_mul_mod_avx2((int16_t *)a, (int16_t *)b, prod);
    for (int i = 0; i < RL_KEM_N; i += 16) {
        __m256i vacc = _mm256_load_si256((const __m256i *)(const void *)(acc + i));
        __m256i vprod = _mm256_load_si256((const __m256i *)(const void *)(prod + i));
        __m256i a0 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(vacc));
        __m256i a1 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(vacc, 1));
        __m256i b0 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(vprod));
        __m256i b1 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(vprod, 1));
        __m256i r0 = avx2_mod_q_epi32_ntt(_mm256_add_epi32(a0, b0));
        __m256i r1 = avx2_mod_q_epi32_ntt(_mm256_add_epi32(a1, b1));
        _mm256_store_si256((__m256i *)(void *)(acc + i), pack_i32_pair_to_i16x16_ntt(r0, r1));
    }
}

void mq_poly_pointwise_muladd2_aligned(int16_t *out,
                                       const int16_t *a0, const int16_t *b0,
                                       const int16_t *a1, const int16_t *b1)
{
    incom_mul_add2_avx2(out, a0, b0, a1, b1);
}
