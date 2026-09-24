#include "sm3_internal.h"

#include <arm_neon.h>
#include <string.h>

#define SM3_IV0 0x7380166fu
#define SM3_IV1 0x4914b2b9u
#define SM3_IV2 0x172442d7u
#define SM3_IV3 0xda8a0600u
#define SM3_IV4 0xa96f30bcu
#define SM3_IV5 0x163138aau
#define SM3_IV6 0xe38dee4du
#define SM3_IV7 0xb0fb0e4eu

#define ROTL32X4(x, n) \
    vorrq_u32(vshlq_n_u32((x), (n)), vshrq_n_u32((x), 32 - (n)))
#define P0X4(x) veorq_u32(veorq_u32((x), ROTL32X4((x), 9)), ROTL32X4((x), 17))
#define P1X4(x) veorq_u32(veorq_u32((x), ROTL32X4((x), 15)), ROTL32X4((x), 23))
#define XOR3X4(a, b, c) veorq_u32(veorq_u32((a), (b)), (c))

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

static uint32x4_t load4_be32(const uint8_t *in0, const uint8_t *in1,
                             const uint8_t *in2, const uint8_t *in3,
                             unsigned int word) {
    uint32_t lanes[4] = {
        load_be32(in0 + 4 * word),
        load_be32(in1 + 4 * word),
        load_be32(in2 + 4 * word),
        load_be32(in3 + 4 * word),
    };

    return vld1q_u32(lanes);
}

static uint32x4_t ff1x4(uint32x4_t x, uint32x4_t y, uint32x4_t z) {
    return vorrq_u32(vorrq_u32(vandq_u32(x, y), vandq_u32(x, z)),
                     vandq_u32(y, z));
}

static uint32x4_t gg1x4(uint32x4_t x, uint32x4_t y, uint32x4_t z) {
    return vorrq_u32(vandq_u32(x, y), vbicq_u32(z, x));
}

