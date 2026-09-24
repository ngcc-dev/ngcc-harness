/**
 * @file test_gf
 * @brief Unit test for GF(2^8) reduction routine and a reference reducer.
 *
 * @details
 * - The test exhaustively checks all 2^16 inputs.
 * - The reduction taps (feedback positions) are provided by
 *   ::gf_reduction_taps and interpreted relative to @c PARAM_M = 8.
 * - The reference routine reduces with the polynomial @c PARAM_GF_POLY (e.g.
 *   0x11D for AES-like GF(2^8)), shifting and xoring for each high bit.
 *
 * @see gf.h, parameters.h, munit.h
 */

#include <inttypes.h>
#include <munit.h>
#include <string.h>
#include "gf.h"
#include "munit_utils.h"
#include "parameters.h"

/**
 * @brief Reduce a 16-bit value into GF(2^m) with XOR/shift feedback.
 *
 * @param x 16-bit input to reduce.
 * @return The value reduced modulo the field polynomial (in the low @c PARAM_M bits).
 *
 */
uint16_t gf_reduce(uint16_t x);

/**
 * @brief Feedback tap positions (in descending order) used by ::gf_reduce.
 *
 */
static const uint8_t gf_reduction_taps[] = {4, 3, 2};

uint16_t gf_reduce(uint16_t x) {
    uint64_t mod;
    const int reduction_steps = 2;           /* For deg(x) = 2 * (PARAM_M - 1), reduce twice */
    const size_t gf_reduction_tap_count = 3; /* Number of feedback positions */

    for (int i = 0; i < reduction_steps; ++i) {
        mod = x >> PARAM_M;      /* Extract upper bits */
        x &= (1 << PARAM_M) - 1; /* Keep lower bits */
        x ^= mod;                /* Pre-XOR with no shift */

        uint16_t z1 = 0;
        for (size_t j = gf_reduction_tap_count; j; --j) {
            uint16_t z2 = gf_reduction_taps[j - 1];
            uint16_t dist = z2 - z1;
            mod <<= dist;
            x ^= mod;
            z1 = z2;
        }
    }
    return x;
}

/**
 * @brief Reference GF(2^8) reduction using explicit polynomial long division.
 *
 * @param x 16-bit value to reduce.
 * @return The reduced 8-bit value (low byte of @p x after reduction).
 */
static uint8_t ref_reduce(uint16_t x) {
    const uint16_t poly = PARAM_GF_POLY; /* 0x11D */
    for (int bit = 15; bit >= 8; --bit) {
        if (x & (1u << bit)) {
            x ^= poly << (bit - 8);
        }
    }
    return (uint8_t)x;
}

/**
 * @brief MUnit test case: exhaustive equivalence of ::gf_reduce and ::ref_reduce.
 *
 * Iterates over all 2^16 inputs and asserts that both reducers produce identical
 * outputs for each input.
 *
 * @param params    Unused MUnit parameters.
 * @param user_data Unused user data pointer.
 * @return @c MUNIT_OK on success, or triggers an assertion failure otherwise.
 *
 * @test
 * The test compares:
 * @code
 * expected = ref_reduce(i);
 * actual   = gf_reduce(i);
 * @endcode
 * for all 0 <= i < 65536.
 */
static MunitResult test_gf_reduce(const MunitParameter params[], void* user_data) {
    (void)params;
    (void)user_data;

    for (uint32_t i = 0; i < (1u << 16); ++i) {
        uint8_t expected = ref_reduce((uint16_t)i);
        uint8_t actual = gf_reduce((uint16_t)i);
        munit_assert_uint8(actual, ==, expected);
    }
    return MUNIT_OK;
}

/**
 * @brief Test registry for Galois Field routines.
 */
MunitTest gf_tests[] = {MUNIT_TEST_ENTRY("gf_reduce", test_gf_reduce), MUNIT_TEST_END};
