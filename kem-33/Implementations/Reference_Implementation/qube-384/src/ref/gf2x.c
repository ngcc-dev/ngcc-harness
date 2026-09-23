/**
 * @file gf2x.c
 * @brief Reference arithmetic in R = F_2[X]/(X^r - 1).
 */

#include "gf2x.h"

#include <string.h>

static void xor_linear_word(uint64_t *out, uint64_t word, uint32_t bitpos);
static void xor_word_mod_xr(uint64_t *out, uint64_t word, uint32_t bitpos);
static void xor_shifted(uint64_t *out, const uint64_t *dense, uint32_t shift);

static void xor_linear_word(uint64_t *out, uint64_t word, uint32_t bitpos) {
    const uint32_t word_index = bitpos >> 6;
    const unsigned offset = bitpos & 63U;

    if (word == 0) {
        return;
    }

    out[word_index] ^= word << offset;
    if (offset != 0 && word_index + 1U < VEC_N_SIZE_64) {
        out[word_index + 1U] ^= word >> (64U - offset);
    }
}

static void xor_word_mod_xr(uint64_t *out, uint64_t word, uint32_t bitpos) {
    const uint32_t first_bits = PARAM_N - bitpos;

    if (word == 0) {
        return;
    }

    if (first_bits >= 64U) {
        xor_linear_word(out, word, bitpos);
    } else {
        const uint64_t low_mask = (UINT64_C(1) << first_bits) - 1U;
        xor_linear_word(out, word & low_mask, bitpos);
        xor_linear_word(out, word >> first_bits, 0);
    }
}

static void xor_shifted(uint64_t *out, const uint64_t *dense, uint32_t shift) {
    for (uint32_t i = 0; i < VEC_N_SIZE_64; i++) {
        uint64_t word = dense[i];
        uint32_t bitpos = i * 64U + shift;

        if (i + 1U == VEC_N_SIZE_64) {
            word &= QUBE_TAIL_MASK(PARAM_N);
        }
        if (bitpos >= PARAM_N) {
            bitpos -= PARAM_N;
        }
        xor_word_mod_xr(out, word, bitpos);
    }
}

void ring_mul_by_support(uint64_t *out, const uint64_t *dense, const uint32_t *support, uint16_t weight) {
    memset(out, 0, VEC_N_SIZE_64 * sizeof(uint64_t));
    for (uint16_t i = 0; i < weight; i++) {
        xor_shifted(out, dense, support[i]);
    }
    out[VEC_N_SIZE_64 - 1] &= QUBE_TAIL_MASK(PARAM_N);
}

void vect_mul(uint64_t *out, const uint64_t *a, const uint64_t *b) {
    memset(out, 0, VEC_N_SIZE_64 * sizeof(uint64_t));
    for (uint32_t i = 0; i < PARAM_N; i++) {
        if (((b[i >> 6] >> (i & 63U)) & 1U) != 0) {
            xor_shifted(out, a, i);
        }
    }
    out[VEC_N_SIZE_64 - 1] &= QUBE_TAIL_MASK(PARAM_N);
}
