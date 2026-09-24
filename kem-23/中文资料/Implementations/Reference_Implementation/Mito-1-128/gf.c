/**
 * @file gf.c
 * @brief Galois field implementation with multiplication using the pclmulqdq instruction
 */

#include "gf.h"
#include "parameters.h"


/**
 * @brief Feedback bit positions used for modular reduction by PARAM_GF_POLY = 0x11D.
 *
 * These values are derived from the binary form of the polynomial:
 *     0x11D = 0b100011101 → bits set at positions: 8, 4, 3, 1, 0
 *
 * To reduce a polynomial modulo this irreducible polynomial:
 * - Bit 8 (the leading term) is handled via shifting: mod = x >> 8
 * - Bit 0 (constant term) is handled by the initial XOR
 *
 * The remaining set bits at positions 4, 3, and 2 define where the shifted
 * high bits (mod) must be XORed back into the result. These represent the
 * feedback positions used during reduction.
 */
static const uint8_t gf_reduction_taps[] = {4, 3, 2};

/**
 * @brief Reduce a polynomial modulo PARAM_GF_POLY in GF(2^8).
 *
 * This function performs modular reduction of a 16-bit polynomial `x`
 * by the irreducible polynomial PARAM_GF_POLY = 0x11D
 * (i.e., x⁸ + x⁴ + x³ + x + 1), used in GF(2^8).
 *
 * It assumes the input polynomial has degree ≤ 14 and uses a fixed
 * number of reduction steps and fixed feedback tap positions
 * ({4, 3, 2}) to produce a result of degree < 8.
 *
 * @param x 16-bit input polynomial to reduce (deg(x) ≤ 14)
 * @return Reduced 8-bit polynomial modulo PARAM_GF_POLY (deg(x) < 8)
 */
uint16_t gf_reduce(uint16_t x) {
    uint64_t mod;
    const int reduction_steps = 2;            // For deg(x) = 2 * (8 - 1) = 14, reduce twice to bring degree < 8
    const int gf_reduction_tap_count = 3;  // Number of feedback positions

    for (int i = 0; i < reduction_steps; ++i) {
        mod = x >> PARAM_M;       // Extract upper bits
        x &= (1 << PARAM_M) - 1;  // Keep lower bits
        x ^= mod;                 // Pre-XOR with no shift

        uint16_t z1 = 0;
        for (int j = gf_reduction_tap_count; j; --j) {
            uint16_t z2 = gf_reduction_taps[j - 1];
            uint16_t dist = z2 - z1;
            mod <<= dist;
            x ^= mod;
            z1 = z2;
        }
    }

    return x;
}

/**
 * Carryless multiplication of two polynomials a and b.
 *
 * Implementation of the algorithm mul1 in https://hal.inria.fr/inria-00188261v4/document
 * with s = 2 and w = 8
 *
 * @param[out] c polynomial c = a * b
 * @param[in] a The first polynomial
 * @param[in] b The second polynomial
 */
void gf_carryless_mul(uint8_t *c, uint8_t a, uint8_t b) {
    uint16_t h = 0, l = 0, g, u[4];
    u[0] = 0;
    u[1] = b & ((1UL << 7) - 1UL);
    u[2] = u[1] << 1;
    u[3] = u[2] ^ u[1];

    g = 0;
    uint16_t tmp1 = a & 3;

    for (int i = 0; i < 4; i++) {
        uint32_t tmp2 = tmp1 - i;
        g ^= (u[i] & -(1 - ((tmp2 | -tmp2) >> 31)));
    }

    l = g;
    h = 0;

    for (uint8_t i = 2; i < 8; i += 2) {
        g = 0;
        uint16_t tmp3 = (a >> i) & 3;
        for (int j = 0; j < 4; ++j) {
            uint32_t tmp2 = tmp3 - j;
            g ^= (u[j] & -(1 - ((tmp2 | -tmp2) >> 31)));
        }

        l ^= g << i;
        h ^= g >> (8 - i);
    }

    uint16_t mask = (-((b >> 7) & 1));
    l ^= ((a << 7) & mask);
    h ^= ((a >> (1)) & mask);

    c[0] = l & 0xFF;
    c[1] = h & 0xFF;
}

/**
 * Multiplies two elements of GF(2^GF_M).
 * @returns the product a*b
 * @param[in] a Element of GF(2^GF_M)
 * @param[in] b Element of GF(2^GF_M)
 */
uint16_t gf_mul(uint16_t a, uint16_t b) {
    uint8_t c[2] = {0};
    gf_carryless_mul(c, (uint8_t)a, (uint8_t)b);
    uint16_t tmp = (uint16_t)(c[0] ^ (c[1] << 8));
    return gf_reduce(tmp);
}

/**
 * Squares an element of GF(2^GF_M).
 * @returns a^2
 * @param[in] a Element of GF(2^GF_M)
 */
uint16_t gf_square(uint16_t a) {
    uint32_t b = a;
    uint32_t s = b & 1;
    for (int i = 1; i < PARAM_M; ++i) {
        b <<= 1;
        s ^= b & (1 << 2 * i);
    }

    return gf_reduce(s);
}

/**
 * Computes the inverse of an element of GF(2^8),
 * using the addition chain 1 2 3 4 7 11 15 30 60 120 127 254
 * @returns the inverse of a
 * @param[in] a Element of GF(2^GF_M)
 */
uint16_t gf_inverse(uint16_t a) {
    uint16_t inv = a;
    uint16_t tmp1, tmp2;

    inv = gf_square(a);       /* a^2 */
    tmp1 = gf_mul(inv, a);    /* a^3 */
    inv = gf_square(inv);     /* a^4 */
    tmp2 = gf_mul(inv, tmp1); /* a^7 */
    tmp1 = gf_mul(inv, tmp2); /* a^11 */
    inv = gf_mul(tmp1, inv);  /* a^15 */
    inv = gf_square(inv);     /* a^30 */
    inv = gf_square(inv);     /* a^60 */
    inv = gf_square(inv);     /* a^120 */
    inv = gf_mul(inv, tmp2);  /* a^127 */
    inv = gf_square(inv);     /* a^254 */
    return inv;
}
