#define _GNU_SOURCE

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "drng.h"
#include "randomness.h"
#include "SIG_AlgorithmInstance.h"

#define WARMUP_ROUNDS 50
#define MEASURE_ROUNDS 200

DRNG_ctx drng_algorithm;

static int init_bench_drng(void)
{
    unsigned char seed[64];

    if (rand_bytes(seed, sizeof(seed)) != 0)
        return -1;
    return init_random_number(&drng_algorithm, seed, sizeof(seed));
}

static inline uint64_t cpucycles_start(void)
{
    unsigned hi, lo;
    __asm__ __volatile__("CPUID\n\t"
                         "RDTSC\n\t"
                         "mov %%edx, %0\n\t"
                         "mov %%eax, %1\n\t"
                         : "=r"(hi), "=r"(lo)
                         :
                         : "%rax", "%rbx", "%rcx", "%rdx");
    return ((uint64_t)lo) ^ (((uint64_t)hi) << 32);
}

static inline uint64_t cpucycles_stop(void)
{
    unsigned hi, lo;
    __asm__ __volatile__("RDTSCP\n\t"
                         "mov %%edx, %0\n\t"
                         "mov %%eax, %1\n\t"
                         "CPUID\n\t"
                         : "=r"(hi), "=r"(lo)
                         :
                         : "%rax", "%rbx", "%rcx", "%rdx");
    return ((uint64_t)lo) ^ (((uint64_t)hi) << 32);
}

static inline uint64_t now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

