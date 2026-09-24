#ifndef RHYME_SIGN_H
#define RHYME_SIGN_H
#include <stddef.h>
#include <stdint.h>
#include "params.h"
#include "packing.h"

#define crypto_sign_keypair RHYME_NAMESPACE(keypair)
#define crypto_sign_signature RHYME_NAMESPACE(signature)
#define crypto_sign RHYME_NAMESPACE(sign)
#define crypto_sign_verify RHYME_NAMESPACE(verify)
#define crypto_sign_open RHYME_NAMESPACE(open)
#define crypto_sign_keypair_from_basis RHYME_NAMESPACE(keypair_from_basis)

int crypto_sign_keypair(uint8_t *pk, uint8_t *sk);
/* deterministic key assembly from an externally produced unimodular basis
 * (used by tests and by the keygen module): B is OUR matrix (already transposed). */
int crypto_sign_keypair_from_basis(uint8_t *pk, uint8_t *sk,
                                   const secret_basis *B,
                                   const uint8_t seedA[SEEDBYTES],
                                   const uint8_t key[SEEDBYTES]);
int crypto_sign_signature(uint8_t *sig, size_t *siglen,
                          const uint8_t *m, size_t mlen, const uint8_t *sk);
int crypto_sign(uint8_t *sm, size_t *smlen, const uint8_t *m, size_t mlen,
                const uint8_t *sk);
int crypto_sign_verify(const uint8_t *sig, size_t siglen,
                       const uint8_t *m, size_t mlen, const uint8_t *pk);
int crypto_sign_open(uint8_t *m, size_t *mlen, const uint8_t *sm, size_t smlen,
                     const uint8_t *pk);
#endif
