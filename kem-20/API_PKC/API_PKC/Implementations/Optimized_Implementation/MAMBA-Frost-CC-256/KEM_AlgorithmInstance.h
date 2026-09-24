/* Official API_PKC KEM interface for MAMBA-Frost-CC-256. */
#ifndef KEM_ALGORITHM_INSTANCE_H
#define KEM_ALGORITHM_INSTANCE_H

#define OUTPUT_BLANK_TEST_VECTORS 0
#define ALGORITHM_INSTANCE "MAMBA-Frost-CC-256"

/* Exact byte lengths from Frost-CC/src/api_frostcc256.h. */
#define PUBLICKEYBYTES    16776ULL
#define SECRETKEYBYTES    19416ULL
#define SHAREDSECRETBYTES   32ULL
#define CIPHERTEXTBYTES   15552ULL

#ifdef __cplusplus
extern "C" {
#endif

/** Return the exact public-key length in bytes. */
unsigned long long kem_get_pk_len_bytes(void);
/** Return the exact private-key length in bytes. */
unsigned long long kem_get_sk_len_bytes(void);
/** Return the exact shared-secret length in bytes. */
unsigned long long kem_get_ss_len_bytes(void);
/** Return the exact ciphertext length in bytes. */
unsigned long long kem_get_ct_len_bytes(void);

/**
 * Generate a Frost-CC key pair.
 * Outputs: pk and sk with lengths returned through pk_len_bytes/sk_len_bytes.
 * Returns 0 on success, -2 for invalid arguments, or -3 for internal failure.
 */
int kem_keygen(unsigned char *pk, unsigned long long *pk_len_bytes,
               unsigned char *sk, unsigned long long *sk_len_bytes);

/**
 * Encapsulate to pk.
 * Outputs: shared secret ss and ciphertext ct with their byte lengths.
 * Returns 0 on success, -2 for invalid arguments/lengths, or -3 internally.
 */
int kem_enc(unsigned char *pk, unsigned long long pk_len_bytes,
            unsigned char *ss, unsigned long long *ss_len_bytes,
            unsigned char *ct, unsigned long long *ct_len_bytes);

/**
 * Decapsulate ct using sk. Frost-CC's core CCA rejection/FO logic is preserved.
 * Returns 0 on success, -2 for invalid API arguments/lengths, or -3 internally.
 */
int kem_dec(unsigned char *sk, unsigned long long sk_len_bytes,
            unsigned char *ct, unsigned long long ct_len_bytes,
            unsigned char *ss, unsigned long long *ss_len_bytes);

#ifdef __cplusplus
}
#endif
#endif
