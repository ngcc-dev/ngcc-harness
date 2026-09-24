#define _POSIX_C_SOURCE 200809L

#include "KEM_AlgorithmInstance.h"

#include <errno.h>
#include <float.h>
#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__APPLE__)
#include <mach/mach_time.h>
#else
#include <time.h>
#endif

#if defined(__x86_64__) || defined(__i386__)
#include <x86intrin.h>
#define SCLOUDPLUS_BENCH_HAS_CYCLES 1
static uint64_t bench_cycles(void)
{
    uint64_t cycles;

#if defined(__SSE2__)
    _mm_lfence();
#endif
    cycles = (uint64_t)__rdtsc();
#if defined(__SSE2__)
    _mm_lfence();
#endif
    return cycles;
}
#else
#define SCLOUDPLUS_BENCH_HAS_CYCLES 0
static uint64_t bench_cycles(void)
{
    return 0;
}
#endif

typedef struct
{
    unsigned char *pk;
    unsigned char *sk;
    unsigned char *ct;
    unsigned char *ss;
    unsigned char *ss_dec;
    unsigned long long pk_len;
    unsigned long long sk_len;
    unsigned long long ct_len;
    unsigned long long ss_len;
} bench_ctx;

typedef int (*bench_op)(bench_ctx *ctx);

typedef struct
{
    uint64_t iterations;
    double elapsed_us;
    uint64_t cycles;
} bench_sample;

static double bench_now_us(void)
{
#if defined(__APPLE__)
    static mach_timebase_info_data_t timebase = {0, 0};
    const uint64_t now = mach_absolute_time();

    if (timebase.denom == 0)
    {
        if (mach_timebase_info(&timebase) != 0 || timebase.denom == 0)
        {
            fprintf(stderr, "mach_timebase_info failed\n");
            exit(2);
        }
    }
    return (double)now * (double)timebase.numer /
           (double)timebase.denom / 1000.0;
#else
    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
    {
        perror("clock_gettime");
        exit(2);
    }
    return (double)ts.tv_sec * 1000000.0 + (double)ts.tv_nsec / 1000.0;
#endif
}

static void *checked_malloc(unsigned long long len, const char *name)
{
    void *p = malloc((size_t)len);

    if (p == NULL)
    {
        fprintf(stderr, "allocation failed for %s (%llu bytes)\n", name, len);
        exit(2);
    }
    return p;
}

static int same_bytes(const unsigned char *a, const unsigned char *b,
                      unsigned long long len)
{
    unsigned char diff = 0;

    for (unsigned long long i = 0; i < len; i++)
    {
        diff |= (unsigned char)(a[i] ^ b[i]);
    }
    return diff == 0;
}

static int op_keygen(bench_ctx *ctx)
{
    unsigned long long pk_len = 0;
    unsigned long long sk_len = 0;

    if (kem_keygen(ctx->pk, &pk_len, ctx->sk, &sk_len) != 0)
    {
        return -1;
    }
    if (pk_len != ctx->pk_len || sk_len != ctx->sk_len)
    {
        return -1;
    }
    return 0;
}

static int op_encaps(bench_ctx *ctx)
{
    unsigned long long ss_len = 0;
    unsigned long long ct_len = 0;

    if (kem_enc(ctx->pk, ctx->pk_len, ctx->ss, &ss_len, ctx->ct, &ct_len) != 0)
    {
        return -1;
    }
    if (ss_len != ctx->ss_len || ct_len != ctx->ct_len)
    {
        return -1;
    }
    return 0;
}

static int op_decaps(bench_ctx *ctx)
{
    unsigned long long ss_len = 0;

    if (kem_dec(ctx->sk, ctx->sk_len, ctx->ct, ctx->ct_len, ctx->ss_dec,
                &ss_len) != 0)
    {
        return -1;
    }
    if (ss_len != ctx->ss_len)
    {
        return -1;
    }
    return 0;
}

static int op_encaps_decaps(bench_ctx *ctx)
{
    if (op_encaps(ctx) != 0 || op_decaps(ctx) != 0)
    {
        return -1;
    }
    if (!same_bytes(ctx->ss, ctx->ss_dec, ctx->ss_len))
    {
        return -1;
    }
    return 0;
}

static int bench_prepare(bench_ctx *ctx)
{
    ctx->pk_len = kem_get_pk_len_bytes();
    ctx->sk_len = kem_get_sk_len_bytes();
    ctx->ct_len = kem_get_ct_len_bytes();
    ctx->ss_len = kem_get_ss_len_bytes();
    if (ctx->pk_len == 0 || ctx->sk_len == 0 || ctx->ct_len == 0 ||
        ctx->ss_len == 0)
    {
        fprintf(stderr, "invalid zero-length KEM parameter\n");
        return -1;
    }
    ctx->pk = checked_malloc(ctx->pk_len, "pk");
    ctx->sk = checked_malloc(ctx->sk_len, "sk");
    ctx->ct = checked_malloc(ctx->ct_len, "ct");
    ctx->ss = checked_malloc(ctx->ss_len, "ss");
    ctx->ss_dec = checked_malloc(ctx->ss_len, "ss_dec");

    if (op_keygen(ctx) != 0 || op_encaps(ctx) != 0 ||
        op_decaps(ctx) != 0 || !same_bytes(ctx->ss, ctx->ss_dec, ctx->ss_len))
    {
        fprintf(stderr, "initial KEM self-check failed\n");
        return -1;
    }
    return 0;
}

