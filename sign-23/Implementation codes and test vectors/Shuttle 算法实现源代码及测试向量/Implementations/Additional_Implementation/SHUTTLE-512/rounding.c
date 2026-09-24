/*
 * avx512/rounding.c -- AVX2/AVX512 SIMD fork of the
 * rounding/lift/hint/ norm substrate, behind USE_AVX2_NTT /
 * USE_AVX512_NTT.
 *
 * === SIMD NTT wiring (the SIMD NTT in the commitment + norm hot path) ===
 * This fork is byte-identical to ref/rounding.c EXCEPT for the two
 * NTT-domain matrix-vector products in the commitment lift:
 *
 *   - mat_mul_2q       (signer: comY = A y' mod 2q, the IRS-loop
 * commitment; PT_COMMIT, ~13.6% of Sign, ~69% of it forward NTT)
 *   - mat_mul_z1_2q    (verifier-side z2' reconstruction = w - 2 z2 mod
 * 2q; PT_NORMCHECK / PT_VF_MATMUL, ~15% of Sign)
 *
 * Previously these ran the SCALAR ntt_ref/invntt_tomont_ref/pointwise_ref
 * oracle (the scheme-facing poly_ntt / poly_invntt_tomont / poly_pointwise
 * shim, which in the avx2/avx512 poly_ntt.c forks dispatches to scalar so
 * the integrated KAT is byte-exact to ref).  HERE we route the genuine
 * vectorized NTT kernels -- poly_ntt_simd / poly_invntt_tomont_simd /
 * poly_pointwise_montgomery_simd / poly_ntt_simd_import -- into those two
 * products.  Those kernels are exported by avx2/avx512 poly_ntt.c and are
 * VALIDATED BYTE-EXACT to the scalar oracle (mod q) by test_ntt_avx /
 * test_ntt_avx512 (the SIMD pipeline ntt->pointwise->invntt == scalar
 * schoolbook product).
 *
 * === WHY THIS IS BYTE-EXACT (the KAT invariant) ===
 * The cached operands bhat / Ahat are produced upstream by the
 * SCHEME-facing poly_ntt (build_cached_matrix in sign.c; the poly_ntt
 * calls in test_rounding) -- i.e. they are in the CANONICAL (ref
 * bit-reversed) NTT slot order, NOT the AVX backend-native order.  The AVX
 * pointwise/invntt kernels consume the BACKEND-NATIVE slot order.  So
 * before any SIMD pointwise we IMPORT each cached canonical operand ONCE
 * into native order via poly_ntt_simd_import (nttunpack: a pure shuffle
 * ladder, no butterflies).  The fresh secret vector (y' / z1) is
 * forward-transformed with poly_ntt_simd, which already lands in native
 * order.  Both pointwise operands then share the one native order, so
 * pointwise needs no reorder; the accumulation (subm16(0,.) negation
 * + poly16_add) is elementwise in [0,q), hence order-agnostic; and
 * poly_invntt_tomont_simd returns standard [0,q) (after the signed-config
 * centered->[0,q) canonicalization it already performs internally).  The
 * validated equivalence nttunpack(canonical) -> pointwise_simd ->
 * invntt_simd  ==  scalar mod q means each q-domain accumulator t_i is
 * IDENTICAL to the scalar compute_t.  The mod-2q lift (2t + true-parity
 * + masked add/sub) downstream is UNCHANGED scalar, so comY (and the
 * reconstructed z2') are bit-identical to the scalar mat_mul_2q /
 * mat_mul_z1_2q -> comY_h / seedC / challenge / signature / KAT all
 * unchanged.  The local oracle is test_rounding section (c): it builds
 * bhat/Ahat via the canonical poly_ntt and asserts mat_mul_2q /
 * mat_mul_z1_2q == a naive plain-int mod-2q reference; that exercise runs
 * through THIS SIMD path here.
 *
 * === IMPORT GRANULARITY (one nttunpack per cached operand per call) ===
 * Each cached operand (bhat[i], Ahat[i*ELL+j]) is consumed exactly ONCE
 * per mat_mul call (bhat[i] once in the i-loop, Ahat[i*ELL+j] once per
 * (i,j)), so importing it once per call == importing it once per use: no
 * per- product-term re-import waste.  nttunpack is shuffle-only and far
 * cheaper than the NTT it precedes.  (Re-importing across IRS iterations
 * -- the cached matrix is rebuilt once per Sign but mat_mul_2q may re-run
 * -- is the price of not forking sign.c; it is bounded by the nttunpack
 * cost, not the NTT cost.)
 *
 * Everything else in this file is IDENTICAL to ref/rounding.c and remains
 * the single CT-audit surface: every op on SECRET-derived data stays
 * branchless masked arithmetic with no idiv / `% const` / float.
 *
 * Read rounding.h FIRST for the API, the StretchS no-mod range analysis,
 * the norm int64-overflow analysis, and the lift/hint/gate rationale.
 *
 * The CompressY round-to-nearest magic reciprocals (RCP_ALPHA_* /
 * SH_ROUND_* / ROUND_BIAS_* / ROUND_K_*) and the LOG2_ALPHA_H shift come from
 * the reproducible generator tools/gen_rounding.py (header
 * tools/rounding_consts.h, included via -I../tools); they reproduce exact
 * integer round-to-nearest-ties-UP (toward +inf) over the full reachable
 * |v| < 2^20 range (proven exhaustively by the generator).  Ties-up is
 * SHIFT-INVARIANT so CompressY(y+StretchS(x)) = CompressY(y)+x holds for every
 * divisor; ties-away broke that identity on even divisors.
 */

#include <stdint.h>

