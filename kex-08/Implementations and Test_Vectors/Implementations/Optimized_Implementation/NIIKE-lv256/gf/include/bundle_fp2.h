// SPDX-FileCopyrightText: 2026 The Project OSIDH-LD Authors
// SPDX-License-Identifier: Apache-2.0

#ifndef BUNDLEFP2_H
#define BUNDLEFP2_H

#include "fp2.h"
#include "protocol_setup.h"

typedef struct bundlefp2_t {
    fp2_t list[CYCLE_LENGTH];
} bundlefp2_t;

void bundlefp2_add(bundlefp2_t* x, const bundlefp2_t* y, const bundlefp2_t* z, int len);
void bundlefp2_sub(bundlefp2_t* x, const bundlefp2_t* y, const bundlefp2_t* z, int len);
void bundlefp2_neg(bundlefp2_t* x, const bundlefp2_t* y, int len);
void bundlefp2_mul(bundlefp2_t* x, const bundlefp2_t* y, const bundlefp2_t* z, int len);
void bundlefp2_sqr(bundlefp2_t* x, const bundlefp2_t* y, int len);
void bundlefp2_inv(bundlefp2_t* x, int len);
void bundlefp2_sqrt(bundlefp2_t* x, int len);
void bundlefp2_set(bundlefp2_t* x, const digit_t val, int len);
void bundlefp2_from_single(bundlefp2_t* x, const fp2_t* y, int len);
void bundlefp2_copy(bundlefp2_t* x, const bundlefp2_t* y, int len);
void bundlefp2_mont_setone(bundlefp2_t* out, int len);
void bundlefp2_setzero(bundlefp2_t* out, int len);
void bundlefp2_shift(bundlefp2_t* x, const bundlefp2_t* y, int len);
bool bundlefp2_is_equal(const bundlefp2_t* a, const bundlefp2_t* b, int len);
void bundlefp2_is_zero_mask(uint32_t* res, const bundlefp2_t *a, int len);
void bundlefp2_swap(bundlefp2_t* P, bundlefp2_t* Q, const digit_t option, int len);
void bundlefp2_select(bundlefp2_t *d, const bundlefp2_t *a0, const bundlefp2_t *a1, uint32_t* ctl, int len);
void bundlefp2_mul_single_fp(bundlefp2_t* x, const bundlefp2_t* y, const fp_t z, int len);
void bundlefp2_mul_all_to_one(fp2_t* x, const bundlefp2_t* y, int len);

#endif