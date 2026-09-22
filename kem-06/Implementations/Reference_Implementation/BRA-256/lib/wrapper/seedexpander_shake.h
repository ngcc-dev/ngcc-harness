
#ifndef SEEDEXPANDER_SHAKE_H
#define SEEDEXPANDER_SHAKE_H

#include "KeccakHash.h"
#include "SimpleFIPS202.h"

#define DOMAIN_SEPARATOR_SEEDEXPANDER 0xE


typedef struct seedexpander_shake_t {
    Keccak_HashInstance state;
} seedexpander_shake_t;


static inline void seedexpander_shake_init(seedexpander_shake_t* seedexpander_shake, const uint8_t* seed, size_t seed_size, const uint8_t* salt, size_t salt_size) {
  uint8_t domain_separator = DOMAIN_SEPARATOR_SEEDEXPANDER;

  Keccak_HashInitialize_SHAKE128(&seedexpander_shake->state);
  Keccak_HashUpdate(&seedexpander_shake->state, seed, seed_size << 3);
  if(salt != NULL) Keccak_HashUpdate(&seedexpander_shake->state, salt, salt_size << 3);
  Keccak_HashUpdate(&seedexpander_shake->state, &domain_separator, sizeof(uint8_t) << 3);
  Keccak_HashFinal(&seedexpander_shake->state, NULL);
}

static inline void seedexpander_shake_get_bytes(seedexpander_shake_t* seedexpander_shake, uint8_t* output, size_t output_size) {
  Keccak_HashSqueeze(&seedexpander_shake->state, output, output_size << 3);
}



#if defined(SHAKE_TIMES4)

#include "KeccakHashtimes4.h"

typedef struct seedexpander_shake_x4_t {
    Keccak_HashInstancetimes4 state;
} seedexpander_shake_x4_t;


static inline void seedexpander_shake_x4_init(seedexpander_shake_x4_t* seedexpander_shake, const uint8_t** seed, size_t seed_size, const uint8_t** salt, size_t salt_size) {
  uint8_t domain_separator = DOMAIN_SEPARATOR_SEEDEXPANDER;
  const uint8_t *domain_separator_x4[] = {&domain_separator, &domain_separator, &domain_separator, &domain_separator};

  Keccak_HashInitializetimes4_SHAKE128(&seedexpander_shake->state);
  Keccak_HashUpdatetimes4(&seedexpander_shake->state, seed, seed_size << 3);
  if(salt != NULL) Keccak_HashUpdatetimes4(&seedexpander_shake->state, salt, salt_size << 3);
  Keccak_HashUpdatetimes4(&seedexpander_shake->state, domain_separator_x4, sizeof(uint8_t) << 3);
  Keccak_HashFinaltimes4(&seedexpander_shake->state, NULL);
}

static inline void seedexpander_shake_x4_get_bytes(seedexpander_shake_x4_t* seedexpander_shake, uint8_t** output, size_t output_size) {
  Keccak_HashSqueezetimes4(&seedexpander_shake->state, output, output_size << 3);
}

#endif

#endif

