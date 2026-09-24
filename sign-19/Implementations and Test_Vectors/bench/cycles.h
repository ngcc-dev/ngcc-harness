#ifndef CYCLES_H
#define CYCLES_H

#include <stdint.h>

static inline uint64_t cpucycles(void) {
    unsigned int lo, hi;
    __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

static inline void init_cpucycles(void) { /* no-op for rdtsc */ }

#endif
