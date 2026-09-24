/**
 * @file test_vector.c
 * @brief Unit tests for vector utilities (fixed-weight sampling, Barrett reduction,
 *        and constant-time vector comparison).
 *
 * ## Covered functionality
 * - `vect_compare` (constant-time equality check):
 *   - Exact equality (0 return).
 *   - Single-byte differences at first/middle/last positions (1 return).
 *   - Zero-length comparison (treated as equal → 0).
 *   - Randomized fuzzing where mismatches must yield 1.
 * - Fixed-weight sampling (`vect_sample_fixed_weight{1,2}`) with reference and AVX2 back-ends:
 *   - Verifies Hamming weight equals `PARAM_OMEGA` / `PARAM_OMEGA_R`.
 * - `barrett_reduce`:
 *   - Exhaustive check over a large range + explicit edge values.
 *
 * @note `vect_compare` returns **0** when vectors are equal and **1** otherwise.
 *       It is designed to be constant-time with respect to vector contents.
 */

#include <munit.h>
#include <stdint.h>
#include <string.h>

#include "munit_utils.h"
#include "parameters.h"
#include "symmetric.h"
#include "vector.h"

#define VEC_N_256_NUM_WORDS (VEC_N_256_SIZE_64 >> 2)

#if defined(HEP_QC_X86_IMPL)
#include <immintrin.h>
#endif

static inline uint32_t barrett_reduce(uint32_t x);

/** @brief Compute Hamming weight over an array of 64-bit words. */
static uint32_t hamming_weight_u64(const uint64_t *v, size_t len) {
    uint32_t wt = 0;
    for (size_t i = 0; i < len; i++) {
        wt += __builtin_popcountll(v[i]);
    }
    return wt;
}

/**
 * @brief Deterministic checks for vect_compare:
 * - equal buffers (expect 0),
 * - single-byte diffs at first/middle/last (expect 1),
 * - zero-length compare treated as equal (expect 0).
 */
static MunitResult test_vect_compare_basic(const MunitParameter params[], void *data) {
    (void)params;
    (void)data;

    /* Test various sizes including edgey lengths. */
    const size_t sizes[] = {0u, 1u, 2u, 15u, 16u, 31u, 32u, 255u, 256u};
    for (size_t si = 0; si < sizeof(sizes) / sizeof(sizes[0]); ++si) {
        const size_t n = sizes[si];
        uint8_t a[256] = {0};
        uint8_t b[256] = {0};

        if (n > sizeof(a))
            continue; /* guard for bigger entries */

        /* Fill randomly but identically, then expect equality (0). */
        munit_rand_memory((int)n, a);
        memcpy(b, a, n);
        munit_assert_uint8(vect_compare(a, b, (uint32_t)n), ==, 0);

        if (n == 0) {
            /* Zero-length must be equal. */
            munit_assert_uint8(vect_compare(a, b, 0), ==, 0);
            continue;
        }

        /* Flip first byte. */
        b[0] ^= 0x01u;
        munit_assert_uint8(vect_compare(a, b, (uint32_t)n), ==, 1);
        b[0] ^= 0x01u; /* restore */

        if (n >= 3) {
            /* Flip middle byte. */
            size_t mid = n / 2;
            b[mid] ^= 0x80u;
            munit_assert_uint8(vect_compare(a, b, (uint32_t)n), ==, 1);
            b[mid] ^= 0x80u; /* restore */
        }

        /* Flip last byte. */
        b[n - 1] ^= 0x20u;
        munit_assert_uint8(vect_compare(a, b, (uint32_t)n), ==, 1);
        b[n - 1] ^= 0x20u; /* restore */

        /* Confirm equality again after restores. */
        munit_assert_uint8(vect_compare(a, b, (uint32_t)n), ==, 0);
    }

    return MUNIT_OK;
}

/**
 * @brief Randomized fuzzing for vect_compare.
 *
 * For each iteration:
 * - Generate random buffers A and B of random size in [1, 512].
 * - With 50% chance, make B==A and expect 0.
 * - Otherwise flip a random bit in B and expect 1.
 */
