/**
 * @file reed_muller.c
 * @brief x86_64/AVX2 implementation of HARE duplicated RM(1,7) coding.
 *
 * This file keeps the current HARE reference semantics exactly:
 *   - reed_muller_encode(cdw, msg)
 *   - reed_muller_decode(msg, erasures, cdw)
 *
 * The decoder still emits both the decoded message byte and the erasure flag
 * required by HARE's distance-informed RM/RS concatenated decoder. It does not
 * use a non-erasure RM interface.
 *
 * Optimized parts:
 *   - AVX2 Hadamard transform over 128 int16 lanes.
 *   - AVX2-friendly 128-bit RM codeword expansion layout.
 *
 * The peak/second-peak extraction is scalar, branchless, and byte-for-byte
 * equivalent to the current reference decision rule. This avoids changing the
 * erasure threshold semantics while still accelerating the dominant Hadamard
 * transform.
 */

#include "reed_muller.h"
#include <immintrin.h>
#include <stdint.h>
#include <string.h>
#include "data_structures.h"
#include "parameters.h"

#define MULTIPLICITY CEIL_DIVIDE(PARAM_N2, 128)

#define BIT0MASK(x) (int32_t)(-((x) & 1))

typedef union {
    uint8_t u8[16];
    uint16_t u16[8];
    uint32_t u32[4];
} rm_codeword128_t;

typedef union {
    __m256i mm[8];
    int16_t i16[128];
} rm_expanded_cdw128_t;

/**
 * @brief Return an all-ones mask when a > b, otherwise zero.
 *
 * RM Hadamard magnitudes are small enough that the unsigned subtraction stays
 * below the 2^31 wrap boundary. This keeps peak extraction branchless without
 * changing the reference threshold semantics.
 */
static inline uint32_t ct_mask_gt_u32(uint32_t a, uint32_t b) {
    return 0u - (((b - a) >> 31) & 1u);
}

/**
 * @brief Select x when mask is all ones, otherwise y.
 */
static inline uint32_t ct_select_u32(uint32_t mask, uint32_t x, uint32_t y) {
    return (mask & x) | (~mask & y);
}

/**
 * @brief Signed 32-bit wrapper around ct_select_u32().
 */
static inline int32_t ct_select_i32(uint32_t mask, int32_t x, int32_t y) {
    return (int32_t)ct_select_u32(mask, (uint32_t)x, (uint32_t)y);
}

/**
 * @brief Compute |x| without a data-dependent branch.
 */
static inline uint32_t ct_abs_i32(int32_t x) {
    const uint32_t ux = (uint32_t)x;
    const uint32_t sign = ux >> 31;
    const uint32_t mask = 0u - sign;
    return (ux ^ mask) + sign;
}

/**
 * @brief Return 1 when x > 0, otherwise 0, without branching.
 */
static inline uint32_t ct_positive_bit_i32(int32_t x) {
    const uint32_t ux = (uint32_t)x;
    const uint32_t nonzero = (ux | (0u - ux)) >> 31;
    const uint32_t sign = ux >> 31;
    return nonzero & (sign ^ 1u);
}
/**
 * Encode one byte into the 128-bit duplicated Reed-Muller representation.
 */
static void rm_encode_byte(rm_codeword128_t *word, int32_t message) {
    int32_t first_word = BIT0MASK(message >> 7);

    first_word ^= BIT0MASK(message >> 0) & 0xaaaaaaaa;
    first_word ^= BIT0MASK(message >> 1) & 0xcccccccc;
    first_word ^= BIT0MASK(message >> 2) & 0xf0f0f0f0;
    first_word ^= BIT0MASK(message >> 3) & 0xff00ff00;
    first_word ^= BIT0MASK(message >> 4) & 0xffff0000;

    word->u32[0] = (uint32_t)first_word;

    first_word ^= BIT0MASK(message >> 5);
    word->u32[1] = (uint32_t)first_word;
    first_word ^= BIT0MASK(message >> 6);
    word->u32[3] = (uint32_t)first_word;
    first_word ^= BIT0MASK(message >> 5);
    word->u32[2] = (uint32_t)first_word;
}

/**
 * Use AVX2 additions to accumulate duplicated RM block evidence.
 */
static void expand_and_sum_avx(rm_expanded_cdw128_t *dst, const rm_codeword128_t src[]) {
    for (size_t part = 0; part < 8; part++) {
        const uint16_t w = src[0].u16[part];
        for (size_t bit = 0; bit < 16; bit++) {
            dst->i16[(part << 4) + bit] = (int16_t)((w >> bit) & 1U);
        }
    }

    for (size_t copy = 1; copy < MULTIPLICITY; copy++) {
        for (size_t part = 0; part < 8; part++) {
            const uint16_t w = src[copy].u16[part];
            for (size_t bit = 0; bit < 16; bit++) {
                dst->i16[(part << 4) + bit] += (int16_t)((w >> bit) & 1U);
            }
        }
    }
}

