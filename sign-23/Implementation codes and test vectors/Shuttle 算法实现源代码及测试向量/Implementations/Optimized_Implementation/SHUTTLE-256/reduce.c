#include "reduce.h"

#include <stdint.h>

#include "params.h"

/* The uint16 Montgomery core (montgomery_reduce16 / fqmul16 / addm16 /
 * subm16, R = 2^16) lives as static inline in reduce.h.  This file carries
 * the signed centered / mod-2q helpers used by the SCHEME domain
 * (poly_reduce/freeze, rounding/LiftToModTwoQ).  Bodies are
 * byte-for-byte the standard Barrett forms (verified bit-identical to a%q
 * / a%2q over the full int32 range); only Q / DQ are re-pointed at the
 * SHUTTLE per-set values. */

/* ============================================================
 * Constant-time Barrett reduction (division-free).
 *
 * The scheme-domain reductions run on SECRET data (NTT residues of s/y/z,
 * the commitment w).  We must NOT lower them to a hardware/variable-time
 * divide, and must not rely on the compiler strength-reducing `% const`.
 * So `a % d` is done explicitly: REC = floor(2^SH / d) (a COMPILE-TIME
 * const -- `const / const` is folded, no runtime divide), then floor(au/d)
 * = (au*REC) >> SH undershoots by at most 1 (au <= 2^31, au*REC < 2^64),
 * and one branchless correction is exact.  Bit-identical to `a % d`
 * (verified by t_reduce.c over a dense int32 sweep that hits every
 * multiple-of-d boundary up to 2^31).
 *
 * SH = 46 (a deviation from the more common SH=47).  SH=47 is the value
 * used in reference Dilithium-style code and is SOUND for a larger q, but
 * for SHUTTLE-128's q=15361 the
 * reciprocal floor(2^47/15361) ~ 2^33.1, so au*REC for au up to 2^31
 * reaches ~2^64.1 and OVERFLOWS uint64 (verified: reduce32==a%q then fails
 * for q=15361 only).  SH=46 is the largest shift that keeps au*REC < 2^64
 * for every q / 2q across ALL three sets while still leaving the
 * undershoot <= 1 (one masked correction); the worst case is at multiples
 * of d and is exact (t_reduce.c).
 * ============================================================ */
#define BARRETT_SH 46
#define BARRETT_Q (((uint64_t)1 << BARRETT_SH) / (uint64_t)Q)
#define BARRETT_2Q (((uint64_t)1 << BARRETT_SH) / (uint64_t)DQ)

/* |a| = au mod d in [0,d), au <= 2^31, d in {q, 2q}, REC = floor(2^SH /
 * d). */
static inline uint32_t barrett_mod_u(uint32_t au, uint32_t d, uint64_t REC)
{
    uint32_t qh = (uint32_t)(((uint64_t)au * REC) >> BARRETT_SH);
    uint32_t r = au - qh * d; /* in [0, 2d)            */
    uint32_t ge =
        (uint32_t)0 - (uint32_t)(r >= d); /* all-ones iff r >= d   */
    return r - (ge & d);                  /* in [0, d)             */
}

/*************************************************
 * Name:        reduce32
 *
 * Description: For finite field element a in [-2^31, 2^31), compute r
 *              congruent to a mod Q with r in (-Q, Q) (truncate toward
 *zero,
 *              == a % Q).  Division-free constant-time (Barrett above).
 *
 * Arguments:   - int32_t: finite field element a
 *
 * Returns r.
 **************************************************/
int32_t reduce32(int32_t a)
{
    uint32_t sa = (uint32_t)(a >> 31);     /* 0 or 0xFFFFFFFF       */
    uint32_t au = ((uint32_t)a ^ sa) - sa; /* |a|                   */
    uint32_t r = barrett_mod_u(au, (uint32_t)Q, BARRETT_Q);
    return (int32_t)((r ^ sa) - sa); /* reapply sign of a    */
}

/*************************************************
 * Name:        caddq
 *
 * Description: Add Q if input coefficient is negative.  Constant-time
 *              (sign-bit mask).
 *
 * Arguments:   - int32_t: finite field element a
 *
 * Returns r.
 **************************************************/
int32_t caddq(int32_t a)
{
    a += (a >> 31) & Q;
    return a;
}

/*************************************************
 * Name:        freeze
 *
 * Description: For finite field element a, compute standard representative
 *              r = a mod^+ Q in [0, Q).
 *
 * Arguments:   - int32_t: finite field element a
 *
 * Returns r.
 **************************************************/
int32_t freeze(int32_t a)
{
    a = reduce32(a);
    a = caddq(a);
    return a;
}

/*************************************************
 * Name:        caddq2
 *
 * Description: Conditional add 2q.  If input is negative, returns a + 2q;
 *              otherwise returns a.  Constant-time (sign-bit mask).
 *
 * Arguments:   - int32_t: input
 *
 * Returns a if a >= 0, else a + 2q.
 **************************************************/
int32_t caddq2(int32_t a)
{
    a += (a >> 31) & DQ;
    return a;
}

/*************************************************
 * Name:        reduce_mod_2q
 *
 * Description: Reduce arbitrary int32_t to canonical residue in [0, 2q).
 *Uses Barrett reduction on |a|, restores the sign, then positive-corrects.
 *No runtime division / modulus is emitted.
 *
 * Arguments:   - int32_t: input
 *
 * Returns r in [0, 2q) with r congruent to a mod 2q.
 **************************************************/
int32_t reduce_mod_2q(int32_t a)
{
    uint32_t sa = (uint32_t)(a >> 31);
    uint32_t au = ((uint32_t)a ^ sa) - sa; /* |a|                  */
    uint32_t ru = barrett_mod_u(au, (uint32_t)DQ, BARRETT_2Q);
    int32_t r = (int32_t)((ru ^ sa) - sa); /* a % 2q (truncate)   */
    r += (r >> 31) & DQ; /* make non-negative -> [0,2q) */
    return r;
}
