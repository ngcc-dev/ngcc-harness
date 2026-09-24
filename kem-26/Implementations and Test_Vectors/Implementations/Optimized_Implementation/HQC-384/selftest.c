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
    rc |= nss_hqc_selftest_ring();
    rc |= nss_hqc_selftest_sample();
    rc |= nss_hqc_selftest_quant();
    rc |= nss_hqc_selftest_dither();
    rc |= nss_hqc_code_selftest();
    rc |= nss_hqc_selftest_pke();
    rc |= nss_hqc_selftest_kem();
    if (rc != 0) {
        fprintf(stderr, "NSS-HQC selftest failed\n");
        return 1;
    }
    printf("NSS-HQC selftest passed\n");
    return 0;
}
