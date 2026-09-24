#ifndef HASHKDF_H
#define HASHKDF_H

#include <stddef.h>
#include <stdint.h>

#if defined(USE_SHA3) == defined(USE_ICCS)
#error "Define exactly one of USE_SHA3 or USE_ICCS"
#endif

#ifdef USE_SHA3
#include "fips202.h"
#endif

#ifdef USE_ICCS
#include "auxfunc.h"
#endif

typedef struct
{
#ifdef USE_SHA3
  keccak_state shake;
  int finalized;
#else
  uint8_t *input;
  size_t input_len;
  size_t input_capacity;
  uint8_t *output_cache;
  size_t output_capacity;
  size_t output_offset;
  int finalized;
  int failed;
#endif
} trine_xof_state;

typedef struct
{
#ifdef USE_SHA3
  keccak_state shake;
  int finalized;
#else
  uint8_t *input;
  size_t input_len;
  size_t input_capacity;
  int finalized;
  int failed;
#endif
} trine_hash_state;

int trine_xof_init(trine_xof_state *state);
int trine_xof_absorb(
    trine_xof_state *state,
    const uint8_t *input,
    size_t input_len);
int trine_xof_finalize(trine_xof_state *state);
int trine_xof_squeeze(
    trine_xof_state *state,
    uint8_t *output,
    size_t output_len);
void trine_xof_release(trine_xof_state *state);

int trine_xof_once(
    uint8_t *output,
    size_t output_len,
    const uint8_t *input,
    size_t input_len);

int trine_hash_init(trine_hash_state *state);
int trine_hash_absorb(
    trine_hash_state *state,
    const uint8_t *input,
    size_t input_len);
int trine_hash_finalize(
    trine_hash_state *state,
    uint8_t *output,
    size_t output_len);
void trine_hash_release(trine_hash_state *state);

#endif
