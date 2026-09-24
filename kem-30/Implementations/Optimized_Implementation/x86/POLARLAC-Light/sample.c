/*
Copyright (c) 2026 Yu Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
File Description: Implements polynomial sampling routines for the optimized POLARLAC-Light instance.
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

#if BIT_USE_SHAKE
/* SHAKE/AVX2 sampling path. */

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

static inline void sample_ternary_3planes_avx2(int16_t *a, const int16_t *t)
{
    for (unsigned int i = 0; i < RL_KEM_N; i += 16) {
        __m256i b0 = _mm256_loadu_si256((const __m256i *)(const void *)(t + i));
        __m256i b1 = _mm256_loadu_si256((const __m256i *)(const void *)(t + RL_KEM_N + i));
        __m256i b2 = _mm256_loadu_si256((const __m256i *)(const void *)(t + 2 * RL_KEM_N + i));
        __m256i r = _mm256_mullo_epi16(_mm256_sub_epi16(b0, b1), b2);
        _mm256_storeu_si256((__m256i *)(void *)(a + i), r);
    }
}

#define TERNARY_SAMPLE_PLANES 3
#define sample_ternary_planes_avx2 sample_ternary_3planes_avx2













static void poly_generate_uniformQ_avx(polarlac_polymat *a, const uint8_t *seed, int transposed)
{
#if BIT_USE_SHAKE
    uint8_t buf0[RL_KEM_N] POLARLAC_SAMPLE_ALIGN32;
    uint8_t buf1[RL_KEM_N] POLARLAC_SAMPLE_ALIGN32;
    uint8_t buf2[RL_KEM_N] POLARLAC_SAMPLE_ALIGN32;
    uint8_t buf3[RL_KEM_N] POLARLAC_SAMPLE_ALIGN32;
    uint8_t exseed0[PK_SEED_LEN_BYTES + 3];
    uint8_t exseed1[PK_SEED_LEN_BYTES + 3];
    uint8_t exseed2[PK_SEED_LEN_BYTES + 3];
    uint8_t exseed3[PK_SEED_LEN_BYTES + 3];

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

    exseed0[PK_SEED_LEN_BYTES + 2] = 0;
    exseed1[PK_SEED_LEN_BYTES + 2] = 0;
    exseed2[PK_SEED_LEN_BYTES + 2] = 0;
    exseed3[PK_SEED_LEN_BYTES + 2] = 0;

    bit_xof128x4_bytes(buf0, buf1, buf2, buf3, RL_KEM_N,
                       exseed0, exseed1, exseed2, exseed3,
                       PK_SEED_LEN_BYTES + 3);

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


// Ternary sampler: {-1:1/8; 0:3/4; 1:1/8}.
static void poly_generate_tenary_avx(int16_t *a, const uint8_t *seed, uint8_t nonce)
{
    uint8_t buf[RL_KEM_N_LEN_BYTES * 3] POLARLAC_SAMPLE_ALIGN32;
    int16_t t[RL_KEM_N * 3] POLARLAC_SAMPLE_ALIGN32;
    xof128_stream stream;

    bit_xof128_stream_init(&stream, seed, KEM_SEED_LEN_BYTES, nonce);
    bit_xof128_stream_squeeze(&stream, buf, RL_KEM_N_LEN_BYTES * 3);
    sample_bitplanes_to_i16_avx2(t, buf, RL_KEM_N_LEN_BYTES * 3);
    sample_ternary_3planes_avx2(a, t);
    bit_xof128_stream_release(&stream);
}

static void poly_generate_tenary_x4_avx(int16_t *a0, int16_t *a1, int16_t *a2, int16_t *a3,
                                        const uint8_t *seed,
                                        uint8_t nonce0, uint8_t nonce1,
                                        uint8_t nonce2, uint8_t nonce3)
{
    uint8_t buf0[RL_KEM_N_LEN_BYTES * TERNARY_SAMPLE_PLANES] POLARLAC_SAMPLE_ALIGN32;
    uint8_t buf1[RL_KEM_N_LEN_BYTES * TERNARY_SAMPLE_PLANES] POLARLAC_SAMPLE_ALIGN32;
    uint8_t buf2[RL_KEM_N_LEN_BYTES * TERNARY_SAMPLE_PLANES] POLARLAC_SAMPLE_ALIGN32;
    uint8_t buf3[RL_KEM_N_LEN_BYTES * TERNARY_SAMPLE_PLANES] POLARLAC_SAMPLE_ALIGN32;
    int16_t t0[RL_KEM_N * TERNARY_SAMPLE_PLANES] POLARLAC_SAMPLE_ALIGN32;
    int16_t t1[RL_KEM_N * TERNARY_SAMPLE_PLANES] POLARLAC_SAMPLE_ALIGN32;
    int16_t t2[RL_KEM_N * TERNARY_SAMPLE_PLANES] POLARLAC_SAMPLE_ALIGN32;
    int16_t t3[RL_KEM_N * TERNARY_SAMPLE_PLANES] POLARLAC_SAMPLE_ALIGN32;
    uint8_t in0[KEM_SEED_LEN_BYTES + 1];
    uint8_t in1[KEM_SEED_LEN_BYTES + 1];
    uint8_t in2[KEM_SEED_LEN_BYTES + 1];
    uint8_t in3[KEM_SEED_LEN_BYTES + 1];

    memcpy(in0, seed, KEM_SEED_LEN_BYTES);
    memcpy(in1, seed, KEM_SEED_LEN_BYTES);
    memcpy(in2, seed, KEM_SEED_LEN_BYTES);
    memcpy(in3, seed, KEM_SEED_LEN_BYTES);
    in0[KEM_SEED_LEN_BYTES] = nonce0;
    in1[KEM_SEED_LEN_BYTES] = nonce1;
    in2[KEM_SEED_LEN_BYTES] = nonce2;
    in3[KEM_SEED_LEN_BYTES] = nonce3;

    bit_xof128x4_bytes(buf0, buf1, buf2, buf3,
                       RL_KEM_N_LEN_BYTES * TERNARY_SAMPLE_PLANES,
                       in0, in1, in2, in3, KEM_SEED_LEN_BYTES + 1);
    sample_bitplanes_to_i16_avx2(t0, buf0, RL_KEM_N_LEN_BYTES * TERNARY_SAMPLE_PLANES);
    sample_bitplanes_to_i16_avx2(t1, buf1, RL_KEM_N_LEN_BYTES * TERNARY_SAMPLE_PLANES);
    sample_bitplanes_to_i16_avx2(t2, buf2, RL_KEM_N_LEN_BYTES * TERNARY_SAMPLE_PLANES);
    sample_bitplanes_to_i16_avx2(t3, buf3, RL_KEM_N_LEN_BYTES * TERNARY_SAMPLE_PLANES);
    sample_ternary_planes_avx2(a0, t0);
    sample_ternary_planes_avx2(a1, t1);
    sample_ternary_planes_avx2(a2, t2);
    sample_ternary_planes_avx2(a3, t3);
}
#endif /* BIT_USE_SHAKE */

#if !BIT_USE_SHAKE

/* SM3/pseudoXOF path restored from ordinary C implementation. */
static void poly_generate_uniformQ_c_sm3(polarlac_polymat *a, const uint8_t *seed, int transposed)
{
    unsigned int i, j, t;
    uint8_t buf[RL_KEM_N], exseed[PK_SEED_LEN_BYTES+2];
    xof128_stream stream;
    memcpy(exseed, seed, PK_SEED_LEN_BYTES);

    for(i = 0; i < RL_KEM_K; i++)
    {
        for(j = 0; j < RL_KEM_K; j++)
        {
            
            if(transposed)
            {
                exseed[PK_SEED_LEN_BYTES] = i;
                exseed[PK_SEED_LEN_BYTES+1] = j;
            }
            else
            {
                exseed[PK_SEED_LEN_BYTES] = j;
                exseed[PK_SEED_LEN_BYTES+1] = i;
            }
            bit_xof128_stream_init(&stream, exseed, PK_SEED_LEN_BYTES+2, 0);
            bit_xof128_stream_squeeze(&stream, buf, RL_KEM_N);
            for(t = 0; t < RL_KEM_N; t++)
            {
                a->row[i].vec[j].coeffs[t] = (int16_t)buf[t];
            }
        }
    }
    bit_xof128_stream_release(&stream);
}

static void poly_generate_tenary_c_sm3(int16_t *a, const uint8_t *seed, uint8_t nonce)
{
    unsigned int i, j;
    uint8_t buf[RL_KEM_N_LEN_BYTES*3];
    int16_t t[RL_KEM_N*3];
    xof128_stream stream;

    bit_xof128_stream_init(&stream, seed, KEM_SEED_LEN_BYTES, nonce);
    bit_xof128_stream_squeeze(&stream, buf, RL_KEM_N_LEN_BYTES*3);
    
    for (i = 0; i < 8; i++) 
    {
        for (j = 0; j < RL_KEM_N_LEN_BYTES * 3; j++) 
        {
            t[i * (RL_KEM_N_LEN_BYTES * 3) + j] = (int16_t)(buf[j] & 1U);
            buf[j] >>= 1;
        }
    }
    for (i = 0; i < RL_KEM_N; i++) 
    {
        a[i] = (int16_t)((t[i] - t[i + RL_KEM_N]) * t[i + 2*RL_KEM_N]);
    }
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

void poly_generate_tenary_x4(int16_t *a0, int16_t *a1, int16_t *a2, int16_t *a3,
                             const uint8_t *seed,
                             uint8_t nonce0, uint8_t nonce1,
                             uint8_t nonce2, uint8_t nonce3)
{
#if BIT_USE_SHAKE
    poly_generate_tenary_x4_avx(a0, a1, a2, a3, seed, nonce0, nonce1, nonce2, nonce3);
#else
    poly_generate_tenary(a0, seed, nonce0);
    poly_generate_tenary(a1, seed, nonce1);
    poly_generate_tenary(a2, seed, nonce2);
    poly_generate_tenary(a3, seed, nonce3);
#endif
}
