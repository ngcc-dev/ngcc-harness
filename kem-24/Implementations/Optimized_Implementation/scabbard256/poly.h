#ifndef POLY_H
#define POLY_H

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "params.h"
#include <immintrin.h>
#define ALIGN8  __attribute__((aligned(8)))
#define ALIGN16  __attribute__((aligned(16)))
#define ALIGN32  __attribute__((aligned(32)))
#define ALIGN64  __attribute__((aligned(64)))

typedef struct
{
  uint16_t coeffs[SCABBARD_N];
} poly;

/// @brief Unpack polynomial with integer coefficients (mod SCABBARD_Q) from bytestream
/// @param[out] r Polynomial with coefficients (mod SCABBARD_Q)
/// @param[in] buf Byte array (SCABBARD_POLYBYTES)
void poly_modq_frombytes(
  poly *r, 
  const uint8_t buf[SCABBARD_POLYBYTES]);

/// @brief Serialize polynomial with coefficients mod SCABBARD_P
/// @param[out] r Byte array (SCABBARD_P_POLYBYTES)
/// @param[in] a Polynomial to be serialized
void poly_modp_tobytes(
  uint8_t r[SCABBARD_P_POLYBYTES], 
  const poly *a);

/// @brief De-serialize polynomial with coefficients mod SCABBARD_P from bytestream
/// @param[out] r Polynomial with coefficients (mod SCABBARD_P)
/// @param[in] a Byte array (SCABBARD_P_POLYBYTES)
void poly_modp_frombytes(
  poly *r, 
  const uint8_t a[SCABBARD_P_POLYBYTES]);

/// @brief Serialize secret polynomial with SCABBARD_B bits per coefficient
/// @param[out] r Byte array (SCABBARD_S_POLYBYTES)
/// @param[in] s Secret polynomials to be serialized
void poly_s_tobytes(
  uint8_t r[SCABBARD_S_POLYBYTES],
  const poly *s);

/// @brief De-serialize secret polynomial with SCABBARD_B bits per coefficient from bytestream
/// @param[out] r Secret polynomial
/// @param[in] s Byte array (SCABBARD_S_POLYBYTES)
void poly_s_frombytes(
  poly *r,
  const uint8_t s[SCABBARD_S_POLYBYTES]);

/// @brief Serialize message polynomial with (SCABBARD_B + SCABBARD_ET) bits per coefficient
/// @param[out] r Byte array (SCABBARD_POLYCOMPRESSEDBYTES)
/// @param[in] m Message polynomial to be serialized
void poly_m_tobytes(
  uint8_t r[SCABBARD_POLYCOMPRESSEDBYTES],
  const poly *m);

/// @brief De-serialize message polynomial with (SCABBARD_B + SCABBARD_ET) bits per coefficient from bytestream
/// @param[out] r Message polynomial
/// @param[in] m Byte array (SCABBARD_POLYCOMPRESSEDBYTES)
void poly_m_frombytes(
  poly *r,
  const uint8_t m[SCABBARD_POLYCOMPRESSEDBYTES]);

/// @brief Convert polynomial to 32-byte message
/// @param[out] msg Byte array (SCABBARD_INDCPA_MSGBYTES)
/// @param[in] v Polynomial to be converted to message
void poly_tomsg(
  uint8_t msg[SCABBARD_INDCPA_MSGBYTES],
  const poly *v);

/// @brief Convert 32-byte message to polynomial
/// @param[out] r Message in polynomial form
/// @param[in] msg Byte array (SCABBARD_INDCPA_MSGBYTES)
void poly_frommsg(
  poly *r,
  const uint8_t msg[SCABBARD_INDCPA_MSGBYTES]);

/* Polynomial Multiplication */

#define N_SM (SCABBARD_N >> 1)
#define N_SM_16 (N_SM >> 1)

#ifdef __AVX2
  #define N_SM_RES (2*N_SM)
  #define N_SM_16_RES (2*N_SM_16)
#else
  #define N_SM_RES (2*N_SM-1)
  #define N_SM_16_RES (2*N_SM_16-1)
#endif

/// @brief Toom-Cook Evaluation of a polynomials
/// @param[out] b_weighted Weighted polynomial
/// @param[in] b Polynomial to be evaluated in different points
void poly_TC_evaluate(
    uint16_t b_weighted[3][3][N_SM_16],
    const poly *b);

/// @brief Toom-Cook Pointwise Multiplication and Accumulation
/// @param[out] acc Accumulated result of pointwise multiplication
/// @param[in] a_weighted Weighted polynomial a
/// @param[in] b_weighted Weighted polynomial b
void poly_TC_pointwise_acc(
    uint16_t acc[3][3][N_SM_16_RES],
    const uint16_t a_weighted[3][3][N_SM_16],
    const uint16_t b_weighted[3][3][N_SM_16]);

/// @brief Toom-Cook Interpolation of a polynomial + reduction
/// @param[out] r Interpolated polynomial
/// @param[in] acc Accumulated result of pointwise multiplication in different points
void poly_TC_interpolate(
    poly *r,
    const uint16_t acc[3][3][N_SM_16_RES]);

#endif