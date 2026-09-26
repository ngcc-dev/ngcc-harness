#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../drng.h"
#include "../sign.h"

extern DRNG_ctx drng_algorithm;

#define MLEN 66
#define NTESTS 1000

int main(void)
{
    unsigned int i, j;
    int ret;
    size_t siglen;
    uint8_t m[MLEN] = {0};
    uint8_t sig[CRYPTO_SIGNATUREBYTES];
    uint8_t pk[CRYPTO_PUBLICKEYBYTES];
    uint8_t sk[CRYPTO_SECRETKEYBYTES];
    uint8_t corrupt;

    unsigned char drng_seed[48];
    memset(drng_seed, 0x55, sizeof(drng_seed));
    init_random_number(&drng_algorithm, drng_seed, sizeof(drng_seed));

    for(i = 0; i < NTESTS; ++i) {
        /* 生成随机消息 */
        get_random_number(&drng_algorithm, m, MLEN * 8);

        crypto_sign_keypair(pk, sk);

        /* 分离式签名 */
        crypto_sign_signature(sig, &siglen, m, MLEN, sk);

        /* 分离式验证 */
        ret = crypto_sign_verify(sig, siglen, m, MLEN, pk);

        if(ret) {
            fprintf(stderr, "Verification failed\n");
            return -1;
        }

        get_random_number(&drng_algorithm, (uint8_t *)&j, sizeof(j) * 8);
        do {
            get_random_number(&drng_algorithm, &corrupt, 8);
        } while(!corrupt);

        sig[j % CRYPTO_SIGNATUREBYTES] += corrupt;

        ret = crypto_sign_verify(sig, siglen, m, MLEN, pk);
        if(!ret) {
            fprintf(stderr, "Trivial forgeries possible\n");
            return -1;
        }
    }

    printf("All tests passed\n");
    printf("CRYPTO_PUBLICKEYBYTES = %d\n", CRYPTO_PUBLICKEYBYTES);
    printf("CRYPTO_SECRETKEYBYTES = %d\n", CRYPTO_SECRETKEYBYTES);
    printf("CRYPTO_SIGNATUREBYTES = %d\n", CRYPTO_SIGNATUREBYTES);

    return 0;
}