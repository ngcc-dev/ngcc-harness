#ifndef SYMMETRIC_H
#define SYMMETRIC_H

#include <stddef.h>
#include <stdint.h>
#include "params.h"

typedef struct {
  uint8_t extseed[KYBER_SYMBYTES + 2];
  size_t squeezed_bytes;
} xof_state;

#define XOF_BLOCKBYTES 168

#define kyber_iccs_hash_h KYBER_NAMESPACE(iccs_hash_h)
void kyber_iccs_hash_h(uint8_t out[32], const uint8_t *in, size_t inlen);

#define kyber_iccs_hash_g KYBER_NAMESPACE(iccs_hash_g)
void kyber_iccs_hash_g(uint8_t out[32], const uint8_t *in, size_t inlen);

#define kyber_iccs_xof_absorb KYBER_NAMESPACE(iccs_xof_absorb)
void kyber_iccs_xof_absorb(xof_state *state,
                           const uint8_t seed[KYBER_SYMBYTES],
                           uint8_t x,
                           uint8_t y);

#define kyber_iccs_xof_squeezeblocks KYBER_NAMESPACE(iccs_xof_squeezeblocks)
void kyber_iccs_xof_squeezeblocks(uint8_t *out, size_t outblocks, xof_state *state);

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
#define prf(OUT, OUTBYTES, KEY, NONCE) kyber_iccs_prf(OUT, OUTBYTES, KEY, NONCE)
#define rkprf(OUT, KEY, INPUT) kyber_iccs_rkprf(OUT, KEY, INPUT)

#endif /* SYMMETRIC_H */
