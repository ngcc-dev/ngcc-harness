#ifndef OPSSIG256_MULT_H
#define OPSSIG256_MULT_H

#include <stdint.h>
#include "params.h"
#include "polyvec.h"
#include "poly.h"

/*
 * Smallpoly + earlycheck for OPSsig-256
 *
 * Parameters: N=512, K=L=4, ETA=3, TAU=45, D=13
 *
 * s1/s2 packing — 16-bit slots (2·TAU·ETA = 270 < 2^16):
 *   One uint64_t per table entry holds 4 polys × 16 bits = 64 bits.
 *   Positive entry: each slot = ETA + coeff  (range [0, 2*ETA] = [0, 6])
 *   MASKS_S: 2*ETA = 6 in every 16-bit slot → 0x0006000600060006
 *   Unpack bias: TAU*ETA = 135 per slot
 *
 * t0 packing — 19-bit slots (2·TAU·4096 = 368640 < 2^19):
 *   4×19 = 76 bits > 64 → two tables.
 *   t0_table_lo: vec[0] (bits 19-37) | vec[1] (bits 0-18)
 *   t0_table_hi: vec[2] (bits 19-37) | vec[3] (bits 0-18)
 *   Positive entry: each slot = 4096 + coeff  (range [1, 8192])
 *   MASKT_T0: 8192 = 2*4096 per slot → (8192ULL<<19)|8192ULL = (1ULL<<32)|(1ULL<<13)
 *   Unpack bias: TAU*4096 = 184320 per slot
 */

/* s1/s2 slot constants (16-bit, 4 slots) */
#define MASKS_S_OPS256        0x0006000600060006ULL
/* s unpack bias per slot */
#define S_UNPACK_SUB_OPS256   (TAU * ETA)            /* 135 */

/* t0 slot constants (19-bit, 2 slots per table) */
#define MASKT_T0_OPS256       ((8192ULL << 19) | 8192ULL)   /* (1ULL<<32)|(1ULL<<13) */
/* t0 unpack bias per slot */
#define T0_UNPACK_SUB_OPS256  (TAU * 4096)           /* 184320 */

/* ---- s1 / s2 tables (uint64_t, 16-bit slots) ---- */

/*
 * Build the packed s1 table.
 * s1_table[k+N]: bits 48-63 = ETA+s1[0][k], bits 32-47 = ETA+s1[1][k],
 *                bits 16-31 = ETA+s1[2][k], bits  0-15 = ETA+s1[3][k]
 * s1_table[k]  = MASKS_S_OPS256 - s1_table[k+N]  (complement for c_i=-1)
 */
void prepare_s1_table_ops256(uint64_t s1_table[2*N], polyvecl *s1);

/* Same layout for s2 (K=4 polys). */
void prepare_s2_table_ops256(uint64_t s2_table[2*N], polyveck *s2);

/*
 * Fused cs1+cs2 with per-coefficient early rejection check.
 *
 * Computes:
 *   z       = y  + c*s1      (output)
 *   w0prime = w0 - c*s2      (output)
 *
 * Returns 1 immediately (reject) if any coefficient of z or w0prime exceeds
 * the given bounds (A for z, B for w0prime).  Returns 0 on success.
 *
 * c must be in time domain (coeffs in {-1, 0, 1}).
 * s1_table / s2_table must be prepared by prepare_s*_table_ops256().
 */
int evaluate_cs1_cs2_early_check_ops256(
        polyvecl *z, polyveck *w0prime, const poly *c,
        const uint64_t s1_table[2*N], const uint64_t s2_table[2*N],
        polyvecl *y, polyveck *w0,
        int32_t A, int32_t B);

/* ---- t0 tables (two uint64_t tables, 19-bit slots) ---- */

/*
 * Build two packed t0 tables.
 * t0_table_lo[k+N] = (4096+t0[0][k]) << 19 | (4096+t0[1][k])
 * t0_table_hi[k+N] = (4096+t0[2][k]) << 19 | (4096+t0[3][k])
 * Complement entries at [k] as usual.
 */
void prepare_t0_table_ops256(uint64_t t0_table_lo[2*N], uint64_t t0_table_hi[2*N],
                              polyveck *t0);

/*
 * Compute c*t0 → z.
 * Unpacks all four t0 polynomials from the two tables.
 */
void evaluate_ct0_ops256(polyveck *z, const poly *c,
                         const uint64_t t0_table_lo[2*N],
                         const uint64_t t0_table_hi[2*N]);

#endif /* OPSSIG256_MULT_H */
