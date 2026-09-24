#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "drng.h"
#include "nss_hqc_core.h"

DRNG_ctx drng_algorithm;

static int ss_equal(const uint8_t *a, const uint8_t *b, size_t len)
{
    size_t i;
    uint8_t diff = 0u;
    for (i = 0u; i < len; i++) {
        diff |= (uint8_t)(a[i] ^ b[i]);
    }
    return diff == 0u;
}

static uint32_t loop_count_for_instance(void)
{
#if NSS_HQC_N == 29443u
    return 1000u;
#elif NSS_HQC_N == 54493u
    return 300u;
#elif NSS_HQC_N == 122579u
    return 100u;
#else
    return 50u;
#endif
}

int main(void)
{
    uint8_t seed[48];
    uint8_t *pk = (uint8_t *)calloc(NSS_HQC_PK_BYTES, 1u);
    uint8_t *sk = (uint8_t *)calloc(NSS_HQC_SK_BYTES, 1u);
    uint8_t *ct = (uint8_t *)calloc(NSS_HQC_CT_FULL_BYTES, 1u);
    uint8_t *tampered = (uint8_t *)calloc(NSS_HQC_CT_FULL_BYTES, 1u);
    uint8_t ss1[NSS_HQC_SS_BYTES];
    uint8_t ss2[NSS_HQC_SS_BYTES];
    unsigned long long pk_len = 0u, sk_len = 0u, ct_len = 0u, ss_len = 0u;
    uint32_t loops = loop_count_for_instance();
    uint32_t success = 0u, decap_failure = 0u, tamper_rejection = 0u;
    uint32_t i;
    clock_t begin, end;

    if (pk == NULL || sk == NULL || ct == NULL || tampered == NULL) {
        free(pk); free(sk); free(ct); free(tampered);
        return 1;
    }
    for (i = 0u; i < sizeof(seed); i++) {
        seed[i] = (uint8_t)(0x70u + i);
    }
    init_random_number(&drng_algorithm, seed, sizeof(seed));

    begin = clock();
    for (i = 0u; i < loops; i++) {
        if (nss_hqc_keygen(pk, &pk_len, sk, &sk_len) != 0 ||
            nss_hqc_enc(pk, pk_len, ss1, &ss_len, ct, &ct_len) != 0) {
            decap_failure++;
            continue;
        }
        memset(ss2, 0, sizeof(ss2));
        if (nss_hqc_dec(sk, sk_len, ct, ct_len, ss2, &ss_len) == 0 &&
            ss_equal(ss1, ss2, NSS_HQC_SS_BYTES)) {
            success++;
        } else {
            decap_failure++;
        }

        memcpy(tampered, ct, NSS_HQC_CT_FULL_BYTES);
        tampered[i % NSS_HQC_CT_FULL_BYTES] ^= 1u;
        memset(ss2, 0, sizeof(ss2));
        if (nss_hqc_dec(sk, sk_len, tampered, ct_len, ss2, &ss_len) != 0 &&
            !ss_equal(ss1, ss2, NSS_HQC_SS_BYTES)) {
            tamper_rejection++;
        }
    }
    end = clock();
    printf("[diag][kem-loop] instance=%s loops=%lu success=%lu decap_failure=%lu tamper_rejection=%lu elapsed_seconds=%.6f\n",
           NSS_HQC_INSTANCE_NAME,
           (unsigned long)loops,
           (unsigned long)success,
           (unsigned long)decap_failure,
           (unsigned long)tamper_rejection,
           (double)(end - begin) / (double)CLOCKS_PER_SEC);
    free(pk); free(sk); free(ct); free(tampered);
    return (success == loops && decap_failure == 0u && tamper_rejection == loops) ? 0 : 1;
}
