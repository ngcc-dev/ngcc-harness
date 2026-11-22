/* ICCS signature API over the pq-crystals ML-DSA reference implementation
 * (FIPS 204; upstream/dilithium/ref, parameter set chosen with
 * -DDILITHIUM_MODE=2|3|5). Pure ML-DSA with an empty context string and the
 * upstream default hedged signing (config.h: DILITHIUM_RANDOMIZED_SIGNING), so
 * each signature draws 32 bytes from the DRNG. The ICCS signature is detached. */
#include <stddef.h>
#include "params.h"
#include "sign.h"
#include "iccs_api.h"

unsigned long long sig_get_pk_len_bytes(void) { return CRYPTO_PUBLICKEYBYTES; }
unsigned long long sig_get_sk_len_bytes(void) { return CRYPTO_SECRETKEYBYTES; }
unsigned long long sig_get_sn_len_bytes(void) { return CRYPTO_BYTES; }

int sig_keygen(unsigned char *pk, unsigned long long *pk_len_bytes,
               unsigned char *sk, unsigned long long *sk_len_bytes)
{
    if (crypto_sign_keypair(pk, sk))
        return -2;
    *pk_len_bytes = CRYPTO_PUBLICKEYBYTES;
    *sk_len_bytes = CRYPTO_SECRETKEYBYTES;
    return 0;
}

int sig_sign(unsigned char *sk, unsigned long long sk_len_bytes,
             unsigned char *m, unsigned long long m_len_bytes,
             unsigned char *sn, unsigned long long *sn_len_bytes)
{
    size_t len = 0;
    if (sk_len_bytes != CRYPTO_SECRETKEYBYTES)
        return -3;
    if (crypto_sign_signature(sn, &len, m, (size_t)m_len_bytes, NULL, 0, sk))
        return -2;
    *sn_len_bytes = len;
    return 0;
}

int sig_verify(unsigned char *pk, unsigned long long pk_len_bytes,
               unsigned char *sn, unsigned long long sn_len_bytes,
               unsigned char *m, unsigned long long m_len_bytes)
{
    if (pk_len_bytes != CRYPTO_PUBLICKEYBYTES)
        return -3;
    return crypto_sign_verify(sn, (size_t)sn_len_bytes, m, (size_t)m_len_bytes, NULL, 0, pk) ? -1 : 0;
}
