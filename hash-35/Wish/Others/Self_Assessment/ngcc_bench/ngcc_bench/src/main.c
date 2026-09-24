#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "bench_hash.h"
#include "bench_kem.h"
#include "bench_kex.h"
#include "bench_sig.h"
#include "cli_parser.h"
#include "cli_types.h"
#include "interactive.h"
#include "json_report.h"
#include "loader.h"
#include "mem_stat.h"
#include "ngcc_log.h"
#include "stability.h"

/* ── Shared test table ─────────────────────────────────────────── */

typedef int (*ngcc_kat_dispatch_fn)(const ngcc_api_t *api,
                                    int digest_len_bits,
                                    const char *kat_path,
                                    unsigned long long *out_total,
                                    unsigned long long *out_passed,
                                    unsigned long long *out_failed);

typedef int (*ngcc_performance_dispatch_fn)(const ngcc_api_t *api,
                                            int digest_len_bits,
                                            size_t msg_len,
                                            const ngcc_perf_config_t *cfg,
                                            ngcc_perf_result_t *out_result);

typedef struct {
    ngcc_correctness_dispatch_fn correctness_fn;
    ngcc_kat_dispatch_fn kat_fn;
    ngcc_performance_dispatch_fn performance_fn;
    ngcc_bytes_per_case_fn bytes_per_case_fn;
} test_dispatch_entry_t;

static int hash_correctness_dispatch(const ngcc_api_t *api, int digest_len_bits, size_t msg_len) {
    return ngcc_hash_correctness(api, digest_len_bits, msg_len);
}

static int sig_correctness_dispatch(const ngcc_api_t *api, int digest_len_bits, size_t msg_len) {
    (void) digest_len_bits;
    return ngcc_sig_correctness(api, msg_len);
}

static int kem_correctness_dispatch(const ngcc_api_t *api, int digest_len_bits, size_t msg_len) {
    (void) digest_len_bits;
    (void) msg_len;
    return ngcc_kem_correctness(api);
}

static int kex_correctness_dispatch(const ngcc_api_t *api, int digest_len_bits, size_t msg_len) {
    (void) digest_len_bits;
    (void) msg_len;
    return ngcc_kex_correctness(api);
}

static int hash_kat_dispatch(const ngcc_api_t *api,
                             int digest_len_bits,
                             const char *kat_path,
                             unsigned long long *out_total,
                             unsigned long long *out_passed,
                             unsigned long long *out_failed) {
    return ngcc_hash_correctness_kat_file(api, digest_len_bits, kat_path, out_total, out_passed, out_failed);
}

static int sig_verify_kat_dispatch(const ngcc_api_t *api,
                                   int digest_len_bits,
                                   const char *kat_path,
                                   unsigned long long *out_total,
                                   unsigned long long *out_passed,
                                   unsigned long long *out_failed) {
    (void) digest_len_bits;
    return ngcc_sig_verify_correctness_kat_file(api, kat_path, out_total, out_passed, out_failed);
}

static int kem_kat_dispatch(const ngcc_api_t *api,
                            int digest_len_bits,
                            const char *kat_path,
                            unsigned long long *out_total,
                            unsigned long long *out_passed,
                            unsigned long long *out_failed) {
    (void) digest_len_bits;
    return ngcc_kem_correctness_kat_file(api, kat_path, out_total, out_passed, out_failed);
}

static int kex_kat_dispatch(const ngcc_api_t *api,
                            int digest_len_bits,
                            const char *kat_path,
                            unsigned long long *out_total,
                            unsigned long long *out_passed,
                            unsigned long long *out_failed) {
    (void) digest_len_bits;
    return ngcc_kex_correctness_kat_file(api, kat_path, out_total, out_passed, out_failed);
}

static int hash_performance_dispatch(const ngcc_api_t *api,
                                     int digest_len_bits,
                                     size_t msg_len,
                                     const ngcc_perf_config_t *cfg,
                                     ngcc_perf_result_t *out_result) {
    return ngcc_hash_performance(api, digest_len_bits, msg_len, cfg, out_result);
}



