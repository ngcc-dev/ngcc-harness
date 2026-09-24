/*
 *  SPDX-License-Identifier: MIT
 */

#ifndef SYDO_REF_VOLE_CHECK_H
#define SYDO_REF_VOLE_CHECK_H

#include "sydo.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

size_t sydo_ref_vole_check_proof_size(const sydo_ref_paramset_t* params);
size_t sydo_ref_vole_check_challenge_size(const sydo_ref_paramset_t* params);
size_t sydo_ref_vole_check_transcript_size(const sydo_ref_paramset_t* params);

bool sydo_ref_vole_check_sender(const sydo_ref_paramset_t* params, const uint8_t* u,
                                const uint8_t* v, const uint8_t* challenge, uint8_t* proof,
                                uint8_t* transcript, size_t transcript_len);
bool sydo_ref_vole_check_receiver(const sydo_ref_paramset_t* params, const uint8_t* q,
                                  const uint8_t* delta_bytes, const uint8_t* challenge,
                                  const uint8_t* proof, uint8_t* transcript,
                                  size_t transcript_len);

#endif