/**
 * Run the AVX2 Hadamard transform used by the optimized RM decoder.
 */
static void hadamard_avx(rm_expanded_cdw128_t *src, rm_expanded_cdw128_t *dst) {
    rm_expanded_cdw128_t *p1 = src;
    rm_expanded_cdw128_t *p2 = dst;

    for (size_t pass = 0; pass < 7; pass++) {
        for (size_t part = 0; part < 4; part++) {
            const __m256i a = p1->mm[2 * part];
            const __m256i b = p1->mm[2 * part + 1];
            p2->mm[part] = _mm256_permute4x64_epi64(_mm256_hadd_epi16(a, b), 0xd8);
            p2->mm[part + 4] = _mm256_permute4x64_epi64(_mm256_hsub_epi16(a, b), 0xd8);
        }

        rm_expanded_cdw128_t *tmp = p1;
        p1 = p2;
        p2 = tmp;
    }
}

/**
 * Return best RM byte or erasure while preserving reference threshold semantics.
 */
static int32_t find_peak_and_second_peak_ref_semantics(const rm_expanded_cdw128_t *transform, uint64_t threshold) {
    uint32_t peak_abs_value = 0;
    int32_t peak_value = 0;
    uint32_t peak_pos = 0;

    uint32_t second_peak_abs_value = 0;
    int32_t second_peak_value = 0;
    uint32_t second_peak_pos = 0;

    for (int32_t i = 0; i < 128; i++) {
        const int32_t t = transform->i16[i];
        const uint32_t absolute = ct_abs_i32(t);

        const uint32_t update_peak = ct_mask_gt_u32(absolute, peak_abs_value);
        const uint32_t update_second_from_current = ~update_peak & ct_mask_gt_u32(absolute, second_peak_abs_value);

        second_peak_abs_value = ct_select_u32(update_peak, peak_abs_value, second_peak_abs_value);
        second_peak_value = ct_select_i32(update_peak, peak_value, second_peak_value);
        second_peak_pos = ct_select_u32(update_peak, peak_pos, second_peak_pos);

        peak_abs_value = ct_select_u32(update_peak, absolute, peak_abs_value);
        peak_value = ct_select_i32(update_peak, t, peak_value);
        peak_pos = ct_select_u32(update_peak, (uint32_t)i, peak_pos);

        second_peak_abs_value = ct_select_u32(update_second_from_current, absolute, second_peak_abs_value);
        second_peak_value = ct_select_i32(update_second_from_current, t, second_peak_value);
        second_peak_pos = ct_select_u32(update_second_from_current, (uint32_t)i, second_peak_pos);
    }

    (void)second_peak_value;
    (void)second_peak_pos;

    const uint32_t abs_diff = peak_abs_value - second_peak_abs_value;
    peak_pos |= ct_positive_bit_i32(peak_value) << 7;

    return ct_select_i32(ct_mask_gt_u32(abs_diff, (uint32_t)threshold), (int32_t)peak_pos, -1);
}
/**
 * Optimized x86 Reed-Muller encoder for HARE code blocks.
 */
void reed_muller_encode(uint64_t *cdw, const uint64_t *msg) {
    const uint8_t *message_array = (const uint8_t *)msg;
    rm_codeword128_t *code_array = (rm_codeword128_t *)cdw;

    for (size_t i = 0; i < VEC_N1_SIZE_BYTES; i++) {
        const size_t pos = i * MULTIPLICITY;
        rm_encode_byte(&code_array[pos], message_array[i]);
        for (size_t copy = 1; copy < MULTIPLICITY; copy++) {
            memcpy(&code_array[pos + copy], &code_array[pos], sizeof(rm_codeword128_t));
        }
    }
}

/**
 * Optimized x86 Reed-Muller decoder that emits erasure flags for RS decoding.
 */
void reed_muller_decode(uint64_t *msg, uint8_t *erasures, const uint64_t *cdw) {
    uint8_t *message_array = (uint8_t *)msg;
    const rm_codeword128_t *code_array = (const rm_codeword128_t *)cdw;

    for (size_t i = 0; i < VEC_N1_SIZE_BYTES; i++) {
        rm_expanded_cdw128_t expanded;
        rm_expanded_cdw128_t transform;

        expand_and_sum_avx(&expanded, &code_array[i * MULTIPLICITY]);
        hadamard_avx(&expanded, &transform);
        transform.i16[0] -= (int16_t)(64 * MULTIPLICITY);

        const int32_t peak = find_peak_and_second_peak_ref_semantics(&transform, PARAM_ALPHA);
        message_array[i] = (uint8_t)peak;
        erasures[i] = (uint8_t)(((uint32_t)peak >> 31) & 1u);
    }
}
