// SPDX-FileCopyrightText: 2026 The Project OSIDH-LD Authors
// SPDX-License-Identifier: Apache-2.0

/*
 * The implementation in this file refers to the algorithm
 * proposed by Cheng Hao et al. in the article
 * "Batching CSIDH Group Actions using AVX-512"
 * (DOI: 10.46586/tches.v2021.i4.618-649)
 */

#include "v8fp.h"

void v8fp_pack8(v8fp_t* x, const fp_t* a1, const fp_t* a2, const fp_t* a3, const fp_t* a4, const fp_t* a5, const fp_t* a6, const fp_t* a7, const fp_t* a8)
{
    for (int i = 0; i < NWORDS_FIELD; i++)
    {
        ((uint64_t*)&((*x)[i]))[0] = (*a1)[i];
        ((uint64_t*)&((*x)[i]))[1] = (*a2)[i];
        ((uint64_t*)&((*x)[i]))[2] = (*a3)[i];
        ((uint64_t*)&((*x)[i]))[3] = (*a4)[i];
        ((uint64_t*)&((*x)[i]))[4] = (*a5)[i];
        ((uint64_t*)&((*x)[i]))[5] = (*a6)[i];
        ((uint64_t*)&((*x)[i]))[6] = (*a7)[i];
        ((uint64_t*)&((*x)[i]))[7] = (*a8)[i];
    }
}

void v8fp_unpack8(fp_t* a1, fp_t* a2, fp_t* a3, fp_t* a4, fp_t* a5, fp_t* a6, fp_t* a7, fp_t* a8, const v8fp_t* x)
{
    for (int i = 0; i < NWORDS_FIELD; i++)
    {
        (*a1)[i] = ((uint64_t*)&((*x)[i]))[0];
        (*a2)[i] = ((uint64_t*)&((*x)[i]))[1];
        (*a3)[i] = ((uint64_t*)&((*x)[i]))[2];
        (*a4)[i] = ((uint64_t*)&((*x)[i]))[3];
        (*a5)[i] = ((uint64_t*)&((*x)[i]))[4];
        (*a6)[i] = ((uint64_t*)&((*x)[i]))[5];
        (*a7)[i] = ((uint64_t*)&((*x)[i]))[6];
        (*a8)[i] = ((uint64_t*)&((*x)[i]))[7];
    }
}

void v8fp_pack7(v8fp_t* x, const fp_t* a1, const fp_t* a2, const fp_t* a3, const fp_t* a4, const fp_t* a5, const fp_t* a6, const fp_t* a7)
{
    for (int i = 0; i < NWORDS_FIELD; i++)
    {
        ((uint64_t*)&((*x)[i]))[0] = (*a1)[i];
        ((uint64_t*)&((*x)[i]))[1] = (*a2)[i];
        ((uint64_t*)&((*x)[i]))[2] = (*a3)[i];
        ((uint64_t*)&((*x)[i]))[3] = (*a4)[i];
        ((uint64_t*)&((*x)[i]))[4] = (*a5)[i];
        ((uint64_t*)&((*x)[i]))[5] = (*a6)[i];
        ((uint64_t*)&((*x)[i]))[6] = (*a7)[i];
    }
}

void v8fp_unpack7(fp_t* a1, fp_t* a2, fp_t* a3, fp_t* a4, fp_t* a5, fp_t* a6, fp_t* a7, const v8fp_t* x)
{
    for (int i = 0; i < NWORDS_FIELD; i++)
    {
        (*a1)[i] = ((uint64_t*)&((*x)[i]))[0];
        (*a2)[i] = ((uint64_t*)&((*x)[i]))[1];
        (*a3)[i] = ((uint64_t*)&((*x)[i]))[2];
        (*a4)[i] = ((uint64_t*)&((*x)[i]))[3];
        (*a5)[i] = ((uint64_t*)&((*x)[i]))[4];
        (*a6)[i] = ((uint64_t*)&((*x)[i]))[5];
        (*a7)[i] = ((uint64_t*)&((*x)[i]))[6];
    }
}

