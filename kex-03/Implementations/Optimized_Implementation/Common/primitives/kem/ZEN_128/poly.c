/*
Copyright (c) 2026 Yu Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
File Description: Declares the ZEN key-encapsulation mechanism layer for the optimized ZEN-128 instance.
*/
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <immintrin.h>
#include "params.h"
#include "auxfunc.h"
#include "sample.h"
#include "symmetric.h"
#include "ntt.h"

void montmul(__m256i *tmp_c, __m256i *tmp_a, __m256i *tmp_b, __m256i *tmp_Q, __m256i *tmp_QINV)
{
    __m256i tmp_c0, tmp_c1, tmp_x;
    tmp_c0 = _mm256_mullo_epi16(*tmp_a, *tmp_b);
    tmp_c1 = _mm256_mulhi_epi16(*tmp_a, *tmp_b);
    tmp_x = _mm256_mullo_epi16(tmp_c0, *tmp_QINV);
    tmp_x = _mm256_mulhi_epi16(tmp_x, *tmp_Q);
    *tmp_c = _mm256_sub_epi16(tmp_c1, tmp_x);
}

void poly_baseinv_ntt(int16_t *f_inv, const int16_t *f)
{
    int i = 0;
    __m256i tmp_Q = _mm256_set1_epi16(769), tmp_QINV = _mm256_set1_epi16(-767),
            setmont2 = _mm256_set1_epi16(19), setmont3 = _mm256_set1_epi16(173),
            setmont4 = _mm256_set1_epi16(361), 
            set2 = _mm256_set1_epi16(342), set4 = _mm256_set1_epi16(684),
            setn4 = _mm256_set1_epi16(-684), tmp_zeta, tmp_a0, tmp_a1, tmp_a2,
            tmp_a3, tmp_b0, tmp_b1, tmp_b2, tmp_b3, det, tmp_t, tmp_k, det2, det3;
    __m128i det0, det1;
    for(i = 0; i < ZEN_N; i += 64)
    {
        tmp_zeta = _mm256_load_si256((__m256i *)(invdata + i / 4));
        tmp_a0 = _mm256_load_si256((__m256i *)(f + i));
        tmp_a1 = _mm256_load_si256((__m256i *)(f + i + 16));
        tmp_a2 = _mm256_load_si256((__m256i *)(f + i + 32));
        tmp_a3 = _mm256_load_si256((__m256i *)(f + i + 48));

        montmul(&tmp_b0, &tmp_a2, &tmp_a2, &tmp_Q, &tmp_QINV);
        montmul(&tmp_t, &tmp_a1, &tmp_a3, &tmp_Q, &tmp_QINV);
        montmul(&tmp_t,  &tmp_t,  &set2,  &tmp_Q,  &tmp_QINV);
        tmp_b0 = _mm256_add_epi16(tmp_b0, tmp_t);
        montmul(&tmp_b0, &tmp_b0, &tmp_a0, &tmp_Q, &tmp_QINV);
        montmul(&tmp_t, &tmp_a1, &tmp_a1, &tmp_Q, &tmp_QINV);
        montmul(&tmp_t, &tmp_t, &tmp_a2, &tmp_Q, &tmp_QINV);
        tmp_b0 = _mm256_sub_epi16(tmp_b0, tmp_t);
        montmul(&tmp_b0, &tmp_b0, &tmp_zeta, &tmp_Q, &tmp_QINV);
        montmul(&tmp_t, &tmp_a3, &tmp_a3, &tmp_Q, &tmp_QINV);
        montmul(&tmp_t, &tmp_t, &tmp_a2, &tmp_Q, &tmp_QINV);
        montmul(&tmp_t, &tmp_t, &tmp_zeta, &tmp_Q, &tmp_QINV);
        montmul(&tmp_t, &tmp_t, &tmp_zeta, &tmp_Q, &tmp_QINV);
        tmp_b0 = _mm256_sub_epi16(tmp_b0, tmp_t);
        montmul(&tmp_t, &tmp_a0, &tmp_a0, &tmp_Q, &tmp_QINV);
        montmul(&tmp_t, &tmp_t, &tmp_a0, &tmp_Q, &tmp_QINV);
        tmp_b0 = _mm256_sub_epi16(tmp_b0, tmp_t);
        montmul(&tmp_b0, &tmp_b0, &setmont3, &tmp_Q, &tmp_QINV);

        montmul(&tmp_b1, &tmp_a1, &tmp_a2, &tmp_Q, &tmp_QINV);
        montmul(&tmp_t, &tmp_a0, &tmp_a3, &tmp_Q, &tmp_QINV);
        montmul(&tmp_t, &tmp_t, &set2, &tmp_Q, &tmp_QINV);
        tmp_b1 = _mm256_sub_epi16(tmp_b1, tmp_t);
        montmul(&tmp_b1, &tmp_b1, &tmp_a2, &tmp_Q, &tmp_QINV);
        montmul(&tmp_t, &tmp_a1, &tmp_a1, &tmp_Q, &tmp_QINV);
        montmul(&tmp_t, &tmp_t, &tmp_a3, &tmp_Q, &tmp_QINV);
        tmp_b1 = _mm256_sub_epi16(tmp_b1, tmp_t);
        montmul(&tmp_b1, &tmp_b1, &tmp_zeta, &tmp_Q, &tmp_QINV);
        montmul(&tmp_t, &tmp_a3, &tmp_a3, &tmp_Q, &tmp_QINV);
        montmul(&tmp_t, &tmp_t, &tmp_a3, &tmp_Q, &tmp_QINV);
        montmul(&tmp_t, &tmp_t, &tmp_zeta, &tmp_Q, &tmp_QINV);
        montmul(&tmp_t, &tmp_t, &tmp_zeta, &tmp_Q, &tmp_QINV);
        tmp_b1 = _mm256_add_epi16(tmp_b1, tmp_t);
        montmul(&tmp_t, &tmp_a0, &tmp_a0, &tmp_Q, &tmp_QINV);
        montmul(&tmp_t, &tmp_t, &tmp_a1, &tmp_Q, &tmp_QINV);
        tmp_b1 = _mm256_add_epi16(tmp_b1, tmp_t);
        montmul(&tmp_b1, &tmp_b1, &setmont3, &tmp_Q, &tmp_QINV);

        montmul(&tmp_b2, &tmp_a1, &tmp_a3, &tmp_Q, &tmp_QINV);
        montmul(&tmp_b2, &tmp_b2, &set2, &tmp_Q, &tmp_QINV);
        montmul(&tmp_t, &tmp_a2, &tmp_a2, &tmp_Q, &tmp_QINV);
        tmp_b2 = _mm256_sub_epi16(tmp_b2, tmp_t);
        montmul(&tmp_b2, &tmp_b2, &tmp_a2, &tmp_Q, &tmp_QINV);
        montmul(&tmp_t, &tmp_a3, &tmp_a3, &tmp_Q, &tmp_QINV);
        montmul(&tmp_t, &tmp_t, &tmp_a0, &tmp_Q, &tmp_QINV);
        tmp_b2 = _mm256_sub_epi16(tmp_b2, tmp_t);
        montmul(&tmp_b2, &tmp_b2, &tmp_zeta, &tmp_Q, &tmp_QINV);
        montmul(&tmp_t, &tmp_a0, &tmp_a0, &tmp_Q, &tmp_QINV);
        montmul(&tmp_t, &tmp_t, &tmp_a2, &tmp_Q, &tmp_QINV);
        tmp_b2 = _mm256_add_epi16(tmp_b2, tmp_t);
        montmul(&tmp_t, &tmp_a1, &tmp_a1, &tmp_Q, &tmp_QINV);
        montmul(&tmp_t, &tmp_t, &tmp_a0, &tmp_Q, &tmp_QINV);
        tmp_b2 = _mm256_sub_epi16(tmp_b2, tmp_t);
        montmul(&tmp_b2, &tmp_b2, &setmont3, &tmp_Q, &tmp_QINV);

        montmul(&tmp_b3, &tmp_a2, &tmp_a2, &tmp_Q, &tmp_QINV);
        montmul(&tmp_t, &tmp_a1, &tmp_a3, &tmp_Q, &tmp_QINV);
        tmp_b3 = _mm256_sub_epi16(tmp_b3, tmp_t);
        montmul(&tmp_b3, &tmp_b3, &tmp_a3, &tmp_Q, &tmp_QINV);
        montmul(&tmp_b3, &tmp_b3, &tmp_zeta, &tmp_Q, &tmp_QINV);
        montmul(&tmp_t, &tmp_a1, &tmp_a1, &tmp_Q, &tmp_QINV);
        montmul(&tmp_t, &tmp_t, &tmp_a1, &tmp_Q, &tmp_QINV);
        tmp_b3 = _mm256_add_epi16(tmp_b3, tmp_t);
        montmul(&tmp_t, &tmp_a0, &tmp_a2, &tmp_Q, &tmp_QINV);
        montmul(&tmp_t, &tmp_t, &tmp_a1, &tmp_Q, &tmp_QINV);
        montmul(&tmp_t, &tmp_t, &set2, &tmp_Q, &tmp_QINV);
        tmp_b3 = _mm256_sub_epi16(tmp_b3, tmp_t);
        montmul(&tmp_t, &tmp_a0, &tmp_a0, &tmp_Q, &tmp_QINV);
        montmul(&tmp_t, &tmp_t, &tmp_a3, &tmp_Q, &tmp_QINV);
        tmp_b3 = _mm256_add_epi16(tmp_b3, tmp_t);
        montmul(&tmp_b3, &tmp_b3, &setmont3, &tmp_Q, &tmp_QINV);

        montmul(&det, &tmp_a2, &tmp_a2, &tmp_Q, &tmp_QINV);
        montmul(&tmp_t, &tmp_a1, &tmp_a3, &tmp_Q, &tmp_QINV);
        montmul(&tmp_t, &tmp_t, &set4, &tmp_Q, &tmp_QINV);
        det = _mm256_sub_epi16(det, tmp_t);
        montmul(&det, &det, &tmp_a2, &tmp_Q, &tmp_QINV);
        montmul(&det, &det, &tmp_a2, &tmp_Q, &tmp_QINV);
        montmul(&tmp_t, &tmp_a0, &tmp_a2, &tmp_Q, &tmp_QINV);
        montmul(&tmp_t, &tmp_t, &set2, &tmp_Q, &tmp_QINV);
        montmul(&tmp_k, &tmp_a1, &tmp_a1, &tmp_Q, &tmp_QINV);
        tmp_t = _mm256_add_epi16(tmp_t, tmp_k);
        montmul(&tmp_t, &tmp_t, &tmp_a3, &tmp_Q, &tmp_QINV);
        montmul(&tmp_t, &tmp_t, &tmp_a3, &tmp_Q, &tmp_QINV);
        montmul(&tmp_t, &tmp_t, &set2, &tmp_Q, &tmp_QINV);
        det = _mm256_add_epi16(tmp_t, det);
        montmul(&det, &det, &tmp_zeta, &tmp_Q, &tmp_QINV);
        montmul(&det, &det, &tmp_zeta, &tmp_Q, &tmp_QINV);
        montmul(&tmp_t, &tmp_a3, &tmp_a3, &tmp_Q, &tmp_QINV);
        montmul(&tmp_t, &tmp_t, &tmp_a3, &tmp_Q, &tmp_QINV);
        montmul(&tmp_t, &tmp_t, &tmp_a3, &tmp_Q, &tmp_QINV);
        montmul(&tmp_t, &tmp_t, &tmp_zeta, &tmp_Q, &tmp_QINV);
        montmul(&tmp_t, &tmp_t, &tmp_zeta, &tmp_Q, &tmp_QINV);
        montmul(&tmp_t, &tmp_t, &tmp_zeta, &tmp_Q, &tmp_QINV);
        det = _mm256_sub_epi16(tmp_t, det);
        montmul(&tmp_t, &tmp_a0, &tmp_a0, &tmp_Q, &tmp_QINV);
        montmul(&tmp_t, &tmp_t, &tmp_a0, &tmp_Q, &tmp_QINV);
        montmul(&tmp_t, &tmp_t, &tmp_a0, &tmp_Q, &tmp_QINV);
        det = _mm256_sub_epi16(det, tmp_t);
        montmul(&tmp_t, &tmp_a1, &tmp_a3, &tmp_Q, &tmp_QINV);
        montmul(&tmp_t, &tmp_t, &set2, &tmp_Q, &tmp_QINV);
        montmul(&tmp_k, &tmp_a2, &tmp_a2, &tmp_Q, &tmp_QINV);
        tmp_t = _mm256_add_epi16(tmp_t, tmp_k);
        montmul(&tmp_t, &tmp_t, &set2, &tmp_Q, &tmp_QINV);
        montmul(&tmp_t, &tmp_t, &tmp_a0, &tmp_Q, &tmp_QINV);
        montmul(&tmp_t, &tmp_t, &tmp_a0, &tmp_Q, &tmp_QINV);
        montmul(&tmp_k, &tmp_a0, &tmp_a2, &tmp_Q, &tmp_QINV);
        montmul(&tmp_k, &tmp_k, &setn4, &tmp_Q, &tmp_QINV);
        montmul(&det2, &tmp_a1, &tmp_a1, &tmp_Q, &tmp_QINV);
        tmp_k = _mm256_add_epi16(tmp_k, det2);
        montmul(&tmp_k, &tmp_k, &tmp_a1, &tmp_Q, &tmp_QINV);
        montmul(&tmp_k, &tmp_k, &tmp_a1, &tmp_Q, &tmp_QINV);
        tmp_t = _mm256_add_epi16(tmp_t, tmp_k);
        montmul(&tmp_t, &tmp_t, &tmp_zeta, &tmp_Q, &tmp_QINV);
        det = _mm256_add_epi16(det, tmp_t);
        montmul(&det, &det, &setmont4, &tmp_Q, &tmp_QINV);
        det = _mm256_add_epi16(det, _mm256_and_si256(_mm256_srai_epi16(det, 15), tmp_Q));
        
        det0 = _mm256_extracti128_si256(det,0);
        det2 = _mm256_cvtepi16_epi32(det0);
        det2 = _mm256_i32gather_epi32(qinv,det2,sizeof(int32_t));

        det1 = _mm256_extracti128_si256(det,1);
        det3 = _mm256_cvtepi16_epi32(det1);
        det3 = _mm256_i32gather_epi32(qinv,det3,sizeof(int32_t));
        det2 = _mm256_packs_epi32(det2,det3);
        det = _mm256_permute4x64_epi64(det2,0xd8);

        montmul(&tmp_b0, &det, &tmp_b0, &tmp_Q, &tmp_QINV);
        montmul(&tmp_b0, &tmp_b0, &setmont2, &tmp_Q, &tmp_QINV);
        montmul(&tmp_b1, &det, &tmp_b1, &tmp_Q, &tmp_QINV);
        montmul(&tmp_b1, &tmp_b1, &setmont2, &tmp_Q, &tmp_QINV);
        montmul(&tmp_b2, &det, &tmp_b2, &tmp_Q, &tmp_QINV);
        montmul(&tmp_b2, &tmp_b2, &setmont2, &tmp_Q, &tmp_QINV);
        montmul(&tmp_b3, &det, &tmp_b3, &tmp_Q, &tmp_QINV);
        montmul(&tmp_b3, &tmp_b3, &setmont2, &tmp_Q, &tmp_QINV);

        _mm256_storeu_si256((__m256i *)(f_inv + i), tmp_b0);
        _mm256_storeu_si256((__m256i *)(f_inv + i + 16), tmp_b1);
        _mm256_storeu_si256((__m256i *)(f_inv + i + 32), tmp_b2);
        _mm256_storeu_si256((__m256i *)(f_inv + i + 48), tmp_b3);
    }
}

