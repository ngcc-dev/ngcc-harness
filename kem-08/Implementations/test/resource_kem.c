#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "KEM_AlgorithmInstance.h"
#include "drng.h"

DRNG_ctx drng_algorithm;

int main(void)
{
    static const uint8_t seed[48] = {
        0x42, 0x57, 0x4b, 0x45, 0x4d, 0x2d, 0x72, 0x65,
        0x73, 0x6f, 0x75, 0x72, 0x63, 0x65, 0x2d, 0x74,
        0x65, 0x73, 0x74, 0x2d, 0x73, 0x65, 0x65, 0x64,
        0x2d, 0x30, 0x31, 0x2d, 0x42, 0x57, 0x4b, 0x45,
        0x4d, 0x2d, 0x72, 0x65, 0x73, 0x6f, 0x75, 0x72,
        0x63, 0x65, 0x2d, 0x74, 0x65, 0x73, 0x74, 0x21
    };
    unsigned long long pk_len = kem_get_pk_len_bytes();
    unsigned long long sk_len = kem_get_sk_len_bytes();
    unsigned long long ct_len = kem_get_ct_len_bytes();
    unsigned long long ss_len = kem_get_ss_len_bytes();
    unsigned long long dec_ss_len = ss_len;
    unsigned char *pk = calloc((size_t)pk_len, 1);
    unsigned char *sk = calloc((size_t)sk_len, 1);
    unsigned char *ct = calloc((size_t)ct_len, 1);
    unsigned char *ss = calloc((size_t)ss_len, 1);
    unsigned char *dec_ss = calloc((size_t)ss_len, 1);
    int rc = 1;

    if (pk == NULL || sk == NULL || ct == NULL || ss == NULL || dec_ss == NULL)
        goto cleanup;
    if (init_random_number(&drng_algorithm, seed, sizeof(seed)) != 0)
        goto cleanup;
    if (kem_keygen(pk, &pk_len, sk, &sk_len) != 0)
        goto cleanup;
    if (kem_enc(pk, pk_len, ss, &ss_len, ct, &ct_len) != 0)
        goto cleanup;
    if (kem_dec(sk, sk_len, ct, ct_len, dec_ss, &dec_ss_len) != 0)
        goto cleanup;
    if (ss_len != dec_ss_len || memcmp(ss, dec_ss, (size_t)ss_len) != 0)
        goto cleanup;

    rc = 0;

cleanup:
    free(dec_ss);
    free(ss);
    free(ct);
    free(sk);
    free(pk);
    return rc;
}
