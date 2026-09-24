/**
 * @file api.h
 * @brief NIST KEM API used by the QUBE-KEM IND-CCA2 scheme
 */

#ifndef QUBE_API_H
#define QUBE_API_H
#include "parameters.h"

#define CRYPTO_ALGNAME "QUBE-1"

#define CRYPTO_PUBLICKEYBYTES  VEC_N_SIZE_BYTES + SEED_BYTES + VEC_N_SIZE_BYTES
#define CRYPTO_SECRETKEYBYTES  CRYPTO_PUBLICKEYBYTES + SEED_BYTES + PARAM_SECURITY_BYTES + SEED_BYTES
#define CRYPTO_BYTES           32
#define CRYPTO_CIPHERTEXTBYTES VEC_N_SIZE_BYTES + VEC_N1N2_SIZE_BYTES + SALT_BYTES

// As a technicality, the public key is appended to the secret key in order to respect the NIST API.
// Without this constraint, CRYPTO_SECRETKEYBYTES would be defined as 32

int crypto_kem_keypair(unsigned char *pk, unsigned char *sk);
int crypto_kem_enc(unsigned char *ct, unsigned char *ss, const unsigned char *pk);
int crypto_kem_dec(unsigned char *ss, const unsigned char *ct, const unsigned char *sk);

#endif  // QUBE_API_H
