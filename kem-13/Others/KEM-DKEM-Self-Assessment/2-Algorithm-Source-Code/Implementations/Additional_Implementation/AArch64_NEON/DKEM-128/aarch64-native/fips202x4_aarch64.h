/*
 * SHAKE128/256 x4 parallel API for AArch64.
 * Uses mlkem-native Keccak x4 assembly (Apache-2.0/ISC/MIT).
 *
 * Drop-in replacement for fips202x4.h (AVX2 version).
 * Active when DKE_USE_AARCH64_NATIVE is defined.
 */
#ifndef FIPS202X4_AARCH64_H
#define FIPS202X4_AARCH64_H

#include <stddef.h>
#include <stdint.h>

#define FIPS202X4_NAMESPACE(s) dke_fips202x4_aarch64_##s

/* 4-way Keccak state: 4 SEQUENTIAL states (state0[25], state1[25], state2[25], state3[25]) */
typedef struct {
    uint64_t s[100];
} keccakx4_state;

#define shake128x4_absorb_once FIPS202X4_NAMESPACE(shake128x4_absorb_once)
void shake128x4_absorb_once(keccakx4_state *state,
    const uint8_t *in0, const uint8_t *in1,
    const uint8_t *in2, const uint8_t *in3, size_t inlen);

#define shake128x4_squeezeblocks FIPS202X4_NAMESPACE(shake128x4_squeezeblocks)
void shake128x4_squeezeblocks(uint8_t *out0, uint8_t *out1,
    uint8_t *out2, uint8_t *out3,
    size_t nblocks, keccakx4_state *state);

#define shake256x4_absorb_once FIPS202X4_NAMESPACE(shake256x4_absorb_once)
void shake256x4_absorb_once(keccakx4_state *state,
    const uint8_t *in0, const uint8_t *in1,
    const uint8_t *in2, const uint8_t *in3, size_t inlen);

#define shake256x4_squeezeblocks FIPS202X4_NAMESPACE(shake256x4_squeezeblocks)
void shake256x4_squeezeblocks(uint8_t *out0, uint8_t *out1,
    uint8_t *out2, uint8_t *out3,
    size_t nblocks, keccakx4_state *state);

#define shake128x4 FIPS202X4_NAMESPACE(shake128x4)
void shake128x4(uint8_t *out0, uint8_t *out1, uint8_t *out2, uint8_t *out3,
    size_t outlen,
    const uint8_t *in0, const uint8_t *in1,
    const uint8_t *in2, const uint8_t *in3, size_t inlen);

#define shake256x4 FIPS202X4_NAMESPACE(shake256x4)
void shake256x4(uint8_t *out0, uint8_t *out1, uint8_t *out2, uint8_t *out3,
    size_t outlen,
    const uint8_t *in0, const uint8_t *in1,
    const uint8_t *in2, const uint8_t *in3, size_t inlen);

#endif /* FIPS202X4_AARCH64_H */
