#include "KEM_AlgorithmInstance.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    unsigned char *ct_bad = calloc((size_t)ct_expected, 1);
    unsigned char *ss = calloc((size_t)ss_expected, 1);
    unsigned char *ss2 = calloc((size_t)ss_expected, 1);
    unsigned char *ss3 = calloc((size_t)ss_expected, 1);
    unsigned long long pk_len = 0;
    unsigned long long sk_len = 0;
    unsigned long long ct_len = 0;
    unsigned long long ss_len = 0;
    unsigned long long ss2_len = 0;
    unsigned long long ss3_len = 0;
    int rc = 0;

    if (!pk || !sk || !ct || !ct_bad || !ss || !ss2 || !ss3)
    {
        rc = 2;
        goto cleanup;
    }
    if (kem_keygen(pk, &pk_len, sk, &sk_len) != 0 ||
        pk_len != pk_expected || sk_len != sk_expected)
    {
        rc = 4;
        goto cleanup;
    }
    if (kem_enc(pk, pk_len, ss, &ss_len, ct, &ct_len) != 0 ||
        ss_len != ss_expected || ct_len != ct_expected)
    {
        rc = 5;
        goto cleanup;
    }

    const size_t positions[3] = {
        0u,
        (size_t)ct_expected / 2u,
        (size_t)ct_expected - 1u
    };
    for (size_t i = 0u; i < sizeof(positions) / sizeof(positions[0]); i++)
    {
        memcpy(ct_bad, ct, (size_t)ct_expected);
        ct_bad[positions[i]] ^= (unsigned char)(1u << (i & 7u));
        memset(ss2, 0, (size_t)ss_expected);
        memset(ss3, 0, (size_t)ss_expected);
        ss2_len = 0;
        ss3_len = 0;

        if (kem_dec(sk, sk_len, ct_bad, ct_len, ss2, &ss2_len) != 0 ||
            ss2_len != ss_expected)
        {
            rc = 6;
            goto cleanup;
        }
        if (kem_dec(sk, sk_len, ct_bad, ct_len, ss3, &ss3_len) != 0 ||
            ss3_len != ss_expected)
        {
            rc = 7;
            goto cleanup;
        }
        if (memcmp(ss2, ss3, (size_t)ss_expected) != 0)
        {
            rc = 8;
            goto cleanup;
        }
        if (memcmp(ss, ss2, (size_t)ss_expected) == 0)
        {
            rc = 9;
            goto cleanup;
        }
    }

    puts("tamper_pass");

cleanup:
    free(pk);
    free(sk);
    free(ct);
    free(ct_bad);
    free(ss);
    free(ss2);
    free(ss3);
    return rc;
}
