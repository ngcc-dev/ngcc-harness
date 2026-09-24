/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.
*/

#ifndef API_H
#define API_H

// Scheme Specific Setup
#include "params.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define kem_keygen KEM_NAMESPACE (kem_keygen)
	/// @brief Key generate
	/// @param[out] pk Public key
	/// @param[out] pk_len_bytes Byte length of the public key
	/// @param[out] sk Private key
	/// @param[out] sk_len_bytes Byte length of the private key
	/// @return If run successfully, return 0; otherwise, return a self-defined
	/// negative (-1 to -99) error code
	int kem_keygen (unsigned char *pk, unsigned long long *pk_len_bytes,
									unsigned char *sk, unsigned long long *sk_len_bytes);

#define kem_enc KEM_NAMESPACE (kem_enc)
	/// @brief Encapsulate
	/// @param[in] pk Public key
	/// @param[in] pk_len_bytes Byte length of the public key
	/// @param[out] ss Shared secret key
	/// @param[out] ss_len_bytes Byte length of the shared secret key
	/// @param[out] ct Ciphertext
	/// @param[out] ct_len_bytes Byte length of the ciphertext
	/// @return If run successfully, return 0; otherwise, return a self-defined
	/// negative (-1 to -99) error code
	int kem_enc (unsigned char *pk, unsigned long long pk_len_bytes,
							 unsigned char *ss, unsigned long long *ss_len_bytes,
							 unsigned char *ct, unsigned long long *ct_len_bytes);

#define kem_dec KEM_NAMESPACE (kem_dec)
	/// @brief Decapsulate
	/// @param[in] sk Private key
	/// @param[in] sk_len_bytes Byte length of the private key
	/// @param[in] ct Ciphertext
	/// @param[in] ct_len_bytes Byte length of the ciphertext
	/// @param[out] ss Shared secret key
	/// @param[out] ss_len_bytes Byte length of the shared secret key
	/// @return If decapsulation successfully, return 0; if decapsulation
	/// unsuccessfully, return -1; otherwise, return a self-defined negative (-2
	/// to -99) error code
	int kem_dec (unsigned char *sk, unsigned long long sk_len_bytes,
							 unsigned char *ct, unsigned long long ct_len_bytes,
							 unsigned char *ss, unsigned long long *ss_len_bytes);

#ifdef __cplusplus
}
#endif
#endif
