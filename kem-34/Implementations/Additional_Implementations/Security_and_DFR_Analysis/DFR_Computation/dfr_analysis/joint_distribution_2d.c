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

#include "mp_matrix.h"
#include "fft_mpc.h"
#include "dfr_2d_codes.h"

#ifndef SAVE_PARTIAL_DISTRIBUTION_RESULTS
#define SAVE_PARTIAL_DISTRIBUTION_RESULTS 0
#endif

#define MOD(a, b) ((((a) % (b)) + (b)) % (b))

#define MAX_STR 1000
// #define MAX_DELTA 1665 // Maximum value of coefficients in delta_u and delta_v
// #define MAX_DELTA 7681/2 // Maximum value of coefficients in delta_u and delta_v
// #define MAX_DELTA (1 << 14) // Maximum value of coefficients in delta_u and delta_v
// #define MAX_DELTA 7681/2 // Maximum value of coefficients in delta_u and delta_v
// #define MAX_DELTA 3900 // Maximum value of coefficients in delta_u and delta_v

#define EXPECTED_SETUP_CSV_HEADER "KEM_Q,KEM_LEVEL,KEM_MESSAGE_BIT_LENGTH,KEM_N,KEM_K,KEM_ETA1,KEM_ETA2,KEM_N_BLOCKS_FOR_DU0,KEM_DU0,KEM_DU1,KEM_DV\n"

typedef struct kem_parameters_s {
    int KEM_Q;
    int KEM_LEVEL;
    int KEM_MESSAGE_BIT_LENGTH;
    int KEM_N; // This is the polynomial degree (`R_q = Z_q[X]/(x^n + 1)`), not necessarily the message length
    int KEM_K;
    int KEM_B_ENCODING; // Number of message bits encoded per coefficient: `KEM_MESSAGE_BIT_LENGTH / KEM_N`
    int KEM_ETA1;
    int KEM_ETA2;
    // We use a generalized view of `KEM_DU` as a vector of `KEM_K` entries of the form:
    // [KEM_DU0] * KEM_N_BLOCKS_FOR_DU0 + [KEM_DU1] * (KEM_K - KEM_N_BLOCKS_FOR_DU0)
    int KEM_N_BLOCKS_FOR_DU0;
    int KEM_DU0;
    int KEM_DU1;
    int KEM_DV;
} kem_parameters_t;


char CURRENT_FILE_PREFIX[MAX_STR/2] = {0};


typedef struct prob_dist_1d_s {
    int min_value;
    int max_value;
    int n;
    mpfr_t *d;
} prob_dist_1d_t;


void prob_dist_1d_init(prob_dist_1d_t *dist, int min_value, int max_value) {
    assert(max_value >= 0);
    assert(min_value <= 0);
    assert(max_value >= min_value);
    dist->min_value = min_value;
    dist->max_value = max_value;
    dist->n = (max_value - min_value) + 1;
    dist->d = malloc(dist->n * sizeof(*dist->d));

    for (int i = 0; i < dist->n; i++) {
        mpfr_init2(dist->d[i], N_PRECISION_BITS);
        mpfr_set_d(dist->d[i], 0, MPFR_RNDD);
    }
}

void prob_dist_1d_clear(prob_dist_1d_t *dist) {
    for (int i = 0; i < dist->n; i++) {
        mpfr_clear(dist->d[i]);
    }
    free(dist->d);
    dist->min_value = 0;
    dist->max_value = -1;
    dist->n = 0;
}

static __inline__ int prob_dist_1d_get_idx_for_value(prob_dist_1d_t *dist, int value) {
    assert(value <= dist->max_value);
    assert(value >= dist->min_value);

    if (value >= 0) {
        return value;
    }
    else {
        return dist->n + value;
    }
}

void prob_dist_1d_add(prob_dist_1d_t *dist, int value, mpfr_t prob) {
    assert(value <= dist->max_value);
    assert(value >= dist->min_value);
    int idx = prob_dist_1d_get_idx_for_value(dist, value);
    mpfr_add(dist->d[idx], dist->d[idx], prob, MPFR_RNDD);
}

int mod_centered(int x, int m) {
    int a = MOD(x, m);
    if (a < (double) m / 2)
        return a;
    return a - m;
}

int mod_switch(int x, int mod_from, int mod_to) {
    double frac = x * ((double) mod_to / mod_from);
    return MOD((int) round(frac), mod_to);
}

void prob_dist_1d_init_as_compress_decompress_error(prob_dist_1d_t *dist,
                                                    int n_bits_compressed, int kem_q) {
    // prob_dist_1d_init(dist, -MAX_DELTA, MAX_DELTA);
    int max_delta = ceil((double) kem_q / (1 << (n_bits_compressed + 1)));
    prob_dist_1d_init(dist, -max_delta, max_delta);

    mpfr_t q_inverse;
    mpfr_init2(q_inverse, N_PRECISION_BITS);
    mpfr_set_ui(q_inverse, kem_q, MPFR_RNDD);
    mpfr_ui_div(q_inverse, 1, q_inverse, MPFR_RNDD);

    for (int x = 0; x < kem_q; x++) {
        int y = mod_switch(x, kem_q, 1 << n_bits_compressed);
        int z = mod_switch(y, 1 << n_bits_compressed, kem_q);
        int d = mod_centered(z - x, kem_q);

        if (d < -max_delta || d > max_delta) {
            printf("d = %d\n", d);
            fflush(stdout);
        }
        assert(d >= -max_delta);
        assert(d <= max_delta);

        prob_dist_1d_add(dist, d, q_inverse);
    }

    mpfr_clear(q_inverse);
}