#include "params.h"
#include "poly_ntt.h" /* poly_ntt / poly_invntt_tomont / pointwise */
#include "reduce.h"   /* reduce_mod_2q / freeze / reduce32 */
#include "rounding_consts.h" /* RCP_ALPHA_* / SH_ROUND_* / ROUND_BIAS_* / LOG2_ALPHA_H */
#include "test/prof.h" /* PT_NTT_FWD/PW/INV -- ((void)0) unless PROF_TIME */

/* A few scalar static folds are live only on the non-SIMD (#else) path;
 * the SIMD fork replaces them with their simd_* mirror.  Mark them so the
 * SIMD build does not warn (-Wunused-function). */
#if defined(__GNUC__) || defined(__clang__)
#    define SHUTTLE_MAYBE_UNUSED __attribute__((unused))
#else
#    define SHUTTLE_MAYBE_UNUSED
#endif

/* ====================================================================== *
 *  SIMD NTT entry points: the genuine vectorized kernels exported by
 * * avx2/avx512 poly_ntt.c.  Declared here (the scheme-facing poly.h shim
 * * prototypes only the canonical/scalar-dispatching ops); only this fork
 * * calls the *_simd_* family.  Guarded so a non-AVX build never
 * references * them. *
 * ======================================================================
 */
#if defined(USE_AVX2_NTT) || defined(USE_AVX512_NTT)
#    define SHUTTLE_ROUNDING_SIMD 1
void poly_ntt_simd(poly16 *a);
void poly_invntt_tomont_simd(poly16 *a);
void poly_pointwise_montgomery_simd(poly16 *c, const poly16 *a,
                                    const poly16 *b);
void poly_ntt_simd_import(poly16 *a);
/* SIMD (ymm/zmm) lane-wise mirrors of the scalar reduce/center/hint folds
 * used to vectorize the freeze-glue / use_hint / norm loops below.  Every
 * helper is bit-exact to its reduce.c / rounding.c scalar counterpart, so
 * the avx2/avx512 forks stay byte-exact to ref. */
#    include "simd_red.h"
#endif

/* ====================================================================== *
 *  Internal helpers (all static, all branchless / no idiv on secret data)
 * *
 * ======================================================================
 */

/* round_div_hup: round v/alpha to nearest, TIES UP (toward +inf), with the
 * per-divisor magic reciprocal (recip, shift, bias, koff) so no hardware
 * divide touches secret data.  Round-half-up is SHIFT-INVARIANT, which the
 * CompressY commitment-reconstruction identity requires (round-half-away was
 * NOT shift-invariant and broke the identity on even divisors -- see
 * rounding.h / gen_rounding.py).  v is offset into the non-negative domain
 * (where ties-up == ties-away) by K*alpha, folded into `bias`; the proven
 * unsigned round is applied, then K is subtracted:
 *   q   = (2*v*recip + bias) >> shift  == round_half_up(v/alpha) + K
 *   res = q - koff                     == round_half_up(v/alpha)
 * The shifted operand 2*v*recip+bias = 2*(v+K*alpha)*recip + alpha*recip is
 * always >= 0 (v+K*alpha >= 0 for every reachable v), so the arithmetic shift
 * is an exact floor and NO sign mask / abs / re-sign is needed.
 * 2*v*recip+bias < 2^63 by the generator's int64-safety check, so the product
 * is computed in int64. */
static int32_t round_div_hup(int32_t v, int64_t recip, int shift, int64_t bias,
                             int32_t koff)
{
    int64_t q = (2 * (int64_t)v * recip + bias) >> shift;
    return (int32_t)q - koff;
}

/* bmodpm_pow2: centered representative mod a power-of-two alpha (2 or 4),
 * range [-alpha/2, alpha/2).  alpha = 1<<lg.  v0 = (v & (alpha-1)) then
 * subtract alpha if the result is >= alpha/2.  Branchless masked subtract.
 * (v is the non-negative rep in [0,q), so v & (alpha-1) is the low bits.)
 */
static int32_t bmodpm_pow2(int32_t v, int32_t alpha)
{
    int32_t r = v & (alpha - 1); /* [0, alpha)                     */
    int32_t half = alpha >> 1;   /* alpha/2                        */
    /* if r >= half : r -= alpha  -> [-alpha/2, alpha/2)             */
    int32_t mge = -(int32_t)(r >= half);
    r -= mge & alpha;
    return r;
}

/* bmodpm_q: centered representative mod q (q odd -> [-(q-1)/2, (q-1)/2]).
 * reduce32 lands a in (-q, q); one masked add and one masked subtract of q
 * fold it into the centered window.  Branchless. */
static int32_t bmodpm_q(int32_t a)
{
    int32_t r = reduce32(a);             /* (-q, q)                  */
    int32_t hi = (int32_t)((Q - 1) / 2); /* q odd: (q-1)/2           */
    int32_t mgt = -(int32_t)(r > hi);    /* r > hi  -> -q            */
    r -= mgt & Q;
    int32_t mlt = -(int32_t)(r < -hi); /* r < -hi -> +q              */
    r += mlt & Q;
    return r;
}

/* addmod_Hh: reduce x in [0, 2*H_h) to [0, H_h) by ONE masked subtract.
 * Used where x = h + floor(./alpha_h) is known to be < 2*H_h.  (Scalar
 * use_hint path only; the SIMD fork uses simd_addmod_Hh.) */
SHUTTLE_MAYBE_UNUSED static int32_t addmod_Hh(int32_t x)
{
    int32_t mge = -(int32_t)(x >= (int32_t)HH);
    return x - (mge & (int32_t)HH);
}

