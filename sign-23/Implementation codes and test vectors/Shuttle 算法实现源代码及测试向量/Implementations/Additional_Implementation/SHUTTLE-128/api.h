/*
 * api.h - NIST-style API header for the SHUTTLE signature scheme.
 *
 * Exposes the standard SUPERCOP/NIST entry points (crypto_sign_keypair,
 * crypto_sign_signature, crypto_sign_verify, crypto_sign,
 * crypto_sign_open) with names mangled per SHUTTLE_MODE x backend via
 * namespace.h, so that e.g. libshuttle128_ref / libshuttle128_avx2 /
 * libshuttle256_ref ... can all coexist in one linker namespace.
 *
 * A caller compiled against mode 128 ref will have its
 * `crypto_sign_keypair` references rewritten by the preprocessor to
 * `shuttle128_ref_keypair`, which matches the symbol exported from
 * libshuttle128_ref.
 *
 * This is the in-tree NIST-style API used by the test/ drivers.  The NGCC
 * sig_* interface (SIG_AlgorithmInstance.{h,c}) is a separate thin adapter
 * that the top-level orchestration wires onto these entry points.
 */

#ifndef SHUTTLE_API_H
#define SHUTTLE_API_H

#include <stddef.h>
#include <stdint.h>

#include "params.h"

/* CRYPTO_PUBLICKEYBYTES / CRYPTO_SECRETKEYBYTES / CRYPTO_BYTES come from
 * params.h (included above).  CRYPTO_ALGNAME ("SHUTTLE-128" / "-256" /
 * "-512") comes from config.h (included via params.h). */

/* NIST-style entry points; mangled (via namespace.h) to match the library
 * symbols. */
int crypto_sign_keypair(uint8_t *pk, uint8_t *sk);

int crypto_sign_signature(uint8_t *sig, size_t *siglen, const uint8_t *m,
                          size_t mlen, const uint8_t *sk);

int crypto_sign_verify(const uint8_t *sig, size_t siglen, const uint8_t *m,
                       size_t mlen, const uint8_t *pk);

int crypto_sign(uint8_t *sm, size_t *smlen, const uint8_t *m, size_t mlen,
                const uint8_t *sk);

int crypto_sign_open(uint8_t *m, size_t *mlen, const uint8_t *sm,
                     size_t smlen, const uint8_t *pk);

#endif /* SHUTTLE_API_H */
