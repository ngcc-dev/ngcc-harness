/*
 *  SPDX-License-Identifier: MIT
 */

#ifndef SYDO_REF_QUICKSILVER_H
#define SYDO_REF_QUICKSILVER_H

#include "sydo.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool sydo_ref_qs_prove_mask_only(const sydo_ref_paramset_t* params, const uint8_t* witness_ext,
                                 const uint8_t* macs_extended, uint8_t* proof,
                                 uint8_t* check);
bool sydo_ref_qs_verify_mask_only(const sydo_ref_paramset_t* params, const uint8_t* delta,
                                  const uint8_t* macs_extended, const uint8_t* proof,
                                  uint8_t* check);

bool sydo_ref_qs_prove_rsd(const sydo_ref_paramset_t* params, const uint8_t* pk,
                           const uint8_t* challenge, const uint8_t* witness_ext,
                           const uint8_t* macs_extended, uint8_t* proof, uint8_t* check);

bool sydo_ref_qs_verify_rsd(const sydo_ref_paramset_t* params, const uint8_t* pk,
                            const uint8_t* challenge, const uint8_t* delta,
                            const uint8_t* macs_extended, const uint8_t* proof,
                            uint8_t* check);

#endif