int check_poly_inv_Zq(int16_t *a)
{
    unsigned int i;
    __m256i acc = _mm256_setzero_si256();
    const __m256i zero = _mm256_setzero_si256(), one = _mm256_set1_epi16(1);

    for(i = 0; i < ZEN_N; i += 64)
    {
        __m256i x0, x1, x2, x3, s0, s1, s2, s3, c0, c1, c2, c3;

        x0 = _mm256_load_si256((const __m256i *)(a + i +  0));
        x1 = _mm256_load_si256((const __m256i *)(a + i + 16));
        x2 = _mm256_load_si256((const __m256i *)(a + i + 32));
        x3 = _mm256_load_si256((const __m256i *)(a + i + 48));

        s0 = _mm256_madd_epi16(x0, one);
        s1 = _mm256_madd_epi16(x1, one);
        s2 = _mm256_madd_epi16(x2, one);
        s3 = _mm256_madd_epi16(x3, one);

        s0 = _mm256_hadd_epi32(s0, s0);
        s1 = _mm256_hadd_epi32(s1, s1);
        s2 = _mm256_hadd_epi32(s2, s2);
        s3 = _mm256_hadd_epi32(s3, s3);

        c0 = _mm256_cmpeq_epi32(s0, zero);
        c1 = _mm256_cmpeq_epi32(s1, zero);
        c2 = _mm256_cmpeq_epi32(s2, zero);
        c3 = _mm256_cmpeq_epi32(s3, zero);

        acc = _mm256_or_si256(acc, c0);
        acc = _mm256_or_si256(acc, c1);
        acc = _mm256_or_si256(acc, c2);
        acc = _mm256_or_si256(acc, c3);
    }

    {
        uint32_t m = (uint32_t)_mm256_movemask_epi8(acc);
        return (int)((m | (0u - m)) >> 31);
    }
}

