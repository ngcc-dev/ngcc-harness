#include "bench_core.h"

#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(__linux__)
#include <sys/random.h>
#endif
#include <time.h>
#include <unistd.h>

#include "cycle_counter.h"
#include "ngcc_log.h"
#include "stats_util.h"

static int g_cycles_warning_printed = 0;

static int compare_double(const void *a, const void *b) {
    double da = *(const double *) a;
    double db = *(const double *) b;
    if (da < db) {
        return -1;
    }
    if (da > db) {
        return 1;
    }
    return 0;
}

static double compute_median(double *values, size_t count) {
    if (values == NULL || count == 0) {
        return 0.0;
    }

    qsort(values, count, sizeof(values[0]), compare_double);
    if ((count % 2U) != 0U) {
        return values[count / 2U];
    }
    return (values[(count / 2U) - 1U] + values[count / 2U]) * 0.5;
}

int ngcc_is_valid_len(unsigned long long n) {
    return n > 0 && n <= NGCC_MAX_BUFFER_LEN;
}

int ngcc_fill_random(unsigned char *buf, size_t len) {
    size_t offset = 0;

    if (buf == NULL) {
        return -1;
    }
    if (len == 0U) {
        return 0;
    }

#if defined(__APPLE__)
    arc4random_buf(buf, len);
    return 0;
#elif defined(__linux__)
    while (offset < len) {
        ssize_t got = getrandom(buf + offset, len - offset, 0);
        if (got > 0) {
            offset += (size_t) got;
            continue;
        }
        if (got < 0 && errno == EINTR) {
            continue;
        }
        break;
    }

    if (offset == len) {
        return 0;
    }
#endif

    {
        int fd = open("/dev/urandom", O_RDONLY);
        if (fd < 0) {
            return -1;
        }

        while (offset < len) {
            ssize_t got = read(fd, buf + offset, len - offset);
            if (got > 0) {
                offset += (size_t) got;
                continue;
            }
            if (got < 0 && errno == EINTR) {
                continue;
            }
            close(fd);
            return -1;
        }

        close(fd);
    }

    return 0;
}

