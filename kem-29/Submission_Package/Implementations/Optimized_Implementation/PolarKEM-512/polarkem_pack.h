/**
 * @file polarkem_pack.h
 * @brief Quantization and fixed-width batch packing for PolarKEM ciphertexts.
 */
#ifndef POLARKEM_PACK_H
#define POLARKEM_PACK_H

#include <stdint.h>

#include "polarkem_params.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Quantize one canonical coefficient from Z_q to a d-bit representative.
 *
 * @param a Coefficient in the inclusive range 0..POLARKEM_Q-1.
 * @return round(a*2^d/q) modulo 2^d.
 */
uint16_t polarkem_compress_coefficient(uint16_t a);

/**
 * Dequantize one d-bit representative to its nearest canonical Z_q value.
 *
 * @param t Integer in the inclusive range 0..2^d-1.
 * @return round(t*q/2^d), in canonical non-negative representation.
 */
uint16_t polarkem_decompress_coefficient(uint16_t t);

/**
 * Quantize and pack N coefficients into the profile's LSB-first payload.
 *
 * Compile-time specialized paths consume eight coefficients per iteration for
 * d=11 and d=9, and directly map one coefficient per byte for d=8.  Every path
 * is exactly the scalar contiguous-bit encoding.
 *
 * @param[out] payload Ciphertext payload, POLARKEM_CT_PAYLOAD_BYTES bytes.
 * @param[in] coefficients POLARKEM_N canonical coefficients modulo q.
 * @return Nothing; every output byte is initialized.
 */
void polarkem_pack_coefficients(
    unsigned char payload[POLARKEM_CT_PAYLOAD_BYTES],
    const uint16_t coefficients[POLARKEM_N]);

/**
 * Unpack and dequantize the profile's LSB-first ciphertext payload.
 *
 * @param[out] coefficients POLARKEM_N canonical approximate coefficients.
 * @param[in] payload Ciphertext payload, POLARKEM_CT_PAYLOAD_BYTES bytes.
 * @return Nothing; every output coefficient is initialized.
 */
void polarkem_unpack_coefficients(
    uint16_t coefficients[POLARKEM_N],
    const unsigned char payload[POLARKEM_CT_PAYLOAD_BYTES]);

#ifdef __cplusplus
}
#endif

#endif /* POLARKEM_PACK_H */

