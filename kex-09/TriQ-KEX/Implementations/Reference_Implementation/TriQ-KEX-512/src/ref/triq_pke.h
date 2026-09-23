/**
 * @file triq_pke.h
 * @brief Header file for triq_pke.c
 */

#ifndef TRIQ_PKE_H
#define TRIQ_PKE_H

#include <stdint.h>
#include "parameters.h"
#include "parsing.h"

void triq_pke_keygen(uint8_t *ek_pke, uint8_t *dk_pke, uint8_t *seed);
void triq_pke_encrypt(ciphertext_pke_t *c_pke, const uint8_t *ek_pke, const uint64_t *m, const uint8_t *theta);
uint8_t triq_pke_decrypt(uint64_t *m, const uint8_t *dk_pke, const ciphertext_pke_t *c_pke);

#endif  // TRIQ_PKE_H
