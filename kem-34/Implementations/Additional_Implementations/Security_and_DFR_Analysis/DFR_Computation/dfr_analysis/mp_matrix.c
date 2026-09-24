// Copyright 2025 LG Electronics, Inc. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <stdio.h>
#include <stdlib.h>
#include <mpc.h>
#include <mpfr.h>
#include <string.h>
#include <assert.h>

#include "mp_matrix.h"
#include "common.h"

#define STR_MAX 1000

void mpc_matrix_init(mpc_matrix_t *matrix, int n_rows, int n_cols) {
    matrix->n_rows = n_rows;
    matrix->n_cols = n_cols;

    matrix->m = malloc(n_rows * sizeof(*matrix->m));
    for (int i = 0; i < n_rows; i++) {
        matrix->m[i] = malloc(n_cols * sizeof(**matrix->m));
        for (int j = 0; j < n_cols; j++) {
            mpc_init2(matrix->m[i][j], N_PRECISION_BITS);
            mpc_set_d(matrix->m[i][j], 0, ROUND_DOWN);
        }
    }
}

void mpc_matrix_free(mpc_matrix_t *matrix) {

    for (int i = 0; i < matrix->n_rows; i++) {
        for (int j = 0; j < matrix->n_cols; j++) {
            mpc_clear(matrix->m[i][j]);
        }
        free(matrix->m[i]);
    }
    free(matrix->m);
    matrix->n_rows = 0;
    matrix->n_cols = 0;
}

void mpc_matrix_print_to_file_ptr(FILE *file, mpc_matrix_t *matrix) {

    mpfr_t zero;
    mpfr_init2(zero, N_PRECISION_BITS);
    mpfr_set_d(zero, ZERO_DOUBLE, MPFR_RNDD);

    mpfr_t real_part;
    mpfr_init2(real_part, N_PRECISION_BITS);
    mpfr_t imag_part;
    mpfr_init2(imag_part, N_PRECISION_BITS);

    fprintf(file, "row,col,prob_real,prob_im\n");
    for (int i = 0; i < matrix->n_rows; i++) {
        for (int j = 0; j < matrix->n_cols; j++) {
            mpc_real(real_part, matrix->m[i][j], ROUND_DOWN);
            mpc_imag(imag_part, matrix->m[i][j], ROUND_DOWN);

            int cmp_real = mpfr_cmp_abs(real_part, zero);
            int cmp_imag = mpfr_cmp_abs(imag_part, zero);

            if ((cmp_real <= 0) && (cmp_imag <= 0)) {
                continue;
            }

            fprintf(file, "%d,%d,", center_mod(i, matrix->n_rows), center_mod(j, matrix->n_cols));
            if (cmp_real > 0) {
                mpfr_out_str(file, 10, 0, real_part, ROUND_DOWN);
            }
            else {
                fprintf(file, "0");
            }
            fprintf(file, ",");
            if (cmp_imag > 0) {
                mpfr_out_str(file, 10, 0, imag_part, ROUND_DOWN);
            }
            else {
                fprintf(file, "0");
            }
            // mpc_out_str(file, 10, 0, matrix->m[i][j], ROUND_DOWN);
            fprintf(file, "\n");
        }
    }
    mpfr_clear(zero);
    mpfr_clear(real_part);
    mpfr_clear(imag_part);
}

void mpc_matrix_print(mpc_matrix_t *matrix) {

    mpfr_t zero;
    mpfr_init2(zero, N_PRECISION_BITS);
    mpfr_set_d(zero, ZERO_DOUBLE, MPFR_RNDD);

    mpfr_t real_part;
    mpfr_init2(real_part, N_PRECISION_BITS);
    mpfr_t imag_part;
    mpfr_init2(imag_part, N_PRECISION_BITS);

    for (int i = 0; i < matrix->n_rows; i++) {
        for (int j = 0; j < matrix->n_cols; j++) {
            mpc_real(real_part, matrix->m[i][j], ROUND_DOWN);
            mpc_imag(imag_part, matrix->m[i][j], ROUND_DOWN);

            int cmp_real = mpfr_cmp_abs(real_part, zero);
            int cmp_imag = mpfr_cmp_abs(imag_part, zero);

            if ((cmp_real <= 0) && (cmp_imag <= 0)) {
                continue;
            }

            printf("[%d, %d]: ", center_mod(i, matrix->n_rows), center_mod(j, matrix->n_cols));
            if (cmp_real > 0) {
                mpfr_out_str(stdout, 10, 0, real_part, ROUND_DOWN);
            }
            else {
                printf("0");
            }
            printf(" ");
            if (cmp_imag > 0) {
                mpfr_out_str(stdout, 10, 0, imag_part, ROUND_DOWN);
            }
            else {
                printf("0");
            }
            // mpc_out_str(stdout, 10, 0, matrix->m[i][j], ROUND_DOWN);
            printf("\n");
        }
    }
    mpfr_clear(zero);
    mpfr_clear(real_part);
    mpfr_clear(imag_part);
}

