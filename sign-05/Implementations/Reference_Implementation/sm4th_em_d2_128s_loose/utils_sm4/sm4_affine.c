/*
 * Shared SM4 affine-transform helper.
 * - S-box affine: y = A·x + c, for byte input/output and tag. Remember that S(x) = A·inverse(A·x + c) + c, so the same affine transform is used in both S-box and inverse S-box.
 * - S-box inverse affine: x = A^{-1}·y + d, for byte input/output and tag.
 * - L'_inv affine, for 32-bit word input/output and tag. Remember that T(x0, x1, x2, x3) = L'(S(x0), S(x1), S(x2), S(x3)) in the key expansion routine.
 * - L_inv affine, for 32-bit word input/output and tag. Remember that T(x0, x1, x2, x3) = L(S(x0), S(x1), S(x2), S(x3)) is used in encryption rounds.
 */

#include "sm4.h"
#include "compat.h"
#include "fields.h"

#include <stdint.h>

/*
 * SM4 affine transformation: y = A·x + c.
 */

static const uint8_t SM4_AFFINE_ROW_MASKS[8] = {
    0xA7u, 0x4Fu, 0x9Eu, 0x3Du, 0x7Au, 0xF4u, 0xE9u, 0xD3u
};

const uint8_t SM4_AFFINE_CONST = 0xD3u;

uint8_t sm4_affine_byte(uint8_t x)
{
    uint8_t y = 0;

    for (unsigned int i = 0; i < 8; ++i) {
        y |= (uint8_t)(parity8((uint8_t)(x & SM4_AFFINE_ROW_MASKS[i])) << i);
    }

    return y ^ SM4_AFFINE_CONST;
}

/*
 * SM4 inverse affine transformation: x = A^{-1}·y + d.
 */

static const uint8_t SM4_INV_AFFINE_ROW_MASKS[8] = {
    0x43u, 0x86u, 0x0Du, 0x1Au, 0x34u, 0x68u, 0xD0u, 0xA1u
};

const uint8_t SM4_INV_AFFINE_CONST = 0x75u;

uint8_t sm4_inv_affine_byte(uint8_t y)
{
    uint8_t x = 0;

    for (unsigned int i = 0; i < 8; ++i) {
        x |= (uint8_t)(parity8((uint8_t)(y & SM4_INV_AFFINE_ROW_MASKS[i])) << i);
    }

    return x ^ SM4_INV_AFFINE_CONST;
}

/* ================================================================== *
 *  Tag-side SM4 affine-transform helper                              *
 * ================================================================== */

/* SM4_AFFINE_ROW_MASKS = {0xA7,0x4F,0x9E,0x3D,0x7A,0xF4,0xE9,0xD3}
 * Each output bit depends on 5 input bits — fully unrolled, no loop/branch. */
void sm4_affine_byte_tag(bf128_t *out_tag,
                         const bf128_t *input_tag,
                         const bf128_t *const_tag)
{
    /* Load all 8 inputs first so in-place (out==input) is safe */
    const bf128_t i0 = input_tag[0], i1 = input_tag[1], i2 = input_tag[2], i3 = input_tag[3];
    const bf128_t i4 = input_tag[4], i5 = input_tag[5], i6 = input_tag[6], i7 = input_tag[7];

    /* 0xA7 = bits 0,1,2,5,7 */
    out_tag[0] = bf128_add(const_tag[0], bf128_add(bf128_add(i0, i1), bf128_add(bf128_add(i2, i5), i7)));
    /* 0x4F = bits 0,1,2,3,6 */
    out_tag[1] = bf128_add(const_tag[1], bf128_add(bf128_add(i0, i1), bf128_add(bf128_add(i2, i3), i6)));
    /* 0x9E = bits 1,2,3,4,7 */
    out_tag[2] = bf128_add(const_tag[2], bf128_add(bf128_add(i1, i2), bf128_add(bf128_add(i3, i4), i7)));
    /* 0x3D = bits 0,2,3,4,5 */
    out_tag[3] = bf128_add(const_tag[3], bf128_add(bf128_add(i0, i2), bf128_add(bf128_add(i3, i4), i5)));
    /* 0x7A = bits 1,3,4,5,6 */
    out_tag[4] = bf128_add(const_tag[4], bf128_add(bf128_add(i1, i3), bf128_add(bf128_add(i4, i5), i6)));
    /* 0xF4 = bits 2,4,5,6,7 */
    out_tag[5] = bf128_add(const_tag[5], bf128_add(bf128_add(i2, i4), bf128_add(bf128_add(i5, i6), i7)));
    /* 0xE9 = bits 0,3,5,6,7 */
    out_tag[6] = bf128_add(const_tag[6], bf128_add(bf128_add(i0, i3), bf128_add(bf128_add(i5, i6), i7)));
    /* 0xD3 = bits 0,1,4,6,7 */
    out_tag[7] = bf128_add(const_tag[7], bf128_add(bf128_add(i0, i1), bf128_add(bf128_add(i4, i6), i7)));
}

