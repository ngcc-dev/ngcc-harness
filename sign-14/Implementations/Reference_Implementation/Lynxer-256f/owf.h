/*
 *  SPDX-License-Identifier: MIT
 */

#ifndef OWF_H
#define OWF_H

#include "macros.h"

#include <stdint.h>

SIG_BEGIN_C_DECL

void owf_lynx_128(const uint8_t* key, const uint8_t* input, uint8_t* output);
void owf_lynx_160(const uint8_t* key, const uint8_t* input, uint8_t* output);
void owf_lynx_192(const uint8_t* key, const uint8_t* input, uint8_t* output);
void owf_lynx_256(const uint8_t* key, const uint8_t* input, uint8_t* output);
void owf_lynx_384(const uint8_t* key, const uint8_t* input, uint8_t* output);
void owf_lynx_512(const uint8_t* key, const uint8_t* input, uint8_t* output);

// Witness extension for LYNX (placeholder until custom OWF is implemented)
struct sig_paramset_t;
void lynx_extend_witness(uint8_t* w, const uint8_t* key, const uint8_t* in,
                         const struct sig_paramset_t* params);

#define owf_160 owf_lynx_160
#define owf_256 owf_lynx_256
#define owf_384 owf_lynx_384
#define owf_512 owf_lynx_512

SIG_END_C_DECL

#endif
