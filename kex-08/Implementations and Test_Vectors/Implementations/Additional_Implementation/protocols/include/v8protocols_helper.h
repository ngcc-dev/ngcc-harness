// SPDX-FileCopyrightText: 2026 The Project OSIDH-LD Authors
// SPDX-License-Identifier: Apache-2.0

#ifndef V8HELPER_H
#define V8HELPER_H

#include "v8protocols.h"
#include "assert.h"

typedef struct v8vertex_t {
    v8ec_point_t A24;
    v8ec_point_t Ps;
    v8ec_point_t Pt;
    v8ec_point_t Qs;
    v8ec_point_t Qt;
} v8vertex_t;

typedef enum {
    ONE = 1,
    TWO,
    THREE,
    FOUR,
    FIVE,
    SIX,
    SEVEN
} SPLIT;

static inline void v8AC_to_A24(v8ec_point_t *A24, v8ec_curve_t const *E)
{
    // A24 = (A+2C : 4C)
    v8fp2_add(&A24->z, &E->C, &E->C);
    v8fp2_add(&A24->x, &E->A, &A24->z);
    v8fp2_add(&A24->z, &A24->z, &A24->z);
}

static inline void v8A24_to_AC(v8ec_curve_t *E, v8ec_point_t const *A24)
{
    // (A:C) = ((A+2C)*2-4C : 4C)
    v8fp2_add(&E->A, &A24->x, &A24->x);
    v8fp2_sub(&E->A, &E->A, &A24->z);
    v8fp2_add(&E->A, &E->A, &E->A);
    v8fp2_copy(&E->C, &A24->z);
}

static void v8vertex_pack8(v8vertex_t *out, const vertex_t *v1, const vertex_t *v2, const vertex_t *v3, const vertex_t *v4, const vertex_t *v5, const vertex_t *v6, const vertex_t *v7, const vertex_t *v8)
{
    v8ec_point_pack8(&(out->A24), &(v1->A24), &(v2->A24), &(v3->A24), &(v4->A24), &(v5->A24), &(v6->A24), &(v7->A24), &(v8->A24));
    v8ec_point_pack8(&(out->Ps), &(v1->Ps), &(v2->Ps), &(v3->Ps), &(v4->Ps), &(v5->Ps), &(v6->Ps), &(v7->Ps), &(v8->Ps));
    v8ec_point_pack8(&(out->Pt), &(v1->Pt), &(v2->Pt), &(v3->Pt), &(v4->Pt), &(v5->Pt), &(v6->Pt), &(v7->Pt), &(v8->Pt));
    v8ec_point_pack8(&(out->Qs), &(v1->Qs), &(v2->Qs), &(v3->Qs), &(v4->Qs), &(v5->Qs), &(v6->Qs), &(v7->Qs), &(v8->Qs));
    v8ec_point_pack8(&(out->Qt), &(v1->Qt), &(v2->Qt), &(v3->Qt), &(v4->Qt), &(v5->Qt), &(v6->Qt), &(v7->Qt), &(v8->Qt));
}

static void v8vertex_pack7(v8vertex_t *out, const vertex_t *v1, const vertex_t *v2, const vertex_t *v3, const vertex_t *v4, const vertex_t *v5, const vertex_t *v6, const vertex_t *v7)
{
    v8ec_point_pack7(&(out->A24), &(v1->A24), &(v2->A24), &(v3->A24), &(v4->A24), &(v5->A24), &(v6->A24), &(v7->A24));
    v8ec_point_pack7(&(out->Ps), &(v1->Ps), &(v2->Ps), &(v3->Ps), &(v4->Ps), &(v5->Ps), &(v6->Ps), &(v7->Ps));
    v8ec_point_pack7(&(out->Pt), &(v1->Pt), &(v2->Pt), &(v3->Pt), &(v4->Pt), &(v5->Pt), &(v6->Pt), &(v7->Pt));
    v8ec_point_pack7(&(out->Qs), &(v1->Qs), &(v2->Qs), &(v3->Qs), &(v4->Qs), &(v5->Qs), &(v6->Qs), &(v7->Qs));
    v8ec_point_pack7(&(out->Qt), &(v1->Qt), &(v2->Qt), &(v3->Qt), &(v4->Qt), &(v5->Qt), &(v6->Qt), &(v7->Qt));
}

