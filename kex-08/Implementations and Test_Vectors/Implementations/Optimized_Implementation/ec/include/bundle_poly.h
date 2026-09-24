// SPDX-FileCopyrightText: Copyright 2023 the SQIsign team. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

/*
 * This file is derived from poly.h in the SQIsign (version 1.0) 
 * project (https://github.com/SQIsign/the-sqisign/tree/nist-v1),
 * which is licensed under the Apache-2.0 license.
 *
 * Modifications made:
 *   - Changed the data structure to SoA
 */

#ifndef _BUNDLE_POLY_H_
#define _BUNDLE_POLY_H_

#include <bundle_fp2.h>

typedef bundlefp2_t *bundlepoly; // Polynomials are arrays of coeffs over Fq, lowest degree first

void bundlepoly_mul(bundlepoly h, const bundlepoly f, const int lenf, const bundlepoly g, const int leng, int len);
void bundlepoly_mul_low(bundlepoly h, const int n, const bundlepoly f, const int lenf, const bundlepoly g, const int leng, int len);
void bundlepoly_mul_middle(bundlepoly h, const bundlepoly g, const int leng, const bundlepoly f, const int lenf, int len);
void bundlepoly_mul_selfreciprocal(bundlepoly h, const bundlepoly g, const int leng, const bundlepoly f, const int lenf, int len);

void bundleproduct_tree(bundlepoly H[], int DEG[], const int root, const bundlepoly F[], const int LENF, const int n, int len);
void bundleproduct_tree_LENFeq2(bundlepoly H[], int DEG[], const int root, const bundlefp2_t F[][2], const int n, int len);
void bundleproduct_tree_LENFeq3(bundlepoly H[], int DEG[], const int root, const bundlefp2_t F[][3], const int n, int len);
void bundleproduct_tree_selfreciprocal(bundlepoly H[], int DEG[], const int root, const bundlepoly F[], const int LENF, const int n, int len);
void bundleproduct_tree_selfreciprocal_LENFeq3(bundlepoly H[], int DEG[], const int root, const bundlefp2_t F[][3], const int n, int len);
void bundleclear_tree(bundlepoly H[], const int root, const int n);

void bundleproduct(bundlefp2_t *c, const bundlefp2_t F[], const int n, int len);

void bundlereciprocal(bundlepoly h, bundlefp2_t *c, const bundlepoly f, const int lenf, const int n, int len);
void bundlepoly_redc(bundlepoly h, const bundlepoly g, const int leng, const bundlepoly f, const int lenf,const bundlepoly f_inv, const bundlefp2_t c, int len);
void bundlereciprocal_tree(bundlepoly *R, bundlefp2_t *A, const int leng, const bundlepoly H[], const int DEG[], const int root, const int n, int len);
void bundlemultieval_unscaled(bundlefp2_t REM[], const bundlepoly g, const int leng, const bundlepoly R[], const bundlefp2_t A[], const bundlepoly H[], const int DEG[], const int root, const int n, int len);
void bundlemultieval_scaled(bundlefp2_t REM[], const bundlepoly G, const bundlepoly H[], const int DEG[], const int root, const int n, int len);

#endif