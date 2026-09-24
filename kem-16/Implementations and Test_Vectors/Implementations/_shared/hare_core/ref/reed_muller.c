/**
 * @file reed_muller.c
 * @brief Constant time implementation of Reed-Muller code RM(1,7)
 */

#include "reed_muller.h"
#include <stdint.h>
#include <string.h>
#include "data_structures.h"
#include "parameters.h"

/**
 * @brief Number of repeated 128-bit codeword blocks.
 *
 * Calculates the ceiling of PARAM_N2/128 to determine how many
 * copies of each 128-bit codeword are used in the code expansion.
 */
#define MULTIPLICITY CEIL_DIVIDE(PARAM_N2, 128)

/**
 * @typedef rm_expanded_cdw
 * @brief Internal representation of a codeword with each bit expanded to a 16-bit signed value.
 */
typedef int16_t rm_expanded_cdw[128];

// clang-format off
/**
 * @def BIT0MASK(x)
 * @brief Broadcast the least significant bit of \p x to a 32-bit mask.
 *
 * @param x  An integer expression; only bit 0 is examined.
 * @return   A 32-bit value of all ones (if \p x&1 == 1) or all zeros (if \p x&1 == 0).
 */
#define BIT0MASK(x) (int32_t)(-((x) & 1))
// clang-format on

/**
 * @brief Return an all-ones mask when a > b, otherwise zero.
 *
 * The RM Hadamard-domain magnitudes are small public-width integers, so the
 * unsigned subtraction is safely below the 2^31 wrap boundary. The helper keeps
 * the peak-selection source free of data-dependent branches and conditional
 * operators.
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
 * Encode one byte into a duplicated Reed-Muller codeword in the reference path.
 */
void encode(rm_codeword_t *word, int32_t message);
/**
 * Perform the in-place-style Hadamard transform stage used by RM decoding.
 */
void hadamard(rm_expanded_cdw *src, rm_expanded_cdw *dst);
/**
 * Expand duplicated RM blocks and sum them for soft-decision decoding.
 */
void expand_and_sum(rm_expanded_cdw *dest, rm_codeword_t src[]);
/**
 * Return the best RM codeword index from a Hadamard-domain vector.
 */
int32_t find_peaks(rm_expanded_cdw *transform);
/**
 * Return the best RM codeword or erasure based on first/second peak gap.
 */
int32_t find_peak_and_second_peak(rm_expanded_cdw *transform, uint64_t T);

/**
 * @brief Encode a single byte into a single codeword using RM(1,7)
 *
 * Encoding matrix of this code:
 * bit pattern (note that bits are numbered big endian)
 * 0   aaaaaaaa aaaaaaaa aaaaaaaa aaaaaaaa
 * 1   cccccccc cccccccc cccccccc cccccccc
 * 2   f0f0f0f0 f0f0f0f0 f0f0f0f0 f0f0f0f0
 * 3   ff00ff00 ff00ff00 ff00ff00 ff00ff00
 * 4   ffff0000 ffff0000 ffff0000 ffff0000
 * 5   ffffffff 00000000 ffffffff 00000000
 * 6   ffffffff ffffffff 00000000 00000000
 * 7   ffffffff ffffffff ffffffff ffffffff
 *
 * @param[out] word An RM(1,7) codeword
 * @param[in] message A message
 */
void encode(rm_codeword_t *word, int32_t message) {
    int32_t first_word;

    first_word = BIT0MASK(message >> 7);

    first_word ^= BIT0MASK(message >> 0) & 0xaaaaaaaa;
    first_word ^= BIT0MASK(message >> 1) & 0xcccccccc;
    first_word ^= BIT0MASK(message >> 2) & 0xf0f0f0f0;
    first_word ^= BIT0MASK(message >> 3) & 0xff00ff00;
    first_word ^= BIT0MASK(message >> 4) & 0xffff0000;

    word->u32[0] = first_word;

    first_word ^= BIT0MASK(message >> 5);
    word->u32[1] = first_word;
    first_word ^= BIT0MASK(message >> 6);
    word->u32[3] = first_word;
    first_word ^= BIT0MASK(message >> 5);
    word->u32[2] = first_word;
    return;
}

/**
 * @brief Hadamard transform
 *
 * Perform hadamard transform of src and store result in dst
 * src is overwritten
 *
 * @param[out] src Structure that contain the expanded codeword
 * @param[out] dst Structure that contain the expanded codeword
 */
void hadamard(rm_expanded_cdw *src, rm_expanded_cdw *dst) {
    // the passes move data:
    // src -> dst -> src -> dst -> src -> dst -> src -> dst
    // using p1 and p2 alternately
    rm_expanded_cdw *p1 = src;
    rm_expanded_cdw *p2 = dst;
    for (int32_t pass = 0; pass < 7; pass++) {
        for (int32_t i = 0; i < 64; i++) {
            (*p2)[i] = (*p1)[2 * i] + (*p1)[2 * i + 1];
            (*p2)[i + 64] = (*p1)[2 * i] - (*p1)[2 * i + 1];
        }
        // swap p1, p2 for next round
        rm_expanded_cdw *p3 = p1;
        p1 = p2;
        p2 = p3;
    }
}