void mpc_matrix_init_square_from_file(mpc_matrix_t *matrix, int n_rows, char filename[]) {
    assert(matrix->n_rows == 0);
    mpc_matrix_init(matrix, n_rows, n_rows);

    FILE *f = fopen(filename, "r");
    while (!feof(f)) {
        int a = 0, b = 0;
        double prob = 0;

        fscanf(f, "%d,%d,%lf\n", &a, &b, &prob);
        assert(abs(a) < matrix->n_rows);
        assert(abs(b) < matrix->n_cols);
        a = MOD(a, matrix->n_rows);
        b = MOD(b, matrix->n_rows);
        mpc_set_d (matrix->m[a][b], prob, MPFR_RNDU);
    }
    fclose(f);
}

void mpfr_matrix_init(mpfr_matrix_t *matrix, int n_rows, int n_cols) {
    matrix->n_rows = n_rows;
    matrix->n_cols = n_cols;

    matrix->m = malloc(n_rows * sizeof(*matrix->m));
    for (int i = 0; i < n_rows; i++) {
        matrix->m[i] = malloc(n_cols * sizeof(**matrix->m));
        for (int j = 0; j < n_cols; j++) {
            mpfr_init2(matrix->m[i][j], N_PRECISION_BITS);
            mpfr_set_d(matrix->m[i][j], 0, MPFR_RNDD);
        }
    }
}

void mpfr_matrix_free(mpfr_matrix_t *matrix) {

    for (int i = 0; i < matrix->n_rows; i++) {
        for (int j = 0; j < matrix->n_cols; j++) {
            mpfr_clear(matrix->m[i][j]);
        }
        free(matrix->m[i]);
    }
    free(matrix->m);
    matrix->n_rows = 0;
    matrix->n_cols = 0;
}

void mpfr_matrix_init_from_csv(mpfr_matrix_t *matrix, char filename[]) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "Could not open file %s\n", filename);
        exit(1);
    }
    while (!feof(f)) {
        char str[STR_MAX] = {0};
        fgets(str, STR_MAX, f);
        if (feof(f)) {
            assert(strlen(str) == 0);
            break;
        }
        int i;
        char *token = strtok(str, ",");
        sscanf(token, "%d", &i);

        int j;
        token = strtok(NULL, ",");
        sscanf(token, "%d", &j);

        mpfr_t real_part;
        mpfr_init2(real_part, N_PRECISION_BITS);
        token = strtok(NULL, ",");
        mpfr_set_str(real_part, token, 10, MPFR_RNDD);

        int im_part;
        token = strtok(NULL, ",");
        sscanf(token, "%d", &im_part);
        assert(im_part == 0);

        // To check if the format was correct
        token = strtok(NULL, ",");
        assert(token == NULL);

        mpfr_set(matrix->m[MOD(i, N_SUPPORT_DIST_2D)][MOD(j, N_SUPPORT_DIST_2D)],
                 real_part,
                 MPFR_RNDD);


        mpfr_clear(real_part);
    }

    fclose(f);
}

void mpc_matrix_to_mpfr_matrix(mpfr_matrix_t *real_matrix, mpc_matrix_t *complex_matrix) {
    assert(real_matrix->n_rows == complex_matrix->n_rows);
    assert(real_matrix->n_cols == complex_matrix->n_cols);

    mpfr_t real_part;
    mpfr_init2(real_part, N_PRECISION_BITS);
    mpfr_t imag_part;
    mpfr_init2(imag_part, N_PRECISION_BITS);

    mpfr_t zero;
    mpfr_init2(zero, N_PRECISION_BITS);
    mpfr_set_d(zero, ZERO_DOUBLE, MPFR_RNDD);

    for (int i = 0; i < complex_matrix->n_rows; i++) {
        for (int j = 0; j < complex_matrix->n_cols; j++) {
            mpc_imag(imag_part, complex_matrix->m[i][j], ROUND_DOWN);
            int cmp_imag = mpfr_cmp_abs(imag_part, zero);
            // Makes sure that the imaginary part is 0
            assert(cmp_imag <= 0);

            mpc_real(real_part, complex_matrix->m[i][j], ROUND_DOWN);
            mpfr_set(real_matrix->m[i][j], real_part, ROUND_DOWN);
        }
    }
}

