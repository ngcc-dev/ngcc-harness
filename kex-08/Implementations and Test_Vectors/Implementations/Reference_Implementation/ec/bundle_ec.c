// SPDX-FileCopyrightText: Copyright 2023 the SQIsign team. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

/*
 * This file is derived from ec.c in the SQIsign (version 1.0) 
 * project (https://github.com/SQIsign/the-sqisign/tree/nist-v1),
 * which is licensed under the Apache-2.0 license.
 *
 * Modifications made:
 *   - Deleted some unused functions
 *   - Changed the data structure to SoA
 */

#include "bundle_curve_extras.h"
#include <ec_params.h>
#include <assert.h>
#include <stdio.h> //del

void bundleec_init(bundleec_point_t* P, int len)
{ // Initialize point as identity element (1:0)
    bundlefp2_mont_setone(&(P->x), len);
    bundlefp2_setzero(&(P->z), len);
}

void bundlexDBLv2(bundleec_point_t* Q, bundleec_point_t const* P, bundleec_point_t const* A24, int len)
{
    // This version receives the coefficient value A24 = (A+2C:4C) 
    bundlefp2_t t0, t1, t2;

    bundlefp2_add(&t0, &P->x, &P->z, len);
    bundlefp2_sqr(&t0, &t0, len);
    bundlefp2_sub(&t1, &P->x, &P->z, len);
    bundlefp2_sqr(&t1, &t1, len);
    bundlefp2_sub(&t2, &t0, &t1, len);
    bundlefp2_mul(&t1, &t1, &A24->z, len);
    bundlefp2_mul(&Q->x, &t0, &t1, len);
    bundlefp2_mul(&t0, &t2, &A24->x, len);
    bundlefp2_add(&t0, &t0, &t1, len);
    bundlefp2_mul(&Q->z, &t0, &t2, len);
}

void bundlexADD(bundleec_point_t* R, bundleec_point_t const* P, bundleec_point_t const* Q, bundleec_point_t const* PQ, int len)
{
    bundlefp2_t t0, t1, t2, t3;

    bundlefp2_add(&t0, &(P->x), &(P->z), len);
    bundlefp2_sub(&t1, &(P->x), &(P->z), len);
    bundlefp2_add(&t2, &(Q->x), &(Q->z), len);
    bundlefp2_sub(&t3, &(Q->x), &(Q->z), len);
    bundlefp2_mul(&t0, &t0, &t3, len);
    bundlefp2_mul(&t1, &t1, &t2, len);
    bundlefp2_add(&t2, &t0, &t1, len);
    bundlefp2_sub(&t3, &t0, &t1, len);
    bundlefp2_sqr(&t2, &t2, len);
    bundlefp2_sqr(&t3, &t3, len);
    bundlefp2_mul(&t2, &(PQ->z), &t2, len);
    bundlefp2_mul(&(R->z), &(PQ->x), &t3, len);
    bundlefp2_copy(&(R->x), &t2, len);
}

void bundlexDBLADD(bundleec_point_t* R, bundleec_point_t* S, bundleec_point_t const* P, bundleec_point_t const* Q, bundleec_point_t const* PQ, bundleec_point_t const* A24, int len)
{
    // Requires precomputation of A24 = (A+2C:4C)
    bundlefp2_t t0, t1, t2;

    bundlefp2_add(&t0, &P->x, &P->z, len);
    bundlefp2_sub(&t1, &P->x, &P->z, len);
    bundlefp2_add(&t0, &P->x, &P->z, len);
    bundlefp2_sqr(&R->x, &t0, len);
    bundlefp2_sub(&t2, &Q->x, &Q->z, len);
    bundlefp2_add(&S->x, &Q->x, &Q->z, len);
    bundlefp2_mul(&t0, &t0, &t2, len);
    bundlefp2_sqr(&R->z, &t1, len);
    bundlefp2_mul(&t1, &t1, &S->x, len);
    bundlefp2_sub(&t2, &R->x, &R->z, len);
    bundlefp2_mul(&R->z, &R->z, &A24->z, len);
    bundlefp2_mul(&R->x, &R->x, &R->z, len);
    bundlefp2_mul(&S->x, &A24->x, &t2, len);
    bundlefp2_sub(&S->z, &t0, &t1, len);
    bundlefp2_add(&R->z, &R->z, &S->x, len);
    bundlefp2_add(&S->x, &t0, &t1, len);
    bundlefp2_mul(&R->z, &R->z, &t2, len);
    bundlefp2_sqr(&S->z, &S->z, len);
    bundlefp2_sqr(&S->x, &S->x, len);
    bundlefp2_mul(&S->z, &S->z, &PQ->x, len);
    bundlefp2_mul(&S->x, &S->x, &PQ->z, len);
}

