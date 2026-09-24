// SPDX-FileCopyrightText: Copyright 2023 the SQIsign team. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

/*
 * This file is derived from ec.c in the SQIsign (version 1.0) 
 * project (https://github.com/SQIsign/the-sqisign/tree/nist-v1),
 * which is licensed under the Apache-2.0 license.
 *
 * Modifications made:
 *   - Deleted some unused functions
 *   - Added some functions
 */

#include "curve_extras.h"
#include <ec_params.h>
#include <assert.h>

void ec_init(ec_point_t* P)
{ // Initialize point as identity element (1:0)
    fp_t one = {0};
    
    memset((digit_t*)P, 0, NWORDS_FIELD*RADIX*4/8);
    one[0] = 1;
    fp_tomont(P->x.re, one);
}

void xDBLv2(ec_point_t* Q, ec_point_t const* P, ec_point_t const* A24)
{
    // This version receives the coefficient value A24 = (A+2C:4C) 
    fp2_t t0, t1, t2;

    fp2_add(&t0, &P->x, &P->z);
    fp2_sqr(&t0, &t0);
    fp2_sub(&t1, &P->x, &P->z);
    fp2_sqr(&t1, &t1);
    fp2_sub(&t2, &t0, &t1);
    fp2_mul(&t1, &t1, &A24->z);
    fp2_mul(&Q->x, &t0, &t1);
    fp2_mul(&t0, &t2, &A24->x);
    fp2_add(&t0, &t0, &t1);
    fp2_mul(&Q->z, &t0, &t2);
}

void xADD(ec_point_t* R, ec_point_t const* P, ec_point_t const* Q, ec_point_t const* PQ)
{
    fp2_t t0, t1, t2, t3;

    fp2_add(&t0, &P->x, &P->z);
    fp2_sub(&t1, &P->x, &P->z);
    fp2_add(&t2, &Q->x, &Q->z);
    fp2_sub(&t3, &Q->x, &Q->z);
    fp2_mul(&t0, &t0, &t3);
    fp2_mul(&t1, &t1, &t2);
    fp2_add(&t2, &t0, &t1);
    fp2_sub(&t3, &t0, &t1);
    fp2_sqr(&t2, &t2);
    fp2_sqr(&t3, &t3);
    fp2_mul(&t2, &PQ->z, &t2);
    fp2_mul(&R->z, &PQ->x, &t3);
    fp2_copy(&R->x, &t2);
}

void xDBLADD(ec_point_t* R, ec_point_t* S, ec_point_t const* P, ec_point_t const* Q, ec_point_t const* PQ, ec_point_t const* A24)
{
    // Requires precomputation of A24 = (A+2C:4C)
    fp2_t t0, t1, t2;

    fp2_add(&t0, &P->x, &P->z);
    fp2_sub(&t1, &P->x, &P->z);
    fp2_add(&t0, &P->x, &P->z);
    fp2_sqr(&R->x, &t0);
    fp2_sub(&t2, &Q->x, &Q->z);
    fp2_add(&S->x, &Q->x, &Q->z);
    fp2_mul(&t0, &t0, &t2);
    fp2_sqr(&R->z, &t1);
    fp2_mul(&t1, &t1, &S->x);
    fp2_sub(&t2, &R->x, &R->z);
    fp2_mul(&R->z, &R->z, &A24->z);
    fp2_mul(&R->x, &R->x, &R->z);
    fp2_mul(&S->x, &A24->x, &t2);
    fp2_sub(&S->z, &t0, &t1);
    fp2_add(&R->z, &R->z, &S->x);
    fp2_add(&S->x, &t0, &t1);
    fp2_mul(&R->z, &R->z, &t2);
    fp2_sqr(&S->z, &S->z);
    fp2_sqr(&S->x, &S->x);
    fp2_mul(&S->z, &S->z, &PQ->x);
    fp2_mul(&S->x, &S->x, &PQ->z);
}

