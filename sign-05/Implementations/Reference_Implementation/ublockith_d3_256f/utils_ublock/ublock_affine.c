/*
 * uBlock SubDWord affine transformation helpers for the VOLE algebraic path.
 *
 * Provides two variants of the uBlock nibble S-box:
 *
 *   ublock_subword_nibble(uint8_t x) -> uint8_t
 *     Evaluates the S-box on a single nibble via ANF (Boolean operations).
 *     Equivalent to UBLOCK_SBOX[x & 0xf].
 *
 *   ublock_subdword_nibble_tag(bf256_t y_tag[16][4], const bf256_t x_tag[16][4])
 *     VOLE tag-space variant: applies the same ANF to 16 committed nibbles
 *     using GF(2^256) arithmetic (bf256_add = XOR, bf256_mul = field mul).
 *     Constants (xor 1) are NOT added here; the caller adds ConstantToVOLE(1)
 *     per bit as required by the UBLOCKITH VOLE commitment protocol.
 *
 * ANF (x0 = LSB, verified exhaustively in standalone tests):
 *   y3 = x1 xor x2 xor x3 xor x1*x2
 *   y2 = 1 xor x1 xor x0*x1 xor x2 xor x1*x2*x3
 *   y1 = 1 xor x0 xor x1 xor x0*x1 xor x0*x2 xor x0*x1*x2 xor x0*x3 xor x2*x3
 *   y0 = 1 xor x0 xor x2*x3
 *
 * Structure mirrors the affine helper style used by the original design path.
 */

#include "ublock.h"
#include "fields.h"
#include <stdint.h>

/* uBlock linear T on one nibble over GF(2^4), m(t)=t^4+t+1.
 * For y=(y0,y1,y2,y3): z0=y1, z1=y2, z2=y3 xor y0, z3=y0. */
uint8_t ublock_T_nibble(uint8_t y)
{
    const uint8_t y0 = (y >> 0) & 1;
    const uint8_t y1 = (y >> 1) & 1;
    const uint8_t y2 = (y >> 2) & 1;
    const uint8_t y3 = (y >> 3) & 1;

    const uint8_t z0 = y1;
    const uint8_t z1 = y2;
    const uint8_t z2 = y3 ^ y0;
    const uint8_t z3 = y0;

    return (uint8_t)((z3 << 3) | (z2 << 2) | (z1 << 1) | z0);
}

/* Tag path for linear T: same linear map on tags. */
void ublock_T_nibble_tag(bf256_t *z_tag, const bf256_t *y_tag)
{
    z_tag[0] = y_tag[1];
    z_tag[1] = y_tag[2];
    z_tag[2] = bf256_add(y_tag[3], y_tag[0]);
    z_tag[3] = y_tag[0];
}

/* Key path for linear T: same linear map on keys (delta unused). */
void ublock_T_nibble_key(bf256_t *z_key, const bf256_t *y_key, const bf256_t delta)
{
    (void)delta;
    z_key[0] = y_key[1];
    z_key[1] = y_key[2];
    z_key[2] = bf256_add(y_key[3], y_key[0]);
    z_key[3] = y_key[0];
}

/* ublock_subdword_nibble: scalar S-box via ANF.
 *   x : input nibble (bits 3..0);
 *   returns: S-box output nibble (bits 3..0). */

uint8_t ublock_subword_nibble(uint8_t x)
{
    uint8_t x0 = (x >> 0) & 1;
    uint8_t x1 = (x >> 1) & 1;
    uint8_t x2 = (x >> 2) & 1;
    uint8_t x3 = (x >> 3) & 1;

    /* y3 = x1 ⊕ x2 ⊕ x3 ⊕ x1·x2 */
    uint8_t y3 = x1 ^ x2 ^ x3 ^ (x1 & x2);

    /* y2 = 1 ⊕ x1 ⊕ x0·x1 ⊕ x2 ⊕ x1·x2·x3 */
    uint8_t y2 = 1 ^ x1 ^ (x0 & x1) ^ x2 ^ (x1 & x2 & x3);

    /* y1 = 1 ⊕ x0 ⊕ x1 ⊕ x0·x1 ⊕ x0·x2 ⊕ x0·x1·x2 ⊕ x0·x3 ⊕ x2·x3 */
    uint8_t y1 = 1 ^ x0 ^ x1 ^ (x0 & x1) ^ (x0 & x2)
                   ^ (x0 & x1 & x2) ^ (x0 & x3) ^ (x2 & x3);

    /* y0 = 1 ⊕ x0 ⊕ x2·x3 */
    uint8_t y0 = 1 ^ x0 ^ (x2 & x3);

    return (uint8_t)((y3 << 3) | (y2 << 2) | (y1 << 1) | y0);
}

