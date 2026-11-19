/* sign-02: when the allocation inside BiT's seed_y derivation fails, signing
 * still returns a valid signature, made with whatever its seed_y buffer held.
 *
 * bit_xof256_3 (symmetric.c) is void and discards pseudoXOF's status; sign.c
 * then expands every mask from the unwritten temp_seed_y. An ordinary signing
 * call ends with secure_zero(temp_seed_y), so the next call that hits the
 * failure in the same stack frame uses the all-zero seed: the mask y is public.
 *
 * Linked with -Wl,--wrap=malloc. For each round the driver makes one ordinary
 * signature and then, with no other call in between, one with exactly one
 * allocation failing: pseudoXOF's copy of secret_seed || rnd || mu plus a
 * 4-byte counter (2*SEEDBYTES + MESSAGEBYTES + 4 bytes, independent of the
 * message). Two public recoveries, from pk and signatures only:
 *  single: expand y from the zero seed (first iteration, nonce 0); if
 *          z_{1,0} - y_0 = +-c, solve (+-c) * s0_i = z_{1,1+i} - y_{1+i};
 *  pair:   two faulty signatures with equal verifier commitment w' used the
 *          same y, so d0 = z_{1,0} - z'_{1,0} and d0 * s0_i = z_{1,1+i} - z'_{1,1+i}
 *          (the sign-02-3 relation).
 * Solves are in Z_q[X]/(X^N+1). The secret key is read only to report
 * agreement. Control: the same tests on the ordinary signatures. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "params.h"
#include "poly.h"
#include "polyvec.h"
#include "packing.h"
#include "drng.h"
#include "SIG_AlgorithmInstance.h"

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

#define N BIT_N
#define Q BIT_Q
#define ROUNDS 16
#define MLEN 32
#define SEEDY_ALLOC (2 * BIT_SEEDBYTES + BIT_MESSAGEBYTES + 4)

static unsigned char pk[BIT_PUBLICKEYBYTES], sk[BIT_SECRETKEYBYTES];
static unsigned long long pkl, skl;

/* All signing goes through one non-inlined call, so sig_sign's frame sits at
 * the same stack address each time. hit reports whether the allocation was reached. */
__attribute__((noinline)) static int sign(unsigned char *sig, const unsigned char *m, size_t n, int *hit)
{
    unsigned long long sl;
    int r;
    fail_size = n;
    r = sig_sign(sk, skl, (unsigned char *)m, MLEN, sig, &sl);
    *hit = n && !fail_size;
    fail_size = 0;
    return r;
}

static int64_t md(int64_t a) { a %= Q; return a < 0 ? a + Q : a; }
static int64_t inv(int64_t a)
{
    int64_t r = 1, e = Q - 2;
    a = md(a);
    while (e) { if (e & 1) r = r * a % Q; a = a * a % Q; e >>= 1; }
    return r;
}

/* Solve d * x_r = rhs_r in Z_q[X]/(X^N+1) for BIT_L right-hand sides. */
static int solve(int32_t x[BIT_L][N], const int32_t d[N], int32_t rhs[BIT_L][N])
{
    static int64_t M[N][N + BIT_L];
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            int k = i - j;
            M[i][j] = k >= 0 ? md(d[k]) : md(-d[k + N]);
        }
        for (int r = 0; r < BIT_L; ++r) M[i][N + r] = md(rhs[r][i]);
    }
    for (int col = 0; col < N; ++col) {
        int p = -1;
        for (int r = col; r < N; ++r) if (M[r][col]) { p = r; break; }
        if (p < 0) return -1;
        if (p != col)
            for (int j = 0; j < N + BIT_L; ++j) { int64_t t = M[p][j]; M[p][j] = M[col][j]; M[col][j] = t; }
        int64_t iv = inv(M[col][col]);
        for (int j = col; j < N + BIT_L; ++j) M[col][j] = M[col][j] * iv % Q;
        for (int r = 0; r < N; ++r) {
            if (r == col || !M[r][col]) continue;
            int64_t f = M[r][col];
            for (int j = col; j < N + BIT_L; ++j) M[r][j] = md(M[r][j] - f * M[col][j]);
        }
    }
    for (int r = 0; r < BIT_L; ++r)
        for (int i = 0; i < N; ++i) {
            int64_t v = M[i][N + r];
            x[r][i] = (int32_t)(v > Q / 2 ? v - Q : v);
        }
    return 0;
}

/* Public test: is the signature's mask the expansion of the all-zero seed? */
static int zero_seed_mask(const unsigned char *sig, polyvecm1 *z, poly *c, polyvecy *y, int *sgn)
{
    polyveck h;
    unsigned char ch[BIT_CHALLENGEBYTES];
    uint8_t zero[BIT_SEEDBYTES] = {0};
    uint16_t nonce = 0;
    if (unpack_sig(z, &h, ch, sig)) return 0;
    poly_challenge(c, ch);
    polyvecy_sample_triangular(y, zero, &nonce);
    for (int s = -1; s <= 1; s += 2) {
        int eq = 1;
        for (int j = 0; j < N; j++) eq &= z->vec[0].coeffs[j] - y->vec[0].coeffs[j] == s * c->coeffs[j];
        if (eq) { *sgn = s; return 1; }
    }
    return 0;
}

