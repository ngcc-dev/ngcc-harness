/*
 * reduce.h -- modular reduction substrate for SHUTTLE.
 *
 * Two distinct arithmetic domains live here:
 *
 *  (1) The uint16 Montgomery NTT CORE (R = 2^16, canonical [0,q)): the
 *      static-inline kernels montgomery_reduce16 / fqmul16 / addm16 /
 * subm16 used by the scalar NTT engine (ntt_ref.c) and by any C-side
 * per-coeff ring arithmetic that must stay byte-exact with the asm
 * backends.  Every add/sub is reduced because for the unsigned valley sets
 * (q=61441 / 59393) 2q exceeds 2^16, so a bare uint16 add would wrap.
 *
 *  (2) The int32 SIGNED scheme-domain Barrett helpers (reduce32 / caddq /
 *      freeze / caddq2 / reduce_mod_2q): these run on the raw scheme
 *      coefficients (poly.coeffs is int32) and on SECRET NTT residues, so
 * they are division-free and constant-time (a compile-time Barrett
 * reciprocal, no runtime `/` or `%`; see reduce.c).  reduce_mod_2q is the
 * general mod-2q canonicalizer consumed by LiftToModTwoQ.
 *
 * Per-set Q / DQ come from params.h, selected by SHUTTLE_MODE.  The
 * NTT-domain Montgomery constants (QINV = q^-1 mod 2^16, MONT = 2^16 mod
 * q, NINV_TOMONT = n^-1*R^2 mod q) are NOT in params.h -- they are an
 * internal detail of the NTT layer -- so reduce.h derives them here
 * per-set (all are reproducible closed forms of (n,q), mirroring
 * tools/ntt/<qset>/gen_ntt.py; a tiny mirror, never a magic number).
 */
#ifndef SHUTTLE_REDUCE_H
#define SHUTTLE_REDUCE_H

#include <stdint.h>

#include "params.h" /* Q, DQ = 2*Q (per-set) */
/* Disable with -DDISABLE_NAMESPACE=1. */
#include "namespace.h"

/* ============================================================ *
 *  NTT-domain Montgomery constants (per-set, derived from n,q) *
 *    MONT         = 2^16 mod q                                 *
 *    QINV         = -q^-1 mod 2^16                             *
 *    NINV_TOMONT  = n^-1 * (2^16 mod q)^2 mod q                *
 *                                                              *
 *  NOTE on QINV sign: this C core uses the Dilithium-style     *
 *  "+m*q" Montgomery form (m = a*QINV; t = (a + m*q) >> 16),   *
 *  which requires QINV = -q^-1 mod 2^16 = 2^16 - (q^-1 mod     *
 *  2^16) = 15359 / 61439 / 59391.  This is DISTINCT from the   *
 *  positive q^-1 (50177 / 4097 / 6145) that the AVX asm bakes  *
 *  into its consts: the asm uses the Kyber "hi - mq" vpmulhw   *
 *  form, a different (but equivalent mod q) Montgomery layout. *
 *  Both are reproducible from q; verified bit-exact by         *
 *  t_reduce.c over the [0,q) product grid.                     *
 * ============================================================ */
#if SHUTTLE_MODE == 128
#    define QINV 15359       /* -(15361^-1) mod 2^16 = 2^16-50177 */
#    define MONT 4092        /* 2^16 mod 15361                    */
#    define NINV_TOMONT 3004 /* 256^-1 * R^2 mod q                */
#elif SHUTTLE_MODE == 256
#    define QINV 61439        /* -(61441^-1) mod 2^16 = 2^16-4097  */
#    define MONT 4095         /* 2^16 mod 61441                    */
#    define NINV_TOMONT 32632 /* 512^-1 * R^2 mod q                */
#elif SHUTTLE_MODE == 512
#    define QINV 59391        /* -(59393^-1) mod 2^16 = 2^16-6145  */
#    define MONT 6143         /* 2^16 mod 59393                    */
#    define NINV_TOMONT 36794 /* 1024^-1 * R^2 mod q               */
#else
#    error "Unsupported SHUTTLE_MODE (expected 128, 256, or 512)"
#endif

/* ============================================================
 * uint16 Montgomery core, R = 2^16  (NTT engine; AVX 16-bit lanes)
 * ============================================================
 * Bookkeeping (Kyber-style): the NTT twiddles are stored in the Montgomery
 * domain (zeta*R mod q), so fqmul16(x, zeta*R) = x*zeta mod q yields a
 * normal product -> the forward NTT is normal-in / normal-out.  Plain
 * products (pointwise) come out owing one R (value = a*b*R^-1);
 * invntt_tomont's R^2 scale (NINV_TOMONT) pays that R back so a*b lands in
 * the normal domain.
 *
 * Representation is canonical [0,q): EVERY add/sub is reduced
 * (addm16/subm16) because 2q can exceed 2^16 (unsigned valley sets) and a
 * bare uint16 addition would wrap.  These run on (possibly secret) NTT
 * residues, so the ?: ternaries below MUST lower to a branchless
 * masked-subtract on -O2 -- verified by the t_reduce.c objdump
 * no-branch/no-idiv gate.
 */

/* a*2^-16 mod q in [0,q), valid for 0 <= a < q*2^16 (e.g. a = product of
 * two coefficients in [0,q): since q < 2^16, (q-1)^2 < q*2^16).  The a +
 * m*q sum reaches ~2qR < 2^33, so it is accumulated in uint64. */
static inline uint16_t montgomery_reduce16(uint32_t a)
{
    uint16_t m = (uint16_t)(a * (uint32_t)QINV); /* low 16 of a*(-q^-1) */
    uint32_t t =
        (uint32_t)((a + (uint64_t)m * Q) >> 16); /* exact; 0 <= t < 2q */
    return (uint16_t)(t >= (uint32_t)Q ? t - Q : t);
}

static inline uint16_t fqmul16(uint16_t a, uint16_t b)
{
    return montgomery_reduce16((uint32_t)a * b);
}

/* modular add/sub keeping the canonical [0,q) representation. */
static inline uint16_t addm16(uint16_t a, uint16_t b)
{
    uint32_t s =
        (uint32_t)a + b; /* up to 2q-2; widened to dodge uint16 wrap */
    return (uint16_t)(s >= (uint32_t)Q ? s - Q : s);
}
static inline uint16_t subm16(uint16_t a, uint16_t b)
{
    return (uint16_t)(a >= b
                          ? a - b
                          : a + Q - b); /* subm16(0,z) = q-z = -z mod q */
}

/* ============================================================
 * signed centered / mod-2q helpers (scheme domain; int32; in reduce.c)
 * ============================================================ */

int32_t reduce32(int32_t a); /* a mod q in (-q, q), == a % q (truncate) */
int32_t caddq(int32_t a);    /* conditional + q  : a >= 0 ? a : a + q   */
int32_t freeze(int32_t a);   /* standard representative in [0, q)       */
int32_t caddq2(int32_t a);   /* conditional + 2q : a >= 0 ? a : a + 2q  */
int32_t reduce_mod_2q(int32_t a); /* canonical residue in [0, 2q) */

#endif /* SHUTTLE_REDUCE_H */
