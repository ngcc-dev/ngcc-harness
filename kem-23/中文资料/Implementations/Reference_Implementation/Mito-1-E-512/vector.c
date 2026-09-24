/**
 * @file vector.c
 * @brief Implementation of vectors sampling and some utilities for the MITO scheme
 */

#include <stdlib.h>
#include "vector.h"

static const uint32_t PARAM_N_MU = ((1LL << 32) / PARAM_N);  ///<  Define a precomputed multiplier for Barrett reduction mu = floor(2^32 / PARAM_N)
static const uint32_t UTILS_REJECTION_THRESHOLD = (((1LL << 24) / PARAM_N) * PARAM_N);  ///< Rejection threshold for uniform sampling in [0, PARAM_N)


/**
 * @brief Constant-time Barrett reduction modulo PARAM_N.
 *
 * Reduces \p x modulo PARAM_N using the precomputed value PARAM_N_MU = ⌊2^32 / PARAM_N⌋.
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
 * Internally, it samples 24-bit random values and rejects any value ≥ UTILS_REJECTION_THRESHOLD,
 * where the threshold is precomputed as:
 * \f[
 * t = \left\lfloor \frac{2^{24}}{\text{PARAM\_N}} \right\rfloor \times \text{PARAM\_N}
 * \f]
 *
 * @param[in,out] ctx     XOF context used for random byte generation.
 * @param[out]    support Output array to store the `weight` unique indices.
 * @param[in]     weight  Desired Hamming weight.
 */
void vect_generate_random_support(DRNG_ctx* ctx, uint32_t* support, int weight) {
    uint8_t rand_bytes[3];
    uint32_t candidate = 0;

    for (size_t i = 0; i < weight;) {
        xof_get_bytes(ctx, rand_bytes, 3);
        candidate = rand_bytes[0] | (rand_bytes[1] << 8) | (rand_bytes[2] << 16);

        if (candidate >= UTILS_REJECTION_THRESHOLD) {
            continue;
        }
        candidate = barrett_reduce(candidate);

        int is_position_available = 1;
        for (size_t j = 0; j < i; j++) {
            if (candidate == support[j]) {
                is_position_available = 0;
                break;
            }
        }

        if (is_position_available == 1) {
            support[i] = candidate;
            i++;
        }
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
static void vect_write_support_to_vector(uint64_t v[][VEC_N_SIZE_64], uint32_t* support, const int* weight, int dim) {
    int cnt = 0, pos, k, i, j;
    uint32_t** index_tab = (uint32_t**)malloc(dim * sizeof(uint32_t*));
    uint64_t** bit_tab = (uint64_t**)malloc(dim * sizeof(uint64_t*));
    for (k = 0; k < dim; k++) {
        index_tab[k] = (uint32_t*)malloc(weight[k] * sizeof(uint32_t));
        bit_tab[k] = (uint64_t*)malloc(weight[k] * sizeof(uint64_t));
        for (i = 0; i < weight[k]; i++) {
            index_tab[k][i] = support[cnt + i] >> 6;
            pos = support[cnt + i] & 0x3f;
            bit_tab[k][i] = 1ULL << pos;
        }
        cnt += weight[k];
    }

    uint64_t val = 0;
    for (k = 0; k < dim; k++) {
        for (i = 0; i < VEC_N_SIZE_64; i++) {
            val = 0;
            for (j = 0; j < weight[k]; j++) {
                uint32_t tmp = i - index_tab[k][j];
                int val1 = 1 ^ ((tmp | -tmp) >> 31);
                uint64_t mask = -val1;
                val |= (bit_tab[k][j] & mask);
            }
            v[k][i] |= val;
        }
        free(bit_tab[k]);
        free(index_tab[k]);
    }
    free(bit_tab);
    free(index_tab);
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
 * @param[in,out] ctx     Pointer to a previously initialized XOF context.
 * @param[out]    v       Pointer to an array of ⌈PARAM_N/64⌉ 64-bit words.
 *                        On return, **v** is a bitmask with exactly `weight`
 *                        bits set to 1.
 * @param[in]     weight  Desired Hamming weight.
 */
void vect_sample_fixed_weight(DRNG_ctx* ctx, uint64_t v[][VEC_N_SIZE_64], const int* weight, int dim) {
    int i, total_weight = 0;

    for (i = 0; i < dim; i++)
        total_weight += weight[i];
    uint32_t* support = (uint32_t*)malloc(total_weight * sizeof(uint32_t));
    vect_generate_random_support(ctx, support, total_weight);
    vect_write_support_to_vector(v, support, weight, dim);
    free(support);
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
void vect_set_random(DRNG_ctx* ctx, uint64_t* v) {
    xof_get_bytes(ctx, (uint8_t*)v, VEC_N_SIZE_BYTES);
    v[VEC_N_SIZE_64 - 1] &= BITMASK;
}

/**
 * @brief Adds two vectors
 *
 * @param[out] o Pointer to an array that is the result
 * @param[in] v1 Pointer to an array that is the first vector
 * @param[in] v2 Pointer to an array that is the second vector
 * @param[in] size Integer that is the size of the vectors
 */
void vect_add(uint64_t* o, const uint64_t* v1, const uint64_t* v2, uint32_t size) {
    for (uint32_t i = 0; i < size; ++i) {
        o[i] = v1[i] ^ v2[i];
    }
}

/**
 * @brief Compares two vectors
 *
 * Function borrowed from liboqs:
 * https://github.com/open-quantum-safe/liboqs/commit/cce1bfde4e52c524b087b9687020d283fbde0f24
 *
 * @param[in] v1 Pointer to an array that is first vector
 * @param[in] v2 Pointer to an array that is second vector
 * @param[in] size Integer that is the size of the vectors
 * @returns 0 if the vectors are equals and 1 otherwise
 */
uint8_t vect_compare(const uint8_t* v1, const uint8_t* v2, uint32_t size) {
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
void vect_truncate(uint64_t* v) {
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