void print_dist_1d(FILE *out, prob_dist_1d_t *dist) {
    mpfr_t zero;
    mpfr_init2(zero, N_PRECISION_BITS);
    mpfr_set_d(zero, ZERO_DOUBLE, MPFR_RNDD);

    fprintf(out, "value,probability\n");
    for (int i = dist->min_value; i <= dist->max_value; i++) {
        int idx = prob_dist_1d_get_idx_for_value(dist, i);
        // int cmp_real = mpfr_cmp_abs(dist->d[idx], zero);

        // if (cmp_real <= 0) {
        //     continue;
        // }
        fprintf(out, "%d,", i);
        mpfr_out_str(out, 10, 0, dist->d[idx], ROUND_DOWN);
        fprintf(out, "\n");
    }

    mpfr_clear(zero);
}

int file_exists(const char filename[]) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        return 0;
    }
    fclose(file);
    return 1;
}

void print_dist_1d_to_file(const char filename[], prob_dist_1d_t *dist) {
    assert(!file_exists(filename));

    FILE *f = fopen(filename, "w");
    if (!f) {
        fprintf(stderr, "Could not open output file %s\n", filename);
        exit(1);
    }
    print_dist_1d(f, dist);
    fclose(f);
}

void print_mpc_matrix_to_file(const char filename[], mpc_matrix_t *dist) {
    assert(!file_exists(filename));

    FILE *f = fopen(filename, "w");
    if (!f) {
        fprintf(stderr, "Could not open output file %s\n", filename);
        exit(1);
    }
    mpc_matrix_print_to_file_ptr(f, dist);
    fclose(f);
}

int binomial(int n, int k) {
    if (k > n) return 0;
    if (k == n) return 1;
    if (k == 0) return 1;

    return binomial(n - 1, k - 1) + binomial(n - 1, k);
}

void prob_dist_1d_init_as_centered_binomial(prob_dist_1d_t *dist, int eta) {
    prob_dist_1d_init(dist, -eta, eta);

    mpfr_t tmp;
    mpfr_init2(tmp, N_PRECISION_BITS);
    for (int x = -eta; x <= eta; x++) {
        // prob_t prob_x = (prob_t) binomial(2 * eta, x + eta) / (1 << (2 * eta));
        mpfr_set_ui(tmp, binomial(2 * eta, x + eta), MPFR_RNDD);
        mpfr_div_ui(tmp, tmp, (1 << (2 * eta)), MPFR_RNDD);
        int idx = prob_dist_1d_get_idx_for_value(dist, x);
        mpfr_set(dist->d[idx], tmp, MPFR_RNDD);
    }
    mpfr_clear(tmp);
}

void prob_dist_1d_init_as_sum(prob_dist_1d_t *out_dist, prob_dist_1d_t *dist1, prob_dist_1d_t *dist2) {
    prob_dist_1d_init(out_dist, (dist1->min_value + dist2->min_value), (dist1->max_value + dist2->max_value));

    mpfr_t tmp_prob;
    mpfr_init2(tmp_prob, N_PRECISION_BITS);
    for (int v1 = dist1->min_value; v1 <= dist1->max_value; v1++) {
        int idx1 = prob_dist_1d_get_idx_for_value(dist1, v1);
        for (int v2 = dist2->min_value; v2 <= dist2->max_value; v2++) {
            int idx2 = prob_dist_1d_get_idx_for_value(dist2, v2);

            int sum = v1 + v2;
            mpfr_mul(tmp_prob, dist1->d[idx1], dist2->d[idx2], MPFR_RNDD);
            prob_dist_1d_add(out_dist, sum, tmp_prob);
        }
    }
    mpfr_clear(tmp_prob);
}


int prob_dist_1d_is_symmetric(prob_dist_1d_t *dist) {
    for (int value = dist->min_value; value <= dist->max_value; value++) {
        int idx = prob_dist_1d_get_idx_for_value(dist, value);
        int idx_sym = prob_dist_1d_get_idx_for_value(dist, -value);
        if (mpfr_cmp(dist->d[idx], dist->d[idx_sym]) != 0) {
            return 0;
        }
    }

    return 1;
}

