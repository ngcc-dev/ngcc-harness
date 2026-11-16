/* kem-20-3: MAMBA-Frost's shake wrapper zeroes its output when pseudoXOF
 * fails but returns void, and crypto_kem_enc/crypto_kem_dec return 0.
 *
 * Linked with -Wl,--wrap=malloc. When armed, the wrapper fails exactly one
 * allocation of a given size: pseudoXOF's first allocation (input + 4 bytes)
 * for one targeted shake call. Three faults are tested after a control run:
 *   G:     the G hash at kem.c:354 (input pkh || mu || salt, 64 + ss bytes):
 *          seedSE and k become zero, so ss = shake(ct || 0^ss).
 *   Fenc:  the final hash at kem.c:413 (input ct || k): ss = 0^ss.
 *   Fdec:  the final hash at kem.c:573 (input ct || k'): ss = 0^ss.
 * For each, the attacker's value is computed from ct alone with an unfaulted
 * shake. Prints return codes and comparisons. */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "KEM_AlgorithmInstance.h"
#include "drng.h"
#include "common/sha3/fips202.h"

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

int main(void)
{
    unsigned char seed[48];
    unsigned long long pkl = kem_get_pk_len_bytes(), skl = kem_get_sk_len_bytes();
    unsigned long long ssl = kem_get_ss_len_bytes(), ctl = kem_get_ct_len_bytes();
    unsigned char *pk = malloc(pkl), *sk = malloc(skl), *ct = malloc(ctl);
    unsigned char *ss = malloc(ssl), *ss_dec = malloc(ssl), *ss_att = malloc(ssl);
    unsigned char *fin = calloc(ctl + ssl, 1);
    int rc, rd;

    memset(seed, 9, sizeof seed);
    init_random_number(&drng_algorithm, seed, 48);
    if (kem_keygen(pk, &pkl, sk, &skl) != 0) return 2;

    rc = kem_enc(pk, pkl, ss, &ssl, ct, &ctl);
    rd = kem_dec(sk, skl, ct, ctl, ss_dec, &ssl);
    memset(ss_att, 0, ssl);
    printf("control: rc=%d rd=%d dec_agrees=%d ss_zero=%d\n", rc, rd, !memcmp(ss, ss_dec, ssl),
           !memcmp(ss, ss_att, ssl));

    /* G fails during encapsulation */
    memset(ss, 0xEE, ssl);
    n_alloc = 0; fail_at = 0; fail_size = 64 + ssl + 4;
    rc = kem_enc(pk, pkl, ss, &ssl, ct, &ctl);
    fail_size = 0;
    memcpy(fin, ct, ctl); memset(fin + ctl, 0, ssl);
    shake256(ss_att, ssl, fin, ctl + ssl);           /* attacker: public ct only */
    rd = kem_dec(sk, skl, ct, ctl, ss_dec, &ssl);
    printf("G: alloc=%zu size=%llu rc=%d attacker_matches=%d dec_agrees=%d\n", fail_at, 64 + ssl + 4, rc,
           !memcmp(ss, ss_att, ssl), !memcmp(ss, ss_dec, ssl));

    /* final hash fails during encapsulation */
    memset(ss, 0xEE, ssl);
    n_alloc = 0; fail_at = 0; fail_size = ctl + ssl + 4;
    rc = kem_enc(pk, pkl, ss, &ssl, ct, &ctl);
    fail_size = 0;
    memset(ss_att, 0, ssl);
    rd = kem_dec(sk, skl, ct, ctl, ss_dec, &ssl);
    printf("Fenc: alloc=%zu size=%llu rc=%d attacker_matches=%d dec_agrees=%d\n", fail_at, ctl + ssl + 4, rc,
           !memcmp(ss, ss_att, ssl), !memcmp(ss, ss_dec, ssl));

    /* final hash fails during decapsulation of an honest ciphertext */
    rc = kem_enc(pk, pkl, ss, &ssl, ct, &ctl);
    memset(ss_dec, 0xEE, ssl);
    n_alloc = 0; fail_at = 0; fail_size = ctl + ssl + 4;
    rd = kem_dec(sk, skl, ct, ctl, ss_dec, &ssl);
    fail_size = 0;
    memset(ss_att, 0, ssl);
    printf("Fdec: alloc=%zu size=%llu rc=%d rd=%d attacker_matches=%d dec_agrees=%d\n", fail_at, ctl + ssl + 4,
           rc, rd, !memcmp(ss_dec, ss_att, ssl), !memcmp(ss, ss_dec, ssl));
    return 0;
}
