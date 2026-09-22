/* Reproduce Tianyuan Xie's one-signature Tins witness-recovery attack.
 *
 * This driver uses the submitted Tins128 implementation to make a fresh key
 * and signature.  It reconstructs two verifier-visible polynomial
 * evaluations, turns their non-subfield coefficients into a binary linear
 * system, solves for (alpha,beta), and checks the recovered witness against
 * the public NSBC relation.  It never reads the secret-key seed after signing.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SIG_TINS128.h"
#include "bavc_commit.h"
#include "drng.h"
#include "ff_arith.h"
#include "params.h"

DRNG_ctx drng_algorithm;

#define D (N_TUPLE - 2)
#define NVARS (2 * D)
#define MAX_ROWS (2 * (K - MU))
#define ROW_WORDS ((NVARS + 1 + 63) / 64)

static uint64_t rows[MAX_ROWS][ROW_WORDS];

static void add_product(fe out, fe a, fe b)
{
    fe t;
    fe_mul(a, b, t);
    fe_add(out, t, out);
}

static void add_scaled(fe out, fe a, ff12b b)
{
    fe t;
    fe_ff12b_mul(a, b, t);
    fe_add(out, t, out);
}

static int field_bit(const fe a, int bit)
{
    return (a[bit / MU] >> (bit % MU)) & 1;
}

static void set_row_bit(uint64_t row[ROW_WORDS], int bit)
{
    row[bit / 64] |= UINT64_C(1) << (bit % 64);
}

static int get_row_bit(const uint64_t row[ROW_WORDS], int bit)
{
    return (row[bit / 64] >> (bit % 64)) & 1;
}

static void xor_row(uint64_t dst[ROW_WORDS], const uint64_t src[ROW_WORDS])
{
    for (int i = 0; i < ROW_WORDS; i++) dst[i] ^= src[i];
}

static int add_execution_rows(
    int first_row, ff12b phi, const ff12b *eval, const fe p_mid,
    fe u[N_TUPLE], fe v[N_TUPLE])
{
    const ff12b *alpha_eval = eval;
    const ff12b *beta_eval = eval + D;
    fe uae, ube, vae, vbe, constant;
    fe_ff12b_inner(u, alpha_eval, D, uae);
    fe_ff12b_inner(u, beta_eval, D, ube);
    fe_ff12b_inner(v, alpha_eval, D, vae);
    fe_ff12b_inner(v, beta_eval, D, vbe);

    memset(constant, 0, sizeof(constant));
    add_product(constant, u[D], vbe);
    add_product(constant, v[D + 1], uae);
    add_product(constant, u[D + 1], vae);
    add_product(constant, v[D], ube);

    fe rhs;
    fe_add(p_mid, constant, rhs);

    fe *ca = calloc(D, sizeof(fe));
    fe *cb = calloc(D, sizeof(fe));
    if (!ca || !cb) {
        free(ca);
        free(cb);
        return -1;
    }

    for (int j = 0; j < D; j++) {
        /* alpha_j coefficient:
         * u_j<v,beta_eval> + v_j<u,beta_eval>
         * + phi (v_(n-1)u_j + u_(n-1)v_j).
         */
        memset(ca[j], 0, sizeof(fe));
        add_product(ca[j], u[j], vbe);
        add_product(ca[j], v[j], ube);
        fe t;
        memset(t, 0, sizeof(t));
        add_product(t, v[D + 1], u[j]);
        add_product(t, u[D + 1], v[j]);
        add_scaled(ca[j], t, phi);

        /* beta_j coefficient:
         * v_j<u,alpha_eval> + u_j<v,alpha_eval>
         * + phi (u_(n-2)v_j + v_(n-2)u_j).
         */
        memset(cb[j], 0, sizeof(fe));
        add_product(cb[j], v[j], uae);
        add_product(cb[j], u[j], vae);
        memset(t, 0, sizeof(t));
        add_product(t, u[D], v[j]);
        add_product(t, v[D], u[j]);
        add_scaled(cb[j], t, phi);
    }

    /* delta is an element of GF(2^12) embedded in the final field limb.
     * The first K-MU coefficient bits are therefore clean equations.
     */
    for (int bit = 0; bit < K - MU; bit++) {
        uint64_t *row = rows[first_row + bit];
        memset(row, 0, ROW_WORDS * sizeof(*row));
        for (int j = 0; j < D; j++) {
            if (field_bit(ca[j], bit)) set_row_bit(row, j);
            if (field_bit(cb[j], bit)) set_row_bit(row, D + j);
        }
        if (field_bit(rhs, bit)) set_row_bit(row, NVARS);
    }

    free(ca);
    free(cb);
    return 0;
}

static int solve(int nrows, unsigned char alpha[N_TUPLE_SIZE],
                 unsigned char beta[N_TUPLE_SIZE])
{
    int pivot_row[NVARS];
    for (int i = 0; i < NVARS; i++) pivot_row[i] = -1;

    int rank = 0;
    for (int col = 0; col < NVARS; col++) {
        int p = rank;
        while (p < nrows && !get_row_bit(rows[p], col)) p++;
        if (p == nrows) continue;
        if (p != rank) {
            for (int w = 0; w < ROW_WORDS; w++) {
                uint64_t t = rows[p][w];
                rows[p][w] = rows[rank][w];
                rows[rank][w] = t;
            }
        }
        for (int r = 0; r < nrows; r++) {
            if (r != rank && get_row_bit(rows[r], col))
                xor_row(rows[r], rows[rank]);
        }
        pivot_row[col] = rank++;
        if (rank == NVARS) break;
    }

    printf("rank=%d/%d (rows=%d)\n", rank, NVARS, nrows);
    if (rank != NVARS) return -1;
    memset(alpha, 0, N_TUPLE_SIZE);
    memset(beta, 0, N_TUPLE_SIZE);
    for (int col = 0; col < NVARS; col++) {
        int value = get_row_bit(rows[pivot_row[col]], NVARS);
        if (!value) continue;
        int which = col >= D;
        int j = col - (which ? D : 0);
        (which ? beta : alpha)[j / 8] |= 1u << (7 - (j & 7));
    }
    return 0;
}

