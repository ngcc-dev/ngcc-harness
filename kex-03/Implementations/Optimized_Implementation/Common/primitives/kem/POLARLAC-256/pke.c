/*
Copyright (c) 2026 Ying Liu.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences
File Description: Implements the MLWE-style POLARLAC public-key encryption layer for the optimized POLARLAC-256 instance.
*/

#include <stdint.h>
#include <string.h>

#include "params.h"
#include "sample.h"
#include "ntt.h"
#include "fft.h"
#include "symmetric.h"
#include "poly.h"
#include "pke.h"

#define POLY_NTT_BYTES (RL_KEM_N * sizeof(int16_t))

static int spectral_bound(const int16_t *poly)
{
    return fft_within_bound_int16(poly, (int32_t)RL_KEM_T);
}

static void sample_screened_poly(polarlac_poly *poly, const uint8_t *seed, uint8_t *nonce)
{
    for (;;) {
        poly_generate_tenary(poly->coeffs, seed, *nonce);
        *nonce = (uint8_t)(*nonce + 1U);
        if (spectral_bound(poly->coeffs)) {
            return;
        }
    }
}

static void sample_screened_polyvec(polarlac_polyvec *vec, const uint8_t *seed, uint8_t *nonce)
{
    for (int k = 0; k < RL_KEM_K; k++) {
        sample_screened_poly(&vec->vec[k], seed, nonce);
    }
}

static inline void poly_zero(polarlac_poly *poly)
{
    memset(poly->coeffs, 0, sizeof(poly->coeffs));
}

static inline int16_t mod_q_upto_2q(int32_t a)
{
    int16_t t = (int16_t)(a - RL_KEM_Q);

    t += (int16_t)((t >> 15) & RL_KEM_Q);
    return t;
}

static inline void poly_ntt_from(polarlac_poly *dst, const polarlac_poly *src)
{
    *dst = *src;
    mq_poly_ntt(dst->coeffs);
}

static inline void polyvec_ntt_from(polarlac_polyvec *dst, const polarlac_polyvec *src)
{
    for (int k = 0; k < RL_KEM_K; k++) {
        poly_ntt_from(&dst->vec[k], &src->vec[k]);
    }
}

static inline void poly_muladd2_ntt_products(polarlac_poly *out,
                                             const polarlac_poly *a0_ntt, const polarlac_poly *b0_ntt,
                                             const polarlac_poly *a1_ntt, const polarlac_poly *b1_ntt)
{
    mq_poly_pointwise_muladd2_aligned(out->coeffs,
                                      a0_ntt->coeffs, b0_ntt->coeffs,
                                      a1_ntt->coeffs, b1_ntt->coeffs);
}

static inline void poly_intt_to_coeff(polarlac_poly *poly)
{
    mq_poly_intt(poly->coeffs);
}

static inline void serialize_polyvec_ntt(uint8_t *dst, const polarlac_polyvec *vec)
{
    for (int k = 0; k < RL_KEM_K; k++) {
        memcpy(dst + k * POLY_NTT_BYTES, vec->vec[k].coeffs, POLY_NTT_BYTES);
    }
}

static inline void deserialize_polyvec_ntt(polarlac_polyvec *vec, const uint8_t *src)
{
    for (int k = 0; k < RL_KEM_K; k++) {
        memcpy(vec->vec[k].coeffs, src + k * POLY_NTT_BYTES, POLY_NTT_BYTES);
    }
}

int PKE_KeyGen(uint8_t *pk, uint8_t *sk, const uint8_t *seed)
{
    uint8_t expanded[PKE_EXPANDED_SEED_BYTES];
    uint8_t seed_se[KEM_SEED_LEN_BYTES];
    polarlac_polymat a_ntt;
    polarlac_polyvec s;
    polarlac_polyvec e;
    polarlac_polyvec s_ntt;
    polarlac_polyvec e_ntt;
    polarlac_polyvec b_ntt;
    uint8_t nonce = 0;

    if (pk == NULL || sk == NULL || seed == NULL) {
        return -1;
    }

    bit_xof_bytes(expanded, PKE_EXPANDED_SEED_BYTES, seed, KEM_SEED_LEN_BYTES);
    memcpy(pk, expanded, PK_SEED_LEN_BYTES);
    memcpy(seed_se, expanded + PK_SEED_LEN_BYTES, KEM_SEED_LEN_BYTES);

    poly_generate_uniformQ(&a_ntt, pk, 0);

    for (;;) {
        sample_screened_polyvec(&s, seed_se, &nonce);
        sample_screened_polyvec(&e, seed_se, &nonce);
        polyvec_ntt_from(&s_ntt, &s);
        polyvec_ntt_from(&e_ntt, &e);

        for (int i = 0; i < RL_KEM_K; i++) {
            poly_muladd2_ntt_products(&b_ntt.vec[i],
                                      &a_ntt.row[i].vec[0], &s_ntt.vec[0],
                                      &a_ntt.row[i].vec[1], &s_ntt.vec[1]);
            poly_add_inplace_reduce_q_avx2(b_ntt.vec[i].coeffs, e_ntt.vec[i].coeffs);
        }

        if (polyvec_compress(pk + PK_SEED_LEN_BYTES, &b_ntt) == 0) {
            serialize_polyvec_ntt(sk, &s_ntt);
            break;
        }
    }

    return 0;
}

