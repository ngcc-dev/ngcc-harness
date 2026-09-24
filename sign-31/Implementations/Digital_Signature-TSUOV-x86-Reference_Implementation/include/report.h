#ifndef REPORT_H
#define REPORT_H
#include <stddef.h>
#include <stdint.h>

#define REPORT_PATH_LEN   512

extern FILE *g_log_file;

#define BENCH_LOG(format, ...)                      \
{                                                   \
    printf(format, ##__VA_ARGS__);                  \
    fflush(stdout);                                 \
    if(g_log_file) {                                \
        fprintf(g_log_file, format, ##__VA_ARGS__); \
        fflush(g_log_file);                         \
    }                                               \
}

struct mem_profile {
    size_t text_size;
    size_t data_size;
    size_t bss_size;
    size_t heap_peak;
    size_t stack_peak;
    long peak_rss;
};

struct perf_profile {
    uint64_t max_cycles;
    uint64_t min_cycles;
    double avg_cycles;
    double throughput;
    uint32_t run_time;
};

struct sig_func_profile {
    uint8_t correct;
    uint8_t sig_forgery;
};

struct sig_parameter {
    size_t pub_len;
    size_t pri_len;
    size_t sig_len;
};

struct sig_perf_profile {
    struct perf_profile keygen_perf;
    struct perf_profile sign_perf;
    struct perf_profile verify_perf;
};

typedef struct bench_report {
    const char *alg_name;
    const char *author_name;
    int security_level;
    uint32_t iterations;
    struct sig_parameter sig_para;
    struct sig_perf_profile sig_perf;
    struct sig_func_profile sig_func;
    struct mem_profile mem_report;
} bench_report_t;

void save_sig_full_report(const char *report_file_name, bench_report_t *report,
                          const char *timestamp);
void append_sig_csv_summary(const char *output_dir, bench_report_t *report, const char *timestamp);

#endif
