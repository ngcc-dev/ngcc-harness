#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "utils.h"
#include "auxfunc.h"
#include "hash_sm3.h"

/* SM3 (GM/T 0004-2012) hash function implementation.
   Block size: 64 bytes (512 bits), Output: 32 bytes (256 bits). */

static uint32_t load_bigendian_32(const uint8_t *x) {
    return (uint32_t)(x[0]) << 24 | (uint32_t)(x[1]) << 16 |
           (uint32_t)(x[2]) << 8  | (uint32_t)(x[3]);
}

static uint64_t load_bigendian_64(const uint8_t *x) {
    return (uint64_t)(x[0]) << 56 | (uint64_t)(x[1]) << 48 |
           (uint64_t)(x[2]) << 40 | (uint64_t)(x[3]) << 32 |
           (uint64_t)(x[4]) << 24 | (uint64_t)(x[5]) << 16 |
           (uint64_t)(x[6]) << 8  | (uint64_t)(x[7]);
}

static void store_bigendian_32(uint8_t *x, uint32_t u) {
    x[0] = (uint8_t)(u >> 24);
    x[1] = (uint8_t)(u >> 16);
    x[2] = (uint8_t)(u >> 8);
    x[3] = (uint8_t)u;
}

static void store_bigendian_64(uint8_t *x, uint64_t u) {
    x[0] = (uint8_t)(u >> 56);
    x[1] = (uint8_t)(u >> 48);
    x[2] = (uint8_t)(u >> 40);
    x[3] = (uint8_t)(u >> 32);
    x[4] = (uint8_t)(u >> 24);
    x[5] = (uint8_t)(u >> 16);
    x[6] = (uint8_t)(u >> 8);
    x[7] = (uint8_t)u;
}

#define ROTL32(x, n) (((x) << ((n) & 31)) | ((x) >> (32 - ((n) & 31))))

#define P0(x) ((x) ^ ROTL32(x, 9) ^ ROTL32(x, 17))
#define P1(x) ((x) ^ ROTL32(x, 15) ^ ROTL32(x, 23))

#define FF0(x, y, z) ((x) ^ (y) ^ (z))
#define FF1(x, y, z) (((x) & (y)) | ((x) & (z)) | ((y) & (z)))
#define GG0(x, y, z) ((x) ^ (y) ^ (z))
#define GG1(x, y, z) (((x) & (y)) | (~(x) & (z)))

static const uint8_t iv_sm3[32] = {
    0x73, 0x80, 0x16, 0x6f, 0x49, 0x14, 0xb2, 0xb9,
    0x17, 0x24, 0x42, 0xd7, 0xda, 0x8a, 0x06, 0x00,
    0xa9, 0x6f, 0x30, 0xbc, 0x16, 0x31, 0x38, 0xaa,
    0xe3, 0x8d, 0xee, 0x4d, 0xb0, 0xfb, 0x0e, 0x4e
};

/* Pre-computed SM3 round constants T_j = ROTL32(c, j) */
static const uint32_t sm3_Tj[64] = {
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
    0xa7a879d8u, 0x4f50f3b1u, 0x9ea1e762u, 0x3d43cec5u
};

static size_t crypto_hashblocks_sm3(uint8_t *statebytes,
                                    const uint8_t *in, size_t inlen) {
    uint32_t A, B, C, D, E, F, G, H;
    uint32_t SS1, SS2, TT1, TT2;
    uint32_t W[68], Wp[64];
    unsigned int j;

    A = load_bigendian_32(statebytes + 0);
    B = load_bigendian_32(statebytes + 4);
    C = load_bigendian_32(statebytes + 8);
    D = load_bigendian_32(statebytes + 12);
    E = load_bigendian_32(statebytes + 16);
    F = load_bigendian_32(statebytes + 20);
    G = load_bigendian_32(statebytes + 24);
    H = load_bigendian_32(statebytes + 28);

    while (inlen >= 64) {
        /* Message expansion */
        for (j = 0; j < 16; j++) {
            W[j] = load_bigendian_32(in + j * 4);
        }
        for (j = 16; j < 68; j++) {
            W[j] = P1(W[j - 16] ^ W[j - 9] ^ ROTL32(W[j - 3], 15))
                   ^ ROTL32(W[j - 13], 7) ^ W[j - 6];
        }
        for (j = 0; j < 64; j++) {
            Wp[j] = W[j] ^ W[j + 4];
        }

        uint32_t a = A, b = B, c = C, d = D, e = E, f = F, g = G, h = H;

        /* 64 rounds: 0..15 */
        for (j = 0; j < 16; j++) {
            uint32_t a12 = ROTL32(a, 12);
            SS1 = ROTL32(a12 + e + sm3_Tj[j], 7);
            SS2 = SS1 ^ a12;
            TT1 = FF0(a, b, c) + d + SS2 + Wp[j];
            TT2 = GG0(e, f, g) + h + SS1 + W[j];
            d = c;
            c = ROTL32(b, 9);
            b = a;
            a = TT1;
            h = g;
            g = ROTL32(f, 19);
            f = e;
            e = P0(TT2);
        }
        /* 64 rounds: 16..63 */
        for (j = 16; j < 64; j++) {
            uint32_t a12 = ROTL32(a, 12);
            SS1 = ROTL32(a12 + e + sm3_Tj[j], 7);
            SS2 = SS1 ^ a12;
            TT1 = FF1(a, b, c) + d + SS2 + Wp[j];
            TT2 = GG1(e, f, g) + h + SS1 + W[j];
            d = c;
            c = ROTL32(b, 9);
            b = a;
            a = TT1;
            h = g;
            g = ROTL32(f, 19);
            f = e;
            e = P0(TT2);
        }

        A ^= a;
        B ^= b;
        C ^= c;
        D ^= d;
        E ^= e;
        F ^= f;
        G ^= g;
        H ^= h;

        in += 64;
        inlen -= 64;
    }

    store_bigendian_32(statebytes + 0, A);
    store_bigendian_32(statebytes + 4, B);
    store_bigendian_32(statebytes + 8, C);
    store_bigendian_32(statebytes + 12, D);
    store_bigendian_32(statebytes + 16, E);
    store_bigendian_32(statebytes + 20, F);
    store_bigendian_32(statebytes + 24, G);
    store_bigendian_32(statebytes + 28, H);

    return inlen;
}

