#include "KEM_AlgorithmInstance.h"
#include "loong_kem.h"
#include "loong_parameters.h"
#include "loong_status.h"

#include <limits.h>
#include <stdint.h>

#define LOONG_STATIC_ASSERT(cond, name) typedef char loong_static_assert_##name[(cond) ? 1 : -1]

LOONG_STATIC_ASSERT(CHAR_BIT == 8, char_bit_is_8);
LOONG_STATIC_ASSERT(UINT32_MAX == 0xffffffffU, uint32_is_32_bits);
LOONG_STATIC_ASSERT(LOONG_Q == 2, q_is_2);
LOONG_STATIC_ASSERT(0 < LOONG_K && LOONG_K < LOONG_N_PRIME, k_range_valid);
LOONG_STATIC_ASSERT(LOONG_N_PRIME <= LOONG_M, n_prime_le_m);
LOONG_STATIC_ASSERT(LOONG_EPSILON <= LOONG_N1 * LOONG_N2 - LOONG_N_PRIME, epsilon_room_valid);
LOONG_STATIC_ASSERT(LOONG_EPSILON <= LOONG_N_PRIME - LOONG_K, epsilon_decode_valid);
LOONG_STATIC_ASSERT(2 * LOONG_K * LOONG_M >= 2 * LOONG_SECURITY_BITS, message_entropy_valid);
LOONG_STATIC_ASSERT(LOONG_AXY_01 == LOONG_AXY_10, axy_symmetric);
LOONG_STATIC_ASSERT(LOONG_AR_01 == LOONG_AR_10, ar_symmetric);
LOONG_STATIC_ASSERT(LOONG_AXY_01 <= LOONG_AXY_00 && LOONG_AXY_01 <= LOONG_AXY_11, axy_intersection_valid);
LOONG_STATIC_ASSERT(LOONG_AR_01 <= LOONG_AR_00 && LOONG_AR_01 <= LOONG_AR_11, ar_intersection_valid);
LOONG_STATIC_ASSERT(LOONG_VEC_N_N1_BYTES == LOONG_X_OR_S_BYTES, vector_size_valid);
LOONG_STATIC_ASSERT(LOONG_PK_BYTES == LOONG_PK_SEED_BYTES + LOONG_X_OR_S_BYTES, pk_size_valid);
LOONG_STATIC_ASSERT(LOONG_SK_PRIME_BYTES == LOONG_PK_SEED_BYTES + LOONG_X_OR_S_BYTES, sk_prime_size_valid);
LOONG_STATIC_ASSERT(LOONG_SK_BYTES == LOONG_SK_PRIME_BYTES + LOONG_PK_BYTES + LOONG_XI_BYTES, sk_size_valid);
LOONG_STATIC_ASSERT(LOONG_PKE_CT_DERIVED_BYTES == LOONG_PKE_CT_BYTES, pke_ct_size_valid);
LOONG_STATIC_ASSERT(LOONG_CT_BYTES == LOONG_PKE_CT_BYTES + LOONG_SALT_BYTES, api_ct_size_valid);
LOONG_STATIC_ASSERT(LOONG_SS_BYTES == LOONG_LAMBDA_BYTES, ss_size_valid);
LOONG_STATIC_ASSERT(LOONG_XI_BYTES == 64, xi_size_valid);

unsigned long long kem_get_pk_len_bytes()
{
	return LOONG_PK_BYTES;
}

unsigned long long kem_get_sk_len_bytes()
{
	return LOONG_SK_BYTES;
}

unsigned long long kem_get_ss_len_bytes()
{
	return LOONG_SS_BYTES;
}

unsigned long long kem_get_ct_len_bytes()
{
	return LOONG_CT_BYTES;
}

int kem_keygen(
	unsigned char *pk, unsigned long long *pk_len_bytes,
	unsigned char *sk, unsigned long long *sk_len_bytes)
{
	if (pk_len_bytes == 0 || sk_len_bytes == 0) {
		return LOONG_ERR_NULL;
	}
	*pk_len_bytes = LOONG_PK_BYTES;
	*sk_len_bytes = LOONG_SK_BYTES;
	if (pk == 0 || sk == 0) {
		return LOONG_ERR_NULL;
	}
	return loong_kem_keygen(pk, LOONG_PK_BYTES, sk, LOONG_SK_BYTES);
}

int kem_enc(
	unsigned char *pk, unsigned long long pk_len_bytes,
	unsigned char *ss, unsigned long long *ss_len_bytes,
	unsigned char *ct, unsigned long long *ct_len_bytes)
{
	if (ss_len_bytes == 0 || ct_len_bytes == 0) {
		return LOONG_ERR_NULL;
	}
	*ss_len_bytes = LOONG_SS_BYTES;
	*ct_len_bytes = LOONG_CT_BYTES;
	if (pk == 0 || ss == 0 || ct == 0) {
		return LOONG_ERR_NULL;
	}
	if (pk_len_bytes != LOONG_PK_BYTES) {
		return LOONG_ERR_BAD_LENGTH;
	}
	return loong_kem_encapsulate(pk, pk_len_bytes, ss, LOONG_SS_BYTES, ct,
	                             LOONG_CT_BYTES);
}

int kem_dec(
	unsigned char *sk, unsigned long long sk_len_bytes,
	unsigned char *ct, unsigned long long ct_len_bytes,
	unsigned char *ss, unsigned long long *ss_len_bytes)
{
	if (ss_len_bytes == 0) {
		return LOONG_ERR_NULL;
	}
	*ss_len_bytes = LOONG_SS_BYTES;
	if (sk == 0 || ct == 0 || ss == 0) {
		return LOONG_ERR_NULL;
	}
	if (sk_len_bytes != LOONG_SK_BYTES || ct_len_bytes != LOONG_CT_BYTES) {
		return LOONG_ERR_BAD_LENGTH;
	}
	return loong_kem_decapsulate(sk, sk_len_bytes, ct, ct_len_bytes, ss,
	                             LOONG_SS_BYTES);
}
