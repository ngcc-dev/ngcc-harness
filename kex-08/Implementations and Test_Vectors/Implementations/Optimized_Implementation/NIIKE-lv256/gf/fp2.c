// SPDX-FileCopyrightText: Copyright 2023 the SQIsign team. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

/*
 * This file is derived from fp2.c in the SQIsign (version 1.0) 
 * project (https://github.com/SQIsign/the-sqisign/tree/nist-v1),
 * which is licensed under the Apache-2.0 license.
 *
 * Modified to adapt to OSIDH-LD
 */

#include "fp2.h"
#include <stdio.h>
#include "encode_sizes.h"

// extern const digit_t R[NWORDS_FIELD];

/* Arithmetic modulo X^2 + 1 */
const fp2_t fp2_1 = {{0x707d3246fdc8e382, 0x74fc75c5abeecd3f, 0xd8c63c99c8ca2803, 0x766b7cf60deff1bc, 0xc83c9b3d8525f891, 0x0208938b30a684e0, 0xbc77764796d422f5, 0x3aa1910d92a44138}, {0, 0, 0, 0, 0, 0, 0, 0}};

void fp2_set(fp2_t* x, const digit_t val)
{
    fp_set(x->re, val);
    fp_set(x->im, 0);
}

void fp2_setone(fp2_t *x)
{
    fp_mont_setone(&(x->re));
    fp_set(&(x->im), 0);
}

bool fp2_is_zero(const fp2_t* a)
{ // Is a GF(p^2) element zero?
  // Returns 1 (true) if a=0, 0 (false) otherwise

    return fp_is_zero(a->re) & fp_is_zero(a->im);
}

uint32_t fp2_is_zero_mask(const fp2_t *a)
{
    return fp_is_zero_mask(a->re) & fp_is_zero_mask(a->im);
}

bool fp2_is_equal(const fp2_t* a, const fp2_t* b)
{ // Compare two GF(p^2) elements in constant time
  // Returns 1 (true) if a=b, 0 (false) otherwise

    return fp_is_equal(a->re, b->re) & fp_is_equal(a->im, b->im);
}

void fp2_copy(fp2_t* x, const fp2_t* y)
{
    fp_copy(x->re, y->re);
    fp_copy(x->im, y->im);
}

fp2_t fp2_non_residue()
{ // 2 + i is a quadratic non-residue for p1913
    fp_t one = {0};
    fp2_t res;

    one[0] = 1;
    fp_tomont(one, one);
    fp_add(res.re, one, one);
    fp_copy(res.im, one);
    return res;
}

void fp2_add(fp2_t* x, const fp2_t* y, const fp2_t* z)
{
    fp_add(x->re, y->re, z->re);
    fp_add(x->im, y->im, z->im);
}

void fp2_sub(fp2_t* x, const fp2_t* y, const fp2_t* z)
{
    fp_sub(x->re, y->re, z->re);
    fp_sub(x->im, y->im, z->im);
}

void fp2_neg(fp2_t* x, const fp2_t* y)
{
    fp_neg(x->re, y->re);
    fp_neg(x->im, y->im);
}

void fp2_mul(fp2_t* x, const fp2_t* y, const fp2_t* z)
{
    fp_t t0, t1;

    fp_add(t0, y->re, y->im);
    fp_add(t1, z->re, z->im);
    fp_mul(t0, t0, t1);
    fp_mul(t1, y->im, z->im);
    fp_mul(x->re, y->re, z->re);
    fp_sub(x->im, t0, t1);
    fp_sub(x->im, x->im, x->re);
    fp_sub(x->re, x->re, t1);
}

void fp2_sqr(fp2_t* x, const fp2_t* y)
{
    fp_t sum, diff;

    fp_add(sum, y->re, y->im);
    fp_sub(diff, y->re, y->im);
    fp_mul(x->im, y->re, y->im);
    fp_add(x->im, x->im, x->im);
    fp_mul(x->re, sum, diff);
}

void fp2_inv(fp2_t* x)
{
    fp_t t0, t1;

    fp_sqr(t0, x->re);
    fp_sqr(t1, x->im);
    fp_add(t0, t0, t1);
    fp_inv(t0);
    fp_mul(x->re, x->re, t0);
    fp_mul(x->im, x->im, t0);
    fp_neg(x->im, x->im);
}