void sm3_inc_init(uint8_t *state) {
    memcpy(state, iv_sm3, 32);
    memset(state + 32, 0, 8);
}

void sm3_inc_blocks(uint8_t *state, const uint8_t *in, size_t inblocks) {
    uint64_t bytes = load_bigendian_64(state + 32);
    crypto_hashblocks_sm3(state, in, 64 * inblocks);
    bytes += 64 * inblocks;
    store_bigendian_64(state + 32, bytes);
}

void sm3_inc_finalize(uint8_t *out, uint8_t *state, const uint8_t *in, size_t inlen) {
    uint8_t padded[128];
    uint64_t bytes = load_bigendian_64(state + 32) + inlen;

    crypto_hashblocks_sm3(state, in, inlen);
    in += inlen;
    inlen &= 63;
    in -= inlen;

    memcpy(padded, in, inlen);
    padded[inlen] = 0x80;

    if (inlen < 56) {
        memset(padded + inlen + 1, 0, 56 - (inlen + 1));
        padded[56] = (uint8_t)(bytes >> 53);
        padded[57] = (uint8_t)(bytes >> 45);
        padded[58] = (uint8_t)(bytes >> 37);
        padded[59] = (uint8_t)(bytes >> 29);
        padded[60] = (uint8_t)(bytes >> 21);
        padded[61] = (uint8_t)(bytes >> 13);
        padded[62] = (uint8_t)(bytes >> 5);
        padded[63] = (uint8_t)(bytes << 3);
        crypto_hashblocks_sm3(state, padded, 64);
    } else {
        memset(padded + inlen + 1, 0, 120 - (inlen + 1));
        padded[120] = (uint8_t)(bytes >> 53);
        padded[121] = (uint8_t)(bytes >> 45);
        padded[122] = (uint8_t)(bytes >> 37);
        padded[123] = (uint8_t)(bytes >> 29);
        padded[124] = (uint8_t)(bytes >> 21);
        padded[125] = (uint8_t)(bytes >> 13);
        padded[126] = (uint8_t)(bytes >> 5);
        padded[127] = (uint8_t)(bytes << 3);
        crypto_hashblocks_sm3(state, padded, 128);
    }

    for (size_t i = 0; i < 32; ++i) {
        out[i] = state[i];
    }
}

void sm3(uint8_t *out, const uint8_t *in, size_t inlen) {
    uint8_t state[40];
    sm3_inc_init(state);
    sm3_inc_finalize(out, state, in, inlen);
}

void mgf1_sm3(unsigned char *out, unsigned long outlen,
              const unsigned char *in, unsigned long inlen) {
    /* Use pseudoXOF from auxfunc.c for arbitrary length output (supports any length) */
    pseudoXOF(outlen * 8, in, inlen * 8, out);
}

void seed_state_sm3(spx_ctx *ctx) {
    uint8_t block[SPX_SM3_BLOCK_BYTES];

    memcpy(block, ctx->pub_seed, SPX_N);
    memset(block + SPX_N, 0, SPX_SM3_BLOCK_BYTES - SPX_N);

    sm3_inc_init(ctx->state_seeded_sm3);
    sm3_inc_blocks(ctx->state_seeded_sm3, block, 1);
}
