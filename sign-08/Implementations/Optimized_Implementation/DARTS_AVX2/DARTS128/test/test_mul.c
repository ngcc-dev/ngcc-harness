#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../params.h"
#include "../drng.h"
#include "../poly.h"

#define NTESTS 10000

static void poly_naivemul(poly *c, const poly *a, const poly *b) {
    unsigned int i, j;
    int64_t r[2*N] = {0};

    for (i = 0; i < N; i++) {
        for (j = 0; j < N; j++) {
            r[i+j] = (r[i+j] + (int64_t)a->coeffs[i] * b->coeffs[j]) % Q;
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
    uint8_t seed[SEEDBYTES];
    uint16_t nonce = 0;
    poly a, b, c, d;

    /* 初始化 DRNG */
    DRNG_ctx drng;
    unsigned char drng_seed[48];
    memset(drng_seed, 0x42, sizeof(drng_seed));
    init_random_number(&drng, drng_seed, sizeof(drng_seed));

    get_random_number(&drng, seed, sizeof(seed) * 8);

    for (i = 0; i < NTESTS; ++i) {
        poly_uniform(&a, seed, nonce++);
        poly_uniform(&b, seed, nonce++);

        c = a;

        poly_ntt(&c);
        poly_invntt_tomont(&c);

        poly_mul_mont(&a);

        for (j = 0; j < N; ++j) {
            if ((a.coeffs[j] - c.coeffs[j]) % Q) {
                fprintf(stderr, "NTT and inverse NTT don't match: a[%d] = %d, c[%d] = %d\n", j, a.coeffs[j], j, c.coeffs[j]);
                return -1;
            }
        }

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

    printf("NTT and inverse NTT test passed!\n");
    printf("Polynomial multiplication test passed!\n");

    return 0;
}