// Copyright 2025 LG Electronics, Inc. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <gmp.h>
#include <mpc.h>
#include <omp.h>
#include <stdint.h>

#include "mp_matrix.h"
#include "fft_mpc.h"

#define MOD(a, b) ((((a) % (b)) + (b)) % (b))

static mpc_t *TWIDDLES_FOR_POWER_OF_TWO[MAX_N_LEVELS_FOR_PURE_FFT + 1];
static mpc_t *INVERSE_TWIDDLES_FOR_POWER_OF_TWO[MAX_N_LEVELS_FOR_PURE_FFT + 1];
static int FFT_MPC_IS_INITIALIZED = 0;


void __precompute_TWIDDLES_FOR_POWER_OF_TWO() {
    fprintf(stderr, "\nforward levels: ");
    for (int n_levels = 0; n_levels <= MAX_N_LEVELS_FOR_PURE_FFT; n_levels++) {
        int n = (1 << n_levels);
        int idx = 0;
        TWIDDLES_FOR_POWER_OF_TWO[n_levels] = malloc(n * n_levels * sizeof(mpc_t));
        for (int s = n_levels; s >= 0; s--) {
            int m = (1 << s);

            for (int k = 0; k < n; k += m) {
                #pragma omp parallel for schedule(dynamic)
                for (int j = 0; j < m/2; j++) {
                    int tmp_idx = idx + j;
                    mpc_init2(TWIDDLES_FOR_POWER_OF_TWO[n_levels][tmp_idx], N_PRECISION_BITS);
                    mpc_rootofunity(TWIDDLES_FOR_POWER_OF_TWO[n_levels][tmp_idx], n, -j * (1 << (n_levels - s)), ROUND_DOWN);
                }
                idx += m / 2;
            }
        }
        fprintf(stderr, "%d..", n_levels);
        fflush(stderr);
    }
    fprintf(stderr, "\n");
}

void __clear_TWIDDLES_FOR_POWER_OF_TWO() {
    for (int n_levels = 0; n_levels <= MAX_N_LEVELS_FOR_PURE_FFT; n_levels++) {
        int n = (1 << n_levels);
        int idx = 0;
        for (int s = n_levels; s >= 0; s--) {
            int m = (1 << s);

            for (int k = 0; k < n; k += m) {
                for (int j = 0; j < m/2; j++) {
                    mpc_clear(TWIDDLES_FOR_POWER_OF_TWO[n_levels][idx++]);
                }
            }
        }
        free(TWIDDLES_FOR_POWER_OF_TWO[n_levels]);
    }
}

void __precompute_INVERSE_TWIDDLES_FOR_POWER_OF_TWO() {
    fprintf(stderr, "\ninverse levels: ");
    for (int n_levels = 0; n_levels <= MAX_N_LEVELS_FOR_PURE_FFT; n_levels++) {
        int n = (1 << n_levels);
        int idx = 0;
        INVERSE_TWIDDLES_FOR_POWER_OF_TWO[n_levels] = malloc(n * n_levels * sizeof(mpc_t));
        for (int s = 0; s <= n_levels; s++) {
            int m = (1 << s);

            for (int k = 0; k < n; k += m) {
                #pragma omp parallel for schedule(dynamic)
                for (int j = 0; j < m/2; j++) {
                    mpc_init2(INVERSE_TWIDDLES_FOR_POWER_OF_TWO[n_levels][idx + j], N_PRECISION_BITS);
                    mpc_rootofunity(INVERSE_TWIDDLES_FOR_POWER_OF_TWO[n_levels][idx + j], n, j * (1 << (n_levels - s)), ROUND_DOWN);
                }
                idx += m/2;
            }
        }
        fprintf(stderr, "%d..", n_levels);
        fflush(stderr);
    }
    fprintf(stderr, "\n");
}

