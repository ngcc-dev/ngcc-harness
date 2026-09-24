/**
 * @file parsing.h
 * @brief Header file for parsing.c
 */

#ifndef QUBE_PARSING_H
#define QUBE_PARSING_H

//#include <immintrin.h>
#include <stdint.h>
#include "data_structures.h"
#include "parameters.h"


void qube_dk_pke_from_string(uint64_t *y, const uint8_t *dk_pke);
void qube_ek_pke_from_string(uint64_t *h1, uint64_t *h2, uint64_t *s1, uint64_t *s2, const uint8_t *ek_pke);

void qube_c_kem_to_string(uint8_t *ct, const ciphertext_kem_t *c_kem);
void qube_c_kem_from_string(ciphertext_pke_t *c_pke, uint8_t *salt, const uint8_t *ct);

#endif  // QUBE_PARSING_H
