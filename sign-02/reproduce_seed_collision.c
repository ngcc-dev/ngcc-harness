/* sign-02-3 BiT-128: masking-seed collisions recover an equivalent key.
 *
 * Built by sign-02/reproduce_seed_collision.sh against a temporary copy of
 * the submitted BiT-128 reference tree. With -DNGCC_SEED_BITS=w the copy keeps
 * only w bits of the 128-bit seed_y (reduced-width model); without it the
 * shipped width is used (control).
 *
 * Usage: reproduce_seed_collision <signatures> [drbg-variant]
 *
 * Ordinary signatures on distinct messages come from sig_sign. The ATTACK
 * phase reads only pk, messages and signatures:
 *  1. recompute the verifier's commitment w' for every signature and sort;
 *  2. on a match between distinct messages (same y),
 *       d0 = z_{1,0} - z'_{1,0} = (-1)^b c - (-1)^b' c'  and
 *       z_{1,1,i} - z'_{1,1,i} = s0_i * d0  exactly;
 *  3. solve s0 mod q, complete (e', b0') from pk, sign a fresh message with
 *     the equivalent key and check it with the unmodified verifier.
 * The oracle secret key is read only after recovery, to report agreement.
 */
#include "params.h"
#include "poly.h"
#include "polyvec.h"
#include "packing.h"
#include "symmetric.h"
#include "drng.h"
#include "SIG_AlgorithmInstance.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

DRNG_ctx drng_algorithm;
#define MSGLEN 32
#define Q BIT_Q
#define N BIT_N

typedef struct { uint64_t h; unsigned idx; } entry;
static int cmp_entry(const void *a, const void *b)
{
    const entry *x = a, *y = b;
    if (x->h != y->h) return x->h < y->h ? -1 : 1;
    return (int)x->idx - (int)y->idx;
}
static uint64_t fnv(const uint8_t *p, size_t n)
{
    uint64_t h = 1469598103934665603ULL;
    while (n--) { h ^= *p++; h *= 1099511628211ULL; }
    return h;
}

/* The verifier's commitment w' = UseHint(HighBits(...), h), from (pk, sig). */
static int public_commitment(uint8_t *out, const uint8_t *pk, const uint8_t *sig)
{
    unsigned char seed_A[BIT_SEEDBYTES], ch[BIT_CHALLENGEBYTES];
    poly_matrix_ntt A; polyveck b1, h, wh, wp; polyveck_ntt b1n;
    polyvecl zt; polyvecl_ntt ztn; poly_ntt z0n; polyvecm1 z1; poly c;
    unpack_pk(seed_A, &b1, pk);
    if (unpack_sig(&z1, &h, ch, sig) != 0) return -1;
    poly_matrix_expand_ntt(&A, seed_A);
    polyveck_b1_scaled_to_ntt(&b1n, &b1);
    poly_challenge(&c, ch);
    for (int i = 0; i < BIT_L; i++) zt.vec[i] = z1.vec[i + 1];
    polyvecl_to_ntt(&ztn, &zt);
    poly_to_ntt(&z0n, &z1.vec[0]);
    polyveck_compute_w_hat_prime_ntt(&wh, &A, &b1n, &ztn, &z0n, &z1.vec[0], &c);
    polyveck_compute_w_prime(&wp, &wh, &h);
    polyveck_pack_w1(out, &wp);
    return 0;
}

static int64_t md(int64_t a) { a %= Q; return a < 0 ? a + Q : a; }
static int64_t inv(int64_t a)
{
    int64_t r = 1, e = Q - 2; a = md(a);
    while (e) { if (e & 1) r = r * a % Q; a = a * a % Q; e >>= 1; }
    return r;
}

