/**
 * @file polarkem_polar.h
 * @brief Portable 64-bit Polar transform for the PolarKEM-256 profile.
 */
#ifndef POLARKEM_POLAR_H
#define POLARKEM_POLAR_H

#include <stdint.h>

#include "polarkem_params.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Apply the no-bit-reversal row-vector transform F^(tensor logN) in place.
 *
 * @param[in,out] words Sixteen little-bit-endian 64-bit words (1024 bits).
 * @return Nothing.  The same function is the inverse over GF(2).
 */
void polarkem_polar_transform(uint64_t words[POLARKEM_WORDS]);

/**
 * Insert a 32-byte message at the fixed BEC information positions and encode.
 *
 * Message bit j is bit (j modulo 8) of byte floor(j/8), starting at the least
 * significant bit.  All non-information positions are fixed to zero.
 *
 * @param[out] codeword Encoded 1024-bit codeword in little-bit-endian words.
 * @param[in] mu Encapsulated message, exactly POLARKEM_MU_BYTES bytes.
 * @return Nothing; every output word is initialized.
 */
void polarkem_polar_encode(uint64_t codeword[POLARKEM_WORDS],
                           const unsigned char mu[POLARKEM_MU_BYTES]);

/**
 * Invert an encoded codeword and recover its 32-byte information payload.
 *
 * @param[out] mu Recovered message, exactly POLARKEM_MU_BYTES bytes.
 * @param[in] codeword Encoded 1024-bit codeword in little-bit-endian words.
 * @return Nothing; every output byte is initialized.
 */
void polarkem_polar_decode(unsigned char mu[POLARKEM_MU_BYTES],
                           const uint64_t codeword[POLARKEM_WORDS]);

#ifdef __cplusplus
}
#endif

#endif /* POLARKEM_POLAR_H */
