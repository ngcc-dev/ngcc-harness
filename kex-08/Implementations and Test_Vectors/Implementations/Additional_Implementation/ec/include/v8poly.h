// SPDX-FileCopyrightText: Copyright 2023 the SQIsign team. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

/*
 * This file is derived from poly.h in the SQIsign (version 1.0) 
 * project (https://github.com/SQIsign/the-sqisign/tree/nist-v1),
 * which is licensed under the Apache-2.0 license.
 *
 * Modifications made:
 *   - Changed the data structure to make it suitable for vectorization
 */

#ifndef _V8_POLY_H_
#define _V8_POLY_H_

#include <v8fp2.h>

typedef v8fp2_t *v8poly; // Polynomials are arrays of coeffs over Fq, lowest degree first

void v8poly_mul(v8poly h, const v8poly f, const int lenf, const v8poly g, const int leng);
void v8poly_mul_low(v8poly h, const int n, const v8poly f, const int lenf, const v8poly g, const int leng);
void v8poly_mul_middle(v8poly h, const v8poly g, const int leng, const v8poly f, const int lenf);
void v8poly_mul_selfreciprocal(v8poly h, const v8poly g, const int leng, const v8poly f, const int lenf);

void v8product_tree(v8poly H[], int DEG[], const int root, const v8poly F[], const int LENF, const int n);
void v8product_tree_LENFeq2(v8poly H[], int DEG[], const int root, const v8fp2_t F[][2], const int n);
void v8product_tree_LENFeq3(v8poly H[], int DEG[], const int root, const v8fp2_t F[][3], const int n);
void v8product_tree_selfreciprocal(v8poly H[], int DEG[], const int root, const v8poly F[], const int LENF, const int n);
void v8product_tree_selfreciprocal_LENFeq3(v8poly H[], int DEG[], const int root, const v8fp2_t F[][3], const int n);
void v8clear_tree(v8poly H[], const int root, const int n);

void v8product(v8fp2_t *c, const v8fp2_t F[], const int n);

void v8reciprocal(v8poly h, v8fp2_t *c, const v8poly f, const int lenf, const int n);
void v8poly_redc(v8poly h, const v8poly g, const int leng, const v8poly f, const int lenf,const v8poly f_inv, const v8fp2_t c);
void v8reciprocal_tree(v8poly *R, v8fp2_t *A, const int leng, const v8poly H[], const int DEG[], const int root, const int n);
void v8multieval_unscaled(v8fp2_t REM[], const v8poly g, const int leng, const v8poly R[], const v8fp2_t A[], const v8poly H[], const int DEG[], const int root, const int n);
void v8multieval_scaled(v8fp2_t REM[], const v8poly G, const v8poly H[], const int DEG[], const int root, const int n);

#endif