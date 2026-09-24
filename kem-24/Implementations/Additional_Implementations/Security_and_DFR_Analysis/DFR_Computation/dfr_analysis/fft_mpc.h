// Copyright 2025 LG Electronics, Inc. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <gmp.h>
#include <mpc.h>

#include "mp_matrix.h"
#include "common.h"

// Important notice: OUR FFT FUNCTIONS ONLY WORK FOR VECTORS WHOSE SIZE IS A POWER OF TWO!

// The constants defined next define important values related to the FFT computation and
// the selection of which algorithm to use.

// `MAX_N_LEVELS_FOR_PURE_FFT`:
//      This is the maximum number of FFT layers that are used by the `fft_pure` function.
//      It controls the number of precomputed twiddle factors.
// `MAX_N_FOR_PURE_FFT`:
//      This is then maximum vector size for which `fft_pure` is used when `fft` is called.
//      It is simply defined as `2^MAX_N_LEVELS_FOR_PURE_FFT`.
#ifndef MINAL_ANALYSIS_FOR_LWR
#define MAX_N_LEVELS_FOR_PURE_FFT 12
#else
#define MAX_N_LEVELS_FOR_PURE_FFT 13
#endif
#define MAX_N_FOR_PURE_FFT (1 << MAX_N_LEVELS_FOR_PURE_FFT)

// `MAX_N_FOR_BAILEY_FFT`:
//      This is then maximum vector size for which `fft_bailey` is used when `fft` is called.
//      It is simply defined as `2^(MAX_N_LEVELS_FOR_PURE_FFT*2) = MAX_N_FOR_PURE_FFT^2`.
// #define MAX_N_FOR_BAILEY_FFT (1 << (2*MAX_N_LEVELS_FOR_PURE_FFT))
#define MAX_N_FOR_BAILEY_FFT (1 << (2*MAX_N_LEVELS_FOR_PURE_FFT - 1))

#define MAX_N_FOR_FFT MAX_N_FOR_BAILEY_FFT


void init_fft_mpc();
void clear_fft_mpc();
void fft(mpc_t *vector, int n);
void ifft(mpc_t *vector, int n);
void fft2(mpc_matrix_t *matrix);
void ifft2(mpc_matrix_t *matrix);
void fft_pure(mpc_t *vector, int n);
void ifft_pure(mpc_t *vector, int n);
void fft_bailey(mpc_t *vector, int n, int n_rows, int n_cols);
void ifft_bailey(mpc_t *vector, int n, int n_rows, int n_cols);
