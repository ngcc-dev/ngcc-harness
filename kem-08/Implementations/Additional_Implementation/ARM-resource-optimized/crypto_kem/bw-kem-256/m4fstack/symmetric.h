#ifndef SYMMETRIC_H
#define SYMMETRIC_H

#include <stddef.h>
#include <stdint.h>
#include "params.h"

typedef struct {
  uint8_t extseed[KYBER_SYMBYTES + 2];
  size_t squeezed_bytes;
} xof_state;

/* keep 168-byte blocks to match SHAKE128 rate assumptions in gen_matrix */
#define XOF_BLOCKBYTES 168

#define kyber_iccs_hash_h KYBER_NAMESPACE(iccs_hash_h)
void kyber_iccs_hash_h(uint8_t out[32], const uint8_t *in, size_t inlen);

#define kyber_iccs_hash_g KYBER_NAMESPACE(iccs_hash_g)
void kyber_iccs_hash_g(uint8_t out[64], const uint8_t *in, size_t inlen);

#define kyber_iccs_xof_absorb KYBER_NAMESPACE(iccs_xof_absorb)
void kyber_iccs_xof_absorb(xof_state *state,
                           const uint8_t seed[KYBER_SYMBYTES],
                           uint8_t x,
                           uint8_t y);

#define kyber_iccs_xof_squeezeblocks KYBER_NAMESPACE(iccs_xof_squeezeblocks)
void kyber_iccs_xof_squeezeblocks(uint8_t *out, size_t outblocks, xof_state *state);

/* Direct-write squeeze: regenerate the first `outlen` bytes of the (prefix-
 * consistent) SM3 pseudoXOF stream straight into `out`, with no internal
 * scratch. `out` must hold `outlen` bytes (the cumulative prefix). Lets a
 * caller sample in place instead of the squeezeblocks path that needed a VLA
 * copy of the whole prefix. */
#define kyber_iccs_xof_squeeze_full KYBER_NAMESPACE(iccs_xof_squeeze_full)
void kyber_iccs_xof_squeeze_full(uint8_t *out, size_t outlen, xof_state *state);

#define kyber_iccs_prf KYBER_NAMESPACE(iccs_prf)
void kyber_iccs_prf(uint8_t *out,
                    size_t outlen,
                    const uint8_t key[KYBER_SYMBYTES],
                    uint8_t nonce);

#define kyber_iccs_rkprf KYBER_NAMESPACE(iccs_rkprf)
void kyber_iccs_rkprf(uint8_t out[KYBER_SSBYTES],
                      const uint8_t key[KYBER_SYMBYTES],
                      const uint8_t input[KYBER_CIPHERTEXTBYTES]);

#define hash_h(OUT, IN, INBYTES) kyber_iccs_hash_h(OUT, IN, INBYTES)
#define hash_g(OUT, IN, INBYTES) kyber_iccs_hash_g(OUT, IN, INBYTES)
#define xof_absorb(STATE, SEED, X, Y) kyber_iccs_xof_absorb(STATE, SEED, X, Y)
#define xof_squeezeblocks(OUT, OUTBLOCKS, STATE) \
  kyber_iccs_xof_squeezeblocks(OUT, OUTBLOCKS, STATE)
#define xof_squeeze_full(OUT, OUTLEN, STATE) \
  kyber_iccs_xof_squeeze_full(OUT, OUTLEN, STATE)
#define prf(OUT, OUTBYTES, KEY, NONCE) kyber_iccs_prf(OUT, OUTBYTES, KEY, NONCE)
#define rkprf(OUT, KEY, INPUT) kyber_iccs_rkprf(OUT, KEY, INPUT)

#endif /* SYMMETRIC_H */
