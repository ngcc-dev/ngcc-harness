/* Basic API_PKC correctness and implicit-rejection test for one Frost instance. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "drng.h"
#include "KEM_AlgorithmInstance.h"

DRNG_ctx drng_algorithm;

static void clear_secret(unsigned char *buffer, size_t length)
{
    volatile unsigned char *p = buffer;
    while (buffer != NULL && length-- != 0U) {
        *p++ = 0;
    }
}

static int run_tests(unsigned long iterations)
{
    unsigned char seed[64];
    unsigned char *pk = NULL, *sk = NULL, *ct = NULL;
    unsigned char *ss_enc = NULL, *ss_dec = NULL, *ss_bad = NULL;
    unsigned long long pk_len, sk_len, ct_len, ss_len;
    int result = EXIT_FAILURE;

    for (size_t i = 0; i < sizeof(seed); ++i) seed[i] = (unsigned char)(i * 29U + 7U);
    if (init_random_number(&drng_algorithm, seed, sizeof(seed)) != 0) return EXIT_FAILURE;

    pk = (unsigned char *)malloc((size_t)kem_get_pk_len_bytes());
    sk = (unsigned char *)malloc((size_t)kem_get_sk_len_bytes());
    ct = (unsigned char *)malloc((size_t)kem_get_ct_len_bytes());
    ss_enc = (unsigned char *)malloc((size_t)kem_get_ss_len_bytes());
    ss_dec = (unsigned char *)malloc((size_t)kem_get_ss_len_bytes());
    ss_bad = (unsigned char *)malloc((size_t)kem_get_ss_len_bytes());
    if (pk == NULL || sk == NULL || ct == NULL || ss_enc == NULL ||
        ss_dec == NULL || ss_bad == NULL) goto cleanup;

    for (unsigned long i = 0; i < iterations; ++i) {
        if (kem_keygen(pk, &pk_len, sk, &sk_len) != 0 ||
            kem_enc(pk, pk_len, ss_enc, &ss_len, ct, &ct_len) != 0 ||
            kem_dec(sk, sk_len, ct, ct_len, ss_dec, &ss_len) != 0 ||
            memcmp(ss_enc, ss_dec, (size_t)ss_len) != 0) {
            fprintf(stderr, "KEM agreement failed at iteration %lu\n", i);
            goto cleanup;
        }
        ct[(i * 131UL + 1UL) % ct_len] ^= (unsigned char)(1U << (i & 7U));
        if (kem_dec(sk, sk_len, ct, ct_len, ss_bad, &ss_len) != 0 ||
            memcmp(ss_enc, ss_bad, (size_t)ss_len) == 0) {
            fprintf(stderr, "Implicit rejection failed at iteration %lu\n", i);
            goto cleanup;
        }
    }
    printf("%s: %lu keypair/encaps/decaps rounds passed.\n",
           ALGORITHM_INSTANCE, iterations);
    result = EXIT_SUCCESS;

cleanup:
    clear_secret(ss_bad, (size_t)kem_get_ss_len_bytes());
    clear_secret(ss_dec, (size_t)kem_get_ss_len_bytes());
    clear_secret(ss_enc, (size_t)kem_get_ss_len_bytes());
    clear_secret(sk, (size_t)kem_get_sk_len_bytes());
    free(ss_bad); free(ss_dec); free(ss_enc); free(ct); free(sk); free(pk);
    return result;
}

int main(int argc, char **argv)
{
    char *end = NULL;
    unsigned long iterations = 1000UL;
    if (argc == 2) {
        iterations = strtoul(argv[1], &end, 10);
        if (end == argv[1] || *end != '\0' || iterations == 0UL) {
            fprintf(stderr, "Usage: %s [positive-iteration-count]\n", argv[0]);
            return EXIT_FAILURE;
        }
    }
    return run_tests(iterations);
}
