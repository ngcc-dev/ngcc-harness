#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "twopke.h"
#include "primitive_interfaces.h"
#include "cretake_params.h"

#define POLY_NTT_BYTES (RL_KEM_N * sizeof(int16_t))

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

//output [0,Q)
static inline void poly_accumulate_ntt_product(polarlac_poly *acc, const polarlac_poly *a_ntt, const polarlac_poly *b_ntt)
{
    polarlac_poly prod;

    mq_poly_pointwise_mul(prod.coeffs, (int16_t *)a_ntt->coeffs, (int16_t *)b_ntt->coeffs); //[-kQ,pQ)
    for (int i = 0; i < RL_KEM_N; i++) {
        acc->coeffs[i] = rl_kem_mod_q((int32_t)acc->coeffs[i] + prod.coeffs[i]); // [0,Q)
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

void twopke_enc(const unsigned char *pk1, const unsigned char *pk2, const unsigned char *m, const unsigned char *seed1, const unsigned char *seed2, unsigned char *c)
{

    uint8_t hatm[RL_KEM_Lv];
    uint8_t test[PKE_MESSAGE_BYTES];
    memset(test, 0, PKE_MESSAGE_BYTES);
    Encode_m(hatm, (uint8_t *)test);

    unsigned char cbar[PKE_CIPHERTEXT_BYTES];
    unsigned char cprime[PKE_CIPHERTEXT_BYTES];
    unsigned char ct3[C2_LEN_BYTES];
    unsigned char zero[PKE_MESSAGE_BYTES];
    uint8_t com_cbar[RL_KEM_Lv], com_cprime[RL_KEM_Lv];
    int16_t hatc[RL_KEM_Lv];
    uint8_t com_c3[RL_KEM_Lv];
    int16_t v1[RL_KEM_Lv], v2[RL_KEM_Lv];

    memset(zero, 0, sizeof(zero));

    PKE_Encrypt(cbar, pk1, m, seed1);
    PKE_Encrypt(cprime, pk2, zero, seed2);
    
    unpack_c2_dbit(com_cprime, cprime + C1_LEN_BYTES);
    poly_decompress_c2(v1, com_cprime, D_C2_BITS);
    unpack_c2_dbit(com_cbar, cbar + C1_LEN_BYTES);
    poly_decompress_c2(v2, com_cbar, D_C2_BITS);

    for (int i = 0; i < RL_KEM_N; i++) {
        hatc[i] = rl_kem_mod_q((int32_t)v1[i] + v2[i]);
    }

    

    poly_compress_c2(com_c3, hatc, D_C2_BITS);
    pack_c2_dbit(ct3, com_c3);

    memcpy(c, cbar, C1_LEN_BYTES);
    memcpy(c + C1_LEN_BYTES, cprime, C1_LEN_BYTES);
    memcpy(c + 2*C1_LEN_BYTES, ct3, C2_LEN_BYTES);

}

void twopke_dec(const unsigned char *sk1, const unsigned char *sk2, const unsigned char *c, unsigned char *m)
{ 
    const unsigned char *c1 = c;
    const unsigned char *c2 = c + C1_LEN_BYTES;
    const unsigned char *c3 = c + 2*C1_LEN_BYTES;
    polarlac_polyvec ubar, uprime;
    polarlac_polyvec s_ntt1, s_ntt2;
    polarlac_poly subar, suprime;
    polarlac_poly v;
    int16_t hatm[RL_KEM_Lv];
    uint8_t com_c3[RL_KEM_Lv];
    polarlac_poly ubar_i_ntt, uprime_i_ntt;

  if (m == NULL || c1 == NULL || c2 == NULL || sk1 == NULL || sk2 == NULL) {
        return;
    }

    polyvec_byte_decompress(&ubar, c1);
    polyvec_byte_decompress(&uprime, c2);
    deserialize_polyvec_ntt(&s_ntt1, sk1);
    deserialize_polyvec_ntt(&s_ntt2, sk2);

    poly_zero(&subar);
    for (int i = 0; i < RL_KEM_K; i++) {
        poly_ntt_from(&ubar_i_ntt, &ubar.vec[i]);
        poly_accumulate_ntt_product(&subar, &ubar_i_ntt, &s_ntt1.vec[i]);
    }
    poly_intt_to_coeff(&subar);

    poly_zero(&suprime);
    for (int i = 0; i < RL_KEM_K; i++) {
        poly_ntt_from(&uprime_i_ntt, &uprime.vec[i]);
        poly_accumulate_ntt_product(&suprime, &uprime_i_ntt, &s_ntt2.vec[i]);
    }
    poly_intt_to_coeff(&suprime);

    unpack_c2_dbit(com_c3, c3);
    poly_decompress_c2(v.coeffs, com_c3, D_C2_BITS);

    for (int i = 0; i < RL_KEM_Lv; i++) {
        int16_t subar_i = rl_kem_mod_q(subar.coeffs[i]);
        int16_t suprime_i = rl_kem_mod_q(suprime.coeffs[i]);
        hatm[i] = v.coeffs[i] - subar_i - suprime_i;
        hatm[i] += (hatm[i] >> 15) & RL_KEM_Q;
    }

    Decode_m(m, hatm);
}