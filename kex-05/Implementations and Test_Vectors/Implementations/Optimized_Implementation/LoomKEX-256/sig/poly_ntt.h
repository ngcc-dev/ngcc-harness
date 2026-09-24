/*
 * poly_ntt.h -- the per-config NTT shim.
 *
 * This is the ONE uniform contract that poly.c / polyvec.c call.  It
 * hides three sources of per-config divergence that would otherwise leak
 * upward:
 *
 *  1. TWO AVX2 signature families.  The signed config (q15361) takes
 *     (poly, qdata, ztab, z0/z0inv, scale) + an extra reduce_avx; the
 * unsigned valley configs (q61441/q59393) take (poly, qdata, ztab, cross,
 * ninv) and have no reduce_avx.  The AVX512 family is uniform
 * (poly,qdata,ztab,scale). The shim bodies hard-code the constant-table
 * arguments per config so the caller never sees this.
 *
 *  2. The s256_/s512_/s1024_ symbol prefix the vendored asm/consts carry
 * (so all three configs co-link).  This header maps the generic backend
 * names (NTT_FWD/NTT_INV/...) to the prefixed kernels in one #if-block per
 * mode.
 *
 *  3. Coefficient-ORDER divergence.  ntt_ref leaves output in
 * standard bit-reversed order; the asm leaves it in a fixed
 * shuffle-network permutation.  poly_ntt produces the BACKEND-NATIVE order
 * (consumed only by the matching pointwise+invntt, so order is internal).
 *     poly_ntt_canonical produces the EXACT ref bit-reversed order for
 * wire bytes (pk/sk/com); poly_ntt_import lays a canonical-order poly into
 * the backend-native order (ref = no-op; AVX = nttunpack).
 *
 * === READBACK RULE (feeds the packing layer) ===
 * When serializing an NTT-domain coefficient before canonicalization:
 *   - q15361 (SIGNED): a lane is a TRUE signed int16 (lazy outputs reach
 * ~2q); read it as smod(v) = ((int)v % q + q) % q.  (uint16_t)v % q is
 * WRONG.
 *   - q61441/q59393 (UNSIGNED valley): read the raw 16 bits UNSIGNED,
 *     (uint16_t)x; values in (2^15,q) look negative as int16 but 2^16 !=0
 * mod q so reducing the SIGNED value mod q is wrong ("the q>2^15 trap").
 * In practice pk/sk/com bytes are produced AFTER poly_invntt_tomont has
 * canonicalized to [0,q) (and after freeze/reduce_mod_2q in the scheme
 * layer), so the per-config readback is internal to this NTT layer; the
 * poly16-> bytes packer is uniform on [0,q).
 *
 * === POINTWISE PRE-REDUCE (NON-uniform contract) ===
 * Do NOT pre-freeze pointwise inputs in the shim.  The unsigned
 * pointwise_avx assumes already-[0,q) inputs (which poly_ntt for the
 * unsigned configs guarantees).  The signed pointwise_avx red16s BOTH
 * inputs ITSELF (its lazy NTT outputs reach ~2q).
 * poly_pointwise_montgomery always takes the backend-native poly_ntt
 * output of both operands verbatim.  Both operands pass the SAME poly_ntt,
 * so they share the permutation and pointwise needs no reorder.
 */
#ifndef SHUTTLE_POLY_NTT_H
#define SHUTTLE_POLY_NTT_H

#include "params.h" /* SHUTTLE_MODE, N */
/* Disable with -DDISABLE_NAMESPACE=1. */
#include "namespace.h"
#include "poly.h"   /* poly16, the shim prototypes (poly_ntt, ...) */

/* Pull in the per-mode vendored scalar header (it defines NTT_Q/NTT_N and
 * the s<n>_-prefixed function/const declarations for ALL backends of this
 * set). */
#if SHUTTLE_MODE == 128
#    include "ntt/q15361n256/ntt_ref.h"
#    define SHUTTLE_NTT_SIGNED \
        1 /* q < 2^15: signed lazy path + canonicalize */
#elif SHUTTLE_MODE == 256
#    include "ntt/q61441n512/ntt_ref.h"
#    define SHUTTLE_NTT_SIGNED 0
#elif SHUTTLE_MODE == 512
#    include "ntt/q59393n1024/ntt_ref.h"
#    define SHUTTLE_NTT_SIGNED 0
#else
#    error "Unsupported SHUTTLE_MODE (expected 128, 256, or 512)"
#endif

_Static_assert(NTT_N == N, "vendored NTT degree must match params.h N");
_Static_assert(NTT_Q == Q, "vendored NTT modulus must match params.h Q");

/* The shim prototypes (poly_ntt / poly_invntt_tomont /
 * poly_pointwise_montgomery / poly_ntt_canonical / poly_ntt_import) are
 * declared in poly.h (which owns the types + these shims).  The bodies
 * live in ref/poly_ntt.c (scalar) and the avx2/avx512 forks (behind
 * USE_AVX2_NTT / USE_AVX512_NTT). */

#endif /* SHUTTLE_POLY_NTT_H */