/* Solve d0 * x = rhs[k] in Z_q[X]/(X^N+1) for BIT_L right-hand sides. */
static int solve(int32_t x[BIT_L][N], const int32_t d0[N], int32_t rhs[BIT_L][N])
{
    static int64_t M[N][N + BIT_L];
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            int k = i - j;
            M[i][j] = k >= 0 ? md(d0[k]) : md(-d0[k + N]);
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

/* Public completion: r = A0 s0 - gamma_b b1 (mod q) must equal b0 - e with
 * b0 in [-gamma_b/2, gamma_b/2) and e in {-1,0,1}. */
static int complete_key(uint8_t *sk_out, const uint8_t *pk, const polyvecl *s0)
{
    unsigned char seed_A[BIT_SEEDBYTES], ss[BIT_SEEDBYTES] = {0}, tr[BIT_TRBYTES];
    poly_matrix_ntt A; polyveck b1, bs, e, b0; polyvecl_ntt s0n;
    for (int i = 0; i < BIT_L; ++i)
        for (int j = 0; j < N; ++j)
            if (s0->vec[i].coeffs[j] < -1 || s0->vec[i].coeffs[j] > 1) return -2;
    unpack_pk(seed_A, &b1, pk);
    poly_matrix_expand_ntt(&A, seed_A);
    polyvecl_to_ntt(&s0n, s0);
    poly_matrix_mul_vector_ntt(&bs, &A, &s0n);
    const int lo = -(BIT_GAMMA_B / 2), hi = BIT_GAMMA_B / 2 - 1;
    for (int i = 0; i < BIT_K; ++i)
        for (int j = 0; j < N; ++j) {
            int64_t r = md((int64_t)bs.vec[i].coeffs[j] - (int64_t)BIT_GAMMA_B * b1.vec[i].coeffs[j]);
            if (r > Q / 2) r -= Q;
            if (r < lo - 1 || r > hi + 1) return -1;
            int ee = 0, bb = (int)r;
            if (r < lo) { bb = lo; ee = 1; }
            if (r > hi) { bb = hi; ee = -1; }
            e.vec[i].coeffs[j] = (int16_t)ee; b0.vec[i].coeffs[j] = (int16_t)bb;
        }
    bit_h256(tr, pk, BIT_PUBLICKEYBYTES);
    pack_sk(sk_out, seed_A, &b1, ss, tr, s0, &e, &b0);
    return 0;
}

static int recover_pair(polyvecl *s0, const uint8_t *sa, const uint8_t *sb, int verbose)
{
    polyvecm1 za, zb; polyveck h; unsigned char ch[BIT_CHALLENGEBYTES];
    static int32_t d0[N], rhs[BIT_L][N], x[BIT_L][N];
    if (unpack_sig(&za, &h, ch, sa) || unpack_sig(&zb, &h, ch, sb)) return -3;
    int nz = 0, mx = 0;
    for (int j = 0; j < N; ++j) {
        d0[j] = za.vec[0].coeffs[j] - zb.vec[0].coeffs[j];
        if (d0[j]) nz++;
        if (abs(d0[j]) > mx) mx = abs(d0[j]);
    }
    for (int i = 0; i < BIT_L; ++i)
        for (int j = 0; j < N; ++j) rhs[i][j] = za.vec[1 + i].coeffs[j] - zb.vec[1 + i].coeffs[j];
    if (verbose) printf("  d0 = z10-z10': %d nonzero coefficients, max |d0_j| = %d\n", nz, mx);
    if (solve(x, d0, rhs) != 0) { if (verbose) printf("  d0 singular mod q\n"); return -2; }
    for (int i = 0; i < BIT_L; ++i)
        for (int j = 0; j < N; ++j) s0->vec[i].coeffs[j] = (int16_t)x[i][j];
    return 0;
}

int main(int argc, char **argv)
{
    unsigned nsig = argc > 1 ? (unsigned)strtoul(argv[1], 0, 10) : 4096;
    unsigned char seed[48];
    for (unsigned i = 0; i < sizeof seed; ++i) seed[i] = (unsigned char)(11 + 29 * i);
    if (argc > 2) seed[0] ^= (unsigned char)strtoul(argv[2], 0, 10);
    if (init_random_number(&drng_algorithm, seed, sizeof seed) != 0) return 2;
#ifdef NGCC_SEED_BITS
    printf("model: reduced-width seed_y (%d of %d bits kept)\n", NGCC_SEED_BITS, 8 * BIT_SEEDBYTES);
#else
    printf("model: shipped seed_y width (%d bits)\n", 8 * BIT_SEEDBYTES);
#endif
    unsigned long long pkl = 0, skl = 0, sl = 0;
    size_t SL = (size_t)sig_get_sn_len_bytes();
    uint8_t *pk = malloc(BIT_PUBLICKEYBYTES), *sk = malloc(BIT_SECRETKEYBYTES), *eq = malloc(BIT_SECRETKEYBYTES);
    uint8_t *sigs = malloc(SL * nsig), *fresh = malloc(SL);
    uint8_t (*enc)[BIT_POLYVECK_W1_BYTES] = malloc((size_t)nsig * BIT_POLYVECK_W1_BYTES);
    entry *E = malloc(sizeof(entry) * nsig);
    if (!pk || !sk || !eq || !sigs || !fresh || !enc || !E) return 2;
    if (sig_keygen(pk, &pkl, sk, &skl) != 0) return 2;
    for (unsigned q = 0; q < nsig; ++q) {
        uint8_t msg[MSGLEN] = {0};
        memcpy(msg, &q, sizeof q);
        if (sig_sign(sk, skl, msg, MSGLEN, sigs + SL * q, &sl) != 0 || sl != SL) return 3;
        if (sig_verify(pk, pkl, sigs + SL * q, sl, msg, MSGLEN) != 0) return 4;
    }
    printf("honest: %u signatures on distinct messages, all verify\n", nsig);
    for (unsigned q = 0; q < nsig; ++q) {
        if (public_commitment(enc[q], pk, sigs + SL * q) != 0) return 5;
        E[q].h = fnv(enc[q], BIT_POLYVECK_W1_BYTES); E[q].idx = q;
    }
    qsort(E, nsig, sizeof(entry), cmp_entry);
    unsigned matches = 0, recovered = 0, shown = 0;
    int done = 0, v1 = 99, v2 = 99, same = -1;
    for (unsigned t = 1; t < nsig; ++t) {
        if (E[t].h != E[t - 1].h) continue;
        unsigned a = E[t - 1].idx, b = E[t].idx;
        if (memcmp(enc[a], enc[b], BIT_POLYVECK_W1_BYTES) != 0) continue;
        matches++;
        int verbose = shown < 3;
        if (verbose) { printf("commitment match: signatures %u and %u (distinct messages)\n", a, b); shown++; }
        polyvecl s0;
        if (recover_pair(&s0, sigs + SL * a, sigs + SL * b, verbose) != 0) continue;
        if (complete_key(eq, pk, &s0) != 0) { if (verbose) printf("  public completion rejected\n"); continue; }
        if (verbose) printf("  public completion accepted: A0*s0 - gamma_b*b1 = b0 - e is short\n");
        recovered++;
        if (done) continue;
        const uint8_t m[] = "sign-02-3: BiT seed-collision forgery";
        unsigned long long fl = 0;
        if (sig_sign(eq, skl, (uint8_t *)m, sizeof m - 1, fresh, &fl) != 0) return 6;
        v1 = sig_verify(pk, pkl, fresh, fl, (uint8_t *)m, sizeof m - 1);
        uint8_t m2[sizeof m]; memcpy(m2, m, sizeof m); m2[0] ^= 1;
        v2 = sig_verify(pk, pkl, fresh, fl, m2, sizeof m - 1);
        printf("  fresh-message forgery: verify=%d (0=accept); changed-message control: verify=%d\n", v1, v2);
        {
            unsigned char sA[BIT_SEEDBYTES], ss[BIT_SEEDBYTES], tr[BIT_TRBYTES];
            polyveck b1, e, b0; polyvecl ts;
            unpack_sk(sA, &b1, ss, tr, &ts, &e, &b0, sk);
            same = 1;
            for (int i = 0; i < BIT_L; ++i)
                for (int j = 0; j < N; ++j) same &= ts.vec[i].coeffs[j] == s0.vec[i].coeffs[j];
            printf("  [evaluation] recovered s0 equals oracle s0: %s\n", same ? "yes" : "no");
        }
        done = 1;
    }
    printf("RESULT matches=%u completed=%u forgery=%d changed=%d s0_equal=%d\n",
           matches, recovered, v1, v2, same);
    {
        polyvecl s0;
        int rc = recover_pair(&s0, sigs, sigs + SL, 0);
        int cc = rc == 0 ? complete_key(eq, pk, &s0) : -9;
        printf("RESULT nonmatching_pair recover=%d completion=%d\n", rc, cc);
    }
    return 0;
}