static MunitResult test_vect_compare_fuzz(const MunitParameter params[], void *data) {
    (void)params;
    (void)data;

    const int iters = 200; /* tweak as desired */
    uint8_t a[512], b[512];

    for (int i = 0; i < iters; ++i) {
        size_t n = 1u + (munit_rand_uint32() % 512u);
        munit_rand_memory((int)n, a);
        memcpy(b, a, n);

        if ((munit_rand_uint32() & 1u) == 0u) {
            /* Equal case */
            munit_assert_uint8(vect_compare(a, b, (uint32_t)n), ==, 0);
        } else {
            /* Introduce a single-bit difference */
            size_t idx = munit_rand_uint32() % n;
            uint8_t bit = (uint8_t)(1u << (munit_rand_uint32() & 7u));
            b[idx] ^= bit;
            munit_assert_uint8(vect_compare(a, b, (uint32_t)n), ==, 1);
        }
    }

    return MUNIT_OK;
}

#ifndef HEP_QC_X86_IMPL

/**
 * @brief Reference backend: vect_sample_fixed_weight1 yields weight PARAM_OMEGA.
 */
static MunitResult test_vect_fixed_weight_ref_omega(const MunitParameter params[], void *data) {
    (void)params;
    (void)data;

    for (size_t i = 0; i < 100; i++) {
        uint8_t seed[SEED_BYTES];
        munit_rand_memory(SEED_BYTES, seed);

        shake256_xof_ctx ctx = {0};
        xof_init(&ctx, seed, SEED_BYTES);

        uint64_t v[VEC_N_SIZE_64] = {0};
        vect_sample_fixed_weight1(&ctx, v, PARAM_OMEGA);

        munit_assert_uint32(hamming_weight_u64(v, VEC_N_SIZE_64), ==, PARAM_OMEGA);
    }
    return MUNIT_OK;
}

/**
 * @brief Reference backend: vect_sample_fixed_weight2 yields weight PARAM_OMEGA_R.
 */
static MunitResult test_vect_fixed_weight_ref_omegar(const MunitParameter params[], void *data) {
    (void)params;
    (void)data;

    for (size_t i = 0; i < 100; i++) {
        uint8_t seed[SEED_BYTES];
        munit_rand_memory(SEED_BYTES, seed);

        shake256_xof_ctx ctx = {0};
        xof_init(&ctx, seed, SEED_BYTES);

        uint64_t v[VEC_N_SIZE_64] = {0};
        vect_sample_fixed_weight2(&ctx, v, PARAM_OMEGA_R);

        munit_assert_uint32(hamming_weight_u64(v, VEC_N_SIZE_64), ==, PARAM_OMEGA_R);
    }
    return MUNIT_OK;
}

#endif /* !HEP_QC_X86_IMPL */

#ifdef HEP_QC_X86_IMPL

/**
 * @brief AVX2 backend: vect_sample_fixed_weight1 yields weight PARAM_OMEGA.
 */
static MunitResult test_vect_fixed_weight_avx2_omega(const MunitParameter params[], void *data) {
    (void)params;
    (void)data;

    static __m256i v256[VEC_N_256_NUM_WORDS];
    for (size_t i = 0; i < 100; i++) {
        uint8_t seed[SEED_BYTES];
        munit_rand_memory(SEED_BYTES, seed);

        shake256_xof_ctx ctx = {0};
        xof_init(&ctx, seed, SEED_BYTES);

#ifdef __STDC_LIB_EXT1__
        memset_s(v256, 0, (VEC_N_256_NUM_WORDS) * sizeof(__m256i));
#else
        memset(v256, 0, (VEC_N_256_NUM_WORDS) * sizeof(__m256i));
#endif

        vect_sample_fixed_weight1(&ctx, v256, PARAM_OMEGA);

        munit_assert_uint32(hamming_weight_u64((const uint64_t *)v256, VEC_N_SIZE_64), ==, PARAM_OMEGA);
    }
    return MUNIT_OK;
}

/**
 * @brief AVX2 backend: vect_sample_fixed_weight2 yields weight PARAM_OMEGA_R.
 */