static unsigned long long msg_len_bytes_per_case(const ngcc_api_t *api, size_t msg_len) {
    (void) api;
    return (unsigned long long) msg_len;
}

static unsigned long long kem_bytes_per_case(const ngcc_api_t *api, size_t msg_len) {
    (void) msg_len;
    if (api == NULL) {
        return 0;
    }
    return api->kem_get_ct_len_bytes();
}

static unsigned long long kex_bytes_per_case(const ngcc_api_t *api, size_t msg_len) {
    (void) msg_len;
    if (api == NULL) {
        return 0;
    }
    return api->kex_get_total_msg_len_bytes();
}

#define NGCC_TEST_DISPATCH_TABLE(X) \
    X(TEST_MASK_HASH, NGCC_TEST_HASH, "hash", hash_correctness_dispatch, hash_kat_dispatch, hash_performance_dispatch, msg_len_bytes_per_case) \
    X(TEST_MASK_SIG, NGCC_TEST_SIG, "sig", sig_correctness_dispatch, sig_verify_kat_dispatch, NULL, msg_len_bytes_per_case) \
    X(TEST_MASK_KEM, NGCC_TEST_KEM, "kem", kem_correctness_dispatch, kem_kat_dispatch, NULL, kem_bytes_per_case) \
    X(TEST_MASK_KEX, NGCC_TEST_KEX, "kex", kex_correctness_dispatch, kex_kat_dispatch, NULL, kex_bytes_per_case)

#define NGCC_TEST_ENTRY_ROW(mask, kind, name, correctness_fn, kat_fn, performance_fn, bytes_per_case_fn) \
    {mask, kind, name},
const test_entry_t k_tests[] = {
    NGCC_TEST_DISPATCH_TABLE(NGCC_TEST_ENTRY_ROW)
};
#undef NGCC_TEST_ENTRY_ROW

#define NGCC_TEST_DISPATCH_ROW(mask, kind, name, correctness_fn, kat_fn, performance_fn, bytes_per_case_fn) \
    {correctness_fn, kat_fn, performance_fn, bytes_per_case_fn},
static const test_dispatch_entry_t k_test_dispatch[] = {
    NGCC_TEST_DISPATCH_TABLE(NGCC_TEST_DISPATCH_ROW)
};
#undef NGCC_TEST_DISPATCH_ROW
#undef NGCC_TEST_DISPATCH_TABLE

_Static_assert((sizeof(k_tests) / sizeof(k_tests[0])) == NGCC_NUM_TESTS, "k_tests size mismatch");
_Static_assert((sizeof(k_test_dispatch) / sizeof(k_test_dispatch[0])) == NGCC_NUM_TESTS, "k_test_dispatch size mismatch");

typedef struct {
    int algorithm_rc;
    uint64_t static_memory_bytes;
    uint64_t peak_memory_bytes;
} ngcc_memory_probe_result_t;

/* ── Test dispatch functions ───────────────────────────────────── */

static int run_correctness_for_test(const ngcc_api_t *api,
                                    size_t test_index,
                                    const cli_options_t *opts,
                                    test_report_t *report) {
    const test_entry_t *test = &k_tests[test_index];
    const test_dispatch_entry_t *dispatch = &k_test_dispatch[test_index];
    int rc;

    if (opts->kat_path != NULL) {
        unsigned long long total = 0;
        unsigned long long passed = 0;
        unsigned long long failed = 0;
        int kat_rc = dispatch->kat_fn(api,
                                      opts->digest_len_bits,
                                      opts->kat_path,
                                      &total,
                                      &passed,
                                      &failed);

        if (kat_rc == 0 || kat_rc < 0) {
            if (kat_rc != 0) {
                ngcc_log_error("[correctness][%s][kat] failed: total=%llu passed=%llu failed=%llu path=%s",
                               test->name,
                               total,
                               passed,
                               failed,
                               opts->kat_path);
            }
            if (report != NULL) {
                report->correctness_status = (kat_rc == 0) ? STATUS_PASS : STATUS_FAIL;
                report->kat_used = 1;
                report->kat_total = total;
                report->kat_passed = passed;
                report->kat_failed = failed;
            }
            return kat_rc;
        }
    }

    rc = dispatch->correctness_fn(api, opts->digest_len_bits, k_msg_lens[0]);
    if (rc != 0) {
        ngcc_log_error("[correctness][%s] failed: digest_len_bits=%d msg_len=%zu rc=%d",
                       test->name,
                       opts->digest_len_bits,
                       k_msg_lens[0],
                       rc);
    }
    if (report != NULL) {
        report->correctness_status = (rc == 0) ? STATUS_PASS : STATUS_FAIL;
    }
    return rc;
}

