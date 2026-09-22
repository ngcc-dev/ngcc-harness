/**
 * \file parsing.h
 * \brief Key and ciphertext serialization/deserialization function declarations for the BRA scheme
 *
 * Provides conversion between byte strings and internal data structures for
 * public keys, secret keys, and ciphertexts.
 * Secret key sk = seed || sigma || pk (seed + implicit rejection value + public key).
 * Public key pk = s || pk_seed (syndrome s + public key seed).
 * Ciphertext ct = u || v || salt.
 */

#ifndef BRA_PARSING_H
#define BRA_PARSING_H

#include "rbc_qre.h"

void bra_secret_key_to_string(uint8_t* sk, const uint8_t* seed, const rbc_vec sigma, const uint8_t* pk);
void bra_secret_key_from_string(rbc_qre x, rbc_qre y, rbc_vec sigma, uint8_t* pk, const uint8_t* sk);

void bra_public_key_to_string(uint8_t* pk, const rbc_qre s, const uint8_t* seed);
void bra_public_key_from_string(rbc_qre g, rbc_qre h, rbc_qre s, const uint8_t* pk);

void bra_kem_ciphertext_to_string(uint8_t* ct, const rbc_qre u, const rbc_qre v, const uint8_t* d);
void bra_kem_ciphertext_from_string(rbc_qre u, rbc_qre v, uint8_t* d, const uint8_t* ct);

#endif