/* SPDX-License-Identifier: MIT */
#ifndef OWF_H
#define OWF_H

#include <stdint.h>

void owf_ublock_256(const uint8_t* key, const uint8_t* input, uint8_t* output);
void owf_ublock_em_256(const uint8_t* key, const uint8_t* input, uint8_t* output);

#define ublockith_em_d3_256s_owf owf_ublock_em_256

#endif
