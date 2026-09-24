// SPDX-FileCopyrightText: Copyright 2023 the SQIsign team. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

/*
 * This file is derived from curve_extras.h in the SQIsign (version 1.0) 
 * project (https://github.com/SQIsign/the-sqisign/tree/nist-v1),
 * which is licensed under the Apache-2.0 license.
 *
 * Modifications made:
 *   - Deleted some unused functions
 */

#ifndef CURVE_EXTRAS_H
#define CURVE_EXTRAS_H

#include "ec.h"
#include "torsion_constants.h"

void copy_point(ec_point_t* P, ec_point_t const* Q);
void swap_points(ec_point_t* P, ec_point_t* Q, const digit_t option);
void ec_init(ec_point_t* P);
void xDBLv2(ec_point_t* Q, ec_point_t const* P, ec_point_t const* A24);
void xDBLADD(ec_point_t* R, ec_point_t* S, ec_point_t const* P, ec_point_t const* Q, ec_point_t const* PQ, ec_point_t const* A24);

#define xADD ec_add

#endif

