/**
 * \file random_source.h
 * \brief Random source abstraction layer header (DRNG backend)
 *
 * Adapts the rbc library's random source interface to the API_PKC
 * deterministic random number generator (SM3-DRBG).
 * Supports two modes:
 *   - RANDOM_SOURCE_PRNG: uses the global drng_algorithm context
 *   - RANDOM_SOURCE_SEEDEXP: uses a locally seeded context
 */

#ifndef RANDOM_SOURCE_H
#define RANDOM_SOURCE_H

#include "drng.h"
#include <stddef.h>

#define RANDOM_SOURCE_PRNG 0    // PRNG mode (uses global context)
#define RANDOM_SOURCE_SEEDEXP 1 // Seed expander mode (uses local context)

// Global DRNG context (defined and initialized in KAT_KEM.c)
extern DRNG_ctx drng_algorithm;

typedef struct {
  DRNG_ctx ctx; // Local DRNG context
  int type;     // Random source type
  int seeded;   // Whether seed has been initialized
} random_source;

/// @brief Initialize random source
void random_source_init(random_source *o, int type);
/// @brief Seed the local DRNG context
void random_source_seed(random_source *o, unsigned char *seed);
/// @brief Seed the local DRNG context with an explicit seed length in bytes
void random_source_seed_with_len(random_source *o, const unsigned char *seed,
                                 size_t seed_len_bytes);
/// @brief Generate random bytes of specified length
void random_source_get_bytes(random_source *e, unsigned char *buffer,
                             size_t len);
/// @brief Clear random source state
void random_source_clear(random_source *o);

#endif