static int run_sig_sub_performance(const ngcc_api_t *api,
                                   const ngcc_perf_config_t *cfg,
                                   test_report_t *report) {
    ngcc_perf_result_t result;
    int any_fail = 0;
    int rc;
    int idx = 0;
    static const size_t sig_msg_len = 64;

    /* keygen */
    rc = ngcc_sig_keygen_performance(api, cfg, &result);
    if (rc != 0) {
        ngcc_log_error("[performance][sig][keygen] failed: iterations=%llu rc=%d",
                       cfg != NULL ? cfg->iterations : 0ULL,
                       rc);
        any_fail = 1;
    } else {
        if (report != NULL) {
            report->performance[idx] = result;
            report->performance_labels[idx] = "\u5bc6\u94a5\u751f\u6210";
            report->performance_labels_en[idx] = "keygen";
            idx++;
        }
    }

    /* sign */
    rc = ngcc_sig_sign_performance(api, sig_msg_len, cfg, &result);
    if (rc != 0) {
        ngcc_log_error("[performance][sig][sign] failed: msg_len=%zu iterations=%llu rc=%d",
                       sig_msg_len,
                       cfg != NULL ? cfg->iterations : 0ULL,
                       rc);
        any_fail = 1;
    } else {
        if (report != NULL) {
            report->performance[idx] = result;
            report->performance_labels[idx] = "\u7b7e\u540d";
            report->performance_labels_en[idx] = "sign";
            idx++;
        }
    }

    /* verify */
    rc = ngcc_sig_verify_performance(api, sig_msg_len, cfg, &result);
    if (rc != 0) {
        ngcc_log_error("[performance][sig][verify] failed: msg_len=%zu iterations=%llu rc=%d",
                       sig_msg_len,
                       cfg != NULL ? cfg->iterations : 0ULL,
                       rc);
        any_fail = 1;
    } else {
        if (report != NULL) {
            report->performance[idx] = result;
            report->performance_labels[idx] = "\u9a8c\u7b7e";
            report->performance_labels_en[idx] = "verify";
            idx++;
        }
    }

    if (report != NULL) {
        report->performance_count = idx;
    }
    return any_fail ? -1 : 0;
}

static int run_kem_sub_performance(const ngcc_api_t *api,
                                   const ngcc_perf_config_t *cfg,
                                   test_report_t *report) {
    static const char *const labels[] = {"\u5bc6\u94a5\u751f\u6210", "\u5c01\u88c5", "\u89e3\u5c01\u88c5"};
    static const char *const labels_en[] = {"keygen", "encap", "decap"};
    ngcc_perf_result_t result;
    int any_fail = 0;
    int idx = 0;
    size_t si;

    typedef int (*kem_sub_perf_fn)(const ngcc_api_t *, const ngcc_perf_config_t *, ngcc_perf_result_t *);
    kem_sub_perf_fn sub_fns[] = {
        ngcc_kem_keygen_performance,
        ngcc_kem_encap_performance,
        ngcc_kem_decap_performance
    };

    for (si = 0; si < 3; ++si) {
        int rc = sub_fns[si](api, cfg, &result);
        if (rc != 0) {
            ngcc_log_error("[performance][kem][%s] failed: iterations=%llu rc=%d",
                           labels_en[si],
                           cfg != NULL ? cfg->iterations : 0ULL,
                           rc);
            any_fail = 1;
            continue;
        }
        if (report != NULL) {
            report->performance[idx] = result;
            report->performance_labels[idx] = labels[si];
            report->performance_labels_en[idx] = labels_en[si];
            idx++;
        }
    }
    if (report != NULL) {
        report->performance_count = idx;
    }
    return any_fail ? -1 : 0;
}

