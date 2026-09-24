/*
Copyright (c) 2026 Yu Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
File Description: Declares the ZEN key-encapsulation mechanism layer for the optimized ZEN-512 instance.
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

void poly_basemul_ntt(int16_t *r, int16_t *a, int16_t *b, int16_t *muldata)
{
    int i, j, k;
    __m256i zeta, prod, tmp, t0;
    __m256i av[16], bv[16], as[8], bs[8];
    __m256i p0[15], p1[15], pm[15], cross[15];
    __m256i tmp_Q = _mm256_set1_epi16(769);
    __m256i tmp_QINV = _mm256_set1_epi16(-767);
    __m256i set1 = _mm256_set1_epi16(171);
    __m256i zero = _mm256_setzero_si256();

    for(i = 0; i < ZEN_N; i += 256)
    {
        zeta = _mm256_load_si256((__m256i *)(muldata + i / 16));

        av[0] = _mm256_load_si256((__m256i *)(a + i));
        av[1] = _mm256_load_si256((__m256i *)(a + i + 16));
        av[2] = _mm256_load_si256((__m256i *)(a + i + 32));
        av[3] = _mm256_load_si256((__m256i *)(a + i + 48));
        av[4] = _mm256_load_si256((__m256i *)(a + i + 128));
        av[5] = _mm256_load_si256((__m256i *)(a + i + 144));
        av[6] = _mm256_load_si256((__m256i *)(a + i + 160));
        av[7] = _mm256_load_si256((__m256i *)(a + i + 176));
        av[8] = _mm256_load_si256((__m256i *)(a + i + 64));
        av[9] = _mm256_load_si256((__m256i *)(a + i + 80));
        av[10] = _mm256_load_si256((__m256i *)(a + i + 96));
        av[11] = _mm256_load_si256((__m256i *)(a + i + 112));
        av[12] = _mm256_load_si256((__m256i *)(a + i + 192));
        av[13] = _mm256_load_si256((__m256i *)(a + i + 208));
        av[14] = _mm256_load_si256((__m256i *)(a + i + 224));
        av[15] = _mm256_load_si256((__m256i *)(a + i + 240));

        bv[0] = _mm256_load_si256((__m256i *)(b + i));
        bv[1] = _mm256_load_si256((__m256i *)(b + i + 16));
        bv[2] = _mm256_load_si256((__m256i *)(b + i + 32));
        bv[3] = _mm256_load_si256((__m256i *)(b + i + 48));
        bv[4] = _mm256_load_si256((__m256i *)(b + i + 128));
        bv[5] = _mm256_load_si256((__m256i *)(b + i + 144));
        bv[6] = _mm256_load_si256((__m256i *)(b + i + 160));
        bv[7] = _mm256_load_si256((__m256i *)(b + i + 176));
        bv[8] = _mm256_load_si256((__m256i *)(b + i + 64));
        bv[9] = _mm256_load_si256((__m256i *)(b + i + 80));
        bv[10] = _mm256_load_si256((__m256i *)(b + i + 96));
        bv[11] = _mm256_load_si256((__m256i *)(b + i + 112));
        bv[12] = _mm256_load_si256((__m256i *)(b + i + 192));
        bv[13] = _mm256_load_si256((__m256i *)(b + i + 208));
        bv[14] = _mm256_load_si256((__m256i *)(b + i + 224));
        bv[15] = _mm256_load_si256((__m256i *)(b + i + 240));

        for(j = 0; j < 8; j++)
        {
            as[j] = _mm256_add_epi16(av[j], av[j + 8]);
            bs[j] = _mm256_add_epi16(bv[j], bv[j + 8]);
        }

        for(j = 0; j < 15; j++)
        {
            p0[j] = zero;
            p1[j] = zero;
            pm[j] = zero;
        }

        for(j = 0; j < 8; j++)
        {
            for(k = 0; k < 8; k++)
            {
                montmul(&prod, &av[j], &bv[k], &tmp_Q, &tmp_QINV);
                p0[j + k] = _mm256_add_epi16(p0[j + k], prod);

                montmul(&prod, &av[j + 8], &bv[k + 8], &tmp_Q, &tmp_QINV);
                p1[j + k] = _mm256_add_epi16(p1[j + k], prod);

                montmul(&prod, &as[j], &bs[k], &tmp_Q, &tmp_QINV);
                pm[j + k] = _mm256_add_epi16(pm[j + k], prod);
            }
        }

        for(j = 0; j < 15; j++)
        {
            cross[j] = _mm256_sub_epi16(_mm256_sub_epi16(pm[j], p0[j]), p1[j]);
        }

        montmul(&tmp, &p1[0], &zeta, &tmp_Q, &tmp_QINV);
        montmul(&prod, &cross[8], &zeta, &tmp_Q, &tmp_QINV);
        t0 = _mm256_add_epi16(_mm256_add_epi16(p0[0], tmp), prod);
        montmul(&t0, &t0, &set1, &tmp_Q, &tmp_QINV);
        _mm256_store_si256((__m256i *)(r + i), t0);

        montmul(&tmp, &p1[1], &zeta, &tmp_Q, &tmp_QINV);
        montmul(&prod, &cross[9], &zeta, &tmp_Q, &tmp_QINV);
        t0 = _mm256_add_epi16(_mm256_add_epi16(p0[1], tmp), prod);
        montmul(&t0, &t0, &set1, &tmp_Q, &tmp_QINV);
        _mm256_store_si256((__m256i *)(r + i + 16), t0);

        montmul(&tmp, &p1[2], &zeta, &tmp_Q, &tmp_QINV);
        montmul(&prod, &cross[10], &zeta, &tmp_Q, &tmp_QINV);
        t0 = _mm256_add_epi16(_mm256_add_epi16(p0[2], tmp), prod);
        montmul(&t0, &t0, &set1, &tmp_Q, &tmp_QINV);
        _mm256_store_si256((__m256i *)(r + i + 32), t0);

        montmul(&tmp, &p1[3], &zeta, &tmp_Q, &tmp_QINV);
        montmul(&prod, &cross[11], &zeta, &tmp_Q, &tmp_QINV);
        t0 = _mm256_add_epi16(_mm256_add_epi16(p0[3], tmp), prod);
        montmul(&t0, &t0, &set1, &tmp_Q, &tmp_QINV);
        _mm256_store_si256((__m256i *)(r + i + 48), t0);

        montmul(&tmp, &p1[4], &zeta, &tmp_Q, &tmp_QINV);
        montmul(&prod, &cross[12], &zeta, &tmp_Q, &tmp_QINV);
        t0 = _mm256_add_epi16(_mm256_add_epi16(p0[4], tmp), prod);
        montmul(&t0, &t0, &set1, &tmp_Q, &tmp_QINV);
        _mm256_store_si256((__m256i *)(r + i + 128), t0);

        montmul(&tmp, &p1[5], &zeta, &tmp_Q, &tmp_QINV);
        montmul(&prod, &cross[13], &zeta, &tmp_Q, &tmp_QINV);
        t0 = _mm256_add_epi16(_mm256_add_epi16(p0[5], tmp), prod);
        montmul(&t0, &t0, &set1, &tmp_Q, &tmp_QINV);
        _mm256_store_si256((__m256i *)(r + i + 144), t0);

        montmul(&tmp, &p1[6], &zeta, &tmp_Q, &tmp_QINV);
        montmul(&prod, &cross[14], &zeta, &tmp_Q, &tmp_QINV);
        t0 = _mm256_add_epi16(_mm256_add_epi16(p0[6], tmp), prod);
        montmul(&t0, &t0, &set1, &tmp_Q, &tmp_QINV);
        _mm256_store_si256((__m256i *)(r + i + 160), t0);

        montmul(&tmp, &p1[7], &zeta, &tmp_Q, &tmp_QINV);
        t0 = _mm256_add_epi16(p0[7], tmp);
        montmul(&t0, &t0, &set1, &tmp_Q, &tmp_QINV);
        _mm256_store_si256((__m256i *)(r + i + 176), t0);

        montmul(&tmp, &p1[8], &zeta, &tmp_Q, &tmp_QINV);
        t0 = _mm256_add_epi16(_mm256_add_epi16(p0[8], tmp), cross[0]);
        montmul(&t0, &t0, &set1, &tmp_Q, &tmp_QINV);
        _mm256_store_si256((__m256i *)(r + i + 64), t0);

        montmul(&tmp, &p1[9], &zeta, &tmp_Q, &tmp_QINV);
        t0 = _mm256_add_epi16(_mm256_add_epi16(p0[9], tmp), cross[1]);
        montmul(&t0, &t0, &set1, &tmp_Q, &tmp_QINV);
        _mm256_store_si256((__m256i *)(r + i + 80), t0);

        montmul(&tmp, &p1[10], &zeta, &tmp_Q, &tmp_QINV);
        t0 = _mm256_add_epi16(_mm256_add_epi16(p0[10], tmp), cross[2]);
        montmul(&t0, &t0, &set1, &tmp_Q, &tmp_QINV);
        _mm256_store_si256((__m256i *)(r + i + 96), t0);

        montmul(&tmp, &p1[11], &zeta, &tmp_Q, &tmp_QINV);
        t0 = _mm256_add_epi16(_mm256_add_epi16(p0[11], tmp), cross[3]);
        montmul(&t0, &t0, &set1, &tmp_Q, &tmp_QINV);
        _mm256_store_si256((__m256i *)(r + i + 112), t0);

        montmul(&tmp, &p1[12], &zeta, &tmp_Q, &tmp_QINV);
        t0 = _mm256_add_epi16(_mm256_add_epi16(p0[12], tmp), cross[4]);
        montmul(&t0, &t0, &set1, &tmp_Q, &tmp_QINV);
        _mm256_store_si256((__m256i *)(r + i + 192), t0);

        montmul(&tmp, &p1[13], &zeta, &tmp_Q, &tmp_QINV);
        t0 = _mm256_add_epi16(_mm256_add_epi16(p0[13], tmp), cross[5]);
        montmul(&t0, &t0, &set1, &tmp_Q, &tmp_QINV);
        _mm256_store_si256((__m256i *)(r + i + 208), t0);

        montmul(&tmp, &p1[14], &zeta, &tmp_Q, &tmp_QINV);
        t0 = _mm256_add_epi16(_mm256_add_epi16(p0[14], tmp), cross[6]);
        montmul(&t0, &t0, &set1, &tmp_Q, &tmp_QINV);
        _mm256_store_si256((__m256i *)(r + i + 224), t0);

        montmul(&t0, &cross[7], &set1, &tmp_Q, &tmp_QINV);
        _mm256_store_si256((__m256i *)(r + i + 240), t0);
    }
}

void poly_basemul_ntt_mq(int16_t *r, int16_t *a, int16_t *b, int16_t *muldata)
{
    int i, j, k;
    __m256i zeta, prod, tmp, t0;
    __m256i av[16], bv[16], as[8], bs[8];
    __m256i p0[15], p1[15], pm[15], cross[15];
    __m256i tmp_Q = _mm256_set1_epi16(769);
    __m256i tmp_QINV = _mm256_set1_epi16(-767);
    __m256i set1 = _mm256_set1_epi16(19);
    __m256i zero = _mm256_setzero_si256();

    for(i = 0; i < ZEN_N; i += 256)
    {
        zeta = _mm256_load_si256((__m256i *)(muldata + i / 16));

        av[0] = _mm256_load_si256((__m256i *)(a + i));
        av[1] = _mm256_load_si256((__m256i *)(a + i + 16));
        av[2] = _mm256_load_si256((__m256i *)(a + i + 32));
        av[3] = _mm256_load_si256((__m256i *)(a + i + 48));
        av[4] = _mm256_load_si256((__m256i *)(a + i + 128));
        av[5] = _mm256_load_si256((__m256i *)(a + i + 144));
        av[6] = _mm256_load_si256((__m256i *)(a + i + 160));
        av[7] = _mm256_load_si256((__m256i *)(a + i + 176));
        av[8] = _mm256_load_si256((__m256i *)(a + i + 64));
        av[9] = _mm256_load_si256((__m256i *)(a + i + 80));
        av[10] = _mm256_load_si256((__m256i *)(a + i + 96));
        av[11] = _mm256_load_si256((__m256i *)(a + i + 112));
        av[12] = _mm256_load_si256((__m256i *)(a + i + 192));
        av[13] = _mm256_load_si256((__m256i *)(a + i + 208));
        av[14] = _mm256_load_si256((__m256i *)(a + i + 224));
        av[15] = _mm256_load_si256((__m256i *)(a + i + 240));

        bv[0] = _mm256_load_si256((__m256i *)(b + i));
        bv[1] = _mm256_load_si256((__m256i *)(b + i + 16));
        bv[2] = _mm256_load_si256((__m256i *)(b + i + 32));
        bv[3] = _mm256_load_si256((__m256i *)(b + i + 48));
        bv[4] = _mm256_load_si256((__m256i *)(b + i + 128));
        bv[5] = _mm256_load_si256((__m256i *)(b + i + 144));
        bv[6] = _mm256_load_si256((__m256i *)(b + i + 160));
        bv[7] = _mm256_load_si256((__m256i *)(b + i + 176));
        bv[8] = _mm256_load_si256((__m256i *)(b + i + 64));
        bv[9] = _mm256_load_si256((__m256i *)(b + i + 80));
        bv[10] = _mm256_load_si256((__m256i *)(b + i + 96));
        bv[11] = _mm256_load_si256((__m256i *)(b + i + 112));
        bv[12] = _mm256_load_si256((__m256i *)(b + i + 192));
        bv[13] = _mm256_load_si256((__m256i *)(b + i + 208));
        bv[14] = _mm256_load_si256((__m256i *)(b + i + 224));
        bv[15] = _mm256_load_si256((__m256i *)(b + i + 240));

        for(j = 0; j < 8; j++)
        {
            as[j] = _mm256_add_epi16(av[j], av[j + 8]);
            bs[j] = _mm256_add_epi16(bv[j], bv[j + 8]);
        }

        for(j = 0; j < 15; j++)
        {
            p0[j] = zero;
            p1[j] = zero;
            pm[j] = zero;
        }

        for(j = 0; j < 8; j++)
        {
            for(k = 0; k < 8; k++)
            {
                montmul(&prod, &av[j], &bv[k], &tmp_Q, &tmp_QINV);
                p0[j + k] = _mm256_add_epi16(p0[j + k], prod);

                montmul(&prod, &av[j + 8], &bv[k + 8], &tmp_Q, &tmp_QINV);
                p1[j + k] = _mm256_add_epi16(p1[j + k], prod);

                montmul(&prod, &as[j], &bs[k], &tmp_Q, &tmp_QINV);
                pm[j + k] = _mm256_add_epi16(pm[j + k], prod);
            }
        }

        for(j = 0; j < 15; j++)
        {
            cross[j] = _mm256_sub_epi16(_mm256_sub_epi16(pm[j], p0[j]), p1[j]);
        }

        montmul(&tmp, &p1[0], &zeta, &tmp_Q, &tmp_QINV);
        montmul(&prod, &cross[8], &zeta, &tmp_Q, &tmp_QINV);
        t0 = _mm256_add_epi16(_mm256_add_epi16(p0[0], tmp), prod);
        montmul(&t0, &t0, &set1, &tmp_Q, &tmp_QINV);
        t0 = _mm256_add_epi16(t0, _mm256_and_si256(_mm256_srai_epi16(t0, 15), tmp_Q));
        _mm256_store_si256((__m256i *)(r + i), t0);

        montmul(&tmp, &p1[1], &zeta, &tmp_Q, &tmp_QINV);
        montmul(&prod, &cross[9], &zeta, &tmp_Q, &tmp_QINV);
        t0 = _mm256_add_epi16(_mm256_add_epi16(p0[1], tmp), prod);
        montmul(&t0, &t0, &set1, &tmp_Q, &tmp_QINV);
        t0 = _mm256_add_epi16(t0, _mm256_and_si256(_mm256_srai_epi16(t0, 15), tmp_Q));
        _mm256_store_si256((__m256i *)(r + i + 16), t0);

        montmul(&tmp, &p1[2], &zeta, &tmp_Q, &tmp_QINV);
        montmul(&prod, &cross[10], &zeta, &tmp_Q, &tmp_QINV);
        t0 = _mm256_add_epi16(_mm256_add_epi16(p0[2], tmp), prod);
        montmul(&t0, &t0, &set1, &tmp_Q, &tmp_QINV);
        t0 = _mm256_add_epi16(t0, _mm256_and_si256(_mm256_srai_epi16(t0, 15), tmp_Q));
        _mm256_store_si256((__m256i *)(r + i + 32), t0);

        montmul(&tmp, &p1[3], &zeta, &tmp_Q, &tmp_QINV);
        montmul(&prod, &cross[11], &zeta, &tmp_Q, &tmp_QINV);
        t0 = _mm256_add_epi16(_mm256_add_epi16(p0[3], tmp), prod);
        montmul(&t0, &t0, &set1, &tmp_Q, &tmp_QINV);
        t0 = _mm256_add_epi16(t0, _mm256_and_si256(_mm256_srai_epi16(t0, 15), tmp_Q));
        _mm256_store_si256((__m256i *)(r + i + 48), t0);

        montmul(&tmp, &p1[4], &zeta, &tmp_Q, &tmp_QINV);
        montmul(&prod, &cross[12], &zeta, &tmp_Q, &tmp_QINV);
        t0 = _mm256_add_epi16(_mm256_add_epi16(p0[4], tmp), prod);
        montmul(&t0, &t0, &set1, &tmp_Q, &tmp_QINV);
        t0 = _mm256_add_epi16(t0, _mm256_and_si256(_mm256_srai_epi16(t0, 15), tmp_Q));
        _mm256_store_si256((__m256i *)(r + i + 128), t0);

        montmul(&tmp, &p1[5], &zeta, &tmp_Q, &tmp_QINV);
        montmul(&prod, &cross[13], &zeta, &tmp_Q, &tmp_QINV);
        t0 = _mm256_add_epi16(_mm256_add_epi16(p0[5], tmp), prod);
        montmul(&t0, &t0, &set1, &tmp_Q, &tmp_QINV);
        t0 = _mm256_add_epi16(t0, _mm256_and_si256(_mm256_srai_epi16(t0, 15), tmp_Q));
        _mm256_store_si256((__m256i *)(r + i + 144), t0);

        montmul(&tmp, &p1[6], &zeta, &tmp_Q, &tmp_QINV);
        montmul(&prod, &cross[14], &zeta, &tmp_Q, &tmp_QINV);
        t0 = _mm256_add_epi16(_mm256_add_epi16(p0[6], tmp), prod);
        montmul(&t0, &t0, &set1, &tmp_Q, &tmp_QINV);
        t0 = _mm256_add_epi16(t0, _mm256_and_si256(_mm256_srai_epi16(t0, 15), tmp_Q));
        _mm256_store_si256((__m256i *)(r + i + 160), t0);

        montmul(&tmp, &p1[7], &zeta, &tmp_Q, &tmp_QINV);
        t0 = _mm256_add_epi16(p0[7], tmp);
        montmul(&t0, &t0, &set1, &tmp_Q, &tmp_QINV);
        t0 = _mm256_add_epi16(t0, _mm256_and_si256(_mm256_srai_epi16(t0, 15), tmp_Q));
        _mm256_store_si256((__m256i *)(r + i + 176), t0);

        montmul(&tmp, &p1[8], &zeta, &tmp_Q, &tmp_QINV);
        t0 = _mm256_add_epi16(_mm256_add_epi16(p0[8], tmp), cross[0]);
        montmul(&t0, &t0, &set1, &tmp_Q, &tmp_QINV);
        t0 = _mm256_add_epi16(t0, _mm256_and_si256(_mm256_srai_epi16(t0, 15), tmp_Q));
        _mm256_store_si256((__m256i *)(r + i + 64), t0);

        montmul(&tmp, &p1[9], &zeta, &tmp_Q, &tmp_QINV);
        t0 = _mm256_add_epi16(_mm256_add_epi16(p0[9], tmp), cross[1]);
        montmul(&t0, &t0, &set1, &tmp_Q, &tmp_QINV);
        t0 = _mm256_add_epi16(t0, _mm256_and_si256(_mm256_srai_epi16(t0, 15), tmp_Q));
        _mm256_store_si256((__m256i *)(r + i + 80), t0);

        montmul(&tmp, &p1[10], &zeta, &tmp_Q, &tmp_QINV);
        t0 = _mm256_add_epi16(_mm256_add_epi16(p0[10], tmp), cross[2]);
        montmul(&t0, &t0, &set1, &tmp_Q, &tmp_QINV);
        t0 = _mm256_add_epi16(t0, _mm256_and_si256(_mm256_srai_epi16(t0, 15), tmp_Q));
        _mm256_store_si256((__m256i *)(r + i + 96), t0);

        montmul(&tmp, &p1[11], &zeta, &tmp_Q, &tmp_QINV);
        t0 = _mm256_add_epi16(_mm256_add_epi16(p0[11], tmp), cross[3]);
        montmul(&t0, &t0, &set1, &tmp_Q, &tmp_QINV);
        t0 = _mm256_add_epi16(t0, _mm256_and_si256(_mm256_srai_epi16(t0, 15), tmp_Q));
        _mm256_store_si256((__m256i *)(r + i + 112), t0);

        montmul(&tmp, &p1[12], &zeta, &tmp_Q, &tmp_QINV);
        t0 = _mm256_add_epi16(_mm256_add_epi16(p0[12], tmp), cross[4]);
        montmul(&t0, &t0, &set1, &tmp_Q, &tmp_QINV);
        t0 = _mm256_add_epi16(t0, _mm256_and_si256(_mm256_srai_epi16(t0, 15), tmp_Q));
        _mm256_store_si256((__m256i *)(r + i + 192), t0);

        montmul(&tmp, &p1[13], &zeta, &tmp_Q, &tmp_QINV);
        t0 = _mm256_add_epi16(_mm256_add_epi16(p0[13], tmp), cross[5]);
        montmul(&t0, &t0, &set1, &tmp_Q, &tmp_QINV);
        t0 = _mm256_add_epi16(t0, _mm256_and_si256(_mm256_srai_epi16(t0, 15), tmp_Q));
        _mm256_store_si256((__m256i *)(r + i + 208), t0);

        montmul(&tmp, &p1[14], &zeta, &tmp_Q, &tmp_QINV);
        t0 = _mm256_add_epi16(_mm256_add_epi16(p0[14], tmp), cross[6]);
        montmul(&t0, &t0, &set1, &tmp_Q, &tmp_QINV);
        t0 = _mm256_add_epi16(t0, _mm256_and_si256(_mm256_srai_epi16(t0, 15), tmp_Q));
        _mm256_store_si256((__m256i *)(r + i + 224), t0);

        montmul(&t0, &cross[7], &set1, &tmp_Q, &tmp_QINV);
        t0 = _mm256_add_epi16(t0, _mm256_and_si256(_mm256_srai_epi16(t0, 15), tmp_Q));
        _mm256_store_si256((__m256i *)(r + i + 240), t0);
    }
}

void poly_baseinv_ntt(int16_t *r, int16_t *a)
{
    int i, j, k;
    __m128i v0, v1;
    __m256i zeta, t, x;
    __m256i av[16], ae[8], ao[8], h[8], he[4], ho[4], b4[4], g[8], rv[16];
    __m256i pe15[15], po15[15], pe7[7], po7[7];
    __m256i c0, c1, e, f0, f1, f2, f3;
    __m256i tmp_Q = _mm256_set1_epi16(769);
    __m256i tmp_QINV = _mm256_set1_epi16(-767);
    __m256i set2 = _mm256_set1_epi16(342);
    __m256i setn1 = _mm256_set1_epi16(-171);
    __m256i zero = _mm256_setzero_si256();

    for(i = 0; i < ZEN_N; i += 256)
    {
        zeta = _mm256_load_si256((__m256i *)(invdata + i / 8));

        av[0] = _mm256_load_si256((__m256i *)(a + i));
        av[1] = _mm256_load_si256((__m256i *)(a + i + 16));
        av[2] = _mm256_load_si256((__m256i *)(a + i + 32));
        av[3] = _mm256_load_si256((__m256i *)(a + i + 48));
        av[4] = _mm256_load_si256((__m256i *)(a + i + 128));
        av[5] = _mm256_load_si256((__m256i *)(a + i + 144));
        av[6] = _mm256_load_si256((__m256i *)(a + i + 160));
        av[7] = _mm256_load_si256((__m256i *)(a + i + 176));
        av[8] = _mm256_load_si256((__m256i *)(a + i + 64));
        av[9] = _mm256_load_si256((__m256i *)(a + i + 80));
        av[10] = _mm256_load_si256((__m256i *)(a + i + 96));
        av[11] = _mm256_load_si256((__m256i *)(a + i + 112));
        av[12] = _mm256_load_si256((__m256i *)(a + i + 192));
        av[13] = _mm256_load_si256((__m256i *)(a + i + 208));
        av[14] = _mm256_load_si256((__m256i *)(a + i + 224));
        av[15] = _mm256_load_si256((__m256i *)(a + i + 240));

        for(j = 0; j < 8; j++)
        {
            ae[j] = av[2 * j];
            ao[j] = av[2 * j + 1];
        }

        for(j = 0; j < 15; j++)
        {
            pe15[j] = zero;
            po15[j] = zero;
        }

        for(j = 0; j < 8; j++)
        {
            for(k = 0; k < 8; k++)
            {
                montmul(&t, &ae[j], &ae[k], &tmp_Q, &tmp_QINV);
                pe15[j + k] = _mm256_add_epi16(pe15[j + k], t);
                montmul(&t, &ao[j], &ao[k], &tmp_Q, &tmp_QINV);
                po15[j + k] = _mm256_add_epi16(po15[j + k], t);
            }
        }

        t = _mm256_sub_epi16(pe15[8], po15[7]);
        montmul(&t, &t, &zeta, &tmp_Q, &tmp_QINV);
        h[0] = _mm256_sub_epi16(pe15[0], t);
        t = _mm256_sub_epi16(pe15[9], po15[8]);
        montmul(&t, &t, &zeta, &tmp_Q, &tmp_QINV);
        h[1] = _mm256_sub_epi16(_mm256_sub_epi16(pe15[1], po15[0]), t);
        t = _mm256_sub_epi16(pe15[10], po15[9]);
        montmul(&t, &t, &zeta, &tmp_Q, &tmp_QINV);
        h[2] = _mm256_sub_epi16(_mm256_sub_epi16(pe15[2], po15[1]), t);
        t = _mm256_sub_epi16(pe15[11], po15[10]);
        montmul(&t, &t, &zeta, &tmp_Q, &tmp_QINV);
        h[3] = _mm256_sub_epi16(_mm256_sub_epi16(pe15[3], po15[2]), t);
        t = _mm256_sub_epi16(pe15[12], po15[11]);
        montmul(&t, &t, &zeta, &tmp_Q, &tmp_QINV);
        h[4] = _mm256_sub_epi16(_mm256_sub_epi16(pe15[4], po15[3]), t);
        t = _mm256_sub_epi16(pe15[13], po15[12]);
        montmul(&t, &t, &zeta, &tmp_Q, &tmp_QINV);
        h[5] = _mm256_sub_epi16(_mm256_sub_epi16(pe15[5], po15[4]), t);
        t = _mm256_sub_epi16(pe15[14], po15[13]);
        montmul(&t, &t, &zeta, &tmp_Q, &tmp_QINV);
        h[6] = _mm256_sub_epi16(_mm256_sub_epi16(pe15[6], po15[5]), t);
        montmul(&t, &po15[14], &zeta, &tmp_Q, &tmp_QINV);
        h[7] = _mm256_add_epi16(_mm256_sub_epi16(pe15[7], po15[6]), t);

        he[0] = h[0];
        he[1] = h[2];
        he[2] = h[4];
        he[3] = h[6];
        ho[0] = h[1];
        ho[1] = h[3];
        ho[2] = h[5];
        ho[3] = h[7];

        for(j = 0; j < 7; j++)
        {
            pe7[j] = zero;
            po7[j] = zero;
        }

        for(j = 0; j < 4; j++)
        {
            for(k = 0; k < 4; k++)
            {
                montmul(&t, &he[j], &he[k], &tmp_Q, &tmp_QINV);
                pe7[j + k] = _mm256_add_epi16(pe7[j + k], t);
                montmul(&t, &ho[j], &ho[k], &tmp_Q, &tmp_QINV);
                po7[j + k] = _mm256_add_epi16(po7[j + k], t);
            }
        }

        t = _mm256_sub_epi16(pe7[4], po7[3]);
        montmul(&t, &t, &zeta, &tmp_Q, &tmp_QINV);
        b4[0] = _mm256_sub_epi16(pe7[0], t);
        t = _mm256_sub_epi16(pe7[5], po7[4]);
        montmul(&t, &t, &zeta, &tmp_Q, &tmp_QINV);
        b4[1] = _mm256_sub_epi16(_mm256_sub_epi16(pe7[1], po7[0]), t);
        t = _mm256_sub_epi16(pe7[6], po7[5]);
        montmul(&t, &t, &zeta, &tmp_Q, &tmp_QINV);
        b4[2] = _mm256_sub_epi16(_mm256_sub_epi16(pe7[2], po7[1]), t);
        montmul(&t, &po7[6], &zeta, &tmp_Q, &tmp_QINV);
        b4[3] = _mm256_add_epi16(_mm256_sub_epi16(pe7[3], po7[2]), t);

        montmul(&c0, &b4[0], &b4[0], &tmp_Q, &tmp_QINV);
        montmul(&t, &b4[2], &b4[2], &tmp_Q, &tmp_QINV);
        montmul(&c1, &b4[0], &b4[2], &tmp_Q, &tmp_QINV);
        montmul(&c1, &c1, &set2, &tmp_Q, &tmp_QINV);
        montmul(&e, &b4[1], &b4[1], &tmp_Q, &tmp_QINV);
        montmul(&f0, &b4[3], &b4[3], &tmp_Q, &tmp_QINV);
        montmul(&f1, &b4[1], &b4[3], &tmp_Q, &tmp_QINV);
        montmul(&f1, &f1, &set2, &tmp_Q, &tmp_QINV);
        x = _mm256_sub_epi16(t, f1);
        montmul(&x, &x, &zeta, &tmp_Q, &tmp_QINV);
        c0 = _mm256_sub_epi16(c0, x);
        montmul(&x, &f0, &zeta, &tmp_Q, &tmp_QINV);
        c1 = _mm256_add_epi16(_mm256_sub_epi16(c1, e), x);

        montmul(&e, &c1, &c1, &tmp_Q, &tmp_QINV);
        montmul(&e, &e, &zeta, &tmp_Q, &tmp_QINV);
        montmul(&t, &c0, &c0, &tmp_Q, &tmp_QINV);
        e = _mm256_add_epi16(e, t);
        e = _mm256_add_epi16(e, _mm256_and_si256(_mm256_srai_epi16(e, 15), tmp_Q));
        v0 = _mm256_extracti128_si256(e, 0);
        t = _mm256_cvtepi16_epi32(v0);
        t = _mm256_i32gather_epi32(qinv, t, sizeof(int32_t));
        v1 = _mm256_extracti128_si256(e, 1);
        x = _mm256_cvtepi16_epi32(v1);
        x = _mm256_i32gather_epi32(qinv, x, sizeof(int32_t));
        t = _mm256_packs_epi32(t, x);
        e = _mm256_permute4x64_epi64(t, 0xd8);

        montmul(&c0, &e, &c0, &tmp_Q, &tmp_QINV);
        montmul(&c1, &e, &c1, &tmp_Q, &tmp_QINV);
        montmul(&c1, &c1, &setn1, &tmp_Q, &tmp_QINV);

        montmul(&f0, &c1, &b4[2], &tmp_Q, &tmp_QINV);
        montmul(&f0, &f0, &zeta, &tmp_Q, &tmp_QINV);
        montmul(&t, &c0, &b4[0], &tmp_Q, &tmp_QINV);
        f0 = _mm256_sub_epi16(t, f0);

        montmul(&f1, &c1, &b4[3], &tmp_Q, &tmp_QINV);
        montmul(&f1, &f1, &zeta, &tmp_Q, &tmp_QINV);
        montmul(&t, &c0, &b4[1], &tmp_Q, &tmp_QINV);
        f1 = _mm256_sub_epi16(f1, t);

        montmul(&f2, &c0, &b4[2], &tmp_Q, &tmp_QINV);
        montmul(&t, &c1, &b4[0], &tmp_Q, &tmp_QINV);
        f2 = _mm256_add_epi16(f2, t);

        montmul(&f3, &c0, &b4[3], &tmp_Q, &tmp_QINV);
        montmul(&t, &c1, &b4[1], &tmp_Q, &tmp_QINV);
        f3 = _mm256_add_epi16(f3, t);
        montmul(&f3, &f3, &setn1, &tmp_Q, &tmp_QINV);

        for(j = 0; j < 7; j++)
        {
            pe7[j] = zero;
            po7[j] = zero;
        }

        for(j = 0; j < 4; j++)
        {
            __m256i fv = j == 0 ? f0 : j == 1 ? f1 : j == 2 ? f2 : f3;
            for(k = 0; k < 4; k++)
            {
                montmul(&t, &fv, &he[k], &tmp_Q, &tmp_QINV);
                pe7[j + k] = _mm256_add_epi16(pe7[j + k], t);
                montmul(&t, &fv, &ho[k], &tmp_Q, &tmp_QINV);
                po7[j + k] = _mm256_add_epi16(po7[j + k], t);
            }
        }

        montmul(&t, &pe7[4], &zeta, &tmp_Q, &tmp_QINV);
        g[0] = _mm256_sub_epi16(pe7[0], t);
        montmul(&t, &po7[4], &zeta, &tmp_Q, &tmp_QINV);
        g[1] = _mm256_sub_epi16(t, po7[0]);
        montmul(&t, &pe7[5], &zeta, &tmp_Q, &tmp_QINV);
        g[2] = _mm256_sub_epi16(pe7[1], t);
        montmul(&t, &po7[5], &zeta, &tmp_Q, &tmp_QINV);
        g[3] = _mm256_sub_epi16(t, po7[1]);
        montmul(&t, &pe7[6], &zeta, &tmp_Q, &tmp_QINV);
        g[4] = _mm256_sub_epi16(pe7[2], t);
        montmul(&t, &po7[6], &zeta, &tmp_Q, &tmp_QINV);
        g[5] = _mm256_sub_epi16(t, po7[2]);
        g[6] = pe7[3];
        montmul(&g[7], &po7[3], &setn1, &tmp_Q, &tmp_QINV);

        for(j = 0; j < 15; j++)
        {
            pe15[j] = zero;
            po15[j] = zero;
        }

        for(j = 0; j < 8; j++)
        {
            for(k = 0; k < 8; k++)
            {
                montmul(&t, &g[j], &ae[k], &tmp_Q, &tmp_QINV);
                pe15[j + k] = _mm256_add_epi16(pe15[j + k], t);
                montmul(&t, &g[j], &ao[k], &tmp_Q, &tmp_QINV);
                po15[j + k] = _mm256_add_epi16(po15[j + k], t);
            }
        }

        montmul(&t, &pe15[8], &zeta, &tmp_Q, &tmp_QINV);
        rv[0] = _mm256_sub_epi16(pe15[0], t);
        montmul(&t, &po15[8], &zeta, &tmp_Q, &tmp_QINV);
        rv[1] = _mm256_sub_epi16(t, po15[0]);
        montmul(&t, &pe15[9], &zeta, &tmp_Q, &tmp_QINV);
        rv[2] = _mm256_sub_epi16(pe15[1], t);
        montmul(&t, &po15[9], &zeta, &tmp_Q, &tmp_QINV);
        rv[3] = _mm256_sub_epi16(t, po15[1]);
        montmul(&t, &pe15[10], &zeta, &tmp_Q, &tmp_QINV);
        rv[4] = _mm256_sub_epi16(pe15[2], t);
        montmul(&t, &po15[10], &zeta, &tmp_Q, &tmp_QINV);
        rv[5] = _mm256_sub_epi16(t, po15[2]);
        montmul(&t, &pe15[11], &zeta, &tmp_Q, &tmp_QINV);
        rv[6] = _mm256_sub_epi16(pe15[3], t);
        montmul(&t, &po15[11], &zeta, &tmp_Q, &tmp_QINV);
        rv[7] = _mm256_sub_epi16(t, po15[3]);
        montmul(&t, &pe15[12], &zeta, &tmp_Q, &tmp_QINV);
        rv[8] = _mm256_sub_epi16(pe15[4], t);
        montmul(&t, &po15[12], &zeta, &tmp_Q, &tmp_QINV);
        rv[9] = _mm256_sub_epi16(t, po15[4]);
        montmul(&t, &pe15[13], &zeta, &tmp_Q, &tmp_QINV);
        rv[10] = _mm256_sub_epi16(pe15[5], t);
        montmul(&t, &po15[13], &zeta, &tmp_Q, &tmp_QINV);
        rv[11] = _mm256_sub_epi16(t, po15[5]);
        montmul(&t, &pe15[14], &zeta, &tmp_Q, &tmp_QINV);
        rv[12] = _mm256_sub_epi16(pe15[6], t);
        montmul(&t, &po15[14], &zeta, &tmp_Q, &tmp_QINV);
        rv[13] = _mm256_sub_epi16(t, po15[6]);
        rv[14] = pe15[7];
        montmul(&rv[15], &po15[7], &setn1, &tmp_Q, &tmp_QINV);

        _mm256_store_si256((__m256i *)(r + i), rv[0]);
        _mm256_store_si256((__m256i *)(r + i + 16), rv[1]);
        _mm256_store_si256((__m256i *)(r + i + 32), rv[2]);
        _mm256_store_si256((__m256i *)(r + i + 48), rv[3]);
        _mm256_store_si256((__m256i *)(r + i + 128), rv[4]);
        _mm256_store_si256((__m256i *)(r + i + 144), rv[5]);
        _mm256_store_si256((__m256i *)(r + i + 160), rv[6]);
        _mm256_store_si256((__m256i *)(r + i + 176), rv[7]);
        _mm256_store_si256((__m256i *)(r + i + 64), rv[8]);
        _mm256_store_si256((__m256i *)(r + i + 80), rv[9]);
        _mm256_store_si256((__m256i *)(r + i + 96), rv[10]);
        _mm256_store_si256((__m256i *)(r + i + 112), rv[11]);
        _mm256_store_si256((__m256i *)(r + i + 192), rv[12]);
        _mm256_store_si256((__m256i *)(r + i + 208), rv[13]);
        _mm256_store_si256((__m256i *)(r + i + 224), rv[14]);
        _mm256_store_si256((__m256i *)(r + i + 240), rv[15]);
    }
}

int check_poly_inv_Zq(int16_t *a)
{
    unsigned int i;
    uint32_t m;

    __m256i acc;
    __m256i zero;
    __m256i one;

    __m256i x0, x1, x2, x3;
    __m256i s0, s1, s2, s3;
    __m256i t0, t1, t2, t3;
    __m256i c0, c1, c2, c3;

    acc  = _mm256_setzero_si256();
    zero = _mm256_setzero_si256();
    one  = _mm256_set1_epi16(1);

    for(i = 0; i < ZEN_N; i += 64)
    {
        x0 = _mm256_load_si256((const __m256i *)(a + i +  0));
        x1 = _mm256_load_si256((const __m256i *)(a + i + 16));
        x2 = _mm256_load_si256((const __m256i *)(a + i + 32));
        x3 = _mm256_load_si256((const __m256i *)(a + i + 48));

        s0 = _mm256_madd_epi16(x0, one);
        s1 = _mm256_madd_epi16(x1, one);
        s2 = _mm256_madd_epi16(x2, one);
        s3 = _mm256_madd_epi16(x3, one);

        t0 = _mm256_permute2x128_si256(s0, s0, 0x01);
        t1 = _mm256_permute2x128_si256(s1, s1, 0x01);
        t2 = _mm256_permute2x128_si256(s2, s2, 0x01);
        t3 = _mm256_permute2x128_si256(s3, s3, 0x01);

        s0 = _mm256_add_epi32(s0, t0);
        s1 = _mm256_add_epi32(s1, t1);
        s2 = _mm256_add_epi32(s2, t2);
        s3 = _mm256_add_epi32(s3, t3);

        s0 = _mm256_hadd_epi32(s0, s0);
        s1 = _mm256_hadd_epi32(s1, s1);
        s2 = _mm256_hadd_epi32(s2, s2);
        s3 = _mm256_hadd_epi32(s3, s3);

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

    m = (uint32_t)_mm256_movemask_epi8(acc);

    return (int)((m | (0u - m)) >> 31);
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
    __m256i x0, x1, x2, x3;
    __m256i y0, y1;
    __m256i one;
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
    __m256i bitmask;
    __m256i zero;
    __m256i one;
    __m256i v0, v1, v2, v3;

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

#define CLMUL32_STEP(A, B, SH) do {                     \
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
    __m256i va;
    __m256i vb;
    __m256i vlo;
    __m256i vhi;

    va = _mm256_set_epi64x((long long)a3, (long long)a2,
                           (long long)a1, (long long)a0);
    vb = _mm256_set_epi64x((long long)b3, (long long)b2,
                           (long long)b1, (long long)b0);

    clmul64x4_avx2_vec(va, vb, &vlo, &vhi);

    *lo0 = (uint64_t)_mm256_extract_epi64(vlo, 0);
    *lo1 = (uint64_t)_mm256_extract_epi64(vlo, 1);
    *lo2 = (uint64_t)_mm256_extract_epi64(vlo, 2);
    *lo3 = (uint64_t)_mm256_extract_epi64(vlo, 3);

    *hi0 = (uint64_t)_mm256_extract_epi64(vhi, 0);
    *hi1 = (uint64_t)_mm256_extract_epi64(vhi, 1);
    *hi2 = (uint64_t)_mm256_extract_epi64(vhi, 2);
    *hi3 = (uint64_t)_mm256_extract_epi64(vhi, 3);
}

static inline void clmul64_mod_full_avx2(uint64_t a, uint64_t b, uint64_t *r)
{
    uint64_t p0, p1;
    uint64_t d0, d1, d2, d3, d4, d5;

    clmul64x4_avx2(a, 0, 0, 0,
                   b, 0, 0, 0,
                   &p0, &p1,
                   &d0, &d1,
                   &d2, &d3,
                   &d4, &d5);

    *r = p0 ^ p1;
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

static inline void clmul256_mod_avx2(uint64_t a0, uint64_t a1,
                                     uint64_t a2, uint64_t a3,
                                     uint64_t b0, uint64_t b1,
                                     uint64_t b2, uint64_t b3,
                                     uint64_t *r0, uint64_t *r1,
                                     uint64_t *r2, uint64_t *r3)
{
    uint64_t p0, p1, p2, p3;
    uint64_t p4, p5, p6, p7;

    clmul256_avx2(a0, a1, a2, a3,
                  b0, b1, b2, b3,
                  &p0, &p1, &p2, &p3,
                  &p4, &p5, &p6, &p7);

    *r0 = p0 ^ p4;
    *r1 = p1 ^ p5;
    *r2 = p2 ^ p6;
    *r3 = p3 ^ p7;
}

static inline void clmul512_avx2(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3,
                                 uint64_t a4, uint64_t a5, uint64_t a6, uint64_t a7,
                                 uint64_t b0, uint64_t b1, uint64_t b2, uint64_t b3,
                                 uint64_t b4, uint64_t b5, uint64_t b6, uint64_t b7,
                                 uint64_t *p0, uint64_t *p1, uint64_t *p2, uint64_t *p3,
                                 uint64_t *p4, uint64_t *p5, uint64_t *p6, uint64_t *p7,
                                 uint64_t *p8, uint64_t *p9, uint64_t *p10, uint64_t *p11,
                                 uint64_t *p12, uint64_t *p13, uint64_t *p14, uint64_t *p15)
{
    uint64_t z00, z01, z02, z03, z04, z05, z06, z07;
    uint64_t z20, z21, z22, z23, z24, z25, z26, z27;
    uint64_t t0, t1, t2, t3, t4, t5, t6, t7;
    uint64_t m0, m1, m2, m3, m4, m5, m6, m7;

    clmul256_avx2(a0, a1, a2, a3,
                  b0, b1, b2, b3,
                  &z00, &z01, &z02, &z03,
                  &z04, &z05, &z06, &z07);

    clmul256_avx2(a4, a5, a6, a7,
                  b4, b5, b6, b7,
                  &z20, &z21, &z22, &z23,
                  &z24, &z25, &z26, &z27);

    clmul256_avx2(a0 ^ a4, a1 ^ a5, a2 ^ a6, a3 ^ a7,
                  b0 ^ b4, b1 ^ b5, b2 ^ b6, b3 ^ b7,
                  &t0, &t1, &t2, &t3,
                  &t4, &t5, &t6, &t7);

    m0 = t0 ^ z00 ^ z20;
    m1 = t1 ^ z01 ^ z21;
    m2 = t2 ^ z02 ^ z22;
    m3 = t3 ^ z03 ^ z23;
    m4 = t4 ^ z04 ^ z24;
    m5 = t5 ^ z05 ^ z25;
    m6 = t6 ^ z06 ^ z26;
    m7 = t7 ^ z07 ^ z27;

    *p0  = z00;
    *p1  = z01;
    *p2  = z02;
    *p3  = z03;
    *p4  = z04 ^ m0;
    *p5  = z05 ^ m1;
    *p6  = z06 ^ m2;
    *p7  = z07 ^ m3;
    *p8  = z20 ^ m4;
    *p9  = z21 ^ m5;
    *p10 = z22 ^ m6;
    *p11 = z23 ^ m7;
    *p12 = z24;
    *p13 = z25;
    *p14 = z26;
    *p15 = z27;
}

void mul_in_R2_1024(int16_t *a, int16_t *b, int16_t *res)
{
    uint64_t a0, a1, a2, a3, a4, a5, a6, a7;
    uint64_t a8, a9, a10, a11, a12, a13, a14, a15;
    uint64_t b0, b1, b2, b3, b4, b5, b6, b7;
    uint64_t b8, b9, b10, b11, b12, b13, b14, b15;

    uint64_t z00, z01, z02, z03, z04, z05, z06, z07;
    uint64_t z08, z09, z10, z11, z12, z13, z14, z15;
    uint64_t z20, z21, z22, z23, z24, z25, z26, z27;
    uint64_t z28, z29, z30, z31, z32, z33, z34, z35;
    uint64_t t0, t1, t2, t3, t4, t5, t6, t7;
    uint64_t t8, t9, t10, t11, t12, t13, t14, t15;
    uint64_t m0, m1, m2, m3, m4, m5, m6, m7;
    uint64_t m8, m9, m10, m11, m12, m13, m14, m15;

    uint64_t r0, r1, r2, r3, r4, r5, r6, r7;
    uint64_t r8, r9, r10, r11, r12, r13, r14, r15;

    a0  = pack64_bits_avx2(a +   0);
    a1  = pack64_bits_avx2(a +  64);
    a2  = pack64_bits_avx2(a + 128);
    a3  = pack64_bits_avx2(a + 192);
    a4  = pack64_bits_avx2(a + 256);
    a5  = pack64_bits_avx2(a + 320);
    a6  = pack64_bits_avx2(a + 384);
    a7  = pack64_bits_avx2(a + 448);
    a8  = pack64_bits_avx2(a + 512);
    a9  = pack64_bits_avx2(a + 576);
    a10 = pack64_bits_avx2(a + 640);
    a11 = pack64_bits_avx2(a + 704);
    a12 = pack64_bits_avx2(a + 768);
    a13 = pack64_bits_avx2(a + 832);
    a14 = pack64_bits_avx2(a + 896);
    a15 = pack64_bits_avx2(a + 960);

    b0  = pack64_bits_avx2(b +   0);
    b1  = pack64_bits_avx2(b +  64);
    b2  = pack64_bits_avx2(b + 128);
    b3  = pack64_bits_avx2(b + 192);
    b4  = pack64_bits_avx2(b + 256);
    b5  = pack64_bits_avx2(b + 320);
    b6  = pack64_bits_avx2(b + 384);
    b7  = pack64_bits_avx2(b + 448);
    b8  = pack64_bits_avx2(b + 512);
    b9  = pack64_bits_avx2(b + 576);
    b10 = pack64_bits_avx2(b + 640);
    b11 = pack64_bits_avx2(b + 704);
    b12 = pack64_bits_avx2(b + 768);
    b13 = pack64_bits_avx2(b + 832);
    b14 = pack64_bits_avx2(b + 896);
    b15 = pack64_bits_avx2(b + 960);

    clmul512_avx2(a0, a1, a2, a3, a4, a5, a6, a7,
                  b0, b1, b2, b3, b4, b5, b6, b7,
                  &z00, &z01, &z02, &z03, &z04, &z05, &z06, &z07,
                  &z08, &z09, &z10, &z11, &z12, &z13, &z14, &z15);

    clmul512_avx2(a8, a9, a10, a11, a12, a13, a14, a15,
                  b8, b9, b10, b11, b12, b13, b14, b15,
                  &z20, &z21, &z22, &z23, &z24, &z25, &z26, &z27,
                  &z28, &z29, &z30, &z31, &z32, &z33, &z34, &z35);

    clmul512_avx2(a0 ^ a8, a1 ^ a9, a2 ^ a10, a3 ^ a11,
                  a4 ^ a12, a5 ^ a13, a6 ^ a14, a7 ^ a15,
                  b0 ^ b8, b1 ^ b9, b2 ^ b10, b3 ^ b11,
                  b4 ^ b12, b5 ^ b13, b6 ^ b14, b7 ^ b15,
                  &t0, &t1, &t2, &t3, &t4, &t5, &t6, &t7,
                  &t8, &t9, &t10, &t11, &t12, &t13, &t14, &t15);

    m0  = t0  ^ z00 ^ z20;
    m1  = t1  ^ z01 ^ z21;
    m2  = t2  ^ z02 ^ z22;
    m3  = t3  ^ z03 ^ z23;
    m4  = t4  ^ z04 ^ z24;
    m5  = t5  ^ z05 ^ z25;
    m6  = t6  ^ z06 ^ z26;
    m7  = t7  ^ z07 ^ z27;
    m8  = t8  ^ z08 ^ z28;
    m9  = t9  ^ z09 ^ z29;
    m10 = t10 ^ z10 ^ z30;
    m11 = t11 ^ z11 ^ z31;
    m12 = t12 ^ z12 ^ z32;
    m13 = t13 ^ z13 ^ z33;
    m14 = t14 ^ z14 ^ z34;
    m15 = t15 ^ z15 ^ z35;

    r0  = z00 ^ z20 ^ m8;
    r1  = z01 ^ z21 ^ m9;
    r2  = z02 ^ z22 ^ m10;
    r3  = z03 ^ z23 ^ m11;
    r4  = z04 ^ z24 ^ m12;
    r5  = z05 ^ z25 ^ m13;
    r6  = z06 ^ z26 ^ m14;
    r7  = z07 ^ z27 ^ m15;
    r8  = z08 ^ z28 ^ m0;
    r9  = z09 ^ z29 ^ m1;
    r10 = z10 ^ z30 ^ m2;
    r11 = z11 ^ z31 ^ m3;
    r12 = z12 ^ z32 ^ m4;
    r13 = z13 ^ z33 ^ m5;
    r14 = z14 ^ z34 ^ m6;
    r15 = z15 ^ z35 ^ m7;

    unpack64_bits_avx2(res +   0, r0);
    unpack64_bits_avx2(res +  64, r1);
    unpack64_bits_avx2(res + 128, r2);
    unpack64_bits_avx2(res + 192, r3);
    unpack64_bits_avx2(res + 256, r4);
    unpack64_bits_avx2(res + 320, r5);
    unpack64_bits_avx2(res + 384, r6);
    unpack64_bits_avx2(res + 448, r7);
    unpack64_bits_avx2(res + 512, r8);
    unpack64_bits_avx2(res + 576, r9);
    unpack64_bits_avx2(res + 640, r10);
    unpack64_bits_avx2(res + 704, r11);
    unpack64_bits_avx2(res + 768, r12);
    unpack64_bits_avx2(res + 832, r13);
    unpack64_bits_avx2(res + 896, r14);
    unpack64_bits_avx2(res + 960, r15);
}

#define PREFIX64(X) do {                  \
    (X) ^= (X) << 1;                      \
    (X) ^= (X) << 2;                      \
    (X) ^= (X) << 4;                      \
    (X) ^= (X) << 8;                      \
    (X) ^= (X) << 16;                     \
    (X) ^= (X) << 32;                     \
} while (0)

#define XOR_SHIFT512(SH) do {                                             \
    sh = (SH);                                                            \
    if(sh < 64)                                                           \
    {                                                                     \
        x0 = k0 << sh;                                                    \
        x1 = (k1 << sh) | (k0 >> (64 - sh));                              \
        x2 = (k2 << sh) | (k1 >> (64 - sh));                              \
        x3 = (k3 << sh) | (k2 >> (64 - sh));                              \
        x4 = (k4 << sh) | (k3 >> (64 - sh));                              \
        x5 = (k5 << sh) | (k4 >> (64 - sh));                              \
        x6 = (k6 << sh) | (k5 >> (64 - sh));                              \
        x7 = (k7 << sh) | (k6 >> (64 - sh));                              \
        k0 ^= x0;                                                         \
        k1 ^= x1;                                                         \
        k2 ^= x2;                                                         \
        k3 ^= x3;                                                         \
        k4 ^= x4;                                                         \
        k5 ^= x5;                                                         \
        k6 ^= x6;                                                         \
        k7 ^= x7;                                                         \
    }                                                                     \
    else if(sh == 64)                                                     \
    {                                                                     \
        k7 ^= k6;                                                         \
        k6 ^= k5;                                                         \
        k5 ^= k4;                                                         \
        k4 ^= k3;                                                         \
        k3 ^= k2;                                                         \
        k2 ^= k1;                                                         \
        k1 ^= k0;                                                         \
    }                                                                     \
    else if(sh == 128)                                                    \
    {                                                                     \
        k7 ^= k5;                                                         \
        k6 ^= k4;                                                         \
        k5 ^= k3;                                                         \
        k4 ^= k2;                                                         \
        k3 ^= k1;                                                         \
        k2 ^= k0;                                                         \
    }                                                                     \
    else                                                                  \
    {                                                                     \
        k7 ^= k3;                                                         \
        k6 ^= k2;                                                         \
        k5 ^= k1;                                                         \
        k4 ^= k0;                                                         \
    }                                                                     \
} while (0)

#define BLOCK_PREFIX512(N) do {                                           \
    for(sh = (N); sh < 512; sh <<= 1)                                     \
    {                                                                     \
        XOR_SHIFT512(sh);                                                 \
    }                                                                     \
} while (0)

#define MUL_MOD_SMALL_ROTATE_CONST(AP, TP, N, MASKN, OUT) do {            \
    unsigned int _i;                                                       \
    uint64_t _b;                                                           \
    uint64_t _r;                                                           \
    uint64_t _mask;                                                        \
    uint64_t _wrap;                                                        \
                                                                          \
    _b = (TP) & (MASKN);                                                   \
    _r = 0;                                                               \
                                                                          \
    for(_i = 0; _i < (N); _i++)                                           \
    {                                                                     \
        _mask = 0ULL - (((AP) >> _i) & 1ULL);                             \
        _r ^= _b & _mask;                                                  \
        _wrap = (_b >> ((N) - 1)) & 1ULL;                                  \
        _b = ((_b << 1) | _wrap) & (MASKN);                                \
    }                                                                     \
                                                                          \
    (OUT) = _r & (MASKN);                                                  \
} while (0)

#define MUL_LOW_BY_F512_ROTATE_CONST(BP, BITS) do {                       \
    unsigned int _i;                                                       \
    uint64_t _g0, _g1, _g2, _g3, _g4, _g5, _g6, _g7;                      \
    uint64_t _u0, _u1, _u2, _u3, _u4, _u5, _u6, _u7;                      \
    uint64_t _mask;                                                        \
    uint64_t _wrap;                                                        \
                                                                          \
    _g0 = f0;                                                              \
    _g1 = f1;                                                              \
    _g2 = f2;                                                              \
    _g3 = f3;                                                              \
    _g4 = f4;                                                              \
    _g5 = f5;                                                              \
    _g6 = f6;                                                              \
    _g7 = f7;                                                              \
                                                                          \
    r0 = 0;                                                               \
    r1 = 0;                                                               \
    r2 = 0;                                                               \
    r3 = 0;                                                               \
    r4 = 0;                                                               \
    r5 = 0;                                                               \
    r6 = 0;                                                               \
    r7 = 0;                                                               \
                                                                          \
    for(_i = 0; _i < (BITS); _i++)                                        \
    {                                                                     \
        _mask = 0ULL - (((BP) >> _i) & 1ULL);                             \
                                                                          \
        r0 ^= _g0 & _mask;                                                 \
        r1 ^= _g1 & _mask;                                                 \
        r2 ^= _g2 & _mask;                                                 \
        r3 ^= _g3 & _mask;                                                 \
        r4 ^= _g4 & _mask;                                                 \
        r5 ^= _g5 & _mask;                                                 \
        r6 ^= _g6 & _mask;                                                 \
        r7 ^= _g7 & _mask;                                                 \
                                                                          \
        _wrap = _g7 >> 63;                                                 \
                                                                          \
        _u0 = (_g0 << 1) | _wrap;                                          \
        _u1 = (_g1 << 1) | (_g0 >> 63);                                    \
        _u2 = (_g2 << 1) | (_g1 >> 63);                                    \
        _u3 = (_g3 << 1) | (_g2 >> 63);                                    \
        _u4 = (_g4 << 1) | (_g3 >> 63);                                    \
        _u5 = (_g5 << 1) | (_g4 >> 63);                                    \
        _u6 = (_g6 << 1) | (_g5 >> 63);                                    \
        _u7 = (_g7 << 1) | (_g6 >> 63);                                    \
                                                                          \
        _g0 = _u0;                                                         \
        _g1 = _u1;                                                         \
        _g2 = _u2;                                                         \
        _g3 = _u3;                                                         \
        _g4 = _u4;                                                         \
        _g5 = _u5;                                                         \
        _g6 = _u6;                                                         \
        _g7 = _u7;                                                         \
    }                                                                     \
} while (0)

static inline void mul64_by_f512_avx2(uint64_t bp,
                                      uint64_t f0, uint64_t f1,
                                      uint64_t f2, uint64_t f3,
                                      uint64_t f4, uint64_t f5,
                                      uint64_t f6, uint64_t f7,
                                      uint64_t *r0, uint64_t *r1,
                                      uint64_t *r2, uint64_t *r3,
                                      uint64_t *r4, uint64_t *r5,
                                      uint64_t *r6, uint64_t *r7)
{
    uint64_t p0, p1, q0, q1;
    uint64_t s0, s1, t0, t1;
    uint64_t u0, u1, v0, v1;
    uint64_t w0, w1, y0, y1;

    clmul64x4_avx2(bp, bp, bp, bp,
                   f0, f1, f2, f3,
                   &p0, &p1,
                   &q0, &q1,
                   &s0, &s1,
                   &t0, &t1);

    clmul64x4_avx2(bp, bp, bp, bp,
                   f4, f5, f6, f7,
                   &u0, &u1,
                   &v0, &v1,
                   &w0, &w1,
                   &y0, &y1);

    *r0 = p0 ^ y1;
    *r1 = p1 ^ q0;
    *r2 = q1 ^ s0;
    *r3 = s1 ^ t0;
    *r4 = t1 ^ u0;
    *r5 = u1 ^ v0;
    *r6 = v1 ^ w0;
    *r7 = w1 ^ y0;
}

static inline void mul128_by_f512_avx2(uint64_t bp0, uint64_t bp1,
                                       uint64_t f0, uint64_t f1,
                                       uint64_t f2, uint64_t f3,
                                       uint64_t f4, uint64_t f5,
                                       uint64_t f6, uint64_t f7,
                                       uint64_t *r0, uint64_t *r1,
                                       uint64_t *r2, uint64_t *r3,
                                       uint64_t *r4, uint64_t *r5,
                                       uint64_t *r6, uint64_t *r7)
{
    uint64_t a0, a1, a2, a3;
    uint64_t b0, b1, b2, b3;
    uint64_t c0, c1, c2, c3;
    uint64_t d0, d1, d2, d3;

    clmul128_avx2(bp0, bp1, f0, f1, &a0, &a1, &a2, &a3);
    clmul128_avx2(bp0, bp1, f2, f3, &b0, &b1, &b2, &b3);
    clmul128_avx2(bp0, bp1, f4, f5, &c0, &c1, &c2, &c3);
    clmul128_avx2(bp0, bp1, f6, f7, &d0, &d1, &d2, &d3);

    *r0 = a0 ^ d2;
    *r1 = a1 ^ d3;
    *r2 = a2 ^ b0;
    *r3 = a3 ^ b1;
    *r4 = b2 ^ c0;
    *r5 = b3 ^ c1;
    *r6 = c2 ^ d0;
    *r7 = c3 ^ d1;
}

static inline void mul256_by_f512_avx2(uint64_t bp0, uint64_t bp1,
                                       uint64_t bp2, uint64_t bp3,
                                       uint64_t f0, uint64_t f1,
                                       uint64_t f2, uint64_t f3,
                                       uint64_t f4, uint64_t f5,
                                       uint64_t f6, uint64_t f7,
                                       uint64_t *r0, uint64_t *r1,
                                       uint64_t *r2, uint64_t *r3,
                                       uint64_t *r4, uint64_t *r5,
                                       uint64_t *r6, uint64_t *r7)
{
    uint64_t a0, a1, a2, a3, a4, a5, a6, a7;
    uint64_t b0, b1, b2, b3, b4, b5, b6, b7;

    clmul256_avx2(bp0, bp1, bp2, bp3,
                  f0, f1, f2, f3,
                  &a0, &a1, &a2, &a3,
                  &a4, &a5, &a6, &a7);

    clmul256_avx2(bp0, bp1, bp2, bp3,
                  f4, f5, f6, f7,
                  &b0, &b1, &b2, &b3,
                  &b4, &b5, &b6, &b7);

    *r0 = a0 ^ b4;
    *r1 = a1 ^ b5;
    *r2 = a2 ^ b6;
    *r3 = a3 ^ b7;
    *r4 = a4 ^ b0;
    *r5 = a5 ^ b1;
    *r6 = a6 ^ b2;
    *r7 = a7 ^ b3;
}

#define DO_LEVEL_LT64_512(N, MASKN) do {                                \
    t64 = k0 ^ k1 ^ k2 ^ k3 ^ k4 ^ k5 ^ k6 ^ k7;                        \
                                                                         \
    if((N) <= 32) t64 ^= t64 >> 32;                                      \
    if((N) <= 16) t64 ^= t64 >> 16;                                      \
    if((N) <= 8)  t64 ^= t64 >> 8;                                       \
    if((N) <= 4)  t64 ^= t64 >> 4;                                       \
    if((N) <= 2)  t64 ^= t64 >> 2;                                       \
                                                                         \
    tp = t64 & (MASKN);                                                  \
    ap = inv0 & (MASKN);                                                 \
                                                                         \
    MUL_MOD_SMALL_ROTATE_CONST(ap, tp, N, MASKN, bp);                    \
    MUL_LOW_BY_F512_ROTATE_CONST(bp, N);                                 \
                                                                         \
    k0 ^= r0;                                                            \
    k1 ^= r1;                                                            \
    k2 ^= r2;                                                            \
    k3 ^= r3;                                                            \
    k4 ^= r4;                                                            \
    k5 ^= r5;                                                            \
    k6 ^= r6;                                                            \
    k7 ^= r7;                                                            \
                                                                         \
    BLOCK_PREFIX512(N);                                                  \
                                                                         \
    inv0 ^= bp;                                                          \
    inv0 ^= bp << (N);                                                   \
} while (0)

#define DO_LEVEL_64_512() do {                                           \
    tp = k0 ^ k1 ^ k2 ^ k3 ^ k4 ^ k5 ^ k6 ^ k7;                          \
    ap = inv0;                                                           \
                                                                         \
    clmul64_mod_full_avx2(ap, tp, &bp);                                  \
                                                                         \
    mul64_by_f512_avx2(bp, f0, f1, f2, f3, f4, f5, f6, f7,                \
                       &r0, &r1, &r2, &r3,                                \
                       &r4, &r5, &r6, &r7);                               \
                                                                         \
    k0 ^= r0;                                                            \
    k1 ^= r1;                                                            \
    k2 ^= r2;                                                            \
    k3 ^= r3;                                                            \
    k4 ^= r4;                                                            \
    k5 ^= r5;                                                            \
    k6 ^= r6;                                                            \
    k7 ^= r7;                                                            \
                                                                         \
    BLOCK_PREFIX512(64);                                                 \
                                                                         \
    inv0 ^= bp;                                                          \
    inv1 ^= bp;                                                          \
} while (0)

void FastInversion(int16_t *f_inv, int16_t *f)
{
    unsigned int sh;

    uint64_t f0, f1, f2, f3, f4, f5, f6, f7;
    uint64_t k0, k1, k2, k3, k4, k5, k6, k7;
    uint64_t inv0, inv1, inv2, inv3, inv4, inv5, inv6, inv7;

    uint64_t t64;
    uint64_t tp, ap, bp;
    uint64_t tp0, tp1, tp2, tp3;
    uint64_t ap0, ap1, ap2, ap3;
    uint64_t bp0, bp1, bp2, bp3;

    uint64_t r0, r1, r2, r3, r4, r5, r6, r7;
    uint64_t acc64, acc, mask;

    uint64_t x0, x1, x2, x3, x4, x5, x6, x7;

    f0 = pack64_bits_avx2(f +   0);
    f1 = pack64_bits_avx2(f +  64);
    f2 = pack64_bits_avx2(f + 128);
    f3 = pack64_bits_avx2(f + 192);
    f4 = pack64_bits_avx2(f + 256);
    f5 = pack64_bits_avx2(f + 320);
    f6 = pack64_bits_avx2(f + 384);
    f7 = pack64_bits_avx2(f + 448);

    k0 = f0;
    PREFIX64(k0);

    k1 = f1;
    PREFIX64(k1);
    k1 ^= 0ULL - (k0 >> 63);

    k2 = f2;
    PREFIX64(k2);
    k2 ^= 0ULL - (k1 >> 63);

    k3 = f3;
    PREFIX64(k3);
    k3 ^= 0ULL - (k2 >> 63);

    k4 = f4;
    PREFIX64(k4);
    k4 ^= 0ULL - (k3 >> 63);

    k5 = f5;
    PREFIX64(k5);
    k5 ^= 0ULL - (k4 >> 63);

    k6 = f6;
    PREFIX64(k6);
    k6 ^= 0ULL - (k5 >> 63);

    k7 = f7;
    PREFIX64(k7);
    k7 ^= 0ULL - (k6 >> 63);

    acc64 = k0 ^ k1 ^ k2 ^ k3 ^ k4 ^ k5 ^ k6 ^ k7;
    acc = parity64_u64(acc64);

    mask = 0ULL - acc;

    k0 ^= f0 & mask;
    k1 ^= f1 & mask;
    k2 ^= f2 & mask;
    k3 ^= f3 & mask;
    k4 ^= f4 & mask;
    k5 ^= f5 & mask;
    k6 ^= f6 & mask;
    k7 ^= f7 & mask;

    PREFIX64(k0);

    PREFIX64(k1);
    k1 ^= 0ULL - (k0 >> 63);

    PREFIX64(k2);
    k2 ^= 0ULL - (k1 >> 63);

    PREFIX64(k3);
    k3 ^= 0ULL - (k2 >> 63);

    PREFIX64(k4);
    k4 ^= 0ULL - (k3 >> 63);

    PREFIX64(k5);
    k5 ^= 0ULL - (k4 >> 63);

    PREFIX64(k6);
    k6 ^= 0ULL - (k5 >> 63);

    PREFIX64(k7);
    k7 ^= 0ULL - (k6 >> 63);

    inv0 = (acc ^ 1ULL) | (acc << 1);
    inv1 = 0;
    inv2 = 0;
    inv3 = 0;
    inv4 = 0;
    inv5 = 0;
    inv6 = 0;
    inv7 = 0;

    DO_LEVEL_LT64_512(2,  0x0000000000000003ULL);
    DO_LEVEL_LT64_512(4,  0x000000000000000FULL);
    DO_LEVEL_LT64_512(8,  0x00000000000000FFULL);
    DO_LEVEL_LT64_512(16, 0x000000000000FFFFULL);
    DO_LEVEL_LT64_512(32, 0x00000000FFFFFFFFULL);
    DO_LEVEL_64_512();

    tp0 = k0 ^ k2 ^ k4 ^ k6;
    tp1 = k1 ^ k3 ^ k5 ^ k7;

    ap0 = inv0;
    ap1 = inv1;

    clmul128_mod_avx2(ap0, ap1, tp0, tp1, &bp0, &bp1);

    mul128_by_f512_avx2(bp0, bp1,
                        f0, f1, f2, f3, f4, f5, f6, f7,
                        &r0, &r1, &r2, &r3,
                        &r4, &r5, &r6, &r7);

    k0 ^= r0;
    k1 ^= r1;
    k2 ^= r2;
    k3 ^= r3;
    k4 ^= r4;
    k5 ^= r5;
    k6 ^= r6;
    k7 ^= r7;

    BLOCK_PREFIX512(128);

    inv0 ^= bp0;
    inv1 ^= bp1;
    inv2 ^= bp0;
    inv3 ^= bp1;

    tp0 = k0 ^ k4;
    tp1 = k1 ^ k5;
    tp2 = k2 ^ k6;
    tp3 = k3 ^ k7;

    ap0 = inv0;
    ap1 = inv1;
    ap2 = inv2;
    ap3 = inv3;

    clmul256_mod_avx2(ap0, ap1, ap2, ap3,
                      tp0, tp1, tp2, tp3,
                      &bp0, &bp1, &bp2, &bp3);

    mul256_by_f512_avx2(bp0, bp1, bp2, bp3,
                        f0, f1, f2, f3, f4, f5, f6, f7,
                        &r0, &r1, &r2, &r3,
                        &r4, &r5, &r6, &r7);

    k0 ^= r0;
    k1 ^= r1;
    k2 ^= r2;
    k3 ^= r3;
    k4 ^= r4;
    k5 ^= r5;
    k6 ^= r6;
    k7 ^= r7;

    BLOCK_PREFIX512(256);

    inv0 ^= bp0;
    inv1 ^= bp1;
    inv2 ^= bp2;
    inv3 ^= bp3;
    inv4 ^= bp0;
    inv5 ^= bp1;
    inv6 ^= bp2;
    inv7 ^= bp3;

    unpack64_bits_avx2(f_inv +   0, inv0);
    unpack64_bits_avx2(f_inv +  64, inv1);
    unpack64_bits_avx2(f_inv + 128, inv2);
    unpack64_bits_avx2(f_inv + 192, inv3);
    unpack64_bits_avx2(f_inv + 256, inv4);
    unpack64_bits_avx2(f_inv + 320, inv5);
    unpack64_bits_avx2(f_inv + 384, inv6);
    unpack64_bits_avx2(f_inv + 448, inv7);
}

void poly_generate_gf(int16_t *a, const uint8_t *seed, uint8_t nonce)
{
    uint8_t buf[ZEN_N_LEN_BYTES*5];

    zen_pseudoXOF(ZEN_N*5, seed, SEED_LEN_BYTES*8, buf, nonce);
    tenary3_32(a, buf);
}

void poly_generate_se(int16_t *a, const uint8_t *seed, uint8_t nonce)
{
    uint8_t buf[ZEN_N_LEN_BYTES*3];

    zen_pseudoXOF(ZEN_N*3, seed, SEED_LEN_BYTES*8, buf, nonce);
    tenary1_8(a, buf);
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
    uint64_t tmp[410] = {0};   /* 409 packed 48-bit blocks + 1 packed 29-bit block */
    uint64_t res[308] = {0};   /* ceil((409*48 + 29)/64) = 308 */

    idx = 0;
    for(i = 0; i < ZEN_N - 3; i += 5)
    {
        tmp[idx] =
              (uint64_t)(uint16_t)a[i]
            + (uint64_t)(uint16_t)a[i + 1] * pack_table[1]
            + (uint64_t)(uint16_t)a[i + 2] * pack_table[2]
            + (uint64_t)(uint16_t)a[i + 3] * pack_table[3]
            + (uint64_t)(uint16_t)a[i + 4] * pack_table[4];
        idx++;
    }

    /* last 3 coefficients -> 29 bits */
    tmp[409] =
          (uint64_t)(uint16_t)a[ZEN_N - 3]
        + (uint64_t)(uint16_t)a[ZEN_N - 2] * pack_table[1]
        + (uint64_t)(uint16_t)a[ZEN_N - 1] * pack_table[2];

    idx = 0;

    /* first 408 blocks = 102 groups of 4 blocks -> 306 uint64 words */
    for(i = 0; i < 408; i += 4)
    {
        uint64_t x0 = tmp[i];
        uint64_t x1 = tmp[i + 1];
        uint64_t x2 = tmp[i + 2];
        uint64_t x3 = tmp[i + 3];

        res[idx++] = x0 | ((x3 & 0xFFFFULL) << 48);
        res[idx++] = x1 | (((x3 >> 16) & 0xFFFFULL) << 48);
        res[idx++] = x2 | (((x3 >> 32) & 0xFFFFULL) << 48);
    }

    /* remaining one 48-bit block tmp[408] + one 29-bit block tmp[409] */
    res[idx++] = tmp[408] | ((tmp[409] & 0xFFFFULL) << 48);
    res[idx++] = (tmp[409] >> 16) & 0x1FFFULL;

    memcpy(pa, (const uint8_t *)res, ZEN_INDCPA_PUBLICKEY_LEN_BYTES);
}

