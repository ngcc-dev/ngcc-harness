// Copyright 2025 LG Electronics, Inc. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <mpfr.h>
#include <omp.h>

#include "common.h"
#include "prob_dist.h"

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

__inline__ int prob_dist_1d_get_idx_for_value(prob_dist_1d_t *dist, int value) {
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

        assert(d >= -max_delta);
        assert(d <= max_delta);

        prob_dist_1d_add(dist, d, q_inverse);
    }

    mpfr_clear(q_inverse);
}

void prob_dist_1d_init_as_compress_decompress_error_plus(prob_dist_1d_t *dist,
                                                    int n_bits_compressed, int kem_q, double prob_plus) {
    int max_delta = ceil((double) kem_q / (1 << (n_bits_compressed + 1)));
    prob_dist_1d_init(dist, -max_delta, max_delta);

    mpfr_t q_inverse;
    mpfr_init2(q_inverse, N_PRECISION_BITS);
    mpfr_set_ui(q_inverse, kem_q, MPFR_RNDD);
    mpfr_ui_div(q_inverse, 1, q_inverse, MPFR_RNDD);
    mpfr_t tmp;
    mpfr_init2(tmp, N_PRECISION_BITS);

    for (int x = 0; x < kem_q; x++) {
        int y = mod_switch(x, kem_q, 1 << n_bits_compressed);
        int z = mod_switch(y, 1 << n_bits_compressed, kem_q);
        int d = mod_centered(z - x, kem_q);

        assert(d >= -max_delta);
        assert(d <= max_delta);

        mpfr_mul_d(tmp, q_inverse, 1 - prob_plus, N_PRECISION_BITS);

        // prob_dist_1d_add(dist, d, q_inverse);
        prob_dist_1d_add(dist, d, tmp);
    }
    for (int x = 0; x < kem_q; x++) {
        int y = mod_switch(x, kem_q, 1 << (n_bits_compressed + 1));
        int z = mod_switch(y, 1 << (n_bits_compressed + 1), kem_q);
        int d = mod_centered(z - x, kem_q);

        assert(d >= -max_delta);
        assert(d <= max_delta);

        mpfr_mul_d(tmp, q_inverse, prob_plus, N_PRECISION_BITS);

        // prob_dist_1d_add(dist, d, q_inverse);
        prob_dist_1d_add(dist, d, tmp);
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
        int cmp_real = mpfr_cmp_abs(dist->d[idx], zero);

        if (cmp_real <= 0) {
            continue;
        }
        fprintf(out, "%d,", i);
        mpfr_out_str(out, 10, 0, dist->d[idx], ROUND_DOWN);
        fprintf(out, "\n");
    }

    mpfr_clear(zero);
}