static int run_kex_sub_performance(const ngcc_api_t *api,
                                   const ngcc_perf_config_t *cfg,
                                   test_report_t *report) {
    ngcc_perf_result_t result_a, result_b;
    int rc = ngcc_kex_derive_ss_performance(api, cfg, &result_a, &result_b);
    if (rc != 0) {
        ngcc_log_error("[performance][kex][derive_ss] failed: iterations=%llu rc=%d",
                       cfg != NULL ? cfg->iterations : 0ULL,
                       rc);
        return -1;
    }
    if (report != NULL) {
        report->performance[0] = result_a;
        report->performance_labels[0] = "\u5bc6\u94a5\u534f\u5546A";
        report->performance_labels_en[0] = "kex_derive_a";
        report->performance[1] = result_b;
        report->performance_labels[1] = "\u5bc6\u94a5\u534f\u5546B";
        report->performance_labels_en[1] = "kex_derive_b";
        report->performance_count = 2;
    }
    return 0;
}

static int run_performance_for_test(const ngcc_api_t *api,
                                    size_t test_index,
                                    const cli_options_t *opts,
                                    test_report_t *report) {
    const test_entry_t *test = &k_tests[test_index];
    const test_dispatch_entry_t *dispatch = &k_test_dispatch[test_index];
    ngcc_perf_config_t cfg;
    ngcc_perf_result_t result;
    int rc;
    size_t mi;
    int any_fail = 0;
    int digest_bits = 0;


    if (test->kind == NGCC_TEST_HASH) {
        digest_bits = opts->digest_len_bits;
    }

    cfg.iterations = NGCC_PERF_ITERATIONS;
    cfg.bytes_per_op = 0;

    /* For SIG, KEM, KEX: run sub-operation performance (no aggregate) */
    if (test->kind == NGCC_TEST_SIG) {
        rc = run_sig_sub_performance(api, &cfg, report);
        if (report != NULL) {
            report->performance_status = (rc != 0) ? STATUS_FAIL : STATUS_PASS;
        }
        return rc;
    }
    if (test->kind == NGCC_TEST_KEM) {
        rc = run_kem_sub_performance(api, &cfg, report);
        if (report != NULL) {
            report->performance_status = (rc != 0) ? STATUS_FAIL : STATUS_PASS;
        }
        return rc;
    }
    if (test->kind == NGCC_TEST_KEX) {
        rc = run_kex_sub_performance(api, &cfg, report);
        if (report != NULL) {
            report->performance_status = (rc != 0) ? STATUS_FAIL : STATUS_PASS;
        }
        return rc;
    }

    /* HASH only: run the aggregate performance function */
    for (mi = 0; mi < NGCC_NUM_MSG_LENS; ++mi) {
        size_t msg_len = k_msg_lens[mi];

        rc = dispatch->performance_fn(api, digest_bits, msg_len, &cfg, &result);

        if (rc != 0) {
            ngcc_log_error("[performance][%s] failed: msg_len=%zu digest_len_bits=%d iterations=%llu rc=%d",
                           test->name,
                           msg_len,
                           digest_bits,
                           cfg.iterations,
                           rc);
            any_fail = 1;
            continue;
        }

        if (report != NULL) {
            char *label = (char *) malloc(16);
            if (label != NULL) {
                snprintf(label, 16, "%zuB", msg_len);
            }
            report->performance[mi] = result;
            report->performance_labels[mi] = label;
            report->performance_labels_en[mi] = label;
            report->performance_count = (int) mi + 1;
        }
    }

    if (report != NULL) {
        report->performance_status = any_fail ? STATUS_FAIL : STATUS_PASS;
    }
    return any_fail ? -1 : 0;
}

