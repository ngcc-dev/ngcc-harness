/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.
*/

#include "CryptHash_AlgorithmInstance.h"

#define _CRT_SECURE_NO_WARNINGS
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#ifndef RESTRICT
#  if defined(_MSC_VER)
#    define RESTRICT __restrict
#  elif defined(__GNUC__) || defined(__clang__)
#    define RESTRICT __restrict__
#  else
#    define RESTRICT
#  endif
#endif

#ifndef CRYPTHASH_SUCCESS
#define CRYPTHASH_SUCCESS            0
#define CRYPTHASH_ERR_NULLPTR       -1
#define CRYPTHASH_ERR_BAD_DIGEST_LEN -2
#define CRYPTHASH_ERR_ALLOC         -3
#endif

#define VEDAKF_ROUNDS 40
#define VEDAK_STATE_WORDS 40

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint8_t  u8;

static const u32 RC[VEDAKF_ROUNDS] = {
    0x2c387d69U, 0x988cc9ddU, 0xf0e4a1b5U, 0x21357064U,
    0x8397d2c6U, 0xc7d39682U, 0x4f5b1e0aU, 0x5e4a0f1bU,
    0x7c682d39U, 0x392d687cU, 0xb3a7e2f6U, 0xa7b3f6e2U,
    0x8e9adfcbU, 0xdcc88d99U, 0x786c293dU, 0x30246175U,
    0xa1b5f0e4U, 0x8296d3c7U, 0xc5d19480U, 0x4a5e1b0fU,
    0x55410410U, 0x6b7f3a2eU, 0x17034652U, 0xeffbbeaaU,
    0x1f0b4e5aU, 0xffebaebaU, 0x3f2b6e7aU, 0xbfabeefaU,
    0xbeaaeffbU, 0xbca8edf9U, 0xb9ade8fcU, 0xb2a6e3f7U,
    0xa5b1f4e0U, 0x8b9fdaceU, 0xd7c38692U, 0x6f7b3e2aU,
    0x1e0a4f5bU, 0xfde9acb8U, 0x3a2e6b7fU, 0xb4a0e5f1U
};

static const u8 S_BOX_8[256] = {
    0x11,0x10,0x15,0x16,0x1C,0x19,0x12,0x18,0x1A,0x17,0x13,0x1F,0x1E,0x1B,0x14,0x1D,
    0x01,0x00,0x05,0x06,0x0C,0x09,0x02,0x08,0x0A,0x07,0x03,0x0F,0x0E,0x0B,0x04,0x0D,
    0x51,0x50,0x55,0x56,0x5C,0x59,0x52,0x58,0x5A,0x57,0x53,0x5F,0x5E,0x5B,0x54,0x5D,
    0x61,0x60,0x65,0x66,0x6C,0x69,0x62,0x68,0x6A,0x67,0x63,0x6F,0x6E,0x6B,0x64,0x6D,
    0xC1,0xC0,0xC5,0xC6,0xCC,0xC9,0xC2,0xC8,0xCA,0xC7,0xC3,0xCF,0xCE,0xCB,0xC4,0xCD,
    0x91,0x90,0x95,0x96,0x9C,0x99,0x92,0x98,0x9A,0x97,0x93,0x9F,0x9E,0x9B,0x94,0x9D,
    0x21,0x20,0x25,0x26,0x2C,0x29,0x22,0x28,0x2A,0x27,0x23,0x2F,0x2E,0x2B,0x24,0x2D,
    0x81,0x80,0x85,0x86,0x8C,0x89,0x82,0x88,0x8A,0x87,0x83,0x8F,0x8E,0x8B,0x84,0x8D,
    0xA1,0xA0,0xA5,0xA6,0xAC,0xA9,0xA2,0xA8,0xAA,0xA7,0xA3,0xAF,0xAE,0xAB,0xA4,0xAD,
    0x71,0x70,0x75,0x76,0x7C,0x79,0x72,0x78,0x7A,0x77,0x73,0x7F,0x7E,0x7B,0x74,0x7D,
    0x31,0x30,0x35,0x36,0x3C,0x39,0x32,0x38,0x3A,0x37,0x33,0x3F,0x3E,0x3B,0x34,0x3D,
    0xF1,0xF0,0xF5,0xF6,0xFC,0xF9,0xF2,0xF8,0xFA,0xF7,0xF3,0xFF,0xFE,0xFB,0xF4,0xFD,
    0xE1,0xE0,0xE5,0xE6,0xEC,0xE9,0xE2,0xE8,0xEA,0xE7,0xE3,0xEF,0xEE,0xEB,0xE4,0xED,
    0xB1,0xB0,0xB5,0xB6,0xBC,0xB9,0xB2,0xB8,0xBA,0xB7,0xB3,0xBF,0xBE,0xBB,0xB4,0xBD,
    0x41,0x40,0x45,0x46,0x4C,0x49,0x42,0x48,0x4A,0x47,0x43,0x4F,0x4E,0x4B,0x44,0x4D,
    0xD1,0xD0,0xD5,0xD6,0xDC,0xD9,0xD2,0xD8,0xDA,0xD7,0xD3,0xDF,0xDE,0xDB,0xD4,0xDD
};

