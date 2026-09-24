/*
 * QingLuan Digital Signature Scheme
 * hash.c - SM3 hash and XOF implementation (GB/T 32905-2016)
 *
 * Multi-pipe SM3 widens the hash output across security levels:
 *   - QingLuan-128 (HASH_PIPES=1): standard SM3 (256-bit output)
 *   - QingLuan-256 (HASH_PIPES=2): SM3(0x00||m) || SM3(0x01||m)
 *   - QingLuan-512 (HASH_PIPES=4): 4 independent pipes
 *
 * The widened output provides (2nd-)preimage / TCR resistance up to its width.
 * It does NOT provide plain (birthday) collision resistance beyond ~2^128
 * (Joux multicollisions on a shared compression function). The protocol relies
 * on salted/randomized hashing (preimage/TCR), never on plain collision.
 *
 * XOF uses multi-pipe hash for absorb (producing PARAM_HASH_BYTES key),
 * then SM3 counter mode for squeeze: Oi = SM3(key || counter_i).
 */

#include "hash.h"
#include "utils.h"
#include <string.h>

/* ============================================================
 * SM3 core (unchanged)
 * ============================================================ */

static inline uint32_t rotl32(uint32_t x, int n)
{
    return (x << (n & 31)) | (x >> ((32 - n) & 31));
}

static inline uint32_t load32_be(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8)  | (uint32_t)p[3];
}

static inline void store32_be(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)(v);
}

static inline uint32_t sm3_ff(uint32_t x, uint32_t y, uint32_t z, int j)
{
    if (j < 16) return x ^ y ^ z;
    return (x & y) | (x & z) | (y & z);
}

static inline uint32_t sm3_gg(uint32_t x, uint32_t y, uint32_t z, int j)
{
    if (j < 16) return x ^ y ^ z;
    return (x & y) | ((~x) & z);
}

static inline uint32_t sm3_p0(uint32_t x)
{
    return x ^ rotl32(x, 9) ^ rotl32(x, 17);
}

static inline uint32_t sm3_p1(uint32_t x)
{
    return x ^ rotl32(x, 15) ^ rotl32(x, 23);
}

static void sm3_compress(uint32_t state[8], const uint8_t block[SM3_BLOCK_SIZE])
{
    uint32_t W[68];
    uint32_t W1[64];
    int i;

    for (i = 0; i < 16; i++)
        W[i] = load32_be(block + 4 * i);

    for (i = 16; i < 68; i++)
        W[i] = sm3_p1(W[i - 16] ^ W[i - 9] ^ rotl32(W[i - 3], 15))
               ^ rotl32(W[i - 13], 7) ^ W[i - 6];

    for (i = 0; i < 64; i++)
        W1[i] = W[i] ^ W[i + 4];

    uint32_t A = state[0], B = state[1], C = state[2], D = state[3];
    uint32_t E = state[4], F = state[5], G = state[6], H = state[7];

    for (int j = 0; j < 64; j++) {
        uint32_t T = (j < 16) ? 0x79cc4519u : 0x7a879d8au;
        uint32_t SS1 = rotl32(rotl32(A, 12) + E + rotl32(T, j), 7);
        uint32_t SS2 = SS1 ^ rotl32(A, 12);
        uint32_t TT1 = sm3_ff(A, B, C, j) + D + SS2 + W1[j];
        uint32_t TT2 = sm3_gg(E, F, G, j) + H + SS1 + W[j];
        D = C;
        C = rotl32(B, 9);
        B = A;
        A = TT1;
        H = G;
        G = rotl32(F, 19);
        F = E;
        E = sm3_p0(TT2);
    }

    state[0] ^= A; state[1] ^= B; state[2] ^= C; state[3] ^= D;
    state[4] ^= E; state[5] ^= F; state[6] ^= G; state[7] ^= H;
}

static const uint32_t SM3_IV[8] = {
    0x7380166fu, 0x4914b2b9u, 0x172442d7u, 0xda8a0600u,
    0xa96f30bcu, 0x163138aau, 0xe38dee4du, 0xb0fb0e4eu
};

/* Standalone SM3 (single-shot, always 32-byte output) */
void sm3(const uint8_t *data, size_t len, uint8_t *out)
{
    uint32_t state[8];
    memcpy(state, SM3_IV, sizeof(SM3_IV));

    size_t offset = 0;
    while (offset + SM3_BLOCK_SIZE <= len) {
        sm3_compress(state, data + offset);
        offset += SM3_BLOCK_SIZE;
    }

    uint8_t buf[SM3_BLOCK_SIZE * 2];
    size_t rem = len - offset;
    memcpy(buf, data + offset, rem);

    buf[rem] = 0x80;

    if (rem >= 56) {
        memset(buf + rem + 1, 0, SM3_BLOCK_SIZE * 2 - rem - 1);
        store32_be(buf + SM3_BLOCK_SIZE - 4, 0);
        store32_be(buf + SM3_BLOCK_SIZE - 8, 0);
        sm3_compress(state, buf);
        memset(buf, 0, 56);
    } else {
        memset(buf + rem + 1, 0, 55 - rem);
    }

    uint64_t bit_len = (uint64_t)len * 8;
    store32_be(buf + 56, (uint32_t)(bit_len >> 32));
    store32_be(buf + 60, (uint32_t)bit_len);
    sm3_compress(state, buf);

    for (int i = 0; i < 8; i++)
        store32_be(out + 4 * i, state[i]);
}

