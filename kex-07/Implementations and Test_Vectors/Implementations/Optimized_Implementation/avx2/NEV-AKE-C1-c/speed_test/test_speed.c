#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../owpke.h"
#include "../cca.h"
#include "../cpa.h"
#include "../api.h"
#include "../poly.h"
#include "../randombytes.h"
#include "cpucycles.h"
#include "speed_print.h"
#include "../sample.h"
#include "../ake.h"
#include "../owkem.h"

#define NTESTS 30000

uint64_t t[NTESTS];

int main()
{
    unsigned int i;
    unsigned char pki[CRYPTO_PUBLICKEYBYTES + 32] = {0};
    unsigned char ski[CRYPTO_SECRETKEYBYTES + 32] = {0};
    unsigned char pkj[CRYPTO_PUBLICKEYBYTES + 32] = {0};
    unsigned char skj[CRYPTO_SECRETKEYBYTES + 32] = {0};
    unsigned char M[AKE_M_BYTES] = {0};
    unsigned char st[AKE_ST_BYTES] = {0};
    unsigned char M_prime[AKE_M_BYTES] = {0};
    unsigned char ct[CRYPTO_CIPHERTEXTBYTES + 32] = {0};
    unsigned char key[CRYPTO_BYTES] = {0};
    unsigned char K[SEED_BYTES] = {0};
    unsigned char K_prime[SEED_BYTES] = {0};
    unsigned char idj[SEED_BYTES] = {0};
    unsigned char idi[SEED_BYTES] = {0};
    unsigned char mu[CRYPTO_BYTES] = {0};
    uint64_t t0[NTESTS], t1[NTESTS], t2[NTESTS], t3[NTESTS];

    printf("*****************************\n");
    printf("PK Sizes:%d\n",CRYPTO_PUBLICKEYBYTES);
    printf("SK Sizes:%d\n",CRYPTO_SECRETKEYBYTES);
    printf("CT Sizes:%d\n",CRYPTO_CIPHERTEXTBYTES);
    printf("TOTAL Sizes:%d\n",CRYPTO_PUBLICKEYBYTES + CRYPTO_CIPHERTEXTBYTES);
    printf("*****************************\n");


    for (i = 0; i < NTESTS; i++) {
        t0[i] = cpucycles();
        ake_keygen(pki, ski, pkj, skj);
        t0[i] =cpucycles() - t0[i];

        t1[i] = cpucycles();
        ake_init(M, st, ski, pkj);
        t1[i] =cpucycles() - t1[i];

        t2[i] = cpucycles();
        ake_der_response(M_prime, K_prime, idi, idj, skj, pki, M);
        t2[i] =cpucycles() - t2[i];

        t3[i] = cpucycles();
        ake_der_init(K, idi, idj, ski, pkj, st, M_prime);
        t3[i] =cpucycles() - t3[i];
    }
    print_results("ake_keygen: ", t0, NTESTS);
    print_results("ake_init: ", t1, NTESTS);
    print_results("ake_der_response: ", t2, NTESTS);
    print_results("ake_der_init: ", t3, NTESTS);

    return 0;
}