//static const u8 S_BOX_8[256] = {
//    0x33,0x30,0x36,0x32,0x35,0x34,0x3F,0x3E,0x3A,0x38,0x37,0x39,0x31,0x3C,0x3D,0x3B,
//    0x03,0x00,0x06,0x02,0x05,0x04,0x0F,0x0E,0x0A,0x08,0x07,0x09,0x01,0x0C,0x0D,0x0B,
//    0x63,0x60,0x66,0x62,0x65,0x64,0x6F,0x6E,0x6A,0x68,0x67,0x69,0x61,0x6C,0x6D,0x6B,
//    0x23,0x20,0x26,0x22,0x25,0x24,0x2F,0x2E,0x2A,0x28,0x27,0x29,0x21,0x2C,0x2D,0x2B,
//    0x53,0x50,0x56,0x52,0x55,0x54,0x5F,0x5E,0x5A,0x58,0x57,0x59,0x51,0x5C,0x5D,0x5B,
//    0x43,0x40,0x46,0x42,0x45,0x44,0x4F,0x4E,0x4A,0x48,0x47,0x49,0x41,0x4C,0x4D,0x4B,
//    0xF3,0xF0,0xF6,0xF2,0xF5,0xF4,0xFF,0xFE,0xFA,0xF8,0xF7,0xF9,0xF1,0xFC,0xFD,0xFB,
//    0xE3,0xE0,0xE6,0xE2,0xE5,0xE4,0xEF,0xEE,0xEA,0xE8,0xE7,0xE9,0xE1,0xEC,0xED,0xEB,
//    0xA3,0xA0,0xA6,0xA2,0xA5,0xA4,0xAF,0xAE,0xAA,0xA8,0xA7,0xA9,0xA1,0xAC,0xAD,0xAB,
//    0x83,0x80,0x86,0x82,0x85,0x84,0x8F,0x8E,0x8A,0x88,0x87,0x89,0x81,0x8C,0x8D,0x8B,
//    0x73,0x70,0x76,0x72,0x75,0x74,0x7F,0x7E,0x7A,0x78,0x77,0x79,0x71,0x7C,0x7D,0x7B,
//    0x93,0x90,0x96,0x92,0x95,0x94,0x9F,0x9E,0x9A,0x98,0x97,0x99,0x91,0x9C,0x9D,0x9B,
//    0x13,0x10,0x16,0x12,0x15,0x14,0x1F,0x1E,0x1A,0x18,0x17,0x19,0x11,0x1C,0x1D,0x1B,
//    0xC3,0xC0,0xC6,0xC2,0xC5,0xC4,0xCF,0xCE,0xCA,0xC8,0xC7,0xC9,0xC1,0xCC,0xCD,0xCB,
//    0xD3,0xD0,0xD6,0xD2,0xD5,0xD4,0xDF,0xDE,0xDA,0xD8,0xD7,0xD9,0xD1,0xDC,0xDD,0xDB,
//    0xB3,0xB0,0xB6,0xB2,0xB5,0xB4,0xBF,0xBE,0xBA,0xB8,0xB7,0xB9,0xB1,0xBC,0xBD,0xBB
//};



static const int PI[10][4] = {
    {2, 3, 0, 1},
    {1, 0, 3, 2},
    {3, 2, 1, 0},
    {3, 2, 1, 0},
    {1, 0, 3, 2},
    {0, 1, 2, 3},
    {0, 1, 2, 3},
    {2, 3, 0, 1},
    {2, 3, 0, 1},
    {0, 1, 2, 3}
};

