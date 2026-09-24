/*
 * QingLuan Digital Signature Scheme
 * hash.h - Cryptographic hash and XOF interface (SM3-based)
 *
 * SM3 is the Chinese national hash standard (GB/T 32905-2016).
 *
 * Multi-pipe construction widens the output for security levels > 128 bits:
 *   For PARAM_HASH_BYTES > 32, multiple independent SM3 instances
 *   (pipes) are run with distinct prefix bytes. Pipe i processes
 *   SM3(byte(i) || message); Output = pipe_0 || pipe_1 || ...
 *
 *   SECURITY CLAIM: the widened output provides (2nd-)PREIMAGE / target-
 *   collision (TCR) resistance up to its width. It does NOT claim plain
 *   (birthday) collision resistance beyond a single SM3 (~2^128): concatenating
 *   the same compression function is subject to Joux multicollisions. The
 *   protocol is therefore designed (salted commitments / randomized message
 *   hash) to rely on preimage/TCR, not on plain collision resistance.
 *
 * For PARAM_HASH_BYTES == 32 (QingLuan-128), HASH_PIPES == 1,
 * and no prefix is added, preserving standard SM3 behavior.
 *
 * XOF is built on the multi-pipe hash for absorb, producing a
 * PARAM_HASH_BYTES-wide key, then SM3 counter mode for squeeze.
 */

#ifndef QINGLUAN_HASH_H
#define QINGLUAN_HASH_H

#include "params.h"
#include <stddef.h>
#include <stdint.h>

#define SM3_BLOCK_SIZE  64
#define SM3_DIGEST_SIZE 32

/* Number of independent SM3 pipes needed for the target hash width */
#define HASH_PIPES ((PARAM_HASH_BYTES + SM3_DIGEST_SIZE - 1) / SM3_DIGEST_SIZE)

/*
 * Incremental hash context (multi-pipe for wider outputs).
 * Each pipe maintains independent SM3 state and buffer.
 * For HASH_PIPES == 1, this is equivalent to standard SM3.
 */
typedef struct {
    uint32_t state[HASH_PIPES][8];
    uint8_t  buf[HASH_PIPES][SM3_BLOCK_SIZE];
    size_t   buf_pos;
    uint64_t msg_len;
} hash_ctx_t;

/*
 * XOF context. Uses multi-pipe hash for absorb phase to produce
 * a PARAM_HASH_BYTES-wide key, then SM3 counter mode for squeeze.
 * No fixed-size absorb buffer — handles arbitrary input length.
 */
typedef struct {
    hash_ctx_t absorb;                    /* Incremental hash for absorb */
    uint8_t    key[PARAM_HASH_BYTES];     /* Wide key from finalized absorb */
    uint32_t   counter;
    uint8_t    squeeze_buf[SM3_DIGEST_SIZE];
    size_t     squeeze_pos;
    int        finalized;
} xof_ctx_t;

void hash_init(hash_ctx_t *ctx);
void hash_update(hash_ctx_t *ctx, const uint8_t *data, size_t len);
void hash_final(hash_ctx_t *ctx, uint8_t *out);
void hash_digest(uint8_t *out, const uint8_t *in, size_t inlen);
void hash_with_domain(uint8_t *out, uint8_t domain,
                      const uint8_t *in, size_t inlen);

/*
 * Salted target-collision-resistant (TCR) commitment, width PARAM_HASH_BYTES:
 *   out = Hw(DOMAIN_COMMIT || salt_len || salt || idx_be32 || payload_len || payload)
 * Per-signature salt makes binding rely on (2nd-)preimage/TCR, not plain collision.
 */
void hash_commit(uint8_t *out,
                 const uint8_t *salt, size_t salt_len,
                 uint32_t idx,
                 const uint8_t *payload, size_t payload_len);

void xof_init(xof_ctx_t *ctx);
void xof_absorb(xof_ctx_t *ctx, const uint8_t *data, size_t len);
void xof_finalize(xof_ctx_t *ctx);
void xof_squeeze(xof_ctx_t *ctx, uint8_t *out, size_t outlen);
void xof_with_domain(uint8_t *out, size_t outlen, uint8_t domain,
                     const uint8_t *in, size_t inlen);

void sm3(const uint8_t *data, size_t len, uint8_t *out);

#endif /* QINGLUAN_HASH_H */