void __clear_INVERSE_TWIDDLES_FOR_POWER_OF_TWO() {
    for (int n_levels = 0; n_levels <= MAX_N_LEVELS_FOR_PURE_FFT; n_levels++) {
        int n = (1 << n_levels);
        int idx = 0;
        for (int s = 0; s <= n_levels; s++) {
            int m = (1 << s);

            for (int k = 0; k < n; k += m) {
                for (int j = 0; j < m/2; j++) {
                    mpc_clear(INVERSE_TWIDDLES_FOR_POWER_OF_TWO[n_levels][idx++]);
                }
            }
        }
        free(INVERSE_TWIDDLES_FOR_POWER_OF_TWO[n_levels]);
    }
}

void init_fft_mpc() {
    assert(FFT_MPC_IS_INITIALIZED == 0);
    fprintf(stderr, "Initializing twiddle factors for lenghts up to 2^%d...", MAX_N_LEVELS_FOR_PURE_FFT);
    __precompute_TWIDDLES_FOR_POWER_OF_TWO();
    __precompute_INVERSE_TWIDDLES_FOR_POWER_OF_TWO();
    FFT_MPC_IS_INITIALIZED = 1;
    fprintf(stderr, " Done!\n");
}

void clear_fft_mpc() {
    assert(FFT_MPC_IS_INITIALIZED == 1);
    FFT_MPC_IS_INITIALIZED = 0;
    __clear_TWIDDLES_FOR_POWER_OF_TWO();
    __clear_INVERSE_TWIDDLES_FOR_POWER_OF_TWO();
}

int32_t ilog2(uint32_t x) {
    return sizeof(uint32_t) * CHAR_BIT -  __builtin_clz(x) - 1;
}


void fft(mpc_t *vector, int n) {
    if (n == 1) {
        return;
    }

    assert(__builtin_popcount(n) == 1); // Assert n is a power of two

    assert(n <= MAX_N_FOR_FFT);

    if (n <= MAX_N_FOR_PURE_FFT) {
        fft_pure(vector, n);
    }
    else {
        assert(n <= MAX_N_FOR_BAILEY_FFT);

        int n_levels = ilog2(n);

        int n_rows = 1 << (n_levels / 2);
        int n_cols = n / n_rows;

        // // printf("n = %d\n", n);
        // // printf("n_rows = %d\n", n_rows);
        // // printf("n_cols = %d\n", n_cols);

        // fflush(stdout);

        fft_bailey(vector, n, n_rows, n_cols);
    }
}

void ifft(mpc_t *vector, int n) {
    if (n == 1) {
        return;
    }

    assert(__builtin_popcount(n) == 1); // Assert n is a power of two

    assert(n <= MAX_N_FOR_FFT);

    if (n <= MAX_N_FOR_PURE_FFT) {
        ifft_pure(vector, n);
    }
    else {
        assert(n <= MAX_N_FOR_BAILEY_FFT);

        int n_levels = ilog2(n);

        int n_rows = 1 << (n_levels / 2);
        int n_cols = n / n_rows;

        ifft_bailey(vector, n, n_rows, n_cols);
    }
}




void fft_pure(mpc_t *vector, int n) {
    assert(__builtin_popcount(n) == 1); // Assert n is a power of two
    assert(FFT_MPC_IS_INITIALIZED);

    int n_levels = ilog2(n);
    assert(n_levels <= MAX_N_LEVELS_FOR_PURE_FFT);

    mpc_t w, t, u;
    mpc_init2(w, N_PRECISION_BITS);
    mpc_init2(t, N_PRECISION_BITS);
    mpc_init2(u, N_PRECISION_BITS);


    int idx = 0;
    for (int s = n_levels; s >= 0; s--) {
        int m = (1 << s);

        for (int k = 0; k < n; k += m) {
            for (int j = 0; j < m/2; j++) {

                mpc_set(t, vector[k + j], ROUND_DOWN);
                mpc_set(u, vector[k + j +  m / 2], ROUND_DOWN);
                mpc_add(vector[k + j], t, u, ROUND_DOWN);
                mpc_sub(vector[k + j + m / 2], t, u, ROUND_DOWN);
                mpc_mul(vector[k + j + m / 2], vector[k + j + m / 2], TWIDDLES_FOR_POWER_OF_TWO[n_levels][idx++], ROUND_DOWN);

                // If TWIDDLES_FOR_POWER_OF_TWO was not precomputed, then use:
                //      mpc_rootofunity(w, n, -j * (1 << (n_levels - s)), ROUND_DOWN);
                //      mpc_mul(vector[k + j + m / 2], vector[k + j + m / 2], w, ROUND_DOWN);
            }
        }
    }
    mpc_clear(w);
    mpc_clear(t);
    mpc_clear(u);
}

