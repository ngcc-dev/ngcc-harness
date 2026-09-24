#include "KEM_AlgorithmInstance.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef SCLOUDPLUS_INSTANCE_NAME
#define SCLOUDPLUS_INSTANCE_NAME ALGORITHM_INSTANCE
#endif

int main(void)
{
    const unsigned long long pk_expected = kem_get_pk_len_bytes();
    const unsigned long long sk_expected = kem_get_sk_len_bytes();
    const unsigned long long ct_expected = kem_get_ct_len_bytes();
    const unsigned long long ss_expected = kem_get_ss_len_bytes();

    if (pk_expected == 0 || sk_expected == 0 || ct_expected == 0 ||
        ss_expected == 0)
    {
        return 3;
    }

    unsigned char *pk = calloc((size_t)pk_expected, 1);
    unsigned char *sk = calloc((size_t)sk_expected, 1);
    unsigned char *ct = calloc((size_t)ct_expected, 1);
    unsigned char *ss = calloc((size_t)ss_expected, 1);
    unsigned char *ss2 = calloc((size_t)ss_expected, 1);
    unsigned long long pk_len;
    unsigned long long sk_len;
    unsigned long long ct_len;
    unsigned long long ss_len;
    unsigned long long ss2_len;
    int rc = 0;

    if (!pk || !sk || !ct || !ss || !ss2)
    {
        rc = 2;
        goto cleanup;
    }
    pk_len = sk_len = ct_len = ss_len = ss2_len = 0;
    if (kem_keygen(NULL, &pk_len, sk, &sk_len) != -2 ||
        kem_keygen(pk, NULL, sk, &sk_len) != -2 ||
        kem_keygen(pk, &pk_len, NULL, &sk_len) != -2 ||
        kem_keygen(pk, &pk_len, sk, NULL) != -2 ||
        kem_enc(NULL, pk_expected, ss, &ss_len, ct, &ct_len) != -2 ||
        kem_enc(pk, pk_expected, NULL, &ss_len, ct, &ct_len) != -2 ||
        kem_enc(pk, pk_expected, ss, NULL, ct, &ct_len) != -2 ||
        kem_enc(pk, pk_expected, ss, &ss_len, NULL, &ct_len) != -2 ||
        kem_enc(pk, pk_expected, ss, &ss_len, ct, NULL) != -2 ||
        kem_dec(NULL, sk_expected, ct, ct_expected, ss2, &ss2_len) != -2 ||
        kem_dec(sk, sk_expected, NULL, ct_expected, ss2, &ss2_len) != -2 ||
        kem_dec(sk, sk_expected, ct, ct_expected, NULL, &ss2_len) != -2 ||
        kem_dec(sk, sk_expected, ct, ct_expected, ss2, NULL) != -2)
    {
        rc = 4;
        goto cleanup;
    }
    if (kem_enc(pk, pk_expected - 1u, ss, &ss_len, ct, &ct_len) != -3 ||
        kem_dec(sk, sk_expected - 1u, ct, ct_expected, ss2, &ss2_len) != -3 ||
        kem_dec(sk, sk_expected, ct, ct_expected - 1u, ss2, &ss2_len) != -3)
    {
        rc = 5;
        goto cleanup;
    }

    for (int i = 0; i < 100; i++)
    {
        pk_len = sk_len = ct_len = ss_len = ss2_len = 0;
        if (kem_keygen(pk, &pk_len, sk, &sk_len) != 0 ||
            pk_len != pk_expected || sk_len != sk_expected)
        {
            rc = 6;
            goto cleanup;
        }
        if (kem_enc(pk, pk_len, ss, &ss_len, ct, &ct_len) != 0 ||
            ss_len != ss_expected || ct_len != ct_expected)
        {
            rc = 7;
            goto cleanup;
        }
        if (kem_dec(sk, sk_len, ct, ct_len, ss2, &ss2_len) != 0 ||
            ss2_len != ss_expected)
        {
            rc = 8;
            goto cleanup;
        }
        if (memcmp(ss, ss2, (size_t)ss_expected) != 0)
        {
            rc = 9;
            goto cleanup;
        }
    }

    printf("kem_loop_pass %s\n", SCLOUDPLUS_INSTANCE_NAME);

cleanup:
    free(pk);
    free(sk);
    free(ct);
    free(ss);
    free(ss2);
    return rc;
}
