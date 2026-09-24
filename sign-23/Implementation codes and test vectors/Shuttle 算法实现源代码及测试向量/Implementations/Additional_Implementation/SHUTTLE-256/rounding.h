/*
 * rounding.h -- CompressY / StretchS / RoundB / mod-2q lift / hint / norm
 *               gates for SHUTTLE.
 *
 * This is the compression and commitment substrate that sits between the
 * samplers and the top-level KeyGen/Sign/Verify.  It owns:
 *
 *   - LSB / LiftToModTwoQ        (alg:LSB, alg:lift22Q)
 *   - CompressY / StretchS       (alg:compress-y, alg:stretch-s)
 *   - RoundB fused with e'       (alg:roundb + KeyGen 8-9):
 * roundB_update_s2
 *   - the mod-2q commitment lift (mat_mul_2q signer / mat_mul_z1_2q
 * verifier)
 *   - MakeHint / UseHint         (LIVE mod-q UseHint; alg:makehint,
 *                                 alg:usehint 1326-1344)
 *   - the two-norm gates         (compare-of-squares, no sqrt, no float)
 *
 * CONSTANT-TIME / no-float / no-div discipline:
 *   - All round-to-nearest divides on SECRET-derived data use a magic
 *     reciprocal (round_div_taway) or a power-of-two shift -- NEVER a
 *     hardware idiv / `% const`.
 *   - The mod-2q reductions use ONE masked conditional add/subtract (not
 * the general reduce_mod_2q Barrett), matching the AVX2/AVX512 lift so the
 *     SIMD forks are byte-exact.
 *   - The non-power-of-2 `mod H_h` in MakeHint/UseHint is a FOLD of an
 *     in-range difference via branchless masked add/subtract, NOT a wrap
 *     (range-CHECK-then-reject lives on the DECODE side).
 *   - The norm gates compare INTEGER squared L2 against an integer-square
 *     bound, accumulated in int64 (no sqrt, no float, no data-dependent
 *     branch; the caller branches on the PUBLIC accept/reject outcome).
 *
 * Three subtle correctness risks live here:
 *   (1) the mod-2q parity correction uses the RAW coefficient
 *                parity `x0 & 1`, NOT `freeze(x0) & 1` (freeze adds odd q
 * to negatives, flipping their parity).  This is THE gotcha that makes the
 * SIMD lift byte-exact to scalar. (2) the hint range [0,H_h) is
 * non-power-of-2 (30/120/58); the MakeHint/UseHint `mod H_h` folds an
 * in-range difference and `highbits_reduced` never over-reduces -- it
 * produces only canonical [0,H_h) hints. (3) implement the LIVE
 * mod-q UseHint (Description.tex 1326-1344) ONLY; the commented-out mod-2q
 * block (1346-1363) is dead and gives wrong results.
 */
#ifndef SHUTTLE_ROUNDING_H
#define SHUTTLE_ROUNDING_H

#include <stdint.h>

#include "params.h"
#include "poly.h" /* poly = int32 coeffs[N]; poly16 = uint16 coeffs[N] */

/* ---------------------------------------------------------------------- *
 *  LSB / lift (alg:LSB, alg:lift22Q) *
 * ----------------------------------------------------------------------
 */

/* LSB(x) = (x mod 2q) mod 2 in {0,1} ; coefficient-wise.  x may be any
 * int32 (the mod-2q is via reduce_mod_2q, then the low bit). */
int32_t lsb_coeff(int32_t x);

/* LiftToModTwoQ (per coeff): xbar in [0,q) (NON-negative rep), parity bit
 * b in {0,1}.  Returns x in [0,2q): xbar if (xbar&1)==b else xbar+q.
 * Branchless.  q is odd for all sets, so x |-> (x mod q, x mod 2) is a
 * bijection Z_{2q} -> Z_q x Z_2 and the lift is exact. */
int32_t lift_to_mod2q_coeff(int32_t xbar, int32_t b);

/* Vector lift: x[k] = LiftToModTwoQ(xbar[k], bpar[k]).  bpar is LSB(.)*j,
 * so only the first poly slot of a commitment vector carries a nonzero
 * parity bit (j = [1,0,...,0]); the caller passes the per-coeff parity
 * poly. */
void poly_lift_to_mod2q(poly *x, const poly *xbar, const poly *bpar);

/* ---------------------------------------------------------------------- *
 *  CompressY / StretchS (alg:compress-y, alg:stretch-s) *
 * ----------------------------------------------------------------------
 */

