/*
 *  SPDX-License-Identifier: MIT
 */

#ifndef SYDO_REF_RSD_H
#define SYDO_REF_RSD_H

#include "sydo.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

size_t sydo_ref_rsd_position_bits(const sydo_ref_paramset_t* params);
size_t sydo_ref_rsd_position_bytes(const sydo_ref_paramset_t* params);
size_t sydo_ref_rsd_y_storage_bytes(const sydo_ref_paramset_t* params);

bool sydo_ref_rsd_sample_positions(uint8_t* x_pos_out, const sydo_ref_paramset_t* params,
                                   const uint8_t* seed_sk);
bool sydo_ref_rsd_positions_to_witness(uint8_t* witness, const sydo_ref_paramset_t* params,
                                       const uint8_t* x_positions);
bool sydo_ref_rsd_witness_to_positions(uint8_t* x_positions, const sydo_ref_paramset_t* params,
                                       const uint8_t* witness);
bool sydo_ref_rsd_compute_y(uint8_t* y_storage, const sydo_ref_paramset_t* params,
                            const uint8_t* seed_pk, const uint8_t* x_positions);
bool sydo_ref_rsd_witness_matches_public_key(const sydo_ref_paramset_t* params, const uint8_t* pk,
                                             const uint8_t* witness);
bool sydo_ref_rsd_extend_witness(uint8_t* extended_witness, size_t extended_witness_len,
                                 const sydo_ref_paramset_t* params, const uint8_t* witness);

#endif