static int run_stability_for_test(const ngcc_api_t *api,
                                  size_t test_index,
                                  const cli_options_t *opts,
                                  test_report_t *report) {
    const test_entry_t *test = &k_tests[test_index];
    const test_dispatch_entry_t *dispatch = &k_test_dispatch[test_index];
    ngcc_stability_result_t result;
    int rc;
    int unstable_fail = 0;

    memset(&result, 0, sizeof(result));
    rc = ngcc_run_stability(api,
                            dispatch->correctness_fn,
                            (test->kind == NGCC_TEST_HASH) ? dispatch->bytes_per_case_fn : NULL,
                            (test->kind == NGCC_TEST_HASH) ? opts->digest_len_bits : 0,
                            NGCC_STABILITY_MSG_LEN,
                            1,
                            opts->stability_sample_ms,
                            opts->duration_hours,
                            opts->stability_max_cases,
                            &opts->stability_thresholds,
                            &result);

    if (rc == 0) {
        if (!result.interrupted && strcmp(result.status, "UNSTABLE") == 0) {
            unstable_fail = 1;
        }
        if (report != NULL) {
            report->stability = result;
            if (result.interrupted) {
                report->stability_status = STATUS_STOPPED;
            } else if (unstable_fail) {
                report->stability_status = STATUS_FAIL;
            } else {
                report->stability_status = STATUS_PASS;
            }
        }
    } else {
        if (report != NULL) {
            report->stability = result;
            report->stability_status = STATUS_FAIL;
        }
        ngcc_log_error("[stability][%s] failed: rc=%d cases_run=%llu errors=%llu status=%s reasons=%s",
                       test->name,
                       rc,
                       result.cases_run,
                       result.error_count,
                       result.status,
                       result.failure_reasons);
    }

    if (rc == 0 && unstable_fail) {
        ngcc_log_error("[stability][%s] unstable: cases_run=%llu samples=%llu errors=%llu error_rate=%.6f%% reasons=%s",
                       test->name,
                       result.cases_run,
                       result.sample_count,
                       result.error_count,
                       result.error_rate_percent,
                       result.failure_reasons);
        return -1;
    }
    return rc;
}

#ifdef __linux__
#define NGCC_MEMORY_HELPER_ARG "--internal-memory-helper"
#define NGCC_MEMORY_LIB_FD 198
#define NGCC_MEMORY_RESULT_FD 199

static uint64_t memory_delta_bytes(uint64_t end, uint64_t start) {
    return (end > start) ? (end - start) : 0U;
}

static int write_probe_result(int fd, const ngcc_memory_probe_result_t *result) {
    const unsigned char *p;
    size_t left;

    if (result == NULL) {
        return -1;
    }

    p = (const unsigned char *) result;
    left = sizeof(*result);
    while (left > 0U) {
        ssize_t n = write(fd, p, left);

        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        if (n == 0) {
            return -1;
        }
        p += (size_t) n;
        left -= (size_t) n;
    }

    return 0;
}

static int read_probe_result(int fd, ngcc_memory_probe_result_t *result) {
    unsigned char *p;
    size_t left;

    if (result == NULL) {
        return -1;
    }

    p = (unsigned char *) result;
    left = sizeof(*result);
    while (left > 0U) {
        ssize_t n = read(fd, p, left);

        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        if (n == 0) {
            break;
        }
        p += (size_t) n;
        left -= (size_t) n;
    }

    return (left == 0U) ? 0 : -1;
}

