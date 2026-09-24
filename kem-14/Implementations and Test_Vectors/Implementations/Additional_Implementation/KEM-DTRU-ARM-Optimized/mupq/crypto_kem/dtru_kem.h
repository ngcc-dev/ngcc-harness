// SPDX-License-Identifier: Apache-2.0 or CC0-1.0
#ifndef MUPQ_CRYPTO_KEM_DTRU_KEM_H
#define MUPQ_CRYPTO_KEM_DTRU_KEM_H

#include "KEM_AlgorithmInstance.h"
#include "drng.h"
#include "params.h"

#define DTRU_TEST_SEED_BYTES 64

DRNG_ctx drng_algorithm;

static void dtru_init_drng(const unsigned char *seed)
{
  init_random_number(&drng_algorithm, seed, DTRU_TEST_SEED_BYTES);
}

#endif
