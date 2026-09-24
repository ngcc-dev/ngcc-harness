/*
 *  SPDX-License-Identifier: MIT
 */

#include "randomness.h"
#include "drng.h"

#include <limits.h>

int rand_bytes(uint8_t* dst, size_t len) {
  static DRNG_ctx drng;
  static int initialized = 0;
  unsigned char seed[SEEDLEN];

  if (!dst && len != 0) {
    return -1;
  }
  if (len > ULLONG_MAX / 8) {
    return -1;
  }
  if (!initialized) {
    for (size_t i = 0; i != sizeof(seed); ++i) {
      seed[i] = (unsigned char)(0xA5u ^ (unsigned char)i);
    }
    if (init_random_number(&drng, seed, sizeof(seed)) != 0) {
      return -1;
    }
    initialized = 1;
  }
  return get_random_number(&drng, dst, (unsigned long long)len * 8);
}
