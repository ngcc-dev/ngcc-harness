/*
 * Copyright (c) 2026 Hang Zhang.
 * State Key Laboratory of Cyberspace Security Defense,
 * Institute of Information Engineering, CAS
 * School of Cyber Security, University of Chinese Academy of Sciences
 */

#ifndef BIT_NTT_H
#define BIT_NTT_H

#include <stdint.h>
#include "params.h"
#include "reduce.h"

void ntt_forward(int32_t a[BIT_N]);
void ntt_inverse(int32_t a[BIT_N]);
void ntt_basemul_raw(int32_t r[BIT_N], const int32_t a[BIT_N], const int32_t b[BIT_N]);
void ntt_basemul_acc_raw(int32_t r[BIT_N], const int32_t a[BIT_N], const int32_t b[BIT_N]);
void ntt_montgomery_lift(int32_t r[BIT_N]);


#endif
