/*
 * HARE KEM self-evaluation adapter declarations for HARE-128-kr.
 */
#ifndef HARE_KEM_ALGORITHM_INSTANCE_H
#define HARE_KEM_ALGORITHM_INSTANCE_H

#define ALGORITHM_INSTANCE "HARE-128-kr"
#define OUTPUT_BLANK_TEST_VECTORS 0

/** Return the public-key length in bytes for this algorithm instance. */
unsigned long long kem_get_pk_len_bytes(void);

/** Return the secret-key length in bytes for this algorithm instance. */
unsigned long long kem_get_sk_len_bytes(void);

/** Return the shared-secret length in bytes for this algorithm instance. */
unsigned long long kem_get_ss_len_bytes(void);

/** Return the ciphertext length in bytes for this algorithm instance. */
unsigned long long kem_get_ct_len_bytes(void);

/** Generate a keypair and optionally report output lengths. */
int kem_keygen(unsigned char *pk, unsigned long long *pk_len,
               unsigned char *sk, unsigned long long *sk_len);

/** Encapsulate to a fixed-length public key and optionally report output lengths. */
int kem_enc(unsigned char *pk, unsigned long long pk_len,
            unsigned char *ss, unsigned long long *ss_len,
            unsigned char *ct, unsigned long long *ct_len);

/** Decapsulate a fixed-length ciphertext and optionally report output length. */
int kem_dec(unsigned char *sk, unsigned long long sk_len,
            unsigned char *ct, unsigned long long ct_len,
            unsigned char *ss, unsigned long long *ss_len);

#endif /* HARE_KEM_ALGORITHM_INSTANCE_H */
