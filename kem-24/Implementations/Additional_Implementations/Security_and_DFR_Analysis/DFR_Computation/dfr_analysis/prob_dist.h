// Copyright 2025 LG Electronics, Inc. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

typedef struct prob_dist_1d_s {
    int min_value;
    int max_value;
    int n;
    mpfr_t *d;
} prob_dist_1d_t;

void prob_dist_1d_init(prob_dist_1d_t *dist, int min_value, int max_value);
void prob_dist_1d_clear(prob_dist_1d_t *dist);
int prob_dist_1d_get_idx_for_value(prob_dist_1d_t *dist, int value);
void prob_dist_1d_add(prob_dist_1d_t *dist, int value, mpfr_t prob);
void prob_dist_1d_init_as_compress_decompress_error(prob_dist_1d_t *dist,
                                                    int n_bits_compressed, int kem_q);
void print_dist_1d(FILE *out, prob_dist_1d_t *dist);
void print_dist_1d_to_file(const char filename[], prob_dist_1d_t *dist);

void prob_dist_1d_init_as_centered_binomial(prob_dist_1d_t *dist, int eta);
void prob_dist_1d_init_as_sum(prob_dist_1d_t *out_dist, prob_dist_1d_t *dist1, prob_dist_1d_t *dist2);
int prob_dist_1d_is_symmetric(prob_dist_1d_t *dist);
int prob_dist_1d_update_value_to_next_one(prob_dist_1d_t *dist, int *value);

void prob_dist_1d_chernoff(mpfr_t out, double value, int n_summed_random_variables, prob_dist_1d_t *dist, double t);
void prob_dist_1d_minimize_chernoff_bound(mpfr_t out, double value, int n_summed_random_variables, prob_dist_1d_t *dist);
void prob_dist_1d_chernoff_with_precomputed_expectation_of_exp_tX(mpfr_t out, double value,
                                                                  int n_summed_random_variables,
                                                                  double t,
                                                                  mpfr_t expectation_of_exp_tX);
double prob_dist_1d_minimize_t_for_chernoff(double value, int n_summed_random_variables, prob_dist_1d_t *dist);
void prob_dist_1d_get_expectation_of_exp_tX(mpfr_t out, prob_dist_1d_t *dist, double t);
void prob_dist_1d_init_as_compress_decompress_error_plus(prob_dist_1d_t *dist,
                                                         int n_bits_compressed, int kem_q, double prob_plus);