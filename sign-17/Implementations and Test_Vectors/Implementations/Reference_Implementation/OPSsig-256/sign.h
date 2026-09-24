#ifndef OPSSIG_SIGN_H
#define OPSSIG_SIGN_H

#include <stddef.h>
#include <stdint.h>

int crypto_sign_keypair(uint8_t *pk, uint8_t *sk);
int crypto_sign_signature(uint8_t *sig,
                          size_t *siglen,
                          const uint8_t *m,
                          size_t mlen,
                          const uint8_t *ctx,
                          size_t ctxlen,
                          const uint8_t *sk);
int crypto_sign_verify(uint8_t *sig,
                       size_t siglen,
                       const uint8_t *m,
                       size_t mlen,
                       const uint8_t *ctx,
                       size_t ctxlen,
                       const uint8_t *pk);

#endif
