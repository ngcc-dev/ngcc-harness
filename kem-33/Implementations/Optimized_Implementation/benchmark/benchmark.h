/*
 * $Id: benchmark.h 1271 2008-06-08 08:06:13Z owenhsin $
 */

#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

// #define CONFIG_BENCH_SYSTIME

#if defined(CONFIG_BENCH_SYSTIME)
#include <sys/types.h>
#include <sys/time.h>
#endif



#ifdef __cplusplus
extern "C"
{
#endif


#define RECMAX 6

#define BENCHMARK(bm,call) do { \
        bm_start(&(bm)); \
        call; \
        bm_stop(&(bm)); \
    } while (0)

struct benchmark {
    #if defined(CONFIG_BENCH_SYSTIME)
    struct timeval start;
    struct timeval stop;
    #else
    uint64_t start;
    uint64_t stop;
    #endif  // CONFIG_BENCH_SYSTIME
    double record[RECMAX];
    double acc;
    int currec;
    int count;
};


#ifdef __cplusplus
}
#endif


#define _M1CYCLES_

#if defined(_APPLE_SILICON_)&&defined(_M1CYCLES_)
#include "m1cycles.h"
#endif

#if !defined(CONFIG_BENCH_SYSTIME)
#if defined(__aarch64__)
static inline uint64_t rdtsc(void) {
    uint64_t val;
    #if defined(_APPLE_SILICON_)
    #if defined(_M1CYCLES_)
    return __m1_rdtsc();
    #else
    // counter-timer virtual count register
    // better than nothing.
    __asm__ __volatile__ ("mrs %0, cntvct_el0" : "=r" (val));
    #endif
    #else
    // performance monitors cycle count register. likely to be banned in user space.
    // see https://github.com/mupq/pqax/tree/main/enable_ccr
    __asm__ __volatile__ ("mrs %0, PMCCNTR_EL0" : "=r" (val));
    #endif  // _APPLE_SILICON_
    return val;
}
#else
/* Copied from http://en.wikipedia.org/wiki/RDTSC */
static inline uint64_t rdtsc(void) {
    uint32_t lo, hi;
    /* We cannot use "=A", since this would use %rax on x86_64 */
    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
    return (uint64_t)hi << 32 | lo;
}
#endif  // __aarch64__
#endif  // CONFIG_BENCH_SYSTIME



static inline void
bm_init(struct benchmark *bm) {
    if (NULL != bm ) memset(bm, 0, sizeof(*bm));
    #if defined(_APPLE_SILICON_)&&defined(_M1CYCLES_)
    __m1_setup_rdtsc();
    #endif
}

static inline void
bm_start(struct benchmark *bm) {
    #if defined(CONFIG_BENCH_SYSTIME)
    gettimeofday(& bm->start, NULL);
    #else
    bm->start = rdtsc();
    #endif
}

static inline void
bm_stop(struct benchmark *bm) {
    #if defined(CONFIG_BENCH_SYSTIME)
    gettimeofday(& bm->stop, NULL);
    bm->record[bm->currec] = (bm->stop.tv_sec - bm->start.tv_sec) * 1000000.0; /* sec to us */
    bm->record[bm->currec] += bm->stop.tv_usec - bm->start.tv_usec;
    #else
    bm->stop = rdtsc();
    bm->record[bm->currec] = bm->stop - bm->start;
    #endif
    bm->acc += bm->record[bm->currec];
    bm->currec = (bm->currec + 1) % RECMAX;
    ++bm->count;
}

static inline void
bm_dump(char *buf, size_t bufsize, const struct benchmark *bm) {
    int i;
    size_t len;
    #if defined(CONFIG_BENCH_SYSTIME)
    const char *unit = "micro sec.";
    #else
    const char *unit = "cycles";
    #endif

    len = snprintf(buf, bufsize, "%.0lf (%s, avg. of %d):", bm->acc / bm->count, unit, bm->count);
    buf += len;
    bufsize -= len;
    for (i = 0; i < RECMAX; ++i) {
        len = snprintf(buf, bufsize, " %.0lf", bm->record[i]);
        buf += len;
        bufsize -= len;
    }
}

static
int _cmp_u64(const void *_a, const void *_b) {
    uint64_t a = *(const uint64_t *)_a;
    uint64_t b = *(const uint64_t *)_b;
    if ( a > b ) {
        return 1;
    } else if ( a == b ) {
        return 0;
    } else {
        return -1;
    }
}

static inline
void report(char *buf, size_t bufsize, uint64_t *recs, unsigned len) {
    qsort((void *)recs, len, sizeof(uint64_t), _cmp_u64);
    uint64_t sum = 0;
    for (unsigned i = 0; i < len; i++) {
        sum += recs[i];
    }
    snprintf(buf, bufsize, " %lf /%d (%" PRIu64 ",%" PRIu64 ",%" PRIu64 ")", ((double)sum) / ((double)len), len, recs[len / 4], recs[len / 2], recs[len / 2 + len / 4]);
}

#define REC_TIMING(recs,len,call) do { \
        recs[*len]=rdtsc(); \
        call; \
        recs[*len]=rdtsc()-recs[*len]; \
        (*len)++; \
    } while (0)

#endif /* BENCHMARK_H */

