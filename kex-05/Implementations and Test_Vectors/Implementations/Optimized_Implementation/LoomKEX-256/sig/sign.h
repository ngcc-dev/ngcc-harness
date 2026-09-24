#ifndef SHUTTLE_SIGN_H
#define SHUTTLE_SIGN_H

#include <stddef.h>
#include <stdint.h>

#include "params.h"
/* Disable with -DDISABLE_NAMESPACE=1. */
#include "namespace.h"
#ifndef RNDBYTES
#    define RNDBYTES SEEDBYTES
#endif

/* CRYPTO_PUBLICKEYBYTES / CRYPTO_SECRETKEYBYTES / CRYPTO_BYTES come from
 * params.h (included above).  CRYPTO_ALGNAME ("SHUTTLE-128" / "-256" /
 * "-512") comes from config.h (included via params.h). */

/* NIST-style entry points; mangled (via namespace.h) to match the library
 * symbols. */
// int crypto_sign_keypair(uint8_t *pk, uint8_t *sk);

// int crypto_sign_signature(uint8_t *sig, size_t *siglen, const uint8_t *m,
//                           size_t mlen, const uint8_t *sk);

int crypto_sign_keypair_xi(uint8_t *pk, uint8_t *sk,
                           const uint8_t xi[SEEDBYTES]);

int crypto_sign_signature_rnd(uint8_t *sig, size_t *siglen,
                              const uint8_t *m, size_t mlen,
                              const uint8_t *sk,
                              const uint8_t rnd[RNDBYTES]);

int crypto_sign_verify(const uint8_t *sig, size_t siglen, const uint8_t *m,
                       size_t mlen, const uint8_t *pk);

#endif /* SHUTTLE_SIGN_H */
