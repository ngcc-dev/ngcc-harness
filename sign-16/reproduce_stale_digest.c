/* sign-16: the Octarine verifier ignores a failure of pseudoXOF when it hashes
 * the message representative mu and the challenge seed ctilde', and compares
 * against whatever its stack buffers held.
 *
 * Linked with -Wl,--wrap=malloc. The wrapper fails exactly one allocation of a
 * chosen size, when armed, and reports whether it was reached. For each route
 * the driver signs message A, verifies (A, sig) to leave A's mu and ctilde' in
 * the verifier's stack frame, then verifies (B, sig) with that allocation
 * failing. Controls: (B, sig) without the failure, and (B, sig) with the
 * failure but after the stack has been overwritten.
 *
 * Routes (sizes in bytes, mlen = message length):
 *   mu: pseudoXOF's copy of the encoded (tr, m) input plus a 4-byte counter,
 *       RRLWR_SIGN_HASH_MU's checked input length + 4 = 175 + mlen + 4;
 *   ch: pseudoXOF's copy of the encoded (mu, w1) input plus the counter,
 *       independent of the message. */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "sign.h"
#include "hash_domain.h"
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
    volatile uint8_t junk[1 << 17];
    for (size_t i = 0; i < sizeof junk; i++) junk[i] = (uint8_t)(0xA5 ^ i);
}

#define LONG 5000
#define SHORT 64
#define MU_FIXED (RRLWR_DOMAIN_HEADER_LEN(RRLWR_DOMAIN_LABEL_MU) + 2 * RRLWR_DOMAIN_LEN_BYTES + RRLWR_SIGN_TR_LEN)
#define CH_INPUT RRLWR_DOMAIN_INPUT_LEN(RRLWR_DOMAIN_LABEL_CH, RRLWR_SIGN_MU_LEN + RRLWR_SIGN_PACKED_W1_LEN, 2)

static unsigned char pk[RRLWR_SIGN_PK_LEN], sk[RRLWR_SIGN_SK_LEN], sig[RRLWR_SIGN_SIG_LEN];
static unsigned char ma[LONG], mb[LONG];
static unsigned long long pkl, skl, sl;

/* Every verification goes through this one non-inlined call, so the verifier's
 * frame sits at the same stack address each time. With n != 0 the first
 * allocation of n bytes fails; returns 99 if no such allocation was requested. */
__attribute__((noinline)) static int verify(unsigned char *m, size_t mlen, size_t n)
{
    int r;
    fail_size = n;
    r = sig_verify(pk, pkl, sig, sl, m, mlen);
    if (fail_size) { fail_size = 0; return 99; }
    return r;
}

static void route(const char *name, size_t mlen, size_t n)
{
    int honest, plain_b, stale_b, scrubbed_b;

    if (sig_sign(sk, skl, ma, mlen, sig, &sl) != 0) { printf("%s: sign failed\n", name); return; }
    honest = verify(ma, mlen, 0);
    plain_b = verify(mb, mlen, 0);

    verify(ma, mlen, 0);                               /* leave A's mu and ctilde' behind */
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

    route("mu", SHORT, MU_FIXED + SHORT + 4);
    route("mu", LONG, MU_FIXED + LONG + 4);
    route("ch", SHORT, CH_INPUT + 4);
    return 0;
}
