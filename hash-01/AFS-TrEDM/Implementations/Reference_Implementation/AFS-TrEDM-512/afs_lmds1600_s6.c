/*
 * AFS-LMDS-1600-S6 reference implementation.
 *
 * Reference design goals:
 *   - ISO C99 only;
 *   - direct uint64_t lane-major implementation;
 *   - generic GF(32) xtime/multiply code for auditability;
 *   - no bit gather/scatter and no legacy 25x25 mask table.
 *
 * Algebra:
 *   GF(32) = F_2[a] / (a^5 + a^2 + 1).
 *   A 5-lane column is interpreted as a bitsliced GF(32) symbol
 *   S = S0 + a*S1 + a^2*S2 + a^3*S3 + a^4*S4.
 */

#include "afs_lmds1600_s6.h"

#include <stdint.h>
#include <string.h>

#define AFS_LANES 25U
#define AFS_MOD5(x) ((unsigned)((x) % 5U))
#define AFS_IDX(x,y) ((unsigned)((x) + 5U * (y)))
#define AFS_DIR_INF (-1)

/* Function rotr64: rotates a 64-bit word right by a constant-count modulo 64 amount. */
static uint64_t rotr64(uint64_t x, unsigned n)
{
    n &= 63U;
    return (uint64_t)((x >> n) | (x << ((64U - n) & 63U)));
}

/* Cauchy MDS matrix C over GF(32), coefficients encoded as 5-bit constants. */
static const unsigned char AFS_GF32_C[5][5] = {
    {0x11U, 0x13U, 0x02U, 0x01U, 0x05U},
    {0x10U, 0x06U, 0x01U, 0x02U, 0x09U},
    {0x13U, 0x11U, 0x05U, 0x09U, 0x02U},
    {0x06U, 0x10U, 0x09U, 0x05U, 0x01U},
    {0x01U, 0x09U, 0x10U, 0x11U, 0x06U}
};

/* rho_AFS[x][y]. */
static const unsigned char AFS_RHO[5][5] = {
    { 0U,  1U,  3U,  7U, 19U},
    { 5U,  9U, 23U, 11U, 24U},
    {14U,  6U, 25U,  8U, 21U},
    {10U, 15U, 29U, 37U, 34U},
    {28U, 33U, 39U, 43U, 27U}
};

/* d_r = [INF, 0, 1, 2, 3, 4][r mod 6]. */
static const signed char AFS_S6_DIR[6] = {
    AFS_DIR_INF, 0, 1, 2, 3, 4
};

/* Function gf32_xtime: multiplies a bitsliced GF(32) symbol by alpha in F2[a]/(a^5+a^2+1). */
static void gf32_xtime(uint64_t s[5])
{
    uint64_t t0 = s[4];
    uint64_t t1 = s[0];
    uint64_t t2 = s[1] ^ s[4];
    uint64_t t3 = s[2];
    uint64_t t4 = s[3];

    s[0] = t0;
    s[1] = t1;
    s[2] = t2;
    s[3] = t3;
    s[4] = t4;
}

/* Function gf32_mul_const: multiplies a bitsliced GF(32) symbol by a 5-bit public constant. */
static void gf32_mul_const(uint64_t out[5], const uint64_t in[5], unsigned c)
{
    uint64_t p[5];
    unsigned i, k;

    for (k = 0U; k < 5U; k++) {
        out[k] = 0U;
        p[k] = in[k];
    }

    for (i = 0U; i < 5U; i++) {
        if (((c >> i) & 1U) != 0U) {
            for (k = 0U; k < 5U; k++) {
                out[k] ^= p[k];
            }
        }
        gf32_xtime(p);
    }
}

/*
 * M_col: Treat each coordinate column x as one bitsliced GF(32) symbol
 * U_x = A[x,0] + a A[x,1] + ... + a^4 A[x,4], then compute V = C * U.
 */
