#ifndef POLYVEC_H
#define POLYVEC_H

#include <stdint.h>
#include "params.h"
#include "poly.h"

typedef struct{
  poly vec[SCABBARD_L];
} polyvec;

/// @brief Sample/generate a matrix from uniformly random distribution
/// @param[out] a Public matrix A (SCABBARD_L x SCABBARD_L)
/// @param[in] seed Public seed (SCABBARD_SYMBYTES)
void gen_a(
    polyvec *a, 
    const uint8_t seed[SCABBARD_SYMBYTES]);

/// @brief Sample/generate vector of secret polynomials from centered binomial distribution
/// @param[out] s Secret vector s (SCABBARD_L)
/// @param[in] seed Private seed (SCABBARD_SYMBYTES)
void gen_s(
    polyvec *s,
    const uint8_t seed[SCABBARD_SYMBYTES]);

void gen_s_packed(
    uint8_t s[SCABBARD_INDCPA_SECRETKEYBYTES],
    const uint8_t seed[SCABBARD_SYMBYTES]);

void matrix_vector_mul_tobytes(
    uint8_t *out,
    const uint8_t seed[SCABBARD_SYMBYTES],
    const uint8_t *s_packed,
    int16_t transpose,
    uint16_t rounding_const);

uint8_t matrix_vector_mul_tobytes_cmp(
    const uint8_t *out,
    const uint8_t seed[SCABBARD_SYMBYTES],
    const uint8_t *s_packed,
    int16_t transpose,
    uint16_t rounding_const);

void inner_prod_packed(
    poly *r,
    const uint8_t *a_packed,
    const uint8_t *s_packed);

/// @brief Serialize a vector of polynomials with coefficients mod SCABBARD_P
/// @param[out] r Byte array
/// @param[in] a Vector of polynomials to be serialized
void polyvec_modp_tobytes(
    uint8_t r[SCABBARD_POLYVECCOMPRESSEDBYTES],
    const polyvec *a);

/// @brief De-serialize a vector of polynomials with coefficients mod SCABBARD_P
/// @param[out] a Vector of polynomials
/// @param[in] r Byte array
void polyvec_modp_frombytes(
    polyvec *a,
    const uint8_t r[SCABBARD_POLYVECCOMPRESSEDBYTES]);

/// @brief Serialize a vector of secret polynomials with SCABBARD_B bits per coefficient
/// @param[out] r Byte array (SCABBARD_S_POLYVECBYTES)
/// @param[in] s Vector of secret polynomials to be serialized
void polyvec_s_tobytes(
    uint8_t r[SCABBARD_S_POLYVECBYTES],
    const polyvec *s);

/// @brief De-serialize a vector of secret polynomials with SCABBARD_B bits per coefficient
/// @param[out] s Vector of secret polynomials
/// @param[in] r Byte array (SCABBARD_S_POLYVECBYTES)
void polyvec_s_frombytes(
    polyvec *s,
    const uint8_t r[SCABBARD_S_POLYVECBYTES]);

#endif
