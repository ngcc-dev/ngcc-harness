/*
 *  SPDX-License-Identifier: MIT
 */

#ifndef SYDO_REF_MACS_H
#define SYDO_REF_MACS_H

#include "sydo.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

size_t sydo_ref_witness_blocks(const sydo_ref_paramset_t* params);
size_t sydo_ref_quicksilver_rows_padded_extended(const sydo_ref_paramset_t* params);

void sydo_ref_compute_macs(const sydo_ref_paramset_t* params, const uint8_t* vole_cols,
                           uint8_t* mac_rows, size_t rows_padded);
bool sydo_ref_compute_macs_extended_prover(const sydo_ref_paramset_t* params, const uint8_t* v,
                                           uint8_t* macs_extended);
bool sydo_ref_compute_macs_extended_verifier(const sydo_ref_paramset_t* params, uint8_t* q,
                                             const uint8_t* correction_padded,
                                             const uint8_t* delta_bytes,
                                             uint8_t* macs_extended);

#endif
