/**
 * @file vector.c
 * @brief Implementation of vectors sampling and some utilities for the TriQ scheme
 */

#include "vector.h"
#include <immintrin.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "parameters.h"

/**
 * @brief Number of 256-bit chunks needed to cover PARAM_N bits.
 *
 * Computes ⌈PARAM_N / 256�? i.e. how many 256-bit “loop iterations�?are required
 * to process PARAM_N elements in blocks of 256.
 */
#define LOOP_SIZE CEIL_DIVIDE(PARAM_N, 256)

static inline uint32_t compare_u32(const uint32_t v1, const uint32_t v2);

/**
 * @brief Constant-time comparison of two integers v1 and v2
 *
 * Returns 1 if v1 is equal to v2 and 0 otherwise
 *
 * @param[in] v1
 * @param[in] v2
 */
static inline uint32_t compare_u32(const uint32_t v1, const uint32_t v2) {
    return 1 ^ (((v1 - v2) | (v2 - v1)) >> 31);
}

/**
 * @brief Constant-time Barrett reduction modulo PARAM_N.
 *
 * Reduces \p x modulo PARAM_N using the precomputed value PARAM_N_MU = �?^32 / PARAM_N�?
 *
 * @param[in] x Input value to reduce.
 * @return x mod PARAM_N in constant time.
 */
static inline uint32_t barrett_reduce(uint32_t x) {
    uint64_t q = ((uint64_t)x * PARAM_N_MU) >> 32;
    uint32_t r = x - (uint32_t)(q * PARAM_N);

    uint32_t reduce_flag = (((r - PARAM_N) >> 31) ^ 1);
    uint32_t mask = -reduce_flag;
    r -= mask & PARAM_N;
    return r;
}

/**
 * @brief Generates a random support set with uniform and unbiased sampling.
 *
 * This function implements a rejection sampling algorithm to generate `weight`
 * distinct indices uniformly at random from the interval [0, PARAM_N).
 * It ensures that the output is non-biased and the values are uniformly distributed.
 *
 * Internally, it samples 24-bit random values and rejects any value �?UTILS_REJECTION_THRESHOLD,
 * where the threshold is precomputed as:
 * \f[
 * t = \left\lfloor \frac{2^{24}}{\text{PARAM\_N}} \right\rfloor \times \text{PARAM\_N}
 * \f]
 *
 * @param[in,out] ctx     API_PKC XOF context used for random byte generation.
 * @param[out]    support Output array to store the `weight` unique indices.
 * @param[in]     weight  Desired Hamming weight.
 */
