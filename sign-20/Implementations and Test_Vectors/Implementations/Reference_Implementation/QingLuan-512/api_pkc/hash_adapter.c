/*
 * QingLuan hash/XOF — API_PKC integration variant
 *
 * Replicates the Reference_Implementation construction (the AUTHORITATIVE
 * QingLuan hash; analysed in docs/security-argument.md §3) on top of the
 * framework's SM3 primitive sm3hash() from auxfunc.{h,c}:
 *
 *   - Wide hash = multi-pipe SM3.  pipe p = SM3( byte(p) || message ),
 *     concatenated to PARAM_HASH_BYTES.  HASH_PIPES = 1/2/3/4 for the
 *     128/256/384/512 levels.  HASH_PIPES == 1 is plain SM3 (no prefix byte).
 *
 *   - XOF = multi-pipe absorb -> PARAM_HASH_BYTES key, then SM3 counter mode:
 *     O_i = SM3( key || BE32(counter) ), counter starting at 0.
 *
 * This is byte-identical to Reference_Implementation/src/hash.c (same GB/T
 * 32905 SM3, same construction), so adapter-generated keys, signatures and
 * KAT vectors match the reference implementation exactly.
 *
 * NOTE: earlier revisions routed the wide hash/XOF through auxfunc's
 * pseudohash()/pseudoXOF() (an HMAC-SM3 cascade).  That is a DIFFERENT
 * construction and produced signatures incompatible with the reference; it
 * also did not support the 384 level.  Replaced here so the adapter and the
 * reference are one algorithm.
 *
 * Incremental hash/XOF buffer the full input and hash once at finalize.
 */

#include "hash.h"
#include "utils.h"
#include "auxfunc.h"
#include <stdlib.h>
#include <string.h>

static void buf_grow(uint8_t **buf, size_t *cap, size_t need)
{
    if (*cap >= need) return;
    size_t new_cap = *cap ? *cap : 256;
    while (new_cap < need) new_cap *= 2;
    uint8_t *nb = (uint8_t *)realloc(*buf, new_cap);
    if (!nb) abort();
    *buf = nb;
    *cap = new_cap;
}

/*
 * Multi-pipe wide hash over a contiguous message, using the framework SM3.
 * pipe p = SM3(byte(p) || msg); HASH_PIPES==1 -> plain SM3(msg), no prefix.
 * Writes exactly PARAM_HASH_BYTES (= HASH_PIPES * 32) bytes to out.
 */
static void multipipe_hash(const uint8_t *msg, size_t mlen, uint8_t *out)
{
    uint8_t one = 0;
    const uint8_t *m = mlen ? msg : &one;   /* never deref NULL on empty input */

    if (HASH_PIPES == 1) {
        sm3hash(256, m, (unsigned long long)mlen * 8, out);
        return;
    }
    for (int p = 0; p < HASH_PIPES; p++) {
        size_t tlen = mlen + 1;
        uint8_t *tmp = (uint8_t *)malloc(tlen);
        if (!tmp) abort();
        tmp[0] = (uint8_t)p;
        if (mlen) memcpy(tmp + 1, m, mlen);
        sm3hash(256, tmp, (unsigned long long)tlen * 8,
                out + (size_t)p * SM3_DIGEST_SIZE);
        secure_zero(tmp, tlen);
        free(tmp);
    }
}

/* ============================================================
 * Incremental hash
 * ============================================================ */

void hash_init(hash_ctx_t *ctx)
{
    ctx->buf = NULL;
    ctx->len = 0;
    ctx->cap = 0;
}

void hash_update(hash_ctx_t *ctx, const uint8_t *data, size_t len)
{
    buf_grow(&ctx->buf, &ctx->cap, ctx->len + len);
    memcpy(ctx->buf + ctx->len, data, len);
    ctx->len += len;
}

void hash_final(hash_ctx_t *ctx, uint8_t *out)
{
    multipipe_hash(ctx->buf, ctx->len, out);

    if (ctx->buf) {
        secure_zero(ctx->buf, ctx->len);
        free(ctx->buf);
    }
    ctx->buf = NULL;
    ctx->len = 0;
    ctx->cap = 0;
}