void poly_publickey_unpack(int16_t *a, const uint8_t *pa)
{
    int i, idx;
    uint64_t res[308] = {0};
    uint64_t tmp[410] = {0};
    const uint64_t MASK48   = 0x0000FFFFFFFFFFFFULL;
    const uint64_t MASK13   = 0x1FFFULL;
    const uint64_t DIV769_M = 374811932576999ULL; /* exact for x < 769^5 */

    memcpy((uint8_t *)res, pa, 2458);

    idx = 307;

    /* last packed 29-bit block */
    tmp[409] = (res[idx--] & MASK13) << 16;

    /* remaining one 48-bit block + low 16 bits of tmp[409] */
    tmp[408] = res[idx] & MASK48;
    tmp[409] |= ((res[idx--] >> 48) & 0xFFFFULL);

    /* recover first 408 packed 48-bit blocks */
    for(i = 407; i > 0; i -= 4)
    {
        tmp[i - 1] = res[idx] & MASK48;
        tmp[i]     = ((res[idx--] >> 48) & 0xFFFFULL) << 32;

        tmp[i - 2] = res[idx] & MASK48;
        tmp[i]    |= ((res[idx--] >> 48) & 0xFFFFULL) << 16;

        tmp[i - 3] = res[idx] & MASK48;
        tmp[i]    |= ((res[idx--] >> 48) & 0xFFFFULL);
    }

    /* decode last 3 coefficients from 29-bit block */
    for(i = 0; i < 3; i++)
    {
        uint64_t x = tmp[409];
        uint64_t q = (uint64_t)(((__uint128_t)x * DIV769_M) >> 58);
        uint64_t r = x - q * 769ULL;

        a[ZEN_N - 3 + i] = (int16_t)r;
        tmp[409] = q;
    }

    idx = 0;
    for(i = 0; i < ZEN_N - 3; i += 5)
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