void vect_generate_random_support1(triq_xof_ctx *ctx, uint32_t *support, uint16_t weight) {
    size_t random_bytes_size = 3 * weight;
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

/**
 * @brief Generates a random support set of distinct indices.
 *
 * This implements the **GenerateRandomSupport** algorithm from the specification.
 *
 * @param[in,out] ctx     Initialized API_PKC XOF context used for randomness.
 * @param[out]    support Output array of unique indices (the support set).
 * @param[in]     weight  Number of elements to generate (Hamming weight).
 */
void vect_generate_random_support2(triq_xof_ctx *ctx, uint32_t *support, uint16_t weight) {
    uint32_t rand_u32[PARAM_OMEGA_MAX] = {0};

    xof_get_bytes(ctx, (uint8_t *)&rand_u32, 4 * weight);

    for (size_t i = 0; i < weight; ++i) {
        uint64_t buff = rand_u32[i];
        support[i] = i + ((buff * (PARAM_N - i)) >> 32);
    }

    for (int32_t i = (weight - 1); i-- > 0;) {
        uint32_t found = 0;

        for (size_t j = i + 1; j < weight; ++j) {
            found |= compare_u32(support[j], support[i]);
        }

        uint32_t mask = -found;
        support[i] = (mask & i) ^ (~mask & support[i]);
    }
}

/**
 * @brief Sets bits in a vector based on a support set.
 *
 * Writes `weight` positions from `support` into the bit-vector `v`.
 * Each index in `support` sets a corresponding bit in `v`.
 *
 * @param[out] v       Output vector.
 * @param[in]  support Array of bit indices to set.
 * @param[in]  weight  Number of positions to set.
 */
void vect_write_support_to_vector(__m256i *v, uint32_t *support, uint16_t weight) {
    __m256i bit256[PARAM_OMEGA_MAX];
    __m256i bloc256[PARAM_OMEGA_MAX];
    static __m256i posCmp256 = (__m256i){0UL, 1UL, 2UL, 3UL};

    for (uint32_t i = 0; i < weight; i++) {
        uint64_t bloc = support[i] >> 6;
        bloc256[i] = _mm256_set1_epi64x(bloc >> 2);
        uint64_t pos = (bloc & 0x3UL);
        __m256i pos256 = _mm256_set1_epi64x(pos);
        __m256i mask256 = _mm256_cmpeq_epi64(pos256, posCmp256);
        uint64_t bit64 = 1ULL << (support[i] & 0x3f);
        __m256i bl256 = _mm256_set1_epi64x(bit64);
        bit256[i] = bl256 & mask256;
    }

    for (uint32_t i = 0; i < LOOP_SIZE; i++) {
        __m256i aux = _mm256_setzero_si256();
        __m256i i256 = _mm256_set1_epi64x(i);

        for (uint32_t j = 0; j < weight; j++) {
            __m256i mask256 = _mm256_cmpeq_epi64(bloc256[j], i256);
            aux ^= bit256[j] & mask256;
        }

        _mm256_storeu_si256(&v[i], _mm256_xor_si256(v[i], aux));
    }
}

/**
 * @brief Generate a random binary vector of fixed Hamming weight.
 *
 * It samples a binary vector with exactly `weight` bits set to 1, where the
 * positions are chosen **uniformly at random** without bias.
 *
 * This sampling procedure is used **exclusively during key generation** to generate
 * the vectors **x** and **y**
 *
 * @param[in,out] ctx     Pointer to a previously initialized API_PKC XOF context.
 * @param[out]    v       Pointer to an array of ⌈PARAM_N/64�?64-bit words.
 *                        On return, **v** is a bitmask with exactly `weight`
 *                        bits set to 1.
 * @param[in]     weight  Desired Hamming weight.
 */
void vect_sample_fixed_weight1(triq_xof_ctx *ctx, __m256i *v, uint16_t weight) {
    uint32_t support[PARAM_OMEGA_MAX] = {0};
    vect_generate_random_support1(ctx, support, weight);
    vect_write_support_to_vector(v, support, weight);
}

/**
 * @brief Generate a random binary vector of fixed Hamming weight.
 *
 * Fixed-weight sampling routine.
 * This sampling procedure is used **exclusively during encryption** to generate
 * the vectors **r1**, **r2**, and **e**.
 *
 * @param[in,out] ctx     Pointer to a previously initialized API_PKC XOF context.
 * @param[out]    v       Pointer to an array of ⌈PARAM_N/64�?64-bit words.
 *                        On return, **v** is a mask with exactly **weight**
 *                        bits set to 1.
 * @param[in]     weight  Desired Hamming weight.
 */
void vect_sample_fixed_weight2(triq_xof_ctx *ctx, __m256i *v, uint16_t weight) {
    uint32_t support[PARAM_OMEGA_MAX] = {0};
    vect_generate_random_support2(ctx, support, weight);
    vect_write_support_to_vector(v, support, weight);
}

uint8_t vect_check_bounded_density(const uint32_t *support, uint16_t weight,
                                   uint16_t L, uint16_t gamma) {
    uint16_t max_seen = 0;
    for (uint16_t i = 0; i < weight; i++) {
        uint16_t count = 0;
        uint32_t start = support[i];
        for (uint16_t j = 0; j < weight; j++) {
            uint32_t dist = (support[j] >= start)
                ? (support[j] - start)
                : (PARAM_N - start + support[j]);
            count += (uint16_t)(dist < (uint32_t)L);
        }
        uint16_t diff = count ^ max_seen;
        uint16_t mask = (uint16_t)(-(int16_t)(count > max_seen));
        max_seen ^= diff & mask;
    }
    uint16_t flag = (uint16_t)(-(int16_t)(max_seen > gamma));
    return (uint8_t)(flag & 1);
}

void vect_sample_fixed_weight_bd(triq_xof_ctx *ctx, __m256i *v,
                                 uint16_t weight, uint16_t L, uint16_t gamma,
                                 uint16_t N_max) {
    uint32_t support[PARAM_OMEGA_MAX] = {0};
    uint16_t iter;
    for (iter = 0; iter < N_max; iter++) {
        vect_generate_random_support2(ctx, support, weight);
        if (!vect_check_bounded_density(support, weight, L, gamma)) {
            break;
        }
    }
    vect_write_support_to_vector(v, support, weight);
}

/**
 * @brief Generates a random vector of dimension <b>PARAM_N</b>
 *
 * This function generates a random binary vector of dimension <b>PARAM_N</b>. It generates a random
 * array of bytes using the xof, and drop the extra bits using a mask.
 *
 * @param[in] ctx Pointer to the context of the xof
 * @param[in] v Pointer to an array
 */
void vect_set_random(triq_xof_ctx *ctx, uint64_t *v) {
    xof_get_bytes(ctx, (uint8_t *)v, VEC_N_SIZE_BYTES);
    v[VEC_N_SIZE_64 - 1] &= BITMASK(PARAM_N, 64);
}

/**
 * @brief Adds two vectors
 *
 * @param[out] o Pointer to an array that is the result
 * @param[in] v1 Pointer to an array that is the first vector
 * @param[in] v2 Pointer to an array that is the second vector
 * @param[in] size Integer that is the size of the vectors
 */
void vect_add(uint64_t *o, const uint64_t *v1, const uint64_t *v2, uint32_t size) {
    for (uint32_t i = 0; i < size; ++i) {
        o[i] = v1[i] ^ v2[i];
    }
}

/**
 * @brief Compares two vectors
 *
 * Constant-time vector comparison.
 *
 * @param[in] v1 Pointer to an array that is first vector
 * @param[in] v2 Pointer to an array that is second vector
 * @param[in] size Integer that is the size of the vectors
 * @returns 0 if the vectors are equals and 1 otherwise
 */
uint8_t vect_compare(const uint8_t *v1, const uint8_t *v2, uint32_t size) {
    uint16_t r = 0x0100;

    for (size_t i = 0; i < size; i++) {
        r |= v1[i] ^ v2[i];
    }

    return (r - 1) >> 8;
}

/**
 * Truncate the bit-array v in-place to PARAM_N1N2 bits,
 * zeroing out all bits beyond that.
 *
 * @param[in,out] v         Pointer to the uint64_t array containing the bits.
 */
void vect_truncate(uint64_t *v) {
    size_t orig_words = (PARAM_N + 63) / 64;
    size_t new_full_words = PARAM_N1N2 / 64;
    size_t remaining_bits = PARAM_N1N2 % 64;

    // Mask the last word if there's a partial word
    if (remaining_bits > 0) {
        uint64_t mask = (UINT64_C(1) << remaining_bits) - 1;
        v[new_full_words] &= mask;
        new_full_words++;  // keep that partial word
    }

    // Zero out all subsequent words up to the original length
    for (size_t i = new_full_words; i < orig_words; i++) {
        v[i] = 0;
    }
}

/**
 * @brief Prints a given number of bytes
 *
 * @param[in] v Pointer to an array of bytes
 * @param[in] size Integer that is number of bytes to be displayed
 */
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