static int run_memory_helper_process(int lib_fd,
                                     int result_fd,
                                     size_t test_index,
                                     int digest_len_bits) {
    ngcc_memory_probe_result_t result;
    ngcc_library_t lib;
    uint64_t base_vmsize;
    uint64_t loaded_vmsize = 0;
    uint64_t peak_vmsize = 0;
    int rc = -1;

    memset(&result, 0, sizeof(result));
    memset(&lib, 0, sizeof(lib));
    if (test_index >= sizeof(k_tests) / sizeof(k_tests[0])) {
        return 127;
    }

    base_vmsize = ngcc_mem_current_vmsize_bytes();
    if (base_vmsize == 0U) {
        ngcc_log_error("[memory][%s] failed to read base VmSize before dlopen",
                       k_tests[test_index].name);
    } else if (ngcc_load_library_fd(lib_fd, k_tests[test_index].mask, &lib) == 0) {
        int digest_bits = (k_tests[test_index].kind == NGCC_TEST_HASH) ? digest_len_bits : 0;

        loaded_vmsize = ngcc_mem_current_vmsize_bytes();
        rc = k_test_dispatch[test_index].correctness_fn(&lib.api, digest_bits, k_msg_lens[0]);
        peak_vmsize = ngcc_mem_vm_peak_bytes();
        if (peak_vmsize == 0U) {
            peak_vmsize = ngcc_mem_current_vmsize_bytes();
        }
        if (peak_vmsize < loaded_vmsize) {
            peak_vmsize = loaded_vmsize;
        }
        result.static_memory_bytes = memory_delta_bytes(loaded_vmsize, base_vmsize);
        result.peak_memory_bytes = memory_delta_bytes(peak_vmsize, base_vmsize);
        if (result.peak_memory_bytes < result.static_memory_bytes) {
            result.peak_memory_bytes = result.static_memory_bytes;
        }
        ngcc_unload_library(&lib);
    } else {
        ngcc_log_error("[memory][%s] failed to load library from fixed fd",
                       k_tests[test_index].name);
    }

    result.algorithm_rc = rc;
    return write_probe_result(result_fd, &result) == 0 ? 0 : 127;
}

static int parse_internal_int(const char *value, int *out) {
    char *end = NULL;
    long parsed;

    errno = 0;
    parsed = strtol(value, &end, 10);
    if (errno != 0 || end == value || *end != '\0' || parsed < 0 || parsed > 2147483647L) {
        return -1;
    }
    *out = (int) parsed;
    return 0;
}

static int maybe_run_memory_helper(int argc, char **argv, int *out_exit_code) {
    int lib_fd;
    int result_fd;
    int test_index;
    int digest_len_bits;

    if (argc < 2 || strcmp(argv[1], NGCC_MEMORY_HELPER_ARG) != 0) {
        return 0;
    }
    if (argc != 6 || parse_internal_int(argv[2], &lib_fd) != 0 ||
        parse_internal_int(argv[3], &result_fd) != 0 ||
        parse_internal_int(argv[4], &test_index) != 0 ||
        parse_internal_int(argv[5], &digest_len_bits) != 0) {
        *out_exit_code = 127;
        return 1;
    }
    *out_exit_code = run_memory_helper_process(lib_fd, result_fd,
                                                (size_t) test_index,
                                                digest_len_bits);
    return 1;
}
#endif

static int run_memory_probe_child(const cli_options_t *opts,
                                  int lib_fd,
                                  size_t test_index,
                                  ngcc_memory_probe_result_t *out_result) {
#ifdef __linux__
    int pipefd[2];
    pid_t pid;
    int status = 0;

    if (opts == NULL || out_result == NULL) {
        return -1;
    }

    if (pipe(pipefd) != 0) {
        return -1;
    }

    pid = fork();
    if (pid < 0) {
        close(pipefd[0]);
        close(pipefd[1]);
        return -1;
    }

    if (pid == 0) {
        char lib_fd_arg[32];
        char result_fd_arg[32];
        char test_index_arg[32];
        char digest_arg[32];
        char *helper_argv[7];

        close(pipefd[0]);
        if (dup2(lib_fd, NGCC_MEMORY_LIB_FD) < 0 ||
            dup2(pipefd[1], NGCC_MEMORY_RESULT_FD) < 0) {
            _exit(127);
        }
        if (fcntl(NGCC_MEMORY_LIB_FD, F_SETFD, 0) < 0 ||
            fcntl(NGCC_MEMORY_RESULT_FD, F_SETFD, 0) < 0) {
            _exit(127);
        }
        if (pipefd[1] != NGCC_MEMORY_RESULT_FD) {
            close(pipefd[1]);
        }
        snprintf(lib_fd_arg, sizeof(lib_fd_arg), "%d", NGCC_MEMORY_LIB_FD);
        snprintf(result_fd_arg, sizeof(result_fd_arg), "%d", NGCC_MEMORY_RESULT_FD);
        snprintf(test_index_arg, sizeof(test_index_arg), "%zu", test_index);
        snprintf(digest_arg, sizeof(digest_arg), "%d", opts->digest_len_bits);
        helper_argv[0] = (char *) "ngcc_bench";
        helper_argv[1] = NGCC_MEMORY_HELPER_ARG;
        helper_argv[2] = lib_fd_arg;
        helper_argv[3] = result_fd_arg;
        helper_argv[4] = test_index_arg;
        helper_argv[5] = digest_arg;
        helper_argv[6] = NULL;
        execv("/proc/self/exe", helper_argv);
        _exit(127);
    }

    close(pipefd[1]);
    if (read_probe_result(pipefd[0], out_result) != 0) {
        close(pipefd[0]);
        waitpid(pid, &status, 0);
        return -1;
    }
    close(pipefd[0]);

    if (waitpid(pid, &status, 0) < 0) {
        return -1;
    }
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        return -1;
    }
    return 0;
