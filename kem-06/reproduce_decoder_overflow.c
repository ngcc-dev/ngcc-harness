/*
 * BRA-128 malformed-secret-key decoder-overflow witness.
 *
 * This links the submitted reference implementation directly so that
 * AddressSanitizer can identify the out-of-bounds q-polynomial store.  The
 * honest mode is the control; mutated mode changes only secret-key byte zero.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "KEM_BRA-128.h"
#include "drng.h"

DRNG_ctx drng_algorithm;

static void fail(const char *what)
{
    fprintf(stderr, "ERROR: %s\n", what);
    exit(2);
}

int main(int argc, char **argv)
{
    int mutate;
    unsigned char seed[64];
    unsigned char *pk, *sk, *ct, *ss, *ss2;
    unsigned long long pk_len = kem_get_pk_len_bytes();
    unsigned long long sk_len = kem_get_sk_len_bytes();
    unsigned long long ct_len = kem_get_ct_len_bytes();
    unsigned long long ss_len = kem_get_ss_len_bytes();
    unsigned long long ss2_len = ss_len;

    if (argc != 2 || (strcmp(argv[1], "--honest") != 0 &&
                      strcmp(argv[1], "--mutated") != 0)) {
        fprintf(stderr, "usage: %s --honest|--mutated\n", argv[0]);
        return 2;
    }
    mutate = strcmp(argv[1], "--mutated") == 0;

    for (size_t i = 0; i < sizeof seed; ++i)
        seed[i] = (unsigned char)(0x42 + 7 * i);
    if (init_random_number(&drng_algorithm, seed, sizeof seed) != 0)
        fail("DRNG initialization failed");

    pk = calloc(pk_len, 1);
    sk = calloc(sk_len, 1);
    ct = calloc(ct_len, 1);
    ss = calloc(ss_len, 1);
    ss2 = calloc(ss_len, 1);
    if (!pk || !sk || !ct || !ss || !ss2)
        fail("allocation failed");

    if (kem_keygen(pk, &pk_len, sk, &sk_len) != 0)
        fail("key generation failed");
    if (kem_enc(pk, pk_len, ss, &ss_len, ct, &ct_len) != 0)
        fail("encapsulation failed");
    if (kem_dec(sk, sk_len, ct, ct_len, ss2, &ss2_len) != 0 ||
        ss2_len != ss_len || memcmp(ss, ss2, ss_len) != 0)
        fail("honest decapsulation failed");

    puts("CONTROL honest BRA-128 decapsulation PASS");
    fflush(stdout);
    if (!mutate)
        return 0;

    sk[0] ^= 0xff;
    ss2_len = kem_get_ss_len_bytes();
    fprintf(stderr,
            "WITNESS changed only secret-key byte 0; entering decapsulation\n");
    fflush(stderr);
    (void)kem_dec(sk, sk_len, ct, ct_len, ss2, &ss2_len);
    fprintf(stderr, "NOT-CONFIRMED: mutated decapsulation returned\n");
    return 1;
}
