/*
 *  SPDX-License-Identifier: MIT
 */

#ifndef SYDO_REF_PRIMITIVES_H
#define SYDO_REF_PRIMITIVES_H

#include "sydo.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void sydo_ref_block_zero(uint8_t* out, size_t len);
void sydo_ref_block_xor(uint8_t* out, const uint8_t* a, const uint8_t* b, size_t len);
void sydo_ref_block_and(uint8_t* out, const uint8_t* a, const uint8_t* b, size_t len);
void sydo_ref_block_set_all_8(uint8_t* out, size_t len, uint8_t byte);
void sydo_ref_block_add32(uint8_t* out, const uint8_t* a, const uint8_t* b, size_t len);
void sydo_ref_block_set_low_high32(uint8_t* out, size_t len, uint32_t low, uint32_t high);

bool sydo_ref_blake2s_512_bc_eval(uint8_t* out, const uint8_t* key, const uint8_t* input);
bool sydo_ref_tree_prg_eval(const sydo_ref_paramset_t* params, uint8_t* out,
                            const uint8_t* key, const uint8_t* iv, uint32_t tweak,
                            uint32_t counter);
bool sydo_ref_tree_prg_expand_2(const sydo_ref_paramset_t* params, uint8_t* out,
                                const uint8_t* key, const uint8_t* iv, uint32_t tweak);
bool sydo_ref_leaf_hash_prg(const sydo_ref_paramset_t* params, uint8_t* leaf_seed,
                            uint8_t* leaf_hash, const uint8_t* key, const uint8_t* iv,
                            uint32_t tweak);
void sydo_ref_prg_cache_clear(void);

#endif
