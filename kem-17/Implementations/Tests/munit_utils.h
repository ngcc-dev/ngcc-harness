/**
 * @file munit_utils.h
 * @brief Helper macros to declare MUnit tests and suites, plus a simple
 *        deterministic randomized-test harness.
 *
 * @ingroup munit_helpers
 */

/**
 * @defgroup munit_helpers MUnit helper macros
 * @brief Convenience wrappers for defining tests and suites with MUnit.
 * @{
 */

#ifndef MUNIT_UTILS_H
#define MUNIT_UTILS_H
// clang-format off
#include "munit.h"

/**
 * @def MUNIT_TEST_ENTRY(name_str, test_func)
 * @brief Create a @c MunitTest initializer for a single test with no parameters.
 *
 * Expands to:
 * @code
 * { "/<name_str>", (test_func), NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
 * @endcode
 *
 * @param name_str  Literal test name **without** leading slash (e.g. `"reed_solomon"`).
 * @param test_func Function pointer of the test (e.g. @c test_reed_solomon).
 *
 * @par Example
 * @code
 * static MunitResult test_reed_solomon(const MunitParameter[], void*);
 * MunitTest rs_tests[] = {
 *   MUNIT_TEST_ENTRY("reed_solomon", test_reed_solomon),
 *   MUNIT_TEST_END
 * };
 * @endcode
 */
#define MUNIT_TEST_ENTRY(name_str, test_func)   \
    {                                           \
        "/" name_str,                           \
        (test_func),                            \
        NULL,                                   \
        NULL,                                   \
        MUNIT_TEST_OPTION_NONE,                 \
        NULL                                    \
    }

/**
 * @def MUNIT_TEST_ENTRY_ITER(name_str, test_func, param_enum)
 * @brief Create a @c MunitTest initializer that accepts parameter enumerations.
 *
 * Same as ::MUNIT_TEST_ENTRY but allows passing a @c MunitParameterEnum table
 * (e.g., to vary the number of iterations).
 *
 * @param name_str   Literal test name **without** leading slash (e.g. `"kem_api"`).
 * @param test_func  Function pointer of the test (e.g. @c test_kem_api).
 * @param param_enum Pointer to a @c MunitParameterEnum[] terminated with @c { NULL, NULL }.
 *
 * @par Example
 * @code
 * static char* iteration_values[] = { "1", "10000", NULL };
 * static MunitParameterEnum kem_api_params[] = {
 *   { "iterations", iteration_values },
 *   { NULL, NULL }
 * };
 *
 * MunitTest kem_tests[] = {
 *   MUNIT_TEST_ENTRY_ITER("kem_api", test_kem_api, kem_api_params),
 *   MUNIT_TEST_END
 * };
 * @endcode
 */
#define MUNIT_TEST_ENTRY_ITER(name_str, test_func, param_enum) \
    {                                                         \
        "/" name_str,                                         \
        (test_func),                                          \
        NULL,                                                 \
        NULL,                                                 \
        MUNIT_TEST_OPTION_NONE,                               \
        (param_enum)                                          \
    }


/**
 * @def MUNIT_TEST_END
 * @brief Sentinel terminator for a @c MunitTest array.
 *
 * Use as the final element in a @c MunitTest[] list.
 */
#define MUNIT_TEST_END  \
    {                   \
        NULL,           \
        NULL,           \
        NULL,           \
        NULL,           \
        0,              \
        NULL            \
    }

/**
 * @def MUNIT_LEAF_ONCE(prefix_str, test_array)
 * @brief Define a leaf @c MunitSuite that runs once and contains tests (no subsuites).
 *
 * Expands to a @c MunitSuite initializer equivalent to:
 * @code
 * {
 *   .prefix     = "/<prefix_str>",
 *   .tests      = <test_array>,
 *   .suites     = NULL,
 *   .iterations = 1,
 *   .options    = MUNIT_SUITE_OPTION_NONE
 * }
 * @endcode
 *
 * @param prefix_str Literal suite prefix **without** leading slash (e.g. `"rs"`).
 * @param test_array Name of a @c MunitTest[] array (e.g. @c rs_tests).
 */
#define MUNIT_LEAF_ONCE(prefix_str, test_array)  \
    {                                           \
        "/" prefix_str,                         \
        (test_array),                           \
        NULL,                                   \
        1,                                      \
        MUNIT_SUITE_OPTION_NONE                 \
    }

/**
 * @def MUNIT_TOP_SUITE(prefix_str, suite_array)
 * @brief Define a top-level @c MunitSuite that nests other suites (no direct tests).
 *
 * Expands to a @c MunitSuite initializer equivalent to:
 * @code
 * {
 *   .prefix     = "/<prefix_str>",
 *   .tests      = NULL,
 *   .suites     = <suite_array>,
 *   .iterations = 1,
 *   .options    = MUNIT_SUITE_OPTION_NONE
 * }
 * @endcode
 *
 * @param prefix_str  Literal suite prefix **without** leading slash (e.g. `"all"`).
 * @param suite_array Name of a @c MunitSuite[] array (e.g. @c nested_suites).
 */
#define MUNIT_TOP_SUITE(prefix_str, suite_array)   \
    {                                             \
        "/" prefix_str,                           \
        NULL,                                     \
        (suite_array),                            \
        1,                                        \
        MUNIT_SUITE_OPTION_NONE                   \
    }

/**
 * @def MUNIT_SUITE_END
 * @brief Sentinel terminator for a @c MunitSuite array.
 *
 * Use as the final element in a @c MunitSuite[] list (i.e., @c .prefix == NULL).
 */
#define MUNIT_SUITE_END  \
    {                    \
        NULL,            \
        NULL,            \
        NULL,            \
        0,               \
        0                \
    }

/**
 * @def MUNIT_RANDOMIZED_TEST(iter_count, body)
 * @brief Deterministic loop harness for randomized property tests.
 *
 * Runs @p body exactly @p iter_count times. Each iteration reseeds MUnit’s
 * PRNG with a seed derived from the iteration index:
 * @code
 * munit_rand_seed(0xFFFFFFFFu ^ iteration_index);
 * @endcode
 *
 * This enables reproducible failures (given the same iteration index).
 *
 * @param iter_count Number of iterations to execute.
 * @param body       Braced code block executed on each iteration.
 *
 * @par Example
 * @code
 * MUNIT_RANDOMIZED_TEST(10000, {
 *   uint32_t x = munit_rand_uint32();
 *   munit_assert(x < 42);
 * });
 * @endcode
 *
 * @note To see @c printf output inside @p body, set the environment variables:
 * @code
 * export MUNIT_NO_CAPTURE_STDOUT=1
 * export MUNIT_NO_CAPTURE_STDERR=1
 * export MUNIT_VERBOSE=1
 * @endcode
 */
#define MUNIT_RANDOMIZED_TEST(iter_count, ...)                    \
    do {                                                          \
        for (uint32_t _m_iter = 0; _m_iter < (iter_count); _m_iter++) { \
            uint32_t _m_seed = (0xFFFFFFFFU ^ _m_iter);           \
            munit_rand_seed(_m_seed);                             \
            __VA_ARGS__                                           \
        }                                                         \
    } while (0)
// clang-format on
#endif /* MUNIT_UTILS_H */