void get_base_2x2_distribution_for_self_convolution(mpc_matrix_t *dist2x2,
                                                    prob_dist_1d_t *coeff_dist_a,
                                                    prob_dist_1d_t *symmetric_coeff_dist_b) {
    assert(prob_dist_1d_is_symmetric(symmetric_coeff_dist_b));

    mpc_matrix_init(dist2x2, N_SUPPORT_DIST_2D, N_SUPPORT_DIST_2D);

    mpfr_t tmp_prob;
    mpfr_init2(tmp_prob, N_PRECISION_BITS);
    for (int a0 = coeff_dist_a->min_value; a0 <= coeff_dist_a->max_value; a0++) {
        int idx_a0 = prob_dist_1d_get_idx_for_value(coeff_dist_a, a0);

        for (int a1 = coeff_dist_a->min_value; a1 <= coeff_dist_a->max_value; a1++) {
            int idx_a1 = prob_dist_1d_get_idx_for_value(coeff_dist_a, a1);

            for (int b0 = symmetric_coeff_dist_b->min_value; b0 <= symmetric_coeff_dist_b->max_value; b0++) {
                int idx_b0 = prob_dist_1d_get_idx_for_value(symmetric_coeff_dist_b, b0);

                for (int b1 = symmetric_coeff_dist_b->min_value; b1 <= symmetric_coeff_dist_b->max_value; b1++) {
                    int idx_b1 = prob_dist_1d_get_idx_for_value(symmetric_coeff_dist_b, b1);

                    int c0 = a0 * b0 + a1 * b1;
                    int c1 = a0 * b1 - a1 * b0;

                    mpfr_mul(tmp_prob, coeff_dist_a->d[idx_a0], coeff_dist_a->d[idx_a1], MPFR_RNDD);
                    mpfr_mul(tmp_prob, tmp_prob, symmetric_coeff_dist_b->d[idx_b0], MPFR_RNDD);
                    mpfr_mul(tmp_prob, tmp_prob, symmetric_coeff_dist_b->d[idx_b1], MPFR_RNDD);

                    mpc_add_fr(dist2x2->m[MOD(c0, N_SUPPORT_DIST_2D)][MOD(c1, N_SUPPORT_DIST_2D)],
                               dist2x2->m[MOD(c0, N_SUPPORT_DIST_2D)][MOD(c1, N_SUPPORT_DIST_2D)],
                               tmp_prob, ROUND_DOWN);
                }
            }
        }
    }
    mpfr_clear(tmp_prob);
}

void self_convolution_in_fft_domain(mpc_matrix_t *dist, unsigned long order) {
    #pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < dist->n_rows; i++) {
        for (int j = 0; j < dist->n_cols; j++) {
            mpc_pow_ui(dist->m[i][j], dist->m[i][j], order, MPFR_RNDU);
        }
    }
}


void init_as_convolution_in_fft_domain(mpc_matrix_t *out, mpc_matrix_t *dist1, mpc_matrix_t *dist2) {
    mpc_matrix_init(out, N_SUPPORT_DIST_2D, N_SUPPORT_DIST_2D);

    #pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < out->n_rows; i++) {
        for (int j = 0; j < out->n_cols; j++) {
            mpc_mul(out->m[i][j], dist1->m[i][j], dist2->m[i][j], MPFR_RNDU);
        }
    }
}

void init_as_copy(mpc_matrix_t *out, mpc_matrix_t *dist) {
    mpc_matrix_init(out, N_SUPPORT_DIST_2D, N_SUPPORT_DIST_2D);

    for (int i = 0; i < out->n_rows; i++) {
        for (int j = 0; j < out->n_cols; j++) {
            mpc_set(out->m[i][j], dist->m[i][j], MPFR_RNDU);
        }
    }
}

void init_as_2d_from_1d(mpc_matrix_t *out, prob_dist_1d_t *dist) {
    mpc_matrix_init(out, N_SUPPORT_DIST_2D, N_SUPPORT_DIST_2D);

    mpfr_t tmp_prob;
    mpfr_init2(tmp_prob, N_PRECISION_BITS);
    for (int a0 = dist->min_value; a0 <= dist->max_value; a0++) {
        int idx_a0 = prob_dist_1d_get_idx_for_value(dist, a0);

        for (int a1 = dist->min_value; a1 <= dist->max_value; a1++) {
            int idx_a1 = prob_dist_1d_get_idx_for_value(dist, a1);

            mpfr_mul(tmp_prob, dist->d[idx_a0], dist->d[idx_a1], MPFR_RNDD);
            mpc_set_fr(out->m[MOD(a0, N_SUPPORT_DIST_2D)][MOD(a1, N_SUPPORT_DIST_2D)], tmp_prob, ROUND_DOWN);
        }
    }
    mpfr_clear(tmp_prob);
}


void print_params(const char filename[], kem_parameters_t *params) {
    assert(!file_exists(filename));

    FILE *file = fopen(filename, "w");
    if (!file) {
        fprintf(stderr, "Could not open %s\n", filename);
        exit(1);
    }

    fprintf(file, "parameter,value\n");
    fprintf(file, "KEM_Q,%d\n", params->KEM_Q);
    fprintf(file, "KEM_LEVEL,%d\n", params->KEM_LEVEL);
    fprintf(file, "KEM_N,%d\n", params->KEM_N);
    fprintf(file, "KEM_K,%d\n", params->KEM_K);
    fprintf(file, "KEM_ETA1,%d\n", params->KEM_ETA1);
    fprintf(file, "KEM_ETA2,%d\n", params->KEM_ETA2);
    fprintf(file, "KEM_N_BLOCKS_FOR_DU0,%d\n", params->KEM_N_BLOCKS_FOR_DU0);
    fprintf(file, "KEM_DU0,%d\n", params->KEM_DU0);
    fprintf(file, "KEM_DU1,%d\n", params->KEM_DU1);
    fprintf(file, "KEM_DV,%d\n", params->KEM_DV);
    fprintf(file, "N_PRECISION_BITS,%d\n", N_PRECISION_BITS);
    fprintf(file, "ZERO_DOUBLE,%.30g\n", ZERO_DOUBLE);

    fclose(file);
}

#if SAVE_PARTIAL_DISTRIBUTION_RESULTS != 1
void DEBUG_print_params(__attribute__((unused)) const char output_dir[],
                        __attribute__((unused)) kem_parameters_t *params,
                        __attribute__((unused)) const char base_name[]) {
    return;
}

void DEBUG_print_dist_1d_to_file(__attribute__((unused)) const char output_dir[],
                                 __attribute__((unused)) kem_parameters_t *params,
                                 __attribute__((unused)) const char base_name[],
                                 __attribute__((unused)) prob_dist_1d_t *dist) {

    return;
}

