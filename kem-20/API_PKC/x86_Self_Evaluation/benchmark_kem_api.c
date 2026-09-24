#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "KEM_AlgorithmInstance.h"
#include "drng.h"

#if defined(__x86_64__) || defined(__i386__) || defined(_M_X64) || defined(_M_IX86)
#include <x86intrin.h>
#endif

DRNG_ctx drng_algorithm;

static unsigned long long read_cycles(void)
{
#if defined(__x86_64__) || defined(_M_X64)
    unsigned int aux;
    _mm_lfence();
    return __rdtscp(&aux);
#elif defined(__i386__) || defined(_M_IX86)
    _mm_lfence();
    return __rdtsc();
#else
    return 0ULL;
#endif
}

static long long now_ns(void)
{
#if defined(CLOCK_MONOTONIC_RAW)
    const clockid_t clock_id = CLOCK_MONOTONIC_RAW;
#else
    const clockid_t clock_id = CLOCK_MONOTONIC;
#endif
    struct timespec ts;
    if (clock_gettime(clock_id, &ts) != 0) {
        return 0;
    }
    return (long long)ts.tv_sec * 1000000000LL + (long long)ts.tv_nsec;
}

static int seed_drng(void)
{
    unsigned char seed[64];
    size_t i;
    for (i = 0; i < sizeof(seed); ++i) {
        seed[i] = (unsigned char)(0x5au ^ (unsigned int)(i * 37u));
    }
    return init_random_number(&drng_algorithm, seed, sizeof(seed));
}

static int cmp_ull(const void *a, const void *b)
{
    const unsigned long long av = *(const unsigned long long *)a;
    const unsigned long long bv = *(const unsigned long long *)b;
    return (av > bv) - (av < bv);
}

static int cmp_ll(const void *a, const void *b)
{
    const long long av = *(const long long *)a;
    const long long bv = *(const long long *)b;
    return (av > bv) - (av < bv);
}

static double mean_ull(const unsigned long long *v, unsigned long n)
{
    long double sum = 0.0;
    unsigned long i;
    for (i = 0; i < n; ++i) sum += (long double)v[i];
    return (double)(sum / (long double)n);
}

static double mean_ll(const long long *v, unsigned long n)
{
    long double sum = 0.0;
    unsigned long i;
    for (i = 0; i < n; ++i) sum += (long double)v[i];
    return (double)(sum / (long double)n);
}

static double stddev_ull(const unsigned long long *v, unsigned long n, double mean)
{
    long double sum = 0.0;
    unsigned long i;
    if (n < 2UL) return 0.0;
    for (i = 0; i < n; ++i) {
        long double d = (long double)v[i] - (long double)mean;
        sum += d * d;
    }
    return sqrt((double)(sum / (long double)(n - 1UL)));
}

static unsigned long parse_level(const char *instance)
{
    const char *p = instance + strlen(instance);
    while (p > instance && p[-1] >= '0' && p[-1] <= '9') --p;
    return strtoul(p, NULL, 10);
}

static void csv_timestamp(char *buf, size_t len)
{
    time_t t = time(NULL);
    struct tm tmv;
    if (gmtime_r(&t, &tmv) == NULL) {
        snprintf(buf, len, "unknown");
        return;
    }
    strftime(buf, len, "%Y-%m-%dT%H:%M:%SZ", &tmv);
}

static void print_stats(const char *implementation, const char *operation,
                        unsigned long warmup, unsigned long runs,
                        const unsigned long long *cycles_in,
                        const long long *ns_in,
                        const char *status, const char *notes)
{
    unsigned long long *cycles = NULL;
    long long *ns = NULL;
    double mean_cycles, mean_ns, stddev_cycles, ops_per_sec;
    char ts[32];
    unsigned long level;

    cycles = (unsigned long long *)malloc(runs * sizeof(*cycles));
    ns = (long long *)malloc(runs * sizeof(*ns));
    if (cycles == NULL || ns == NULL) {
        free(cycles);
        free(ns);
        return;
    }
    memcpy(cycles, cycles_in, runs * sizeof(*cycles));
    memcpy(ns, ns_in, runs * sizeof(*ns));
    qsort(cycles, runs, sizeof(*cycles), cmp_ull);
    qsort(ns, runs, sizeof(*ns), cmp_ll);
    mean_cycles = mean_ull(cycles_in, runs);
    mean_ns = mean_ll(ns_in, runs);
    stddev_cycles = stddev_ull(cycles_in, runs, mean_cycles);
    ops_per_sec = mean_ns > 0.0 ? 1000000000.0 / mean_ns : 0.0;
    csv_timestamp(ts, sizeof(ts));
    level = parse_level(ALGORITHM_INSTANCE);

    printf("%s,%s,%s,%lu,%s,%lu,%lu,%.2f,%llu,%llu,%llu,%.2f,%.2f,%lld,%.2f,%llu,%llu,%llu,%llu,%s,%s\n",
           ts, implementation, ALGORITHM_INSTANCE, level, operation, warmup, runs,
           mean_cycles, cycles[runs / 2UL], cycles[0], cycles[runs - 1UL], stddev_cycles,
           mean_ns, ns[runs / 2UL], ops_per_sec,
           kem_get_pk_len_bytes(), kem_get_sk_len_bytes(), kem_get_ct_len_bytes(), kem_get_ss_len_bytes(),
           status, notes);
    free(cycles);
    free(ns);
}

static int run_keygen(unsigned char *pk, unsigned long long *pk_len,
                      unsigned char *sk, unsigned long long *sk_len)
{
    return kem_keygen(pk, pk_len, sk, sk_len);
}

