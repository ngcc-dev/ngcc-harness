#ifndef CPUCYCLES_H
#define CPUCYCLES_H

#include <stdint.h>

#if defined(__aarch64__)

static inline uint64_t cpucycles(void) {
    uint64_t result;

    __asm__ volatile("mrs %0, cntvct_el0" : "=r"(result));

    return result;
}

#elif defined(USE_RDPMC) && (defined(__x86_64__) || defined(__i386__))
/* Needs echo 2 > /sys/devices/cpu/rdpmc */

static inline uint64_t cpucycles(void) {
    const uint32_t ecx = (1U << 30) + 1;
    uint64_t result;

    __asm__ volatile("rdpmc; shlq $32,%%rdx; orq %%rdx,%%rax"
                     : "=a"(result)
                     : "c"(ecx)
                     : "rdx");

    return result;
}

#elif defined(__x86_64__) || defined(__i386__)

static inline uint64_t cpucycles(void) {
    uint64_t result;

    __asm__ volatile("rdtsc; shlq $32,%%rdx; orq %%rdx,%%rax"
                     : "=a"(result)
                     :
                     : "%rdx");

    return result;
}

#else

#include <sys/time.h>

static inline uint64_t cpucycles(void) {
    struct timeval tv;

    gettimeofday(&tv, 0);

    return (uint64_t)tv.tv_sec * 1000000ULL + (uint64_t)tv.tv_usec;
}

#endif

uint64_t cpucycles_overhead(void);

#endif