void DEBUG_print_mpc_matrix_to_file(__attribute__((unused)) const char output_dir[],
                                    __attribute__((unused)) kem_parameters_t *params,
                                    __attribute__((unused)) const char base_name[],
                                    __attribute__((unused)) mpc_matrix_t *dist) {

    return;
}

#else

void DEBUG_print_params(const char output_dir[],
                        kem_parameters_t *params,
                        const char base_name[]) {
    char filename[MAX_STR];
    sprintf(filename,
            "%s/%s_q=%d_l=%d_n=%d_k=%d_e1=%d_e2=%d_du0rep=%d_du0=%d_du1=%d_dv=%d.csv", output_dir, base_name,
            params->KEM_Q, params->KEM_LEVEL, params->KEM_N, params->KEM_K, params->KEM_ETA1, params->KEM_ETA2,
            params->KEM_N_BLOCKS_FOR_DU0, params->KEM_DU0, params->KEM_DU1, params->KEM_DV);
    print_params(filename, params);
}

void DEBUG_print_dist_1d_to_file(const char output_dir[],
                                 kem_parameters_t *params,
                                 const char base_name[],
                                 prob_dist_1d_t *dist) {

    char filename[MAX_STR];
    sprintf(filename,
            "%s/%s_q=%d_l=%d_n=%d_k=%d_e1=%d_e2=%d_du0rep=%d_du0=%d_du1=%d_dv=%d.csv", output_dir, base_name,
            params->KEM_Q, params->KEM_LEVEL, params->KEM_N, params->KEM_K, params->KEM_ETA1, params->KEM_ETA2,
            params->KEM_N_BLOCKS_FOR_DU0, params->KEM_DU0, params->KEM_DU1, params->KEM_DV);
    print_dist_1d_to_file(filename, dist);
}

void DEBUG_print_mpc_matrix_to_file(const char output_dir[],
                                    kem_parameters_t *params,
                                    const char base_name[],
                                    mpc_matrix_t *dist) {
    char filename[MAX_STR];
    sprintf(filename,
            "%s/%s_q=%d_l=%d_n=%d_k=%d_e1=%d_e2=%d_du0rep=%d_du0=%d_du1=%d_dv=%d.csv", output_dir, base_name,
            params->KEM_Q, params->KEM_LEVEL, params->KEM_N, params->KEM_K, params->KEM_ETA1, params->KEM_ETA2,
            params->KEM_N_BLOCKS_FOR_DU0, params->KEM_DU0, params->KEM_DU1, params->KEM_DV);
    print_mpc_matrix_to_file(filename, dist);
}

#endif

