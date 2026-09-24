/**
 * @file KEM_AlgorithmInstance.h
 * @brief Shared API_PKC declarations exported by each Scloud+ KEM selection.
 */
#ifndef SCLOUDPLUS_KEM_ALGORITHM_INSTANCE_H
#define SCLOUDPLUS_KEM_ALGORITHM_INSTANCE_H

#include "scloudplus_param_common.h"

#define OUTPUT_BLANK_TEST_VECTORS 0
#define ALGORITHM_INSTANCE SYSTEM_NAME

#ifdef __cplusplus
extern "C" {
#endif

/** Return the configured public-key length in bytes. */
unsigned long long kem_get_pk_len_bytes(void);

/** Return the configured secret-key length in bytes. */
unsigned long long kem_get_sk_len_bytes(void);

/** Return the configured shared-secret length in bytes. */
unsigned long long kem_get_ss_len_bytes(void);

/** Return the configured ciphertext length in bytes. */
unsigned long long kem_get_ct_len_bytes(void);

/** Generate a keypair through the API_PKC-facing wrapper. */
int kem_keygen(unsigned char *pk, unsigned long long *pk_len_bytes,
               unsigned char *sk, unsigned long long *sk_len_bytes);

/** Encapsulate through the API_PKC-facing wrapper. */
int kem_enc(const unsigned char *pk, unsigned long long pk_len_bytes,
            unsigned char *ss, unsigned long long *ss_len_bytes,
            unsigned char *ct, unsigned long long *ct_len_bytes);

/** Decapsulate through the API_PKC-facing wrapper. */
int kem_dec(const unsigned char *sk, unsigned long long sk_len_bytes,
            const unsigned char *ct, unsigned long long ct_len_bytes,
            unsigned char *ss, unsigned long long *ss_len_bytes);

#ifdef __cplusplus
}
#endif

#endif /* SCLOUDPLUS_KEM_ALGORITHM_INSTANCE_H */
