// SPDX-FileCopyrightText: Copyright 2023 the SQIsign team. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

/*
 * This file is derived from ec_params.h in the SQIsign (version 1.0) 
 * project (https://github.com/SQIsign/the-sqisign/tree/nist-v1),
 * which is licensed under the Apache-2.0 license.
 *
 * Modified to adapt to NIIKE
 */
                        
#ifndef EC_PARAMS_H
#define EC_PARAMS_H

#include <fp_constants.h>

#define scaled 0	// unscaled (0) or scaled (1) remainder tree approach
#define gap 83

#define P_LEN 54
#define M_LEN 30

static digit_t p_plus_minus_bitlength[P_LEN + M_LEN] =
        { 2, 2, 3, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 10, 10, 10, 3, 5, 6, 6, 6, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 9, 9, 10, 10, 10 };

static int sizeI[] = {
        0, 0, 1, 2, 3, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 6, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 10, 10, 10, 10, 11, 12, 14, 15, 1, 3, 3, 3, 5, 4, 4, 4, 5, 5, 6, 7, 8, 7, 7, 7, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 11, 12, 12, 12
};
static int sizeJ[] = {
        0, 0, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 10, 12, 14, 15, 1, 2, 3, 3, 3, 4, 4, 4, 4, 5, 6, 6, 6, 7, 7, 7, 7, 7, 8, 8, 8, 8, 9, 9, 9, 9, 10, 12, 12, 12
};
static int sizeK[] = {
        0, 1, 1, 1, 0, 0, 1, 3, 3, 0, 5, 2, 5, 4, 1, 0, 0, 1, 3, 6, 3, 5, 8, 9, 2, 3, 9, 11, 5, 6, 11, 13, 1, 2, 4, 7, 0, 3, 6, 12, 9, 11, 12, 14, 6, 11, 3, 6, 9, 11, 10, 8, 1, 18, 0, 2, 2, 3, 0, 1, 3, 7, 4, 4, 6, 2, 0, 0, 1, 7, 8, 13, 7, 10, 13, 2, 3, 12, 14, 17, 1, 5, 12, 15
};

#define sI_max 15
#define sJ_max 15
#define sK_max 41

#define ceil_log_sI_max 4
#define ceil_log_sJ_max 4

#endif