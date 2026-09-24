#define _POSIX_C_SOURCE 199309L

/*
 * XOF x4 Performance Benchmark for Phoenix-SM3
 * 
 * This benchmark measures the performance of 4-way parallel XOF operations
 * used in thashx4 and prf_addrx4 functions.
 * 
 * Current implementation uses 4x sm3_xof calls (scalar pseudoXOF).
 * SHAKE implementation uses true 4-lane parallel Keccak via AVX2.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

#include "../hash_sm3.h"
#include "../thash.h"
#include "../thashx4.h"
#include "../params.h"
#include "../context.h"
#include "../address.h"
#include "../randombytes.h"
#include "cycles.h"

#define NTESTS 100
#define ITERATIONS_PER_TEST 1000

static void prf_addrx4_wrapper(unsigned char *out0,
                                 unsigned char *out1,
                                 unsigned char *out2,
                                 unsigned char *out3,
                                 const spx_ctx *ctx,
                                 const uint32_t addrx4[4 * 8]);

static int cmp_llu(const void *a, const void *b)
{
    if (*(unsigned long long *)a < *(unsigned long long *)b) return -1;
    if (*(unsigned long long *)a > *(unsigned long long *)b) return 1;
    return 0;
}

static unsigned long long median(unsigned long long *l, size_t llen)
{
    qsort(l, llen, sizeof(unsigned long long), cmp_llu);
    if (llen % 2) return l[llen / 2];
    else return (l[llen / 2 - 1] + l[llen / 2]) / 2;
}

static void delta(unsigned long long *l, size_t llen)
{
    for (unsigned int i = 0; i < llen - 1; i++) {
        l[i] = l[i + 1] - l[i];
    }
}

static void printfcomma(unsigned long long n)
{
    if (n < 1000) {
        printf("%llu", n);
        return;
    }
    printfcomma(n / 1000);
    printf(",%03llu", n % 1000);
}

static void display_result(double result, unsigned long long *l, size_t llen,
                           unsigned long long mul, const char *unit)
{
    unsigned long long med;
    delta(l, llen);
    med = median(l, llen);
    printf("avg: %11.2f %s | median: ", result, unit);
    printfcomma(med);
    printf(" cycles | %llux: ", mul);
    printfcomma(mul * med);
    printf(" cycles\n");
}

#define MEASURE_XOF4(TEXT, MUL, FNCALL, CORR, UNIT)                          \
    do {                                                                      \
        unsigned long long t[NTESTS + 1];                                     \
        struct timespec start, stop;                                          \
        double result;                                                        \
        int i;                                                                \
        printf(TEXT);                                                         \
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);                      \
        for (i = 0; i < NTESTS; i++) {                                       \
            t[i] = cpucycles();                                              \
            FNCALL;                                                           \
        }                                                                     \
        t[NTESTS] = cpucycles();                                              \
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &stop);                       \
        result = ((stop.tv_sec - start.tv_sec) * 1e6 +                        \
                 (stop.tv_nsec - start.tv_nsec) / 1e3) / (double)(CORR);     \
        display_result(result, t, NTESTS, MUL, UNIT);                         \
    } while (0)

/*
 * Wrapper for prf_addrx4 to match the expected signature
 */
static void prf_addrx4_wrapper(unsigned char *out0,
                                 unsigned char *out1,
                                 unsigned char *out2,
                                 unsigned char *out3,
                                 const spx_ctx *ctx,
                                 const uint32_t addrx4[4 * 8])
{
    SPX_VLA(uint8_t, buf0, SPX_SM3_ADDR_BYTES + SPX_N);
    SPX_VLA(uint8_t, buf1, SPX_SM3_ADDR_BYTES + SPX_N);
    SPX_VLA(uint8_t, buf2, SPX_SM3_ADDR_BYTES + SPX_N);
    SPX_VLA(uint8_t, buf3, SPX_SM3_ADDR_BYTES + SPX_N);

    memcpy(buf0, addrx4 + 0 * 8, SPX_SM3_ADDR_BYTES);
    memcpy(buf1, addrx4 + 1 * 8, SPX_SM3_ADDR_BYTES);
    memcpy(buf2, addrx4 + 2 * 8, SPX_SM3_ADDR_BYTES);
    memcpy(buf3, addrx4 + 3 * 8, SPX_SM3_ADDR_BYTES);

    memcpy(buf0 + SPX_SM3_ADDR_BYTES, ctx->sk_seed, SPX_N);
    memcpy(buf1 + SPX_SM3_ADDR_BYTES, ctx->sk_seed, SPX_N);
    memcpy(buf2 + SPX_SM3_ADDR_BYTES, ctx->sk_seed, SPX_N);
    memcpy(buf3 + SPX_SM3_ADDR_BYTES, ctx->sk_seed, SPX_N);

    /* 4x sm3_xof calls - NOT parallelized */
    sm3_xof(out0, SPX_N, buf0, SPX_SM3_ADDR_BYTES + SPX_N);
    sm3_xof(out1, SPX_N, buf1, SPX_SM3_ADDR_BYTES + SPX_N);
    sm3_xof(out2, SPX_N, buf2, SPX_SM3_ADDR_BYTES + SPX_N);
    sm3_xof(out3, SPX_N, buf3, SPX_SM3_ADDR_BYTES + SPX_N);
}

