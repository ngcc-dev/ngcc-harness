/*
 * HARE KEM API_PKC adapter for HARE-512-kr.
 *
 * This file exposes the submission harness entry points used by KAT replay,
 * CTest, and self-evaluation. It delegates all cryptographic operations to
 * the instance-local API_PKC-compatible crypto_kem_* functions and performs
 * only length/null validation around those calls.
 */

#include "KEM_AlgorithmInstance.h"
#include "api.h"

/**
 * Return the public-key length in bytes for this HARE instance.
 */
unsigned long long kem_get_pk_len_bytes(void) {
    return CRYPTO_PUBLICKEYBYTES;
}

/**
 * Return the decapsulation-key length in bytes for this HARE instance.
 */
unsigned long long kem_get_sk_len_bytes(void) {
    return CRYPTO_SECRETKEYBYTES;
}

/**
 * Return the shared-secret length in bytes for this HARE instance.
 */
unsigned long long kem_get_ss_len_bytes(void) {
    return CRYPTO_BYTES;
}

/**
 * Return the ciphertext length in bytes for this HARE instance.
 */
unsigned long long kem_get_ct_len_bytes(void) {
    return CRYPTO_CIPHERTEXTBYTES;
}

/**
 * Generate one HARE KEM keypair and report the fixed output lengths.
 */
int kem_keygen(unsigned char *pk, unsigned long long *pk_len,
               unsigned char *sk, unsigned long long *sk_len) {
    if (!pk || !sk) {
        return -2;
    }
    if (pk_len) {
        *pk_len = kem_get_pk_len_bytes();
    }
    if (sk_len) {
        *sk_len = kem_get_sk_len_bytes();
    }
    return crypto_kem_keypair(pk, sk);
}

/**
 * Encapsulate to a public key after validating the supplied public-key length.
 */
int kem_enc(unsigned char *pk, unsigned long long pk_len,
            unsigned char *ss, unsigned long long *ss_len,
            unsigned char *ct, unsigned long long *ct_len) {
    if (!pk || !ss || !ct) {
        return -2;
    }
    if (pk_len != kem_get_pk_len_bytes()) {
        return -2;
    }
    if (ss_len) {
        *ss_len = kem_get_ss_len_bytes();
    }
    if (ct_len) {
        *ct_len = kem_get_ct_len_bytes();
    }
    return crypto_kem_enc(ct, ss, pk);
}

/**
 * Decapsulate a ciphertext after validating fixed key and ciphertext lengths.
 *
 * The shared HARE decapsulation path always writes either the recovered key or
 * the FO implicit-rejection key. This API_PKC adapter returns 0 for ciphertexts
 * that verify and -1 for ciphertexts that trigger implicit rejection.
 */
int kem_dec(unsigned char *sk, unsigned long long sk_len,
            unsigned char *ct, unsigned long long ct_len,
            unsigned char *ss, unsigned long long *ss_len) {
    if (!sk || !ct || !ss) {
        return -2;
    }
    if (sk_len != kem_get_sk_len_bytes() || ct_len != kem_get_ct_len_bytes()) {
        return -2;
    }
    if (ss_len) {
        *ss_len = kem_get_ss_len_bytes();
    }
    return crypto_kem_dec_status(ss, ct, sk);
}
