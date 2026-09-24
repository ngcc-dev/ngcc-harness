// Copyright 2025 LG Electronics, Inc. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <mpc.h>

#define MOD(a, b) ((((a) % (b)) + (b)) % (b))
#define N_PRECISION_BITS 260
// #define N_PRECISION_BITS 512
#define ROUND_DOWN MPC_RNDDN

#define ZERO_DOUBLE (1e-78)
#ifndef MINAL_ANALYSIS_FOR_LWR
#define N_SUPPORT_DIST_2D 2*2048
#else
#define N_SUPPORT_DIST_2D 4*2048
#endif

static __inline__ int center_mod(int x, int m) {
    x = MOD(x, m);
    if (x <= m - x) {
        return x;
    }
    return - (m - x);
}

#define MAX_CODE_BETA 1500
