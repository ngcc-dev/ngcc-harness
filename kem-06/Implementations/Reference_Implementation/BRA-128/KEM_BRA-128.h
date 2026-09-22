/**
 * \file KEM_BRA-128.h
 * \brief API_PKC KEM programming interface header for the BRA KEM scheme
 *
 * Implements the Key Encapsulation Mechanism (KEM) programming interface
 * defined by ICCS for the Next-Generation Commercial Cryptography (NGCC)
 * algorithm solicitation.
 */

#ifndef KEM_ALGORITHM_INSTANCE_H
#define KEM_ALGORITHM_INSTANCE_H

// Set to 0 to generate test vector files; set to 1 to generate blank templates
#define OUTPUT_BLANK_TEST_VECTORS 0

// Algorithm instance name (automatically replaced by the packaging script)
#define ALGORITHM_INSTANCE "BRA-128"

#ifdef __cplusplus
extern "C" {
#endif

/// @brief Get the public key length in bytes
/// @return Public key length in bytes
unsigned long long kem_get_pk_len_bytes();

/// @brief Get the secret key length in bytes
/// @return Secret key length in bytes
unsigned long long kem_get_sk_len_bytes();

/// @brief Get the shared secret length in bytes
/// @return Shared secret length in bytes
unsigned long long kem_get_ss_len_bytes();

/// @brief Get the ciphertext length in bytes
/// @return Ciphertext length in bytes
unsigned long long kem_get_ct_len_bytes();

/// @brief Key generation
/// @param[out] pk Public key
/// @param[out] pk_len_bytes Public key length in bytes
/// @param[out] sk Secret key
/// @param[out] sk_len_bytes Secret key length in bytes
/// @return 0 on success; negative error code (-1 to -99) on failure
int kem_keygen(unsigned char *pk, unsigned long long *pk_len_bytes,
               unsigned char *sk, unsigned long long *sk_len_bytes);

/// @brief Encapsulation (generate shared secret and ciphertext)
/// @param[in] pk Public key
/// @param[in] pk_len_bytes Public key length in bytes
/// @param[out] ss Shared secret
/// @param[out] ss_len_bytes Shared secret length in bytes
/// @param[out] ct Ciphertext
/// @param[out] ct_len_bytes Ciphertext length in bytes
/// @return 0 on success; negative error code (-1 to -99) on failure
int kem_enc(unsigned char *pk, unsigned long long pk_len_bytes,
            unsigned char *ss, unsigned long long *ss_len_bytes,
            unsigned char *ct, unsigned long long *ct_len_bytes);

/// @brief Decapsulation (recover shared secret from ciphertext)
/// @param[in] sk Secret key
/// @param[in] sk_len_bytes Secret key length in bytes
/// @param[in] ct Ciphertext
/// @param[in] ct_len_bytes Ciphertext length in bytes
/// @param[out] ss Shared secret
/// @param[out] ss_len_bytes Shared secret length in bytes
/// @return 0 on success; -1 on decapsulation failure; -2 to -99 for other errors
int kem_dec(unsigned char *sk, unsigned long long sk_len_bytes,
            unsigned char *ct, unsigned long long ct_len_bytes,
            unsigned char *ss, unsigned long long *ss_len_bytes);

#ifdef __cplusplus
}
#endif
#endif
