/*
 * sign-15-15: with an attacker-chosen key (rho, t1 = 0) and signature
 * (z, h, c) = (0, 0, c), verification reduces to c == challenge(mu, 0).
 * The program builds such a signature for one message, checks that the
 * unmodified verifier accepts it, and checks that a second message rejects.
 * A collision of challenge(mu(M1), 0) and challenge(mu(M2), 0) would make the
 * same signature verify both messages (MBS); the generic search cost is set
 * by the size of the challenge image, printed by the wrapper.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "params.h"
#include "polyvec.h"
#include "packing.h"
#include "auxfunc.h"
#include "SIG_AlgorithmInstance.h"

#define R 32

unsigned long long sig_get_pk_len_bytes(void), sig_get_sn_len_bytes(void);

static void mu_of(unsigned char mu[CRHBYTES], const unsigned char *pk, const unsigned char *m)
{
    unsigned char buf[CRHBYTES + R];
    pseudoXOF(CRHBYTES * 8, pk, sig_get_pk_len_bytes() * 8ULL, buf);
    memcpy(buf + CRHBYTES, m, R);
    pseudoXOF(CRHBYTES * 8, buf, (CRHBYTES + R) * 8ULL, mu);
}

int main(void)
{
    unsigned long long pl = sig_get_pk_len_bytes(), cb = sig_get_sn_len_bytes();
    unsigned char rho[SEEDBYTES], mu[CRHBYTES], m1[R], m2[R];
    unsigned char *pk = calloc(pl + PK_SIZE_PACKED, 1), *sig = calloc(cb + SIG_SIZE_PACKED + R, 1);
    polyveck t1, h, w1;
    polyvecl z;
    poly c;
    int a, b;

    memset(rho, 0x5a, sizeof rho);
    memset(&t1, 0, sizeof t1);
    memset(&h, 0, sizeof h);
    memset(&w1, 0, sizeof w1);
    memset(&z, 0, sizeof z);
    memset(m1, 'A', R);
    memset(m2, 'B', R);
    pack_pk(pk, rho, &t1);
    mu_of(mu, pk, m1);
    challenge(&c, mu, &w1);
    pack_sig(sig, &z, &h, &c);
    a = sig_verify(pk, pl, sig, cb, m1, R);
    memset(m1, 'A', R);   /* sig_verify overwrites its message buffer */
    b = sig_verify(pk, pl, sig, cb, m2, R);
    printf("zero_signature_accepts_M1=%d rejects_M2=%d\n", a == 0, b == -1);
    return a == 0 && b == -1 ? 0 : 1;
}
