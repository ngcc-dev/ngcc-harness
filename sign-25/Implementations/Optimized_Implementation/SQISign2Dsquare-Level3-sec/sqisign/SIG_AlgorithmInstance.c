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
#include "sqisigndim2.h"
#include "drng.h"

// DRNG_ctx for generating pseudorandom numbers within the SIG scheme
extern DRNG_ctx drng_algorithm;

// The following should be used to get pseudorandom numbers
// get_random_number(&drng_algorithm, random_number, random_number_len_bits);

unsigned long long sig_get_pk_len_bytes()
{
	return PUBLICKEY_BYTES;
}

unsigned long long sig_get_sk_len_bytes()
{
	return SECRETKEY_BYTES;
}

unsigned long long sig_get_sn_len_bytes()
{
	return SIGNATURE_BYTES;
}

int sig_keygen(
	unsigned char *pk, unsigned long long *pk_len_bytes,
	unsigned char *sk, unsigned long long *sk_len_bytes)
{
	protocols_keygen(pk, sk);
	*pk_len_bytes = PUBLICKEY_BYTES;
	*sk_len_bytes = SECRETKEY_BYTES;
	return 0;
}

int sig_sign(
    unsigned char *sk, unsigned long long sk_len_bytes,
    unsigned char *m, unsigned long long m_len_bytes,
    unsigned char *sn, unsigned long long *sn_len_bytes)
{
    (void)sk_len_bytes;
    protocols_sign(sn, sk, m, m_len_bytes);
    *sn_len_bytes = SIGNATURE_BYTES;
    return 0;
}

int sig_verify(
	unsigned char *pk, unsigned long long pk_len_bytes,
	unsigned char *sn, unsigned long long sn_len_bytes,
	unsigned char *m, unsigned long long m_len_bytes)
{
	(void)pk_len_bytes;
	(void)sn_len_bytes;

	return protocols_verif(sn, pk, m, m_len_bytes) ? 0 : -1;
}