/**
 * @brief Add multiple codewords into expanded codeword
 *
 * Accesses memory in order
 * Note: this does not write the codewords as -1 or +1 as the green machine does
 * instead, just 0 and 1 is used.
 * The resulting hadamard transform has:
 * all values are halved
 * the first entry is 64 too high
 *
 * @param[out] dest Structure that contain the expanded codeword
 * @param[in] src Structure that contain the codeword
 */
void expand_and_sum(rm_expanded_cdw *dest, rm_codeword_t src[]) {
    // start with the first copy
    for (int32_t part = 0; part < 4; part++) {
        for (int32_t bit = 0; bit < 32; bit++) {
            (*dest)[part * 32 + bit] = src[0].u32[part] >> bit & 1;
        }
    }
    // sum the rest of the copies
    for (int32_t copy = 1; copy < MULTIPLICITY; copy++) {
        for (int32_t part = 0; part < 4; part++) {
            for (int32_t bit = 0; bit < 32; bit++) {
                (*dest)[part * 32 + bit] += src[copy].u32[part] >> bit & 1;
            }
        }
    }
}

/**
 * @brief Finding the location of the highest value
 *
 * This is the final step of the green machine: find the location of the highest value,
 * and add 128 if the peak is positive
 * if there are two identical peaks, the peak with smallest value
 * in the lowest 7 bits it taken
 * @param[in] transform Structure that contain the expanded codeword
 */
int32_t find_peaks(rm_expanded_cdw *transform) {
    uint32_t peak_abs_value = 0;
    int32_t peak_value = 0;
    uint32_t peak_pos = 0;

    for (int32_t i = 0; i < 128; i++) {
        const int32_t t = (*transform)[i];
        const uint32_t absolute = ct_abs_i32(t);
        const uint32_t update_peak = ct_mask_gt_u32(absolute, peak_abs_value);

        peak_value = ct_select_i32(update_peak, t, peak_value);
        peak_pos = ct_select_u32(update_peak, (uint32_t)i, peak_pos);
        peak_abs_value = ct_select_u32(update_peak, absolute, peak_abs_value);
    }

    peak_pos |= ct_positive_bit_i32(peak_value) << 7;
    return (int32_t)peak_pos;
}

int32_t find_peak_and_second_peak(rm_expanded_cdw *transform, uint64_t T) {
    uint32_t peak_abs_value = 0;
    int32_t peak_value = 0;
    uint32_t peak_pos = 0;

    uint32_t second_peak_abs_value = 0;
    int32_t second_peak_value = 0;
    uint32_t second_peak_pos = 0;

    for (int32_t i = 0; i < 128; i++) {
        const int32_t t = (*transform)[i];
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

    return ct_select_i32(ct_mask_gt_u32(abs_diff, (uint32_t)T), (int32_t)peak_pos, -1);
}
/**
 * @brief Encodes the received word
 *
 * The message consists of N1 bytes each byte is encoded into PARAM_N2 bits,
 * or MULTIPLICITY repeats of 128 bits
 *
 * @param[out] cdw Array of size VEC_N1N2_SIZE_64 receiving the encoded message
 * @param[in] msg Array of size VEC_N1_SIZE_64 storing the message
 */
/**
 * Encode the RS message bytes as duplicated Reed-Muller blocks.
 */
void reed_muller_encode(uint64_t *cdw, const uint64_t *msg) {
    uint8_t *message_array = (uint8_t *)msg;
    rm_codeword_t *codeArray = (rm_codeword_t *)cdw;
    for (size_t i = 0; i < VEC_N1_SIZE_BYTES; i++) {
        // fill entries i * MULTIPLICITY to (i+1) * MULTIPLICITY
        int32_t pos = i * MULTIPLICITY;
        // encode first word
        encode(&codeArray[pos], message_array[i]);
        // copy to other identical codewords
        for (size_t copy = 1; copy < MULTIPLICITY; copy++) {
            memcpy(&codeArray[pos + copy], &codeArray[pos], sizeof(rm_codeword_t));
        }
    }
}

/**
 * Decode duplicated Reed-Muller blocks and emit erasure flags for RS decoding.
 */
void reed_muller_decode(uint64_t *msg, uint8_t *erasures, const uint64_t *cdw) {
    uint8_t *message_array = (uint8_t *)msg;
    rm_codeword_t *codeArray = (rm_codeword_t *)cdw;
    rm_expanded_cdw expanded;
    for (size_t i = 0; i < VEC_N1_SIZE_BYTES; i++) {
        // collect the codewords
        expand_and_sum(&expanded, &codeArray[i * MULTIPLICITY]);
        // apply hadamard transform
        rm_expanded_cdw transform;
        hadamard(&expanded, &transform);
        // fix the first entry to get the half Hadamard transform
        transform[0] -= 64 * MULTIPLICITY;
        // finish the decoding
        int32_t peak = find_peak_and_second_peak(&transform, PARAM_ALPHA);
        message_array[i] = (uint8_t)peak;
        erasures[i] = (uint8_t)(((uint32_t)peak >> 31) & 1u);
    }
}
