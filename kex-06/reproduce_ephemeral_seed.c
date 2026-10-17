/* kex-06-1 extension: the MAMBA-NIKE-384/-512 initiator draws only a public
 * 32-byte mu and a 32-byte noiseseed per session (nike.c nike_sharedb). The
 * ephemeral secret r and the HelpRec bits are both expanded from noiseseed.
 * Given the public transcript (pk_B, M1) and the right 32-byte guess, a
 * passive attacker recomputes M1 exactly (the guess test) and the session key.
 *
 * Built against the unmodified submitted nike.c and its dependencies; this
 * file only supplies nike_randombytes so the test can feed chosen bytes. */
#include <stdio.h>
#include <string.h>
#include "nike.h"
#include "params.h"

static unsigned char q[256];
static unsigned qlen, qpos, drawn;
static unsigned ctr = 1;

int nike_randombytes(unsigned char *x, unsigned long long n)
{
    for (unsigned long long i = 0; i < n; i++, drawn++)
        x[i] = (qpos < qlen) ? q[qpos++] : (unsigned char)(ctr++ * 131 + 7);
    return 0;
}
void randombytes(unsigned char *x, unsigned long long n) { nike_randombytes(x, n); }

static void feed(const unsigned char *a, const unsigned char *b)
{
    memcpy(q, a, 32); memcpy(q + 32, b, 32); qlen = 64; qpos = 0; drawn = 0;
}

int main(void)
{
    static unsigned char pkb[NIKE_SENDABYTES], m1[NIKE_SENDBBYTES], m1e[NIKE_SENDBBYTES];
    unsigned char ka[NIKE_SSBYTES], kb[NIKE_SSBYTES], ke[NIKE_SSBYTES], mu[32], ns[32], ns2[32];
    unsigned per_session;
    int ok = 1;
    poly skb;

    nike_keygen(pkb, &skb);                               /* responder static key */
    for (int i = 0; i < 32; i++) { mu[i] = (unsigned char)(0x40 + i); ns[i] = (unsigned char)(0x99 ^ (5 * i)); }
    feed(mu, ns);
    nike_sharedb(ka, m1, pkb);                            /* honest initiator: (M1, K_A) */
    per_session = drawn;
    nike_shareda(kb, &skb, pkb, m1);                      /* honest responder: K_B */
    printf("MAMBA-NIKE-%d: n=%d ss=%d bytes, random bytes drawn per session=%u (mu 32 + noiseseed 32)\n",
           NIKE_LEVEL, PARAM_N, NIKE_SSBYTES, per_session);
    ok &= per_session == 64;
    ok &= memcmp(ka, kb, NIKE_SSBYTES) == 0;
    printf("  control: honest agreement K_A == K_B: %s\n", memcmp(ka, kb, NIKE_SSBYTES) ? "no" : "yes");

    memcpy(mu, m1, 32);                                   /* mu is the first 32 bytes of M1 */
    feed(mu, ns);                                         /* attacker: public mu + guessed noiseseed */
    nike_sharedb(ke, m1e, pkb);
    printf("  correct 256-bit noiseseed guess: M1 match %s, session key match %s\n",
           memcmp(m1e, m1, NIKE_SENDBBYTES) ? "no" : "yes", memcmp(ke, kb, NIKE_SSBYTES) ? "no" : "yes");
    ok &= memcmp(m1e, m1, NIKE_SENDBBYTES) == 0 && memcmp(ke, kb, NIKE_SSBYTES) == 0;

    memcpy(ns2, ns, 32); ns2[31] ^= 0x80;                 /* control: wrong guess */
    feed(mu, ns2);
    nike_sharedb(ke, m1e, pkb);
    printf("  control: one-bit-wrong guess: M1 match %s, session key match %s\n",
           memcmp(m1e, m1, NIKE_SENDBBYTES) ? "no" : "yes", memcmp(ke, kb, NIKE_SSBYTES) ? "no" : "yes");
    ok &= memcmp(m1e, m1, NIKE_SENDBBYTES) != 0 && memcmp(ke, kb, NIKE_SSBYTES) != 0;
    return ok ? 0 : 1;
}
