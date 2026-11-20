/* sign-24-5 driver: sig_deserialize_signature checks too short a length.
 *
 *   bounds          print the entry bound of sig_deserialize_signature and
 *                   the fixed prefix it reads before its second length check
 *   keyless <len>   arbitrary public-key bytes of the correct length and an
 *                   arbitrary signature in a heap buffer of exactly <len>
 *                   bytes, passed to sig_verify with that length
 *   honest <len>    key pair and signature from the submitted sig_keygen and
 *                   sig_sign; the first <len> bytes of the signature (or all
 *                   of it for "full") are copied into a heap buffer of exactly
 *                   that size and passed to sig_verify with that length
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SIG_AlgorithmInstance.h"
#include "sig_core.h"
#include "drng.h"

DRNG_ctx drng_algorithm;

int main(int argc, char **argv) {
    const unsigned long long entry = 2ULL * RAW_POLY_BYTES +
        (unsigned long long)total_groups * HASH_DIGEST_LENGTH + 4ULL;
    const unsigned long long prefix = (unsigned long long)total_groups * HASH_DIGEST_LENGTH +
        (2ULL + 2ULL * SIGMA_REPETITIONS + 6ULL + N_PRIME_LEN) * RAW_POLY_BYTES + 4ULL;
    unsigned long long pklen = sig_get_pk_len_bytes(), sklen = sig_get_sk_len_bytes();
    unsigned long long snmax = sig_get_sn_len_bytes(), snlen = 0, len;
    unsigned char m[34] = "short signature witness message";
    unsigned char *pk, *sk, *sn, *buf;
    int rc;

    if (argc == 2 && strcmp(argv[1], "bounds") == 0) {
        printf("entry=%llu prefix=%llu max=%llu\n", entry, prefix, snmax);
        return 0;
    }
    if (argc != 3) return 2;
    pk = malloc(pklen); sk = malloc(sklen); sn = malloc(snmax);
    if (!pk || !sk || !sn) return 2;

    if (strcmp(argv[1], "honest") == 0) {
        unsigned char seed[64];
        for (size_t i = 0; i < sizeof seed; i++) seed[i] = (unsigned char)(5 * i + 3);
        init_random_number(&drng_algorithm, seed, sizeof seed);
        if (sig_keygen(pk, &pklen, sk, &sklen) != 0) return 2;
        snlen = snmax;
        if (sig_sign(sk, sklen, m, sizeof m, sn, &snlen) != 0) return 2;
    } else if (strcmp(argv[1], "keyless") == 0) {
        for (unsigned long long i = 0; i < pklen; i++) pk[i] = (unsigned char)(31 * i + 7);
        for (unsigned long long i = 0; i < snmax; i++) sn[i] = (unsigned char)(17 * i + 1);
        snlen = snmax;
    } else {
        return 2;
    }
    len = strcmp(argv[2], "full") == 0 ? snlen : strtoull(argv[2], NULL, 10);
    if (len == 0 || len > snlen) return 2;

    buf = malloc((size_t)len);                  /* exactly the declared length */
    if (!buf) return 2;
    memcpy(buf, sn, (size_t)len);
    printf("sn_len=%llu of %llu\n", len, snlen);
    fflush(stdout);
    rc = sig_verify(pk, pklen, buf, len, m, sizeof m);
    printf("verify=%d\n", rc);
    free(buf); free(pk); free(sk); free(sn);
    return 0;
}
