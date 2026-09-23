// SPDX-FileCopyrightText: 2026 The Project OSIDH-LD Authors
// SPDX-License-Identifier: Apache-2.0

#include "bundle_fp2.h"

void bundlefp2_add(bundlefp2_t* x, const bundlefp2_t* y, const bundlefp2_t* z, int len)
{
    for(int i = 0; i < len; i++)
    {
        fp2_add(&(x->list[i]), &(y->list[i]), &(z->list[i]));
    }
}

void bundlefp2_sub(bundlefp2_t* x, const bundlefp2_t* y, const bundlefp2_t* z, int len)
{
    for(int i = 0; i < len; i++)
    {
        fp2_sub(&(x->list[i]), &(y->list[i]), &(z->list[i]));
    }
}

void bundlefp2_neg(bundlefp2_t* x, const bundlefp2_t* y, int len)
{
    for(int i = 0; i < len; i++)
    {
        fp2_neg(&(x->list[i]), &(y->list[i]));
    }
}

void bundlefp2_mul(bundlefp2_t* x, const bundlefp2_t* y, const bundlefp2_t* z, int len)
{
    for(int i = 0; i < len; i++)
    {
        fp2_mul(&(x->list[i]), &(y->list[i]), &(z->list[i]));
    }
}

void bundlefp2_sqr(bundlefp2_t* x, const bundlefp2_t* y, int len)
{
    for(int i = 0; i < len; i++)
    {
        fp2_sqr(&(x->list[i]), &(y->list[i]));
    }
}

void bundlefp2_inv(bundlefp2_t* x, int len)
{
    for(int i = 0; i < len; i++)
    {
        fp2_inv(&(x->list[i]));
    }
}

void bundlefp2_sqrt(bundlefp2_t* x, int len)
{
    for(int i = 0; i < len; i++)
    {
        fp2_sqrt(&(x->list[i]));
    }
}

void bundlefp2_set(bundlefp2_t* x, const digit_t val, int len)
{
    for(int i = 0; i < len; i++)
    {
        fp2_set(&(x->list[i]), val);
    }
}

void bundlefp2_copy(bundlefp2_t* x, const bundlefp2_t* y, int len)
{
    for(int i = 0; i < len; i++)
    {
        fp2_copy(&(x->list[i]), &(y->list[i]));
    }
}

void bundlefp2_from_single(bundlefp2_t* x, const fp2_t* y, int len)
{
    for(int i = 0; i < len; i++)
    {
        fp2_copy(&(x->list[i]), y);
    }
}

void bundlefp2_mont_setone(bundlefp2_t* out, int len)
{
    for(int i = 0; i < len; i++)
    {
        fp_mont_setone(out->list[i].re);
        fp_set(out->list[i].im, 0);
    }
}

void bundlefp2_setzero(bundlefp2_t* out, int len)
{
    for(int i = 0; i < len; i++)
    {
        fp_set(out->list[i].re, 0);
        fp_set(out->list[i].im, 0);
    }
}

void bundlefp2_shift(bundlefp2_t* x, const bundlefp2_t* y, int len)
{
    // for (int i = 0; i < len - 1; i++)
    // {
    //     fp2_copy(&(x->list[i]), &(y->list[i + 1]));
    // }
    // if (len == CYCLE_LENGTH)
    // {
    //     fp2_copy(&(x->list[CYCLE_LENGTH - 1]), &(y->list[0]));
    // }
    if (len == CYCLE_LENGTH)
    {
        for (int i = 0; i < CYCLE_LENGTH - 1; i++)
        {
            fp2_copy(&(x->list[i]), &(y->list[i + 1]));
        }
        fp2_copy(&(x->list[CYCLE_LENGTH - 1]), &(y->list[0]));
    }
    else
    {
        for (int i = 0; i < len; i++)
        {
            fp2_copy(&(x->list[i]), &(y->list[i + 1]));
        }
    }
}

void bundlefp2_is_zero_mask(uint32_t* res, const bundlefp2_t *a, int len)
{
    for (int i = 0; i < len; i++)
    {
        res[i] = fp2_is_zero_mask(&(a->list[i]));
    }
}

void bundlefp2_swap(bundlefp2_t* P, bundlefp2_t* Q, const digit_t option, int len)
{ // If option = 0 then P <- P and Q <- Q, else if option = 0xFF...FF then P <- Q and Q <- P
    for(int i = 0; i < len; i++)
    {
        fp2_swap(&(P->list[i]), &(Q->list[i]), option);
    }
}

void bundlefp2_select(bundlefp2_t *d, const bundlefp2_t *a0, const bundlefp2_t *a1, uint32_t* ctl, int len)
{
    for(int i = 0; i < len; i++)
    {
        fp2_select(&(d->list[i]), &(a0->list[i]), &(a1->list[i]), ctl[i]);
    }
}

void bundlefp2_mul_single_fp(bundlefp2_t* x, const bundlefp2_t* y, const fp_t z, int len)
{
    for(int i = 0; i < len; i++)
    {
        fp_mul(x->list[i].re, y->list[i].re, z);
        fp_mul(x->list[i].im, y->list[i].im, z);
    }
}

void bundlefp2_mul_all_to_one(fp2_t* x, const bundlefp2_t* y, int len)
{
    for(int i = 0; i < len; i++)
    {
        fp2_mul(x, x, (&(y->list[i])));
    }
}