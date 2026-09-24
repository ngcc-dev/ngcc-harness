/**
 * @file api.h
 * @brief NIST KEM API used by the MITO-KEM IND-CCA2 scheme
 */

#ifndef MITO_API_H
#define MITO_API_H

#include "parameters.h"

// As a technicality, the public key is appended to the secret key in order to respect the NIST API.
// Without this constraint, CRYPTO_SECRETKEYBYTES would be defined as 32

int crypto_kem_keypair(unsigned char *pk, unsigned char *sk);
int crypto_kem_enc(unsigned char *ct, unsigned char *ss, const unsigned char *pk);
int crypto_kem_dec(unsigned char *ss, const unsigned char *ct, const unsigned char *sk);

#endif  // MITO_API_H
