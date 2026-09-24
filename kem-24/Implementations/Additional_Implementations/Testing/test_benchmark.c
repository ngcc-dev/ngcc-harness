#define _POSIX_C_SOURCE 199309L
#include "api.h"
#include "drng.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <x86intrin.h>

#define NTESTS 10000
#define SEED_LEN_BYTES 64
DRNG_ctx drng_algorithm;

struct BenchmarkResults
{
    size_t keygen_cycles;
    size_t enc_cycles;
    size_t dec_cycles;
};

static void
calculate_results (struct BenchmarkResults results[NTESTS])
{
    size_t keygen_sum = 0, enc_sum = 0, dec_sum = 0;
    size_t keygen_max = 0, enc_max = 0, dec_max = 0;
    size_t keygen_min = results[0].keygen_cycles,
                 enc_min = results[0].enc_cycles, dec_min = results[0].dec_cycles;
    for (int i = 0; i < NTESTS; i++)
        {
            if (results[i].keygen_cycles > keygen_max)
                keygen_max = results[i].keygen_cycles;
            if (results[i].enc_cycles > enc_max)
                enc_max = results[i].enc_cycles;
            if (results[i].dec_cycles > dec_max)
                dec_max = results[i].dec_cycles;
            if (results[i].keygen_cycles < keygen_min)
                keygen_min = results[i].keygen_cycles;
            if (results[i].enc_cycles < enc_min)
                enc_min = results[i].enc_cycles;
            if (results[i].dec_cycles < dec_min)
                dec_min = results[i].dec_cycles;
            keygen_sum += results[i].keygen_cycles;
            enc_sum += results[i].enc_cycles;
            dec_sum += results[i].dec_cycles;
        }
    printf ("=========== BENCHMARK RESULTS (%d tests) ===========\n", NTESTS);
    printf ("KEYGEN RESULTS:\n");
    printf ("\taverage: %f\n", ((float)keygen_sum) / ((float)NTESTS));
    printf ("\tmax: %ld\n", keygen_max);
    printf ("\tmin: %ld\n", keygen_min);
    printf ("\trange: %ld\n", keygen_max - keygen_min);
    printf ("ENCAPSULATION RESULTS:\n");
    printf ("\taverage: %f\n", ((float)enc_sum) / ((float)NTESTS));
    printf ("\tmax: %ld\n", enc_max);
    printf ("\tmin: %ld\n", enc_min);
    printf ("\trange: %ld\n", enc_max - enc_min);
    printf ("DECAPSULATION RESULTS:\n");
    printf ("\taverage: %f\n", ((float)dec_sum) / ((float)NTESTS));
    printf ("\tmax: %ld\n", dec_max);
    printf ("\tmin: %ld\n", dec_min);
    printf ("\trange: %ld\n", dec_max - dec_min);
    // TODO: Print to file (csv)
}

