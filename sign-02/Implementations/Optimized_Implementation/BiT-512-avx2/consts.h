/*
 * Copyright (c) 2026 Hang Zhang.
 * State Key Laboratory of Cyberspace Security Defense,
 * Institute of Information Engineering, CAS
 * School of Cyber Security, University of Chinese Academy of Sciences
 */
#ifndef CONSTS_H
#define CONSTS_H

#include <stdint.h>
#include "align.h"

/* Scalar broadcast vectors (8× int32 filling one ymm register). */
extern const int32_t q[8];        /* modulus                   */
extern const int32_t qhalf[8];    /* floor(q/2)                */
extern const int32_t qinv[8];     /* q^{-1} for Montgomery     */
extern const int32_t ninv_h[8];   /* N^{-1} high half          */
extern const int32_t ninv_l[8];   /* N^{-1} low half           */
extern const int32_t r2_h[8];     /* R^2 mod q  (Montgomery lift) */

/* NTT zeta tables (256 scalars each, broadcast per butterfly step).
 * zfwd_h/l: forward NTT zetas and zetas × q^{-1}
 * zinv_h/l: inverse NTT zetas and zetas × q^{-1} */
extern const int32_t zfwd_h[256];
extern const int32_t zfwd_l[256];
extern const int32_t zinv_h[256];
extern const int32_t zinv_l[256];

/* Intra-register forward butterfly tables (t=4,2 via shuffle; t=1 per-lane).
 * fit_h/l: 1024 entries (8 per block × 128 blocks), for t=4 and t=2 stages.
 * fit1_h:  1024 entries, for the t=1 stage. */
extern const int32_t fit_h[1024];
extern const int32_t fit_l[1024];
extern const int32_t fit1_h[1024];

/* Intra-register inverse butterfly tables (t=2,4 via shuffle; t=1 per-lane). */
extern const int32_t iit_h[1024];
extern const int32_t iit_l[1024];
extern const int32_t iit1_h[1024];

/* S1 rejection-sampling lookup tables (keyed by input byte). */
extern const int32_t rej_s1_vals[256][4];
extern const uint8_t rej_s1_cnt[256];

/* AVX2 unpacking shuffle tables for triangular y-sampler. */
extern const int8_t unpack14_shuf[32];

#endif /* CONSTS_H */
