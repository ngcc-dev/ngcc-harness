// Copyright 2025 LG Electronics, Inc. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <mpc.h>
#include <mpfr.h>


typedef struct mpfr_matrix_s {
    int n_rows;
    int n_cols;
    mpfr_t **m;
} mpfr_matrix_t;

typedef struct mpc_matrix_s {
    int n_rows;
    int n_cols;
    mpc_t **m;
} mpc_matrix_t;


void mpc_matrix_init(mpc_matrix_t *matrix, int n_rows, int n_cols);
void mpc_matrix_free(mpc_matrix_t *matrix);
void mpc_matrix_print_to_file_ptr(FILE *file, mpc_matrix_t *matrix);
void mpc_matrix_print(mpc_matrix_t *matrix);
void mpc_matrix_init_square_from_file(mpc_matrix_t *matrix, int n_rows, char filename[]);
void mpfr_matrix_init(mpfr_matrix_t *matrix, int n_rows, int n_cols);
void mpfr_matrix_free(mpfr_matrix_t *matrix);
void mpfr_matrix_init_from_csv(mpfr_matrix_t *matrix, char filename[]);
void mpc_matrix_to_mpfr_matrix(mpfr_matrix_t *real_matrix, mpc_matrix_t *complex_matrix);