/* centermod_Hh: fold a difference x in (-H_h, H_h) into [0, H_h) by ONE
 * masked add (x<0 -> +H_h).  Used for highbits(.)-highbits(.) in MakeHint
 * (both operands in [0,H_h), so the difference is in (-H_h, H_h)).  This
 * is a FOLD of an in-range value (range-check), never a general `% H_h`.
 * (Scalar make_hint path only; the SIMD fork uses simd_centermod_Hh.)
 */
SHUTTLE_MAYBE_UNUSED static int32_t centermod_Hh(int32_t x)
{
    int32_t mlt = (x >> 31); /* -1 if x<0, else 0 (x in (-H_h, H_h)) */
    return x + (mlt & (int32_t)HH);
}

/* ====================================================================== *
 *  LSB / lift (alg:LSB, alg:lift22Q) *
 * ======================================================================
 */

int32_t lsb_coeff(int32_t x)
{
    return reduce_mod_2q(x) & 1; /* (x mod 2q) mod 2 */
}

int32_t lift_to_mod2q_coeff(int32_t xbar, int32_t b)
{
    /* x = xbar if (xbar&1)==b else xbar+q.  Branchless: add q exactly when
     * the parities differ.  xbar in [0,q) -> x in [0,2q). */
    int32_t diff = (xbar & 1) ^ (b & 1); /* 1 iff parities differ */
    return xbar + (-(int32_t)diff & (int32_t)Q);
}

void poly_lift_to_mod2q(poly *x, const poly *xbar, const poly *bpar)
{
    unsigned i;
    for (i = 0; i < N; ++i)
        x->coeffs[i] =
            lift_to_mod2q_coeff(xbar->coeffs[i], bpar->coeffs[i]);
}

/* ====================================================================== *
 *  CompressY / StretchS (alg:compress-y, alg:stretch-s) *
 * ======================================================================
 */

/* The per-block divisor / multiplier is selected by the KVEC partition
 * index (compile-time: block 0 is poly index 0, block s is 1..ELL, block e
 * is 1+ELL..KVEC-1), NOT by any coefficient value. */
static void compress_block(poly *out, const poly *in, int64_t recip,
                           int shift, int64_t bias, int32_t koff)
{
    unsigned i;
    for (i = 0; i < N; ++i)
        out->coeffs[i] =
            round_div_hup(in->coeffs[i], recip, shift, bias, koff);
}

void compress_y(poly out[KVEC], const poly in[KVEC])
{
    int p;
    /* block 0: alpha_1 */
    compress_block(&out[0], &in[0], RCP_ALPHA_1, SH_ROUND_1, ROUND_BIAS_1,
                   ROUND_K_1);
    /* block s: alpha_s over polys 1..ELL */
    for (p = 1; p < 1 + ELL; ++p)
        compress_block(&out[p], &in[p], RCP_ALPHA_S, SH_ROUND_S, ROUND_BIAS_S,
                       ROUND_K_S);
    /* block e: alpha_e over polys 1+ELL..KVEC-1 */
    for (p = 1 + ELL; p < KVEC; ++p)
        compress_block(&out[p], &in[p], RCP_ALPHA_E, SH_ROUND_E, ROUND_BIAS_E,
                       ROUND_K_E);
}

void stretch_s(poly out[KVEC], const poly in[KVEC])
{
    int p;
    unsigned i;
    /* Exact integer multiply, NO modular reduction (see rounding.h COMP).
     * The block multipliers are compile-time constants. */
    for (i = 0; i < N; ++i)
        out[0].coeffs[i] = (int32_t)ALPHA_1 * in[0].coeffs[i];
    for (p = 1; p < 1 + ELL; ++p)
        for (i = 0; i < N; ++i)
            out[p].coeffs[i] = (int32_t)ALPHA_S * in[p].coeffs[i];
    for (p = 1 + ELL; p < KVEC; ++p)
        for (i = 0; i < N; ++i)
            out[p].coeffs[i] = (int32_t)ALPHA_E * in[p].coeffs[i];
}

/* ====================================================================== *
 *  RoundB fused with e' update (alg:roundb + KeyGen 8-9) *
 * ======================================================================
 */

void roundB_update_s2(poly pkb[EM], poly ep[EM], const poly pkb0[EM],
                      const poly e[EM])
{
    int p;
    unsigned i;
    for (p = 0; p < EM; ++p) {
        for (i = 0; i < N; ++i) {
            int32_t b0 = pkb0[p].coeffs[i];
            int32_t vp = freeze(b0);               /* b0 mod q, [0,q)   */
            int32_t v0 = bmodpm_pow2(vp, ALPHA_B); /* [-ab/2, ab/2)    */
            int32_t b = vp - v0; /* multiple of alpha_b in [0,q)        */
            /* delta = (b - b0) bmodpm q, centered rounding error.       */
            int32_t delta = bmodpm_q(b - b0);
            pkb[p].coeffs[i] = b;
            ep[p].coeffs[i] = e[p].coeffs[i] + delta; /* absorb */
        }
    }
}

/* ====================================================================== *
 *  mod-2q commitment products (the lift)                                 *
 * ======================================================================
 */

/* poly_to_ntt_dom: bridge a signed scheme-domain `poly` (small coeffs)
 * into the NTT domain.  freeze each coeff into [0,q) (poly16), then NTT.
 * The compressed mask / response coeffs are bounded < q (CompressY shrinks
 * by alpha_* >= 3), so this is exact -- it stands in for a dedicated
 * small-coefficient NTT entry (SHUTTLE has none; freeze+NTT is the uniform
 * bridge).
 *
 * The fresh secret vector is the one operand we forward-transform per
 * mat_mul, so it goes through the SIMD forward NTT (poly_ntt_simd), which
 * lands in the AVX backend-native order -- the order the SIMD pointwise
 * expects.  (Scalar build: poly_ntt, canonical order.) */
