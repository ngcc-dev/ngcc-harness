/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.
*/

#include "ublockith_em_d3_256f_AlgorithmInstance.h"
#include "drng.h"
#include "ublockith_em_d3_256f.h"
#include <stdbool.h>

// DRNG_ctx for generating pseudorandom numbers within the SIG scheme
extern DRNG_ctx drng_algorithm;

// The following should be used to get pseudorandom numbers
// get_random_number(&drng_algorithm, random_number, random_number_len_bits);

#define ptr_get_bit(value, index) (((value)[(index) / 8] >> ((index) % 8)) & 1)

unsigned long long sig_get_pk_len_bytes(void)
{
	return UBLOCKITH_EM_D3_256F_PUBLIC_KEY_SIZE;
}

unsigned long long sig_get_sk_len_bytes(void)
{
	return UBLOCKITH_EM_D3_256F_PRIVATE_KEY_SIZE;
}

unsigned long long sig_get_sn_len_bytes(void)
{
	return UBLOCKITH_EM_D3_256F_SIGNATURE_SIZE;
}

// pk_len 和pk长度 相等是输入保障的
// 这里输入*pk和*len的目的是因为*pk的长度无法访问
int sig_keygen(
	unsigned char *pk, unsigned long long *pk_len_bytes,
	unsigned char *sk, unsigned long long *sk_len_bytes)
{
	if (!pk_len_bytes || !sk_len_bytes) {
		return -1;
	}
	if (*pk_len_bytes != sig_get_pk_len_bytes() ||
	    *sk_len_bytes != sig_get_sk_len_bytes()) {
		return -1;
	}

	uint8_t sk_key[256 / 8];
	uint8_t sk_input[256 / 8];

	/* sk_key = uBlock input (secret k): keyspace-restricted witness */
	bool done = false;
	while (!done) {
		get_random_number(&drng_algorithm, sk_key, 256);
		done = (ptr_get_bit(sk_key, 0) & ptr_get_bit(sk_key, 1)) == 0;
	}

	/* sk_input = uBlock key (public x): uniformly random */
	get_random_number(&drng_algorithm, sk_input, 256);
	
	return ublockith_em_d3_256f_keygen(pk, sk, sk_key, sk_input);
}

int sig_sign(
	unsigned char *sk, unsigned long long sk_len_bytes,
	unsigned char *m, unsigned long long m_len_bytes,
	unsigned char *sn, unsigned long long *sn_len_bytes)
{
	if (sk_len_bytes != sig_get_sk_len_bytes() || !sn_len_bytes) {
		return -1;
	}

	uint8_t rho[256 / 8];
	get_random_number(&drng_algorithm, rho, 256);
	return ublockith_em_d3_256f_sign_with_randomness(sk, m, (size_t) m_len_bytes, rho, sizeof(rho), sn, (size_t*) sn_len_bytes);
}

int sig_verify(
	unsigned char *pk, unsigned long long pk_len_bytes,
	unsigned char *sn, unsigned long long sn_len_bytes,
	unsigned char *m, unsigned long long m_len_bytes)
{
	if (pk_len_bytes != sig_get_pk_len_bytes() || sn_len_bytes != sig_get_sn_len_bytes()) {
		return -1;
	}
	return ublockith_em_d3_256f_verify(pk, m, (size_t) m_len_bytes, sn, (size_t) sn_len_bytes);
}