void v8fp_pack6(v8fp_t* x, const fp_t* a1, const fp_t* a2, const fp_t* a3, const fp_t* a4, const fp_t* a5, const fp_t* a6)
{
    for (int i = 0; i < NWORDS_FIELD; i++)
    {
        ((uint64_t*)&((*x)[i]))[0] = (*a1)[i];
        ((uint64_t*)&((*x)[i]))[1] = (*a2)[i];
        ((uint64_t*)&((*x)[i]))[2] = (*a3)[i];
        ((uint64_t*)&((*x)[i]))[3] = (*a4)[i];
        ((uint64_t*)&((*x)[i]))[4] = (*a5)[i];
        ((uint64_t*)&((*x)[i]))[5] = (*a6)[i];
    }
}

void v8fp_unpack6(fp_t* a1, fp_t* a2, fp_t* a3, fp_t* a4, fp_t* a5, fp_t* a6, const v8fp_t* x)
{
    for (int i = 0; i < NWORDS_FIELD; i++)
    {
        (*a1)[i] = ((uint64_t*)&((*x)[i]))[0];
        (*a2)[i] = ((uint64_t*)&((*x)[i]))[1];
        (*a3)[i] = ((uint64_t*)&((*x)[i]))[2];
        (*a4)[i] = ((uint64_t*)&((*x)[i]))[3];
        (*a5)[i] = ((uint64_t*)&((*x)[i]))[4];
        (*a6)[i] = ((uint64_t*)&((*x)[i]))[5];
    }
}

void v8fp_pack5(v8fp_t* x, const fp_t* a1, const fp_t* a2, const fp_t* a3, const fp_t* a4, const fp_t* a5)
{
    for (int i = 0; i < NWORDS_FIELD; i++)
    {
        ((uint64_t*)&((*x)[i]))[0] = (*a1)[i];
        ((uint64_t*)&((*x)[i]))[1] = (*a2)[i];
        ((uint64_t*)&((*x)[i]))[2] = (*a3)[i];
        ((uint64_t*)&((*x)[i]))[3] = (*a4)[i];
        ((uint64_t*)&((*x)[i]))[4] = (*a5)[i];
    }
}

void v8fp_unpack5(fp_t* a1, fp_t* a2, fp_t* a3, fp_t* a4, fp_t* a5, const v8fp_t* x)
{
    for (int i = 0; i < NWORDS_FIELD; i++)
    {
        (*a1)[i] = ((uint64_t*)&((*x)[i]))[0];
        (*a2)[i] = ((uint64_t*)&((*x)[i]))[1];
        (*a3)[i] = ((uint64_t*)&((*x)[i]))[2];
        (*a4)[i] = ((uint64_t*)&((*x)[i]))[3];
        (*a5)[i] = ((uint64_t*)&((*x)[i]))[4];
    }
}

void v8fp_pack4(v8fp_t* x, const fp_t* a1, const fp_t* a2, const fp_t* a3, const fp_t* a4)
{
    for (int i = 0; i < NWORDS_FIELD; i++)
    {
        ((uint64_t*)&((*x)[i]))[0] = (*a1)[i];
        ((uint64_t*)&((*x)[i]))[1] = (*a2)[i];
        ((uint64_t*)&((*x)[i]))[2] = (*a3)[i];
        ((uint64_t*)&((*x)[i]))[3] = (*a4)[i];
    }
}

void v8fp_unpack4(fp_t* a1, fp_t* a2, fp_t* a3, fp_t* a4, const v8fp_t* x)
{
    for (int i = 0; i < NWORDS_FIELD; i++)
    {
        (*a1)[i] = ((uint64_t*)&((*x)[i]))[0];
        (*a2)[i] = ((uint64_t*)&((*x)[i]))[1];
        (*a3)[i] = ((uint64_t*)&((*x)[i]))[2];
        (*a4)[i] = ((uint64_t*)&((*x)[i]))[3];
    }
}

void v8fp_pack3(v8fp_t* x, const fp_t* a1, const fp_t* a2, const fp_t* a3)
{
    for (int i = 0; i < NWORDS_FIELD; i++)
    {
        ((uint64_t*)&((*x)[i]))[0] = (*a1)[i];
        ((uint64_t*)&((*x)[i]))[1] = (*a2)[i];
        ((uint64_t*)&((*x)[i]))[2] = (*a3)[i];
    }
}