/* inverse S-box: scalar evaluation via ANF */
uint8_t ublock_invsubword_nibble(uint8_t y)
{
    uint8_t y0 = (y >> 0) & 1;
    uint8_t y1 = (y >> 1) & 1;
    uint8_t y2 = (y >> 2) & 1;
    uint8_t y3 = (y >> 3) & 1;

    uint8_t z0 = (y0 & y1) ^ y2 ^ y3 ^ (y0 & y3) ^ (y2 & y3) ^ (y0 & y1 & y3);
    uint8_t z1 = y0 ^ y1 ^ y3 ^ (y0 & y3);
    uint8_t z2 = 1 ^ y0 ^ y2 ^ (y0 & y1);
    uint8_t z3 = 1 ^ y2 ^ y3 ^ (y0 & y2) ^ (y1 & y2) ^ (y2 & y3) ^ (y0 & y2 & y3);

    return (uint8_t)((z3 << 3) | (z2 << 2) | (z1 << 1) | z0);
}

/* ublock_subword_nibble_tag: VOLE tag-space SubWord.
 *   q1 = w1 Delta + v1, q2 = w2 Delta + v2, q3 = w3 Delta + v3
 *   then we have 
 *   q1q2q3 = w1w2w3 Delta^3 + (w1w2v3 + w1w3v2 + w2w3v1) Delta^2 + (w1v2v3 + w2v1v3 + w3v1v2) Delta + v1v2v3
 *   Given q4 = w4 Delta + v4, we have 
 *   To compute q1q2q3 + q4, we need to compute q1q2q3 + Delta^2 q4 = 
 *   (w1w2w3 + w4) Delta^3 + (w1w2v3 + w1w3v2 + w2w3v1 + w4v4) Delta^2 + (w1v2v3 + w2v1v3 + w3v1v2 + w4v4) Delta + (v1v2v3 + v4)
 *   For constant 1, its tag is zero, e.g., ConstantToVOLE(1) = 1*Delta + 0
 *   The target is to find deg-2, deg-1. and deg-0 tags for the output nibble, y0,..y3
 * */