int compute_joint_distribution_of_delta_m_and_save_to_file(const char output_dir[], kem_parameters_t *params) {
    init_fft_mpc();

    DEBUG_print_params(output_dir, params, "params.csv");


    prob_dist_1d_t dist_dv;
    prob_dist_1d_init_as_compress_decompress_error(&dist_dv, params->KEM_DV, params->KEM_Q);
    DEBUG_print_dist_1d_to_file(output_dir, params, "dist_dv.csv", &dist_dv);

    prob_dist_1d_t dist_eta1;
    prob_dist_1d_init_as_centered_binomial(&dist_eta1, params->KEM_ETA1);
    DEBUG_print_dist_1d_to_file(output_dir, params, "dist_eta1.csv", &dist_eta1);

    prob_dist_1d_t dist_eta2;
    prob_dist_1d_init_as_centered_binomial(&dist_eta2, params->KEM_ETA2);
    DEBUG_print_dist_1d_to_file(output_dir, params, "dist_eta2.csv", &dist_eta2);

    prob_dist_1d_t dist_du0;
    prob_dist_1d_init_as_compress_decompress_error(&dist_du0, params->KEM_DU0, params->KEM_Q);
    DEBUG_print_dist_1d_to_file(output_dir, params, "dist_du0.csv", &dist_du0);

    prob_dist_1d_t dist_du0_sum_eta2;
    prob_dist_1d_init_as_sum(&dist_du0_sum_eta2, &dist_du0, &dist_eta2);
    DEBUG_print_dist_1d_to_file(output_dir, params, "dist_du0_sum_eta2.csv", &dist_du0_sum_eta2);

    prob_dist_1d_t dist_dv_sum_eta2;
    prob_dist_1d_init_as_sum(&dist_dv_sum_eta2, &dist_dv, &dist_eta2);
    DEBUG_print_dist_1d_to_file(output_dir, params, "dist_dv_sum_eta2.csv", &dist_dv_sum_eta2);

    mpc_matrix_t dist_s_du0_sum_eta2_product;
    get_base_2x2_distribution_for_self_convolution(&dist_s_du0_sum_eta2_product, &dist_du0_sum_eta2, &dist_eta1);
    DEBUG_print_mpc_matrix_to_file(output_dir, params, "dist_s_du0_sum_eta2_product_level1.csv", &dist_s_du0_sum_eta2_product);
    fft2(&dist_s_du0_sum_eta2_product);
    // We divide `params->KEM_N` by 2 because `dist_s_du0_sum_eta2_product` starts with a 2D distribution
    self_convolution_in_fft_domain(&dist_s_du0_sum_eta2_product, (params->KEM_N / 2) * params->KEM_N_BLOCKS_FOR_DU0);

    mpc_matrix_t dist_s_du_sum_eta2_product;
    // Only computes the du1 part if it is really needed.
    if (params->KEM_N_BLOCKS_FOR_DU0 < params->KEM_K) {
        prob_dist_1d_t dist_du1 = {0};
        prob_dist_1d_init_as_compress_decompress_error(&dist_du1, params->KEM_DU1, params->KEM_Q);
        DEBUG_print_dist_1d_to_file(output_dir, params, "dist_du1.csv", &dist_du1);

        prob_dist_1d_t dist_du1_sum_eta2 = {0};
        prob_dist_1d_init_as_sum(&dist_du1_sum_eta2, &dist_du1, &dist_eta2);
        DEBUG_print_dist_1d_to_file(output_dir, params, "dist_du1_sum_eta2.csv", &dist_du1_sum_eta2);

        mpc_matrix_t dist_s_du1_sum_eta2_product;
        get_base_2x2_distribution_for_self_convolution(&dist_s_du1_sum_eta2_product, &dist_du1_sum_eta2, &dist_eta1);
        DEBUG_print_mpc_matrix_to_file(output_dir, params, "dist_s_du1_sum_eta2_product_level1.csv", &dist_s_du1_sum_eta2_product);
        fft2(&dist_s_du1_sum_eta2_product);
        // We divide `params->KEM_N` by 2 because `dist_s_du1_sum_eta2_product` starts with a 2D distribution
        self_convolution_in_fft_domain(&dist_s_du1_sum_eta2_product, (params->KEM_N / 2) * (params->KEM_K - params->KEM_N_BLOCKS_FOR_DU0));

        init_as_convolution_in_fft_domain(&dist_s_du_sum_eta2_product, &dist_s_du0_sum_eta2_product, &dist_s_du1_sum_eta2_product);
        mpc_matrix_free(&dist_s_du0_sum_eta2_product);
        mpc_matrix_free(&dist_s_du1_sum_eta2_product);

        prob_dist_1d_clear(&dist_du1);
        prob_dist_1d_clear(&dist_du1_sum_eta2);
    }
    else {
        init_as_copy(&dist_s_du_sum_eta2_product, &dist_s_du0_sum_eta2_product);
        mpc_matrix_free(&dist_s_du0_sum_eta2_product);
    }

    mpc_matrix_t dist_e_r_product;
    get_base_2x2_distribution_for_self_convolution(&dist_e_r_product, &dist_eta1, &dist_eta1);
    DEBUG_print_mpc_matrix_to_file(output_dir, params, "dist_e_r_product_level1.csv", &dist_e_r_product);
    fft2(&dist_e_r_product);
    // We divide `params->KEM_N` by 2 because `dist_e_r_product` starts with a 2D distribution
    self_convolution_in_fft_domain(&dist_e_r_product, (params->KEM_N / 2) * params->KEM_K);

    mpc_matrix_t dist_sum_dot_products;
    init_as_convolution_in_fft_domain(&dist_sum_dot_products, &dist_e_r_product, &dist_s_du_sum_eta2_product);

    mpc_matrix_free(&dist_e_r_product);
    mpc_matrix_free(&dist_s_du_sum_eta2_product);

    mpc_matrix_t dist_dv_sum_eta2_as_2d;
    init_as_2d_from_1d(&dist_dv_sum_eta2_as_2d, &dist_dv_sum_eta2);
    fft2(&dist_dv_sum_eta2_as_2d);

    mpc_matrix_t dist_delta_m;
    init_as_convolution_in_fft_domain(&dist_delta_m, &dist_sum_dot_products, &dist_dv_sum_eta2_as_2d);
    ifft2(&dist_delta_m);

    char delta_m_output_path[MAX_STR] = {0};
    sprintf(delta_m_output_path,
            "%s/delta_m_pair_q=%d_l=%d_k=%d_e1=%d_e2=%d_du0rep=%d_du0=%d_du1=%d_dv=%d.csv", output_dir,
            params->KEM_Q, params->KEM_LEVEL, params->KEM_K, params->KEM_ETA1, params->KEM_ETA2,
            params->KEM_N_BLOCKS_FOR_DU0, params->KEM_DU0, params->KEM_DU1, params->KEM_DV);

    print_mpc_matrix_to_file(delta_m_output_path, &dist_delta_m);

    prob_dist_1d_clear(&dist_du0);
    prob_dist_1d_clear(&dist_dv);
    prob_dist_1d_clear(&dist_eta1);
    prob_dist_1d_clear(&dist_eta2);
    prob_dist_1d_clear(&dist_du0_sum_eta2);
    prob_dist_1d_clear(&dist_dv_sum_eta2);

    mpc_matrix_free(&dist_sum_dot_products);
    mpc_matrix_free(&dist_dv_sum_eta2_as_2d);
    mpc_matrix_free(&dist_delta_m);

    clear_fft_mpc();
    return 0;
}

