#ifndef API_H
#define API_H

#include "config.h"
#include <stdint.h>
#include <stddef.h>

#if SIGN_MODE == 128
#define CRYPTO_PUBLICKEYBYTES 1120
#define CRYPTO_SECRETKEYBYTES 1536
#define CRYPTO_SIGNATUREBYTES 1449

#elif SIGN_MODE == 256
#define CRYPTO_PUBLICKEYBYTES 2208 
#define CRYPTO_SECRETKEYBYTES 2880
#define CRYPTO_SIGNATUREBYTES 2489

#elif SIGN_MODE == 512
#define CRYPTO_PUBLICKEYBYTES 4672
#define CRYPTO_SECRETKEYBYTES 6016
#define CRYPTO_SIGNATUREBYTES 5851

#else
#error "SIGN_MODE must be defined as 128, 256, or 512"

#endif

#define crypto_sign_keypair DARTS_NAMESPACE(keypair)
int crypto_sign_keypair(uint8_t *pk, uint8_t *sk);

#define crypto_sign_sign DARTS_NAMESPACE(sign)
int crypto_sign_sign(uint8_t *sm, size_t *smlen, const uint8_t *m, size_t mlen,
                     const uint8_t *sk);

#define crypto_sign_open DARTS_NAMESPACE(open)
int crypto_sign_open(uint8_t *m, size_t *mlen, const uint8_t *sm, size_t smlen,
                     const uint8_t *pk);

#endif