static void poly_to_ntt_dom(poly16 *out, const poly *in)
{
#ifdef SHUTTLE_ROUNDING_SIMD
    unsigned k;
    /* Vectorized freeze + int32->uint16 pack (16 coeffs / iteration).
     * Bit-exact to the scalar (uint16_t)freeze: each lane is freeze in
     * [0,q) < 2^16, so the pack is lossless. */
    for (k = 0; k < N; k += 16)
        simd_freeze_pack(&out->coeffs[k], &in->coeffs[k]);
    poly_ntt_simd(out); /* -> AVX backend-native NTT order */
#else
    unsigned i;
    for (i = 0; i < N; ++i)
        out->coeffs[i] = (uint16_t)freeze(in->coeffs[i]);
    poly_ntt(out);
#endif
}

#ifdef SHUTTLE_ROUNDING_SIMD
/* import_cached: lay a CANONICAL (ref bit-reversed) cached NTT operand
 * into the AVX backend-native slot order via nttunpack (shuffle-only, no
 * butterflies).  Called once per cached operand per mat_mul (== once per
 * use; see file banner), bridging the build_cached_matrix / test poly_ntt
 * (canonical) operand into the order the SIMD pointwise consumes. */
static void import_cached(poly16 *dst, const poly16 *canonical)
{
    *dst = *canonical;
    poly_ntt_simd_import(dst); /* canonical -> native (nttunpack) */
}

/* simd_pw_canon: canonicalize a SIMD pointwise output to [0,q).
 *
 * Representation gotcha, SIGNED config only (q15361, mode 128): the
 * signed pointwise_avx leaves a CENTERED signed Montgomery product in
 * (-q,q) (the dmul output |x|<q), NOT in [0,q).  The unsigned valley
 * configs (q61441/q59393) already return [0,q).  The compute_t
 * accumulation below (subm16(0,.) negation + poly16_add == addm16) is
 * defined on the unsigned [0,q) representation, so for the signed config
 * we must fold the centered value into [0,q) first -- one constant-time
 * conditional add of q to the negative lanes (identical to poly_ntt.c's
 * canon_signed_to_unsigned after invntt).  The scalar pointwise_ref
 * already yields [0,q), so this is the SIMD-only reconciliation that makes
 * the accumulation -- and hence the q-domain accumulator t_i -- byte-exact
 * to the scalar compute_t. */
static void simd_pw_canon(poly16 *a)
{
#    if SHUTTLE_NTT_SIGNED
    unsigned k;
    for (k = 0; k < N; ++k) {
        int16_t x = (int16_t)a->coeffs[k];
        x = (int16_t)(x + ((x >> 15) & (int16_t)Q)); /* x<0 ? x+q : x */
        a->coeffs[k] = (uint16_t)x;
    }
#    else
    (void)a; /* unsigned valley configs: pointwise already [0,q) */
#    endif
}
#endif

/* compute_t: t_i = -bhat_i . x0 + sum_j Ahat[i][j] . xs_j  (mod q), normal
 * domain in [0,q).  Inputs are NTT-domain poly16 IN THE BACKEND-NATIVE
 * ORDER (the SIMD build imports the cached operands + forward-transforms
 * the fresh secret into native order before calling this; the scalar build
 * is canonical order -- compute_t is order-agnostic because
 * pointwise/accum/ invntt all stay within one order). */
static void compute_t(poly *t, const poly16 *bh_i,
                      const poly16 Ahat_native[EM * ELL],
                      const poly16 *x0h, const poly16 xsh[ELL], int i)
{
    poly16 acc, prod, th;
    unsigned k;
    int j;
    (void)k; /* unused in the SIMD path (vectorized loops below) */
#ifdef SHUTTLE_ROUNDING_SIMD
    poly_pointwise_montgomery_simd(&acc, bh_i, x0h);
    simd_pw_canon(&acc); /* signed config: centered (-q,q) -> [0,q) */
    simd_poly16_negate(acc.coeffs, N); /* -bhat_i . x0 (elemwise) */
#else
    poly_pointwise_montgomery(&acc, bh_i, x0h);
    for (k = 0; k < N; ++k)
        acc.coeffs[k] =
            subm16(0, acc.coeffs[k]); /* -bhat_i . x0 (elemwise) */
#endif
    for (j = 0; j < ELL; ++j) {
#ifdef SHUTTLE_ROUNDING_SIMD
        poly_pointwise_montgomery_simd(&prod, &Ahat_native[i * ELL + j],
                                       &xsh[j]);
        simd_pw_canon(&prod); /* signed config: centered -> [0,q) */
#else
        poly_pointwise_montgomery(&prod, &Ahat_native[i * ELL + j],
                                  &xsh[j]);
#endif
        poly16_add(&acc, &acc, &prod); /* addm16, elementwise [0,q) */
    }
    th = acc;
#ifdef SHUTTLE_ROUNDING_SIMD
    poly_invntt_tomont_simd(&th); /* -> [0,q) (canonicalized internally) */
    simd_widen_u16_to_i32(t->coeffs, th.coeffs, N);
#else
    poly_invntt_tomont(&th); /* -> [0,q) (LiftToModTwoQ-ready) */
    for (k = 0; k < N; ++k)
        t->coeffs[k] = (int32_t)th.coeffs[k];
#endif
}

