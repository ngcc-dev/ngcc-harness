// SPDX-License-Identifier: Apache-2.0

#ifndef api_h
#define api_h

#include <sqisign_namespace.h>

#define CRYPTO_SECRETKEYBYTES 1409
#define CRYPTO_PUBLICKEYBYTES 257
#define CRYPTO_BYTES 816

#define CRYPTO_ALGNAME "SQIsign_lvl6"

#if defined(ENABLE_SIGN)
SQISIGN_API
int
crypto_sign_keypair(unsigned char *pk, unsigned char *sk);

SQISIGN_API
int
crypto_sign(unsigned char *sm, unsigned long long *smlen,
            const unsigned char *m, unsigned long long mlen,
            const unsigned char *sk);
#endif

SQISIGN_API
int
crypto_sign_open(unsigned char *m, unsigned long long *mlen,
                 const unsigned char *sm, unsigned long long smlen,
                 const unsigned char *pk);

#endif /* api_h */