void v8fp_unpack3(fp_t* a1, fp_t* a2, fp_t* a3, const v8fp_t* x)
{
    for (int i = 0; i < NWORDS_FIELD; i++)
    {
        (*a1)[i] = ((uint64_t*)&((*x)[i]))[0];
        (*a2)[i] = ((uint64_t*)&((*x)[i]))[1];
        (*a3)[i] = ((uint64_t*)&((*x)[i]))[2];
    }
}

void v8fp_pack2(v8fp_t* x, const fp_t* a1, const fp_t* a2)
{
    for (int i = 0; i < NWORDS_FIELD; i++)
    {
        ((uint64_t*)&((*x)[i]))[0] = (*a1)[i];
        ((uint64_t*)&((*x)[i]))[1] = (*a2)[i];
    }
}

void v8fp_unpack2(fp_t* a1, fp_t* a2, const v8fp_t* x)
{
    for (int i = 0; i < NWORDS_FIELD; i++)
    {
        (*a1)[i] = ((uint64_t*)&((*x)[i]))[0];
        (*a2)[i] = ((uint64_t*)&((*x)[i]))[1];
    }
}

void v8fp_add(v8fp_t r, const v8fp_t a, const v8fp_t b)
{
    __m512i a0 = a[0], a1 = a[1], a2 = a[2], a3 = a[3], a4 = a[4];
    __m512i b0 = b[0], b1 = b[1], b2 = b[2], b3 = b[3], b4 = b[4];
    __m512i r0, r1, r2, r3, r4, smask;
    const __m512i vp0 = _mm512_set1_epi64(ht_pmul2[0]);
    const __m512i vp1 = _mm512_set1_epi64(ht_pmul2[1]);
    const __m512i vp2 = _mm512_set1_epi64(ht_pmul2[2]);
    const __m512i vp3 = _mm512_set1_epi64(ht_pmul2[3]);
    const __m512i vp4 = _mm512_set1_epi64(ht_pmul2[4]);
    const __m512i vbmask = _mm512_set1_epi64(HT_BMASK);

    r0 = _mm512_add_epi64(a0, b0);
    r1 = _mm512_add_epi64(a1, b1);
    r2 = _mm512_add_epi64(a2, b2);
    r3 = _mm512_add_epi64(a3, b3);
    r4 = _mm512_add_epi64(a4, b4);

    r0 = _mm512_sub_epi64(r0, vp0);
    r1 = _mm512_sub_epi64(r1, vp1);
    r2 = _mm512_sub_epi64(r2, vp2);
    r3 = _mm512_sub_epi64(r3, vp3);
    r4 = _mm512_sub_epi64(r4, vp4);

    r1  = _mm512_add_epi64(r1, _mm512_srai_epi64(r0, HT_BRADIX)); r0  = _mm512_and_si512(r0, vbmask);
    r2  = _mm512_add_epi64(r2, _mm512_srai_epi64(r1, HT_BRADIX)); r1  = _mm512_and_si512(r1, vbmask);
    r3  = _mm512_add_epi64(r3, _mm512_srai_epi64(r2, HT_BRADIX)); r2  = _mm512_and_si512(r2, vbmask);
    r4  = _mm512_add_epi64(r4, _mm512_srai_epi64(r3, HT_BRADIX)); r3  = _mm512_and_si512(r3, vbmask);

    smask = _mm512_srai_epi64(r4, 63);
    r0 = _mm512_add_epi64(r0, _mm512_and_si512(vp0, smask)); 
    r1 = _mm512_add_epi64(r1, _mm512_and_si512(vp1, smask));
    r2 = _mm512_add_epi64(r2, _mm512_and_si512(vp2, smask)); 
    r3 = _mm512_add_epi64(r3, _mm512_and_si512(vp3, smask));
    r4 = _mm512_add_epi64(r4, _mm512_and_si512(vp4, smask));

    r1 = _mm512_add_epi64(r1, _mm512_srli_epi64(r0, HT_BRADIX)); r0 = _mm512_and_si512(r0, vbmask);
    r2 = _mm512_add_epi64(r2, _mm512_srli_epi64(r1, HT_BRADIX)); r1 = _mm512_and_si512(r1, vbmask);
    r3 = _mm512_add_epi64(r3, _mm512_srli_epi64(r2, HT_BRADIX)); r2 = _mm512_and_si512(r2, vbmask);
    r4 = _mm512_add_epi64(r4, _mm512_srli_epi64(r3, HT_BRADIX)); r3 = _mm512_and_si512(r3, vbmask);
    r4 = _mm512_and_si512(r4, vbmask);

    r[0] = r0; r[1] = r1; r[2] = r2; r[3] = r3; r[4] = r4;
}

