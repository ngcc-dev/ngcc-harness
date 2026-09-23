/**
 * @file data_structures.h
 * @brief Internal QUBE ciphertext structures.
 */

#ifndef QUBE_DATA_STRUCTURES_H
#define QUBE_DATA_STRUCTURES_H

#include <stdint.h>
#include "parameters.h"

typedef struct {
    uint64_t u[VEC_N_SIZE_64];
    uint64_t v[VEC_N1N2_SIZE_64];
} ciphertext_pke_t;

typedef struct {
    ciphertext_pke_t c_pke;
    uint8_t salt[SALT_BYTES];
} ciphertext_kem_t;

typedef union {
    uint8_t u8[16];
    uint32_t u32[4];
} rm_codeword_t;

#endif  // QUBE_DATA_STRUCTURES_H
