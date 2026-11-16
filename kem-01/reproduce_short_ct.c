/* kem-01-6 and kem-01-7: kem_dec ignores ct_len_bytes and parses the ciphertext at its
 * fixed length. A ciphertext held in a heap buffer of exactly the declared
 * (one byte short) length is read out of bounds. Built with AddressSanitizer:
 * argv[1] = "full" decapsulates a full-length ciphertext in an exactly sized
 * heap buffer; "short" uses ct_len - 1 in a buffer of that length; "padded"
 * passes the full length in a buffer 16 bytes larger. The secret key is placed 64 bytes into its heap
 * block so that the separately filed read before the key object (kem-01-2)
 * does not stop AddressSanitizer first. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "KEM_AlgorithmInstance.h"
#include "drng.h"

DRNG_ctx drng_algorithm;

int main(int argc, char **argv)
{
    unsigned char seed[80];
    unsigned long long pkl = kem_get_pk_len_bytes(), skl = kem_get_sk_len_bytes();
    unsigned long long ssl = kem_get_ss_len_bytes(), ctl = kem_get_ct_len_bytes();
    unsigned char *pk = malloc(pkl), *sk = (unsigned char *)malloc(64 + skl) + 64, *ct = malloc(ctl);
    unsigned char *ss = malloc(ssl), *ss2 = malloc(ssl);
    const char *mode = argc > 1 ? argv[1] : "full";
    unsigned long long n = ctl - !strcmp(mode, "short");
    unsigned char *c2 = malloc(n + 16 * !strcmp(mode, "padded"));
    int rc;

    memset(seed, 5, sizeof seed);
    init_random_number(&drng_algorithm, seed, 48);
    if (kem_keygen(pk, &pkl, sk, &skl) != 0) return 2;
    kem_enc(pk, pkl, ss, &ssl, ct, &ctl);
    memcpy(c2, ct, n);
    rc = kem_dec(sk, skl, c2, n, ss2, &ssl);
    printf("%s: declared=%llu rc=%d agrees=%d\n", mode, n, rc, !memcmp(ss, ss2, ssl));
    return 0;
}