static void v8vertex_pack6(v8vertex_t *out, const vertex_t *v1, const vertex_t *v2, const vertex_t *v3, const vertex_t *v4, const vertex_t *v5, const vertex_t *v6)
{
    v8ec_point_pack6(&(out->A24), &(v1->A24), &(v2->A24), &(v3->A24), &(v4->A24), &(v5->A24), &(v6->A24));
    v8ec_point_pack6(&(out->Ps), &(v1->Ps), &(v2->Ps), &(v3->Ps), &(v4->Ps), &(v5->Ps), &(v6->Ps));
    v8ec_point_pack6(&(out->Pt), &(v1->Pt), &(v2->Pt), &(v3->Pt), &(v4->Pt), &(v5->Pt), &(v6->Pt));
    v8ec_point_pack6(&(out->Qs), &(v1->Qs), &(v2->Qs), &(v3->Qs), &(v4->Qs), &(v5->Qs), &(v6->Qs));
    v8ec_point_pack6(&(out->Qt), &(v1->Qt), &(v2->Qt), &(v3->Qt), &(v4->Qt), &(v5->Qt), &(v6->Qt));
}

static void v8vertex_pack5(v8vertex_t *out, const vertex_t *v1, const vertex_t *v2, const vertex_t *v3, const vertex_t *v4, const vertex_t *v5)
{
    v8ec_point_pack5(&(out->A24), &(v1->A24), &(v2->A24), &(v3->A24), &(v4->A24), &(v5->A24));
    v8ec_point_pack5(&(out->Ps), &(v1->Ps), &(v2->Ps), &(v3->Ps), &(v4->Ps), &(v5->Ps));
    v8ec_point_pack5(&(out->Pt), &(v1->Pt), &(v2->Pt), &(v3->Pt), &(v4->Pt), &(v5->Pt));
    v8ec_point_pack5(&(out->Qs), &(v1->Qs), &(v2->Qs), &(v3->Qs), &(v4->Qs), &(v5->Qs));
    v8ec_point_pack5(&(out->Qt), &(v1->Qt), &(v2->Qt), &(v3->Qt), &(v4->Qt), &(v5->Qt));
}

static void v8vertex_pack4(v8vertex_t *out, const vertex_t *v1, const vertex_t *v2, const vertex_t *v3, const vertex_t *v4)
{
    v8ec_point_pack4(&(out->A24), &(v1->A24), &(v2->A24), &(v3->A24), &(v4->A24));
    v8ec_point_pack4(&(out->Ps), &(v1->Ps), &(v2->Ps), &(v3->Ps), &(v4->Ps));
    v8ec_point_pack4(&(out->Pt), &(v1->Pt), &(v2->Pt), &(v3->Pt), &(v4->Pt));
    v8ec_point_pack4(&(out->Qs), &(v1->Qs), &(v2->Qs), &(v3->Qs), &(v4->Qs));
    v8ec_point_pack4(&(out->Qt), &(v1->Qt), &(v2->Qt), &(v3->Qt), &(v4->Qt));
}

static void v8vertex_pack3(v8vertex_t *out, const vertex_t *v1, const vertex_t *v2, const vertex_t *v3)
{
    v8ec_point_pack3(&(out->A24), &(v1->A24), &(v2->A24), &(v3->A24));
    v8ec_point_pack3(&(out->Ps), &(v1->Ps), &(v2->Ps), &(v3->Ps));
    v8ec_point_pack3(&(out->Pt), &(v1->Pt), &(v2->Pt), &(v3->Pt));
    v8ec_point_pack3(&(out->Qs), &(v1->Qs), &(v2->Qs), &(v3->Qs));
    v8ec_point_pack3(&(out->Qt), &(v1->Qt), &(v2->Qt), &(v3->Qt));
}