bool fp2_is_square(const fp2_t* x)
{
    fp_t t0, t1;

    fp_sqr(t0, x->re);
    fp_sqr(t1, x->im);
    fp_add(t0, t0, t1);

    return fp_is_square(t0);
}

void fp2_frob(fp2_t* x, const fp2_t* y)
{
    memcpy((digit_t*)x->re, (digit_t*)y->re, NWORDS_FIELD*RADIX/8);
    fp_neg(x->im, y->im);
}

void fp2_tomont(fp2_t* x, const fp2_t* y)
{ 
    fp_tomont(x->re, y->re);
    fp_tomont(x->im, y->im);
}

void fp2_frommont(fp2_t* x, const fp2_t* y)
{
    fp_frommont(x->re, y->re);
    fp_frommont(x->im, y->im);
}

// NOTE: old, non-constant-time implementation. Could be optimized
// void fp2_sqrt(fp2_t* x)
// {
//     fp_t sdelta, re, tmp1, tmp2, inv2, im;

//     if (fp_is_zero(x->im)) {
//         if (fp_is_square(x->re)) {
//             fp_sqrt(x->re);
//             return;
//         } else {
//             fp_neg(x->im, x->re);
//             fp_sqrt(x->im);
//             fp_set(x->re, 0);
//             return;
//         }
//     }

//     // sdelta = sqrt(re^2 + im^2)
//     fp_sqr(sdelta, x->re);
//     fp_sqr(tmp1, x->im);
//     fp_add(sdelta, sdelta, tmp1);
//     fp_sqrt(sdelta);

//     fp_set(inv2, 2);
//     fp_tomont(inv2, inv2);     // inv2 <- 2
//     fp_inv(inv2);
//     fp_add(re, x->re, sdelta);
//     fp_mul(re, re, inv2);
//     memcpy((digit_t*)tmp2, (digit_t*)re, NWORDS_FIELD*RADIX/8);

//     if (!fp_is_square(tmp2)) {
//         fp_sub(re, x->re, sdelta);
//         fp_mul(re, re, inv2);
//     }

//     fp_sqrt(re);
//     memcpy((digit_t*)im, (digit_t*)re, NWORDS_FIELD*RADIX/8);

//     fp_inv(im);
//     fp_mul(im, im, inv2);
//     fp_mul(x->im, im, x->im);    
//     memcpy((digit_t*)x->re, (digit_t*)re, NWORDS_FIELD*RADIX/8);
// }

void
fp2_sqrt(fp2_t *a)
{
    fp_t x0, x1, t0, t1;

    /* From "Optimized One-Dimensional SQIsign Verification on Intel and
     * Cortex-M4" by Aardal et al: https://eprint.iacr.org/2024/1563 */

    // x0 = \delta = sqrt(a0^2 + a1^2).
    fp_sqr(x0, (a->re));
    fp_sqr(x1, (a->im));
    fp_add(x0, x0, x1);
    fp_sqrt(x0);
    // If a1 = 0, there is a risk of \delta = -a0, which makes x0 = 0 below.
    // In that case, we restore the value \delta = a0.
    fp_select(&x0, &x0, &(a->re), fp_is_zero_mask(&(a->im)));
    // x0 = \delta + a0, t0 = 2 * x0.
    fp_add(x0, x0, (a->re));
    fp_add(t0, x0, x0);

    // x1 = t0^(p-3)/4
    fp_exp3div4(x1, t0);

    // x0 = x0 * x1, x1 = x1 * a1, t1 = (2x0)^2.
    fp_mul(x0, x0, x1);
    fp_mul(x1, x1, (a->im));
    fp_add(t1, x0, x0);
    fp_sqr(t1, t1);
    // If t1 = t0, return x0 + x1*i, otherwise x1 - x0*i.
    fp_sub(t0, t0, t1);
    uint32_t f = fp_is_zero_mask(&t0);
    fp_neg(t1, x0);
    fp_copy(t0, x1);
    fp_select(&t0, &t0, &x0, f);
    fp_select(&t1, &t1, &x1, f);

    // Check if t0 is zero
    uint32_t t0_is_zero = fp_is_zero_mask(&t0);

    // Check whether t0, t1 are odd
    // Note: we encode to ensure canonical representation
    uint8_t tmp_bytes[FP_ENCODED_BYTES];
    fp_encode(tmp_bytes, &t0);
    uint32_t t0_is_odd = -((uint32_t)tmp_bytes[0] & 1);
    fp_encode(tmp_bytes, &t1);
    uint32_t t1_is_odd = -((uint32_t)tmp_bytes[0] & 1);

    // We negate the output if:
    // t0 is odd, or
    // t0 is zero and t1 is odd
    uint32_t negate_output = t0_is_odd | (t0_is_zero & t1_is_odd);
    fp_neg(x0, t0);
    fp_select(&(a->re), &t0, &x0, negate_output);
    fp_neg(x0, t1);
    fp_select(&(a->im), &t1, &x0, negate_output);
}

