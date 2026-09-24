/*
 *  SPDX-License-Identifier: MIT
 */

#ifndef LYNX_MATRICES_H
#define LYNX_MATRICES_H

#include <stddef.h>
#include <stdint.h>

#include "macros.h"

SIG_BEGIN_C_DECL

#define LYNX_MAX_CSP 512
#define LYNX_MAX_WORDS (LYNX_MAX_CSP / 64)

typedef struct lynx_t {
  unsigned int lambda;
  unsigned int words;
  uint64_t L0[LYNX_MAX_CSP][LYNX_MAX_WORDS];
  uint64_t L1[LYNX_MAX_CSP][LYNX_MAX_WORDS];
  uint64_t L2[LYNX_MAX_CSP][LYNX_MAX_WORDS];
  uint64_t L3[LYNX_MAX_CSP][LYNX_MAX_WORDS];
  uint8_t C0[LYNX_MAX_CSP / 8];
  uint8_t C1[LYNX_MAX_CSP / 8];
  uint8_t C2[LYNX_MAX_CSP / 8];
} lynx_t;

int lynx_init(lynx_t* mats, unsigned int lambda, const uint8_t* seed,
              size_t seed_len);
int lynx_is_valid(const lynx_t* mats);
int lynx_seed_is_valid(unsigned int lambda, const uint8_t* seed, size_t seed_len);

SIG_END_C_DECL

#endif
