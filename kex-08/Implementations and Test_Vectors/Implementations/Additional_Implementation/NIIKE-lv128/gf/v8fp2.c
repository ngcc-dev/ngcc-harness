// SPDX-FileCopyrightText: 2026 The Project OSIDH-LD Authors
// SPDX-License-Identifier: Apache-2.0

#include "v8fp2.h"

void v8fp2_pack8(v8fp2_t* x, const fp2_t* a1, const fp2_t* a2, const fp2_t* a3, const fp2_t* a4, const fp2_t* a5, const fp2_t* a6, const fp2_t* a7, const fp2_t* a8)
{
    v8fp_pack8(&(x->re), &(a1->re), &(a2->re), &(a3->re), &(a4->re), &(a5->re), &(a6->re), &(a7->re), &(a8->re));
    v8fp_pack8(&(x->im), &(a1->im), &(a2->im), &(a3->im), &(a4->im), &(a5->im), &(a6->im), &(a7->im), &(a8->im));
}

void v8fp2_pack7(v8fp2_t* x, const fp2_t* a1, const fp2_t* a2, const fp2_t* a3, const fp2_t* a4, const fp2_t* a5, const fp2_t* a6, const fp2_t* a7)
{
    v8fp_pack7(&(x->re), &(a1->re), &(a2->re), &(a3->re), &(a4->re), &(a5->re), &(a6->re), &(a7->re));
    v8fp_pack7(&(x->im), &(a1->im), &(a2->im), &(a3->im), &(a4->im), &(a5->im), &(a6->im), &(a7->im));
}

void v8fp2_pack6(v8fp2_t* x, const fp2_t* a1, const fp2_t* a2, const fp2_t* a3, const fp2_t* a4, const fp2_t* a5, const fp2_t* a6)
{
    v8fp_pack6(&(x->re), &(a1->re), &(a2->re), &(a3->re), &(a4->re), &(a5->re), &(a6->re));
    v8fp_pack6(&(x->im), &(a1->im), &(a2->im), &(a3->im), &(a4->im), &(a5->im), &(a6->im));
}

void v8fp2_pack5(v8fp2_t* x, const fp2_t* a1, const fp2_t* a2, const fp2_t* a3, const fp2_t* a4, const fp2_t* a5)
{
    v8fp_pack5(&(x->re), &(a1->re), &(a2->re), &(a3->re), &(a4->re), &(a5->re));
    v8fp_pack5(&(x->im), &(a1->im), &(a2->im), &(a3->im), &(a4->im), &(a5->im));
}

void v8fp2_pack4(v8fp2_t* x, const fp2_t* a1, const fp2_t* a2, const fp2_t* a3, const fp2_t* a4)
{
    v8fp_pack4(&(x->re), &(a1->re), &(a2->re), &(a3->re), &(a4->re));
    v8fp_pack4(&(x->im), &(a1->im), &(a2->im), &(a3->im), &(a4->im));
}

void v8fp2_pack3(v8fp2_t* x, const fp2_t* a1, const fp2_t* a2, const fp2_t* a3)
{
    v8fp_pack3(&(x->re), &(a1->re), &(a2->re), &(a3->re));
    v8fp_pack3(&(x->im), &(a1->im), &(a2->im), &(a3->im));
}

void v8fp2_pack2(v8fp2_t* x, const fp2_t* a1, const fp2_t* a2)
{
    v8fp_pack2(&(x->re), &(a1->re), &(a2->re));
    v8fp_pack2(&(x->im), &(a1->im), &(a2->im));
}

void v8fp2_unpack8(fp2_t* a1, fp2_t* a2, fp2_t* a3, fp2_t* a4, fp2_t* a5, fp2_t* a6, fp2_t* a7, fp2_t* a8, const v8fp2_t* x)
{
    v8fp_unpack8(&(a1->re), &(a2->re), &(a3->re), &(a4->re), &(a5->re), &(a6->re), &(a7->re), &(a8->re), &(x->re));
    v8fp_unpack8(&(a1->im), &(a2->im), &(a3->im), &(a4->im), &(a5->im), &(a6->im), &(a7->im), &(a8->im), &(x->im));
}

void v8fp2_unpack7(fp2_t* a1, fp2_t* a2, fp2_t* a3, fp2_t* a4, fp2_t* a5, fp2_t* a6, fp2_t* a7, const v8fp2_t* x)
{
    v8fp_unpack7(&(a1->re), &(a2->re), &(a3->re), &(a4->re), &(a5->re), &(a6->re), &(a7->re), &(x->re));
    v8fp_unpack7(&(a1->im), &(a2->im), &(a3->im), &(a4->im), &(a5->im), &(a6->im), &(a7->im), &(x->im));
}

