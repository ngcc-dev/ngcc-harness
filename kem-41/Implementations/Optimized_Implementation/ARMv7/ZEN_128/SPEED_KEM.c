#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "params.h"
#include "drng.h"
#include "auxfunc.h"
#include "ntt.h"
#include "poly.h"
#include "pke.h"
#include "KEM_AlgorithmInstance.h"
#include "cpucycles.h"

#define NTESTS 1000

uint64_t t[NTESTS];
uint8_t seed[SEED_LEN_BYTES] = {0};

DRNG_ctx drng_algorithm;

int main()
{
    unsigned int i;
    unsigned char *ss, *ss1, *ct, *pk, *sk;
    unsigned long long pk_len_bytes, sk_len_bytes, ss_len_bytes, ct_len_bytes;

    // ===== 初始化性能计数器 =====
    cpucycles_init();
    // ===== 初始化结束 =====

    pk = (unsigned char *)calloc(ZEN_PUBLICKEY_LEN_BYTES, sizeof(unsigned char));
    sk = (unsigned char *)calloc(ZEN_SECREKEY_LEN_BYTES, sizeof(unsigned char));
    ss = (unsigned char *)calloc(ZEN_SHAREDKEY_LEN_BYTES, sizeof(unsigned char));
    ss1 = (unsigned char *)calloc(ZEN_SHAREDKEY_LEN_BYTES, sizeof(unsigned char));
    ct = (unsigned char *)calloc(ZEN_CIPHERTEXT_LEN_BYTES, sizeof(unsigned char));
    
    printf("Running %d iterations.\n", NTESTS);
    for(i = 0; i < NTESTS; i++) 
    {
        t[i] = cpucycles();
        kem_keygen(pk, &pk_len_bytes, sk, &sk_len_bytes);
    }
    print_results("kem_keygen: ", t, NTESTS);

    kem_keygen(pk, &pk_len_bytes, sk, &sk_len_bytes);
    for(i = 0; i < NTESTS; i++) 
    {
        t[i] = cpucycles();
	    kem_enc(pk, pk_len_bytes, ss, &ss_len_bytes, ct, &ct_len_bytes);
    }
    print_results("kem_enc: ", t, NTESTS);

    kem_keygen(pk, &pk_len_bytes, sk, &sk_len_bytes);
    kem_enc(pk, pk_len_bytes, ss, &ss_len_bytes, ct, &ct_len_bytes);
    for(i = 0; i < NTESTS; i++) 
    {
        t[i] = cpucycles();
	    kem_dec(sk, sk_len_bytes, ct, ct_len_bytes, ss1, &ss_len_bytes);
    }
    print_results("kem_dec: ", t, NTESTS);

    // ===== 清理性能计数器 =====
    cpucycles_cleanup();
    // ===== 清理结束 =====

    return 0;
}