static void bench_free(bench_ctx *ctx)
{
    free(ctx->pk);
    free(ctx->sk);
    free(ctx->ct);
    free(ctx->ss);
    free(ctx->ss_dec);
}

static int bench_warmup(bench_ctx *ctx, bench_op op)
{
    for (unsigned i = 0; i < 8; i++)
    {
        if (op(ctx) != 0)
        {
            return -1;
        }
    }
    return 0;
}

static int bench_run_once(bench_ctx *ctx, bench_op op, double target_seconds,
                          bench_sample *sample)
{
    const double target_us = target_seconds * 1000000.0;
    const double start_us = bench_now_us();
    const uint64_t start_cycles = bench_cycles();
    uint64_t iterations = 0;

    do
    {
        if (op(ctx) != 0)
        {
            return -1;
        }
        iterations++;
    } while ((bench_now_us() - start_us) < target_us);

    sample->elapsed_us = bench_now_us() - start_us;
    sample->cycles = bench_cycles() - start_cycles;
    sample->iterations = iterations;
    return 0;
}

static int bench_operation(bench_ctx *ctx, const char *name, bench_op op,
                           double target_seconds, unsigned repetitions)
{
    uint64_t total_iterations = 0;
    double total_us = 0.0;
    uint64_t total_cycles = 0;

    if (bench_warmup(ctx, op) != 0)
    {
        fprintf(stderr, "%s warmup failed\n", name);
        return -1;
    }

    for (unsigned rep = 0; rep < repetitions; rep++)
    {
        bench_sample sample;

        if (bench_run_once(ctx, op, target_seconds, &sample) != 0)
        {
            fprintf(stderr, "%s benchmark failed\n", name);
            return -1;
        }
        total_iterations += sample.iterations;
        total_us += sample.elapsed_us;
        total_cycles += sample.cycles;
    }

    printf("%s,%u,%" PRIu64 ",%.3f,", name, repetitions, total_iterations,
           total_us / (double)total_iterations);
#if SCLOUDPLUS_BENCH_HAS_CYCLES
    printf("%.3f\n", (double)total_cycles / (double)total_iterations);
#else
    printf("n/a\n");
#endif
    return 0;
}

static int parse_args(int argc, char **argv, double *target_seconds,
                      unsigned *repetitions)
{
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--seconds") == 0 && i + 1 < argc)
        {
            char *endptr = NULL;

            errno = 0;
            *target_seconds = strtod(argv[++i], &endptr);
            if (errno != 0 || endptr == argv[i] || *endptr != '\0')
            {
                fprintf(stderr, "invalid seconds value: %s\n", argv[i]);
                return -1;
            }
        }
        else if (strcmp(argv[i], "--reps") == 0 && i + 1 < argc)
        {
            char *endptr = NULL;
            unsigned long parsed;

            errno = 0;
            parsed = strtoul(argv[++i], &endptr, 10);
            if (errno != 0 || endptr == argv[i] || *endptr != '\0' ||
                parsed > (unsigned long)UINT_MAX)
            {
                fprintf(stderr, "invalid reps value: %s\n", argv[i]);
                return -1;
            }
            *repetitions = (unsigned)parsed;
        }
        else if (strcmp(argv[i], "--help") == 0)
        {
            printf("Usage: %s [--seconds N] [--reps N]\n", argv[0]);
            return 1;
        }
        else
        {
            fprintf(stderr, "unknown option: %s\n", argv[i]);
            return -1;
        }
    }
    if (!(*target_seconds > 0.0) ||
        *target_seconds > DBL_MAX / 1000000.0 ||
        *repetitions == 0)
    {
        fprintf(stderr, "seconds must be a positive finite value and reps must be positive\n");
        return -1;
    }
    return 0;
}

int main(int argc, char **argv)
{
    double target_seconds = 1.0;
    unsigned repetitions = 5;
    bench_ctx ctx;
    int parsed;
    int ok = 1;

    parsed = parse_args(argc, argv, &target_seconds, &repetitions);
    if (parsed != 0)
    {
        return parsed > 0 ? 0 : 2;
    }

    memset(&ctx, 0, sizeof(ctx));
    if (bench_prepare(&ctx) != 0)
    {
        bench_free(&ctx);
        return 1;
    }

    printf("Algorithm: %s\n", ALGORITHM_INSTANCE);
    printf("pk_bytes: %llu\n", ctx.pk_len);
    printf("sk_bytes: %llu\n", ctx.sk_len);
    printf("ct_bytes: %llu\n", ctx.ct_len);
    printf("ss_bytes: %llu\n", ctx.ss_len);
    printf("seconds_per_repetition: %.3f\n", target_seconds);
    printf("operation,repetitions,total_iterations,avg_time_us,avg_cycles\n");

    ok &= bench_operation(&ctx, "Key generation", op_keygen, target_seconds,
                          repetitions) == 0;
    ok &= bench_operation(&ctx, "KEM encapsulate", op_encaps, target_seconds,
                          repetitions) == 0;
    ok &= bench_operation(&ctx, "KEM decapsulate", op_decaps, target_seconds,
                          repetitions) == 0;
    ok &= bench_operation(&ctx, "KEM enc and decapsulate", op_encaps_decaps,
                          target_seconds, repetitions) == 0;

    bench_free(&ctx);
    return ok ? 0 : 1;
}
