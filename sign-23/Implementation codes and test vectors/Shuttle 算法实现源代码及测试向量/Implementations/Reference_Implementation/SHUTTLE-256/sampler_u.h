/*
 * sampler_u.h -- SHUTTLE SamplerU: draw U ~ Uniform(0,1] as an
 * exponent+mantissa pair and return ell ~= log2(U).  Pure C99,
 * integer-only, constant-time; shared VERBATIM across all three parameter
 * sets (SHUTTLE-128/256/512) -- no per-set table, no per-set #if.
 *
 * ===================== WHAT SamplerU COMPUTES (alg:sampler-u)
 * =============
 *
 * SamplerU writes a uniform U in (0,1] as  U = 2^-a * b, where
 *   - the integer EXPONENT  a in {1..kappa_a+1} = {1..81}  is one plus the
 *     count of leading zeros of an 80-bit MSB-first random string (a
 *     geometric variable: Pr[a=j] = 2^-j; the all-zero string collapses to
 *     a = kappa_a+1 = 81, the tail U < 2^-80), and
 *   - the MANTISSA  b = 1 + m/2^kappa_b in [1,2)  with m the top
 *     kappa_b = 57 MSB-first bits of an 8-byte squeeze.
 * Then  log2(U) = log2(b) - a, where log2(b) = ApproxLog's Q62 frac and
 * the integer term -a is EXACT in base two (this is the whole
 * point of the base-2 ApproxLog -- no stored log2 constant, no log2
 * error).
 *
 * ============ MSB-FIRST BIT EXTRACTION (the lone exception)
 * ===========
 *
 * Everywhere ELSE in SHUTTLE bytes/bits are little-endian / LSB-first
 * (PolyToBytes, BytesToInteger).  SamplerU is the SINGLE normative
 * exception: BytesToBits(B, len) reads bytes in squeeze order and, WITHIN
 * each byte, most-significant bit FIRST.  The
 * two conventions coexist; both are normative.  Concretely:
 *
 *   Exponent block: squeeze 10 bytes rho_a; rho_a[0]'s MSB is R_0.
 *     a = CLZ(R) + 1 = 1 + (#leading zero bits scanning rho_a[0] MSB-first
 *     through rho_a[9] LSB).  All-zero R -> CLZ = 80 -> a = 81.
 *     => clz80_msb_first big-endian-loads the 10 bytes into an 80-bit
 * field (uint64 of bytes 0..7 in the high limb + uint16 of bytes 8..9) and
 *        counts leading zeros branchlessly; all-zero returns 80.
 *
 *   Mantissa block: squeeze 8 bytes rho_b; rho_b[0]'s MSB is M_0.
 *     m = sum_{i=0..56} M_i 2^{56-i}  (MSB-first 57-bit integer).  This is
 *     EXACTLY the big-endian 64-bit load of rho_b[0..7] right-shifted by
 * 7: m = be64(rho_b) >> 7              (top 57 bits kept, low 7 discarded)
 *     where be64 = __builtin_bswap64 of a memcpy'd uint64 (no aliasing
 * UB).
 *
 * Each SamplerU call consumes EXACTLY 10 + 8 = 18 bytes from ctx, in that
 * fixed order; the Python reference must mirror `be64 >> 7` and the
 * MSB-first CLZ bit-for-bit or the IRS KAT diverges.
 *
 * ================ RETURN FORM: (a, frac_q62), NOT a folded int64
 * ==========
 *
 * The natural single-value return  log2(U) = frac_q62 - (a << 62)
 * OVERFLOWS int64: a reaches 81, and 81 * 2^62 ~ 2^68.3 > 2^63.  So
 * sampler_u returns the pair (a, frac_q62) UNFOLDED; the
 * caller (irs.c R) forms the natural-log test variable  u = (2 r^2 ln2) *
 * log2(U)  in
 * __int128, multiplying the exponent and fraction contributions SEPARATELY
 * at the pinned fixed-point scale (see irs.c R2LN2_QF / R_transition).
 * This avoids the overflow and keeps the whole path integer-only.
 *
 *   frac_q62 = shuttle_log2_frac_q62(j, x_q64) in [0, 2^62)  -- log2(b),
 * Q62 a        in {1..81}                                       --
 * exponent
 *   => log2(U) = frac_q62 / 2^62 - a    (computed by the caller, never
 * here)
 *
 * The ApproxLog caller split (g=2, kappa_b=57): from the MSB-first
 * mantissa integer m,
 *   j     = floor(m / 2^(kappa_b-g)) = m >> 55          (top g=2 bits)
 *   x_q64 = (m mod 2^55) << 9                            (Q64 reduced arg)
 *
 * ============================ CONSTANT-TIME
 * ===============================
 *
 * clz80_msb_first and mantissa57_msb_first are branchless; the ApproxLog
 * kernel does a full-table eqmask scan.  rho_a is fresh XOF output
 * off a secret seed, so its CLZ leaks nothing about the long-term secret;
 * we keep it branchless anyway for audit uniformity (constant-time).  NO
 * division, NO float.
 */
