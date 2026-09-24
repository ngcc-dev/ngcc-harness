/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.
*/

#ifndef KEM_ALGORITHM_INSTANCE_H
#define KEM_ALGORITHM_INSTANCE_H

/* Generate populated test vectors with the unmodified official KAT driver. */
#define OUTPUT_BLANK_TEST_VECTORS 0

/* Official instance identifier: at most 64 permitted ASCII characters. */
#define ALGORITHM_INSTANCE "PolarKEM-128"

/* Stable API return codes used by all Polar-KEM instances. */
#define KEM_SUCCESS 0
#define KEM_ERR_CIPHERTEXT_INVALID (-1)
#define KEM_ERR_NULL_POINTER (-2)
#define KEM_ERR_LENGTH (-3)
#define KEM_ERR_PRIMITIVE (-4)

#ifdef __cplusplus
extern "C"
{
#endif

    /** Obtain the public-key length in bytes. */
    unsigned long long kem_get_pk_len_bytes(void);

    /** Obtain the private-key length in bytes. */
    unsigned long long kem_get_sk_len_bytes(void);

    /** Obtain the shared-secret length in bytes. */
    unsigned long long kem_get_ss_len_bytes(void);

    /** Obtain the ciphertext length in bytes. */
    unsigned long long kem_get_ct_len_bytes(void);

    /**
     * Generate a key pair using the official algorithm DRNG.
     *
     * @param[out] pk Public-key buffer of kem_get_pk_len_bytes() bytes.
     * @param[out] pk_len_bytes Resulting public-key length in bytes.
     * @param[out] sk Private-key buffer of kem_get_sk_len_bytes() bytes.
     * @param[out] sk_len_bytes Resulting private-key length in bytes.
     * @return 0 on success, -2 for a null pointer, or -4 on a primitive failure.
     */
    int kem_keygen(
        unsigned char *pk, unsigned long long *pk_len_bytes,
        unsigned char *sk, unsigned long long *sk_len_bytes);

    /**
     * Encapsulate to a public key using the official algorithm DRNG.
     *
     * @param[in] pk Public-key buffer.
     * @param[in] pk_len_bytes Public-key length in bytes.
     * @param[out] ss Shared-secret buffer of kem_get_ss_len_bytes() bytes.
     * @param[out] ss_len_bytes Resulting shared-secret length in bytes.
     * @param[out] ct Ciphertext buffer of kem_get_ct_len_bytes() bytes.
     * @param[out] ct_len_bytes Resulting ciphertext length in bytes.
     * @return 0 on success, -2 for a null pointer, -3 for an invalid input
     *         length or non-canonical public key, or -4 on a primitive failure.
     */
    int kem_enc(
        unsigned char *pk, unsigned long long pk_len_bytes,
        unsigned char *ss, unsigned long long *ss_len_bytes,
        unsigned char *ct, unsigned long long *ct_len_bytes);

    /**
     * Decapsulate a ciphertext, including implicit rejection.
     *
     * @param[in] sk Private-key buffer.
     * @param[in] sk_len_bytes Private-key length in bytes.
     * @param[in] ct Ciphertext buffer.
     * @param[in] ct_len_bytes Ciphertext length in bytes.
     * @param[out] ss Shared-secret buffer of kem_get_ss_len_bytes() bytes.
     * @param[out] ss_len_bytes Resulting shared-secret length in bytes.  This
     *             length is set on both valid decapsulation and rejection.
     * @return 0 on success, -1 when the ciphertext is rejected (with a
     *         rejection secret written), -2 for a null pointer, -3 for an
     *         invalid input length or non-canonical private key, or -4 on a
     *         primitive failure.
     */
    int kem_dec(
        unsigned char *sk, unsigned long long sk_len_bytes,
        unsigned char *ct, unsigned long long ct_len_bytes,
        unsigned char *ss, unsigned long long *ss_len_bytes);

#ifdef __cplusplus
}
#endif

#endif