// Lexicographic comparison of two field elements. Returns +1 if x > y, -1 if x < y, 0 if x = y
int fp2_cmp(fp2_t* x, fp2_t* y){
    fp2_t a, b;
    fp2_frommont(&a, x);
    fp2_frommont(&b, y);
    for(int i = NWORDS_FIELD-1; i >= 0; i--){
        if(a.re[i] > b.re[i])
            return 1;
        if(a.re[i] < b.re[i])
            return -1;
    }
    for(int i = NWORDS_FIELD-1; i >= 0; i--){
        if(a.im[i] > b.im[i])
            return 1;
        if(a.im[i] < b.im[i])
            return -1;
    }
    return 0;
}

void fp2_swap(fp2_t* P, fp2_t* Q, const digit_t option)
{ // If option = 0 then P <- P and Q <- Q, else if option = 0xFF...FF then P <- Q and Q <- P
    fp_swap(P->re, Q->re, option);
    fp_swap(P->im, Q->im, option);
}

/*
 * If ctl == 0x00000000, then *d is set to a0
 * If ctl == 0xFFFFFFFF, then *d is set to a1
 * ctl MUST be either 0x00000000 or 0xFFFFFFFF.
 */
void fp2_select(fp2_t *d, const fp2_t *a0, const fp2_t *a1, uint32_t ctl)
{
    fp_select(&(d->re), &(a0->re), &(a1->re), ctl);
    fp_select(&(d->im), &(a0->im), &(a1->im), ctl);
}

#  define __PRI64_PREFIX	"l"
# define PRIx64		__PRI64_PREFIX "x"
void fp2_print(fp2_t const a){
    fp2_t b;
    fp2_frommont(&b, &a);
    printf("0x");
    for(int i = NWORDS_FIELD - 1; i >=0; i--)
        printf("%016" PRIx64, b.re[i]);
    printf(" + a*0x");
    for(int i = NWORDS_FIELD - 1; i >=0; i--)
        printf("%016" PRIx64, b.im[i]);
}

void fp2_encode(void *dst, const fp2_t *a)
{
    uint8_t *buf = dst;
    fp_encode(buf, &(a->re));
    fp_encode(buf + FP_ENCODED_BYTES, &(a->im));
}

void fp2_decode(fp2_t *d, const void *src)
{
    const uint8_t *buf = src;

    fp_decode(&(d->re), buf);
    fp_decode(&(d->im), buf + FP_ENCODED_BYTES);
}

void fp2_batched_inv(fp2_t *x, int len)
{
    fp2_t t1[len], t2[len];
    fp2_t inverse;

    // x = x0,...,xn
    // t1 = x0, x0*x1, ... ,x0 * x1 * ... * xn
    fp2_copy(&t1[0], &x[0]);
    for (int i = 1; i < len; i++) {
        fp2_mul(&t1[i], &t1[i - 1], &x[i]);
    }

    // inverse = 1/ (x0 * x1 * ... * xn)
    fp2_copy(&inverse, &t1[len - 1]);
    fp2_inv(&inverse);

    fp2_copy(&t2[0], &inverse);
    // t2 = 1/ (x0 * x1 * ... * xn), 1/ (x0 * x1 * ... * x(n-1)) , ... , 1/xO
    for (int i = 1; i < len; i++) {
        fp2_mul(&t2[i], &t2[i - 1], &x[len - i]);
    }

    fp2_copy(&x[0], &t2[len - 1]);

    for (int i = 1; i < len; i++) {
        fp2_mul(&x[i], &t1[i - 1], &t2[len - i - 1]);
    }
}