/**
 * @file qube.h
 * @brief Header file for qube.c
 */

#ifndef QUBE_QUBE_H
#define QUBE_QUBE_H

//#include <immintrin.h>
#include <stdint.h>
#include "parameters.h"
#include "parsing.h"

void qube_pke_keygen(uint8_t *ek_pke, uint8_t *dk_pke, uint8_t *seed);
void qube_pke_encrypt(ciphertext_pke_t *c_pke, const uint8_t *ek_pke, const uint64_t *m, const uint8_t *theta);
uint8_t qube_pke_decrypt(uint64_t *m, const uint8_t *dk_pke, const ciphertext_pke_t *c_pke);

#endif  // QUBE_QUBE_H
