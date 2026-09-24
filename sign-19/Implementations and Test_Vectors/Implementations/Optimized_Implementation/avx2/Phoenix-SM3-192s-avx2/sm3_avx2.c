#include "sm3_internal.h"

#include <immintrin.h>
#include <string.h>

#define SM3_IV0 0x7380166fu
#define SM3_IV1 0x4914b2b9u
#define SM3_IV2 0x172442d7u
#define SM3_IV3 0xda8a0600u
#define SM3_IV4 0xa96f30bcu
#define SM3_IV5 0x163138aau
#define SM3_IV6 0xe38dee4du
#define SM3_IV7 0xb0fb0e4eu

#define ROTL32X8(x, n) \
    _mm256_or_si256(_mm256_slli_epi32((x), (n)), \
                    _mm256_srli_epi32((x), 32 - (n)))
#define P0X8(x) _mm256_xor_si256(_mm256_xor_si256((x), ROTL32X8((x), 9)), \
                                  ROTL32X8((x), 17))
#define P1X8(x) _mm256_xor_si256(_mm256_xor_si256((x), ROTL32X8((x), 15)), \
                                  ROTL32X8((x), 23))
#define XOR3X8(a, b, c) _mm256_xor_si256(_mm256_xor_si256((a), (b)), (c))

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

static __m256i load8_be32(const uint8_t *blocks[8], unsigned int word) {
    return _mm256_set_epi32(
        (int)load_be32(blocks[7] + 4 * word),
        (int)load_be32(blocks[6] + 4 * word),
        (int)load_be32(blocks[5] + 4 * word),
        (int)load_be32(blocks[4] + 4 * word),
        (int)load_be32(blocks[3] + 4 * word),
        (int)load_be32(blocks[2] + 4 * word),
        (int)load_be32(blocks[1] + 4 * word),
        (int)load_be32(blocks[0] + 4 * word));
}

static __m256i ff1x8(__m256i x, __m256i y, __m256i z) {
    return _mm256_or_si256(
        _mm256_or_si256(_mm256_and_si256(x, y), _mm256_and_si256(x, z)),
        _mm256_and_si256(y, z));
}

static __m256i gg1x8(__m256i x, __m256i y, __m256i z) {
    return _mm256_or_si256(_mm256_and_si256(x, y), _mm256_andnot_si256(x, z));
}

