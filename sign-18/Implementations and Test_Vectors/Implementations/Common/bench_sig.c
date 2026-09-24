#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "SIG_AlgorithmInstance.h"
#include "drng.h"

#define BENCH_SEED_LEN_BYTES 64ULL
#define DEFAULT_ITERATIONS 3ULL
#define DEFAULT_MESSAGE_BYTES 64ULL
#define MAX_SIGN_ATTEMPTS 10000ULL

#define STRINGIFY_INNER(x) #x
#define STRINGIFY(x) STRINGIFY_INNER(x)

DRNG_ctx drng_algorithm;

static volatile unsigned long long bench_sink = 0;

static double now_seconds(void)
{
#if defined(_WIN32)
    LARGE_INTEGER frequency;
    LARGE_INTEGER counter;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart / (double)frequency.QuadPart;
#else
    struct timespec timestamp;
    if (clock_gettime(CLOCK_MONOTONIC, &timestamp) == 0) {
        return (double)timestamp.tv_sec + ((double)timestamp.tv_nsec / 1000000000.0);
    }
    return (double)clock() / (double)CLOCKS_PER_SEC;
#endif
}

static unsigned long long parse_positive_ull(const char *text, const char *name)
{
    char *end = NULL;
    unsigned long long value;

    errno = 0;
    value = strtoull(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' || value == 0ULL) {
        fprintf(stderr, "ERROR: invalid %s value: %s\n", name, text);
        exit(EXIT_FAILURE);
    }
    return value;
}

static void *checked_calloc(unsigned long long bytes, const char *name)
{
    void *ptr;

    if (bytes > (unsigned long long)SIZE_MAX) {
        fprintf(stderr, "ERROR: allocation too large for %s\n", name);
        exit(EXIT_FAILURE);
    }

    ptr = calloc((size_t)bytes, 1U);
    if (ptr == NULL) {
        fprintf(stderr, "ERROR: allocation failed for %s\n", name);
        exit(EXIT_FAILURE);
    }
    return ptr;
}

static void fill_seed(unsigned char seed[BENCH_SEED_LEN_BYTES], const char *label)
{
    size_t label_len = strlen(label);
    unsigned long long index;

    for (index = 0; index < BENCH_SEED_LEN_BYTES; index++) {
        unsigned int base = (unsigned int)(unsigned char)label[index % label_len];
        seed[index] = (unsigned char)((base + (unsigned int)(13ULL * index)) & 0xffU);
    }
}

static void reset_algorithm_drng(const char *label)
{
    unsigned char seed[BENCH_SEED_LEN_BYTES];

    fill_seed(seed, label);
    if (init_random_number(&drng_algorithm, seed, BENCH_SEED_LEN_BYTES) != 0) {
        fprintf(stderr, "ERROR: init_random_number failed for %s\n", label);
        exit(EXIT_FAILURE);
    }
}

static void fill_message(unsigned char *message, unsigned long long message_len)
{
    unsigned long long index;

    for (index = 0; index < message_len; index++) {
        message[index] = (unsigned char)((0x53U + (unsigned int)(index * 17ULL)) & 0xffU);
    }
}

static void require_success(int result, const char *operation)
{
    if (result != 0) {
        fprintf(stderr, "ERROR: %s returned %d\n", operation, result);
        exit(EXIT_FAILURE);
    }
}

static int sign_with_retry(unsigned char *sk, unsigned long long sk_len,
                           unsigned char *message, unsigned long long message_len,
                           unsigned char *signature, unsigned long long *sig_len,
                           unsigned long long *attempts_used)
{
    unsigned long long attempt;
    int result = -4;

    for (attempt = 1ULL; attempt <= MAX_SIGN_ATTEMPTS; attempt++) {
        *sig_len = sig_get_sn_len_bytes();
        result = sig_sign(sk, sk_len, message, message_len, signature, sig_len);
        if (result == 0) {
            *attempts_used = attempt;
            return 0;
        }
        if (result != -4) {
            *attempts_used = attempt;
            return result;
        }
    }

    *attempts_used = MAX_SIGN_ATTEMPTS;
    return result;
}

