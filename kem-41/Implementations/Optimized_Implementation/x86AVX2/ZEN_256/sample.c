/*
Copyright (c) 2026 Yu Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
File Description: Declares the ZEN key-encapsulation mechanism layer for the optimized ZEN-256 instance.
*/
#include <stdint.h>
#include <string.h>
#include <immintrin.h>
#include "params.h"
#include "poly.h"

void cbd1(int16_t *r, const uint8_t *buf)
{
    unsigned int i;
    __m256i f0, f1, f2, f3;
    __m256i g0, g1, g2, g3;
    __m256i h0, h1, h2, h3;
    const __m256i mask01 = _mm256_set1_epi32(0x01010101);

    for(i = 0; i < ZEN_N / 128; i++)
    {
        f0 = _mm256_load_si256((const __m256i *)(buf + 32 * i));

        g0 = _mm256_and_si256(f0, mask01);
        g1 = _mm256_srli_epi16(f0, 1);
        g1 = _mm256_and_si256(g1, mask01);
        g0 = _mm256_sub_epi8(g0, g1);

        g1 = _mm256_srli_epi16(f0, 2);
        g2 = _mm256_srli_epi16(f0, 3);
        g1 = _mm256_and_si256(g1, mask01);
        g2 = _mm256_and_si256(g2, mask01);
        g1 = _mm256_sub_epi8(g1, g2);

        g2 = _mm256_srli_epi16(f0, 4);
        g3 = _mm256_srli_epi16(f0, 5);
        g2 = _mm256_and_si256(g2, mask01);
        g3 = _mm256_and_si256(g3, mask01);
        g2 = _mm256_sub_epi8(g2, g3);

        g3 = _mm256_srli_epi16(f0, 6);
        f1 = _mm256_srli_epi16(f0, 7);
        g3 = _mm256_and_si256(g3, mask01);
        f1 = _mm256_and_si256(f1, mask01);
        g3 = _mm256_sub_epi8(g3, f1);

        f0 = _mm256_unpacklo_epi8(g0, g1);
        f1 = _mm256_unpackhi_epi8(g0, g1);
        f2 = _mm256_unpacklo_epi8(g2, g3);
        f3 = _mm256_unpackhi_epi8(g2, g3);

        h0 = _mm256_unpacklo_epi16(f0, f2);
        h1 = _mm256_unpackhi_epi16(f0, f2);
        h2 = _mm256_unpacklo_epi16(f1, f3);
        h3 = _mm256_unpackhi_epi16(f1, f3);

        f0 = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(h0));
        f1 = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(h1));
        f2 = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(h2));
        f3 = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(h3));

        _mm256_store_si256((__m256i *)(r + 128 * i +  0), f0);
        _mm256_store_si256((__m256i *)(r + 128 * i + 16), f1);
        _mm256_store_si256((__m256i *)(r + 128 * i + 32), f2);
        _mm256_store_si256((__m256i *)(r + 128 * i + 48), f3);

        f0 = _mm256_cvtepi8_epi16(_mm256_extracti128_si256(h0, 1));
        f1 = _mm256_cvtepi8_epi16(_mm256_extracti128_si256(h1, 1));
        f2 = _mm256_cvtepi8_epi16(_mm256_extracti128_si256(h2, 1));
        f3 = _mm256_cvtepi8_epi16(_mm256_extracti128_si256(h3, 1));

        _mm256_store_si256((__m256i *)(r + 128 * i +  64), f0);
        _mm256_store_si256((__m256i *)(r + 128 * i +  80), f1);
        _mm256_store_si256((__m256i *)(r + 128 * i +  96), f2);
        _mm256_store_si256((__m256i *)(r + 128 * i + 112), f3);
    }
}

