/*
 * QingLuan Digital Signature Scheme
 * hash.h - API_PKC adapter variant
 *
 * This header SHADOWS Reference_Implementation/include/hash.h when
 * compiled with -I API_PKC_Adapter before -I Reference_Implementation/include.
 *
 * Struct layouts below are private to the adapter. Function signatures
 * match Reference_Implementation/include/hash.h, so QingLuan source files
 * (rsdp.c, mpc.c, seedtree.c, sign.c, verify.c, keygen.c) recompile
 * unchanged against this header.
 */

#ifndef QINGLUAN_HASH_H
#define QINGLUAN_HASH_H

#include "params.h"
#include <stddef.h>
#include <stdint.h>

#define SM3_BLOCK_SIZE  64
#define SM3_DIGEST_SIZE 32
#define HASH_PIPES ((PARAM_HASH_BYTES + SM3_DIGEST_SIZE - 1) / SM3_DIGEST_SIZE)

/* Incremental hash: dynamic input buffer, multi-pipe SM3 digest produced at
 * hash_final() (matches Reference_Implementation/src/hash.c). */
typedef struct {
    uint8_t *buf;
    size_t   len;
    size_t   cap;
} hash_ctx_t;

/* Incremental XOF: absorb buffer -> multi-pipe key at finalize, then
 * SM3 counter-mode squeeze O_i = SM3(key || BE32(counter)). */
typedef struct {
    uint8_t *buf;
    size_t   len;
    size_t   cap;
    uint8_t  key[PARAM_HASH_BYTES];   /* = multipipe(absorbed), set at finalize */
    uint32_t counter;
    uint8_t  cache[SM3_DIGEST_SIZE];
    uint8_t  cache_pos;
    uint8_t  finalized;
} xof_ctx_t;

void hash_init(hash_ctx_t *ctx);
void hash_update(hash_ctx_t *ctx, const uint8_t *data, size_t len);
void hash_final(hash_ctx_t *ctx, uint8_t *out);
void hash_digest(uint8_t *out, const uint8_t *in, size_t inlen);
void hash_with_domain(uint8_t *out, uint8_t domain,
                      const uint8_t *in, size_t inlen);

void xof_init(xof_ctx_t *ctx);
void xof_absorb(xof_ctx_t *ctx, const uint8_t *data, size_t len);
void xof_finalize(xof_ctx_t *ctx);
void xof_squeeze(xof_ctx_t *ctx, uint8_t *out, size_t outlen);
void xof_with_domain(uint8_t *out, size_t outlen, uint8_t domain,
                     const uint8_t *in, size_t inlen);

void sm3(const uint8_t *data, size_t len, uint8_t *out);

#endif /* QINGLUAN_HASH_H */
