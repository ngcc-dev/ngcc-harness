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

/* Forward NTT butterfly zeta tables, 16-bit Montgomery form.
 * Each entry is 16 copies (one ymm) of the same zeta value, packed for
 * the BFLY4 macro (16 int16_t per AVX2 register).
 * zfwd_h: the zeta value itself   (used with vpmulhw)
 * zfwd_l: zeta × (-q^{-1} mod 2^16)  (used with vpmullw) */
extern const int16_t zfwd_h[1008];
extern const int16_t zfwd_l[1008];

/* Inverse NTT butterfly zeta tables (same layout as forward). */
extern const int16_t zinv_h[1008];
extern const int16_t zinv_l[1008];

/* Degree-2 CRT basemul zeta tables — odd-indexed group roots only.
 * Each entry: 0 in even lane, root in odd lane (as required by BMUL macro).
 * bmul_h_odd: zeta values
 * bmul_l_odd: zeta × (-q^{-1} mod 2^16) */
extern const int16_t bmul_h_odd[256];
extern const int16_t bmul_l_odd[256];

/* S1 rejection-sampling lookup tables (keyed by input byte).
 * rej_s1_vals[b][4]: up to 4 accepted ternary values {-1,0,1} from byte b.
 * rej_s1_cnt[b]:     how many of those 4 slots are valid. */
extern const int16_t rej_s1_vals[256][4];
extern const uint8_t rej_s1_cnt[256];

#endif /* CONSTS_H */
