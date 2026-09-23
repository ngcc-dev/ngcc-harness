// SPDX-FileCopyrightText: Copyright 2023 the SQIsign team. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

/*
 * This file is derived from curve_extras.h in the SQIsign (version 1.0) 
 * project (https://github.com/SQIsign/the-sqisign/tree/nist-v1),
 * which is licensed under the Apache-2.0 license.
 *
 * Modifications made:
 *   - Deleted some unused functions
 *   - Changed the data structure to SoA
 */

#ifndef BUNDLE_CURVE_EXTRAS_H
#define BUNDLE_CURVE_EXTRAS_H

#include "bundle_ec.h"
#include "torsion_constants.h"

void bundlecopy_point(bundleec_point_t* P, bundleec_point_t const* Q, int len);
void bundleswap_points(bundleec_point_t* P, bundleec_point_t* Q, const digit_t option, int len);
void bundleec_init(bundleec_point_t* P, int len);
void bundlexDBLv2(bundleec_point_t* Q, bundleec_point_t const* P, bundleec_point_t const* A24, int len);
void bundlexADD(bundleec_point_t* R, bundleec_point_t const* P, bundleec_point_t const* Q, bundleec_point_t const* PQ, int len);
void bundlexDBLADD(bundleec_point_t* R, bundleec_point_t* S, bundleec_point_t const* P, bundleec_point_t const* Q, bundleec_point_t const* PQ, bundleec_point_t const* A24, int len);

// #define bundlexADD bundleec_add

#endif

