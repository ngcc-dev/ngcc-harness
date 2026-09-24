/*
 * Shared SM4 affine-transform helper.
 * - S-box affine: y = A·x + c, for byte input/output and tag. Remember that S(x) = A·inverse(A·x + c) + c, so the same affine transform is used in both S-box and inverse S-box.
 * - S-box inverse affine: x = A^{-1}·y + d, for byte input/output and tag.
 * - L'_inv affine, for 32-bit word input/output and tag. Remember that T(x0, x1, x2, x3) = L'(S(x0), S(x1), S(x2), S(x3)) in the key expansion routine.
 * - L_inv affine, for 32-bit word input/output and tag. Remember that T(x0, x1, x2, x3) = L(S(x0), S(x1), S(x2), S(x3)) is used in encryption rounds.
 */

#include "sm4.h"
#include "compat.h"

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
