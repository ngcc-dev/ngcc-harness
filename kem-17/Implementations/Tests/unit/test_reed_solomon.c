/**
 * @file test_reed_solomon.c
 * @brief Tests for the Reed–Solomon (RS) encoder/decoder.
 *
 * This test suite:
 *  - Generates random RS messages.
 *  - Encodes them into codewords.
 *  - Injects a controlled number of single-bit errors at distinct symbol
 *    positions.
 *  - Decodes and verifies that the original message is recovered.
 *
 * @see reed_solomon.h, parameters.h, munit.h
 */

#include <inttypes.h>
#include <munit.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "munit_utils.h"
#include "parameters.h"
#include "reed_solomon.h"

/**
 * @brief Choose @p nb_errors distinct error positions in [0, @c PARAM_N1).
 *
 * Fills @p errors_pos with unique byte indices where errors will be injected.
 * Uses MUnit's PRNG (see @c munit_rand_uint32).
 *
 * @param[out] errors_pos Array of length at least @p nb_errors to receive positions.
 * @param[in]  nb_errors  Number of positions to choose (must be ≤ @c PARAM_N1).
 * @retval MUNIT_OK on success.
 */
static MunitResult generate_errors(uint8_t *errors_pos, uint8_t nb_errors) {
    munit_assert_ptr_not_null(errors_pos);
    munit_assert(PARAM_N1 <= 256);
    munit_assert(nb_errors <= PARAM_N1);

    const uint32_t range = (uint32_t)PARAM_N1;
    const uint32_t limit = UINT32_MAX - (UINT32_MAX % range);

    for (uint8_t i = 0; i < nb_errors; i++) {
        uint32_t r;
        bool unique;
        do {
            /* draw a uniform random in [0, range) */
            do {
                r = munit_rand_uint32();
            } while (r >= limit);
            r %= range;

            /* check against previous entries */
            unique = true;
            for (uint8_t j = 0; j < i; j++) {
                if (errors_pos[j] == (uint8_t)r) {
                    unique = false;
                    break;
                }
            }
        } while (!unique);

        errors_pos[i] = (uint8_t)r;
    }
    return MUNIT_OK;
}

/**
 * @brief Flip one bit in each of @p nb_errors symbols of a codeword.
 *
 * For each position in @p errors_pos, toggles the corresponding byte in
 * @p codeword.
 *
 * @param[in,out] codeword     Bytewise codeword buffer of length @c PARAM_N1.
 * @param[in]     errors_pos   Array of positions as produced by generate_errors().
 * @param[in]     nb_errors    Number of error positions to apply.
 * @retval MUNIT_OK on success.
 */
static MunitResult inject_errors(uint8_t *codeword, const uint8_t *errors_pos, uint8_t nb_errors) {
    munit_assert(codeword != NULL);
    munit_assert(errors_pos != NULL);
    munit_assert(nb_errors <= PARAM_N1);

    for (uint8_t i = 0; i < nb_errors; i++) {
        uint8_t pos = errors_pos[i];
        munit_assert(pos < PARAM_N1);
        /* flip the bit at position pos */
        codeword[pos] = ~codeword[pos];
    }
    return MUNIT_OK;
}

/**
 * @brief Fill a buffer with uniformly random bytes using MUnit’s PRNG.
 *
 * @param[out] buf Destination buffer.
 * @param[in]  len Number of bytes to write (must be > 0).
 * @retval MUNIT_OK on success.
 *
 * @pre @p buf != NULL
 * @pre @p len > 0
 */
static MunitResult generate_random_bytes(uint8_t *buf, size_t len) {
    munit_assert_ptr_not_null(buf);
    munit_assert(len > 0);

    for (size_t i = 0; i < len; i++) {
        buf[i] = (uint8_t)(munit_rand_uint32() & 0xFF);
    }
    return MUNIT_OK;
}

/**
 * @brief Reed Solomon error-correction property test.
 *
 * For 100 randomized iterations (see @c MUNIT_RANDOMIZED_TEST), this test:
 *  - Draws a random message of @c PARAM_K bytes.
 *  - Encodes to a bytewise codeword.
 *  - For each i = 1..@c PARAM_DELTA, injects i single-bit errors at distinct
 *    symbol positions.
 *  - Decodes and asserts the original message is recovered exactly.
 *
 * @param params    (unused) MUnit parameters.
 * @param user_data (unused) Test context pointer.
 * @return @c MUNIT_OK on success, otherwise triggers assertions.
 *
 * @see reed_solomon_encode, reed_solomon_decode
 */
static MunitResult test_error_correction(const MunitParameter params[], void *user_data) {
    (void)params;
    (void)user_data;

    /* Loop 100×, reseeding the PRNG each time. */
    MUNIT_RANDOMIZED_TEST(100, {
        /* To print the iteration’s seed, uncomment:*/
        //              printf("[RS iter %u] seed = 0x%08" PRIx32 "\n",
        //                     _m_iter, _m_seed);
        //              fflush(stdout);

        for (size_t i = 1; i <= PARAM_DELTA; ++i) {
            uint8_t errors[PARAM_DELTA + 10] = {0};
            uint8_t m1[PARAM_K] = {0};
            uint64_t cw[VEC_N1_SIZE_64] = {0};
            uint8_t cw_corrupted[PARAM_N1] = {0};
            uint8_t decoded[PARAM_K] = {0};

            munit_assert(generate_random_bytes(m1, PARAM_K) == MUNIT_OK);

            /* encode into a true bytewise codeword */
            reed_solomon_encode(cw, (uint64_t *)m1);

            /* copy to our “corrupted” buffer */
            memcpy(cw_corrupted, cw, PARAM_N1);

            /* pick i distinct symbol positions */
            munit_assert(generate_errors(errors, i) == MUNIT_OK);

            /* flip one bit in each of those symbols */
            munit_assert(inject_errors(cw_corrupted, errors, i) == MUNIT_OK);

            /* decode back */
            reed_solomon_decode((uint64_t *)decoded, (uint64_t *)cw_corrupted);

            /* original and decoded must match */
            munit_assert_memory_equal(PARAM_K, m1, decoded);
        }
    });

    return MUNIT_OK;
}

/**
 * @brief Test registry for Reed–Solomon routines.
 */
MunitTest rs_tests[] = {MUNIT_TEST_ENTRY("error correction", test_error_correction), MUNIT_TEST_END};
