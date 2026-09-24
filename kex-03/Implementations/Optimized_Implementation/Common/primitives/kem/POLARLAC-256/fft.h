/*
Copyright (c) 2026 Ying Liu.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
File Description: Declares FFT-based spectral helper routines and constants for the optimized POLARLAC-256 instance.
*/

#ifndef FFT_H
#define FFT_H

#include <stdint.h>
#include "params.h"

#if defined(__GNUC__) || defined(__clang__)
#define POLARLAC_FFT_ALIGN64 __attribute__((aligned(64)))
#else
#define POLARLAC_FFT_ALIGN64
#endif

/**
 * @file fft.h
 * @brief FFT helper types, twiddle table and API used by the reference
 *        RL-KEM implementation.
 *
 * Notes:
 * - This header provides a small fixed-size 16-bit complex type and a
 *   precomputed twiddle table for an FFT/NTT-like forward transform used in
 *   reference code. The implementation assumes RL_KEM_N from `params.h`.
 * - The twiddle table is declared as a static const array for simple
 *   inclusion in the reference build. In production builds consider moving
 *   large tables to a single C file to avoid multiple-definition issues.
 */

/**
 * @brief 16-bit complex integer type (real, imag)
 */
typedef struct { int16_t re, im; } ci16_t;




/**
 * @brief Compute forward FFT of int16 input and produce int16 real/imag
 *        outputs.
 * @param[in]  a       Input array of length RL_KEM_N (int16_t).
 * @param[out] out_re  Output real parts (length RL_KEM_N).
 * @param[out] out_im  Output imaginary parts (length RL_KEM_N).
 *
 * Note: This reference function uses a fixed-length transform determined by
 *       the `RL_KEM_N` macro in `params.h`. The parameter `n` is kept for API
 *       compatibility but is not used.
 */
void fft_forward_int16(const int16_t *a, int16_t *out_re, int16_t *out_im);

/**
 * @brief Check whether every squared spectral component stays below `bound`.
 * @param[in] a Input array of length RL_KEM_N (int16_t).
 * @param[in] bound Exclusive upper bound for re^2 + im^2.
 * @return 1 if every spectral component is < bound, otherwise 0.
 */
int fft_within_bound_int16(const int16_t *a, int32_t bound);
int fft_within_bound_int16_avx2(const int16_t *a, int32_t bound);


/**
 * @brief Compute the unique half-spectrum of a real int16 input polynomial.
 * @param[in] a Input array. Elements are read with the provided stride.
 * @param[in] stride Distance between consecutive input samples.
 * @param[in] n Requested transform length; current implementations use
 *        `RL_KEM_N` and keep this parameter for compatibility.
 * @param[out] out Output array of length `RL_KEM_N_Half`.
 */
void real_fft_unique_raw_i16(const int16_t *a, int stride, int n, ci16_t *out);
#endif // FFT_H
