/* sign-18-8 driver: Origami verification derives its target with a squeeze
 * helper that zero-fills on failure and returns nothing, so after one failed
 * allocation the target is zero and the all-zero signature verifies.
 *
 * Linked with -Wl,--wrap=malloc. Target derivation makes three consecutive
 * allocations: the squeeze buffer (BYTES_HASH bytes), then pseudoXOF's copy
 * of its input and its output buffer. The same sizes also occur earlier in a
 * verification, so the driver first runs a fault-free verification, finds the
 * consecutive triple and reports which occurrence of each size it is.
 *
 * One run generates a key pair, performs the fault-free checks, locates the
 * triple, and then fails each of its three allocations in turn (the OCC-th
 * allocation of that SIZE inside each faulted sig_verify call).
 *
 * Signatures tested: zero (all-zero bytes of the signature length) on two
 * unrelated messages and a fixed nonzero pattern, all with the fault; an
 * honest signature without it. One line per verification. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SIG_AlgorithmInstance.h"
#include "origami.h"
#include "drng.h"

DRNG_ctx drng_algorithm;
void *__real_malloc(size_t);

#define MAXPOS 4096
static const size_t sizes[3] = { (size_t)BYTES_HASH, (size_t)(6 + BYTES_DIGEST + BYTES_SALT + 4),
                                 (size_t)((BYTES_HASH * 8 + 255) / 8) };
static int armed;
static size_t fail_size, fail_occ, occ, n_alloc, npos, pos_size[MAXPOS], pos_index[MAXPOS];

void *__wrap_malloc(size_t n)
{
    if (armed) {
        n_alloc++;
        for (int i = 0; i < 3; i++)
            if (n == sizes[i] && npos < MAXPOS) { pos_size[npos] = (size_t)i; pos_index[npos++] = n_alloc; }
        if (fail_size && n == fail_size && ++occ == fail_occ) return NULL;
    }
    return __real_malloc(n);
}

static int verify(unsigned char *pk, unsigned long long pkl, unsigned char *sn, unsigned long long snl,
                  const char *m, size_t size, size_t which)
{
    n_alloc = npos = occ = 0;
    fail_size = size;
    fail_occ = which;
    armed = 1;
    int rc = sig_verify(pk, pkl, sn, snl, (unsigned char *)m, strlen(m));
    armed = 0;
    return rc;
}

int main(void)
{
    unsigned long long pkl = sig_get_pk_len_bytes(), skl = sig_get_sk_len_bytes(), snl = sig_get_sn_len_bytes();
    unsigned char seed[48], *pk = malloc(pkl), *sk = malloc(skl);
    unsigned char *zero = calloc(1, snl), *honest = malloc(snl), *pattern = malloc(snl);
    const char *a = "message A, signed honestly", *b = "message B, never signed";
    unsigned long long hl = snl;
    size_t tsize[3], tocc[3], found = 0;
    int r = 0;
    if (!pk || !sk || !zero || !honest || !pattern) return 2;

    /* Some keys fail to sign (the retry limit of sign-18-6); take the first
     * seed whose key pair signs message A. */
    for (int s = 9;; s++) {
        if (s == 64) { printf("no seed signed: keygen/sign rc=%d\n", r); return 3; }
        memset(seed, s, sizeof seed);
        init_random_number(&drng_algorithm, seed, sizeof seed);
        pkl = sig_get_pk_len_bytes(); skl = sig_get_sk_len_bytes(); hl = snl;
        if ((r = sig_keygen(pk, &pkl, sk, &skl)) != 0) continue;
        if ((r = sig_sign(sk, skl, (unsigned char *)a, strlen(a), honest, &hl)) == 0 && hl == snl) break;
    }
    for (unsigned long long i = 0; i < snl; i++) pattern[i] = (unsigned char)(0x11 * (i % 15) + 1);

    printf("zero sig, message B, no fault: rc=%d\n", verify(pk, pkl, zero, snl, b, 0, 0));
    /* find squeeze, xof_in, xof_out at consecutive allocation indices */
    size_t seen[3] = { 0, 0, 0 };
    for (size_t p = 0; p < npos; p++) {
        seen[pos_size[p]]++;
        if (p + 2 < npos && pos_size[p] == 0 && pos_size[p + 1] == 1 && pos_size[p + 2] == 2 &&
            pos_index[p + 1] == pos_index[p] + 1 && pos_index[p + 2] == pos_index[p] + 2 && !found) {
            tsize[0] = sizes[0]; tocc[0] = seen[0];
            tsize[1] = sizes[1]; tocc[1] = seen[1] + 1;
            tsize[2] = sizes[2]; tocc[2] = seen[2] + 1;
            found = pos_index[p];
        }
    }
    printf("honest sig, message A, no fault: rc=%d\n", verify(pk, pkl, honest, snl, a, 0, 0));
    if (!found) { printf("target allocations not found\n"); return 3; }
    printf("triple at allocation %zu: %zu:%zu %zu:%zu %zu:%zu\n",
           found, tsize[0], tocc[0], tsize[1], tocc[1], tsize[2], tocc[2]);
    for (int i = 0; i < 3; i++) {
        printf("fault %zu:%zu: zero/B rc=%d zero/A rc=%d pattern/B rc=%d\n", tsize[i], tocc[i],
               verify(pk, pkl, zero, snl, b, tsize[i], tocc[i]),
               verify(pk, pkl, zero, snl, a, tsize[i], tocc[i]),
               verify(pk, pkl, pattern, snl, b, tsize[i], tocc[i]));
    }
    return 0;
}
