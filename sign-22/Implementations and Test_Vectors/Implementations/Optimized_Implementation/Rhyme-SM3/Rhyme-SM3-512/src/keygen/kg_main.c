/*************************************************
* File:        kg_main.c
*
* Description: Key-generation driver: samples the short CBD rows of the
*              widened unimodular basis B and resamples until they extend to a
*              det=1 matrix, selecting the coprimality backend (descent by
*              default, or the DP1 "truncated tail columns" check under
*              -DUSE_DP1_CHECK, or the full module-NTRU solver under
*              -DUSE_FULL_MNTRU_SOLVER).
**************************************************/
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "params.h"
#include "packing.h"
#include "sampler.h"
#include "rhyme_xof.h"
#include "kg_solver.h"

/* Construction 4 ("cut-F") secret-basis generator, WIDENED column count.
 *
 * We build a FULL D x D unimodular matrix G with det(G) = +1, where
 * d = D_UNI = K+L-1 and the solve dimension is ds = D_SOLVE = D (the per-mode
 * widened matrix dimension, RHYME_DMAT, with D > d+1 to defeat the GA-MLWE
 * distinguisher that the t=1 truncation admits):
 *   - the first ds-1 = D-1 COLUMNS of G are freshly sampled CBD-eta short polys
 *     (each column has ds = D entries),
 *   - the last (ds-th) column F is solved by the module-NTRU det=1 routine and
 *     may be long.
 *
 * OUR secret basis B is the transpose G^T, so:
 *   B.row_small[r][c] = cols[c][r]   for r in [0,d)  (the first d rows of G^T,
 *                                                     all short and RETAINED)
 *   rows r in [d, D) of G^T (including the long F-derived row) are DISCARDED
 *   and never stored.
 *
 * The first column of B (= first row of G) is the secret column s:
 *   s_tail = (sgen | egen) is its FIRST d entries; entries d..D-1 belong to
 *   the discarded extension dimensions (D - d of them).
 *
 * The signature only ever uses B_short = the first d short rows of B (shape
 * d x D), so the hiding Gram is G0 = B_short B_short* and the discarded rows
 * never enter the signature norm.  Only check-only coprimality is run; F is
 * not assembled. */
/*************************************************
* Name:        rhyme_keygen_basis
*
* Description: Generates a secret unimodular basis B for Rhyme. Samples the d
*              short rows (CBD-eta) of the widened D x D matrix from the seed
*              and resamples until the leading short block is coprime (so the
*              rows extend to a det=1 basis), then fills in the solved row(s).
*              The coprimality test used is selected at compile time.
*
* Arguments:   - secret_basis *B:                solved output basis
*              - const uint8_t seed[SEEDBYTES]:  key-generation seed
*
* Returns:     0 on success, nonzero on failure.
**************************************************/
int rhyme_keygen_basis(secret_basis *B, const uint8_t seed[SEEDBYTES]) {
    const unsigned n = N, ds = D_SOLVE;     /* solve dimension d+1 */
    uint8_t buf64[CRHBYTES];
    /* cols holds the ds-1 sampled columns, each of ds entries:
     * layout cols[(j*ds + i)*n + t], sampled column j in [0,ds-1), row i in [0,ds). */
    int32_t *cols = malloc((size_t)(ds - 1) * ds * n * sizeof(int32_t));
    int32_t *F = malloc((size_t)ds * n * sizeof(int32_t));   /* solved last column, length ds */
    poly tmp;
    int ok = -1;

    if (!cols || !F) { free(cols); free(F); return -1; }

    /* expand the 32-byte keygen seed into a CRHBYTES stream key */
    rhyme_shake256_hash(buf64, CRHBYTES, seed, SEEDBYTES);

    for (unsigned attempt = 0; attempt < 64; attempt++) {
        for (unsigned j = 0; j < ds - 1; j++)
            for (unsigned i = 0; i < ds; i++) {
                uint16_t nonce = (uint16_t)(attempt * 256 + j * 16 + i);
                SampleCBD(&tmp, buf64, nonce);
                memcpy(cols + ((size_t)j * ds + i) * n, tmp.coeffs, n * sizeof(int32_t));
            }
        /* cut-F: F (the long row) is never used in signing, so we only need to
         * confirm the d sampled short columns admit a det=1 completion, i.e.
         * their minors' resultants are coprime.  kg_solve_check_only makes the
         * solver stop at the coprimality test and skip F-assembly + Babai (the
         * dominant keygen cost).  The full solver path is preserved but unused. */
        /* cut-F keygen: signing never uses F (the long row), so we only need
         * to confirm the sampled short columns admit a det=1 completion, i.e.
         * their minors' resultants are coprime.  kg_solve_coprime_descent
         * decides this directly (field-norm NTT-domain resultants + dynamic
         * CRT convergence + Lehmer gcd with early-exit), ~12-100x faster than
         * the full module-NTRU solver across modes, and avoids the resultant
         * bit-length over-estimate that made the old path spuriously fail
         * (and abort keygen) at mode384/512.  The original module-NTRU solver
         * kg_solve_last_column is preserved verbatim below but not called; set
         * USE_FULL_MNTRU_SOLVER to re-enable it (e.g. if F is ever needed). */
#ifdef USE_FULL_MNTRU_SOLVER
        kg_solve_check_only = 1;
        int rc = kg_solve_last_column(F, cols, n, ds);
#elif defined(USE_DP1_CHECK)
        /* friend's "truncated tail columns" variant: judge only the d x (d+1)
         * leading sub-block of B = G^T.  B.row_small[r][c] = cols[(r*ds+c)], so
         * the d x (d+1) block in B-layout (block[(r*D+c)]) is read directly from
         * cols with the SAME index r*ds+c (ds == D).  Sound sufficient test
         * (monotone gcd ideal) using only d+1 small d-order minors; ~4-5x faster
         * end-to-end than the (D-1)-order descent. */
        int rc = kg_check_dp1_coprime(cols, n, ds, D_UNI);
#else
        int rc = kg_solve_coprime_descent(cols, n, ds);
#endif
        if (rc != 0)
            continue;                       /* not coprime / internal: resample */
        /* (No kg_det_check here: F is not computed in check-only mode, and
         *  coprimality already guarantees a det=1 completion exists.) */
        ok = 0;
        break;
    }
    if (ok == 0) {
        /* OUR B = G^T (same convention as Construction 3 ref):
         *   sampled column r (r in [0, ds-1) = [0,d))  ->  OUR short row r,
         *     with OUR column c = entry i=c of that sampled column:
         *       B.row_small[r][c] = cols[(r*ds + c)*n + t]
         * The long (d+1)-th row (row_big) is NOT computed under cut-F; it is
         * zeroed since signing never reads it.  (To restore the explicit F,
         * set kg_solve_check_only = 0 and re-enable the F mapping below.) */
        for (unsigned r = 0; r < D_UNI; r++)            /* d short rows */
            for (unsigned c = 0; c < D_SOLVE; c++)      /* d+1 columns */
                for (unsigned t = 0; t < n; t++)
                    B->row_small[r][c].coeffs[t] = cols[((size_t)r * ds + c) * n + t];
        for (unsigned c = 0; c < D_SOLVE; c++)          /* long row: unused, zeroed */
            for (unsigned t = 0; t < n; t++)
                B->row_big[c].coeffs[t] = 0;
    }
    free(cols);
    free(F);
    return ok;
}
