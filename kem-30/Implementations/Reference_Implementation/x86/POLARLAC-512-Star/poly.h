/*
Copyright (c) 2026 Ying Liu, Yu Zhang, Ziyao Liu.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
File Description: Declares polynomial compression, encoding, and arithmetic helpers for the reference POLARLAC-512-Star instance.
*/

#ifndef POLY_H
#define POLY_H

#include <stdint.h>
#include "params.h"

typedef struct {
    int16_t coeffs[RL_KEM_N];
} polarlac_poly;

typedef struct {
    polarlac_poly vec[RL_KEM_K];
} polarlac_polyvec;

typedef struct {
    polarlac_polyvec row[RL_KEM_K];
} polarlac_polymat;

/// @brief Losslessly encode a canonical polynomial using DAWN-style CRT packing.
/// @param[out] code_c Output byte array of length `PK_POLY_BYTES`.
/// @param[in] c Input coefficients modulo `RL_KEM_Q` of length `RL_KEM_N`.
/// @return 0 on success.
int poly_compress(uint8_t *code_c, const int16_t *c);

/// @brief Decode a canonical polynomial from DAWN-style CRT packing.
/// @param[out] c Recovered coefficients modulo `RL_KEM_Q` of length `RL_KEM_N`.
/// @param[in] code_c Input byte array of length `PK_POLY_BYTES`.
void poly_decompress(int16_t *c, const uint8_t *code_c);

/// @brief Encode a length-K polynomial vector as K independently compressed polynomials.
int polyvec_compress(uint8_t *code_c, const polarlac_polyvec *c);

/// @brief Decode a length-K polynomial vector from K independently compressed polynomials.
void polyvec_decompress(polarlac_polyvec *c, const uint8_t *code_c);

/// @brief Quantize the first `RL_KEM_Lv` coefficients of `c2` to `d` bits each.
/// @param[out] code_c Base address of the output coefficient array of length `RL_KEM_Lv`.
/// @param[in] c Base address of the input coefficient array of length `RL_KEM_N`.
/// @param[in] d Number of quantization bits kept per retained coefficient.
/// @details Coefficients with index `RL_KEM_Lv` and above are not serialized.
void poly_compress_c2(uint8_t *code_c, const int16_t *c, unsigned int d);

/// @brief Dequantize the retained `c2` coefficients from their d-bit representation.
/// @param[out] c Base address of the recovered coefficient array of length `RL_KEM_N`.
/// @param[in] code_c Base address of the compressed coefficient array of length `RL_KEM_Lv`.
/// @param[in] d Number of quantization bits stored per retained coefficient.
/// @details The output tail from index `RL_KEM_Lv` to `RL_KEM_N - 1` is cleared to zero.
void poly_decompress_c2(int16_t *c, const uint8_t *code_c, unsigned int d);

/// @brief Pack the d-bit `c2` coefficients into a byte string.
/// @param[out] dst Base address of the packed byte array of length `C2_LEN_BYTES`.
/// @param[in] com_c2 Base address of the compressed coefficient array of length `RL_KEM_Lv`.
/// @details The bit width is selected by `D_C2_BITS`; coefficients are packed
///          consecutively into `C2_LEN_BYTES` bytes.
void pack_c2_dbit(uint8_t *dst, const uint8_t *com_c2);

/// @brief Unpack the packed `c2` payload into the retained d-bit coefficient values.
/// @param[out] com_c2 Base address of the unpacked coefficient array of length `RL_KEM_Lv`.
/// @param[in] src Base address of the packed byte array of length `C2_LEN_BYTES`.
/// @details The ciphertext stores only the first `RL_KEM_Lv` compressed `c2`
///          coefficients, so this helper reconstructs exactly that prefix.
void unpack_c2_dbit(uint8_t *com_c2, const uint8_t *src);

/// @brief Multiply two polynomials in the ring using NTT acceleration.
/// @param[out] out Base address of the product coefficient array of length `RL_KEM_N`.
/// @param[in] a Base address of the first input polynomial.
/// @param[in] s Base address of the second input polynomial.
void poly_mul(int16_t *out, const int16_t *a, const int16_t *s);

/// @brief Multiply a coefficient-domain polynomial by a cached NTT-domain polynomial.
/// @param[out] out Base address of the product coefficient array of length `RL_KEM_N`.
/// @param[in] a Base address of the coefficient-domain polynomial.
/// @param[in] s_ntt Base address of the cached NTT-domain polynomial.
void poly_mul_with_cached_ntt(int16_t *out, const int16_t *a, const int16_t *s_ntt);

/// @brief Compute `a * s + e` when `a` is already interpreted as NTT-domain data.
/// @param[out] out Base address of the output coefficient array of length `RL_KEM_N`.
/// @param[in] a Base address of the first multiplicand.
/// @param[in] s Base address of the second multiplicand.
/// @param[in] e Base address of the additive error polynomial.
void poly_mul_add_with_ntt(int16_t *out, const int16_t *a, const int16_t *s, const int16_t *e);

/// @brief Compute `a_ntt * b_ntt + e` using two cached NTT-domain multiplicands.
/// @param[out] out Base address of the output coefficient array of length `RL_KEM_N`.
/// @param[in] a_ntt Base address of the first NTT-domain multiplicand.
/// @param[in] b_ntt Base address of the second NTT-domain multiplicand.
/// @param[in] e Base address of the additive error polynomial.
void poly_mul_add_with_cached_ntt(int16_t *out, const int16_t *a_ntt, const int16_t *b_ntt, const int16_t *e);

/// @brief Compute `a * s + e` with both multiplicands transformed internally.
/// @param[out] out Base address of the output coefficient array of length `RL_KEM_N`.
/// @param[in] a Base address of the first multiplicand.
/// @param[in] s Base address of the second multiplicand.
/// @param[in] e Base address of the additive error polynomial.
void poly_mul_add(int16_t *out, const int16_t *a, const int16_t *s, const int16_t *e);

/// @brief Polar-encode a plaintext message into the retained `c2` bit positions.
/// @param[out] code_m Base address of the output bit array of length `RL_KEM_Lv`.
/// @param[in] m Base address of the input message byte array of length `MESSAGE_LEN_BYTES`.
void Encode_m(uint8_t *code_m, uint8_t *m);

/// @brief Recover a plaintext message from the retained `c2` coefficients.
/// @param[out] m Base address of the recovered message byte array.
/// @param[in,out] hatm Base address of the soft/hard decision vector.
/// @details Only the first `RL_KEM_Lv` entries of `hatm` are meaningful inputs.
void Decode_m(uint8_t *m, int16_t *hatm);

#endif
