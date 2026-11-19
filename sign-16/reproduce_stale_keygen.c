/* sign-16: Octarine key generation ignores a failure of pseudoXOF when it
 * derives (rho, rho', K) from the fresh seed xi, and builds the key pair from
 * whatever its stack buffer held.
 *
 * Linked with -Wl,--wrap=malloc. The wrapper fails exactly one allocation of
 * the armed size and reports whether it was reached. The size is pseudoXOF's
 * copy of the encoded xi plus a 4-byte counter, KDF_INPUT + 4. Every key
 * generation goes through one non-inlined call, so sig_keygen's frame sits at
 * the same stack address each time.
 *
 * Sequence: an ordinary key generation (party A), then one with that
 * allocation failing (party B), then an ordinary one (control), then one with
 * the failure after the stack has been overwritten (control). */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "sign.h"
#include "hash_domain.h"
#include "drng.h"

DRNG_ctx drng_algorithm;
void *__real_malloc(size_t);
static size_t fail_size, count_size;
static int reached, counted;

void *__wrap_malloc(size_t n)
{
    if (count_size && n == count_size) counted++;
    if (fail_size && n == fail_size) {
        fail_size = 0;
        reached = 1;
        return NULL;
    }
    return __real_malloc(n);
}

__attribute__((noinline)) static void scrub_stack(void)
{
    volatile uint8_t junk[1 << 17];
    for (size_t i = 0; i < sizeof junk; i++) junk[i] = (uint8_t)(0xA5 ^ i);
}

#define KDF_INPUT RRLWR_DOMAIN_INPUT_LEN(RRLWR_DOMAIN_LABEL_KDF, RRLWR_SIGN_XI_LEN, 1)
#define FAIL (KDF_INPUT + 4)

static unsigned char pk[4][RRLWR_SIGN_PK_LEN], sk[4][RRLWR_SIGN_SK_LEN];
static unsigned char sig[RRLWR_SIGN_SIG_LEN], msg[64];

/* With n != 0 the first allocation of n bytes fails; returns 99 if no such
 * allocation was requested. */
__attribute__((noinline)) static int keygen(int i, size_t n)
{
    unsigned long long pkl, skl;
    int r;
    fail_size = n;
    reached = 0;
    r = sig_keygen(pk[i], &pkl, sk[i], &skl);
    fail_size = 0;
    if (n && !reached) return 99;
    return r;
}

static int same(int i, int j)
{
    return memcmp(pk[i], pk[j], sizeof pk[i]) == 0 && memcmp(sk[i], sk[j], sizeof sk[i]) == 0;
}

int main(void)
{
    uint8_t seed[48];
    unsigned long long sl;
    int r0, r1, r2, r3, cross, changed;

    for (int i = 0; i < 48; i++) seed[i] = (uint8_t)(5 * i + 7);
    init_random_number(&drng_algorithm, seed, 48);
    for (int i = 0; i < 64; i++) msg[i] = (uint8_t)(i * 13);

    count_size = FAIL;
    r0 = keygen(0, 0);                 /* party A, ordinary */
    count_size = 0;
    r1 = keygen(1, FAIL);              /* party B, KDF copy fails */
    r2 = keygen(2, 0);                 /* control: ordinary */
    scrub_stack();
    r3 = keygen(3, FAIL);              /* control: failure after scrubbing */

    /* Party A's secret key signs for party B's public key. */
    if (sig_sign(sk[0], RRLWR_SIGN_SK_LEN, msg, sizeof msg, sig, &sl) != 0) return 1;
    cross = sig_verify(pk[1], RRLWR_SIGN_PK_LEN, sig, sl, msg, sizeof msg);
    msg[0] ^= 1;
    changed = sig_verify(pk[1], RRLWR_SIGN_PK_LEN, sig, sl, msg, sizeof msg);

    printf("fail_size=%d allocations_of_that_size_per_keygen=%d keygen_A=%d keygen_B=%d "
           "B_equals_A=%d keygen_control=%d control_equals_A=%d keygen_scrubbed=%d "
           "scrubbed_equals_A=%d scrubbed_equals_control=%d A_signs_for_B=%d changed_message=%d\n",
           (int)FAIL, counted, r0, r1, same(1, 0), r2, same(2, 0), r3, same(3, 0), same(3, 2),
           cross, changed);
    return 0;
}
