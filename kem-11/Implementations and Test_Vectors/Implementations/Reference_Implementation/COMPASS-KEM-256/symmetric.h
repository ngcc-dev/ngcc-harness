#ifndef SYMMETRIC_H
#define SYMMETRIC_H

#include <stddef.h>
#include <stdint.h>
#include "params.h"
#include "auxfunc.h"

#define XOF_BLOCKBYTES 168
#define XOF_BUFFER_SIZE 8192

typedef struct {
  uint8_t buf[XOF_BUFFER_SIZE];
  size_t pos;
} xof_state;

#define COMPASS_KEM_xof_absorb COMPASS_KEM_NAMESPACE(xof_absorb)
void COMPASS_KEM_xof_absorb(xof_state *state,
                            const uint8_t seed[COMPASS_KEM_SYMBYTES],
                            uint8_t x,
                            uint8_t y);

#define COMPASS_KEM_xof_squeezeblocks COMPASS_KEM_NAMESPACE(xof_squeezeblocks)
void COMPASS_KEM_xof_squeezeblocks(uint8_t *out,
                                   size_t nblocks,
                                   xof_state *state);

#define COMPASS_KEM_prf COMPASS_KEM_NAMESPACE(prf)
void COMPASS_KEM_prf(uint8_t *out,
                     size_t outlen,
                     const uint8_t key[COMPASS_KEM_SYMBYTES],
                     uint8_t nonce);

#define COMPASS_KEM_rkprf COMPASS_KEM_NAMESPACE(rkprf)
void COMPASS_KEM_rkprf(uint8_t out[COMPASS_KEM_SSBYTES],
                       const uint8_t key[COMPASS_KEM_SYMBYTES],
                       const uint8_t input[COMPASS_KEM_CIPHERTEXTBYTES]);

#define hash_h(OUT, IN, INBYTES) \
  sm3hash(256, (const unsigned char *)(IN), (unsigned long long)(INBYTES) * 8, (OUT))

#define hash_g(OUT, IN, INBYTES) \
  pseudohash(512, (const unsigned char *)(IN), (unsigned long long)(INBYTES) * 8, (OUT))

#define xof_absorb(STATE, SEED, X, Y) \
  COMPASS_KEM_xof_absorb(STATE, SEED, X, Y)

#define xof_squeezeblocks(OUT, OUTBLOCKS, STATE) \
  COMPASS_KEM_xof_squeezeblocks(OUT, OUTBLOCKS, STATE)

#define prf(OUT, OUTBYTES, KEY, NONCE) \
  COMPASS_KEM_prf(OUT, OUTBYTES, KEY, NONCE)

#define rkprf(OUT, KEY, INPUT) \
  COMPASS_KEM_rkprf(OUT, KEY, INPUT)

#endif /* SYMMETRIC_H */
