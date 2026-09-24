// SPDX-FileCopyrightText: Copyright 2023 the SQIsign team. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

/*
 * This file is derived from fp2.h in the SQIsign (version 1.0) 
 * project (https://github.com/SQIsign/the-sqisign/tree/nist-v1),
 * which is licensed under the Apache-2.0 license.
 *
 * Modified to adapt to OSIDH-LD
 */

#ifndef FP2_H
#define FP2_H

#include "fp.h"

// Structure for representing elements in GF(p^2)
typedef struct fp2_t {
    fp_t re, im;
} fp2_t;

extern const fp2_t fp2_1;

void fp2_set(fp2_t* x, const digit_t val);
void fp2_setone(fp2_t *x);
bool fp2_is_zero(const fp2_t* a);
bool fp2_is_equal(const fp2_t* a, const fp2_t* b);
void fp2_copy(fp2_t* x, const fp2_t* y);
fp2_t fp2_non_residue();
void fp2_add(fp2_t* x, const fp2_t* y, const fp2_t* z);
void fp2_sub(fp2_t* x, const fp2_t* y, const fp2_t* z);
void fp2_neg(fp2_t* x, const fp2_t* y);
void fp2_mul(fp2_t* x, const fp2_t* y, const fp2_t* z);
void fp2_sqr(fp2_t* x, const fp2_t* y);
void fp2_inv(fp2_t* x);
bool fp2_is_square(const fp2_t* x);
void fp2_frob(fp2_t* x, const fp2_t* y);
void fp2_sqrt(fp2_t* x);
void fp2_tomont(fp2_t* x, const fp2_t* y);
void fp2_frommont(fp2_t* x, const fp2_t* y);
int fp2_cmp(fp2_t* x, fp2_t* y);
void fp2_print(fp2_t const a);
void fp2_encode(void *dst, const fp2_t *a);
uint32_t fp2_decode(fp2_t *d, const void *src);
void fp2_batched_inv(fp2_t *x, int len);

#endif
