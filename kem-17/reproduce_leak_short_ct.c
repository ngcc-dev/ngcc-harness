/* kem-17: HEP-QC per-call heap leak in hep_qc_pke_encrypt and kem_dec's
 * unchecked ct_len_bytes.
 *
 * LEAK build (-DLEAK, -Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc,
 * --wrap=free): hep_qc_pke_encrypt allocates G_mask = calloc(MSG_BIT,
 * VEC_N_SIZE_64 * 8) (PKE_HEP_QC.c:120) and never frees it; kem_enc and
 * kem_dec (FO re-encryption) both call it. Prints the live blocks and bytes
 * added by one key generation, by REPS encapsulations, by REPS honest and by
 * REPS tampered decapsulations, and the expected G_mask size.
 *
 * ASAN build: kem_dec discards ct_len_bytes (KEM_HEP_QC.c:241) and
 * hep_qc_c_kem_from_string copies the fixed layout (parsing.c:76-80). argv[1]
 * gives the declared length; the honest ciphertext is copied into a heap
 * buffer of exactly that many bytes. "full" uses CIPHERTEXT_BYTES (control).
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "drng.h"
#include "parameters.h"

DRNG_ctx drng_algorithm;
unsigned long long kem_get_pk_len_bytes(void), kem_get_sk_len_bytes(void),
    kem_get_ct_len_bytes(void), kem_get_ss_len_bytes(void);
int kem_keygen(unsigned char *, unsigned long long *, unsigned char *, unsigned long long *);
int kem_enc(unsigned char *, unsigned long long, unsigned char *, unsigned long long *,
            unsigned char *, unsigned long long *);
int kem_dec(unsigned char *, unsigned long long, unsigned char *, unsigned long long,
            unsigned char *, unsigned long long *);

#ifndef REPS
#define REPS 20
#endif
#ifdef LEAK
void *__real_malloc(size_t), *__real_calloc(size_t, size_t), *__real_realloc(void *, size_t);
void __real_free(void *);
#define SLOTS (1 << 16)
static void *key[SLOTS]; static size_t val[SLOTS];
static long blocks; static long long bytes;
static void put(void *p, size_t n)
{
    size_t h = ((uintptr_t)p >> 4) & (SLOTS - 1);
    while (key[h]) h = (h + 1) & (SLOTS - 1);
    key[h] = p; val[h] = n; blocks++; bytes += n;
}
static void del(void *p)
{
    size_t h = ((uintptr_t)p >> 4) & (SLOTS - 1);
    while (key[h] != p) { if (!key[h]) return; h = (h + 1) & (SLOTS - 1); }
    blocks--; bytes -= val[h]; key[h] = NULL;
    /* re-insert the rest of the cluster */
    for (size_t j = (h + 1) & (SLOTS - 1); key[j]; j = (j + 1) & (SLOTS - 1)) {
        void *k = key[j]; size_t v = val[j]; key[j] = NULL; blocks--; bytes -= v; put(k, v);
    }
}
void *__wrap_malloc(size_t n) { void *p = __real_malloc(n); if (p) put(p, n); return p; }
void *__wrap_calloc(size_t a, size_t b) { void *p = __real_calloc(a, b); if (p) put(p, a * b); return p; }
void *__wrap_realloc(void *q, size_t n) { if (q) del(q); void *p = __real_realloc(q, n); if (p) put(p, n); return p; }
void __wrap_free(void *p) { if (p) del(p); __real_free(p); }
#endif

int main(int argc, char **argv)
{
    unsigned long long PL = kem_get_pk_len_bytes(), SL = kem_get_sk_len_bytes(),
                       CL = kem_get_ct_len_bytes(), XL = kem_get_ss_len_bytes(), a, b;
    unsigned char *pk = malloc(PL), *sk = malloc(SL), *ct = malloc(CL), *s1 = malloc(XL), *s2 = malloc(XL);
    unsigned char seed[48];
    for (int i = 0; i < 48; i++) seed[i] = (unsigned char)(5 * i + 1);
    init_random_number(&drng_algorithm, seed, 48);
#ifdef LEAK
    (void)argc; (void)argv;
    long b0 = blocks; long long y0 = bytes;
    if (kem_keygen(pk, &a, sk, &b)) return 3;
    long b1 = blocks; long long y1 = bytes;
    int ok = 0;
    for (int i = 0; i < REPS; i++) if (kem_enc(pk, PL, s1, &a, ct, &b)) return 4;
    long b2 = blocks; long long y2 = bytes;
    for (int i = 0; i < REPS; i++) ok += kem_dec(sk, SL, ct, CL, s2, &a) == 0 && !memcmp(s1, s2, XL);
    long b3 = blocks; long long y3 = bytes;
    ct[0] ^= 1;
    for (int i = 0; i < REPS; i++) kem_dec(sk, SL, ct, CL, s2, &a);
    long b4 = blocks; long long y4 = bytes;
    printf("G_mask=%llu keygen: +%ld blocks +%lld bytes; %d enc: +%ld blocks +%lld bytes; "
           "%d dec (honest, %d agree): +%ld blocks +%lld bytes; %d dec (tampered): +%ld blocks +%lld bytes\n",
           (unsigned long long)MSG_BIT * VEC_N_SIZE_64 * 8, b1 - b0, y1 - y0, REPS, b2 - b1, y2 - y1,
           REPS, ok, b3 - b2, y3 - y2, REPS, b4 - b3, y4 - y3);
#else
    if (kem_keygen(pk, &a, sk, &b) || kem_enc(pk, PL, s1, &a, ct, &b)) return 3;
    unsigned long long L = (argc > 1 && strcmp(argv[1], "full")) ? strtoull(argv[1], 0, 10) : CL;
    if (L > CL) return 2;
    unsigned char *t = malloc(L ? L : 1);
    memcpy(t, ct, L);
    int rc = kem_dec(sk, SL, t, L, s2, &a);
    printf("declared=%llu rc=%d agrees=%d\n", L, rc, !memcmp(s1, s2, XL));
#endif
    return 0;
}
