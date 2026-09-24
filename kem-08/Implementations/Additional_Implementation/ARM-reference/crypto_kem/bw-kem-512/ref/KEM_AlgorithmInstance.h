// SPDX-License-Identifier: Apache-2.0 or CC0-1.0
#ifndef KEM_ALGORITHM_INSTANCE_H
#define KEM_ALGORITHM_INSTANCE_H

/* ICCS KEM API wrapper for the BW_KEM_C512 pqm4 implementation. */

/* Set to 0 for real KAT generation; 1 only for a blank template. */
#define OUTPUT_BLANK_TEST_VECTORS 0

/* Only letters, numbers, '-' or '_' are permitted. */
#define ALGORITHM_INSTANCE "BW_KEM_C512"

#ifdef __cplusplus
extern "C"
{
#endif

unsigned long long kem_get_pk_len_bytes(void);
unsigned long long kem_get_sk_len_bytes(void);
unsigned long long kem_get_ss_len_bytes(void);
unsigned long long kem_get_ct_len_bytes(void);

int kem_keygen(
    unsigned char *pk, unsigned long long *pk_len_bytes,
    unsigned char *sk, unsigned long long *sk_len_bytes);

int kem_enc(
    unsigned char *pk, unsigned long long pk_len_bytes,
    unsigned char *ss, unsigned long long *ss_len_bytes,
    unsigned char *ct, unsigned long long *ct_len_bytes);

int kem_dec(
    unsigned char *sk, unsigned long long sk_len_bytes,
    unsigned char *ct, unsigned long long ct_len_bytes,
    unsigned char *ss, unsigned long long *ss_len_bytes);

#ifdef __cplusplus
}
#endif

#endif
