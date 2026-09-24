/**
 * @file gf.c
 * @brief x86_64/PCLMULQDQ GF(2^8) arithmetic for optimized HARE targets.
 *
 * This file replaces ref/gf.c only for Optimized_Implementation targets. It
 * keeps the same public API and the same GF(2^8) primitive polynomial 0x11D.
 * Multiplication uses carry-less multiplication (PCLMULQDQ) followed by the
 * same fixed reduction schedule as the reference implementation.
 */

#include "gf.h"

#include <immintrin.h>
#include <stddef.h>
#include <stdint.h>

#include "parameters.h"

/**
 * Reduce a carry-less product candidate modulo the GF(2^8) primitive polynomial.
 */
static uint16_t gf_reduce_x86(uint32_t x) {
    uint32_t mod;

    for (int i = 0; i < 2; ++i) {
        mod = x >> PARAM_M;
        x &= (1u << PARAM_M) - 1u;
        x ^= mod;

        mod <<= 2;
        x ^= mod;
        mod <<= 1;
        x ^= mod;
        mod <<= 1;
        x ^= mod;
    }

    return (uint16_t)(x & ((1u << PARAM_M) - 1u));
}

/**
 * Generate exponent/log lookup tables for GF(2^8).
 */
void gf_generate(uint16_t *exp, uint16_t *log, const int16_t m) {
    uint16_t elt = 1;
    uint16_t alpha = 2;
    uint16_t gf_poly = PARAM_GF_POLY;

    for (size_t i = 0; i < (1U << m) - 1U; ++i) {
        exp[i] = elt;
        log[elt] = (uint16_t)i;

        elt = (uint16_t)(elt * alpha);
        if (elt >= (uint16_t)(1U << m)) {
            elt ^= gf_poly;
        }
    }

    exp[(1U << m) - 1U] = 1;
    exp[1U << m] = 2;
    exp[(1U << m) + 1U] = 4;
    log[0] = 0;
}

/**
 * Multiply two GF(2^8) elements using the optimized x86 helper path.
 */
uint16_t gf_mul(uint16_t a, uint16_t b) {
    const __m128i va = _mm_cvtsi32_si128((int)(a & 0xffu));
    const __m128i vb = _mm_cvtsi32_si128((int)(b & 0xffu));
    const __m128i vp = _mm_clmulepi64_si128(va, vb, 0x00);
    const uint32_t raw = (uint32_t)_mm_cvtsi128_si32(vp);
    return gf_reduce_x86(raw);
}

/**
 * Square one GF(2^8) element.
 */
uint16_t gf_square(uint16_t a) {
    uint32_t b = a;
    uint32_t s = b & 1u;

    for (size_t i = 1; i < PARAM_M; ++i) {
        b <<= 1;
        s ^= b & (1u << (2u * i));
    }

    return gf_reduce_x86(s);
}

/**
 * Invert one GF(2^8) element, returning zero for zero input as in the reference path.
 */
uint16_t gf_inverse(uint16_t a) {
    uint16_t inv = a;
    uint16_t tmp1;
    uint16_t tmp2;

    inv = gf_square(a);
    tmp1 = gf_mul(inv, a);
    inv = gf_square(inv);
    tmp2 = gf_mul(inv, tmp1);
    tmp1 = gf_mul(inv, tmp2);
    inv = gf_mul(tmp1, inv);
    inv = gf_square(inv);
    inv = gf_square(inv);
    inv = gf_square(inv);
    inv = gf_mul(inv, tmp2);
    inv = gf_square(inv);
    return inv;
}
