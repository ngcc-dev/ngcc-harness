/*
Copyright (c) 2026 Yu Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
File Description: Implements polynomial sampling routines for the optimized POLARLAC-512 instance.
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













static inline void sample_unpack_7bytes_to_8_7bit(uint8_t *dst, const uint8_t *src)
{
    dst[0] = src[0] & 127U;
    dst[1] = src[1] & 127U;
    dst[2] = src[2] & 127U;
    dst[3] = src[3] & 127U;
    dst[4] = src[4] & 127U;
    dst[5] = src[5] & 127U;
    dst[6] = src[6] & 127U;
    dst[7] = (uint8_t)((((src[0] >> 7) & 1U) << 6)
                     | (((src[1] >> 7) & 1U) << 5)
                     | (((src[2] >> 7) & 1U) << 4)
                     | (((src[3] >> 7) & 1U) << 3)
                     | (((src[4] >> 7) & 1U) << 2)
                     | (((src[5] >> 7) & 1U) << 1)
                     |  ((src[6] >> 7) & 1U));
}

static inline void sample_ternary_11_128_avx2(int16_t *a, const uint8_t *buf)
{
    uint8_t kbuf[16] POLARLAC_SAMPLE_ALIGN32;
    const __m256i one = _mm256_set1_epi16(1);
    const __m256i eleven = _mm256_set1_epi16(11);
    const __m256i high = _mm256_set1_epi16(116);
    unsigned int in = 0;

    for (unsigned int j = 0; j < RL_KEM_N; j += 16) {
        sample_unpack_7bytes_to_8_7bit(kbuf, buf + in);
        sample_unpack_7bytes_to_8_7bit(kbuf + 8, buf + in + 7);
        in += 14;

        __m128i kb = _mm_loadu_si128((const __m128i *)(const void *)kbuf);
        __m256i k = _mm256_cvtepu8_epi16(kb);
        __m256i mneg = _mm256_and_si256(_mm256_cmpgt_epi16(eleven, k), one);  /* k < 11 */
        __m256i mpos = _mm256_and_si256(_mm256_cmpgt_epi16(k, high), one);    /* k > 116 */
        __m256i r = _mm256_sub_epi16(mpos, mneg);
        _mm256_storeu_si256((__m256i *)(void *)(a + j), r);
    }
}

#define TERNARY_SAMPLE_BYTES (RL_KEM_N_LEN_BYTES * 7)


#if BIT_USE_SHAKE
/* SHAKE/AVX2 sampling path. */

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


// Ternary sampler: {-1:11/128; 0:106/128; 1:11/128}.
static void poly_generate_tenary_avx(int16_t *a, const uint8_t *seed, uint8_t nonce)
{
    uint8_t buf[TERNARY_SAMPLE_BYTES] POLARLAC_SAMPLE_ALIGN32;
    xof128_stream stream;

    bit_xof128_stream_init(&stream, seed, KEM_SEED_LEN_BYTES, nonce);
    bit_xof128_stream_squeeze(&stream, buf, TERNARY_SAMPLE_BYTES);
    sample_ternary_11_128_avx2(a, buf);
    bit_xof128_stream_release(&stream);
}

static void poly_generate_tenary_x4_avx(int16_t *a0, int16_t *a1, int16_t *a2, int16_t *a3,
                                        const uint8_t *seed,
                                        uint8_t nonce0, uint8_t nonce1,
                                        uint8_t nonce2, uint8_t nonce3)
{
    uint8_t buf0[TERNARY_SAMPLE_BYTES] POLARLAC_SAMPLE_ALIGN32;
    uint8_t buf1[TERNARY_SAMPLE_BYTES] POLARLAC_SAMPLE_ALIGN32;
    uint8_t buf2[TERNARY_SAMPLE_BYTES] POLARLAC_SAMPLE_ALIGN32;
    uint8_t buf3[TERNARY_SAMPLE_BYTES] POLARLAC_SAMPLE_ALIGN32;
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

    bit_xof128x4_bytes(buf0, buf1, buf2, buf3, TERNARY_SAMPLE_BYTES,
                       in0, in1, in2, in3, KEM_SEED_LEN_BYTES + 1);
    sample_ternary_11_128_avx2(a0, buf0);
    sample_ternary_11_128_avx2(a1, buf1);
    sample_ternary_11_128_avx2(a2, buf2);
    sample_ternary_11_128_avx2(a3, buf3);
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
    uint8_t buf[TERNARY_SAMPLE_BYTES];
    xof128_stream stream;

    bit_xof128_stream_init(&stream, seed, KEM_SEED_LEN_BYTES, nonce);
    bit_xof128_stream_squeeze(&stream, buf, TERNARY_SAMPLE_BYTES);
    sample_ternary_11_128_avx2(a, buf);
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
