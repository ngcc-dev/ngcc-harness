#ifndef KEM_ALGORITHM_INSTANCE_H
#define KEM_ALGORITHM_INSTANCE_H

#include "params.h"

/* 
 * Set as 0 to generate real test vectors.
 * Set as 1 only when you intentionally want blank templates.
 */
#define OUTPUT_BLANK_TEST_VECTORS 0

/*
 * Submitted algorithm instance name.
 * Only letters, numbers, '-' or '_' are permitted by the KAT driver.
 * If needed, you can replace this with a fixed literal such as "Lore_128".
 */
#define ALGORITHM_INSTANCE LORE_INSTANCE_NAME

#ifdef __cplusplus
extern "C"
{
#endif

    /* Obtain the claimed byte length of the public key */
    unsigned long long kem_get_pk_len_bytes(void);

    /* Obtain the claimed byte length of the private key */
    unsigned long long kem_get_sk_len_bytes(void);

    /* Obtain the claimed byte length of the shared secret key */
    unsigned long long kem_get_ss_len_bytes(void);

    /* Obtain the claimed byte length of the ciphertext */
    unsigned long long kem_get_ct_len_bytes(void);

    /*
     * Key generation
     * Return 0 on success; otherwise return a self-defined negative error code.
     */
    int kem_keygen(
        unsigned char *pk, unsigned long long *pk_len_bytes,
        unsigned char *sk, unsigned long long *sk_len_bytes);

    /*
     * Encapsulation
     * Return 0 on success; otherwise return a self-defined negative error code.
     */
    int kem_enc(
        unsigned char *pk, unsigned long long pk_len_bytes,
        unsigned char *ss, unsigned long long *ss_len_bytes,
        unsigned char *ct, unsigned long long *ct_len_bytes);

    /*
     * Decapsulation
     * Return 0 on success.
     * If decapsulation semantically fails, return -1.
     * For other internal errors, return a self-defined negative error code <= -2.
     */
    int kem_dec(
        unsigned char *sk, unsigned long long sk_len_bytes,
        unsigned char *ct, unsigned long long ct_len_bytes,
        unsigned char *ss, unsigned long long *ss_len_bytes);

#ifdef __cplusplus
}
#endif

#endif