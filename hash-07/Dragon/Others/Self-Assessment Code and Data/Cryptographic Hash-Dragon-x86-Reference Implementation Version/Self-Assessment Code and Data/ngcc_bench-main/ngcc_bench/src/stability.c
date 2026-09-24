#include "stability.h"

#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "cycle_counter.h"
#include "mem_stat.h"
#include "ngcc_log.h"
#include "stats_util.h"

static volatile sig_atomic_t g_stop_requested = 0;

static void on_signal(int signo) {
    (void) signo;
    g_stop_requested = 1;
}

static int install_signal_handlers(struct sigaction *old_int, struct sigaction *old_term) {
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_signal;

    if (sigaction(SIGINT, &sa, old_int) != 0) {
        return -1;
    }
    if (sigaction(SIGTERM, &sa, old_term) != 0) {
        sigaction(SIGINT, old_int, NULL);
        return -1;
    }

    return 0;
}

static void restore_signal_handlers(const struct sigaction *old_int, const struct sigaction *old_term) {
    sigaction(SIGINT, old_int, NULL);
    sigaction(SIGTERM, old_term, NULL);
}

static void append_reason(char *dst, size_t cap, const char *reason) {
    size_t cur = strlen(dst);
    size_t n;
    if (cur >= cap - 1U) {
        return;
    }
    n = strlen(reason);
    if (n > cap - 1U - cur) {
        n = cap - 1U - cur;
    }
    memcpy(dst + cur, reason, n);
    dst[cur + n] = '\0';
}

void ngcc_stability_thresholds_set_defaults(ngcc_stability_thresholds_t *out_thresholds) {
    if (out_thresholds == NULL) {
        return;
    }

    out_thresholds->stable_throughput_cv_percent = 5.0;
    out_thresholds->stable_cycles_cv_percent = 5.0;
    out_thresholds->stable_time_cv_percent = 5.0;
    out_thresholds->stable_rss_growth_percent = 1.0;
    out_thresholds->stable_rss_growth_abs_bytes = 100000;
    out_thresholds->stable_error_rate_percent = 0.0;
}

int ngcc_evaluate_rss_stability(uint64_t rss_start,
                                uint64_t rss_end,
                                const ngcc_stability_thresholds_t *thresholds,
                                ngcc_stability_result_t *out_result) {
    if (thresholds == NULL || out_result == NULL) {
        return -1;
    }

    out_result->rss_start_bytes = rss_start;
    out_result->rss_end_bytes = rss_end;
    out_result->rss_growth_abs_bytes = (rss_end > rss_start) ? (rss_end - rss_start) : 0U;
    out_result->rss_growth_percent = 0.0;
    out_result->memory_stable = 0;

    if (rss_start == 0U || rss_end == 0U) {
        return 0;
    }

    out_result->rss_growth_percent = (double) out_result->rss_growth_abs_bytes * 100.0 /
                                     (double) rss_start;
    out_result->memory_stable =
        out_result->rss_growth_percent < thresholds->stable_rss_growth_percent &&
        out_result->rss_growth_abs_bytes < thresholds->stable_rss_growth_abs_bytes;
    return 0;
}

