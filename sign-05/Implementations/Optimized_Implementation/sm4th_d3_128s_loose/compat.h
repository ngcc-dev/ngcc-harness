/*
* several compatibility functions for C11 and OpenBSD/FreeBSD adapted from sm4th
*/

#ifndef COMPAT_H
#define COMPAT_H

#include "macros.h"
#include <stddef.h>

/**
 * Some aligned_alloc compatbility implementations require custom free
 * functions, so we provide one too.
 */
void aligned_free(void* ptr);
/**
 * Compatibility implementation of aligned_alloc from ISO C 2011.
 */
void* aligned_alloc(size_t alignment, size_t size);


#include "endian_compat.h"

/**
 * Compatibility implementation of timingsafe_bcmp from OpenBSD 4.9 and FreeBSD 12.0.
 */
int timingsafe_bcmp(const void* a, const void* b, size_t len);


/**
 * Compatibility implementation of explicit_bzero
 */
void explicit_bzero(void* a, size_t len);


#include <string.h>
/* Provide prototype for `mempcpy`. Some build configurations force-include
 * `fallbacks.h` which already defines a `static inline mempcpy`. To avoid
 * duplicate symbol definitions we only declare the function here and let the
 * fallback or a platform libc provide the definition.
 */
void* mempcpy(void* dst, const void* src, size_t len);

#include <limits.h>
#include <stdint.h>

/* helper functions for left and right rotations of bytes */
static inline uint8_t rotl8(uint8_t n, unsigned int c) {
  const unsigned int mask = (CHAR_BIT * sizeof(n) - 1);
  c &= mask;
  return (n << c) | (n >> ((-c) & mask));
}

static inline uint8_t rotr8(uint8_t n, unsigned int c) {
  const unsigned int mask = (CHAR_BIT * sizeof(n) - 1);
  c &= mask;
  return (n >> c) | (n << ((-c) & mask));
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

/* helper functions for byte parity: 0 if even number of bits are set, 1 if odd number of bts are
 * set */
static inline uint8_t parity8(uint8_t n) {
  n ^= n >> 4;
  n ^= n >> 2;
  n ^= n >> 1;
  return !((~n) & 1);
}

#endif
