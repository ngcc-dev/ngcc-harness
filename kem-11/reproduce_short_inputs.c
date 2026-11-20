/* kem-11-4 driver: kem_enc and kem_dec ignore the declared input lengths.
 *
 *   ct <len>   honest key pair and encapsulation; the ciphertext is copied
 *              into a heap buffer of exactly <len> bytes (or the full length
 *              for "full") and passed to kem_dec with that length
 *   pk <len>   honest key pair; the public key is copied into a heap buffer
 *              of exactly <len> bytes and passed to kem_enc with that length
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "KEM_AlgorithmInstance.h"
#include "drng.h"

DRNG_ctx drng_algorithm;

int main(int argc, char **argv) {
    unsigned long long pkl = kem_get_pk_len_bytes(), skl = kem_get_sk_len_bytes();
    unsigned long long ssl = kem_get_ss_len_bytes(), ctl = kem_get_ct_len_bytes();
    unsigned char *pk = malloc(pkl), *sk = malloc(skl), *ct = malloc(ctl);
    unsigned char *ss = malloc(ssl), *ss2 = malloc(ssl), *buf;
    unsigned char seed[48];
    unsigned long long full, len, n;
    int ctmode, rc;

    if (argc != 3 || !pk || !sk || !ct || !ss || !ss2) return 2;
    ctmode = strcmp(argv[1], "ct") == 0;
    if (!ctmode && strcmp(argv[1], "pk") != 0) return 2;
    full = ctmode ? ctl : pkl;
    len = strcmp(argv[2], "full") == 0 ? full : strtoull(argv[2], NULL, 10);
    if (len == 0 || len > full) return 2;

    memset(seed, 7, sizeof seed);
    init_random_number(&drng_algorithm, seed, sizeof seed);
    if (kem_keygen(pk, &pkl, sk, &skl) != 0) return 2;
    if (kem_enc(pk, pkl, ss, &ssl, ct, &ctl) != 0) return 2;

    buf = malloc((size_t)len);                  /* exactly the declared length */
    if (!buf) return 2;
    memcpy(buf, ctmode ? ct : pk, (size_t)len);
    printf("%s_len=%llu of %llu\n", argv[1], len, full);
    fflush(stdout);
    if (ctmode) {
        rc = kem_dec(sk, skl, buf, len, ss2, &n);
        printf("dec=%d agrees=%d\n", rc, memcmp(ss, ss2, ssl) == 0);
    } else {
        rc = kem_enc(buf, len, ss2, &n, ct, &ctl);
        printf("enc=%d\n", rc);
    }
    free(buf); free(pk); free(sk); free(ct); free(ss); free(ss2);
    return 0;
}
