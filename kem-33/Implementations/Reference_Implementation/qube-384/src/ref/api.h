/**
 * @file api.h
 * @brief NIST-style KEM API for QUBE-384.
 */

#ifndef QUBE_API_H
#define QUBE_API_H

#include "parameters.h"

#define CRYPTO_ALGNAME "QUBE-384-SM3"
#define CRYPTO_PUBLICKEYBYTES  PUBLIC_KEY_BYTES
#define CRYPTO_SECRETKEYBYTES  SECRET_KEY_BYTES
#define CRYPTO_CIPHERTEXTBYTES CIPHERTEXT_BYTES
#define CRYPTO_BYTES           SHARED_SECRET_BYTES

int crypto_kem_keypair(unsigned char *pk, unsigned char *sk);
int crypto_kem_enc(unsigned char *ct, unsigned char *ss, const unsigned char *pk);
int crypto_kem_dec(unsigned char *ss, const unsigned char *ct, const unsigned char *sk);

#endif  // QUBE_API_H