static void v8vertex_pack2(v8vertex_t *out, const vertex_t *v1, const vertex_t *v2)
{
    v8ec_point_pack2(&(out->A24), &(v1->A24), &(v2->A24));
    v8ec_point_pack2(&(out->Ps), &(v1->Ps), &(v2->Ps));
    v8ec_point_pack2(&(out->Pt), &(v1->Pt), &(v2->Pt));
    v8ec_point_pack2(&(out->Qs), &(v1->Qs), &(v2->Qs));
    v8ec_point_pack2(&(out->Qt), &(v1->Qt), &(v2->Qt));
}

static void v8vertex_swappack8(v8vertex_t *out, const vertex_t *v1, const vertex_t *v2, const vertex_t *v3, const vertex_t *v4, const vertex_t *v5, const vertex_t *v6, const vertex_t *v7, const vertex_t *v8)
{
    v8ec_point_pack8(&(out->A24), &(v1->A24), &(v2->A24), &(v3->A24), &(v4->A24), &(v5->A24), &(v6->A24), &(v7->A24), &(v8->A24));
    v8ec_point_pack8(&(out->Ps), &(v1->Qs), &(v2->Qs), &(v3->Qs), &(v4->Qs), &(v5->Qs), &(v6->Qs), &(v7->Qs), &(v8->Qs));
    v8ec_point_pack8(&(out->Pt), &(v1->Qt), &(v2->Qt), &(v3->Qt), &(v4->Qt), &(v5->Qt), &(v6->Qt), &(v7->Qt), &(v8->Qt));
    v8ec_point_pack8(&(out->Qs), &(v1->Ps), &(v2->Ps), &(v3->Ps), &(v4->Ps), &(v5->Ps), &(v6->Ps), &(v7->Ps), &(v8->Ps));
    v8ec_point_pack8(&(out->Qt), &(v1->Pt), &(v2->Pt), &(v3->Pt), &(v4->Pt), &(v5->Pt), &(v6->Pt), &(v7->Pt), &(v8->Pt));
}

static void v8vertex_swappack7(v8vertex_t *out, const vertex_t *v1, const vertex_t *v2, const vertex_t *v3, const vertex_t *v4, const vertex_t *v5, const vertex_t *v6, const vertex_t *v7)
{
    v8ec_point_pack7(&(out->A24), &(v1->A24), &(v2->A24), &(v3->A24), &(v4->A24), &(v5->A24), &(v6->A24), &(v7->A24));
    v8ec_point_pack7(&(out->Ps), &(v1->Qs), &(v2->Qs), &(v3->Qs), &(v4->Qs), &(v5->Qs), &(v6->Qs), &(v7->Qs));
    v8ec_point_pack7(&(out->Pt), &(v1->Qt), &(v2->Qt), &(v3->Qt), &(v4->Qt), &(v5->Qt), &(v6->Qt), &(v7->Qt));
    v8ec_point_pack7(&(out->Qs), &(v1->Ps), &(v2->Ps), &(v3->Ps), &(v4->Ps), &(v5->Ps), &(v6->Ps), &(v7->Ps));
    v8ec_point_pack7(&(out->Qt), &(v1->Pt), &(v2->Pt), &(v3->Pt), &(v4->Pt), &(v5->Pt), &(v6->Pt), &(v7->Pt));
}

static void v8vertex_swappack6(v8vertex_t *out, const vertex_t *v1, const vertex_t *v2, const vertex_t *v3, const vertex_t *v4, const vertex_t *v5, const vertex_t *v6)
{
    v8ec_point_pack6(&(out->A24), &(v1->A24), &(v2->A24), &(v3->A24), &(v4->A24), &(v5->A24), &(v6->A24));
    v8ec_point_pack6(&(out->Ps), &(v1->Qs), &(v2->Qs), &(v3->Qs), &(v4->Qs), &(v5->Qs), &(v6->Qs));
    v8ec_point_pack6(&(out->Pt), &(v1->Qt), &(v2->Qt), &(v3->Qt), &(v4->Qt), &(v5->Qt), &(v6->Qt));
    v8ec_point_pack6(&(out->Qs), &(v1->Ps), &(v2->Ps), &(v3->Ps), &(v4->Ps), &(v5->Ps), &(v6->Ps));
    v8ec_point_pack6(&(out->Qt), &(v1->Pt), &(v2->Pt), &(v3->Pt), &(v4->Pt), &(v5->Pt), &(v6->Pt));
}

