#ifndef TEST_UTIL_H
#define TEST_UTIL_H

#include <stdint.h>
#include <stdio.h>
#include "registry.h"

#define TEST_LOG_FILE       "ngcc_test.log"
#define DEFAULT_PERF_TIME   10000
#define MIN_PERF_TIME       100

typedef struct _bench_opt {
    uint32_t times;
    uint32_t len;
} bench_opt;

typedef struct _cmd_opt {
    bench_opt bench_opt;
    uint8_t candidate_algs[MAX_ALG];
} cmd_opt;

void parse_options(int argc, char **argv, cmd_opt *opts);

void print_banner(void);

char *get_timestamp(const char *date_format);

int run_hash_bench(const ALGORITHM *, const bench_opt *opt);

#endif