/* sign-23: Shuttle sig_verify discards pk_len_bytes (SIG_AlgorithmInstance.c)
 * and unpack_pk (packing.c) reads the fixed CRYPTO_PUBLICKEYBYTES layout.
 * A key pair and a signature are generated; the public key is copied into a
 * heap buffer of exactly argv[1] bytes ("short": CRYPTO_PUBLICKEYBYTES - 1,
 * "full": CRYPTO_PUBLICKEYBYTES) and verified with that declared length.
 * Built with AddressSanitizer.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "drng.h"

DRNG_ctx drng_algorithm;
unsigned long long sig_get_pk_len_bytes(void), sig_get_sk_len_bytes(void), sig_get_sn_len_bytes(void);
int sig_keygen(unsigned char *, unsigned long long *, unsigned char *, unsigned long long *);
int sig_sign(unsigned char *, unsigned long long, unsigned char *, unsigned long long,
             unsigned char *, unsigned long long *);
int sig_verify(unsigned char *, unsigned long long, unsigned char *, unsigned long long,
               unsigned char *, unsigned long long);

int main(int argc, char **argv)
{
    unsigned long long PL = sig_get_pk_len_bytes(), SL = sig_get_sk_len_bytes(), NL = sig_get_sn_len_bytes(), a, b;
    unsigned char *pk = malloc(PL), *sk = malloc(SL), *sig = malloc(NL), m[33], seed[48];
    for (int i = 0; i < 48; i++) seed[i] = (unsigned char)(19 * i + 1);
    for (int i = 0; i < 33; i++) m[i] = (unsigned char)(3 * i);
    init_random_number(&drng_algorithm, seed, 48);
    if (sig_keygen(pk, &a, sk, &b) || sig_sign(sk, SL, m, sizeof m, sig, &a)) return 3;
    unsigned long long dl = (argc > 1 && !strcmp(argv[1], "short")) ? PL - 1 : PL;
    unsigned char *t = malloc(dl);
    memcpy(t, pk, dl);
    printf("pk=%llu declared=%llu\n", PL, dl);
    fflush(stdout);
    int rc = sig_verify(t, dl, sig, a, m, sizeof m);
    printf("verify rc=%d\n", rc);
    free(t);
    return 0;
}