static void v8vertex_swappack5(v8vertex_t *out, const vertex_t *v1, const vertex_t *v2, const vertex_t *v3, const vertex_t *v4, const vertex_t *v5)
{
    v8ec_point_pack5(&(out->A24), &(v1->A24), &(v2->A24), &(v3->A24), &(v4->A24), &(v5->A24));
    v8ec_point_pack5(&(out->Ps), &(v1->Qs), &(v2->Qs), &(v3->Qs), &(v4->Qs), &(v5->Qs));
    v8ec_point_pack5(&(out->Pt), &(v1->Qt), &(v2->Qt), &(v3->Qt), &(v4->Qt), &(v5->Qt));
    v8ec_point_pack5(&(out->Qs), &(v1->Ps), &(v2->Ps), &(v3->Ps), &(v4->Ps), &(v5->Ps));
    v8ec_point_pack5(&(out->Qt), &(v1->Pt), &(v2->Pt), &(v3->Pt), &(v4->Pt), &(v5->Pt));
}

static void v8vertex_swappack4(v8vertex_t *out, const vertex_t *v1, const vertex_t *v2, const vertex_t *v3, const vertex_t *v4)
{
    v8ec_point_pack4(&(out->A24), &(v1->A24), &(v2->A24), &(v3->A24), &(v4->A24));
    v8ec_point_pack4(&(out->Ps), &(v1->Qs), &(v2->Qs), &(v3->Qs), &(v4->Qs));
    v8ec_point_pack4(&(out->Pt), &(v1->Qt), &(v2->Qt), &(v3->Qt), &(v4->Qt));
    v8ec_point_pack4(&(out->Qs), &(v1->Ps), &(v2->Ps), &(v3->Ps), &(v4->Ps));
    v8ec_point_pack4(&(out->Qt), &(v1->Pt), &(v2->Pt), &(v3->Pt), &(v4->Pt));
}

static void v8vertex_swappack3(v8vertex_t *out, const vertex_t *v1, const vertex_t *v2, const vertex_t *v3)
{
    v8ec_point_pack3(&(out->A24), &(v1->A24), &(v2->A24), &(v3->A24));
    v8ec_point_pack3(&(out->Ps), &(v1->Qs), &(v2->Qs), &(v3->Qs));
    v8ec_point_pack3(&(out->Pt), &(v1->Qt), &(v2->Qt), &(v3->Qt));
    v8ec_point_pack3(&(out->Qs), &(v1->Ps), &(v2->Ps), &(v3->Ps));
    v8ec_point_pack3(&(out->Qt), &(v1->Pt), &(v2->Pt), &(v3->Pt));
}

static void v8vertex_swappack2(v8vertex_t *out, const vertex_t *v1, const vertex_t *v2)
{
    v8ec_point_pack2(&(out->A24), &(v1->A24), &(v2->A24));
    v8ec_point_pack2(&(out->Ps), &(v1->Qs), &(v2->Qs));
    v8ec_point_pack2(&(out->Pt), &(v1->Qt), &(v2->Qt));
    v8ec_point_pack2(&(out->Qs), &(v1->Ps), &(v2->Ps));
    v8ec_point_pack2(&(out->Qt), &(v1->Pt), &(v2->Pt));
}

