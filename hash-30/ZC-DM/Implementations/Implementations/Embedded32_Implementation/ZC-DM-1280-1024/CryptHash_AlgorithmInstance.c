/*
Embedded 32-bit implementation profile (no uint64_t arithmetic in permutation).
*/

#include "CryptHash_AlgorithmInstance.h"

#include <string.h>
#include <stdint.h>

#include "../../../lib/low/ZuD-1280/32bit/ZuD1280-32bit.h"

#define WIDTH_BITS 1280u
#define WIDTH_BYTES (WIDTH_BITS / 8u)

static void permute(ZuD1280_32_state *s)
{
    ZuD1280_32_Permute_12rounds(s);
}

static void xor_bytes(uint8_t *dst, const uint8_t *src, size_t n)
{
    for (size_t i = 0; i < n; i++) dst[i] ^= src[i];
}

static void xor_bit_msb(uint8_t *buf, unsigned int bitpos, uint8_t bit)
{
    if (!bit) return;
    unsigned int byteIndex = bitpos >> 3;
    unsigned int bitInByte = 7u - (bitpos & 7u);
    buf[byteIndex] ^= (uint8_t)(1u << bitInByte);
}

static void absorb_block_custom(ZuD1280_32_state *state, const uint8_t *block, unsigned int rateBytes);

static void append_bit_msb_custom(
    ZuD1280_32_state *st,
    uint8_t *block,
    unsigned int *bitpos,
    unsigned int rateBytes,
    uint8_t b)
{
    if (*bitpos == rateBytes * 8u) {
        absorb_block_custom(st, block, rateBytes);
        memset(block, 0, rateBytes);
        *bitpos = 0;
    }
    if (b) {
        xor_bit_msb(block, *bitpos, 1);
    }
    (*bitpos)++;
}
//    for (unsigned int k = 0; k < 8u; k++) {
//        uint8_t b = (uint8_t)((v >> (7u - k)) & 1u);
//        if (*bitpos == rateBytes * 8u) {
//            absorb_block_custom(st, block, rateBytes);
//            memset(block, 0, rateBytes);
//            *bitpos = 0;
//        }
//        xor_bit_msb(block, *bitpos, b);
//        (*bitpos)++;
//    }
//}

static void absorb_block_custom(ZuD1280_32_state *state, const uint8_t *block, unsigned int rateBytes)
{
    ZuD1280_32_AddBytes(state, block, 0, rateBytes);
    uint8_t oldState[WIDTH_BYTES];
    memcpy(oldState, state, WIDTH_BYTES);

    //ZuD1280_32_AddBytes(state, block, 0, rateBytes);
    permute(state);

    xor_bytes((uint8_t *)state, oldState, WIDTH_BYTES);
    //xor_bytes((uint8_t *)state, block, rateBytes);
}

static int squeeze_bytes(ZuD1280_32_state *state, unsigned int rateBytes, uint8_t *out, unsigned int outBytes)
{
    unsigned int produced = 0;
    while (produced < outBytes) {
        unsigned int take = outBytes - produced;
        if (take > rateBytes) take = rateBytes;
        ZuD1280_32_ExtractBytes(state, out + produced, 0, take);
        produced += take;
        if (produced < outBytes) permute(state);
    }
    return 0;
}

int CryptHash(int digest_len_bits, const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *digest)
{
    if ((digest_len_bits <= 0) || ((digest_len_bits % 8) != 0))
        return 1;

    unsigned int capacity = (unsigned int)digest_len_bits + 64u;
    if (!((capacity == 576u) || (capacity == 832u) || (capacity == 1088u)))
        return 1;

    unsigned int rate = WIDTH_BITS - capacity;
    if ((rate == 0) || (rate % 8u))
        return 1;
    unsigned int rateBytes = rate / 8u;

    //const uint8_t suffix = 0x06;

    ZuD1280_32_state st;
    ZuD1280_32_Initialize(&st);

    unsigned long long fullBytes = msg_len_bits / 8ull;
    unsigned int remBits = (unsigned int)(msg_len_bits % 8ull);

    unsigned long long offsetBytes = 0;
    while (fullBytes - offsetBytes >= (unsigned long long)rateBytes) {
        absorb_block_custom(&st, (const uint8_t *)msg + offsetBytes, rateBytes);
        offsetBytes += rateBytes;
    }

    uint8_t block[160];
    if (rateBytes > sizeof(block)) return 1;
    memset(block, 0, rateBytes);

    unsigned int bitpos = 0;

    unsigned int tailBytes = (unsigned int)(fullBytes - offsetBytes);
    if (tailBytes) {
        memcpy(block, (const uint8_t *)msg + offsetBytes, tailBytes);
        bitpos = tailBytes * 8u;
    }

    if (remBits) {
        uint8_t last = ((const uint8_t *)msg)[fullBytes];
        uint8_t mask = (uint8_t)(0xFFu << (8u - remBits));
        block[tailBytes] = (uint8_t)(last & mask);
        bitpos += remBits;
    }
    /* Append domain separation bits "01" (MSB-first within our bit indexing).  */
    append_bit_msb_custom(&st, block, &bitpos, rateBytes, 0);
    append_bit_msb_custom(&st, block, &bitpos, rateBytes, 1);
    /*
    if (bitpos == rateBytes * 8u) {
        absorb_block_custom(&st, block, rateBytes);
        memset(block, 0, rateBytes);
        bitpos = 0;
    }
    xor_bit_msb(block, bitpos, 1);
    bitpos++;

    if (bitpos > rateBytes * 8u - 1u) {
        absorb_block_custom(&st, block, rateBytes);
        memset(block, 0, rateBytes);
        bitpos = 0;
    }*/

    /* pad10*1 after domain bits: append first '1', then implicit zeros, then final '1' at end of block. */
    append_bit_msb_custom(&st, block, &bitpos, rateBytes, 1);
    /* if first pad '1' filled the block, absorb and continue on next block */
    if (bitpos == rateBytes * 8u) {
        absorb_block_custom(&st, block, rateBytes);
        memset(block, 0, rateBytes);
        bitpos = 0;
    }
    /* zeros until last bit position of THIS block */
    while (bitpos < rateBytes * 8u - 1u) {
        append_bit_msb_custom(&st, block, &bitpos, rateBytes, 0);
    }
    xor_bit_msb(block, rateBytes * 8u - 1u, 1);
    bitpos = rateBytes * 8u;   /* optional bookkeeping */
    absorb_block_custom(&st, block, rateBytes);

    unsigned int outBytes = (unsigned int)digest_len_bits / 8u;
    return squeeze_bytes(&st, rateBytes, digest, outBytes);
}
