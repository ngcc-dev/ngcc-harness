#include "SIG_AlgorithmInstance.h"
#include <stdlib.h>
#include <string.h>

#include "drng.h"
#include "api.h"
extern DRNG_ctx drng_algorithm;

unsigned long long sig_get_pk_len_bytes()
{
    return CRYPTO_PUBLICKEYBYTES;
}

unsigned long long sig_get_sk_len_bytes()
{
    return CRYPTO_SECRETKEYBYTES;
}

unsigned long long sig_get_sn_len_bytes()
{
    return CRYPTO_BYTES;
}

int sig_keygen(
    unsigned char *pk, unsigned long long *pk_len_bytes,
    unsigned char *sk, unsigned long long *sk_len_bytes)
{
    int ret = crypto_sign_keypair(pk, sk);
    if (ret != 0) {
        return -2;
    }

    *pk_len_bytes = CRYPTO_PUBLICKEYBYTES;
    *sk_len_bytes = CRYPTO_SECRETKEYBYTES;
    return 0;
}

int sig_sign(
    unsigned char *sk, unsigned long long sk_len_bytes,
    unsigned char *m, unsigned long long m_len_bytes,
    unsigned char *sn, unsigned long long *sn_len_bytes)
{
    if (sk_len_bytes != CRYPTO_SECRETKEYBYTES) {
        return -2;
    }

    unsigned long long signed_len = 0;
    unsigned char *signed_msg = (unsigned char *)malloc(m_len_bytes + CRYPTO_BYTES);
    if (signed_msg == NULL) {
        return -4;
    }

    int ret = crypto_sign(signed_msg, &signed_len, m, m_len_bytes, sk);
    if (ret != 0) {
        free(signed_msg);
        return -3;
    }

    if (signed_len != m_len_bytes + CRYPTO_BYTES) {
        free(signed_msg);
        return -3;
    }

    memcpy(sn, signed_msg + m_len_bytes, CRYPTO_BYTES);
    *sn_len_bytes = CRYPTO_BYTES;
    free(signed_msg);
    return 0;
}

int sig_verify(
    unsigned char *pk, unsigned long long pk_len_bytes,
    unsigned char *sn, unsigned long long sn_len_bytes,
    unsigned char *m, unsigned long long m_len_bytes)
{
    if (pk_len_bytes != CRYPTO_PUBLICKEYBYTES) {
        return -2;
    }

    if (sn_len_bytes != CRYPTO_BYTES) {
        return -1;
    }

    unsigned long long opened_len = 0;
    unsigned char *signed_msg = (unsigned char *)malloc(m_len_bytes + sn_len_bytes);
    unsigned char *opened_msg = (unsigned char *)malloc(m_len_bytes);
    if (signed_msg == NULL || opened_msg == NULL) {
        free(signed_msg);
        free(opened_msg);
        return -4;
    }

    memcpy(signed_msg, m, m_len_bytes);
    memcpy(signed_msg + m_len_bytes, sn, sn_len_bytes);

    const int ret = crypto_sign_open(
        opened_msg, &opened_len, signed_msg, m_len_bytes + sn_len_bytes, pk);
    const int ok = ret == 0 && opened_len == m_len_bytes &&
        memcmp(opened_msg, m, m_len_bytes) == 0;

    free(signed_msg);
    free(opened_msg);
    return ok ? 0 : -1;
}
