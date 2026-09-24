#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "../params.h"
#include "../indcpa.h"
#include "../drng.h"

DRNG_ctx drng_algorithm;

#define MLEN KEM_CPAPKE_MSGBYTES
#define NTESTS 1000

int main(void)
{
    unsigned int i, j;
    uint8_t m[MLEN];
    uint8_t m2[MLEN];
    uint8_t c[KEM_CPAPKE_CIPHERTEXTBYTES];
    uint8_t coins[SEEDBYTES];
    uint8_t pk[KEM_CPAPKE_PUBLICKEYBYTES];
    uint8_t sk[KEM_CPAPKE_SECRETKEYBYTES];

    /* 初始化 drng */
    unsigned char init_seed[64];
    for (i = 0; i < 64; i++) init_seed[i] = (unsigned char)i;
    init_random_number(&drng_algorithm, init_seed, 64);

    for (i = 0; i < NTESTS; ++i) {
        get_random_number(&drng_algorithm, m, (unsigned long long)MLEN * 8);
        get_random_number(&drng_algorithm, coins, (unsigned long long)SEEDBYTES * 8);

        indcpa_keypair(pk, sk);
        indcpa_enc(c, m, pk, coins);
        indcpa_dec(m2, c, sk);

        if (memcmp(m, m2, MLEN) != 0) {
            fprintf(stderr, "Decryption failed at test %d\n", i);
            fprintf(stderr, "Original:  ");
            for (j = 0; j < MLEN; j++) fprintf(stderr, "%02x", m[j]);
            fprintf(stderr, "\n");
            fprintf(stderr, "Decrypted: ");
            for (j = 0; j < MLEN; j++) fprintf(stderr, "%02x", m2[j]);
            fprintf(stderr, "\n");
            return -1;
        }
    }

    printf("All %d encryption/decryption tests passed!\n", NTESTS);
    printf("KEM_CPAPKE_PUBLICKEYBYTES = %d\n", KEM_CPAPKE_PUBLICKEYBYTES);
    printf("KEM_CPAPKE_SECRETKEYBYTES = %d\n", KEM_CPAPKE_SECRETKEYBYTES);
    printf("KEM_CPAPKE_CIPHERTEXTBYTES = %d\n", KEM_CPAPKE_CIPHERTEXTBYTES);
    printf("KEM_CPAPKE_MSGBYTES = %d\n", KEM_CPAPKE_MSGBYTES);

    return 0;
}