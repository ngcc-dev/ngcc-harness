#ifndef SYMMETRIC_H
#define SYMMETRIC_H

#include "params.h"
#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint8_t  derived_seed[64];
    uint32_t block_counter;
} sm3_xof_state;

// Internal structure to store 4 independent SM3 XOF states for 4x operations
typedef struct {
    sm3_xof_state states[4];
} sm3_xof_4x_state;

#define SM3_XOF_BLOCKBYTES 128

// Compatibility type aliases for FIPS 202 (Keccak) replacement
typedef sm3_xof_state stream128_state;
typedef sm3_xof_state stream256_state;
typedef sm3_xof_state xof256_state;
typedef sm3_xof_4x_state keccakx4_state;

// Compatibility constants
#define STREAM128_BLOCKBYTES 128
#define STREAM256_BLOCKBYTES 128
#define XOF256_BLOCKBYTES 128

void sm3_xof_absorb_once(sm3_xof_state *state,
                         const uint8_t *in, size_t inlen);

void sm3_xof_absorb_twice(sm3_xof_state *state,
                          const uint8_t *in1, size_t in1len,
                          const uint8_t *in2, size_t in2len);

void sm3_xof_squeeze(uint8_t *out, size_t outlen,
                     sm3_xof_state *state);

void sm3_xof_squeezeblocks(uint8_t *out, size_t nblocks,
                           sm3_xof_state *state);

void sm3_xof_stream_init(sm3_xof_state *state,
                         const uint8_t *seed, size_t seedlen,
                         uint16_t nonce);

void sm3_shake256(uint8_t *out, size_t outlen,
                  const uint8_t *in, size_t inlen);

// Compatibility function wrappers for stream128 (STREAM128_BLOCKBYTES = 128)
static inline void stream128_init(stream128_state *state,
                                  const uint8_t *seed, uint16_t nonce) {
    sm3_xof_stream_init(state, seed, 32, nonce);
}

static inline void stream128_squeezeblocks(uint8_t *out, size_t nblocks,
                                          stream128_state *state) {
    sm3_xof_squeezeblocks(out, nblocks, state);
}

// Compatibility function wrappers for stream256 (STREAM256_BLOCKBYTES = 128)
static inline void stream256_init(stream256_state *state,
                                  const uint8_t *seed, uint16_t nonce) {
    sm3_xof_stream_init(state, seed, 32, nonce);
}

static inline void stream256_squeezeblocks(uint8_t *out, size_t nblocks,
                                          stream256_state *state) {
    sm3_xof_squeezeblocks(out, nblocks, state);
}

// Compatibility function wrappers for xof256
static inline void xof256_absorbe_twice(xof256_state *state,
                                        const uint8_t *in1, size_t in1len,
                                        const uint8_t *in2, size_t in2len) {
    sm3_xof_absorb_twice(state, in1, in1len, in2, in2len);
}

static inline void xof256_squeezeblocks(uint8_t *out, size_t nblocks,
                                       xof256_state *state) {
    sm3_xof_squeezeblocks(out, nblocks, state);
}

// Compatibility wrappers for shake128x4 and shake256x4 (single instance versions)
void shake128x4_absorb_once(keccakx4_state *state,
                            const uint8_t *in0, const uint8_t *in1,
                            const uint8_t *in2, const uint8_t *in3,
                            size_t inlen);

void shake128x4_squeezeblocks(uint8_t *out0, uint8_t *out1,
                              uint8_t *out2, uint8_t *out3,
                              size_t nblocks, keccakx4_state *state);

void shake256x4_absorb_once(keccakx4_state *state,
                            const uint8_t *in0, const uint8_t *in1,
                            const uint8_t *in2, const uint8_t *in3,
                            size_t inlen);

void shake256x4_squeezeblocks(uint8_t *out0, uint8_t *out1,
                              uint8_t *out2, uint8_t *out3,
                              size_t nblocks, keccakx4_state *state);

void shake256x4_squeezeblocks_vec(uint8_t *out, size_t nblocks,
                                  keccakx4_state *state);

// Compatibility alias for shake256 (used in polyfix.c)
#define shake256(out, outlen, in, inlen) sm3_shake256(out, outlen, in, inlen)

#endif