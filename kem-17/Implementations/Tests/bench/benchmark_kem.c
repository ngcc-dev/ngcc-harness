#define _DEFAULT_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>
#include "KEM_HEP_QC.h"
#include "parameters.h"
#include "symmetric.h"

#define NB_TEST    5
#define NB_SAMPLES 1

inline static uint64_t cpucyclesStart(void) {
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

inline static uint64_t cpucyclesStop(void) {
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

static inline uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

int main(void) {
    unsigned char *pk = calloc(PUBLIC_KEY_BYTES, sizeof(*pk));
    unsigned char *sk = calloc(SECRET_KEY_BYTES, sizeof(*sk));
    unsigned char ct[CIPHERTEXT_BYTES];
    unsigned char ss1[SHARED_SECRET_BYTES];
    unsigned char ss2[SHARED_SECRET_BYTES];

    uint64_t t1, t2, ns1, ns2;
    uint64_t keygen_cycles_total = 0, encaps_cycles_total = 0, decaps_cycles_total = 0;
    uint64_t keygen_ns_total = 0, encaps_ns_total = 0, decaps_ns_total = 0;

    unsigned char seed[48] = {0};
    syscall(SYS_getrandom, seed, 48, 0);
    prng_init(seed, 48);

    // warm-up
    for (size_t i = 0; i < NB_TEST; i++) {
        kem_keygen(pk, NULL, sk, NULL);
    }
    for (size_t i = 0; i < NB_SAMPLES; i++) {
        uint64_t timer_c = 0, timer_ns = 0;
        for (size_t j = 0; j < NB_TEST; j++) {
            t1 = cpucyclesStart();
            ns1 = get_time_ns();
            kem_keygen(pk, NULL, sk, NULL);
            ns2 = get_time_ns();
            t2 = cpucyclesStop();

            timer_c += (t2 - t1);
            timer_ns += (ns2 - ns1);
        }
        keygen_cycles_total += timer_c / NB_TEST;
        keygen_ns_total += timer_ns / NB_TEST;
    }

    // warm-up
    for (size_t i = 0; i < NB_TEST; i++) {
        kem_enc(pk, 0, ss1, NULL, ct, NULL);
    }
    for (size_t i = 0; i < NB_SAMPLES; i++) {
        uint64_t timer_c = 0, timer_ns = 0;
        for (size_t j = 0; j < NB_TEST; j++) {
            t1 = cpucyclesStart();
            ns1 = get_time_ns();
            kem_enc(pk, 0, ss1, NULL, ct, NULL);
            ns2 = get_time_ns();
            t2 = cpucyclesStop();

            timer_c += (t2 - t1);
            timer_ns += (ns2 - ns1);
        }
        encaps_cycles_total += timer_c / NB_TEST;
        encaps_ns_total += timer_ns / NB_TEST;
    }

    // warm-up
    for (size_t i = 0; i < NB_TEST; i++) {
        kem_dec(sk, 0, ct, 0, ss2, NULL);
    }
    for (size_t i = 0; i < NB_SAMPLES; i++) {
        if (memcmp(ss1, ss2, SHARED_SECRET_BYTES)) {
            exit(1);
        }

        uint64_t timer_c = 0, timer_ns = 0;
        for (size_t j = 0; j < NB_TEST; j++) {
            t1 = cpucyclesStart();
            ns1 = get_time_ns();
            kem_dec(sk, 0, ct, 0, ss2, NULL);
            ns2 = get_time_ns();
            t2 = cpucyclesStop();

            timer_c += (t2 - t1);
            timer_ns += (ns2 - ns1);
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

    printf("\n--- HEP-QC KEM Benchmark ---\n");
    printf("Keygen : %.0f kilo cycles, %.2f ms \n", keygen_cycles_avg / 1000, keygen_ms);
    printf("Encaps : %.0f kilo cycles, %.2f ms \n", encaps_cycles_avg / 1000, encaps_ms);
    printf("Decaps : %.0f kilo cycles, %.2f ms \n", decaps_cycles_avg / 1000, decaps_ms);
    printf("\n");

    free(pk);
    free(sk);
    return 0;
}
