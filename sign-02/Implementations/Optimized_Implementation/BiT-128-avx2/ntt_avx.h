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
#include "reduce.h"

/* ── Public NTT API — identical surface to BiT-256/BiT-512 (only the
 *    coefficient type differs: int16_t here because q < 2^16 allows the
 *    faster 16-bit Kyber-style arithmetic; 256/512 use int32_t). ──────── */
void ntt_avx_forward(int16_t a[BIT_N]);
void ntt_avx_inverse(int16_t a[BIT_N]);
void ntt_basemul_raw(int16_t *r, const int16_t *a, const int16_t *b);
void ntt_basemul_acc_raw(int16_t *r, const int16_t *a, const int16_t *b);
void ntt_montgomery_lift(int16_t *r);

static inline void ntt_forward(int16_t a[BIT_N])  { ntt_avx_forward(a); }
static inline void ntt_inverse(int16_t a[BIT_N])  { ntt_avx_inverse(a); }

/* ── Internal assembly entry points (ntt.S); not part of the unified API ── */
void ntt_avx_forward_stages(int16_t a[BIT_N]);
void basemul_avx2(int16_t c[BIT_N], const int16_t a[BIT_N], const int16_t b[BIT_N]);
void basemul_avx2_acc(int16_t r[BIT_N], const int16_t a[BIT_N], const int16_t b[BIT_N]);

#endif
