#ifndef NTT_AVX_H
#define NTT_AVX_H

#include <stdint.h>
#include "params.h"
#include "consts.h"

/* Wrappers around assembly NTT functions with standardized names */
void ntt_avx_forward(int32_t a[BIT_N]);
void ntt_avx_inverse(int32_t a[BIT_N]);
void ntt_pointwise_mul_raw(int32_t *r, const int32_t *a, const int32_t *b);
void ntt_pointwise_acc_raw(int32_t *r, const int32_t *a, const int32_t *b);
void ntt_montgomery_lift(int32_t *r);

static inline void ntt_forward(int32_t a[BIT_N])  { ntt_avx_forward(a); }
static inline void ntt_inverse(int32_t a[BIT_N])  { ntt_avx_inverse(a); }

#endif
