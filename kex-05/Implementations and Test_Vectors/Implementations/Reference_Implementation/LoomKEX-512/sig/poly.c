/*
 * poly.c -- scheme-domain polynomial arithmetic helpers + the fused
 * public-key unpack.
 *
 * These wrap the per-coefficient reduce.{c,h} primitives over the signed
 * int32 `poly` type.  Every loop is fixed-length (N) and the underlying
 * reduce32 / caddq / freeze are division-free, masked-correction Barrett
 * (constant-time): so the helpers contain NO data-dependent branch, index,
 * shift count, or hardware divide -- they run on possibly-secret s/e'/NTT
 * residues and must satisfy the constant-time scan.
 *
 * The NTT shim entry points (poly_ntt / poly_invntt_tomont /
 * poly_pointwise_montgomery / poly_ntt_canonical / poly_ntt_import) live
 * in poly_ntt.c, not here.
 */
#include "poly.h"

#include <stddef.h>
#include <stdint.h>

#include "params.h"
#include "reduce.h"

/*************************************************
 * Name:        poly_reduce
 *
 * Description: Coefficient-wise reduce32: each coeff -> (-q, q).
 **************************************************/
void poly_reduce(poly *a)
{
    unsigned i;
    for (i = 0; i < N; ++i)
        a->coeffs[i] = reduce32(a->coeffs[i]);
}

/*************************************************
 * Name:        poly_caddq
 *
 * Description: Coefficient-wise conditional add q (sign-bit mask).
 *              Precondition: each coeff already in (-q, q); output [0, q).
 **************************************************/
void poly_caddq(poly *a)
{
    unsigned i;
    for (i = 0; i < N; ++i)
        a->coeffs[i] = caddq(a->coeffs[i]);
}

/*************************************************
 * Name:        poly_freeze
 *
 * Description: Coefficient-wise standard representative in [0, q)
 *              (reduce32 then caddq == freeze).
 **************************************************/
void poly_freeze(poly *a)
{
    unsigned i;
    for (i = 0; i < N; ++i)
        a->coeffs[i] = freeze(a->coeffs[i]);
}

/*************************************************
 * Name:        poly_add
 *
 * Description: c = a + b coefficient-wise (no modular reduction).
 **************************************************/
void poly_add(poly *c, const poly *a, const poly *b)
{
    unsigned i;
    for (i = 0; i < N; ++i)
        c->coeffs[i] = a->coeffs[i] + b->coeffs[i];
}

/*************************************************
 * Name:        poly_sub
 *
 * Description: c = a - b coefficient-wise (no modular reduction).
 **************************************************/
void poly_sub(poly *c, const poly *a, const poly *b)
{
    unsigned i;
    for (i = 0; i < N; ++i)
        c->coeffs[i] = a->coeffs[i] - b->coeffs[i];
}

/*************************************************
 * Name:        poly16_add
 *
 * Description: c = a + b in the canonical [0,q) NTT domain (addm16).
 **************************************************/
void poly16_add(poly16 *c, const poly16 *a, const poly16 *b)
{
    unsigned i;
    for (i = 0; i < N; ++i)
        c->coeffs[i] = addm16(a->coeffs[i], b->coeffs[i]);
}

/*************************************************
 * Name:        poly_sqnorm
 *
 * Description: Centered squared L2 norm of a, accumulated in int64.
 *              Each coeff is centered mod q to the representative in
 *              (-q/2, q/2] before squaring (so the result is the scheme's
 *              geometric norm for ANY input representative).  int64 cannot
 *              overflow: N*(q/2)^2 <= 1024*30721^2 ~ 9.66e11 << 2^63.
 *
 *              The centering is done division-free: reduce32 lands the
 *coeff in (-q, q), then a single masked add/subtract of q folds it into
 *the centered window.  No branch on the coeff value.
 **************************************************/
int64_t poly_sqnorm(const poly *a)
{
    int64_t acc = 0;
    unsigned i;
    for (i = 0; i < N; ++i) {
        /* r in (-q, q) */
        int32_t r = reduce32(a->coeffs[i]);
        /* Fold into the centered window (-q/2, q/2].  For the centered
         * representative we want r in (-(q-1)/2 .. (q-1)/2] (q is odd for
         * all three sets).  If r > q/2 subtract q; if r <= -q/2 add q.
         * Branchless via sign-extended comparisons. */
        int32_t hi = (int32_t)((Q - 1) / 2); /* q is odd: (q-1)/2 */
        /* if r > hi : r -= q  (mask = -1 when r > hi) */
        int32_t mgt = -(int32_t)(r > hi);
        r -= mgt & Q;
        /* if r < -hi : r += q  (mask = -1 when r < -hi) */
        int32_t mlt = -(int32_t)(r < -hi);
        r += mlt & Q;
        acc += (int64_t)r * (int64_t)r;
    }
    return acc;
}

/*
 * unpack_pk_bn -- fused bitunpack + alpha_b rescale (FAST trusted path).
 *
 * Reads EM packed pk polynomials (each POLYPK_PACKEDBYTES bytes, d_b-bit
 * LSB-first fields) and writes EM poly's holding b = alpha_b*b1 in [0, q).
 * See poly.h for the full SHUTTLE-divergence rationale: the *alpha_b
 * rescale alone canonicalizes (no cond-subtract of q, no nttunpack).
 * Trusted input -> NO b1 < ceil(q/alpha_b) range check (that guard lives
 * in pk_decode in packing.c).
 *
 * The bit reader uses the SAME data-independent LSB-first byte-schedule
 * accumulator as bytes_to_poly (packing.c): no per-bit branch, no
 * data-dependent shift count -> KAT-stable, constant-time.
 */
void unpack_pk_bn(poly b[EM], const uint8_t *packed)
{
    /* alpha_b is a power of two for all sets (2/2/4): rescale is a left
     * shift by log2(alpha_b).  Asserted reproducible. */
    _Static_assert((ALPHA_B & (ALPHA_B - 1)) == 0,
                   "alpha_b must be a power of two for the shift rescale");
    const unsigned shift = (ALPHA_B == 2) ? 1u : 2u; /* log2(alpha_b) */
    const uint32_t mask = ((uint32_t)1u << DB_BITS) - 1u;

    unsigned p, i;
    for (p = 0; p < (unsigned)EM; ++p) {
        const uint8_t *in = packed + (size_t)p * POLYPK_PACKEDBYTES;
        uint32_t acc = 0; /* bit accumulator (LSB-first)              */
        unsigned accbits = 0;
        unsigned bytepos = 0;
        for (i = 0; i < N; ++i) {
            /* Refill the accumulator until it holds >= DB_BITS bits.
             * DB_BITS <= 15 and acc is 32-bit, so accbits < 8 before each
             * refill guarantees no overflow.  Byte schedule depends only
             * on the loop counters, never on the data. */
            while (accbits < DB_BITS) {
                acc |= (uint32_t)in[bytepos++] << accbits;
                accbits += 8;
            }
            uint32_t b1 = acc & mask; /* d_b-bit field, in [0,2^d_b) */
            acc >>= DB_BITS;
            accbits -= DB_BITS;
            /* b = alpha_b * b1, lands in [0,q) on its own (trusted). */
            b[p].coeffs[i] = (int32_t)(b1 << shift);
        }
    }
}