int set_kem_parameters(kem_parameters_t *params, FILE *experiment_setup) {
    char csv_line[MAX_STR] = {0};
    fgets(csv_line, MAX_STR, experiment_setup);

    if (strlen(csv_line) == 0) {
        // Empty line
        return 0;
    }
    sscanf(csv_line,
           "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
           &params->KEM_Q, &params->KEM_LEVEL, &params->KEM_MESSAGE_BIT_LENGTH, &params->KEM_N,
           &params->KEM_K, &params->KEM_ETA1, &params->KEM_ETA2,
           &params->KEM_N_BLOCKS_FOR_DU0, &params->KEM_DU0, &params->KEM_DU1, &params->KEM_DV);

    params->KEM_B_ENCODING = params->KEM_MESSAGE_BIT_LENGTH / params->KEM_N;
    // KEM_Q,KEM_LEVEL,KEM_MESSAGE_BIT_LENGTH,KEM_N,KEM_K,KEM_ETA1,KEM_ETA2,KEM_N_BLOCKS_FOR_DU0,KEM_DU0,KEM_DU1,KEM_DV
    fprintf(stderr, "    KEM_Q = %d\n", params->KEM_Q);
    fprintf(stderr, "    KEM_LEVEL = %d\n", params->KEM_LEVEL);
    fprintf(stderr, "    KEM_MESSAGE_BIT_LENGTH = %d\n", params->KEM_MESSAGE_BIT_LENGTH);
    fprintf(stderr, "    KEM_B_ENCODING = %d\n", params->KEM_B_ENCODING);
    fprintf(stderr, "    KEM_N = %d\n", params->KEM_N);
    fprintf(stderr, "    KEM_K = %d\n", params->KEM_K);
    fprintf(stderr, "    KEM_ETA1 = %d\n", params->KEM_ETA1);
    fprintf(stderr, "    KEM_ETA2 = %d\n", params->KEM_ETA2);
    fprintf(stderr, "    KEM_N_BLOCKS_FOR_DU0 = %d\n", params->KEM_N_BLOCKS_FOR_DU0);
    fprintf(stderr, "    KEM_DU0 = %d\n", params->KEM_DU0);
    fprintf(stderr, "    KEM_DU1 = %d\n", params->KEM_DU1);
    fprintf(stderr, "    KEM_DV = %d\n", params->KEM_DV);

    int ciphertext_size = params->KEM_N * params->KEM_N_BLOCKS_FOR_DU0 * params->KEM_DU0 ; // u0 part
    ciphertext_size    += params->KEM_N * (params->KEM_K - params->KEM_N_BLOCKS_FOR_DU0) * params->KEM_DU1 ; // u1 part
    ciphertext_size    += params->KEM_N * params->KEM_DV; // v part
    ciphertext_size    /= 8; // To have it in bytes
    fprintf(stderr, "    CIPHERTEXT_SIZE_BYTES = %d\n", ciphertext_size);

    assert(params->KEM_N_BLOCKS_FOR_DU0 <= params->KEM_K);
    assert(params->KEM_N_BLOCKS_FOR_DU0 >= 0);

    return 1;
}

void run_experiment_compute_joint_distributions_and_save(const char output_dir[], const char experiment_setup_filename[]) {
    FILE *experiment_setup = fopen(experiment_setup_filename, "r");
    if (!experiment_setup) {
        fprintf(stderr, "Could not open experiment_setup file %s\n", experiment_setup_filename);
        exit(1);
    }

    char first_line[MAX_STR];
    fgets(first_line, MAX_STR, experiment_setup);
    assert(strncmp(first_line, EXPECTED_SETUP_CSV_HEADER, MAX_STR) == 0);

    while (!feof(experiment_setup)) {
        printf("==========================================================\n");
        printf("Computing Pr(delta m[i, i + n/2]) for parameters:\n");
        kem_parameters_t params = {0};
        int ret = set_kem_parameters(&params, experiment_setup);
        if (ret == 0) {
            printf("Found empty line in setup file, skipping it...\n");
        }
        else {
            compute_joint_distribution_of_delta_m_and_save_to_file(output_dir, &params);
        }
    }
    fclose(experiment_setup);
}