static int file_exists(const char filename[]) {
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


int prob_dist_1d_update_value_to_next_one(prob_dist_1d_t *dist, int *value) {
    // assert(*value <= dist->max_value);
    // assert(*value >= dist->min_value);

    mpfr_t zero;
    mpfr_init2(zero, N_PRECISION_BITS);
    mpfr_set_d(zero, ZERO_DOUBLE, MPFR_RNDD);

    for (int v = *value + 1; v <= dist->max_value; v++) {
        int cmp_real = mpfr_cmp_abs(dist->d[prob_dist_1d_get_idx_for_value(dist, v)], zero);
        if (cmp_real > 0) {
            *value = v;
            mpfr_clear(zero);
            return 1;
        }
    }
    mpfr_clear(zero);
    *value = dist->min_value - 1;
    return 0;

}


// Computes the expected value of e^{tX}
void prob_dist_1d_get_expectation_of_exp_tX(mpfr_t out, prob_dist_1d_t *dist, double t) {
    mpfr_set_d(out, 0, MPFR_RNDD);

    int n_threads = omp_get_max_threads();

    mpfr_t tmps[n_threads];
    mpfr_t outs[n_threads];
    for (int i = 0; i < n_threads; i++) {
        mpfr_init2(tmps[i], N_PRECISION_BITS);
        mpfr_init2(outs[i], N_PRECISION_BITS);
        mpfr_set_d(outs[i], 0, MPFR_RNDD);
    }

    #pragma omp parallel for
    for (int v = dist->min_value; v <= dist->max_value; v++) {
        int idx = prob_dist_1d_get_idx_for_value(dist, v);
        if (mpfr_cmp_d(dist->d[idx], ZERO_DOUBLE) <= 0) {
            continue;
        }
        int thread_id = omp_get_thread_num();

        mpfr_set_d(tmps[thread_id], t * v, MPFR_RNDD);                          // tmp = t * v
        mpfr_exp(tmps[thread_id], tmps[thread_id], MPFR_RNDD);                  // tmp = exp(tmp)
        mpfr_mul(tmps[thread_id], tmps[thread_id], dist->d[idx], MPFR_RNDD);    // tmp = tmp * Pr(v)
        mpfr_add(outs[thread_id], outs[thread_id], tmps[thread_id], MPFR_RNDD);
    }
    for (int i = 0; i < n_threads; i++) {
        mpfr_clear(tmps[i]);
        mpfr_add(out, out, outs[i], MPFR_RNDD);
        mpfr_clear(outs[i]);
    }
}


// Returns the upper bound on P(X ≥ a) given by Chernoff as:
//      P(X ≥ a) ≤ exp(−ta + n_variables ln(E[e^{t Xi}]))
void prob_dist_1d_chernoff(mpfr_t out, double value, int n_summed_random_variables, prob_dist_1d_t *dist, double t) {
    mpfr_set_d(out, 0, MPFR_RNDD);

    mpfr_t aux;
    mpfr_init2(aux, N_PRECISION_BITS);

    prob_dist_1d_get_expectation_of_exp_tX(aux, dist, t);


    mpfr_log(aux, aux, MPFR_RNDD);
    mpfr_mul_si(aux, aux, n_summed_random_variables, MPFR_RNDD);


    mpfr_t aux2;
    mpfr_init2(aux2, N_PRECISION_BITS);
    mpfr_set_d(aux2, t * value, MPFR_RNDD);

    mpfr_sub(aux, aux, aux2, MPFR_RNDD);

    mpfr_exp(out, aux, MPFR_RNDD);


    mpfr_clear(aux);
    mpfr_clear(aux2);
}


// Returns the upper bound on P(X ≥ a) given by Chernoff as:
//      P(X ≥ a) ≤ exp(−ta + n_variables ln(E[e^{t Xi}]))


void prob_dist_1d_chernoff_with_precomputed_expectation_of_exp_tX(mpfr_t out, double value,
                                                                  int n_summed_random_variables,
                                                                  double t,
                                                                  mpfr_t expectation_of_exp_tX) {
    mpfr_set_d(out, 0, MPFR_RNDD);

    mpfr_t aux;
    mpfr_init2(aux, N_PRECISION_BITS);

    mpfr_log(aux, expectation_of_exp_tX, MPFR_RNDD);
    mpfr_mul_si(aux, aux, n_summed_random_variables, MPFR_RNDD);

    mpfr_t aux2;
    mpfr_init2(aux2, N_PRECISION_BITS);
    mpfr_set_d(aux2, t * value, MPFR_RNDD);

    mpfr_sub(aux, aux, aux2, MPFR_RNDD);

    mpfr_exp(out, aux, MPFR_RNDD);

    mpfr_clear(aux);
    mpfr_clear(aux2);
}

// Uses a ternary search to find the `t` value that minimizes the Chernoff bound
void prob_dist_1d_minimize_chernoff_bound(mpfr_t out,
                                          double value,
                                          int n_summed_random_variables,
                                          prob_dist_1d_t *dist) {

    double best_t = prob_dist_1d_minimize_t_for_chernoff(value, n_summed_random_variables, dist);
    prob_dist_1d_chernoff(out, value, n_summed_random_variables, dist, best_t);
}

// Uses a ternary search to find the `t` value that minimizes the Chernoff bound
double prob_dist_1d_minimize_t_for_chernoff(double value, int n_summed_random_variables, prob_dist_1d_t *dist) {

    double left = 1e-6;
    double right = 1;
    double left_third, right_third;

    mpfr_t chernoff_left_third;
    mpfr_t chernoff_right_third;

    mpfr_init2(chernoff_left_third, N_PRECISION_BITS);
    mpfr_init2(chernoff_right_third, N_PRECISION_BITS);

    while (fabs(right - left) >= 1e-10) {
        left_third = left + (right - left) / 3;
        right_third = right - (right - left) / 3;

        prob_dist_1d_chernoff(chernoff_left_third, value, n_summed_random_variables, dist, left_third);
        prob_dist_1d_chernoff(chernoff_right_third, value, n_summed_random_variables, dist, right_third);

        if (mpfr_cmp(chernoff_left_third, chernoff_right_third) > 0) {
            left = left_third;
        }
        else {
            right = right_third;
        }

        // printf("t: %lf\n", (left_third + right_third) / 2);
    }


    mpfr_clear(chernoff_left_third);
    mpfr_clear(chernoff_right_third);

    return (left + right) / 2;
}

// Functions for testing ==========================================================================

#ifdef TEST_PROB_DIST_C

int main(int argc, char const *argv[]) {
    prob_dist_1d_t dist;
    prob_dist_1d_init_as_centered_binomial(&dist, 3);
    print_dist_1d(stdout, &dist);

    // int value = atoi(argv[1]);
    // int n_variables = atoi(argv[2]);
    // double t = atof(argv[3]);

    int value = 60;
    int n_variables = 100;
    double t = 0.1;

    mpfr_t chernoff_bound;
    mpfr_init2(chernoff_bound, N_PRECISION_BITS);
    prob_dist_1d_chernoff(chernoff_bound, value, n_variables, &dist, t);

    printf("Pr(sum_{i=1}^{%d} X_i > %d) = ", n_variables, value);
    mpfr_out_str(stdout, 10, 0, chernoff_bound, ROUND_DOWN);
    printf("\n");

    prob_dist_1d_minimize_chernoff_bound(chernoff_bound, value, n_variables, &dist);

    printf("Best: Pr(sum_{i=1}^{%d} X_i > %d) = ", n_variables, value);
    mpfr_out_str(stdout, 10, 0, chernoff_bound, ROUND_DOWN);
    printf("\n");

    mpfr_clear(chernoff_bound);
    prob_dist_1d_clear(&dist);

    return 0;
}

#endif  // ifdef TEST_PROB_DIST_C
