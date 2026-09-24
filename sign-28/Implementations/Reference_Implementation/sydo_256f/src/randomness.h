/*
 *  SPDX-License-Identifier: MIT
 */

#ifndef RANDOMNESS_H
#define RANDOMNESS_H

#include "macros.h"

#include <stddef.h>
#include <stdint.h>

SYDO_BEGIN_C_DECL

int rand_bytes(uint8_t* dst, size_t num_bytes);

SYDO_END_C_DECL

#endif
