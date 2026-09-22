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

#include "params.h"

// Set "OUTPUT_BLANK_TEST_VECTORS" as 0 to generate test vector files
// Set "OUTPUT_BLANK_TEST_VECTORS" as 1 to generate blank template (default)
#define OUTPUT_BLANK_TEST_VECTORS 0

// Set "ALGORITHM_INSTANCE" as your algorithm instance name (no more than 64 bytes)
// Only letters, numbers, '-' or '_' are permitted

#define ALGORITHM_INSTANCE "UVW_KEM_128"

// 私钥种子长度（256比特）
#define PRIVATE_KEY_SEED_BYTES (UVW_M / 8)

// 定义公钥结构体：只存储系统形式 [I_k | T] 中的 T 部分
typedef struct
{
    // 行数: UVW_K, 列数: UVW_N - UVW_K
    gf_elem_t T[UVW_K * (UVW_N - UVW_K)];
} public_key_t;

typedef struct
{
	gf_elem_t A[UVW_K * UVW_K];
	gf_elem_t G_U[UVW_K1 * (UVW_N / 2)];
	gf_elem_t G_W[UVW_K2 * (UVW_N / 2)];
	gf_elem_t G_RS[UVW_K2 * (UVW_N / 2)];

	// D 的置换部分 (记录列的移动)
	uint16_t D_perm[UVW_N];
	// D 的对角部分 (记录每列乘的系数，必须非零)
    gf_elem_t D_coeffs[UVW_N];

} private_key_t;

typedef struct
{
	gf_elem_t A[UVW_K * UVW_K];
	gf_elem_t G_U[UVW_K1 * (UVW_N / 2)];
	gf_elem_t G_W[UVW_K2 * (UVW_N / 2)];
	gf_elem_t G_RS[UVW_K2 * (UVW_N / 2)];

	// D 的置换部分 (记录列的移动)
	uint16_t D_perm[UVW_N];
	// D 的对角部分 (记录每列乘的系数，必须非零)
    gf_elem_t D_coeffs[UVW_N];

} private_key_expand_t;

// 定义密文结构体
typedef struct
{
    gf_elem_t c1[UVW_N];       
    unsigned char c2[UVW_M / 8]; 
} ciphertext_t;

typedef struct
{
    gf_elem_t c1[UVW_N];       
    unsigned char c2[UVW_M / 8]; 
	unsigned char d[UVW_M / 8];
} kem_ciphertext_t;


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

	/// @brief Obtain the claimed byte length of the shared secret key (encapsulated key)
	/// @return Claimed byte length of the shared secret key
	unsigned long long kem_get_ss_len_bytes();

	/// @brief Obtain the claimed byte length of the ciphertext
	/// @return Claimed byte length of the ciphertext
	unsigned long long kem_get_ct_len_bytes();

	/// @brief Key generate
	/// @param[out] pk Public key
	/// @param[out] pk_len_bytes Byte length of the public key
	/// @param[out] sk Private key
	/// @param[out] sk_len_bytes Byte length of the private key
	/// @return If run successfully, return 0; otherwise, return a self-defined negative (-1 to -99) error code
	int kem_keygen(
		unsigned char *pk, unsigned long long *pk_len_bytes,
		unsigned char *sk, unsigned long long *sk_len_bytes);

	/// @brief Encapsulate
	/// @param[in] pk Public key
	/// @param[in] pk_len_bytes Byte length of the public key
	/// @param[out] ss Shared secret key
	/// @param[out] ss_len_bytes Byte length of the shared secret key
	/// @param[out] ct Ciphertext
	/// @param[out] ct_len_bytes Byte length of the ciphertext
	/// @return If run successfully, return 0; otherwise, return a self-defined negative (-1 to -99) error code
	int kem_enc(
		unsigned char *pk, unsigned long long pk_len_bytes,
		unsigned char *ss, unsigned long long *ss_len_bytes,
		unsigned char *ct, unsigned long long *ct_len_bytes);

	/// @brief Decapsulate
	/// @param[in] sk Private key
	/// @param[in] sk_len_bytes Byte length of the private key
	/// @param[in] ct Ciphertext
	/// @param[in] ct_len_bytes Byte length of the ciphertext
	/// @param[out] ss Shared secret key
	/// @param[out] ss_len_bytes Byte length of the shared secret key
	/// @return If decapsulation successfully, return 0; if decapsulation unsuccessfully, return -1; otherwise, return a self-defined negative (-2 to -99) error code
	int kem_dec(
		unsigned char *sk, unsigned long long sk_len_bytes,
		unsigned char *ct, unsigned long long ct_len_bytes,
		unsigned char *ss, unsigned long long *ss_len_bytes);

#ifdef __cplusplus
}
#endif
#endif