static void print_result(const char *operation, double total_seconds,
                         unsigned long long iterations)
{
    double average_us = (total_seconds * 1000000.0) / (double)iterations;
    printf("%s,%.9f,%.3f\n", operation, total_seconds, average_us);
}

int main(int argc, char **argv)
{
    unsigned long long iterations = DEFAULT_ITERATIONS;
    unsigned long long message_len = DEFAULT_MESSAGE_BYTES;
    unsigned long long pk_len = sig_get_pk_len_bytes();
    unsigned long long sk_len = sig_get_sk_len_bytes();
    unsigned long long sig_len = sig_get_sn_len_bytes();
    unsigned char *pk;
    unsigned char *sk;
    unsigned char *signature;
    unsigned char *message;
    double start_time;
    double elapsed;
    unsigned long long index;
    unsigned long long attempts_used = 0ULL;
    unsigned long long sign_attempts_total = 0ULL;

    if (argc > 1) {
        iterations = parse_positive_ull(argv[1], "iterations");
    }
    if (argc > 2) {
        message_len = parse_positive_ull(argv[2], "message length");
    }

    pk = checked_calloc(pk_len, "public key");
    sk = checked_calloc(sk_len, "secret key");
    signature = checked_calloc(sig_len, "signature");
    message = checked_calloc(message_len, "message");
    fill_message(message, message_len);

    reset_algorithm_drng("origami-bench-setup");
    require_success(sig_keygen(pk, &pk_len, sk, &sk_len), "sig_keygen setup");
    require_success(sign_with_retry(sk, sk_len, message, message_len, signature,
                                    &sig_len, &attempts_used),
                    "sig_sign setup");
    require_success(sig_verify(pk, pk_len, signature, sig_len, message, message_len),
                    "sig_verify setup");

    printf("Algorithm instance: %s\n", ALGORITHM_INSTANCE);
    printf("Implementation profile: %s\n", STRINGIFY(ORIGAMI_OPT));
    printf("Iterations: %llu\n", iterations);
    printf("Message bytes: %llu\n", message_len);
    printf("Public key bytes: %llu\n", sig_get_pk_len_bytes());
    printf("Secret key bytes: %llu\n", sig_get_sk_len_bytes());
    printf("Signature bytes: %llu\n", sig_get_sn_len_bytes());
    printf("Sign retry limit: %llu\n", MAX_SIGN_ATTEMPTS);
    printf("operation,total_seconds,average_microseconds\n");

    reset_algorithm_drng("origami-bench-keygen");
    start_time = now_seconds();
    for (index = 0; index < iterations; index++) {
        pk_len = sig_get_pk_len_bytes();
        sk_len = sig_get_sk_len_bytes();
        require_success(sig_keygen(pk, &pk_len, sk, &sk_len), "sig_keygen");
        bench_sink += (unsigned long long)pk[0] + (unsigned long long)sk[0];
    }
    elapsed = now_seconds() - start_time;
    print_result("keygen", elapsed, iterations);

    reset_algorithm_drng("origami-bench-sign");
    start_time = now_seconds();
    for (index = 0; index < iterations; index++) {
        require_success(sign_with_retry(sk, sk_len, message, message_len, signature,
                                        &sig_len, &attempts_used),
                        "sig_sign");
        sign_attempts_total += attempts_used;
        bench_sink += (unsigned long long)signature[0];
    }
    elapsed = now_seconds() - start_time;
    print_result("sign", elapsed, iterations);

    start_time = now_seconds();
    for (index = 0; index < iterations; index++) {
        require_success(sig_verify(pk, pk_len, signature, sig_len, message, message_len),
                        "sig_verify");
        bench_sink += (unsigned long long)signature[index % sig_len];
    }
    elapsed = now_seconds() - start_time;
    print_result("verify", elapsed, iterations);
    printf("Sign attempts used: %llu\n", sign_attempts_total);

    if (bench_sink == ULLONG_MAX) {
        printf("sink=%llu\n", bench_sink);
    }

    free(message);
    free(signature);
    free(sk);
    free(pk);

    return EXIT_SUCCESS;
}
