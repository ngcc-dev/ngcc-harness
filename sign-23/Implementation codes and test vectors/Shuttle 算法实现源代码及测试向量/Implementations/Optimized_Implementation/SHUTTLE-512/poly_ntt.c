/*
 * avx2/poly_ntt.c -- AVX2 fork of the NTT shim, behind USE_AVX2_NTT.
 *
 * === Scalar-dispatch wiring (byte-exactness) ===
 * In the byte-exact configuration the SCHEME (sign.c / polyvec.c /
 * rounding.c, all symlinked SCALAR) runs unmodified.  The scalar ExpandA
 * writes the cached matrix hAgen in CANONICAL (ref bit-reversed) NTT order
 * and -- crucially -- never calls poly_ntt_import on it (that import is
 * the AVX-fork step).  The scalar sign.c then forms NTT-domain products
 * poly_pointwise_montgomery(hAgen, poly_ntt(s)).  For that product to be
 * correct BOTH operands must share one NTT order AND one Montgomery
 * scaling convention.  hAgen is fixed at the canonical/scalar convention,
 * so the scheme-facing shim ops here MUST also be the canonical/scalar
 * convention.
 *
 * Therefore the scheme-facing shim entries (poly_ntt / poly_invntt_tomont
 * / poly_pointwise_montgomery / poly_ntt_canonical) dispatch to the
 * per-config s<n>_*_ref scalar kernels.  This makes the AVX2 build's
 * integrated KAT BYTE-EXACT to the reference (ref==avx2==avx512), which is
 * the whole byte-exactness gate. poly_ntt_import is a no-op (poly_ntt
 * already emits canonical == the order the scheme expects; nothing to
 * unpack).
 *
 * The genuine AVX2 NTT asm kernels (s<n>_ntt_avx / s<n>_invntt_tomont_avx
 * / s<n>_pointwise_avx / s<n>_nttunpack_avx + the signed canonicalization)
 * are NOT dead: they are validated BYTE-EXACT to the scalar oracle by
 * test/test_ntt_avx.c, which drives them through the poly_ntt_simd_*
 * wrappers exported below.  Wiring those SIMD kernels into the *scheme*
 * hot path (with a forked polyvec.c/sign.c that poly_ntt_imports the
 * canonical hAgen into the AVX2-native slot layout) is a later PERF
 * step -- it is a perf change, not a correctness one, and it is gated
 * on the AVX2 forks of polyvec/sign.
 *
 * === The AVX2 SIMD kernels (exercised by test_ntt_avx via the *_simd_*
 *     wrappers) ===
 * TWO signature families:
 *   - SIGNED q15361 (SHUTTLE_MODE==128): ntt/invntt take (poly, qdata,
 * ztab, z0/z0inv, scale); pointwise (c,a,b,qdata); plus s256_reduce_avx.
 * The signed invntt leaves a lazy ~2q value; reduce_avx + a conditional +q
 * for negative lanes canonicalizes it to [0,q).
 *   - UNSIGNED q61441/q59393: ntt/invntt take (poly, qdata, ztab, cross,
 * ninv); pointwise (c,a,b,qdata); invntt already yields [0,q). poly16
 * (uint16_t[N]) is reinterpret-cast to int16_t* at each backend call;
 * sound because sizeof(poly16)==2*N (asserted in poly.h).
 */
#include "poly_ntt.h"

/* ---- one-shot init of the scalar twiddle/scale tables (used by both the
 * scheme-facing canonical path and the test wrappers' nttunpack source).
 * ---- */
static int s_ntt_inited = 0;
static void ensure_init(void)
{
    if (!s_ntt_inited) {
        ntt_ref_init();
        s_ntt_inited = 1;
    }
}

/* ===================================================================== *
 *  Scheme-facing shim: CANONICAL / scalar convention (byte-exact).      *
 * ===================================================================== */

void poly_ntt(poly16 *a)
{
    ensure_init();
    ntt_ref(
        a->coeffs); /* normal-in -> canonical bit-reversed NTT, [0,q) */
}

void poly_invntt_tomont(poly16 *a)
{
    ensure_init();
    invntt_tomont_ref(a->coeffs); /* -> [0,q); bare round-trip = a*R */
}

void poly_pointwise_montgomery(poly16 *c, const poly16 *a, const poly16 *b)
{
    ensure_init();
    pointwise_ref(a->coeffs, b->coeffs,
                  c->coeffs); /* c = a*b*R^-1, [0,q) */
}

void poly_ntt_canonical(poly16 *a)
{
    /* Canonical (ref bit-reversed) wire order == the scheme NTT order
     * here. */
    ensure_init();
    ntt_ref(a->coeffs);
}

void poly_ntt_import(poly16 *a)
{
    /* No-op: poly_ntt already emits canonical order (the scheme's
     * NTT-domain operands all share it), so there is nothing to unpack.
     * The real AVX2 nttunpack lives in poly_ntt_simd_import below and is
     * exercised by test_ntt_avx; it becomes the scheme path once the
     * polyvec/sign forks wire it in.
     */
    (void)a;
}