/* ============================================================
 * Multi-pipe incremental hash
 * ============================================================ */

void hash_init(hash_ctx_t *ctx)
{
    int p;
    for (p = 0; p < HASH_PIPES; p++)
        memcpy(ctx->state[p], SM3_IV, sizeof(SM3_IV));

    memset(ctx->buf, 0, sizeof(ctx->buf));
    ctx->buf_pos = 0;
    ctx->msg_len = 0;

#if HASH_PIPES > 1
    /*
     * Multi-pipe mode: each pipe gets a distinct prefix byte so that
     * pipe i computes SM3(byte(i) || message). This provides
     * HASH_PIPES * 128-bit collision resistance.
     * For HASH_PIPES == 1, no prefix is added (standard SM3 behavior).
     */
    for (p = 0; p < HASH_PIPES; p++)
        ctx->buf[p][0] = (uint8_t)p;
    ctx->buf_pos = 1;
    ctx->msg_len = 1;
#endif
}

void hash_update(hash_ctx_t *ctx, const uint8_t *data, size_t len)
{
    int p;
    ctx->msg_len += len;

    size_t offset = 0;

    /* Fill partial buffer */
    if (ctx->buf_pos > 0) {
        size_t fill = SM3_BLOCK_SIZE - ctx->buf_pos;
        if (fill > len) fill = len;
        for (p = 0; p < HASH_PIPES; p++)
            memcpy(ctx->buf[p] + ctx->buf_pos, data, fill);
        ctx->buf_pos += fill;
        offset += fill;

        if (ctx->buf_pos == SM3_BLOCK_SIZE) {
            for (p = 0; p < HASH_PIPES; p++)
                sm3_compress(ctx->state[p], ctx->buf[p]);
            ctx->buf_pos = 0;
        }
    }

    /* Process full blocks — all pipes see the same data block */
    while (offset + SM3_BLOCK_SIZE <= len) {
        for (p = 0; p < HASH_PIPES; p++)
            sm3_compress(ctx->state[p], data + offset);
        offset += SM3_BLOCK_SIZE;
    }

    /* Buffer remainder */
    if (len - offset > 0) {
        for (p = 0; p < HASH_PIPES; p++)
            memcpy(ctx->buf[p], data + offset, len - offset);
        ctx->buf_pos = len - offset;
    }
}

void hash_final(hash_ctx_t *ctx, uint8_t *out)
{
    int p;
    size_t out_pos = 0;

    for (p = 0; p < HASH_PIPES; p++) {
        /* Copy state so we can finalize independently per pipe */
        uint32_t st[8];
        memcpy(st, ctx->state[p], sizeof(st));

        uint8_t pad[SM3_BLOCK_SIZE * 2];
        size_t rem = ctx->buf_pos;
        memcpy(pad, ctx->buf[p], rem);

        pad[rem] = 0x80;

        uint8_t extra[SM3_BLOCK_SIZE];
        int need_extra = (rem >= 56) ? 1 : 0;

        if (need_extra) {
            memset(pad + rem + 1, 0, SM3_BLOCK_SIZE - rem - 1);
            sm3_compress(st, pad);
            memset(extra, 0, 56);
        } else {
            memset(pad + rem + 1, 0, 55 - rem);
            extra[0] = 0; /* unused */
        }

        uint8_t *final_buf = need_extra ? extra : pad;
        uint64_t bit_len = ctx->msg_len * 8;
        store32_be(final_buf + 56, (uint32_t)(bit_len >> 32));
        store32_be(final_buf + 60, (uint32_t)bit_len);
        sm3_compress(st, final_buf);

        uint8_t digest[SM3_DIGEST_SIZE];
        for (int i = 0; i < 8; i++)
            store32_be(digest + 4 * i, st[i]);

        /* Copy pipe's digest to output (may truncate last pipe) */
        size_t take = SM3_DIGEST_SIZE;
        if (out_pos + take > (size_t)PARAM_HASH_BYTES)
            take = (size_t)PARAM_HASH_BYTES - out_pos;
        memcpy(out + out_pos, digest, take);
        out_pos += take;

        secure_zero(st, sizeof(st));
        secure_zero(digest, sizeof(digest));
        secure_zero(pad, sizeof(pad));
    }
}

