#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(_WIN32)
#include <windows.h>
#endif

#include "api.h"
#include "kem_qube.h"
#include "parameters.h"
#include "randombytes.h"
#include "symmetric.h"

#define NB_TEST 10
#define NB_SAMPLES 1000

static inline uint64_t cpucycles_start(void) {
    unsigned hi, lo;
    __asm__ __volatile__(
        "CPUID\n\t"
        "RDTSC\n\t"
        "mov %%edx, %0\n\t"
        "mov %%eax, %1\n\t"
        : "=r"(hi), "=r"(lo)
        :
        : "%rax", "%rbx", "%rcx", "%rdx");
    return ((uint64_t)lo) ^ (((uint64_t)hi) << 32);
}

static inline uint64_t cpucycles_stop(void) {
    unsigned hi, lo;
    __asm__ __volatile__(
        "RDTSCP\n\t"
        "mov %%edx, %0\n\t"
        "mov %%eax, %1\n\t"
        "CPUID\n\t"
        : "=r"(hi), "=r"(lo)
        :
        : "%rax", "%rbx", "%rcx", "%rdx");
    return ((uint64_t)lo) ^ (((uint64_t)hi) << 32);
}

static uint64_t get_time_ns(void) {
#if defined(_WIN32)
    static LARGE_INTEGER frequency;
    LARGE_INTEGER counter;

    if (frequency.QuadPart == 0) {
        QueryPerformanceFrequency(&frequency);
    }

    QueryPerformanceCounter(&counter);
    return (uint64_t)((counter.QuadPart * 1000000000ULL) / frequency.QuadPart);
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
#endif
}

int main(void) {
    unsigned char pk[PUBLIC_KEY_BYTES];
    unsigned char sk[SECRET_KEY_BYTES];
    unsigned char ct[CIPHERTEXT_BYTES];
    unsigned char ss1[SHARED_SECRET_BYTES];
    unsigned char ss2[SHARED_SECRET_BYTES];

    unsigned long long pk_len_bytes = 0;
    unsigned long long sk_len_bytes = 0;
    unsigned long long ct_len_bytes = 0;
    unsigned long long key1_len_bytes = 0;
    unsigned long long key2_len_bytes = 0;

    uint64_t t1, t2, ns1, ns2;
    uint64_t keygen_cycles_total = 0;
    uint64_t encaps_cycles_total = 0;
    uint64_t decaps_cycles_total = 0;
    uint64_t keygen_ns_total = 0;
    uint64_t encaps_ns_total = 0;
    uint64_t decaps_ns_total = 0;

    unsigned char seed[48] = {0};
    CryptoRandomBytes(seed, sizeof(seed));
    prng_init(seed, NULL, sizeof(seed), 0);

    printf("\nRunning QUBE KEM Benchmark ....\n");
    printf("QUBE-%d-%d\n", PARAM_SECURITY, PARAM_DFR_EXP);
    printf("%d function calls, %d samples average\n\n", NB_TEST, NB_SAMPLES);

    for (size_t i = 0; i < NB_TEST; i++) {
        kem_keygen(pk, &pk_len_bytes, sk, &sk_len_bytes);
    }

    for (size_t i = 0; i < NB_SAMPLES; i++) {
        uint64_t timer_c = 0;
        uint64_t timer_ns = 0;

        for (size_t j = 0; j < NB_TEST; j++) {
            t1 = cpucycles_start();
            ns1 = get_time_ns();
            kem_keygen(pk, &pk_len_bytes, sk, &sk_len_bytes);
            ns2 = get_time_ns();
            t2 = cpucycles_stop();

            timer_c += t2 - t1;
            timer_ns += ns2 - ns1;
        }

        keygen_cycles_total += timer_c / NB_TEST;
        keygen_ns_total += timer_ns / NB_TEST;
    }

    for (size_t i = 0; i < NB_TEST; i++) {
        kem_enc(pk, pk_len_bytes, ss1, &key1_len_bytes, ct, &ct_len_bytes);
    }

    for (size_t i = 0; i < NB_SAMPLES; i++) {
        uint64_t timer_c = 0;
        uint64_t timer_ns = 0;

        for (size_t j = 0; j < NB_TEST; j++) {
            t1 = cpucycles_start();
            ns1 = get_time_ns();
            kem_enc(pk, pk_len_bytes, ss1, &key1_len_bytes, ct, &ct_len_bytes);
            ns2 = get_time_ns();
            t2 = cpucycles_stop();

            timer_c += t2 - t1;
            timer_ns += ns2 - ns1;
        }

        encaps_cycles_total += timer_c / NB_TEST;
        encaps_ns_total += timer_ns / NB_TEST;
    }

    for (size_t i = 0; i < NB_TEST; i++) {
        kem_dec(sk, sk_len_bytes, ct, ct_len_bytes, ss2, &key2_len_bytes);
    }

    if (memcmp(ss1, ss2, SHARED_SECRET_BYTES) != 0) {
        fprintf(stderr, "ERROR: decapsulation shared secret key != encapsulation shared secret key\n");
        return 1;
    }

    for (size_t i = 0; i < NB_SAMPLES; i++) {
        uint64_t timer_c = 0;
        uint64_t timer_ns = 0;

        for (size_t j = 0; j < NB_TEST; j++) {
            t1 = cpucycles_start();
            ns1 = get_time_ns();
            kem_dec(sk, sk_len_bytes, ct, ct_len_bytes, ss2, &key2_len_bytes);
            ns2 = get_time_ns();
            t2 = cpucycles_stop();

            timer_c += t2 - t1;
            timer_ns += ns2 - ns1;
        }

        decaps_cycles_total += timer_c / NB_TEST;
        decaps_ns_total += timer_ns / NB_TEST;
    }

    double keygen_cycles_avg = (double)keygen_cycles_total / NB_SAMPLES;
    double encaps_cycles_avg = (double)encaps_cycles_total / NB_SAMPLES;
    double decaps_cycles_avg = (double)decaps_cycles_total / NB_SAMPLES;

    double keygen_ms = keygen_ns_total / (double)NB_SAMPLES / 1e6;
    double encaps_ms = encaps_ns_total / (double)NB_SAMPLES / 1e6;
    double decaps_ms = decaps_ns_total / (double)NB_SAMPLES / 1e6;

    uint64_t ged_cycles_total = keygen_cycles_total + encaps_cycles_total + decaps_cycles_total;
    uint64_t ged_ns_total = keygen_ns_total + encaps_ns_total + decaps_ns_total;
    double ged_cycles_avg = (double)ged_cycles_total / NB_SAMPLES;
    double ged_ms = ged_ns_total / (double)NB_SAMPLES / 1e6;

    printf("\n--- QUBE KEM Benchmark ---\n");
    printf("Keygen : %.0f cycles, %.2f ms\n", keygen_cycles_avg, keygen_ms);
    printf("Encaps : %.0f cycles, %.2f ms\n", encaps_cycles_avg, encaps_ms);
    printf("Decaps : %.0f cycles, %.2f ms\n", decaps_cycles_avg, decaps_ms);
    printf("G-E-D  : %.0f cycles, %.2f ms\n", ged_cycles_avg, ged_ms);
    printf("\n");

    return 0;
}
