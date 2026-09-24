/**
 * \file KEM_CMultiURAG-256.h
 * \brief API_PKC KEM programming interface header for the CMULTIURAG KEM scheme
 */

#ifndef KEM_ALGORITHM_INSTANCE_H
#define KEM_ALGORITHM_INSTANCE_H

#define OUTPUT_BLANK_TEST_VECTORS 0

#define ALGORITHM_INSTANCE "CMultiURAG-256"

#ifdef __cplusplus
extern "C" {
#endif

unsigned long long kem_get_pk_len_bytes();
unsigned long long kem_get_sk_len_bytes();
unsigned long long kem_get_ss_len_bytes();
unsigned long long kem_get_ct_len_bytes();

int kem_keygen(unsigned char *pk, unsigned long long *pk_len_bytes,
               unsigned char *sk, unsigned long long *sk_len_bytes);

int kem_enc(unsigned char *pk, unsigned long long pk_len_bytes,
            unsigned char *ss, unsigned long long *ss_len_bytes,
            unsigned char *ct, unsigned long long *ct_len_bytes);

int kem_dec(unsigned char *sk, unsigned long long sk_len_bytes,
            unsigned char *ct, unsigned long long ct_len_bytes,
            unsigned char *ss, unsigned long long *ss_len_bytes);

#ifdef __cplusplus
}
#endif
#endif
