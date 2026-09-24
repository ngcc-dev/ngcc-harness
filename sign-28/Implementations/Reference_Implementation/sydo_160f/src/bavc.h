/*
 *  SPDX-License-Identifier: MIT
 */

#ifndef SYDO_REF_BAVC_H
#define SYDO_REF_BAVC_H

#include "sydo.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct sydo_ref_bavc_t {
  uint8_t* tree;
  uint8_t* leaves;
  uint8_t* hashed_leaves;
  size_t commit_leaves;
  size_t commit_nodes;
  size_t hash_len;
} sydo_ref_bavc_t;

void sydo_ref_bavc_init(sydo_ref_bavc_t* bavc);
void sydo_ref_bavc_clear(sydo_ref_bavc_t* bavc);
bool sydo_ref_bavc_commit(const sydo_ref_paramset_t* params, sydo_ref_bavc_t* bavc,
                          const uint8_t* seed, const uint8_t* iv);
bool sydo_ref_bavc_open(const sydo_ref_paramset_t* params, const sydo_ref_bavc_t* bavc,
                        const uint8_t* delta_bytes, uint8_t* opening);
bool sydo_ref_bavc_verify(const sydo_ref_paramset_t* params, const uint8_t* opening,
                          const uint8_t* delta_bytes, const uint8_t* iv, uint8_t* leaves,
                          uint8_t* hashed_leaves);

#endif
