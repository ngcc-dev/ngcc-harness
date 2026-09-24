/**
 * @file parsing.h
 * @brief Header file for parsing.c
 */

#ifndef triq_PARSING_H
#define triq_PARSING_H

#include <immintrin.h>
#include <stdint.h>
#include "data_structures.h"
#include "parameters.h"

void triq_dk_pke_from_string(__m256i *y, const uint8_t *dk_pke);
void triq_ek_pke_from_string(uint64_t *h, uint64_t *s, const uint8_t *ek_pke);

void triq_c_kem_to_string(uint8_t *ct, const ciphertext_kem_t *c_kem);
void triq_c_kem_from_string(ciphertext_pke_t *c_pke, uint8_t *salt, const uint8_t *ct);

#endif  // triq_PARSING_H
