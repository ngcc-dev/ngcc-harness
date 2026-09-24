/*
Copyright (c) 2026 Yu Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
File Description: Implements polynomial sampling routines for the optimized POLARLAC-128 instance.
*/

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <immintrin.h>
#include "params.h"
#include "ntt.h"
#include "symmetric.h"
#include "poly.h"


#if defined(__GNUC__) || defined(__clang__)
#define POLARLAC_SAMPLE_ALIGN32 __attribute__((aligned(32)))
#else
#define POLARLAC_SAMPLE_ALIGN32
#endif

static inline void sample_bytes_to_i16_avx2(int16_t *dst, const uint8_t *src, unsigned int n)
{
    unsigned int i = 0;
    for (; i + 32 <= n; i += 32) {
        __m256i bytes = _mm256_load_si256((const __m256i *)(const void *)(src + i));
        __m128i lo = _mm256_castsi256_si128(bytes);
        __m128i hi = _mm256_extracti128_si256(bytes, 1);
        __m256i v0 = _mm256_cvtepu8_epi16(lo);
        __m256i v1 = _mm256_cvtepu8_epi16(hi);
        _mm256_store_si256((__m256i *)(void *)(dst + i), v0);
        _mm256_store_si256((__m256i *)(void *)(dst + i + 16), v1);
    }
    for (; i < n; i++) {
        dst[i] = (int16_t)src[i];
    }
}

static inline void sample_bitplanes_to_i16_avx2(int16_t *t, const uint8_t *buf, unsigned int buflen)
{
    for (unsigned int bit = 0; bit < 8; bit++) {
        unsigned int j = 0;
        for (; j + 16 <= buflen; j += 16) {
            __m128i b = _mm_loadu_si128((const __m128i *)(const void *)(buf + j));
            __m256i v = _mm256_cvtepu8_epi16(b);
            v = _mm256_and_si256(_mm256_srli_epi16(v, bit), _mm256_set1_epi16(1));
            _mm256_storeu_si256((__m256i *)(void *)(t + bit * buflen + j), v);
        }
        for (; j < buflen; j++) {
            t[bit * buflen + j] = (int16_t)((buf[j] >> bit) & 1U);
        }
    }
}



static inline void sample_ternary_4planes_avx2(int16_t *a, const int16_t *t)
{
    for (unsigned int i = 0; i < RL_KEM_N; i += 16) {
        __m256i b0 = _mm256_loadu_si256((const __m256i *)(const void *)(t + i));
        __m256i b1 = _mm256_loadu_si256((const __m256i *)(const void *)(t + RL_KEM_N + i));
        __m256i b2 = _mm256_loadu_si256((const __m256i *)(const void *)(t + 2 * RL_KEM_N + i));
        __m256i b3 = _mm256_loadu_si256((const __m256i *)(const void *)(t + 3 * RL_KEM_N + i));
        __m256i pos = _mm256_and_si256(b0, b1);
        __m256i neg = _mm256_and_si256(b2, b3);
        __m256i r = _mm256_sub_epi16(pos, neg);
        _mm256_storeu_si256((__m256i *)(void *)(a + i), r);
    }
}










#if BIT_USE_SHAKE
/* SHAKE/AVX2 sampling path. */