static int witness_valid(fe u[N_TUPLE], fe v[N_TUPLE],
                         const unsigned char alpha[N_TUPLE_SIZE],
                         const unsigned char beta[N_TUPLE_SIZE])
{
    fe ua, ub, va, vb, left, right, diff;
    fe_f2_inner(u, alpha, D, ua);
    fe_add(ua, u[D], ua);
    fe_f2_inner(u, beta, D, ub);
    fe_add(ub, u[D + 1], ub);
    fe_f2_inner(v, alpha, D, va);
    fe_add(va, v[D], va);
    fe_f2_inner(v, beta, D, vb);
    fe_add(vb, v[D + 1], vb);
    fe_mul(ua, vb, left);
    fe_mul(ub, va, right);
    fe_add(left, right, diff);
    return deg(diff) == -1;
}

int main(void)
{
    unsigned char seed[64], msg[32];
    for (size_t i = 0; i < sizeof(seed); i++) seed[i] = (unsigned char)i;
    for (size_t i = 0; i < sizeof(msg); i++) msg[i] = (unsigned char)(0xa5 ^ i);
    init_random_number(&drng_algorithm, seed, sizeof(seed));

    unsigned char pk[PK_SIZE], sk[SK_SIZE], *sig = calloc(1, SIG_SIZE);
    unsigned long long pk_len = PK_SIZE, sk_len = SK_SIZE, sig_len = SIG_SIZE;
    if (!sig || sig_keygen(pk, &pk_len, sk, &sk_len) != 0 ||
        sig_sign(sk, sk_len, msg, sizeof(msg), sig, &sig_len) != 0) {
        fprintf(stderr, "key generation or signing failed\n");
        free(sig);
        return 1;
    }

    unsigned char salt[SALT_SIZE], aux[TAU][N_TUPLE_SIZE * 2];
    hash_t h_piop, h_sh;
    long long ctr;
    node path[T_OPEN];
    commitment proof[TAU];
    fe p_mid[TAU];
    int fixed = SALT_SIZE + (int)sizeof(long long) + (int)sizeof(hash_t) +
                (int)sizeof(commitment) * TAU +
                ((D * TAU * 2 + K * TAU + 7) / 8);
    int path_size = ((int)sig_len - fixed) / NODE_SIZE;
    int pos = 0;
    memcpy(salt, sig + pos, SALT_SIZE); pos += SALT_SIZE;
    memcpy(&ctr, sig + pos, sizeof(ctr)); pos += sizeof(ctr);
    memcpy(h_piop, sig + pos, sizeof(h_piop)); pos += sizeof(h_piop);
    memcpy(path, sig + pos, NODE_SIZE * path_size); pos += NODE_SIZE * path_size;
    memcpy(proof, sig + pos, sizeof(proof)); pos += sizeof(proof);
    fe_decompress(sig + pos, p_mid, TAU); pos += K * TAU / 8;
    decompress_aux(sig + pos, aux, TAU);

    unsigned char grinding;
    int points[TAU];
    ff12b evals[TAU][2 * N_TUPLE - 3];
    memset(evals, 0, sizeof(evals));
    if (ComputeEva(salt, ctr, h_piop, path, path_size, proof, aux,
                   &grinding, points, evals, h_sh) != 0) {
        fprintf(stderr, "could not reconstruct verifier evaluations\n");
        free(sig);
        return 1;
    }

    DRNG_ctx pk_expander;
    init_random_number(&pk_expander, pk, PK_SEEDLEN);
    fe u[N_TUPLE], v[N_TUPLE];
    memset(u, 0, sizeof(u));
    memset(v, 0, sizeof(v));
    unsigned char ubuf[(N_TUPLE * K + 7) / 8];
    unsigned char vbuf[((N_TUPLE - 1) * K + 7) / 8];
    get_random_number(&pk_expander, ubuf, N_TUPLE * K);
    get_random_number(&pk_expander, vbuf, (N_TUPLE - 1) * K);
    fe_decompress(ubuf, u, N_TUPLE);
    fe_decompress(vbuf, v, N_TUPLE - 1);
    fe_decompress(pk + PK_SEEDLEN, v + N_TUPLE - 1, 1);

    if (add_execution_rows(0, (ff12b)points[0], evals[0], p_mid[0], u, v) ||
        add_execution_rows(K - MU, (ff12b)points[1], evals[1], p_mid[1], u, v)) {
        fprintf(stderr, "allocation failed\n");
        free(sig);
        return 1;
    }

    unsigned char alpha[N_TUPLE_SIZE], beta[N_TUPLE_SIZE];
    int ok = solve(MAX_ROWS, alpha, beta) == 0 &&
             witness_valid(u, v, alpha, beta);
    printf("public NSBC witness check: %s\n", ok ? "PASS" : "FAIL");
    printf("result: %s\n", ok ? "TINS128_ONE_SIGNATURE_WITNESS_RECOVERY_CONFIRMED"
                               : "NOT_CONFIRMED");
    free(sig);
    return ok ? 0 : 1;
}