#else
    (void) opts;
    (void) lib_fd;
    (void) test_index;
    (void) out_result;
    return -1;
#endif
}

static int run_memory_mode(const cli_options_t *opts,
                           run_report_t *report,
                           int lib_fd) {
    size_t i;
    int failed = 0;

    for (i = 0; i < sizeof(k_tests) / sizeof(k_tests[0]); ++i) {
        ngcc_memory_probe_result_t probe;
        int probe_rc;

        if ((opts->test_mask & k_tests[i].mask) == 0) {
            continue;
        }

        memset(&probe, 0, sizeof(probe));
        probe_rc = run_memory_probe_child(opts, lib_fd, i, &probe);
        if (probe_rc != 0) {
            report->tests[i].memory_status = STATUS_FAIL;
            ngcc_log_error("[memory][%s] helper probe failed",
                           k_tests[i].name);
            failed = 1;
            continue;
        }

        report->tests[i].static_memory_bytes = probe.static_memory_bytes;
        report->tests[i].peak_memory_bytes = probe.peak_memory_bytes;
        report->tests[i].memory_status = (probe.algorithm_rc == 0) ? STATUS_PASS : STATUS_FAIL;

        if (probe.algorithm_rc != 0) {
            ngcc_log_error("[memory][%s] correctness probe failed: digest_len_bits=%d msg_len=%zu rc=%d static_memory=%llu peak_memory=%llu",
                           k_tests[i].name,
                           (k_tests[i].kind == NGCC_TEST_HASH) ? opts->digest_len_bits : 0,
                           k_msg_lens[0],
                           probe.algorithm_rc,
                           (unsigned long long) probe.static_memory_bytes,
                           (unsigned long long) probe.peak_memory_bytes);
            failed = 1;
        }
    }

    return failed ? -1 : 0;
}

/* ── Entry point ───────────────────────────────────────────────── */

