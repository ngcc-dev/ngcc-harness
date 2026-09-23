/**
 * @file parsing.h
 * @brief Serialization helpers for QUBE keys and ciphertexts.
 */

#ifndef QUBE_PARSING_H
#define QUBE_PARSING_H

#include <stdint.h>
#include "data_structures.h"

int qube_dk_pke_from_string(uint64_t *y, const uint8_t *dk_pke);
int qube_ek_pke_from_string(uint64_t *h1, uint64_t *h2, uint64_t *s1, uint64_t *s2, const uint8_t *ek_pke);

void qube_c_pke_to_string(uint8_t *ct, const ciphertext_pke_t *c_pke);
void qube_c_pke_from_string(ciphertext_pke_t *c_pke, const uint8_t *ct);
void qube_c_kem_to_string(uint8_t *ct, const ciphertext_kem_t *c_kem);
void qube_c_kem_from_string(ciphertext_kem_t *c_kem, const uint8_t *ct);

#endif  // QUBE_PARSING_H
