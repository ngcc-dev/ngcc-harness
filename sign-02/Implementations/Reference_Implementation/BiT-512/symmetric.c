/*
Copyright (c) 2026 Hang Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense, Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences
*/
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#if BIT_USE_SHAKE
#include "fips202.h"
#else
#include "auxfunc.h"
#endif
#include "symmetric.h"
#include "params.h"
#include "endian.h"

static inline void secure_zero(void *ptr, size_t len) {
    volatile unsigned char *p = (volatile unsigned char *)ptr;
    while (len--) *p++ = 0;
}

/* Fixed-output hash (SHA3-256 / SM3) */

void bit_h256(uint8_t out[32], const uint8_t *in, size_t inlen)
{
#if BIT_USE_SHAKE
    sha3_256(out, in, inlen);
#else
    (void)sm3hash(256, in, (unsigned long long)inlen * 8, out);
#endif
}

void bit_h256_2(uint8_t out[32],
                const uint8_t *in0, size_t inlen0,
                const uint8_t *in1, size_t inlen1)
{
#if BIT_USE_SHAKE
    sha3_256incctx state;
    sha3_256_inc_init(&state);
    sha3_256_inc_absorb(&state, in0, inlen0);
    sha3_256_inc_absorb(&state, in1, inlen1);
    sha3_256_inc_finalize(out, &state);
#else
    unsigned char buf[4096];
    size_t total = inlen0 + inlen1;
    if (total > sizeof(buf)) {
        unsigned char *msg = malloc(total);
        if (msg == NULL) return;
        memcpy(msg, in0, inlen0);
        memcpy(msg + inlen0, in1, inlen1);
        (void)sm3hash(256, msg, (unsigned long long)total * 8, out);
        secure_zero(msg, total);
        free(msg);
    } else {
        memcpy(buf, in0, inlen0);
        memcpy(buf + inlen0, in1, inlen1);
        (void)sm3hash(256, buf, (unsigned long long)total * 8, out);
        secure_zero(buf, total);
    }
#endif
}

void bit_h512(uint8_t out[64], const uint8_t *in, size_t inlen)
{
#if BIT_USE_SHAKE
    sha3_512(out, in, inlen);
#else
    (void)pseudohash(512, in, (unsigned long long)inlen * 8, out);
#endif
}

void bit_h512_2(uint8_t out[64],
                const uint8_t *in0, size_t inlen0,
                const uint8_t *in1, size_t inlen1)
{
#if BIT_USE_SHAKE
    sha3_512incctx state;
    sha3_512_inc_init(&state);
    sha3_512_inc_absorb(&state, in0, inlen0);
    sha3_512_inc_absorb(&state, in1, inlen1);
    sha3_512_inc_finalize(out, &state);
#else
    unsigned char buf[4096];
    size_t total = inlen0 + inlen1;
    if (total > sizeof(buf)) {
        unsigned char *msg = malloc(total);
        if (msg == NULL) return;
        memcpy(msg, in0, inlen0);
        memcpy(msg + inlen0, in1, inlen1);
        (void)pseudohash(512, msg, (unsigned long long)total * 8, out);
        secure_zero(msg, total);
        free(msg);
    } else {
        memcpy(buf, in0, inlen0);
        memcpy(buf + inlen0, in1, inlen1);
        (void)pseudohash(512, buf, (unsigned long long)total * 8, out);
        secure_zero(buf, total);
    }
#endif
}

/* One-shot SHAKE256 XOF */

void bit_xof256(uint8_t *out, size_t outlen, const uint8_t *in, size_t inlen)
{
#if BIT_USE_SHAKE
    shake256(out, outlen, in, inlen);
#else
    (void)pseudoXOF((unsigned long long)outlen * 8,
                    in,
                    (unsigned long long)inlen * 8,
                    out);
#endif
}