/* Per-block divisors over a KVEC-long poly array:
 *   block 0  (1 poly,  index 0)            divided/multiplied by ALPHA_1
 *   block s  (ELL polys, index 1..ELL)     by ALPHA_S
 *   block e  (EM  polys, index 1+ELL..)    by ALPHA_E
 * The block boundaries (1 / 1+ELL / 1+ELL+EM) are a COMPILE-TIME partition
 * of KVEC, NOT secret data -- compress_y/stretch_s branch on the block
 * index only, never on a coefficient value.
 *
 * CompressY = round-to-nearest, TIES UP toward +inf (shift-invariant; see the
 * translation-identity note below).  StretchS = exact integer multiply, NO
 * modular reduction. */
void compress_y(poly out[KVEC], const poly in[KVEC]);
void stretch_s(poly out[KVEC], const poly in[KVEC]);

/* ====================================================================== *
 *  StretchS no-mod range analysis (part a) -- WHY the multiply never *
 *  overflows int32 / never needs a reduction. *
 * ====================================================================== *
 * StretchS(x) = (alpha_1*x_0, alpha_s*x_s, alpha_e*x_e), coefficient-wise
 * exact integer multiply.  Its only callers feed bounded small integers:
 *
 *   (1) KeyGen StretchS(1, s, e'): the constant block is the scalar 1
 *       (constant poly value 1), |s| <= B_{s,enc} <= 10, |e'| <= B_{e,enc}
 *       <= 12.  Stretched coeff magnitudes:
 *         block 0 : alpha_1 * 1               = 90 / 135 / 144
 *         block s : alpha_s * B_{s,enc} <= 10*9 / 5*10 / 3*10  = 90 / 50 /
 * 30 block e : alpha_e * B_{e,enc} <= 5*10 / 5*12 / 3*12  = 50 / 60 / 36
 *       So every stretched coeff has magnitude <= 144 -- trivially within
 *       int32, no wrap.  The resulting norm^2 ~ B_k^2 ~ 85k-88k (int64).
 *
 *   (2) Sign sk_tilde = StretchS(sk_full): the SAME secret, so the SAME
 *       per-coeff bound <= 144.  sk_tilde feeds IRS with V =
 *       ||sk_tilde||^2 <= B_k^2 ~ 88000.
 *
 * The translation identities
 *   CompressY(StretchS(x))      = x
 *   CompressY(y + StretchS(x))  = CompressY(y) + x
 * hold for ALL divisors PRECISELY BECAUSE CompressY rounds half-UP (toward
 * +inf), which is shift-invariant: round((v + alpha*x)/alpha) = round(v/alpha)
 * + x for every integer x.  (Round-half-AWAY-from-zero is NOT shift-invariant
 * -- at a tie the round direction depends on the sign -- and breaks the
 * identity whenever a divisor alpha is even, e.g. alpha_1=90/alpha_s=10 for
 * SHUTTLE-128, which silently made Sign reject ~81% of attempts at the B_v
 * gate.  See gen_rounding.py.)  This identity is what makes the verifier's
 * commitment reconstruction from z = CompressY(y) + sk*c' consistent in Sign.
 * ======================================================================
 */

/* ---------------------------------------------------------------------- *
 *  RoundB fused with e' update (alg:roundb + KeyGen 8-9) *
 * ----------------------------------------------------------------------
 */

/* ONE pass over each (b0, e) coeff pair, per pk poly i in [0,EM):
 *   vp     = b0 mod q              (non-neg rep, [0,q))
 *   v0     = vp bmodpm alpha_b     ([-alpha_b/2, alpha_b/2))
 *   b      = vp - v0               (multiple of alpha_b, in [0,q))
 *   delta  = (b - b0) bmodpm q     (centered rounding error,
 * |delta|<=alpha_b/2) e'     = e + delta             (absorb) Writes b
 * into pkb[i], e' into ep[i].  alpha_b is a power of two (2 or 4), so the
 * bmodpm-alpha_b centering is a masked subtract on the low log2(ab) bits.
 * This fuses RoundB and the rounding-error absorption in one pass,
 * eliminating the separate delta read/write. */
void roundB_update_s2(poly pkb[EM], poly ep[EM], const poly pkb0[EM],
                      const poly e[EM]);

/* ---------------------------------------------------------------------- *
 *  mod-2q commitment products (the lift)                                 *
 * ----------------------------------------------------------------------
 */

