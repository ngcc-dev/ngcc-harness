/**
 * \file random_source.c
 * \brief Random source abstraction layer implementation (DRNG backend)
 *
 * Adapts the rbc core layer's random source interface to the API_PKC
 * DRNG (SM3-DRBG). Uses the global drng_algorithm context when unseeded,
 * and a locally seeded context after seeding.
 */

#include "random_source.h"
#include <stdlib.h>
#include <string.h>

#define RANDOM_SOURCE_DEFAULT_SEED_BYTES 64

/**
 * @brief Initialize random source, set type and zero out context
 */
void random_source_init(random_source *o, int type) {
  o->type = type;
  o->seeded = 0;
  memset(&o->ctx, 0, sizeof(DRNG_ctx));
}

/**
 * @brief Seed the local DRNG context with an explicit seed length
 */
void random_source_seed_with_len(random_source *o, const unsigned char *seed,
                                 size_t seed_len_bytes) {
  init_random_number(&o->ctx, seed, seed_len_bytes);
  o->seeded = 1;
}

/**
 * @brief Seed the local DRNG context with the NGCC default seed length
 */
void random_source_seed(random_source *o, unsigned char *seed) {
  random_source_seed_with_len(o, seed, RANDOM_SOURCE_DEFAULT_SEED_BYTES);
}

/**
 * @brief Generate random bytes
 *
 * Uses the global drng_algorithm context when unseeded (PRNG mode),
 * and the locally seeded context when seeded (seed expander mode).
 * The DRNG interface requires length in bits, hence len * 8.
 */
void random_source_get_bytes(random_source *o, unsigned char *buffer,
                             size_t len) {
  if (o->type == RANDOM_SOURCE_PRNG && !o->seeded) {
    // Use global context (initialized by KAT_KEM.c)
    get_random_number(&drng_algorithm, buffer, (unsigned long long)len * 8);
  } else {
    // Use locally seeded context
    get_random_number(&o->ctx, buffer, (unsigned long long)len * 8);
  }
}

/**
 * @brief Clear random source state, zero out context
 */
void random_source_clear(random_source *o) {
  memset(&o->ctx, 0, sizeof(DRNG_ctx));
  o->seeded = 0;
}