/* The verifier's commitment w' = UseHint(HighBits(...), h), from (pk, sig). */
static void commitment(uint8_t *out, const uint8_t *sig)
{
    unsigned char seed_A[BIT_SEEDBYTES], ch[BIT_CHALLENGEBYTES];
    static poly_matrix_ntt A;
    polyveck b1, h, wh, wp; polyveck_ntt b1n; polyvecl zt; polyvecl_ntt ztn; poly_ntt z0n; polyvecm1 z1; poly c;
    unpack_pk(seed_A, &b1, pk);
    memset(out, 0, BIT_POLYVECK_W1_BYTES);
    if (unpack_sig(&z1, &h, ch, sig) != 0) return;
    poly_matrix_expand_ntt(&A, seed_A);
    polyveck_b1_scaled_to_ntt(&b1n, &b1);
    poly_challenge(&c, ch);
    for (int i = 0; i < BIT_L; i++) zt.vec[i] = z1.vec[i + 1];
    polyvecl_to_ntt(&ztn, &zt);
    poly_to_ntt(&z0n, &z1.vec[0]);
    polyveck_compute_w_hat_prime_ntt(&wh, &A, &b1n, &ztn, &z0n, &z1.vec[0], &c);
    polyveck_compute_w_prime(&wp, &wh, &h);
    polyveck_pack_w1(out, &wp);
}

static polyvecl s0;
static int32_t d[N], rhs[BIT_L][N], x[BIT_L][N];

static int solve_matches_key(void)
{
    if (solve(x, d, rhs) != 0) return 0;
    for (int i = 0; i < BIT_L; i++)
        for (int j = 0; j < N; j++) if (x[i][j] != s0.vec[i].coeffs[j]) return 0;
    return 1;
}

/* Over a set of signatures: count zero-seed masks and equal-commitment pairs,
 * and solve for s0 from the first of each (when solve is set). */
static void attack(unsigned char sigs[ROUNDS][BIT_SIGNBYTES], int solve_it,
                   int *zero_mask, int *single_ok, int *pairs, int *pair_ok)
{
    static uint8_t w[ROUNDS][BIT_POLYVECK_W1_BYTES];
    polyvecm1 z, z2; polyveck h; unsigned char ch[BIT_CHALLENGEBYTES]; polyvecy y; poly c; int sgn;
    *zero_mask = *single_ok = *pairs = *pair_ok = 0;
    for (int t = 0; t < ROUNDS; t++) {
        if (!zero_seed_mask(sigs[t], &z, &c, &y, &sgn)) continue;
        if ((*zero_mask)++ || !solve_it) continue;
        for (int j = 0; j < N; j++) d[j] = sgn * c.coeffs[j];
        for (int i = 0; i < BIT_L; i++)
            for (int j = 0; j < N; j++) rhs[i][j] = z.vec[1 + i].coeffs[j] - y.vec[1 + i].coeffs[j];
        *single_ok = solve_matches_key();
    }
    for (int t = 0; t < ROUNDS; t++) commitment(w[t], sigs[t]);
    for (int a = 0; a < ROUNDS; a++)
        for (int b = a + 1; b < ROUNDS; b++) {
            if (memcmp(w[a], w[b], BIT_POLYVECK_W1_BYTES) != 0) continue;
            if ((*pairs)++ || !solve_it) continue;
            unpack_sig(&z, &h, ch, sigs[a]);
            unpack_sig(&z2, &h, ch, sigs[b]);
            for (int j = 0; j < N; j++) d[j] = z.vec[0].coeffs[j] - z2.vec[0].coeffs[j];
            for (int i = 0; i < BIT_L; i++)
                for (int j = 0; j < N; j++) rhs[i][j] = z.vec[1 + i].coeffs[j] - z2.vec[1 + i].coeffs[j];
            *pair_ok = solve_matches_key();
        }
}

int main(void)
{
    static unsigned char faulty[ROUNDS][BIT_SIGNBYTES], ordinary[ROUNDS][BIT_SIGNBYTES];
    unsigned char seed[48], m[MLEN], sA[BIT_SEEDBYTES], ss[BIT_SEEDBYTES], tr[BIT_TRBYTES];
    polyveck b1, e, b0;
    int hit, reached = 0, valid = 0, zm, so, pr, po, ozm, oso, opr, opo;

    for (int i = 0; i < 48; i++) seed[i] = (unsigned char)(5 * i + 3);
    init_random_number(&drng_algorithm, seed, 48);
    if (sig_keygen(pk, &pkl, sk, &skl) != 0) { printf("keygen failed\n"); return 1; }
    unpack_sk(sA, &b1, ss, tr, &s0, &e, &b0, sk);           /* evaluation only */

    for (int t = 0; t < ROUNDS; t++) {
        memset(m, 0x80 + t, MLEN);                           /* ordinary signature, then at once */
        if (sign(ordinary[t], m, 0, &hit) != 0) { printf("ordinary signing failed\n"); return 1; }
        memset(m, t, MLEN);                                  /* one whose seed_y allocation fails */
        int r = sign(faulty[t], m, SEEDY_ALLOC, &hit);
        reached += hit;
        valid += r == 0 && sig_verify(pk, pkl, faulty[t], BIT_SIGNBYTES, m, MLEN) == 0;
    }
    attack(faulty, 1, &zm, &so, &pr, &po);
    attack(ordinary, 0, &ozm, &oso, &opr, &opo);
    printf("allocation=%d reached=%d/%d faulty_valid=%d zero_seed_masks=%d single_s0_equal=%d "
           "equal_commitment_pairs=%d pair_s0_equal=%d ordinary_zero_seed_masks=%d ordinary_pairs=%d\n",
           SEEDY_ALLOC, reached, ROUNDS, valid, zm, so, pr, po, ozm, opr);
    return 0;
}
