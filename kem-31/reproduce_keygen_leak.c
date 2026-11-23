/* kem-31-7 driver: key generation and decapsulation leak theta-chain arrays.
 *
 * Linked with -Wl,--wrap=malloc,--wrap=free,--wrap=theta_chain_comput_strategy.
 * The malloc and free wrappers keep a table of live blocks and their sizes;
 * GMP's allocator is counted separately through mp_set_memory_functions. The
 * third wrapper records the steps array that each theta_chain_comput_strategy
 * call allocates. main runs two deterministic key generations, two
 * encapsulations, two decapsulations and one decapsulation of the second
 * ciphertext with its last byte flipped, and prints after each call the live
 * blocks and bytes, how many of them are recorded step arrays, and the live
 * GMP blocks, all relative to the counts after DRNG setup. It then frees the
 * recorded arrays that are still live and prints what remains.
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gmp.h>
#include "KEM_AlgorithmInstance.h"
#include "drng.h"
#include <hd.h>
#include <torsion_constants.h>

DRNG_ctx drng_algorithm;

#define CAP 4096
static void *live_ptr[CAP];
static size_t live_len[CAP];
static long live, untracked;
static size_t live_bytes;
static long gmp_live;
static long base_live, base_gmp;
static size_t base_bytes;

void *__real_malloc(size_t n);
void __real_free(void *p);
void *__wrap_malloc(size_t n) {
    void *p = __real_malloc(n);
    if (p) {
        if (live == CAP) abort();
        live_ptr[live] = p; live_len[live] = n; live++; live_bytes += n;
    }
    return p;
}
void __wrap_free(void *p) {
    long i;
    if (!p) return;
    for (i = live - 1; i >= 0 && live_ptr[i] != p; i--) ;
    if (i < 0) untracked++;
    else { live_bytes -= live_len[i]; live--; live_ptr[i] = live_ptr[live]; live_len[i] = live_len[live]; }
    __real_free(p);
}
static void *gmp_alloc(size_t n) { gmp_live++; return __real_malloc(n); }
static void *gmp_realloc(void *p, size_t o, size_t n) { (void)o; return realloc(p, n); }
static void gmp_free(void *p, size_t n) { (void)n; gmp_live--; __real_free(p); }

static void *steps[64];
static long nsteps;
void __real_theta_chain_comput_strategy(theta_chain_t *out, int n, theta_couple_curve_t *E12,
                                        const theta_couple_point_t *T1, const theta_couple_point_t *T2,
                                        const theta_couple_point_t *T1m2, int *strategy, int eight_above);
void __wrap_theta_chain_comput_strategy(theta_chain_t *out, int n, theta_couple_curve_t *E12,
                                        const theta_couple_point_t *T1, const theta_couple_point_t *T2,
                                        const theta_couple_point_t *T1m2, int *strategy, int eight_above) {
    __real_theta_chain_comput_strategy(out, n, E12, T1, T2, T1m2, strategy, eight_above);
    if (nsteps < 64) steps[nsteps++] = out->steps;
}

static void report(const char *op, int rc) {
    long i, j, s = 0;
    for (i = 0; i < live; i++)
        for (j = 0; j < nsteps; j++)
            if (live_ptr[i] == steps[j]) { s++; break; }
    printf("%s rc=%d live=%ld bytes=%zu steps=%ld gmp=%ld\n", op, rc, live - base_live, live_bytes - base_bytes, s,
           gmp_live - base_gmp);
}

int main(void) {
    unsigned long long pkl = kem_get_pk_len_bytes(), skl = kem_get_sk_len_bytes();
    unsigned long long ssl = kem_get_ss_len_bytes(), ctl = kem_get_ct_len_bytes(), ssl2 = ssl;
    static unsigned char pk[1 << 14], sk[1 << 14], ct[1 << 14], ss[256], ss2[256], seed[64];
    long i, j, n;

    if (pkl > sizeof pk || skl > sizeof sk || ctl > sizeof ct || ssl > sizeof ss) return 2;
    mp_set_memory_functions(gmp_alloc, gmp_realloc, gmp_free);
    for (i = 0; i < 64; i++) seed[i] = (unsigned char)(7 * i + 1);
    init_random_number(&drng_algorithm, seed, sizeof seed);
    printf("array=%llu*%zu=%llu\n", (unsigned long long)TORSION_PLUS_EVEN_POWER - 3, sizeof(theta_isogeny_t),
           (unsigned long long)(TORSION_PLUS_EVEN_POWER - 3) * sizeof(theta_isogeny_t));
    base_live = live; base_bytes = live_bytes; base_gmp = gmp_live;
    for (i = 0; i < 2; i++) report("keygen", kem_keygen(pk, &pkl, sk, &skl));
    for (i = 0; i < 2; i++) report("enc", kem_enc(pk, pkl, ss, &ssl, ct, &ctl));
    for (i = 0; i < 2; i++) report("dec", kem_dec(sk, skl, ct, ctl, ss2, &ssl2));
    printf("agree=%d\n", ssl == ssl2 && memcmp(ss, ss2, ssl) == 0);
    ct[ctl - 1] ^= 1;
    report("dec-flipped", kem_dec(sk, skl, ct, ctl, ss2, &ssl2));
    for (i = 0, n = 0; i < nsteps; i++) {
        for (j = 0; j < live && live_ptr[j] != steps[i]; j++) ;
        if (j < live) { __wrap_free(steps[i]); n++; }
    }
    printf("freed %ld arrays: live=%ld bytes=%zu untracked_frees=%ld\n", n, live - base_live, live_bytes - base_bytes,
           untracked);
    return 0;
}
