#ifndef TEST_UTIL_H
#define TEST_UTIL_H

#include <stdint.h>
#include <registry.h>
#include <stdio.h>

#define TEST_LOG_FILE       "logs/self_assessment.log"
#define DEFAULT_PERF_TIME   1000
#define DEFAULT_OUTPUT_DIR  "reports"

typedef struct _bench_opt {
    uint32_t times;
    const char *lib_path;
    const char *output_dir;
} bench_opt;

typedef struct _cmd_opt {
    bench_opt bench_opt;
    uint8_t candidate_algs[MAX_ALG];
} cmd_opt;

void parse_options(int argc, char **argv, cmd_opt *opts);

void print_banner(void);

char *get_timestamp(const char *date_format);

int run_sig_bench(const ALGORITHM *alg, const bench_opt *opt);

#endif
