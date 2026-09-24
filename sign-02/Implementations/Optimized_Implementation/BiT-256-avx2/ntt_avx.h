/*
 * Copyright (c) 2026 Hang Zhang.
 * State Key Laboratory of Cyberspace Security Defense,
 * Institute of Information Engineering, CAS
 * School of Cyber Security, University of Chinese Academy of Sciences
 */
#ifndef NTT_AVX_H
#define NTT_AVX_H

#include <stdint.h>
#include "params.h"
#include "consts.h"

/* Wrappers around assembly NTT functions with standardized names */
void ntt_avx_forward(int32_t a[BIT_N]);
void ntt_avx_inverse(int32_t a[BIT_N]);
void ntt_basemul_raw(int32_t *r, const int32_t *a, const int32_t *b);
void ntt_basemul_acc_raw(int32_t *r, const int32_t *a, const int32_t *b);
void ntt_montgomery_lift(int32_t *r);

static inline void ntt_forward(int32_t a[BIT_N])  { ntt_avx_forward(a); }
static inline void ntt_inverse(int32_t a[BIT_N])  { ntt_avx_inverse(a); }

#endif