void swap_points(ec_point_t* P, ec_point_t* Q, const digit_t option)
{ // Swap points
  // If option = 0 then P <- P and Q <- Q, else if option = 0xFF...FF then P <- Q and Q <- P
    fp2_swap(&(P->x), &(Q->x), option);
    fp2_swap(&(P->z), &(Q->z), option);
}

void copy_point(ec_point_t* P, ec_point_t const* Q)
{
    fp2_copy(&(P->x), &(Q->x));
    fp2_copy(&(P->z), &(Q->z));
}

void ec_normalize(ec_point_t* P){
    fp2_inv(&P->z);
    fp2_mul(&P->x, &P->x, &P->z);
    fp_mont_setone(P->z.re);
    fp_set(P->z.im, 0);
}

void xMULv2(ec_point_t* Q, ec_point_t const* P, digit_t const* k, const int kbits, ec_point_t const* A24)
{
    // This version receives the coefficient value A24 = (A+2C:4C) 
    ec_point_t R0, R1;
    digit_t mask;
    unsigned int bit = 0, prevbit = 0, swap;

    // R0 <- (1:0), R1 <- P
    ec_init(&R0);
    fp2_copy(&R1.x, &P->x);
    fp2_copy(&R1.z, &P->z);

    // Main loop
    for (int i = kbits-1; i >= 0; i--) {                              
        bit = (k[i >> LOG2RADIX] >> (i & (RADIX-1))) & 1;                      
        swap = bit ^ prevbit;
        prevbit = bit;
        mask = 0 - (digit_t)swap;

        swap_points(&R0, &R1, mask);
        xDBLADD(&R0, &R1, &R0, &R1, P, A24);
    }
    swap = 0 ^ prevbit;
    mask = 0 - (digit_t)swap;
    swap_points(&R0, &R1, mask);

    fp2_copy(&Q->x, &R0.x);
    fp2_copy(&Q->z, &R0.z);
}

void ec_j_inv(fp2_t* j_inv, const ec_curve_t* curve){
    /* j-invariant computation for montgommery coefficient A2=(A+2C:4C) */
    fp2_t t0, t1;
    
    fp2_sqr(&t1, &curve->C);
    fp2_sqr(j_inv, &curve->A);
    fp2_add(&t0, &t1, &t1);
    fp2_sub(&t0, j_inv, &t0);
    fp2_sub(&t0, &t0, &t1);
    fp2_sub(j_inv, &t0, &t1);
    fp2_sqr(&t1, &t1);
    fp2_mul(j_inv, j_inv, &t1);
    fp2_add(&t0, &t0, &t0);
    fp2_add(&t0, &t0, &t0);
    fp2_sqr(&t1, &t0);
    fp2_mul(&t0, &t0, &t1);
    fp2_add(&t0, &t0, &t0);
    fp2_add(&t0, &t0, &t0);
    fp2_inv(j_inv);
    fp2_mul(j_inv, &t0, j_inv);    
}

void ec_ker_mul(fp2_t* pi_X, fp2_t* pi_Z, const ec_point_t* ker, const ec_point_t* A24, int ell)
{
    int i = 0;
    fp2_mul(pi_X, pi_X, &(ker->x));
    fp2_mul(pi_Z, pi_Z, &(ker->z));
    i++;

    if (!(i < ell / 2))
    {
        return;
    }

    ec_point_t R;
    xDBLv2(&R, ker, A24);
    fp2_mul(pi_X, pi_X, &(R.x));
    fp2_mul(pi_Z, pi_Z, &(R.z));
    i++;

    ec_point_t Q = *ker, S;
    ec_point_t *now = &R, *diff = &Q, *res = &S;
    while(i < ell / 2)
    {
        ec_add(res, now, ker, diff);
        fp2_mul(pi_X, pi_X, &(res->x));
        fp2_mul(pi_Z, pi_Z, &(res->z));
        ec_point_t *temp;
        temp = diff;
        diff = now;
        now = res;
        res = temp;
        i++;
    }
}