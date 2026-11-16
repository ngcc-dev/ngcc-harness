/* sign-17-3: the OPS-SIG verifier ignores a failure of its message-digest
 * call and compares against whatever its digest buffer held.
 *
 * Linked with -Wl,--wrap=malloc. The wrapper fails exactly one allocation of
 * the size pseudohash_1024 requests for the message digest (inlen + 2 bytes),
 * when armed. The driver signs message A, verifies (A, sig) to leave A's digest
 * in the verifier's stack frame, then verifies (B, sig) with that allocation
 * failing. Controls: (B, sig) without the failure, and (B, sig) with the
 * failure but after the stack has been overwritten. Prints the return values. */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "params.h"
#include "sign.h"
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

#define MLEN 64

int main(void)
{
    static uint8_t pk[CRYPTO_PUBLICKEYBYTES], sk[CRYPTO_SECRETKEYBYTES], sig[CRYPTO_BYTES];
    uint8_t seed[48], ma[MLEN], mb[MLEN];
    size_t siglen;
    int honest, plain_b, stale_b, scrubbed_b;

    for (int i = 0; i < 48; i++) seed[i] = (uint8_t)(3 * i + 1);
    init_random_number(&drng_algorithm, seed, 48);
    for (int i = 0; i < MLEN; i++) { ma[i] = (uint8_t)i; mb[i] = (uint8_t)(i ^ 0x5c); }
    crypto_sign_keypair(pk, sk);
    crypto_sign_signature(sig, &siglen, ma, MLEN, NULL, 0, sk);

    honest = crypto_sign_verify(sig, siglen, ma, MLEN, NULL, 0, pk);
    plain_b = crypto_sign_verify(sig, siglen, mb, MLEN, NULL, 0, pk);

    crypto_sign_verify(sig, siglen, ma, MLEN, NULL, 0, pk);        /* leave A's digest behind */
    fail_size = CRHBYTES + 2 + MLEN + 2;                           /* the digest's internal copy: tr, prefix, message, 2 tag bytes */
    stale_b = crypto_sign_verify(sig, siglen, mb, MLEN, NULL, 0, pk);

    crypto_sign_verify(sig, siglen, ma, MLEN, NULL, 0, pk);
    scrub_stack();
    fail_size = CRHBYTES + 2 + MLEN + 2;
    scrubbed_b = crypto_sign_verify(sig, siglen, mb, MLEN, NULL, 0, pk);

    printf("verify(A)=%d verify(B)=%d verify(B, digest allocation fails)=%d "
           "verify(B, digest allocation fails, stack scrubbed)=%d\n", honest, plain_b, stale_b, scrubbed_b);
    return 0;
}
