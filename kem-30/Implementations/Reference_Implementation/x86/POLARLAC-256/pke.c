/*
Copyright (c) 2026 Ying Liu.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences
File Description: Implements the MLWE-style POLARLAC public-key encryption layer for the reference POLARLAC-256 instance.
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
#if RL_KEM_USE_CONJ_NTT_REJECTION
    return con_poly_within_bound(poly, (int32_t)CONJ_NTT_T);
#else
    return fft_within_bound_int16(poly, (int32_t)RL_KEM_T);
#endif
}

static void sample_screened_poly(polarlac_poly *poly, const uint8_t *seed, uint8_t *nonce)
{
    polarlac_poly candidate;

    for (;;) {
        poly_generate_tenary(candidate.coeffs, seed, *nonce);
        *nonce = (uint8_t)(*nonce + 1U);
        if (spectral_bound(candidate.coeffs)) {
            *poly = candidate;
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

static inline void poly_accumulate_ntt_product(polarlac_poly *acc, const polarlac_poly *a_ntt, const polarlac_poly *b_ntt)
{
    polarlac_poly prod;

    mq_poly_pointwise_mul(prod.coeffs, (int16_t *)a_ntt->coeffs, (int16_t *)b_ntt->coeffs);
    for (int i = 0; i < RL_KEM_N; i++) {
        acc->coeffs[i] = rl_kem_mod_q((int32_t)acc->coeffs[i] + prod.coeffs[i]);
    }
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

    bit_xof(PKE_EXPANDED_SEED_BYTES * 8ULL, seed, KEM_SEED_LEN_BYTES * 8ULL, expanded);
    memcpy(pk, expanded, PK_SEED_LEN_BYTES);
    memcpy(seed_se, expanded + PK_SEED_LEN_BYTES, KEM_SEED_LEN_BYTES);

    poly_generate_uniformQ(&a_ntt, pk, 0);

    for (;;) {
        sample_screened_polyvec(&s, seed_se, &nonce);
        sample_screened_polyvec(&e, seed_se, &nonce);
        polyvec_ntt_from(&s_ntt, &s);
        polyvec_ntt_from(&e_ntt, &e);

        for (int i = 0; i < RL_KEM_K; i++) {
            poly_zero(&b_ntt.vec[i]);
            for (int j = 0; j < RL_KEM_K; j++) {
                poly_accumulate_ntt_product(&b_ntt.vec[i], &a_ntt.row[i].vec[j], &s_ntt.vec[j]);
            }
            for (int t = 0; t < RL_KEM_N; t++) {
                b_ntt.vec[i].coeffs[t] = rl_kem_mod_q((int32_t)b_ntt.vec[i].coeffs[t] + e_ntt.vec[i].coeffs[t]);
            }
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
        poly_zero(&u.vec[i]);
        for (int j = 0; j < RL_KEM_K; j++) {
            poly_accumulate_ntt_product(&u.vec[i], &a_t_ntt.row[i].vec[j], &r_ntt.vec[j]);
        }
        poly_intt_to_coeff(&u.vec[i]);
        for (int t = 0; t < RL_KEM_N; t++) {
            u.vec[i].coeffs[t] = rl_kem_mod_q((int32_t)u.vec[i].coeffs[t] + e1.vec[i].coeffs[t]);
        }
    }

    poly_zero(&v);
    for (int i = 0; i < RL_KEM_K; i++) {
        poly_accumulate_ntt_product(&v, &b_ntt.vec[i], &r_ntt.vec[i]);
    }
    poly_intt_to_coeff(&v);
    for (int i = 0; i < RL_KEM_N; i++) {
        v.coeffs[i] = rl_kem_mod_q((int32_t)v.coeffs[i] + e2.coeffs[i]);
    }
    for (int i = 0; i < RL_KEM_Lv; i++) {
        // v.coeffs[i] = mod_q_upto_2q((int32_t)v.coeffs[i] + (RATIO * (int16_t)hatm[i]));

        v.coeffs[i] = v.coeffs[i] + (RATIO * (int16_t)hatm[i]) - RL_KEM_Q;
        v.coeffs[i] += (v.coeffs[i] >> 15) & RL_KEM_Q;
    }

    polyvec_byte_compress(c, &u);
    poly_compress_c2(com_c2, v.coeffs, D_C2_BITS);
    pack_c2_dbit(c + C1_LEN_BYTES, com_c2);
}

void PKE_Decrypt(uint8_t *m, const uint8_t *c, const uint8_t *sk)
{
    polarlac_polyvec u;
    polarlac_polyvec s_ntt;
    polarlac_poly u_i_ntt;
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

    poly_zero(&su);
    for (int i = 0; i < RL_KEM_K; i++) {
        poly_ntt_from(&u_i_ntt, &u.vec[i]);
        poly_accumulate_ntt_product(&su, &u_i_ntt, &s_ntt.vec[i]);
    }
    poly_intt_to_coeff(&su);

    for (int i = 0; i < RL_KEM_Lv; i++) {
        su.coeffs[i] += (su.coeffs[i] >> 15) & RL_KEM_Q;
        hatm[i] = v.coeffs[i] - su.coeffs[i];
        hatm[i] += (hatm[i] >> 15) & RL_KEM_Q;
    }

    Decode_m(m, hatm);
}
