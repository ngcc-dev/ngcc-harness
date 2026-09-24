// SPDX-FileCopyrightText: 2026 The Project OSIDH-LD Authors
// SPDX-License-Identifier: Apache-2.0

#ifndef V8FP2_H
#define V8FP2_H

#include "v8fp.h"
#include "fp2.h"

typedef struct v8fp2_t {
    v8fp_t re, im;
} v8fp2_t;

void v8fp2_pack8(v8fp2_t* x, const fp2_t* a1, const fp2_t* a2, const fp2_t* a3, const fp2_t* a4, const fp2_t* a5, const fp2_t* a6, const fp2_t* a7, const fp2_t* a8);
void v8fp2_unpack8(fp2_t* a1, fp2_t* a2, fp2_t* a3, fp2_t* a4, fp2_t* a5, fp2_t* a6, fp2_t* a7, fp2_t* a8, const v8fp2_t* x);
void v8fp2_pack7(v8fp2_t* x, const fp2_t* a1, const fp2_t* a2, const fp2_t* a3, const fp2_t* a4, const fp2_t* a5, const fp2_t* a6, const fp2_t* a7);
void v8fp2_unpack7(fp2_t* a1, fp2_t* a2, fp2_t* a3, fp2_t* a4, fp2_t* a5, fp2_t* a6, fp2_t* a7, const v8fp2_t* x);
void v8fp2_pack6(v8fp2_t* x, const fp2_t* a1, const fp2_t* a2, const fp2_t* a3, const fp2_t* a4, const fp2_t* a5, const fp2_t* a6);
void v8fp2_unpack6(fp2_t* a1, fp2_t* a2, fp2_t* a3, fp2_t* a4, fp2_t* a5, fp2_t* a6, const v8fp2_t* x);
void v8fp2_pack5(v8fp2_t* x, const fp2_t* a1, const fp2_t* a2, const fp2_t* a3, const fp2_t* a4, const fp2_t* a5);
void v8fp2_unpack5(fp2_t* a1, fp2_t* a2, fp2_t* a3, fp2_t* a4, fp2_t* a5, const v8fp2_t* x);
void v8fp2_pack4(v8fp2_t* x, const fp2_t* a1, const fp2_t* a2, const fp2_t* a3, const fp2_t* a4);
void v8fp2_unpack4(fp2_t* a1, fp2_t* a2, fp2_t* a3, fp2_t* a4, const v8fp2_t* x);
void v8fp2_pack3(v8fp2_t* x, const fp2_t* a1, const fp2_t* a2, const fp2_t* a3);
void v8fp2_unpack3(fp2_t* a1, fp2_t* a2, fp2_t* a3, const v8fp2_t* x);
void v8fp2_pack2(v8fp2_t* x, const fp2_t* a1, const fp2_t* a2);
void v8fp2_unpack2(fp2_t* a1, fp2_t* a2, const v8fp2_t* x);

void v8fp2_add(v8fp2_t* x, const v8fp2_t* y, const v8fp2_t* z);
void v8fp2_sub(v8fp2_t* x, const v8fp2_t* y, const v8fp2_t* z);
void v8fp2_neg(v8fp2_t* x, const v8fp2_t* y);
void v8fp2_mul(v8fp2_t* x, const v8fp2_t* y, const v8fp2_t* z);
void v8fp2_sqr(v8fp2_t* x, const v8fp2_t* y);
void v8fp2_set(v8fp2_t* x, const digit_t val);
void v8fp2_copy(v8fp2_t* x, const v8fp2_t* y);
void v8fp2_mont_setone(v8fp2_t* out);
void v8fp2_setzero(v8fp2_t* out);
void v8fp2_swap(v8fp2_t* P, v8fp2_t* Q, const __m512i option);

#endif