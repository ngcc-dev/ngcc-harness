/* sign-22-9 driver: sig_verify ignores pk_len_bytes.
 *
 *   honest <len>   key pair and signature from the submitted sig_keygen and
 *                  sig_sign; the public key is copied into a heap buffer of
 *                  exactly <len> bytes (or the full length for "full") and
 *                  passed to sig_verify with that length.
 *   keyless <len>  as above, but with no key generation: the signature is a
 *                  well-formed (c_tilde, z = 0) encoding from the submitted
 *                  pack_sig and the public key bytes are arbitrary.  Such a
 *                  signature passes decoding and the norm checks, so the
 *                  verifier reaches unpack_pk; it is then rejected.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SIG_AlgorithmInstance.h"
#include "drng.h"
#include "params.h"
#include "packing.h"

DRNG_ctx drng_algorithm;

int main(int argc, char **argv) {
    if (argc != 3) return 2;
    int honest = strcmp(argv[1], "honest") == 0;
    unsigned long long full = sig_get_pk_len_bytes();
    unsigned long long len = strcmp(argv[2], "full") == 0 ? full : strtoull(argv[2], NULL, 10);
    unsigned char *pk = malloc(full), *sk = malloc(sig_get_sk_len_bytes());
    unsigned char *sn = malloc(sig_get_sn_len_bytes());
    unsigned char m[33] = "short public key witness message";
    unsigned long long pklen = 0, sklen = 0, snlen = 0;
    if (!pk || !sk || !sn || len == 0 || len > full) return 2;

    if (honest) {
        unsigned char seed[48];
        for (size_t i = 0; i < sizeof seed; i++) seed[i] = (unsigned char)(5 * i + 3);
        init_random_number(&drng_algorithm, seed, sizeof seed);
        if (sig_keygen(pk, &pklen, sk, &sklen) != 0 || pklen != full) return 2;
        if (sig_sign(sk, sklen, m, sizeof m, sn, &snlen) != 0) return 2;
    } else {
        uint8_t ct[CTILDEBYTES];
        poly z1, zr[D_REST];
        size_t siglen = 0;
        memset(&z1, 0, sizeof z1);
        memset(zr, 0, sizeof zr);
        for (int i = 0; i < CTILDEBYTES; i++) ct[i] = (uint8_t)i;
        if (pack_sig(sn, &siglen, ct, &z1, zr) != 0) return 2;
        snlen = siglen;
        for (unsigned long long i = 0; i < full; i++) pk[i] = (unsigned char)(i * 31 + 7);
    }

    unsigned char *pkbuf = malloc((size_t)len);   /* exactly the declared length */
    if (!pkbuf) return 2;
    memcpy(pkbuf, pk, (size_t)len);
    printf("pk_len=%llu of %llu sig_len=%llu\n", len, full, snlen);
    fflush(stdout);
    int rc = sig_verify(pkbuf, len, sn, snlen, m, sizeof m);
    printf("verify=%d\n", rc);
    free(pkbuf); free(pk); free(sk); free(sn);
    return 0;
}