void bit_xof256_2(uint8_t *out, size_t outlen,
                  const uint8_t *in0, size_t inlen0,
                  const uint8_t *in1, size_t inlen1)
{
    unsigned char buf[1024], *msg = buf;
    size_t total = inlen0 + inlen1;
    if (total > sizeof(buf)) { msg = malloc(total); if (!msg) return; }
    memcpy(msg, in0, inlen0);
    memcpy(msg + inlen0, in1, inlen1);
    bit_xof256(out, outlen, msg, total);
    secure_zero(msg, total);
    if (msg != buf) { free(msg); }
}

/* One-shot SHAKE256 with seed || nonce */

void bit_xof256_nonce(uint8_t *out, size_t outlen,
                      const uint8_t *seed, size_t seedlen, uint16_t nonce)
{
#if BIT_USE_SHAKE
    uint8_t extseed[BIT_SEEDBYTES + sizeof(uint16_t)];

    memcpy(extseed, seed, seedlen);
    store16_le(extseed + seedlen, nonce);
    shake256(out, outlen, extseed, seedlen + sizeof(uint16_t));
    secure_zero(extseed, sizeof(extseed));
#else
    uint8_t extseed[BIT_SEEDBYTES + sizeof(uint16_t)];
    memcpy(extseed, seed, seedlen);
    store16_le(extseed + seedlen, nonce);
    (void)pseudoXOF((unsigned long long)outlen * 8,
                    extseed,
                    (unsigned long long)(seedlen + sizeof(uint16_t)) * 8,
                    out);
    secure_zero(extseed, sizeof(extseed));
#endif
}

/* Three-input absorb XOF */

void bit_xof256_3(uint8_t *out, size_t outlen,
                  const uint8_t *in0, size_t inlen0,
                  const uint8_t *in1, size_t inlen1,
                  const uint8_t *in2, size_t inlen2)
{
#if BIT_USE_SHAKE
    shake256incctx state;
    shake256_inc_init(&state);
    shake256_inc_absorb(&state, in0, inlen0);
    shake256_inc_absorb(&state, in1, inlen1);
    shake256_inc_absorb(&state, in2, inlen2);
    shake256_inc_finalize(&state);
    shake256_inc_squeeze(out, outlen, &state);
    secure_zero(state.ctx, PQC_SHAKEINCCTX_BYTES);
    shake256_inc_ctx_release(&state);
#else
    uint8_t msg[BIT_SEEDBYTES + BIT_SEEDBYTES + BIT_MESSAGEBYTES];
    memcpy(msg, in0, inlen0);
    memcpy(msg + inlen0, in1, inlen1);
    memcpy(msg + inlen0 + inlen1, in2, inlen2);
    size_t msg_len = inlen0 + inlen1 + inlen2;
    (void)pseudoXOF((unsigned long long)outlen * 8, msg, (unsigned long long)msg_len * 8, out);
    secure_zero(msg, msg_len);
#endif
}

/* SHAKE256 stateful stream */

void bit_xof256_init(bit_xof256_state *st, const uint8_t *seed, size_t seedlen, uint16_t nonce)
{
#if BIT_USE_SHAKE
    uint8_t nonce_le[2];
    shake256_inc_init(&st->state);
    shake256_inc_absorb(&st->state, seed, seedlen);
    store16_le(nonce_le, nonce);
    shake256_inc_absorb(&st->state, nonce_le, sizeof(nonce_le));
    shake256_inc_finalize(&st->state);
#else
    st->seed = seed;
    st->seed_len_bytes = seedlen;
    st->block = 0;
#endif
    st->nonce = nonce;
}

void bit_xof256_squeeze(bit_xof256_state *st, uint8_t *out, size_t outlen)
{
#if BIT_USE_SHAKE
    shake256_inc_squeeze(out, outlen, &st->state);
#else
    uint8_t extseed[st->seed_len_bytes + 2 + 4];
    memcpy(extseed, st->seed, st->seed_len_bytes);
    store16_le(extseed + st->seed_len_bytes, st->nonce);
    store32_be(extseed + st->seed_len_bytes + 2, st->block);
    st->block++;
    (void)bit_xof256(out, outlen, extseed, st->seed_len_bytes + 2 + 4);
#endif
}