int check_poly_inv_Z2(int16_t *a)
{
    unsigned int i;
    __m256i acc = _mm256_setzero_si256();
    __m128i lo, hi, x;
    uint32_t v;

    for(i = 0; i < ZEN_N4; i += 64)
    {
        __m256i x0, x1, x2, x3;

        x0 = _mm256_load_si256((const __m256i *)(a + i +  0));
        x1 = _mm256_load_si256((const __m256i *)(a + i + 16));
        x2 = _mm256_load_si256((const __m256i *)(a + i + 32));
        x3 = _mm256_load_si256((const __m256i *)(a + i + 48));

        acc = _mm256_xor_si256(acc, x0);
        acc = _mm256_xor_si256(acc, x1);
        acc = _mm256_xor_si256(acc, x2);
        acc = _mm256_xor_si256(acc, x3);
    }

    lo = _mm256_castsi256_si128(acc);
    hi = _mm256_extracti128_si256(acc, 1);
    x = _mm_xor_si128(lo, hi);

    x = _mm_xor_si128(x, _mm_srli_si128(x, 8));
    x = _mm_xor_si128(x, _mm_srli_si128(x, 4));
    x = _mm_xor_si128(x, _mm_srli_si128(x, 2));

    v = (uint32_t)(uint16_t)_mm_extract_epi16(x, 0);

    return (int)((((v | (0u - v)) >> 31) ^ 1u) & 1u);
}

#include <stdint.h>
#include <immintrin.h>

static inline uint64_t pack64_bits_avx2(const int16_t *a)
{
    __m256i x0, x1, x2, x3, y0, y1, one;
    uint32_t m0, m1;

    one = _mm256_set1_epi16(1);

    x0 = _mm256_loadu_si256((const __m256i *)(a +  0));
    x1 = _mm256_loadu_si256((const __m256i *)(a + 16));
    x2 = _mm256_loadu_si256((const __m256i *)(a + 32));
    x3 = _mm256_loadu_si256((const __m256i *)(a + 48));

    x0 = _mm256_and_si256(x0, one);
    x1 = _mm256_and_si256(x1, one);
    x2 = _mm256_and_si256(x2, one);
    x3 = _mm256_and_si256(x3, one);

    y0 = _mm256_packus_epi16(x0, x1);
    y1 = _mm256_packus_epi16(x2, x3);

    y0 = _mm256_permute4x64_epi64(y0, 0xD8);
    y1 = _mm256_permute4x64_epi64(y1, 0xD8);

    y0 = _mm256_slli_epi16(y0, 7);
    y1 = _mm256_slli_epi16(y1, 7);

    m0 = (uint32_t)_mm256_movemask_epi8(y0);
    m1 = (uint32_t)_mm256_movemask_epi8(y1);

    return (uint64_t)m0 | ((uint64_t)m1 << 32);
}

