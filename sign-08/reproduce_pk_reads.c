/* sign-08: DARTS public-key reads and the key-generation singular-value test.
 *
 * guard / ctrl (AVX2 build): the AVX2 poly_q_unpack loads 32 bytes per 8
 * coefficients while advancing 17 (DARTS128/256) or 18 (DARTS512) bytes, so
 * decoding the last A0 polynomial of pk reads 15 or 14 bytes past it. A valid
 * pk is copied so that it ends exactly at a page boundary; "guard" makes the
 * next page PROT_NONE, "ctrl" leaves it readable. Prints the verify result.
 *
 * short / full (ASan build): sig_verify discards pk_len_bytes; a valid pk is
 * copied into a heap buffer one byte short ("short") or of full length.
 *
 * sqsing (reference build): polyveckl_sqsing_value accumulates N(s)*1024 in
 * int32_t res (polyvec.c:652,707). For M keys from sig_keygen the secret is
 * unpacked and N(s) recomputed with 64-bit integers; prints how many keys
 * violate N(s) <= GAMMA^2*N (Algorithm 6 step 6) and the code's own values.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include "params.h"
#include "drng.h"

DRNG_ctx drng_algorithm;
unsigned long long sig_get_pk_len_bytes(void), sig_get_sk_len_bytes(void), sig_get_sn_len_bytes(void);
int sig_keygen(unsigned char *, unsigned long long *, unsigned char *, unsigned long long *);
int sig_sign(unsigned char *, unsigned long long, unsigned char *, unsigned long long,
             unsigned char *, unsigned long long *);
int sig_verify(unsigned char *, unsigned long long, unsigned char *, unsigned long long,
               unsigned char *, unsigned long long);

#ifdef SQSING
#include "poly.h"
#include "polyvec.h"
#include "packing.h"
#include "fft.h"
static int64_t nsq64(const poly *s0, const polyvecl_1 *s1, const polyveck *e)
{
    static complex_fp32_16 in[FFT_N];
    int64_t sum[N] = {0}, best[N / TAU + 1], mn, res = 0;
    const poly *v[K + L];
    int c = 0;
    v[c++] = s0;
    for (int i = 0; i < L - 1; i++) v[c++] = &s1->vec[i];
    for (int i = 0; i < K; i++) v[c++] = &e->vec[i];
    for (int p = 0; p < c; p++) {
        memset(in, 0, sizeof in);
        fft_init_and_bitrev(in, v[p]); fft(in);
        for (int j = 0; j < N; j++) sum[j] += complex_fp_sqabs(in[j]);
    }
    for (int i = 0; i < N / TAU + 1; i++) best[i] = sum[i];
    for (int i = N / TAU + 1; i < N; i++)
        for (int j = 0; j < N / TAU + 1; j++)
            if (sum[i] > best[j]) { int64_t t = sum[i]; sum[i] = best[j]; best[j] = t; }
    mn = best[0];
    for (int i = 1; i < N / TAU + 1; i++) if (best[i] < mn) mn = best[i];
    for (int i = 0; i < N / TAU + 1; i++) res += ((best[i] + 32) >> 6) * (best[i] > mn ? TAU : N % TAU);
    return (res + (1 << 9)) >> 10;
}
#endif

int main(int argc, char **argv)
{
    unsigned long long PL = sig_get_pk_len_bytes(), SL = sig_get_sk_len_bytes(), NL = sig_get_sn_len_bytes(), a, b;
    unsigned char *pk = malloc(PL), *sk = malloc(SL), *sig = malloc(NL), m[33], seed[48];
    const char *mode = argc > 1 ? argv[1] : "ctrl";
    for (int i = 0; i < 48; i++) seed[i] = (unsigned char)(11 * i + 7);
    for (int i = 0; i < 33; i++) m[i] = (unsigned char)i;
    init_random_number(&drng_algorithm, seed, 48);
#ifdef SQSING
    int nkeys = argc > 2 ? atoi(argv[2]) : 100, bad = 0, below = 0;
    double bound = GAMMA * GAMMA * N, mx = 0;
    static polyvecl A[K]; poly s0; polyvecl_1 s1; polyveck e; uint8_t key[SEEDBYTES];
    for (int r = 0; r < nkeys; r++) {
        if (sig_keygen(pk, &a, sk, &b)) return 3;
        unpack_sk(A, &s0, &s1, &e, key, sk);
        int64_t t = nsq64(&s0, &s1, &e), code = polyveckl_sqsing_value(&s0, &s1, &e);
        bad += t >= bound; below += code < bound;
        if (t > mx) mx = t;
    }
    printf("DARTS%d keys=%d bound=%.0f code_value_below_bound=%d exact_N(s)_above_bound=%d max_exact=%.0f\n",
           DARTS_MODE, nkeys, bound, below, bad, mx);
    (void)mode; (void)NL;
    return 0;
#endif
    if (sig_keygen(pk, &a, sk, &b) || sig_sign(sk, SL, m, sizeof m, sig, &a)) return 3;
    unsigned long long siglen = a;
    if (!strcmp(mode, "guard") || !strcmp(mode, "ctrl")) {
        long pg = sysconf(_SC_PAGESIZE);
        size_t slack = argc > 2 ? (size_t)atoi(argv[2]) : 0;  /* bytes left between pk and the page end */
        size_t span = ((PL + slack + pg - 1) / pg) * pg;
        unsigned char *map = mmap(NULL, span + pg, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (map == MAP_FAILED) return 4;
        unsigned char *p = map + span - slack - PL;    /* pk ends slack bytes before the page boundary */
        memcpy(p, pk, PL);
        if (mprotect(map + span, pg, !strcmp(mode, "guard") ? PROT_NONE : PROT_READ)) return 5;
        printf("%s: pk %llu bytes ends %zu bytes before the page boundary; verify...\n", mode, PL, slack);
        fflush(stdout);
        int rc = sig_verify(p, PL, sig, siglen, m, sizeof m);
        printf("%s: verify rc=%d\n", mode, rc);
    } else {
        unsigned long long dl = !strcmp(mode, "short") ? PL - 1 : PL;
        unsigned char *t = malloc(dl);
        memcpy(t, pk, dl);
        int rc = sig_verify(t, dl, sig, siglen, m, sizeof m);
        printf("%s: declared=%llu verify rc=%d\n", mode, dl, rc);
        free(t);
    }
    return 0;
}