int init_delta_m_as_joint_distribution(mpc_matrix_t *dist_delta_m, kem_parameters_t *params) {
    init_fft_mpc();

    prob_dist_1d_t dist_dv;
    prob_dist_1d_init_as_compress_decompress_error(&dist_dv, params->KEM_DV, params->KEM_Q);

    prob_dist_1d_t dist_eta1;
    prob_dist_1d_init_as_centered_binomial(&dist_eta1, params->KEM_ETA1);

    prob_dist_1d_t dist_eta2;
    prob_dist_1d_init_as_centered_binomial(&dist_eta2, params->KEM_ETA2);

    prob_dist_1d_t dist_du0;
    prob_dist_1d_init_as_compress_decompress_error(&dist_du0, params->KEM_DU0, params->KEM_Q);

    prob_dist_1d_t dist_du0_sum_eta2;
    prob_dist_1d_init_as_sum(&dist_du0_sum_eta2, &dist_du0, &dist_eta2);

    prob_dist_1d_t dist_dv_sum_eta2;
    prob_dist_1d_init_as_sum(&dist_dv_sum_eta2, &dist_dv, &dist_eta2);

    mpc_matrix_t dist_s_du0_sum_eta2_product;
    get_base_2x2_distribution_for_self_convolution(&dist_s_du0_sum_eta2_product, &dist_du0_sum_eta2, &dist_eta1);
    fft2(&dist_s_du0_sum_eta2_product);
    // We divide `params->KEM_N` by 2 because `dist_s_du0_sum_eta2_product` starts with a 2D distribution
    self_convolution_in_fft_domain(&dist_s_du0_sum_eta2_product, (params->KEM_N / 2) * params->KEM_N_BLOCKS_FOR_DU0);

    mpc_matrix_t dist_s_du_sum_eta2_product;
    // Only computes the du1 part if it is really needed.
    if (params->KEM_N_BLOCKS_FOR_DU0 < params->KEM_K) {
        prob_dist_1d_t dist_du1 = {0};
        prob_dist_1d_init_as_compress_decompress_error(&dist_du1, params->KEM_DU1, params->KEM_Q);

        prob_dist_1d_t dist_du1_sum_eta2 = {0};
        prob_dist_1d_init_as_sum(&dist_du1_sum_eta2, &dist_du1, &dist_eta2);

        mpc_matrix_t dist_s_du1_sum_eta2_product;
        get_base_2x2_distribution_for_self_convolution(&dist_s_du1_sum_eta2_product, &dist_du1_sum_eta2, &dist_eta1);
        fft2(&dist_s_du1_sum_eta2_product);
        // We divide `params->KEM_N` by 2 because `dist_s_du1_sum_eta2_product` starts with a 2D distribution
        self_convolution_in_fft_domain(&dist_s_du1_sum_eta2_product, (params->KEM_N / 2) * (params->KEM_K - params->KEM_N_BLOCKS_FOR_DU0));

        init_as_convolution_in_fft_domain(&dist_s_du_sum_eta2_product, &dist_s_du0_sum_eta2_product, &dist_s_du1_sum_eta2_product);
        mpc_matrix_free(&dist_s_du0_sum_eta2_product);
        mpc_matrix_free(&dist_s_du1_sum_eta2_product);

        prob_dist_1d_clear(&dist_du1);
        prob_dist_1d_clear(&dist_du1_sum_eta2);
    }
    else {
        init_as_copy(&dist_s_du_sum_eta2_product, &dist_s_du0_sum_eta2_product);
        mpc_matrix_free(&dist_s_du0_sum_eta2_product);
    }

// Compile with -DSIMULATE_FOR_PK_COMPRESSION_11BITS to compute the DFR when 11 bits are used for the
// public key (instead of the regular 12 bits)
#ifdef SIMULATE_FOR_PK_COMPRESSION_11BITS
    prob_dist_1d_t dist_dt;
    prob_dist_1d_init_as_compress_decompress_error(&dist_dt, 11, params->KEM_Q);
    prob_dist_1d_t dist_dt_sum_eta1;
    prob_dist_1d_init_as_sum(&dist_dt_sum_eta1, &dist_dt, &dist_eta1);

    mpc_matrix_t dist_e_r_product;
    get_base_2x2_distribution_for_self_convolution(&dist_e_r_product, &dist_dt_sum_eta1, &dist_eta1);

    prob_dist_1d_clear(&dist_dt);
    prob_dist_1d_clear(&dist_dt_sum_eta1);
#else
    mpc_matrix_t dist_e_r_product;
# ifndef MINAL_ANALYSIS_FOR_LWR
    get_base_2x2_distribution_for_self_convolution(&dist_e_r_product, &dist_eta1, &dist_eta1);
# else
    // FOR LWR Scabbard:
    get_base_2x2_distribution_for_self_convolution(&dist_e_r_product, &dist_du0, &dist_eta1);
    // DEBUG_print_mpc_matrix_to_file("tmpdist", params, "dist_e_r_product.csv", &dist_e_r_product);
# endif

#endif
    fft2(&dist_e_r_product);
    // We divide `params->KEM_N` by 2 because `dist_e_r_product` starts with a 2D distribution
    self_convolution_in_fft_domain(&dist_e_r_product, (params->KEM_N / 2) * params->KEM_K);

    mpc_matrix_t dist_sum_dot_products;
    init_as_convolution_in_fft_domain(&dist_sum_dot_products, &dist_e_r_product, &dist_s_du_sum_eta2_product);

    mpc_matrix_free(&dist_e_r_product);
    mpc_matrix_free(&dist_s_du_sum_eta2_product);

    mpc_matrix_t dist_dv_sum_eta2_as_2d;
    init_as_2d_from_1d(&dist_dv_sum_eta2_as_2d, &dist_dv_sum_eta2);
    fft2(&dist_dv_sum_eta2_as_2d);

    init_as_convolution_in_fft_domain(dist_delta_m, &dist_sum_dot_products, &dist_dv_sum_eta2_as_2d);
    ifft2(dist_delta_m);

    prob_dist_1d_clear(&dist_du0);
    prob_dist_1d_clear(&dist_dv);
    prob_dist_1d_clear(&dist_eta1);
    prob_dist_1d_clear(&dist_eta2);
    prob_dist_1d_clear(&dist_du0_sum_eta2);
    prob_dist_1d_clear(&dist_dv_sum_eta2);

    mpc_matrix_free(&dist_sum_dot_products);
    mpc_matrix_free(&dist_dv_sum_eta2_as_2d);

    clear_fft_mpc();
    return 0;
}

void print_dfr_csv_for_2d_codes_header() {
    printf("KEM_Q,");
    printf("KEM_LEVEL,");
    printf("KEM_MESSAGE_BIT_LENGTH,");
    printf("KEM_N,");
    printf("KEM_K,");
    printf("KEM_ETA1,");
    printf("KEM_ETA2,");
    printf("KEM_N_BLOCKS_FOR_DU0,");
    printf("KEM_DU0,");
    printf("KEM_DU1,");
    printf("KEM_DV,");
    printf("CODE_ALPHA,");
    printf("CODE_BETA,");
    printf("DFR");
    printf("\n");
}