void ublock_subword_nibble_tag(bf256_t *y_tag_deg0, bf256_t *y_tag_deg1, bf256_t *y_tag_deg2,
                             const bf256_t *x_tag, const uint8_t x)
{
        const bf256_t v0 = x_tag[0];
        const bf256_t v1 = x_tag[1];
        const bf256_t v2 = x_tag[2];
        const bf256_t v3 = x_tag[3];

        const uint8_t w0 = (x >> 0) & 1;
        const uint8_t w1 = (x >> 1) & 1;
        const uint8_t w2 = (x >> 2) & 1;
        const uint8_t w3 = (x >> 3) & 1;

        const bf256_t v0v1   = bf256_mul(v0, v1);
        const bf256_t v0v2   = bf256_mul(v0, v2);
        const bf256_t v0v3   = bf256_mul(v0, v3);
        const bf256_t v1v2   = bf256_mul(v1, v2);
        const bf256_t v1v3   = bf256_mul(v1, v3);
        const bf256_t v2v3   = bf256_mul(v2, v3);
        const bf256_t v0v1v2 = bf256_mul(v0v1, v2);
        const bf256_t v1v2v3 = bf256_mul(v1v2, v3);

        /* y3 = x1 ⊕ x2 ⊕ x3 ⊕ x1·x2 */
        y_tag_deg2[3] = bf256_add(
            bf256_add(bf256_add(v1, v2), v3),
            bf256_add(bf256_mul_bit(v2, w1), bf256_mul_bit(v1, w2)));
        y_tag_deg1[3] = v1v2;
        y_tag_deg0[3] = bf256_zero();

        /* y2 = 1 ⊕ x1 ⊕ x0·x1 ⊕ x2 ⊕ x1·x2·x3 */
        y_tag_deg2[2] = bf256_add(
            bf256_add(
                bf256_add(v1, v2),
                bf256_add(bf256_mul_bit(v1, w0), bf256_mul_bit(v0, w1))),
            bf256_add(
                bf256_add(bf256_mul_bit(v3, (uint8_t)(w1 & w2)),
                          bf256_mul_bit(v2, (uint8_t)(w1 & w3))),
                bf256_mul_bit(v1, (uint8_t)(w2 & w3))));
        y_tag_deg1[2] = bf256_add(
            v0v1,
            bf256_add(
                bf256_add(bf256_mul_bit(v2v3, w1), bf256_mul_bit(v1v3, w2)),
                bf256_mul_bit(v1v2, w3)));
        y_tag_deg0[2] = v1v2v3;

        /* y1 = 1 ⊕ x0 ⊕ x1 ⊕ x0·x1 ⊕ x0·x2 ⊕ x0·x1·x2 ⊕ x0·x3 ⊕ x2·x3 */
        y_tag_deg2[1] = bf256_add(
            bf256_add(
                bf256_add(v0, v1),
                bf256_add(
                    bf256_add(bf256_mul_bit(v1, w0), bf256_mul_bit(v0, w1)),
                    bf256_add(bf256_mul_bit(v2, w0), bf256_mul_bit(v0, w2)))),
            bf256_add(
                bf256_add(
                    bf256_add(bf256_mul_bit(v2, (uint8_t)(w0 & w1)),
                              bf256_mul_bit(v1, (uint8_t)(w0 & w2))),
                    bf256_mul_bit(v0, (uint8_t)(w1 & w2))),
                bf256_add(
                    bf256_add(bf256_mul_bit(v3, w0), bf256_mul_bit(v0, w3)),
                    bf256_add(bf256_mul_bit(v3, w2), bf256_mul_bit(v2, w3)))));
        y_tag_deg1[1] = bf256_add(
            bf256_add(
                bf256_add(v0v1, v0v2),
                bf256_add(v0v3, v2v3)),
            bf256_add(
                bf256_add(bf256_mul_bit(v1v2, w0), bf256_mul_bit(v0v2, w1)),
                bf256_mul_bit(v0v1, w2)));
        y_tag_deg0[1] = v0v1v2;

        /* y0 = 1 ⊕ x0 ⊕ x2·x3 */
        y_tag_deg2[0] = bf256_add(
            v0,
            bf256_add(bf256_mul_bit(v3, w2), bf256_mul_bit(v2, w3)));
        y_tag_deg1[0] = v2v3;
        y_tag_deg0[0] = bf256_zero();

}