void PKE_Encrypt(uint8_t *c, const uint8_t *pk, const uint8_t *m, const uint8_t *seed)
{
    uint8_t com_c2[RL_KEM_Lv];
    uint8_t hatm[RL_KEM_Lv];
    polarlac_polymat a_t_ntt;
    polarlac_polyvec b_ntt;
    polarlac_polyvec r;
    polarlac_polyvec r_ntt;
    polarlac_polyvec e1;
    polarlac_poly e2;
    polarlac_polyvec u;
    polarlac_poly v;
    uint8_t nonce = 0;

    if (c == NULL || pk == NULL || m == NULL || seed == NULL) {
        return;
    }

    polyvec_decompress(&b_ntt, pk + PK_SEED_LEN_BYTES);
    poly_generate_uniformQ(&a_t_ntt, pk, 1);
    Encode_m(hatm, (uint8_t *)m);

    sample_screened_polyvec(&r, seed, &nonce);
    sample_screened_polyvec(&e1, seed, &nonce);
    poly_generate_tenary(e2.coeffs, seed, nonce);
    polyvec_ntt_from(&r_ntt, &r);

    for (int i = 0; i < RL_KEM_K; i++) {
        poly_muladd2_ntt_products(&u.vec[i],
                                  &a_t_ntt.row[i].vec[0], &r_ntt.vec[0],
                                  &a_t_ntt.row[i].vec[1], &r_ntt.vec[1]);
        poly_intt_to_coeff(&u.vec[i]);
        poly_add_inplace_reduce_q_avx2(u.vec[i].coeffs, e1.vec[i].coeffs);
    }

    poly_muladd2_ntt_products(&v,
                              &b_ntt.vec[0], &r_ntt.vec[0],
                              &b_ntt.vec[1], &r_ntt.vec[1]);
    poly_intt_to_coeff(&v);
    poly_add_inplace_reduce_q_avx2(v.coeffs, e2.coeffs);
    poly_add_msg_inplace_reduce_q_avx2(v.coeffs, hatm);

    polyvec_byte_compress(c, &u);
    poly_compress_c2(com_c2, v.coeffs, D_C2_BITS);
    pack_c2_dbit(c + C1_LEN_BYTES, com_c2);
}

void PKE_Decrypt(uint8_t *m, const uint8_t *c, const uint8_t *sk)
{
    polarlac_polyvec u;
    polarlac_polyvec s_ntt;
    polarlac_poly u0_ntt;
    polarlac_poly u1_ntt;
    polarlac_poly su;
    polarlac_poly v;
    int16_t hatm[RL_KEM_Lv];
    uint8_t com_c2[RL_KEM_Lv];

    if (m == NULL || c == NULL || sk == NULL) {
        return;
    }

    polyvec_byte_decompress(&u, c);
    unpack_c2_dbit(com_c2, c + C1_LEN_BYTES);
    poly_decompress_c2(v.coeffs, com_c2, D_C2_BITS);
    deserialize_polyvec_ntt(&s_ntt, sk);

    poly_ntt_from(&u0_ntt, &u.vec[0]);
    poly_ntt_from(&u1_ntt, &u.vec[1]);
    poly_muladd2_ntt_products(&su,
                              &u0_ntt, &s_ntt.vec[0],
                              &u1_ntt, &s_ntt.vec[1]);
    poly_intt_to_coeff(&su);

    poly_normalize_q_avx2(su.coeffs);
    poly_sub_reduce_q_avx2(hatm, v.coeffs, su.coeffs);

    Decode_m(m, hatm);
}
