/*
Copyright (c) 2026 Hang Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences
*/
#ifndef SYMMETRIC_H
#define SYMMETRIC_H

#include <stddef.h>
#include <stdint.h>

#ifndef BIT_USE_SHAKE
#define BIT_USE_SHAKE 0
#endif

#if BIT_USE_SHAKE
#include "fips202.h"
#include "fips202x4.h"
#define BIT_XOF256_RATE SHAKE256_RATE
#define BIT_XOF128_RATE SHAKE128_RATE
#else
#define BIT_XOF256_RATE 32
#define BIT_XOF128_RATE 32
#endif

/* ==========================================================================
   Stream types
   ========================================================================== */

/* SHAKE256 per-polynomial stream — sampling & rejection */
typedef struct {
#if BIT_USE_SHAKE
    keccak_state state;
#else
    const unsigned char *seed;
    size_t seed_len_bytes;
    uint32_t block;
#endif
    uint16_t nonce;
} bit_xof256_state;

/* SHAKE256 block-level context — challenge expansion */
typedef struct {
#if BIT_USE_SHAKE
    keccak_state state;
#else
    const unsigned char *seed;
    size_t seed_len_bytes;
    uint32_t block;
#endif
} bit_xof256_ctx;

/* SHAKE128 per-polynomial stream — matrix expansion */
typedef struct {
#if BIT_USE_SHAKE
    keccak_state state;
    uint8_t leftover[SHAKE128_RATE];
    size_t leftover_len;
    size_t leftover_pos;
#else
    const unsigned char *seed;
    size_t seed_len_bytes;
    uint32_t block;
#endif
} bit_xof128_state;

/* SHAKE128 x4 stream — batch matrix expansion */
typedef struct {
#if BIT_USE_SHAKE
    keccakx4_state state;
#else
    const unsigned char *seed;
    size_t seed_len_bytes;
    uint32_t block0, block1, block2, block3;
#endif
} bit_xof128x4_state;

#ifdef __cplusplus
extern "C" {
#endif

/* ==========================================================================
   1. Hash — fixed-output (SHA3-256 / SM3)
   ========================================================================== */

/* H256(msg) — SHA3-256 / SM3-256, 32-byte output */
void bit_h256(uint8_t out[32], const uint8_t *in, size_t inlen);
void bit_h256_2(uint8_t out[32],
                const uint8_t *in0, size_t inlen0,
                const uint8_t *in1, size_t inlen1);

/* H512(msg) — SHA3-512, 64-byte output; used by BiT-256/512 */
void bit_h512(uint8_t out[64], const uint8_t *in, size_t inlen);
void bit_h512_2(uint8_t out[64],
                const uint8_t *in0, size_t inlen0,
                const uint8_t *in1, size_t inlen1);

/* ==========================================================================
   2. XOF one-shot — fixed-length expand
   ========================================================================== */

/* X(seed, outlen) */
void bit_xof256(uint8_t *out, size_t outlen, const uint8_t *in, size_t inlen);

/* X(seed ‖ nonce, outlen) — PRF for one-shot sampling */
void bit_xof256_nonce(uint8_t *out, size_t outlen,
                      const uint8_t *seed, size_t seedlen, uint16_t nonce);

/* X(a ‖ b ‖ c, outlen) — three-input absorb, no intermediate buffer */
void bit_xof256_3(uint8_t *out, size_t outlen,
                  const uint8_t *in0, size_t inlen0,
                  const uint8_t *in1, size_t inlen1,
                  const uint8_t *in2, size_t inlen2);

/* ==========================================================================
   3. XOF batched — parallel expand
   ========================================================================== */

/* 2 lanes */
void bit_xof256x2(uint8_t *out0, uint8_t *out1,
                  size_t outlen,
                  const uint8_t *seed, size_t seedlen,
                  uint16_t n0, uint16_t n1);

/* 4 lanes */
void bit_xof256x4(uint8_t *out0, uint8_t *out1, uint8_t *out2, uint8_t *out3,
                  size_t outlen,
                  const uint8_t *seed, size_t seedlen,
                  uint16_t n0, uint16_t n1, uint16_t n2, uint16_t n3);

/* N lanes (any count), auto-batched in x4 groups + remainder */
void bit_xof256_xn(uint8_t *const out[], size_t outlen,
                   const uint8_t *seed, size_t seedlen,
                   const uint16_t *nonce, size_t count);

/* ==========================================================================
   4. XOF stateful — SHAKE256
   ========================================================================== */

/* Per-polynomial stream: init / squeeze / zeroize */
void bit_xof256_init   (bit_xof256_state *st, const uint8_t *seed, size_t seedlen, uint16_t nonce);
void bit_xof256_squeeze(bit_xof256_state *st, uint8_t *out, size_t outlen);
void bit_xof256_zeroize(bit_xof256_state *st);

/* Block-level stream — challenge expansion */
void bit_xof256_ctx_init        (bit_xof256_ctx *ctx, const uint8_t *seed, size_t seedlen);
void bit_xof256_ctx_squeezeblocks(bit_xof256_ctx *ctx, uint8_t *out, size_t nblocks);
void bit_xof256_ctx_zeroize     (bit_xof256_ctx *ctx);

/* ==========================================================================
   5. XOF stateful — SHAKE128 (matrix expansion)
   ========================================================================== */

/* Single lane */
void bit_xof128_init   (bit_xof128_state *st, const uint8_t *in, size_t inlen);
void bit_xof128_squeeze(bit_xof128_state *st, uint8_t *out, size_t outlen);
void bit_xof128_zeroize(bit_xof128_state *st);

/* 4-lane batch */
void bit_xof128x4_init         (bit_xof128x4_state *st,
                                const uint8_t *in0, const uint8_t *in1,
                                const uint8_t *in2, const uint8_t *in3,
                                size_t inlen);
void bit_xof128x4_squeezeblocks(bit_xof128x4_state *st,
                                uint8_t *out0, uint8_t *out1,
                                uint8_t *out2, uint8_t *out3,
                                size_t nblocks);

#ifdef __cplusplus
}
#endif
#endif
