#include <getopt.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cli_parser.h"
#include "ngcc_log.h"

#ifndef NGCC_VERSION
#define NGCC_VERSION "unknown"
#endif

/* ── Helpers ───────────────────────────────────────────────────── */

void print_version(void) {
    printf("ngcc_bench %s\n", NGCC_VERSION);
}

void print_usage(const char *prog) {
    printf("ngcc_bench %s\n\n", NGCC_VERSION);
    printf("Usage:\n");
    printf("  %s --lib /path/to/lib.so --test hash|sig|kem|kex|all --mode correctness|performance|memory|stability|all\n", prog);
    printf("     [--digest-len-bits BITS]\n");
    printf("     [--duration-hours H] [--stability-max-cases N]\n");
    printf("     [--json-out PATH] [--kat DIR]\n");
    printf("\n");
    printf("  %s\n", prog);
    printf("     Launch interactive menu mode.\n");
}

void init_default_options(cli_options_t *opts) {
    memset(opts, 0, sizeof(*opts));
    opts->test_mask = TEST_MASK_ALL;
    opts->mode_mask = MODE_MASK_ALL;
    opts->duration_hours = 6.0;
    opts->stability_max_cases = 3000;
    opts->stability_sample_ms = 1.0;
    ngcc_stability_thresholds_set_defaults(&opts->stability_thresholds);
}

int parse_unsigned_ll(const char *s, unsigned long long *out) {
    char *end = NULL;
    unsigned long long v;

    if (s == NULL || out == NULL || *s == '\0') {
        return -1;
    }

    v = strtoull(s, &end, 10);
    if (end == NULL || *end != '\0') {
        return -1;
    }

    *out = v;
    return 0;
}

int parse_int_value(const char *s, int *out) {
    char *end = NULL;
    long v;

    if (s == NULL || out == NULL || *s == '\0') {
        return -1;
    }

    errno = 0;
    v = strtol(s, &end, 10);
    if (errno == ERANGE || end == NULL || *end != '\0' || v < INT_MIN || v > INT_MAX) {
        return -1;
    }

    *out = (int) v;
    return 0;
}

int parse_double_value(const char *s, double *out) {
    char *end = NULL;
    double v;

    if (s == NULL || out == NULL || *s == '\0') {
        return -1;
    }

    v = strtod(s, &end);
    if (end == NULL || *end != '\0') {
        return -1;
    }

    *out = v;
    return 0;
}

static int parse_nonnegative_double_option(const char *opt_name, const char *opt_value, double *out) {
    if (parse_double_value(opt_value, out) != 0 || !isfinite(*out) || *out < 0.0) {
        ngcc_log_error("invalid --%s value: %s", opt_name, opt_value);
        return -1;
    }
    return 0;
}

static int is_valid_digest_len_bits(int digest_len_bits) {
    unsigned long long digest_len;

    if (digest_len_bits <= 0) {
        return 0;
    }

    digest_len = ((unsigned long long) digest_len_bits + 7ULL) / 8ULL;
    return ngcc_is_valid_len(digest_len);
}

int parse_test_mask(const char *s, unsigned int *out_mask) {
    if (strcmp(s, "hash") == 0) {
        *out_mask = TEST_MASK_HASH;
        return 0;
    }
    if (strcmp(s, "sig") == 0) {
        *out_mask = TEST_MASK_SIG;
        return 0;
    }
    if (strcmp(s, "kem") == 0) {
        *out_mask = TEST_MASK_KEM;
        return 0;
    }
    if (strcmp(s, "kex") == 0) {
        *out_mask = TEST_MASK_KEX;
        return 0;
    }
    if (strcmp(s, "all") == 0) {
        *out_mask = TEST_MASK_ALL;
        return 0;
    }
    return -1;
}

