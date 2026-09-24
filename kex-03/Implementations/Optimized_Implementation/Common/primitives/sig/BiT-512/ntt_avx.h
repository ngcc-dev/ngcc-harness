/*
 * BiT-1024 AVX2 NTT interface — matches avx2-128 pattern.
 * All functions operate on int32_t arrays in [0, q) or Montgomery
 * domain as documented.
 */
#ifndef NTT_AVX_H
#define NTT_AVX_H

#include <stdint.h>
#include "params.h"
#include "reduce.h"

/* ---- AVX2 entry points ---- */
void ntt_avx_forward(int32_t a[BIT_N]);
void ntt_avx_inverse(int32_t a[BIT_N]);
void ntt_pointwise_mul_raw(int32_t r[BIT_N], const int32_t a[BIT_N], const int32_t b[BIT_N]);
void ntt_pointwise_acc_raw(int32_t r[BIT_N], const int32_t a[BIT_N], const int32_t b[BIT_N]);
void ntt_montgomery_lift(int32_t r[BIT_N]);

/* ---- Unified wrappers (matches poly layer call convention) ---- */
static inline void ntt_forward(int32_t a[BIT_N])  { ntt_avx_forward(a); }
static inline void ntt_inverse(int32_t a[BIT_N])  { ntt_avx_inverse(a); }

#endif