static inline void unpack64_bits_avx2(int16_t *r, uint64_t x)
{
    __m256i bitmask, zero, one, v0, v1, v2, v3;

    bitmask = _mm256_setr_epi16(
        0x0001, 0x0002, 0x0004, 0x0008,
        0x0010, 0x0020, 0x0040, 0x0080,
        0x0100, 0x0200, 0x0400, 0x0800,
        0x1000, 0x2000, 0x4000, (int16_t)0x8000
    );

    zero = _mm256_setzero_si256();
    one  = _mm256_set1_epi16(1);

    v0 = _mm256_set1_epi16((int16_t)(x));
    v1 = _mm256_set1_epi16((int16_t)(x >> 16));
    v2 = _mm256_set1_epi16((int16_t)(x >> 32));
    v3 = _mm256_set1_epi16((int16_t)(x >> 48));

    v0 = _mm256_and_si256(v0, bitmask);
    v1 = _mm256_and_si256(v1, bitmask);
    v2 = _mm256_and_si256(v2, bitmask);
    v3 = _mm256_and_si256(v3, bitmask);

    v0 = _mm256_cmpeq_epi16(v0, zero);
    v1 = _mm256_cmpeq_epi16(v1, zero);
    v2 = _mm256_cmpeq_epi16(v2, zero);
    v3 = _mm256_cmpeq_epi16(v3, zero);

    v0 = _mm256_andnot_si256(v0, one);
    v1 = _mm256_andnot_si256(v1, one);
    v2 = _mm256_andnot_si256(v2, one);
    v3 = _mm256_andnot_si256(v3, one);

    _mm256_storeu_si256((__m256i *)(r +  0), v0);
    _mm256_storeu_si256((__m256i *)(r + 16), v1);
    _mm256_storeu_si256((__m256i *)(r + 32), v2);
    _mm256_storeu_si256((__m256i *)(r + 48), v3);
}

static inline uint64_t parity64_u64(uint64_t x)
{
    x ^= x >> 32;
    x ^= x >> 16;
    x ^= x >> 8;
    x ^= x >> 4;
    x ^= x >> 2;
    x ^= x >> 1;

    return x & 1ULL;
}

/*
 * Pure AVX2 carryless 32x32 -> 64, four lanes in parallel.
 * This uses radix-16 integer multiplication with 4-bit holes.
 * For 32-bit inputs there are only 8 radix-16 digits, so no base-16 carry
 * can cross into the next digit before the parity mask is applied.
 */
static inline void clmul32x4_radix16_avx2_vec(__m256i a, __m256i b, __m256i *r)
{
    __m256i mask32;
    __m256i mask64;
    __m256i a0, a1, a2, a3;
    __m256i b0, b1, b2, b3;
    __m256i p;
    __m256i acc;

    mask32 = _mm256_set1_epi64x(0x11111111ULL);
    mask64 = _mm256_set1_epi64x(0x1111111111111111ULL);

    a0 = _mm256_and_si256(a, mask32);
    a1 = _mm256_and_si256(_mm256_srli_epi64(a, 1), mask32);
    a2 = _mm256_and_si256(_mm256_srli_epi64(a, 2), mask32);
    a3 = _mm256_and_si256(_mm256_srli_epi64(a, 3), mask32);

    b0 = _mm256_and_si256(b, mask32);
    b1 = _mm256_and_si256(_mm256_srli_epi64(b, 1), mask32);
    b2 = _mm256_and_si256(_mm256_srli_epi64(b, 2), mask32);
    b3 = _mm256_and_si256(_mm256_srli_epi64(b, 3), mask32);

    acc = _mm256_setzero_si256();

#define CLMUL32_STEP(A, B, SH) do {                         \
        p = _mm256_mul_epu32((A), (B));                     \
        p = _mm256_and_si256(p, mask64);                    \
        if((SH) != 0)                                       \
        {                                                   \
            p = _mm256_slli_epi64(p, (SH));                 \
        }                                                   \
        acc = _mm256_xor_si256(acc, p);                     \
    } while (0)

    CLMUL32_STEP(a0, b0, 0);
    CLMUL32_STEP(a0, b1, 1);
    CLMUL32_STEP(a0, b2, 2);
    CLMUL32_STEP(a0, b3, 3);

    CLMUL32_STEP(a1, b0, 1);
    CLMUL32_STEP(a1, b1, 2);
    CLMUL32_STEP(a1, b2, 3);
    CLMUL32_STEP(a1, b3, 4);

    CLMUL32_STEP(a2, b0, 2);
    CLMUL32_STEP(a2, b1, 3);
    CLMUL32_STEP(a2, b2, 4);
    CLMUL32_STEP(a2, b3, 5);

    CLMUL32_STEP(a3, b0, 3);
    CLMUL32_STEP(a3, b1, 4);
    CLMUL32_STEP(a3, b2, 5);
    CLMUL32_STEP(a3, b3, 6);

#undef CLMUL32_STEP

    *r = acc;
}

/*
 * Pure AVX2 carryless 64x64 -> 128, four lanes in parallel.
 * Uses 32-bit Karatsuba over the AVX2 radix-16 32x32 primitive.
 */
static inline void clmul64x4_avx2_vec(__m256i a, __m256i b, __m256i *lo, __m256i *hi)
{
    __m256i mask32;
    __m256i a0, a1;
    __m256i b0, b1;
    __m256i z0, z1, z2;

    mask32 = _mm256_set1_epi64x(0xFFFFFFFFULL);

    a0 = _mm256_and_si256(a, mask32);
    a1 = _mm256_srli_epi64(a, 32);

    b0 = _mm256_and_si256(b, mask32);
    b1 = _mm256_srli_epi64(b, 32);

    clmul32x4_radix16_avx2_vec(a0, b0, &z0);
    clmul32x4_radix16_avx2_vec(a1, b1, &z2);
    clmul32x4_radix16_avx2_vec(_mm256_xor_si256(a0, a1),
                               _mm256_xor_si256(b0, b1),
                               &z1);

    z1 = _mm256_xor_si256(z1, z0);
    z1 = _mm256_xor_si256(z1, z2);

    *lo = _mm256_xor_si256(z0, _mm256_slli_epi64(z1, 32));
    *hi = _mm256_xor_si256(z2, _mm256_srli_epi64(z1, 32));
}

static inline void clmul64x4_avx2(uint64_t a0, uint64_t a1,
                                  uint64_t a2, uint64_t a3,
                                  uint64_t b0, uint64_t b1,
                                  uint64_t b2, uint64_t b3,
                                  uint64_t *lo0, uint64_t *hi0,
                                  uint64_t *lo1, uint64_t *hi1,
                                  uint64_t *lo2, uint64_t *hi2,
                                  uint64_t *lo3, uint64_t *hi3)
{
    uint64_t l[4];
    uint64_t h[4];
    __m256i va, vb;
    __m256i vlo, vhi;

    va = _mm256_set_epi64x((long long)a3, (long long)a2, (long long)a1, (long long)a0);
    vb = _mm256_set_epi64x((long long)b3, (long long)b2, (long long)b1, (long long)b0);

    clmul64x4_avx2_vec(va, vb, &vlo, &vhi);

    _mm256_storeu_si256((__m256i *)l, vlo);
    _mm256_storeu_si256((__m256i *)h, vhi);

    *lo0 = l[0];
    *hi0 = h[0];
    *lo1 = l[1];
    *hi1 = h[1];
    *lo2 = l[2];
    *hi2 = h[2];
    *lo3 = l[3];
    *hi3 = h[3];
}