/* Signer commitment.  Computes comY in [0,2q) directly from the COMPRESSED
 * mask y' (poly array of length 1+ELL+EM = KVEC) and the cached NTT-domain
 * public matrix:
 *   bhat[EM]      : NTT(pkb) per pk poly i (NTT-domain, [0,q)).
 *   Ahat[EM*ELL]  : the hAgen block (NTT-domain, [0,q)), indexed
 * [i*ELL+j].
 *
 * Pipeline:
 *   1. forward-NTT the fresh compressed mask y' (poly_ntt over freeze'd
 *      poly16 copies); the public bhat/Ahat are already NTT-domain.
 *   2. compute_t: t_i = -bhat_i . y'_0 + sum_j Ahat[i][j] . y'_{1+j}  (mod
 * q), via poly_pointwise_montgomery accumulate then poly_invntt_tomont,
 *      landing in [0,q).
 *   3. add the e-block (+ y'_{1+ELL+i}, the 2*I_m contribution), freeze
 * [0,q).
 *   4. the lift, per coeff:  v = 2*t ; for i==0 v += q*(y'_0[k] & 1)  (RAW
 *      parity) ; v -= ((DQ-1-v)>>31) & DQ   (ONE masked conditional
 *      subtract; v in [0,3q) -> [0,2q)).
 *
 * comY has EM components (the commitment vector length). */
void mat_mul_2q(poly comY[EM], const poly yp[KVEC], const poly16 bhat[EM],
                const poly16 Ahat[EM * ELL]);

/* Verifier commitment.  Computes comY_tilde in [0,2q) from z1 = (z0, zs)
 * (length Z1LEN = 1+ELL, NO e-block), the binary challenge c, and the
 * NARROWER matrix (NO 2*I_m block).  This fuses UseHint steps 1-2: the
 * `- q*c*j mod q` then LiftToModTwoQ.  Per coeff for i==0:
 *   v += q*(z1[0][k] & 1) - q*(c[k] & 1)     (RAW parities)
 *   v in [-q,3q): mod-2q via one masked ADD then one masked SUBTRACT. */
void mat_mul_z1_2q(poly comY_tilde[EM], const poly z1[Z1LEN],
                   const poly *c, const poly16 bhat[EM],
                   const poly16 Ahat[EM * ELL]);

/* KeyGen b-product (alg:keygen-internal step 5):
 *   b_0 = agen + iNTT(hAgen o NTT(s)) + e   (mod q, per pk poly i).
 *
 *   s[ELL]            : the secret s vector (signed scheme-domain poly).
 *   hAgen[EM*ELL]     : the A-hat matrix, NTT-domain CANONICAL order,
 *                       indexed [i*ELL+j] (as produced by ExpandA).
 *   agen[EM]          : the a_gen vector ([0,q) poly16).
 *   e[EM]             : the error e vector (signed scheme-domain poly).
 *   b0[EM]            : output, each coeff freeze'd to [0,q).
 *
 * Shares the rounding.c NTT machinery: the scalar (ref) build uses the
 * canonical poly_ntt / poly_pointwise_montgomery / poly_invntt_tomont;
 * the avx2/avx512 forks route through the SIMD NTT kernels (poly_ntt_simd
 * / poly_pointwise_montgomery_simd / poly_invntt_tomont_simd, with the
 * canonical hAgen imported into backend-native order via
 * poly_ntt_simd_import), byte-exact to the scalar oracle by the same
 * argument as mat_mul_2q. */
void keygen_bproduct(poly b0[EM], const poly16 agen[EM],
                     const poly16 hAgen[EM * ELL], const poly s[ELL],
                     const poly e[EM]);

/* ---------------------------------------------------------------------- *
 *  highbits / hint (alg:makehint, alg:usehint LIVE mod-q) *
 * ----------------------------------------------------------------------
 */

/* highbits_reduced(x), x in [0,2q): round-to-nearest floor((x+alpha_h/2)/
 * alpha_h) (alpha_h is a power of two, so a +alpha_h/2 bias then a right
 * shift), then ONE masked subtract folds into [0,H_h).  NO idiv, NO %.
 * The +alpha_h/2 round can reach H_h (x near 2q-1), so the single >= H_h
 * masked subtract is REQUIRED -- but at most one (b <= H_h, never >=
 * 2H_h), so this is a FOLD, not a general mod H_h. */
int32_t highbits_reduced(int32_t x);

/* hbvalue(bucket) = alpha_h * bucket. */
int32_t hbvalue(int32_t bucket);

/* MakeHint (signer, alg:makehint 1306-1319): per pk poly i, per coeff:
 *   comY_tilde = (comY - 2*z2) mod 2q          (one masked add/sub)
 *   h = (highbits(comY) - highbits(comY_tilde)) mod H_h   in [0,H_h)
 * comY is in [0,2q) (mat_mul_2q output); z2 is the omitted error response.
 */
