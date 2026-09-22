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

#include <string.h>
#include <stdint.h>

/* Custom permutation and bytewise API */
#include "../../../lib/low/ZuD-1280/plain/ZuD1280-plain.h"
#include <stdio.h>
/* Width of state in bits */
#define WIDTH_BITS 1280u
#define WIDTH_BYTES (WIDTH_BITS/8u)

static void permute(ZuD1280_plain64_state *s)
{
    ZuD1280_plain_Permute_12rounds(s);
    //ZuD1280_plain_Permute_12rounds(s);
}

static void xor_bytes(uint8_t *dst, const uint8_t *src, size_t n)
{
    for (size_t i = 0; i < n; i++) dst[i] ^= src[i];
}

/* MSB-first bit addressing within bytes, as required by AlgorithmInstance/README.txt */
static void xor_bit_msb(uint8_t *buf, unsigned int bitpos, uint8_t bit)
{
    if (!bit) return;
    unsigned int byteIndex = bitpos >> 3;
    unsigned int bitInByte = 7u - (bitpos & 7u);
    //unsigned int bitInByte = (bitpos & 7u);
    buf[byteIndex] ^= (uint8_t)(1u << bitInByte);
}

/* Forward declaration: used by append_bit_msb_custom. */
static void absorb_block_custom(ZuD1280_plain64_state *state, const uint8_t *block, unsigned int rateBytes);

static void append_bit_msb_custom(
    ZuD1280_plain64_state *st,
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
    if (b){
        xor_bit_msb(block, *bitpos, 1);
    }
    (*bitpos)++;
}

/* Absorb one full rate block with custom update:
 *   state <- P(state XOR block) XOR state XOR block
 * where 'block' is rate bytes and is implicitly zero on the capacity part.
 */
static void absorb_block_custom(ZuD1280_plain64_state *state, const uint8_t *block, unsigned int rateBytes)
{
    ZuD1280_plain_AddBytes(state, block, 0, rateBytes);
    uint8_t oldState[WIDTH_BYTES];
    memcpy(oldState, state, WIDTH_BYTES);

    //ZuD1280_plain_AddBytes(state, block, 0, rateBytes);
    permute(state);

    /* XOR back old full state */
    ZuD1280_plain_AddBytes(state, rateBytes + oldState, rateBytes, WIDTH_BYTES - rateBytes);
    //xor_bytes((uint8_t*)state, oldState, WIDTH_BYTES);
    /* XOR back message block over the rate portion */
    //xor_bytes((uint8_t*)state, block, rateBytes);
}

/* Squeeze digest_len_bits (must be byte-aligned for this harness). */
static int squeeze_bytes(ZuD1280_plain64_state *state, unsigned int rateBytes, uint8_t *out, unsigned int outBytes)
{
    unsigned int produced = 0;
    while (produced < outBytes) {
        unsigned int take = outBytes - produced;
        if (take > rateBytes) take = rateBytes;
        ZuD1280_plain_ExtractBytes(state, out + produced, 0, take);
        produced += take;
        if (produced < outBytes) {
            permute(state);
        }
    }
    return 0;
}

int CryptHash(int digest_len_bits, const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *digest)
{
    /* Map digest length to capacity (SHA3-like rule: output = capacity/2). */
    if ((digest_len_bits <= 0) || ((digest_len_bits % 8) != 0))
        return 1;

    unsigned int capacity = (unsigned int)digest_len_bits + 64u;
    if (!((capacity == 576u) || (capacity == 832u) || (capacity == 1088u)))
        return 1;

    unsigned int rate = WIDTH_BITS - capacity;
    if ((rate == 0) || (rate % 8u))
        return 1;
    unsigned int rateBytes = rate / 8u;
    unsigned int rateOutBytes = 8u;

    ZuD1280_plain64_state st;
    ZuD1280_plain_Initialize(&st);

    /* Fast path: absorb full blocks from whole bytes. */
    unsigned long long fullBytes = msg_len_bits / 8ull;
    unsigned int remBits = (unsigned int)(msg_len_bits % 8ull);

    /* Absorb as many full blocks as possible from the full-byte portion. */
    unsigned long long offsetBytes = 0;
    while (fullBytes - offsetBytes >= (unsigned long long)rateBytes) {
        absorb_block_custom(&st, (const uint8_t*)msg + offsetBytes, rateBytes);
        offsetBytes += rateBytes;
    }

    /* Build and absorb the final padded blocks at bit-level (only for the tail). */
    uint8_t block[/* VLA not allowed in some toolchains; use max */ 160];
    if (rateBytes > sizeof(block)) return 1;
    memset(block, 0, rateBytes);

    /* Current position within block in bits */
    unsigned int bitpos = 0;

    /* Copy remaining whole bytes into block */
    unsigned int tailBytes = (unsigned int)(fullBytes - offsetBytes);
    if (tailBytes) {
        memcpy(block, (const uint8_t*)msg + offsetBytes, tailBytes);
        bitpos = tailBytes * 8u;
    }

    /* Copy remaining bits (MSB-first in the next byte) */
    if (remBits) {
        uint8_t last = ((const uint8_t*)msg)[fullBytes];
        /* Keep only the top remBits */
        uint8_t mask = (uint8_t)(0xFFu << (8u - remBits));
        block[tailBytes] = (uint8_t)(last & mask);
        bitpos += remBits;
    }
    /* Append suffix bits */
    //append_byte_msb_custom(&st, block, &bitpos, rateBytes, suffix);
    append_bit_msb_custom(&st, block, &bitpos, rateBytes, 0);
    append_bit_msb_custom(&st, block, &bitpos, rateBytes, 1);
    /* Append pad10*1 first '1' bit */
    //if (bitpos == rateBytes * 8u) {
    //    absorb_block_custom(&st, block, rateBytes);
    //    memset(block, 0, rateBytes);
    //    bitpos = 0;
    //}
    //xor_bit_msb(block, bitpos, 1);
    //bitpos++;

    /* Append final '1' at last bit of the current block; if we've passed it, start new block. */
    //if (bitpos > rateBytes * 8u - 1u) {
    //    absorb_block_custom(&st, block, rateBytes);
    //    memset(block, 0, rateBytes);
    //    bitpos = 0;
    //}
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

    /* Absorb final block */
    absorb_block_custom(&st, block, rateBytes);

    /* Squeeze fixed output length: digest_len_bits bits */
    unsigned int outBytes = (unsigned int)digest_len_bits / 8u;
    return squeeze_bytes(&st, rateOutBytes, digest, outBytes);
}

