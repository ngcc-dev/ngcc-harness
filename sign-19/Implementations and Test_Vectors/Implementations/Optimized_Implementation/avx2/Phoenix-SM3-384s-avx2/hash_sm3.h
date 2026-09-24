#ifndef SPX_SM3_H
#define SPX_SM3_H

#include "params.h"
#include "context.h"

#define SPX_SM3_BLOCK_BYTES 64
#define SPX_SM3_OUTPUT_BYTES 32

#define SPX_SM3_ADDR_BYTES 32

#include <stddef.h>
#include <stdint.h>

void sm3_inc_init(uint8_t *state);
void sm3_inc_blocks(uint8_t *state, const uint8_t *in, size_t inblocks);
void sm3_inc_finalize(uint8_t *out, uint8_t *state, const uint8_t *in, size_t inlen);
void sm3(uint8_t *out, const uint8_t *in, size_t inlen);

void sm3_xof(unsigned char *out, unsigned long outlen,
              const unsigned char *in, unsigned long inlen);

void sm3_xofx4(unsigned char *out0, unsigned char *out1,
               unsigned char *out2, unsigned char *out3,
               unsigned long outlen,
               const unsigned char *in0, const unsigned char *in1,
               const unsigned char *in2, const unsigned char *in3,
               unsigned long inlen);
void seed_state_sm3(spx_ctx *ctx);

#endif

