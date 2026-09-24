#include <stddef.h>
#include <stdint.h>

#include "packing.h"

void pack_pk(
    uint8_t pk[SCABBARD_INDCPA_PUBLICKEYBYTES], 
    const polyvec *pkpv, 
    const uint8_t seed[SCABBARD_SYMBYTES])
{
    size_t i;

    polyvec_modp_tobytes(pk, pkpv);
    pk += SCABBARD_POLYVECCOMPRESSEDBYTES;

    for (i = 0; i < SCABBARD_SYMBYTES; i++) {
        pk[i] = seed[i];
    }
}

void unpack_pk(
    polyvec *pkpv, 
    uint8_t seed[SCABBARD_SYMBYTES], 
    const uint8_t pk[SCABBARD_INDCPA_PUBLICKEYBYTES])
{
    size_t i;

    polyvec_modp_frombytes(pkpv, pk);
    for (i = 0; i < SCABBARD_SYMBYTES; i++) {
        seed[i] = pk[SCABBARD_POLYVECCOMPRESSEDBYTES + i];
    }
}

void pack_sk(
    uint8_t sk[SCABBARD_INDCPA_SECRETKEYBYTES], 
    const polyvec *skpv)
{
    polyvec_s_tobytes(sk, skpv);
}

void unpack_sk(
    polyvec *skpv, 
    const uint8_t sk[SCABBARD_INDCPA_SECRETKEYBYTES])
{
    polyvec_s_frombytes(skpv, sk);
}

void pack_ciphertext(
    uint8_t ct[SCABBARD_INDCPA_BYTES], 
    const polyvec *u, 
    const poly *v)
{
    polyvec_modp_tobytes(ct, u);
    poly_m_tobytes(ct + SCABBARD_POLYVECCOMPRESSEDBYTES, v);
}

void unpack_ciphertext(
    polyvec *u, 
    poly *v, 
    const uint8_t ct[SCABBARD_INDCPA_BYTES])
{
    polyvec_modp_frombytes(u, ct);
    poly_m_frombytes(v, ct + SCABBARD_POLYVECCOMPRESSEDBYTES);
}