/* ===================================================================== *
 *  AVX2 SIMD kernels, exported for byte-exactness validation             *
 *  (test/test_ntt_avx.c).  These are the genuine vectorized NTT; they    *
 *  are proven bit-identical to the scalar oracle above, which is what    *
 *  authorizes wiring them into the scheme hot path later.               *
 * ===================================================================== */

#if SHUTTLE_NTT_SIGNED
/* Map the centered red16 output of s256_reduce_avx into [0,q): add q to
 * every negative int16 lane (constant-time sign-mask).  Operates in place
 * on the poly16 viewed as signed int16. */
static void canon_signed_to_unsigned(poly16 *a)
{
    int16_t *p = (int16_t *)a->coeffs;
    int i;
    for (i = 0; i < N; i++) {
        int16_t x = p[i];
        x = (int16_t)(x + ((x >> 15) & (int16_t)Q)); /* x<0 ? x+q : x */
        p[i] = x;
    }
}
#endif

void poly_ntt_simd(poly16 *a)
{
#if SHUTTLE_MODE == 128
    s256_ntt_avx((int16_t *)a->coeffs, s256_ntt_qdata, s256_ntt_zetas_fwd,
                 s256_ntt_z0, s256_ntt_scale);
#elif SHUTTLE_MODE == 256
    s512_ntt_avx((int16_t *)a->coeffs, s512_ntt_qdata, s512_ntt_zetas_fwd,
                 s512_ntt_cross_fwd, s512_ntt_ninv);
#elif SHUTTLE_MODE == 512
    s1024_ntt_avx((int16_t *)a->coeffs, s1024_ntt_qdata,
                  s1024_ntt_zetas_fwd, s1024_ntt_cross_fwd,
                  s1024_ntt_ninv);
#endif
}

void poly_ntt_cache(poly16 *a)
{
    /* Forward NTT for a freshly built cached-matrix operand, landing
     * directly in the AVX2 backend-native slot order via the vectorized
     * NTT.  By the validated equivalence nttunpack(ntt_ref(p)) ==
     * ntt_avx(p) (test_ntt_avx [3]) this is byte-identical to the prior
     * flow of a scalar canonical forward NTT followed by a per-use
     * nttunpack (poly_ntt_simd_import) at the pointwise call site, so it
     * is KAT-neutral while folding both into one vectorized pass. */
    poly_ntt_simd(a);
}

void poly_invntt_tomont_simd(poly16 *a)
{
#if SHUTTLE_MODE == 128
    s256_invntt_tomont_avx((int16_t *)a->coeffs, s256_ntt_qdata,
                           s256_ntt_zetas_inv, s256_ntt_z0inv,
                           s256_ntt_scale);
    s256_reduce_avx(
        (int16_t *)a->coeffs);   /* lazy ~2q -> centered |x|<~q/2 */
    canon_signed_to_unsigned(a); /* centered -> [0,q) */
#elif SHUTTLE_MODE == 256
    s512_invntt_tomont_avx((int16_t *)a->coeffs, s512_ntt_qdata,
                           s512_ntt_zetas_inv, s512_ntt_cross_inv,
                           s512_ntt_ninv); /* already [0,q) */
#elif SHUTTLE_MODE == 512
    s1024_invntt_tomont_avx((int16_t *)a->coeffs, s1024_ntt_qdata,
                            s1024_ntt_zetas_inv, s1024_ntt_cross_inv,
                            s1024_ntt_ninv); /* already [0,q) */
#endif
}

void poly_pointwise_montgomery_simd(poly16 *c, const poly16 *a,
                                    const poly16 *b)
{
#if SHUTTLE_MODE == 128
    s256_pointwise_avx((int16_t *)c->coeffs, (const int16_t *)a->coeffs,
                       (const int16_t *)b->coeffs, s256_ntt_qdata);
#elif SHUTTLE_MODE == 256
    s512_pointwise_avx((int16_t *)c->coeffs, (const int16_t *)a->coeffs,
                       (const int16_t *)b->coeffs, s512_ntt_qdata);
#elif SHUTTLE_MODE == 512
    s1024_pointwise_avx((int16_t *)c->coeffs, (const int16_t *)a->coeffs,
                        (const int16_t *)b->coeffs, s1024_ntt_qdata);
#endif
}

void poly_ntt_simd_import(poly16 *a)
{
    /* Canonical (ref bit-reversed) order -> AVX2 backend-native slot order
     * via nttunpack.  nttunpack consumes int32 standard-order [0,q)
     * samples; widen each canonical-order lane to int32 then replay the
     * forward shuffle ladder (no butterflies). */
    int32_t src[N];
    int i;
    for (i = 0; i < N; i++)
        src[i] = (int32_t)a->coeffs[i];
#if SHUTTLE_MODE == 128
    s256_nttunpack_avx((int16_t *)a->coeffs, src);
#elif SHUTTLE_MODE == 256
    s512_nttunpack_avx((int16_t *)a->coeffs, src);
#elif SHUTTLE_MODE == 512
    s1024_nttunpack_avx((int16_t *)a->coeffs, src);
#endif
}
