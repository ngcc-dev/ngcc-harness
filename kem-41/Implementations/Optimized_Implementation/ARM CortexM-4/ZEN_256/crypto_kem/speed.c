#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "hal.h"
#include "sendfn.h"
#include "KEM_AlgorithmInstance.h"
#ifdef USE_KECCAK
#include "randombytes.h"
#else
#include "drng.h"
DRNG_ctx drng_algorithm;
#endif


static const unsigned char speed_seed[] = {
    0x6e, 0x67, 0x63, 0x63, 0x6d, 0x34, 0x2d, 0x73,
    0x70, 0x65, 0x65, 0x64, 0x2d, 0x73, 0x65, 0x65, 0x64
};

static void fail_and_halt(const char *reason) {
    hal_send_str(reason);
#ifdef MPS2_AN386
    __builtin_trap();
#endif
    while (1) {
    }
}

int main(void) {
    unsigned long long pk_len = kem_get_pk_len_bytes();
    unsigned long long sk_len = kem_get_sk_len_bytes();
    unsigned long long ss_len = kem_get_ss_len_bytes();
    unsigned long long ct_len = kem_get_ct_len_bytes();
    unsigned long long ignored_len = 0;
    uint64_t t0;
    uint64_t t1;
    uint64_t keygen_cycles = 0;
    uint64_t enc_cycles = 0;
    uint64_t dec_cycles = 0;
    unsigned char *pk;
    unsigned char *sk;
    unsigned char *ct;
    unsigned char *ss_enc;
    unsigned char *ss_dec;
    unsigned int i;

    hal_setup(CLOCK_BENCHMARK);
    hal_send_str("==========================");
#ifndef USE_KECCAK
    if (init_random_number(&drng_algorithm, speed_seed, sizeof(speed_seed)) != 0)
    {
        fail_and_halt("drng_init_failed");
    }
#endif
   

    pk = malloc(pk_len);
    sk = malloc(sk_len);
    ct = malloc(ct_len);
    ss_enc = malloc(ss_len);
    ss_dec = malloc(ss_len);
    if (pk == NULL || sk == NULL || ct == NULL || ss_enc == NULL || ss_dec == NULL) {
        fail_and_halt("alloc_failed");
    }

    for (i = 0; i < NGCC_ITERATIONS; i++) {
        t0 = hal_get_time();
        if (kem_keygen(pk, &ignored_len, sk, &ignored_len) != 0) {
            fail_and_halt("kem_keygen_failed");
        }
        t1 = hal_get_time();
        send_unsignedll("keypair cycles:", (unsigned long long)(t1 - t0));

        t0 = hal_get_time();
        if (kem_enc(pk, pk_len, ss_enc, &ignored_len, ct, &ignored_len) != 0) {
            fail_and_halt("kem_enc_failed");
        }
        t1 = hal_get_time();
        enc_cycles += t1 - t0;
        send_unsignedll("encaps cycles:", (unsigned long long)(t1 - t0));

        t0 = hal_get_time();
        if (kem_dec(sk, sk_len, ct, ct_len, ss_dec, &ignored_len) != 0) {
            fail_and_halt("kem_dec_failed");
        }
        t1 = hal_get_time();
        dec_cycles += t1 - t0;
        send_unsignedll("decaps cycles:", (unsigned long long)(t1 - t0));
        if (memcmp(ss_enc, ss_dec, ss_len) != 0) {
            fail_and_halt("ERROR KEYS");
        }
        
        hal_send_str("OK KEYS");
        hal_send_str("+");
    }
    free(pk);
    free(sk);
    free(ct);
    free(ss_enc);
    free(ss_dec);
    hal_send_str("#");
    return 0;
}