int ngcc_run_stability(const ngcc_api_t *api,
                       ngcc_correctness_dispatch_fn correctness_fn,
                       ngcc_bytes_per_case_fn bytes_per_case_fn,
                       int digest_len_bits,
                       size_t msg_len,
                       int cycles_enabled,
                       double sample_target_ms,
                       double duration_hours,
                       unsigned long long max_cases,
                       const ngcc_stability_thresholds_t *thresholds,
                       ngcc_stability_result_t *out_result) {
    struct sigaction old_int;
    struct sigaction old_term;
    struct timespec ts_start;
    struct timespec ts_now;
    cycle_counter_t counter;
    running_stats_t throughput_stats;
    running_stats_t throughput_bytes_stats;
    running_stats_t cycles_stats;
    running_stats_t time_stats;
    double max_seconds;
    unsigned long long cases_run = 0;
    unsigned long long sample_count = 0;
    unsigned long long total_executions = 0;
    unsigned long long error_count = 0;
    int loop_failed = 0;
    int cycles_warning_printed = 0;
    ngcc_stability_thresholds_t effective_thresholds;
    unsigned long long bytes_per_case;
    uint64_t rss_start;
    uint64_t rss_end;

    if (api == NULL || correctness_fn == NULL || out_result == NULL || max_cases == 0 ||
        duration_hours <= 0.0 || sample_target_ms <= 0.0 || !isfinite(sample_target_ms)) {
        ngcc_log_error("[stability] invalid arguments: api_null=%d correctness_fn_null=%d out_null=%d max_cases=%llu duration_hours=%.6f sample_target_ms=%.6f",
                       api == NULL,
                       correctness_fn == NULL,
                       out_result == NULL,
                       max_cases,
                       duration_hours,
                       sample_target_ms);
        return -1;
    }

    memset(out_result, 0, sizeof(*out_result));
    g_stop_requested = 0;
    out_result->status[0] = '\0';
    out_result->failure_reasons[0] = '\0';
    ngcc_stability_thresholds_set_defaults(&effective_thresholds);
    if (thresholds != NULL) {
        effective_thresholds = *thresholds;
    }

    if (install_signal_handlers(&old_int, &old_term) != 0) {
        ngcc_log_error("[stability] failed to install signal handlers");
        return -1;
    }

    if (ngcc_monotonic_clock_gettime(&ts_start) != 0) {
        ngcc_log_error("[stability] failed to read start clock");
        restore_signal_handlers(&old_int, &old_term);
        return -1;
    }

    if (cycle_counter_open(&counter, cycles_enabled) != 0) {
        ngcc_log_warning("[stability] cycle counter unavailable, falling back to time-only metrics");
        counter.source = CYCLE_SOURCE_NONE;
        counter.perf_fd = -1;
        cycles_warning_printed = 1;
    }

    rss_start = ngcc_mem_current_rss_bytes();

    stats_init(&throughput_stats);
    stats_init(&throughput_bytes_stats);
    stats_init(&cycles_stats);
    stats_init(&time_stats);
    bytes_per_case = (bytes_per_case_fn != NULL) ? bytes_per_case_fn(api, msg_len) : 0;
    max_seconds = duration_hours * 3600.0;

    while (1) {
        double elapsed;
        struct timespec ts_batch_start;
        struct timespec ts_batch_end;
        double batch_ms;
        unsigned long long batch_cycle_start;
        unsigned long long batch_cycles;
        unsigned long long batch_ok = 0;
        int batch_failed = 0;
        uint64_t current_mem;

        if (g_stop_requested) {
            out_result->interrupted = 1;
            break;
        }

        if (cases_run >= max_cases) {
            break;
        }

        if (ngcc_monotonic_clock_gettime(&ts_now) != 0) {
            ngcc_log_error("[stability] failed to read loop clock: cases_run=%llu total_executions=%llu",
                           cases_run,
                           total_executions);
            loop_failed = 1;
            break;
        }

        elapsed = (double) (ts_now.tv_sec - ts_start.tv_sec) +
                  ((double) (ts_now.tv_nsec - ts_start.tv_nsec) / 1000000000.0);
        if (elapsed >= max_seconds) {
            break;
        }

        if (ngcc_monotonic_clock_gettime(&ts_batch_start) != 0) {
            ngcc_log_error("[stability] failed to read batch start clock: cases_run=%llu total_executions=%llu",
                           cases_run,
                           total_executions);
            loop_failed = 1;
            break;
        }
        batch_cycle_start = cycle_counter_begin(&counter);

        while (1) {
            int case_rc = correctness_fn(api, digest_len_bits, msg_len);

            total_executions++;
            if (case_rc != 0) {
                error_count++;
                if (error_count <= 3ULL) {
                    ngcc_log_error("[stability] correctness case failed: case=%llu execution=%llu digest_len_bits=%d msg_len=%zu rc=%d",
                                   cases_run + 1ULL,
                                   total_executions,
                                   digest_len_bits,
                                   msg_len,
                                   case_rc);
                }
                /* Continue rather than break — accumulate error rate instead of aborting */
            } else {
                batch_ok++;
            }
            cases_run++;

            if (g_stop_requested || cases_run >= max_cases) {
                break;
            }

            if (ngcc_monotonic_clock_gettime(&ts_now) != 0) {
                ngcc_log_error("[stability] failed to read inner loop clock: cases_run=%llu total_executions=%llu",
                               cases_run,
                               total_executions);
                loop_failed = 1;
                batch_failed = 1;
                break;
            }

            batch_ms = (double) (ts_now.tv_sec - ts_batch_start.tv_sec) * 1000.0 +
                       (double) (ts_now.tv_nsec - ts_batch_start.tv_nsec) / 1000000.0;
            if (batch_ms >= sample_target_ms) {
                break;
            }

            elapsed = (double) (ts_now.tv_sec - ts_start.tv_sec) +
                      ((double) (ts_now.tv_nsec - ts_start.tv_nsec) / 1000000000.0);
            if (elapsed >= max_seconds) {
                break;
            }
        }

        batch_cycles = cycle_counter_end(&counter, batch_cycle_start);
        if (ngcc_monotonic_clock_gettime(&ts_batch_end) != 0) {
            ngcc_log_error("[stability] failed to read batch end clock: cases_run=%llu total_executions=%llu",
                           cases_run,
                           total_executions);
            loop_failed = 1;
            break;
        }

        batch_ms = (double) (ts_batch_end.tv_sec - ts_batch_start.tv_sec) * 1000.0 +
                   (double) (ts_batch_end.tv_nsec - ts_batch_start.tv_nsec) / 1000000.0;

        if (batch_ok > 0) {
            double avg_case_ms = batch_ms / (double) batch_ok;
            sample_count++;
            stats_update(&time_stats, avg_case_ms);
            if (batch_ms > 0.0) {
                double throughput_ops = ((double) batch_ok * 1000.0) / batch_ms;
                stats_update(&throughput_stats, throughput_ops);
                if (bytes_per_case > 0U) {
                    stats_update(&throughput_bytes_stats, throughput_ops * (double) bytes_per_case);
                }
            }
            if (counter.source != CYCLE_SOURCE_NONE) {
                stats_update(&cycles_stats, (double) batch_cycles / (double) batch_ok);
            }
        }

        if (batch_failed || loop_failed) {
            break;
        }

        if (g_stop_requested) {
            out_result->interrupted = 1;
            break;
        }
    }

    if (ngcc_monotonic_clock_gettime(&ts_now) == 0) {
        out_result->elapsed_seconds = (double) (ts_now.tv_sec - ts_start.tv_sec) +
                                      ((double) (ts_now.tv_nsec - ts_start.tv_nsec) / 1000000000.0);
    }

    rss_end = ngcc_mem_current_rss_bytes();

    out_result->cases_run = cases_run;
    out_result->sample_count = sample_count;
    out_result->failed = loop_failed;
    out_result->total_executions = total_executions;
    out_result->error_count = error_count;
    if (total_executions > 0U) {
        out_result->error_rate_percent = (double) error_count * 100.0 / (double) total_executions;
    }

    out_result->time_mean_ms = time_stats.mean;
    out_result->time_stddev_ms = stats_stddev(&time_stats);
    out_result->time_min_ms = time_stats.min;
    out_result->time_max_ms = time_stats.max;
    if (time_stats.mean > 0.0) {
        out_result->time_cv_percent = out_result->time_stddev_ms * 100.0 / time_stats.mean;
    }

    out_result->throughput_mean_ops = throughput_stats.mean;
    out_result->throughput_stddev_ops = stats_stddev(&throughput_stats);
    out_result->throughput_min_ops = throughput_stats.min;
    out_result->throughput_max_ops = throughput_stats.max;
    if (throughput_stats.mean > 0.0) {
        out_result->throughput_cv_percent = out_result->throughput_stddev_ops * 100.0 / throughput_stats.mean;
    }
    out_result->bytes_per_case = (double) bytes_per_case;
    out_result->throughput_mean_bytes = throughput_bytes_stats.mean;
    out_result->throughput_stddev_bytes = stats_stddev(&throughput_bytes_stats);
    out_result->throughput_min_bytes = throughput_bytes_stats.min;
    out_result->throughput_max_bytes = throughput_bytes_stats.max;
    out_result->throughput_mean_mb = throughput_bytes_stats.mean / 1000000.0;
    out_result->throughput_stddev_mb = stats_stddev(&throughput_bytes_stats) / 1000000.0;
    out_result->throughput_min_mb = throughput_bytes_stats.min / 1000000.0;
    out_result->throughput_max_mb = throughput_bytes_stats.max / 1000000.0;
    if (throughput_bytes_stats.mean > 0.0) {
        out_result->throughput_cv_percent_bytes = out_result->throughput_stddev_bytes * 100.0 / throughput_bytes_stats.mean;
    }

    out_result->cycles_available = (counter.source != CYCLE_SOURCE_NONE);
    if (out_result->cycles_available) {
        out_result->cycles_mean = cycles_stats.mean;
        out_result->cycles_stddev = stats_stddev(&cycles_stats);
        out_result->cycles_min = cycles_stats.min;
        out_result->cycles_max = cycles_stats.max;
        if (cycles_stats.mean > 0.0) {
            out_result->cycles_cv_percent = out_result->cycles_stddev * 100.0 / cycles_stats.mean;
        }
    }

    if (ngcc_evaluate_rss_stability(rss_start,
                                    rss_end,
                                    &effective_thresholds,
                                    out_result) != 0) {
        out_result->failed = 1;
    }

    out_result->performance_stable = (out_result->throughput_cv_percent < effective_thresholds.stable_throughput_cv_percent &&
                                       out_result->time_cv_percent < effective_thresholds.stable_time_cv_percent &&
                                       (!out_result->cycles_available ||
                                        out_result->cycles_cv_percent < effective_thresholds.stable_cycles_cv_percent));
    out_result->correctness_stable = (out_result->error_rate_percent <= effective_thresholds.stable_error_rate_percent);
    out_result->is_stable = (out_result->performance_stable &&
                             out_result->memory_stable &&
                             out_result->correctness_stable);

    if (out_result->is_stable) {
        strcpy(out_result->status, "STABLE");
    } else {
        strcpy(out_result->status, "UNSTABLE");
    }

    if (!out_result->performance_stable) {
        append_reason(out_result->failure_reasons, sizeof(out_result->failure_reasons), "performance fluctuation; ");
    }
    if (rss_start == 0U || rss_end == 0U) {
        append_reason(out_result->failure_reasons, sizeof(out_result->failure_reasons), "rss measurement unavailable; ");
    } else if (!out_result->memory_stable) {
        append_reason(out_result->failure_reasons, sizeof(out_result->failure_reasons), "memory growth; ");
    }
    if (!out_result->correctness_stable) {
        append_reason(out_result->failure_reasons, sizeof(out_result->failure_reasons), "runtime errors; ");
    }
    if (cycles_warning_printed) {
        append_reason(out_result->failure_reasons, sizeof(out_result->failure_reasons), "cycle counter unavailable; ");
    }

    if (out_result->failed || !out_result->is_stable) {
        ngcc_log_error("[stability] completed with failing status: status=%s cases_run=%llu samples=%llu errors=%llu error_rate=%.6f%% rss_growth_bytes=%llu rss_growth=%.6f%% throughput_cv=%.6f%% time_cv=%.6f%% reasons=%s",
                       out_result->status,
                       out_result->cases_run,
                       out_result->sample_count,
                       out_result->error_count,
                       out_result->error_rate_percent,
                       (unsigned long long) out_result->rss_growth_abs_bytes,
                       out_result->rss_growth_percent,
                       out_result->throughput_cv_percent,
                       out_result->time_cv_percent,
                       out_result->failure_reasons);
    }


    cycle_counter_close(&counter);
    restore_signal_handlers(&old_int, &old_term);
    return out_result->failed ? -1 : 0;
}
