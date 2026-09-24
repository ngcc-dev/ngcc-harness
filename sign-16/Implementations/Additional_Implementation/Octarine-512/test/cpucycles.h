#ifndef CPUCYCLES_H
#define CPUCYCLES_H

#include <stdint.h>

static inline uint64_t cpucycles(void) {
  uint64_t result;

  __asm__ volatile ("rdtsc; shlq $32,%%rdx; orq %%rdx,%%rax"
    : "=a" (result) : : "%rdx");

  return result;
}

uint64_t cpucycles_overhead(void);

#endif