/* Function mds_col: performs the Cauchy MDS column-symbol mixing step. */
static void mds_col(uint64_t A[AFS_LANES])
{
    uint64_t U[5][5];
    uint64_t V[5][5];
    uint64_t prod[5];
    unsigned x, y, i, j, k;

    for (x = 0U; x < 5U; x++) {
        for (y = 0U; y < 5U; y++) {
            U[x][y] = A[AFS_IDX(x, y)];
            V[x][y] = 0U;
        }
    }

    for (i = 0U; i < 5U; i++) {
        for (j = 0U; j < 5U; j++) {
            gf32_mul_const(prod, U[j], AFS_GF32_C[i][j]);
            for (k = 0U; k < 5U; k++) {
                V[i][k] ^= prod[k];
            }
        }
    }

    for (x = 0U; x < 5U; x++) {
        for (y = 0U; y < 5U; y++) {
            A[AFS_IDX(x, y)] = V[x][y];
        }
    }
}

/* Function rho_afs: applies the AFS-specific lane rotation table. */
static void rho_afs(uint64_t A[AFS_LANES])
{
    unsigned x, y;
    for (x = 0U; x < 5U; x++) {
        for (y = 0U; y < 5U; y++) {
            A[AFS_IDX(x, y)] = rotr64(A[AFS_IDX(x, y)], AFS_RHO[x][y]);
        }
    }
}

/* pi_AFS: A[x,y] moves to A[(x+y) mod 5, 3x mod 5]. */
static void pi_afs(uint64_t A[AFS_LANES])
{
    uint64_t B[AFS_LANES];
    unsigned x, y, xp, yp;

    for (x = 0U; x < 5U; x++) {
        for (y = 0U; y < 5U; y++) {
            xp = (x + y) % 5U;
            yp = (3U * x) % 5U;
            B[AFS_IDX(xp, yp)] = A[AFS_IDX(x, y)];
        }
    }

    memcpy(A, B, sizeof(B));
}

/* Function mu_afs: applies the per-lane reversible ring diffusion. */
static void mu_afs(uint64_t A[AFS_LANES])
{
    unsigned i;
    for (i = 0U; i < AFS_LANES; i++) {
        uint64_t x = A[i];
        A[i] = x ^ rotr64(x, 17U) ^ rotr64(x, 32U);
    }
}

/* Function lp_core: applies the single-MDS core L_P = mu o pi_AFS o rho_AFS o M_col. */
static void lp_core(uint64_t A[AFS_LANES])
{
    mds_col(A);
    rho_afs(A);
    pi_afs(A);
    mu_afs(A);
}

/*
 * tau_d:
 *   tau_INF(x,y) = (x,y)
 *   tau_d(x,y)   = (y - d*x mod 5, x), d in {0,1,2,3,4}
 *
 * This function applies either tau_d or tau_d^{-1} as a lane permutation:
 *   B[tau(coord)] = A[coord]      when inverse == 0
 *   B[tau^{-1}(coord)] = A[coord] when inverse != 0
 */
/* Function tau_permute: applies tau_d or its inverse for the six-direction S6 schedule. */
static void tau_permute(uint64_t A[AFS_LANES], int d, int inverse)
{
    uint64_t B[AFS_LANES];
    unsigned x, y, dx, dstx, dsty;

    if (d == AFS_DIR_INF) {
        return;
    }

    dx = (unsigned)d;

    for (x = 0U; x < 5U; x++) {
        for (y = 0U; y < 5U; y++) {
            if (!inverse) {
                dstx = (y + 5U - ((dx * x) % 5U)) % 5U;
                dsty = x;
            } else {
                dstx = y;
                dsty = (x + (dx * y)) % 5U;
            }
            B[AFS_IDX(dstx, dsty)] = A[AFS_IDX(x, y)];
        }
    }

    memcpy(A, B, sizeof(B));
}

/* Function afs_lmds1600_s6: applies the reference AFS-LMDS-1600-S6 linear layer for the selected round. */
void afs_lmds1600_s6(uint64_t A[AFS_LANES], unsigned round)
{
    int d = (int)AFS_S6_DIR[round % 6U];

    tau_permute(A, d, 0);
    lp_core(A);
    tau_permute(A, d, 1);
}