void v8fp_sub(v8fp_t r, const v8fp_t a, const v8fp_t b)
{
    __m512i a0 = a[0], a1 = a[1], a2 = a[2], a3 = a[3], a4 = a[4];
    __m512i b0 = b[0], b1 = b[1], b2 = b[2], b3 = b[3], b4 = b[4];
    __m512i r0, r1, r2, r3, r4, smask;
    const __m512i vp0 = _mm512_set1_epi64(ht_pmul2[0]);
    const __m512i vp1 = _mm512_set1_epi64(ht_pmul2[1]);
    const __m512i vp2 = _mm512_set1_epi64(ht_pmul2[2]);
    const __m512i vp3 = _mm512_set1_epi64(ht_pmul2[3]);
    const __m512i vp4 = _mm512_set1_epi64(ht_pmul2[4]);
    const __m512i vbmask = _mm512_set1_epi64(HT_BMASK);

    r0 = _mm512_sub_epi64(a0, b0);
    r1 = _mm512_sub_epi64(a1, b1);
    r2 = _mm512_sub_epi64(a2, b2);
    r3 = _mm512_sub_epi64(a3, b3);
    r4 = _mm512_sub_epi64(a4, b4);

    r1  = _mm512_add_epi64(r1, _mm512_srai_epi64(r0, HT_BRADIX)); r0  = _mm512_and_si512(r0, vbmask);
    r2  = _mm512_add_epi64(r2, _mm512_srai_epi64(r1, HT_BRADIX)); r1  = _mm512_and_si512(r1, vbmask);
    r3  = _mm512_add_epi64(r3, _mm512_srai_epi64(r2, HT_BRADIX)); r2  = _mm512_and_si512(r2, vbmask);
    r4  = _mm512_add_epi64(r4, _mm512_srai_epi64(r3, HT_BRADIX)); r3  = _mm512_and_si512(r3, vbmask);

    smask = _mm512_srai_epi64(r4, 63);
    r0 = _mm512_add_epi64(r0, _mm512_and_si512(vp0, smask));
    r1 = _mm512_add_epi64(r1, _mm512_and_si512(vp1, smask));
    r2 = _mm512_add_epi64(r2, _mm512_and_si512(vp2, smask)); 
    r3 = _mm512_add_epi64(r3, _mm512_and_si512(vp3, smask));
    r4 = _mm512_add_epi64(r4, _mm512_and_si512(vp4, smask));

    r1 = _mm512_add_epi64(r1, _mm512_srli_epi64(r0, HT_BRADIX)); r0 = _mm512_and_si512(r0, vbmask);
    r2 = _mm512_add_epi64(r2, _mm512_srli_epi64(r1, HT_BRADIX)); r1 = _mm512_and_si512(r1, vbmask);
    r3 = _mm512_add_epi64(r3, _mm512_srli_epi64(r2, HT_BRADIX)); r2 = _mm512_and_si512(r2, vbmask);
    r4 = _mm512_add_epi64(r4, _mm512_srli_epi64(r3, HT_BRADIX)); r3 = _mm512_and_si512(r3, vbmask);
    r4 = _mm512_and_si512(r4, vbmask);

    r[0] = r0; r[1] = r1; r[2] = r2; r[3] = r3; r[4] = r4;
}

