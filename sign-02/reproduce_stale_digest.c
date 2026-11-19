/* sign-02: the BiT verifier ignores a failure of its message-digest helper
 * and of its challenge hash, and compares against whatever its stack buffers
 * held.
 *
 * Linked with -Wl,--wrap=malloc. The wrapper fails exactly one allocation of a
 * chosen size, when armed, and reports whether it was reached. For each route
 * the driver signs message A, verifies (A, sig) to leave A's digest and
 * challenge in the verifier's stack frame, then verifies (B, sig) with that
 * allocation failing. Controls: (B, sig) without the failure, and (B, sig) with
 * the failure but after the stack has been overwritten.
 *
 * Routes (sizes in bytes, mlen = message length):
 *   long:  bit_h*_2's own copy of tr || m, malloc(TRBYTES + mlen), taken only
 *          when TRBYTES + mlen > 4096 (symmetric.c, all levels);
 *   short: pseudohash_512's first copy of tr || m, TRBYTES + mlen + 2
 *          (BiT-256/512 only; BiT-128 hashes with sm3hash, which allocates nothing);
 *   chal:  pseudohash_512's first copy of mu || w1 for the recomputed challenge,
 *          MESSAGEBYTES + POLYVECK_W1_BYTES + 2, independent of the message
 *          (BiT-256/512 only). */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "params.h"
#include "SIG_AlgorithmInstance.h"
#include "drng.h"

DRNG_ctx drng_algorithm;
void *__real_malloc(size_t);
static size_t fail_size;

void *__wrap_malloc(size_t n)
{
    if (fail_size && n == fail_size) {
        fail_size = 0;
        return NULL;
    }
    return __real_malloc(n);
}

__attribute__((noinline)) static void scrub_stack(void)
{
    volatile uint8_t junk[1 << 16];
    for (size_t i = 0; i < sizeof junk; i++) junk[i] = (uint8_t)(0xA5 ^ i);
}

#define LONG 5000
#define SHORT 64

static unsigned char pk[BIT_PUBLICKEYBYTES], sk[BIT_SECRETKEYBYTES], sig[BIT_SIGNBYTES];
static unsigned char ma[LONG], mb[LONG];
static unsigned long long pkl, skl, sl;

/* Every verification goes through this one non-inlined call, so the verifier's
 * frame sits at the same stack address each time. With n != 0 the first
 * allocation of n bytes fails; returns 99 if no such allocation was requested. */
__attribute__((noinline)) static int verify(const unsigned char *m, size_t mlen, size_t n)
{
    int r;
    fail_size = n;
    r = sig_verify(pk, pkl, sig, sl, (unsigned char *)m, mlen);
    if (fail_size) { fail_size = 0; return 99; }
    return r;
}

static void route(const char *name, size_t mlen, size_t n)
{
    int honest, plain_b, stale_b, scrubbed_b;

    if (sig_sign(sk, skl, ma, mlen, sig, &sl) != 0) { printf("%s: sign failed\n", name); return; }
    honest = verify(ma, mlen, 0);
    plain_b = verify(mb, mlen, 0);

    verify(ma, mlen, 0);                               /* leave A's digest and challenge behind */
    stale_b = verify(mb, mlen, n);

    verify(ma, mlen, 0);
    scrub_stack();
    scrubbed_b = verify(mb, mlen, n);

    printf("%s mlen=%zu fail=%zu: verify(A)=%d verify(B)=%d verify(B, allocation fails)=%d "
           "verify(B, allocation fails, stack scrubbed)=%d\n",
           name, mlen, n, honest, plain_b, stale_b, scrubbed_b);
}

int main(void)
{
    uint8_t seed[48];

    for (int i = 0; i < 48; i++) seed[i] = (uint8_t)(3 * i + 1);
    init_random_number(&drng_algorithm, seed, 48);
    for (int i = 0; i < LONG; i++) { ma[i] = (uint8_t)i; mb[i] = (uint8_t)(i ^ 0x5c); }
    if (sig_keygen(pk, &pkl, sk, &skl) != 0) { printf("keygen failed\n"); return 1; }

    route("long", LONG, BIT_TRBYTES + LONG);
#if BIT_MESSAGEBYTES == 64
    route("short", SHORT, BIT_TRBYTES + SHORT + 2);
    route("chal", SHORT, BIT_MESSAGEBYTES + BIT_POLYVECK_W1_BYTES + 2);
#endif
    return 0;
}
