/*
 * poly.h -- ring-element TYPES for SHUTTLE + the NTT shim prototypes.
 *
 * Ownership split:
 *   - The TWO poly types below and the poly_ntt /
 *     poly_invntt_tomont / poly_pointwise / poly_ntt_canonical /
 *     poly_ntt_import shim prototypes (implemented in poly_ntt.{c,h}).
 *   - The scheme-domain arithmetic helpers (poly_add, poly_sub,
 *     poly_reduce, poly_freeze, poly_sqnorm, packing, ...) are APPENDED
 * below the marker comment at the end of this file.
 *
 * TWO coefficient representations coexist:
 *   - `poly`   : int32_t coeffs[N].  The SCHEME-DOMAIN element.
 * Coefficients are signed-centered / reduced via the Barrett helpers in
 *                reduce.{c,h}; wide enough for the mod-2q lift (2q can
 * exceed 2^16 for the valley sets) and for un-reduced accumulators.
 *   - `poly16` : uint16_t coeffs[N].  The NTT-DOMAIN element, canonical
 * [0,q). This is what the NTT backends (scalar ntt_ref / AVX2 / AVX512)
 *                operate on; the (int16_t*) reinterpret cast at each
 * backend call site is a bit-pattern reinterpret (signed-vs-unsigned is
 *                only the asm's VIEW of the same 16 bits) and is guarded
 * by _Static_assert(sizeof(poly16) == 2*N).
 */
#ifndef SHUTTLE_POLY_H
#define SHUTTLE_POLY_H

#include <stdint.h>

#include "params.h" /* N (per-set) */
/* Disable with -DDISABLE_NAMESPACE=1. */
#include "namespace.h"

typedef struct {
    int32_t coeffs[N];
} poly;

typedef struct {
    uint16_t coeffs[N];
} poly16;

/* The NTT backends consume poly16 as a flat int16_t[N] buffer via a
 * reinterpret cast; this pins the in-memory layout the cast relies on. */
_Static_assert(sizeof(poly16) == 2 * N,
               "poly16 must be a flat uint16_t[N] (no padding) for the "
               "(int16_t*) NTT-backend reinterpret cast");

/* ===================================================================== *
 *  NTT shim prototypes (bodies in poly_ntt.c + the avx2/avx512           *
 *  forks).  Uniform UPWARD contract over poly16, [0,q) canonical.  See   *
 *  poly_ntt.h for the full per-config dispatch + readback documentation. *
 * ===================================================================== */

/* Forward NTT.  Output is in the BACKEND-NATIVE NTT slot order (ref =
 * bit-reversed; AVX = shuffle-network permutation) -- order is INTERNAL,
 * consumed only by the matching poly_pointwise + poly_invntt_tomont. */
void poly_ntt(poly16 *a);

/* Inverse NTT + to-Montgomery, canonicalized to [0,q) (after the signed
 * config's red16 + cond-add-q).  LiftToModTwoQ-ready. */
void poly_invntt_tomont(poly16 *a);

/* Coefficient-wise Montgomery product c = a*b*R^-1 (both operands in the
 * SAME poly_ntt order, so no reorder is needed). */
void poly_pointwise_montgomery(poly16 *c, const poly16 *a,
                               const poly16 *b);

/* Forward NTT leaving the EXACT scalar (ref bit-reversed) order -- the
 * canonical wire-byte order for pk/sk/com.  KAT-critical (the NTT-domain
 * wire order). */
void poly_ntt_canonical(poly16 *a);

/* Import a canonical-order (ref bit-reversed) NTT poly into the
 * backend-native slot order: ref = no-op (memcpy identity), AVX =
 * nttunpack shuffle.  Route any standard-sampled / wire-read poly through
 * this before an AVX pointwise (the NTT-domain wire order). */
void poly_ntt_import(poly16 *a);

/* Forward NTT for a freshly built CACHED-matrix operand, landing directly
 * in the BACKEND-NATIVE slot order: ref = ntt_canonical (native ==
 * canonical for the scalar backend), AVX = the vectorized forward NTT.
 * Used when the result is consumed only by the backend-native pointwise
 * (so it can skip the separate canonical->native import).  Byte-identical
 * to poly_ntt_import(poly_ntt(a)). */
