/**
 * @file vector.c
 * @brief AArch64/SVE implementation of HARE vector utilities.
 *
 * Public-length vector add/compare/truncate operations use SVE.  Fixed-weight
 * sampling keeps the reference distribution.  Support-to-vector conversion uses
 * an SVE mask-scan over public word positions: secret support values are used
 * only in lane comparisons and masks, never as direct memory-write addresses.
 */

#include "vector.h"

#include <arm_sve.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "parameters.h"

static inline uint32_t compare_u32(const uint32_t v1, const uint32_t v2) {
    return 1u ^ (((v1 - v2) | (v2 - v1)) >> 31);
}

static inline uint32_t barrett_reduce(uint32_t x) {
    uint64_t q = ((uint64_t)x * PARAM_N_MU) >> 32;
    uint32_t r = x - (uint32_t)(q * PARAM_N);

    uint32_t reduce_flag = (((r - PARAM_N) >> 31) ^ 1u);
    uint32_t mask = 0u - reduce_flag;
    r -= mask & PARAM_N;
    return r;
}

void vect_generate_random_support1(shake256_xof_ctx *ctx, uint32_t *support, uint16_t weight) {
    size_t random_bytes_size = 3u * weight;
    uint8_t rand_bytes[3 * PARAM_OMEGA_MAX] = {0};
    uint8_t inc;
    size_t i, j;

    i = 0;
    j = random_bytes_size;
    while (i < weight) {
        do {
            if (j == random_bytes_size) {
                xof_get_bytes(ctx, rand_bytes, random_bytes_size);
                j = 0;
            }

            support[i] = ((uint32_t)rand_bytes[j++]) << 16;
            support[i] |= ((uint32_t)rand_bytes[j++]) << 8;
            support[i] |= rand_bytes[j++];

        } while (support[i] >= UTILS_REJECTION_THRESHOLD);

        support[i] = barrett_reduce(support[i]);

        inc = 1;
        for (size_t k = 0; k < i; k++) {
            if (support[k] == support[i]) {
                inc = 0;
            }
        }
        i += inc;
    }
}

void vect_generate_random_support2(shake256_xof_ctx *ctx, uint32_t *support, uint16_t weight) {
    uint32_t rand_u32[PARAM_OMEGA_MAX] = {0};

    xof_get_bytes(ctx, (uint8_t *)&rand_u32, 4u * weight);

    for (size_t i = 0; i < weight; ++i) {
        uint64_t buff = rand_u32[i];
        support[i] = (uint32_t)(i + ((buff * (PARAM_N - i)) >> 32));
    }

    for (int32_t i = (weight - 1); i-- > 0;) {
        uint32_t found = 0;

        for (size_t j = (size_t)i + 1u; j < weight; ++j) {
            found |= compare_u32(support[j], support[i]);
        }

        uint32_t mask = 0u - found;
        support[i] = (mask & (uint32_t)i) ^ (~mask & support[i]);
    }
}

void vect_write_support_to_vector(uint64_t *v, uint32_t *support, uint16_t weight) {
    uint32_t index_tab[PARAM_OMEGA_MAX] = {0};
    uint64_t bit_tab[PARAM_OMEGA_MAX] = {0};

    for (size_t i = 0; i < weight; i++) {
        index_tab[i] = support[i] >> 6;
        int32_t pos = (int32_t)(support[i] & 0x3fU);
        bit_tab[i] = UINT64_C(1) << pos;
    }

    /*
     * Public-address SVE mask scan.  The loop scans vector words in ascending
     * public order and uses secret support values only as comparison operands,
     * never as memory-write indices.
     */
    const uint64_t vl = svcntd();
    for (uint64_t i = 0; i < VEC_N_SIZE_64; i += vl) {
        svbool_t pg = svwhilelt_b64(i, (uint64_t)VEC_N_SIZE_64);
        svuint64_t lanes = svindex_u64(i, 1);
        svuint64_t val = svdup_n_u64(0);

        for (uint32_t j = 0; j < weight; j++) {
            svbool_t eq = svcmpeq_u64(pg, lanes, svdup_n_u64((uint64_t)index_tab[j]));
            svuint64_t bit = svdup_n_u64(bit_tab[j]);
            val = svorr_u64_x(pg, val, svand_u64_z(eq, bit, bit));
        }

        svuint64_t old = svld1_u64(pg, v + i);
        svst1_u64(pg, v + i, svorr_u64_x(pg, old, val));
    }
}

