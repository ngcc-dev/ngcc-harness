/*
 * Copyright (c) 2026 Hang Zhang.
 * State Key Laboratory of Cyberspace Security Defense,
 * Institute of Information Engineering, CAS
 * School of Cyber Security, University of Chinese Academy of Sciences
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
#define BIT_XOF256_RATE SHAKE256_RATE
#define BIT_XOF128_RATE SHAKE128_RATE
#else
#define BIT_XOF256_RATE 32
#define BIT_XOF128_RATE 32
#endif

/* SHAKE256 stateful stream */
typedef struct {
#if BIT_USE_SHAKE
    shake256incctx state;
#else
    const unsigned char *seed;
    size_t seed_len_bytes;
    uint32_t block;
#endif
    uint16_t nonce;
} bit_xof256_state;

/* SHAKE128 stateful stream */
typedef struct {
#if BIT_USE_SHAKE
    shake128ctx state;
    uint8_t leftover[SHAKE128_RATE];
    size_t leftover_len;
    size_t leftover_pos;
#else
    const unsigned char *seed;
    size_t seed_len_bytes;
    uint32_t block;
#endif
} bit_xof128_state;
/* SHAKE256 block-level context (challenge expansion) */
typedef struct {
#if BIT_USE_SHAKE
    shake256ctx state;
#else
    const unsigned char *seed;
    size_t seed_len_bytes;
    uint32_t block;
#endif
} bit_xof256_ctx;

#ifdef __cplusplus
extern "C" {
#endif

/* Fixed-output hash (SHA3-256 / SM3) */
void bit_h256(uint8_t out[32], const uint8_t *in, size_t inlen);
void bit_h256_2(uint8_t out[32],
                const uint8_t *in0, size_t inlen0,
                const uint8_t *in1, size_t inlen1);
void bit_h512(uint8_t out[64], const uint8_t *in, size_t inlen);
void bit_h512_2(uint8_t out[64],
                const uint8_t *in0, size_t inlen0,
                const uint8_t *in1, size_t inlen1);

/* One-shot SHAKE256 XOF */
void bit_xof256(uint8_t *out, size_t outlen, const uint8_t *in, size_t inlen);

/* One-shot SHAKE256 with seed || nonce */
void bit_xof256_nonce(uint8_t *out, size_t outlen,
                      const uint8_t *seed, size_t seedlen, uint16_t nonce);
/* Three-input absorb XOF */
void bit_xof256_3(uint8_t *out, size_t outlen,
                  const uint8_t *in0, size_t inlen0,
                  const uint8_t *in1, size_t inlen1,
                  const uint8_t *in2, size_t inlen2);

/* SHAKE256 stateful stream */
void bit_xof256_init   (bit_xof256_state *st, const uint8_t *seed, size_t seedlen, uint16_t nonce);
void bit_xof256_squeeze(bit_xof256_state *st, uint8_t *out, size_t outlen);
void bit_xof256_zeroize(bit_xof256_state *st);

/* SHAKE256 block-level context */
void bit_xof256_ctx_init        (bit_xof256_ctx *ctx, const uint8_t *seed, size_t seedlen);
void bit_xof256_ctx_squeezeblocks(bit_xof256_ctx *ctx, uint8_t *out, size_t nblocks);
void bit_xof256_ctx_zeroize     (bit_xof256_ctx *ctx);

/* SHAKE128 stateful stream (matrix expansion) */
void bit_xof128_init   (bit_xof128_state *st, const uint8_t *in, size_t inlen);
void bit_xof128_squeeze(bit_xof128_state *st, uint8_t *out, size_t outlen);
void bit_xof128_zeroize(bit_xof128_state *st);

#ifdef __cplusplus
}
#endif
#endif
