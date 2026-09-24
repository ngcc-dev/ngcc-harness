#ifndef SM3_XOF_H
#define SM3_XOF_H

#include <stddef.h>
#include <stdint.h>
#include "params.h"

#define SM3_XOF128_RATE 168
#define SM3_XOF256_RATE 136
#define SM3_XOF128_INPUT_MAX (LORE_SYMBYTES + 2)

typedef struct {
    uint8_t input[SM3_XOF128_INPUT_MAX];
    size_t inlen;
    size_t outpos;
    uint8_t mode;
    uint8_t finalized;
    uint8_t overflow;
} sm3_xof_state;

void sm3_xof128_absorb_once(sm3_xof_state *state, const uint8_t *in, size_t inlen);
void sm3_xof128_squeezeblocks(uint8_t *out, size_t nblocks, sm3_xof_state *state);
void sm3_xof256(uint8_t *out, size_t outlen, const uint8_t *in, size_t inlen);

#endif
