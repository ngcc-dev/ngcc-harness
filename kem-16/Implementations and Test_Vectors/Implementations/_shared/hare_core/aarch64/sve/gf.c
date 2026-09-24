/**
 * @file gf.c
 * @brief AArch64/ARM optimized-safe GF(2^8) arithmetic for HARE ARM/SVE targets.
 *
 * This file mirrors the x86 optimized GF API without requiring PMULL.  The
 * multiplication path uses a fixed 8-iteration carry-less product followed by
 * the same public reduction schedule as the reference/x86 implementations.  It
 * is intentionally conservative until the target ARM feature probe confirms
 * whether PMULL/SVE2 polynomial multiply is available.
 */

#include "gf.h"

#include <stddef.h>
#include <stdint.h>

#include "parameters.h"

static uint16_t gf_reduce_arm(uint32_t x) {
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

uint16_t gf_mul(uint16_t a, uint16_t b) {
    uint32_t raw = 0;
    const uint32_t aa = (uint32_t)(a & 0xffu);
    const uint32_t bb = (uint32_t)(b & 0xffu);

    for (unsigned i = 0; i < 8u; ++i) {
        const uint32_t mask = 0u - ((bb >> i) & 1u);
        raw ^= (aa << i) & mask;
    }

    return gf_reduce_arm(raw);
}

uint16_t gf_square(uint16_t a) {
    uint32_t b = a;
    uint32_t s = b & 1u;

    for (size_t i = 1; i < PARAM_M; ++i) {
        b <<= 1;
        s ^= b & (1u << (2u * i));
    }

    return gf_reduce_arm(s);
}

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
