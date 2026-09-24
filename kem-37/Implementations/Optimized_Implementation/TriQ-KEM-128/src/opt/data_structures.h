/**
 * @file    data_structures.h
 * @brief   TriQ PKE and KEM ciphertext data structures.
 */

#ifndef TRIQ_OPT_DATA_STRUCTURES_H
#define TRIQ_OPT_DATA_STRUCTURES_H

#include <stdint.h>
#include <x86intrin.h>
#include "parameters.h"

/**
 * @brief Public-key encryption ciphertext.
 *
 * - u: length VEC_N_256_SIZE_64 words
 * - v: length VEC_N1N2_256_SIZE_64 words
 */
typedef struct {
    uint64_t u[VEC_N_256_SIZE_64];    /**< first vector half */
    uint64_t v[VEC_N1N2_256_SIZE_64]; /**< second vector half */
} ciphertext_pke_t;

/**
 * @brief Key-encapsulation mechanism ciphertext.
 *
 * Wraps a PKE ciphertext along with the salt used in the KEM:
 * - c_pke: the ciphertext for PKE
 * - salt:  additional randomness (SALT_BYTES bytes)
 */
typedef struct {
    ciphertext_pke_t c_pke;   /**< embedded PKE ciphertext */
    uint8_t salt[SALT_BYTES]; /**< per-encapsulation salt */
} ciphertext_kem_t;

/**
 * @brief  a 128-bit reed鈥搈uller codeword, viewable as masks or integer arrays.
 *
 * This union allows interpreting a 128-bit codeword in three ways:
 *   - \c mask : as eight 16-bit predicate masks
 *   - \c u16  : as eight 16-bit unsigned integers
 *   - \c u32  : as four 32-bit unsigned integers
 */
typedef union {
    __mmask16 mask[8]; /**< eight 16-bit avx-512 predicate masks, one per lane */
    uint16_t u16[8];   /**< eight 16-bit unsigned integer views of the codeword */
    uint32_t u32[4];   /**< four 32-bit unsigned integer views of the codeword */
} rm_codeword128_t;

/**
 * @brief  a 256-bit reed鈥搈uller vector register, viewable as raw simd or 16-bit lanes.
 *
 * This union allows interpreting a 256-bit avx2 register in two ways:
 *   - \c mm  : as a single __m256i simd value
 *   - \c u16 : as sixteen 16-bit unsigned integers
 */
typedef union {
    __m256i mm;       /**< raw 256-bit avx2 simd register */
    uint16_t u16[16]; /**< sixteen 16-bit unsigned integer lanes */
} rm_vector256_t;

/**
 * @brief  an 鈥渆xpanded鈥?128-element reed鈥搈uller codeword, packed into eight avx2 registers.
 *
 * This union allows treating a 128-element array of int16_t in two ways:
 *   - \c mm  : as an array of eight __m256i simd registers (8脳32 B = 256 B total)
 *   - \c i16 : as a flat array of 128 signed 16-bit integers
 */
typedef union {
    __m256i mm[8];    /**< eight 256-bit avx2 simd registers (256 bytes total) */
    int16_t i16[128]; /**< flat array of 128 signed 16-bit integers */
} rm_expanded_cdw128_t;

#endif  // TRIQ_OPT_DATA_STRUCTURES_H

