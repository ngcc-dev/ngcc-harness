/*
Copyright (c) 2026 Ying Liu.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
File Description: Declares FFT-based spectral helper routines for the reference POLARLAC-512-Star instance.
*/

#ifndef FFT_H
#define FFT_H

#include <stdint.h>
#include "params.h"

/**
 * @file fft.h
 * @brief FFT helper types and API used by the reference
 *        RL-KEM implementation.
 *
 * Notes:
 * - This header provides a small fixed-size 16-bit complex type and
 *   function prototypes. Implementation tables live in fft.c.
 */

/**
 * @brief 16-bit complex integer type (real, imag)
 */
typedef struct { int16_t re, im; } ci16_t;

/**
 * @brief Check whether every squared spectral component stays below `bound`.
 * @param[in] a Input array of length RL_KEM_N (int16_t).
 * @param[in] bound Exclusive upper bound for re^2 + im^2.
 * @return 1 if every spectral component is < bound, otherwise 0.
 */
int fft_within_bound_int16(const int16_t *a, int32_t bound);


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
