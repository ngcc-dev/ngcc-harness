/*
Copyright (c) 2026 Yu Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
File Description: Declares the ZEN key-encapsulation mechanism layer for the optimized ZEN-128 instance.
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <immintrin.h>
#include "params.h"
#include "sample.h"
#include "ntt.h"
#include "poly.h"
#include "drng.h"

extern DRNG_ctx drng_algorithm;

void pke_keygen_derand(unsigned char *pk, unsigned char *sk, const unsigned char *seed)
{
    unsigned i;
    uint8_t nonce;
    ALIGN32 int16_t f[ZEN_N], g[ZEN_N], f2[ZEN_N4];
    ALIGN32 int16_t t0[ZEN_N], t1[ZEN_N];
    __m256i vdata0, vdata1, vdata2;

    nonce = 0;
    //generate f
    poly_generate_gf(f, seed, nonce++);
    //chechk f
    for(;;)
    {
        vdata0 = _mm256_set1_epi16(1);
        for(i = 0; i < ZEN_N4; i += 16)
        {

            vdata1 = _mm256_load_si256((__m256i*)(f + i));
            vdata1 = _mm256_and_si256(vdata0, vdata1);
            vdata2 = _mm256_load_si256((__m256i*)(f + i + ZEN_N4));
            vdata2 = _mm256_and_si256(vdata0, vdata2);
            vdata1 = _mm256_xor_si256(vdata1, vdata2);
            vdata2 = _mm256_load_si256((__m256i*)(f + i + 2 * ZEN_N4));
            vdata2 = _mm256_and_si256(vdata0, vdata2);
            vdata1 = _mm256_xor_si256(vdata1, vdata2);
            vdata2 = _mm256_load_si256((__m256i*)(f + i + 3 * ZEN_N4));
            vdata2 = _mm256_and_si256(vdata0, vdata2);
            vdata1 = _mm256_xor_si256(vdata1, vdata2);
            _mm256_store_si256((__m256i *)(t0 + i), vdata1);
        }
        if(check_poly_inv_Z2(t0))
        {
            poly_generate_gf(f, seed, nonce++);
            continue;
        }
        poly_ntt_mq(f, nttdata);
        if(check_poly_inv_Zq(f))
        {
            poly_generate_gf(f, seed, nonce++);
            continue;
        }

        poly_baseinv_ntt(t1, f); //t1 = f^(-1) mod (x^n+1, q)
        FastInversion(f2, t0); //t0 = f2
        
        break;
    }

    //generate g
    poly_generate_gf(g, seed, nonce++);
    //check g
    for(;;)
    {
        poly_ntt_mq(g, nttdata);
        if(check_poly_inv_Zq(g))
        {
            poly_generate_gf(g, seed, nonce++);
            continue;
        }
        break;
    }

    poly_basemul_ntt_mq(t0, g, t1, muldata);
    shuffled_order_2_bitreversed_order(t0);
    shuffled_order_2_bitreversed_order(f);
    poly_publickey_pack(pk, t0);
    poly_secretkey_pack(sk, f);
    poly_bit2byte_pack(sk+ZEN_F_NTT_PACK, f2, ZEN_N4);

}

void pke_keygen(unsigned char *pk, unsigned char *sk)
{
    ALIGN32 uint8_t seed[SEED_LEN_BYTES];

    get_random_number(&drng_algorithm, seed, SEED_LEN_BYTES*8);
    pke_keygen_derand(pk, sk, seed);
}

void pke_enc(unsigned char *pk, unsigned char *m, unsigned char *seed, unsigned char *ct)
{
    unsigned i;
    uint8_t nonce;
    ALIGN32 int16_t h[ZEN_N], s[ZEN_N], e[ZEN_N];
    ALIGN32 int16_t t0[ZEN_N], t1[ZEN_N];
    __m256i vdata0, vdata1, vdata2, vdata3, vdata4;

    poly_publickey_unpack(h, pk);
    bitreversed_order_2_shuffled_order(h);

    nonce = 0;
    poly_generate_se(s, seed, nonce++);
    poly_generate_se(e, seed, nonce++);

    poly_byte2bit_unpack(t0, m, ZEN_N4);

    vdata0 = _mm256_set1_epi16(385);
    for(i = 0; i < ZEN_N4; i += 16)
    {
        vdata1 = _mm256_load_si256((__m256i *)(t0 + i));
        vdata1 = _mm256_mullo_epi16(vdata1, vdata0);
        _mm256_store_si256((__m256i *)(t1 + i), vdata1);
        _mm256_store_si256((__m256i *)(t1 + i + ZEN_N4), vdata1);
        _mm256_store_si256((__m256i *)(t1 + i + 2*ZEN_N4), vdata1);
        _mm256_store_si256((__m256i *)(t1 + i + 3*ZEN_N4), vdata1);
    }

    poly_ntt(s, nttdata);
    poly_basemul_ntt(t0, h, s, muldata);
    poly_intt(t0, inttdata);

    vdata0 = _mm256_set1_epi16(769);
    vdata1 = _mm256_set1_epi16(-767);
    vdata2 = _mm256_set1_epi16(171);
    for(i = 0; i < ZEN_N; i += 16)
    {
        vdata3 = _mm256_load_si256((__m256i *)(t0 + i));
        vdata4 = _mm256_load_si256((__m256i *)(e + i));
        vdata3 = _mm256_add_epi16(vdata3, vdata4);
        vdata4 = _mm256_load_si256((__m256i *)(t1 + i));
        vdata3 = _mm256_add_epi16(vdata3, vdata4);
        montmul(&vdata4, &vdata3, &vdata2, &vdata0, &vdata1);
        vdata4 = _mm256_add_epi16(vdata4, _mm256_and_si256(_mm256_srai_epi16(vdata4, 15), vdata0));
        _mm256_store_si256((__m256i *)(t1 + i), vdata4);
    }

    poly_compress(t1);
    poly_ciphertext_pack(ct, t1);

}

void pke_dec(unsigned char *sk, unsigned char *ct, unsigned char *m)
{
    unsigned int i, idx, mask;
    int16_t c0, c1, c2, c3;
    ALIGN32 int16_t f[ZEN_N], f2[ZEN_N2] = {0}, mp0[ZEN_N2], mp1[ZEN_N2];
    ALIGN32 int16_t t0[ZEN_N], t1[ZEN_N], t2[ZEN_N2];
    __m256i vdata0, vdata1, vdata2, vdata3, vdata4;

    poly_secretkey_unpack(f, sk); 
    bitreversed_order_2_shuffled_order(f);
    poly_byte2bit_unpack(f2, sk+ZEN_F_NTT_PACK, ZEN_N4);
    poly_ciphertext_unpack(t0, ct);
    poly_decompress(t0);

    // cf mod <q, x^n+1>
    poly_ntt(t0, nttdata);
    poly_basemul_ntt(t1, t0, f, muldata);
    poly_intt(t1, inttdata);

    // (x^(n/2)+1)cf mod <q, x^n+1>
    vdata0 = _mm256_set1_epi16(769);
    vdata1 = _mm256_set1_epi16(-767);
    vdata2 = _mm256_set1_epi16(171);
    for(i = 0; i < ZEN_N2; i += 16)
    {
        vdata3 = _mm256_load_si256((__m256i *)(t1 + i + ZEN_N2));
        vdata4 = _mm256_load_si256((__m256i *)(t1 + i));
        vdata3 = _mm256_sub_epi16(vdata3, vdata4);
        montmul(&vdata4, &vdata3, &vdata2, &vdata0, &vdata1);
        _mm256_store_si256((__m256i *)(t0 + i), vdata4);
    }
    for(i = ZEN_N2; i < ZEN_N; i += 16)
    {
        vdata3 = _mm256_load_si256((__m256i *)(t1 + i - ZEN_N2));
        vdata4 = _mm256_load_si256((__m256i *)(t1 + i));
        vdata3 = _mm256_add_epi16(vdata3, vdata4);
        montmul(&vdata4, &vdata3, &vdata2, &vdata0, &vdata1);
        _mm256_store_si256((__m256i *)(t0 + i), vdata4);
    }

    // a mod <2, x^(n/2)+1>
    vdata0 = _mm256_set1_epi16(1);
    for(i = 0; i < ZEN_N2; i += 16)
    {
        vdata1 = _mm256_load_si256((__m256i *)(t0 + i));
        vdata1 = _mm256_and_si256(vdata1, vdata0);
        vdata2 = _mm256_load_si256((__m256i *)(t0 + i + ZEN_N2));
        vdata2 = _mm256_and_si256(vdata2, vdata0);
        vdata1 = _mm256_xor_si256(vdata1, vdata2);
        _mm256_store_si256((__m256i *)(t1 + i), vdata1);
    }

    // m_prim = af2 mod <2, x^(n/2)+1>
    memset(f2 + ZEN_N4, 0, ZEN_N4 * sizeof(int16_t));
    // mul_in_R2_n(t1, f2, ZEN_N2, mp0);
    mul_in_R2_1024(t1, f2, mp0);

    //SimpleDecoding
    vdata0 = _mm256_set1_epi16(1);
    for(i = 0; i < ZEN_N4; i += 16)
    {
        vdata1 = _mm256_load_si256((__m256i *)(t0 + i));
        vdata1 = _mm256_and_si256(vdata1, vdata0);
        vdata2 = _mm256_load_si256((__m256i *)(t0 + i + ZEN_N4));
        vdata2 = _mm256_and_si256(vdata2, vdata0);
        vdata1 = _mm256_xor_si256(vdata1, vdata2);
        vdata2 = _mm256_load_si256((__m256i *)(t0 + i + 2*ZEN_N4));
        vdata2 = _mm256_and_si256(vdata2, vdata0);
        vdata1 = _mm256_xor_si256(vdata1, vdata2);
        vdata2 = _mm256_load_si256((__m256i *)(t0 + i + 3*ZEN_N4));
        vdata2 = _mm256_and_si256(vdata2, vdata0);
        vdata1 = _mm256_xor_si256(vdata1, vdata2);
        _mm256_store_si256((__m256i *)(t1 + i), vdata1);
    }
    
    memset(t2, 0, ZEN_N2 * sizeof(int16_t));
    for(i = 0; i < ZEN_N4; i++)
    {
        c0 = t0[i];
        c1 = t0[i + ZEN_N4];
        c2 = t0[i + 2*ZEN_N4];
        c3 = t0[i + 3*ZEN_N4];

        mask = (c0 >= 0);
        c0 = ((ZEN_Q2 - c0) & (-mask)) | ((ZEN_Q2 + c0) & (~(-mask)));
        mask = (c1 >= 0);
        c1 = ((ZEN_Q2 - c1) & (-mask)) | ((ZEN_Q2 + c1) & (~(-mask)));
        mask = (c2 >= 0);
        c2 = ((ZEN_Q2 - c2) & (-mask)) | ((ZEN_Q2 + c2) & (~(-mask)));
        mask = (c3 >= 0);
        c3 = ((ZEN_Q2 - c3) & (-mask)) | ((ZEN_Q2 + c3) & (~(-mask)));

        mask = (c0 <= c2);
        c0 = (c0 & (-mask)) | (c2 & (~(-mask)));
        mask = (c1 <= c3);
        c1 = (c1 & (-mask)) | (c3 & (~(-mask)));
        mask = (c0 <= c1);
        idx = (i & (-mask)) | ((i + ZEN_N4) & (~(-mask)));
        t2[idx] = t1[i] & 1;
    }
    mul_in_R2_1024(t2, f2, t1);

    for(i = 0; i < ZEN_N4; i += 16)
    {
        vdata0 = _mm256_load_si256((__m256i *)(mp0 + i));
        vdata1 = _mm256_load_si256((__m256i *)(t1 + i));
        vdata1 = _mm256_xor_si256(vdata1, vdata0);
        _mm256_store_si256((__m256i *)(mp1 + i), vdata1);
    }

    poly_bit2byte_pack(m, mp1, ZEN_N4);
    
}
