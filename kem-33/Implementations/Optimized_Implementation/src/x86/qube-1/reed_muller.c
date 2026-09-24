/**
 * @file reed_muller.c
 * @brief Constant time implementation of Reed-Muller code RM(1,7)
 */

#include "reed_muller.h"
#include <stdint.h>
#include <string.h>
#include <x86intrin.h>
#include "data_structures.h"
#include "parameters.h"

/**
 * @brief Number of repeated 128-bit codeword blocks.
 *
 * Calculates the ceiling of PARAM_N2/128 to determine how many
 * copies of each 128-bit codeword are used in the code expansion.
 */
#define MULTIPLICITY CEIL_DIVIDE(PARAM_N2, 128)

static const uint16_t values[16] = {
    1 << 0, 1 << 1, 1 << 2, 1 << 3,
    1 << 4, 1 << 5, 1 << 6, 1 << 7,
    1 << 8, 1 << 9, 1 << 10, 1 << 11,
    1 << 12, 1 << 13, 1 << 14, 1 << 15
};
static inline __m256i bitmask_from_16(uint16_t x)
{
    __m256i full_length = _mm256_set1_epi16(x);

    __m256i bitmask = _mm256_loadu_si256((const __m256i *)values);
    __m256i x_mask = _mm256_and_si256(full_length, bitmask);

    __m256i result = _mm256_cmpeq_epi16(x_mask, _mm256_setzero_si256());
    result = _mm256_add_epi16(result, _mm256_set1_epi16(1));

    return result;
}

// clang-format off
/**
 * @def BIT0MASK(x)
 * @brief Broadcast the least significant bit of \p x to a 32-bit mask.
 *
 * @param x  An integer expression; only bit 0 is examined.
 * @return   A 32-bit value of all ones (if \p x&1 == 1) or all zeros (if \p x&1 == 0).
 */
#define BIT0MASK(x) (int64_t)(-((x) & 1))
// clang-format on

static inline void encode(rm_codeword128_t *word, int32_t message);
static inline void expand_and_sum(rm_expanded_cdw128_t *dst, rm_codeword128_t src[]);
static inline void hadamard(rm_expanded_cdw128_t *src, rm_expanded_cdw128_t *dst);
static inline int32_t find_peaks(rm_expanded_cdw128_t *transform);
static inline int32_t peak_search_width(void);

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
 * 5   00000000 ffffffff 00000000 ffffffff
 * 6   00000000 00000000 ffffffff ffffffff
 * 7   ffffffff ffffffff ffffffff ffffffff
 *
 * @param[out] word An RM(1,7) codeword
 * @param[in] message A message to encode
 */