static int
benchmark_throughput (struct BenchmarkResults *res)
{
    unsigned char pk[KEM_PUBLICKEYBYTES];
    unsigned long long pk_len_bytes = KEM_PUBLICKEYBYTES;
    unsigned char sk[KEM_SECRETKEYBYTES];
    unsigned long long sk_len_bytes = KEM_SECRETKEYBYTES;
    unsigned char ct[KEM_CIPHERTEXTBYTES];
    unsigned long long ct_len_bytes = KEM_CIPHERTEXTBYTES;
    unsigned char key_a[KEM_SSBYTES];
    unsigned char key_b[KEM_SSBYTES];
    unsigned long long ss_len_bytes = KEM_SSBYTES;

    struct timespec time_s, time_e;

    // Alice generates a public key
    clock_gettime (CLOCK_MONOTONIC, &time_s);
    kem_keygen (pk, &pk_len_bytes, sk, &sk_len_bytes);
    clock_gettime (CLOCK_MONOTONIC, &time_e);
    res->keygen_cycles
            = ((((uint64_t)time_e.tv_sec - time_s.tv_sec) * 1000000000)
                 + time_e.tv_nsec)
                - time_s.tv_nsec;

    // Bob derives a secret key and creates a response
    clock_gettime (CLOCK_MONOTONIC, &time_s);
    kem_enc (pk, pk_len_bytes, key_b, &ss_len_bytes, ct, &ct_len_bytes);
    clock_gettime (CLOCK_MONOTONIC, &time_e);
    res->enc_cycles = ((((uint64_t)time_e.tv_sec - time_s.tv_sec) * 1000000000)
                                         + time_e.tv_nsec)
                                        - time_s.tv_nsec;

    // Alice uses Bobs response to get her shared key
    clock_gettime (CLOCK_MONOTONIC, &time_s);
    kem_dec (sk, sk_len_bytes, ct, ct_len_bytes, key_a, &ss_len_bytes);
    clock_gettime (CLOCK_MONOTONIC, &time_e);
    res->dec_cycles = ((((uint64_t)time_e.tv_sec - time_s.tv_sec) * 1000000000)
                                         + time_e.tv_nsec)
                                        - time_s.tv_nsec;

    return 0;
}

static int
benchmark_cycles (struct BenchmarkResults *res)
{
    unsigned char pk[KEM_PUBLICKEYBYTES];
    unsigned long long pk_len_bytes = KEM_PUBLICKEYBYTES;
    unsigned char sk[KEM_SECRETKEYBYTES];
    unsigned long long sk_len_bytes = KEM_SECRETKEYBYTES;
    unsigned char ct[KEM_CIPHERTEXTBYTES];
    unsigned long long ct_len_bytes = KEM_CIPHERTEXTBYTES;
    unsigned char key_a[KEM_SSBYTES];
    unsigned char key_b[KEM_SSBYTES];
    unsigned long long ss_len_bytes = KEM_SSBYTES;

    uint64_t cycle_s = 0, cycle_e = 0;

    // Alice generates a public key
    cycle_s = __rdtsc ();
    kem_keygen (pk, &pk_len_bytes, sk, &sk_len_bytes);
    cycle_e = __rdtsc ();
    res->keygen_cycles = cycle_e - cycle_s;

    // Bob derives a secret key and creates a response
    cycle_s = __rdtsc ();
    kem_enc (pk, pk_len_bytes, key_b, &ss_len_bytes, ct, &ct_len_bytes);
    cycle_e = __rdtsc ();
    res->enc_cycles = cycle_e - cycle_s;

    // Alice uses Bobs response to get her shared key
    cycle_s = __rdtsc ();
    kem_dec (sk, sk_len_bytes, ct, ct_len_bytes, key_a, &ss_len_bytes);
    cycle_e = __rdtsc ();
    res->dec_cycles = cycle_e - cycle_s;

    return 0;
}

int
main (void)
{
    unsigned int i;
    int r = 0;

    // Randomness
    // For generating the seed, we assume for testing purposes
    // memory is sufficiently random.
    DRNG_ctx drng_seed;
    unsigned char seed[SEED_LEN_BYTES];
    struct BenchmarkResults results_cycles[NTESTS];
    struct BenchmarkResults results_throughput[NTESTS];

    // Initialise the "randomly", using the drng_seed memory as the random ctx.
    get_random_number (&drng_seed, seed, SEED_LEN_BYTES * 8);

    // Initialise the proper random context for the runs.
    init_random_number (&drng_algorithm, seed, SEED_LEN_BYTES);

    for (i = 0; i < NTESTS; i++)
        {
            r = benchmark_cycles (&results_cycles[i]);
            r = benchmark_throughput (&results_throughput[i]);
            if (r)
                return 1;
        }

    printf ("CYCLE COUNTS\n");
    calculate_results (results_cycles);
    printf ("\n");
    printf ("NANOSECOND COUNTS\n");
    calculate_results (results_throughput);

    return 0;
}