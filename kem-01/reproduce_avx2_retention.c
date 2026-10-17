/* kem-01-1 AVX2 driver: exhaustive single-bit ciphertext flips against the
 * unmodified Aigis-Enc+ AVX2 sources, compiled by reproduce_avx2_retention.sh.
 * Prints one machine-readable line per parameter set. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "drng.h"
#include "KEM_AlgorithmInstance.h"

DRNG_ctx drng_algorithm;

int main(void)
{
    unsigned char seed[48];
    unsigned long long pkl, skl, ssl, ctl, l;
    unsigned char *pk, *sk, *ct, *ct2, *c, *k1, *k2, *kd;
    unsigned long i, tot = 0, keep = 0;
    int b, honest, second;

    for (i = 0; i < sizeof seed; i++)
        seed[i] = (unsigned char)(0xA5 ^ i);
    init_random_number(&drng_algorithm, seed, sizeof seed);

    pkl = kem_get_pk_len_bytes(); skl = kem_get_sk_len_bytes();
    ssl = kem_get_ss_len_bytes(); ctl = kem_get_ct_len_bytes();
    pk = malloc(pkl); sk = malloc(skl); ct = malloc(ctl); ct2 = malloc(ctl);
    c = malloc(ctl); k1 = malloc(ssl); k2 = malloc(ssl); kd = malloc(ssl);
    if (!pk || !sk || !ct || !ct2 || !c || !k1 || !k2 || !kd)
        return 2;

    if (kem_keygen(pk, &l, sk, &l) || kem_enc(pk, pkl, k1, &l, ct, &l))
        return 2;
    memset(kd, 0, ssl);
    kem_dec(sk, skl, ct, ctl, kd, &l);
    honest = memcmp(kd, k1, ssl) == 0;

    if (kem_enc(pk, pkl, k2, &l, ct2, &l))
        return 2;
    second = memcmp(k2, k1, ssl) != 0;

    for (i = 0; i < ctl; i++) {
        for (b = 0; b < 8; b++) {
            memcpy(c, ct, ctl);
            c[i] ^= (unsigned char)(1u << b);
            memset(kd, 0, ssl);
            kem_dec(sk, skl, c, ctl, kd, &l);
            tot++;
            keep += memcmp(kd, k1, ssl) == 0;
        }
    }
    printf("ct=%llu honest=%d second_differs=%d keep=%lu total=%lu\n",
           ctl, honest, second, keep, tot);
    return 0;
}
