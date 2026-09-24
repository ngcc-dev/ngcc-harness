/*
 * QingLuan -> API_PKC adapter
 *
 * Wraps QingLuan's crypto_sign_* into sig_keygen/sig_sign/sig_verify
 * required by the API_PKC KAT framework.
 */

#include "SIG_AlgorithmInstance.h"
#include "drng.h"
#include "api.h"
#include "params.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* drng_algorithm is defined in KAT_SIG.c — used by the adapter layer as
 * the single source of cryptographic randomness for keygen/sign. */
extern DRNG_ctx drng_algorithm;

unsigned long long sig_get_pk_len_bytes(void)  { return (unsigned long long)QINGLUAN_PK_BYTES; }
unsigned long long sig_get_sk_len_bytes(void)  { return (unsigned long long)QINGLUAN_SK_BYTES; }
unsigned long long sig_get_sn_len_bytes(void)  { return (unsigned long long)QINGLUAN_SIG_BYTES; }

int sig_keygen(
    unsigned char *pk, unsigned long long *pk_len_bytes,
    unsigned char *sk, unsigned long long *sk_len_bytes)
{
    if (crypto_sign_keypair(pk, sk) != 0) return -1;
    *pk_len_bytes = (unsigned long long)QINGLUAN_PK_BYTES;
    *sk_len_bytes = (unsigned long long)QINGLUAN_SK_BYTES;
    return 0;
}

int sig_sign(
    unsigned char *sk, unsigned long long sk_len_bytes,
    unsigned char *m,  unsigned long long m_len_bytes,
    unsigned char *sn, unsigned long long *sn_len_bytes)
{
    (void)sk_len_bytes;
    size_t siglen = 0;
    int rc = crypto_sign_signature(sn, &siglen, m, (size_t)m_len_bytes, sk);
    if (rc != 0) return -1;
    *sn_len_bytes = (unsigned long long)siglen;
    return 0;
}

int sig_verify(
    unsigned char *pk, unsigned long long pk_len_bytes,
    unsigned char *sn, unsigned long long sn_len_bytes,
    unsigned char *m,  unsigned long long m_len_bytes)
{
    (void)pk_len_bytes;
    int rc = crypto_sign_verify(sn, (size_t)sn_len_bytes, m, (size_t)m_len_bytes, pk);
    return (rc == 0) ? 0 : -1;
}
