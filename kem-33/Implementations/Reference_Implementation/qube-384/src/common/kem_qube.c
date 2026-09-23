/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).
*/

#include "kem_qube.h"

#include <stddef.h>

#include "api.h"

unsigned long long kem_get_pk_len_bytes(void) {
    return CRYPTO_PUBLICKEYBYTES;
}

unsigned long long kem_get_sk_len_bytes(void) {
    return CRYPTO_SECRETKEYBYTES;
}

unsigned long long kem_get_ss_len_bytes(void) {
    return CRYPTO_BYTES;
}

unsigned long long kem_get_ct_len_bytes(void) {
    return CRYPTO_CIPHERTEXTBYTES;
}

int kem_keygen(unsigned char *pk, unsigned long long *pk_len_bytes,
               unsigned char *sk, unsigned long long *sk_len_bytes) {
    int ret;

    if (pk == NULL || sk == NULL || pk_len_bytes == NULL || sk_len_bytes == NULL) {
        return -1;
    }

    ret = crypto_kem_keypair(pk, sk);
    if (ret != 0) {
        *pk_len_bytes = 0;
        *sk_len_bytes = 0;
        return ret;
    }

    *pk_len_bytes = kem_get_pk_len_bytes();
    *sk_len_bytes = kem_get_sk_len_bytes();
    return 0;
}

int kem_enc(unsigned char *pk, unsigned long long pk_len_bytes,
            unsigned char *ss, unsigned long long *ss_len_bytes,
            unsigned char *ct, unsigned long long *ct_len_bytes) {
    int ret;

    if (pk == NULL || ss == NULL || ct == NULL || ss_len_bytes == NULL || ct_len_bytes == NULL) {
        return -1;
    }
    if (pk_len_bytes != kem_get_pk_len_bytes()) {
        *ss_len_bytes = 0;
        *ct_len_bytes = 0;
        return -2;
    }

    ret = crypto_kem_enc(ct, ss, pk);
    if (ret != 0) {
        *ss_len_bytes = 0;
        *ct_len_bytes = 0;
        return ret;
    }

    *ss_len_bytes = kem_get_ss_len_bytes();
    *ct_len_bytes = kem_get_ct_len_bytes();
    return 0;
}

int kem_dec(unsigned char *sk, unsigned long long sk_len_bytes,
            unsigned char *ct, unsigned long long ct_len_bytes,
            unsigned char *ss, unsigned long long *ss_len_bytes) {
    int ret;

    if (sk == NULL || ct == NULL || ss == NULL || ss_len_bytes == NULL) {
        return -1;
    }
    if (sk_len_bytes != kem_get_sk_len_bytes() || ct_len_bytes != kem_get_ct_len_bytes()) {
        *ss_len_bytes = 0;
        return -2;
    }

    ret = crypto_kem_dec(ss, ct, sk);
    if (ret != 0) {
        *ss_len_bytes = 0;
        return ret;
    }

    *ss_len_bytes = kem_get_ss_len_bytes();
    return 0;
}
