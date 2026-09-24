/*
 * OWF function aliases for SM4th variants.
 */

#ifndef OWF_H
#define OWF_H

#include <stdint.h>

void owf_sm4_128(const uint8_t* key, const uint8_t* input, uint8_t* output);
void owf_sm4_em_128(const uint8_t* key, const uint8_t* input, uint8_t* output);

#define sm4th_em_d2_128f_loose_owf owf_sm4_em_128
#define sm4th_em_d2_128f_owf owf_sm4_em_128

#endif
