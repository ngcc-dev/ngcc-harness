/* Performance test entry for one API_PKC optimized Frost KEM instance. */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "drng.h"
#include "KEM_AlgorithmInstance.h"

#if defined(__x86_64__) || defined(__i386__) || defined(_M_X64) || defined(_M_IX86)
#include <x86intrin.h>
#endif

DRNG_ctx drng_algorithm;

static unsigned long long read_cycles(void)
{
#if defined(__x86_64__) || defined(__i386__) || defined(_M_X64) || defined(_M_IX86)
    return __rdtsc();
#else
    return 0ULL;
#endif
}

static double now_seconds(void)
{
    return (double)clock() / (double)CLOCKS_PER_SEC;
}

static void clear_secret(unsigned char *buffer, size_t length)
{
    volatile unsigned char *p = buffer;
    while (buffer != NULL && length-- != 0U) {
        *p++ = 0;
    }
}

static int seed_drng(void)
{
    unsigned char seed[64];
    for (size_t i = 0; i < sizeof(seed); ++i) {
        seed[i] = (unsigned char)(0xA5U ^ (unsigned int)(i * 17U));
    }
    return init_random_number(&drng_algorithm, seed, sizeof(seed));
}

int main(int argc, char **argv)
{
    unsigned long iterations = 100UL;
    char *end = NULL;
    unsigned char *pk = NULL;
    unsigned char *sk = NULL;
    unsigned char *ct = NULL;
    unsigned char *ss = NULL;
    unsigned long long pk_len = 0, sk_len = 0, ct_len = 0, ss_len = 0;
    unsigned long long c0, keypair_cycles = 0, enc_cycles = 0, dec_cycles = 0;
    double t0, keypair_seconds, enc_seconds, dec_seconds;
    FILE *csv = NULL;
    char csv_name[128];
    int rc = EXIT_FAILURE;

    if (argc == 2) {
        iterations = strtoul(argv[1], &end, 10);
        if (end == argv[1] || *end != '\0' || iterations == 0UL) {
            fprintf(stderr, "Usage: %s [positive-iteration-count]\n", argv[0]);
            return EXIT_FAILURE;
        }
    }
    if (seed_drng() != 0) {
        fprintf(stderr, "%s: DRNG init failed\n", ALGORITHM_INSTANCE);
        return EXIT_FAILURE;
    }

    pk = (unsigned char *)malloc((size_t)kem_get_pk_len_bytes());
    sk = (unsigned char *)malloc((size_t)kem_get_sk_len_bytes());
    ct = (unsigned char *)malloc((size_t)kem_get_ct_len_bytes());
    ss = (unsigned char *)malloc((size_t)kem_get_ss_len_bytes());
    if (pk == NULL || sk == NULL || ct == NULL || ss == NULL) {
        fprintf(stderr, "%s: allocation failed\n", ALGORITHM_INSTANCE);
        goto cleanup;
    }

    t0 = now_seconds();
    for (unsigned long i = 0; i < iterations; ++i) {
        c0 = read_cycles();
        if (kem_keygen(pk, &pk_len, sk, &sk_len) != 0) goto cleanup;
        keypair_cycles += read_cycles() - c0;
    }
    keypair_seconds = now_seconds() - t0;

    t0 = now_seconds();
    for (unsigned long i = 0; i < iterations; ++i) {
        c0 = read_cycles();
        if (kem_enc(pk, pk_len, ss, &ss_len, ct, &ct_len) != 0) goto cleanup;
        enc_cycles += read_cycles() - c0;
    }
    enc_seconds = now_seconds() - t0;

    t0 = now_seconds();
    for (unsigned long i = 0; i < iterations; ++i) {
        c0 = read_cycles();
        if (kem_dec(sk, sk_len, ct, ct_len, ss, &ss_len) != 0) goto cleanup;
        dec_cycles += read_cycles() - c0;
    }
    dec_seconds = now_seconds() - t0;

    printf("%s optimized AVX2 performance (%lu iterations)\n", ALGORITHM_INSTANCE, iterations);
    printf("pk/sk/ct/ss = %llu/%llu/%llu/%llu bytes\n",
           kem_get_pk_len_bytes(), kem_get_sk_len_bytes(),
           kem_get_ct_len_bytes(), kem_get_ss_len_bytes());
    printf("keypair: avg cycles %.2f, ops/s %.2f\n",
           (double)keypair_cycles / (double)iterations,
           keypair_seconds > 0.0 ? (double)iterations / keypair_seconds : 0.0);
    printf("encaps:  avg cycles %.2f, ops/s %.2f\n",
           (double)enc_cycles / (double)iterations,
           enc_seconds > 0.0 ? (double)iterations / enc_seconds : 0.0);
    printf("decaps:  avg cycles %.2f, ops/s %.2f\n",
           (double)dec_cycles / (double)iterations,
           dec_seconds > 0.0 ? (double)iterations / dec_seconds : 0.0);

    snprintf(csv_name, sizeof(csv_name), "perf_%s.csv", ALGORITHM_INSTANCE);
    csv = fopen(csv_name, "wb");
    if (csv == NULL) goto cleanup;
    fprintf(csv, "algorithm,implementation,iterations,compiler_flags,avx2_required,pk_bytes,sk_bytes,ct_bytes,ss_bytes,keypair_avg_cycles,encaps_avg_cycles,decaps_avg_cycles,keypair_ops_per_s,encaps_ops_per_s,decaps_ops_per_s\n");
    fprintf(csv, "%s,optimized-avx2,%lu,\"-O3 -march=x86-64 -mavx2 -maes -mtune=native -fomit-frame-pointer\",yes,%llu,%llu,%llu,%llu,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n",
            ALGORITHM_INSTANCE, iterations,
            kem_get_pk_len_bytes(), kem_get_sk_len_bytes(),
            kem_get_ct_len_bytes(), kem_get_ss_len_bytes(),
            (double)keypair_cycles / (double)iterations,
            (double)enc_cycles / (double)iterations,
            (double)dec_cycles / (double)iterations,
            keypair_seconds > 0.0 ? (double)iterations / keypair_seconds : 0.0,
            enc_seconds > 0.0 ? (double)iterations / enc_seconds : 0.0,
            dec_seconds > 0.0 ? (double)iterations / dec_seconds : 0.0);
    rc = EXIT_SUCCESS;

cleanup:
    if (csv != NULL) fclose(csv);
    clear_secret(ss, (size_t)kem_get_ss_len_bytes());
    clear_secret(sk, (size_t)kem_get_sk_len_bytes());
    free(ss);
    free(ct);
    free(sk);
    free(pk);
    return rc;
}