void poly_ntt_cache(poly16 *a);

/* === arithmetic helpers extend poly.h below === */

/* `poly` (int32 coeffs[N]) is the scheme/wire type: it must hold centered
 * coefficients, un-reduced accumulators, and the mod-2q lift (2q can
 * exceed 2^16 for the valley sets).  Pin its flat layout for the few
 * places that memset/memcpy a whole poly. */
_Static_assert(sizeof(poly) == 4 * N,
               "poly must be a flat int32_t[N] (no padding)");

/* ---------------------------------------------------------------------- *
 *  Scheme-domain arithmetic helpers.  Bodies in poly.c.                  *
 *                                                                        *
 *  These operate on the signed int32 `poly` type and wrap the per-coeff  *
 *  reduce.{c,h} primitives (reduce32 / caddq / freeze).  They run on     *
 *  data that may be SECRET (s, e', NTT residues), so they are            *
 *  division-free and contain no data-dependent branch (the underlying    *
 *  Barrett/masked helpers are constant-time; the loops are fixed-length).*
 * ----------------------------------------------------------------------
 */

/* Coefficient-wise reduce32: every coeff -> (-q, q) (truncating a % q). */
void poly_reduce(poly *a);

/* Coefficient-wise conditional +q: negatives -> +q (precondition: input
 * already in (-q, q), i.e. post-poly_reduce).  Lands in [0, q). */
void poly_caddq(poly *a);

/* Coefficient-wise standard representative in [0, q)  (reduce32 then
 * caddq).
 * == freeze() per coeff. */
void poly_freeze(poly *a);

/* c = a + b coefficient-wise (no reduction; caller reduces if needed). */
void poly_add(poly *c, const poly *a, const poly *b);

/* c = a - b coefficient-wise (no reduction; caller reduces if needed). */
void poly_sub(poly *c, const poly *a, const poly *b);

/* poly16 add in the canonical [0,q) NTT domain (addm16 per coeff). */
void poly16_add(poly16 *c, const poly16 *a, const poly16 *b);

/* Centered squared L2 norm, summed as int64.  Each coeff is first centered
 * mod q (representative in (-q/2, q/2]) so that the squared norm is the
 * scheme's geometric norm regardless of the input representative.  Used by
 * the KeyGen norm-window gate (compare against BK_SQ / BK_LOW_SQ) and the
 * Sign/Verify gate (BV_SQ); int64 cannot overflow (max ~N*(q/2)^2 <<
 * 2^63).
 *
 * NOTE: this centers mod q.  The KeyGen/Sign callers feed it the centered
 * StretchS / response coefficients directly, so the centering is a no-op
 * there; it is included so the helper is correct for any representative.
 */
int64_t poly_sqnorm(const poly *a);

/*
 * unpack_pk_bn -- fused bitunpack + rescale for the public-key b poly.
 *
 * The SHUTTLE design point: the packed pk field is the COEFFICIENT-DOMAIN
 * quotient b1 = b/alpha_b in [0, ceil(q/alpha_b)), NOT an NTT-domain
 * residue.  Reconstruction is b = alpha_b*b1, a left shift by 1 or 2 bits
 * (alpha_b in {2,4}).  Because alpha_b*(ceil(q/alpha_b)-1) < q, the
 * rescale alone lands every coeff in [0, q): there is NO conditional
 * subtract of q (unlike a freeze-fused bitunpack) and NO nttunpack /
 * poly_ntt_import afterwards (b is consumed in the COEFFICIENT domain by
 * KeyGen / Sign / Verify).
 *
 * This is the FAST, TRUSTED-input path: the caller (sk path) guarantees
 * the bytes came from our own pack_pk/pack_sk, so the b1 < ceil(q/ab)
 * range check is SKIPPED.  The public/untrusted path (pk_decode) does the
 * range check and rejects, see packing.h.  `packed` points at the first
 * packed poly; stride is POLYPK_PACKEDBYTES.  KAT-neutral.
 */
void unpack_pk_bn(poly b[EM], const uint8_t *packed);

#endif /* SHUTTLE_POLY_H */
