#include <stdint.h>
#include <stdio.h>

#include "drng.h"
#include "code_layer.h"
#include "nss_hqc_core.h"

DRNG_ctx drng_algorithm;

int main(void)
{
    int rc = 0;
    uint8_t seed[48] = {0};
    uint32_t i;
    for (i = 0u; i < sizeof(seed); i++) {
        seed[i] = (uint8_t)(0xc0u + i);
    }
    init_random_number(&drng_algorithm, seed, sizeof(seed));
    if (nss_hqc_selftest_ring() != 0) { fprintf(stderr, "ring selftest failed\n"); rc = 1; }
    if (nss_hqc_selftest_sample() != 0) { fprintf(stderr, "sample selftest failed\n"); rc = 1; }
    if (nss_hqc_selftest_quant() != 0) { fprintf(stderr, "quant selftest failed\n"); rc = 1; }
    if (nss_hqc_selftest_dither() != 0) { fprintf(stderr, "dither selftest failed\n"); rc = 1; }
    if (nss_hqc_code_selftest() != 0) { fprintf(stderr, "code selftest failed\n"); rc = 1; }
    if (nss_hqc_selftest_pke() != 0) { fprintf(stderr, "pke selftest failed\n"); rc = 1; }
    if (nss_hqc_selftest_kem() != 0) { fprintf(stderr, "kem selftest failed\n"); rc = 1; }
    if (rc != 0) {
        fprintf(stderr, "NSS-HQC selftest failed\n");
        return 1;
    }
    printf("NSS-HQC selftest passed\n");
    return 0;
}
