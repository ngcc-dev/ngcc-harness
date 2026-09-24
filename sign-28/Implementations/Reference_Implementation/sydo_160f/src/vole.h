/*
 *  SPDX-License-Identifier: MIT
 */

#ifndef SYDO_REF_VOLE_H
#define SYDO_REF_VOLE_H

#include "sydo.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum { SYDO_REF_VOLE_BLOCK_BYTES = 16 };

void sydo_ref_vole_sender(const sydo_ref_paramset_t* params, unsigned int k, const uint8_t* keys,
                          const uint8_t* iv, uint32_t tweak, const uint8_t* u, uint8_t* v,
                          uint8_t* c);
void sydo_ref_vole_receiver(const sydo_ref_paramset_t* params, unsigned int k,
                            const uint8_t* keys, const uint8_t* iv, uint32_t tweak,
                            const uint8_t* c, uint8_t* q, const uint8_t* delta);
void sydo_ref_vole_apply_correction(const sydo_ref_paramset_t* params, size_t row_blocks,
                                    size_t cols, const uint8_t* c, uint8_t* q,
                                    const uint8_t* delta);

#endif
