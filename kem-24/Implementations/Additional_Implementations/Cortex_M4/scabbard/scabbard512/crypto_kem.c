#include "api.h"
#include "KEM_scabbard512.h"

int crypto_kem_keypair(unsigned char *pk, unsigned char *sk)
{
    unsigned long long pk_len_bytes;
    unsigned long long sk_len_bytes;

    return kem_keygen(pk, &pk_len_bytes, sk, &sk_len_bytes);
}

int crypto_kem_enc(unsigned char *ct, unsigned char *ss, const unsigned char *pk)
{
    unsigned long long ss_len_bytes;
    unsigned long long ct_len_bytes;

    return kem_enc((unsigned char *)pk,
                   CRYPTO_PUBLICKEYBYTES,
                   ss,
                   &ss_len_bytes,
                   ct,
                   &ct_len_bytes);
}

int crypto_kem_dec(unsigned char *ss, const unsigned char *ct, const unsigned char *sk)
{
    unsigned long long ss_len_bytes;

    return kem_dec((unsigned char *)sk,
                   CRYPTO_SECRETKEYBYTES,
                   (unsigned char *)ct,
                   CRYPTO_CIPHERTEXTBYTES,
                   ss,
                   &ss_len_bytes);
}
