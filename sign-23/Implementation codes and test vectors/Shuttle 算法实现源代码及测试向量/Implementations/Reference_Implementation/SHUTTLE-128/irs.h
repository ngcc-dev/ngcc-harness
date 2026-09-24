/*
 * irs.h -- SHUTTLE Iterative Rejection Sampler: RejectSample / R.
 *
 * The rejection-FREE inner masking transition of SHUTTLE signing.  Pure
 * C99, integer-only, constant-time.  Shared across all three parameter
 * sets (the only per-set inputs are N=IRS_N, B_bdry=IRS_BDRY, tau=TAU and
 * the vector length KVEC from params.h; the R-test constant R2LN2_QF is
 * identical for all sets).
 *
 * ============================ WHAT IT COMPUTES
 * ===========================
 *
 * RejectSample(ctx, z, y, c, sk_tilde) (Algorithm alg:RejectSample) maps
 * the masking sample y to the response
 *
 *     z = y + sk_tilde . c'        (Description.tex:1380)
 *
 * where c' is the SIGNED challenge: c has only {0,1} entries (SampleC
 * output), and the per-position sign of each applied shift is decided by
 * the transition R, NOT carried in c.  For every index j with c_j = 1, in
 * STRICT ASCENDING order j = 0..n-1, R applies one transition that
 * moves z to z +- v with v = sk_tilde . X^j (a negacyclic rotation).  R
 * NEVER aborts: it always emits one of the two neighbours, so RejectSample
 * runs EXACTLY tau transitions (rejection-free).
 *
 * The transition test (Algorithm alg:Ryv) draws U ~ Uniform(0,1] via
 * SamplerU, forms the natural-log test variable u = 2 r^2 ln U from the
 * base-2 ell ~= log2(U) by a single multiply by 2 r^2 ln2, and
 * checks u against the closed-form squared-norm differences
 *
 *     ||y||^2 - ||y + m v||^2 = -2 m t - m^2 V,   t = <y,v>,  V = ||v||^2,
 *
 * over the N=29 truncated terms m=0..28, covered by ceil(N/2)=15 boundary
 * pairs:  -2(2i+1)t - (2i+1)^2 V  <  u  <=  -4 i t - 4 i^2 V   sets flag=1
 * (flag starts at -1).  V = <sk_tilde,sk_tilde> is computed ONCE and
 * reused for every shift, since ||sk_tilde . X^j||^2 = ||sk_tilde||^2 (X^j
 * is an isometry of R = Z[X]/(X^n+1)).  Before the test, v is
 * sign-normalized so t = <y,v> > 0.
 *
 * ============= THE IRS CONTEXT IS A FRESH 0x09||seed_y STREAM
 * =========
 *
 * Sign opens a FRESH xof_ctx, absorbs DS_IRS(0x09) || seed_y, and
 * threads it through reject_sample -> R_transition -> sampler_u.  This is
 * the SAME seed_y that SampleY (tag 0x08) used, separated ONLY by the
 * domain tag. Do NOT continue SampleY's context, and do NOT use the
 * 16-lane SampleY ctx structure -- IRS is a SINGLE context, no lane index.
 * Each SamplerU draw consumes a FIXED 18 bytes (10 exponent + 8 mantissa);
 * with tau transitions and no rejection, the IRS stream consumption is
 * EXACTLY 18*tau bytes per signing attempt (deterministic -- this is what
 * makes the byte accounting trivial vs the wide-Gaussian sampler).
 *
 * ===================== THE u FIXED-POINT FORM
 * ================
 *
 * ell ~= log2(U) is carried UNFOLDED as (a, frac_q62) by SamplerU
 * (frac_q62 is log2(b) in Q62; a in {1..81}; log2(U) = frac/2^62 - a).  R
 * forms u at the pinned scale Q44 (R2LN2_QF = round(2 r^2 ln2 * 2^44), a
 * uint64_t) in
 * __int128:
 *     u_frac = round(R2LN2_QF * frac_q62 / 2^62)   [Q44]   (full Q62 frac)
 *     u_a    = a * R2LN2_QF                          [Q44]
 *     u      = u_frac - u_a                           [Q44]  ( = 2 r^2 ln
 * U ) and compares against the boundary integers B = -2 m t - m^2 V (exact
 * int64) promoted to (int128)B << 44.  The half-open test (lo<<44) < u <=
 * (hi<<44) is EXACT in __int128.  The folded int64 log2(U) = frac -
 * (a<<62) is NOT used: a up to 81 makes 81*2^62 ~ 2^68.3 overflow int64.
 * The scale 44, the rounding rule, and the constant are PINNED by
 * tools/gen_irs_consts.py (the @@AUTOGEN:irs_consts@@ region of irs.c) and
 * are KAT-critical.
 *
 * ====================== ISOCHRONY / LEAKAGE
 * ==================
 *
 * Isochrony of IRS (Description.tex:1385-1393, 2138-2166; Security.tex
 * M_6). Unlike Fiat-Shamir-with-aborts, R is ABORT-FREE: it always outputs
 * one of y +- v, so RejectSample runs for a FIXED number of transitions
 * equal to the (public) challenge weight tau, independent of the secret
 * key or masking sample.  There is NO secret-dependent loop count, so the
 * IRS itself contributes NO timing channel; its wall-clock time is a
 * deterministic function of (tau, n), both public.  Specifically: (a) R is
 * abort-free  => exactly tau R-transitions per RejectSample; (b) the only
 * signing-level variability is the OUTER Sign loop count, gated by the
 * PUBLIC norm test ||(z_1,z_2')||_2 <= B_v; that count leaks only
 * the public emitted-distribution acceptance rate beta, never the secret
 * -- because the emitted response z = y + StretchS(sk) c' is EXACTLY
 * centered Gaussian D_{R,r} independent of sk (M_6 = 1 exact step,
 * Security.tex:701); (c) the per-transition arithmetic (SamplerU, the
 * inner products, the sign-normalize, the 15-pair interval loop, z +=
 * flag*v) is fully branchless / constant-time (no data-dependent jump,
 * index, shift, or divide); the loop bound is a fixed 15 and a fixed n;
 *   (d) the SamplerU byte consumption is fixed at 18*tau bytes (no
 * variable squeeze) -- the byte cursor advance is data-independent.
 * Therefore the entire IRS path is isochronous and the ONLY observable is
 * the emitted distribution, which is secret-independent by construction.
 *
 * SECRET vs PUBLIC classification (for the secret/public audit):
 *   SECRET : seed_y (and the whole IRS ctx), sk_tilde, t=<y,v>, V,
 * ell/frac/a, flag, the input y and output z coefficients. PUBLIC : c
 * (recomputed by the verifier from seedC), tau, n, the Sign outer-loop
 * iteration count, beta. The ascending-j  if (c[j])  branch in
 * reject_sample is therefore a WHITELISTED public-data branch (c is
 * public): gating on c[j] reveals only the public challenge support, not
 * the secret.  Every other branch on the IRS path is forbidden; the
 * SamplerU/R arithmetic on secret data is branchless.  The __int128
 * u-vs-boundary comparison uses GNU two's-complement arithmetic shift
 * (same portability caveat as ApproxExp's high64_s128; re-audit if porting
 * off GCC/Clang).
 */
#ifndef SHUTTLE_IRS_H
#define SHUTTLE_IRS_H

#include <stdint.h>

#include "params.h"
#include "poly.h"
#include "symmetric.h"

/*
 * RejectSample (Algorithm alg:RejectSample).  Apply tau transitions to y,
 * one per nonzero challenge coefficient, in strict ascending index order
 * j = 0..n-1.
 *
 *   ctx       IRS XOF context, ALREADY initialized with
 * DS_IRS(0x09)||seed_y by the caller; mutated by every SamplerU
 * squeeze. z         output response, KVEC polys (z = y + sk_tilde . c').
 *   y         masking sample, KVEC polys (may alias z -> handled: z is
 * filled from y first). c         single binary challenge poly with
 * exactly tau coeffs == 1. sk_tilde  StretchS(sk), KVEC polys (the
 * stretched secret).
 *
 * Rejection-FREE: never aborts; runs exactly tau R-transitions.
 */
void reject_sample(xof_ctx *ctx, poly z[KVEC], const poly y[KVEC],
                   const poly *c, const poly sk_tilde[KVEC]);

#endif /* SHUTTLE_IRS_H */
