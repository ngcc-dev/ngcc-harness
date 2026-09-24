#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../params.h"
#include "../drng.h"
#include "../poly.h"

DRNG_ctx drng_algorithm;

#define NTESTS 10000

static void poly_naivemul(poly *c, const poly *a, const poly *b) {
    unsigned int i, j;
    int32_t r[2*N] = {0};

    for (i = 0; i < N; i++) {
        for (j = 0; j < N; j++) {
            r[i+j] = (r[i+j] + (int32_t)a->coeffs[i] * b->coeffs[j]) % Q;
        }
    }

    for (i = N; i < 2*N; ++i) {
        r[i-N] = (r[i-N] - r[i]) % Q;
    }

    for (i = 0; i < N; ++i) {
        c->coeffs[i] = r[i];
    }
}

int main(void)
{
    unsigned int i, j;
    int invret;
    uint8_t seed[SEEDBYTES];
    uint8_t nonce = 0;
    poly a, b, c, d, ainv;

    /* 初始化 drng */
    unsigned char init_seed[64];
    for (i = 0; i < 64; i++) init_seed[i] = (unsigned char)i;
    init_random_number(&drng_algorithm, init_seed, 64);

    get_random_number(&drng_algorithm, seed, (unsigned long long)sizeof(seed) * 8);

    // Test 1: NTT and inverse NTT roundtrip
    for (i = 0; i < NTESTS; ++i) {
        poly_f_ternary_p(&a, seed, nonce++);
        poly_g_ternary_p(&b, seed, nonce++);

        c = a;
        poly_ntt(&c);
        poly_invntt_tomont(&c);

        poly_tomont(&a);
        for (j = 0; j < N; ++j) {
            if ((a.coeffs[j] - c.coeffs[j]) % Q) {
                fprintf(stderr, "NTT and inverse NTT don't match: a[%d] = %d, c[%d] = %d\n", j, a.coeffs[j], j, c.coeffs[j]);
                return -1;
            }
        }
    }
    printf("NTT and inverse NTT test passed! %u\n", i);

    // Test 2: Polynomial multiplication
    for (i = 0; i < NTESTS; ++i) {
        poly_f_ternary_p(&a, seed, nonce++);
        poly_g_ternary_p(&b, seed, nonce++);

        poly_naivemul(&c, &a, &b);
        poly_ntt(&a);
        poly_ntt(&b);
        poly_basemul_montgomery(&d, &a, &b);
        poly_invntt_tomont(&d);

        for (j = 0; j < N; ++j) {
            if ((c.coeffs[j] - d.coeffs[j]) % Q) {
                fprintf(stderr, "Polynomial multiplication doesn't match: c[%d] = %d, d[%d] = %d\n", j, c.coeffs[j], j, d.coeffs[j]);
                return -1;
            }
        }
    }
    printf("Polynomial multiplication test passed! %u\n", i);

    // Test 3: Polynomial inversion in NTT domain
    for (i = 0; i < NTESTS; ++i) {
        do {
            poly_f_ternary_p(&a, seed, nonce++);
            poly_ntt(&a);
            invret = poly_baseinv(&ainv, &a);
        } while (invret != 0);

        poly_basemul_montgomery(&d, &a, &ainv);
        poly_invntt_tomont(&d);
        poly_freeze(&d);

        if ((d.coeffs[0] - 1) % Q) {
            fprintf(stderr, "Polynomial inverse check failed at constant term: d[0] = %d\n", d.coeffs[0]);
            return -1;
        }

        for (j = 1; j < N; ++j) {
            if (d.coeffs[j] % Q) {
                fprintf(stderr, "Polynomial inverse check failed: d[%d] = %d\n", j, d.coeffs[j]);
                return -1;
            }
        }
    }
    printf("Polynomial inversion (baseinv+basemul) test passed! %u\n", i);

    return 0;
}