static void v8vertex_output8(vertex_t *out1, vertex_t *out2, vertex_t *out3, vertex_t *out4, vertex_t *out5, vertex_t *out6, vertex_t *out7, vertex_t *out8, const v8vertex_t *R0, const v8vertex_t *R1)
{
    v8ec_point_unpack8(&(out1->A24), &(out2->A24), &(out3->A24), &(out4->A24), &(out5->A24), &(out6->A24), &(out7->A24), &(out8->A24), &(R0->A24));
    v8ec_point_unpack8(&(out1->Ps), &(out2->Ps), &(out3->Ps), &(out4->Ps), &(out5->Ps), &(out6->Ps), &(out7->Ps), &(out8->Ps), &(R1->Qs));
    v8ec_point_unpack8(&(out1->Pt), &(out2->Pt), &(out3->Pt), &(out4->Pt), &(out5->Pt), &(out6->Pt), &(out7->Pt), &(out8->Pt), &(R1->Qt));
    v8ec_point_unpack8(&(out1->Qs), &(out2->Qs), &(out3->Qs), &(out4->Qs), &(out5->Qs), &(out6->Qs), &(out7->Qs), &(out8->Qs), &(R0->Qs));
    v8ec_point_unpack8(&(out1->Qt), &(out2->Qt), &(out3->Qt), &(out4->Qt), &(out5->Qt), &(out6->Qt), &(out7->Qt), &(out8->Qt), &(R0->Qt));
}

static void v8vertex_output7(vertex_t *out1, vertex_t *out2, vertex_t *out3, vertex_t *out4, vertex_t *out5, vertex_t *out6, vertex_t *out7, const v8vertex_t *R0, const v8vertex_t *R1)
{
    v8ec_point_unpack7(&(out1->A24), &(out2->A24), &(out3->A24), &(out4->A24), &(out5->A24), &(out6->A24), &(out7->A24), &(R0->A24));
    v8ec_point_unpack7(&(out1->Ps), &(out2->Ps), &(out3->Ps), &(out4->Ps), &(out5->Ps), &(out6->Ps), &(out7->Ps), &(R1->Qs));
    v8ec_point_unpack7(&(out1->Pt), &(out2->Pt), &(out3->Pt), &(out4->Pt), &(out5->Pt), &(out6->Pt), &(out7->Pt), &(R1->Qt));
    v8ec_point_unpack7(&(out1->Qs), &(out2->Qs), &(out3->Qs), &(out4->Qs), &(out5->Qs), &(out6->Qs), &(out7->Qs), &(R0->Qs));
    v8ec_point_unpack7(&(out1->Qt), &(out2->Qt), &(out3->Qt), &(out4->Qt), &(out5->Qt), &(out6->Qt), &(out7->Qt), &(R0->Qt));
}

static void v8vertex_output6(vertex_t *out1, vertex_t *out2, vertex_t *out3, vertex_t *out4, vertex_t *out5, vertex_t *out6, const v8vertex_t *R0, const v8vertex_t *R1)
{
    v8ec_point_unpack6(&(out1->A24), &(out2->A24), &(out3->A24), &(out4->A24), &(out5->A24), &(out6->A24), &(R0->A24));
    v8ec_point_unpack6(&(out1->Ps), &(out2->Ps), &(out3->Ps), &(out4->Ps), &(out5->Ps), &(out6->Ps), &(R1->Qs));
    v8ec_point_unpack6(&(out1->Pt), &(out2->Pt), &(out3->Pt), &(out4->Pt), &(out5->Pt), &(out6->Pt), &(R1->Qt));
    v8ec_point_unpack6(&(out1->Qs), &(out2->Qs), &(out3->Qs), &(out4->Qs), &(out5->Qs), &(out6->Qs), &(R0->Qs));
    v8ec_point_unpack6(&(out1->Qt), &(out2->Qt), &(out3->Qt), &(out4->Qt), &(out5->Qt), &(out6->Qt), &(R0->Qt));
}

static void v8vertex_output5(vertex_t *out1, vertex_t *out2, vertex_t *out3, vertex_t *out4, vertex_t *out5, const v8vertex_t *R0, const v8vertex_t *R1)
{
    v8ec_point_unpack5(&(out1->A24), &(out2->A24), &(out3->A24), &(out4->A24), &(out5->A24), &(R0->A24));
    v8ec_point_unpack5(&(out1->Ps), &(out2->Ps), &(out3->Ps), &(out4->Ps), &(out5->Ps), &(R1->Qs));
    v8ec_point_unpack5(&(out1->Pt), &(out2->Pt), &(out3->Pt), &(out4->Pt), &(out5->Pt), &(R1->Qt));
    v8ec_point_unpack5(&(out1->Qs), &(out2->Qs), &(out3->Qs), &(out4->Qs), &(out5->Qs), &(R0->Qs));
    v8ec_point_unpack5(&(out1->Qt), &(out2->Qt), &(out3->Qt), &(out4->Qt), &(out5->Qt), &(R0->Qt));
}

