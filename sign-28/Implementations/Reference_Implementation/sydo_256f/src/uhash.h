/*
 *  SPDX-License-Identifier: MIT
 */

#ifndef SYDO_REF_UNIVERSAL_HASH_H
#define SYDO_REF_UNIVERSAL_HASH_H

#include "sydo.h"

#include <stddef.h>
#include <stdint.h>

typedef struct sydo_ref_uhash_secpar_key_t {
  uint8_t key_pows[2][64];
} sydo_ref_uhash_secpar_key_t;

typedef struct sydo_ref_uhash_secpar_state_t {
  uint8_t state[128];
  int pow;
  size_t coefficients;
} sydo_ref_uhash_secpar_state_t;

typedef struct sydo_ref_uhash64_key_t {
  uint64_t key_pows[2];
  uint64_t key_pow_times_a64;
} sydo_ref_uhash64_key_t;

typedef struct sydo_ref_uhash64_state_t {
  uint8_t state[16];
  int pow;
} sydo_ref_uhash64_state_t;

typedef struct sydo_ref_uhash_secpar64_key_t {
  uint64_t key;
} sydo_ref_uhash_secpar64_key_t;

typedef struct sydo_ref_uhash_secpar64_state_t {
  uint8_t state[64];
} sydo_ref_uhash_secpar64_state_t;

void sydo_ref_field_mul_wide(uint8_t* out, const uint8_t* a, const uint8_t* b,
                             const sydo_ref_paramset_t* params);
void sydo_ref_field_reduce_wide(uint8_t* out, const uint8_t* in,
                                const sydo_ref_paramset_t* params);
void sydo_ref_field_muladd_wide(uint8_t* acc, const uint8_t* a, const uint8_t* b,
                                const sydo_ref_paramset_t* params);
void sydo_ref_field_muladd(uint8_t* acc, const uint8_t* a, const uint8_t* b,
                           const sydo_ref_paramset_t* params);

void sydo_ref_uhash_secpar_key_init(sydo_ref_uhash_secpar_key_t* key,
                                    const uint8_t* key_bytes,
                                    const sydo_ref_paramset_t* params);
void sydo_ref_uhash_secpar_state_init(sydo_ref_uhash_secpar_state_t* state,
                                      size_t num_coefficients);
void sydo_ref_uhash_secpar_update(sydo_ref_uhash_secpar_state_t* state,
                                  const sydo_ref_uhash_secpar_key_t* key,
                                  const uint8_t* input,
                                  const sydo_ref_paramset_t* params);
void sydo_ref_uhash_secpar_finalize(const sydo_ref_uhash_secpar_state_t* state, uint8_t* out,
                                    const sydo_ref_paramset_t* params);

void sydo_ref_uhash_secpar64_key_init(sydo_ref_uhash_secpar64_key_t* key, uint64_t key64);
void sydo_ref_uhash_secpar64_state_init(sydo_ref_uhash_secpar64_state_t* state,
                                        size_t num_coefficients);
void sydo_ref_uhash_secpar64_update(sydo_ref_uhash_secpar64_state_t* state,
                                    const sydo_ref_uhash_secpar64_key_t* key,
                                    const uint8_t* input,
                                    const sydo_ref_paramset_t* params);
void sydo_ref_uhash_secpar64_finalize(const sydo_ref_uhash_secpar64_state_t* state,
                                      uint8_t* out, const sydo_ref_paramset_t* params);

void sydo_ref_uhash64_key_init(sydo_ref_uhash64_key_t* key, uint64_t key64);
void sydo_ref_uhash64_state_init(sydo_ref_uhash64_state_t* state, size_t num_coefficients);
void sydo_ref_uhash64_update(sydo_ref_uhash64_state_t* state,
                             const sydo_ref_uhash64_key_t* key, uint64_t input);
uint64_t sydo_ref_uhash64_finalize(const sydo_ref_uhash64_state_t* state);

uint64_t sydo_ref_load64_le(const uint8_t* in);
void sydo_ref_store64_le(uint8_t* out, uint64_t v);

#endif
