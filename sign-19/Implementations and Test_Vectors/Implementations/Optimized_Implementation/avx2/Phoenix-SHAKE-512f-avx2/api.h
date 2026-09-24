#ifndef PH_API_H
#define PH_API_H

#include <stddef.h>
#include <stdint.h>

#include "params.h"

#define CRYPTO_ALGNAME "SPHINCS+"

#define CRYPTO_SECRETKEYBYTES SPX_SK_BYTES
#define CRYPTO_PUBLICKEYBYTES SPX_PK_BYTES
#define CRYPTO_BYTES SPX_BYTES
#define CRYPTO_SEEDBYTES 3*SPX_N

unsigned long long crypto_sign_secretkeybytes(void);

unsigned long long crypto_sign_publickeybytes(void);

unsigned long long crypto_sign_bytes(void);

unsigned long long crypto_sign_seedbytes(void);

int crypto_sign_seed_keypair(unsigned char *pk, unsigned char *sk,
                             const unsigned char *seed);

int crypto_sign_keypair(unsigned char *pk, unsigned char *sk);

int crypto_sign_signature(uint8_t *sig, size_t *siglen, size_t *slen,
                          const uint8_t *msg, size_t mlen, const uint8_t *sk);

int crypto_sign_verify(const uint8_t *sig, size_t siglen,
                       const uint8_t *msg, size_t msglen, const uint8_t *pk);

int crypto_sign(unsigned char *sm, unsigned long long *smlen, unsigned long long *siglen,
                unsigned long long *tfslen, const unsigned char *msg, unsigned long long msglen,
                const unsigned char *sk);

int crypto_sign_open(unsigned char *msg, unsigned long long *msglen, unsigned long long *siglen,
                    unsigned long long *tfslen, const unsigned char *sm, unsigned long long smlen,
                     const unsigned char *pk);

#endif