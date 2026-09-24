/*
 * Official NGCC KEM interface for MAMBA-Frost-512.
 *
 * This header preserves the function names and signatures required by the
 * API_PKC KEM test-vector generator.
 */
#ifndef KEM_ALGORITHM_INSTANCE_H
#define KEM_ALGORITHM_INSTANCE_H

/* Generate populated test vectors rather than the blank submission template. */
#define OUTPUT_BLANK_TEST_VECTORS 0
#define ALGORITHM_INSTANCE "MAMBA-Frost-512"

/* Exact byte lengths from Frost/src/api_frost512.h. */
#define PUBLICKEYBYTES    36432ULL
#define SECRETKEYBYTES    41728ULL
#define SHAREDSECRETBYTES   64ULL
#define CIPHERTEXTBYTES   72944ULL

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
 * Generate a key pair. Output buffers must have the lengths returned by the
 * corresponding getter. Returns 0 on success, -2 for invalid arguments, or
 * -3 for an internal algorithm/RNG failure.
 */
int kem_keygen(unsigned char *pk, unsigned long long *pk_len_bytes,
               unsigned char *sk, unsigned long long *sk_len_bytes);

/**
 * Encapsulate to pk. Returns 0 on success, -2 for invalid arguments or input
 * length, or -3 for an internal algorithm/RNG failure.
 */
int kem_enc(unsigned char *pk, unsigned long long pk_len_bytes,
            unsigned char *ss, unsigned long long *ss_len_bytes,
            unsigned char *ct, unsigned long long *ct_len_bytes);

/**
 * Decapsulate ct using sk. Invalid ciphertexts are handled by Frost's
 * constant-time implicit rejection and therefore still return 0 with the
 * pseudorandom fallback secret. Returns -2 for invalid API arguments/lengths
 * or -3 for an internal allocation failure.
 */
int kem_dec(unsigned char *sk, unsigned long long sk_len_bytes,
            unsigned char *ct, unsigned long long ct_len_bytes,
            unsigned char *ss, unsigned long long *ss_len_bytes);

#ifdef __cplusplus
}
#endif
#endif
