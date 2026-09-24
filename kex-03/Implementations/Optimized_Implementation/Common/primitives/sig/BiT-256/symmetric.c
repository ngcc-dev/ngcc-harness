/*
Copyright (c) 2026 Hang Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences
*/
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#if BIT_USE_SHAKE
#include "fips202.h"
#include "fips202x4.h"
#include <immintrin.h>
#else
#include "auxfunc.h"
#endif
#include "symmetric.h"
#include "params.h"
#include "endian.h"

#define BIT_XOF_DUMMY_NONCE0 ((uint16_t)0xFFFF)
#define BIT_XOF_DUMMY_NONCE1 ((uint16_t)0xFFFE)

static inline void secure_zero(void *ptr, size_t len) {
    volatile unsigned char *p = (volatile unsigned char *)ptr;
    while (len--) *p++ = 0;
}

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
    unsigned char buf[4096];
    size_t total = inlen0 + inlen1;
    if (total <= sizeof(buf)) {
        memcpy(buf, in0, inlen0);
        memcpy(buf + inlen0, in1, inlen1);
#if BIT_USE_SHAKE
        sha3_256(out, buf, total);
#else
        (void)sm3hash(256, buf, (unsigned long long)total * 8, out);
#endif
        secure_zero(buf, total);
    } else {
        unsigned char *msg = malloc(total);
        if (msg == NULL) return;
        memcpy(msg, in0, inlen0);
        memcpy(msg + inlen0, in1, inlen1);
#if BIT_USE_SHAKE
        sha3_256(out, msg, total);
#else
        (void)sm3hash(256, msg, (unsigned long long)total * 8, out);
#endif
        secure_zero(msg, total);
        free(msg);
    }
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
    unsigned char buf[4096];
    size_t total = inlen0 + inlen1;
    if (total <= sizeof(buf)) {
        memcpy(buf, in0, inlen0);
        memcpy(buf + inlen0, in1, inlen1);
#if BIT_USE_SHAKE
        sha3_512(out, buf, total);
#else
        (void)pseudohash(512, buf, (unsigned long long)total * 8, out);
#endif
        secure_zero(buf, total);
    } else {
        unsigned char *msg = malloc(total);
        if (msg == NULL) return;
        memcpy(msg, in0, inlen0);
        memcpy(msg + inlen0, in1, inlen1);
#if BIT_USE_SHAKE
        sha3_512(out, msg, total);
#else
        (void)pseudohash(512, msg, (unsigned long long)total * 8, out);
#endif
        secure_zero(msg, total);
        free(msg);
    }
}

void bit_xof256(uint8_t *out, size_t outlen, const uint8_t *in, size_t inlen)
{
#if BIT_USE_SHAKE
    shake256(out, outlen, in, inlen);
#else
    (void)pseudoXOF((unsigned long long)outlen * 8,
                    in, (unsigned long long)inlen * 8, out);
#endif
}

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