void ifft_pure(mpc_t *vector, int n) {
    assert(__builtin_popcount(n) == 1); // Assert n is a power of two
    assert(FFT_MPC_IS_INITIALIZED);

    int n_levels = ilog2(n);
    assert(n_levels <= MAX_N_LEVELS_FOR_PURE_FFT);

    mpc_t w, t, u;
    mpc_init2(w, N_PRECISION_BITS);
    mpc_init2(t, N_PRECISION_BITS);
    mpc_init2(u, N_PRECISION_BITS);

    int idx = 0;
    for (int s = 0; s <= n_levels; s++) {
        int m = (1 << s);

        for (int k = 0; k < n; k += m) {

            for (int j = 0; j < m/2; j++) {

                mpc_set(t, vector[k + j], ROUND_DOWN);
                mpc_set(u, vector[k + j +  m / 2], ROUND_DOWN);

                // If INVERSE_TWIDDLES_FOR_POWER_OF_TWO was not precomputed, then use:
                // mpc_rootofunity(w, n, j * (1 << (n_levels - s)), ROUND_DOWN);
                // mpc_mul(u, u, w, ROUND_DOWN);

                mpc_mul(u, u, INVERSE_TWIDDLES_FOR_POWER_OF_TWO[n_levels][idx++], ROUND_DOWN);
                mpc_add(vector[k + j], t, u, ROUND_DOWN);
                mpc_sub(vector[k + j + m / 2], t, u, ROUND_DOWN);
            }
        }
    }

    for (int i = 0; i < n; i++) {
        mpc_div_ui(vector[i], vector[i], n, ROUND_DOWN);
    }
    mpc_clear(w);
    mpc_clear(t);
    mpc_clear(u);
}


void fft2(mpc_matrix_t *matrix) {
    assert(__builtin_popcount(matrix->n_rows) == 1); // Assert n is a power of two
    assert(__builtin_popcount(matrix->n_cols) == 1); // Assert n is a power of two

    fprintf(stderr, "[*] FFT of rows... "); fflush(stderr);
    #pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < matrix->n_rows; i++) {
        fft(matrix->m[i], matrix->n_cols);
    }
    fprintf(stderr, "Done.\n"); fflush(stderr);

    int nthreads = omp_get_max_threads();

    mpc_t tmp[nthreads][matrix->n_rows];
    for (int t = 0; t < nthreads; t++) {
        for (int i = 0; i < matrix->n_rows; i++) {
            mpc_init2(tmp[t][i], N_PRECISION_BITS);
        }
    }

    fprintf(stderr, "[*] FFT of columns... "); fflush(stderr);

    #pragma omp parallel for schedule(dynamic)
    for (int j = 0; j < matrix->n_cols; j++) {
        int idx = omp_get_thread_num();
        for (int i = 0; i < matrix->n_rows; i++) {
            mpc_set(tmp[idx][i], matrix->m[i][j], ROUND_DOWN);
        }
        fft(tmp[idx], matrix->n_rows);
        for (int i = 0; i < matrix->n_rows; i++) {
            mpc_set(matrix->m[i][j], tmp[idx][i], ROUND_DOWN);
        }
    }
    fprintf(stderr, "Done.\n"); fflush(stderr);


    for (int t = 0; t < nthreads; t++) {
        for (int i = 0; i < matrix->n_rows; i++) {
            mpc_clear(tmp[t][i]);
        }
    }
}