/* compute_t with the NTT pointwise (PT_NTT_PW) / inverse (PT_NTT_INV)
 * sub-buckets timed.  Only mat_mul_2q (the Sign commitment, PT_COMMIT)
 * uses this probed variant, so PT_NTT_* stay disjoint children of
 * PT_COMMIT; the shared (un-probed) compute_t still serves mat_mul_z1_2q
 * (Verify / Sign norm-reconstruction, PT_NORMCHECK / PT_VF_MATMUL).  The
 * probes are ((void)0) unless PROF_TIME, so the data path is identical. */
static void compute_t_prof(poly *t, const poly16 *bh_i,
                           const poly16 Ahat_native[EM * ELL],
                           const poly16 *x0h, const poly16 xsh[ELL], int i)
{
    poly16 acc, prod, th;
    unsigned k;
    int j;
    (void)k; /* unused in the SIMD path (vectorized loops below) */
    {
        PROF_START(t_pw);
#ifdef SHUTTLE_ROUNDING_SIMD
        poly_pointwise_montgomery_simd(&acc, bh_i, x0h);
        simd_pw_canon(&acc); /* signed config: centered (-q,q) -> [0,q) */
        simd_poly16_negate(acc.coeffs, N); /* -bhat_i . x0 */
#else
        poly_pointwise_montgomery(&acc, bh_i, x0h);
        for (k = 0; k < N; ++k)
            acc.coeffs[k] = subm16(0, acc.coeffs[k]); /* -bhat_i . x0 */
#endif
        for (j = 0; j < ELL; ++j) {
#ifdef SHUTTLE_ROUNDING_SIMD
            poly_pointwise_montgomery_simd(
                &prod, &Ahat_native[i * ELL + j], &xsh[j]);
            simd_pw_canon(&prod); /* signed config: centered -> [0,q) */
#else
            poly_pointwise_montgomery(&prod, &Ahat_native[i * ELL + j],
                                      &xsh[j]);
#endif
            poly16_add(&acc, &acc, &prod);
        }
        PROF_STOP(PT_NTT_PW, t_pw);
    }
    th = acc;
    {
        PROF_START(t_inv);
#ifdef SHUTTLE_ROUNDING_SIMD
        poly_invntt_tomont_simd(
            &th); /* -> [0,q) (canonicalized internally) */
#else
        poly_invntt_tomont(&th); /* -> [0,q) (LiftToModTwoQ-ready) */
#endif
        PROF_STOP(PT_NTT_INV, t_inv);
    }
#ifdef SHUTTLE_ROUNDING_SIMD
    simd_widen_u16_to_i32(t->coeffs, th.coeffs, N);
#else
    for (k = 0; k < N; ++k)
        t->coeffs[k] = (int32_t)th.coeffs[k];
#endif
}

void mat_mul_2q(poly comY[EM], const poly yp[KVEC], const poly16 bhat[EM],
                const poly16 Ahat[EM * ELL])
{
    /* bhat / Ahat are already NTT-domain (cached from KeyGen/Sign setup);
     * only the fresh compressed mask y' is forward-transformed here.  The
     * compressed-mask coeff bound is < q (CompressY shrinks y by alpha_*
     * >= 3), so the freeze + NTT bridge is exact.  bhat is cached in the
     * backend-native slot order (poly_ntt_cache); Ahat = hAgen is cached
     * in canonical (ExpandA) order. */
    poly16 x0h, xsh[ELL];
    int i, j;
    unsigned k;
#ifdef SHUTTLE_ROUNDING_SIMD
    /* bhat is already native (built via poly_ntt_cache), so it feeds the
     * SIMD pointwise directly.  Import only the canonical Ahat into the
     * AVX backend-native slot order ONCE (nttunpack); each Ahat operand is
     * imported once and used once per mat_mul call. */
    const poly16 *bhat_n = bhat;
    poly16 Ahat_n[EM * ELL];
    for (i = 0; i < EM * ELL; ++i)
        import_cached(&Ahat_n[i], &Ahat[i]);
#else
    const poly16 *bhat_n = bhat;
    const poly16 *Ahat_n = Ahat;
#endif

    {
        PROF_START(t_fwd);
        poly_to_ntt_dom(&x0h, &yp[0]); /* native order (poly_ntt_simd) */
        for (j = 0; j < ELL; ++j)
            poly_to_ntt_dom(&xsh[j], &yp[1 + j]);
        PROF_STOP(PT_NTT_FWD, t_fwd);
    }

    for (i = 0; i < EM; ++i) {
        poly t;
        compute_t_prof(&t, &bhat_n[i], Ahat_n, &x0h, xsh, i);
#ifdef SHUTTLE_ROUNDING_SIMD
        /* Fused SIMD glue: t += e-block, freeze to [0,q), then the mod-2q
         * lift.  v = 2*t (+ q*parity(y'_0) when i==0), reduced to [0,2q)
         * by one masked conditional subtract.  Bit-exact to the scalar
         * sequence below (simd_freeze == freeze; the parity AND / cmp /
         * mask-sub mirror the scalar ops). */
        {
            const simdv vone = simdv_set1(1);
            const simdv vq = simdv_set1((int)Q);
            const simdv vdq = simdv_set1((int)DQ);
            for (k = 0; k < N; k += SIMD_W) {
                simdv tv = simdv_add(simdv_load(&t.coeffs[k]),
                                     simdv_load(&yp[1 + ELL + i].coeffs[k]));
                tv = simd_freeze(tv);                 /* [0,q)           */
                simdv v = simdv_add(tv, tv);          /* 2*t, [0,2q)     */
                if (i == 0) {
                    /* RAW parity of y'_0 (signed): par = coeff & 1; v +=
                     * q*par.  & 1 on the two's-complement int32 is the true
                     * low bit, matching the scalar yp[0]&1. */
                    simdv par =
                        simdv_and(simdv_load(&yp[0].coeffs[k]), vone);
                    v = simdv_add(v, simdv_mullo(vq, par));
                }
                /* v in [0,3q): v -= ((DQ-1-v)>>31) & DQ. */
                simdv neg = simdv_srai(simdv_sub(simdv_sub(vdq, vone), v), 31);
                v = simdv_sub(v, simdv_and(neg, vdq));
                simdv_store(&comY[i].coeffs[k], v);
            }
        }
#else
        /* + the e-block (the 2*I_m contribution becomes a direct add of
         * the e-block compressed coeff in the q-domain), then freeze to
         * [0,q). */
        for (k = 0; k < N; ++k)
            t.coeffs[k] += yp[1 + ELL + i].coeffs[k];
        for (k = 0; k < N; ++k)
            t.coeffs[k] = freeze(t.coeffs[k]);
        /* The lift, per coeff. */
        for (k = 0; k < N; ++k) {
            int32_t v = 2 * t.coeffs[k]; /* [0,2q)                  */
            if (i == 0) {
                /* q*x0 mod 2q == q*(x0 mod 2): use the TRUE (RAW) coeff
                 * parity yp[0].coeffs[k] & 1, NOT freeze(yp[0])&1 (freeze
                 * adds odd q to negatives, flipping their parity).  The
                 * compressed mask y'_0 is signed (can be negative), so the
                 * freeze'd parity would be wrong for negative coeffs.
                 */
                v += (int32_t)Q * (yp[0].coeffs[k] & 1);
            }
            /* v in [0,3q): mod-2q via ONE masked conditional subtract
             * (matches the avx2/avx512 lift byte-exactly; no Barrett). */
            v -= ((DQ - 1 - v) >> 31) & DQ;
            comY[i].coeffs[k] = v;
        }
#endif
    }
}

