#ifndef KEM_H
#define KEM_H

#include <stdint.h>
#include "params.h"

#define CRYPTO_SECRETKEYBYTES  COMPASS_KEM_SECRETKEYBYTES
#define CRYPTO_PUBLICKEYBYTES  COMPASS_KEM_PUBLICKEYBYTES
#define CRYPTO_CIPHERTEXTBYTES COMPASS_KEM_CIPHERTEXTBYTES
#define CRYPTO_BYTES           COMPASS_KEM_SSBYTES



#if (SECURITY_LEVEL == 128)
#define CRYPTO_ALGNAME "COMPASS-KEM-128"

#elif (SECURITY_LEVEL == 256)

#define CRYPTO_ALGNAME "COMPASS-KEM-256"
#elif (SECURITY_LEVEL == 384)

#define CRYPTO_ALGNAME "COMPASS-KEM-384"
#elif (SECURITY_LEVEL == 512)
#define CRYPTO_ALGNAME "COMPASS-KEM-512"

#else
    #error "SECURITY_LEVEL must be in {128, 256, 384, 512}"
#endif

#define crypto_kem_keypair_derand COMPASS_KEM_NAMESPACE(keypair_derand)
int crypto_kem_keypair_derand(uint8_t *pk, uint8_t *sk, const uint8_t *coins);

#define crypto_kem_keypair COMPASS_KEM_NAMESPACE(keypair)
int crypto_kem_keypair(uint8_t *pk, uint8_t *sk);

#define crypto_kem_enc_derand COMPASS_KEM_NAMESPACE(enc_derand)
int crypto_kem_enc_derand(uint8_t *ct, uint8_t *ss, const uint8_t *pk, const uint8_t *coins);

#define crypto_kem_enc COMPASS_KEM_NAMESPACE(enc)
int crypto_kem_enc(uint8_t *ct, uint8_t *ss, const uint8_t *pk);

#define crypto_kem_dec COMPASS_KEM_NAMESPACE(dec)
int crypto_kem_dec(uint8_t *ss, const uint8_t *ct, const uint8_t *sk);

#endif