static inline void encode(rm_codeword128_t *word, int32_t message) {
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
 * @brief Add multiple codewords into expanded codeword
 *
 * Note: this does not write the codewords as -1 or +1 as the green machine does
 * instead, just 0 and 1 is used.
 * The resulting hadamard transform has:
 * all values are halved
 * the first entry is 64 too high
 *
 * @param[out] dst Structure that contain the expanded codeword
 * @param[in] src Structure that contain the codeword
 */
static inline void expand_and_sum(rm_expanded_cdw128_t *dst, rm_codeword128_t src[]) {
    // start converting the first copy

    for(size_t part = 0; part < 8; part++) {

        __m256i bit_array = bitmask_from_16(src->u16[part]);
        for (size_t copy = 1; copy < MULTIPLICITY; copy++) {
            bit_array = _mm256_add_epi16(bit_array, bitmask_from_16(src[copy].u16[part]));
        }
        dst->mm[part] = bit_array;
    }
    // for (size_t part = 0; part < 8; part++) {
    //     for (size_t i = 0; i < 16; ++i) {
    //         dst->i16[(part << 4) + i] = src->u16[part] >> i & 1;
    //     }
    // }
    // // sum the rest of the copies
    // for (size_t copy = 1; copy < MULTIPLICITY; copy++) {
    //     for (size_t part = 0; part < 8; part++) {
    //         for (size_t i = 0; i < 16; ++i) {
    //             dst->i16[(part << 4) + i] += src[copy].u16[part] >> i & 1;
    //         }
    //     }
    // }
}

/**
 * @brief Hadamard transform
 *
 * Perform hadamard transform of src and store result in dst
 * src is overwritten: it is also used as intermediate buffer
 *
 * @param[out] src Structure that contain the expanded codeword
 * @param[out] dst Structure that contain the expanded codeword
 */
static inline void hadamard(rm_expanded_cdw128_t *src, rm_expanded_cdw128_t *dst) {
    // the passes move data:
    // src -> dst -> src -> dst -> src -> dst -> src -> dst
    // using p1 and p2 alternately
    rm_expanded_cdw128_t *p1 = src;
    rm_expanded_cdw128_t *p2 = dst;
    for (size_t pass = 0; pass < 7; pass++) {
        for (size_t part = 0; part < 4; part++) {
            p2->mm[part] = _mm256_permute4x64_epi64(_mm256_hadd_epi16(p1->mm[2 * part], p1->mm[2 * part + 1]), 0xd8);
            p2->mm[part + 4] =
                _mm256_permute4x64_epi64(_mm256_hsub_epi16(p1->mm[2 * part], p1->mm[2 * part + 1]), 0xd8);
        }
        // swap p1, p2 for next round
        rm_expanded_cdw128_t *p3 = p1;
        p1 = p2;
        p2 = p3;
    }
}

static inline int32_t peak_search_width(void) {
    int32_t width = 1;
    const int32_t max_peak = 64 * MULTIPLICITY;

    while (width < max_peak) {
        width <<= 1;
    }

    return width;
}

/**
 * @brief Finding the location of the highest value
 *
 * This is the final step of the green machine: find the location of the highest value,
 * and add 128 if the peak is positive
 * Notes on decoding
 * The standard "Green machine" decoder works as follows:
 * if the received codeword is W, compute (2 * W - 1) * H7
 * The entries of the resulting vector are always even and vary from
 * -128 (= the complement is a code word, add bit 7 to decode)
 * via 0 (this is a different codeword)
 * to 128 (this is the code word).
 *
 * Our decoding differs in two ways:
 * - We take W instead of 2 * W - 1 (so the entries are 0,1 instead of -1,1)
 * - We take the sum of the repetitions (so the entries are 0..MULTIPLICITY)
 * This implies that we have to subtract 64M (M=MULTIPLICITY)
 * from the first entry to make sure the first codewords is handled properly
 * and that the entries vary from -64M to 64M.
 * -64M or 64M stands for a perfect codeword.
 *
 * @param[in] transform Structure that contain the expanded codeword
 */
static inline int32_t find_peaks(rm_expanded_cdw128_t *transform) {
    __m256i bitmap, abs_rows[8], bound, active_row, max_abs_rows;
    rm_vector256_t peak_mask;
    // compute absolute value of transform
    for (size_t i = 0; i < 8; i++) {
        abs_rows[i] = _mm256_abs_epi16(transform->mm[i]);
    }
    // compute a vector of 16 elements which contains the maximum somewhere
    max_abs_rows = abs_rows[0];
    for (size_t i = 1; i < 8; i++) {
        max_abs_rows = _mm256_max_epi16(max_abs_rows, abs_rows[i]);
    }

    // Binary search for max_abs - 1 over the full duplicated RM peak range.
    int32_t lower = 0;
    int32_t width = peak_search_width();

    while (width > 1) {
        width >>= 1;
        // compare with lower + width; put result in bitmap
        // make vector from value of new bound
        bound = _mm256_broadcastw_epi16(_mm_cvtsi32_si128(lower + width));
        bitmap = _mm256_cmpgt_epi16(max_abs_rows, bound);
        // step up if there are any matches
        int32_t step_mask = _mm256_testz_si256(bitmap, bitmap) - 1;
        lower += step_mask & width;
    }
    // now lower+width contains the maximum value of the vector
    // construct vector filled with bound-1
    bound = _mm256_broadcastw_epi16(_mm_cvtsi32_si128(lower + width - 1));

    // find in which of the 8 groups a maximum occurs to compute bits 4, 5, 6 of message
    size_t message = 0x70;
    for (int32_t i = 7; i >= 0; i--) {
        bitmap = _mm256_cmpgt_epi16(abs_rows[i], bound);
        int message_mask = (-(int16_t)(_mm256_testz_si256(bitmap, bitmap) == 0)) >> 15;
        message ^= message_mask & (message ^ (unsigned)i << 4);
    }
    // build 'active_row' = abs_rows[index], using constant-time row selection.
    int8_t index = message >> 4;
    __m256i res;
    __m256i tmp = (__m256i){0ULL, 0ULL, 0ULL, 0ULL};

    for (int8_t i = 0; i < 8; i++) {
        int8_t abs_value = (int8_t)(index - i);
        int8_t mask1 = abs_value >> 7;
        abs_value ^= mask1;
        abs_value -= mask1;
        int8_t mask2 = ((uint8_t)-abs_value >> 7);
        int64_t mask3 = (-1ULL) + mask2;
        __m256i vect_mask = (__m256i){mask3, mask3, mask3, mask3};
        res = _mm256_and_si256(abs_rows[i], vect_mask);
        tmp = _mm256_or_si256(tmp, res);
    }

    active_row = tmp;

    // get the column number of the vector element
    peak_mask.mm = _mm256_cmpgt_epi16(active_row, bound);
    for (size_t i = 0; i < 16; ++i) {
        peak_mask.u16[i] &= 1 << i;
    }

    for (int32_t i = 0; i < 3; i++) {
        peak_mask.mm = _mm256_hadd_epi16(peak_mask.mm, peak_mask.mm);
    }
    // add low 4 bits of message
    message |= _bit_scan_forward(peak_mask.u16[0] + peak_mask.u16[8]);

    // set bit 7 if sign of biggest value is positive
    tmp = (__m256i){0ULL, 0ULL, 0ULL, 0ULL};
    for (uint32_t i = 0; i < 8; i++) {
        int64_t message_mask = (-(int64_t)(i == message / 16)) >> 63;
        __m256i vect_mask = (__m256i){message_mask, message_mask, message_mask, message_mask};
        tmp = _mm256_or_si256(tmp, _mm256_and_si256(vect_mask, transform->mm[i]));
    }
    rm_vector256_t selected_peak;
    selected_peak.mm = tmp;
    uint16_t result = 0;
    for (uint32_t i = 0; i < 16; i++) {
        int32_t message_mask = (-(int32_t)(i == message % 16)) >> (sizeof(int32_t) * 8 - 1);
        result |= message_mask & selected_peak.u16[i];
    }
    message |= (0x8000 & ~result) >> 8;
    return message;
}

#ifdef QUBE_RM_TEST_HOOKS
uint8_t qube_1_rm_find_peak_for_test(const int16_t transform[128]) {
    rm_expanded_cdw128_t tmp;

    memcpy(tmp.i16, transform, sizeof tmp.i16);
    return (uint8_t)find_peaks(&tmp);
}
#endif

/**
 * @brief Encodes the received word
 *
 * The message consists of N1 bytes each byte is encoded into PARAM_N2 bits,
 * or MULTIPLICITY repeats of 128 bits
 *
 * @param[out] cdw Array of size VEC_N1N2_SIZE_64 receiving the encoded message
 * @param[in] msg Array of size VEC_N1_SIZE_64 storing the message
 */
void reed_muller_encode(uint64_t *cdw, const uint64_t *msg) {
    uint8_t *message_array = (uint8_t *)msg;
    rm_codeword128_t *codeArray = (rm_codeword128_t *)cdw;
    for (size_t i = 0; i < VEC_N1_SIZE_BYTES; i++) {
        // fill entries i * MULTIPLICITY to (i+1) * MULTIPLICITY
        int32_t pos = i * MULTIPLICITY;
        // encode first word
        encode(&codeArray[pos], message_array[i]);
        // copy to other identical codewords
        for (size_t copy = 1; copy < MULTIPLICITY; copy++) {
            memcpy(&codeArray[pos + copy], &codeArray[pos], sizeof(rm_codeword128_t));
        }
    }
}

/**
 * @brief Decodes the received word
 *
 * Decoding uses fast hadamard transform, for a more complete picture on Reed-Muller decoding, see MacWilliams, Florence
 * Jessie, and Neil James Alexander Sloane. The theory of error-correcting codes codes @cite macwilliams1977theory
 *
 * @param[out] msg Array of size VEC_N1_SIZE_64 receiving the decoded message
 * @param[in] cdw Array of size VEC_N1N2_SIZE_64 storing the received word
 */
void reed_muller_decode(uint64_t *msg, const uint64_t *cdw) {
    uint8_t *message_array = (uint8_t *)msg;
    rm_codeword128_t *codeArray = (rm_codeword128_t *)cdw;
    rm_expanded_cdw128_t expanded;
    for (size_t i = 0; i < VEC_N1_SIZE_BYTES; i++) {
        // collect the codewords
        expand_and_sum(&expanded, &codeArray[i * MULTIPLICITY]);
        // apply hadamard transform
        rm_expanded_cdw128_t transform;
        hadamard(&expanded, &transform);
        // fix the first entry to get the half Hadamard transform
        transform.i16[0] -= 64 * MULTIPLICITY;
        // finish the decoding
        message_array[i] = find_peaks(&transform);
    }
}