void bit_xof256_3(uint8_t *out, size_t outlen,
                  const uint8_t *in0, size_t inlen0,
                  const uint8_t *in1, size_t inlen1,
                  const uint8_t *in2, size_t inlen2)
{
#if BIT_USE_SHAKE
    keccak_state state;
    shake256_init(&state);
    shake256_absorb(&state, in0, inlen0);
    shake256_absorb(&state, in1, inlen1);
    shake256_absorb(&state, in2, inlen2);
    shake256_finalize(&state);
    shake256_squeeze(out, outlen, &state);
    secure_zero(&state, sizeof(state));
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

void bit_xof256_ctx_init(bit_xof256_ctx *ctx, const unsigned char *seed, size_t seed_len_bytes)
{
#if BIT_USE_SHAKE
    shake256_init(&ctx->state);
    shake256_absorb(&ctx->state, seed, seed_len_bytes);
    shake256_finalize(&ctx->state);
#else
    ctx->seed = seed;
    ctx->seed_len_bytes = seed_len_bytes;
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
    secure_zero(&ctx->state, sizeof(ctx->state));
#else
    (void)ctx;
#endif
}

void bit_xof256_init(bit_xof256_state *st, const unsigned char *seed, size_t seed_len_bytes, uint16_t nonce)
{
#if BIT_USE_SHAKE
    shake256_init(&st->state);
    unsigned char nonce_le[sizeof(uint16_t)];
    shake256_absorb(&st->state, seed, seed_len_bytes);
    store16_le(nonce_le, nonce);
    shake256_absorb(&st->state, nonce_le, sizeof(nonce_le));
    shake256_finalize(&st->state);
#else
    st->seed = seed;
    st->seed_len_bytes = seed_len_bytes;
    st->block = 0;
#endif
    st->nonce = nonce;
}

void bit_xof256_squeeze(bit_xof256_state *st, unsigned char *out, size_t out_len_bytes)
{
#if BIT_USE_SHAKE
    shake256_squeeze(out, out_len_bytes, &st->state);
#else
    uint8_t extseed[st->seed_len_bytes + 2 + 4];
    memcpy(extseed, st->seed, st->seed_len_bytes);
    store16_le(extseed + st->seed_len_bytes, st->nonce);
    store32_be(extseed + st->seed_len_bytes + 2, st->block);
    st->block++;
    (void)bit_xof256(out, out_len_bytes, extseed, st->seed_len_bytes + 2 + 4);
#endif
}

void bit_xof256_zeroize(bit_xof256_state *st)
{
#if BIT_USE_SHAKE
    secure_zero(&st->state, sizeof(st->state));
#else
    (void)st;
#endif
}

void bit_xof128_init(bit_xof128_state *st, const uint8_t *in, size_t inlen)
{
#if BIT_USE_SHAKE
    shake128_init(&st->state);
    shake128_absorb(&st->state, in, inlen);
    shake128_finalize(&st->state);
    st->leftover_len = 0;
    st->leftover_pos = 0;
#else
    st->seed = in;
    st->seed_len_bytes = inlen;
    st->block = 0;
#endif
}

void bit_xof128_squeeze(bit_xof128_state *st, unsigned char *out, size_t out_len_bytes)
{
#if BIT_USE_SHAKE
    shake128_squeeze(out, out_len_bytes, &st->state);
#else
    uint8_t extseed[st->seed_len_bytes + 2 + 4];
    memcpy(extseed, st->seed, st->seed_len_bytes);
    extseed[st->seed_len_bytes] = 0; extseed[st->seed_len_bytes + 1] = 0;
    store32_be(extseed + st->seed_len_bytes + 2, st->block);
    st->block++;
    (void)bit_xof256(out, out_len_bytes, extseed, st->seed_len_bytes + 2 + 4);
#endif
}

void bit_xof128_zeroize(bit_xof128_state *st)
{
#if BIT_USE_SHAKE
    secure_zero(&st->state, sizeof(st->state));
#else
    (void)st;
#endif
}

static void build_extseeds_x4(uint8_t extseeds[4][BIT_SEEDBYTES + sizeof(uint16_t)],
                              const uint8_t *seed, size_t seed_len_bytes,
                              uint16_t nonce0, uint16_t nonce1,
                              uint16_t nonce2, uint16_t nonce3)
{
    memcpy(extseeds[0], seed, seed_len_bytes);
    memcpy(extseeds[1], seed, seed_len_bytes);
    memcpy(extseeds[2], seed, seed_len_bytes);
    memcpy(extseeds[3], seed, seed_len_bytes);
    store16_le(&extseeds[0][seed_len_bytes], nonce0);
    store16_le(&extseeds[1][seed_len_bytes], nonce1);
    store16_le(&extseeds[2][seed_len_bytes], nonce2);
    store16_le(&extseeds[3][seed_len_bytes], nonce3);
}

void bit_xof256x4(unsigned char *out0, unsigned char *out1,
                     unsigned char *out2, unsigned char *out3,
                     size_t out_len_bytes,
                     const unsigned char *seed, size_t seed_len_bytes,
                     uint16_t nonce0, uint16_t nonce1,
                     uint16_t nonce2, uint16_t nonce3)
{
#if BIT_USE_SHAKE
    uint8_t extseeds[4][BIT_SEEDBYTES + sizeof(uint16_t)];
    build_extseeds_x4(extseeds, seed, seed_len_bytes,
                      nonce0, nonce1, nonce2, nonce3);
    shake256x4(out0, out1, out2, out3, out_len_bytes,
               extseeds[0], extseeds[1], extseeds[2], extseeds[3],
               seed_len_bytes + sizeof(uint16_t));
    secure_zero(extseeds, sizeof(extseeds));
#else
    (void)bit_xof256_nonce(out0, out_len_bytes, seed, seed_len_bytes, nonce0);
    (void)bit_xof256_nonce(out1, out_len_bytes, seed, seed_len_bytes, nonce1);
    (void)bit_xof256_nonce(out2, out_len_bytes, seed, seed_len_bytes, nonce2);
    (void)bit_xof256_nonce(out3, out_len_bytes, seed, seed_len_bytes, nonce3);
#endif
}

void bit_xof256x2(unsigned char *out0, unsigned char *out1,
                         size_t out_len_bytes,
                         const unsigned char *seed, size_t seed_len_bytes,
                         uint16_t nonce0, uint16_t nonce1)
{
#if BIT_USE_SHAKE
    uint8_t extseeds[4][BIT_SEEDBYTES + sizeof(uint16_t)];
    unsigned char *dummy_out2 = malloc(out_len_bytes);
    unsigned char *dummy_out3 = malloc(out_len_bytes);
    if (dummy_out2 == NULL || dummy_out3 == NULL) exit(111);

    memcpy(extseeds[0], seed, seed_len_bytes); store16_le(&extseeds[0][seed_len_bytes], nonce0);
    memcpy(extseeds[1], seed, seed_len_bytes); store16_le(&extseeds[1][seed_len_bytes], nonce1);
    memcpy(extseeds[2], seed, seed_len_bytes); store16_le(&extseeds[2][seed_len_bytes], BIT_XOF_DUMMY_NONCE0);
    memcpy(extseeds[3], seed, seed_len_bytes); store16_le(&extseeds[3][seed_len_bytes], BIT_XOF_DUMMY_NONCE1);

    shake256x4(out0, out1, dummy_out2, dummy_out3, out_len_bytes,
               extseeds[0], extseeds[1], extseeds[2], extseeds[3],
               seed_len_bytes + sizeof(uint16_t));
    secure_zero(extseeds, sizeof(extseeds));
    secure_zero(dummy_out2, out_len_bytes);
    secure_zero(dummy_out3, out_len_bytes);
    free(dummy_out2);
    free(dummy_out3);
#else
    (void)bit_xof256_nonce(out0, out_len_bytes, seed, seed_len_bytes, nonce0);
    (void)bit_xof256_nonce(out1, out_len_bytes, seed, seed_len_bytes, nonce1);
#endif
}

static void bit_xof256x3(unsigned char *out0, unsigned char *out1,
                         unsigned char *out2,
                         size_t out_len_bytes,
                         const unsigned char *seed, size_t seed_len_bytes,
                         uint16_t nonce0, uint16_t nonce1, uint16_t nonce2)
{
#if BIT_USE_SHAKE
    uint8_t extseeds[4][BIT_SEEDBYTES + sizeof(uint16_t)];
    unsigned char *dummy_out3 = malloc(out_len_bytes);
    if (dummy_out3 == NULL) exit(111);

    memcpy(extseeds[0], seed, seed_len_bytes); store16_le(&extseeds[0][seed_len_bytes], nonce0);
    memcpy(extseeds[1], seed, seed_len_bytes); store16_le(&extseeds[1][seed_len_bytes], nonce1);
    memcpy(extseeds[2], seed, seed_len_bytes); store16_le(&extseeds[2][seed_len_bytes], nonce2);
    memcpy(extseeds[3], seed, seed_len_bytes); store16_le(&extseeds[3][seed_len_bytes], BIT_XOF_DUMMY_NONCE0);

    shake256x4(out0, out1, out2, dummy_out3, out_len_bytes,
               extseeds[0], extseeds[1], extseeds[2], extseeds[3],
               seed_len_bytes + sizeof(uint16_t));
    secure_zero(extseeds, sizeof(extseeds));
    secure_zero(dummy_out3, out_len_bytes);
    free(dummy_out3);
#else
    (void)bit_xof256_nonce(out0, out_len_bytes, seed, seed_len_bytes, nonce0);
    (void)bit_xof256_nonce(out1, out_len_bytes, seed, seed_len_bytes, nonce1);
    (void)bit_xof256_nonce(out2, out_len_bytes, seed, seed_len_bytes, nonce2);
#endif
}

void bit_xof256_xn(unsigned char *const out[], size_t out_len_bytes,
                   const unsigned char *seed, size_t seed_len_bytes,
                   const uint16_t *nonce, size_t count)
{
    size_t i = 0;

    for (; i + 4 <= count; i += 4) {
        bit_xof256x4(out[i], out[i + 1], out[i + 2], out[i + 3], out_len_bytes,
                     seed, seed_len_bytes,
                     nonce[i], nonce[i + 1], nonce[i + 2], nonce[i + 3]);
    }

    switch (count - i) {
        case 0: break;
        case 1:
            bit_xof256_nonce(out[i], out_len_bytes, seed, seed_len_bytes, nonce[i]);
            break;
        case 2:
            bit_xof256x2(out[i], out[i + 1], out_len_bytes,
                         seed, seed_len_bytes, nonce[i], nonce[i + 1]);
            break;
        case 3:
            bit_xof256x3(out[i], out[i + 1], out[i + 2], out_len_bytes,
                         seed, seed_len_bytes,
                         nonce[i], nonce[i + 1], nonce[i + 2]);
            break;
        default: break;
    }
}

void bit_xof128x4_init(bit_xof128x4_state *st,
                       const unsigned char *in0, const unsigned char *in1,
                       const unsigned char *in2, const unsigned char *in3,
                       size_t inlen)
{
#if BIT_USE_SHAKE
    shake128x4_absorb_once(&st->state, in0, in1, in2, in3, inlen);
#else
    st->seed          = in0;
    st->seed_len_bytes = inlen;
    st->block0 = 0; st->block1 = 0; st->block2 = 0; st->block3 = 0;
    (void)in1; (void)in2; (void)in3;
#endif
}

void bit_xof128x4_squeezeblocks(bit_xof128x4_state *st,
                                uint8_t *out0, uint8_t *out1,
                                uint8_t *out2, uint8_t *out3,
                                size_t nblocks)
{
#if BIT_USE_SHAKE
    shake128x4_squeezeblocks(out0, out1, out2, out3, nblocks, &st->state);
#else
    (void)out0; (void)out1; (void)out2; (void)out3;
    (void)nblocks; (void)st;
#endif
}
