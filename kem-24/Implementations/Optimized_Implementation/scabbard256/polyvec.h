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

// void gen_s_avx(
//     polyvec *s,
//     const uint8_t seed[SCABBARD_SYMBYTES]);

/// @brief Matrix-vector multiplication
/// @param[out] r Result of multiplication
/// @param[in] a Matrix A (SCABBARD_L x SCABBARD_L)
/// @param[in] b Vector b (SCABBARD_L)
/// @param[in] transpose Flag indicating whether to transpose the matrix
void matrix_vector_mul(
    polyvec *r, 
    const polyvec *a, 
    const polyvec *b,
    int16_t transpose);

/// @brief Inner product of two vectors of polynomials
/// @param[out] r Result of inner product
/// @param[in] a Vector of polynomials a (SCABBARD_L)
/// @param[in] b Vector of polynomials b (SCABBARD_L)
void inner_prod(
    poly *r,
    const polyvec *a,
    const polyvec *b);

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


/* Polynomial Multiplication */

/// @brief Toom-Cook Evaluation of vector of polynomials
/// @param[out] b_weighted Weighted polynomials
/// @param[in] b Polynomials to be evaluated in different points
void polyvec_TC_evaluate(
    uint16_t b_weighted[SCABBARD_L][3][3][N_SM_16],
    const polyvec *b);

#endif