// SPDX-FileCopyrightText: Copyright 2023 the SQIsign team. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

/*
 * This file is derived from ec.c in the SQIsign (version 1.0) 
 * project (https://github.com/SQIsign/the-sqisign/tree/nist-v1),
 * which is licensed under the Apache-2.0 license.
 *
 * Modifications made:
 *   - Deleted some unused functions
 *   - Changed the data structure to make it suitable for vectorization
 */

#include "v8curve_extras.h"
#include <ec_params.h>
#include <assert.h>

void v8ec_init(v8ec_point_t* P)
{ // Initialize point as identity element (1:0)
    v8fp2_mont_setone(&(P->x));
    v8fp2_setzero(&(P->z));
}

void v8xDBLv2(v8ec_point_t* Q, v8ec_point_t const* P, v8ec_point_t const* A24)
{
    // This version receives the coefficient value A24 = (A+2C:4C) 
    v8fp2_t t0, t1, t2;

    v8fp2_add(&t0, &P->x, &P->z);
    v8fp2_sqr(&t0, &t0);
    v8fp2_sub(&t1, &P->x, &P->z);
    v8fp2_sqr(&t1, &t1);
    v8fp2_sub(&t2, &t0, &t1);
    v8fp2_mul(&t1, &t1, &A24->z);
    v8fp2_mul(&Q->x, &t0, &t1);
    v8fp2_mul(&t0, &t2, &A24->x);
    v8fp2_add(&t0, &t0, &t1);
    v8fp2_mul(&Q->z, &t0, &t2);
}

void v8xADD(v8ec_point_t* R, v8ec_point_t const* P, v8ec_point_t const* Q, v8ec_point_t const* PQ)
{
    v8fp2_t t0, t1, t2, t3;

    v8fp2_add(&t0, &P->x, &P->z);
    v8fp2_sub(&t1, &P->x, &P->z);
    v8fp2_add(&t2, &Q->x, &Q->z);
    v8fp2_sub(&t3, &Q->x, &Q->z);
    v8fp2_mul(&t0, &t0, &t3);
    v8fp2_mul(&t1, &t1, &t2);
    v8fp2_add(&t2, &t0, &t1);
    v8fp2_sub(&t3, &t0, &t1);
    v8fp2_sqr(&t2, &t2);
    v8fp2_sqr(&t3, &t3);
    v8fp2_mul(&t2, &PQ->z, &t2);
    v8fp2_mul(&R->z, &PQ->x, &t3);
    v8fp2_copy(&R->x, &t2);
}

void v8xDBLADD(v8ec_point_t* R, v8ec_point_t* S, v8ec_point_t const* P, v8ec_point_t const* Q, v8ec_point_t const* PQ, v8ec_point_t const* A24)
{
    // Requires precomputation of A24 = (A+2C:4C)
    v8fp2_t t0, t1, t2;

    v8fp2_add(&t0, &P->x, &P->z);
    v8fp2_sub(&t1, &P->x, &P->z);
    v8fp2_add(&t0, &P->x, &P->z);
    v8fp2_sqr(&R->x, &t0);
    v8fp2_sub(&t2, &Q->x, &Q->z);
    v8fp2_add(&S->x, &Q->x, &Q->z);
    v8fp2_mul(&t0, &t0, &t2);
    v8fp2_sqr(&R->z, &t1);
    v8fp2_mul(&t1, &t1, &S->x);
    v8fp2_sub(&t2, &R->x, &R->z);
    v8fp2_mul(&R->z, &R->z, &A24->z);
    v8fp2_mul(&R->x, &R->x, &R->z);
    v8fp2_mul(&S->x, &A24->x, &t2);
    v8fp2_sub(&S->z, &t0, &t1);
    v8fp2_add(&R->z, &R->z, &S->x);
    v8fp2_add(&S->x, &t0, &t1);
    v8fp2_mul(&R->z, &R->z, &t2);
    v8fp2_sqr(&S->z, &S->z);
    v8fp2_sqr(&S->x, &S->x);
    v8fp2_mul(&S->z, &S->z, &PQ->x);
    v8fp2_mul(&S->x, &S->x, &PQ->z);
}

void v8copy_point(v8ec_point_t* P, v8ec_point_t const* Q)
{
    v8fp2_copy(&(P->x), &(Q->x));
    v8fp2_copy(&(P->z), &(Q->z));
}

void v8xMULv2(v8ec_point_t* Q, v8ec_point_t const* P, digit_t const* k, const int kbits, v8ec_point_t const* A24)
{
    // This version receives the coefficient value A24 = (A+2C:4C) 
    v8ec_point_t R0, R1;
    digit_t mask;
    unsigned int bit = 0, prevbit = 0, swap;

    // R0 <- (1:0), R1 <- P
    v8ec_init(&R0);
    v8fp2_copy(&R1.x, &P->x);
    v8fp2_copy(&R1.z, &P->z);

    // Main loop
    for (int i = kbits-1; i >= 0; i--) {                              
        bit = (k[i >> LOG2RADIX] >> (i & (RADIX-1))) & 1;                      
        swap = bit ^ prevbit;
        prevbit = bit;
        mask = 0 - (digit_t)swap;

        v8swap_points(&R0, &R1, _mm512_set1_epi64(mask));
        v8xDBLADD(&R0, &R1, &R0, &R1, P, A24);
    }
    swap = 0 ^ prevbit;
    mask = 0 - (digit_t)swap;
    v8swap_points(&R0, &R1, _mm512_set1_epi64(mask));

    v8fp2_copy(&Q->x, &R0.x);
    v8fp2_copy(&Q->z, &R0.z);
}

void v8swap_points(v8ec_point_t* P, v8ec_point_t* Q, const __m512i option)
{ // Swap points
  // If option = 0 then P <- P and Q <- Q, else if option = 0xFF...FF then P <- Q and Q <- P
    v8fp2_swap(&(P->x), &(Q->x), option);
    v8fp2_swap(&(P->z), &(Q->z), option);
}