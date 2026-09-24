/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

This file instantiates the API_PKC SIG interface for SQIsignTriangle_lvl5.
*/

#ifndef SIG_ALGORITHM_INSTANCE_H
#define SIG_ALGORITHM_INSTANCE_H

// Set "OUTPUT_BLANK_TEST_VECTORS" as 0 to generate test vector files
// Set "OUTPUT_BLANK_TEST_VECTORS" as 1 to generate blank template
#define OUTPUT_BLANK_TEST_VECTORS 0

// Algorithm instance name (<= 64 bytes; letters, numbers, '-' or '_').
#define ALGORITHM_INSTANCE "SQIsignTriangle_lvl5"

#ifdef __cplusplus
extern "C"
{
#endif

	unsigned long long sig_get_pk_len_bytes();
	unsigned long long sig_get_sk_len_bytes();
	unsigned long long sig_get_sn_len_bytes();

	int sig_keygen(
		unsigned char *pk, unsigned long long *pk_len_bytes,
		unsigned char *sk, unsigned long long *sk_len_bytes);

	int sig_sign(
		unsigned char *sk, unsigned long long sk_len_bytes,
		unsigned char *m, unsigned long long m_len_bytes,
		unsigned char *sn, unsigned long long *sn_len_bytes);

	int sig_verify(
		unsigned char *pk, unsigned long long pk_len_bytes,
		unsigned char *sn, unsigned long long sn_len_bytes,
		unsigned char *m, unsigned long long m_len_bytes);

#ifdef __cplusplus
}
#endif
#endif
