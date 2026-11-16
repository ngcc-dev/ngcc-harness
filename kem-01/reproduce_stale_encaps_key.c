/* kem-01-5: Aigis-Enc+ mkem_enc ignores a failure of Hash2 and copies the
 * uninitialised stack buffer kr into the shared secret and the coins.
 *
 * Linked with -Wl,--wrap=malloc. When armed, the wrapper fails exactly one
 * allocation of the size pseudohash first requests for Hash2 (2*SEED_BYTES + 2).
 * Scenario: Alice encapsulates to Mallory's key, then to Bob's key from the
 * same caller with the Hash2 allocation failing. Mallory, holding only her
 * own secret key, Bob's public key and the two ciphertexts, decapsulates her
 * ciphertext and predicts the Alice-Bob shared secret. Control: the same
 * sequence without the failure, and the faulted sequence with the stack
 * overwritten between the two sessions. Prints return codes and comparisons. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "KEM_AlgorithmInstance.h"
#include "params.h"
#include "drng.h"
#include "api.h"

DRNG_ctx drng_algorithm;
void *__real_malloc(size_t);
static size_t fail_size, n_alloc, fail_at;

void *__wrap_malloc(size_t n)
{
    n_alloc++;
    if (fail_size && n == fail_size) {
        fail_size = 0;
        fail_at = n_alloc;
        return NULL;
    }
    return __real_malloc(n);
}

static unsigned long long pkl, skl, ssl, ctl;

__attribute__((noinline)) static void scrub_stack(void)
{
    volatile unsigned char junk[1 << 16];
    for (size_t i = 0; i < sizeof junk; i++) junk[i] = (unsigned char)(0xA5 ^ i);
}

/* Alice's application: one encapsulation per session, same call path. */
__attribute__((noinline)) static int alice_session(unsigned char *pk, unsigned char *ct, unsigned char *ss)
{
    return kem_enc(pk, pkl, ss, &ssl, ct, &ctl);
}

static int run(int fault, int scrub, size_t *at)
{
    unsigned char *pkM = malloc(pkl), *skM = malloc(skl), *pkB = malloc(pkl), *skB = malloc(skl);
    unsigned char *ctM = malloc(ctl), *ctB = malloc(ctl), *ssM = malloc(ssl), *ssB = malloc(ssl);
    unsigned char *mallory = malloc(ssl), *bob = malloc(ssl);
    int r1, r2, r = 0;

    kem_keygen(pkM, &pkl, skM, &skl);
    kem_keygen(pkB, &pkl, skB, &skl);
    r1 = alice_session(pkM, ctM, ssM);
    if (scrub) scrub_stack();
    n_alloc = 0; fail_at = 0; fail_size = fault ? 2 * SEED_BYTES + 2 : 0;
    r2 = alice_session(pkB, ctB, ssB);
    fail_size = 0; *at = fail_at;
    kem_dec(skM, skl, ctM, ctl, mallory, &ssl);   /* Mallory: her own key */
    kem_dec(skB, skl, ctB, ctl, bob, &ssl);       /* Bob */
    printf("%s: rc=%d,%d alloc=%zu mallory_has_M=%d mallory_predicts_AB=%d bob_agrees=%d\n",
           fault ? (scrub ? "scrubbed" : "faulted") : "control", r1, r2, *at, !memcmp(mallory, ssM, ssl),
           !memcmp(mallory, ssB, ssl), !memcmp(bob, ssB, ssl));
    return r;
}

int main(void)
{
    unsigned char seed[48];
    size_t at;

    pkl = kem_get_pk_len_bytes(); skl = kem_get_sk_len_bytes();
    ssl = kem_get_ss_len_bytes(); ctl = kem_get_ct_len_bytes();
    memset(seed, 3, sizeof seed);
    init_random_number(&drng_algorithm, seed, 48);
    run(0, 0, &at);
    run(1, 0, &at);
    run(1, 1, &at);
    return 0;
}
