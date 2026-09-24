/*
Copyright (c) 2026 Yu Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
File Description: Implements polynomial sampling routines for the optimized POLARLAC-Star instance.
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







#define GEN_A_Q5_BYTES 1368
#define GEN_A_Q4_BYTES 30

static inline unsigned int rejection_uniformQ5_u48(uint64_t *r, uint32_t len, const uint8_t *buf, uint32_t buflen)
{
    unsigned int i, j;
    uint64_t x;

    const uint64_t Q5 = 268925323054849ULL; /* 769^5 */

    i = j = 0;

    while(i < len && (j + 6) <= buflen)
    {
        x =  (uint64_t)buf[j + 0]
          | ((uint64_t)buf[j + 1] << 8)
          | ((uint64_t)buf[j + 2] << 16)
          | ((uint64_t)buf[j + 3] << 24)
          | ((uint64_t)buf[j + 4] << 32)
          | ((uint64_t)buf[j + 5] << 40);

        j += 6;

        if(x < Q5)
            r[i++] = x;
    }

    return i;
}

static inline unsigned int rejection_uniformQ4_u39(uint64_t *r, uint32_t len, const uint8_t *buf, uint32_t buflen)
{
    unsigned int i, j;
    uint64_t x;

    const uint64_t Q4 = 349707832321ULL; /* 769^4 */

    i = j = 0;

    while(i < len && (j + 5) <= buflen)
    {
        x =  (uint64_t)buf[j + 0]
          | ((uint64_t)buf[j + 1] << 8)
          | ((uint64_t)buf[j + 2] << 16)
          | ((uint64_t)buf[j + 3] << 24)
          | (((uint64_t)buf[j + 4] & 0x7FULL) << 32);

        j += 5;

        if(x < Q4)
            r[i++] = x;
    }

    return i;
}





static inline void sample_uniformQ_coeffs(int16_t *a, xof128_stream *stream)
{
    unsigned int t;
    uint8_t buf5[GEN_A_Q5_BYTES];
    uint8_t buf4[GEN_A_Q4_BYTES];
    uint64_t tmp[205];
    const uint64_t DIV769_M  = 374811932576999ULL; /* exact for x < 769^5 */

    t = 0;
    while(t < 204)
    {
        bit_xof128_stream_squeeze(stream, buf5, GEN_A_Q5_BYTES);
        t += rejection_uniformQ5_u48(tmp + t, 204 - t, buf5, GEN_A_Q5_BYTES);
    }

    t = 0;
    while(t < 1)
    {
        bit_xof128_stream_squeeze(stream, buf4, GEN_A_Q4_BYTES);
        t += rejection_uniformQ4_u39(tmp + 204, 1, buf4, GEN_A_Q4_BYTES);
    }

    for(unsigned int i = 0; i < 4; i++)
    {
        uint64_t x = tmp[204];
        uint64_t q = (uint64_t)(((__uint128_t)x * DIV769_M) >> 58);
        uint64_t r = x - q * 769ULL;
        a[RL_KEM_N - 4 + i] = (int16_t)r;
        tmp[204] = q;
    }

    unsigned int idx = 0;
    for(unsigned int i = 0; i < RL_KEM_N - 4; i += 5)
    {
        uint64_t x = tmp[idx++];
        for(unsigned int j = 0; j < 5; j++)
        {
            uint64_t q = (uint64_t)(((__uint128_t)x * DIV769_M) >> 58);
            uint64_t r = x - q * 769ULL;
            a[i + j] = (int16_t)r;
            x = q;
        }
    }
}



