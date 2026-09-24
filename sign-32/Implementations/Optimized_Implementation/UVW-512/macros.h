#ifndef MACROS_H
#define MACROS_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

#define SWAP(x, y, T) do { \
    T _ = x; \
    x = y; \
    y = _; \
} while (0)

#define TEST(expr, ...) do { \
    fprintf(stderr, (expr) ? "[\e[32m OK \e[39m] " : "[\e[31mFAIL\e[39m] "); \
    fprintf(stderr, __VA_ARGS__); \
    fputc('\n', stderr); \
} while (0)

#define DBG(...) do { \
    fprintf(stderr, "%s:%d: ", __FILE__, __LINE__); \
    fprintf(stderr, __VA_ARGS__); \
    fputc('\n', stderr); \
} while (0)

#define PANIC(...) do { \
    fprintf(stderr, "Panicked at %s:%d\n", __FILE__, __LINE__); \
    fprintf(stderr, __VA_ARGS__); \
    fputc('\n', stderr); \
    exit_with_breakpoint(); \
} while (0)

static inline void exit_with_breakpoint() {
    exit(-1);
}

#define TODO(...) PANIC("TODO " __VA_ARGS__)
#define UNREACHABLE(...) PANIC("Entered unreachable code " __VA_ARGS__)

static inline uint64_t time_now() {
    struct timespec t;
    timespec_get(&t, TIME_UTC);
    return (uint64_t)1e9 * t.tv_sec + t.tv_nsec;
}

#if defined(_MSC_VER)
    #include <intrin.h>
    #pragma intrinsic(__rdtsc)
    static inline uint64_t cycle_now() {
        return __rdtsc();
    }
#else
    static inline uint64_t cycle_now() {
        uint32_t lo, hi;
        __asm__ __volatile__ (
            "rdtsc" : "=a"(lo), "=d"(hi)
        );
        return ((uint64_t)hi << 32) | lo;
    }
#endif

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
    static uint64_t ts[8], te[8], cs[8], ce[8];
    static void *ms[8];
    static size_t sp = 0;
#pragma GCC diagnostic pop

#define TIMING_START(msg) do { \
    ts[sp] = time_now(); \
    cs[sp] = cycle_now(); \
    ms[sp] = msg; \
    DBG("%s - Timing ...", ms[sp]); \
    sp++; \
} while (0)

#define TIMING_END() do { \
    sp--; \
    te[sp] = time_now(); \
    ce[sp] = cycle_now(); \
    DBG("%s - Time: %gms %gm cycles", ms[sp], (te[sp] - ts[sp]) / (double)1e6, (ce[sp] - cs[sp]) / (double)1e6); \
} while (0)

#endif