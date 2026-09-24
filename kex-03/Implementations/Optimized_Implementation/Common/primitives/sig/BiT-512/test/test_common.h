/*
Copyright (c) 2026 Hang Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences
*/
#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include <stddef.h>
#include <stdint.h>

static void test_fill_seed(uint8_t *seed, size_t len, unsigned int domain, unsigned int iter) {
  for (size_t i = 0; i < len; i++) {
    seed[i] = (uint8_t)(0xA5U + 17U * domain + 31U * iter + (unsigned int)i);
  }
}

static void test_fill_message(uint8_t *msg, size_t len, unsigned int domain, unsigned int iter) {
  uint32_t x = 0x9E3779B9U ^ (domain * 0x85EBCA6BU) ^ (iter * 0xC2B2AE35U);

  for (size_t i = 0; i < len; i++) {
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    msg[i] = (uint8_t)(x & 0xFFU);
  }
}

#endif
#include "../drng.h"
DRNG_ctx drng_algorithm;    /* defined+seeded by KAT_SIG.c in KAT builds */
static DRNG_ctx test_rng;
