/**
 * @file vector.c
 * @brief Implementation of vectors sampling and some utilities for the TriQ scheme
 */

#include "vector.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "parameters.h"

static inline uint32_t compare_u32(const uint32_t v1, const uint32_t v2);

/**
 * @brief Branch-free equality test for two 32-bit integers
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
 * @brief Branch-free Barrett reduction modulo PARAM_N.
 *
 * Reduces \p x modulo PARAM_N using the precomputed value PARAM_N_MU = floor(2^32 / PARAM_N).
 *
 * @param[in] x Input value to reduce.
 * @return x mod PARAM_N.
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
 * @param[in,out] ctx     API_PKC XOF adapter context used for random byte generation.
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
 * @param[in,out] ctx     Initialized API_PKC XOF adapter context used for randomness.
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
void vect_write_support_to_vector(uint64_t *v, uint32_t *support, uint16_t weight) {
    uint32_t index_tab[PARAM_OMEGA_MAX] = {0};
    uint64_t bit_tab[PARAM_OMEGA_MAX] = {0};

    for (size_t i = 0; i < weight; i++) {
        index_tab[i] = support[i] >> 6;
        int32_t pos = support[i] & 0x3f;
        bit_tab[i] = ((uint64_t)1) << pos;
    }

    uint64_t val = 0;
    for (uint32_t i = 0; i < VEC_N_SIZE_64; i++) {
        val = 0;
        for (uint32_t j = 0; j < weight; j++) {
            uint32_t tmp = i - index_tab[j];
            int val1 = 1 ^ ((tmp | -tmp) >> 31);
            uint64_t mask = -val1;
            val |= (bit_tab[j] & mask);
        }
        v[i] |= val;
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
 * @param[in,out] ctx     Pointer to a previously initialized API_PKC XOF adapter context.
 * @param[out]    v       Pointer to an array of ⌈PARAM_N/64�?64-bit words.
 *                        On return, **v** is a bitmask with exactly `weight`
 *                        bits set to 1.
 * @param[in]     weight  Desired Hamming weight.
 */
void vect_sample_fixed_weight1(triq_xof_ctx *ctx, uint64_t *v, uint16_t weight) {
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
 * @param[in,out] ctx     Pointer to a previously initialized API_PKC XOF adapter context.
 * @param[out]    v       Pointer to an array of ⌈PARAM_N/64�?64-bit words.
 *                        On return, **v** is a mask with exactly **weight**
 *                        bits set to 1.
 * @param[in]     weight  Desired Hamming weight.
 */
void vect_sample_fixed_weight2(triq_xof_ctx *ctx, uint64_t *v, uint16_t weight) {
    uint32_t support[PARAM_OMEGA_MAX] = {0};
    vect_generate_random_support2(ctx, support, weight);
    vect_write_support_to_vector(v, support, weight);
}

/**
 * @brief Check whether a support set violates the bounded-density condition.
 *
 * Returns 1 (reject) if any cyclic window of length L contains more than gamma
 * support points; returns 0 (accept) otherwise.
 *
 * @param[in] support  Array of distinct indices in [0, n).
 * @param[in] weight   Number of support points.
 * @param[in] L        Window length.
 * @param[in] gamma    Density cap.
 */
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

/**
 * @brief Generate a random fixed-weight vector under bounded-density rejection.
 *
 * Repeatedly samples a support via vect_generate_random_support2 and accepts
 * it only if it passes the bounded-density check with window length L and cap
 * gamma.  At most N_max iterations are attempted; on exhaustion the last
 * candidate is kept to preserve deterministic behaviour given the XOF context.
 *
 * @param[in,out] ctx     API_PKC XOF adapter context.
 * @param[out]    v       Output vector.
 * @param[in]     weight  Desired Hamming weight.
 * @param[in]     L       Bounded-density window length.
 * @param[in]     gamma   Bounded-density cap.
 * @param[in]     N_max   Resampling bound.
 */
void vect_sample_fixed_weight_bd(triq_xof_ctx *ctx, uint64_t *v,
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
 * @brief Compares two vectors without secret-dependent early exits.
 *
 * @param[in] v1 Pointer to an array that is first vector
 * @param[in] v2 Pointer to an array that is second vector
 * @param[in] size Integer that is the size of the vectors
 * @returns 0 if the vectors are equal and 1 otherwise
 */
uint8_t vect_compare(const uint8_t *v1, const uint8_t *v2, uint32_t size) {
    uint16_t diff = 0;

    for (size_t i = 0; i < size; i++) {
        diff |= (uint16_t)(v1[i] ^ v2[i]);
    }

    return (uint8_t)((diff + 0xFFU) >> 8);
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
