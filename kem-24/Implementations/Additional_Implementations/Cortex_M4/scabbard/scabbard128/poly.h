#ifndef POLY_H
#define POLY_H

#include <stdint.h>
#include <string.h>
#include "params.h"

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

uint8_t poly_modp_tobytes_cmp(
  const uint8_t r[SCABBARD_P_POLYBYTES],
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

uint8_t poly_m_tobytes_cmp(
  const uint8_t r[SCABBARD_POLYCOMPRESSEDBYTES],
  const poly *m);

/// @brief De-serialize message polynomial with (SCABBARD_B + SCABBARD_ET) bits per coefficient from bytestream
/// @param[out] r Message polynomial
/// @param[in] m Byte array (SCABBARD_POLYCOMPRESSEDBYTES)
void poly_m_frombytes(
  poly *r,
  const uint8_t m[SCABBARD_POLYCOMPRESSEDBYTES]);

/* Minal Code Encode/Decode */
#define N_CODE_VALS 4
#define MINAL_MAX_N_DIM 2
#define MINAL_MAX_CODEWORDS (1 << MINAL_MAX_N_DIM)

typedef struct minal_params_s {
    int q;
    int beta;
    int alpha;
    int n_dim;
    int16_t CODEWORDS[MINAL_MAX_CODEWORDS][MINAL_MAX_N_DIM];
    int16_t CODE_VALS[N_CODE_VALS];
} minal_params_t;

typedef struct minal_b2_params_s {
    int q;
    int half_q;
    int n_input;
    minal_params_t internal_minal;
} minal_b2_params_t;

/// @brief Convert polynomial to 16-byte message
/// @param[out] msg Byte array (SCABBARD_INDCPA_MSGBYTES)
/// @param[in] v Polynomial to be converted to message
void poly_tomsg(
  uint8_t msg[SCABBARD_INDCPA_MSGBYTES],
  const poly *v);

/// @brief Convert 16-byte message to polynomial
/// @param[out] r Message in polynomial form
/// @param[in] msg Byte array (SCABBARD_INDCPA_MSGBYTES)
void poly_frommsg(
  poly *r,
  const uint8_t msg[SCABBARD_INDCPA_MSGBYTES]);

#define N_SM (SCABBARD_N >> 1)
#define N_SM_RES (2 * N_SM - 1)
#define N_SM_16 (N_SM >> 1)
#define N_SM_16_RES (2 * N_SM_16 - 1)

void poly_TC_evaluate(
  uint16_t b_weighted[3][3][N_SM_16],
  const uint16_t *b);

void poly_TC_pointwise_acc(
  uint16_t acc[3][3][N_SM_16_RES],
  const uint16_t a_weighted[3][3][N_SM_16],
  const uint16_t b_weighted[3][3][N_SM_16]);

void poly_TC_interpolate(
  uint16_t *res,
  const uint16_t acc[3][3][N_SM_16_RES],
  uint16_t mod_mask);

void poly_TC_mul_64(
  const uint16_t *a,
  const uint16_t *b,
  uint16_t *res,
  uint16_t mod_mask);

void minal_code_init(
  minal_params_t *minal, 
  int q, 
  int beta, 
  int n_dim);

void minal_code_clear(
  minal_params_t *minal);

void minal_b2_code_init(
  minal_b2_params_t *minal, 
  int q, 
  int beta, 
  int n_dim);

void minal_b2_code_clear(
  minal_b2_params_t *minal);

void minal_code_encode(
  int16_t codeword[], 
  uint8_t msg_bits[], 
  minal_params_t *minal);

uint16_t minal_code_decode(
  int16_t target[], 
  minal_params_t *minal);  

#endif