void mat_mul_z1_2q(poly comY_tilde[EM], const poly z1[Z1LEN],
                   const poly *c, const poly16 bhat[EM],
                   const poly16 Ahat[EM * ELL])
{
    /* Verifier: z1 = (z0, zs), length Z1LEN = 1+ELL; NO e-block (z2 is
     * reconstructed via the hint, so the matrix has only 1+ELL columns and
     * there is no x_e add).  The challenge c carries the -q*c*j term. */
    poly16 z0h, zsh[ELL];
    int i, j;
    unsigned k;
#ifdef SHUTTLE_ROUNDING_SIMD
    /* Same operand layout as mat_mul_2q: bhat is cached backend-native
     * (poly_ntt_cache) and feeds the SIMD pointwise directly; only the
     * canonical Ahat is imported (nttunpack) into native order. */
    const poly16 *bhat_n = bhat;
    poly16 Ahat_n[EM * ELL];
    for (i = 0; i < EM * ELL; ++i)
        import_cached(&Ahat_n[i], &Ahat[i]);
#else
    const poly16 *bhat_n = bhat;
    const poly16 *Ahat_n = Ahat;
#endif

    poly_to_ntt_dom(&z0h, &z1[0]); /* native order (poly_ntt_simd) */
    for (j = 0; j < ELL; ++j)
        poly_to_ntt_dom(&zsh[j], &z1[1 + j]);

    for (i = 0; i < EM; ++i) {
        poly t;
        compute_t(&t, &bhat_n[i], Ahat_n, &z0h, zsh,
                  i); /* t = -bhat_i.z0 + (Ahat.zs)_i, already [0,q) */
#ifdef SHUTTLE_ROUNDING_SIMD
        /* SIMD lift: v = 2*t (+ q*parity(z0) - q*parity(c) when i==0),
         * reduced to [0,2q) by one masked conditional ADD then one masked
         * conditional SUBTRACT.  Bit-exact to the scalar sequence below. */
        {
            const simdv vone = simdv_set1(1);
            const simdv vq = simdv_set1((int)Q);
            const simdv vdq = simdv_set1((int)DQ);
            for (k = 0; k < N; k += SIMD_W) {
                simdv tv = simdv_load(&t.coeffs[k]);
                simdv v = simdv_add(tv, tv); /* 2*t */
                if (i == 0) {
                    simdv pz =
                        simdv_and(simdv_load(&z1[0].coeffs[k]), vone);
                    simdv pc = simdv_and(simdv_load(&c->coeffs[k]), vone);
                    v = simdv_add(v, simdv_mullo(vq, pz));
                    v = simdv_sub(v, simdv_mullo(vq, pc));
                }
                /* +DQ if v<0 -> [0,3q) */
                v = simdv_add(v, simdv_and(simdv_srai(v, 31), vdq));
                /* -DQ if v>=DQ -> [0,2q) */
                simdv neg = simdv_srai(simdv_sub(simdv_sub(vdq, vone), v), 31);
                v = simdv_sub(v, simdv_and(neg, vdq));
                simdv_store(&comY_tilde[i].coeffs[k], v);
            }
        }
#else
        for (k = 0; k < N; ++k) {
            int32_t v = 2 * t.coeffs[k];
            if (i == 0) {
                /* RAW parities (z1[0] and c), not freeze()&1.
                 * v += q*(z0 parity) - q*(c parity)  (the -q*c*j of
                 * UseHint step 1 carried into the lift). */
                int32_t ck = c->coeffs[k] & 1;
                v += (int32_t)Q * (z1[0].coeffs[k] & 1);
                v -= (int32_t)Q * ck;
            }
            /* v in [-q,3q): mod-2q via one conditional ADD then one
             * conditional SUBTRACT (matches avx2/avx512; no Barrett). */
            v += (v >> 31) & DQ;            /* +DQ if v<0   -> [0,3q)  */
            v -= ((DQ - 1 - v) >> 31) & DQ; /* -DQ if v>=DQ -> [0,2q)  */
            comY_tilde[i].coeffs[k] = v;
        }
#endif
    }
}

