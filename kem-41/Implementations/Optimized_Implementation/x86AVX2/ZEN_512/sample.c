/*
Copyright (c) 2026 Yu Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
File Description: Declares the ZEN key-encapsulation mechanism layer for the optimized ZEN-512 instance.
*/
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <immintrin.h>
#include "params.h"
#include "poly.h"

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

void tenary3_32(int16_t *r, const uint8_t *buf)
{
    unsigned int i;
    ALIGN32 int16_t t[ZEN_N * 5];

    poly_byte2bit_unpack(t, buf, ZEN_N * 5);

    const int16_t *t0 = t;
    const int16_t *t1 = t + ZEN_N;
    const int16_t *t2 = t + 2 * ZEN_N;
    const int16_t *t3 = t + 3 * ZEN_N;
    const int16_t *t4 = t + 4 * ZEN_N;

    __m256i a0, a1, a2, a3;
    __m256i b0, b1, b2, b3;
    __m256i c0, c1, c2, c3;
    __m256i d0, d1, d2, d3;
    __m256i e0, e1, e2, e3;

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

        e0 = _mm256_load_si256((const __m256i *)(t4 + i +  0));
        e1 = _mm256_load_si256((const __m256i *)(t4 + i + 16));
        e2 = _mm256_load_si256((const __m256i *)(t4 + i + 32));
        e3 = _mm256_load_si256((const __m256i *)(t4 + i + 48));

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

        a0 = _mm256_mullo_epi16(a0, e0);
        a1 = _mm256_mullo_epi16(a1, e1);
        a2 = _mm256_mullo_epi16(a2, e2);
        a3 = _mm256_mullo_epi16(a3, e3);

        _mm256_store_si256((__m256i *)(r + i +  0), a0);
        _mm256_store_si256((__m256i *)(r + i + 16), a1);
        _mm256_store_si256((__m256i *)(r + i + 32), a2);
        _mm256_store_si256((__m256i *)(r + i + 48), a3);
    }
}