/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.
*/

#ifndef KEM_WEAVERKEM_128_H
#define KEM_WEAVERKEM_128_H

/* Set OUTPUT_BLANK_TEST_VECTORS to 0 to generate real test vectors */
#define OUTPUT_BLANK_TEST_VECTORS 0

/* Algorithm instance name */
#define ALGORITHM_INSTANCE "WeaverKEM-128"

#ifdef __cplusplus
extern "C"
{
#endif

	/// @brief Obtain the claimed byte length of the public key
	/// @return Claimed byte length of the public key
	unsigned long long kem_get_pk_len_bytes();

	/// @brief Obtain the claimed byte length of the private key
	/// @return Claimed byte length of the private key
	unsigned long long kem_get_sk_len_bytes();

	/// @brief Obtain the claimed byte length of the shared secret key
	/// @return Claimed byte length of the shared secret key
	unsigned long long kem_get_ss_len_bytes();

	/// @brief Obtain the claimed byte length of the ciphertext
	/// @return Claimed byte length of the ciphertext
	unsigned long long kem_get_ct_len_bytes();

	/// @brief Key generation
	/// @param[out] pk Public key
	/// @param[out] pk_len_bytes Byte length of the public key
	/// @param[out] sk Private key
	/// @param[out] sk_len_bytes Byte length of the private key
	/// @return 0 on success; negative error code otherwise
	int kem_keygen(
		unsigned char *pk, unsigned long long *pk_len_bytes,
		unsigned char *sk, unsigned long long *sk_len_bytes);

	/// @brief Encapsulate
	/// @param[in]  pk            Public key
	/// @param[in]  pk_len_bytes  Byte length of the public key
	/// @param[out] ss            Shared secret key
	/// @param[out] ss_len_bytes  Byte length of the shared secret key
	/// @param[out] ct            Ciphertext
	/// @param[out] ct_len_bytes  Byte length of the ciphertext
	/// @return 0 on success; negative error code otherwise
	int kem_enc(
		unsigned char *pk, unsigned long long pk_len_bytes,
		unsigned char *ss, unsigned long long *ss_len_bytes,
		unsigned char *ct, unsigned long long *ct_len_bytes);

	/// @brief Decapsulate
	/// @param[in]  sk            Private key
	/// @param[in]  sk_len_bytes  Byte length of the private key
	/// @param[in]  ct            Ciphertext
	/// @param[in]  ct_len_bytes  Byte length of the ciphertext
	/// @param[out] ss            Shared secret key
	/// @param[out] ss_len_bytes  Byte length of the shared secret key
	/// @return 0 on success; -1 on decapsulation failure; other negative on error
	int kem_dec(
		unsigned char *sk, unsigned long long sk_len_bytes,
		unsigned char *ct, unsigned long long ct_len_bytes,
		unsigned char *ss, unsigned long long *ss_len_bytes);

#ifdef __cplusplus
}
#endif
#endif /* KEM_WEAVERKEM_128_H */