void bundleswap_points(bundleec_point_t* P, bundleec_point_t* Q, const digit_t option, int len)
{ // Swap points
  // If option = 0 then P <- P and Q <- Q, else if option = 0xFF...FF then P <- Q and Q <- P
    bundlefp2_swap(&(P->x), &(Q->x), option, len);
    bundlefp2_swap(&(P->z), &(Q->z), option, len);
}

void bundlecopy_point(bundleec_point_t* P, bundleec_point_t const* Q, int len)
{
    bundlefp2_copy(&(P->x), &(Q->x), len);
    bundlefp2_copy(&(P->z), &(Q->z), len);
}

void bundlexMULv2(bundleec_point_t* Q, bundleec_point_t const* P, digit_t const* k, const int kbits, bundleec_point_t const* A24, int len)
{
    // This version receives the coefficient value A24 = (A+2C:4C) 
    bundleec_point_t R0, R1;
    digit_t mask;
    unsigned int bit = 0, prevbit = 0, swap;

    // R0 <- (1:0), R1 <- P
    bundleec_init(&R0, len);
    bundlefp2_copy(&R1.x, &P->x, len);
    bundlefp2_copy(&R1.z, &P->z, len);

    // Main loop
    for (int i = kbits-1; i >= 0; i--) {                              
        bit = (k[i >> LOG2RADIX] >> (i & (RADIX-1))) & 1;                      
        swap = bit ^ prevbit;
        prevbit = bit;
        mask = 0 - (digit_t)swap;

        bundleswap_points(&R0, &R1, mask, len);
        bundlexDBLADD(&R0, &R1, &R0, &R1, P, A24, len);
    }
    swap = 0 ^ prevbit;
    mask = 0 - (digit_t)swap;
    bundleswap_points(&R0, &R1, mask, len);

    bundlefp2_copy(&Q->x, &R0.x, len);
    bundlefp2_copy(&Q->z, &R0.z, len);
}

void bundleec_point_shift(bundleec_point_t *res, const bundleec_point_t *inp, int len)
{
    bundlefp2_shift(&(res->x), &(inp->x), len);
    bundlefp2_shift(&(res->z), &(inp->z), len);
}

void bundleec_point_select(bundleec_point_t *res, const bundleec_point_t *inp1, const bundleec_point_t *inp2, uint32_t* ctl, int len)
{
    bundlefp2_select(&(res->x), &(inp1->x), &(inp2->x), ctl, len);
    bundlefp2_select(&(res->z), &(inp1->z), &(inp2->z), ctl, len);
}

void bundleec_point_transposition(ec_point_t res[], const bundleec_point_t *inp, int len)
{
    for (int i = 0; i < len; i++)
    {
        fp2_copy(&(res[i].x), &(inp->x.list[i]));
        fp2_copy(&(res[i].z), &(inp->z.list[i]));
    }
}

void get_point_from_bundle(ec_point_t *res, const bundleec_point_t *inp, size_t location)
{
    fp2_copy(&((*res).x), &(inp->x.list[location]));
    fp2_copy(&((*res).z), &(inp->z.list[location]));
}

void set_point_to_bundle(bundleec_point_t *res, const ec_point_t *inp, size_t location)
{
    fp2_copy(&(res->x.list[location]), &((*inp).x));
    fp2_copy(&(res->z.list[location]), &((*inp).z));
}

void bundleec_ker_mul(bundlefp2_t* pi_X, bundlefp2_t* pi_Z, const bundleec_point_t* ker, const bundleec_point_t* A24, int ell, int len)
{
    int i = 0;
    bundlefp2_mul(pi_X, pi_X, &(ker->x), len);
    bundlefp2_mul(pi_Z, pi_Z, &(ker->z), len);
    i++;

    if (!(i < ell / 2))
    {
        return;
    }

    bundleec_point_t R;
    bundlexDBLv2(&R, ker, A24, len);
    bundlefp2_mul(pi_X, pi_X, &(R.x), len);
    bundlefp2_mul(pi_Z, pi_Z, &(R.z), len);
    i++;

    bundleec_point_t Q = *ker, S;
    bundleec_point_t *now = &R, *diff = &Q, *res = &S;
    while(i < ell / 2)
    {
        bundlexADD(res, now, ker, diff, len);
        bundlefp2_mul(pi_X, pi_X, &(res->x), len);
        bundlefp2_mul(pi_Z, pi_Z, &(res->z), len);
        bundleec_point_t *temp;
        temp = diff;
        diff = now;
        now = res;
        res = temp;
        i++;
    }
}