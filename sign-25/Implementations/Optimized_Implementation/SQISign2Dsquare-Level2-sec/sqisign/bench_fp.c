#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <x86intrin.h>

#include "fp.h"
#include "fp2.h"

#ifndef BENCH_ITERS
#define BENCH_ITERS 1000000
#endif

static inline uint64_t
bench_cycles(void)
{
    unsigned int aux;
    return __rdtscp(&aux);
}

static void
init_fp(fp_t *x, uint64_t seed)
{
    fp_set_small(x, (digit_t)(seed & 0xffffffffu));
    for (int i = 0; i < 8; i++) {
        fp_add(x, x, x);
        fp_add(x, x, x);
        fp_add(x, x, x);
        fp_add(x, x, x);
        fp_add(x, x, x);
        fp_add(x, x, x);
        fp_add(x, x, x);
        fp_add(x, x, x);
        fp_add(x, x, x);
        fp_add(x, x, x);
        fp_add(x, x, x);
        fp_add(x, x, x);
        fp_add(x, x, x);
        fp_add(x, x, x);
        fp_add(x, x, x);
        fp_add(x, x, x);
        fp_tomont(x, x);
        fp_add(x, x, (const fp_t *)&ONE);
    }
}

static void
init_fp2(fp2_t *x, uint64_t seed)
{
    init_fp(&x->re, seed);
    init_fp(&x->im, seed ^ 0x9e3779b97f4a7c15ULL);
}

static uint64_t
checksum_fp(const fp_t *x)
{
    uint64_t acc = 0;
    for (int i = 0; i < NWORDS_FIELD; i++) {
        acc ^= (*x)[i] + 0x9e3779b97f4a7c15ULL + (acc << 6) + (acc >> 2);
    }
    return acc;
}

static uint64_t
checksum_fp2(const fp2_t *x)
{
    return checksum_fp(&x->re) ^ (checksum_fp(&x->im) << 1);
}

#define RUN_BENCH(name, stmt, checksum_expr)                                      \
    do {                                                                          \
        uint64_t start = bench_cycles();                                          \
        for (int i = 0; i < BENCH_ITERS; i++) {                                   \
            stmt;                                                                 \
        }                                                                         \
        uint64_t end = bench_cycles();                                            \
        printf("%-10s %10.2f cycles  checksum=%016" PRIx64 "\n",                \
               name, (double)(end - start) / (double)BENCH_ITERS, checksum_expr); \
    } while (0)

int
main(void)
{
    fp_t a, b, c;
    fp2_t x, y, z;

    init_fp(&a, 0x12345678u);
    init_fp(&b, 0x87654321u);
    init_fp2(&x, 0x31415926u);
    init_fp2(&y, 0x27182818u);

    RUN_BENCH("fp_add", fp_add(&c, &a, &b); fp_add(&a, &a, &c), checksum_fp(&a));
    RUN_BENCH("fp_sub", fp_sub(&c, &a, &b); fp_add(&a, &a, &c), checksum_fp(&a));
    RUN_BENCH("fp_mul", fp_mul(&c, &a, &b); fp_add(&a, &a, &c), checksum_fp(&a));
    RUN_BENCH("fp_sqr", fp_sqr(&c, &a); fp_add(&a, &a, &c), checksum_fp(&a));
    RUN_BENCH("fp_tomont", fp_tomont(&c, &a); fp_add(&a, &a, &c), checksum_fp(&a));
    RUN_BENCH("fp_frommont", fp_frommont(&c, &a); fp_add(&a, &a, &c), checksum_fp(&a));
    RUN_BENCH("fp2_mul", fp2_mul(&z, &x, &y); fp2_add(&x, &x, &z), checksum_fp2(&x));
    RUN_BENCH("fp2_sqr", fp2_sqr(&z, &x); fp2_add(&x, &x, &z), checksum_fp2(&x));

    return 0;
}
