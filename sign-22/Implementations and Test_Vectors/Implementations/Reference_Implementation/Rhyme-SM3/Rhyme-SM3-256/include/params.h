#ifndef RHYME_PARAMS_H
#define RHYME_PARAMS_H

#include "config.h"

#ifndef RHYME_MODE
#define RHYME_MODE 256
#endif

#define SEEDBYTES 32
#define CRHBYTES 64
#define CTILDEBYTES (RHYME_MODE / 4)   /* 2*lambda-bit challenge hash: 32/64/96/128 B */

/* Per-mode core parameters + sampler tables (auto-generated). */
#include "params_tables.h"

#define N RHYME_N
#define Q RHYME_Q
#define DQ RHYME_DQ
#define K RHYME_K
#define L RHYME_L
#define TAU RHYME_TAU
#define B0 RHYME_B0
#define B1 RHYME_B1

/* dimensions: A in R_{2q}^{K x M_TOT}, secret s = (1, sgen, egen).
 *
 * Construction 4 (compressed unimodular, "cut-F") with WIDENED column count:
 *   The keygen solver builds a FULL unimodular matrix B in R^{D_SOLVE x D_SOLVE}
 *   with D_SOLVE = D (the per-mode matrix-column count from the parameter
 *   table, RHYME_DMAT) and d = K+L-1.  Its first d rows are SHORT (CBD-eta) and
 *   are the only rows ever used in signing; the remaining D - d rows (including
 *   the long NTRU-solved row) are DISCARDED.  The extra D - d columns are extra
 *   independent masking dimensions.
 *
 *   Why D > d+1 (security fix): if we only widened by the single extension
 *   t = 1 (the old D_SOLVE = d+1), the truncated unimodular construction can
 *   admit a polynomial-time distinguisher against GA-MLWE.  Enlarging the
 *   unimodular column dimension to D and then truncating the trailing rows
 *   removes that algebraic distinguisher.  The cost is that each retained
 *   z_rest coefficient now superposes ~D (rather than ~d+1) short-column
 *   contributions, so its marginal variance grows to
 *       v_marg = sigma_base^2 * (1 + 4 * D * N * (eta/2)),
 *   which is what the parameter-selection script and Table use to derive
 *   B0/B1/B-L2.  D is decoupled from k+l and satisfies D > 4k+1.
 *
 *   The signature uses only B_short = first d (short) rows, of shape d x D.
 *   z_bottom = e_main + 2*B_short*(X0,X') has exactly d retained polys; the
 *   discarded rows never enter the signature norm nor the hiding Gram
 *   G0 = B_short B_short*.
 *
 *   D_UNI  = d        = K+L-1  = #short rows = #retained z_rest polys
 *   D_SOLVE= D        = RHYME_DMAT = full unimodular dimension (rows = columns)
 *   D_REST = d                 = #polys actually transmitted in z_rest
 *   M_SGEN = L-1, K egen polys (unchanged: A is still K x M_TOT, secret length
 *            is d and independent of D). */
#define M_TOT (1 + (L - 1) + K)
#define D_UNI RHYME_D           /* d = K+L-1 : short rows / retained components */
#define D_SOLVE (RHYME_DMAT)    /* D : widened full unimodular matrix dimension */
#define D_REST (D_UNI)          /* number of polys transmitted in z_rest = d */
#define M_SGEN (L - 1)

/* packing bit widths */
#if Q == 3329
#define POLYQ_BITS 12
#define POLY2Q_BITS 13
#elif Q == 9473
#define POLYQ_BITS 14
#define POLY2Q_BITS 15
#elif Q == 11777
#define POLYQ_BITS 14
#define POLY2Q_BITS 15
#elif Q == 18433
#define POLYQ_BITS 15
#define POLY2Q_BITS 16
#else
#error "unsupported Q"
#endif

#define POLYQ_PACKEDBYTES ((N * POLYQ_BITS + 7) / 8)
#define POLY_PACKEDBYTES_2Q ((N * POLY2Q_BITS + 7) / 8)

/* secret key B storage: signed coefficients.
 * small columns: |coeff| <= ETA  (CBD)
 * last row of our B (the transposed big column): bounded by BIGCOEF_BITS budget */
#define SK_SMALL_BITS 8
#define SK_BIG_BITS 16

#define CRYPTO_PUBLICKEYBYTES (SEEDBYTES + K * POLYQ_PACKEDBYTES)
/* sk holds the full (d+1)x(d+1) unimodular basis:
 *   small block: d short rows x (d+1) cols  (int8 / SK_SMALL_BITS)
 *   (the long (d+1)-th row F is NOT stored under cut-F: it is never used in
 *    signing; keygen only verifies its existence via coprimality)          */
#define CRYPTO_SECRETKEYBYTES (CRYPTO_PUBLICKEYBYTES \
        + (D_UNI * D_SOLVE * N * SK_SMALL_BITS) / 8 \
        + SEEDBYTES)

/* signature: [u16 total_len][c_tilde][rANS zblock]; CRYPTO_BYTES is the padded cap */
#define CRYPTO_BYTES (2 + CTILDEBYTES + 2 + (1 + D_REST) * N * 4)

#define MAX_SIGN_ITERATIONS 1023

#endif
