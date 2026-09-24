#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../params.h"
#include "../kem.h"
#include "../drng.h"

DRNG_ctx drng_algorithm;

#define NTESTS 1000

int main(void)
{
    unsigned int i;

    uint8_t pk[KEM_PUBLICKEYBYTES];
    uint8_t sk[KEM_SECRETKEYBYTES];
    uint8_t ct[KEM_CIPHERTEXTBYTES];
    uint8_t ss1[SEEDBYTES];
    uint8_t ss2[SEEDBYTES];

    /* 初始化 drng */
    unsigned char init_seed[64];
    for (i = 0; i < 64; i++) init_seed[i] = (unsigned char)i;
    init_random_number(&drng_algorithm, init_seed, 64);

    for (i = 0; i < NTESTS; ++i) {
        crypto_kem_keypair(pk, sk);
        crypto_kem_enc(ct, ss1, pk);
        crypto_kem_dec(ss2, ct, sk);

        if (memcmp(ss1, ss2, SEEDBYTES) != 0) {
            fprintf(stderr, "crypto_kem_enc/dec mismatch at test %u\n", i);
            return -1;
        }
    }

    printf("All %d KEM tests passed\n", NTESTS);

    printf("\nKEM_PUBLICKEYBYTES = %d\n", KEM_PUBLICKEYBYTES);
    printf("KEM_SECRETKEYBYTES = %d\n", KEM_SECRETKEYBYTES);
    printf("KEM_CIPHERTEXTBYTES = %d\n", KEM_CIPHERTEXTBYTES);
    printf("KEM_MSGBYTES = %d\n", KEM_MSGBYTES);

    return 0;
}