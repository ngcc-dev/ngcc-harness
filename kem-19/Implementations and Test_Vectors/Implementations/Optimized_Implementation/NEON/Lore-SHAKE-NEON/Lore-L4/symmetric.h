#ifndef SYMMETRIC_H
#define SYMMETRIC_H

#include "params.h"
#include "fips202.h"
#include <stdint.h>

typedef keccak_state xof_state;

#define xof_absorb LORE_NAMESPACE(xof_absorb)
void xof_absorb(xof_state *s, const uint8_t seed[LORE_SYMBYTES], uint8_t x, uint8_t y);

#define xof_squeezeblocks LORE_NAMESPACE(xof_squeezeblocks)
void xof_squeezeblocks(uint8_t *out, size_t outblocks, xof_state *s);

#define prf LORE_NAMESPACE(prf)
void prf(uint8_t *out, size_t outlen, const uint8_t key[LORE_SYMBYTES], uint8_t nonce);

#define rkprf(OUT, KEY, INPUT) lore_shake256_rkprf(OUT, KEY, INPUT)

void lore_shake256_rkprf(unsigned char out[LORE_SYMBYTES], const unsigned char key[LORE_SYMBYTES], const unsigned char input[LORE_CIPHERTEXTBYTES]);

#endif /* SYMMETRIC_H */