#ifndef CLI_TYPES_H
#define CLI_TYPES_H

#include <stddef.h>
#include <stdint.h>

#include "bench_core.h"
#include "stability.h"

/* ── Test mask ─────────────────────────────────────────────────── */
#define TEST_MASK_HASH        (1U << 0)
#define TEST_MASK_SIG         (1U << 1)
#define TEST_MASK_KEM         (1U << 2)
#define TEST_MASK_KEX         (1U << 3)
#define TEST_MASK_ALL  (TEST_MASK_HASH | TEST_MASK_SIG | TEST_MASK_KEM | TEST_MASK_KEX)

#define NGCC_NUM_TESTS       4
#define NGCC_PATH_BUF_SIZE   1024
#define NGCC_PERF_ITERATIONS 10000
#define NGCC_STABILITY_MSG_LEN 131072

static const size_t k_msg_lens[] = {32, 128, 512, 1024, 4096, 8192, 16384, 65536};
#define NGCC_NUM_MSG_LENS (sizeof(k_msg_lens) / sizeof(k_msg_lens[0]))

/* ── Mode mask ─────────────────────────────────────────────────── */
#define MODE_MASK_CORRECTNESS (1U << 0)
#define MODE_MASK_PERFORMANCE (1U << 1)
#define MODE_MASK_MEMORY      (1U << 2)
#define MODE_MASK_STABILITY   (1U << 3)
#define MODE_MASK_ALL         (MODE_MASK_CORRECTNESS | MODE_MASK_PERFORMANCE | MODE_MASK_MEMORY | MODE_MASK_STABILITY)

/* ── Long-option IDs ───────────────────────────────────────────── */
enum {
    OPT_STABLE_THROUGHPUT_CV = 1000,
    OPT_STABLE_CYCLES_CV,
    OPT_STABLE_TIME_CV,
    OPT_STABLE_MEMORY_GROWTH,
    OPT_STABLE_ERROR_RATE,
    OPT_WARNING_THROUGHPUT_CV,
    OPT_WARNING_CYCLES_CV,
    OPT_WARNING_TIME_CV,
    OPT_WARNING_MEMORY_GROWTH,
    OPT_WARNING_ERROR_RATE,
    OPT_STABILITY_SAMPLE_MS
};

/* ── CLI options ───────────────────────────────────────────────── */
typedef struct {
    const char *lib_path;
    const char *json_out_path;
    const char *kat_path;
    unsigned int test_mask;
    unsigned int mode_mask;
    int digest_len_bits;
    double duration_hours;
    unsigned long long stability_max_cases;
    double stability_sample_ms;
    ngcc_stability_thresholds_t stability_thresholds;
} cli_options_t;

/* ── Test table entry ──────────────────────────────────────────── */
typedef struct {
    unsigned int mask;
    ngcc_test_kind_t kind;
    const char *name;
} test_entry_t;

/* ── Run status ────────────────────────────────────────────────── */
typedef enum {
    STATUS_SKIPPED = 0,
    STATUS_PASS,
    STATUS_FAIL,
    STATUS_STOPPED
} run_status_t;

/* ── Per-test report ───────────────────────────────────────────── */
typedef struct {
    const char *name;
    int selected;
    run_status_t correctness_status;
    run_status_t performance_status;
    run_status_t stability_status;
    run_status_t memory_status;
    int is_hash;  /* 1 = hash (byte throughput), 0 = pubkey (op throughput) */
    ngcc_perf_result_t performance[NGCC_NUM_MSG_LENS];
    const char *performance_labels[NGCC_NUM_MSG_LENS]; /* key labels for JSON (Chinese) */
    const char *performance_labels_en[NGCC_NUM_MSG_LENS]; /* key labels for JSON (English) */
    int performance_count;  /* how many entries actually ran */
    ngcc_stability_result_t stability;
    uint64_t static_memory_bytes;
    uint64_t peak_memory_bytes;
    int kat_used;
    unsigned long long kat_total;
    unsigned long long kat_passed;
    unsigned long long kat_failed;
} test_report_t;

/* ── Overall report ────────────────────────────────────────────── */
typedef struct {
    test_report_t tests[NGCC_NUM_TESTS];
} run_report_t;

/* ── Shared test table (defined in main.c) ─────────────────────── */
extern const test_entry_t k_tests[];

#endif