static void poly_generate_uniformQ_avx(polarlac_polymat *a, const uint8_t *seed, int transposed)
{
#if BIT_USE_SHAKE
    uint8_t buf0[RL_KEM_N] POLARLAC_SAMPLE_ALIGN32;
    uint8_t buf1[RL_KEM_N] POLARLAC_SAMPLE_ALIGN32;
    uint8_t buf2[RL_KEM_N] POLARLAC_SAMPLE_ALIGN32;
    uint8_t buf3[RL_KEM_N] POLARLAC_SAMPLE_ALIGN32;
    uint8_t exseed0[PK_SEED_LEN_BYTES + 2];
    uint8_t exseed1[PK_SEED_LEN_BYTES + 2];
    uint8_t exseed2[PK_SEED_LEN_BYTES + 2];
    uint8_t exseed3[PK_SEED_LEN_BYTES + 2];

    memcpy(exseed0, seed, PK_SEED_LEN_BYTES);
    memcpy(exseed1, seed, PK_SEED_LEN_BYTES);
    memcpy(exseed2, seed, PK_SEED_LEN_BYTES);
    memcpy(exseed3, seed, PK_SEED_LEN_BYTES);

    if (transposed) {
        exseed0[PK_SEED_LEN_BYTES] = 0; exseed0[PK_SEED_LEN_BYTES + 1] = 0;
        exseed1[PK_SEED_LEN_BYTES] = 0; exseed1[PK_SEED_LEN_BYTES + 1] = 1;
        exseed2[PK_SEED_LEN_BYTES] = 1; exseed2[PK_SEED_LEN_BYTES + 1] = 0;
        exseed3[PK_SEED_LEN_BYTES] = 1; exseed3[PK_SEED_LEN_BYTES + 1] = 1;
    } else {
        exseed0[PK_SEED_LEN_BYTES] = 0; exseed0[PK_SEED_LEN_BYTES + 1] = 0;
        exseed1[PK_SEED_LEN_BYTES] = 1; exseed1[PK_SEED_LEN_BYTES + 1] = 0;
        exseed2[PK_SEED_LEN_BYTES] = 0; exseed2[PK_SEED_LEN_BYTES + 1] = 1;
        exseed3[PK_SEED_LEN_BYTES] = 1; exseed3[PK_SEED_LEN_BYTES + 1] = 1;
    }

    bit_xof128x4_bytes_nonce0(buf0, buf1, buf2, buf3, RL_KEM_N,
                              exseed0, exseed1, exseed2, exseed3,
                              PK_SEED_LEN_BYTES + 2);

    sample_bytes_to_i16_avx2(a->row[0].vec[0].coeffs, buf0, RL_KEM_N);
    sample_bytes_to_i16_avx2(a->row[0].vec[1].coeffs, buf1, RL_KEM_N);
    sample_bytes_to_i16_avx2(a->row[1].vec[0].coeffs, buf2, RL_KEM_N);
    sample_bytes_to_i16_avx2(a->row[1].vec[1].coeffs, buf3, RL_KEM_N);
#else
    uint8_t buf[RL_KEM_N] POLARLAC_SAMPLE_ALIGN32;
    uint8_t exseed[PK_SEED_LEN_BYTES + 2];

    memcpy(exseed, seed, PK_SEED_LEN_BYTES);

    for (unsigned int i = 0; i < RL_KEM_K; i++) {
        for (unsigned int j = 0; j < RL_KEM_K; j++) {
            if (transposed) {
                exseed[PK_SEED_LEN_BYTES] = (uint8_t)i;
                exseed[PK_SEED_LEN_BYTES + 1] = (uint8_t)j;
            } else {
                exseed[PK_SEED_LEN_BYTES] = (uint8_t)j;
                exseed[PK_SEED_LEN_BYTES + 1] = (uint8_t)i;
            }
            bit_xof128_bytes_nonce0(buf, RL_KEM_N, exseed, PK_SEED_LEN_BYTES + 2);
            sample_bytes_to_i16_avx2(a->row[i].vec[j].coeffs, buf, RL_KEM_N);
        }
    }
#endif
}

