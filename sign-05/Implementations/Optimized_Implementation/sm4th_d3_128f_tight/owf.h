/*
 * OWF functions (SM4-only for sm4th_d3_128f_tight).
 */

#ifndef OWF_H
#define OWF_H

#include <stdint.h>

void owf_sm4_128(const uint8_t* key, const uint8_t* input, uint8_t* output);
void owf_sm4_em_128(const uint8_t* key, const uint8_t* input, uint8_t* output);

#define sm4th_d3_128f_tight_owf owf_sm4_128

#endif