/* SM4_INV_AFFINE_ROW_MASKS = {0x43,0x86,0x0D,0x1A,0x34,0x68,0xD0,0xA1}
 * Each output bit depends on 3 input bits — fully unrolled. */
void sm4_inv_affine_byte_tag(bf128_t *out_tag,
                             const bf128_t *input_tag,
                             const bf128_t *const_tag)
{
    const bf128_t i0 = input_tag[0], i1 = input_tag[1], i2 = input_tag[2], i3 = input_tag[3];
    const bf128_t i4 = input_tag[4], i5 = input_tag[5], i6 = input_tag[6], i7 = input_tag[7];

    /* 0x43 = bits 0,1,6 */
    out_tag[0] = bf128_add(const_tag[0], bf128_add(bf128_add(i0, i1), i6));
    /* 0x86 = bits 1,2,7 */
    out_tag[1] = bf128_add(const_tag[1], bf128_add(bf128_add(i1, i2), i7));
    /* 0x0D = bits 0,2,3 */
    out_tag[2] = bf128_add(const_tag[2], bf128_add(bf128_add(i0, i2), i3));
    /* 0x1A = bits 1,3,4 */
    out_tag[3] = bf128_add(const_tag[3], bf128_add(bf128_add(i1, i3), i4));
    /* 0x34 = bits 2,4,5 */
    out_tag[4] = bf128_add(const_tag[4], bf128_add(bf128_add(i2, i4), i5));
    /* 0x68 = bits 3,5,6 */
    out_tag[5] = bf128_add(const_tag[5], bf128_add(bf128_add(i3, i5), i6));
    /* 0xD0 = bits 4,6,7 */
    out_tag[6] = bf128_add(const_tag[6], bf128_add(bf128_add(i4, i6), i7));
    /* 0xA1 = bits 0,5,7 */
    out_tag[7] = bf128_add(const_tag[7], bf128_add(bf128_add(i0, i5), i7));
}

unsigned int sm4_word_tag_index(unsigned int word_bit)
{
    return (3u - (word_bit / 8u)) * 8u + (word_bit % 8u);
}

/* sm4_word_tag_index(b) = (3 - b/8)*8 + b%8.
 * Precomputed table: wti[b] for b in 0..31. */
static const uint8_t SM4_WTI[32] = {
    24,25,26,27,28,29,30,31,  /* bits  0.. 7 -> byte 3 */
    16,17,18,19,20,21,22,23,  /* bits  8..15 -> byte 2 */
     8, 9,10,11,12,13,14,15,  /* bits 16..23 -> byte 1 */
     0, 1, 2, 3, 4, 5, 6, 7   /* bits 24..31 -> byte 0 */
};

/* Precomputed source-index tables for L'_inv and L_inv.
 *
 * L'_inv: shifts = {0,2,4,8,11,12,14,17,22,23,24,30,31}
 *   For each output bit o (0..31): src[o][k] = WTI[(o + 32 - shifts[k]) % 32]
 *
 * L_inv:  shifts = {0,2,4,8,12,14,16,18,22,24,30}
 *   Same formula, 11 terms.
 *
 * Both tables are generated at compile time as static const arrays.
 * Index: src_Lprime[out_bit * 13 + term], src_Linv[out_bit * 11 + term].
 */
