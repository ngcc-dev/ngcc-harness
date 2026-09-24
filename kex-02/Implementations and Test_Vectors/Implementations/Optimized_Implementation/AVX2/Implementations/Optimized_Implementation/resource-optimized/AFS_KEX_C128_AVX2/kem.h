#ifndef KEM_H
#define KEM_H

#include <stdint.h>
#include "params.h"

#define CRYPTO_SECRETKEYBYTES  BWKEM128_SECRETKEYBYTES
#define CRYPTO_PUBLICKEYBYTES  BWKEM128_PUBLICKEYBYTES
#define CRYPTO_CIPHERTEXTBYTES BWKEM128_CIPHERTEXTBYTES
#define CRYPTO_BYTES           BWKEM128_SSBYTES

#define CRYPTO_ALGNAME BWKEM128_NAME

#define crypto_kem_keypair_derand BWKEM128_NAMESPACE(keypair_derand)
int crypto_kem_keypair_derand(uint8_t *pk, uint8_t *sk, const uint8_t *coins);

#define crypto_kem_keypair BWKEM128_NAMESPACE(keypair)
int crypto_kem_keypair(uint8_t *pk, uint8_t *sk);

#define crypto_kem_enc_derand BWKEM128_NAMESPACE(enc_derand)
int crypto_kem_enc_derand(uint8_t *ct, uint8_t *ss, const uint8_t *pk, const uint8_t *coins);

#define crypto_kem_enc BWKEM128_NAMESPACE(enc)
int crypto_kem_enc(uint8_t *ct, uint8_t *ss, const uint8_t *pk);

#define crypto_kem_dec BWKEM128_NAMESPACE(dec)
int crypto_kem_dec(uint8_t *ss, const uint8_t *ct, const uint8_t *sk);

#endif
