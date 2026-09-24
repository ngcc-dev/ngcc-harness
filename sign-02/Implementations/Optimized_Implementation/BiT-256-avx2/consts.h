/*
 * Copyright (c) 2026 Hang Zhang.
 * State Key Laboratory of Cyberspace Security Defense,
 * Institute of Information Engineering, CAS
 * School of Cyber Security, University of Chinese Academy of Sciences
 */
#ifndef CONSTS_H
#define CONSTS_H
#include "params.h"

#ifndef __ASSEMBLER__
#include <stdint.h>
#include "align.h"

/*
 * BiT-256 NTT constants — separate RIP-relative arrays, formally aligned with
 * BiT-128 / BiT-512.  `_h` = the value itself (vpmuldq "high" operand);
 * `_l` = value*qinv (Montgomery "low" operand).
 */
extern const int32_t q[8];        /* modulus broadcast                       */
extern const int32_t qhalf[8];    /* floor(q/2)  (inverse-NTT signed reduce) */
extern const int32_t qinv[8];     /* q^{-1} mod 2^32                         */
extern const int32_t ninv_h[8];   /* N^{-1}        (inverse-NTT final scale) */
extern const int32_t ninv_l[8];   /* N^{-1}*qinv                            */

/* NTT zeta tables (576 each: indices 1..63 single-broadcast at runtime via
 * vpbroadcastd, 64..575 pre-broadcast x8 for the intra-register levels). */
extern const int32_t zfwd_h[576]; /* forward zeta              */
extern const int32_t zfwd_l[576]; /* forward zeta*qinv         */
extern const int32_t zinv_h[576]; /* inverse zeta              */
extern const int32_t zinv_l[576]; /* inverse zeta*qinv         */

/* Degree-2 basemul roots, [r, q-r] in even lanes (512). */
extern const int32_t bmul[512];

/* S1 rejection-sampling lookup tables (keyed by input byte). */
extern const int32_t rej_s1_vals[256][4];
extern const uint8_t rej_s1_cnt[256];

/* AVX2 unpacking shuffle tables for triangular y-sampler. */
extern const int8_t unpack12_shuf[32];
extern const int8_t unpack13_shuf[32];
#endif
#endif
