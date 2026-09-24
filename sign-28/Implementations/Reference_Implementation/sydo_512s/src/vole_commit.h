/*
 *  SPDX-License-Identifier: MIT
 */

#ifndef SYDO_REF_VOLE_COMMIT_H
#define SYDO_REF_VOLE_COMMIT_H

#include "sydo.h"
#include "bavc.h"

#include <stdbool.h>
#include <stdint.h>

bool sydo_ref_vole_commit(const sydo_ref_paramset_t* params, const uint8_t* seed,
                          const uint8_t* iv, sydo_ref_bavc_t* bavc, uint8_t* u, uint8_t* v,
                          uint8_t* commitment, uint8_t* check);
bool sydo_ref_vole_reconstruct(const sydo_ref_paramset_t* params, const uint8_t* iv, uint8_t* q,
                               const uint8_t* delta_bytes, const uint8_t* commitment,
                               const uint8_t* opening, uint8_t* check);

#endif