#ifndef SHUTTLE_SAMPLER_U_H
#define SHUTTLE_SAMPLER_U_H

#include <stdint.h>

#include "params.h"
#include "symmetric.h" /* xof_ctx, xof256_squeeze (the 0x09 IRS stream) */

/* SamplerU result: the unfolded (exponent, log2-fraction) pair (see header
 * "RETURN FORM").  log2(U) = frac_q62/2^62 - a; the caller forms u in
 * __int128 to avoid the int64 overflow of the folded value. */
typedef struct {
    int64_t
        frac_q62; /* log2(b) in Q62, in [0, 2^62)         (ApproxLog)  */
    uint32_t a;   /* exponent in {1..KAPPA_A+1} = {1..81} (CLZ+1)      */
} sampler_u_res;

/* SamplerU DECODE (no XOF): map one 18-byte block (10 exponent bytes rho_a
 * + 8 mantissa bytes rho_b) to ell = (frac_q62 = log2(b), a) via the
 * MSB-first CLZ80 / mantissa57 extraction and the ApproxLog kernel.
 * Touches no ctx, so the byte schedule is owned entirely by the caller's
 * bulk squeeze. Used by the bulk-buffer IRS path (reject_sample, the
 * single-context bulk squeeze): one TAU*18 squeeze, then this decode per
 * ascending-j transition. */
sampler_u_res sampler_u_decode(const uint8_t rho_a[10],
                               const uint8_t rho_b[8]);

/* SamplerU: squeeze 10 exponent bytes + 8 mantissa bytes from ctx (the IRS
 * 0x09||seed_y stream), extract (a, m) MSB-first, and return
 * (frac_q62 = log2(b), a).  Consumes EXACTLY 18 bytes; updates ctx.  Thin
 * wrapper: squeeze the 18 bytes then sampler_u_decode().  (The bulk IRS
 * path bypasses this and decodes slices of a single TAU*18 squeeze.) */
sampler_u_res sampler_u(xof_ctx *ctx);

/* Batch-of-2: draw TWO independent U's, squeezing the 2*(10+8)=36 bytes in
 * the fixed order  rho_a0(10), rho_b0(8), rho_a1(10), rho_b1(8)  -- the
 * exact order of two sequential sampler_u() calls -- and evaluate both
 * ApproxLog fractions with the x2 batch (shuttle_log2_frac_q62_x2).
 * BIT-IDENTICAL to two sequential sampler_u() calls (verified in
 * test_irs).  Used by the IRS ascending-j loop when two transitions are
 * pipelined. */
void sampler_u_x2(xof_ctx *ctx, sampler_u_res out[2]);

#endif /* SHUTTLE_SAMPLER_U_H */
