#ifndef OPSSIG512_MULT_H
#define OPSSIG512_MULT_H

#include <stdint.h>
#include "params.h"
#include "polyvec.h"
#include "poly.h"

/*
 * Smallpoly + earlycheck for OPSsig-512
 *
 * Parameters: N=1024, K=L=4, ETA=2, TAU=90, D=13
 *
 * s1/s2 packing — 16-bit slots (2·TAU·ETA = 360 < 2^16):
 *   One uint64_t per entry holds 4 polys × 16 bits = 64 bits.
 *   Each positive slot = ETA + coeff  (range [0, 2*ETA] = [0, 4])
 *   MASKS_S: 2*ETA = 4 in every 16-bit slot → 0x0004000400040004
 *   Unpack bias: TAU*ETA = 180 per slot
 *
 * t0 packing — 20-bit slots (2·TAU·4096 = 737280 < 2^20 = 1048576):
 *   4×20 = 80 > 64 → two tables (same split as OPS-256 but wider slots).
 *   t0_table_lo: vec[0] (bits 20-39) | vec[1] (bits 0-19)
 *   t0_table_hi: vec[2] (bits 20-39) | vec[3] (bits 0-19)
 *   Positive slot = 4096 + coeff  (range [1, 8192])
 *   MASKT_T0: 8192 per slot → (8192ULL<<20)|8192ULL = (1ULL<<33)|(1ULL<<13)
 *   Unpack mask: 0xFFFFF (20 bits)
 *   Unpack bias: TAU*4096 = 368640 per slot
 */

/* s1/s2 constants (16-bit, 4 slots per uint64_t) */
#define MASKS_S_OPS512        0x0004000400040004ULL   /* 2*ETA=4 per slot */
#define S_UNPACK_SUB_OPS512   (TAU * ETA)             /* 180 */

/* t0 constants (20-bit, 2 slots per uint64_t table) */
#define MASKT_T0_OPS512       ((8192ULL << 20) | 8192ULL)  /* (1ULL<<33)|(1ULL<<13) */
#define T0_UNPACK_SUB_OPS512  (TAU * 4096)            /* 368640 */

/* ---- s1 / s2 tables ---- */

/*
 * s1_table[k+N]: bits 48-63 = ETA+s1[0][k], ..., bits 0-15 = ETA+s1[3][k]
 * s1_table[k]  = MASKS_S_OPS512 - s1_table[k+N]
 */
void prepare_s1_table_ops512(uint64_t s1_table[2*N], polyvecl *s1);
void prepare_s2_table_ops512(uint64_t s2_table[2*N], polyveck *s2);

/*
 * Fused cs1+cs2 with per-coefficient early rejection.
 * Returns 1 (reject) if |z| >= A or |w0prime| >= B.
 * c must be time-domain (coeffs in {-1,0,1}).
 */
int evaluate_cs1_cs2_early_check_ops512(
        polyvecl *z, polyveck *w0prime, const poly *c,
        const uint64_t s1_table[2*N], const uint64_t s2_table[2*N],
        polyvecl *y, polyveck *w0,
        int32_t A, int32_t B);

/* ---- t0 tables (two tables, 20-bit slots) ---- */

/*
 * t0_table_lo[k+N] = (4096+t0[0][k]) << 20 | (4096+t0[1][k])
 * t0_table_hi[k+N] = (4096+t0[2][k]) << 20 | (4096+t0[3][k])
 */
void prepare_t0_table_ops512(uint64_t t0_table_lo[2*N], uint64_t t0_table_hi[2*N],
                              polyveck *t0);

void evaluate_ct0_ops512(polyveck *z, const poly *c,
                         const uint64_t t0_table_lo[2*N],
                         const uint64_t t0_table_hi[2*N]);

#endif /* OPSSIG512_MULT_H */
