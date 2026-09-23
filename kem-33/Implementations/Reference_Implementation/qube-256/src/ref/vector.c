/**
 * @file vector.c
 * @brief Bit-vector helpers and PDF samplers.
 */

#include "vector.h"

#include <stdio.h>
#include <string.h>

#include "crypto_memset.h"
#include "parameters.h"

static unsigned ceil_log2_u32(uint32_t x);
static uint8_t get_bit(const uint64_t *v, uint32_t pos);
static void set_bit(uint64_t *v, uint32_t pos);

static unsigned ceil_log2_u32(uint32_t x) {
    unsigned bits = 0;
    uint32_t value = x - 1U;
    while (value != 0) {
        bits++;
        value >>= 1;
    }
    return bits;
}

static uint8_t get_bit(const uint64_t *v, uint32_t pos) {
    return (uint8_t)((v[pos >> 6] >> (pos & 63U)) & 1U);
}

static void set_bit(uint64_t *v, uint32_t pos) {
    v[pos >> 6] |= UINT64_C(1) << (pos & 63U);
}

void vect_set_zero(uint64_t *v, size_t words) {
    memset(v, 0, words * sizeof(uint64_t));
}

void vect_from_bytes(uint64_t *out, const uint8_t *in, size_t nbits) {
    const size_t out_words = CEIL_DIVIDE(nbits, 64);
    const size_t in_bytes = CEIL_DIVIDE(nbits, 8);

    memset(out, 0, out_words * sizeof(uint64_t));
    for (size_t i = 0; i < in_bytes; i++) {
        out[i >> 3] |= ((uint64_t)in[i]) << (8U * (i & 7U));
    }
    out[out_words - 1] &= QUBE_TAIL_MASK(nbits);
}

void vect_to_bytes(uint8_t *out, const uint64_t *in, size_t nbits) {
    const size_t out_bytes = CEIL_DIVIDE(nbits, 8);
    const size_t in_words = CEIL_DIVIDE(nbits, 64);

    memset(out, 0, out_bytes);
    for (size_t i = 0; i < in_words; i++) {
        uint64_t word = in[i];
        if (i == in_words - 1) {
            word &= QUBE_TAIL_MASK(nbits);
        }
        for (size_t j = 0; j < 8 && (8 * i + j) < out_bytes; j++) {
            out[8 * i + j] = (uint8_t)(word >> (8U * j));
        }
    }
    if ((nbits & 7U) != 0) {
        out[out_bytes - 1] &= (uint8_t)((1U << (nbits & 7U)) - 1U);
    }
}

int vect_set_random(qube_xof_stream_t *ctx, uint64_t *v) {
    uint8_t bytes[VEC_N_SIZE_BYTES];
    int ret = qube_xof_stream_read_bytes(ctx, bytes, sizeof bytes);
    if (ret != 0) {
        memset(v, 0, VEC_N_SIZE_64 * sizeof(uint64_t));
        return ret;
    }
    vect_from_bytes(v, bytes, PARAM_N);
    memset_zero(bytes, sizeof bytes);
    return 0;
}

void vect_write_support_to_vector(uint64_t *v, const uint32_t *support, uint16_t weight) {
    memset(v, 0, VEC_N_SIZE_64 * sizeof(uint64_t));
    for (uint16_t i = 0; i < weight; i++) {
        set_bit(v, support[i]);
    }
}

int vect_sample_fixed_weight(qube_xof_stream_t *ctx, uint64_t *v, uint32_t *support, uint16_t weight) {
    const unsigned bits = ceil_log2_u32(PARAM_N);
    uint16_t count = 0;

    memset(v, 0, VEC_N_SIZE_64 * sizeof(uint64_t));
    memset(support, 0, (size_t)weight * sizeof(uint32_t));

    while (count < weight) {
        uint32_t candidate = 0;
        int ret = qube_xof_stream_read_bits(ctx, bits, &candidate);
        if (ret != 0) {
            return ret;
        }
        if (candidate >= PARAM_N || get_bit(v, candidate) != 0) {
            continue;
        }
        support[count] = candidate;
        set_bit(v, candidate);
        count++;
    }
    return 0;
}

void vect_add(uint64_t *o, const uint64_t *v1, const uint64_t *v2, uint32_t size) {
    for (uint32_t i = 0; i < size; i++) {
        o[i] = v1[i] ^ v2[i];
    }
}

uint8_t vect_compare(const uint8_t *v1, const uint8_t *v2, uint32_t size) {
    uint8_t diff = 0;
    for (uint32_t i = 0; i < size; i++) {
        diff |= (uint8_t)(v1[i] ^ v2[i]);
    }
    return (uint8_t)((((uint16_t)diff) | (uint16_t)(0U - diff)) >> 15);
}

void vect_select(uint8_t *out, const uint8_t *a, const uint8_t *b, size_t len, uint8_t select_b) {
    const uint8_t mask = (uint8_t)(0U - (uint8_t)(select_b != 0));
    for (size_t i = 0; i < len; i++) {
        out[i] = (uint8_t)((a[i] & (uint8_t)~mask) | (b[i] & mask));
    }
}

void vect_truncate(uint64_t *v) {
    const size_t keep_words = CEIL_DIVIDE(PARAM_N1N2, 64);

    v[keep_words - 1] &= QUBE_TAIL_MASK(PARAM_N1N2);
    for (size_t i = keep_words; i < VEC_N_SIZE_64; i++) {
        v[i] = 0;
    }
}

uint32_t vect_weight(const uint64_t *v, size_t words) {
    uint32_t weight = 0;
    for (size_t i = 0; i < words; i++) {
        weight += (uint32_t)__builtin_popcountll(v[i]);
    }
    return weight;
}

void vect_print(const uint64_t *v, uint32_t size) {
    const uint8_t *bytes = (const uint8_t *)v;
    for (uint32_t i = 0; i < size; i++) {
        printf("%02x", bytes[i]);
    }
}
