/*
 * poly_ntt.c -- SCALAR (reference backend) bodies of the NTT shim.
 *
 * The scalar path dispatches to the per-config s<n>_ntt_ref / s<n>_*_ref
 * kernels (selected by SHUTTLE_MODE via poly_ntt.h).  All three configs'
 * scalar kernels keep coefficients in UNSIGNED [0,q) (the scalar uses
 * unsigned math even for the signed q15361 config -- it agrees with the
 * asm mod q), so:
 *   - poly_ntt / poly_invntt_tomont need no signed canonicalization here
 * (that is an AVX2/AVX512-only step; see avx2/poly_ntt.c).
 * invntt_tomont_ref already lands in [0,q), LiftToModTwoQ-ready.
 *   - poly_ntt_canonical IS just ntt_ref: the scalar bit-reversed order is
 * the canonical wire-byte order.
 *   - poly_ntt_import is a no-op: the scalar backend's native order
 * already IS the canonical order, so there is nothing to unpack.
 *
 * ntt_ref_init() fills the scalar twiddle/scale tables; it MUST run once
 * before any kernel.  We guard it with a one-shot flag (the reference
 * build is single-threaded; if a threaded caller ever appears it must
 * pre-init).
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

void poly_ntt(poly16 *a)
{
    ensure_init();
    ntt_ref(a->coeffs); /* normal-in -> bit-reversed NTT order, [0,q) */
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
    /* The scalar ntt_ref output (standard bit-reversed) IS the canonical
     * wire order, so this is identical to poly_ntt for the reference
     * backend. */
    ensure_init();
    ntt_ref(a->coeffs);
}

void poly_ntt_import(poly16 *a)
{
    /* No-op for the scalar backend: native order == canonical order.  The
     * argument is already a canonical-order NTT poly; nothing to unpack.
     * Kept as a function (not a macro) so the call site is
     * backend-agnostic; the AVX backends do real nttunpack work here. */
    (void)a;
}

void poly_ntt_cache(poly16 *a)
{
    /* Scalar backend: native order == canonical order, so the
     * cached-matrix forward NTT is just ntt_ref (== poly_ntt). */
    ensure_init();
    ntt_ref(a->coeffs);
}
