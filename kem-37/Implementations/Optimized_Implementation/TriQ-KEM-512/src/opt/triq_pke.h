/**
 * @file triq_pke.h
 * @brief Header file for TriQ.c
 */

#ifndef TRIQ_PKE_H
#define TRIQ_PKE_H

#include <immintrin.h>
#include <stdint.h>
#include "parameters.h"
#include "parsing.h"

/**
 * @brief Number of 256-bit vector words in the VEC_N_256 representation.
 *
 * VEC_N_256_SIZE_64 is the total count of 64-bit words required for the
 * vector length; shifting right by 2 divides by 4 to convert 64-bit words
 * into 256-bit vector words.
 */
#define VEC_N_256_NUM_WORDS (VEC_N_256_SIZE_64 >> 2)

void triq_pke_keygen(uint8_t *ek_pke, uint8_t *dk_pke, uint8_t *seed);
void triq_pke_encrypt(ciphertext_pke_t *c_pke, const uint8_t *ek_pke, const uint64_t *m, const uint8_t *theta);
uint8_t triq_pke_decrypt(uint64_t *m, const uint8_t *dk_pke, const ciphertext_pke_t *c_pke);

#endif  // TRIQ_PKE_H
