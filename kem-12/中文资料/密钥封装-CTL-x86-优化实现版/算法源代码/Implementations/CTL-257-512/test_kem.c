#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "KEM_AlgorithmInstance.h"
#include "drng.h"

DRNG_ctx drng_algorithm;

int main(void) {
    uint8_t pk[2000], sk[3000], ct[1500], ss[32], ss2[32];
    unsigned long long pk_len, sk_len, ct_len, ss_len, ss2_len;
    uint8_t seed[55];
    int rtn;

    printf("=== KEM Test for CTL-257-512 ===\n\n");

    // Initialize DRNG
    printf("1. Initializing DRNG...\n");
    memset(seed, 0xAA, 55);
    rtn = init_random_number(&drng_algorithm, seed, 55);
    if (rtn != 0) {
        printf("FAILED (code=%d)\n", rtn);
        return 1;
    }
    printf("OK\n\n");

    // Get expected lengths
    printf("2. Getting expected lengths...\n");
    unsigned long long exp_pk_len = kem_get_pk_len_bytes();
    unsigned long long exp_sk_len = kem_get_sk_len_bytes();
    unsigned long long exp_ct_len = kem_get_ct_len_bytes();
    unsigned long long exp_ss_len = kem_get_ss_len_bytes();
    printf("   pk_len=%llu, sk_len=%llu, ct_len=%llu, ss_len=%llu\n", 
           exp_pk_len, exp_sk_len, exp_ct_len, exp_ss_len);
    printf("OK\n\n");

    // Key generation
    printf("3. Key generation...\n");
    rtn = kem_keygen(pk, &pk_len, sk, &sk_len);
    if (rtn != 0) {
        printf("FAILED (code=%d)\n", rtn);
        return 1;
    }
    printf("   pk_len=%llu, sk_len=%llu\n", pk_len, sk_len);
    printf("OK\n\n");

    // Encapsulation
    printf("4. Encapsulation...\n");
    rtn = kem_enc(pk, pk_len, ss, &ss_len, ct, &ct_len);
    if (rtn != 0) {
        printf("FAILED (code=%d)\n", rtn);
        return 1;
    }
    printf("   ss_len=%llu, ct_len=%llu\n", ss_len, ct_len);
    printf("OK\n\n");

    // Decapsulation
    printf("5. Decapsulation...\n");
    rtn = kem_dec(sk, sk_len, ct, ct_len, ss2, &ss2_len);
    if (rtn != 0) {
        printf("FAILED (code=%d)\n", rtn);
        return 1;
    }
    printf("   ss_len=%llu\n", ss2_len);
    printf("OK\n\n");

    // Compare secrets
    printf("6. Comparing secrets...\n");
    if (memcmp(ss, ss2, 16) == 0) {
        printf("OK\n\n");
    } else {
        printf("FAILED - secrets differ\n");
        return 1;
    }

    printf("=== All tests passed! ===\n");
    return 0;
}