static inline void clmul128_avx2(uint64_t a0, uint64_t a1,
                                 uint64_t b0, uint64_t b1,
                                 uint64_t *p0, uint64_t *p1,
                                 uint64_t *p2, uint64_t *p3)
{
    uint64_t z00, z01;
    uint64_t z20, z21;
    uint64_t t0, t1;
    uint64_t d0, d1;
    uint64_t m0, m1;

    clmul64x4_avx2(a0, a1, a0 ^ a1, 0,
                   b0, b1, b0 ^ b1, 0,
                   &z00, &z01,
                   &z20, &z21,
                   &t0,  &t1,
                   &d0,  &d1);

    m0 = t0 ^ z00 ^ z20;
    m1 = t1 ^ z01 ^ z21;

    *p0 = z00;
    *p1 = z01 ^ m0;
    *p2 = z20 ^ m1;
    *p3 = z21;
}

static inline void clmul128_mod_avx2(uint64_t a0, uint64_t a1,
                                     uint64_t b0, uint64_t b1,
                                     uint64_t *r0, uint64_t *r1)
{
    uint64_t p0, p1, p2, p3;

    clmul128_avx2(a0, a1, b0, b1, &p0, &p1, &p2, &p3);

    *r0 = p0 ^ p2;
    *r1 = p1 ^ p3;
}

static inline void clmul256_avx2(uint64_t a0, uint64_t a1,
                                 uint64_t a2, uint64_t a3,
                                 uint64_t b0, uint64_t b1,
                                 uint64_t b2, uint64_t b3,
                                 uint64_t *p0, uint64_t *p1,
                                 uint64_t *p2, uint64_t *p3,
                                 uint64_t *p4, uint64_t *p5,
                                 uint64_t *p6, uint64_t *p7)
{
    uint64_t z00, z01, z02, z03;
    uint64_t z20, z21, z22, z23;
    uint64_t t0, t1, t2, t3;
    uint64_t m0, m1, m2, m3;

    clmul128_avx2(a0, a1, b0, b1, &z00, &z01, &z02, &z03);
    clmul128_avx2(a2, a3, b2, b3, &z20, &z21, &z22, &z23);
    clmul128_avx2(a0 ^ a2, a1 ^ a3, b0 ^ b2, b1 ^ b3, &t0, &t1, &t2, &t3);

    m0 = t0 ^ z00 ^ z20;
    m1 = t1 ^ z01 ^ z21;
    m2 = t2 ^ z02 ^ z22;
    m3 = t3 ^ z03 ^ z23;

    *p0 = z00;
    *p1 = z01;
    *p2 = z02 ^ m0;
    *p3 = z03 ^ m1;
    *p4 = z20 ^ m2;
    *p5 = z21 ^ m3;
    *p6 = z22;
    *p7 = z23;
}

void mul_in_R2_256(int16_t *a, int16_t *b, int16_t *res)
{
    uint64_t a0, a1, a2, a3;
    uint64_t b0, b1, b2, b3;
    uint64_t z00, z01, z02, z03;
    uint64_t z20, z21, z22, z23;
    uint64_t t0, t1, t2, t3;
    uint64_t m0, m1, m2, m3;
    uint64_t p0, p1, p2, p3, p4, p5, p6, p7;
    uint64_t r0, r1, r2, r3;

    a0 = pack64_bits_avx2(a +   0);
    a1 = pack64_bits_avx2(a +  64);
    a2 = pack64_bits_avx2(a + 128);
    a3 = pack64_bits_avx2(a + 192);

    b0 = pack64_bits_avx2(b +   0);
    b1 = pack64_bits_avx2(b +  64);
    b2 = pack64_bits_avx2(b + 128);
    b3 = pack64_bits_avx2(b + 192);

    clmul128_avx2(a0, a1, b0, b1, &z00, &z01, &z02, &z03);
    clmul128_avx2(a2, a3, b2, b3, &z20, &z21, &z22, &z23);
    clmul128_avx2(a0 ^ a2, a1 ^ a3, b0 ^ b2, b1 ^ b3, &t0, &t1, &t2, &t3);

    m0 = t0 ^ z00 ^ z20;
    m1 = t1 ^ z01 ^ z21;
    m2 = t2 ^ z02 ^ z22;
    m3 = t3 ^ z03 ^ z23;

    p0 = z00;
    p1 = z01;
    p2 = z02 ^ m0;
    p3 = z03 ^ m1;
    p4 = z20 ^ m2;
    p5 = z21 ^ m3;
    p6 = z22;
    p7 = z23;

    r0 = p0 ^ p4;
    r1 = p1 ^ p5;
    r2 = p2 ^ p6;
    r3 = p3 ^ p7;

    unpack64_bits_avx2(res +   0, r0);
    unpack64_bits_avx2(res +  64, r1);
    unpack64_bits_avx2(res + 128, r2);
    unpack64_bits_avx2(res + 192, r3);
}

#define PREFIX64(X) do {                  \
    (X) ^= (X) << 1;                      \
    (X) ^= (X) << 2;                      \
    (X) ^= (X) << 4;                      \
    (X) ^= (X) << 8;                      \
    (X) ^= (X) << 16;                     \
    (X) ^= (X) << 32;                     \
} while (0)

#define PREFIX128_WORDS(K0, K1) do {      \
    PREFIX64(K0);                         \
    PREFIX64(K1);                         \
    (K1) ^= 0ULL - ((K0) >> 63);          \
} while (0)

#define XOR_SHIFT128(SH) do {                                      \
    sh = (SH);                                                     \
    if(sh < 64)                                                    \
    {                                                              \
        x0 = k0 << sh;                                             \
        x1 = (k1 << sh) | (k0 >> (64 - sh));                       \
        k0 ^= x0;                                                  \
        k1 ^= x1;                                                  \
    }                                                              \
    else                                                           \
    {                                                              \
        k1 ^= k0;                                                  \
    }                                                              \
} while (0)

#define BLOCK_PREFIX128(N) do {                                    \
    for(sh = (N); sh < 128; sh <<= 1)                              \
    {                                                              \
        XOR_SHIFT128(sh);                                          \
    }                                                              \
} while (0)

#define MUL_MOD_SMALL_ROTATE(AP, TP, N, MASKN, OUT) do {           \
    unsigned int _i;                                                \
    uint64_t _b;                                                    \
    uint64_t _r;                                                    \
    uint64_t _mask;                                                 \
    uint64_t _wrap;                                                 \
                                                                    \
    _b = (TP) & (MASKN);                                            \
    _r = 0;                                                         \
                                                                    \
    for(_i = 0; _i < (N); _i++)                                    \
    {                                                              \
        _mask = 0ULL - (((AP) >> _i) & 1ULL);                      \
        _r ^= _b & _mask;                                          \
        _wrap = (_b >> ((N) - 1)) & 1ULL;                          \
        _b = ((_b << 1) | _wrap) & (MASKN);                        \
    }                                                              \
                                                                    \
    (OUT) = _r & (MASKN);                                           \
} while (0)

