// SPDX-FileCopyrightText: Copyright 2023 the SQIsign team. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

/*
 * This file is derived from curve_extras.h in the SQIsign (version 1.0) 
 * project (https://github.com/SQIsign/the-sqisign/tree/nist-v1),
 * which is licensed under the Apache-2.0 license.
 *
 * Modifications made:
 *   - Deleted some unused functions
 *   - Changed the data structure to make it suitable for vectorization
 */

#ifndef V8CURVE_EXTRAS_H
#define V8CURVE_EXTRAS_H

#include "v8ec.h"
#include "torsion_constants.h"

void v8copy_point(v8ec_point_t* P, v8ec_point_t const* Q);
void v8swap_points(v8ec_point_t* P, v8ec_point_t* Q, const __m512i option);
void v8ec_init(v8ec_point_t* P);
void v8xDBLv2(v8ec_point_t* Q, v8ec_point_t const* P, v8ec_point_t const* A24);
void v8xDBLADD(v8ec_point_t* R, v8ec_point_t* S, v8ec_point_t const* P, v8ec_point_t const* Q, v8ec_point_t const* PQ, v8ec_point_t const* A24);

#define v8xADD v8ec_add

#endif

