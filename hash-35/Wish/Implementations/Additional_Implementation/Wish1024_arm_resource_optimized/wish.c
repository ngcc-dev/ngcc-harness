#include "wish.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include <arm_neon.h>

#define U128_ZEROES vdupq_n_u8(0)
#define U128_EXTEND_FROM_U64(x) vreinterpretq_u8_u64(vsetq_lane_u64((uint64_t)(x), vdupq_n_u64(0), 0))

#define XOR(a, b) veorq_u8(a, b)
#define LOAD(p) vld1q_u8((const uint8_t *)(p))
// #define STORE(p, v) vst1q_u8((uint8_t *)(p), v)

// M1: interleave bytes
#define UNPACKLO8(a, b) vzip1q_u8(a, b)
#define UNPACKHI8(a, b) vzip2q_u8(a, b)
// M2: interleave 16-bit words
#define UNPACKLO16(a, b) ((u128)vzip1q_u16((uint16x8_t)(a), (uint16x8_t)(b)))
#define UNPACKHI16(a, b) ((u128)vzip2q_u16((uint16x8_t)(a), (uint16x8_t)(b)))
// M3: interleave 32-bit dwords
#define UNPACKLO32(a, b) ((u128)vzip1q_u32((uint32x4_t)(a), (uint32x4_t)(b)))
#define UNPACKHI32(a, b) ((u128)vzip2q_u32((uint32x4_t)(a), (uint32x4_t)(b)))
// M4: interleave 64-bit qwords
#define UNPACKLO64(a, b) ((u128)vzip1q_u64((uint64x2_t)(a), (uint64x2_t)(b)))
#define UNPACKHI64(a, b) ((u128)vzip2q_u64((uint64x2_t)(a), (uint64x2_t)(b)))

#define WISH_MIX(granularity, x, y)        \
    do {                                   \
        u128 _lo = UNPACKLO##granularity(x, y);  \
        u128 _hi = UNPACKHI##granularity(x, y);  \
        (x) = _lo; \
        (y) = _hi; \
    } while (0)


// round constants: 24 lanes, added at lane 0 only, twice per step
// (rcon[2*step] after the 1st AES round, rcon[2*step+1] after the 2nd).
// aligned for the aligned vector load in LOAD().
static const unsigned char rcon[24][16] __attribute__((aligned(16))) = {
    {0x24, 0x3f, 0x6a, 0x88, 0x85, 0xa3, 0x08, 0xd3, 0x13, 0x19, 0x8a, 0x2e, 0x03, 0x70, 0x73, 0x44},
    {0x49, 0x7f, 0xd5, 0x11, 0x0b, 0x46, 0x10, 0xa7, 0x26, 0x32, 0x15, 0x5d, 0x06, 0xe1, 0xe7, 0x88},
    {0x92, 0xff, 0xab, 0x22, 0x16, 0x8c, 0x20, 0x4e, 0x4d, 0x65, 0x2a, 0xba, 0x0c, 0xc2, 0xce, 0x11},
    {0x25, 0xfe, 0x56, 0x45, 0x2c, 0x19, 0x41, 0x9c, 0x9a, 0xcb, 0x55, 0x74, 0x18, 0x85, 0x9d, 0x22},
    {0x4b, 0xfc, 0xac, 0x8a, 0x59, 0x32, 0x82, 0x39, 0x35, 0x97, 0xaa, 0xe9, 0x30, 0x0b, 0x3b, 0x45},
    {0x96, 0xf8, 0x58, 0x15, 0xb2, 0x65, 0x05, 0x73, 0x6b, 0x2f, 0x54, 0xd2, 0x61, 0x16, 0x77, 0x8a},
    {0x2d, 0xf0, 0xb0, 0x2a, 0x64, 0xcb, 0x0a, 0xe7, 0xd7, 0x5f, 0xa8, 0xa5, 0xc3, 0x2c, 0xef, 0x15},
    {0x5b, 0xe0, 0x60, 0x55, 0xc9, 0x97, 0x14, 0xce, 0xaf, 0xbe, 0x50, 0x4a, 0x87, 0x59, 0xde, 0x2a},
    {0xb6, 0xc0, 0xc1, 0xaa, 0x93, 0x2f, 0x28, 0x9d, 0x5e, 0x7c, 0xa0, 0x94, 0x0f, 0xb2, 0xbd, 0x55},
    {0x6c, 0x81, 0x83, 0x54, 0x27, 0x5f, 0x51, 0x3b, 0xbc, 0xf9, 0x40, 0x29, 0x1e, 0x64, 0x7a, 0xaa},
    {0xd9, 0x03, 0x07, 0xa8, 0x4f, 0xbe, 0xa2, 0x77, 0x78, 0xf2, 0x80, 0x53, 0x3c, 0xc9, 0xf5, 0x54},
    {0xb3, 0x06, 0x0e, 0x50, 0x9e, 0x7c, 0x44, 0xef, 0xf1, 0xe4, 0x01, 0xa6, 0x79, 0x93, 0xea, 0xa8},
    {0x66, 0x0c, 0x1c, 0xa0, 0x3d, 0xf9, 0x88, 0xde, 0xe2, 0xc8, 0x02, 0x4c, 0xf3, 0x27, 0xd4, 0x50},
    {0xcd, 0x18, 0x38, 0x40, 0x7b, 0xf2, 0x11, 0xbd, 0xc4, 0x91, 0x04, 0x98, 0xe6, 0x4f, 0xa9, 0xa0},
    {0x9b, 0x30, 0x71, 0x80, 0xf7, 0xe4, 0x22, 0x7a, 0x89, 0x23, 0x08, 0x31, 0xcc, 0x9e, 0x52, 0x40},
    {0x37, 0x61, 0xe3, 0x01, 0xee, 0xc8, 0x45, 0xf5, 0x13, 0x47, 0x10, 0x63, 0x99, 0x3d, 0xa4, 0x80},
    {0x6f, 0xc3, 0xc6, 0x02, 0xdc, 0x91, 0x8a, 0xea, 0x26, 0x8e, 0x20, 0xc7, 0x33, 0x7b, 0x48, 0x01},
    {0xdf, 0x87, 0x8d, 0x04, 0xb9, 0x23, 0x15, 0xd4, 0x4d, 0x1d, 0x41, 0x8f, 0x67, 0xf7, 0x90, 0x02},
    {0xbf, 0x0f, 0x1b, 0x08, 0x72, 0x47, 0x2a, 0xa9, 0x9a, 0x3a, 0x82, 0x1f, 0xcf, 0xee, 0x21, 0x04},
    {0x7e, 0x1e, 0x36, 0x10, 0xe5, 0x8e, 0x55, 0x52, 0x35, 0x75, 0x05, 0x3e, 0x9f, 0xdc, 0x43, 0x08},
    {0xfd, 0x3c, 0x6d, 0x20, 0xca, 0x1d, 0xaa, 0xa4, 0x6b, 0xeb, 0x0a, 0x7d, 0x3f, 0xb9, 0x86, 0x10},
    {0xfa, 0x79, 0xdb, 0x41, 0x95, 0x3a, 0x54, 0x48, 0xd7, 0xd6, 0x14, 0xfb, 0x7f, 0x72, 0x0d, 0x20},
    {0xf4, 0xf3, 0xb7, 0x82, 0x2b, 0x75, 0xa8, 0x90, 0xaf, 0xad, 0x28, 0xf6, 0xff, 0xe5, 0x1a, 0x41},
    {0xe8, 0xe6, 0x6e, 0x05, 0x57, 0xeb, 0x50, 0x21, 0x5e, 0x5a, 0x51, 0xec, 0xfe, 0xca, 0x34, 0x82},
};

