/**
 * Unit tests for ngcc_bench core functions.
 *
 * Lightweight assert-based harness — no external dependencies.
 * Exit code 0 = all tests passed, non-zero = failure.
 */
#include <math.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "bench_core.h"
#include "bench_hash.h"
#include "bench_kem.h"
#include "bench_kex.h"
#include "cli_parser.h"
#include "cli_types.h"
#include "json_report.h"
#include "loader.h"
#include "stability.h"
#include "stats_util.h"

#ifndef NGCC_UNIT_MOCK_NGCC
#define NGCC_UNIT_MOCK_NGCC ""
#endif

#ifndef NGCC_UNIT_MOCK_HASH_ONLY
#define NGCC_UNIT_MOCK_HASH_ONLY ""
#endif

/* ── Minimal test harness ─────────────────────────────────────── */

static int g_tests_run = 0;
static int g_tests_failed = 0;

#define TEST_ASSERT(expr)                                                \
    do {                                                                 \
        if (!(expr)) {                                                   \
            fprintf(stderr, "  FAIL  %s:%d: %s\n", __FILE__, __LINE__,  \
                    #expr);                                              \
            g_tests_failed++;                                            \
            return;                                                      \
        }                                                                \
    } while (0)

#define TEST_ASSERT_INT_EQ(a, b)                                         \
    do {                                                                 \
        int _a = (a), _b = (b);                                          \
        if (_a != _b) {                                                  \
            fprintf(stderr, "  FAIL  %s:%d: %d != %d\n", __FILE__,       \
                    __LINE__, _a, _b);                                    \
            g_tests_failed++;                                            \
            return;                                                      \
        }                                                                \
    } while (0)

#define TEST_ASSERT_DOUBLE_NEAR(a, b, tol)                               \
    do {                                                                 \
        double _a = (a), _b = (b);                                       \
        if (fabs(_a - _b) > (tol)) {                                     \
            fprintf(stderr, "  FAIL  %s:%d: %.10f != %.10f (tol %.10f)\n",\
                    __FILE__, __LINE__, _a, _b, (tol));                  \
            g_tests_failed++;                                            \
            return;                                                      \
        }                                                                \
    } while (0)

#define RUN_TEST(fn)                                                     \
    do {                                                                 \
        int _before = g_tests_failed;                                    \
        g_tests_run++;                                                   \
        fn();                                                            \
        if (g_tests_failed == _before) {                                 \
            printf("  PASS  %s\n", #fn);                                 \
        }                                                                \
    } while (0)

static int read_text_file(const char *path, char **out_data) {
    FILE *fp;
    long size;
    char *data;

    if (path == NULL || out_data == NULL) {
        return -1;
    }

    fp = fopen(path, "rb");
    if (fp == NULL) {
        return -1;
    }
    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return -1;
    }
    size = ftell(fp);
    if (size < 0) {
        fclose(fp);
        return -1;
    }
    if (fseek(fp, 0, SEEK_SET) != 0) {
        fclose(fp);
        return -1;
    }

    data = (char *) malloc((size_t) size + 1U);
    if (data == NULL) {
        fclose(fp);
        return -1;
    }
    if (size > 0 && fread(data, 1, (size_t) size, fp) != (size_t) size) {
        fclose(fp);
        free(data);
        return -1;
    }
    fclose(fp);
    data[size] = '\0';
    *out_data = data;
    return 0;
}

static int write_text_file(const char *path, const char *data) {
    FILE *fp;

    if (path == NULL || data == NULL) {
        return -1;
    }

    fp = fopen(path, "wb");
    if (fp == NULL) {
        return -1;
    }
    if (fputs(data, fp) < 0) {
        fclose(fp);
        return -1;
    }
    return fclose(fp) == 0 ? 0 : -1;
}

/* ── stats_util tests ─────────────────────────────────────────── */

static void test_stats_single_value(void) {
    running_stats_t s;
    stats_init(&s);
    stats_update(&s, 42.0);

    TEST_ASSERT(s.count == 1);
    TEST_ASSERT_DOUBLE_NEAR(s.mean, 42.0, 1e-9);
    TEST_ASSERT_DOUBLE_NEAR(s.min, 42.0, 1e-9);
    TEST_ASSERT_DOUBLE_NEAR(s.max, 42.0, 1e-9);
    TEST_ASSERT_DOUBLE_NEAR(stats_stddev(&s), 0.0, 1e-9);
}

static void test_stats_known_dataset(void) {
    /* dataset: {2, 4, 4, 4, 5, 5, 7, 9}
     * mean = 5.0, population variance = 4.0, sample variance = 32/7 ≈ 4.571
     * sample stddev = sqrt(32/7) ≈ 2.138 */
    running_stats_t s;
    double data[] = {2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0};
    size_t i;

    stats_init(&s);
    for (i = 0; i < sizeof(data) / sizeof(data[0]); ++i) {
        stats_update(&s, data[i]);
    }

    TEST_ASSERT(s.count == 8);
    TEST_ASSERT_DOUBLE_NEAR(s.mean, 5.0, 1e-9);
    TEST_ASSERT_DOUBLE_NEAR(s.min, 2.0, 1e-9);
    TEST_ASSERT_DOUBLE_NEAR(s.max, 9.0, 1e-9);
    TEST_ASSERT_DOUBLE_NEAR(stats_stddev(&s), sqrt(32.0 / 7.0), 1e-6);
}

static void test_stats_stddev_zero_count(void) {
    running_stats_t s;
    stats_init(&s);
    TEST_ASSERT_DOUBLE_NEAR(stats_stddev(&s), 0.0, 1e-9);
}

/* ── timespec_ms_diff tests ───────────────────────────────────── */

static void test_timespec_ms_diff_basic(void) {
    struct timespec start = {1, 0};
    struct timespec end = {2, 500000000L}; /* 1.5 seconds later */
    double diff = timespec_ms_diff(&start, &end);
    TEST_ASSERT_DOUBLE_NEAR(diff, 1500.0, 1e-3);
}

static void test_timespec_ms_diff_zero(void) {
    struct timespec t = {5, 100000000L};
    double diff = timespec_ms_diff(&t, &t);
    TEST_ASSERT_DOUBLE_NEAR(diff, 0.0, 1e-9);
}

static void test_timespec_ms_diff_subsecond(void) {
    struct timespec start = {0, 0};
    struct timespec end = {0, 1000000L}; /* 1 ms */
    double diff = timespec_ms_diff(&start, &end);
    TEST_ASSERT_DOUBLE_NEAR(diff, 1.0, 1e-6);
}

static void test_monotonic_clock_gettime_valid(void) {
    struct timespec start;
    struct timespec end;

    TEST_ASSERT_INT_EQ(ngcc_monotonic_clock_gettime(&start), 0);
    TEST_ASSERT_INT_EQ(ngcc_monotonic_clock_gettime(&end), 0);
    TEST_ASSERT(timespec_ms_diff(&start, &end) >= 0.0);
}

static void test_monotonic_clock_gettime_null(void) {
    TEST_ASSERT_INT_EQ(ngcc_monotonic_clock_gettime(NULL), -1);
}

/* ── loader tests ─────────────────────────────────────────────── */

static void test_loader_hash_only_selected_symbols(void) {
    ngcc_library_t lib;

    TEST_ASSERT(NGCC_UNIT_MOCK_HASH_ONLY[0] != '\0');
    TEST_ASSERT_INT_EQ(ngcc_load_library(NGCC_UNIT_MOCK_HASH_ONLY, TEST_MASK_HASH, &lib), 0);
    TEST_ASSERT(lib.handle != NULL);
    TEST_ASSERT(lib.api.CryptHash != NULL);
    TEST_ASSERT(lib.api.sig_get_pk_len_bytes == NULL);
    TEST_ASSERT(lib.api.kem_get_pk_len_bytes == NULL);
    TEST_ASSERT(lib.api.kex_get_passes_num == NULL);
    ngcc_unload_library(&lib);
}

static void test_loader_hash_only_missing_required_symbols(void) {
    ngcc_library_t lib;

    TEST_ASSERT(NGCC_UNIT_MOCK_HASH_ONLY[0] != '\0');
    TEST_ASSERT_INT_EQ(ngcc_load_library(NGCC_UNIT_MOCK_HASH_ONLY, TEST_MASK_SIG, &lib), -1);
    TEST_ASSERT(lib.handle == NULL);
    TEST_ASSERT(lib.api.CryptHash == NULL);
}

static void test_loader_selected_groups_only_loaded(void) {
    ngcc_library_t lib;

    TEST_ASSERT(NGCC_UNIT_MOCK_NGCC[0] != '\0');
    TEST_ASSERT_INT_EQ(ngcc_load_library(NGCC_UNIT_MOCK_NGCC, TEST_MASK_HASH, &lib), 0);
    TEST_ASSERT(lib.handle != NULL);
    TEST_ASSERT(lib.api.CryptHash != NULL);
    TEST_ASSERT(lib.api.sig_keygen == NULL);
    TEST_ASSERT(lib.api.kem_keygen == NULL);
    TEST_ASSERT(lib.api.kex_init_a == NULL);
    ngcc_unload_library(&lib);
}

/* ── parse_test_mask tests ────────────────────────────────────── */

static void test_parse_test_mask_valid(void) {
    unsigned int mask = 0;

    TEST_ASSERT_INT_EQ(parse_test_mask("hash", &mask), 0);
    TEST_ASSERT(mask == TEST_MASK_HASH);

    TEST_ASSERT_INT_EQ(parse_test_mask("sig", &mask), 0);
    TEST_ASSERT(mask == TEST_MASK_SIG);

    TEST_ASSERT_INT_EQ(parse_test_mask("kem", &mask), 0);
    TEST_ASSERT(mask == TEST_MASK_KEM);

    TEST_ASSERT_INT_EQ(parse_test_mask("kex", &mask), 0);
    TEST_ASSERT(mask == TEST_MASK_KEX);

    TEST_ASSERT_INT_EQ(parse_test_mask("all", &mask), 0);
    TEST_ASSERT(mask == TEST_MASK_ALL);
}

static void test_parse_test_mask_invalid(void) {
    unsigned int mask = 0;
    TEST_ASSERT_INT_EQ(parse_test_mask("invalid", &mask), -1);
    TEST_ASSERT_INT_EQ(parse_test_mask("", &mask), -1);
    TEST_ASSERT_INT_EQ(parse_test_mask("dsa", &mask), -1);
    TEST_ASSERT_INT_EQ(parse_test_mask("sig-keygen", &mask), -1);
    TEST_ASSERT_INT_EQ(parse_test_mask("sig-sign", &mask), -1);
    TEST_ASSERT_INT_EQ(parse_test_mask("sig-verify", &mask), -1);
    TEST_ASSERT_INT_EQ(parse_test_mask("kem-keygen", &mask), -1);
    TEST_ASSERT_INT_EQ(parse_test_mask("kem-encap", &mask), -1);
    TEST_ASSERT_INT_EQ(parse_test_mask("kem-decap", &mask), -1);
    TEST_ASSERT_INT_EQ(parse_test_mask("HASH", &mask), -1); /* case-sensitive */
}

/* ── parse_mode_mask tests ────────────────────────────────────── */

static void test_parse_mode_mask_valid(void) {
    unsigned int mask = 0;

    TEST_ASSERT_INT_EQ(parse_mode_mask("correctness", &mask), 0);
    TEST_ASSERT(mask == MODE_MASK_CORRECTNESS);

    TEST_ASSERT_INT_EQ(parse_mode_mask("performance", &mask), 0);
    TEST_ASSERT(mask == MODE_MASK_PERFORMANCE);

    TEST_ASSERT_INT_EQ(parse_mode_mask("memory", &mask), 0);
    TEST_ASSERT(mask == MODE_MASK_MEMORY);

    TEST_ASSERT_INT_EQ(parse_mode_mask("stability", &mask), 0);
    TEST_ASSERT(mask == MODE_MASK_STABILITY);

    TEST_ASSERT_INT_EQ(parse_mode_mask("all", &mask), 0);
    TEST_ASSERT(mask == MODE_MASK_ALL);
}

static void test_parse_mode_mask_invalid(void) {
    unsigned int mask = 0;
    TEST_ASSERT_INT_EQ(parse_mode_mask("wrong", &mask), -1);
    TEST_ASSERT_INT_EQ(parse_mode_mask("", &mask), -1);
}

/* ── parse_unsigned_ll tests ──────────────────────────────────── */

static void test_parse_unsigned_ll_valid(void) {
    unsigned long long val = 0;

    TEST_ASSERT_INT_EQ(parse_unsigned_ll("0", &val), 0);
    TEST_ASSERT(val == 0);

    TEST_ASSERT_INT_EQ(parse_unsigned_ll("12345", &val), 0);
    TEST_ASSERT(val == 12345);

    TEST_ASSERT_INT_EQ(parse_unsigned_ll("18446744073709551615", &val), 0);
    TEST_ASSERT(val == 18446744073709551615ULL);
}

static void test_parse_unsigned_ll_invalid(void) {
    unsigned long long val = 0;
    TEST_ASSERT_INT_EQ(parse_unsigned_ll(NULL, &val), -1);
    TEST_ASSERT_INT_EQ(parse_unsigned_ll("", &val), -1);
    TEST_ASSERT_INT_EQ(parse_unsigned_ll("abc", &val), -1);
    TEST_ASSERT_INT_EQ(parse_unsigned_ll("123abc", &val), -1);
}

/* ── parse_int_value tests ────────────────────────────────────── */

static void test_parse_int_value_valid(void) {
    int val = 0;

    TEST_ASSERT_INT_EQ(parse_int_value("0", &val), 0);
    TEST_ASSERT_INT_EQ(val, 0);

    TEST_ASSERT_INT_EQ(parse_int_value("256", &val), 0);
    TEST_ASSERT_INT_EQ(val, 256);

    TEST_ASSERT_INT_EQ(parse_int_value("-10", &val), 0);
    TEST_ASSERT_INT_EQ(val, -10);
}

static void test_parse_int_value_invalid(void) {
    int val = 0;
#if LONG_MAX > INT_MAX
    char too_big[64];
    TEST_ASSERT(snprintf(too_big, sizeof(too_big), "%ld", (long) INT_MAX + 1L) < (int) sizeof(too_big));
    TEST_ASSERT_INT_EQ(parse_int_value(too_big, &val), -1);
#endif
    TEST_ASSERT_INT_EQ(parse_int_value(NULL, &val), -1);
    TEST_ASSERT_INT_EQ(parse_int_value("", &val), -1);
    TEST_ASSERT_INT_EQ(parse_int_value("xyz", &val), -1);
    TEST_ASSERT_INT_EQ(parse_int_value("999999999999999999999999999999999999", &val), -1);
}

/* ── parse_double_value tests ─────────────────────────────────── */

static void test_parse_double_value_valid(void) {
    double val = 0.0;

    TEST_ASSERT_INT_EQ(parse_double_value("3.14", &val), 0);
    TEST_ASSERT_DOUBLE_NEAR(val, 3.14, 1e-9);

    TEST_ASSERT_INT_EQ(parse_double_value("-1.5", &val), 0);
    TEST_ASSERT_DOUBLE_NEAR(val, -1.5, 1e-9);

    TEST_ASSERT_INT_EQ(parse_double_value("0", &val), 0);
    TEST_ASSERT_DOUBLE_NEAR(val, 0.0, 1e-9);
}

static void test_parse_double_value_invalid(void) {
    double val = 0.0;
    TEST_ASSERT_INT_EQ(parse_double_value(NULL, &val), -1);
    TEST_ASSERT_INT_EQ(parse_double_value("", &val), -1);
    TEST_ASSERT_INT_EQ(parse_double_value("abc", &val), -1);
}

/* ── ngcc_fill_random tests ───────────────────────────────────── */

static void test_fill_random_nonzero(void) {
    unsigned char buf[64];
    int all_zero = 1;
    size_t i;

    memset(buf, 0, sizeof(buf));
    TEST_ASSERT_INT_EQ(ngcc_fill_random(buf, sizeof(buf)), 0);

    for (i = 0; i < sizeof(buf); ++i) {
        if (buf[i] != 0) {
            all_zero = 0;
            break;
        }
    }
    TEST_ASSERT(!all_zero);
}

static void test_fill_random_unique(void) {
    unsigned char buf_a[32];
    unsigned char buf_b[32];

    TEST_ASSERT_INT_EQ(ngcc_fill_random(buf_a, sizeof(buf_a)), 0);
    TEST_ASSERT_INT_EQ(ngcc_fill_random(buf_b, sizeof(buf_b)), 0);
    TEST_ASSERT(memcmp(buf_a, buf_b, sizeof(buf_a)) != 0);
}

static void test_fill_random_null(void) {
    TEST_ASSERT_INT_EQ(ngcc_fill_random(NULL, 16), -1);
}

/* ── ngcc_is_valid_len tests ──────────────────────────────────── */

static void test_is_valid_len(void) {
    TEST_ASSERT(!ngcc_is_valid_len(0));
    TEST_ASSERT(ngcc_is_valid_len(1));
    TEST_ASSERT(ngcc_is_valid_len(NGCC_MAX_BUFFER_LEN));
    TEST_ASSERT(!ngcc_is_valid_len(NGCC_MAX_BUFFER_LEN + 1));
}

static void test_validate_options_rejects_large_digest_len(void) {
    cli_options_t opts;

    init_default_options(&opts);
    opts.lib_path = "/tmp/mock_lib.so";
    opts.test_mask = TEST_MASK_HASH;
    opts.digest_len_bits = (int) (NGCC_MAX_BUFFER_LEN * 8ULL) + 1;

    TEST_ASSERT_INT_EQ(validate_options(&opts), -1);
}

/* ── correctness sentinel tests ───────────────────────────────── */

static int test_hash_no_write(int digest_len_bits,
                              const unsigned char *msg,
                              unsigned long long msg_len_bits,
                              unsigned char *digest) {
    (void) digest_len_bits;
    (void) msg;
    (void) msg_len_bits;
    (void) digest;
    return 0;
}

static int test_hash_xor(int digest_len_bits,
                         const unsigned char *msg,
                         unsigned long long msg_len_bits,
                         unsigned char *digest) {
    unsigned long long msg_len = msg_len_bits / 8ULL;
    unsigned char acc = (unsigned char) digest_len_bits;
    unsigned long long i;

    for (i = 0; i < msg_len; ++i) {
        acc ^= msg[i];
    }
    digest[0] = acc;
    return 0;
}

static void test_hash_correctness_rejects_no_write_digest(void) {
    ngcc_api_t api;

    memset(&api, 0, sizeof(api));
    api.CryptHash = test_hash_no_write;

    TEST_ASSERT_INT_EQ(ngcc_hash_correctness(&api, 64, 16), -1);
}

static void test_hash_kat_rejects_short_msg_for_msg_len(void) {
    char tmp_dir[] = "/tmp/ngcc_unit_hash_kat.XXXXXX";
    char kat_212_path[PATH_MAX];
    char kat_223_path[PATH_MAX];
    char kat_233_path[PATH_MAX];
    char kat_loop_path[PATH_MAX];
    ngcc_api_t api;
    unsigned long long total = 0;
    unsigned long long passed = 0;
    unsigned long long failed = 0;

    static const char *const bad_short_msg =
        "COUNT = 0\n"
        "Msg_Len = 16\n"
        "Msg = 00\n"
        "Dst_Len = 8\n"
        "Dst = 08\n";
    static const char *const valid_vector =
        "COUNT = 0\n"
        "Msg_Len = 8\n"
        "Msg = 00\n"
        "Dst_Len = 8\n"
        "Dst = 08\n";
    static const char *const valid_loop =
        "COUNT = 0\n"
        "Msg_Len = 8\n"
        "Msg = 00\n"
        "Dst_Len = 8\n"
        "Dst = 00\n";

    memset(&api, 0, sizeof(api));
    api.CryptHash = test_hash_xor;

    TEST_ASSERT(mkdtemp(tmp_dir) != NULL);
    TEST_ASSERT(snprintf(kat_212_path, sizeof(kat_212_path), "%s/KAT_2_12_short.txt", tmp_dir) < (int) sizeof(kat_212_path));
    TEST_ASSERT(snprintf(kat_223_path, sizeof(kat_223_path), "%s/KAT_2_23_valid.txt", tmp_dir) < (int) sizeof(kat_223_path));
    TEST_ASSERT(snprintf(kat_233_path, sizeof(kat_233_path), "%s/KAT_2_33_valid.txt", tmp_dir) < (int) sizeof(kat_233_path));
    TEST_ASSERT(snprintf(kat_loop_path, sizeof(kat_loop_path), "%s/KAT_Loop_valid.txt", tmp_dir) < (int) sizeof(kat_loop_path));

    TEST_ASSERT_INT_EQ(write_text_file(kat_212_path, bad_short_msg), 0);
    TEST_ASSERT_INT_EQ(write_text_file(kat_223_path, valid_vector), 0);
    TEST_ASSERT_INT_EQ(write_text_file(kat_233_path, valid_vector), 0);
    TEST_ASSERT_INT_EQ(write_text_file(kat_loop_path, valid_loop), 0);

    TEST_ASSERT_INT_EQ(ngcc_hash_correctness_kat_file(&api, 8, tmp_dir, &total, &passed, &failed), -1);
    TEST_ASSERT(total >= 4);
    TEST_ASSERT(failed >= 1);

    TEST_ASSERT(unlink(kat_212_path) == 0);
    TEST_ASSERT(unlink(kat_223_path) == 0);
    TEST_ASSERT(unlink(kat_233_path) == 0);
    TEST_ASSERT(unlink(kat_loop_path) == 0);
    TEST_ASSERT(rmdir(tmp_dir) == 0);
}

/* ── KEM performance length validation tests ───────────────────── */

#define TEST_KEM_PK_CAP 8ULL
#define TEST_KEM_SK_CAP 8ULL
#define TEST_KEM_CT_CAP 8ULL
#define TEST_KEM_SS_CAP 8ULL

static unsigned long long test_kem_get_pk_len_bytes(void) { return TEST_KEM_PK_CAP; }
static unsigned long long test_kem_get_sk_len_bytes(void) { return TEST_KEM_SK_CAP; }
static unsigned long long test_kem_get_ct_len_bytes(void) { return TEST_KEM_CT_CAP; }
static unsigned long long test_kem_get_ss_len_bytes(void) { return TEST_KEM_SS_CAP; }

static int test_kem_keygen_valid(unsigned char *pk, unsigned long long *pk_len_bytes,
                                 unsigned char *sk, unsigned long long *sk_len_bytes) {
    memset(pk, 0xA5, (size_t) TEST_KEM_PK_CAP);
    memset(sk, 0x5A, (size_t) TEST_KEM_SK_CAP);
    *pk_len_bytes = TEST_KEM_PK_CAP;
    *sk_len_bytes = TEST_KEM_SK_CAP;
    return 0;
}

static int test_kem_keygen_bad_pk_len(unsigned char *pk, unsigned long long *pk_len_bytes,
                                      unsigned char *sk, unsigned long long *sk_len_bytes) {
    memset(pk, 0xA5, (size_t) TEST_KEM_PK_CAP);
    memset(sk, 0x5A, (size_t) TEST_KEM_SK_CAP);
    *pk_len_bytes = TEST_KEM_PK_CAP + 1ULL;
    *sk_len_bytes = TEST_KEM_SK_CAP;
    return 0;
}

static int test_kem_keygen_bad_sk_len(unsigned char *pk, unsigned long long *pk_len_bytes,
                                      unsigned char *sk, unsigned long long *sk_len_bytes) {
    memset(pk, 0xA5, (size_t) TEST_KEM_PK_CAP);
    memset(sk, 0x5A, (size_t) TEST_KEM_SK_CAP);
    *pk_len_bytes = TEST_KEM_PK_CAP;
    *sk_len_bytes = TEST_KEM_SK_CAP + 1ULL;
    return 0;
}

static int test_kem_enc_valid(unsigned char *pk, unsigned long long pk_len_bytes,
                              unsigned char *ss, unsigned long long *ss_len_bytes,
                              unsigned char *ct, unsigned long long *ct_len_bytes) {
    (void) pk;
    (void) pk_len_bytes;
    memset(ss, 0x11, (size_t) TEST_KEM_SS_CAP);
    memset(ct, 0x22, (size_t) TEST_KEM_CT_CAP);
    *ss_len_bytes = TEST_KEM_SS_CAP;
    *ct_len_bytes = TEST_KEM_CT_CAP;
    return 0;
}

static int test_kem_enc_bad_ct_len(unsigned char *pk, unsigned long long pk_len_bytes,
                                   unsigned char *ss, unsigned long long *ss_len_bytes,
                                   unsigned char *ct, unsigned long long *ct_len_bytes) {
    (void) pk;
    (void) pk_len_bytes;
    memset(ss, 0x11, (size_t) TEST_KEM_SS_CAP);
    memset(ct, 0x22, (size_t) TEST_KEM_CT_CAP);
    *ss_len_bytes = TEST_KEM_SS_CAP;
    *ct_len_bytes = TEST_KEM_CT_CAP + 1ULL;
    return 0;
}

static int test_kem_enc_no_write_ss(unsigned char *pk, unsigned long long pk_len_bytes,
                                    unsigned char *ss, unsigned long long *ss_len_bytes,
                                    unsigned char *ct, unsigned long long *ct_len_bytes) {
    (void) pk;
    (void) pk_len_bytes;
    (void) ss;
    memset(ct, 0x22, (size_t) TEST_KEM_CT_CAP);
    *ss_len_bytes = TEST_KEM_SS_CAP;
    *ct_len_bytes = TEST_KEM_CT_CAP;
    return 0;
}

static int test_kem_dec_valid(unsigned char *sk, unsigned long long sk_len_bytes,
                              unsigned char *ct, unsigned long long ct_len_bytes,
                              unsigned char *ss, unsigned long long *ss_len_bytes) {
    (void) sk;
    (void) sk_len_bytes;
    (void) ct;
    (void) ct_len_bytes;
    memset(ss, 0x11, (size_t) TEST_KEM_SS_CAP);
    *ss_len_bytes = TEST_KEM_SS_CAP;
    return 0;
}

static int test_kem_dec_bad_ss_len(unsigned char *sk, unsigned long long sk_len_bytes,
                                   unsigned char *ct, unsigned long long ct_len_bytes,
                                   unsigned char *ss, unsigned long long *ss_len_bytes) {
    (void) sk;
    (void) sk_len_bytes;
    (void) ct;
    (void) ct_len_bytes;
    memset(ss, 0x11, (size_t) TEST_KEM_SS_CAP);
    *ss_len_bytes = TEST_KEM_SS_CAP + 1ULL;
    return 0;
}

static int test_kem_dec_no_write_ss(unsigned char *sk, unsigned long long sk_len_bytes,
                                    unsigned char *ct, unsigned long long ct_len_bytes,
                                    unsigned char *ss, unsigned long long *ss_len_bytes) {
    (void) sk;
    (void) sk_len_bytes;
    (void) ct;
    (void) ct_len_bytes;
    (void) ss;
    *ss_len_bytes = TEST_KEM_SS_CAP;
    return 0;
}

static void init_test_kem_api(ngcc_api_t *api,
                              int (*keygen_fn)(unsigned char *, unsigned long long *,
                                                unsigned char *, unsigned long long *),
                              int (*enc_fn)(unsigned char *, unsigned long long,
                                             unsigned char *, unsigned long long *,
                                             unsigned char *, unsigned long long *),
                              int (*dec_fn)(unsigned char *, unsigned long long,
                                             unsigned char *, unsigned long long,
                                             unsigned char *, unsigned long long *)) {
    memset(api, 0, sizeof(*api));
    api->kem_get_pk_len_bytes = test_kem_get_pk_len_bytes;
    api->kem_get_sk_len_bytes = test_kem_get_sk_len_bytes;
    api->kem_get_ct_len_bytes = test_kem_get_ct_len_bytes;
    api->kem_get_ss_len_bytes = test_kem_get_ss_len_bytes;
    api->kem_keygen = keygen_fn;
    api->kem_enc = enc_fn;
    api->kem_dec = dec_fn;
}

static ngcc_perf_config_t test_kem_perf_config(void) {
    ngcc_perf_config_t cfg;
    cfg.iterations = 1;
    cfg.bytes_per_op = 0;
    return cfg;
}

static void test_kem_correctness_rejects_no_write_shared_secret(void) {
    ngcc_api_t api;

    init_test_kem_api(&api, test_kem_keygen_valid, test_kem_enc_no_write_ss, test_kem_dec_no_write_ss);

    TEST_ASSERT_INT_EQ(ngcc_kem_correctness(&api), -1);
}

static void test_kem_keygen_performance_rejects_bad_lengths(void) {
    ngcc_api_t api;
    ngcc_perf_config_t cfg = test_kem_perf_config();
    ngcc_perf_result_t result;

    memset(&result, 0, sizeof(result));
    init_test_kem_api(&api, test_kem_keygen_bad_pk_len, test_kem_enc_valid, test_kem_dec_valid);

    TEST_ASSERT_INT_EQ(ngcc_kem_keygen_performance(&api, &cfg, &result), -1);
}

static void test_kem_encap_performance_rejects_bad_setup_key_length(void) {
    ngcc_api_t api;
    ngcc_perf_config_t cfg = test_kem_perf_config();
    ngcc_perf_result_t result;

    memset(&result, 0, sizeof(result));
    init_test_kem_api(&api, test_kem_keygen_bad_pk_len, test_kem_enc_valid, test_kem_dec_valid);

    TEST_ASSERT_INT_EQ(ngcc_kem_encap_performance(&api, &cfg, &result), -1);
}

static void test_kem_encap_performance_rejects_bad_encap_length(void) {
    ngcc_api_t api;
    ngcc_perf_config_t cfg = test_kem_perf_config();
    ngcc_perf_result_t result;

    memset(&result, 0, sizeof(result));
    init_test_kem_api(&api, test_kem_keygen_valid, test_kem_enc_bad_ct_len, test_kem_dec_valid);

    TEST_ASSERT_INT_EQ(ngcc_kem_encap_performance(&api, &cfg, &result), -1);
}

static void test_kem_decap_performance_rejects_bad_setup_key_length(void) {
    ngcc_api_t api;
    ngcc_perf_config_t cfg = test_kem_perf_config();
    ngcc_perf_result_t result;

    memset(&result, 0, sizeof(result));
    init_test_kem_api(&api, test_kem_keygen_bad_sk_len, test_kem_enc_valid, test_kem_dec_valid);

    TEST_ASSERT_INT_EQ(ngcc_kem_decap_performance(&api, &cfg, &result), -1);
}

static void test_kem_decap_performance_rejects_bad_decap_length(void) {
    ngcc_api_t api;
    ngcc_perf_config_t cfg = test_kem_perf_config();
    ngcc_perf_result_t result;

    memset(&result, 0, sizeof(result));
    init_test_kem_api(&api, test_kem_keygen_valid, test_kem_enc_valid, test_kem_dec_bad_ss_len);

    TEST_ASSERT_INT_EQ(ngcc_kem_decap_performance(&api, &cfg, &result), -1);
}

/* ── KEX correctness sentinel tests ────────────────────────────── */

#define TEST_KEX_CAP 8ULL
#define TEST_KEX_PASSES 2ULL

static unsigned long long test_kex_get_passes_num(void) { return TEST_KEX_PASSES; }
static unsigned long long test_kex_get_pk_len_bytes(void) { return TEST_KEX_CAP; }
static unsigned long long test_kex_get_sk_len_bytes(void) { return TEST_KEX_CAP; }
static unsigned long long test_kex_get_sta_len_bytes(void) { return TEST_KEX_CAP; }
static unsigned long long test_kex_get_stb_len_bytes(void) { return TEST_KEX_CAP; }
static unsigned long long test_kex_get_ss_len_bytes(void) { return TEST_KEX_CAP; }
static unsigned long long test_kex_get_total_msg_len_bytes(void) { return TEST_KEX_CAP; }

static int test_kex_init_a_valid(unsigned char *pka, unsigned long long *pka_len_bytes,
                                 unsigned char *ska, unsigned long long *ska_len_bytes,
                                 unsigned char *sta, unsigned long long *sta_len_bytes) {
    memset(pka, 0xA1, (size_t) TEST_KEX_CAP);
    memset(ska, 0xA2, (size_t) TEST_KEX_CAP);
    memset(sta, 0xA3, (size_t) TEST_KEX_CAP);
    *pka_len_bytes = TEST_KEX_CAP;
    *ska_len_bytes = TEST_KEX_CAP;
    *sta_len_bytes = TEST_KEX_CAP;
    return 0;
}

static int test_kex_init_a_bad_pka_len(unsigned char *pka, unsigned long long *pka_len_bytes,
                                       unsigned char *ska, unsigned long long *ska_len_bytes,
                                       unsigned char *sta, unsigned long long *sta_len_bytes) {
    if (test_kex_init_a_valid(pka, pka_len_bytes, ska, ska_len_bytes, sta, sta_len_bytes) != 0) {
        return -1;
    }
    *pka_len_bytes = TEST_KEX_CAP + 1ULL;
    return 0;
}

static int test_kex_init_b_valid(unsigned char *pkb, unsigned long long *pkb_len_bytes,
                                 unsigned char *skb, unsigned long long *skb_len_bytes,
                                 unsigned char *stb, unsigned long long *stb_len_bytes) {
    memset(pkb, 0xB1, (size_t) TEST_KEX_CAP);
    memset(skb, 0xB2, (size_t) TEST_KEX_CAP);
    memset(stb, 0xB3, (size_t) TEST_KEX_CAP);
    *pkb_len_bytes = TEST_KEX_CAP;
    *skb_len_bytes = TEST_KEX_CAP;
    *stb_len_bytes = TEST_KEX_CAP;
    return 0;
}

static int test_kex_pass1_valid(unsigned char *sk, unsigned long long sk_len,
                                unsigned char *pk, unsigned long long pk_len,
                                unsigned char *st, unsigned long long *st_len,
                                unsigned char *m_out, unsigned long long *m_out_len) {
    (void) sk;
    (void) sk_len;
    (void) pk;
    (void) pk_len;
    memset(st, 0xC1, (size_t) TEST_KEX_CAP);
    memset(m_out, 0xD1, (size_t) TEST_KEX_CAP);
    *st_len = TEST_KEX_CAP;
    *m_out_len = TEST_KEX_CAP;
    return 0;
}

static int test_kex_pass1_bad_msg_len(unsigned char *sk, unsigned long long sk_len,
                                      unsigned char *pk, unsigned long long pk_len,
                                      unsigned char *st, unsigned long long *st_len,
                                      unsigned char *m_out, unsigned long long *m_out_len) {
    if (test_kex_pass1_valid(sk, sk_len, pk, pk_len, st, st_len, m_out, m_out_len) != 0) {
        return -1;
    }
    *m_out_len = TEST_KEX_CAP + 1ULL;
    return 0;
}

static int test_kex_pass2_valid(unsigned char *sk, unsigned long long sk_len,
                                unsigned char *pk, unsigned long long pk_len,
                                unsigned char *m_in, unsigned long long m_in_len,
                                unsigned char *st, unsigned long long *st_len,
                                unsigned char *m_out, unsigned long long *m_out_len) {
    (void) sk;
    (void) sk_len;
    (void) pk;
    (void) pk_len;
    (void) m_in;
    (void) m_in_len;
    memset(st, 0xC2, (size_t) TEST_KEX_CAP);
    memset(m_out, 0xD2, (size_t) TEST_KEX_CAP);
    *st_len = TEST_KEX_CAP;
    *m_out_len = TEST_KEX_CAP;
    return 0;
}

static int test_kex_pass2_bad_msg_len(unsigned char *sk, unsigned long long sk_len,
                                      unsigned char *pk, unsigned long long pk_len,
                                      unsigned char *m_in, unsigned long long m_in_len,
                                      unsigned char *st, unsigned long long *st_len,
                                      unsigned char *m_out, unsigned long long *m_out_len) {
    if (test_kex_pass2_valid(sk, sk_len, pk, pk_len, m_in, m_in_len, st, st_len, m_out, m_out_len) != 0) {
        return -1;
    }
    *m_out_len = TEST_KEX_CAP + 1ULL;
    return 0;
}

static int test_kex_derive_ss_a_no_write(unsigned char *ska, unsigned long long ska_len_bytes,
                                         unsigned char *pkb, unsigned long long pkb_len_bytes,
                                         unsigned char *mb, unsigned long long mb_len_bytes,
                                         unsigned char *sta, unsigned long long sta_len_bytes,
                                         unsigned char *ssa, unsigned long long *ssa_len_bytes) {
    (void) ska;
    (void) ska_len_bytes;
    (void) pkb;
    (void) pkb_len_bytes;
    (void) mb;
    (void) mb_len_bytes;
    (void) sta;
    (void) sta_len_bytes;
    (void) ssa;
    *ssa_len_bytes = TEST_KEX_CAP;
    return 0;
}

static int test_kex_derive_ss_a_bad_len(unsigned char *ska, unsigned long long ska_len_bytes,
                                        unsigned char *pkb, unsigned long long pkb_len_bytes,
                                        unsigned char *mb, unsigned long long mb_len_bytes,
                                        unsigned char *sta, unsigned long long sta_len_bytes,
                                        unsigned char *ssa, unsigned long long *ssa_len_bytes) {
    (void) ska;
    (void) ska_len_bytes;
    (void) pkb;
    (void) pkb_len_bytes;
    (void) mb;
    (void) mb_len_bytes;
    (void) sta;
    (void) sta_len_bytes;
    memset(ssa, 0x44, (size_t) TEST_KEX_CAP);
    *ssa_len_bytes = TEST_KEX_CAP + 1ULL;
    return 0;
}

static int test_kex_derive_ss_b_no_write(unsigned char *skb, unsigned long long skb_len_bytes,
                                         unsigned char *pka, unsigned long long pka_len_bytes,
                                         unsigned char *ma, unsigned long long ma_len_bytes,
                                         unsigned char *stb, unsigned long long stb_len_bytes,
                                         unsigned char *ssb, unsigned long long *ssb_len_bytes) {
    (void) skb;
    (void) skb_len_bytes;
    (void) pka;
    (void) pka_len_bytes;
    (void) ma;
    (void) ma_len_bytes;
    (void) stb;
    (void) stb_len_bytes;
    (void) ssb;
    *ssb_len_bytes = TEST_KEX_CAP;
    return 0;
}

static kex_pass_fn_t test_kex_pass_fns[] = {
    test_kex_pass2_valid
};

static kex_pass_fn_t test_kex_bad_pass_fns[] = {
    test_kex_pass2_bad_msg_len
};

static void init_test_kex_no_write_api(ngcc_api_t *api) {
    memset(api, 0, sizeof(*api));
    api->kex_get_passes_num = test_kex_get_passes_num;
    api->kex_get_pk_len_bytes = test_kex_get_pk_len_bytes;
    api->kex_get_sk_len_bytes = test_kex_get_sk_len_bytes;
    api->kex_get_sta_len_bytes = test_kex_get_sta_len_bytes;
    api->kex_get_stb_len_bytes = test_kex_get_stb_len_bytes;
    api->kex_get_ss_len_bytes = test_kex_get_ss_len_bytes;
    api->kex_get_total_msg_len_bytes = test_kex_get_total_msg_len_bytes;
    api->kex_init_a = test_kex_init_a_valid;
    api->kex_init_b = test_kex_init_b_valid;
    api->kex_passes_num = TEST_KEX_PASSES;
    api->kex_pass1_fn = test_kex_pass1_valid;
    api->kex_pass_fns = test_kex_pass_fns;
    api->kex_derive_ss_a = test_kex_derive_ss_a_no_write;
    api->kex_derive_ss_b = test_kex_derive_ss_b_no_write;
}

static ngcc_perf_config_t test_kex_perf_config(void) {
    ngcc_perf_config_t cfg;
    cfg.iterations = 1;
    cfg.bytes_per_op = 0;
    return cfg;
}

static void test_kex_correctness_rejects_no_write_shared_secret(void) {
    ngcc_api_t api;

    init_test_kex_no_write_api(&api);

    TEST_ASSERT_INT_EQ(ngcc_kex_correctness(&api), -1);
}

static void test_kex_correctness_rejects_one_pass(void) {
    ngcc_api_t api;

    memset(&api, 0, sizeof(api));
    api.kex_passes_num = NGCC_KEX_MIN_PASSES - 1ULL;

    TEST_ASSERT_INT_EQ(ngcc_kex_correctness(&api), -1);
}

static void test_kex_derive_performance_rejects_one_pass(void) {
    ngcc_api_t api;
    ngcc_perf_config_t cfg = test_kex_perf_config();
    ngcc_perf_result_t out_a;
    ngcc_perf_result_t out_b;

    memset(&api, 0, sizeof(api));
    memset(&out_a, 0, sizeof(out_a));
    memset(&out_b, 0, sizeof(out_b));
    api.kex_passes_num = NGCC_KEX_MIN_PASSES - 1ULL;

    TEST_ASSERT_INT_EQ(ngcc_kex_derive_ss_performance(&api, &cfg, &out_a, &out_b), -1);
}

static void test_kex_derive_performance_rejects_bad_init_length(void) {
    ngcc_api_t api;
    ngcc_perf_config_t cfg = test_kex_perf_config();
    ngcc_perf_result_t out_a;
    ngcc_perf_result_t out_b;

    memset(&out_a, 0, sizeof(out_a));
    memset(&out_b, 0, sizeof(out_b));
    init_test_kex_no_write_api(&api);
    api.kex_init_a = test_kex_init_a_bad_pka_len;

    TEST_ASSERT_INT_EQ(ngcc_kex_derive_ss_performance(&api, &cfg, &out_a, &out_b), -1);
}

static void test_kex_derive_performance_rejects_bad_pass1_length(void) {
    ngcc_api_t api;
    ngcc_perf_config_t cfg = test_kex_perf_config();
    ngcc_perf_result_t out_a;
    ngcc_perf_result_t out_b;

    memset(&out_a, 0, sizeof(out_a));
    memset(&out_b, 0, sizeof(out_b));
    init_test_kex_no_write_api(&api);
    api.kex_pass1_fn = test_kex_pass1_bad_msg_len;

    TEST_ASSERT_INT_EQ(ngcc_kex_derive_ss_performance(&api, &cfg, &out_a, &out_b), -1);
}

static void test_kex_derive_performance_rejects_bad_pass2_length(void) {
    ngcc_api_t api;
    ngcc_perf_config_t cfg = test_kex_perf_config();
    ngcc_perf_result_t out_a;
    ngcc_perf_result_t out_b;

    memset(&out_a, 0, sizeof(out_a));
    memset(&out_b, 0, sizeof(out_b));
    init_test_kex_no_write_api(&api);
    api.kex_pass_fns = test_kex_bad_pass_fns;

    TEST_ASSERT_INT_EQ(ngcc_kex_derive_ss_performance(&api, &cfg, &out_a, &out_b), -1);
}

static void test_kex_derive_performance_rejects_bad_derive_length(void) {
    ngcc_api_t api;
    ngcc_perf_config_t cfg = test_kex_perf_config();
    ngcc_perf_result_t out_a;
    ngcc_perf_result_t out_b;

    memset(&out_a, 0, sizeof(out_a));
    memset(&out_b, 0, sizeof(out_b));
    init_test_kex_no_write_api(&api);
    api.kex_derive_ss_a = test_kex_derive_ss_a_bad_len;

    TEST_ASSERT_INT_EQ(ngcc_kex_derive_ss_performance(&api, &cfg, &out_a, &out_b), -1);
}

/* ── stability thresholds defaults tests ──────────────────────── */

static void test_stability_thresholds_defaults(void) {
    ngcc_stability_thresholds_t t;
    memset(&t, 0, sizeof(t));
    ngcc_stability_thresholds_set_defaults(&t);

    TEST_ASSERT(t.stable_throughput_cv_percent > 0.0);
    TEST_ASSERT(t.stable_cycles_cv_percent > 0.0);
    TEST_ASSERT(t.stable_time_cv_percent > 0.0);
    TEST_ASSERT(t.stable_heap_growth_percent > 0.0);
    TEST_ASSERT(t.stable_heap_growth_abs_bytes > 0);
    TEST_ASSERT(t.stable_rss_growth_percent > 0.0);
    TEST_ASSERT(t.stable_rss_growth_abs_bytes > 0);
    TEST_ASSERT(t.stable_error_rate_percent >= 0.0);
}

/* ── json_report tests ─────────────────────────────────────────── */

static void test_write_json_report_basic(void) {
    static const char *const k_test_names[NGCC_NUM_TESTS] = {
        "hash", "sig", "kem", "kex"
    };
    char tmp_dir[] = "/tmp/ngcc_unit_json.XXXXXX";
    char json_path[PATH_MAX];
    char link_path[PATH_MAX];
    char target_path[PATH_MAX];
    cli_options_t opts;
    run_report_t report;
    char *json_data = NULL;
    FILE *fp;
    size_t i;

    TEST_ASSERT(mkdtemp(tmp_dir) != NULL);
    TEST_ASSERT(snprintf(json_path, sizeof(json_path), "%s/report.json", tmp_dir) < (int) sizeof(json_path));
    TEST_ASSERT(snprintf(link_path, sizeof(link_path), "%s/report-link.json", tmp_dir) < (int) sizeof(link_path));
    TEST_ASSERT(snprintf(target_path, sizeof(target_path), "%s/report-target.json", tmp_dir) < (int) sizeof(target_path));

    init_default_options(&opts);
    memset(&report, 0, sizeof(report));
    opts.lib_path = "/tmp/mock_lib.so";
    opts.json_out_path = json_path;
    opts.kat_path = "/tmp/vectors.kat";


    for (i = 0; i < NGCC_NUM_TESTS; ++i) {
        report.tests[i].name = k_test_names[i];
        report.tests[i].correctness_status = STATUS_SKIPPED;
        report.tests[i].performance_status = STATUS_SKIPPED;
        report.tests[i].stability_status = STATUS_SKIPPED;
    }

    report.tests[0].selected = 1;
    report.tests[0].correctness_status = STATUS_PASS;
    report.tests[0].performance_status = STATUS_PASS;
    report.tests[0].stability_status = STATUS_PASS;
    report.tests[0].kat_used = 1;
    report.tests[0].kat_total = 3;
    report.tests[0].kat_passed = 3;
    report.tests[0].kat_failed = 0;
    report.tests[0].performance[0].iterations = 8;
    report.tests[0].performance[0].warmup_iterations = 10;
    report.tests[0].performance[0].elapsed_ms = 1.5;
    report.tests[0].performance[0].bytes_per_op = 64.0;
    report.tests[0].performance[0].ops_per_sec = 1000.0;
    report.tests[0].performance[0].bytes_per_sec = 64000.0;
    report.tests[0].performance[0].time_ms_mean = 0.2;
    report.tests[0].performance[0].time_ms_median = 0.2;
    report.tests[0].performance[0].time_ms_stddev = 0.01;
    report.tests[0].performance[0].time_ms_cv_percent = 5.0;
    report.tests[0].stability.status[0] = 'W';
    strcpy(report.tests[0].stability.status, "UNSTABLE");

    /* Test Chinese output */
    TEST_ASSERT_INT_EQ(write_json_report(&opts, &report, 1, LANG_ZH, json_path), 0);
    TEST_ASSERT_INT_EQ(read_text_file(json_path, &json_data), 0);
    TEST_ASSERT(strstr(json_data, "\"\u62a5\u544a\u7248\u672c\": 4") != NULL);
    TEST_ASSERT(strstr(json_data, "\"\u7b97\u6cd5\u5e93\u8def\u5f84\": \"/tmp/mock_lib.so\"") != NULL);
    TEST_ASSERT(strstr(json_data, "\"\u7a33\u5b9a\u6027\": \"UNSTABLE\"") != NULL);
    free(json_data);
    json_data = NULL;
    TEST_ASSERT(unlink(json_path) == 0);

    /* Test English output */
    TEST_ASSERT_INT_EQ(write_json_report(&opts, &report, 1, LANG_EN, json_path), 0);
    TEST_ASSERT_INT_EQ(read_text_file(json_path, &json_data), 0);
    TEST_ASSERT(strstr(json_data, "\"schema_version\": 4") != NULL);
    TEST_ASSERT(strstr(json_data, "\"library\": \"/tmp/mock_lib.so\"") != NULL);
    TEST_ASSERT(strstr(json_data, "\"stability\": \"UNSTABLE\"") != NULL);
    TEST_ASSERT(strstr(json_data, "\"overall\"") != NULL);
    free(json_data);
    json_data = NULL;
    TEST_ASSERT(unlink(json_path) == 0);

    fp = fopen(json_path, "wb");
    TEST_ASSERT(fp != NULL);
    TEST_ASSERT(fputs("keep", fp) >= 0);
    TEST_ASSERT(fclose(fp) == 0);
    TEST_ASSERT_INT_EQ(write_json_report(&opts, &report, 1, LANG_EN, json_path), -1);
    TEST_ASSERT_INT_EQ(read_text_file(json_path, &json_data), 0);
    TEST_ASSERT(strcmp(json_data, "keep") == 0);
    free(json_data);
    json_data = NULL;
    TEST_ASSERT(unlink(json_path) == 0);

    fp = fopen(target_path, "wb");
    TEST_ASSERT(fp != NULL);
    TEST_ASSERT(fputs("target", fp) >= 0);
    TEST_ASSERT(fclose(fp) == 0);
    TEST_ASSERT(symlink(target_path, link_path) == 0);
    TEST_ASSERT_INT_EQ(write_json_report(&opts, &report, 1, LANG_EN, link_path), -1);
    TEST_ASSERT_INT_EQ(read_text_file(target_path, &json_data), 0);
    TEST_ASSERT(strcmp(json_data, "target") == 0);
    free(json_data);
    json_data = NULL;
    TEST_ASSERT(unlink(link_path) == 0);
    TEST_ASSERT(unlink(target_path) == 0);

    TEST_ASSERT(rmdir(tmp_dir) == 0);
}

/* ── stability runner tests ───────────────────────────────────── */

static int stability_stub_success(const ngcc_api_t *api, int digest_len_bits, size_t msg_len) {
    (void) api;
    (void) digest_len_bits;
    (void) msg_len;
    return 0;
}

static int stability_stub_fail(const ngcc_api_t *api, int digest_len_bits, size_t msg_len) {
    (void) api;
    (void) digest_len_bits;
    (void) msg_len;
    return -1;
}

static unsigned long long stability_stub_bytes(const ngcc_api_t *api, size_t msg_len) {
    (void) api;
    return (unsigned long long) msg_len;
}

static void test_run_stability_single_success(void) {
    ngcc_api_t api;
    ngcc_stability_result_t result;
    ngcc_stability_thresholds_t thresholds;

    memset(&api, 0, sizeof(api));
    memset(&result, 0, sizeof(result));
    ngcc_stability_thresholds_set_defaults(&thresholds);
    thresholds.stable_throughput_cv_percent = 100.0;
    thresholds.stable_time_cv_percent = 100.0;
    thresholds.stable_heap_growth_percent = 100.0;
    thresholds.stable_rss_growth_percent = 100.0;
    TEST_ASSERT_INT_EQ(ngcc_run_stability(&api,
                                          stability_stub_success,
                                          stability_stub_bytes,
                                          0,
                                          64,
                                          0,
                                          1.0,
                                          0.0001,
                                          1,
                                          &thresholds,
                                          &result),
                       0);
    TEST_ASSERT(result.cases_run == 1);
    TEST_ASSERT(result.total_executions == 1);
    TEST_ASSERT(result.error_count == 0);
    TEST_ASSERT_DOUBLE_NEAR(result.bytes_per_case, 64.0, 1e-9);
    TEST_ASSERT(strcmp(result.status, "STABLE") == 0);
}

static void test_run_stability_single_failure(void) {
    ngcc_api_t api;
    ngcc_stability_result_t result;

    memset(&api, 0, sizeof(api));
    memset(&result, 0, sizeof(result));
    TEST_ASSERT_INT_EQ(ngcc_run_stability(&api,
                                          stability_stub_fail,
                                          stability_stub_bytes,
                                          0,
                                          64,
                                          0,
                                          1.0,
                                          0.0001,
                                          1,
                                          NULL,
                                          &result),
                       -1);
    TEST_ASSERT(result.cases_run == 0);
    TEST_ASSERT(result.total_executions == 1);
    TEST_ASSERT(result.error_count == 1);
    TEST_ASSERT(result.failed);
    TEST_ASSERT(strstr(result.failure_reasons, "runtime errors;") != NULL);
}

/* ── main ─────────────────────────────────────────────────────── */

int main(void) {
    printf("=== ngcc_bench unit tests ===\n");

    /* stats_util */
    RUN_TEST(test_stats_single_value);
    RUN_TEST(test_stats_known_dataset);
    RUN_TEST(test_stats_stddev_zero_count);

    /* timespec */
    RUN_TEST(test_timespec_ms_diff_basic);
    RUN_TEST(test_timespec_ms_diff_zero);
    RUN_TEST(test_timespec_ms_diff_subsecond);
    RUN_TEST(test_monotonic_clock_gettime_valid);
    RUN_TEST(test_monotonic_clock_gettime_null);

    /* loader */
    RUN_TEST(test_loader_hash_only_selected_symbols);
    RUN_TEST(test_loader_hash_only_missing_required_symbols);
    RUN_TEST(test_loader_selected_groups_only_loaded);

    /* parse_test_mask */
    RUN_TEST(test_parse_test_mask_valid);
    RUN_TEST(test_parse_test_mask_invalid);

    /* parse_mode_mask */
    RUN_TEST(test_parse_mode_mask_valid);
    RUN_TEST(test_parse_mode_mask_invalid);

    /* parse_unsigned_ll */
    RUN_TEST(test_parse_unsigned_ll_valid);
    RUN_TEST(test_parse_unsigned_ll_invalid);

    /* parse_int_value */
    RUN_TEST(test_parse_int_value_valid);
    RUN_TEST(test_parse_int_value_invalid);

    /* parse_double_value */
    RUN_TEST(test_parse_double_value_valid);
    RUN_TEST(test_parse_double_value_invalid);

    /* ngcc_fill_random */
    RUN_TEST(test_fill_random_nonzero);
    RUN_TEST(test_fill_random_unique);
    RUN_TEST(test_fill_random_null);

    /* ngcc_is_valid_len */
    RUN_TEST(test_is_valid_len);
    RUN_TEST(test_validate_options_rejects_large_digest_len);

    /* correctness sentinels */
    RUN_TEST(test_hash_correctness_rejects_no_write_digest);
    RUN_TEST(test_hash_kat_rejects_short_msg_for_msg_len);
    RUN_TEST(test_kem_correctness_rejects_no_write_shared_secret);
    RUN_TEST(test_kex_correctness_rejects_no_write_shared_secret);
    RUN_TEST(test_kex_correctness_rejects_one_pass);
    RUN_TEST(test_kex_derive_performance_rejects_one_pass);
    RUN_TEST(test_kex_derive_performance_rejects_bad_init_length);
    RUN_TEST(test_kex_derive_performance_rejects_bad_pass1_length);
    RUN_TEST(test_kex_derive_performance_rejects_bad_pass2_length);
    RUN_TEST(test_kex_derive_performance_rejects_bad_derive_length);

    /* KEM performance length validation */
    RUN_TEST(test_kem_keygen_performance_rejects_bad_lengths);
    RUN_TEST(test_kem_encap_performance_rejects_bad_setup_key_length);
    RUN_TEST(test_kem_encap_performance_rejects_bad_encap_length);
    RUN_TEST(test_kem_decap_performance_rejects_bad_setup_key_length);
    RUN_TEST(test_kem_decap_performance_rejects_bad_decap_length);

    /* stability thresholds defaults */
    RUN_TEST(test_stability_thresholds_defaults);

    /* json_report */
    RUN_TEST(test_write_json_report_basic);

    /* stability runner */
    RUN_TEST(test_run_stability_single_success);
    RUN_TEST(test_run_stability_single_failure);

    printf("\n%d/%d tests passed\n", g_tests_run - g_tests_failed, g_tests_run);

    return g_tests_failed > 0 ? 1 : 0;
}
