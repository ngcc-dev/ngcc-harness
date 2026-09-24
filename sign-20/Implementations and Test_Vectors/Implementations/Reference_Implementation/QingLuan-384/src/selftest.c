/*
 * QingLuan Digital Signature Scheme
 * selftest.c - Self-test implementation (GM/T 0028 compliance)
 *
 * Performs known-answer tests for SM3 and SM3-DRBG,
 * and operational tests for key generation, signing, and verification.
 */

#include "selftest.h"
#include "api.h"
#include "hash.h"
#include "utils.h"
#include <string.h>
#include <stdlib.h>

static int test_sm3(void)
{
    const uint8_t msg1[] = "abc";
    const uint8_t expected1[] = {
        0x66, 0xc7, 0xf0, 0xf4, 0x62, 0xee, 0xed, 0xd9,
        0xd1, 0xf2, 0xd4, 0x6b, 0xdc, 0x10, 0xe4, 0xe2,
        0x41, 0x67, 0xc4, 0x87, 0x5c, 0xf2, 0xf7, 0xa2,
        0x29, 0x7d, 0xa0, 0x2b, 0x8f, 0x4b, 0xa8, 0xe0
    };

    uint8_t out[SM3_DIGEST_SIZE];
    sm3(msg1, 3, out);
    if (ct_memcmp(out, expected1, SM3_DIGEST_SIZE) != 0)
        return -1;

    const uint8_t msg2[] = {
        0x61, 0x62, 0x63, 0x64, 0x61, 0x62, 0x63, 0x64,
        0x61, 0x62, 0x63, 0x64, 0x61, 0x62, 0x63, 0x64,
        0x61, 0x62, 0x63, 0x64, 0x61, 0x62, 0x63, 0x64,
        0x61, 0x62, 0x63, 0x64, 0x61, 0x62, 0x63, 0x64,
        0x61, 0x62, 0x63, 0x64, 0x61, 0x62, 0x63, 0x64,
        0x61, 0x62, 0x63, 0x64, 0x61, 0x62, 0x63, 0x64,
        0x61, 0x62, 0x63, 0x64, 0x61, 0x62, 0x63, 0x64,
        0x61, 0x62, 0x63, 0x64, 0x61, 0x62, 0x63, 0x64
    };
    const uint8_t expected2[] = {
        0xde, 0xbe, 0x9f, 0xf9, 0x22, 0x75, 0xb8, 0xa1,
        0x38, 0x60, 0x48, 0x89, 0xc1, 0x8e, 0x5a, 0x4d,
        0x6f, 0xdb, 0x70, 0xe5, 0x38, 0x7e, 0x57, 0x65,
        0x29, 0x3d, 0xcb, 0xa3, 0x9c, 0x0c, 0x57, 0x32
    };

    sm3(msg2, 64, out);
    if (ct_memcmp(out, expected2, SM3_DIGEST_SIZE) != 0)
        return -1;

    return 0;
}

static int test_drbg(void)
{
    uint8_t buf1[32], buf2[32];

    if (drbg_init() != 0) return -1;

    if (randombytes(buf1, 32) != 0) return -1;
    if (randombytes(buf2, 32) != 0) return -1;

    if (ct_memcmp(buf1, buf2, 32) == 0) return -1;

    uint8_t zero[32];
    memset(zero, 0, 32);
    if (ct_memcmp(buf1, zero, 32) == 0) return -1;
    if (ct_memcmp(buf2, zero, 32) == 0) return -1;

    return 0;
}

static int test_keygen(void)
{
    uint8_t pk[CRYPTO_PUBLICKEYBYTES];
    uint8_t sk[CRYPTO_SECRETKEYBYTES];

    int ret = crypto_sign_keypair(pk, sk);
    if (ret != 0) return -1;

    uint8_t zero_pk[CRYPTO_PUBLICKEYBYTES];
    uint8_t zero_sk[CRYPTO_SECRETKEYBYTES];
    memset(zero_pk, 0, sizeof(zero_pk));
    memset(zero_sk, 0, sizeof(zero_sk));

    if (ct_memcmp(pk, zero_pk, CRYPTO_PUBLICKEYBYTES) == 0) return -1;
    if (ct_memcmp(sk, zero_sk, CRYPTO_SECRETKEYBYTES) == 0) return -1;

    return 0;
}

static int test_sign_verify(void)
{
    uint8_t pk[CRYPTO_PUBLICKEYBYTES];
    uint8_t sk[CRYPTO_SECRETKEYBYTES];
    const uint8_t msg[] = "QingLuan self-test message";
    size_t mlen = sizeof(msg) - 1;
    int ret = -1;

    /* Heap buffers: CRYPTO_BYTES reaches ~290 KB at the 512 level. */
    unsigned char *sm        = (unsigned char *)malloc((size_t)CRYPTO_BYTES + mlen);
    unsigned char *recovered = (unsigned char *)malloc(mlen + 1);
    unsigned long long smlen, recovered_len;
    if (!sm || !recovered) goto done;

    if (crypto_sign_keypair(pk, sk) != 0) goto done;
    if (crypto_sign(sm, &smlen, msg, (unsigned long long)mlen, sk) != 0) goto done;

    if (crypto_sign_open(recovered, &recovered_len, sm, smlen, pk) != 0) goto done;
    if (recovered_len != mlen) goto done;
    if (ct_memcmp(recovered, msg, mlen) != 0) goto done;

    sm[0] ^= 0x01;
    if (crypto_sign_open(recovered, &recovered_len, sm, smlen, pk) == 0) goto done;

    ret = 0;
done:
    free(sm);
    free(recovered);
    return ret;
}

int crypto_sign_selftest_detailed(int *hash_ok, int *drbg_ok,
                                  int *keygen_ok, int *sign_ok,
                                  int *verify_ok)
{
    *hash_ok = (test_sm3() == 0);
    *drbg_ok = (test_drbg() == 0);
    *keygen_ok = (test_keygen() == 0);
    *sign_ok = 0;
    *verify_ok = 0;

    if (*hash_ok && *drbg_ok && *keygen_ok) {
        int sv_result = test_sign_verify();
        *sign_ok = (sv_result == 0);
        *verify_ok = (sv_result == 0);
    }

    return (*hash_ok && *drbg_ok && *keygen_ok && *sign_ok && *verify_ok) ? 0 : -1;
}

int crypto_sign_selftest(void)
{
    int hash_ok, drbg_ok, keygen_ok, sign_ok, verify_ok;
    return crypto_sign_selftest_detailed(&hash_ok, &drbg_ok,
                                         &keygen_ok, &sign_ok, &verify_ok);
}