void print_dfr_csv_for_2d_codes(mpc_matrix_t *mpc_dist_delta_m, kem_parameters_t *params) {
    mpfr_matrix_t real_dist_delta_m;
    mpfr_matrix_init(&real_dist_delta_m, mpc_dist_delta_m->n_rows, mpc_dist_delta_m->n_cols);

    mpfr_t prev_error_prob;
    mpfr_init2(prev_error_prob, N_PRECISION_BITS);

    mpc_matrix_to_mpfr_matrix(&real_dist_delta_m, mpc_dist_delta_m);
    for (int code_beta = 0; code_beta <= MAX_CODE_BETA; code_beta += 10) {

        mpfr_t error_prob;
        mpfr_init2(error_prob, N_PRECISION_BITS);
        mpfr_set_d(error_prob, 0, MPFR_RNDD);

        int code_alpha = 0;
        if (params->KEM_B_ENCODING == 1) {
            code_alpha = params->KEM_Q/2;
            // get_dfr_for_2d_code_with_real_decoder(error_prob, &real_dist_delta_m, code_alpha, code_beta, params->KEM_Q);
            get_dfr_for_2d_code_with_minimum_distance_decoding(error_prob, &real_dist_delta_m, code_alpha, code_beta, params->KEM_Q,
                                                               params->KEM_N, params->KEM_MESSAGE_BIT_LENGTH);
        }
        else if (params->KEM_B_ENCODING == 2) {
            code_alpha = params->KEM_Q/4;
            // get_dfr_for_2d_code_with_minimum_distance_decoding_for_b2_using_generic_decoder(error_prob, &real_dist_delta_m, code_alpha, code_beta, params->KEM_Q,
            //                                                           params->KEM_N, params->KEM_MESSAGE_BIT_LENGTH);

            get_dfr_for_2d_code_with_minimum_distance_decoding_for_b2(error_prob, &real_dist_delta_m, code_alpha, code_beta, params->KEM_Q,
                                                                      params->KEM_N, params->KEM_MESSAGE_BIT_LENGTH);

        }
        else {
            fprintf(stderr, "Fatal: KEM_B_ENCODING should be either 1 or 2\n");
            exit(1);
        }


        printf("%d,", params->KEM_Q);
        printf("%d,", params->KEM_LEVEL);
        printf("%d,", params->KEM_MESSAGE_BIT_LENGTH);
        printf("%d,", params->KEM_N);
        printf("%d,", params->KEM_K);
        printf("%d,", params->KEM_ETA1);
        printf("%d,", params->KEM_ETA2);
        printf("%d,", params->KEM_N_BLOCKS_FOR_DU0);
        printf("%d,", params->KEM_DU0);
        printf("%d,", params->KEM_DU1);
        printf("%d,", params->KEM_DV);
        printf("%d,", code_alpha);
        printf("%d,", code_beta);
        mpfr_out_str(stdout, 10, 0, error_prob, MPFR_RNDD);
        printf("\n");
        fflush(stdout);


        // Break if the current error probability is greater than
        // the previous iteration's error probability.
        if (code_beta > 50 && mpfr_cmp(error_prob, prev_error_prob) > 0) {
            mpfr_clear(error_prob);
            break;
        }

        mpfr_set(prev_error_prob, error_prob, MPFR_RNDD);

        mpfr_clear(error_prob);

    }
    mpfr_clear(prev_error_prob);

    mpfr_matrix_free(&real_dist_delta_m);
}


void run_experiment_compute_joint_distribution_and_dfr(const char experiment_setup_filename[]) {
    FILE *experiment_setup = fopen(experiment_setup_filename, "r");
    if (!experiment_setup) {
        fprintf(stderr, "Could not open experiment_setup file %s\n", experiment_setup_filename);
        exit(1);
    }
    char first_line[MAX_STR];
    fgets(first_line, MAX_STR, experiment_setup);
    assert(strncmp(first_line, EXPECTED_SETUP_CSV_HEADER, MAX_STR) == 0);

    print_dfr_csv_for_2d_codes_header();
    while (!feof(experiment_setup)) {
        fprintf(stderr, "==========================================================\n");
        fprintf(stderr, "Computing Pr(delta m[i, i + n/2]) for parameters:\n");
        kem_parameters_t params = {0};
        int ret = set_kem_parameters(&params, experiment_setup);
        if (ret == 0) {
            fprintf(stderr, "Found empty line in setup file, skipping it...\n");
            continue;
        }
        mpc_matrix_t mpc_dist_delta_m;
        init_delta_m_as_joint_distribution(&mpc_dist_delta_m, &params);
        print_dfr_csv_for_2d_codes(&mpc_dist_delta_m, &params);
        mpc_matrix_free(&mpc_dist_delta_m);
    }
    fclose(experiment_setup);
}


#if COMPUTE_JOINT_DISTRIBUTIONS_AND_SAVE

int main(int argc, char const *argv[]) {

    if (argc != 3) {
        fprintf(stderr, "Usage: %s output_dir experiment_setup.csv\n", argv[0]);
        exit(1);
    }
    if (SAVE_PARTIAL_DISTRIBUTION_RESULTS == 1)
        fprintf(stderr, "[*] Printing partial distributions to files.\n");


    run_experiment_compute_joint_distributions_and_save(argv[1], argv[2]);
}

#elif COMPUTE_JOINT_DISTRIBUTIONS_AND_DFR

int main(int argc, char const *argv[]) {

    if (argc != 2) {
        fprintf(stderr, "Usage: %s experiment_setup.csv\n", argv[0]);
        exit(1);
    }

    run_experiment_compute_joint_distribution_and_dfr(argv[1]);
}

#else
#error "Either COMPUTE_JOINT_DISTRIBUTIONS_AND_SAVE or COMPUTE_JOINT_DISTRIBUTIONS_AND_DFR must be set."
#endif
