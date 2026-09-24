#include "symmetric.h"
#include "auxfunc.h"
#include <string.h>
#include <stdlib.h>

void sm3_xof_absorb_once(sm3_xof_state *state,
                         const uint8_t *in, size_t inlen)
{
    memset(state, 0, sizeof(*state));
    pseudohash(512, in, (unsigned long long)inlen * 8, state->derived_seed);
    state->block_counter = 0;
}

void sm3_xof_absorb_twice(sm3_xof_state *state,
                          const uint8_t *in1, size_t in1len,
                          const uint8_t *in2, size_t in2len)
{
    size_t total = in1len + in2len;
    uint8_t *combined = (uint8_t *)malloc(total);
    if (combined == NULL) {
        memset(state, 0, sizeof(*state));
        return;
    }
    memcpy(combined, in1, in1len);
    memcpy(combined + in1len, in2, in2len);

    memset(state, 0, sizeof(*state));
    pseudohash(512, combined, (unsigned long long)total * 8, state->derived_seed);
    state->block_counter = 0;

    free(combined);
}

void sm3_xof_squeezeblocks(uint8_t *out, size_t nblocks,
                           sm3_xof_state *state)
{
    if (nblocks == 0) return;

    uint8_t tmp[64 + 4];
    memcpy(tmp, state->derived_seed, 64);
    uint32_t start = state->block_counter;
    tmp[64] = (uint8_t)(start >> 24);
    tmp[65] = (uint8_t)(start >> 16);
    tmp[66] = (uint8_t)(start >>  8);
    tmp[67] = (uint8_t)(start);

    pseudoXOF((unsigned long long)nblocks * 1024,
              tmp, 68ULL * 8, out);

    state->block_counter += (uint32_t)nblocks;
}

void sm3_xof_squeeze(uint8_t *out, size_t outlen,
                     sm3_xof_state *state)
{
    if (outlen == 0) return;

    uint8_t tmp[64 + 4];
    memcpy(tmp, state->derived_seed, 64);
    uint32_t start = state->block_counter;
    tmp[64] = (uint8_t)(start >> 24);
    tmp[65] = (uint8_t)(start >> 16);
    tmp[66] = (uint8_t)(start >>  8);
    tmp[67] = (uint8_t)(start);

    pseudoXOF((unsigned long long)outlen * 8,
              tmp, 68ULL * 8, out);

    size_t nblocks = (outlen + 127) / 128;
    state->block_counter += (uint32_t)nblocks;
}

void sm3_xof_stream_init(sm3_xof_state *state,
                         const uint8_t *seed, size_t seedlen,
                         uint16_t nonce)
{
    uint8_t tmp[128];
    memcpy(tmp, seed, seedlen);
    tmp[seedlen]     = (uint8_t)(nonce & 0xFF);
    tmp[seedlen + 1] = (uint8_t)(nonce >> 8);

    memset(state, 0, sizeof(*state));
    pseudohash(512, tmp, (unsigned long long)(seedlen + 2) * 8,
               state->derived_seed);
    state->block_counter = 0;
}

void sm3_shake256(uint8_t *out, size_t outlen,
                  const uint8_t *in, size_t inlen)
{
    sm3_xof_state state;
    sm3_xof_absorb_once(&state, in, inlen);
    sm3_xof_squeeze(out, outlen, &state);
}

// Compatibility wrappers for shake128x4 (4x single instance processing)
void shake128x4_absorb_once(keccakx4_state *state,
                            const uint8_t *in0, const uint8_t *in1,
                            const uint8_t *in2, const uint8_t *in3,
                            size_t inlen)
{
    sm3_xof_absorb_once(&state->states[0], in0, inlen);
    sm3_xof_absorb_once(&state->states[1], in1, inlen);
    sm3_xof_absorb_once(&state->states[2], in2, inlen);
    sm3_xof_absorb_once(&state->states[3], in3, inlen);
}

void shake128x4_squeezeblocks(uint8_t *out0, uint8_t *out1,
                              uint8_t *out2, uint8_t *out3,
                              size_t nblocks, keccakx4_state *state)
{
    sm3_xof_squeezeblocks(out0, nblocks, &state->states[0]);
    sm3_xof_squeezeblocks(out1, nblocks, &state->states[1]);
    sm3_xof_squeezeblocks(out2, nblocks, &state->states[2]);
    sm3_xof_squeezeblocks(out3, nblocks, &state->states[3]);
}

// Compatibility wrappers for shake256x4 (4x single instance processing)
void shake256x4_absorb_once(keccakx4_state *state,
                            const uint8_t *in0, const uint8_t *in1,
                            const uint8_t *in2, const uint8_t *in3,
                            size_t inlen)
{
    sm3_xof_absorb_once(&state->states[0], in0, inlen);
    sm3_xof_absorb_once(&state->states[1], in1, inlen);
    sm3_xof_absorb_once(&state->states[2], in2, inlen);
    sm3_xof_absorb_once(&state->states[3], in3, inlen);
}

void shake256x4_squeezeblocks(uint8_t *out0, uint8_t *out1,
                              uint8_t *out2, uint8_t *out3,
                              size_t nblocks, keccakx4_state *state)
{
    sm3_xof_squeezeblocks(out0, nblocks, &state->states[0]);
    sm3_xof_squeezeblocks(out1, nblocks, &state->states[1]);
    sm3_xof_squeezeblocks(out2, nblocks, &state->states[2]);
    sm3_xof_squeezeblocks(out3, nblocks, &state->states[3]);
}

void shake256x4_squeezeblocks_vec(uint8_t *out, size_t nblocks,
                                  keccakx4_state *state)
{
    size_t blockbytes = 128;
    for (int i = 0; i < 4; i++) {
        sm3_xof_squeezeblocks(out + i * nblocks * blockbytes, nblocks, &state->states[i]);
    }
}