/* API_PKC adapter from the official KEM interface to the Frost core API. */
#include <stddef.h>
#include "KEM_AlgorithmInstance.h"
#include "Frost/src/api_frost256.h"

#define KEM_ERR_INVALID_ARGUMENT (-2)
#define KEM_ERR_INTERNAL         (-3)

static void api_clear(unsigned char *buffer, unsigned long long length)
{
    volatile unsigned char *p = buffer;
    while (length-- != 0U) {
        *p++ = 0;
    }
}

unsigned long long kem_get_pk_len_bytes(void) { return PUBLICKEYBYTES; }
unsigned long long kem_get_sk_len_bytes(void) { return SECRETKEYBYTES; }
unsigned long long kem_get_ss_len_bytes(void) { return SHAREDSECRETBYTES; }
unsigned long long kem_get_ct_len_bytes(void) { return CIPHERTEXTBYTES; }

int kem_keygen(unsigned char *pk, unsigned long long *pk_len_bytes,
               unsigned char *sk, unsigned long long *sk_len_bytes)
{
    int rc;
    if (pk == NULL || sk == NULL || pk_len_bytes == NULL || sk_len_bytes == NULL) {
        return KEM_ERR_INVALID_ARGUMENT;
    }
    rc = crypto_kem_keypair_Frost256(pk, sk);
    if (rc != 0) {
        api_clear(pk, PUBLICKEYBYTES);
        api_clear(sk, SECRETKEYBYTES);
        return KEM_ERR_INTERNAL;
    }
    *pk_len_bytes = PUBLICKEYBYTES;
    *sk_len_bytes = SECRETKEYBYTES;
    return 0;
}

int kem_enc(unsigned char *pk, unsigned long long pk_len_bytes,
            unsigned char *ss, unsigned long long *ss_len_bytes,
            unsigned char *ct, unsigned long long *ct_len_bytes)
{
    int rc;
    if (pk == NULL || ss == NULL || ss_len_bytes == NULL ||
        ct == NULL || ct_len_bytes == NULL || pk_len_bytes != PUBLICKEYBYTES) {
        return KEM_ERR_INVALID_ARGUMENT;
    }
    rc = crypto_kem_enc_Frost256(ct, ss, pk);
    if (rc != 0) {
        api_clear(ss, SHAREDSECRETBYTES);
        api_clear(ct, CIPHERTEXTBYTES);
        return KEM_ERR_INTERNAL;
    }
    *ss_len_bytes = SHAREDSECRETBYTES;
    *ct_len_bytes = CIPHERTEXTBYTES;
    return 0;
}

int kem_dec(unsigned char *sk, unsigned long long sk_len_bytes,
            unsigned char *ct, unsigned long long ct_len_bytes,
            unsigned char *ss, unsigned long long *ss_len_bytes)
{
    int rc;
    if (sk == NULL || ct == NULL || ss == NULL || ss_len_bytes == NULL ||
        sk_len_bytes != SECRETKEYBYTES || ct_len_bytes != CIPHERTEXTBYTES) {
        return KEM_ERR_INVALID_ARGUMENT;
    }
    rc = crypto_kem_dec_Frost256(ss, ct, sk);
    if (rc != 0) {
        api_clear(ss, SHAREDSECRETBYTES);
        return KEM_ERR_INTERNAL;
    }
    *ss_len_bytes = SHAREDSECRETBYTES;
    return 0;
}