int main(int argc, char **argv) {
    cli_options_t opts;
    ngcc_library_t lib;
    run_report_t report;
    size_t i;
    int failed = 0;
    int lib_fd = -1;
#ifdef __linux__
    int helper_exit_code;
    if (maybe_run_memory_helper(argc, argv, &helper_exit_code)) {
        return helper_exit_code;
    }
#endif
    char interactive_lib[NGCC_PATH_BUF_SIZE];
    char interactive_kat[NGCC_PATH_BUF_SIZE];
    char interactive_json[NGCC_PATH_BUF_SIZE];

    init_default_options(&opts);
    memset(&report, 0, sizeof(report));
    memset(interactive_lib, 0, sizeof(interactive_lib));
    memset(interactive_kat, 0, sizeof(interactive_kat));
    memset(interactive_json, 0, sizeof(interactive_json));

    for (i = 0; i < sizeof(k_tests) / sizeof(k_tests[0]); ++i) {
        report.tests[i].name = k_tests[i].name;
        report.tests[i].is_hash = (k_tests[i].kind == NGCC_TEST_HASH);
        report.tests[i].correctness_status = STATUS_SKIPPED;
        report.tests[i].performance_status = STATUS_SKIPPED;
        report.tests[i].stability_status = STATUS_SKIPPED;
        report.tests[i].memory_status = STATUS_SKIPPED;
    }

    if (argc == 1) {
        if (run_interactive_setup(&opts,
                                  interactive_lib,
                                  sizeof(interactive_lib),
                                  interactive_kat,
                                  sizeof(interactive_kat),
                                  interactive_json,
                                  sizeof(interactive_json)) != 0) {
            return 1;
        }
    } else {
        int parse_rc = parse_cli_options(argc, argv, &opts);
        if (parse_rc > 0) {
            return 0;
        }
        if (parse_rc < 0) {
            return 1;
        }
    }

    if (validate_options(&opts) != 0) {
        if (argc != 1) {
            print_usage(argv[0]);
        }
        return 1;
    }

    lib_fd = ngcc_open_library_file(opts.lib_path);
    if (lib_fd < 0) {
        return 1;
    }
    if (ngcc_load_library_fd(lib_fd, opts.test_mask, &lib) != 0) {
        ngcc_log_error("[main] failed to load library: %s", opts.lib_path);
        close(lib_fd);
        return 1;
    }

    for (i = 0; i < sizeof(k_tests) / sizeof(k_tests[0]); ++i) {
        report.tests[i].selected = ((opts.test_mask & k_tests[i].mask) != 0);
    }

    if (opts.mode_mask & MODE_MASK_CORRECTNESS) {
        int correctness_failed = 0;
        printf("[correctness] BEGIN\n");
        fflush(stdout);
        for (i = 0; i < sizeof(k_tests) / sizeof(k_tests[0]); ++i) {
            if ((opts.test_mask & k_tests[i].mask) == 0) {
                continue;
            }
            if (run_correctness_for_test(&lib.api, i, &opts, &report.tests[i]) != 0) {
                correctness_failed = 1;
            }
        }
        printf("[correctness] END status=%s\n", correctness_failed ? "FAIL" : "PASS");
        fflush(stdout);
        if (correctness_failed) {
            ngcc_log_error("[correctness] stage failed");
            failed = 1;
        }
    }

    if (opts.mode_mask & MODE_MASK_PERFORMANCE) {
        int performance_failed = 0;
        printf("[performance] BEGIN\n");
        fflush(stdout);
        for (i = 0; i < sizeof(k_tests) / sizeof(k_tests[0]); ++i) {
            if ((opts.test_mask & k_tests[i].mask) == 0) {
                continue;
            }
            if (run_performance_for_test(&lib.api, i, &opts, &report.tests[i]) != 0) {
                performance_failed = 1;
            }
        }
        printf("[performance] END status=%s\n", performance_failed ? "FAIL" : "PASS");
        fflush(stdout);
        if (performance_failed) {
            ngcc_log_error("[performance] stage failed");
            failed = 1;
        }
    }

    if (opts.mode_mask & MODE_MASK_MEMORY) {
        int memory_failed;
        printf("[memory] BEGIN\n");
        fflush(stdout);
        memory_failed = run_memory_mode(&opts, &report, lib_fd);
        printf("[memory] END status=%s\n", memory_failed != 0 ? "FAIL" : "PASS");
        fflush(stdout);
        if (memory_failed != 0) {
            ngcc_log_error("[memory] stage failed");
            failed = 1;
        }
    }

    if (opts.mode_mask & MODE_MASK_STABILITY) {
        int stability_failed = 0;
        printf("[stability] BEGIN\n");
        fflush(stdout);
        for (i = 0; i < sizeof(k_tests) / sizeof(k_tests[0]); ++i) {
            if ((opts.test_mask & k_tests[i].mask) == 0) {
                continue;
            }
            if (run_stability_for_test(&lib.api, i, &opts, &report.tests[i]) != 0) {
                stability_failed = 1;
            }
        }
        printf("[stability] END status=%s\n", stability_failed ? "FAIL" : "PASS");
        fflush(stdout);
        if (stability_failed) {
            ngcc_log_error("[stability] stage failed");
            failed = 1;
        }
    }

    if (opts.json_out_path != NULL) {
        if (write_json_reports(&opts, &report, failed) != 0) {
            ngcc_log_error("[report] failed to write json reports: path=%s", opts.json_out_path);
            failed = 1;
        }
    }

    ngcc_unload_library(&lib);
    close(lib_fd);
    ngcc_log_close();
    return failed ? 1 : 0;
}