int main(void)
{
    setbuf(stdout, NULL);
    init_cpucycles();

    spx_ctx ctx;
    unsigned char pk[SPX_PK_BYTES];
    unsigned char sk[SPX_SK_BYTES];

    unsigned char out0[SPX_N], out1[SPX_N], out2[SPX_N], out3[SPX_N];
    unsigned char in0[SPX_N], in1[SPX_N], in2[SPX_N], in3[SPX_N];
    unsigned char block0[SPX_N], block1[SPX_N], block2[SPX_N], block3[SPX_N];
    
    uint32_t addrx4[4 * 8];
    unsigned long long t[NTESTS + 1];
    struct timespec start, stop;
    double result;
    int i;

    /* Initialize */
    randombytes(ctx.pub_seed, SPX_N);
    randombytes(ctx.sk_seed, SPX_N);
    randombytes(in0, SPX_N);
    randombytes(in1, SPX_N);
    randombytes(in2, SPX_N);
    randombytes(in3, SPX_N);
    randombytes((unsigned char *)addrx4, sizeof(addrx4));

    printf("=================================================================\n");
    printf("SM3 XOF x4 Performance Benchmark\n");
    printf("=================================================================\n");
    printf("Parameter set: %s\n", PARAMNAME);
    printf("SPX_N = %d bytes\n", SPX_N);
    printf("SM3_DIGEST_BYTES = %d bytes\n", SM3_DIGEST_BYTES);
    printf("SPX_SM3_ADDR_BYTES = %d bytes\n", SPX_SM3_ADDR_BYTES);
    printf("\n");
    printf("Running %d iterations (%d calls per iteration)\n", 
           NTESTS, ITERATIONS_PER_TEST);
    printf("-----------------------------------------------------------------\n\n");

    /* Benchmark 1: Single sm3_xof call */
    printf("1. Single sm3_xof (baseline):\n");
    MEASURE_XOF4("   sm3_xof x1       ", 1,
        do { for (int j = 0; j < ITERATIONS_PER_TEST; j++) \
            sm3_xof(out0, SPX_N, in0, SPX_N); } while (0),
        ITERATIONS_PER_TEST, "us");

    /* Benchmark 2: 4x sm3_xof calls (current x4 implementation) */
    printf("\n2. 4x sm3_xof (current x4 implementation):\n");
    MEASURE_XOF4("   sm3_xof x4 (seq) ", 4,
        do { for (int j = 0; j < ITERATIONS_PER_TEST; j++) { \
            sm3_xof(out0, SPX_N, in0, SPX_N); \
            sm3_xof(out1, SPX_N, in1, SPX_N); \
            sm3_xof(out2, SPX_N, in2, SPX_N); \
            sm3_xof(out3, SPX_N, in3, SPX_N); \
        }} while (0),
        ITERATIONS_PER_TEST, "us");

    /* Benchmark 3: prf_addrx4 (4x sm3_xof) */
    printf("\n3. prf_addrx4 (uses 4x sm3_xof):\n");
    MEASURE_XOF4("   prf_addrx4        ", 4,
        do { for (int j = 0; j < ITERATIONS_PER_TEST; j++) \
            prf_addrx4_wrapper(out0, out1, out2, out3, &ctx, addrx4); \
        } while (0),
        ITERATIONS_PER_TEST, "us");

    /* Benchmark 4: thashx4 (uses 4x sm3_xof) */
    printf("\n4. thashx4 (uses 4x sm3_xof):\n");
    MEASURE_XOF4("   thashx4 (1 block)", 4,
        do { for (int j = 0; j < ITERATIONS_PER_TEST; j++) \
            thashx4(out0, out1, out2, out3, in0, in1, in2, in3, 1, &ctx, addrx4); \
        } while (0),
        ITERATIONS_PER_TEST, "us");

    printf("\n-----------------------------------------------------------------\n");
    printf("Analysis:\n");
    printf("-----------------------------------------------------------------\n");
    printf("Current SM3 XOF x4 implementation: 4x sequential sm3_xof calls\n");
    printf("- Each sm3_xof uses pseudoXOF (bit-level SM3 with counter extension)\n");
    printf("- No true parallelization across the 4 lanes\n");
    printf("- Each call: ~same latency as single sm3_xof\n");
    printf("\n");
    printf("Expected performance gap vs SHAKE:\n");
    printf("- SHAKE256x4 uses KeccakP1600times4_PermuteAll (AVX2 4-lane parallel)\n");
    printf("- True parallel permutation provides ~4x speedup per squeeze\n");
    printf("- SHAKE can absorb and squeeze in larger chunks\n");
    printf("\n");
    printf("Note: For accurate comparison, run SHAKE benchmark and compare.\n");
    printf("=================================================================\n");

    return 0;
}