void keygen_bproduct(poly b0[EM], const poly16 agen[EM],
                     const poly16 hAgen[EM * ELL], const poly s[ELL],
                     const poly e[EM])
{
    /* b_0 = agen + iNTT(hAgen o NTT(s)) + e   (mod q).  Same SIMD-NTT
     * machinery as mat_mul_2q: the canonical hAgen is imported ONCE into
     * backend-native slot order (nttunpack, shuffle-only); the fresh secret
     * s is forward-transformed via the SIMD NTT into native order; the
     * pointwise/accumulate/invntt then stay within native order.  Byte-exact
     * to the scalar oracle by the same argument as mat_mul_2q (validated by
     * test_ntt_avx512). */
    poly16 shat[ELL];
    int i, j;
    unsigned k;
#ifdef SHUTTLE_ROUNDING_SIMD
    /* Import the canonical hAgen into native order ONCE (hoisted out of the
     * EM row loop). */
    poly16 hAgen_n[EM * ELL];
    for (i = 0; i < EM * ELL; ++i)
        import_cached(&hAgen_n[i], &hAgen[i]);
#else
    const poly16 *hAgen_n = hAgen;
#endif

    /* NTT(s_j) ONCE per j, reused across all EM rows (shat widening hoisted
     * out of the i-loop). */
    for (j = 0; j < ELL; ++j)
        poly_to_ntt_dom(&shat[j], &s[j]);

    for (i = 0; i < EM; ++i) {
        poly16 acc, prod, th;
        for (j = 0; j < ELL; ++j) {
#ifdef SHUTTLE_ROUNDING_SIMD
            poly_pointwise_montgomery_simd(&prod, &hAgen_n[i * ELL + j],
                                           &shat[j]);
            simd_pw_canon(&prod); /* signed config: centered -> [0,q) */
#else
            poly_pointwise_montgomery(&prod, &hAgen_n[i * ELL + j],
                                      &shat[j]);
#endif
            if (j == 0)
                acc = prod;
            else
                poly16_add(&acc, &acc, &prod);
        }
        th = acc;
#ifdef SHUTTLE_ROUNDING_SIMD
        poly_invntt_tomont_simd(&th); /* -> [0,q) (canonicalized) */
        /* Vectorized glue: v = agen + th + e, freeze to [0,q).  simd_freeze
         * == scalar freeze; the int32 widen of the uint16 th/agen lanes is
         * lossless (both in [0,q) < 2^16). */
        {
            int32_t agw[N], thw[N];
            simd_widen_u16_to_i32(agw, agen[i].coeffs, N);
            simd_widen_u16_to_i32(thw, th.coeffs, N);
            for (k = 0; k < N; k += SIMD_W) {
                simdv v =
                    simdv_add(simdv_load(&agw[k]), simdv_load(&thw[k]));
                v = simdv_add(v, simdv_load(&e[i].coeffs[k]));
                simdv_store(&b0[i].coeffs[k], simd_freeze(v));
            }
        }
#else
        poly_invntt_tomont(&th); /* -> (hAgen.s)_i in [0,q) */
        for (k = 0; k < N; ++k) {
            int32_t v = (int32_t)agen[i].coeffs[k] +
                        (int32_t)th.coeffs[k] + e[i].coeffs[k];
            b0[i].coeffs[k] = freeze(v); /* [0,q) */
        }
#endif
    }
}

/* ====================================================================== *
 *  highbits / hint (alg:makehint, alg:usehint LIVE mod-q) *
 * ======================================================================
 */

int32_t highbits_reduced(int32_t x)
{
    /* x in [0,2q).  round-to-nearest /alpha_h (alpha_h a power of two): a
     * +alpha_h/2 bias then a right shift.  x is non-negative so no sign
     * dance. */
    int32_t b = (x + ((int32_t)ALPHA_H >> 1)) >> LOG2_ALPHA_H;
    /* b can reach H_h (x near 2q-1); ONE masked subtract folds into
     * [0,H_h) (b <= H_h, never >= 2H_h -- a FOLD, not a general mod H_h;
     * range-check). */
    int32_t mge = -(int32_t)(b >= (int32_t)HH);
    b -= mge & (int32_t)HH;
    return b; /* [0, H_h) */
}

int32_t hbvalue(int32_t bucket)
{
    return (int32_t)ALPHA_H * bucket;
}

void make_hint(poly h[EM], const poly comY[EM], const poly z2[EM])
{
    int p;
    unsigned i;
    for (p = 0; p < EM; ++p) {
#ifdef SHUTTLE_ROUNDING_SIMD
        /* SIMD over the inner N coeffs.  Lane-wise mirror: wt =
         * reduce_mod_2q(w - 2*z2), hb = highbits(w) - highbits(wt), then
         * centermod_Hh. */
        for (i = 0; i < N; i += SIMD_W) {
            simdv w = simdv_load(&comY[p].coeffs[i]);
            simdv z2v = simdv_load(&z2[p].coeffs[i]);
            simdv wt = simd_reduce_mod_2q(
                simdv_sub(w, simdv_slli(z2v, 1))); /* w - 2*z2 */
            simdv hb = simdv_sub(simd_highbits_reduced(w),
                                 simd_highbits_reduced(wt));
            simdv_store(&h[p].coeffs[i], simd_centermod_Hh(hb));
        }
#else
        for (i = 0; i < N; ++i) {
            int32_t w = comY[p].coeffs[i]; /* in [0,2q)              */
            /* comY_tilde = (comY - 2*z2) mod 2q.  2*z2 can be negative /
             * exceed 2q, so reduce to [0,2q): one masked add then one
             * masked subtract suffices iff the pre-reduced value is in
             * (-2q, 4q); to be safe over any z2 we use reduce_mod_2q
             * (Barrett, [0,2q)). comY is already reduced, so highbits
             * skips its reduce. */
            int32_t wt = reduce_mod_2q(w - 2 * z2[p].coeffs[i]);
            int32_t hb = highbits_reduced(w) - highbits_reduced(wt);
            /* hb in (-H_h, H_h): fold into [0,H_h) with one masked add. */
            h[p].coeffs[i] = centermod_Hh(hb);
        }
#endif
    }
}

