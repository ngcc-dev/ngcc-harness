/*
 *  SPDX-License-Identifier: MIT
 */

#ifndef SYDO_REF_FIELD_H
#define SYDO_REF_FIELD_H

#include "sydo.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

uint32_t sydo_ref_field_modulus(const sydo_ref_paramset_t* params);
void sydo_ref_field_zero(uint8_t* out, size_t len);
void sydo_ref_field_one(uint8_t* out, size_t len);
void sydo_ref_field_xor(uint8_t* out, const uint8_t* in, size_t len);
void sydo_ref_field_add(uint8_t* out, const uint8_t* a, const uint8_t* b, size_t len);
void sydo_ref_field_mul(uint8_t* out, const uint8_t* a, const uint8_t* b,
                        const sydo_ref_paramset_t* params);
void sydo_ref_field_mul_bit(uint8_t* out, const uint8_t* a, uint8_t bit, size_t len);
bool sydo_ref_field_is_zero(const uint8_t* a, size_t len);

#endif
