/**
 * @file hqc.h
 * @brief HARE-PKE core interface following the HQC-style quasi-cyclic code framework.
 *
 * The function names keep the historical hqc_* prefix used by the shared core,
 * while the implemented algorithm is the HARE submission line configured by
 * each instance's parameters.h and selected KR compression backend.
 */

#ifndef HARE_REF_PKE_H
#define HARE_REF_PKE_H

#include <stdint.h>
#include "parameters.h"
#include "parsing.h"

/** Deterministically generate a HARE-PKE encryption/decryption keypair from a seed. */
void hqc_pke_keygen(uint8_t *ek_pke, uint8_t *dk_pke, uint8_t *seed);

/** Encrypt a PARAM_K-byte message under a HARE-PKE encryption key and randomness theta. */
void hqc_pke_encrypt(ciphertext_pke_t *c_pke, const uint8_t *ek_pke, const uint8_t *m, const uint8_t *theta);

/** Decrypt a HARE-PKE ciphertext using the seed-form decryption key. */
uint8_t hqc_pke_decrypt(uint8_t *m, const uint8_t *dk_pke, const ciphertext_pke_t *c_pke);

#endif /* HARE_REF_PKE_H */