// Ternary sampler: {-1:3/16; 0:5/8; 1:3/16}.
static void poly_generate_tenary_avx(int16_t *a, const uint8_t *seed, uint8_t nonce)
{
    uint8_t buf[RL_KEM_N_LEN_BYTES * 4] POLARLAC_SAMPLE_ALIGN32;
    int16_t t[RL_KEM_N * 4] POLARLAC_SAMPLE_ALIGN32;
    xof128_stream stream;

    bit_xof128_stream_init(&stream, seed, KEM_SEED_LEN_BYTES, nonce);
    bit_xof128_stream_squeeze(&stream, buf, RL_KEM_N_LEN_BYTES * 4);
    sample_bitplanes_to_i16_avx2(t, buf, RL_KEM_N_LEN_BYTES * 4);
    sample_ternary_4planes_avx2(a, t);
    bit_xof128_stream_release(&stream);
}
#endif /* BIT_USE_SHAKE */

#if !BIT_USE_SHAKE

/* SM3/pseudoXOF path restored from ordinary C implementation. */
static void poly_generate_uniformQ_c_sm3(polarlac_polymat *a, const uint8_t *seed, int transposed)
{
    unsigned int i, j;
    uint8_t buf[RL_KEM_N] POLARLAC_SAMPLE_ALIGN32;
    uint8_t exseed[PK_SEED_LEN_BYTES + 2] POLARLAC_SAMPLE_ALIGN32;
    xof128_stream stream;

    memcpy(exseed, seed, PK_SEED_LEN_BYTES);

    if(transposed)
    {
        for(i = 0; i < RL_KEM_K; i++)
        {
            exseed[PK_SEED_LEN_BYTES] = (uint8_t)i;

            for(j = 0; j < RL_KEM_K; j++)
            {
                exseed[PK_SEED_LEN_BYTES + 1] = (uint8_t)j;

                bit_xof128_stream_init(&stream, exseed, PK_SEED_LEN_BYTES + 2, 0);
                bit_xof128_stream_squeeze(&stream, buf, RL_KEM_N);

                sample_bytes_to_i16_avx2(a->row[i].vec[j].coeffs, buf, RL_KEM_N);
            }
        }
    }
    else
    {
        for(i = 0; i < RL_KEM_K; i++)
        {
            exseed[PK_SEED_LEN_BYTES + 1] = (uint8_t)i;

            for(j = 0; j < RL_KEM_K; j++)
            {
                exseed[PK_SEED_LEN_BYTES] = (uint8_t)j;

                bit_xof128_stream_init(&stream, exseed, PK_SEED_LEN_BYTES + 2, 0);
                bit_xof128_stream_squeeze(&stream, buf, RL_KEM_N);

                sample_bytes_to_i16_avx2(a->row[i].vec[j].coeffs, buf, RL_KEM_N);
            }
        }
    }

    bit_xof128_stream_release(&stream);
}

static void poly_generate_tenary_c_sm3(int16_t *a, const uint8_t *seed, uint8_t nonce)
{
    uint8_t buf[RL_KEM_N_LEN_BYTES * 4] POLARLAC_SAMPLE_ALIGN32;
    int16_t t[RL_KEM_N * 4] POLARLAC_SAMPLE_ALIGN32;
    xof128_stream stream;

    bit_xof128_stream_init(&stream, seed, KEM_SEED_LEN_BYTES, nonce);
    bit_xof128_stream_squeeze(&stream, buf, RL_KEM_N_LEN_BYTES * 4);
    sample_bitplanes_to_i16_avx2(t, buf, RL_KEM_N_LEN_BYTES * 4);
    sample_ternary_4planes_avx2(a, t);
    bit_xof128_stream_release(&stream);
}

#endif /* !BIT_USE_SHAKE */


void poly_generate_uniformQ(polarlac_polymat *a, const uint8_t *seed, int transposed)
{
#if BIT_USE_SHAKE
    poly_generate_uniformQ_avx(a, seed, transposed);
#else
    poly_generate_uniformQ_c_sm3(a, seed, transposed);
#endif
}

void poly_generate_tenary(int16_t *a, const uint8_t *seed, uint8_t nonce)
{
#if BIT_USE_SHAKE
    poly_generate_tenary_avx(a, seed, nonce);
#else
    poly_generate_tenary_c_sm3(a, seed, nonce);
#endif
}
