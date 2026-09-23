// SPDX-FileCopyrightText: 2026 The Project OSIDH-LD Authors
// SPDX-License-Identifier: Apache-2.0

#ifndef PROTOCOL_CONSTANTS_H
#define PROTOCOL_CONSTANTS_H

#include <protocols.h>

// extern const orient_cycle_t base_cycle;

// extern const fp2_t alpha_sq;
// extern const fp_t ell_mont[];
// extern const fp2_t Ms_mont;
// extern const fp2_t Mt_mont;
// extern const fp2_t MsxMt_mont;

// extern const digit_t mscalar_s[];
// extern const digit_t mscalar_t[];
// extern const int mscalar_s_bits[];
// extern const int mscalar_s_limbs[];
// extern const int mscalar_t_bits[];
// extern const int mscalar_t_limbs[];

extern const size_t degree_s_num;
// extern const digit_t degree_s_value[];
// extern const int degree_s_bits[];
extern const size_t degree_t_num;
// extern const digit_t degree_t_value[];
// extern const int degree_t_bits[];
extern const int strategy_s[];
extern const int strategy_t[];
extern const size_t stack_volume;

// extern const digit_t strascalar_s[];
// extern const int strascalar_s_bits[];
// extern const int strascalar_s_limbs[];

// extern const digit_t strascalar_t[];
// extern const int strascalar_t_bits[];
// extern const int strascalar_t_limbs[];

static const int secretkey_mask[P_LEN + M_LEN] = {0xFF, 0x3F, 0x3F, 0x7F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F};

#endif