static void sm3_compress8(__m256i state[8], const uint8_t *blocks[8]) {
    __m256i w[68];
    __m256i wp[64];

    for (unsigned int j = 0; j < 16; ++j) {
        w[j] = load8_be32(blocks, j);
    }
    for (unsigned int j = 16; j < 68; ++j) {
        w[j] = _mm256_xor_si256(
            _mm256_xor_si256(P1X8(_mm256_xor_si256(
                                  _mm256_xor_si256(w[j - 16], w[j - 9]),
                                  ROTL32X8(w[j - 3], 15))),
                             ROTL32X8(w[j - 13], 7)),
            w[j - 6]);
    }
    for (unsigned int j = 0; j < 64; ++j) {
        wp[j] = _mm256_xor_si256(w[j], w[j + 4]);
    }

    __m256i a = state[0];
    __m256i b = state[1];
    __m256i c = state[2];
    __m256i d = state[3];
    __m256i e = state[4];
    __m256i f = state[5];
    __m256i g = state[6];
    __m256i h = state[7];

    for (unsigned int j = 0; j < 16; ++j) {
        __m256i a12 = ROTL32X8(a, 12);
        __m256i ss1 = ROTL32X8(_mm256_add_epi32(
                                   _mm256_add_epi32(a12, e),
                                   _mm256_set1_epi32((int)sm3_tj[j])), 7);
        __m256i ss2 = _mm256_xor_si256(ss1, a12);
        __m256i tt1 = _mm256_add_epi32(
            _mm256_add_epi32(_mm256_add_epi32(XOR3X8(a, b, c), d), ss2),
            wp[j]);
        __m256i tt2 = _mm256_add_epi32(
            _mm256_add_epi32(_mm256_add_epi32(XOR3X8(e, f, g), h), ss1),
            w[j]);
        d = c;
        c = ROTL32X8(b, 9);
        b = a;
        a = tt1;
        h = g;
        g = ROTL32X8(f, 19);
        f = e;
        e = P0X8(tt2);
    }

    for (unsigned int j = 16; j < 64; ++j) {
        __m256i a12 = ROTL32X8(a, 12);
        __m256i ss1 = ROTL32X8(_mm256_add_epi32(
                                   _mm256_add_epi32(a12, e),
                                   _mm256_set1_epi32((int)sm3_tj[j])), 7);
        __m256i ss2 = _mm256_xor_si256(ss1, a12);
        __m256i tt1 = _mm256_add_epi32(
            _mm256_add_epi32(_mm256_add_epi32(ff1x8(a, b, c), d), ss2),
            wp[j]);
        __m256i tt2 = _mm256_add_epi32(
            _mm256_add_epi32(_mm256_add_epi32(gg1x8(e, f, g), h), ss1),
            w[j]);
        d = c;
        c = ROTL32X8(b, 9);
        b = a;
        a = tt1;
        h = g;
        g = ROTL32X8(f, 19);
        f = e;
        e = P0X8(tt2);
    }

    state[0] = _mm256_xor_si256(state[0], a);
    state[1] = _mm256_xor_si256(state[1], b);
    state[2] = _mm256_xor_si256(state[2], c);
    state[3] = _mm256_xor_si256(state[3], d);
    state[4] = _mm256_xor_si256(state[4], e);
    state[5] = _mm256_xor_si256(state[5], f);
    state[6] = _mm256_xor_si256(state[6], g);
    state[7] = _mm256_xor_si256(state[7], h);
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

static void store_digest8(uint8_t *outs[8], const __m256i state[8]) {
    uint32_t lanes[8];

    for (unsigned int word = 0; word < 8; ++word) {
        _mm256_storeu_si256((__m256i *)(void *)lanes, state[word]);
        for (unsigned int lane = 0; lane < 8; ++lane) {
            store_be32(outs[lane] + 4 * word, lanes[lane]);
        }
    }
}

void sm3x8_avx2(uint8_t out0[SM3_DIGEST_BYTES],
                uint8_t out1[SM3_DIGEST_BYTES],
                uint8_t out2[SM3_DIGEST_BYTES],
                uint8_t out3[SM3_DIGEST_BYTES],
                uint8_t out4[SM3_DIGEST_BYTES],
                uint8_t out5[SM3_DIGEST_BYTES],
                uint8_t out6[SM3_DIGEST_BYTES],
                uint8_t out7[SM3_DIGEST_BYTES],
                const uint8_t *in0,
                const uint8_t *in1,
                const uint8_t *in2,
                const uint8_t *in3,
                const uint8_t *in4,
                const uint8_t *in5,
                const uint8_t *in6,
                const uint8_t *in7,
                size_t inlen) {
    const uint8_t *ins[8] = {in0, in1, in2, in3, in4, in5, in6, in7};
    uint8_t *outs[8] = {out0, out1, out2, out3, out4, out5, out6, out7};
    uint8_t pads[8][2 * SM3_BLOCK_BYTES];
    __m256i state[8] = {
        _mm256_set1_epi32((int)SM3_IV0), _mm256_set1_epi32((int)SM3_IV1),
        _mm256_set1_epi32((int)SM3_IV2), _mm256_set1_epi32((int)SM3_IV3),
        _mm256_set1_epi32((int)SM3_IV4), _mm256_set1_epi32((int)SM3_IV5),
        _mm256_set1_epi32((int)SM3_IV6), _mm256_set1_epi32((int)SM3_IV7),
    };
    size_t full_len = inlen & ~(size_t)(SM3_BLOCK_BYTES - 1);
    size_t tail_len = inlen - full_len;

    for (size_t offset = 0; offset < full_len; offset += SM3_BLOCK_BYTES) {
        const uint8_t *blocks[8] = {
            ins[0] + offset, ins[1] + offset, ins[2] + offset, ins[3] + offset,
            ins[4] + offset, ins[5] + offset, ins[6] + offset, ins[7] + offset,
        };
        sm3_compress8(state, blocks);
    }

    size_t blocks = 0;
    for (unsigned int lane = 0; lane < 8; ++lane) {
        const uint8_t *tail = tail_len ? ins[lane] + full_len : NULL;
        size_t lane_blocks = sm3_final_blocks(pads[lane], tail, tail_len, inlen);
        if (lane == 0) {
            blocks = lane_blocks;
        }
    }

    for (size_t i = 0; i < blocks; ++i) {
        const uint8_t *pblocks[8] = {
            pads[0] + i * SM3_BLOCK_BYTES,
            pads[1] + i * SM3_BLOCK_BYTES,
            pads[2] + i * SM3_BLOCK_BYTES,
            pads[3] + i * SM3_BLOCK_BYTES,
            pads[4] + i * SM3_BLOCK_BYTES,
            pads[5] + i * SM3_BLOCK_BYTES,
            pads[6] + i * SM3_BLOCK_BYTES,
            pads[7] + i * SM3_BLOCK_BYTES,
        };
        sm3_compress8(state, pblocks);
    }

    store_digest8(outs, state);
}
