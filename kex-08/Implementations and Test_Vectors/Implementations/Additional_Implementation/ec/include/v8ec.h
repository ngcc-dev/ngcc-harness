// SPDX-FileCopyrightText: Copyright 2023 the SQIsign team. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

/*
 * This file is derived from ec.h in the SQIsign (version 1.0) 
 * project (https://github.com/SQIsign/the-sqisign/tree/nist-v1),
 * which is licensed under the Apache-2.0 license.
 *
 * Modifications made:
 *   - Deleted some unused functions
 *   - Added some functions
 *   - Changed the data structure to make it suitable for vectorization
 */

#ifndef V8EC_H
#define V8EC_H

#include <v8fp2.h>
#include <ec_params.h>
#include "ec.h"

typedef struct v8ec_point_t {
    v8fp2_t x;
    v8fp2_t z;
} v8ec_point_t;

typedef struct v8ec_curve_t {
    v8fp2_t A;
    v8fp2_t C; ///< cannot be 0
} v8ec_curve_t;

void v8xMULv2(v8ec_point_t* Q, v8ec_point_t const* P, digit_t const* k, const int kbits, v8ec_point_t const* A24);
// void v8ec_j_inv(v8fp2_t* j_inv, const v8ec_curve_t* curve);

static void v8ec_point_pack8(v8ec_point_t *out, const ec_point_t *p1, const ec_point_t *p2, const ec_point_t *p3, const ec_point_t *p4, const ec_point_t *p5, const ec_point_t *p6, const ec_point_t *p7, const ec_point_t *p8)
{
    v8fp2_pack8(&(out->x), &(p1->x), &(p2->x), &(p3->x), &(p4->x), &(p5->x), &(p6->x), &(p7->x), &(p8->x));
    v8fp2_pack8(&(out->z), &(p1->z), &(p2->z), &(p3->z), &(p4->z), &(p5->z), &(p6->z), &(p7->z), &(p8->z));
}

static void v8ec_point_unpack8(ec_point_t *p1, ec_point_t *p2, ec_point_t *p3, ec_point_t *p4, ec_point_t *p5, ec_point_t *p6, ec_point_t *p7, ec_point_t *p8, const v8ec_point_t *in)
{
    v8fp2_unpack8(&(p1->x), &(p2->x), &(p3->x), &(p4->x), &(p5->x), &(p6->x), &(p7->x), &(p8->x), &(in->x));
    v8fp2_unpack8(&(p1->z), &(p2->z), &(p3->z), &(p4->z), &(p5->z), &(p6->z), &(p7->z), &(p8->z), &(in->z));
}

static void v8ec_point_pack7(v8ec_point_t *out, const ec_point_t *p1, const ec_point_t *p2, const ec_point_t *p3, const ec_point_t *p4, const ec_point_t *p5, const ec_point_t *p6, const ec_point_t *p7)
{
    v8fp2_pack7(&(out->x), &(p1->x), &(p2->x), &(p3->x), &(p4->x), &(p5->x), &(p6->x), &(p7->x));
    v8fp2_pack7(&(out->z), &(p1->z), &(p2->z), &(p3->z), &(p4->z), &(p5->z), &(p6->z), &(p7->z));
}

static void v8ec_point_unpack7(ec_point_t *p1, ec_point_t *p2, ec_point_t *p3, ec_point_t *p4, ec_point_t *p5, ec_point_t *p6, ec_point_t *p7, const v8ec_point_t *in)
{
    v8fp2_unpack7(&(p1->x), &(p2->x), &(p3->x), &(p4->x), &(p5->x), &(p6->x), &(p7->x), &(in->x));
    v8fp2_unpack7(&(p1->z), &(p2->z), &(p3->z), &(p4->z), &(p5->z), &(p6->z), &(p7->z), &(in->z));
}

