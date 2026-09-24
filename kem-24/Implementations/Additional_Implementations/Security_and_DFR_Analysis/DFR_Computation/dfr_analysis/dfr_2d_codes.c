// Copyright 2025 LG Electronics, Inc. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <mpfr.h>
#include <sys/resource.h>
#include <omp.h>
#include <stdint.h>

#include "mp_matrix.h"
#include "common.h"
#include "decoding_lines.h"
// #include "minal_codes/minal_b2_codes.h"
#include "minal.h"

#define N_CODEWORDS_FOR_B1 4
#define N_CODEWORDS_FOR_B2 16

#define STR_MAX 1000

typedef struct point_s {
    int x[2];
} point_t;

void print_point(point_t *point) {
    printf("(%d, %d)\n", point->x[0], point->x[1]);
}

static __inline__ int get_distance_sqr(point_t *a, point_t *b) {
    int x0_diff = a->x[0] - b->x[0];
    int x1_diff = a->x[1] - b->x[1];
    return x0_diff * x0_diff + x1_diff * x1_diff;
}

static __inline__ uint32_t lower_than_mask32(const uint32_t v1, const uint32_t v2) {
  return -((v1 - v2) >> 31);
}

static __inline__ int64_t get_distance_sqr_reduced_points(point_t *a, point_t *b, int kem_q) {

    int64_t min_dist_sqr = kem_q * kem_q;

    #pragma GCC unroll 16
    for (int x0 = -kem_q; x0 <= kem_q; x0 += kem_q) {
        #pragma GCC unroll 16
        for (int x1 = -kem_q; x1 <= kem_q; x1 += kem_q) {
            point_t tmp_a = {{a->x[0] + x0, a->x[1] + x1}};
            int64_t dist_sqr = get_distance_sqr(&tmp_a, b);
            min_dist_sqr = dist_sqr < min_dist_sqr ? dist_sqr : min_dist_sqr;
        }
    }
    return min_dist_sqr;
}

int find_closest_codeword(point_t codewords[], int n_codewords, point_t *target_brute, int kem_q) {

    point_t target = {{MOD(target_brute->x[0], kem_q), MOD(target_brute->x[1], kem_q)}};

    int64_t min_dist_sqr = kem_q * kem_q;
    int closest_codeword_index = -1;
    #pragma GCC unroll 16
    for (int i = 0; i < n_codewords; i++) {
        int64_t dist_sqr = get_distance_sqr_reduced_points(&target, &codewords[i], kem_q);
        if (dist_sqr < min_dist_sqr) {
            closest_codeword_index = i;
            min_dist_sqr = dist_sqr;
        }
    }

    return closest_codeword_index;
}

// This is a general decoding algorithm, that, although less inefficient, can be applied to multiple parameter
// sets without much trouble. In particular, we don't need decoding lines or to know other geometrical properties
// of the code.
void get_dfr_for_2d_code_with_minimum_distance_decoding(mpfr_t error_prob, mpfr_matrix_t *delta_m, int code_alpha, int code_beta,
                                                        int kem_q, int kem_n, int kem_message_bit_length) {

    int nthreads = omp_get_max_threads();

    assert(0 <= code_alpha);
    assert(code_alpha < kem_q);
    assert(0 <= code_beta);
    assert(code_beta < kem_q);
    assert(code_beta + code_beta < kem_q);
    assert(kem_message_bit_length / kem_n == 1); // This function assumes ENCODING_B == 1.

    point_t points[N_CODEWORDS_FOR_B1] = {
        {{0, 0}},
        {{code_beta, code_alpha}},
        {{code_alpha, code_beta}},
        {{code_alpha + code_beta, code_alpha + code_beta}},
    };


    mpfr_t pair_error_prob_for_thread[nthreads];
    for (int t = 0; t < nthreads; t++) {
        mpfr_init2(pair_error_prob_for_thread[t], N_PRECISION_BITS);
        mpfr_set_d(pair_error_prob_for_thread[t], 0, MPFR_RNDD);
    }

    for (int j = 0; j < N_CODEWORDS_FOR_B1; j++) {

        // Iterates over possible pairs of errors (e0, e1)
        #pragma omp parallel for schedule(static) // Using static scheduling the results are reproducible
        for (int a0 = 0; a0 < N_SUPPORT_DIST_2D; a0++) {
            int t = omp_get_thread_num();
            for (int a1 = 0; a1 < N_SUPPORT_DIST_2D; a1++) {
                int e0 = center_mod(a0, N_SUPPORT_DIST_2D);
                int e1 = center_mod(a1, N_SUPPORT_DIST_2D);

                point_t target = {{points[j].x[0] + e0, points[j].x[1] + e1}};
                int i = find_closest_codeword(points, N_CODEWORDS_FOR_B1, &target, kem_q);

                if (i != j) {
                    mpfr_add(pair_error_prob_for_thread[t],
                             pair_error_prob_for_thread[t],
                             delta_m->m[a0][a1],
                             MPFR_RNDD);
                }
            }
        }
    }
    // mpfr_t error_prob;
    // mpfr_init2(error_prob, N_PRECISION_BITS);
    mpfr_set_d(error_prob, 0, MPFR_RNDD);

    // Sums the partial results from each thread
    for (int t = 0; t < nthreads; t++) {
        mpfr_add(error_prob, error_prob, pair_error_prob_for_thread[t], MPFR_RNDD);
    }
    // Multiplies by probability of each codeword appearing (division by 4)
    mpfr_div_ui(error_prob, error_prob, N_CODEWORDS_FOR_B1, MPFR_RNDD);

    // Important: At this point we have the error_prob for 2B coefficients (2B = 2 * MESSAGE_BIT_LENGTH / N)

    // Union bound over pairs of coefficients (n/2) to get the final bound for the full `MESSAGE_BIT_LENGTH`
    mpfr_mul_ui(error_prob, error_prob, kem_n / 2, MPFR_RNDD);

    for (int t = 0; t < nthreads; t++) {
        mpfr_clear(pair_error_prob_for_thread[t]);
    }
}