int parse_mode_mask(const char *s, unsigned int *out_mask) {
    if (strcmp(s, "correctness") == 0) {
        *out_mask = MODE_MASK_CORRECTNESS;
        return 0;
    }
    if (strcmp(s, "performance") == 0) {
        *out_mask = MODE_MASK_PERFORMANCE;
        return 0;
    }
    if (strcmp(s, "memory") == 0) {
        *out_mask = MODE_MASK_MEMORY;
        return 0;
    }
    if (strcmp(s, "stability") == 0) {
        *out_mask = MODE_MASK_STABILITY;
        return 0;
    }
    if (strcmp(s, "all") == 0) {
        *out_mask = MODE_MASK_ALL;
        return 0;
    }
    return -1;
}



/* ── Main option parser ────────────────────────────────────────── */

int parse_cli_options(int argc, char **argv, cli_options_t *opts) {
    int ch;
    int option_index = 0;
    static const struct option long_options[] = {
        {"lib", required_argument, NULL, 'l'},
        {"test", required_argument, NULL, 't'},
        {"mode", required_argument, NULL, 'm'},
        {"digest-len-bits", required_argument, NULL, 'b'},
        {"duration-hours", required_argument, NULL, 'd'},
        {"stability-max-cases", required_argument, NULL, 's'},
        {"json-out", required_argument, NULL, 'j'},
        {"kat", required_argument, NULL, 'k'},
        {"help", no_argument, NULL, 'h'},
        {"version", no_argument, NULL, 'v'},
        {0, 0, 0, 0}
    };

    while ((ch = getopt_long(argc, argv, "", long_options, &option_index)) != -1) {
        switch (ch) {
            case 'l':
                opts->lib_path = optarg;
                break;
            case 't':
                if (parse_test_mask(optarg, &opts->test_mask) != 0) {
                    ngcc_log_error("invalid --test value: %s", optarg);
                    return -1;
                }
                break;
            case 'm':
                if (parse_mode_mask(optarg, &opts->mode_mask) != 0) {
                    ngcc_log_error("invalid --mode value: %s", optarg);
                    return -1;
                }
                break;
            case 'b':
                if (parse_int_value(optarg, &opts->digest_len_bits) != 0 ||
                    !is_valid_digest_len_bits(opts->digest_len_bits)) {
                    ngcc_log_error("invalid --digest-len-bits value: %s (max %llu bits)",
                                   optarg,
                                   NGCC_MAX_BUFFER_LEN * 8ULL);
                    return -1;
                }
                break;
            case 'd':
                if (parse_double_value(optarg, &opts->duration_hours) != 0 || opts->duration_hours <= 0.0) {
                    ngcc_log_error("invalid --duration-hours value: %s", optarg);
                    return -1;
                }
                break;
            case 's':
                if (parse_unsigned_ll(optarg, &opts->stability_max_cases) != 0 || opts->stability_max_cases == 0) {
                    ngcc_log_error("invalid --stability-max-cases value: %s", optarg);
                    return -1;
                }
                break;
            case 'j':
                opts->json_out_path = optarg;
                break;
            case 'k':
                opts->kat_path = optarg;
                break;
            case 'h':
                print_usage(argv[0]);
                return 1;
            case 'v':
                print_version();
                return 1;
            default:
                print_usage(argv[0]);
                return -1;
        }
    }

    return 0;
}

/* ── Validation ────────────────────────────────────────────────── */

int validate_options(const cli_options_t *opts) {
    int correctness_selected;

    if (opts == NULL) {
        return -1;
    }

    if (opts->lib_path == NULL) {
        ngcc_log_error("--lib is required");
        return -1;
    }

    if (opts->test_mask & TEST_MASK_HASH) {
        if (opts->digest_len_bits <= 0) {
            ngcc_log_error("--digest-len-bits is required when hash test is selected");
            return -1;
        }
        if (!is_valid_digest_len_bits(opts->digest_len_bits)) {
            ngcc_log_error("--digest-len-bits exceeds maximum supported digest size (%llu bits)",
                           NGCC_MAX_BUFFER_LEN * 8ULL);
            return -1;
        }
    }

    correctness_selected = (opts->mode_mask & MODE_MASK_CORRECTNESS) != 0;
    if (opts->kat_path != NULL && !correctness_selected) {
        ngcc_log_error("--kat requires correctness mode");
        return -1;
    }

    return 0;
}
