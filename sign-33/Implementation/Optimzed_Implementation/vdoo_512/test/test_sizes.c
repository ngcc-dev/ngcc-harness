#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../api.h"
#include "../vdoo_config.h"
#include "../drng.h"

#define MLEN 64

DRNG_ctx drng_algorithm;

static void print_size(const char *name, size_t bytes)
{
    printf("  %-20s: %10zu bytes  (%6.2f KB)  (%6.2f MB)\n",
           name, bytes,
           (double)bytes / 1024,
           (double)bytes / (1024 * 1024));
}

int main(void)
{
    static unsigned char pk[CRYPTO_PUBLICKEYBYTES];
    static unsigned char sk[CRYPTO_SECRETKEYBYTES];
    static unsigned char sig[CRYPTO_BYTES];
    static unsigned char msg[MLEN];
    static unsigned long long sig_len;
    static unsigned long long pk_len_bytes, sk_len_bytes;

    printf("========================================\n");
    printf("VDOO-512 Transmission & Storage Overhead\n");
    printf("========================================\n");

    static unsigned char seed[48] = {0};
    init_randombytes(seed, 48);

    if (sig_keygen(pk, &pk_len_bytes, sk, &sk_len_bytes) != 0)
    {
        printf("Keygen failed\n");
        return -1;
    }

    get_random_number(&drng_algorithm, msg, MLEN * 8);
    if (sig_sign(sk, sk_len_bytes, msg, MLEN, sig, &sig_len) != 0)
    {
        printf("Sign failed\n");
        return -1;
    }

    printf("\n--- Key and Signature Sizes ---\n");
    printf("  Parameter                Value (bytes)\n");
    printf("  ------------------------  ------------\n");
    print_size("Public Key", pk_len_bytes);
    print_size("Private Key", sk_len_bytes);
    print_size("Signature", sig_len);
    print_size("Hash (SM3)", HASH_LEN);
    print_size("Salt", SALT_BYTES);
    print_size("Message (test)", MLEN);

    size_t total_key = pk_len_bytes + sk_len_bytes;
    size_t total_transmission = pk_len_bytes + sig_len + MLEN;
    size_t total_storage = pk_len_bytes + sk_len_bytes + sig_len;

    printf("\n--- Totals ---\n");
    print_size("Total Key Pair", total_key);
    print_size("Total Transmission (PK + Sig + Msg)", total_transmission);
    print_size("Total Storage (PK + SK + Sig)", total_storage);

    double ratio = (double)sig_len / pk_len_bytes * 100;
    printf("\n  Signature/Public Key ratio: %.2f%%\n", ratio);

    printf("\n========================================\n");
    printf("Transmission & Storage Overhead Complete\n");
    printf("========================================\n");

    return 0;
}
