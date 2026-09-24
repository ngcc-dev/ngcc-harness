/**
 * @file parsing.h
 * @brief Header file for parsing.c
 */

#ifndef MITO_PARSING_H
#define MITO_PARSING_H

#include <stdint.h>
#include <string.h>
#include "data_structures.h"

void mito_dk_pke_from_string(uint64_t y[PARAM_L][VEC_N_SIZE_64], const uint8_t* dk_pke);
void mito_ek_pke_from_string(uint64_t h[PARAM_L][VEC_N_SIZE_64], uint64_t s[PARAM_L][VEC_N_SIZE_64], const uint8_t* ek_pke);

void mito_c_kem_to_string(uint8_t *ct, const ciphertext_kem_t *c_kem);
void mito_c_kem_from_string(ciphertext_pke_t *c_pke, uint8_t *salt, const uint8_t *ct);

#endif  // MITO_PARSING_H
