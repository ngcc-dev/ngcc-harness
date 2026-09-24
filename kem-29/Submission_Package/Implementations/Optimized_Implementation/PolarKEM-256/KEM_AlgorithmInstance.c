/* Official API_PKC wrapper for PolarKEM-256. */

#include <stddef.h>

#include "KEM_AlgorithmInstance.h"
#include "polarkem_core.h"
#include "polarkem_params.h"

/* The unmodified official KAT driver owns and initializes this context. */
extern DRNG_ctx drng_algorithm;

enum
{
    POLARKEM_API_PK_BYTES = 2048,
    POLARKEM_API_SK_BYTES = 4096,
    POLARKEM_API_CT_BYTES = 1280,
    POLARKEM_API_SS_BYTES = 32
};

/* C99 compile-time checks keep the official API and parameter profile aligned. */
typedef char polarkem_api_pk_size_must_match[
    (POLARKEM_PK_BYTES == POLARKEM_API_PK_BYTES) ? 1 : -1];
typedef char polarkem_api_sk_size_must_match[
    (POLARKEM_SK_BYTES == POLARKEM_API_SK_BYTES) ? 1 : -1];
typedef char polarkem_api_ct_size_must_match[
    (POLARKEM_CT_BYTES == POLARKEM_API_CT_BYTES) ? 1 : -1];
typedef char polarkem_api_ss_size_must_match[
    (POLARKEM_SS_BYTES == POLARKEM_API_SS_BYTES) ? 1 : -1];

/** Return the fixed public-key length in bytes. */
unsigned long long kem_get_pk_len_bytes(void)
{
    return (unsigned long long)POLARKEM_API_PK_BYTES;
}

/** Return the fixed private-key length in bytes. */
unsigned long long kem_get_sk_len_bytes(void)
{
    return (unsigned long long)POLARKEM_API_SK_BYTES;
}

/** Return the fixed shared-secret length in bytes. */
unsigned long long kem_get_ss_len_bytes(void)
{
    return (unsigned long long)POLARKEM_API_SS_BYTES;
}

/** Return the fixed ciphertext length in bytes. */
unsigned long long kem_get_ct_len_bytes(void)
{
    return (unsigned long long)POLARKEM_API_CT_BYTES;
}

/** Validate output pointers and delegate key generation to the core. */
int kem_keygen(
    unsigned char *pk, unsigned long long *pk_len_bytes,
    unsigned char *sk, unsigned long long *sk_len_bytes)
{
    int status;

    if (pk_len_bytes != NULL)
    {
        *pk_len_bytes = 0;
    }
    if (sk_len_bytes != NULL)
    {
        *sk_len_bytes = 0;
    }
    if ((pk == NULL) || (pk_len_bytes == NULL) ||
        (sk == NULL) || (sk_len_bytes == NULL))
    {
        return KEM_ERR_NULL_POINTER;
    }

    status = polarkem_keygen(pk, sk, &drng_algorithm);
    if (status != KEM_SUCCESS)
    {
        return KEM_ERR_PRIMITIVE;
    }

    *pk_len_bytes = kem_get_pk_len_bytes();
    *sk_len_bytes = kem_get_sk_len_bytes();
    return KEM_SUCCESS;
}

/** Validate pointers and the public-key length, then encapsulate. */
int kem_enc(
    unsigned char *pk, unsigned long long pk_len_bytes,
    unsigned char *ss, unsigned long long *ss_len_bytes,
    unsigned char *ct, unsigned long long *ct_len_bytes)
{
    int status;

    if (ss_len_bytes != NULL)
    {
        *ss_len_bytes = 0;
    }
    if (ct_len_bytes != NULL)
    {
        *ct_len_bytes = 0;
    }
    if ((pk == NULL) || (ss == NULL) || (ss_len_bytes == NULL) ||
        (ct == NULL) || (ct_len_bytes == NULL))
    {
        return KEM_ERR_NULL_POINTER;
    }
    if (pk_len_bytes != kem_get_pk_len_bytes())
    {
        return KEM_ERR_LENGTH;
    }

    status = polarkem_validate_public_key(pk);
    if (status == KEM_ERR_LENGTH)
    {
        return KEM_ERR_LENGTH;
    }
    if (status != KEM_SUCCESS)
    {
        return KEM_ERR_PRIMITIVE;
    }

    status = polarkem_enc(pk, ss, ct, &drng_algorithm);
    if (status == KEM_ERR_LENGTH)
    {
        return KEM_ERR_LENGTH;
    }
    if (status != KEM_SUCCESS)
    {
        return KEM_ERR_PRIMITIVE;
    }

    *ss_len_bytes = kem_get_ss_len_bytes();
    *ct_len_bytes = kem_get_ct_len_bytes();
    return KEM_SUCCESS;
}

/** Validate inputs, decapsulate, and preserve implicit-rejection semantics. */
int kem_dec(
    unsigned char *sk, unsigned long long sk_len_bytes,
    unsigned char *ct, unsigned long long ct_len_bytes,
    unsigned char *ss, unsigned long long *ss_len_bytes)
{
    int status;

    if (ss_len_bytes != NULL)
    {
        *ss_len_bytes = 0;
    }
    if ((sk == NULL) || (ct == NULL) ||
        (ss == NULL) || (ss_len_bytes == NULL))
    {
        return KEM_ERR_NULL_POINTER;
    }
    if ((sk_len_bytes != kem_get_sk_len_bytes()) ||
        (ct_len_bytes != kem_get_ct_len_bytes()))
    {
        return KEM_ERR_LENGTH;
    }

    status = polarkem_validate_secret_key(sk);
    if (status == KEM_ERR_LENGTH)
    {
        return KEM_ERR_LENGTH;
    }
    if (status != KEM_SUCCESS)
    {
        return KEM_ERR_PRIMITIVE;
    }

    status = polarkem_dec(sk, ct, ss);
    if ((status == KEM_SUCCESS) ||
        (status == KEM_ERR_CIPHERTEXT_INVALID))
    {
        *ss_len_bytes = kem_get_ss_len_bytes();
        return status;
    }
    if (status == KEM_ERR_LENGTH)
    {
        return KEM_ERR_LENGTH;
    }

    return KEM_ERR_PRIMITIVE;
}