static void poly_generate_uniformQ_avx(polarlac_polymat *a, const uint8_t *seed, int transposed)
{
    unsigned int i, j;
    uint8_t exseed[PK_SEED_LEN_BYTES + 2];
    xof128_stream stream;

    memcpy(exseed, seed, PK_SEED_LEN_BYTES);

    for(i = 0; i < RL_KEM_K; i++)
    {
        for(j = 0; j < RL_KEM_K; j++)
        {
            if(transposed)
            {
                exseed[PK_SEED_LEN_BYTES]     = i;
                exseed[PK_SEED_LEN_BYTES + 1] = j;
            }
            else
            {
                exseed[PK_SEED_LEN_BYTES]     = j;
                exseed[PK_SEED_LEN_BYTES + 1] = i;
            }

            bit_xof128_stream_init(&stream, exseed, PK_SEED_LEN_BYTES + 2, 0);
            sample_uniformQ_coeffs(a->row[i].vec[j].coeffs, &stream);
            bit_xof128_stream_release(&stream);
        }
    }
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

/* q=769 SM3 C buffer sizes */
#define GEN_A_Q5_BYTES 1368
#define GEN_A_Q4_BYTES 30

/* SM3/pseudoXOF path restored from ordinary C implementation. */
static inline unsigned int rejection_uniformQ5_u48_c_sm3(uint64_t *r, uint32_t len, const uint8_t *buf, uint32_t buflen)
{
    unsigned int i, j;
    uint64_t x;

    const uint64_t Q5 = 268925323054849ULL; /* 769^5 */

    i = j = 0;

    while(i < len && (j + 6) <= buflen)
    {
        x =  (uint64_t)buf[j + 0]
          | ((uint64_t)buf[j + 1] << 8)
          | ((uint64_t)buf[j + 2] << 16)
          | ((uint64_t)buf[j + 3] << 24)
          | ((uint64_t)buf[j + 4] << 32)
          | ((uint64_t)buf[j + 5] << 40);

        j += 6;

        if(x < Q5)
            r[i++] = x;
    }

    return i;
}

static inline unsigned int rejection_uniformQ4_u39_c_sm3(uint64_t *r, uint32_t len, const uint8_t *buf, uint32_t buflen)
{
    unsigned int i, j;
    uint64_t x;

    const uint64_t Q4 = 349707832321ULL; /* 769^4 */

    i = j = 0;

    while(i < len && (j + 5) <= buflen)
    {
        x =  (uint64_t)buf[j + 0]
          | ((uint64_t)buf[j + 1] << 8)
          | ((uint64_t)buf[j + 2] << 16)
          | ((uint64_t)buf[j + 3] << 24)
          | (((uint64_t)buf[j + 4] & 0x7FULL) << 32);

        j += 5;

        if(x < Q4)
            r[i++] = x;
    }

    return i;
}

static inline void pack_uniformQ_tmp_c_sm3(uint8_t *pa, const uint64_t *tmp)
{
    int i, idx;
    uint64_t res[154];
    uint64_t x0, x1, x2, x3;

    memset(res, 0, sizeof(res));

    idx = 0;
    for(i = 0; i < 204; i += 4)
    {
        x0 = tmp[i + 0];
        x1 = tmp[i + 1];
        x2 = tmp[i + 2];
        x3 = tmp[i + 3];

        res[idx++] = x0 | ((x3 & 0xFFFFULL) << 48);
        res[idx++] = x1 | (((x3 >> 16) & 0xFFFFULL) << 48);
        res[idx++] = x2 | (((x3 >> 32) & 0xFFFFULL) << 48);
    }

    res[idx++] = tmp[204];

    memcpy(pa, (const uint8_t *)res, PK_CRT_POLY_BYTES);
}

static inline void sample_uniformQ_packed_c_sm3(uint8_t *pa, xof128_stream *stream)
{
    unsigned int t;
    uint8_t buf5[GEN_A_Q5_BYTES];
    uint8_t buf4[GEN_A_Q4_BYTES];
    uint64_t tmp[205];

    t = 0;
    while(t < 204)
    {
        bit_xof128_stream_squeeze(stream, buf5, GEN_A_Q5_BYTES);
        t += rejection_uniformQ5_u48_c_sm3(tmp + t, 204 - t, buf5, GEN_A_Q5_BYTES);
    }

    t = 0;
    while(t < 1)
    {
        bit_xof128_stream_squeeze(stream, buf4, GEN_A_Q4_BYTES);
        t += rejection_uniformQ4_u39_c_sm3(tmp + 204, 1, buf4, GEN_A_Q4_BYTES);
    }

    pack_uniformQ_tmp_c_sm3(pa, tmp);
}

static inline void poly_unpack_c_sm3(int16_t *a, const uint8_t *pa)
{
    int i, idx;
    uint64_t res[154] = {0};
    uint64_t tmp[205] = {0};
    const uint64_t MASK48    = 0x0000FFFFFFFFFFFFULL;
    const uint64_t MASK39    = 0x0000007FFFFFFFFFULL;

    memcpy((uint8_t *)res, pa, 1229);

    idx = 153;

    /* last packed block: 4 coefficients packed into 39 bits */
    tmp[204] = res[idx--] & MASK39;

    for(i = 203; i > 0; i -= 4)
    {
        tmp[i - 1] = res[idx] & MASK48;
        tmp[i]     = ((res[idx--] >> 48) & 0xFFFFULL) << 32;

        tmp[i - 2] = res[idx] & MASK48;
        tmp[i]    |= ((res[idx--] >> 48) & 0xFFFFULL) << 16;

        tmp[i - 3] = res[idx] & MASK48;
        tmp[i]    |= ((res[idx--] >> 48) & 0xFFFFULL);
    }

    for(i = 0; i < 4; i++)
    {
        a[RL_KEM_N - 4 + i] = (int16_t)(tmp[204] % (uint64_t)RL_KEM_Q);
        tmp[204] /= (uint64_t)RL_KEM_Q;
    }

    idx = 0;
    for(i = 0; i < RL_KEM_N - 4; i += 5)
    {
        uint64_t x;

        x = tmp[idx];

        a[i] = (int16_t)(x % (uint64_t)RL_KEM_Q);
        x /= (uint64_t)RL_KEM_Q;

        a[i + 1] = (int16_t)(x % (uint64_t)RL_KEM_Q);
        x /= (uint64_t)RL_KEM_Q;

        a[i + 2] = (int16_t)(x % (uint64_t)RL_KEM_Q);
        x /= (uint64_t)RL_KEM_Q;

        a[i + 3] = (int16_t)(x % (uint64_t)RL_KEM_Q);
        x /= (uint64_t)RL_KEM_Q;

        a[i + 4] = (int16_t)(x % (uint64_t)RL_KEM_Q);

        idx++;
    }
}

static void poly_generate_uniformQ_c_sm3(polarlac_polymat *a, const uint8_t *seed, int transposed)
{
    unsigned int i, j;
    uint8_t pa[PK_CRT_POLY_BYTES];
    uint8_t exseed[PK_SEED_LEN_BYTES + 2];
    xof128_stream stream;

    memcpy(exseed, seed, PK_SEED_LEN_BYTES);

    for(i = 0; i < RL_KEM_K; i++)
    {
        for(j = 0; j < RL_KEM_K; j++)
        {
            if(transposed)
            {
                exseed[PK_SEED_LEN_BYTES]     = i;
                exseed[PK_SEED_LEN_BYTES + 1] = j;
            }
            else
            {
                exseed[PK_SEED_LEN_BYTES]     = j;
                exseed[PK_SEED_LEN_BYTES + 1] = i;
            }

            bit_xof128_stream_init(&stream, exseed, PK_SEED_LEN_BYTES + 2, 0);

            sample_uniformQ_packed_c_sm3(pa, &stream);

            poly_unpack_c_sm3(a->row[i].vec[j].coeffs, pa);

            bit_xof128_stream_release(&stream);
        }
    }
}

static void poly_generate_tenary_c_sm3(int16_t *a, const uint8_t *seed, uint8_t nonce)
{
    unsigned int i, j;
    uint8_t buf[RL_KEM_N_LEN_BYTES*4];
    int16_t t[RL_KEM_N*4];
    xof128_stream stream;

    bit_xof128_stream_init(&stream, seed, KEM_SEED_LEN_BYTES, nonce);
    bit_xof128_stream_squeeze(&stream, buf, RL_KEM_N_LEN_BYTES*4);
    for (i = 0; i < 8; i++) 
    {
        for (j = 0; j < RL_KEM_N_LEN_BYTES*4; j++) 
        {
            t[i * (RL_KEM_N_LEN_BYTES*4) + j] = (int16_t)(buf[j] & 1U);
            buf[j] >>= 1;
        }
    }
    for (i = 0; i < RL_KEM_N; i++) 
    {
        a[i] = (int16_t)((t[i] & t[i + RL_KEM_N]) - (t[i + 2*RL_KEM_N] & t[i + 3*RL_KEM_N]));
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
