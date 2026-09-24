#ifndef BENCH_LOCAL_H
#define BENCH_LOCAL_H

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <x86intrin.h>
#include "report.h"

#define CRYPTO_FAILED       1
#define CRYPTO_SUCCESS      0

void randombytes(uint8_t *out, size_t outlen);

int static_mem_parse(const char *lib_name, struct mem_profile *mem_prof);

static inline uint64_t bench_rdtsc(void)
{
    unsigned int aux;
    return (uint64_t)__rdtscp(&aux);
}

static inline uint64_t bench_now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

#endif
