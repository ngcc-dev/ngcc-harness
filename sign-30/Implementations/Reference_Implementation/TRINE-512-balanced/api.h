#ifndef API_H
#define API_H

#include "params.h"

#define CRYPTO_SECRETKEYBYTES TRINE_SK_BYTES
#define CRYPTO_PUBLICKEYBYTES TRINE_PK_BYTES
#define CRYPTO_BYTES TRINE_SIG_BYTES

#if PARAMS == 1
#define CRYPTO_ALGNAME "TRINE-Balanced-I"
#elif PARAMS == 2
#define CRYPTO_ALGNAME "TRINE-Balanced-III"
#elif PARAMS == 3
#define CRYPTO_ALGNAME "TRINE-Balanced-V"
#elif PARAMS == 4
#define CRYPTO_ALGNAME "TRINE-ShortSig-I"
#elif PARAMS == 5
#define CRYPTO_ALGNAME "TRINE-ShortSig-III"
#elif PARAMS == 6
#define CRYPTO_ALGNAME "TRINE-ShortSig-V"
#endif

int crypto_sign_keypair(
    unsigned char *pk,
    unsigned char *sk
  );

int crypto_sign(
    unsigned char *sm, unsigned long long *smlen,
    const unsigned char *m, unsigned long long mlen,
    const unsigned char *sk
  );

int crypto_sign_verify(
    const unsigned char *sig,
    unsigned long long siglen,
    const unsigned char *m,
    unsigned long long mlen,
    const unsigned char *pk
  );

int crypto_sign_open(
    unsigned char *m, unsigned long long *mlen,
    const unsigned char *sm, unsigned long long smlen,
    const unsigned char *pk
  );

#endif
