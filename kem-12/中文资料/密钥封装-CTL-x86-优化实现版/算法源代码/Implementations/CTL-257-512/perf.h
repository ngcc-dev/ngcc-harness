#ifndef PERF_H
#define PERF_H

#include <stdint.h>
#include <stdio.h>

/* Lightweight cross-platform perf hooks.
 * Define CTL_ENABLE_PERF to enable instrumentation at compile time.
 */

#if defined(__i386__) || defined(__x86_64__) || defined(_M_X64) || defined(_M_IX86)
  #if defined(_MSC_VER)
    #include <intrin.h>
    static inline uint64_t perf_rdtsc(void) { return __rdtsc(); }
  #else
    static inline uint64_t perf_rdtsc(void) {
      unsigned int hi, lo;
      __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
      return ((uint64_t)hi << 32) | lo;
    }
  #endif

  #define PERF_START(var) uint64_t var = perf_rdtsc();
  #define PERF_END(var, msg) do { uint64_t perf_end = perf_rdtsc(); \
      printf("PERF %s: %llu cycles\n", msg, (unsigned long long)(perf_end - (var))); } while(0)

#else
  #include <time.h>
  static inline uint64_t perf_rdtsc(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
  }
  #define PERF_START(var) uint64_t var = perf_rdtsc();
  #define PERF_END(var, msg) do { uint64_t perf_end = perf_rdtsc(); \
      printf("PERF %s: %llu ns\n", msg, (unsigned long long)(perf_end - (var))); } while(0)
#endif

#endif /* PERF_H */