/* inverse S-box: VOLE tag-space (deg0/1/2), constants omitted (tag(const)=0) */
void ublock_invsubword_nibble_tag(bf256_t *z_tag_deg0, bf256_t *z_tag_deg1, bf256_t *z_tag_deg2,
                                  const bf256_t *y_tag, const uint8_t y)
{
    const bf256_t v0 = y_tag[0];
    const bf256_t v1 = y_tag[1];
    const bf256_t v2 = y_tag[2];
    const bf256_t v3 = y_tag[3];

    const uint8_t w0 = (y >> 0) & 1;
    const uint8_t w1 = (y >> 1) & 1;
    const uint8_t w2 = (y >> 2) & 1;
    const uint8_t w3 = (y >> 3) & 1;

    /* Reuse pair/triple products across all output bits. */
    const bf256_t v0v1 = bf256_mul(v0, v1);
    const bf256_t v0v2 = bf256_mul(v0, v2);
    const bf256_t v0v3 = bf256_mul(v0, v3);
    const bf256_t v1v2 = bf256_mul(v1, v2);
    const bf256_t v1v3 = bf256_mul(v1, v3);
    const bf256_t v2v3 = bf256_mul(v2, v3);
    const bf256_t v0v1v3 = bf256_mul(v0v1, v3);
    const bf256_t v0v2v3 = bf256_mul(v0v2, v3);

    /* z0 = y0 y1 ⊕ y2 ⊕ y3 ⊕ y0 y3 ⊕ y2 y3 ⊕ y0 y1 y3 */
    bf256_t z0_d2 = bf256_add(v2, v3);
    z0_d2 = bf256_add(z0_d2, bf256_add(bf256_mul_bit(v1, w0), bf256_mul_bit(v0, w1)));      /* y0y1 */
    z0_d2 = bf256_add(z0_d2, bf256_add(bf256_mul_bit(v3, w0), bf256_mul_bit(v0, w3)));      /* y0y3 */
    z0_d2 = bf256_add(z0_d2, bf256_add(bf256_mul_bit(v3, w2), bf256_mul_bit(v2, w3)));      /* y2y3 */
    z0_d2 = bf256_add(z0_d2, bf256_add(
        bf256_add(bf256_mul_bit(v1, (uint8_t)(w0 & w3)), bf256_mul_bit(v0, (uint8_t)(w1 & w3))),
        bf256_mul_bit(v3, (uint8_t)(w0 & w1))));                                            /* cubic Δ^2 part */

    bf256_t z0_d1 = bf256_add(v0v1, v0v3);
    z0_d1 = bf256_add(z0_d1, v2v3);
    /* simpler cubic d1: w0 v1 v3 + w1 v0 v3 + w3 v0 v1 */
    z0_d1 = bf256_add(z0_d1, bf256_add(
        bf256_mul_bit(v1v3, w0),
        bf256_add(bf256_mul_bit(v0v3, w1), bf256_mul_bit(v0v1, w3))));

    bf256_t z0_d0 = v0v1v3;

    z_tag_deg2[0] = z0_d2;
    z_tag_deg1[0] = z0_d1;
    z_tag_deg0[0] = z0_d0;

    /* z1 = y0 ⊕ y1 ⊕ y3 ⊕ y0 y3 */
    bf256_t z1_d2 = bf256_add(bf256_add(v0, v1), v3);
    z1_d2 = bf256_add(z1_d2, bf256_add(bf256_mul_bit(v3, w0), bf256_mul_bit(v0, w3)));
    bf256_t z1_d1 = v0v3;
    z_tag_deg2[1] = z1_d2;
    z_tag_deg1[1] = z1_d1;
    z_tag_deg0[1] = bf256_zero();

    /* z2 = 1 ⊕ y0 ⊕ y2 ⊕ y0 y1 (const dropped) */
    bf256_t z2_d2 = bf256_add(v0, v2);
    z2_d2 = bf256_add(z2_d2, bf256_add(bf256_mul_bit(v1, w0), bf256_mul_bit(v0, w1)));
    bf256_t z2_d1 = v0v1;
    z_tag_deg2[2] = z2_d2;
    z_tag_deg1[2] = z2_d1;
    z_tag_deg0[2] = bf256_zero();

    /* z3 = 1 ⊕ y2 ⊕ y3 ⊕ y0 y2 ⊕ y1 y2 ⊕ y2 y3 ⊕ y0 y2 y3 (const dropped) */
    bf256_t z3_d2 = bf256_add(v2, v3);
    z3_d2 = bf256_add(z3_d2, bf256_add(bf256_mul_bit(v2, w0), bf256_mul_bit(v0, w2))); /* y0y2 */
    z3_d2 = bf256_add(z3_d2, bf256_add(bf256_mul_bit(v2, w1), bf256_mul_bit(v1, w2))); /* y1y2 */
    z3_d2 = bf256_add(z3_d2, bf256_add(bf256_mul_bit(v3, w2), bf256_mul_bit(v2, w3))); /* y2y3 */
    /* cubic Δ^2 part: w0 w2 v3 + w0 w3 v2 + w2 w3 v0 */
    z3_d2 = bf256_add(z3_d2, bf256_add(
        bf256_add(bf256_mul_bit(v3, (uint8_t)(w0 & w2)), bf256_mul_bit(v2, (uint8_t)(w0 & w3))),
        bf256_mul_bit(v0, (uint8_t)(w2 & w3))));

    bf256_t z3_d1 = bf256_add(v0v2, v1v2);
    z3_d1 = bf256_add(z3_d1, v2v3);
    /* cubic Δ^1: w0 v2 v3 + w2 v0 v3 + w3 v0 v2 */
    z3_d1 = bf256_add(z3_d1, bf256_add(
        bf256_add(bf256_mul_bit(v2v3, w0), bf256_mul_bit(v0v3, w2)),
        bf256_mul_bit(v0v2, w3)));

    bf256_t z3_d0 = v0v2v3;

    z_tag_deg2[3] = z3_d2;
    z_tag_deg1[3] = z3_d1;
    z_tag_deg0[3] = z3_d0;
}

