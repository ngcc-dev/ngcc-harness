#define _POSIX_C_SOURCE 200809L

#include "eijen.h"

#if defined(__x86_64__) || defined(__i386__)
#include <x86intrin.h>
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define EIJEN_BENCH_REPETITIONS 5U

typedef struct {
    const char *level;
    size_t len;
} bench_size;

static const bench_size BENCH_SIZES[] = {
    {"S1", 32U},
    {"S2", 128U},
    {"S3", 512U},
    {"S4", 1024U},
    {"S5", 4096U},
    {"S6", 8192U},
    {"S7", 16384U},
    {"S8", 65536U}
};

static volatile uint64_t eijen_bench_sink = 0U;

static uint64_t now_ns(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC_RAW, &ts) != 0) {
        return 0U;
    }
    return ((uint64_t)ts.tv_sec * UINT64_C(1000000000)) + (uint64_t)ts.tv_nsec;
}

static uint64_t cycles_now(void) {
#if defined(__x86_64__) || defined(__i386__)
    _mm_lfence();
    return (uint64_t)__rdtsc();
#else
    return 0U;
#endif
}

static uint64_t cycles_done(void) {
#if defined(__x86_64__) || defined(__i386__)
    unsigned int aux;
    uint64_t t = (uint64_t)__rdtscp(&aux);
    _mm_lfence();
    return t;
#else
    return 0U;
#endif
}

static size_t iterations_for_len(size_t len) {
    size_t target_bytes = 2U * 1024U * 1024U;
    size_t iters = target_bytes / len;
    if (iters < 200U) {
        iters = 200U;
    }
    return iters;
}

static void fill_input(uint8_t *buf, size_t len, unsigned int seed) {
    size_t i;
    uint32_t x = UINT32_C(0x9e3779b9) ^ seed;
    for (i = 0; i < len; i++) {
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        buf[i] = (uint8_t)(x & 0xffU);
    }
}

static uint64_t checksum_digest(const uint8_t *digest, size_t len) {
    size_t i;
    uint64_t x = UINT64_C(0xcbf29ce484222325);
    for (i = 0; i < len; i++) {
        x ^= (uint64_t)digest[i];
        x *= UINT64_C(0x100000001b3);
    }
    return x;
}

static int run_case(int digest_bits, const bench_size *sz) {
    uint8_t *msg;
    uint8_t digest[EIJEN_MAX_DIGEST_BYTES];
    size_t i;
    unsigned int rep;
    size_t iters = iterations_for_len(sz->len);
    double best_ns = -1.0;
    double best_cycles = -1.0;
    double total_mib;
    double avg_ns;
    double avg_cycles;
    double mib_s;
    uint64_t best_checksum = 0U;
    size_t digest_len = eijen_digest_size(digest_bits);

    msg = (uint8_t *)malloc(sz->len);
    if (msg == NULL) {
        return -1;
    }
    fill_input(msg, sz->len, (unsigned int)(digest_bits + sz->len));
    memset(digest, 0, sizeof(digest));

    if (eijen_hash(digest_bits, msg, sz->len, digest) != 0) {
        free(msg);
        return -1;
    }

    for (rep = 0U; rep < EIJEN_BENCH_REPETITIONS; rep++) {
        uint64_t t0;
        uint64_t t1;
        uint64_t c0;
        uint64_t c1;
        uint64_t checksum = 0U;
        fill_input(msg, sz->len, (unsigned int)(digest_bits + sz->len + rep));
        c0 = cycles_now();
        t0 = now_ns();
        for (i = 0; i < iters; i++) {
            msg[i % sz->len] = (uint8_t)(msg[i % sz->len] + (uint8_t)i + (uint8_t)rep);
            if (eijen_hash(digest_bits, msg, sz->len, digest) != 0) {
                free(msg);
                return -1;
            }
            checksum ^= checksum_digest(digest, digest_len) + (uint64_t)i;
        }
        t1 = now_ns();
        c1 = cycles_done();
        eijen_bench_sink ^= checksum;
        if (best_ns < 0.0 || (double)(t1 - t0) < best_ns) {
            best_ns = (double)(t1 - t0);
            best_cycles = (double)(c1 - c0);
            best_checksum = checksum;
        }
    }

    total_mib = ((double)sz->len * (double)iters) / (1024.0 * 1024.0);
    avg_ns = best_ns / (double)iters;
    avg_cycles = best_cycles / (double)iters;
    mib_s = total_mib / (best_ns / 1000000000.0);

    printf("%s,%d,%s,%lu,%lu,%u,%.0f,%.0f,%.2f,%.2f,%.2f,%016llx\n",
           eijen_implementation_name(),
           digest_bits,
           sz->level,
           (unsigned long)sz->len,
           (unsigned long)iters,
           EIJEN_BENCH_REPETITIONS,
           best_ns,
           best_cycles,
           avg_ns,
           avg_cycles,
           mib_s,
           (unsigned long long)best_checksum);
    free(msg);
    return 0;
}

int main(void) {
    int digest_bits[] = {256, 384, 512, 768, 1024};
    size_t i;
    size_t j;

    printf("implementation,digest_bits,level,input_bytes,iterations,repetitions,best_ns,best_cycles,avg_ns,avg_cycles,throughput_mib_s,checksum\n");
    for (i = 0; i < sizeof(digest_bits) / sizeof(digest_bits[0]); i++) {
        for (j = 0; j < sizeof(BENCH_SIZES) / sizeof(BENCH_SIZES[0]); j++) {
            if (run_case(digest_bits[i], &BENCH_SIZES[j]) != 0) {
                fprintf(stderr, "benchmark failed for Eijen-%d %s\n",
                        digest_bits[i], BENCH_SIZES[j].level);
                return EXIT_FAILURE;
            }
        }
    }
    return EXIT_SUCCESS;
}
