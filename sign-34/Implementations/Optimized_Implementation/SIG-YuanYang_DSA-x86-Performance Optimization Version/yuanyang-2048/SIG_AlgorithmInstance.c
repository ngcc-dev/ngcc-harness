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
 * yuanyang-2048 NGCC submission API wrapper.
 *
 * Keep this file close to SIG_AlgorithmInstance.h.  Actual codec, common
 * arithmetic/hash code, and verification live in separate modules.
 */

#include <stddef.h>

#include "drng.h"
#include "SIG_AlgorithmInstance.h"
#include "yuanyang_inner.h"

// DRNG_ctx for generating pseudorandom numbers within the SIG scheme
extern DRNG_ctx drng_algorithm;

unsigned long long
sig_get_pk_len_bytes(void)
{
	return YUANYANG_PK_BYTES;
}

unsigned long long
sig_get_sk_len_bytes(void)
{
	return YUANYANG_SK_BYTES;
}

unsigned long long
sig_get_sn_len_bytes(void)
{
	return YUANYANG_SIGNATURE_BYTES;
}

int
sig_keygen(
	unsigned char *pk, unsigned long long *pk_len_bytes,
	unsigned char *sk, unsigned long long *sk_len_bytes)
{
	return yuanyang_keygen_core(pk, pk_len_bytes, sk, sk_len_bytes);
}

int
sig_sign(
	unsigned char *sk, unsigned long long sk_len_bytes,
	unsigned char *m, unsigned long long m_len_bytes,
	unsigned char *sn, unsigned long long *sn_len_bytes)
{
	return yuanyang_sign_core(sk, sk_len_bytes, m, m_len_bytes, sn, sn_len_bytes);
}

int
sig_verify(
	unsigned char *pk, unsigned long long pk_len_bytes,
	unsigned char *sn, unsigned long long sn_len_bytes,
	unsigned char *m, unsigned long long m_len_bytes)
{
	return yuanyang_verify_core(pk, pk_len_bytes, sn, sn_len_bytes, m, m_len_bytes);
}
