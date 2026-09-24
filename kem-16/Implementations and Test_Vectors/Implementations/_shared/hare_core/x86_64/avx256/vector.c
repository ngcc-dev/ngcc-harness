/**
 * @file vector.c
 * @brief x86_64/AVX2 implementation of vector utilities for HARE optimized targets.
 *
 * This file is derived from the current reference vector.c and keeps the same
 * public semantics. Only low-risk vector primitives are AVX2-specialized;
 * sampling and printing remain reference-equivalent C code.
 */

#include "vector.h"
#include <immintrin.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "parameters.h"

static inline uint32_t compare_u32(const uint32_t v1, const uint32_t v2);

/**
 * @brief Constant-time comparison of two integers v1 and v2
 *
 * Returns 1 if v1 is equal to v2 and 0 otherwise
 * https://gist.github.com/sneves/10845247
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
 * @param[in,out] ctx     SHAKE256 XOF context used for random byte generation.
 * @param[out]    support Output array to store the `weight` unique indices.
 * @param[in]     weight  Desired Hamming weight.
 */
void vect_generate_random_support1(shake256_xof_ctx *ctx, uint32_t *support, uint16_t weight) {
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
 * @param[in,out] ctx     Initialized SHAKE256 XOF context used for randomness.
 * @param[out]    support Output array of unique indices (the support set).
 * @param[in]     weight  Number of elements to generate (Hamming weight).
 */
void vect_generate_random_support2(shake256_xof_ctx *ctx, uint32_t *support, uint16_t weight) {
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

#if defined(__AVX2__)
    /*
     * AVX2 mask-scan support write.  This keeps the reference side-channel
     * discipline: all public vector words are visited in order and the secret
     * support positions are used only in lane comparisons and masks, never as
     * direct memory-write indices.
     */
    uint32_t i = 0;
    for (; i + 4u <= VEC_N_SIZE_64; i += 4u) {
        const __m256i word_idx = _mm256_set_epi64x((long long)(i + 3u),
                                                  (long long)(i + 2u),
                                                  (long long)(i + 1u),
                                                  (long long)i);
        __m256i val = _mm256_setzero_si256();

        for (uint32_t j = 0; j < weight; j++) {
            const __m256i idx = _mm256_set1_epi64x((long long)index_tab[j]);
            const __m256i bit = _mm256_set1_epi64x((long long)bit_tab[j]);
            const __m256i mask = _mm256_cmpeq_epi64(word_idx, idx);
            val = _mm256_or_si256(val, _mm256_and_si256(bit, mask));
        }

        const __m256i oldv = _mm256_loadu_si256((const __m256i *)(const void *)(v + i));
        _mm256_storeu_si256((__m256i *)(void *)(v + i), _mm256_or_si256(oldv, val));
    }

    for (; i < VEC_N_SIZE_64; i++) {
        uint64_t val = 0;
        for (uint32_t j = 0; j < weight; j++) {
            uint32_t tmp = i - index_tab[j];
            int val1 = 1 ^ ((tmp | -tmp) >> 31);
            uint64_t mask = UINT64_C(0) - (uint64_t)(uint32_t)val1;
            val |= (bit_tab[j] & mask);
        }
        v[i] |= val;
    }
#else
    for (uint32_t i = 0; i < VEC_N_SIZE_64; i++) {
        uint64_t val = 0;
        for (uint32_t j = 0; j < weight; j++) {
            uint32_t tmp = i - index_tab[j];
            int val1 = 1 ^ ((tmp | -tmp) >> 31);
            uint64_t mask = UINT64_C(0) - (uint64_t)(uint32_t)val1;
            val |= (bit_tab[j] & mask);
        }
        v[i] |= val;
    }
#endif
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
 * @param[in,out] ctx     Pointer to a previously initialized SHAKE-256 XOF context.
 * @param[out]    v       Pointer to an array of ⌈PARAM_N/64⌉ 64-bit words.
 *                        On return, **v** is a bitmask with exactly `weight`
 *                        bits set to 1.
 * @param[in]     weight  Desired Hamming weight.
 */
void vect_sample_fixed_weight1(shake256_xof_ctx *ctx, uint64_t *v, uint16_t weight) {
    uint32_t support[PARAM_OMEGA_MAX] = {0};
    vect_generate_random_support1(ctx, support, weight);
    vect_write_support_to_vector(v, support, weight);
}

/**
 * @brief Generate a random binary vector of fixed Hamming weight.
 *
 * Implementation of Algorithm 5 in https://eprint.iacr.org/2021/1631.pdf
 * This sampling procedure is used **exclusively during encryption** to generate
 * the vectors **r1**, **r2**, and **e**.
 *
 * @param[in,out] ctx     Pointer to a previously initialized SHAKE-256 XOF context.
 * @param[out]    v       Pointer to an array of ⌈PARAM_N/64⌉ 64-bit words.
 *                        On return, **v** is a mask with exactly **weight**
 *                        bits set to 1.
 * @param[in]     weight  Desired Hamming weight.
 */
void vect_sample_fixed_weight2(shake256_xof_ctx *ctx, uint64_t *v, uint16_t weight) {
    uint32_t support[PARAM_OMEGA_MAX] = {0};
    vect_generate_random_support2(ctx, support, weight);
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
void vect_set_random(shake256_xof_ctx *ctx, uint64_t *v) {
    xof_get_bytes(ctx, (uint8_t *)v, VEC_N_SIZE_BYTES);
    v[VEC_N_SIZE_64 - 1] &= BITMASK(PARAM_N, 64);
}

/**
 * @brief Adds two vectors using AVX2 when available.
 *
 * The operation is the same as the reference implementation: o[i] = v1[i] ^ v2[i]
 * for size 64-bit words. The tail loop keeps the exact reference behavior for
 * non-multiple-of-four sizes.
 */
void vect_add(uint64_t *o, const uint64_t *v1, const uint64_t *v2, uint32_t size) {
#if defined(__AVX2__)
    uint32_t i = 0;
    const uint32_t blocks = size / 4u;

    for (uint32_t b = 0; b < blocks; ++b) {
        const __m256i a = _mm256_loadu_si256((const __m256i *)(const void *)(v1 + i));
        const __m256i c = _mm256_loadu_si256((const __m256i *)(const void *)(v2 + i));
        const __m256i r = _mm256_xor_si256(a, c);
        _mm256_storeu_si256((__m256i *)(void *)(o + i), r);
        i += 4u;
    }

    for (; i < size; ++i) {
        o[i] = v1[i] ^ v2[i];
    }
#else
    for (uint32_t i = 0; i < size; ++i) {
        o[i] = v1[i] ^ v2[i];
    }
#endif
}

/**
 * @brief Compares two byte strings in full length using AVX2 when available.
 *
 * Returns 0 when equal and 1 otherwise. The loop always scans every supplied
 * byte. This preserves the reference implementation's full-length comparison
 * contract used by the FO implicit rejection path.
 */
uint8_t vect_compare(const uint8_t *v1, const uint8_t *v2, uint32_t size) {
#if defined(__AVX2__)
    uint32_t i = 0;
    uint8_t acc = 0u;
    __m256i vacc = _mm256_setzero_si256();

    for (; i + 32u <= size; i += 32u) {
        const __m256i a = _mm256_loadu_si256((const __m256i *)(const void *)(v1 + i));
        const __m256i b = _mm256_loadu_si256((const __m256i *)(const void *)(v2 + i));
        vacc = _mm256_or_si256(vacc, _mm256_xor_si256(a, b));
    }

    {
        uint8_t tmp[32];
        _mm256_storeu_si256((__m256i *)(void *)tmp, vacc);
        for (uint32_t j = 0; j < 32u; ++j) {
            acc |= tmp[j];
        }
    }

    for (; i < size; ++i) {
        acc |= (uint8_t)(v1[i] ^ v2[i]);
    }

    return (uint8_t)(((uint16_t)acc | (uint16_t)(0u - (uint16_t)acc)) >> 15);
#else
    uint16_t r = 0x0100;

    for (size_t i = 0; i < size; i++) {
        r |= v1[i] ^ v2[i];
    }

    return (r - 1) >> 8;
#endif
}

/**
 * Truncate the bit-array v in-place to PARAM_N1N2 bits,
 * zeroing out all bits beyond that.
 *
 * This is the same semantic operation as the reference implementation. The
 * zeroing tail is vectorized when AVX2 is available.
 */
void vect_truncate(uint64_t *v) {
    size_t orig_words = (PARAM_N + 63) / 64;
    size_t new_full_words = PARAM_N1N2 / 64;
    size_t remaining_bits = PARAM_N1N2 % 64;

    if (remaining_bits > 0) {
        uint64_t mask = (UINT64_C(1) << remaining_bits) - 1;
        v[new_full_words] &= mask;
        new_full_words++;
    }

#if defined(__AVX2__)
    {
        const __m256i z = _mm256_setzero_si256();
        size_t i = new_full_words;
        for (; i + 4u <= orig_words; i += 4u) {
            _mm256_storeu_si256((__m256i *)(void *)(v + i), z);
        }
        for (; i < orig_words; i++) {
            v[i] = 0;
        }
    }
#else
    for (size_t i = new_full_words; i < orig_words; i++) {
        v[i] = 0;
    }
#endif
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
