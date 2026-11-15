/* kex-03-11: decapsulate invalid K2K-PLAC512 ciphertexts and compare their
 * implicit-rejection keys. Two different ciphertexts that agree on bytes
 * 0..583 must give the same key; flipping byte 583 must change it. */
#include <stdio.h>
#include <string.h>
#include "twokem.h"
#include "cretake_params.h"
#include "drng.h"

DRNG_ctx drng_algorithm;

int main(void)
{
    static unsigned char pk1[8192], sk1[16384], pk2[8192], sk2[16384];
    static unsigned char ct[16384], ct2[16384], ct3[16384];
    unsigned char ss[64], d0[64], d1[64], d2[64], d3[64], seed[64];
    unsigned long long pk1l, sk1l, pk2l, sk2l, ctl, ssl, l;

    memset(seed, 7, sizeof seed);
    init_random_number(&drng_algorithm, seed, 64);
    twokem_keygen1(pk1, &pk1l, sk1, &sk1l);
    twokem_keygen2(pk2, &pk2l, sk2, &sk2l);
    twokem_enc(pk1, pk1l, pk2, pk2l, ss, &ssl, ct, &ctl);
    twokem_dec(sk1, sk1l, sk2, sk2l, pk2, pk2l, ct, ctl, d0, &l);
    memcpy(ct2, ct, ctl);
    memcpy(ct3, ct, ctl);
    for (unsigned long long i = 584; i < ctl; i++) {
        ct2[i] ^= 0x5a;
        ct3[i] = (unsigned char)(ct3[i] * 7 + i + 1);
    }
    twokem_dec(sk1, sk1l, sk2, sk2l, pk2, pk2l, ct2, ctl, d1, &l);
    twokem_dec(sk1, sk1l, sk2, sk2l, pk2, pk2l, ct3, ctl, d2, &l);
    int distinct = memcmp(ct2, ct3, ctl) != 0;
    memcpy(ct3, ct2, ctl);
    ct3[583] ^= 1;
    twokem_dec(sk1, sk1l, sk2, sk2l, pk2, pk2l, ct3, ctl, d3, &l);
    printf("ct_bytes=%llu honest_ok=%d distinct_ciphertexts=%d same_reject_key=%d "
           "rejected=%d byte583_changes_key=%d\n", ctl, !memcmp(ss, d0, 64), distinct,
           !memcmp(d1, d2, 64), memcmp(d1, d0, 64) != 0, memcmp(d1, d3, 64) != 0);
    return 0;
}