void ifft2(mpc_matrix_t *matrix) {
    assert(__builtin_popcount(matrix->n_rows) == 1); // Assert n is a power of two
    assert(__builtin_popcount(matrix->n_cols) == 1); // Assert n is a power of two

    fprintf(stderr, "[*] Inverse FFT of rows... "); fflush(stderr);
    #pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < matrix->n_rows; i++) {
        ifft(matrix->m[i], matrix->n_cols);
    }
    fprintf(stderr, "Done.\n"); fflush(stderr);

    int nthreads = omp_get_max_threads();

    mpc_t tmp[nthreads][matrix->n_rows];
    for (int t = 0; t < nthreads; t++) {
        for (int i = 0; i < matrix->n_rows; i++) {
            mpc_init2(tmp[t][i], N_PRECISION_BITS);
        }
    }

    fprintf(stderr, "[*] Inverse FFT of columns... "); fflush(stderr);
    #pragma omp parallel for schedule(dynamic)
    for (int j = 0; j < matrix->n_cols; j++) {
        int idx = omp_get_thread_num();
        for (int i = 0; i < matrix->n_rows; i++) {
            mpc_set(tmp[idx][i], matrix->m[i][j], ROUND_DOWN);
        }
        ifft(tmp[idx], matrix->n_rows);
        for (int i = 0; i < matrix->n_rows; i++) {
            mpc_set(matrix->m[i][j], tmp[idx][i], ROUND_DOWN);
        }
    }
    fprintf(stderr, "Done.\n"); fflush(stderr);

    for (int t = 0; t < nthreads; t++) {
        for (int i = 0; i < matrix->n_rows; i++) {
            mpc_clear(tmp[t][i]);
        }
    }
}

int bit_rev(int x, int length) {
    int ret = 0;
    int power = 0;

    while (x > 0) {
        ret += (x % 2) * (1 << (length - 1 - power));
        power += 1;
        x /= 2;
    }

    return ret;
}


void fft_bailey(mpc_t *vector, int n, int n_rows, int n_cols) {
    assert(n = n_rows * n_cols);

    assert(__builtin_popcount(n_rows) == 1); // Assert `n_rows` is a power of two
    assert(__builtin_popcount(n_cols) == 1); // Assert `n_cols` is a power of two

    mpc_matrix_t matrix;
    mpc_matrix_init(&matrix, n_rows, n_cols);

    // Initialize the temporary rows that are used to efficiently compute the FFT over columns
    int nthreads = omp_get_max_threads();
    mpc_t tmp[nthreads][matrix.n_rows];
    for (int t = 0; t < nthreads; t++) {
        for (int i = 0; i < matrix.n_rows; i++) {
            mpc_init2(tmp[t][i], N_PRECISION_BITS);
        }
    }

    fprintf(stderr, "[*] Starting FFT (Bailey): ");
    fprintf(stderr, "Reshaping... "); fflush(stderr);
    // Reshape vector as a `n_rows * n_cols` matrix
    int idx = 0;
    for (int i = 0; i < matrix.n_rows; i++) {
        for (int j = 0; j < matrix.n_cols; j++) {
            mpc_set(matrix.m[i][j], vector[idx++], MPFR_RNDD);
        }
    }

    fprintf(stderr, "FFT cols... "); fflush(stderr);
    // Apply FFT over the columns of `matrix`
    #pragma omp parallel for schedule(dynamic)
    for (int j = 0; j < matrix.n_cols; j++) {
        int thread_id = omp_get_thread_num();
        for (int i = 0; i < matrix.n_rows; i++) {
            mpc_set(tmp[thread_id][i], matrix.m[i][j], ROUND_DOWN);
        }
        fft_pure(tmp[thread_id], matrix.n_rows);
        for (int i = 0; i < matrix.n_rows; i++) {
            mpc_set(matrix.m[i][j], tmp[thread_id][i], ROUND_DOWN);
        }
    }

    fprintf(stderr, "Correction factor... "); fflush(stderr);
    // Multiply by correction factor
    int log2_n_rows = ilog2(n_rows);
    for (int i = 0; i < matrix.n_rows; i++) {
        int bit_rev_i = bit_rev(i, log2_n_rows);
        #pragma omp parallel for schedule(dynamic)
        for (int j = 0; j < matrix.n_cols; j++) {
            int thread_id = omp_get_thread_num();
            mpc_rootofunity(tmp[thread_id][0], n, -j * bit_rev_i, ROUND_DOWN);
            mpc_mul(matrix.m[i][j], matrix.m[i][j], tmp[thread_id][0], MPFR_RNDD);
        }
    }

    fprintf(stderr, "FFT rows... "); fflush(stderr);
    // Apply FFT over the rows of `matrix`
    #pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < matrix.n_rows; i++) {
        fft_pure(matrix.m[i], matrix.n_cols);
    }

    // Clear space used by temporary rows
    for (int t = 0; t < nthreads; t++) {
        for (int i = 0; i < matrix.n_rows; i++) {
            mpc_clear(tmp[t][i]);
        }
    }

    fprintf(stderr, "Writing result... "); fflush(stderr);

    idx = 0;
    for (int i = 0; i < matrix.n_rows; i++) {
        for (int j = 0; j < matrix.n_cols; j++) {
            mpc_set(vector[idx++], matrix.m[i][j], MPFR_RNDD);
        }
    }
    fprintf(stderr, "Done!\n"); fflush(stderr);


    mpc_matrix_free(&matrix);
}