/* inverse S-box: verifier key path with degree lifted to 3 */
void ublock_invsubword_nibble_key_pows(bf256_t *z_key, const bf256_t *y_key,
                                       const bf256_t delta, const bf256_t delta2,
                                       const bf256_t delta3)
{
    const bf256_t q0 = y_key[0];
    const bf256_t q1 = y_key[1];
    const bf256_t q2 = y_key[2];
    const bf256_t q3 = y_key[3];

    const bf256_t q0q1 = bf256_mul(q0, q1);
    const bf256_t q0q2 = bf256_mul(q0, q2);
    const bf256_t q0q3 = bf256_mul(q0, q3);
    const bf256_t q1q2 = bf256_mul(q1, q2);
    const bf256_t q2q3 = bf256_mul(q2, q3);
    const bf256_t q0q1q3 = bf256_mul(q0q1, q3);
    const bf256_t q0q2q3 = bf256_mul(q0q2, q3);

    /* z0 = y0 y1 ⊕ y2 ⊕ y3 ⊕ y0 y3 ⊕ y2 y3 ⊕ y0 y1 y3 */
    bf256_t z0 = bf256_mul(delta2, bf256_add(q2, q3));
    z0 = bf256_add(z0, bf256_mul(delta, bf256_add(q0q1, q0q3)));
    z0 = bf256_add(z0, bf256_mul(delta, q2q3));
    z0 = bf256_add(z0, q0q1q3);

    /* z1 = y0 ⊕ y1 ⊕ y3 ⊕ y0 y3 */
    bf256_t z1 = bf256_mul(delta2, bf256_add(bf256_add(q0, q1), q3));
    z1 = bf256_add(z1, bf256_mul(delta, q0q3));

    /* z2 = 1 ⊕ y0 ⊕ y2 ⊕ y0 y1 */
    bf256_t z2 = delta3;
    z2 = bf256_add(z2, bf256_mul(delta2, bf256_add(q0, q2)));
    z2 = bf256_add(z2, bf256_mul(delta, q0q1));

    /* z3 = 1 ⊕ y2 ⊕ y3 ⊕ y0 y2 ⊕ y1 y2 ⊕ y2 y3 ⊕ y0 y2 y3 */
    bf256_t z3 = delta3;
    z3 = bf256_add(z3, bf256_mul(delta2, bf256_add(q2, q3)));
    z3 = bf256_add(z3, bf256_mul(delta, bf256_add(
        bf256_add(q0q2, q1q2),
        q2q3)));
    z3 = bf256_add(z3, q0q2q3);

    z_key[0] = z0;
    z_key[1] = z1;
    z_key[2] = z2;
    z_key[3] = z3;
}

void ublock_invsubword_nibble_key(bf256_t *z_key, const bf256_t *y_key, const bf256_t delta)
{
    const bf256_t delta2 = bf256_mul(delta, delta);
    const bf256_t delta3 = bf256_mul(delta2, delta);
    ublock_invsubword_nibble_key_pows(z_key, y_key, delta, delta2, delta3);
}