void bit_xof256_zeroize(bit_xof256_state *st)
{
#if BIT_USE_SHAKE
    secure_zero(st->state.ctx, PQC_SHAKEINCCTX_BYTES);
    shake256_inc_ctx_release(&st->state);
#else
    (void)st;
#endif
}

/* SHAKE256 block-level context (challenge expansion) */

void bit_xof256_ctx_init(bit_xof256_ctx *ctx, const uint8_t *seed, size_t seedlen)
{
#if BIT_USE_SHAKE
    shake256_absorb(&ctx->state, seed, seedlen);
#else
    ctx->seed = seed;
    ctx->seed_len_bytes = seedlen;
    ctx->block = 0;
#endif
}

void bit_xof256_ctx_squeezeblocks(bit_xof256_ctx *ctx, uint8_t *out, size_t nblocks)
{
#if BIT_USE_SHAKE
    shake256_squeezeblocks(out, nblocks, &ctx->state);
#else
    for (size_t i = 0; i < nblocks; i++) {
        uint8_t extseed[ctx->seed_len_bytes + 4];
        memcpy(extseed, ctx->seed, ctx->seed_len_bytes);
        store32_be(extseed + ctx->seed_len_bytes, ctx->block);
        ctx->block++;
        (void)bit_xof256(out + i * BIT_XOF256_RATE, BIT_XOF256_RATE, extseed, sizeof(extseed));
    }
#endif
}

void bit_xof256_ctx_zeroize(bit_xof256_ctx *ctx)
{
#if BIT_USE_SHAKE
    shake256_ctx_release(&ctx->state);
#else
    (void)ctx;
#endif
}

/* SHAKE128 stateful stream (matrix expansion) */

void bit_xof128_init(bit_xof128_state *st, const uint8_t *in, size_t inlen)
{
#if BIT_USE_SHAKE
    shake128_absorb(&st->state, in, inlen);
    st->leftover_len = 0;
    st->leftover_pos = 0;
#else
    st->seed = in;
    st->seed_len_bytes = inlen;
    st->block = 0;
#endif
}

void bit_xof128_squeeze(bit_xof128_state *st, uint8_t *out, size_t outlen)
{
#if BIT_USE_SHAKE
    /* Drain leftovers from previous call */
    if (st->leftover_pos < st->leftover_len) {
        size_t left = st->leftover_len - st->leftover_pos;
        size_t take = outlen < left ? outlen : left;
        memcpy(out, st->leftover + st->leftover_pos, take);
        st->leftover_pos += take;
        out += take;
        outlen -= take;
    }

    /* Full blocks */
    size_t nblocks = outlen / SHAKE128_RATE;
    if (nblocks != 0) {
        shake128_squeezeblocks(out, nblocks, &st->state);
        out += nblocks * SHAKE128_RATE;
        outlen -= nblocks * SHAKE128_RATE;
    }

    /* Partial tail block */
    if (outlen != 0) {
        shake128_squeezeblocks(st->leftover, 1, &st->state);
        memcpy(out, st->leftover, outlen);
        st->leftover_pos = outlen;
        st->leftover_len = SHAKE128_RATE;
    } else if (st->leftover_pos == st->leftover_len) {
        st->leftover_pos = 0;
        st->leftover_len = 0;
    }
#else
    uint8_t extseed[st->seed_len_bytes + 6];
    memcpy(extseed, st->seed, st->seed_len_bytes);
    extseed[st->seed_len_bytes] = 0; extseed[st->seed_len_bytes + 1] = 0;
    store32_be(extseed + st->seed_len_bytes + 2, st->block);
    st->block++;
    (void)bit_xof256(out, outlen, extseed, st->seed_len_bytes + 2 + 4);
#endif
}

void bit_xof128_zeroize(bit_xof128_state *st)
{
#if BIT_USE_SHAKE
    shake128_ctx_release(&st->state);
#else
    (void)st;
#endif
}
