/* kem-15-4 and kem-15-5: FLIT AVX2 self-decapsulation failures.
 *
 * Calls only the archived API (kem_keygen, kem_enc, kem_dec) and seeds the
 * algorithm DRNG exactly as the archived KAT_KEM.c does: the 64-byte nonce
 * "seed"x16 seeds a seed DRNG, and each iteration draws a fresh 64-byte seed
 * for the algorithm DRNG.  A round trip fails when the encapsulated and
 * decapsulated shared secrets differ; return codes are not used as evidence.
 *
 * Usage: reproduce_avx2_roundtrip N
 * Prints: runs=N fails=F first=I enc_wraps=E dec_wraps=D
 *         fail_without_enc_wrap=X dec_wrap_fails=Y
 * The wrap counters count round trips in which the instrumented inverse NTT
 * wrapped during encapsulation or decapsulation; they are zero otherwise. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "drng.h"
#include "KEM_AlgorithmInstance.h"

DRNG_ctx drng_algorithm;
static DRNG_ctx drng_seed;
extern long ngcc_overflow __attribute__((weak));

static uint8_t pk[8192], sk[8192], ct[8192], ss_enc[64], ss_dec[64];

static long overflow_count(void)
{
    return &ngcc_overflow ? ngcc_overflow : 0;
}

static int enc_wrap, dec_wrap;

static int round_trip(const uint8_t seed[64])
{
    unsigned long long pk_n = kem_get_pk_len_bytes(), sk_n = kem_get_sk_len_bytes();
    unsigned long long ct_n = kem_get_ct_len_bytes(), ss_n = kem_get_ss_len_bytes();
    unsigned long long ss2_n = ss_n;

    if (ss_n > sizeof ss_enc || pk_n > sizeof pk || sk_n > sizeof sk || ct_n > sizeof ct)
        exit(2);
    init_random_number(&drng_algorithm, seed, 64);
    kem_keygen(pk, &pk_n, sk, &sk_n);
    long w0 = overflow_count();
    kem_enc(pk, pk_n, ss_enc, &ss_n, ct, &ct_n);
    long w1 = overflow_count();
    kem_dec(sk, sk_n, ct, ct_n, ss_dec, &ss2_n);
    enc_wrap = w1 != w0;
    dec_wrap = overflow_count() != w1;
    return memcmp(ss_enc, ss_dec, ss_n) == 0;
}

int main(int argc, char **argv)
{
    long n = argc > 1 ? atol(argv[1]) : 12000, fails = 0, first = -1;
    long enc_wraps = 0, dec_wraps = 0, fail_without_enc_wrap = 0, dec_wrap_fails = 0;
    uint8_t nonce[64], seed[64];

    for (int i = 0; i < 16; i++)
        memcpy(nonce + 4 * i, "seed", 4);
    if (init_random_number(&drng_seed, nonce, 64) != 0)
        return 2;
    for (long i = 0; i < n; i++) {
        get_random_number(&drng_seed, seed, 64 * 8);
        int ok = round_trip(seed);
        enc_wraps += enc_wrap;
        dec_wraps += dec_wrap;
        if (!ok) {
            if (first < 0)
                first = i;
            fails++;
            fail_without_enc_wrap += !enc_wrap;
            dec_wrap_fails += dec_wrap;
        }
    }
    printf("runs=%ld fails=%ld first=%ld enc_wraps=%ld dec_wraps=%ld "
           "fail_without_enc_wrap=%ld dec_wrap_fails=%ld\n",
           n, fails, first, enc_wraps, dec_wraps, fail_without_enc_wrap, dec_wrap_fails);
    return 0;
}
