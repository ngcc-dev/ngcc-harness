#ifndef REPORT_H
#define REPORT_H

#include <stddef.h>
#include <stdint.h>

#define REPORT_PATH_LEN     128
#define HASH_PERF_LEVELS    8

#define BENCH_LOG(format, ...)                      \
{                                                   \
    printf(format, ##__VA_ARGS__);                  \
    if(g_log_file) {                                \
        fprintf(g_log_file, format, ##__VA_ARGS__); \
        fflush(g_log_file);                         \
    }                                               \
}

struct mem_profile {
    size_t text_size;
    size_t data_size;
    size_t bss_size;
    size_t static_total;
    size_t heap_peak;
    size_t stack_peak;
    size_t peak_total;
};

struct perf_profile {
    uint64_t max_cycles;
    uint64_t min_cycles;
    double avg_cycles;
    double throughput;
    uint32_t run_time;
};

struct hash_func_profile {
    uint8_t long_input_correct;
    uint8_t short_input_correct;
};

struct hash_parameter {
    size_t block_len;
    size_t digest_len;
};

struct hash_perf_item {
    const char *level;
    size_t input_size;
    struct perf_profile hash_perf;
};

struct hash_perf_profile {
    struct hash_perf_item items[HASH_PERF_LEVELS];
};

typedef struct bench_report {
    const char *alg_name;
    const char *author_name;

    union {
        struct hash_parameter hash_para;
    } alg_parameter;

    union {
        struct hash_perf_profile hash_perf;
    } perf_report;

    union {
        struct hash_func_profile hash_func;
    } func_report;

    struct mem_profile mem_report;
} bench_report_t;

void save_hash_full_report(char *report_file_name, bench_report_t *report);

#endif