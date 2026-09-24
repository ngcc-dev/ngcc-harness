/**
 * @file KEM_AlgorithmInstance.c
 * @brief Shared API_PKC adapter used by all Scloud+ KEM selections.
 *
 * The concrete algorithm parameters are supplied by the checked-in
 * parameters.h for the leaf entry and the compiler family/backend macros.
 * Keeping this wrapper in one place avoids maintaining identical copies in
 * every family/mode/tier entry.
 */

#include "KEM_AlgorithmInstance.h"
#include "kem.h"
#include <stdint.h>

unsigned long long kem_get_pk_len_bytes(void)
{
    return SCLOUDPLUS_PUBLIC_KEY_BYTES;
}

unsigned long long kem_get_sk_len_bytes(void)
{
    return SCLOUDPLUS_SECRET_KEY_BYTES;
}

unsigned long long kem_get_ss_len_bytes(void)
{
    return SCLOUDPLUS_SHARED_SECRET_BYTES;
}

unsigned long long kem_get_ct_len_bytes(void)
{
    return SCLOUDPLUS_CIPHERTEXT_BYTES;
}

int kem_keygen(unsigned char *pk, unsigned long long *pk_len_bytes,
               unsigned char *sk, unsigned long long *sk_len_bytes)
{
    int status;

    if (pk == 0 || sk == 0 || pk_len_bytes == 0 || sk_len_bytes == 0)
    {
        return -2;
    }
    status = scloud_kemkeygen((uint8_t *)pk, (uint8_t *)sk);
    if (status != 0)
    {
        return status;
    }
    *pk_len_bytes = SCLOUDPLUS_PUBLIC_KEY_BYTES;
    *sk_len_bytes = SCLOUDPLUS_SECRET_KEY_BYTES;
    return 0;
}

int kem_enc(const unsigned char *pk, unsigned long long pk_len_bytes,
            unsigned char *ss, unsigned long long *ss_len_bytes,
            unsigned char *ct, unsigned long long *ct_len_bytes)
{
    int status;

    if (pk == 0 || ss == 0 || ct == 0 || ss_len_bytes == 0 ||
        ct_len_bytes == 0)
    {
        return -2;
    }
    if (pk_len_bytes != SCLOUDPLUS_PUBLIC_KEY_BYTES)
    {
        return -3;
    }
    status = scloud_kemencaps((const uint8_t *)pk, (uint8_t *)ct, (uint8_t *)ss);
    if (status != 0)
    {
        return status;
    }
    *ss_len_bytes = SCLOUDPLUS_SHARED_SECRET_BYTES;
    *ct_len_bytes = SCLOUDPLUS_CIPHERTEXT_BYTES;
    return 0;
}

int kem_dec(const unsigned char *sk, unsigned long long sk_len_bytes,
            const unsigned char *ct, unsigned long long ct_len_bytes,
            unsigned char *ss, unsigned long long *ss_len_bytes)
{
    int status;

    if (sk == 0 || ct == 0 || ss == 0 || ss_len_bytes == 0)
    {
        return -2;
    }
    if (sk_len_bytes != SCLOUDPLUS_SECRET_KEY_BYTES ||
        ct_len_bytes != SCLOUDPLUS_CIPHERTEXT_BYTES)
    {
        return -3;
    }
    status = scloud_kemdecaps((const uint8_t *)sk, (const uint8_t *)ct, (uint8_t *)ss);
    *ss_len_bytes = SCLOUDPLUS_SHARED_SECRET_BYTES;
    return status;
}
