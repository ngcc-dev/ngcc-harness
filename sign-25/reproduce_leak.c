/* sign-25: SQIsign2D2 per-call heap growth in sig_keygen, sig_sign and
 * sig_verify.
 *
 * Linked with -Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc,--wrap=free so
 * that the candidate's own allocations (e.g. ibz_rand_interval's
 * malloc(sizeof(mp_limb_t)*len_limbs), intbig.c:484, never freed) are
 * counted; GMP's internal allocations are counted through
 * mp_set_memory_functions. After one warm-up call of each kind, the driver
 * makes REPS further calls of each kind and prints the live blocks and bytes
 * they add (candidate / GMP), per call. Verification is run on an honest
 * signature and on the same signature with one bit flipped; the stack is
 * overwritten before each flipped verification so that the stale-stack
 * verdict of sign-25-1 cannot accept it.
 */
#include <gmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "drng.h"

#ifndef REPS
#define REPS 4
#endif
DRNG_ctx drng_algorithm;
unsigned long long sig_get_pk_len_bytes(void), sig_get_sk_len_bytes(void), sig_get_sn_len_bytes(void);
int sig_keygen(unsigned char *, unsigned long long *, unsigned char *, unsigned long long *);
int sig_sign(unsigned char *, unsigned long long, unsigned char *, unsigned long long,
             unsigned char *, unsigned long long *);
int sig_verify(unsigned char *, unsigned long long, unsigned char *, unsigned long long,
               unsigned char *, unsigned long long);

/* pointer -> size table for the candidate's direct allocations */
#define SLOTS (1 << 20)
static void **key; static size_t *val;
static long cb; static long long cy;     /* candidate blocks / bytes */
static long gb; static long long gy;     /* GMP blocks / bytes */
void *__real_malloc(size_t), *__real_calloc(size_t, size_t), *__real_realloc(void *, size_t);
void __real_free(void *);
static void put(void *p, size_t n)
{
    size_t h = ((uintptr_t)p >> 4) & (SLOTS - 1);
    while (key[h]) h = (h + 1) & (SLOTS - 1);
    key[h] = p; val[h] = n; cb++; cy += n;
}
static void del(void *p)
{
    size_t h = ((uintptr_t)p >> 4) & (SLOTS - 1);
    while (key[h] != p) { if (!key[h]) return; h = (h + 1) & (SLOTS - 1); }
    cb--; cy -= val[h]; key[h] = NULL;
    for (size_t j = (h + 1) & (SLOTS - 1); key[j]; j = (j + 1) & (SLOTS - 1)) {
        void *k = key[j]; size_t v = val[j]; key[j] = NULL; cb--; cy -= v; put(k, v);
    }
}
void *__wrap_malloc(size_t n) { void *p = __real_malloc(n); if (p && key) put(p, n); return p; }
void *__wrap_calloc(size_t a, size_t b) { void *p = __real_calloc(a, b); if (p && key) put(p, a * b); return p; }
void *__wrap_realloc(void *q, size_t n) { if (q && key) del(q); void *p = __real_realloc(q, n); if (p && key) put(p, n); return p; }
void __wrap_free(void *p) { if (p && key) del(p); __real_free(p); }
static void *gm(size_t n) { gb++; gy += n; return __real_malloc(n); }
static void *gr(void *p, size_t o, size_t n) { gy += (long long)n - (long long)o; return __real_realloc(p, n); }
static void gf(void *p, size_t n) { gb--; gy -= n; __real_free(p); }

__attribute__((noinline)) static void scrub(void)
{
    volatile unsigned char junk[1 << 17];
    for (size_t i = 0; i < sizeof junk; i++) junk[i] = (unsigned char)(131 * i + 7);
}

static long b0, g0; static long long y0, z0;
static void mark(void) { b0 = cb; y0 = cy; g0 = gb; z0 = gy; }
static void report(const char *what)
{
    printf("%s: per call +%ld/+%ld blocks, +%lld/+%lld bytes (candidate/GMP), total +%lld bytes\n", what,
           (cb - b0) / REPS, (gb - g0) / REPS, (cy - y0) / REPS, (gy - z0) / REPS, (cy - y0 + gy - z0) / REPS);
}

int main(void)
{
    key = __real_calloc(SLOTS, sizeof *key); val = __real_calloc(SLOTS, sizeof *val);
    mp_set_memory_functions(gm, gr, gf);
    unsigned long long PL = sig_get_pk_len_bytes(), SL = sig_get_sk_len_bytes(), NL = sig_get_sn_len_bytes(), a, b;
    unsigned char *pk = __real_malloc(PL), *sk = __real_malloc(SL), *sig = __real_malloc(NL), *bad = __real_malloc(NL);
    unsigned char m[32], seed[48];
    for (int i = 0; i < 48; i++) seed[i] = (unsigned char)(17 * i + 3);
    for (int i = 0; i < 32; i++) m[i] = (unsigned char)i;
    init_random_number(&drng_algorithm, seed, 48);
    int ok = 0, rej = 0;

    sig_keygen(pk, &a, sk, &b);                                 /* warm-up */
    mark(); for (int r = 0; r < REPS; r++) sig_keygen(pk, &a, sk, &b); report("sig_keygen");
    sig_sign(sk, SL, m, sizeof m, sig, &a);
    mark(); for (int r = 0; r < REPS; r++) sig_sign(sk, SL, m, sizeof m, sig, &a); report("sig_sign");
    memcpy(bad, sig, NL); bad[NL / 2] ^= 1;
    sig_verify(pk, PL, sig, NL, m, sizeof m);
    mark(); for (int r = 0; r < REPS; r++) ok += sig_verify(pk, PL, sig, NL, m, sizeof m) == 0; report("sig_verify (honest)");
    scrub(); sig_verify(pk, PL, bad, NL, m, sizeof m);
    mark(); for (int r = 0; r < REPS; r++) { scrub(); rej += sig_verify(pk, PL, bad, NL, m, sizeof m) != 0; } report("sig_verify (bit-flipped)");
    printf("honest accepted %d/%d, bit-flipped rejected %d/%d\n", ok, REPS, rej, REPS);
    return 0;
}
