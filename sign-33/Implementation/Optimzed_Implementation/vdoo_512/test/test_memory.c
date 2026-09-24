#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../api.h"
#include "../vdoo_config.h"
#include "../drng.h"

#define MLEN 64

DRNG_ctx drng_algorithm;

static void measure_memory_proc(void)
{
    FILE *fp = fopen("/proc/self/status", "r");
    if (fp == NULL)
    {
        printf("  Warning: Cannot open /proc/self/status\n");
        return;
    }

    char line[256];
    size_t vm_peak = 0, vm_rss = 0, vm_data = 0, vm_stack = 0;

    while (fgets(line, sizeof(line), fp))
    {
        if (sscanf(line, "VmPeak: %zu kB", &vm_peak) == 1)
	{
            printf("  VmPeak (peak virtual memory):   %zu bytes (%.2f MB)\n", 
                   vm_peak * 1024, (double)(vm_peak * 1024) / (1024 * 1024));
        }
        if (sscanf(line, "VmRSS: %zu kB", &vm_rss) == 1)
	{
            printf("  VmRSS (resident set size):      %zu bytes (%.2f MB)\n",
                   vm_rss * 1024, (double)(vm_rss * 1024) / (1024 * 1024));
        }
        if (sscanf(line, "VmData: %zu kB", &vm_data) == 1)
	{
            printf("  VmData (data segment):          %zu bytes (%.2f MB)\n",
                   vm_data * 1024, (double)(vm_data * 1024) / (1024 * 1024));
        }
        if (sscanf(line, "VmStk: %zu kB", &vm_stack) == 1)
	{
            printf("  VmStk (stack):                  %zu bytes (%.2f MB)\n",
                   vm_stack * 1024, (double)(vm_stack * 1024) / (1024 * 1024));
        }
    }
    fclose(fp);
}

static void measure_static_memory(void)
{
    printf("\n  Static memory (from vdoo_config.h):\n");
    printf("    Public key (CRYPTO_PUBLICKEYBYTES):  %d bytes (%.2f KB)\n", 
           CRYPTO_PUBLICKEYBYTES, (double)CRYPTO_PUBLICKEYBYTES / 1024);
    printf("    Private key (CRYPTO_SECRETKEYBYTES): %d bytes (%.2f KB)\n",
           CRYPTO_SECRETKEYBYTES, (double)CRYPTO_SECRETKEYBYTES / 1024);
    printf("    Signature (CRYPTO_BYTES):            %d bytes (%.2f KB)\n",
           CRYPTO_BYTES, (double)CRYPTO_BYTES / 1024);
    printf("    Hash length (HASH_LEN):              %d bytes\n", HASH_LEN);
    printf("    Salt length (SALT_BYTES):            %d bytes\n", SALT_BYTES);

    size_t total_key_memory = CRYPTO_PUBLICKEYBYTES + CRYPTO_SECRETKEYBYTES + CRYPTO_BYTES;
    printf("\n  Total key/signature memory footprint: %zu bytes (%.2f KB)\n",
           total_key_memory, (double)total_key_memory / 1024);
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
    printf("VDOO-512 Resource Consumption Test\n");
    printf("========================================\n");

    static unsigned char seed[48] = {0};
    init_randombytes(seed, 48);

    printf("\n--- Before key generation ---\n");
    measure_memory_proc();

    if (sig_keygen(pk, &pk_len_bytes, sk, &sk_len_bytes) != 0)
    {
        printf("Keygen failed\n");
        return -1;
    }

    printf("\n--- After key generation ---\n");
    measure_memory_proc();

    get_random_number(&drng_algorithm, msg, MLEN * 8);
    if (sig_sign(sk, sk_len_bytes, msg, MLEN, sig, &sig_len) != 0)
    {
        printf("Sign failed\n");
        return -1;
    }

    printf("\n--- After signature generation ---\n");
    measure_memory_proc();
    measure_static_memory();

    printf("\n========================================\n");
    printf("Resource Consumption Test Complete\n");
    printf("========================================\n");

    return 0;
}