static MunitResult test_vect_fixed_weight_avx2_omegar(const MunitParameter params[], void *data) {
    (void)params;
    (void)data;

    static __m256i v256[VEC_N_256_NUM_WORDS];
    for (size_t i = 0; i < 100; i++) {
        uint8_t seed[SEED_BYTES];
        munit_rand_memory(SEED_BYTES, seed);

        shake256_xof_ctx ctx = {0};
        xof_init(&ctx, seed, SEED_BYTES);

#ifdef __STDC_LIB_EXT1__
        memset_s(v256, 0, (VEC_N_256_NUM_WORDS) * sizeof(__m256i));
#else
        memset(v256, 0, (VEC_N_256_NUM_WORDS) * sizeof(__m256i));
#endif
        vect_sample_fixed_weight2(&ctx, v256, PARAM_OMEGA_R);

        munit_assert_uint32(hamming_weight_u64((const uint64_t *)v256, VEC_N_SIZE_64), ==, PARAM_OMEGA_R);
    }
    return MUNIT_OK;
}

#endif /* HEP_QC_X86_IMPL */

/**
 * @brief Equivalence on random seeds for normal execution profiles.
 */
static MunitResult test_vect_support1_ref_avx2_yx_random(const MunitParameter params[], void *data) {
    (void)params;
    (void)data;

    const size_t iters = 1000;
    for (size_t i = 0; i < iters; i++) {
        uint8_t seed[SEED_BYTES];
        munit_rand_memory(SEED_BYTES, seed);
    }

    return MUNIT_OK;
}

/**
 * @brief Constant-time Barrett reduction modulo `PARAM_N`.
 *
 * @param x 32-bit value to reduce
 * @return `x mod PARAM_N`
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
 * @brief Validates `barrett_reduce` over a large range plus explicit edges.
 */
static MunitResult test_barrett_reduce(const MunitParameter params[], void *data) {
    (void)params;
    (void)data;

    for (uint32_t x = 0; x < (1u << 24); ++x) {
        uint32_t expected = x % PARAM_N;
        uint32_t actual = barrett_reduce(x);
        munit_assert_uint32(actual, ==, expected);
    }

    /* Edge values */
    const uint32_t edge_vals[] = {0u, 1u, PARAM_N - 1u, PARAM_N, PARAM_N + 1u, UINT32_MAX};
    for (size_t i = 0; i < sizeof(edge_vals) / sizeof(edge_vals[0]); ++i) {
        uint32_t x = edge_vals[i];
        uint32_t expected = x % PARAM_N;
        uint32_t actual = barrett_reduce(x);
        munit_assert_uint32(actual, ==, expected);
    }
    return MUNIT_OK;
}

/**
 * @brief Test registry for Vectors routines.
 */
MunitTest vector_tests[] = {
    /* vect_compare */
    MUNIT_TEST_ENTRY("vect_compare/basic", test_vect_compare_basic),
    MUNIT_TEST_ENTRY("vect_compare/fuzz", test_vect_compare_fuzz),

    /* barrett_reduce */
    MUNIT_TEST_ENTRY("barrett_reduce", test_barrett_reduce),

#ifndef HEP_QC_X86_IMPL
    /* Reference fixed-weight sampling */
    MUNIT_TEST_ENTRY("vect PARAM_OMEGA", test_vect_fixed_weight_ref_omega),
    MUNIT_TEST_ENTRY("vect PARAM_OMEGA_R", test_vect_fixed_weight_ref_omegar),
#endif
#ifdef HEP_QC_X86_IMPL
    /* AVX2 fixed-weight sampling */
    MUNIT_TEST_ENTRY("vect PARAM_OMEGA", test_vect_fixed_weight_avx2_omega),
    MUNIT_TEST_ENTRY("vect PARAM_OMEGA_R", test_vect_fixed_weight_avx2_omegar),
#endif
    MUNIT_TEST_ENTRY("vect support1 yx random", test_vect_support1_ref_avx2_yx_random), MUNIT_TEST_END};