/*  An example:
 *   q1 = w1 Delta + v1, q2 = w2 Delta + v2, q3 = w3 Delta + v3
 *   then we have 
 *   q1q2q3 = w1w2w3 Delta^3 + (w1w2v3 + w1w3v2 + w2w3v1) Delta^2 + (w1v2v3 + w2v1v3 + w3v1v2) Delta + v1v2v3
 *   Given q4 = w4 Delta + v4, we have 
 *   To compute q1q2q3 + q4, we need to compute q1q2q3 + Delta^2 q4 
 *   For constant 1, its key is 1 \cdot Delta^i
*/

void ublock_subword_nibble_key_pows(bf256_t *y_key, const bf256_t *x_key,
                                    const bf256_t delta, const bf256_t delta2,
                                    const bf256_t delta3)
{
    const bf256_t q0 = x_key[0];
    const bf256_t q1 = x_key[1];
    const bf256_t q2 = x_key[2];
    const bf256_t q3 = x_key[3];

    const bf256_t q0q1 = bf256_mul(q0, q1);
    const bf256_t q0q2 = bf256_mul(q0, q2);
    const bf256_t q0q3 = bf256_mul(q0, q3);
    const bf256_t q1q2 = bf256_mul(q1, q2);
    const bf256_t q2q3 = bf256_mul(q2, q3);
    const bf256_t q1q2q3 = bf256_mul(q1q2, q3);
    const bf256_t q0q1q2 = bf256_mul(q0q1, q2);

    /* y3 = x1 ⊕ x2 ⊕ x3 ⊕ x1·x2
     * lift: linear * Δ^2, quadratic * Δ */
    bf256_t y3 = bf256_mul(delta2, bf256_add(bf256_add(q1, q2), q3));
    y3 = bf256_add(y3, bf256_mul(delta, q1q2));

    /* y2 = 1 ⊕ x1 ⊕ x0·x1 ⊕ x2 ⊕ x1·x2·x3
     * lift: const -> Δ^3, linear -> Δ^2, quadratic -> Δ, cubic unchanged */
    bf256_t y2 = delta3;
    y2 = bf256_add(y2, bf256_mul(delta2, bf256_add(q1, q2)));
    y2 = bf256_add(y2, bf256_mul(delta, q0q1));
    y2 = bf256_add(y2, q1q2q3);

    /* y1 = 1 ⊕ x0 ⊕ x1 ⊕ x0·x1 ⊕ x0·x2 ⊕ x0·x1·x2 ⊕ x0·x3 ⊕ x2·x3
     * lift: const -> Δ^3, linear -> Δ^2, quadratic -> Δ, cubic unchanged */
    bf256_t y1 = delta3;
    y1 = bf256_add(y1, bf256_mul(delta2, bf256_add(q0, q1)));
    y1 = bf256_add(y1, bf256_mul(delta, bf256_add(
        bf256_add(q0q1, q0q2),
        bf256_add(q0q3, q2q3))));
    y1 = bf256_add(y1, q0q1q2);

    /* y0 = 1 ⊕ x0 ⊕ x2·x3
     * lift: const -> Δ^3, linear -> Δ^2, quadratic -> Δ */
    bf256_t y0 = delta3;
    y0 = bf256_add(y0, bf256_mul(delta2, q0));
    y0 = bf256_add(y0, bf256_mul(delta, q2q3));

    /* Store as (y0, y1, y2, y3) matching bit order in scalar S-box helper */
    y_key[0] = y0;
    y_key[1] = y1;
    y_key[2] = y2;
    y_key[3] = y3;
}

void ublock_subword_nibble_key(bf256_t *y_key, const bf256_t *x_key, const bf256_t delta)
{
    const bf256_t delta2 = bf256_mul(delta, delta);
    const bf256_t delta3 = bf256_mul(delta2, delta);
    ublock_subword_nibble_key_pows(y_key, x_key, delta, delta2, delta3);
}
