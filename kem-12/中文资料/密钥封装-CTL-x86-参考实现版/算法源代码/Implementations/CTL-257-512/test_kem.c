#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "KEM_AlgorithmInstance.h"
#include "drng.h"
#include "ctl.h"

DRNG_ctx drng_algorithm;

int main() {
    printf("Test 1: Simple print\n");
    
    unsigned char *pk = (unsigned char *)malloc(1000);
    unsigned char *sk = (unsigned char *)malloc(3000);
    unsigned char *ct = (unsigned char *)malloc(500);
    unsigned char ss[64], ss2[64];
    unsigned long long pk_len, sk_len, ct_len, ss_len, ss2_len;
    unsigned char seed[55];
    int rtn;

    printf("Test 2: Memory allocation OK\n");

    memset(seed, 0xAA, 55);
    
    printf("Test 3: Initializing DRNG...\n");
    rtn = init_random_number(&drng_algorithm, seed, 55);
    if (rtn != 0) {
        printf("FAILED (code=%d)\n", rtn);
        return 1;
    }
    printf("OK\n");

    printf("Test 4: Getting expected lengths...\n");
    unsigned long long exp_pk_len = kem_get_pk_len_bytes();
    unsigned long long exp_sk_len = kem_get_sk_len_bytes();
    unsigned long long exp_ct_len = kem_get_ct_len_bytes();
    unsigned long long exp_ss_len = kem_get_ss_len_bytes();
    printf("   Expected: pk_len=%llu, sk_len=%llu, ct_len=%llu, ss_len=%llu\n", 
           exp_pk_len, exp_sk_len, exp_ct_len, exp_ss_len);
    printf("OK\n");

    printf("Test 5: Key generation...\n");
    rtn = kem_keygen(pk, &pk_len, sk, &sk_len);
    if (rtn != 0) {
        printf("FAILED (code=%d)\n", rtn);
        return 1;
    }
    printf("   Actual: pk_len=%llu, sk_len=%llu\n", pk_len, sk_len);
    printf("OK\n");

    printf("Test 6: Encapsulation...\n");
    rtn = kem_enc(pk, pk_len, ss, &ss_len, ct, &ct_len);
    if (rtn != 0) {
        printf("FAILED (code=%d)\n", rtn);
        return 1;
    }
    printf("   Actual: ss_len=%llu, ct_len=%llu\n", ss_len, ct_len);
    printf("OK\n");

    printf("Test 7: Decapsulation...\n");
    rtn = kem_dec(sk, sk_len, ct, ct_len, ss2, &ss2_len);
    if (rtn != 0) {
        printf("FAILED (code=%d)\n", rtn);
        printf("   Debug info:\n");
        printf("   - sk_len=%llu\n", sk_len);
        printf("   - ct_len=%llu\n", ct_len);
        return 1;
    }
    printf("   Actual: ss_len=%llu\n", ss2_len);
    printf("OK\n");

    printf("Test 8: Comparing secrets...\n");
    if (memcmp(ss, ss2, ss_len) == 0) {
        printf("OK\n");
    } else {
        printf("FAILED - secrets differ\n");
        return 1;
    }

    free(pk);
    free(sk);
    free(ct);

    printf("All tests passed!\n");
    return 0;
}