static const int ALPHA[10][4] = {
    {1, 1, 1, 1},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {1, 1, 1, 1},
    {1, 1, 1, 1},
    {0, 0, 0, 0},
    {0, 1, 0, 1},
    {0, 1, 0, 1},
    {1, 0, 1, 0},
    {1, 0, 1, 0}
};

static const int RHO[10] = { 4, 1, 3, 0, 7, 0, 5, 3, 5, 1 };

static u32 rotr32(u32 x, unsigned n)
{
    n &= 31u;
    if (n == 0u) return x;
    return (x >> n) | (x << (32u - n));
}

static u64 swap32(u64 x)
{
    return (x >> 32) | (x << 32);
}

static u64 load64_be(const u8* p)
{
    return ((u64)p[0] << 56) | ((u64)p[1] << 48) | ((u64)p[2] << 40) | ((u64)p[3] << 32) |
        ((u64)p[4] << 24) | ((u64)p[5] << 16) | ((u64)p[6] << 8) | ((u64)p[7]);
}

static void store64_be(u8* p, u64 x)
{
    p[0] = (u8)(x >> 56);
    p[1] = (u8)(x >> 48);
    p[2] = (u8)(x >> 40);
    p[3] = (u8)(x >> 32);
    p[4] = (u8)(x >> 24);
    p[5] = (u8)(x >> 16);
    p[6] = (u8)(x >> 8);
    p[7] = (u8)(x);
}

static int get_bit_msb_first(const u8* msg, size_t bit_index)
{
    return (msg[bit_index >> 3] >> (7u - (unsigned)(bit_index & 7u))) & 1u;
}

static void set_bit_msb_first(u8* buf, size_t bit_index, int bit)
{
    u8 mask = (u8)(1u << (7u - (unsigned)(bit_index & 7u)));
    if (bit) buf[bit_index >> 3] |= mask;
    else     buf[bit_index >> 3] &= (u8)~mask;
}

static void apply_S(u64 state[VEDAK_STATE_WORDS])
{
    int i;
    for (i = 0; i < VEDAK_STATE_WORDS; ++i) {
        u64 x = state[i];
        state[i] =
            ((u64)S_BOX_8[(x) & 0xFFu]) |
            ((u64)S_BOX_8[(x >> 8) & 0xFFu] << 8) |
            ((u64)S_BOX_8[(x >> 16) & 0xFFu] << 16) |
            ((u64)S_BOX_8[(x >> 24) & 0xFFu] << 24) |
            ((u64)S_BOX_8[(x >> 32) & 0xFFu] << 32) |
            ((u64)S_BOX_8[(x >> 40) & 0xFFu] << 40) |
            ((u64)S_BOX_8[(x >> 48) & 0xFFu] << 48) |
            ((u64)S_BOX_8[(x >> 56) & 0xFFu] << 56);
    }
}

static void apply_L(u64 state[VEDAK_STATE_WORDS])
{
    u64 new_state[VEDAK_STATE_WORDS];
    int j;
    for (j = 0; j < 4; ++j) {
        u64 x0 = state[j + 0 * 4];
        u64 x1 = state[j + 1 * 4];
        u64 x2 = state[j + 2 * 4];
        u64 x3 = state[j + 3 * 4];
        u64 x4 = state[j + 4 * 4];
        u64 x5 = state[j + 5 * 4];
        u64 x6 = state[j + 6 * 4];
        u64 x7 = state[j + 7 * 4];
        u64 x8 = state[j + 8 * 4];
        u64 x9 = state[j + 9 * 4];

        new_state[j + 0 * 4] = x0 ^ x2 ^ x6 ^ x7 ^ x8;
        new_state[j + 1 * 4] = x2 ^ x4 ^ x5 ^ x8 ^ x9;
        new_state[j + 2 * 4] = x0 ^ x3 ^ x5 ^ x6 ^ x9;
        new_state[j + 3 * 4] = x1 ^ x4 ^ x5 ^ x6 ^ x7;
        new_state[j + 4 * 4] = x1 ^ x3 ^ x7 ^ x8 ^ x9;
        new_state[j + 5 * 4] = x0 ^ x1 ^ x2 ^ x5 ^ x7 ^ x9;
        new_state[j + 6 * 4] = x0 ^ x3 ^ x4 ^ x5 ^ x7 ^ x8;
        new_state[j + 7 * 4] = x2 ^ x3 ^ x4 ^ x6 ^ x7 ^ x9;
        new_state[j + 8 * 4] = x1 ^ x2 ^ x3 ^ x5 ^ x6 ^ x8;
        new_state[j + 9 * 4] = x0 ^ x1 ^ x4 ^ x6 ^ x8 ^ x9;
    }
    memcpy(state, new_state, sizeof(new_state));
}

