/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.
*/

/*
Copyright (c) 2026 Ying Liu.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
File Description: Declares the API_PKC KEM interface for the optimized POLARLAC-256 instance.
*/

#ifndef KEM_ALGORITHM_INSTANCE_H
#define KEM_ALGORITHM_INSTANCE_H

// Set "OUTPUT_BLANK_TEST_VECTORS" as 0 to generate test vector files
// Set "OUTPUT_BLANK_TEST_VECTORS" as 1 to generate blank template (default)
#define OUTPUT_BLANK_TEST_VECTORS 0

// Set "ALGORITHM_INSTANCE" as your algorithm instance name (no more than 64 bytes)
// Only letters, numbers, '-' or '_' are permitted
#define ALGORITHM_INSTANCE "POLARLAC-256"

#ifdef __cplusplus
extern "C"
{
#endif

	/// @brief Obtain the claimed byte length of the public key.
	/// @return Claimed byte length of the public key buffer consumed by
	///         `kem_enc()` and produced by `kem_keygen()`.
	unsigned long long kem_get_pk_len_bytes();

	/// @brief Obtain the claimed byte length of the private key.
	/// @return Claimed byte length of the private key buffer produced by
	///         `kem_keygen()`. In the current implementation this buffer stores
	///         the PKE secret key, a copy of the public key, and a 32-byte
	///         reject seed for constant-time decapsulation output selection.
	unsigned long long kem_get_sk_len_bytes();

	/// @brief Obtain the claimed byte length of the shared secret key.
	/// @return Claimed byte length of the encapsulated/decapsulated shared secret.
	unsigned long long kem_get_ss_len_bytes();

	/// @brief Obtain the claimed byte length of the ciphertext.
	/// @return Claimed byte length of the KEM ciphertext buffer.
	unsigned long long kem_get_ct_len_bytes();

	/// @brief Generate a Polar-LAC.KEM key pair.
	/// @param[out] pk Base address of the public key output buffer.
	/// @param[out] pk_len_bytes Byte length of the generated public key.
	/// @param[out] sk Base address of the private key output buffer.
	/// @param[out] sk_len_bytes Byte length of the generated private key.
	/// @return 0 on success; otherwise a self-defined negative (-1 to -99)
	///         error code.
	int kem_keygen(
		unsigned char *pk, unsigned long long *pk_len_bytes,
		unsigned char *sk, unsigned long long *sk_len_bytes);

	/// @brief Encapsulate a shared secret under a public key.
	/// @param[in] pk Base address of the public key input buffer.
	/// @param[in] pk_len_bytes Byte length of the public key input.
	/// @param[out] ss Base address of the shared secret output buffer.
	/// @param[out] ss_len_bytes Byte length of the generated shared secret.
	/// @param[out] ct Base address of the ciphertext output buffer.
	/// @param[out] ct_len_bytes Byte length of the generated ciphertext.
	/// @return 0 on success; otherwise a self-defined negative (-1 to -99)
	///         error code.
	int kem_enc(
		unsigned char *pk, unsigned long long pk_len_bytes,
		unsigned char *ss, unsigned long long *ss_len_bytes,
		unsigned char *ct, unsigned long long *ct_len_bytes);

	/// @brief Decapsulate a shared secret from a ciphertext.
	/// @param[in] sk Base address of the private key input buffer.
	/// @param[in] sk_len_bytes Byte length of the private key input.
	/// @param[in] ct Base address of the ciphertext input buffer.
	/// @param[in] ct_len_bytes Byte length of the ciphertext input.
	/// @param[out] ss Base address of the shared secret output buffer.
	/// @param[out] ss_len_bytes Byte length of the output shared secret.
	/// @return 0 if the routine executes successfully. On ciphertext verification
	///         failure, this implementation follows the revised FO transform and
	///         outputs a fallback pseudorandom shared key derived from the PKE
	///         secret key and ciphertext, rather than returning -1. Other
	///         internal failures return a self-defined negative (-1 to -99)
	///         error code.
	int kem_dec(
		unsigned char *sk, unsigned long long sk_len_bytes,
		unsigned char *ct, unsigned long long ct_len_bytes,
		unsigned char *ss, unsigned long long *ss_len_bytes);

#ifdef __cplusplus
}
#endif
#endif
