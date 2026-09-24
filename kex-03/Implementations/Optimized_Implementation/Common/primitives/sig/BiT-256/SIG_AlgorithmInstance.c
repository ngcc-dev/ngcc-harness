/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.
*/

#include "SIG_AlgorithmInstance.h"
#include "params.h"
#include "sign.h"


unsigned long long sig_get_pk_len_bytes(void)
{
	return BIT_PUBLICKEYBYTES;
}

unsigned long long sig_get_sk_len_bytes(void)
{
	return BIT_SECRETKEYBYTES;
}

unsigned long long sig_get_sn_len_bytes(void)
{
	return BIT_SIGNBYTES;
}

int sig_keygen(
	unsigned char *pk, unsigned long long *pk_len_bytes,
	unsigned char *sk, unsigned long long *sk_len_bytes)
{
	return bit_sig_keygen(pk, pk_len_bytes, sk, sk_len_bytes);
}

int sig_sign(
	unsigned char *sk, unsigned long long sk_len_bytes,
	unsigned char *m, unsigned long long m_len_bytes,
	unsigned char *sn, unsigned long long *sn_len_bytes)
{
	return bit_sig_sign(sk, sk_len_bytes, m, m_len_bytes, sn, sn_len_bytes);
}

int sig_verify(
	unsigned char *pk, unsigned long long pk_len_bytes,
	unsigned char *sn, unsigned long long sn_len_bytes,
	unsigned char *m, unsigned long long m_len_bytes)
{
	return bit_sig_verify(sn, sn_len_bytes, m, m_len_bytes, pk, pk_len_bytes);
}