void vect_sample_fixed_weight1(shake256_xof_ctx *ctx, uint64_t *v, uint16_t weight) {
    uint32_t support[PARAM_OMEGA_MAX] = {0};
    vect_generate_random_support1(ctx, support, weight);
    vect_write_support_to_vector(v, support, weight);
}

void vect_sample_fixed_weight2(shake256_xof_ctx *ctx, uint64_t *v, uint16_t weight) {
    uint32_t support[PARAM_OMEGA_MAX] = {0};
    vect_generate_random_support2(ctx, support, weight);
    vect_write_support_to_vector(v, support, weight);
}

void vect_set_random(shake256_xof_ctx *ctx, uint64_t *v) {
    xof_get_bytes(ctx, (uint8_t *)v, VEC_N_SIZE_BYTES);
    v[VEC_N_SIZE_64 - 1] &= BITMASK(PARAM_N, 64);
}

void vect_add(uint64_t *o, const uint64_t *v1, const uint64_t *v2, uint32_t size) {
    uint64_t i = 0;
    const uint64_t vl = svcntd();

    for (; i < size; i += vl) {
        svbool_t pg = svwhilelt_b64(i, (uint64_t)size);
        svuint64_t a = svld1_u64(pg, v1 + i);
        svuint64_t b = svld1_u64(pg, v2 + i);
        svst1_u64(pg, o + i, sveor_u64_x(pg, a, b));
    }
}

uint8_t vect_compare(const uint8_t *v1, const uint8_t *v2, uint32_t size) {
    uint64_t i = 0;
    const uint64_t vl = svcntb();
    svuint8_t accv = svdup_n_u8(0);

    for (; i < size; i += vl) {
        svbool_t pg = svwhilelt_b8(i, (uint64_t)size);
        svuint8_t a = svld1_u8(pg, v1 + i);
        svuint8_t b = svld1_u8(pg, v2 + i);
        /* Inactive lanes are zeroed before the full-vector OR reduction below. */
        svuint8_t d = sveor_u8_z(pg, a, b);
        accv = svorr_u8_x(svptrue_b8(), accv, d);
    }

    uint8_t acc = svorv_u8(svptrue_b8(), accv);
    return (uint8_t)(((uint16_t)acc | (uint16_t)(0u - (uint16_t)acc)) >> 15);
}

void vect_truncate(uint64_t *v) {
    size_t orig_words = (PARAM_N + 63u) / 64u;
    size_t new_full_words = PARAM_N1N2 / 64u;
    size_t remaining_bits = PARAM_N1N2 % 64u;

    if (remaining_bits > 0) {
        uint64_t mask = (UINT64_C(1) << remaining_bits) - 1u;
        v[new_full_words] &= mask;
        new_full_words++;
    }

    size_t i = new_full_words;
    const uint64_t vl = svcntd();
    svuint64_t z = svdup_n_u64(0);
    for (; i < orig_words; i += vl) {
        svbool_t pg = svwhilelt_b64((uint64_t)i, (uint64_t)orig_words);
        svst1_u64(pg, v + i, z);
    }
}

void vect_print(const uint64_t *v, const uint32_t size) {
    if (size == VEC_K_SIZE_BYTES) {
        uint8_t tmp[VEC_K_SIZE_BYTES] = {0};
        memcpy(tmp, v, VEC_K_SIZE_BYTES);
        for (uint32_t i = 0; i < VEC_K_SIZE_BYTES; ++i) {
            printf("%02x", tmp[i]);
        }
    } else if (size == VEC_N_SIZE_BYTES) {
        uint8_t tmp[VEC_N_SIZE_BYTES] = {0};
        memcpy(tmp, v, VEC_N_SIZE_BYTES);
        for (uint32_t i = 0; i < VEC_N_SIZE_BYTES; ++i) {
            printf("%02x", tmp[i]);
        }
    } else if (size == VEC_N1N2_SIZE_BYTES) {
        uint8_t tmp[VEC_N1N2_SIZE_BYTES] = {0};
        memcpy(tmp, v, VEC_N1N2_SIZE_BYTES);
        for (uint32_t i = 0; i < VEC_N1N2_SIZE_BYTES; ++i) {
            printf("%02x", tmp[i]);
        }
    } else if (size == VEC_N1_SIZE_BYTES) {
        uint8_t tmp[VEC_N1_SIZE_BYTES] = {0};
        memcpy(tmp, v, VEC_N1_SIZE_BYTES);
        for (uint32_t i = 0; i < VEC_N1_SIZE_BYTES; ++i) {
            printf("%02x", tmp[i]);
        }
    }
}
