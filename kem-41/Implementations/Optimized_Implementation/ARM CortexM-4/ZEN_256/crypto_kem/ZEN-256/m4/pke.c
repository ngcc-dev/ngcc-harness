#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "params.h"
#include "sample.h"
#include "ntt.h"
#include "poly.h"
#ifdef USE_KECCAK
#include "randombytes.h"
#else
#include "drng.h"
#endif
#include "radix16_r2.h"

#ifndef USE_KECCAK
extern DRNG_ctx drng_algorithm;
#endif

void pke_keygen_derand(unsigned char *pk, unsigned char *sk, const unsigned char *seed)
{
    unsigned i, j;
    uint8_t nonce;
    int16_t f[ZEN_N], g[ZEN_N];
    int16_t t0[ZEN_N], t1[ZEN_N];
    uint32_t t0_rad[R2_RADIX16_WORDS(ZEN_N4)];
    uint32_t f2_rad[R2_RADIX16_WORDS(ZEN_N4)];

    nonce = 0;
    //generate f
    poly_generate_f(f, seed, nonce++);
    //chechk f
    for(;;)
    {
        if (poly_xor4_radix16(t0_rad, f)) // t0_rad = f mod (x^(n/4)+1)
        {
            poly_generate_f(f, seed, nonce++);
            continue;
        }
        poly_ntt_mq(f);
        if(check_poly_inv_Zq(f))
        {
            poly_generate_f(f, seed, nonce++);
            continue;
        }

        poly_baseinv_ntt(t1, f); //t1 = f^(-1) mod (x^n+1, q)
        FastInversion(f2_rad, t0_rad); //f2_rad = t0_rad^(-1) in R2

        break;
    }

    //generate g
    poly_generate_g(g, seed, nonce++);
    //check g
    for(;;)
    {
        poly_ntt_mq(g);
        if(check_poly_inv_Zq(g))
        {
            poly_generate_g(g, seed, nonce++);
            continue;
        }
        break;
    }

    poly_basemul_ntt_mq(t0, g, t1);
    poly_publickey_pack(pk, t0);
    poly_secretkey_pack(sk, f);
    r2_radix16_tobytes(sk+ZEN_F_NTT_PACK, f2_rad, ZEN_N4);

}

void pke_keygen(unsigned char *pk, unsigned char *sk)
{
    uint8_t seed[SEED_LEN_BYTES], nonce;

#ifdef USE_KECCAK
    randombytes(seed, SEED_LEN_BYTES);
#else
    get_random_number(&drng_algorithm, seed, SEED_LEN_BYTES*8);
#endif
    pke_keygen_derand(pk, sk, seed);
}

void pke_enc(unsigned char *pk, unsigned char *m, unsigned char *seed, unsigned char *ct)
{
    unsigned i, j;
    uint8_t nonce;
    int16_t h[ZEN_N], s[ZEN_N], e[ZEN_N];
    int16_t t0[ZEN_N], t1[ZEN_N];

    poly_publickey_unpack(h, pk);

    nonce = 0;
    poly_generate_se(s, seed, nonce++);
    poly_generate_se(e, seed, nonce++);

    poly_unpack_f2(t1, m);
    poly_mul385(t1);

    poly_ntt(s);
    poly_basemul_ntt(t0, h, s);
    poly_intt(t0);

    poly_add(t1, t1, t0);
    poly_add(t1, t1, e);
    mq_poly_reduce_mq(t1, ZEN_N);

    poly_compress(t1);
    poly_ciphertext_pack(ct, t1);

}

void pke_dec(unsigned char *sk, unsigned char *ct, unsigned char *m)
{
    unsigned int i, mask1;
    uint16_t delta, even_wins;
    int16_t c0, c1, c2, c3;
    int16_t f[ZEN_N];
    int16_t t0[ZEN_N], t1[ZEN_N];
    uint32_t f2_rad[R2_RADIX16_WORDS(ZEN_N4)];
    uint32_t t1_rad[R2_RADIX16_WORDS(ZEN_N2)];
    uint32_t mp0_rad[R2_RADIX16_WORDS(ZEN_N2)], mp1_rad[R2_RADIX16_WORDS(ZEN_N2)];

    poly_secretkey_unpack(f, sk);
    r2_radix16_frombytes(f2_rad, sk + ZEN_F_NTT_PACK, ZEN_N4);
    poly_ciphertext_unpack(t0, ct);
    poly_decompress(t0);

    // cf mod <q, x^n+1>
    poly_ntt(t0);
    poly_basemul_ntt(t1, t0, f);
    poly_intt(t1);

    // (x^(n/2)+1)cf mod <q, x^n+1>, with parity for mod <2, x^(n/2)+1>
    poly_cp(t0, t1);

    // m_prim = af2 mod <2, x^(n/2)+1>
    r2_radix16_pack(t1_rad, t1, ZEN_N2);
    r2_radix16_mul_512x256(mp0_rad, t1_rad, f2_rad);

    // SimpleDecoding: build S=\delta*x^{j*\times n/4}, then materialize S*f_inv in R_{2L,2}.
    memset(t1, 0, ZEN_N2 * sizeof(int16_t));
    for (i = 0; i < ZEN_N4; i++)
    {
        c0 = t0[i];
        c1 = t0[i + ZEN_N4];
        c2 = t0[i + 2 * ZEN_N4];
        c3 = t0[i + 3 * ZEN_N4];
        delta = (uint16_t)((c0 & 1) ^ (c1 & 1) ^ (c2 & 1) ^ (c3 & 1));

        mask1 = (c0 >= 0);
        c0 = ((ZEN_Q2 - c0) & (-mask1)) | ((ZEN_Q2 + c0) & (~(-mask1)));
        mask1 = (c1 >= 0);
        c1 = ((ZEN_Q2 - c1) & (-mask1)) | ((ZEN_Q2 + c1) & (~(-mask1)));
        mask1 = (c2 >= 0);
        c2 = ((ZEN_Q2 - c2) & (-mask1)) | ((ZEN_Q2 + c2) & (~(-mask1)));
        mask1 = (c3 >= 0);
        c3 = ((ZEN_Q2 - c3) & (-mask1)) | ((ZEN_Q2 + c3) & (~(-mask1)));

        mask1 = (c0 <= c2);
        c0 = (c0 & (-mask1)) | (c2 & (~(-mask1)));
        mask1 = (c1 <= c3);
        c1 = (c1 & (-mask1)) | (c3 & (~(-mask1)));

        even_wins = (uint16_t)(c0 <= c1);
        t1[0] ^= (int16_t)(delta ^ 1u);
        t1[i] ^= (int16_t)(delta & even_wins);
        t1[i + ZEN_N4] ^= (int16_t)(delta & (even_wins ^ 1u));
    }

    r2_radix16_pack(t1_rad, t1, ZEN_N2);
    r2_radix16_mul_512x256(mp1_rad, t1_rad, f2_rad);

    for (i = 0; i < R2_RADIX16_WORDS(ZEN_N4); i++)
    {
        mp1_rad[i] = mp0_rad[i] ^ mp1_rad[i];
    }
    r2_radix16_tobytes(m, mp1_rad, ZEN_N4);
}
