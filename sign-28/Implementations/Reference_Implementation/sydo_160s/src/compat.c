/*
 *  SPDX-License-Identifier: MIT
 */

#include "compat.h"

#include <stdint.h>
#include <stdlib.h>

typedef struct sydo_aligned_header_t {
  void* base;
} sydo_aligned_header_t;

void* sydo_aligned_alloc(size_t alignment, size_t size) {
  if (alignment < sizeof(void*)) {
    alignment = sizeof(void*);
  }
  if ((alignment & (alignment - 1u)) != 0u || size == 0u) {
    return NULL;
  }

  const size_t extra = alignment - 1u + sizeof(sydo_aligned_header_t);
  uint8_t* base = (uint8_t*)malloc(size + extra);
  if (!base) {
    return NULL;
  }

  uintptr_t aligned = (uintptr_t)(base + sizeof(sydo_aligned_header_t) + alignment - 1u);
  aligned &= ~((uintptr_t)alignment - 1u);
  sydo_aligned_header_t* header = ((sydo_aligned_header_t*)aligned) - 1;
  header->base = base;
  return (void*)aligned;
}

void sydo_aligned_free(void* ptr) {
  if (ptr) {
    sydo_aligned_header_t* header = ((sydo_aligned_header_t*)ptr) - 1;
    free(header->base);
  }
}

int sydo_timingsafe_bcmp(const void* a, const void* b, size_t len) {
  const unsigned char* p1 = (const unsigned char*)a;
  const unsigned char* p2 = (const unsigned char*)b;
  unsigned int res = 0;
  for (; len; --len, ++p1, ++p2) {
    res |= (unsigned int)(*p1 ^ *p2);
  }
  return (int)res;
}

void sydo_explicit_bzero(void* a, size_t len) {
  volatile unsigned char* p = (volatile unsigned char*)a;
  for (; len; ++p, --len) {
    *p = 0;
  }
}
