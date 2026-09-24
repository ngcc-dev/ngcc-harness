/*
 *  SPDX-License-Identifier: MIT
 */

#if defined(HAVE_CONFIG_H)
#include <config.h>
#endif

#include "hash.h"

#include "compat.h"
#include "lib/auxfunc.h"

#include <stdlib.h>
#include <string.h>

bool sydo_ref_xof(uint8_t* out, size_t out_len, const uint8_t* msg, size_t msg_len) {
  static const uint8_t empty_msg = 0;
  const unsigned long long output_len_bits = (unsigned long long)out_len * 8u;

  if (msg_len == 0) {
    msg = &empty_msg;
  }
  return pseudoXOF(output_len_bits, msg, (unsigned long long)msg_len * 8u, out) == 0;
}

bool sydo_ref_xof_with_suffix(uint8_t* out, size_t out_len, uint8_t* msg, size_t msg_len,
                              uint8_t suffix) {
  const uint8_t saved = msg[msg_len];
  bool ok;

  msg[msg_len] = suffix;
  ok = sydo_ref_xof(out, out_len, msg, msg_len + 1u);
  msg[msg_len] = saved;
  return ok;
}

bool sydo_ref_xof3(uint8_t* out, size_t out_len, const void* part0, size_t part0_len,
                   const void* part1, size_t part1_len, const void* part2, size_t part2_len) {
  const size_t input_len = part0_len + part1_len + part2_len;
  uint8_t* input = (uint8_t*)malloc(input_len == 0 ? 1u : input_len);
  size_t off = 0;
  bool ok;

  if (!input) {
    return false;
  }
  if (part0_len != 0) {
    memcpy(input + off, part0, part0_len);
    off += part0_len;
  }
  if (part1_len != 0) {
    memcpy(input + off, part1, part1_len);
    off += part1_len;
  }
  if (part2_len != 0) {
    memcpy(input + off, part2, part2_len);
  }

  ok = sydo_ref_xof(out, out_len, input, input_len);
  sydo_explicit_bzero(input, input_len == 0 ? 1u : input_len);
  free(input);
  return ok;
}
