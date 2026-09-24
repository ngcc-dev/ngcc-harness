#ifndef INDCPA_H
#define INDCPA_H

#include <stdint.h>
#include "params.h"

#define h1 (1 << (SCABBARD_EQ - SCABBARD_EP - 1))
#define h2 ((1 << SCABBARD_EP - (SCABBARD_B + 1)) - (1 << (SCABBARD_EP - (SCABBARD_ET + SCABBARD_B) - 1)) - (1 << (SCABBARD_EQ - SCABBARD_EP - 1)))

/// @brief CPA-secure Key generate
/// @param[out] pk Public key (SCABBARD_INDCPA_PUBLICKEYBYTES)
/// @param[out] sk Private key (SCABBARD_INDCPA_SECRETKEYBYTES)
void indcpa_keypair(
    uint8_t pk[SCABBARD_INDCPA_PUBLICKEYBYTES], 
    uint8_t sk[SCABBARD_INDCPA_SECRETKEYBYTES]);

/// @brief CPA-secure Encrypt
/// @param[out] c Ciphertext (SCABBARD_INDCPA_BYTES)
/// @param[in] m Message (SCABBARD_INDCPA_MSGBYTES)
/// @param[in] pk Public key (SCABBARD_INDCPA_PUBLICKEYBYTES)
/// @param[in] r Random coins (SCABBARD_SYMBYTES)
void indcpa_enc(
    uint8_t c[SCABBARD_INDCPA_BYTES], 
    const uint8_t m[SCABBARD_INDCPA_MSGBYTES], 
    const uint8_t pk[SCABBARD_INDCPA_PUBLICKEYBYTES], 
    const uint8_t r[SCABBARD_SYMBYTES]);

uint8_t indcpa_enc_cmp(
    const uint8_t c[SCABBARD_INDCPA_BYTES],
    const uint8_t m[SCABBARD_INDCPA_MSGBYTES],
    const uint8_t pk[SCABBARD_INDCPA_PUBLICKEYBYTES],
    const uint8_t r[SCABBARD_SYMBYTES]);

/// @brief CPA-secure Decrypt
/// @param[out] m Message (SCABBARD_INDCPA_MSGBYTES)
/// @param[in] c Ciphertext (SCABBARD_INDCPA_BYTES)
/// @param[in] sk Private key (SCABBARD_INDCPA_SECRETKEYBYTES)
void indcpa_dec(
    uint8_t m[SCABBARD_INDCPA_MSGBYTES], 
    const uint8_t c[SCABBARD_INDCPA_BYTES], 
    const uint8_t sk[SCABBARD_INDCPA_SECRETKEYBYTES]);

#endif
