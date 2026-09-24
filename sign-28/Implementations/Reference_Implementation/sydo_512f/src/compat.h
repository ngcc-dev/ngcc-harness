/*
 *  SPDX-License-Identifier: MIT
 */

#ifndef SYDO_COMPAT_H
#define SYDO_COMPAT_H

#include "macros.h"

#include <limits.h>
#include <stddef.h>
#include <stdint.h>

SYDO_BEGIN_C_DECL

void sydo_aligned_free(void* ptr);
void* sydo_aligned_alloc(size_t alignment, size_t size);
int sydo_timingsafe_bcmp(const void* a, const void* b, size_t len);
void sydo_explicit_bzero(void* a, size_t len);

SYDO_END_C_DECL

#include "endian_compat.h"
#include <string.h>

#define sydo_mempcpy(dst, src, len) ((void*)(((uint8_t*)memcpy((dst), (src), (len))) + (len)))

static inline uint8_t rotl8(uint8_t n, unsigned int c) {
  const unsigned int mask = (CHAR_BIT * sizeof(n) - 1);
  c &= mask;
  return (uint8_t)((n << c) | (n >> ((-c) & mask)));
}

static inline uint8_t rotr8(uint8_t n, unsigned int c) {
  const unsigned int mask = (CHAR_BIT * sizeof(n) - 1);
  c &= mask;
  return (uint8_t)((n >> c) | (n << ((-c) & mask)));
}

static inline uint32_t rotl32(uint32_t n, unsigned int c) {
  const unsigned int mask = (CHAR_BIT * sizeof(n) - 1);
  c &= mask;
  return (n << c) | (n >> ((-c) & mask));
}

static inline uint32_t rotr32(uint32_t n, unsigned int c) {
  const unsigned int mask = (CHAR_BIT * sizeof(n) - 1);
  c &= mask;
  return (n >> c) | (n << ((-c) & mask));
}

static inline uint8_t parity8(uint8_t n) {
  n ^= (uint8_t)(n >> 4);
  n ^= (uint8_t)(n >> 2);
  n ^= (uint8_t)(n >> 1);
  return (uint8_t)(n & 1u);
}

#if !defined(__cplusplus)
#include <assert.h>
#if !defined(static_assert)
#define static_assert _Static_assert
#endif
#endif

#endif