static void v8ec_point_pack6(v8ec_point_t *out, const ec_point_t *p1, const ec_point_t *p2, const ec_point_t *p3, const ec_point_t *p4, const ec_point_t *p5, const ec_point_t *p6)
{
    v8fp2_pack6(&(out->x), &(p1->x), &(p2->x), &(p3->x), &(p4->x), &(p5->x), &(p6->x));
    v8fp2_pack6(&(out->z), &(p1->z), &(p2->z), &(p3->z), &(p4->z), &(p5->z), &(p6->z));
}

static void v8ec_point_unpack6(ec_point_t *p1, ec_point_t *p2, ec_point_t *p3, ec_point_t *p4, ec_point_t *p5, ec_point_t *p6, const v8ec_point_t *in)
{
    v8fp2_unpack6(&(p1->x), &(p2->x), &(p3->x), &(p4->x), &(p5->x), &(p6->x), &(in->x));
    v8fp2_unpack6(&(p1->z), &(p2->z), &(p3->z), &(p4->z), &(p5->z), &(p6->z), &(in->z));
}

static void v8ec_point_pack5(v8ec_point_t *out, const ec_point_t *p1, const ec_point_t *p2, const ec_point_t *p3, const ec_point_t *p4, const ec_point_t *p5)
{
    v8fp2_pack5(&(out->x), &(p1->x), &(p2->x), &(p3->x), &(p4->x), &(p5->x));
    v8fp2_pack5(&(out->z), &(p1->z), &(p2->z), &(p3->z), &(p4->z), &(p5->z));
}

static void v8ec_point_unpack5(ec_point_t *p1, ec_point_t *p2, ec_point_t *p3, ec_point_t *p4, ec_point_t *p5, const v8ec_point_t *in)
{
    v8fp2_unpack5(&(p1->x), &(p2->x), &(p3->x), &(p4->x), &(p5->x), &(in->x));
    v8fp2_unpack5(&(p1->z), &(p2->z), &(p3->z), &(p4->z), &(p5->z), &(in->z));
}

static void v8ec_point_pack4(v8ec_point_t *out, const ec_point_t *p1, const ec_point_t *p2, const ec_point_t *p3, const ec_point_t *p4)
{
    v8fp2_pack4(&(out->x), &(p1->x), &(p2->x), &(p3->x), &(p4->x));
    v8fp2_pack4(&(out->z), &(p1->z), &(p2->z), &(p3->z), &(p4->z));
}

static void v8ec_point_unpack4(ec_point_t *p1, ec_point_t *p2, ec_point_t *p3, ec_point_t *p4, const v8ec_point_t *in)
{
    v8fp2_unpack4(&(p1->x), &(p2->x), &(p3->x), &(p4->x), &(in->x));
    v8fp2_unpack4(&(p1->z), &(p2->z), &(p3->z), &(p4->z), &(in->z));
}

static void v8ec_point_pack3(v8ec_point_t *out, const ec_point_t *p1, const ec_point_t *p2, const ec_point_t *p3)
{
    v8fp2_pack3(&(out->x), &(p1->x), &(p2->x), &(p3->x));
    v8fp2_pack3(&(out->z), &(p1->z), &(p2->z), &(p3->z));
}

static void v8ec_point_unpack3(ec_point_t *p1, ec_point_t *p2, ec_point_t *p3, const v8ec_point_t *in)
{
    v8fp2_unpack3(&(p1->x), &(p2->x), &(p3->x), &(in->x));
    v8fp2_unpack3(&(p1->z), &(p2->z), &(p3->z), &(in->z));
}

static void v8ec_point_pack2(v8ec_point_t *out, const ec_point_t *p1, const ec_point_t *p2)
{
    v8fp2_pack2(&(out->x), &(p1->x), &(p2->x));
    v8fp2_pack2(&(out->z), &(p1->z), &(p2->z));
}

static void v8ec_point_unpack2(ec_point_t *p1, ec_point_t *p2, const v8ec_point_t *in)
{
    v8fp2_unpack2(&(p1->x), &(p2->x), &(in->x));
    v8fp2_unpack2(&(p1->z), &(p2->z), &(in->z));
}

#endif