static const unsigned char theta[16]  __attribute__((aligned(16))) = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};


int Wish1024Absorb(u128 *state, u128 counter, int last_block, const unsigned char *block) {
    // operate in place on state[] to minimise stack footprint (no local shadow copy)


    // save second half for feedforward (pre-theta)
    u128 feedforward[WISH_1024_NLANES / 2];
    memcpy(feedforward, &state[WISH_1024_NLANES / 2], (WISH_1024_NLANES / 2) * sizeof(u128));

    // #pragma GCC unroll 12
    for (int i = 0; i < WISH_1024_NSTEPS; ++i) {

        // 1st AES round: lane 0 (rate), embed msg for i==0; rcon[2*i] XOR after MC
        state[0] = XOR(vaesmcq_u8(vaeseq_u8(state[0],
                        (i == 0) ? LOAD(block) : U128_ZEROES)),
                        LOAD(rcon[2 * i]));
        // 1st AES round: rate lanes 1..NLANES/2-1, embed msg for i==0
        for (int j = 1; j < WISH_1024_NLANES / 2; ++j) {
            state[j] = vaesmcq_u8(vaeseq_u8(state[j],
                        (i == 0) ? LOAD(block + (16 * j)) : U128_ZEROES));
        }
        // 1st AES round: first capacity lane, embed counter every step
        state[WISH_1024_NLANES / 2] = vaesmcq_u8(vaeseq_u8(state[WISH_1024_NLANES / 2], counter));
        // 1st AES round: capacity lanes NLANES/2+1..NLANES-2
        for (int j = WISH_1024_NLANES / 2 + 1; j < WISH_1024_NLANES - 1; ++j) {
            state[j] = vaesmcq_u8(vaeseq_u8(state[j], U128_ZEROES));
        }
        // 1st AES round: last lane, embed theta for i==0 && last_block
        state[WISH_1024_NLANES - 1] = vaesmcq_u8(vaeseq_u8(state[WISH_1024_NLANES - 1],
                        (i == 0 && last_block == 1) ? LOAD(theta) : U128_ZEROES));

        // 2nd AES round: lane 0, rcon[2*i+1] XOR after MC
        state[0] = XOR(vaesmcq_u8(vaeseq_u8(state[0], U128_ZEROES)), LOAD(rcon[2 * i + 1]));
        // 2nd AES round: all other lanes
        // #pragma GCC unroll 15
        for (int j = 1; j < WISH_1024_NLANES; ++j) {
            state[j] = vaesmcq_u8(vaeseq_u8(state[j], U128_ZEROES));
        }


        // mixing M1: 8-bit granularity
        WISH_MIX(8, state[0], state[1]);
        WISH_MIX(8, state[2], state[3]);
        WISH_MIX(8, state[4], state[5]);
        WISH_MIX(8, state[6], state[7]);
        WISH_MIX(8, state[8], state[9]);
        WISH_MIX(8, state[10], state[11]);
        WISH_MIX(8, state[12], state[13]);
        WISH_MIX(8, state[14], state[15]);

        // mixing M2: 16-bit granularity
        WISH_MIX(16, state[0], state[2]);
        WISH_MIX(16, state[1], state[3]);
        WISH_MIX(16, state[4], state[6]);
        WISH_MIX(16, state[5], state[7]);
        WISH_MIX(16, state[8], state[10]);
        WISH_MIX(16, state[9], state[11]);
        WISH_MIX(16, state[12], state[14]);
        WISH_MIX(16, state[13], state[15]);

        // mixing M3: 32-bit granularity
        WISH_MIX(32, state[0], state[4]);
        WISH_MIX(32, state[1], state[5]);
        WISH_MIX(32, state[2], state[6]);
        WISH_MIX(32, state[3], state[7]);
        WISH_MIX(32, state[8], state[12]);
        WISH_MIX(32, state[9], state[13]);
        WISH_MIX(32, state[10], state[14]);
        WISH_MIX(32, state[11], state[15]);

        // mixing M4: 64-bit granularity
        WISH_MIX(64, state[0], state[8]);
        WISH_MIX(64, state[1], state[9]);
        WISH_MIX(64, state[2], state[10]);
        WISH_MIX(64, state[3], state[11]);
        WISH_MIX(64, state[4], state[12]);
        WISH_MIX(64, state[5], state[13]);
        WISH_MIX(64, state[6], state[14]);
        WISH_MIX(64, state[7], state[15]);

    }

    // feedforward the second half (pre-theta)
    // #pragma GCC unroll 8
    for (int i = WISH_1024_NLANES / 2; i < WISH_1024_NLANES; ++i) {
        state[i] = XOR(state[i], feedforward[i - (WISH_1024_NLANES / 2)]);
    }
    // compensate theta if last block
    if (last_block == 1) {
        state[WISH_1024_NLANES - 1] = XOR(state[WISH_1024_NLANES - 1], LOAD(theta));
    }


    return 0;
}