// This is a general decoding algorithm, that, although less inefficient, can be applied to multiple parameter
// sets without much trouble.
void get_dfr_for_2d_code_with_minimum_distance_decoding_for_b2_using_generic_decoder(mpfr_t error_prob, mpfr_matrix_t *delta_m, int code_alpha, int code_beta,
                                                                                     int kem_q, int kem_n, int kem_message_bit_length) {

    int nthreads = omp_get_max_threads();

    assert(0 <= code_alpha);
    assert(code_alpha < kem_q);
    assert(0 <= code_beta);
    assert(code_beta < kem_q);
    assert(code_beta + code_beta < kem_q);

    int half_kem_q = kem_q / 2;

    assert(kem_message_bit_length / kem_n == 2); // This function assumes ENCODING_B == 1.

    point_t points[N_CODEWORDS_FOR_B2] = {
        {{0, 0}},
        {{code_beta, code_alpha}},
        {{code_alpha, code_beta}},
        {{code_alpha + code_beta, code_alpha + code_beta}},

        {{half_kem_q + 0, 0}},
        {{half_kem_q + code_beta, code_alpha}},
        {{half_kem_q + code_alpha, code_beta}},
        {{half_kem_q + code_alpha + code_beta, code_alpha + code_beta}},

        {{0, half_kem_q + 0}},
        {{code_beta, half_kem_q + code_alpha}},
        {{code_alpha, half_kem_q + code_beta}},
        {{code_alpha + code_beta, half_kem_q + code_alpha + code_beta}},

        {{half_kem_q + 0, half_kem_q + 0}},
        {{half_kem_q + code_beta, half_kem_q + code_alpha}},
        {{half_kem_q + code_alpha, half_kem_q + code_beta}},
        {{half_kem_q + code_alpha + code_beta, half_kem_q + code_alpha + code_beta}},

    };


    mpfr_t pair_error_prob_for_thread[nthreads];
    for (int t = 0; t < nthreads; t++) {
        mpfr_init2(pair_error_prob_for_thread[t], N_PRECISION_BITS);
        mpfr_set_d(pair_error_prob_for_thread[t], 0, MPFR_RNDD);
    }

    for (int j = 0; j < N_CODEWORDS_FOR_B2; j++) {

        // Iterates over possible pairs of errors (e0, e1)
        #pragma omp parallel for schedule(static) // Using static scheduling the results are reproducible
        for (int a0 = 0; a0 < N_SUPPORT_DIST_2D; a0++) {
            int t = omp_get_thread_num();
            for (int a1 = 0; a1 < N_SUPPORT_DIST_2D; a1++) {
                int e0 = center_mod(a0, N_SUPPORT_DIST_2D);
                int e1 = center_mod(a1, N_SUPPORT_DIST_2D);

                point_t target = {{points[j].x[0] + e0, points[j].x[1] + e1}};
                int i = find_closest_codeword(points, N_CODEWORDS_FOR_B2, &target, kem_q);

                if (i != j) {
                    mpfr_add(pair_error_prob_for_thread[t],
                             pair_error_prob_for_thread[t],
                             delta_m->m[a0][a1],
                             MPFR_RNDD);
                }
            }
        }
    }
    // mpfr_t error_prob;
    // mpfr_init2(error_prob, N_PRECISION_BITS);
    mpfr_set_d(error_prob, 0, MPFR_RNDD);

    // Sums the partial results from each thread
    for (int t = 0; t < nthreads; t++) {
        mpfr_add(error_prob, error_prob, pair_error_prob_for_thread[t], MPFR_RNDD);
    }
    // Multiplies by probability of each codeword appearing (division by 4)
    mpfr_div_ui(error_prob, error_prob, N_CODEWORDS_FOR_B2, MPFR_RNDD);

    // Important: At this point we have the error_prob for 2B coefficients (2B = 2 * MESSAGE_BIT_LENGTH / N)

    // Union bound over pairs of coefficients (n/2) to get the final bound for the full `MESSAGE_BIT_LENGTH`
    mpfr_mul_ui(error_prob, error_prob, kem_n / 2, MPFR_RNDD);

    for (int t = 0; t < nthreads; t++) {
        mpfr_clear(pair_error_prob_for_thread[t]);
    }
}

