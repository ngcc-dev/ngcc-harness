/* kem-23-5: Mito's encapsulation ignores a failure of hash_g and returns the
 * zero-initialised K_theta, so the shared secret and the coins theta are zero.
 *
 * Linked with -Wl,--wrap=malloc. When armed, the wrapper fails exactly one
 * allocation of the size pseudohash_1024 first requests inside hash_g
 * (SEED_BYTES + PARAM_K_BYTES + SALT_BYTES + 2 bytes) and records its position
 * among the allocations of the call. The driver runs a control encapsulation,
 * then a faulted one, and prints the return codes, whether the faulted shared
 * secret equals the value an attacker computes from public data (64 zero
 * bytes), and whether the receiver's decapsulation agrees. */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parameters.h"
#include "KEM_AlgorithmInstance.h"
#include "drng.h"

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

static int is_zero(const unsigned char *p, size_t n)
{
    unsigned char a = 0;
    for (size_t i = 0; i < n; i++) a |= p[i];
    return a == 0;
}

int main(void)
{
    unsigned char seed[64];
    unsigned long long pkl = kem_get_pk_len_bytes(), skl = kem_get_sk_len_bytes();
    unsigned long long ssl = kem_get_ss_len_bytes(), ctl = kem_get_ct_len_bytes();
    unsigned char *pk = calloc(pkl, 1), *sk = calloc(skl, 1), *ct = calloc(ctl, 1);
    unsigned char *ss = calloc(ssl, 1), *ss_dec = calloc(ssl, 1), *ss_att = calloc(ssl, 1);
    int rc_ctl, rc_flt;

    memset(seed, 7, sizeof seed);
    init_random_number(&drng_algorithm, seed, 64);
    kem_keygen(pk, &pkl, sk, &skl);

    /* control: no failure */
    rc_ctl = kem_enc(pk, pkl, ss, &ssl, ct, &ctl);
    kem_dec(sk, skl, ct, ctl, ss_dec, &ssl);
    printf("control: rc=%d ss_zero=%d dec_agrees=%d\n", rc_ctl, is_zero(ss, ssl), !memcmp(ss, ss_dec, ssl));

    /* faulted: the first allocation inside hash_g fails */
    memset(ss, 0xEE, ssl);
    n_alloc = 0; fail_at = 0;
    fail_size = SEED_BYTES + PARAM_K_BYTES + SALT_BYTES + 2;
    rc_flt = kem_enc(pk, pkl, ss, &ssl, ct, &ctl);
    fail_size = 0;
    /* attacker: knows pk and ct only; predicts the all-zero K_theta prefix */
    memset(ss_att, 0, ssl);
    kem_dec(sk, skl, ct, ctl, ss_dec, &ssl);
    printf("faulted: alloc=%zu/%zu size=%d rc=%d ss_zero=%d attacker_matches=%d dec_agrees=%d\n",
           fail_at, n_alloc, (int)(SEED_BYTES + PARAM_K_BYTES + SALT_BYTES + 2), rc_flt,
           is_zero(ss, ssl), !memcmp(ss, ss_att, ssl), !memcmp(ss, ss_dec, ssl));
    return 0;
}
