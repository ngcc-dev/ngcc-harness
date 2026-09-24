/*
 *  SPDX-License-Identifier: MIT
 */

#ifndef SYDO_REF_HASH_H
#define SYDO_REF_HASH_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool sydo_ref_xof(uint8_t* out, size_t out_len, const uint8_t* msg, size_t msg_len);
bool sydo_ref_xof_with_suffix(uint8_t* out, size_t out_len, uint8_t* msg, size_t msg_len,
                              uint8_t suffix);
bool sydo_ref_xof3(uint8_t* out, size_t out_len, const void* part0, size_t part0_len,
                   const void* part1, size_t part1_len, const void* part2, size_t part2_len);

#endif
