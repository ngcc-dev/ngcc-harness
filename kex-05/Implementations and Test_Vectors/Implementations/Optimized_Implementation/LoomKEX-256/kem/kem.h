#ifndef KEM_H
#define KEM_H

#include <stdint.h>
#include "params.h"

#define crypto_kem_keypair_derand WEAVER_NAMESPACE(_keypair_derand)
int crypto_kem_keypair_derand(uint8_t *pk, uint8_t *sk, const uint8_t *coins);

#define crypto_kem_keypair WEAVER_NAMESPACE(_keypair)
int crypto_kem_keypair(uint8_t *pk, uint8_t *sk);

#define crypto_kem_enc_derand WEAVER_NAMESPACE(_enc_derand)
int crypto_kem_enc_derand(uint8_t *ct, uint8_t *ss, const uint8_t *pk,
                          const uint8_t coins[WEAVER_KEM_DERAND_COINBYTES]);

#define crypto_kem_enc WEAVER_NAMESPACE(_enc)
int crypto_kem_enc(uint8_t *ct, uint8_t *ss, const uint8_t *pk);

// #define crypto_kem_dec WEAVER_NAMESPACE(_dec)
// int crypto_kem_dec(uint8_t *ss, const uint8_t *ct, const uint8_t *sk);

/* Rigid decap for LOOM-AKE; see kem_cpaf.c crypto_kem_dec_rigid. */
#define crypto_kem_dec_rigid WEAVER_NAMESPACE(_dec_rigid)
int crypto_kem_dec_rigid(uint8_t *ss, const uint8_t *ct, const uint8_t *sk);

#endif