void v8fp2_unpack6(fp2_t* a1, fp2_t* a2, fp2_t* a3, fp2_t* a4, fp2_t* a5, fp2_t* a6, const v8fp2_t* x)
{
    v8fp_unpack6(&(a1->re), &(a2->re), &(a3->re), &(a4->re), &(a5->re), &(a6->re), &(x->re));
    v8fp_unpack6(&(a1->im), &(a2->im), &(a3->im), &(a4->im), &(a5->im), &(a6->im), &(x->im));
}

void v8fp2_unpack5(fp2_t* a1, fp2_t* a2, fp2_t* a3, fp2_t* a4, fp2_t* a5, const v8fp2_t* x)
{
    v8fp_unpack5(&(a1->re), &(a2->re), &(a3->re), &(a4->re), &(a5->re), &(x->re));
    v8fp_unpack5(&(a1->im), &(a2->im), &(a3->im), &(a4->im), &(a5->im), &(x->im));
}

void v8fp2_unpack4(fp2_t* a1, fp2_t* a2, fp2_t* a3, fp2_t* a4, const v8fp2_t* x)
{
    v8fp_unpack4(&(a1->re), &(a2->re), &(a3->re), &(a4->re), &(x->re));
    v8fp_unpack4(&(a1->im), &(a2->im), &(a3->im), &(a4->im), &(x->im));
}

void v8fp2_unpack3(fp2_t* a1, fp2_t* a2, fp2_t* a3, const v8fp2_t* x)
{
    v8fp_unpack3(&(a1->re), &(a2->re), &(a3->re), &(x->re));
    v8fp_unpack3(&(a1->im), &(a2->im), &(a3->im), &(x->im));
}

void v8fp2_unpack2(fp2_t* a1, fp2_t* a2, const v8fp2_t* x)
{
    v8fp_unpack2(&(a1->re), &(a2->re), &(x->re));
    v8fp_unpack2(&(a1->im), &(a2->im), &(x->im));
}

void v8fp2_add(v8fp2_t* x, const v8fp2_t* y, const v8fp2_t* z)
{
    v8fp_add(x->re, y->re, z->re);
    v8fp_add(x->im, y->im, z->im);
}

void v8fp2_sub(v8fp2_t* x, const v8fp2_t* y, const v8fp2_t* z)
{
    v8fp_sub(x->re, y->re, z->re);
    v8fp_sub(x->im, y->im, z->im);
}

void v8fp2_neg(v8fp2_t* x, const v8fp2_t* y)
{
    v8fp_neg(x->re, y->re);
    v8fp_neg(x->im, y->im);
}

void v8fp2_mul(v8fp2_t* x, const v8fp2_t* y, const v8fp2_t* z)
{
    v8fp_t t0, t1;

    v8fp_add(t0, y->re, y->im);
    v8fp_add(t1, z->re, z->im);
    v8fp_mul(t0, t0, t1);
    v8fp_mul(t1, y->im, z->im);
    v8fp_mul(x->re, y->re, z->re);
    v8fp_sub(x->im, t0, t1);
    v8fp_sub(x->im, x->im, x->re);
    v8fp_sub(x->re, x->re, t1);
}

void v8fp2_sqr(v8fp2_t* x, const v8fp2_t* y)
{
    v8fp_t sum, diff;

    v8fp_add(sum, y->re, y->im);
    v8fp_sub(diff, y->re, y->im);
    v8fp_mul(x->im, y->re, y->im);
    v8fp_add(x->im, x->im, x->im);
    v8fp_mul(x->re, sum, diff);
}

void v8fp2_set(v8fp2_t* x, const digit_t val)
{
    v8fp_set(x->re, val);
    v8fp_set(x->im, 0);
}

void v8fp2_copy(v8fp2_t* x, const v8fp2_t* y)
{
    v8fp_copy(x->re, y->re);
    v8fp_copy(x->im, y->im);
}

void v8fp2_mont_setone(v8fp2_t* out)
{
    v8fp_mont_setone(out->re);
    v8fp_set(out->im, 0);
}

void v8fp2_setzero(v8fp2_t* out)
{
    v8fp_set(out->re, 0);
    v8fp_set(out->im, 0);
}

void v8fp2_swap(v8fp2_t* P, v8fp2_t* Q, const __m512i option)
{ // If option = 0 then P <- P and Q <- Q, else if option = 0xFF...FF then P <- Q and Q <- P
    v8fp_swap(P->re, Q->re, option);
    v8fp_swap(P->im, Q->im, option);
}