// This is uses the real decoder for the internal minal_code.
void get_dfr_for_2d_code_with_minimum_distance_decoding_for_b2(mpfr_t error_prob, mpfr_matrix_t *delta_m, int code_alpha, int code_beta,
                                                               int kem_q, int kem_n, int kem_message_bit_length) {

    int nthreads = omp_get_max_threads();

    assert(0 <= code_alpha);
    assert(code_alpha < kem_q);
    assert(0 <= code_beta);
    assert(code_beta < kem_q);
    assert(code_beta + code_beta < kem_q);

    assert(kem_message_bit_length / kem_n == 2); // This function assumes ENCODING_B == 2

    // minal_b2_params_t minal;
    // minal_b2_code_init(&minal, kem_q, code_beta, 2);

    mpfr_t pair_error_prob_for_thread[nthreads];
    for (int t = 0; t < nthreads; t++) {
        mpfr_init2(pair_error_prob_for_thread[t], N_PRECISION_BITS);
        mpfr_set_d(pair_error_prob_for_thread[t], 0, MPFR_RNDD);
    }

    minal_b2_init(kem_q, code_beta);

    for (int j = 0; j < N_CODEWORDS_FOR_B2; j++) {
        uint8_t msg_bits[4] = {
            (j >> 3) & 1,
            (j >> 2) & 1,
            (j >> 1) & 1,
            (j >> 0) & 1,
        };
        int16_t codeword[2];
        minal_b2_code_encode(codeword, msg_bits);

        // Iterates over possible pairs of errors (e0, e1)
        #pragma omp parallel for schedule(static) // Using static scheduling the results are reproducible
        for (int a0 = 0; a0 < N_SUPPORT_DIST_2D; a0++) {
            int t = omp_get_thread_num();
            for (int a1 = 0; a1 < N_SUPPORT_DIST_2D; a1++) {
                int e0 = center_mod(a0, N_SUPPORT_DIST_2D);
                int e1 = center_mod(a1, N_SUPPORT_DIST_2D);

                int16_t target[2] = {codeword[0] + e0, codeword[1] + e1};
                uint16_t i = minal_b2_code_decode(target);

                if (i != j) {
                    mpfr_add(pair_error_prob_for_thread[t],
                             pair_error_prob_for_thread[t],
                             delta_m->m[a0][a1],
                             MPFR_RNDD);
                }
            }
        }
    }
    // mpfr_t error_prob;
    // mpfr_init2(error_prob, N_PRECISION_BITS);
    mpfr_set_d(error_prob, 0, MPFR_RNDD);

    // Sums the partial results from each thread
    for (int t = 0; t < nthreads; t++) {
        mpfr_add(error_prob, error_prob, pair_error_prob_for_thread[t], MPFR_RNDD);
    }
    // Multiplies by probability of each codeword appearing (division by 4)
    mpfr_div_ui(error_prob, error_prob, N_CODEWORDS_FOR_B2, MPFR_RNDD);

    // Important: At this point we have the error_prob for 2B coefficients (2B = 2 * MESSAGE_BIT_LENGTH / N)

    // Union bound over pairs of coefficients (n/2) to get the final bound for the full `MESSAGE_BIT_LENGTH`
    mpfr_mul_ui(error_prob, error_prob, kem_n / 2, MPFR_RNDD);

    for (int t = 0; t < nthreads; t++) {
        mpfr_clear(pair_error_prob_for_thread[t]);
    }
}


void find_best_code(mpfr_matrix_t *delta_m, int kem_q) {

    for (int code_beta = 0; code_beta < kem_q/2; code_beta++) {

        mpfr_t error_prob;
        mpfr_init2(error_prob, N_PRECISION_BITS);
        mpfr_set_d(error_prob, 0, MPFR_RNDD);


        printf("%d,", code_beta);

        // You can uncomment the lines below to print the DFR computation using the minimum distance decoder.
        get_dfr_for_2d_code_with_minimum_distance_decoding(error_prob, delta_m, kem_q/2, code_beta, kem_q, 256, 256);
        mpfr_out_str(stdout, 10, 0, error_prob, MPFR_RNDD);
        // printf(",");

        // get_dfr_for_2d_code_with_real_decoder(error_prob, delta_m, kem_q/2, code_beta, kem_q);
        // mpfr_out_str(stdout, 10, 0, error_prob, MPFR_RNDD);
        printf("\n"); fflush(stdout);

        mpfr_clear(error_prob);
    }
}

#ifdef MAIN_PROGRAM

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s kem_q delta_m.csv\n", argv[0]);
        exit(1);
    }
    int kem_q = atoi(argv[1]);
    mpfr_matrix_t delta_m;
    printf("Reading delta_m file... "); fflush(stdout);
    mpfr_matrix_init(&delta_m, N_SUPPORT_DIST_2D, N_SUPPORT_DIST_2D);
    mpfr_matrix_init_from_csv(&delta_m, argv[2]);
    printf("Done.\n"); fflush(stdout);
    find_best_code(&delta_m, kem_q);
    mpfr_matrix_free(&delta_m);
}

#endif