void make_hint(poly h[EM], const poly comY[EM], const poly z2[EM]);

/* UseHint (verifier, LIVE mod-q variant; alg:usehint 1326-1344).
 * The caller (Verify) first runs mat_mul_z1_2q to get comY_tilde in
 * [0,2q) (UseHint steps 1-2 fused into the matrix product), then use_hint
 * does steps 3-5, per pk poly i, per coeff:
 *   comY_h   = (h + highbits(comY_tilde)) mod H_h          in [0,H_h)
 *   comY_app = ALPHA_H * comY_h + comY0p                   plain, in
 * [0,2q) z2'      = ((comY_app - comY_tilde) / 2) bmodpm q      (even by
 * parity) Outputs comY_h (in [0,H_h)) and z2' (centered rep mod q). comY0p
 * is the recovered low bit (LSB(z0-c)*j), so only its first poly slot is
 * nonzero. The /2 is EXACT because comY_app - comY_tilde is even
 * (lem:commitment- parity); an off-by-one in the LSB lift would break this
 * silently. */
void use_hint(poly comY_h[EM], poly z2p[EM], const poly h[EM],
              const poly comY_tilde[EM], const poly *comY0p);

/* recover comY0' = LSB(z0 - c) * j  (verifier, Verify line 580).  Only the
 * first poly slot of z1 (= z0) and the challenge c are used; j kills the
 * rest, so comY0p[k] = LSB(z0[k]-c[k]) for the first slot and 0 elsewhere.
 * This single-poly helper writes the first commitment-vector poly slot. */
void recon_comY0p(poly *comY0p, const poly *z0, const poly *c);

/* ---------------------------------------------------------------------- *
 *  norm gates (compare-of-squares, no sqrt, constant-time)               *
 * ----------------------------------------------------------------------
 */

/* int64 centered squared L2 of a `len`-long poly array (each coeff
 * centered mod q before squaring; for the StretchS / response coeffs this
 * centering is a no-op since they are already small/centered). */
int64_t poly_array_sqnorm(const poly *v, unsigned int len);

/* KeyGen window: returns 1 iff BK_LOW_SQ <= ||StretchS(1,s,e')||^2 <=
 * BK_SQ (closed interval, inclusive both ends).  Input is the
 * STRETCHED vector (alpha_1*1, alpha_s*s, alpha_e*e'), KVEC long. */
int keygen_norm_ok(const poly stretched[KVEC]);

/* Sign/Verify gate: returns 1 iff ||(z1, z2')||^2 <= BV_SQ.  z1 is Z1LEN
 * (=1+ELL) polys, z2' is EM polys; the concatenated squared norm. */
int response_norm_ok(const poly z1[Z1LEN], const poly z2p[EM]);

/* ====================================================================== *
 *  Norm int64-overflow analysis *
 * ====================================================================== *
 * poly_array_sqnorm accumulates sum_k c_k^2 over all polys in int64.
 *
 * (a) KeyGen window: input StretchS(1,s,e'), per-coeff |.| <= alpha_e*
 *     B_{e,enc} <= 60 (worst 256), so per-coeff square <= 3600; over <=
 *     KVEC*n <= 3072 coeffs the sum <= ~1.1e7 -- but the ACTUAL norm is
 *     ~B_k^2 ~ 88000 (the accept window is a thin band).  Far below 2^63.
 *
 * (b) Sign/Verify gate ||(z1,z2')||^2 <= B_v^2 ~ 6.3e8.  The VERIFIER sees
 *     attacker-chosen z, so the bound must hold over ALL int32 coeffs that
 *     survive sigDecode's per-coeff range checks.  With each z coeff
 *     bounded to its modelled support (~2^15), c_k^2 < 2^30, and over <=
 *     (1+ELL+EM)*n = 6144 coeffs the sum < 2^43 -- no int64 overflow.
 *     OVERFLOW GUARD: even at the full int32 range |c_k| < 2^31 a single
 *     c_k^2 < 2^62 and a few terms would overflow int64; the
 * implementation therefore RELIES on sigDecode's per-coeff range bound to
 * keep the running sum < 2^53.  The
 * norm gate is NOT a substitute for the decode-side range checks.
 *
 * Inclusivity: Sign rejects on strict `> B_v` => response_norm_ok
 * returns 1 iff sqnorm <= BV_SQ; Verify accepts on `<=` -- same boundary.
 * Since the norm-square is an integer and B_v^2 is non-integer,
 * `<= floor(B_v^2)` == `<= B_v^2` exactly.
 * ======================================================================
 */

#endif /* SHUTTLE_ROUNDING_H */
