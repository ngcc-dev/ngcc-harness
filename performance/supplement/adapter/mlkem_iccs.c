/* ICCS KEM API over the pq-crystals ML-KEM reference implementation
 * (FIPS 203; upstream/kyber/ref, parameter set chosen with -DKYBER_K=2|3|4).
 * Decapsulation uses implicit rejection and therefore always returns 0. */
#include <string.h>
#include "kem.h"
#include "iccs_api.h"

unsigned long long kem_get_pk_len_bytes(void) { return CRYPTO_PUBLICKEYBYTES; }
unsigned long long kem_get_sk_len_bytes(void) { return CRYPTO_SECRETKEYBYTES; }
unsigned long long kem_get_ss_len_bytes(void) { return CRYPTO_BYTES; }
unsigned long long kem_get_ct_len_bytes(void) { return CRYPTO_CIPHERTEXTBYTES; }

int kem_keygen(unsigned char *pk, unsigned long long *pk_len_bytes,
               unsigned char *sk, unsigned long long *sk_len_bytes)
{
    if (crypto_kem_keypair(pk, sk))
        return -2;
    *pk_len_bytes = CRYPTO_PUBLICKEYBYTES;
    *sk_len_bytes = CRYPTO_SECRETKEYBYTES;
    return 0;
}

int kem_enc(unsigned char *pk, unsigned long long pk_len_bytes,
            unsigned char *ss, unsigned long long *ss_len_bytes,
            unsigned char *ct, unsigned long long *ct_len_bytes)
{
    if (pk_len_bytes != CRYPTO_PUBLICKEYBYTES)
        return -3;
    if (crypto_kem_enc(ct, ss, pk))
        return -2;
    *ss_len_bytes = CRYPTO_BYTES;
    *ct_len_bytes = CRYPTO_CIPHERTEXTBYTES;
    return 0;
}

int kem_dec(unsigned char *sk, unsigned long long sk_len_bytes,
            unsigned char *ct, unsigned long long ct_len_bytes,
            unsigned char *ss, unsigned long long *ss_len_bytes)
{
    if (sk_len_bytes != CRYPTO_SECRETKEYBYTES || ct_len_bytes != CRYPTO_CIPHERTEXTBYTES)
        return -3;
    if (crypto_kem_dec(ss, ct, sk))
        return -2;
    *ss_len_bytes = CRYPTO_BYTES;
    return 0;
}
