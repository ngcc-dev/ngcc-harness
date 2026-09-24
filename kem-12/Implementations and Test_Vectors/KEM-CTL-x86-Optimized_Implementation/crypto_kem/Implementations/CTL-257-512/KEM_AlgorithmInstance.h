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

#define OUTPUT_BLANK_TEST_VECTORS 0
#define ALGORITHM_INSTANCE "CTL-257-512"

#ifdef __cplusplus
extern "C"
{
#endif

	unsigned long long kem_get_pk_len_bytes();
	unsigned long long kem_get_sk_len_bytes();
	unsigned long long kem_get_ss_len_bytes();
	unsigned long long kem_get_ct_len_bytes();
	int kem_keygen(
		unsigned char *pk, unsigned long long *pk_len_bytes,
		unsigned char *sk, unsigned long long *sk_len_bytes);
	int kem_enc(
		unsigned char *pk, unsigned long long pk_len_bytes,
		unsigned char *ss, unsigned long long *ss_len_bytes,
		unsigned char *ct, unsigned long long *ct_len_bytes);
	int kem_dec(
		unsigned char *sk, unsigned long long sk_len_bytes,
		unsigned char *ct, unsigned long long ct_len_bytes,
		unsigned char *ss, unsigned long long *ss_len_bytes);

#ifdef __cplusplus
}
#endif
#endif
