#ifndef OPSSIG128_MULT_H
#define OPSSIG128_MULT_H

#include <stdint.h>
#include "params.h"
#include "poly.h"
#include "polyvec.h"

/*
 * Small-polynomial multiplication for OPSsig-128.
 *
 * Ported from the Dilithium2 PSPM / PSPM-TEE implementations:
 *   ACSAC 2022: "Parallel Small Polynomial Multiplication for Dilithium"
 *   TDSC 2025:  "Optimized Vectorization Implementation of CRYSTALS-Dilithium"
 *
 * Parameter differences from Dilithium2 (N=256, K=L=4, ETA=2, TAU=39):
 *   N=512  K=L=3  ETA=2  TAU=39  D=13  (same ETA/TAU/D, larger N, smaller K/L)
 *
 * --- s1/s2 packing (8-bit slots, 3 polys per uint32_t) ---
 *   Shift: ETA=2 → stored value = ETA + s[j][k] ∈ [0, 2*ETA=4]
 *   Slot width: 8 bits (one byte per polynomial index j)
 *   Slots per word: L=K=3  (3 bytes used out of 4 in a uint32_t)
 *   Max accumulation per slot: TAU * 2*ETA = 39*4 = 156 < 256 = 2^8  ✓
 *   MASKS_S = 0x040404  (complement constant: 2*ETA per byte, 3 bytes)
 *
 * --- t0 packing (19-bit slots, 3 polys per uint64_t) ---
 *   Shift: 2^(D-1)=4096 → stored value = 4096 + t0[j][k] ∈ [0, 8192]
 *   Slot width: 19 bits
 *   Slots per word: K=3  (3 × 19 = 57 bits fit in uint64_t)
 *   Max accumulation per slot: TAU * 8192 = 39*8192 = 319488 < 2^19=524288  ✓
 *   MASKT_T0 = 2^51 | 2^32 | 2^13  (8192 = 2*4096 per slot at positions 0, 19, 38)
 *   T0_UNPACK_SUB = TAU * 4096 = 159744  (subtract to recover signed result)
 *
 * --- Negacyclic sign handling ---
 *   The j-i+N index into the 2*N table automatically picks:
 *     j >= i  →  index ∈ [N, 2N-1]: positive table (no wrap, +coeff)
 *     j <  i  →  index ∈ [1, N-1]:  complement table (wrap, effectively −coeff)
 *   This mirrors the negacyclic ring Z[x]/(x^N+1) sign flip without branches.
 */

/* 8-bit packing constants for s1/s2 (L=K=3 slots per uint32_t word) */
#define MASKS_S   0x040404U

/* 19-bit packing constants for t0 (K=3 slots per uint64_t word)      */
#define MASKT_T0  ((1ULL<<51)|(1ULL<<32)|(1ULL<<13))
#define T0_UNPACK_SUB  (TAU * 4096)   /* = 39 * 4096 = 159744 */

/* --- Table preparation (call once per sign invocation, before rej loop) --- */

/* Build s1 packing table.
 *   s1_table[k+N] = packed positive:  byte j = ETA + s1[j][k]  (j=0..L-1)
 *   s1_table[k]   = complement:       MASKS_S - s1_table[k+N]           */
void prepare_s1_table_ops128(uint32_t s1_table[2*N], polyvecl *s1);

/* Build s2 packing table (same layout, K=3 polys). */
void prepare_s2_table_ops128(uint32_t s2_table[2*N], polyveck *s2);

/* Build t0 packing table.
 *   t0_table[k+N] = packed positive (19-bit slots, j=0..K-1):
 *     slot 0 (bits  0-18) = t0[K-1][k]+4096, ..., slot K-1 (bits 38-56) = t0[0][k]+4096
 *   t0_table[k]   = complement: MASKT_T0 - t0_table[k+N]                */
void prepare_t0_table_ops128(uint64_t t0_table[2*N], polyveck *t0);

/* --- Evaluation (call every rej iteration after poly_challenge) --- */

/* Compute cs1 and cs2 simultaneously via smallpoly accumulation.
 * Simultaneously recovers z = y + cs1 and w0prime = w0 - cs2.
 * Performs per-coefficient early rejection checks during recovery.
 *
 *   A = GAMMA1 - BETA  (rejection bound for z)
 *   B = GAMMA2 - BETA  (rejection bound for w0prime)
 *
 * Returns 1 (reject) immediately if any coefficient violates a bound,
 * 0 if all coefficients pass (z and w0prime are fully written in that case).
 */
int evaluate_cs1_cs2_early_check_ops128(
        polyvecl *z,       polyveck *w0prime,
        const poly *c,
        const uint32_t s1_table[2*N],
        const uint32_t s2_table[2*N],
        polyvecl *y,       polyveck *w0,
        int32_t A,         int32_t B);

/* Compute ct0 via smallpoly accumulation and write to z.
 * Does NOT perform the norm check; caller must call polyveck_chknorm after. */
void evaluate_ct0_ops128(polyveck *z, const poly *c,
                         const uint64_t t0_table[2*N]);

#endif /* OPSSIG128_MULT_H */