static void v8vertex_output4(vertex_t *out1, vertex_t *out2, vertex_t *out3, vertex_t *out4, const v8vertex_t *R0, const v8vertex_t *R1)
{
    v8ec_point_unpack4(&(out1->A24), &(out2->A24), &(out3->A24), &(out4->A24), &(R0->A24));
    v8ec_point_unpack4(&(out1->Ps), &(out2->Ps), &(out3->Ps), &(out4->Ps), &(R1->Qs));
    v8ec_point_unpack4(&(out1->Pt), &(out2->Pt), &(out3->Pt), &(out4->Pt), &(R1->Qt));
    v8ec_point_unpack4(&(out1->Qs), &(out2->Qs), &(out3->Qs), &(out4->Qs), &(R0->Qs));
    v8ec_point_unpack4(&(out1->Qt), &(out2->Qt), &(out3->Qt), &(out4->Qt), &(R0->Qt));
}

static void v8vertex_output3(vertex_t *out1, vertex_t *out2, vertex_t *out3, const v8vertex_t *R0, const v8vertex_t *R1)
{
    v8ec_point_unpack3(&(out1->A24), &(out2->A24), &(out3->A24), &(R0->A24));
    v8ec_point_unpack3(&(out1->Ps), &(out2->Ps), &(out3->Ps), &(R1->Qs));
    v8ec_point_unpack3(&(out1->Pt), &(out2->Pt), &(out3->Pt), &(R1->Qt));
    v8ec_point_unpack3(&(out1->Qs), &(out2->Qs), &(out3->Qs), &(R0->Qs));
    v8ec_point_unpack3(&(out1->Qt), &(out2->Qt), &(out3->Qt), &(R0->Qt));
}

static void v8vertex_output2(vertex_t *out1, vertex_t *out2, const v8vertex_t *R0, const v8vertex_t *R1)
{
    v8ec_point_unpack2(&(out1->A24), &(out2->A24), &(R0->A24));
    v8ec_point_unpack2(&(out1->Ps), &(out2->Ps), &(R1->Qs));
    v8ec_point_unpack2(&(out1->Pt), &(out2->Pt), &(R1->Qt));
    v8ec_point_unpack2(&(out1->Qs), &(out2->Qs), &(R0->Qs));
    v8ec_point_unpack2(&(out1->Qt), &(out2->Qt), &(R0->Qt));
}

static void v8fp2_split_update(v8fp2_t *res, v8fp2_t *in1, v8fp2_t *in2, SPLIT split)
{
    // for (int i = 0; i < split; i++)
    // {
    //     fp2_copy(&((*res)[i]), &((*in1)[i]));
    // }
    // for (int i = split; i < 8; i++)
    // {
    //     fp2_copy(&((*res)[i]), &((*in2)[i]));
    // }
    __mmask8 split_mask = (1 << split) - 1;
    for (int i = 0; i < NWORDS_FIELD; i++)
    {
        res->re[i] = _mm512_mask_mov_epi64(in2->re[i], split_mask, in1->re[i]);
    }
    for (int i = 0; i < NWORDS_FIELD; i++)
    {
        res->im[i] = _mm512_mask_mov_epi64(in2->im[i], split_mask, in1->im[i]);
    }
}

static void v8point_split_update(v8ec_point_t *res, v8ec_point_t *in1, v8ec_point_t *in2, SPLIT split)
{
    v8fp2_split_update(&(res->x), &(in1->x), &(in2->x), split);
    v8fp2_split_update(&(res->z), &(in1->z), &(in2->z), split);
}

static void v8vertex_split_update_s(v8vertex_t *res, v8vertex_t *in1, v8vertex_t *in2, SPLIT split)
{
    v8point_split_update(&(res->A24), &(in1->A24), &(in2->A24), split);
    // v8point_split_update(&(res->Ps), &(in1->Ps), &(in2->Ps), split);
    v8point_split_update(&(res->Pt), &(in1->Pt), &(in2->Pt), split);
    v8point_split_update(&(res->Qs), &(in1->Qs), &(in2->Qs), split);
    v8point_split_update(&(res->Qt), &(in1->Qt), &(in2->Qt), split);
}

