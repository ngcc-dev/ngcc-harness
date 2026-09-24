#ifndef BENCH_CORE_H
#define BENCH_CORE_H

#include <stdint.h>
#include <stddef.h>

/* Maximum buffer size for any single allocation (64 MiB). */
#define NGCC_MAX_BUFFER_LEN (64ULL * 1024ULL * 1024ULL)

typedef int (*ngcc_operation_fn)(void *ctx);

typedef struct {
    unsigned long long iterations;
    unsigned long long bytes_per_op;
} ngcc_perf_config_t;

typedef struct {
    unsigned long long iterations;
    unsigned long long warmup_iterations;
    double elapsed_ms;
    double ops_per_sec;
    double bytes_per_sec;
    double mb_per_sec;
    double bytes_per_op;
    int cycles_available;
    double cycles_per_op;
    double time_ms_mean;
    double time_ms_median;
    double time_ms_stddev;
    double time_ms_cv_percent;
    double cycles_min;
    double cycles_median;
    double cycles_max;
    double cycles_stddev;
    double cycles_cv_percent;
    double cycles_per_byte;
} ngcc_perf_result_t;

int ngcc_is_valid_len(unsigned long long n);
int ngcc_fill_random(unsigned char *buf, size_t len);
int ngcc_run_performance_op(const ngcc_perf_config_t *cfg,
                            ngcc_operation_fn op,
                            void *op_ctx,
                            ngcc_perf_result_t *out_result);

#endif