static void apply_P(u64 state[VEDAK_STATE_WORDS])
{
    int i, j;
    for (i = 0; i < 10; ++i) {
        u64 row[4];
        unsigned shift = (unsigned)(RHO[i] * 4);
        for (j = 0; j < 4; ++j) row[j] = state[i * 4 + j];
        for (j = 0; j < 4; ++j) {
            u64 x = row[PI[i][j]];
            if (ALPHA[i][j]) x = swap32(x);

            u32 L = (u32)(x >> 32);
            u32 R = (u32)(x);
            L = rotr32(L, shift);
            R = rotr32(R, shift);
            state[i * 4 + j] = ((u64)L << 32) | (u64)R;
        }
    }
}

static void apply_AC(u64 state[VEDAK_STATE_WORDS], u32 rc)
{
    state[0] ^= ((u64)rc << 32);
}

static void vedak_p40(u64 state[VEDAK_STATE_WORDS])
{
    size_t r;
    for (r = 0; r < VEDAKF_ROUNDS; ++r) {
        apply_S(state);
        apply_L(state);
        apply_P(state);
        apply_AC(state, RC[r]);
    }
}

static int vedak_hash_bits(const u8* msg,
    size_t msg_len_bits,
    int digest_len_bits,
    u8* digest)
{
    u64 state[VEDAK_STATE_WORDS] = { 0 };
    size_t r_bitlen, r_bytelen, r_celllen;
    size_t padded_bits, padded_bytes, zero_bits;
    u8* padded = NULL;
    size_t i, block, blocks, out_bytes, out_words, copied_words;

    if (digest_len_bits != 512 && digest_len_bits != 768 && digest_len_bits != 1024) {
        return CRYPTHASH_ERR_BAD_DIGEST_LEN;
    }

    r_bitlen = 2560u - (size_t)(2 * digest_len_bits);
    r_bytelen = r_bitlen / 8u;
    r_celllen = r_bitlen / 64u;

    zero_bits = (r_bitlen - ((msg_len_bits + 2u) % r_bitlen)) % r_bitlen;
    padded_bits = msg_len_bits + 2u + zero_bits;
    padded_bytes = padded_bits / 8u;
    padded = (u8*)calloc(padded_bytes, 1u);
    if (padded == NULL) return CRYPTHASH_ERR_ALLOC;

    for (i = 0; i < msg_len_bits; ++i) {
        set_bit_msb_first(padded, i, get_bit_msb_first(msg, i));
    }
    set_bit_msb_first(padded, msg_len_bits, 1);
    set_bit_msb_first(padded, padded_bits - 1u, 1);

    blocks = padded_bytes / r_bytelen;
    for (block = 0; block < blocks; ++block) {
        const u8* blk = padded + block * r_bytelen;
        for (i = 0; i < r_celllen; ++i) {
            state[i] ^= load64_be(blk + i * 8u);
        }
        vedak_p40(state);
    }

    free(padded);
    padded = NULL;

    out_bytes = (size_t)digest_len_bits / 8u;
    out_words = (size_t)digest_len_bits / 64u;
    copied_words = 0u;
    while (copied_words < out_words) {
        size_t take = r_celllen;
        if (take > out_words - copied_words) take = out_words - copied_words;
        for (i = 0; i < take; ++i) {
            store64_be(digest + (copied_words + i) * 8u, state[i]);
        }
        copied_words += take;
        if (copied_words < out_words) {
            vedak_p40(state);
        }
    }

    (void)out_bytes;
    return CRYPTHASH_SUCCESS;
}

int CryptHash(int digest_len_bits,
    const unsigned char* msg,
    unsigned long long msg_len_bits,
    unsigned char* digest)
{
    if (digest == NULL) return CRYPTHASH_ERR_NULLPTR;
    if (msg_len_bits > 0ULL && msg == NULL) return CRYPTHASH_ERR_NULLPTR;
    return vedak_hash_bits((const u8*)msg, (size_t)msg_len_bits, digest_len_bits, (u8*)digest);
}