void hash_digest(uint8_t *out, const uint8_t *in, size_t inlen)
{
    hash_ctx_t ctx;
    hash_init(&ctx);
    hash_update(&ctx, in, inlen);
    hash_final(&ctx, out);
}

void hash_commit(uint8_t *out, const uint8_t *salt, size_t salt_len,
                 uint32_t idx, const uint8_t *payload, size_t payload_len)
{
    hash_ctx_t ctx;
    hash_init(&ctx);

    uint8_t dom = DOMAIN_COMMIT;
    hash_update(&ctx, &dom, 1);

    uint8_t lb[4];
    store32_be(lb, (uint32_t)salt_len);
    hash_update(&ctx, lb, 4);
    hash_update(&ctx, salt, salt_len);

    uint8_t ib[4];
    store32_be(ib, idx);
    hash_update(&ctx, ib, 4);

    store32_be(lb, (uint32_t)payload_len);
    hash_update(&ctx, lb, 4);
    hash_update(&ctx, payload, payload_len);

    hash_final(&ctx, out);
}

void hash_with_domain(uint8_t *out, uint8_t domain,
                      const uint8_t *in, size_t inlen)
{
    hash_ctx_t ctx;
    hash_init(&ctx);
    hash_update(&ctx, &domain, 1);
    /* Encode input length to prevent domain separation ambiguity
     * when the same domain tag is used with variable-length inputs */
    uint8_t len_buf[4];
    store32_be(len_buf, (uint32_t)inlen);
    hash_update(&ctx, len_buf, 4);
    hash_update(&ctx, in, inlen);
    hash_final(&ctx, out);
}

/* ============================================================
 * XOF: multi-pipe absorb + SM3 counter-mode squeeze
 *
 * Absorb phase uses the incremental multi-pipe hash to handle
 * arbitrary-length input (no fixed-size buffer).
 * Finalize produces a PARAM_HASH_BYTES-wide key.
 * Squeeze produces output via SM3(key || counter).
 * ============================================================ */

void xof_init(xof_ctx_t *ctx)
{
    hash_init(&ctx->absorb);
    memset(ctx->key, 0, sizeof(ctx->key));
    ctx->counter = 0;
    ctx->squeeze_pos = SM3_DIGEST_SIZE; /* force regeneration on first squeeze */
    ctx->finalized = 0;
    memset(ctx->squeeze_buf, 0, sizeof(ctx->squeeze_buf));
}

void xof_absorb(xof_ctx_t *ctx, const uint8_t *data, size_t len)
{
    hash_update(&ctx->absorb, data, len);
}

void xof_finalize(xof_ctx_t *ctx)
{
    hash_final(&ctx->absorb, ctx->key);
    /* Clear absorb state */
    secure_zero(&ctx->absorb, sizeof(ctx->absorb));
    ctx->counter = 0;
    ctx->squeeze_pos = SM3_DIGEST_SIZE;
    ctx->finalized = 1;
}

void xof_squeeze(xof_ctx_t *ctx, uint8_t *out, size_t outlen)
{
    size_t offset = 0;

    /* Use any leftover from previous squeeze */
    if (ctx->squeeze_pos < SM3_DIGEST_SIZE) {
        size_t avail = SM3_DIGEST_SIZE - ctx->squeeze_pos;
        size_t take = avail < outlen ? avail : outlen;
        memcpy(out, ctx->squeeze_buf + ctx->squeeze_pos, take);
        ctx->squeeze_pos += take;
        offset += take;
    }

    while (offset < outlen) {
        /* Generate next block: SM3(key || counter) */
        uint8_t tmp[PARAM_HASH_BYTES + 4];
        memcpy(tmp, ctx->key, PARAM_HASH_BYTES);
        store32_be(tmp + PARAM_HASH_BYTES, ctx->counter);
        sm3(tmp, PARAM_HASH_BYTES + 4, ctx->squeeze_buf);
        ctx->counter++;
        ctx->squeeze_pos = 0;

        size_t remaining = outlen - offset;
        size_t take = SM3_DIGEST_SIZE < remaining ? SM3_DIGEST_SIZE : remaining;
        memcpy(out + offset, ctx->squeeze_buf, take);
        ctx->squeeze_pos = take;
        offset += take;

        secure_zero(tmp, sizeof(tmp));
    }
}

void xof_with_domain(uint8_t *out, size_t outlen, uint8_t domain,
                     const uint8_t *in, size_t inlen)
{
    xof_ctx_t ctx;
    xof_init(&ctx);
    xof_absorb(&ctx, &domain, 1);
    /* Encode input length for domain separation safety */
    uint8_t len_buf[4];
    store32_be(len_buf, (uint32_t)inlen);
    xof_absorb(&ctx, len_buf, 4);
    xof_absorb(&ctx, in, inlen);
    xof_finalize(&ctx);
    xof_squeeze(&ctx, out, outlen);
}
