/*
 * QingLuan Digital Signature Scheme
 * api.h - Public API (compatible with the standard programming interface)
 */

#ifndef QINGLUAN_API_H
#define QINGLUAN_API_H

#include "params.h"
#include <stddef.h>
#include <stdint.h>

#define CRYPTO_PUBLICKEYBYTES   QINGLUAN_PK_BYTES
#define CRYPTO_SECRETKEYBYTES   QINGLUAN_SK_BYTES
#define CRYPTO_BYTES            QINGLUAN_SIG_BYTES
#define CRYPTO_ALGNAME          QINGLUAN_NAME

int crypto_sign_keypair(unsigned char *pk, unsigned char *sk);

int crypto_sign(unsigned char *sm, unsigned long long *smlen,
                const unsigned char *m, unsigned long long mlen,
                const unsigned char *sk);

int crypto_sign_open(unsigned char *m, unsigned long long *mlen,
                     const unsigned char *sm, unsigned long long smlen,
                     const unsigned char *pk);

int crypto_sign_signature(unsigned char *sig, size_t *siglen,
                          const unsigned char *m, size_t mlen,
                          const unsigned char *sk);

int crypto_sign_verify(const unsigned char *sig, size_t siglen,
                       const unsigned char *m, size_t mlen,
                       const unsigned char *pk);

int crypto_sign_selftest(void);

int crypto_sign_selftest_detailed(int *hash_ok, int *drbg_ok,
                                  int *keygen_ok, int *sign_ok,
                                  int *verify_ok);

#endif /* QINGLUAN_API_H */
