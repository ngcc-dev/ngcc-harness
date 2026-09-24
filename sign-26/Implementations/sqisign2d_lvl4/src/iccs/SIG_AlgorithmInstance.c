#include "SIG_AlgorithmInstance.h"

#include <stddef.h>

#include <torsion_constants.h>
#include <encoded_sizes.h>
#include <sig.h>

#define ICCS_SIG_SUCCESS 0
#define ICCS_SIG_INVALID_SIGNATURE -1
#define ICCS_SIG_INVALID_PARAMETER -2
#define ICCS_SIG_CRYPTO_FAILURE -3

unsigned long long
sig_get_pk_len_bytes(void)
{
    return PUBLICKEY_BYTES;
}

unsigned long long
sig_get_sk_len_bytes(void)
{
    return SECRETKEY_BYTES;
}

unsigned long long
sig_get_sn_len_bytes(void)
{
    return SIGNATURE_LEN;
}

int
sig_keygen(unsigned char *pk,
           unsigned long long *pk_len_bytes,
           unsigned char *sk,
           unsigned long long *sk_len_bytes)
{
    if (pk == NULL || pk_len_bytes == NULL || sk == NULL || sk_len_bytes == NULL) {
        return ICCS_SIG_INVALID_PARAMETER;
    }

    if (sqisign_keypair(pk, sk) != 0) {
        return ICCS_SIG_CRYPTO_FAILURE;
    }

    *pk_len_bytes = PUBLICKEY_BYTES;
    *sk_len_bytes = SECRETKEY_BYTES;

    return ICCS_SIG_SUCCESS;
}

int
sig_sign(unsigned char *sk,
         unsigned long long sk_len_bytes,
         unsigned char *m,
         unsigned long long m_len_bytes,
         unsigned char *sn,
         unsigned long long *sn_len_bytes)
{
    if (sk == NULL || m == NULL || sn == NULL || sn_len_bytes == NULL) {
        return ICCS_SIG_INVALID_PARAMETER;
    }
    if (sk_len_bytes != SECRETKEY_BYTES) {
        return ICCS_SIG_INVALID_PARAMETER;
    }

    if (sqisign_signature(sn, sn_len_bytes, m, m_len_bytes, sk) != 0) {
        return ICCS_SIG_CRYPTO_FAILURE;
    }
    if (*sn_len_bytes != SIGNATURE_LEN) {
        return ICCS_SIG_CRYPTO_FAILURE;
    }

    return ICCS_SIG_SUCCESS;
}

int
sig_verify(unsigned char *pk,
           unsigned long long pk_len_bytes,
           unsigned char *sn,
           unsigned long long sn_len_bytes,
           unsigned char *m,
           unsigned long long m_len_bytes)
{
    if (pk == NULL || sn == NULL || m == NULL) {
        return ICCS_SIG_INVALID_PARAMETER;
    }
    if (pk_len_bytes != PUBLICKEY_BYTES) {
        return ICCS_SIG_INVALID_PARAMETER;
    }

    if (sqisign_verify(m, m_len_bytes, sn, sn_len_bytes, pk) != 0) {
        return ICCS_SIG_INVALID_SIGNATURE;
    }

    return ICCS_SIG_SUCCESS;
}
