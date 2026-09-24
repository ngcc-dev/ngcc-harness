#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../params.h"
#include "../kem.h"
#include "../rng.h"

#define NTESTS 1000

int main(void)
{
    unsigned int i;
    unsigned char init_seed[64];
    for (unsigned int j=0; j<64; j++) init_seed[j] = j;

    /* 初始化 drng */

    uint8_t pk[KEM_PUBLICKEYBYTES] __attribute__((aligned(32)));
    uint8_t sk[KEM_SECRETKEYBYTES] __attribute__((aligned(32)));
    uint8_t ct[KEM_CIPHERTEXTBYTES] __attribute__((aligned(32)));
    uint8_t ss1[SEEDBYTES] __attribute__((aligned(32)));
    uint8_t ss2[SEEDBYTES] __attribute__((aligned(32)));

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