void ifft_bailey(mpc_t *vector, int n, int n_rows, int n_cols) {
    assert(n = n_rows * n_cols);

    assert(__builtin_popcount(n_rows) == 1); // Assert `n_rows` is a power of two
    assert(__builtin_popcount(n_cols) == 1); // Assert `n_cols` is a power of two

    mpc_matrix_t matrix;
    mpc_matrix_init(&matrix, n_rows, n_cols);

    // Initialize the temporary rows that are used to efficiently compute the FFT over columns
    int nthreads = omp_get_max_threads();
    mpc_t tmp[nthreads][matrix.n_rows];
    for (int t = 0; t < nthreads; t++) {
        for (int i = 0; i < matrix.n_rows; i++) {
            mpc_init2(tmp[t][i], N_PRECISION_BITS);
        }
    }

    fprintf(stderr, "[*] Starting IFFT (Bailey): ");
    fprintf(stderr, "Reshaping... "); fflush(stderr);
    // Reshape vector as a `n_rows * n_cols` matrix
    int idx = 0;
    for (int i = 0; i < matrix.n_rows; i++) {
        for (int j = 0; j < matrix.n_cols; j++) {
            mpc_set(matrix.m[i][j], vector[idx++], MPFR_RNDD);
        }
    }

    fprintf(stderr, "IFFT cols... "); fflush(stderr);
    // Apply IFFT over the rows of `matrix`
    #pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < matrix.n_rows; i++) {
        ifft_pure(matrix.m[i], matrix.n_cols);
    }

    fprintf(stderr, "Correction factor... "); fflush(stderr);
    // Multiply by correction factor
    int log2_n_rows = ilog2(n_rows);
    for (int i = 0; i < matrix.n_rows; i++) {
        int bit_rev_i = bit_rev(i, log2_n_rows);
        #pragma omp parallel for schedule(dynamic)
        for (int j = 0; j < matrix.n_cols; j++) {
            int thread_id = omp_get_thread_num();
            mpc_rootofunity(tmp[thread_id][0], n, j * bit_rev_i, ROUND_DOWN);
            mpc_mul(matrix.m[i][j], matrix.m[i][j], tmp[thread_id][0], MPFR_RNDD);
        }
    }

    fprintf(stderr, "IFFT rows... "); fflush(stderr);
    // Apply IFFT over the columns of `matrix`
    #pragma omp parallel for schedule(dynamic)
    for (int j = 0; j < matrix.n_cols; j++) {
        int thread_id = omp_get_thread_num();
        for (int i = 0; i < matrix.n_rows; i++) {
            mpc_set(tmp[thread_id][i], matrix.m[i][j], ROUND_DOWN);
        }
        ifft_pure(tmp[thread_id], matrix.n_rows);
        for (int i = 0; i < matrix.n_rows; i++) {
            mpc_set(matrix.m[i][j], tmp[thread_id][i], ROUND_DOWN);
        }
    }

    // Clear space used by temporary rows
    for (int t = 0; t < nthreads; t++) {
        for (int i = 0; i < matrix.n_rows; i++) {
            mpc_clear(tmp[t][i]);
        }
    }

    idx = 0;
    for (int i = 0; i < matrix.n_rows; i++) {
        for (int j = 0; j < matrix.n_cols; j++) {
            mpc_set(vector[idx++], matrix.m[i][j], MPFR_RNDD);
        }
    }

    mpc_matrix_free(&matrix);
}


