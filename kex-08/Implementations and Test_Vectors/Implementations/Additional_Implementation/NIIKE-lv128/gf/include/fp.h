// SPDX-FileCopyrightText: Copyright 2023 the SQIsign team. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

/*
 * This file is derived from fp.h in the SQIsign (version 1.0) 
 * project (https://github.com/SQIsign/the-sqisign/tree/nist-v1),
 * which is licensed under the Apache-2.0 license.
 *
 * Modified to adapt to OSIDH-LD
 */

#ifndef FP_H
#define FP_H

//////////////////////////////////////////////// NOTE: this is placed here for now
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <tutil.h>
#include <fp_constants.h>

typedef digit_t fp_t[NWORDS_FIELD];  // Datatype for representing field elements

void fp_set(fp_t* x, const digit_t val);
bool fp_is_equal(const fp_t* a, const fp_t* b);
bool fp_is_zero(const fp_t* a);
void fp_copy(fp_t* out, const fp_t* a);
void fp_add(fp_t* out, const fp_t* a, const fp_t* b);
void fp_sub(fp_t* out, const fp_t* a, const fp_t* b);
void fp_neg(fp_t* out, const fp_t* a);
void fp_sqr(fp_t* out, const fp_t* a);
void fp_mul(fp_t* out, const fp_t* a, const fp_t* b);
void fp_inv(fp_t* x);
bool fp_is_square(const fp_t* a);
void fp_sqrt(fp_t* a);
void fp_tomont(fp_t* out, const fp_t* a);
void fp_frommont(fp_t* out, const fp_t* a);
void fp_mont_setone(fp_t* out);
void fp_encode(void *dst, const fp_t *a);
uint32_t fp_decode(fp_t *d, const void *src);

#endif