int ngcc_run_performance_op(const ngcc_perf_config_t *cfg,
                            ngcc_operation_fn op,
                            void *op_ctx,
                            ngcc_perf_result_t *out_result) {
    unsigned long long i;
    unsigned long long warmup;
    struct timespec ts_total_start;
    struct timespec ts_total_end;
    struct timespec ts_iter_start;
    struct timespec ts_iter_end;
    cycle_counter_t counter;
    running_stats_t time_stats;
    running_stats_t cycle_stats;
    double *time_samples = NULL;
    double *cycle_samples = NULL;
    size_t sample_capacity = 0;
    int keep_time_samples = 0;
    int keep_cycle_samples = 0;
    int rc = -1;

    if (cfg == NULL || op == NULL || out_result == NULL) {
        ngcc_log_error("[performance] invalid benchmark arguments: cfg_null=%d op_null=%d out_null=%d",
                       cfg == NULL,
                       op == NULL,
                       out_result == NULL);
        return -1;
    }
    if (cfg->iterations == 0) {
        ngcc_log_error("[performance] invalid benchmark iterations: 0");
        return -1;
    }

    /* 预热: iterations 的 1%, 最少 10 次, 消除冷缓存影响 */
    warmup = cfg->iterations / 100;
    if (warmup < 10) {
        warmup = 10;
    }

    for (i = 0; i < warmup; ++i) {
        if (op(op_ctx) != 0) {
            ngcc_log_error("[performance] warmup operation failed: iteration=%llu/%llu",
                           i + 1ULL,
                           warmup);
            return -1;
        }
    }

    memset(out_result, 0, sizeof(*out_result));
    out_result->iterations = cfg->iterations;
    out_result->warmup_iterations = warmup;

    if (cycle_counter_open(&counter, 1) != 0) {
        if (!g_cycles_warning_printed) {
            ngcc_log_warning("[performance] cycle counter unavailable, falling back to time-only metrics");
            g_cycles_warning_printed = 1;
        }
        counter.source = CYCLE_SOURCE_NONE;
        counter.perf_fd = -1;
    }

    stats_init(&time_stats);
    stats_init(&cycle_stats);

    /* 分配样本数组, 用于计算中位数 (若 malloc 失败则降级为用均值代替中位数) */
    if (cfg->iterations <= (unsigned long long) SIZE_MAX) {
        sample_capacity = (size_t) cfg->iterations;
    }

    if (sample_capacity > 0) {
        time_samples = (double *) malloc(sample_capacity * sizeof(time_samples[0]));
        if (time_samples != NULL) {
            keep_time_samples = 1;
        }
        if (counter.source != CYCLE_SOURCE_NONE) {
            cycle_samples = (double *) malloc(sample_capacity * sizeof(cycle_samples[0]));
            if (cycle_samples != NULL) {
                keep_cycle_samples = 1;
            }
        }
    }

    if (ngcc_monotonic_clock_gettime(&ts_total_start) != 0) {
        ngcc_log_error("[performance] failed to read total start clock");
        goto cleanup;
    }

    /* ===== 测量循环: 每次迭代记录时间和 CPU 周期 ===== */
    for (i = 0; i < cfg->iterations; ++i) {
        unsigned long long iter_cycle_start;
        unsigned long long iter_cycles;
        double iter_time_ms;

        if (ngcc_monotonic_clock_gettime(&ts_iter_start) != 0) {
            ngcc_log_error("[performance] failed to read iteration start clock: iteration=%llu/%llu",
                           i + 1ULL,
                           cfg->iterations);
            goto cleanup;
        }

        iter_cycle_start = cycle_counter_begin(&counter);

        if (op(op_ctx) != 0) {
            ngcc_log_error("[performance] measured operation failed: iteration=%llu/%llu",
                           i + 1ULL,
                           cfg->iterations);
            goto cleanup;
        }

        iter_cycles = cycle_counter_end(&counter, iter_cycle_start);

        if (ngcc_monotonic_clock_gettime(&ts_iter_end) != 0) {
            ngcc_log_error("[performance] failed to read iteration end clock: iteration=%llu/%llu",
                           i + 1ULL,
                           cfg->iterations);
            goto cleanup;
        }

        iter_time_ms = timespec_ms_diff(&ts_iter_start, &ts_iter_end);
        stats_update(&time_stats, iter_time_ms);
        if (keep_time_samples) {
            time_samples[(size_t) i] = iter_time_ms;
        }

        if (counter.source != CYCLE_SOURCE_NONE) {
            double iter_cycle_value = (double) iter_cycles;
            stats_update(&cycle_stats, iter_cycle_value);
            if (keep_cycle_samples) {
                cycle_samples[(size_t) i] = iter_cycle_value;
            }
        }
    }

    if (ngcc_monotonic_clock_gettime(&ts_total_end) != 0) {
        ngcc_log_error("[performance] failed to read total end clock");
        goto cleanup;
    }

    /* ===== 吞吐量计算 ===== */
    out_result->elapsed_ms = timespec_ms_diff(&ts_total_start, &ts_total_end);
    if (out_result->elapsed_ms > 0.0) {
        /* ops_per_sec = iterations / (elapsed_ms / 1000) */
        out_result->ops_per_sec = ((double) cfg->iterations * 1000.0) / out_result->elapsed_ms;
    }
    out_result->bytes_per_op = (double) cfg->bytes_per_op;
    if (cfg->bytes_per_op > 0 && out_result->elapsed_ms > 0.0) {
        /* bytes_per_sec = iterations * bytes_per_op / (elapsed_ms / 1000) */
        out_result->bytes_per_sec = ((double) cfg->iterations * (double) cfg->bytes_per_op * 1000.0) / out_result->elapsed_ms;
        out_result->mb_per_sec = out_result->bytes_per_sec / 1000000.0;
    }

    /* ===== 时间统计: 均值 / 标准差 / 变异系数 / 中位数 ===== */
    out_result->cycles_available = (counter.source != CYCLE_SOURCE_NONE);
    out_result->time_ms_mean = time_stats.mean;
    out_result->time_ms_stddev = stats_stddev(&time_stats);
    if (time_stats.mean > 0.0) {
        /* CV = (stddev / mean) * 100% */
        out_result->time_ms_cv_percent = (out_result->time_ms_stddev / time_stats.mean) * 100.0;
    }
    out_result->time_ms_median = keep_time_samples ? compute_median(time_samples, sample_capacity) : time_stats.mean;

    /* ===== CPU 周期统计: 均值 / 标准差 / 变异系数 / min / max / 中位数 ===== */
    if (out_result->cycles_available && cycle_stats.count > 0) {
        out_result->cycles_per_op = cycle_stats.mean;
        out_result->cycles_min = cycle_stats.min;
        out_result->cycles_max = cycle_stats.max;
        out_result->cycles_stddev = stats_stddev(&cycle_stats);
        if (cycle_stats.mean > 0.0) {
            out_result->cycles_cv_percent = (out_result->cycles_stddev / cycle_stats.mean) * 100.0;
        }
        out_result->cycles_median = keep_cycle_samples ? compute_median(cycle_samples, sample_capacity) : cycle_stats.mean;
    } else {
        out_result->cycles_per_op = 0.0;
        out_result->cycles_min = 0.0;
        out_result->cycles_max = 0.0;
        out_result->cycles_stddev = 0.0;
        out_result->cycles_cv_percent = 0.0;
        out_result->cycles_median = 0.0;
    }

    /* cycles_per_byte = cycles_per_op / bytes_per_op  */
    if (out_result->cycles_available && out_result->cycles_per_op > 0.0 && cfg->bytes_per_op > 0) {
        out_result->cycles_per_byte = out_result->cycles_per_op / (double) cfg->bytes_per_op;
    }

    rc = 0;

cleanup:
    free(time_samples);
    free(cycle_samples);
    cycle_counter_close(&counter);
    return rc;
}
