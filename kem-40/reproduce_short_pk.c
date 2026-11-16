/* kem-40-7: yy_encapsulate_API does not check pk_len_bytes. A public key held
 * in a heap buffer of exactly the declared (one byte short) length is decoded
 * at its fixed length. Built with AddressSanitizer: argv[1] = "full" encaps
 * with a full-length heap key (control); "short" with pk_len - 1. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "KEM_AlgorithmInstance.h"
#include "drng.h"

DRNG_ctx drng_algorithm;

int main(int argc, char **argv)
{
    unsigned char seed[48];
    unsigned long long pkl = kem_get_pk_len_bytes(), skl = kem_get_sk_len_bytes();
    unsigned long long ssl = kem_get_ss_len_bytes(), ctl = kem_get_ct_len_bytes();
    unsigned char *pk = malloc(pkl), *sk = malloc(skl), *ct = malloc(ctl), *ss = malloc(ssl);
    unsigned long long n = pkl - (argc > 1 && !strcmp(argv[1], "short"));
    unsigned char *p2 = malloc(n);
    int rc;

    memset(seed, 5, sizeof seed);
    init_random_number(&drng_algorithm, seed, 48);
    if (kem_keygen(pk, &pkl, sk, &skl) != 0) return 2;
    memcpy(p2, pk, n);
    rc = kem_enc(p2, n, ss, &ssl, ct, &ctl);
    printf("declared=%llu rc=%d\n", n, rc);
    return 0;
}