int Wish1024(const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *digest) {
    u128 state[WISH_1024_NLANES];

    // state initialized to all 0s
    memset(state, 0, (WISH_1024_NLANES) * sizeof(u128));


    // number of 128-byte blocks, rounded up, without final partial block
    // (which is padded and processed separately)
    unsigned long long n_full_blocks = msg_len_bits / WISH_1024_BLOCK_SIZE_BITS;

    for (unsigned long long i = 0; i < n_full_blocks; ++i) {
        // i as 64-bit counter in lower 64 bits
        const u128 counter = U128_EXTEND_FROM_U64(i);
        Wish1024Absorb(state, counter, 0, msg + (WISH_1024_BLOCK_SIZE_BYTES * i));
    }

    // final block: copy any partial tail, then pad with a 1-bit followed by zeros
    unsigned char padded_block[WISH_1024_BLOCK_SIZE_BYTES];
    const unsigned long long full_bytes = msg_len_bits / 8;
    const unsigned long long partial_bits = msg_len_bits % 8;
    const unsigned long long tail = full_bytes % WISH_1024_BLOCK_SIZE_BYTES;

    memset(padded_block, 0, WISH_1024_BLOCK_SIZE_BYTES);
    if (tail > 0) {
        memcpy(padded_block, msg + (full_bytes - tail), tail);
    }


    // append the padding 1-bit. for a byte-aligned message there is no partial
    // input byte to preserve, so just write the 0x80 marker (avoids reading
    // msg[full_bytes], one byte past the input). e.g. if partial_bits is 3, the
    // mask is 0b11100000 for the input bits and 0b00010000 for the appended bit
    if (partial_bits == 0) {
        padded_block[tail] = 0x80;
    } else {
        const unsigned char mask_input_bits = 0xff << (8 - partial_bits);
        const unsigned char mask_append_bit = 0x80 >> partial_bits;
        padded_block[tail] = (msg[full_bytes] & mask_input_bits) | mask_append_bit;
    }


    const u128 pad_counter = U128_EXTEND_FROM_U64(n_full_blocks);
    Wish1024Absorb(state, pad_counter, 1, padded_block);


    memcpy(digest, &state[WISH_1024_NLANES / 2], (WISH_1024_NLANES / 2) * sizeof(u128));
    return 0;
}