static int run_enc(unsigned char *pk, unsigned long long pk_len,
                   unsigned char *ss, unsigned long long *ss_len,
                   unsigned char *ct, unsigned long long *ct_len)
{
    return kem_enc(pk, pk_len, ss, ss_len, ct, ct_len);
}

static int run_dec(unsigned char *sk, unsigned long long sk_len,
                   unsigned char *ct, unsigned long long ct_len,
                   unsigned char *ss, unsigned long long *ss_len)
{
    return kem_dec(sk, sk_len, ct, ct_len, ss, ss_len);
}

int main(int argc, char **argv)
{
    const char *implementation;
    unsigned long runs;
    unsigned long warmup;
    unsigned char *pk = NULL, *sk = NULL, *ct = NULL, *ss = NULL, *ss_dec = NULL;
    unsigned long long pk_len = 0, sk_len = 0, ct_len = 0, ss_len = 0, ss_dec_len = 0;
    unsigned long long *cycles = NULL;
    long long *nsecs = NULL;
    unsigned long i;
    int rc = EXIT_FAILURE;

    if (argc != 4) {
        fprintf(stderr, "Usage: %s <implementation> <runs> <warmup>\n", argv[0]);
        return EXIT_FAILURE;
    }
    implementation = argv[1];
    runs = strtoul(argv[2], NULL, 10);
    warmup = strtoul(argv[3], NULL, 10);
    if (runs == 0UL) {
        fprintf(stderr, "runs must be positive\n");
        return EXIT_FAILURE;
    }
    if (seed_drng() != 0) {
        fprintf(stderr, "DRNG init failed\n");
        return EXIT_FAILURE;
    }

    pk = (unsigned char *)malloc((size_t)kem_get_pk_len_bytes());
    sk = (unsigned char *)malloc((size_t)kem_get_sk_len_bytes());
    ct = (unsigned char *)malloc((size_t)kem_get_ct_len_bytes());
    ss = (unsigned char *)malloc((size_t)kem_get_ss_len_bytes());
    ss_dec = (unsigned char *)malloc((size_t)kem_get_ss_len_bytes());
    cycles = (unsigned long long *)malloc(runs * sizeof(*cycles));
    nsecs = (long long *)malloc(runs * sizeof(*nsecs));
    if (pk == NULL || sk == NULL || ct == NULL || ss == NULL || ss_dec == NULL || cycles == NULL || nsecs == NULL) {
        fprintf(stderr, "allocation failed\n");
        goto cleanup;
    }

    for (i = 0; i < warmup; ++i) {
        if (run_keygen(pk, &pk_len, sk, &sk_len) != 0) goto cleanup;
    }
    for (i = 0; i < runs; ++i) {
        unsigned long long c0 = read_cycles();
        long long t0 = now_ns();
        if (run_keygen(pk, &pk_len, sk, &sk_len) != 0) goto cleanup;
        nsecs[i] = now_ns() - t0;
        cycles[i] = read_cycles() - c0;
    }
    print_stats(implementation, "kem_keygen", warmup, runs, cycles, nsecs, "PASS", "");

    if (run_keygen(pk, &pk_len, sk, &sk_len) != 0) goto cleanup;
    for (i = 0; i < warmup; ++i) {
        if (run_enc(pk, pk_len, ss, &ss_len, ct, &ct_len) != 0) goto cleanup;
    }
    for (i = 0; i < runs; ++i) {
        unsigned long long c0 = read_cycles();
        long long t0 = now_ns();
        if (run_enc(pk, pk_len, ss, &ss_len, ct, &ct_len) != 0) goto cleanup;
        nsecs[i] = now_ns() - t0;
        cycles[i] = read_cycles() - c0;
    }
    print_stats(implementation, "kem_enc", warmup, runs, cycles, nsecs, "PASS", "");

    if (run_enc(pk, pk_len, ss, &ss_len, ct, &ct_len) != 0) goto cleanup;
    for (i = 0; i < warmup; ++i) {
        if (run_dec(sk, sk_len, ct, ct_len, ss_dec, &ss_dec_len) != 0) goto cleanup;
        if (ss_dec_len != ss_len || memcmp(ss, ss_dec, (size_t)ss_len) != 0) goto cleanup;
    }
    for (i = 0; i < runs; ++i) {
        unsigned long long c0;
        long long t0;
        if (run_enc(pk, pk_len, ss, &ss_len, ct, &ct_len) != 0) goto cleanup;
        c0 = read_cycles();
        t0 = now_ns();
        if (run_dec(sk, sk_len, ct, ct_len, ss_dec, &ss_dec_len) != 0) goto cleanup;
        nsecs[i] = now_ns() - t0;
        cycles[i] = read_cycles() - c0;
        if (ss_dec_len != ss_len || memcmp(ss, ss_dec, (size_t)ss_len) != 0) goto cleanup;
    }
    if (ct_len > 0ULL) {
        ct[0] ^= 0x01u;
        ss_dec_len = 0;
        if (run_dec(sk, sk_len, ct, ct_len, ss_dec, &ss_dec_len) != 0 || ss_dec_len != kem_get_ss_len_bytes()) {
            fprintf(stderr, "implicit rejection valid-length invalid-ciphertext check failed\n");
            goto cleanup;
        }
    }
    print_stats(implementation, "kem_dec", warmup, runs, cycles, nsecs, "PASS", "valid_length_invalid_ct_returns_success_implicit_rejection");
    rc = EXIT_SUCCESS;

cleanup:
    free(nsecs);
    free(cycles);
    free(ss_dec);
    free(ss);
    free(ct);
    free(sk);
    free(pk);
    return rc;
}
