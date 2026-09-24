/**
 * @file mito.h
 * @brief Header file for mito.c
 */

#ifndef MITO_MITO_H
#define MITO_MITO_H

#include <stdint.h>
#include "parameters.h"
#include "parsing.h"

void mito_pke_keygen(uint8_t *ek_pke, uint8_t *dk_pke, uint8_t *seed);
void mito_pke_encrypt(ciphertext_pke_t *c_pke, const uint8_t *ek_pke, const uint64_t *m, const uint8_t *theta);
uint8_t mito_pke_decrypt(uint64_t *m, const uint8_t *dk_pke, const ciphertext_pke_t *c_pke);

#endif  // MITO_MITO_H