static inline void mul_low_by_f128_rotate(uint64_t bp,
                                          unsigned int bits,
                                          uint64_t f0, uint64_t f1,
                                          uint64_t *r0, uint64_t *r1)
{
    unsigned int i;
    uint64_t g0, g1;
    uint64_t u0, u1;
    uint64_t mask;
    uint64_t wrap;

    g0 = f0;
    g1 = f1;

    *r0 = 0;
    *r1 = 0;

    for(i = 0; i < bits; i++)
    {
        mask = 0ULL - ((bp >> i) & 1ULL);

        *r0 ^= g0 & mask;
        *r1 ^= g1 & mask;

        wrap = g1 >> 63;
        u0 = (g0 << 1) | wrap;
        u1 = (g1 << 1) | (g0 >> 63);

        g0 = u0;
        g1 = u1;
    }
}

static inline void mul64_by_f128_avx2(uint64_t bp,
                                      uint64_t f0, uint64_t f1,
                                      uint64_t *r0, uint64_t *r1)
{
    uint64_t p0, p1;
    uint64_t q0, q1;
    uint64_t d0, d1;
    uint64_t d2, d3;

    clmul64x4_avx2(bp, bp, 0, 0,
                   f0, f1, 0, 0,
                   &p0, &p1,
                   &q0, &q1,
                   &d0, &d1,
                   &d2, &d3);

    *r0 = p0 ^ q1;
    *r1 = p1 ^ q0;
}

#define DO_LEVEL_LT64(N, MASKN) do {                               \
    t64 = k0 ^ k1;                                                 \
                                                                    \
    if((N) <= 32) t64 ^= t64 >> 32;                                \
    if((N) <= 16) t64 ^= t64 >> 16;                                \
    if((N) <= 8)  t64 ^= t64 >> 8;                                 \
    if((N) <= 4)  t64 ^= t64 >> 4;                                 \
    if((N) <= 2)  t64 ^= t64 >> 2;                                 \
                                                                    \
    tp = t64 & (MASKN);                                            \
    ap = inv0 & (MASKN);                                           \
                                                                    \
    MUL_MOD_SMALL_ROTATE(ap, tp, (N), (MASKN), bp);                \
                                                                    \
    mul_low_by_f128_rotate(bp, (N), f0, f1, &r0, &r1);             \
                                                                    \
    k0 ^= r0;                                                      \
    k1 ^= r1;                                                      \
                                                                    \
    BLOCK_PREFIX128(N);                                            \
                                                                    \
    inv0 ^= bp;                                                    \
    inv0 ^= bp << (N);                                             \
} while (0)

#define DO_LEVEL_64() do {                                         \
    t64 = k0 ^ k1;                                                 \
    tp = t64;                                                      \
    ap = inv0;                                                     \
                                                                    \
    MUL_MOD_SMALL_ROTATE(ap, tp, 64, UINT64_MAX, bp);              \
                                                                    \
    mul64_by_f128_avx2(bp, f0, f1, &r0, &r1);                     \
                                                                    \
    k0 ^= r0;                                                      \
    k1 ^= r1;                                                      \
                                                                    \
    BLOCK_PREFIX128(64);                                           \
                                                                    \
    inv0 ^= bp;                                                    \
    inv1 ^= bp;                                                    \
} while (0)

void FastInversion(int16_t *f_inv, int16_t *f)
{
    unsigned int sh;
    uint64_t f0, f1;
    uint64_t k0, k1;
    uint64_t inv0, inv1;
    uint64_t t64, tp, ap, bp;
    uint64_t r0, r1;
    uint64_t acc64, acc, mask;
    uint64_t x0, x1;

    f0 = pack64_bits_avx2(f +  0);
    f1 = pack64_bits_avx2(f + 64);

    k0 = f0;
    k1 = f1;
    PREFIX128_WORDS(k0, k1);

    acc64 = k0 ^ k1;
    acc = parity64_u64(acc64);

    mask = 0ULL - acc;

    k0 ^= f0 & mask;
    k1 ^= f1 & mask;

    PREFIX128_WORDS(k0, k1);

    inv0 = (acc ^ 1ULL) | (acc << 1);
    inv1 = 0;

    DO_LEVEL_LT64(2,  0x0000000000000003ULL);
    DO_LEVEL_LT64(4,  0x000000000000000FULL);
    DO_LEVEL_LT64(8,  0x00000000000000FFULL);
    DO_LEVEL_LT64(16, 0x000000000000FFFFULL);
    DO_LEVEL_LT64(32, 0x00000000FFFFFFFFULL);
    DO_LEVEL_64();

    unpack64_bits_avx2(f_inv +  0, inv0);
    unpack64_bits_avx2(f_inv + 64, inv1);
}

void poly_generate_g(int16_t *a, const uint8_t *seed, uint8_t nonce)
{
    unsigned int i;
    ALIGN32 uint8_t buf[ZEN_N_LEN_BYTES*5];
    ALIGN32 int16_t t[ZEN_N*2];
    zen_pseudoXOF(ZEN_N*5, seed, SEED_LEN_BYTES*8, buf, nonce);
    cbd1(t, buf);
    tenary1_8(t+ZEN_N, buf+ZEN_N_LEN_BYTES*2);
    for(i = 0; i < ZEN_N; i++)
    {
        a[i] = t[i] + t[i+ZEN_N];
    }
}

void poly_generate_f(int16_t *a, const uint8_t *seed, uint8_t nonce)
{
    ALIGN32 uint8_t buf[ZEN_N_LEN_BYTES*2];

    zen_pseudoXOF(ZEN_N*2, seed, SEED_LEN_BYTES*8, buf, nonce);
    cbd1(a, buf);
}

void poly_generate_s(int16_t *a, const uint8_t *seed, uint8_t nonce)
{
    unsigned int i;
    ALIGN32 uint8_t buf[ZEN_N_LEN_BYTES*7];
    ALIGN32 int16_t t[ZEN_N*2];
    zen_pseudoXOF(ZEN_N*7, seed, SEED_LEN_BYTES*8, buf, nonce);
    cbd1(t, buf);
    tenary3_32(t+ZEN_N, buf+ZEN_N_LEN_BYTES*2);
    for(i = 0; i < ZEN_N; i++)
    {
        a[i] = t[i] + t[i+ZEN_N];
    }
}

void poly_generate_e(int16_t *a, const uint8_t *seed, uint8_t nonce)
{
    ALIGN32 uint8_t buf[ZEN_N_LEN_BYTES*4];

    zen_pseudoXOF(ZEN_N*4, seed, SEED_LEN_BYTES*8, buf, nonce);
    cbd2(a, buf);
}


void poly_bit2byte_pack(uint8_t *pa, const int16_t *a, const unsigned int n)
{
    unsigned int i, j;
    const unsigned int nbytes = n >> 3;
    const __m256i mask01 = _mm256_set1_epi16(1);

    for(i = 0; i < nbytes; i += 32)
    {
        uint32_t m[8];

        for(j = 0; j < 8; j++)
        {
            const int16_t *src = a + ((i + 4 * j) << 3);
            __m256i x0, x1, y;

            x0 = _mm256_loadu_si256((const __m256i *)(src +  0));
            x1 = _mm256_loadu_si256((const __m256i *)(src + 16));

            x0 = _mm256_and_si256(x0, mask01);
            x1 = _mm256_and_si256(x1, mask01);

            y = _mm256_packus_epi16(x0, x1);
            y = _mm256_permute4x64_epi64(y, 0xD8);
            y = _mm256_slli_epi16(y, 7);

            m[j] = (uint32_t)_mm256_movemask_epi8(y);
        }

        _mm256_storeu_si256((__m256i *)(pa + i),
            _mm256_setr_epi32(
                (int)m[0], (int)m[1], (int)m[2], (int)m[3],
                (int)m[4], (int)m[5], (int)m[6], (int)m[7]
            )
        );
    }
}

