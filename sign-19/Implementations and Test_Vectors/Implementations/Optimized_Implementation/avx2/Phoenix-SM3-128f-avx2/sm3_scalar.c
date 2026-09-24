#include "sm3.h"

#include <string.h>

#define SM3_IV0 0x7380166fu
#define SM3_IV1 0x4914b2b9u
#define SM3_IV2 0x172442d7u
#define SM3_IV3 0xda8a0600u
#define SM3_IV4 0xa96f30bcu
#define SM3_IV5 0x163138aau
#define SM3_IV6 0xe38dee4du
#define SM3_IV7 0xb0fb0e4eu

static uint32_t load_be32(const uint8_t *x) {
    return ((uint32_t)x[0] << 24) |
           ((uint32_t)x[1] << 16) |
           ((uint32_t)x[2] << 8) |
           (uint32_t)x[3];
}

static void store_be32(uint8_t *x, uint32_t u) {
    x[0] = (uint8_t)(u >> 24);
    x[1] = (uint8_t)(u >> 16);
    x[2] = (uint8_t)(u >> 8);
    x[3] = (uint8_t)u;
}

static void store_be64(uint8_t *x, uint64_t u) {
    x[0] = (uint8_t)(u >> 56);
    x[1] = (uint8_t)(u >> 48);
    x[2] = (uint8_t)(u >> 40);
    x[3] = (uint8_t)(u >> 32);
    x[4] = (uint8_t)(u >> 24);
    x[5] = (uint8_t)(u >> 16);
    x[6] = (uint8_t)(u >> 8);
    x[7] = (uint8_t)u;
}

static uint32_t rotl32(uint32_t x, unsigned int n) {
    return (x << n) | (x >> (32 - n));
}

static uint32_t p0(uint32_t x) {
    return x ^ rotl32(x, 9) ^ rotl32(x, 17);
}

static uint32_t p1(uint32_t x) {
    return x ^ rotl32(x, 15) ^ rotl32(x, 23);
}

static uint32_t ff0(uint32_t x, uint32_t y, uint32_t z) {
    return x ^ y ^ z;
}

static uint32_t ff1(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) | (x & z) | (y & z);
}

static uint32_t gg0(uint32_t x, uint32_t y, uint32_t z) {
    return x ^ y ^ z;
}

static uint32_t gg1(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) | (~x & z);
}

static const uint32_t sm3_tj[64] = {
    0x79cc4519u, 0xf3988a32u, 0xe7311465u, 0xce6228cbu,
    0x9cc45197u, 0x3988a32fu, 0x7311465eu, 0xe6228cbcu,
    0xcc451979u, 0x988a32f3u, 0x311465e7u, 0x6228cbceu,
    0xc451979cu, 0x88a32f39u, 0x11465e73u, 0x228cbce6u,
    0x9d8a7a87u, 0x3b14f50fu, 0x7629ea1eu, 0xec53d43cu,
    0xd8a7a879u, 0xb14f50f3u, 0x629ea1e7u, 0xc53d43ceu,
    0x8a7a879du, 0x14f50f3bu, 0x29ea1e76u, 0x53d43cecu,
    0xa7a879d8u, 0x4f50f3b1u, 0x9ea1e762u, 0x3d43cec5u,
    0x7a879d8au, 0xf50f3b14u, 0xea1e7629u, 0xd43cec53u,
    0xa879d8a7u, 0x50f3b14fu, 0xa1e7629eu, 0x43cec53du,
    0x879d8a7au, 0x0f3b14f5u, 0x1e7629eau, 0x3cec53d4u,
    0x79d8a7a8u, 0xf3b14f50u, 0xe7629ea1u, 0xcec53d43u,
    0x9d8a7a87u, 0x3b14f50fu, 0x7629ea1eu, 0xec53d43cu,
    0xd8a7a879u, 0xb14f50f3u, 0x629ea1e7u, 0xc53d43ceu,
    0x8a7a879du, 0x14f50f3bu, 0x29ea1e76u, 0x53d43cecu,
    0xa7a879d8u, 0x4f50f3b1u, 0x9ea1e762u, 0x3d43cec5u,
};