void v8fp_neg(v8fp_t r, const v8fp_t a)
{
    v8fp_t zero;
    zero[0] = _mm512_setzero_si512();
    zero[1] = _mm512_setzero_si512();
    zero[2] = _mm512_setzero_si512();
    zero[3] = _mm512_setzero_si512();
    zero[4] = _mm512_setzero_si512();
    v8fp_sub(r, zero, a);
}

void v8fp_mul(v8fp_t r, const v8fp_t a, const v8fp_t b)
{
    __m512i a0 = a[0], a1 = a[1], a2 = a[2], a3 = a[3], a4 = a[4];
    __m512i b0 = b[0], b1 = b[1], b2 = b[2], b3 = b[3], b4 = b[4];
    __m512i z0 = _mm512_setzero_si512(), z1 = _mm512_setzero_si512();
    __m512i z2 = _mm512_setzero_si512(), z3 = _mm512_setzero_si512();
    __m512i z4 = _mm512_setzero_si512(), z5 = _mm512_setzero_si512();
    __m512i z6 = _mm512_setzero_si512(), z7 = _mm512_setzero_si512();
    __m512i z8 = _mm512_setzero_si512(), z9 = _mm512_setzero_si512();
    __m512i r0, r1, r2, r3, r4, u;
    const __m512i vp0 = _mm512_set1_epi64(ht_p[0]);
    const __m512i vp1 = _mm512_set1_epi64(ht_p[1]);
    const __m512i vp2 = _mm512_set1_epi64(ht_p[2]);
    const __m512i vp3 = _mm512_set1_epi64(ht_p[3]);
    const __m512i vp4 = _mm512_set1_epi64(ht_p[4]);
    const __m512i vbmask = _mm512_set1_epi64(HT_BMASK);
    const __m512i vw = _mm512_set1_epi64(HT_MONTW);
    const __m512i zero = _mm512_setzero_si512();

    z0 = _mm512_madd52lo_epu64(z0, a0, b0);
    z1 = _mm512_madd52hi_epu64(z1, a0, b0);

    z1 = _mm512_madd52lo_epu64(z1, a0, b1);
    z1 = _mm512_madd52lo_epu64(z1, a1, b0);
    z2 = _mm512_madd52hi_epu64(z2, a0, b1);
    z2 = _mm512_madd52hi_epu64(z2, a1, b0);

    z2 = _mm512_madd52lo_epu64(z2, a0, b2);
    z2 = _mm512_madd52lo_epu64(z2, a1, b1);
    z2 = _mm512_madd52lo_epu64(z2, a2, b0);
    z3 = _mm512_madd52hi_epu64(z3, a0, b2);
    z3 = _mm512_madd52hi_epu64(z3, a1, b1);
    z3 = _mm512_madd52hi_epu64(z3, a2, b0);

    z3 = _mm512_madd52lo_epu64(z3, a0, b3);
    z3 = _mm512_madd52lo_epu64(z3, a1, b2);
    z3 = _mm512_madd52lo_epu64(z3, a2, b1);
    z3 = _mm512_madd52lo_epu64(z3, a3, b0);
    z4 = _mm512_madd52hi_epu64(z4, a0, b3);
    z4 = _mm512_madd52hi_epu64(z4, a1, b2);
    z4 = _mm512_madd52hi_epu64(z4, a2, b1);
    z4 = _mm512_madd52hi_epu64(z4, a3, b0);

    z4 = _mm512_madd52lo_epu64(z4, a0, b4);
    z4 = _mm512_madd52lo_epu64(z4, a1, b3);
    z4 = _mm512_madd52lo_epu64(z4, a2, b2);
    z4 = _mm512_madd52lo_epu64(z4, a3, b1);
    z4 = _mm512_madd52lo_epu64(z4, a4, b0);
    z5 = _mm512_madd52hi_epu64(z5, a0, b4);
    z5 = _mm512_madd52hi_epu64(z5, a1, b3);
    z5 = _mm512_madd52hi_epu64(z5, a2, b2);
    z5 = _mm512_madd52hi_epu64(z5, a3, b1);
    z5 = _mm512_madd52hi_epu64(z5, a4, b0);

    u = _mm512_madd52lo_epu64(zero, z0, vw); 
    z0 = _mm512_madd52lo_epu64(z0, u, vp0); z1  = _mm512_madd52hi_epu64(z1, u, vp0);
    z1 = _mm512_madd52lo_epu64(z1, u, vp1); z2  = _mm512_madd52hi_epu64(z2, u, vp1);
    z2 = _mm512_madd52lo_epu64(z2, u, vp2); z3  = _mm512_madd52hi_epu64(z3, u, vp2);
    z3 = _mm512_madd52lo_epu64(z3, u, vp3); z4  = _mm512_madd52hi_epu64(z4, u, vp3);
    z4 = _mm512_madd52lo_epu64(z4, u, vp4); z5  = _mm512_madd52hi_epu64(z5, u, vp4);
    z1 = _mm512_add_epi64(z1, _mm512_srli_epi64(z0, HT_BRADIX));

    z5 = _mm512_madd52lo_epu64(z5, a1, b4);
    z5 = _mm512_madd52lo_epu64(z5, a2, b3);
    z5 = _mm512_madd52lo_epu64(z5, a3, b2);
    z5 = _mm512_madd52lo_epu64(z5, a4, b1);
    z6 = _mm512_madd52hi_epu64(z6, a1, b4);
    z6 = _mm512_madd52hi_epu64(z6, a2, b3);
    z6 = _mm512_madd52hi_epu64(z6, a3, b2);
    z6 = _mm512_madd52hi_epu64(z6, a4, b1);

    u = _mm512_madd52lo_epu64(zero, z1, vw); 
    z1  = _mm512_madd52lo_epu64(z1, u, vp0); z2  = _mm512_madd52hi_epu64(z2, u, vp0);
    z2  = _mm512_madd52lo_epu64(z2, u, vp1); z3  = _mm512_madd52hi_epu64(z3, u, vp1);
    z3  = _mm512_madd52lo_epu64(z3, u, vp2); z4  = _mm512_madd52hi_epu64(z4, u, vp2);
    z4  = _mm512_madd52lo_epu64(z4, u, vp3); z5  = _mm512_madd52hi_epu64(z5, u, vp3);
    z5  = _mm512_madd52lo_epu64(z5, u, vp4); z6  = _mm512_madd52hi_epu64(z6, u, vp4);
    z2 = _mm512_add_epi64(z2, _mm512_srli_epi64(z1, HT_BRADIX));

    z6 = _mm512_madd52lo_epu64(z6, a2, b4);
    z6 = _mm512_madd52lo_epu64(z6, a3, b3);
    z6 = _mm512_madd52lo_epu64(z6, a4, b2);
    z7 = _mm512_madd52hi_epu64(z7, a2, b4);
    z7 = _mm512_madd52hi_epu64(z7, a3, b3);
    z7 = _mm512_madd52hi_epu64(z7, a4, b2);

    u = _mm512_madd52lo_epu64(zero, z2, vw); 
    z2  = _mm512_madd52lo_epu64(z2, u, vp0); z3  = _mm512_madd52hi_epu64(z3, u, vp0);
    z3  = _mm512_madd52lo_epu64(z3, u, vp1); z4  = _mm512_madd52hi_epu64(z4, u, vp1);
    z4  = _mm512_madd52lo_epu64(z4, u, vp2); z5  = _mm512_madd52hi_epu64(z5, u, vp2);
    z5  = _mm512_madd52lo_epu64(z5, u, vp3); z6  = _mm512_madd52hi_epu64(z6, u, vp3);
    z6  = _mm512_madd52lo_epu64(z6, u, vp4); z7  = _mm512_madd52hi_epu64(z7, u, vp4);
    z3 = _mm512_add_epi64(z3, _mm512_srli_epi64(z2, HT_BRADIX));

    z7 = _mm512_madd52lo_epu64(z7, a3, b4);
    z7 = _mm512_madd52lo_epu64(z7, a4, b3);
    z8 = _mm512_madd52hi_epu64(z8, a3, b4);
    z8 = _mm512_madd52hi_epu64(z8, a4, b3);

    u = _mm512_madd52lo_epu64(zero, z3, vw); 
    z3  = _mm512_madd52lo_epu64(z3, u, vp0); z4  = _mm512_madd52hi_epu64(z4, u, vp0);
    z4  = _mm512_madd52lo_epu64(z4, u, vp1); z5  = _mm512_madd52hi_epu64(z5, u, vp1);
    z5  = _mm512_madd52lo_epu64(z5, u, vp2); z6  = _mm512_madd52hi_epu64(z6, u, vp2);
    z6  = _mm512_madd52lo_epu64(z6, u, vp3); z7  = _mm512_madd52hi_epu64(z7, u, vp3);
    z7  = _mm512_madd52lo_epu64(z7, u, vp4); z8  = _mm512_madd52hi_epu64(z8, u, vp4);
    z4 = _mm512_add_epi64(z4, _mm512_srli_epi64(z3, HT_BRADIX));

    z8 = _mm512_madd52lo_epu64(z8, a4, b4);
    z9 = _mm512_madd52hi_epu64(z9, a4, b4);

    u = _mm512_madd52lo_epu64(zero, z4, vw); 
    z4  = _mm512_madd52lo_epu64(z4, u, vp0); z5  = _mm512_madd52hi_epu64(z5, u, vp0);
    z5  = _mm512_madd52lo_epu64(z5, u, vp1); z6  = _mm512_madd52hi_epu64(z6, u, vp1);
    z6  = _mm512_madd52lo_epu64(z6, u, vp2); z7  = _mm512_madd52hi_epu64(z7, u, vp2);
    z7  = _mm512_madd52lo_epu64(z7, u, vp3); z8  = _mm512_madd52hi_epu64(z8, u, vp3);
    z8  = _mm512_madd52lo_epu64(z8, u, vp4); z9  = _mm512_madd52hi_epu64(z9, u, vp4);
    z5 = _mm512_add_epi64(z5, _mm512_srli_epi64(z4, HT_BRADIX));

    z6 = _mm512_add_epi64(z6, _mm512_srli_epi64(z5, HT_BRADIX)); z5 = _mm512_and_si512(z5, vbmask);
    z7 = _mm512_add_epi64(z7, _mm512_srli_epi64(z6, HT_BRADIX)); z6 = _mm512_and_si512(z6, vbmask);
    z8 = _mm512_add_epi64(z8, _mm512_srli_epi64(z7, HT_BRADIX)); z7 = _mm512_and_si512(z7, vbmask);
    z9 = _mm512_add_epi64(z9, _mm512_srli_epi64(z8, HT_BRADIX)); z8 = _mm512_and_si512(z8, vbmask);

    // ---------------------------------------------------------------------------

    r[0] = z5; r[1] = z6; r[2] = z7; r[3] = z8; r[4] = z9;
}