static void sm3_compress4(uint32x4_t state[8],
                          const uint8_t *in0, const uint8_t *in1,
                          const uint8_t *in2, const uint8_t *in3) {
    uint32x4_t w[68];
    uint32x4_t wp[64];

    for (unsigned int j = 0; j < 16; ++j) {
        w[j] = load4_be32(in0, in1, in2, in3, j);
    }
    for (unsigned int j = 16; j < 68; ++j) {
        w[j] = veorq_u32(
            veorq_u32(P1X4(veorq_u32(veorq_u32(w[j - 16], w[j - 9]),
                                     ROTL32X4(w[j - 3], 15))),
                      ROTL32X4(w[j - 13], 7)),
            w[j - 6]);
    }
    for (unsigned int j = 0; j < 64; ++j) {
        wp[j] = veorq_u32(w[j], w[j + 4]);
    }

    uint32x4_t a = state[0];
    uint32x4_t b = state[1];
    uint32x4_t c = state[2];
    uint32x4_t d = state[3];
    uint32x4_t e = state[4];
    uint32x4_t f = state[5];
    uint32x4_t g = state[6];
    uint32x4_t h = state[7];

    for (unsigned int j = 0; j < 16; ++j) {
        uint32x4_t a12 = ROTL32X4(a, 12);
        uint32x4_t ss1 = ROTL32X4(vaddq_u32(vaddq_u32(a12, e),
                                             vdupq_n_u32(sm3_tj[j])), 7);
        uint32x4_t ss2 = veorq_u32(ss1, a12);
        uint32x4_t tt1 = vaddq_u32(vaddq_u32(vaddq_u32(XOR3X4(a, b, c), d),
                                             ss2), wp[j]);
        uint32x4_t tt2 = vaddq_u32(vaddq_u32(vaddq_u32(XOR3X4(e, f, g), h),
                                             ss1), w[j]);
        d = c;
        c = ROTL32X4(b, 9);
        b = a;
        a = tt1;
        h = g;
        g = ROTL32X4(f, 19);
        f = e;
        e = P0X4(tt2);
    }

    for (unsigned int j = 16; j < 64; ++j) {
        uint32x4_t a12 = ROTL32X4(a, 12);
        uint32x4_t ss1 = ROTL32X4(vaddq_u32(vaddq_u32(a12, e),
                                             vdupq_n_u32(sm3_tj[j])), 7);
        uint32x4_t ss2 = veorq_u32(ss1, a12);
        uint32x4_t tt1 = vaddq_u32(vaddq_u32(vaddq_u32(ff1x4(a, b, c), d),
                                             ss2), wp[j]);
        uint32x4_t tt2 = vaddq_u32(vaddq_u32(vaddq_u32(gg1x4(e, f, g), h),
                                             ss1), w[j]);
        d = c;
        c = ROTL32X4(b, 9);
        b = a;
        a = tt1;
        h = g;
        g = ROTL32X4(f, 19);
        f = e;
        e = P0X4(tt2);
    }

    state[0] = veorq_u32(state[0], a);
    state[1] = veorq_u32(state[1], b);
    state[2] = veorq_u32(state[2], c);
    state[3] = veorq_u32(state[3], d);
    state[4] = veorq_u32(state[4], e);
    state[5] = veorq_u32(state[5], f);
    state[6] = veorq_u32(state[6], g);
    state[7] = veorq_u32(state[7], h);
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

static void store_digest4(uint8_t out0[SM3_DIGEST_BYTES],
                          uint8_t out1[SM3_DIGEST_BYTES],
                          uint8_t out2[SM3_DIGEST_BYTES],
                          uint8_t out3[SM3_DIGEST_BYTES],
                          const uint32x4_t state[8]) {
    uint32_t lanes[4];

    for (unsigned int word = 0; word < 8; ++word) {
        vst1q_u32(lanes, state[word]);
        store_be32(out0 + 4 * word, lanes[0]);
        store_be32(out1 + 4 * word, lanes[1]);
        store_be32(out2 + 4 * word, lanes[2]);
        store_be32(out3 + 4 * word, lanes[3]);
    }
}

void sm3x4_neon(uint8_t out0[SM3_DIGEST_BYTES],
                uint8_t out1[SM3_DIGEST_BYTES],
                uint8_t out2[SM3_DIGEST_BYTES],
                uint8_t out3[SM3_DIGEST_BYTES],
                const uint8_t *in0,
                const uint8_t *in1,
                const uint8_t *in2,
                const uint8_t *in3,
                size_t inlen) {
    uint32x4_t state[8] = {
        vdupq_n_u32(SM3_IV0), vdupq_n_u32(SM3_IV1),
        vdupq_n_u32(SM3_IV2), vdupq_n_u32(SM3_IV3),
        vdupq_n_u32(SM3_IV4), vdupq_n_u32(SM3_IV5),
        vdupq_n_u32(SM3_IV6), vdupq_n_u32(SM3_IV7),
    };
    uint8_t pad0[2 * SM3_BLOCK_BYTES], pad1[2 * SM3_BLOCK_BYTES];
    uint8_t pad2[2 * SM3_BLOCK_BYTES], pad3[2 * SM3_BLOCK_BYTES];
    size_t full_len = inlen & ~(size_t)(SM3_BLOCK_BYTES - 1);
    size_t tail_len = inlen - full_len;

    for (size_t offset = 0; offset < full_len; offset += SM3_BLOCK_BYTES) {
        sm3_compress4(state, in0 + offset, in1 + offset,
                      in2 + offset, in3 + offset);
    }

    const uint8_t *tail0 = tail_len ? in0 + full_len : NULL;
    const uint8_t *tail1 = tail_len ? in1 + full_len : NULL;
    const uint8_t *tail2 = tail_len ? in2 + full_len : NULL;
    const uint8_t *tail3 = tail_len ? in3 + full_len : NULL;
    size_t blocks = sm3_final_blocks(pad0, tail0, tail_len, inlen);
    (void)sm3_final_blocks(pad1, tail1, tail_len, inlen);
    (void)sm3_final_blocks(pad2, tail2, tail_len, inlen);
    (void)sm3_final_blocks(pad3, tail3, tail_len, inlen);

    for (size_t i = 0; i < blocks; ++i) {
        sm3_compress4(state,
                      pad0 + i * SM3_BLOCK_BYTES,
                      pad1 + i * SM3_BLOCK_BYTES,
                      pad2 + i * SM3_BLOCK_BYTES,
                      pad3 + i * SM3_BLOCK_BYTES);
    }

    store_digest4(out0, out1, out2, out3, state);
}
