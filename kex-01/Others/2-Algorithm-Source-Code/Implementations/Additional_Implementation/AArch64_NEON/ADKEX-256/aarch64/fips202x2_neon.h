/*
 * AArch64 NEON 2-way parallel Keccak (SHAKE128x2 / SHAKE256x2).
 * Uses uint64x2_t to interleave 2 Keccak-f[1600] states.
 * Only active when DKE_USE_AARCH64 is defined.
 */
#ifndef FIPS202X2_NEON_H
#define FIPS202X2_NEON_H

#include "../parameters.h"

#if defined(DKE_USE_AARCH64)

#include <arm_neon.h>
#include <stddef.h>
#include <stdint.h>
#include "../fips202.h"

typedef struct {
    uint64x2_t s[25];
} keccakx2_state;

void shake128x2_absorb_once(keccakx2_state *state,
                             const uint8_t *in0, const uint8_t *in1,
                             size_t inlen);

void shake128x2_squeezeblocks(uint8_t *out0, uint8_t *out1,
                               size_t nblocks, keccakx2_state *state);

void shake256x2_absorb_once(keccakx2_state *state,
                             const uint8_t *in0, const uint8_t *in1,
                             size_t inlen);

void shake256x2_squeezeblocks(uint8_t *out0, uint8_t *out1,
                               size_t nblocks, keccakx2_state *state);

#endif /* DKE_USE_AARCH64 */
#endif /* FIPS202X2_NEON_H */