static void sm3_compress(uint32_t state[8], const uint8_t block[SM3_BLOCK_BYTES]) {
    uint32_t w[68];
    uint32_t wp[64];

    for (unsigned int j = 0; j < 16; ++j) {
        w[j] = load_be32(block + 4 * j);
    }
    for (unsigned int j = 16; j < 68; ++j) {
        w[j] = p1(w[j - 16] ^ w[j - 9] ^ rotl32(w[j - 3], 15)) ^
               rotl32(w[j - 13], 7) ^ w[j - 6];
    }
    for (unsigned int j = 0; j < 64; ++j) {
        wp[j] = w[j] ^ w[j + 4];
    }

    uint32_t a = state[0];
    uint32_t b = state[1];
    uint32_t c = state[2];
    uint32_t d = state[3];
    uint32_t e = state[4];
    uint32_t f = state[5];
    uint32_t g = state[6];
    uint32_t h = state[7];

    for (unsigned int j = 0; j < 16; ++j) {
        uint32_t a12 = rotl32(a, 12);
        uint32_t ss1 = rotl32(a12 + e + sm3_tj[j], 7);
        uint32_t ss2 = ss1 ^ a12;
        uint32_t tt1 = ff0(a, b, c) + d + ss2 + wp[j];
        uint32_t tt2 = gg0(e, f, g) + h + ss1 + w[j];
        d = c;
        c = rotl32(b, 9);
        b = a;
        a = tt1;
        h = g;
        g = rotl32(f, 19);
        f = e;
        e = p0(tt2);
    }

    for (unsigned int j = 16; j < 64; ++j) {
        uint32_t a12 = rotl32(a, 12);
        uint32_t ss1 = rotl32(a12 + e + sm3_tj[j], 7);
        uint32_t ss2 = ss1 ^ a12;
        uint32_t tt1 = ff1(a, b, c) + d + ss2 + wp[j];
        uint32_t tt2 = gg1(e, f, g) + h + ss1 + w[j];
        d = c;
        c = rotl32(b, 9);
        b = a;
        a = tt1;
        h = g;
        g = rotl32(f, 19);
        f = e;
        e = p0(tt2);
    }

    state[0] ^= a;
    state[1] ^= b;
    state[2] ^= c;
    state[3] ^= d;
    state[4] ^= e;
    state[5] ^= f;
    state[6] ^= g;
    state[7] ^= h;
}

static size_t sm3_final_blocks(uint8_t padded[2 * SM3_BLOCK_BYTES],
                               const uint8_t *tail, size_t tail_len,
                               uint64_t total_len) {
    const size_t len_offset = (tail_len < 56) ? 56 : 120;

    if (tail_len > 0) {
        memcpy(padded, tail, tail_len);
    }
    padded[tail_len] = 0x80;
    memset(padded + tail_len + 1, 0, len_offset - tail_len - 1);
    store_be64(padded + len_offset, total_len << 3);

    return (tail_len < 56) ? 1 : 2;
}

void sm3(uint8_t out[SM3_DIGEST_BYTES], const uint8_t *in, size_t inlen) {
    uint32_t state[8] = {
        SM3_IV0, SM3_IV1, SM3_IV2, SM3_IV3,
        SM3_IV4, SM3_IV5, SM3_IV6, SM3_IV7,
    };
    uint8_t padded[2 * SM3_BLOCK_BYTES];
    size_t full_len = inlen & ~(size_t)(SM3_BLOCK_BYTES - 1);
    size_t tail_len = inlen - full_len;

    for (size_t offset = 0; offset < full_len; offset += SM3_BLOCK_BYTES) {
        sm3_compress(state, in + offset);
    }

    const uint8_t *tail = tail_len ? in + full_len : NULL;
    size_t blocks = sm3_final_blocks(padded, tail, tail_len, inlen);
    for (size_t i = 0; i < blocks; ++i) {
        sm3_compress(state, padded + i * SM3_BLOCK_BYTES);
    }

    for (unsigned int i = 0; i < 8; ++i) {
        store_be32(out + 4 * i, state[i]);
    }
}
