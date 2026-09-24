#include "SIG_AlgorithmInstance.h"
#include "drng.h"
#include <stddef.h>
#include "params.h"
#include "sign.h"

/* Global DRNG context — initialized by KAT program with seed */
extern DRNG_ctx drng_algorithm;

unsigned long long sig_get_pk_len_bytes() {
    return CRYPTO_PUBLICKEYBYTES;
}

unsigned long long sig_get_sk_len_bytes() {
    return CRYPTO_SECRETKEYBYTES;
}

unsigned long long sig_get_sn_len_bytes() {
    return CRYPTO_BYTES;
}

int sig_keygen(
    unsigned char *pk, unsigned long long *pk_len_bytes,
    unsigned char *sk, unsigned long long *sk_len_bytes)
{
    int ret = crypto_sign_keypair(pk, sk);
    *pk_len_bytes = CRYPTO_PUBLICKEYBYTES;
    *sk_len_bytes = CRYPTO_SECRETKEYBYTES;
    return ret;
}

int sig_sign(
    unsigned char *sk, unsigned long long sk_len_bytes,
    unsigned char *m, unsigned long long m_len_bytes,
    unsigned char *sn, unsigned long long *sn_len_bytes)
{
    /* KAT testing does not use context string */
    size_t out_len;
    int ret = crypto_sign_signature(sn, &out_len, m, m_len_bytes, NULL, 0, sk);
    *sn_len_bytes = out_len;
    return ret;
}

int sig_verify(
    unsigned char *pk, unsigned long long pk_len_bytes,
    unsigned char *sn, unsigned long long sn_len_bytes,
    unsigned char *m, unsigned long long m_len_bytes)
{
    /* Returns 0 on success, -1 on failure */
    return crypto_sign_verify(sn, sn_len_bytes, m, m_len_bytes, NULL, 0, pk);
}