void v8fp_set(v8fp_t r, const digit_t a)
{
    r[0] = _mm512_set1_epi64(a);
    r[1] = _mm512_setzero_si512();
    r[2] = _mm512_setzero_si512();
    r[3] = _mm512_setzero_si512();
    r[4] = _mm512_setzero_si512();
}

void v8fp_copy(v8fp_t r, const v8fp_t a)
{
    r[0] = a[0];
    r[1] = a[1];
    r[2] = a[2];
    r[3] = a[3];
    r[4] = a[4];
}

void v8fp_mont_setone(v8fp_t r)
{
    r[0] = _mm512_set1_epi64(ht_montR[0]);
    r[1] = _mm512_set1_epi64(ht_montR[1]);
    r[2] = _mm512_set1_epi64(ht_montR[2]);
    r[3] = _mm512_set1_epi64(ht_montR[3]);
    r[4] = _mm512_set1_epi64(ht_montR[4]);
}

void v8fp_swap(v8fp_t P, v8fp_t Q, const __m512i option)
{ // If option = 0 then P <- P and Q <- Q, else if option = 0xFF...FF then P <- Q and Q <- P
    __m512i temp;

    for (int i = 0; i < NWORDS_FIELD; i++) {
        temp = option & (P[i] ^ Q[i]);
        P[i] = temp ^ P[i];
        Q[i] = temp ^ Q[i];
    }
}