void poly_byte2bit_unpack(int16_t *a, const uint8_t *pa, const unsigned int n)
{
    unsigned int i, j;
    const unsigned int nbytes = n >> 3;

    const __m256i zero = _mm256_setzero_si256(), one = _mm256_set1_epi16(1),
                  bitmask = _mm256_setr_epi16(
        0x0001, 0x0002, 0x0004, 0x0008,
        0x0010, 0x0020, 0x0040, 0x0080,
        0x0100, 0x0200, 0x0400, 0x0800,
        0x1000, 0x2000, 0x4000, (int16_t)0x8000
    );

    for(i = 0; i < nbytes; i += 32)
    {
        for(j = 0; j < 8; j++)
        {
            uint32_t w;
            __m256i x0, x1;
            int16_t *dst = a + ((i + 4 * j) << 3);

            w  = (uint32_t)pa[i + 4 * j + 0];
            w |= (uint32_t)pa[i + 4 * j + 1] << 8;
            w |= (uint32_t)pa[i + 4 * j + 2] << 16;
            w |= (uint32_t)pa[i + 4 * j + 3] << 24;

            x0 = _mm256_set1_epi16((int16_t)(w & 0xFFFFu));
            x1 = _mm256_set1_epi16((int16_t)(w >> 16));

            x0 = _mm256_and_si256(x0, bitmask);
            x1 = _mm256_and_si256(x1, bitmask);

            x0 = _mm256_cmpeq_epi16(x0, zero);
            x1 = _mm256_cmpeq_epi16(x1, zero);

            x0 = _mm256_andnot_si256(x0, one);
            x1 = _mm256_andnot_si256(x1, one);

            _mm256_storeu_si256((__m256i *)(dst +  0), x0);
            _mm256_storeu_si256((__m256i *)(dst + 16), x1);
        }
    }
}

void poly_secretkey_pack(uint8_t *ss, const int16_t *a)
{
    unsigned int i, k;
    const __m256i mask01 = _mm256_set1_epi16(1);

    for(i = 0; i < ZEN_N; i += 32)
    {
        const int16_t *src = a + i;
        uint8_t *dst = ss + (i / 32) * 40;

        __m256i x0 = _mm256_loadu_si256((const __m256i *)(src +  0)),
                x1 = _mm256_loadu_si256((const __m256i *)(src + 16));

        uint32_t w[10];

        for(k = 0; k < 10; k++)
        {
            __m128i cnt = _mm_cvtsi32_si128((int)k);
            __m256i y0, y1, y;

            y0 = _mm256_srl_epi16(x0, cnt);
            y1 = _mm256_srl_epi16(x1, cnt);

            y0 = _mm256_and_si256(y0, mask01);
            y1 = _mm256_and_si256(y1, mask01);

            y = _mm256_packus_epi16(y0, y1);
            y = _mm256_permute4x64_epi64(y, 0xD8);
            y = _mm256_slli_epi16(y, 7);

            w[k] = (uint32_t)_mm256_movemask_epi8(y);
        }

        _mm256_storeu_si256((__m256i *)(dst + 0),
            _mm256_setr_epi32(
                (int)w[0], (int)w[1], (int)w[2], (int)w[3],
                (int)w[4], (int)w[5], (int)w[6], (int)w[7]
            )
        );

        _mm_storel_epi64((__m128i *)(dst + 32),
            _mm_setr_epi32((int)w[8], (int)w[9], 0, 0)
        );
    }
}

void poly_secretkey_unpack(int16_t *a, const uint8_t *ss)
{
    unsigned int i, k;
    const __m256i zero = _mm256_setzero_si256(), one = _mm256_set1_epi16(1),
                  bitmask = _mm256_setr_epi16(
        0x0001, 0x0002, 0x0004, 0x0008,
        0x0010, 0x0020, 0x0040, 0x0080,
        0x0100, 0x0200, 0x0400, 0x0800,
        0x1000, 0x2000, 0x4000, (int16_t)0x8000
    );

    for(i = 0; i < ZEN_N; i += 32)
    {
        int16_t *dst = a + i;
        const uint8_t *src = ss + (i / 32) * 40;

        __m256i acc0 = _mm256_setzero_si256();
        __m256i acc1 = _mm256_setzero_si256();

        for(k = 0; k < 10; k++)
        {
            uint32_t w;
            __m128i cnt;
            __m256i x0, x1;

            w  = (uint32_t)src[4 * k + 0];
            w |= (uint32_t)src[4 * k + 1] << 8;
            w |= (uint32_t)src[4 * k + 2] << 16;
            w |= (uint32_t)src[4 * k + 3] << 24;

            cnt = _mm_cvtsi32_si128((int)k);

            x0 = _mm256_set1_epi16((int16_t)(w & 0xFFFFu));
            x1 = _mm256_set1_epi16((int16_t)(w >> 16));

            x0 = _mm256_and_si256(x0, bitmask);
            x1 = _mm256_and_si256(x1, bitmask);

            x0 = _mm256_cmpeq_epi16(x0, zero);
            x1 = _mm256_cmpeq_epi16(x1, zero);

            x0 = _mm256_andnot_si256(x0, one);
            x1 = _mm256_andnot_si256(x1, one);

            x0 = _mm256_sll_epi16(x0, cnt);
            x1 = _mm256_sll_epi16(x1, cnt);

            acc0 = _mm256_or_si256(acc0, x0);
            acc1 = _mm256_or_si256(acc1, x1);
        }

        _mm256_storeu_si256((__m256i *)(dst +  0), acc0);
        _mm256_storeu_si256((__m256i *)(dst + 16), acc1);
    }
}

static const uint64_t pack_table[] = 
{
    1, 769, 591361, 454756609, 349707832321
};

void poly_publickey_pack(uint8_t *pa, const int16_t *a)
{
    int i, idx;
    uint64_t tmp[103] = {0}, res[77] = {0};

    idx = 0;
    for(i = 0; i < ZEN_N - 2; i += 5)
    {
        tmp[idx] =
              (uint64_t)(uint16_t)a[i]
            + (uint64_t)(uint16_t)a[i + 1] * pack_table[1]
            + (uint64_t)(uint16_t)a[i + 2] * pack_table[2]
            + (uint64_t)(uint16_t)a[i + 3] * pack_table[3]
            + (uint64_t)(uint16_t)a[i + 4] * pack_table[4];
        idx++;
    }

    tmp[102] =
          (uint64_t)(uint16_t)a[ZEN_N - 2]
        + (uint64_t)(uint16_t)a[ZEN_N - 1] * pack_table[1];

    idx = 0;
    for(i = 0; i < 100; i += 4)
    {
        uint64_t x0 = tmp[i];
        uint64_t x1 = tmp[i + 1];
        uint64_t x2 = tmp[i + 2];
        uint64_t x3 = tmp[i + 3];

        res[idx++] = x0 | ((x3 & 0xFFFFULL) << 48);
        res[idx++] = x1 | (((x3 >> 16) & 0xFFFFULL) << 48);
        res[idx++] = x2 | (((x3 >> 32) & 0xFFFFULL) << 48);
    }

    res[idx++] = tmp[100] | ((tmp[102] & 0xFFFFULL) << 48);
    res[idx++] = tmp[101] | (((tmp[102] >> 16) & 0xFULL) << 48);

    memcpy(pa, (const uint8_t *)res, ZEN_INDCPA_PUBLICKEY_LEN_BYTES);
}

