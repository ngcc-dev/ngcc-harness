/**
 * @brief Header file for PKE_AlgorithmInstance.c
 */

#ifndef PKE_ALGORITHMINSTANCE_H
#define PKE_ALGORITHMINSTANCE_H

#include <immintrin.h>
#include <stdint.h>
#include "parameters.h"
#include "parsing.h"

void hep_qc_pke_keygen(uint8_t *ek_pke, uint8_t *dk_pke, uint8_t *seed);
void hep_qc_pke_encrypt(ciphertext_pke_t *c_pke, const uint8_t *ek_pke, const uint64_t *m, const uint8_t *theta);
uint8_t hep_qc_pke_decrypt(uint64_t *m, const uint8_t *dk_pke, const ciphertext_pke_t *c_pke);

#endif  // PKE_ALGORITHMINSTANCE_H
