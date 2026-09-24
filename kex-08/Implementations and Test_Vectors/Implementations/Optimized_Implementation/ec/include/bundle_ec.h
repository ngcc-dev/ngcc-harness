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
 *   - Changed the data structure to SoA
 */

#ifndef BUNDLE_EC_H
#define BUNDLE_EC_H

#include <bundle_fp2.h>
#include <ec_params.h>
#include "ec.h"

typedef struct bundleec_point_t {
    bundlefp2_t x;
    bundlefp2_t z;
} bundleec_point_t;

typedef struct bundleec_curve_t {
    bundlefp2_t A;
    bundlefp2_t C; ///< cannot be 0
} bundleec_curve_t;

void bundlexMULv2(bundleec_point_t* Q, bundleec_point_t const* P, digit_t const* k, const int kbits, bundleec_point_t const* A24, int len);
void bundleec_point_shift(bundleec_point_t *res, const bundleec_point_t *inp, int len);
void bundleec_point_select(bundleec_point_t *res, const bundleec_point_t *inp1, const bundleec_point_t *inp2, uint32_t* ctl, int len);
void bundleec_point_transposition(ec_point_t res[], const bundleec_point_t *inp, int len);
void get_point_from_bundle(ec_point_t *res, const bundleec_point_t *inp, size_t location);
void set_point_to_bundle(bundleec_point_t *res, const ec_point_t *inp, size_t location);
void bundleec_ker_mul(bundlefp2_t* pi_X, bundlefp2_t* pi_Z, const bundleec_point_t* ker, const bundleec_point_t* A24, int ell, int len);

#endif