void hash_digest(uint8_t *out, const uint8_t *in, size_t inlen)
{
    hash_ctx_t ctx;
    hash_init(&ctx);
    hash_update(&ctx, in, inlen);
    hash_final(&ctx, out);
}

void hash_with_domain(uint8_t *out, uint8_t domain,
                      const uint8_t *in, size_t inlen)
{
    hash_ctx_t ctx;
    hash_init(&ctx);
    hash_update(&ctx, &domain, 1);
    uint8_t len_buf[4];
    u32_to_be(len_buf, (uint32_t)inlen);
    hash_update(&ctx, len_buf, 4);
    hash_update(&ctx, in, inlen);
    hash_final(&ctx, out);
}

/* Standalone SM3 — thin wrapper over the framework sm3hash. */
void sm3(const uint8_t *data, size_t len, uint8_t *out)
{
    sm3hash(256, data, (unsigned long long)len * 8, out);
}

/* ============================================================
 * XOF: multi-pipe absorb + SM3 counter-mode squeeze
 *   key = multipipe(absorbed);  O_i = SM3(key || BE32(counter)), counter 0..
 * ============================================================ */

void xof_init(xof_ctx_t *ctx)
{
    ctx->buf = NULL;
    ctx->len = 0;
    ctx->cap = 0;
    memset(ctx->key, 0, sizeof(ctx->key));
    ctx->counter = 0;
    ctx->cache_pos = SM3_DIGEST_SIZE;   /* cache empty */
    ctx->finalized = 0;
}

void xof_absorb(xof_ctx_t *ctx, const uint8_t *data, size_t len)
{
    buf_grow(&ctx->buf, &ctx->cap, ctx->len + len);
    memcpy(ctx->buf + ctx->len, data, len);
    ctx->len += len;
}

void xof_finalize(xof_ctx_t *ctx)
{
    multipipe_hash(ctx->buf, ctx->len, ctx->key);
    if (ctx->buf) {
        secure_zero(ctx->buf, ctx->len);
        free(ctx->buf);
    }
    ctx->buf = NULL;
    ctx->len = 0;
    ctx->cap = 0;
    ctx->counter = 0;
    ctx->cache_pos = SM3_DIGEST_SIZE;
    ctx->finalized = 1;
}

static void xof_refill(xof_ctx_t *ctx)
{
    uint8_t tmp[PARAM_HASH_BYTES + 4];
    memcpy(tmp, ctx->key, PARAM_HASH_BYTES);
    u32_to_be(tmp + PARAM_HASH_BYTES, ctx->counter);
    sm3hash(256, tmp, (unsigned long long)(PARAM_HASH_BYTES + 4) * 8, ctx->cache);
    secure_zero(tmp, sizeof(tmp));
    ctx->counter++;
    ctx->cache_pos = 0;
}

void xof_squeeze(xof_ctx_t *ctx, uint8_t *out, size_t outlen)
{
    if (!ctx->finalized) return;

    size_t off = 0;
    while (off < outlen) {
        if (ctx->cache_pos >= SM3_DIGEST_SIZE) xof_refill(ctx);
        size_t avail = SM3_DIGEST_SIZE - ctx->cache_pos;
        size_t take = avail < (outlen - off) ? avail : (outlen - off);
        memcpy(out + off, ctx->cache + ctx->cache_pos, take);
        ctx->cache_pos += (uint8_t)take;
        off += take;
    }
}

void xof_with_domain(uint8_t *out, size_t outlen, uint8_t domain,
                     const uint8_t *in, size_t inlen)
{
    xof_ctx_t ctx;
    xof_init(&ctx);
    xof_absorb(&ctx, &domain, 1);
    uint8_t len_buf[4];
    u32_to_be(len_buf, (uint32_t)inlen);
    xof_absorb(&ctx, len_buf, 4);
    xof_absorb(&ctx, in, inlen);
    xof_finalize(&ctx);
    xof_squeeze(&ctx, out, outlen);

    if (ctx.buf) {
        secure_zero(ctx.buf, ctx.len);
        free(ctx.buf);
    }
}
