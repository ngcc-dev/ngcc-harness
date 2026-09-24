/*
 * avx512/poly_ntt.c -- AVX-512 fork of the NTT shim, behind
 * USE_AVX512_NTT.  Opt-in (built only by the AVX512 targets).
 *
 * === Scalar-dispatch wiring (byte-exactness) ===
 * Identical rationale to avx2/poly_ntt.c: in the byte-exact configuration
 * the SCHEME (sign.c/polyvec.c/ rounding.c, all symlinked SCALAR) runs
 * unmodified.  The scalar ExpandA emits the cached matrix hAgen in
 * CANONICAL order and never imports it (that import is the AVX-fork step).
 * So the scheme-facing shim ops MUST be the canonical/scalar convention
 * for the NTT-domain products to be correct, which makes the AVX-512
 * build's integrated KAT BYTE-EXACT to the reference -- the byte-exactness
 * gate.
 * poly_ntt_import is a no-op.
 *
 * The genuine AVX-512 NTT asm kernels are validated byte-exact to the
 * scalar oracle by test/test_ntt_avx512.c (via the poly_ntt_simd_*
 * wrappers below). Wiring them into the scheme hot path is a later perf
 * step (forked polyvec/sign that poly_ntt_imports the canonical
 * hAgen).
 *
 * The AVX-512 kernel family is UNIFORM across all three configs:
 *   ntt/invntt: (poly, qdata, ztab, scale);  pointwise: (c,a,b,qdata).
 * For q59393 (n=1024) this is the 2x512-coeff SUPERBLOCK kernel; the
 * wrapper does not see that -- the entry signature is identical.  q15361
 * (signed) needs a centered->[0,q) canonicalization after invntt;
 * the AVX-512 invntt has no reduce_avx export so it is open-coded.
 */
#include "poly_ntt.h"

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
    ntt_ref(a->coeffs);
}

void poly_invntt_tomont(poly16 *a)
{
    ensure_init();
    invntt_tomont_ref(a->coeffs);
}

void poly_pointwise_montgomery(poly16 *c, const poly16 *a, const poly16 *b)
{
    ensure_init();
    pointwise_ref(a->coeffs, b->coeffs, c->coeffs);
}

void poly_ntt_canonical(poly16 *a)
{
    ensure_init();
    ntt_ref(a->coeffs);
}

void poly_ntt_import(poly16 *a)
{
    (void)
        a; /* no-op (poly_ntt is canonical); real nttunpack below */
}

/* ===================================================================== *
 *  AVX-512 SIMD kernels, exported for byte-exactness validation          *
 *  (test/test_ntt_avx512.c).                                             *
 * ===================================================================== */

#if SHUTTLE_NTT_SIGNED
/* Centered signed int16 (|x| < q) -> [0,q): add q to negative lanes. */
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
    s256_ntt_avx512((int16_t *)a->coeffs, s256_ntt512_qdata,
                    s256_ntt512_zetas_fwd, s256_ntt512_scale);
#elif SHUTTLE_MODE == 256
    s512_ntt_avx512((int16_t *)a->coeffs, s512_ntt512_qdata,
                    s512_ntt512_zetas_fwd, s512_ntt512_scale);
#elif SHUTTLE_MODE == 512
    s1024_ntt_avx512((int16_t *)a->coeffs, s1024_ntt512_qdata,
                     s1024_ntt512_zetas_fwd, s1024_ntt512_scale);
#endif
}

void poly_ntt_cache(poly16 *a)
{
    /* Forward NTT for a freshly built cached-matrix operand, landing
     * directly in the AVX512 backend-native slot order via the vectorized
     * NTT.  By the validated equivalence nttunpack(ntt_ref(p)) ==
     * ntt_avx(p) (test_ntt_avx512 [3]) this is byte-identical to the
     * prior flow of a scalar canonical forward NTT followed by a per-use
     * nttunpack (poly_ntt_simd_import) at the pointwise call site, so it
     * is KAT-neutral while folding both into one vectorized pass. */
    poly_ntt_simd(a);
}

void poly_invntt_tomont_simd(poly16 *a)
{
#if SHUTTLE_MODE == 128
    s256_invntt_tomont_avx512((int16_t *)a->coeffs, s256_ntt512_qdata,
                              s256_ntt512_zetas_inv, s256_ntt512_scale);
    canon_signed_to_unsigned(a); /* centered (|x|<q) -> [0,q) */
#elif SHUTTLE_MODE == 256
    s512_invntt_tomont_avx512((int16_t *)a->coeffs, s512_ntt512_qdata,
                              s512_ntt512_zetas_inv, s512_ntt512_scale);
#elif SHUTTLE_MODE == 512
    s1024_invntt_tomont_avx512((int16_t *)a->coeffs, s1024_ntt512_qdata,
                               s1024_ntt512_zetas_inv, s1024_ntt512_scale);
#endif
}

void poly_pointwise_montgomery_simd(poly16 *c, const poly16 *a,
                                    const poly16 *b)
{
#if SHUTTLE_MODE == 128
    s256_pointwise_avx512((int16_t *)c->coeffs, (const int16_t *)a->coeffs,
                          (const int16_t *)b->coeffs, s256_ntt512_qdata);
#elif SHUTTLE_MODE == 256
    s512_pointwise_avx512((int16_t *)c->coeffs, (const int16_t *)a->coeffs,
                          (const int16_t *)b->coeffs, s512_ntt512_qdata);
#elif SHUTTLE_MODE == 512
    s1024_pointwise_avx512((int16_t *)c->coeffs,
                           (const int16_t *)a->coeffs,
                           (const int16_t *)b->coeffs, s1024_ntt512_qdata);
#endif
}

void poly_ntt_simd_import(poly16 *a)
{
    int32_t src[N];
    int i;
    for (i = 0; i < N; i++)
        src[i] = (int32_t)a->coeffs[i];
#if SHUTTLE_MODE == 128
    s256_nttunpack_avx512((int16_t *)a->coeffs, src);
#elif SHUTTLE_MODE == 256
    s512_nttunpack_avx512((int16_t *)a->coeffs, src);
#elif SHUTTLE_MODE == 512
    s1024_nttunpack_avx512((int16_t *)a->coeffs, src);
#endif
}