void tenary3_16(int16_t *r, const uint8_t *buf)
{
    unsigned int i;
    ALIGN32 int16_t t[ZEN_N * 4];

    poly_byte2bit_unpack(t, buf, ZEN_N * 4);

    const int16_t *t0 = t;
    const int16_t *t1 = t + ZEN_N;
    const int16_t *t2 = t + 2 * ZEN_N;
    const int16_t *t3 = t + 3 * ZEN_N;

    __m256i a0, a1, a2, a3;
    __m256i b0, b1, b2, b3;
    __m256i c0, c1, c2, c3;
    __m256i d0, d1, d2, d3;

    for(i = 0; i < ZEN_N; i += 64)
    {
        a0 = _mm256_load_si256((const __m256i *)(t0 + i +  0));
        a1 = _mm256_load_si256((const __m256i *)(t0 + i + 16));
        a2 = _mm256_load_si256((const __m256i *)(t0 + i + 32));
        a3 = _mm256_load_si256((const __m256i *)(t0 + i + 48));

        b0 = _mm256_load_si256((const __m256i *)(t1 + i +  0));
        b1 = _mm256_load_si256((const __m256i *)(t1 + i + 16));
        b2 = _mm256_load_si256((const __m256i *)(t1 + i + 32));
        b3 = _mm256_load_si256((const __m256i *)(t1 + i + 48));

        c0 = _mm256_load_si256((const __m256i *)(t2 + i +  0));
        c1 = _mm256_load_si256((const __m256i *)(t2 + i + 16));
        c2 = _mm256_load_si256((const __m256i *)(t2 + i + 32));
        c3 = _mm256_load_si256((const __m256i *)(t2 + i + 48));

        d0 = _mm256_load_si256((const __m256i *)(t3 + i +  0));
        d1 = _mm256_load_si256((const __m256i *)(t3 + i + 16));
        d2 = _mm256_load_si256((const __m256i *)(t3 + i + 32));
        d3 = _mm256_load_si256((const __m256i *)(t3 + i + 48));

        a0 = _mm256_and_si256(a0, b0);
        a1 = _mm256_and_si256(a1, b1);
        a2 = _mm256_and_si256(a2, b2);
        a3 = _mm256_and_si256(a3, b3);

        c0 = _mm256_and_si256(c0, d0);
        c1 = _mm256_and_si256(c1, d1);
        c2 = _mm256_and_si256(c2, d2);
        c3 = _mm256_and_si256(c3, d3);

        a0 = _mm256_sub_epi16(a0, c0);
        a1 = _mm256_sub_epi16(a1, c1);
        a2 = _mm256_sub_epi16(a2, c2);
        a3 = _mm256_sub_epi16(a3, c3);

        _mm256_store_si256((__m256i *)(r + i +  0), a0);
        _mm256_store_si256((__m256i *)(r + i + 16), a1);
        _mm256_store_si256((__m256i *)(r + i + 32), a2);
        _mm256_store_si256((__m256i *)(r + i + 48), a3);
    }
}

void tenary1_8(int16_t *r, const uint8_t *buf)
{
    unsigned int i;
    ALIGN32 int16_t t[ZEN_N * 3];

    poly_byte2bit_unpack(t, buf, ZEN_N * 3);

    const int16_t *t0 = t;
    const int16_t *t1 = t + ZEN_N;
    const int16_t *t2 = t + 2 * ZEN_N;
    __m256i a0, a1, a2, a3;
    __m256i b0, b1, b2, b3;
    __m256i c0, c1, c2, c3;

    for(i = 0; i < ZEN_N; i += 64)
    {
        a0 = _mm256_load_si256((const __m256i *)(t0 + i +  0));
        a1 = _mm256_load_si256((const __m256i *)(t0 + i + 16));
        a2 = _mm256_load_si256((const __m256i *)(t0 + i + 32));
        a3 = _mm256_load_si256((const __m256i *)(t0 + i + 48));

        b0 = _mm256_load_si256((const __m256i *)(t1 + i +  0));
        b1 = _mm256_load_si256((const __m256i *)(t1 + i + 16));
        b2 = _mm256_load_si256((const __m256i *)(t1 + i + 32));
        b3 = _mm256_load_si256((const __m256i *)(t1 + i + 48));

        c0 = _mm256_load_si256((const __m256i *)(t2 + i +  0));
        c1 = _mm256_load_si256((const __m256i *)(t2 + i + 16));
        c2 = _mm256_load_si256((const __m256i *)(t2 + i + 32));
        c3 = _mm256_load_si256((const __m256i *)(t2 + i + 48));

        a0 = _mm256_sub_epi16(a0, b0);
        a1 = _mm256_sub_epi16(a1, b1);
        a2 = _mm256_sub_epi16(a2, b2);
        a3 = _mm256_sub_epi16(a3, b3);

        a0 = _mm256_mullo_epi16(a0, c0);
        a1 = _mm256_mullo_epi16(a1, c1);
        a2 = _mm256_mullo_epi16(a2, c2);
        a3 = _mm256_mullo_epi16(a3, c3);

        _mm256_store_si256((__m256i *)(r + i +  0), a0);
        _mm256_store_si256((__m256i *)(r + i + 16), a1);
        _mm256_store_si256((__m256i *)(r + i + 32), a2);
        _mm256_store_si256((__m256i *)(r + i + 48), a3);
    }
}