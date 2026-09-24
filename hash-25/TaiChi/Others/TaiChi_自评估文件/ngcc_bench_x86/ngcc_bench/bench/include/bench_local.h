#ifndef BENCH_LOCAL_H
#define BENCH_LOCAL_H

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "report.h"

#define CRYPTO_FAILED       1
#define CRYPTO_SUCCESS      0
#define HEAD_LENGTH         256


void randombytes(uint8_t *out, size_t outlen);

int measure_memory(const char *worker_path, struct mem_profile *mem_prof);

int static_mem_parse(const char *lib_name, struct mem_profile *mem_prof);

#if defined(__x86_64__) || defined(__i386__)
#include <x86intrin.h>
static inline uint64_t read_cycle_counter(void) {
    unsigned int unused = 0;
    return __rdtscp(&unused);
}
#elif defined(__aarch64__)
static inline uint64_t read_cycle_counter(void) {
    uint64_t value;
    asm volatile("mrs %0, cntvct_el0" : "=r"(value));
    return value;
}
#else
static inline uint64_t read_cycle_counter(void) {
    return 0;
}
#endif

#define BENCH_TIMES(func, times)                                                                         \
    {                                                                                                    \
        struct timespec start, end;                                                                      \
        clock_gettime(CLOCK_REALTIME, &start);                                                           \
        for (uint32_t bench_loop_idx = 0; bench_loop_idx < (times); bench_loop_idx++) {                   \
            if ((func) != CRYPTO_SUCCESS) {                                                              \
                printf("Error: %s\n", #func);                                                            \
                break;                                                                                   \
            }                                                                                            \
        }                                                                                                \
        clock_gettime(CLOCK_REALTIME, &end);                                                             \
        elapsedTime = (end.tv_sec - start.tv_sec) * 1000000000 + (end.tv_nsec - start.tv_nsec);          \
    }

#define BENCH_CYCLES(func, times, max_cycle, min_cycle, total_cycle)                                      \
    {                                                                                                    \
        for(uint32_t bench_loop_idx = 0; bench_loop_idx < (times); bench_loop_idx++) {                    \
            uint64_t timer1 = read_cycle_counter();                                                       \
            if((func) != CRYPTO_SUCCESS) {                                                               \
                printf("Error: %s\n", #func);                                                            \
                break;                                                                                   \
            }                                                                                            \
            uint64_t timer2 = read_cycle_counter();                                                       \
            uint64_t cost_cycle = timer2 - timer1;                                                        \
            if(cost_cycle > (max_cycle)) (max_cycle) = cost_cycle;                                        \
            if(cost_cycle < (min_cycle)) (min_cycle) = cost_cycle;                                        \
            (total_cycle) += cost_cycle;                                                                 \
        }                                                                                                \
    }

#define BENCH_HASH_SPEED(func, times, len, avg_cycle, max_cycle, min_cycle, throughput)                 \
    {                                                                                                   \
        uint64_t elapsedTime = 0;                                                                        \
        uint64_t total_cycle = 0;                                                                        \
        BENCH_TIMES(func, times);                                                                        \
        BENCH_CYCLES(func, times, max_cycle, min_cycle, total_cycle);                                    \
        avg_cycle = (double)total_cycle / times;                                                        \
        throughput = ((double)(len) * (double)(times) * 1000000000.0) /                                  \
                     ((double)elapsedTime * 1024.0 * 1024.0);                                           \
    }

#endif // BENCH_LOCAL_H