void use_hint(poly comY_h[EM], poly z2p[EM], const poly h[EM],
              const poly comY_tilde[EM], const poly *comY0p)
{
    int p;
    unsigned i;
    for (p = 0; p < EM; ++p) {
#ifdef SHUTTLE_ROUNDING_SIMD
        /* SIMD over the inner N coeffs (SIMD_W int32 / vector).  Lane-wise
         * mirror of the scalar body: highbits_reduced + addmod_Hh fold,
         * then alpha_h*cyh + c0, then ((.)-wt)>>1 centered mod q.  c0 =
         * comY0p only for p==0 (j kills the rest), else 0. */
        const simdv valpha = simdv_set1((int)ALPHA_H);
        for (i = 0; i < N; i += SIMD_W) {
            simdv wt = simdv_load(&comY_tilde[p].coeffs[i]);
            simdv c0 = (p == 0) ? simdv_load(&comY0p->coeffs[i])
                                : simdv_setzero();
            simdv hb = simd_highbits_reduced(wt);
            simdv cyh = simd_addmod_Hh(
                simdv_add(simdv_load(&h[p].coeffs[i]), hb));
            simdv_store(&comY_h[p].coeffs[i], cyh);
            simdv app = simdv_add(simdv_mullo(valpha, cyh), c0);
            simdv even = simdv_sub(app, wt);
            simdv z2 = simd_bmodpm_q(simdv_srai(even, 1)); /* arith /2 */
            simdv_store(&z2p[p].coeffs[i], z2);
        }
#else
        for (i = 0; i < N; ++i) {
            int32_t wt = comY_tilde[p].coeffs[i]; /* in [0,2q)        */
            /* comY0p is LSB(z0-c)*j: only the FIRST poly slot is nonzero;
             * elsewhere comY0p is 0 (j kills the rest). */
            int32_t c0 = (p == 0) ? comY0p->coeffs[i] : 0;
            /* comY_h = (h + highbits(comY_tilde)) mod H_h.  Both addends
             * are in [0,H_h), so the sum is in [0, 2*H_h): one masked
             * subtract folds it (range-check fold, not a wrap). */
            int32_t cyh = addmod_Hh(h[p].coeffs[i] + highbits_reduced(wt));
            comY_h[p].coeffs[i] = cyh;
            /* comY_app = alpha_h*comY_h + comY0p  (plain, in [0,2q) since
             * comY_h < H_h and comY0p in {0,1}). */
            int32_t app = hbvalue(cyh) + c0;
            /* z2' = ((comY_app - comY_tilde)/2) bmodpm q.  The bracket is
             * EVEN by the commitment-parity lemma, so the /2 is exact (no
             * rounding); an off-by-one in the LSB lift would make it odd.
             */
            int32_t even = app - wt;
            z2p[p].coeffs[i] = bmodpm_q(even >> 1); /* arithmetic /2 */
        }
#endif
    }
}

void recon_comY0p(poly *comY0p, const poly *z0, const poly *c)
{
    unsigned i;
    /* comY0p = LSB(z0 - c) * j: only the first poly slot is filled here
     * (the caller passes the first commitment-vector slot; j kills the
     * rest). */
    for (i = 0; i < N; ++i)
        comY0p->coeffs[i] = lsb_coeff(z0->coeffs[i] - c->coeffs[i]);
}

/* ====================================================================== *
 *  norm gates (compare-of-squares, no sqrt, constant-time)               *
 * ======================================================================
 */

int64_t poly_array_sqnorm(const poly *v, unsigned int len)
{
    int64_t acc = 0;
    unsigned int p;
    for (p = 0; p < len; ++p)
        acc += poly_sqnorm(&v[p]); /* centered sum c_k^2 in int64 */
    return acc;
}

int keygen_norm_ok(const poly stretched[KVEC])
{
    int64_t nsq = poly_array_sqnorm(stretched, (unsigned)KVEC);
    /* Closed window, inclusive both ends: accept iff
     * BK_LOW_SQ <= nsq <= BK_SQ.  Branchless comparison; the CALLER
     * branches on the PUBLIC accept/reject outcome (KeyGen-only -> no
     * signing-time leak). */
    int ge_lo = (nsq >= BK_LOW_SQ);
    int le_hi = (nsq <= BK_SQ);
    return ge_lo & le_hi;
}

int response_norm_ok(const poly z1[Z1LEN], const poly z2p[EM])
{
    int64_t nsq = poly_array_sqnorm(z1, (unsigned)Z1LEN);
    nsq += poly_array_sqnorm(z2p, (unsigned)EM);
    /* accept iff nsq <= BV_SQ (Sign rejects on strict '>'; Verify accepts
     * on
     * '<='; same boundary). */
    return (nsq <= BV_SQ);
}
