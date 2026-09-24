#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "SIG_AlgorithmInstance.h"
#include "drng.h"

#define STRINGIFY_INNER(x) #x
#define STRINGIFY(x) STRINGIFY_INNER(x)

#define COST_SEED_LEN_BYTES 72ULL
#define DEFAULT_MESSAGE_BYTES 64ULL
#define MAX_SIGN_ATTEMPTS 10000ULL

DRNG_ctx drng_algorithm;

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

static void fill_message(unsigned char *message, unsigned long long len)
{
    unsigned long long i;
    for (i = 0; i < len; i++) {
        message[i] = (unsigned char)((i * 7 + 3) & 0xFFULL);
    }
}

static void fill_seed(unsigned char seed[COST_SEED_LEN_BYTES])
{
    unsigned long long i;
    for (i = 0; i < COST_SEED_LEN_BYTES; i++) {
        seed[i] = (unsigned char)((i * 17 + 13) & 0xFFULL);
    }
}

int main(int argc, char *argv[])
{
    unsigned long long pk_bytes, sk_bytes, sig_bytes;
    unsigned long long msg_bytes = DEFAULT_MESSAGE_BYTES;
    unsigned char *pk, *sk, *sig;
    unsigned char *msg;
    unsigned char seed[COST_SEED_LEN_BYTES];

    (void)argc;
    (void)argv;

    fill_seed(seed);
    if (init_random_number(&drng_algorithm, seed, COST_SEED_LEN_BYTES) != 0) {
        fprintf(stderr, "ERROR: init_random_number failed\n");
        return EXIT_FAILURE;
    }

    pk_bytes = sig_get_pk_len_bytes();
    sk_bytes = sig_get_sk_len_bytes();
    sig_bytes = sig_get_sn_len_bytes();

    pk = checked_calloc(pk_bytes, "public key");
    sk = checked_calloc(sk_bytes, "secret key");
    sig = checked_calloc(sig_bytes, "signature");
    msg = checked_calloc(msg_bytes, "message");
    fill_message(msg, msg_bytes);

    if (sig_keygen(pk, &pk_bytes, sk, &sk_bytes) != 0) {
        fprintf(stderr, "ERROR: keygen failed\n");
        return EXIT_FAILURE;
    }

    {
        unsigned long long attempt;
        int result = -1;
        for (attempt = 1ULL; attempt <= MAX_SIGN_ATTEMPTS; attempt++) {
            sig_bytes = sig_get_sn_len_bytes();
            result = sig_sign(sk, sk_bytes, msg, msg_bytes, sig, &sig_bytes);
            if (result == 0) break;
            if (result != -4) break;
        }
        if (result != 0) {
            fprintf(stderr, "ERROR: sign failed\n");
            return EXIT_FAILURE;
        }
    }

    {
        unsigned long long pk_sig_bytes = pk_bytes + sig_bytes;
        unsigned long long keypair_bytes = pk_bytes + sk_bytes;

        printf("%-40s %10s %10s %10s %10s %10s %12s %12s %12s %12s\n",
               "instance",
               "pk_bytes", "sk_bytes", "sig_bytes",
               "pk_sig_bytes", "keypair_bytes",
               "sig_100M_us", "pk_sig_100M_us",
               "sig_1G_us", "pk_sig_1G_us");
        printf("%-40s %10llu %10llu %10llu %10llu %10llu %12.2f %12.2f %12.2f %12.2f\n",
               ALGORITHM_INSTANCE,
               pk_bytes, sk_bytes, sig_bytes,
               pk_sig_bytes, keypair_bytes,
               (8.0 * sig_bytes) / 100.0,
               (8.0 * pk_sig_bytes) / 100.0,
               (8.0 * sig_bytes) / 1000.0,
               (8.0 * pk_sig_bytes) / 1000.0);
    }

    free(pk);
    free(sk);
    free(sig);
    free(msg);

    return EXIT_SUCCESS;
}