static void v8vertex_split_update_t(v8vertex_t *res, v8vertex_t *in1, v8vertex_t *in2, SPLIT split)
{
    v8point_split_update(&(res->A24), &(in1->A24), &(in2->A24), split);
    // v8point_split_update(&(res->Ps), &(in1->Ps), &(in2->Ps), split);
    // v8point_split_update(&(res->Pt), &(in1->Pt), &(in2->Pt), split);
    v8point_split_update(&(res->Qs), &(in1->Qs), &(in2->Qs), split);
    v8point_split_update(&(res->Qt), &(in1->Qt), &(in2->Qt), split);
}

static void v8vertex_pack_split(v8vertex_t *R0, v8vertex_t *R1,
                                const vertex_t *in1, const vertex_t *in2, const vertex_t *in3, const vertex_t *in4, const vertex_t *in5, const vertex_t *in6, const vertex_t *in7, const vertex_t *in8, const vertex_t *in9, const vertex_t *in10,
                                SPLIT split)
{
    switch (split)
    {
    case ONE:
        v8vertex_pack8(R0, in1, in3, in4, in5, in6, in7, in8, in9);
        v8vertex_swappack8(R1, in2, in4, in5, in6, in7, in8, in9, in10);
        break;
    case TWO:
        v8vertex_pack8(R0, in1, in2, in4, in5, in6, in7, in8, in9);
        v8vertex_swappack8(R1, in2, in3, in5, in6, in7, in8, in9, in10);
        break;
    case THREE:
        v8vertex_pack8(R0, in1, in2, in3, in5, in6, in7, in8, in9);
        v8vertex_swappack8(R1, in2, in3, in4, in6, in7, in8, in9, in10);
        break;
    case FOUR:
        v8vertex_pack8(R0, in1, in2, in3, in4, in6, in7, in8, in9);
        v8vertex_swappack8(R1, in2, in3, in4, in5, in7, in8, in9, in10);
        break;
    case FIVE:
        v8vertex_pack8(R0, in1, in2, in3, in4, in5, in7, in8, in9);
        v8vertex_swappack8(R1, in2, in3, in4, in5, in6, in8, in9, in10);
        break;
    case SIX:
        v8vertex_pack8(R0, in1, in2, in3, in4, in5, in6, in8, in9);
        v8vertex_swappack8(R1, in2, in3, in4, in5, in6, in7, in9, in10);
        break;
    case SEVEN:
        v8vertex_pack8(R0, in1, in2, in3, in4, in5, in6, in7, in9);
        v8vertex_swappack8(R1, in2, in3, in4, in5, in6, in7, in8, in10);
        break;
    default:
        assert(0);
        break;
    }
}

//If a > b, return 0xFF...FF, else return 0
static inline __m512i v8compare_gt(int a, int b)
{
    digit_t res = (uint64_t)(a > b) - 1;
    return _mm512_set1_epi64(res);
}

static inline __m512i v8compare_gt_split(int a, int b, int c, int d, SPLIT split)
{
    digit_t res1 = (uint64_t)(a > b) - 1;
    digit_t res2 = (uint64_t)(c > d) - 1;
    uint8_t mask = (1 << (split)) - 1;
    return _mm512_mask_blend_epi64(mask, _mm512_set1_epi64(res2), _mm512_set1_epi64(res1));
}

static void v8swap_vertices(v8vertex_t* A, v8vertex_t* B, const __m512i option)
{ // Swap vertices
  // If option = 0 then P <- P and Q <- Q, else if option = 0xFF...FF then P <- Q and Q <- P
    v8swap_points(&(A->A24), &(B->A24), option);
    v8swap_points(&(A->Ps), &(B->Ps), option);
    v8swap_points(&(A->Pt), &(B->Pt), option);
    v8swap_points(&(A->Qs), &(B->Qs), option);
    v8swap_points(&(A->Qt), &(B->Qt), option);
}

#endif