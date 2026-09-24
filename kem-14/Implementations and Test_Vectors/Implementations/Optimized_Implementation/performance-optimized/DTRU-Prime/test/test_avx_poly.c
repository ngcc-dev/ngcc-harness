#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../poly.h"
#include "../avx2_poly.h"

static void print_failure(const char *func_name, int iter, int idx, int16_t expected, int16_t got) {
    printf("\n[FAILED] %s at iteration %d\n", func_name, iter);
    printf("  Index: %d\n", idx);
    printf("  Expected: %d\n", expected);
    printf("  Got:      %d\n", got);
    exit(1);
}

static void random_poly(poly *a) {
    for (int i = 0; i < DTRU_N; i++) {
        int v = rand() % (2 * DTRU_Q);
        a->coeffs[i] = (int16_t)(v - DTRU_Q);
    }
}

static void random_poly_nonnegative(poly *a) {
    for (int i = 0; i < DTRU_N; i++) {
        int v = rand() % DTRU_Q;
        a->coeffs[i] = (int16_t)v;
    }
}

static void test_poly_add() {
    printf("Testing poly_add vs poly_add_avx2... ");
    poly a, b, c_ref, c_avx;
    int iterations = 10000;

    for (int iter = 0; iter < iterations; iter++) {
        random_poly(&a);
        random_poly(&b);

        poly_add(&c_ref, &a, &b);
        poly_add_avx2(&c_avx, &a, &b);

        for (int i = 0; i < DTRU_N; i++) {
            if (c_ref.coeffs[i] != c_avx.coeffs[i]) {
                print_failure("poly_add", iter, i, c_ref.coeffs[i], c_avx.coeffs[i]);
            }
        }
    }
    printf("PASSED (%d iterations)\n", iterations);
}

static void test_poly_multi_p() {
    printf("Testing poly_multi_p vs poly_multi_p_avx2... ");
    poly a, b_ref, b_avx;
    int iterations = 10000;

    for (int iter = 0; iter < iterations; iter++) {
        random_poly(&a);

        poly_multi_p(&b_ref, &a);
        poly_multi_p_avx2(&b_avx, &a);

        for (int i = 0; i < DTRU_N; i++) {
            if (b_ref.coeffs[i] != b_avx.coeffs[i]) {
                print_failure("poly_multi_p", iter, i, b_ref.coeffs[i], b_avx.coeffs[i]);
            }
        }
    }
    printf("PASSED (%d iterations)\n", iterations);
}

static void test_poly_fqcsubq() {
    printf("Testing poly_fqcsubq vs poly_fqcsubq_avx2... ");
    poly a_ref, a_avx;
    int iterations = 10000;

    for (int iter = 0; iter < iterations; iter++) {
        random_poly_nonnegative(&a_ref);
        memcpy(&a_avx, &a_ref, sizeof(poly));

        poly_fqcsubq(&a_ref);
        poly_fqcsubq_avx2(&a_avx);

        for (int i = 0; i < DTRU_N; i++) {
            if (a_ref.coeffs[i] != a_avx.coeffs[i]) {
                print_failure("poly_fqcsubq", iter, i, a_ref.coeffs[i], a_avx.coeffs[i]);
            }
        }
    }
    printf("PASSED (%d iterations)\n", iterations);
}

int main() {
    srand((unsigned)time(NULL));
    test_poly_add();
    test_poly_multi_p();
    test_poly_fqcsubq();
    return 0;
}

