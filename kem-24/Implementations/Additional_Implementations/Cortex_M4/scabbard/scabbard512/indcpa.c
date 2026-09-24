#include <stddef.h>
#include <stdint.h>

#include "indcpa.h"
#include "auxfunc.h"
#include "randombytes.h"
#include "polyvec.h"
#include "poly.h"
#include "packing.h"

void indcpa_keypair(
    uint8_t pk[SCABBARD_INDCPA_PUBLICKEYBYTES], 
    uint8_t sk[SCABBARD_INDCPA_SECRETKEYBYTES])
{
    uint8_t *publicseed = pk + SCABBARD_POLYVECCOMPRESSEDBYTES;

    randombytes(publicseed, SCABBARD_SYMBYTES);
    pseudoXOF(SCABBARD_SYMBYTES * 8, publicseed, SCABBARD_SYMBYTES * 8, publicseed);

    randombytes(sk, SCABBARD_SYMBYTES);
    gen_s_packed(sk, sk);

    matrix_vector_mul_tobytes(pk, publicseed, sk, 1, h1);
}

void indcpa_enc(
    uint8_t ct[SCABBARD_INDCPA_BYTES], 
    const uint8_t msg[SCABBARD_INDCPA_MSGBYTES], 
    const uint8_t pk[SCABBARD_INDCPA_PUBLICKEYBYTES], 
    const uint8_t r[SCABBARD_SYMBYTES])
{
    size_t i;
    uint8_t skpv[SCABBARD_INDCPA_SECRETKEYBYTES];
    poly v, m;

    poly_frommsg(&m, msg);
    gen_s_packed(skpv, r);

    matrix_vector_mul_tobytes(ct, pk + SCABBARD_POLYVECCOMPRESSEDBYTES, skpv, 0, h1);

    inner_prod_packed(&v, pk, skpv);
    for (i = 0; i < SCABBARD_N; i++) {
        v.coeffs[i] = (v.coeffs[i]) & SCABBARD_P;
        v.coeffs[i] <<= (SCABBARD_EQ - SCABBARD_EP);
        v.coeffs[i] = ((v.coeffs[i] - m.coeffs[i]) & SCABBARD_Q) >>
                      (SCABBARD_EQ - (SCABBARD_ET + SCABBARD_B));
    }

    poly_m_tobytes(ct + SCABBARD_POLYVECCOMPRESSEDBYTES, &v);
}

uint8_t indcpa_enc_cmp(
    const uint8_t ct[SCABBARD_INDCPA_BYTES],
    const uint8_t msg[SCABBARD_INDCPA_MSGBYTES],
    const uint8_t pk[SCABBARD_INDCPA_PUBLICKEYBYTES],
    const uint8_t r[SCABBARD_SYMBYTES])
{
    size_t i;
    uint8_t fail = 0;
    uint16_t f;
    uint8_t skpv[SCABBARD_INDCPA_SECRETKEYBYTES];
    poly v, m;

    poly_frommsg(&m, msg);
    gen_s_packed(skpv, r);

    fail |= matrix_vector_mul_tobytes_cmp(
        ct,
        pk + SCABBARD_POLYVECCOMPRESSEDBYTES,
        skpv,
        0,
        h1);

    inner_prod_packed(&v, pk, skpv);
    for (i = 0; i < SCABBARD_N; i++) {
        v.coeffs[i] = (v.coeffs[i]) & SCABBARD_P;
        v.coeffs[i] <<= (SCABBARD_EQ - SCABBARD_EP);
        v.coeffs[i] = ((v.coeffs[i] - m.coeffs[i]) & SCABBARD_Q) >>
                      (SCABBARD_EQ - (SCABBARD_ET + SCABBARD_B));
    }

    fail |= poly_m_tobytes_cmp(ct + SCABBARD_POLYVECCOMPRESSEDBYTES, &v);

    f = fail;
    return (uint8_t)((f | (uint16_t)(0U - f)) >> 15);
}

void indcpa_dec(
    uint8_t msg[SCABBARD_INDCPA_MSGBYTES], 
    const uint8_t ct[SCABBARD_INDCPA_BYTES], 
    const uint8_t sk[SCABBARD_INDCPA_SECRETKEYBYTES])
{
    size_t i;
    poly v, v_prime;

    poly_m_frombytes(&v, ct + SCABBARD_POLYVECCOMPRESSEDBYTES);
    inner_prod_packed(&v_prime, ct, sk);

    for (i = 0; i < SCABBARD_N; i++) {
        v.coeffs[i] = (v.coeffs[i] & SCABBARD_Q) <<
                      (SCABBARD_EQ - (SCABBARD_ET + SCABBARD_B));
        v_prime.coeffs[i] = ((v_prime.coeffs[i]) & SCABBARD_P) <<
                            (SCABBARD_EQ - SCABBARD_EP);
        v_prime.coeffs[i] = (v_prime.coeffs[i] - v.coeffs[i]) & SCABBARD_Q;
    }

    poly_tomsg(msg, &v_prime);
}
