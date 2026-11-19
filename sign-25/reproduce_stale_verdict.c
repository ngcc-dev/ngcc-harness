/* sign-25-1: verdicts of the SQIsign2D2 verifier depend on stale stack state.
 *
 * Usage: reproduce_stale_verdict flip fresh|primed|scrubbed
 *        reproduce_stale_verdict msg
 * Both modes generate a key pair from a fixed DRBG seed and sign a fixed
 * 32-byte message m.
 *   flip: optionally verifies the unmodified signature first (primed) or
 *         overwrites 128 KiB of stack first (scrubbed), then flips one bit in
 *         the middle byte of the signature and prints the verdict.
 *   msg:  verifies the signature on m against another message m2 in a fresh
 *         state, then after an honest verification of an unrelated signature,
 *         then verifies an all-zero signature on m2 in that state.
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

__attribute__((noinline)) static void scrub(void)
{
    volatile unsigned char junk[1 << 17];
    for (size_t i = 0; i < sizeof junk; i++)
        junk[i] = (unsigned char)(i * 131 + 7);
}

int main(int argc, char **argv)
{
    unsigned long long pl = sig_get_pk_len_bytes(), sl = sig_get_sk_len_bytes(),
                       nl = sig_get_sn_len_bytes(), a, b;
    unsigned char *pk = malloc(pl), *sk = malloc(sl), *sig = malloc(nl), *sig2 = malloc(nl),
                  *zero = calloc(1, nl), m[32], m2[32], m3[32], seed[48];
    if (argc < 2 || !pk || !sk || !sig || !sig2 || !zero)
        return 2;
    for (int i = 0; i < 48; i++) seed[i] = (unsigned char)(17 * i + 3);
    for (int i = 0; i < 32; i++) {
        m[i] = (unsigned char)i;
        m2[i] = (unsigned char)(i ^ 0xa5);
        m3[i] = (unsigned char)(i * 7 + 1);
    }
    init_random_number(&drng_algorithm, seed, 48);
    if (sig_keygen(pk, &a, sk, &b) != 0 || sig_sign(sk, sl, m, 32, sig, &a) != 0)
        return 3;
    if (!strcmp(argv[1], "flip") && argc == 3) {
        int honest = 99;
        if (!strcmp(argv[2], "primed"))
            honest = sig_verify(pk, pl, sig, nl, m, 32);
        else if (!strcmp(argv[2], "scrubbed"))
            scrub();
        sig[nl / 2] ^= 1;
        printf("flip %s honest=%d flipped=%d\n", argv[2], honest, sig_verify(pk, pl, sig, nl, m, 32));
        return 0;
    }
    if (!strcmp(argv[1], "msg")) {
        if (sig_sign(sk, sl, m3, 32, sig2, &a) != 0)
            return 3;
        int fresh = sig_verify(pk, pl, sig, nl, m2, 32);
        int honest = sig_verify(pk, pl, sig2, nl, m3, 32);
        int other = sig_verify(pk, pl, sig, nl, m2, 32);
        int honest2 = sig_verify(pk, pl, sig2, nl, m3, 32);
        int zeros = sig_verify(pk, pl, zero, nl, m2, 32);
        printf("msg fresh_other=%d honest=%d primed_other=%d honest=%d primed_zero=%d\n",
               fresh, honest, other, honest2, zeros);
        return 0;
    }
    return 2;
}
