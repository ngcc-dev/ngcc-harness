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