static const uint8_t SRC_LPRIME_INV[32 * 13] = {
    /* out_bit =  0 */ 24, 6, 4, 0,13,12,10,23,18,17,16,26,25,
    /* out_bit =  1 */ 25, 7, 5, 1,14,13,11, 8,19,18,17,27,26,
    /* out_bit =  2 */ 26,24, 6, 2,15,14,12, 9,20,19,18,28,27,
    /* out_bit =  3 */ 27,25, 7, 3, 0,15,13,10,21,20,19,29,28,
    /* out_bit =  4 */ 28,26,24, 4, 1, 0,14,11,22,21,20,30,29,
    /* out_bit =  5 */ 29,27,25, 5, 2, 1,15,12,23,22,21,31,30,
    /* out_bit =  6 */ 30,28,26, 6, 3, 2, 0,13, 8,23,22,16,31,
    /* out_bit =  7 */ 31,29,27, 7, 4, 3, 1,14, 9, 8,23,17,16,
    /* out_bit =  8 */ 16,30,28,24, 5, 4, 2,15,10, 9, 8,18,17,
    /* out_bit =  9 */ 17,31,29,25, 6, 5, 3, 0,11,10, 9,19,18,
    /* out_bit = 10 */ 18,16,30,26, 7, 6, 4, 1,12,11,10,20,19,
    /* out_bit = 11 */ 19,17,31,27,24, 7, 5, 2,13,12,11,21,20,
    /* out_bit = 12 */ 20,18,16,28,25,24, 6, 3,14,13,12,22,21,
    /* out_bit = 13 */ 21,19,17,29,26,25, 7, 4,15,14,13,23,22,
    /* out_bit = 14 */ 22,20,18,30,27,26,24, 5, 0,15,14, 8,23,
    /* out_bit = 15 */ 23,21,19,31,28,27,25, 6, 1, 0,15, 9, 8,
    /* out_bit = 16 */  8,22,20,16,29,28,26, 7, 2, 1, 0,10, 9,
    /* out_bit = 17 */  9,23,21,17,30,29,27,24, 3, 2, 1,11,10,
    /* out_bit = 18 */ 10, 8,22,18,31,30,28,25, 4, 3, 2,12,11,
    /* out_bit = 19 */ 11, 9,23,19,16,31,29,26, 5, 4, 3,13,12,
    /* out_bit = 20 */ 12,10, 8,20,17,16,30,27, 6, 5, 4,14,13,
    /* out_bit = 21 */ 13,11, 9,21,18,17,31,28, 7, 6, 5,15,14,
    /* out_bit = 22 */ 14,12,10,22,19,18,16,29,24, 7, 6, 0,15,
    /* out_bit = 23 */ 15,13,11,23,20,19,17,30,25,24, 7, 1, 0,
    /* out_bit = 24 */  0,14,12, 8,21,20,18,31,26,25,24, 2, 1,
    /* out_bit = 25 */  1,15,13, 9,22,21,19,16,27,26,25, 3, 2,
    /* out_bit = 26 */  2, 0,14,10,23,22,20,17,28,27,26, 4, 3,
    /* out_bit = 27 */  3, 1,15,11, 8,23,21,18,29,28,27, 5, 4,
    /* out_bit = 28 */  4, 2, 0,12, 9, 8,22,19,30,29,28, 6, 5,
    /* out_bit = 29 */  5, 3, 1,13,10, 9,23,20,31,30,29, 7, 6,
    /* out_bit = 30 */  6, 4, 2,14,11,10, 8,21,16,31,30,24, 7,
    /* out_bit = 31 */  7, 5, 3,15,12,11, 9,22,17,16,31,25,24,
};

