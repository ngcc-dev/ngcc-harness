/* sign-06-9: sig_verify and sig_sign discard pk_len_bytes and sk_len_bytes.
 * argv[1] = "pk" | "sk" | "control". Copies an honest key into a heap buffer
 * of exactly the declared length (one byte short, or full for the control)
 * and calls the submitted API; AddressSanitizer reports the read past it. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SIG_AlgorithmInstance.h"
#include "drng.h"
DRNG_ctx drng_algorithm;
int main(int argc, char **argv)
{
    unsigned long long pkl = sig_get_pk_len_bytes(), skl = sig_get_sk_len_bytes(), snl = sig_get_sn_len_bytes();
    unsigned char *pk = malloc(pkl), *sk = malloc(skl), *sn = malloc(snl), m[32] = {1};
    unsigned long long a, b, l;
    uint8_t seed[48] = {7};
    init_random_number(&drng_algorithm, seed, 48);
    sig_keygen(pk, &a, sk, &b);
    sig_sign(sk, skl, m, 32, sn, &l);
    int mode = argv[1][0];
    unsigned long long decl = mode == 'p' ? pkl - 1 : mode == 's' ? skl - 1 : pkl;
    unsigned char *k = malloc(decl);
    memcpy(k, mode == 's' ? sk : pk, decl);
    int rc = mode == 's' ? sig_sign(k, decl, m, 32, sn, &l) : sig_verify(k, decl, sn, l, m, 32);
    printf("mode=%s declared=%llu rc=%d\n", argv[1], decl, rc);
    return 0;
}
