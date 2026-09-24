#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include "hal.h"
#include "sendfn.h"
#include "KEM_AlgorithmInstance.h"
#if defined(__has_include)
#if __has_include("parameters.h")
#include "parameters.h"
#endif
#if __has_include("ntt.h")
#include "ntt.h"
#endif
#endif
#ifdef USE_KECCAK
#include "randombytes.h"
#else
#include "drng.h"
DRNG_ctx drng_algorithm;
#endif
static const unsigned char test_seed[] = {
    0x6e, 0x67, 0x63, 0x63, 0x6d, 0x34, 0x2d, 0x74,
    0x65, 0x73, 0x74, 0x2d, 0x73, 0x65, 0x65, 0x64
};

static void fail_and_halt(const char *reason) {
    hal_send_str(reason);
#ifdef MPS2_AN386
    __builtin_trap();
#endif
    while (1) {
    }
}

static int test_roundtrip(void) {
    unsigned long long pk_len = kem_get_pk_len_bytes();
    unsigned long long sk_len = kem_get_sk_len_bytes();
    unsigned long long ss_len = kem_get_ss_len_bytes();
    unsigned long long ct_len = kem_get_ct_len_bytes();
    unsigned long long ignored_len = 0;
    unsigned char *pk = malloc(pk_len);
    unsigned char *sk = malloc(sk_len);
    unsigned char *ct = malloc(ct_len);
    unsigned char *ss_a = malloc(ss_len);
    unsigned char *ss_b = malloc(ss_len);

    if (pk == NULL || sk == NULL || ct == NULL || ss_a == NULL || ss_b == NULL) {
        return -1;
    }
    if (kem_keygen(pk, &ignored_len, sk, &ignored_len) != 0) {
        return -1;
    }
    if (kem_enc(pk, pk_len, ss_a, &ignored_len, ct, &ignored_len) != 0) {
        return -1;
    }
    if (kem_dec(sk, sk_len, ct, ct_len, ss_b, &ignored_len) != 0) {
        return -1;
    }
    int ret = memcmp(ss_a, ss_b, ss_len);

    free(pk);
    free(sk);
    free(ct);
    free(ss_a);
    free(ss_b);
    return ret;
}

int main(void) {
    int i;

    hal_setup(CLOCK_FAST);
    hal_send_str("==========================");

#ifndef USE_KECCAK
    if (init_random_number(&drng_algorithm, test_seed, sizeof(test_seed)) != 0) {
        fail_and_halt("drng_init_failed");
    }
#endif

    for (i = 0; i < NGCC_ITERATIONS; i++) {
        if (test_roundtrip() != 0) {
            hal_send_str("ERROR KEYS");
            return -1;
        }
        else{
            hal_send_str("OK KEYS");
        }        
        hal_send_str("+");
    }

    hal_send_str("#");
    return 0;
}
