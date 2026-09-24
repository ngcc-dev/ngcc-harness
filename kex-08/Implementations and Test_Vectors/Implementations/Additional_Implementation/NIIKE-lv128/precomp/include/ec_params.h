// SPDX-FileCopyrightText: Copyright 2023 the SQIsign team. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

/*
 * This file is derived from ec_params.h in the SQIsign (version 1.0) 
 * project (https://github.com/SQIsign/the-sqisign/tree/nist-v1),
 * which is licensed under the Apache-2.0 license.
 *
 * Modified to adapt to OSIDH-LD
 */

#ifndef EC_PARAMS_H
#define EC_PARAMS_H

#include <fp_constants.h>

#define scaled 0	// unscaled (0) or scaled (1) remainder tree approach
#define gap 83

#define P_LEN 31
#define M_LEN 18

static digit_t p_plus_minus_bitlength[P_LEN + M_LEN] =
        { 2, 3, 3, 4, 5, 5, 5, 5, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 9, 4, 5, 6, 6, 6, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8 };

static int sizeI[] = {
        0, 1, 1, 2, 2, 2, 3, 3, 3, 3, 3, 4, 4, 4, 5, 5, 6, 6, 6, 5, 6, 6, 6, 6, 6, 8, 8, 8, 8, 8, 8, 3, 2, 3, 4, 5, 4, 4, 6, 6, 6, 8, 7, 8, 8, 8, 8, 8, 10
};
static int sizeJ[] = {
        0, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 6, 6, 6, 5, 5, 5, 7, 7, 8, 1, 2, 3, 3, 3, 4, 4, 4, 4, 5, 4, 6, 5, 5, 6, 6, 6, 6
};
static int sizeK[] = {
        1, 0, 1, 1, 1, 3, 2, 3, 2, 3, 5, 2, 3, 7, 1, 4, 0, 5, 6, 6, 5, 8, 2, 3, 6, 1, 3, 10, 2, 7, 6, 0, 0, 0, 5, 0, 1, 4, 2, 3, 3, 5, 2, 9, 15, 0, 3, 9, 0
};

#define sI_max 10
#define sJ_max 8
#define sK_max 41

#define ceil_log_sI_max 4
#define ceil_log_sJ_max 3

#endif