static const uint8_t SRC_LINV[32 * 11] = {
    /* out_bit =  0 */ 24, 6, 4, 0,12,10, 8,22,18,16,26,
    /* out_bit =  1 */ 25, 7, 5, 1,13,11, 9,23,19,17,27,
    /* out_bit =  2 */ 26,24, 6, 2,14,12,10, 8,20,18,28,
    /* out_bit =  3 */ 27,25, 7, 3,15,13,11, 9,21,19,29,
    /* out_bit =  4 */ 28,26,24, 4, 0,14,12,10,22,20,30,
    /* out_bit =  5 */ 29,27,25, 5, 1,15,13,11,23,21,31,
    /* out_bit =  6 */ 30,28,26, 6, 2, 0,14,12, 8,22,16,
    /* out_bit =  7 */ 31,29,27, 7, 3, 1,15,13, 9,23,17,
    /* out_bit =  8 */ 16,30,28,24, 4, 2, 0,14,10, 8,18,
    /* out_bit =  9 */ 17,31,29,25, 5, 3, 1,15,11, 9,19,
    /* out_bit = 10 */ 18,16,30,26, 6, 4, 2, 0,12,10,20,
    /* out_bit = 11 */ 19,17,31,27, 7, 5, 3, 1,13,11,21,
    /* out_bit = 12 */ 20,18,16,28,24, 6, 4, 2,14,12,22,
    /* out_bit = 13 */ 21,19,17,29,25, 7, 5, 3,15,13,23,
    /* out_bit = 14 */ 22,20,18,30,26,24, 6, 4, 0,14, 8,
    /* out_bit = 15 */ 23,21,19,31,27,25, 7, 5, 1,15, 9,
    /* out_bit = 16 */  8,22,20,16,28,26,24, 6, 2, 0,10,
    /* out_bit = 17 */  9,23,21,17,29,27,25, 7, 3, 1,11,
    /* out_bit = 18 */ 10, 8,22,18,30,28,26,24, 4, 2,12,
    /* out_bit = 19 */ 11, 9,23,19,31,29,27,25, 5, 3,13,
    /* out_bit = 20 */ 12,10, 8,20,16,30,28,26, 6, 4,14,
    /* out_bit = 21 */ 13,11, 9,21,17,31,29,27, 7, 5,15,
    /* out_bit = 22 */ 14,12,10,22,18,16,30,28,24, 6, 0,
    /* out_bit = 23 */ 15,13,11,23,19,17,31,29,25, 7, 1,
    /* out_bit = 24 */  0,14,12, 8,20,18,16,30,26,24, 2,
    /* out_bit = 25 */  1,15,13, 9,21,19,17,31,27,25, 3,
    /* out_bit = 26 */  2, 0,14,10,22,20,18,16,28,26, 4,
    /* out_bit = 27 */  3, 1,15,11,23,21,19,17,29,27, 5,
    /* out_bit = 28 */  4, 2, 0,12, 8,22,20,18,30,28, 6,
    /* out_bit = 29 */  5, 3, 1,13, 9,23,21,19,31,29, 7,
    /* out_bit = 30 */  6, 4, 2,14,10, 8,22,20,16,30,24,
    /* out_bit = 31 */  7, 5, 3,15,11, 9,23,21,17,31,25,
};

void SM4_L_prime_inv_bytes_tag(bf128_t *out_tag,
                               const bf128_t *in_tag)
{
    bf128_t tmp[32];

    for (unsigned int o = 0; o < 32; ++o) {
        const uint8_t *src = SRC_LPRIME_INV + o * 13;
        bf128_t t = in_tag[src[0]];
        t = bf128_add(t, in_tag[src[ 1]]);
        t = bf128_add(t, in_tag[src[ 2]]);
        t = bf128_add(t, in_tag[src[ 3]]);
        t = bf128_add(t, in_tag[src[ 4]]);
        t = bf128_add(t, in_tag[src[ 5]]);
        t = bf128_add(t, in_tag[src[ 6]]);
        t = bf128_add(t, in_tag[src[ 7]]);
        t = bf128_add(t, in_tag[src[ 8]]);
        t = bf128_add(t, in_tag[src[ 9]]);
        t = bf128_add(t, in_tag[src[10]]);
        t = bf128_add(t, in_tag[src[11]]);
        t = bf128_add(t, in_tag[src[12]]);
        tmp[o] = t;
    }

    for (unsigned int o = 0; o < 32; ++o) {
        out_tag[SM4_WTI[o]] = tmp[o];
    }
}

void SM4_L_inv_bytes_tag(bf128_t *out_tag,
                         const bf128_t *in_tag)
{
    bf128_t tmp[32];

    for (unsigned int o = 0; o < 32; ++o) {
        const uint8_t *src = SRC_LINV + o * 11;
        bf128_t t = in_tag[src[0]];
        t = bf128_add(t, in_tag[src[ 1]]);
        t = bf128_add(t, in_tag[src[ 2]]);
        t = bf128_add(t, in_tag[src[ 3]]);
        t = bf128_add(t, in_tag[src[ 4]]);
        t = bf128_add(t, in_tag[src[ 5]]);
        t = bf128_add(t, in_tag[src[ 6]]);
        t = bf128_add(t, in_tag[src[ 7]]);
        t = bf128_add(t, in_tag[src[ 8]]);
        t = bf128_add(t, in_tag[src[ 9]]);
        t = bf128_add(t, in_tag[src[10]]);
        tmp[o] = t;
    }

    for (unsigned int o = 0; o < 32; ++o) {
        out_tag[SM4_WTI[o]] = tmp[o];
    }
}