int main(void)
{
    const unsigned char message[] = "SYndrome DecOding (SYDO) signature scheme";
    const unsigned long long message_len = sizeof(message) - 1;
    unsigned long long pk_len = sig_get_pk_len_bytes();
    unsigned long long sk_len = sig_get_sk_len_bytes();
    unsigned long long sig_len = sig_get_sn_len_bytes();

    unsigned char* pk = calloc(pk_len, 1);
    unsigned char* sk = calloc(sk_len, 1);
    unsigned char* sig = calloc(sig_len, 1);
    if (!pk || !sk || !sig) {
        fprintf(stderr, "allocation failed\n");
        free(pk);
        free(sk);
        free(sig);
        return 1;
    }

    uint64_t timer;
    uint64_t t1;
    uint64_t t2;
    uint64_t elapsed_ns = 0;
    uint64_t keygen_cycles = 0;
    double keygen_us = 0.0;
    uint64_t sign_cycles = 0;
    double sign_us = 0.0;
    uint64_t verify_cycles = 0;
    double verify_us = 0.0;
    int failures = 0;

    if (init_bench_drng() != 0) {
        fprintf(stderr, "drng initialization failed\n");
        free(pk);
        free(sk);
        free(sig);
        return 1;
    }

    for (size_t i = 0; i < WARMUP_ROUNDS; ++i) {
        unsigned long long cur_pk_len = pk_len;
        unsigned long long cur_sk_len = sk_len;
        if (sig_keygen(pk, &cur_pk_len, sk, &cur_sk_len) != 0) {
            fprintf(stderr, "sig_keygen failed during warmup\n");
            free(pk);
            free(sk);
            free(sig);
            return 1;
        }
    }

    timer = 0;
    elapsed_ns = 0;
    for (size_t i = 0; i < MEASURE_ROUNDS; ++i) {
        unsigned long long cur_pk_len = pk_len;
        unsigned long long cur_sk_len = sk_len;
        t1 = cpucycles_start();
        uint64_t n1 = now_ns();
        if (sig_keygen(pk, &cur_pk_len, sk, &cur_sk_len) != 0) {
            fprintf(stderr, "sig_keygen failed\n");
            free(pk);
            free(sk);
            free(sig);
            return 1;
        }
        uint64_t n2 = now_ns();
        t2 = cpucycles_stop();
        timer += t2 - t1;
        elapsed_ns += n2 - n1;
    }
    keygen_cycles = timer / MEASURE_ROUNDS;
    keygen_us = (double)elapsed_ns / (1000.0 * (double)MEASURE_ROUNDS);

    {
        unsigned long long cur_pk_len = pk_len;
        unsigned long long cur_sk_len = sk_len;
        if (sig_keygen(pk, &cur_pk_len, sk, &cur_sk_len) != 0) {
            fprintf(stderr, "sig_keygen failed before sign benchmark\n");
            free(pk);
            free(sk);
            free(sig);
            return 1;
        }
    }
    for (size_t i = 0; i < WARMUP_ROUNDS; ++i) {
        unsigned long long cur_sig_len = sig_len;
        if (sig_sign(sk, sk_len, (unsigned char*)message, message_len, sig, &cur_sig_len) != 0) {
            fprintf(stderr, "sig_sign failed during warmup\n");
            free(pk);
            free(sk);
            free(sig);
            return 1;
        }
    }
    timer = 0;
    elapsed_ns = 0;
    for (size_t i = 0; i < MEASURE_ROUNDS; ++i) {
        unsigned long long cur_sig_len = sig_len;
        t1 = cpucycles_start();
        uint64_t n1 = now_ns();
        if (sig_sign(sk, sk_len, (unsigned char*)message, message_len, sig, &cur_sig_len) != 0) {
            fprintf(stderr, "sig_sign failed\n");
            free(pk);
            free(sk);
            free(sig);
            return 1;
        }
        uint64_t n2 = now_ns();
        t2 = cpucycles_stop();
        timer += t2 - t1;
        elapsed_ns += n2 - n1;
    }
    sign_cycles = timer / MEASURE_ROUNDS;
    sign_us = (double)elapsed_ns / (1000.0 * (double)MEASURE_ROUNDS);

    {
        unsigned long long cur_pk_len = pk_len;
        unsigned long long cur_sk_len = sk_len;
        unsigned long long cur_sig_len = sig_len;
        if (sig_keygen(pk, &cur_pk_len, sk, &cur_sk_len) != 0) {
            fprintf(stderr, "sig_keygen failed before verify benchmark\n");
            free(pk);
            free(sk);
            free(sig);
            return 1;
        }
        if (sig_sign(sk, sk_len, (unsigned char*)message, message_len, sig, &cur_sig_len) != 0) {
            fprintf(stderr, "sig_sign failed before verify benchmark\n");
            free(pk);
            free(sk);
            free(sig);
            return 1;
        }
        for (size_t i = 0; i < WARMUP_ROUNDS; ++i) {
            if (sig_verify(pk, pk_len, sig, cur_sig_len, (unsigned char*)message, message_len) != 0) {
                failures++;
            }
        }
        timer = 0;
        elapsed_ns = 0;
        for (size_t i = 0; i < MEASURE_ROUNDS; ++i) {
            t1 = cpucycles_start();
            uint64_t n1 = now_ns();
            if (sig_verify(pk, pk_len, sig, cur_sig_len, (unsigned char*)message, message_len) != 0) {
                failures++;
            }
            uint64_t n2 = now_ns();
            t2 = cpucycles_stop();
            timer += t2 - t1;
            elapsed_ns += n2 - n1;
        }
    }
    verify_cycles = timer / MEASURE_ROUNDS;
    verify_us = (double)elapsed_ns / (1000.0 * (double)MEASURE_ROUNDS);

    printf("\nOptimized Implementation - %s\n", "SYDO 512 short");
    printf("measure rounds: %d\n", MEASURE_ROUNDS);
    printf("failures      : %d\n\n", failures);

    printf("Sizes (bytes)\n");
    printf("+------------+------------+\n");
    printf("| Object     |      Bytes |\n");
    printf("+------------+------------+\n");
    printf("| %-10s | %10llu |\n", "Public key", (unsigned long long)pk_len);
    printf("| %-10s | %10llu |\n", "Secret key", (unsigned long long)sk_len);
    printf("| %-10s | %10llu |\n", "Signature", (unsigned long long)sig_len);
    printf("+------------+------------+\n\n");

    printf("Performance\n");
    printf("+-----------+------------+--------------+------------+\n");
    printf("| Operation |  Time (us) |       Cycles |    Mcycles |\n");
    printf("+-----------+------------+--------------+------------+\n");
    printf("| %-9s | %10.2f | %12llu | %10.3f |\n", "Keygen", keygen_us,
           (unsigned long long)keygen_cycles, (double)keygen_cycles / 1000000.0);
    printf("| %-9s | %10.2f | %12llu | %10.3f |\n", "Sign", sign_us,
           (unsigned long long)sign_cycles, (double)sign_cycles / 1000000.0);
    printf("| %-9s | %10.2f | %12llu | %10.3f |\n", "Verify", verify_us,
           (unsigned long long)verify_cycles, (double)verify_cycles / 1000000.0);
    printf("+-----------+------------+--------------+------------+\n\n");

    free(pk);
    free(sk);
    free(sig);
    return failures == 0 ? 0 : 1;
}
