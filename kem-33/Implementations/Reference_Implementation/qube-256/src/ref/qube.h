/**
 * @file qube.h
 * @brief QUBE-PKE interface.
 */

#ifndef QUBE_QUBE_H
#define QUBE_QUBE_H

#include <stdint.h>
#include "data_structures.h"

int qube_pke_keygen(uint8_t *pk_pke, uint8_t *sk_pke, const uint8_t rho[SEED_BYTES]);
int qube_pke_encrypt(ciphertext_pke_t *c_pke, const uint8_t *pk_pke,
                     const uint8_t m[PARAM_SECURITY_BYTES], const uint8_t rho_c[SEED_BYTES]);
int qube_pke_decrypt(uint8_t m[PARAM_SECURITY_BYTES], const uint8_t sk_pke[SEED_BYTES],
                     const ciphertext_pke_t *c_pke);

#endif  // QUBE_QUBE_H