#ifdef FFT_MPC_TEST

int main2(void) {

    int n = 32;
    mpc_t vector[32];

    for (int i = 0; i < n; i++) {
        mpc_init2(vector[i], N_PRECISION_BITS);
        mpc_set_d(vector[i], i, ROUND_DOWN);
    }
    for (int i = 0; i < n; i++) {
        printf("vector[%d] = ", i);
        mpc_out_str(stdout, 10, 0, vector[i], ROUND_DOWN);
        printf("\n");
    }
    fft(vector, n);
    for (int i = 0; i < n; i++) {
        printf("hat_vector[%d] = ", i);
        mpc_out_str(stdout, 10, 0, vector[i], ROUND_DOWN);
        printf("\n");
    }
    ifft(vector, n);
    for (int i = 0; i < n; i++) {
        printf("inv_hat_vector[%d] = ", i);
        mpc_out_str(stdout, 10, 0, vector[i], ROUND_DOWN);
        printf("\n");
    }
    for (int i = 0; i < n; i++) {
        mpc_clear(vector[i]);
    }
    return 0;
}


int main3(int argc, char *argv[]) {

    if (argc != 2) {
        fprintf(stderr, "Usage: %s dist_file.csv\n", argv[0]);
        exit(1);
    }

    init_fft_mpc();

    mpc_matrix_t matrix = {0};
    // mpc_matrix_init(&matrix, 16, 16);
    // mpc_matrix_init_square_from_file(&matrix, 2048*2, argv[1]);
    mpc_matrix_init_square_from_file(&matrix, 256, argv[1]);
    printf("original\n");
    mpc_matrix_print(&matrix);

    fft2(&matrix);

    for (int i = 0; i < matrix.n_rows; i++) {
        for (int j = 0; j < matrix.n_rows; j++) {
            mpc_pow_ui(matrix.m[i][j], matrix.m[i][j], 4, ROUND_DOWN);
        }
    }

    printf("fft\n");
    mpc_matrix_print(&matrix);

    ifft2(&matrix);

    printf("ifft\n");
    mpc_matrix_print(&matrix);

    mpc_matrix_free(&matrix);

    clear_fft_mpc();

    return 0;
}



int main(int argc, char *argv[]) {

    init_fft_mpc();

    int n = 16;
    mpc_t vector[n];
    for (int i = 0; i < n; i++) {
        mpc_init2(vector[i], N_PRECISION_BITS);
        mpc_set_d(vector[i], i, ROUND_DOWN);
    }

    fft_bailey(vector, n, 8, 2);

    printf("FFT (Bailey): \n");
    for (int i = 0; i < n; i++) {
        printf("  [%2d]: ", i);
        mpc_out_str(stdout, 10, 0, vector[i], ROUND_DOWN);
        printf("\n");
    }

    ifft_bailey(vector, n, 8, 2);

    printf("IFFT (Bailey): \n");
    for (int i = 0; i < n; i++) {
        printf("  [%2d]: ", i);
        mpc_out_str(stdout, 10, 0, vector[i], ROUND_DOWN);
        printf("\n");
    }

    for (int i = 0; i < n; i++) {
        mpc_clear(vector[i]);
    }

    clear_fft_mpc();

    return 0;
}


#endif