void poly_publickey_unpack(int16_t *a, const uint8_t *pa)
{
    int i, idx;
    uint64_t res[77] = {0}, tmp[103] = {0};
    const uint64_t MASK48 = 0x0000FFFFFFFFFFFFULL, DIV769_M = 374811932576999ULL;

    memcpy((uint8_t *)res, pa, ZEN_INDCPA_PUBLICKEY_LEN_BYTES);

    idx = 76;

    tmp[101] = res[idx] & MASK48;
    tmp[102] = ((res[idx--] >> 48) & 0xFULL) << 16;

    tmp[100] = res[idx] & MASK48;
    tmp[102] |= ((res[idx--] >> 48) & 0xFFFFULL);

    for(i = 99; i > 0; i -= 4)
    {
        tmp[i - 1] = res[idx] & MASK48;
        tmp[i]     = ((res[idx--] >> 48) & 0xFFFFULL) << 32;

        tmp[i - 2] = res[idx] & MASK48;
        tmp[i]    |= ((res[idx--] >> 48) & 0xFFFFULL) << 16;

        tmp[i - 3] = res[idx] & MASK48;
        tmp[i]    |= ((res[idx--] >> 48) & 0xFFFFULL);
    }

    for(i = 0; i < 2; i++)
    {
        uint64_t x = tmp[102];
        uint64_t q = (uint64_t)(((__uint128_t)x * DIV769_M) >> 58);
        uint64_t r = x - q * 769ULL;

        a[ZEN_N - 2 + i] = (int16_t)r;
        tmp[102] = q;
    }

    idx = 0;
    for(i = 0; i < ZEN_N - 2; i += 5)
    {
        uint64_t x, q, r;

        x = tmp[idx];

        q = (uint64_t)(((__uint128_t)x * DIV769_M) >> 58);
        r = x - q * 769ULL;
        a[i] = (int16_t)r;
        x = q;

        q = (uint64_t)(((__uint128_t)x * DIV769_M) >> 58);
        r = x - q * 769ULL;
        a[i + 1] = (int16_t)r;
        x = q;

        q = (uint64_t)(((__uint128_t)x * DIV769_M) >> 58);
        r = x - q * 769ULL;
        a[i + 2] = (int16_t)r;
        x = q;

        q = (uint64_t)(((__uint128_t)x * DIV769_M) >> 58);
        r = x - q * 769ULL;
        a[i + 3] = (int16_t)r;
        x = q;

        q = (uint64_t)(((__uint128_t)x * DIV769_M) >> 58);
        r = x - q * 769ULL;
        a[i + 4] = (int16_t)r;

        idx++;
    }
}

void poly_ciphertext_pack(uint8_t *pa, const int16_t *a)
{
    unsigned int i;
    const __m256i maskff = _mm256_set1_epi16(0x00FF);
    __m256i x0, x1, y;

    for(i = 0; i < ZEN_N; i += 32)
    {
        x0 = _mm256_load_si256((const __m256i *)(a + i +  0));
        x1 = _mm256_load_si256((const __m256i *)(a + i + 16));

        x0 = _mm256_and_si256(x0, maskff);
        x1 = _mm256_and_si256(x1, maskff);

        y = _mm256_packus_epi16(x0, x1);
        y = _mm256_permute4x64_epi64(y, 0xD8);

        _mm256_store_si256((__m256i *)(pa + i), y);
    }
}

void poly_ciphertext_unpack(int16_t *a, const uint8_t *pa)
{
    unsigned int i;
    __m256i x, y0, y1;

    for(i = 0; i < ZEN_N; i += 32)
    {
        x = _mm256_load_si256((const __m256i *)(pa + i));

        y0 = _mm256_cvtepu8_epi16(_mm256_castsi256_si128(x));
        y1 = _mm256_cvtepu8_epi16(_mm256_extracti128_si256(x, 1));

        _mm256_store_si256((__m256i *)(a + i +  0), y0);
        _mm256_store_si256((__m256i *)(a + i + 16), y1);
    }
}

void poly_compress(int16_t *a)
{
    unsigned int i;

    const __m256i add384 = _mm256_set1_epi32(384), mul10908 = _mm256_set1_epi32(10908),
                  mask255 = _mm256_set1_epi16(0x00FF);
    __m256i v, lo, hi, r;

    for(i = 0; i < ZEN_N; i += 16)
    {
        v = _mm256_load_si256((const __m256i *)(a + i));

        lo = _mm256_cvtepu16_epi32(_mm256_castsi256_si128(v));
        hi = _mm256_cvtepu16_epi32(_mm256_extracti128_si256(v, 1));

        lo = _mm256_slli_epi32(lo, 8);
        hi = _mm256_slli_epi32(hi, 8);

        lo = _mm256_add_epi32(lo, add384);
        hi = _mm256_add_epi32(hi, add384);

        lo = _mm256_mullo_epi32(lo, mul10908);
        hi = _mm256_mullo_epi32(hi, mul10908);

        lo = _mm256_srli_epi32(lo, 23);
        hi = _mm256_srli_epi32(hi, 23);

        r = _mm256_packus_epi32(lo, hi);
        r = _mm256_permute4x64_epi64(r, 0xD8);
        r = _mm256_and_si256(r, mask255);

        _mm256_store_si256((__m256i *)(a + i), r);
    }
}

void poly_decompress(int16_t *a)
{
    unsigned int i;

    const __m256i q769 = _mm256_set1_epi32(769), add128 = _mm256_set1_epi32(128);
    __m256i v, lo, hi, r;

    for(i = 0; i < ZEN_N; i += 16)
    {
        v = _mm256_load_si256((const __m256i *)(a + i));

        lo = _mm256_cvtepu16_epi32(_mm256_castsi256_si128(v));
        hi = _mm256_cvtepu16_epi32(_mm256_extracti128_si256(v, 1));

        lo = _mm256_mullo_epi32(lo, q769);
        hi = _mm256_mullo_epi32(hi, q769);

        lo = _mm256_add_epi32(lo, add128);
        hi = _mm256_add_epi32(hi, add128);

        lo = _mm256_srli_epi32(lo, 8);
        hi = _mm256_srli_epi32(hi, 8);

        r = _mm256_packus_epi32(lo, hi);
        r = _mm256_permute4x64_epi64(r, 0xD8);

        _mm256_store_si256((__m256i *)(a + i), r);
    }
}
