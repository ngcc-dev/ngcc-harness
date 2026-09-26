#ifndef SYMMETRIC_H
#define SYMMETRIC_H

#include "params.h"
#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint8_t  derived_seed[64];
    uint32_t block_counter;
} sm3_xof_state;

#define SM3_XOF_BLOCKBYTES 128

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

#endif