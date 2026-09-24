#ifndef OUR_SIGN_H
#define OUR_SIGN_H

#include "params.h"
#include "poly.h"
#include "polymat.h"
#include "polyvec.h"
#include <stddef.h>
#include <stdint.h>

#define crypto_sign_keypair DARTS_NAMESPACE(keypair)
int crypto_sign_keypair(uint8_t *pk, uint8_t *sk);

#define crypto_sign_signature DARTS_NAMESPACE(signature)
int crypto_sign_signature(uint8_t *sig, size_t *siglen, const uint8_t *m,
                          size_t mlen, const uint8_t *sk);

#define crypto_sign_sign DARTS_NAMESPACE(sign)
int crypto_sign_sign(uint8_t *sm, size_t *smlen, const uint8_t *m, size_t mlen,
                     const uint8_t *sk);

#define crypto_sign_verify DARTS_NAMESPACE(verify)
int crypto_sign_verify(const uint8_t *sig, size_t siglen, const uint8_t *m,
                       size_t mlen, const uint8_t *pk);

#define crypto_sign_open DARTS_NAMESPACE(open)
int crypto_sign_open(uint8_t *m, size_t *mlen, const uint8_t *sm, size_t smlen,
                     const uint8_t *pk);

#endif