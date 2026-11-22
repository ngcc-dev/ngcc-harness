/* kem-28-3 driver: kem_enc and kem_dec ignore the declared input lengths.
 *
 *   ct <len>   honest key pair and encapsulation; the ciphertext is copied
 *              into a heap buffer of exactly <len> bytes ("full" = declared
 *              length) and passed to kem_dec with that length
 *   pk <len>   the public key is copied likewise and passed to kem_enc
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "KEM_AlgorithmInstance.h"
#include "drng.h"

DRNG_ctx drng_algorithm;

int main(int argc, char **argv)
{
    if (argc != 3) return 2;
    unsigned long long pkl = kem_get_pk_len_bytes(), skl = kem_get_sk_len_bytes();
    unsigned long long ssl = kem_get_ss_len_bytes(), ctl = kem_get_ct_len_bytes(), n = 0;
    int ctmode = strcmp(argv[1], "ct") == 0;
    unsigned long long full = ctmode ? ctl : pkl;
    unsigned long long len = strcmp(argv[2], "full") == 0 ? full : strtoull(argv[2], NULL, 10);
    unsigned char *pk = malloc(pkl), *sk = malloc(skl), *ct = malloc(ctl);
    unsigned char *ss = malloc(ssl), *ss2 = malloc(ssl), seed[48];
    if (!pk || !sk || !ct || !ss || !ss2 || len == 0 || len > full) return 2;

    memset(seed, 7, sizeof seed);
    init_random_number(&drng_algorithm, seed, sizeof seed);
    if (kem_keygen(pk, &pkl, sk, &skl) != 0) return 2;
    if (kem_enc(pk, pkl, ss, &ssl, ct, &ctl) != 0) return 2;

    unsigned char *buf = malloc((size_t)len);          /* exactly the declared length */
    if (!buf) return 2;
    memcpy(buf, ctmode ? ct : pk, (size_t)len);
    printf("%s_len=%llu of %llu\n", argv[1], len, full);
    fflush(stdout);
    if (ctmode) {
        int rc = kem_dec(sk, skl, buf, len, ss2, &n);
        printf("dec=%d agrees=%d\n", rc, memcmp(ss, ss2, ssl) == 0);
    } else {
        int rc = kem_enc(buf, len, ss2, &n, ct, &ctl);
        printf("enc=%d\n", rc);
    }
    return 0;
}
