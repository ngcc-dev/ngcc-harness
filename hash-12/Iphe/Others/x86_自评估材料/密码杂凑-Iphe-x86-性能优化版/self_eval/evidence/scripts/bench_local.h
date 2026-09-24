#ifndef BENCH_LOCAL_H
#define BENCH_LOCAL_H

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <sys/time.h>
#include <x86intrin.h>
#include "report.h"

#define CRYPTO_FAILED       1
#define CRYPTO_SUCCESS      0
#define HEAD_LENGTH         256


void randombytes(uint8_t *out, size_t outlen);

int measure_memory(uint32_t alg_id, struct mem_profile *mem_prof);

int static_mem_parse(const char *lib_name, struct mem_profile *mem_prof);

#define BENCH_TIMES(func, times)                                                                         \
    {                                                                                                    \
        struct timespec start, end;                                                                      \
        clock_gettime(CLOCK_REALTIME, &start);                                                           \
        for (uint32_t i = 0; i < times; i++) {                                                                \
            if (func != CRYPTO_SUCCESS) {                                                                \
                printf("Error: %s\n", #func);                                                            \
                break;                                                                                   \
            }                                                                                            \
        }                                                                                                \
        clock_gettime(CLOCK_REALTIME, &end);                                                             \
        elapsedTime = (end.tv_sec - start.tv_sec) * 1000000000 + (end.tv_nsec - start.tv_nsec);          \
    }

#define BENCH_CYCLES(func, times, max_cycle, min_cycle, total_cycle)                                     \
    {                                                                                                    \
        register uint64_t timer1, timer2;                                                                \
        unsigned int unused __attribute__((unused)) = 0;                                                 \
        for(uint32_t i = 0; i < times; i++) {                                                                 \
            timer1 = __rdtscp(&unused);                                                                  \
            if (func != CRYPTO_SUCCESS) {                                                                \
                printf("Error: %s\n", #func);                                                            \
                break;                                                                                   \
            }                                                                                            \
            timer2 = __rdtscp(&unused);                                                                  \
            uint64_t cost_cycle = timer2 - timer1;                                                       \
            if(cost_cycle > max_cycle) max_cycle = cost_cycle;                                           \
            if(cost_cycle < min_cycle) min_cycle = cost_cycle;                                           \
            total_cycle += cost_cycle;                                                                   \
        }                                                                                                \
    }

#define BENCH_PKC_SPEED_VA(func, times, avg_cycle, max_cycle,                   \
            min_cycle, throughput, headerFmt, ...)                              \
    {                                                                           \
        uint64_t elapsedTime;                                                   \
        uint64_t total_cycle = 0;                                               \
        char header[HEAD_LENGTH] = {0};                                         \
        snprintf(header, sizeof(header), headerFmt, ##__VA_ARGS__);             \
        BENCH_TIMES(func, times);                                               \
        BENCH_CYCLES(func, times, max_cycle, min_cycle, total_cycle);           \
        avg_cycle = (double)total_cycle / times;                                \
        throughput = ((double)times * 1000000000) / elapsedTime;                \
        BENCH_LOG("\t|   %-25s, %10d, %20.2f, %15.2f   |\n", header, times,     \
                avg_cycle, throughput);                                         \
    }

#define BENCH_CIPHER_SPEED_VA(func, times, len, avg_cycle, max_cycle,                   \
            min_cycle, throughput, header)                                              \
    {                                                                                   \
        uint64_t elapsedTime;                                                           \
        uint64_t total_cycle = 0;                                                       \
        BENCH_TIMES(func, times);                                                       \
        BENCH_CYCLES(func, times, max_cycle, min_cycle, total_cycle);                   \
        avg_cycle = (double)total_cycle / times;                                        \
        throughput = elapsedTime == 0 ? 0.0 :                                          \
            ((double)(len) * (double)(times) * 8.0 * 1000.0) / (double)elapsedTime;    \
        BENCH_LOG("\t|   %-12s, %10d, %12d, %18.2f, %16.2f   |\n", header, times,       \
            len, avg_cycle, throughput);                                                \
    }

#endif // BENCH_LOCAL_H