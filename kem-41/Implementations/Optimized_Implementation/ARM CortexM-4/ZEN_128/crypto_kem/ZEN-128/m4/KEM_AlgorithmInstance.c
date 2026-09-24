/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.
*/

#include "KEM_AlgorithmInstance.h"
#ifdef USE_KECCAK
#include "randombytes.h"
#include "fips202.h"
#else
#include "drng.h"
// DRNG_ctx for generating pseudorandom numbers within the KEM scheme
extern DRNG_ctx drng_algorithm;
#endif
#include "params.h"

#include <string.h>
#include "pke.h"
#include "auxfunc.h"

#include <stdio.h>

// The following should be used to get pseudorandom numbers
// get_random_number(&drng_algorithm, random_number, random_number_len_bits);

unsigned long long kem_get_pk_len_bytes()
{
	return ZEN_PUBLICKEY_LEN_BYTES;
}

unsigned long long kem_get_sk_len_bytes()
{
	return ZEN_SECREKEY_LEN_BYTES;
}

unsigned long long kem_get_ss_len_bytes()
{
	return ZEN_SHAREDKEY_LEN_BYTES;
}

unsigned long long kem_get_ct_len_bytes()
{
	return ZEN_CIPHERTEXT_LEN_BYTES;
}

int kem_keygen_derand(
	unsigned char *pk, unsigned long long *pk_len_bytes,
	unsigned char *sk, unsigned long long *sk_len_bytes,
	const unsigned char *seed)
{
	unsigned int i;

	pke_keygen_derand(pk, sk, seed);
	for(i = 0; i < ZEN_INDCPA_PUBLICKEY_LEN_BYTES; i++)
	{
		sk[i+ZEN_INDCPA_SECREKEY_LEN_BYTES] = pk[i];
	}
#ifdef USE_KECCAK
	sha3_256(sk + ZEN_INDCPA_SECREKEY_LEN_BYTES + ZEN_INDCPA_PUBLICKEY_LEN_BYTES, pk, ZEN_INDCPA_PUBLICKEY_LEN_BYTES);
	randombytes(sk+ZEN_INDCPA_SECREKEY_LEN_BYTES+ZEN_INDCPA_PUBLICKEY_LEN_BYTES+ZEN_SYM_LEN_BYTES, ZEN_SHAREDKEY_LEN_BYTES);
#else
	sm3hash(2*ZEN_SYM_LEN_BYTES*8, pk, ZEN_INDCPA_PUBLICKEY_LEN_BYTES*8, sk+ZEN_INDCPA_SECREKEY_LEN_BYTES+ZEN_INDCPA_PUBLICKEY_LEN_BYTES);
	get_random_number(&drng_algorithm, sk+ZEN_INDCPA_SECREKEY_LEN_BYTES+ZEN_INDCPA_PUBLICKEY_LEN_BYTES+ZEN_SYM_LEN_BYTES, ZEN_SHAREDKEY_LEN_BYTES*8);
#endif
	return 0;
}

int kem_keygen(
	unsigned char *pk, unsigned long long *pk_len_bytes,
	unsigned char *sk, unsigned long long *sk_len_bytes)
{
	unsigned int i;

	pke_keygen(pk, sk);
	for(i = 0; i < ZEN_INDCPA_PUBLICKEY_LEN_BYTES; i++)
	{
		sk[i+ZEN_INDCPA_SECREKEY_LEN_BYTES] = pk[i];
	}
#ifdef USE_KECCAK
	sha3_256(sk + ZEN_INDCPA_SECREKEY_LEN_BYTES + ZEN_INDCPA_PUBLICKEY_LEN_BYTES, pk, ZEN_INDCPA_PUBLICKEY_LEN_BYTES);
	randombytes(sk+ZEN_INDCPA_SECREKEY_LEN_BYTES+ZEN_INDCPA_PUBLICKEY_LEN_BYTES+ZEN_SYM_LEN_BYTES, ZEN_SHAREDKEY_LEN_BYTES);
#else
	sm3hash(2*ZEN_SYM_LEN_BYTES*8, pk, ZEN_INDCPA_PUBLICKEY_LEN_BYTES*8, sk+ZEN_INDCPA_SECREKEY_LEN_BYTES+ZEN_INDCPA_PUBLICKEY_LEN_BYTES);
	get_random_number(&drng_algorithm, sk+ZEN_INDCPA_SECREKEY_LEN_BYTES+ZEN_INDCPA_PUBLICKEY_LEN_BYTES+ZEN_SYM_LEN_BYTES, ZEN_SHAREDKEY_LEN_BYTES*8);
#endif
	return 0;
}

int kem_enc(
	unsigned char *pk, unsigned long long pk_len_bytes,
	unsigned char *ss, unsigned long long *ss_len_bytes,
	unsigned char *ct, unsigned long long *ct_len_bytes)
{
	uint8_t buf[ZEN_INDCPA_MSG_LEN_BYTES+2*ZEN_SYM_LEN_BYTES];
	uint8_t kr[2*ZEN_SYM_LEN_BYTES+SEED_LEN_BYTES];
#ifdef USE_KECCAK
	randombytes(buf, ZEN_INDCPA_MSG_LEN_BYTES);
	sha3_256(buf+ZEN_INDCPA_MSG_LEN_BYTES, pk, ZEN_INDCPA_PUBLICKEY_LEN_BYTES);
	shake256(kr, 2 * ZEN_SYM_LEN_BYTES + SEED_LEN_BYTES, buf, (ZEN_INDCPA_MSG_LEN_BYTES + ZEN_SYM_LEN_BYTES));
#else
	//m
	get_random_number(&drng_algorithm, buf, ZEN_INDCPA_MSG_LEN_BYTES*8);
	//hash(pk)
	sm3hash(2*ZEN_SYM_LEN_BYTES*8, pk, ZEN_INDCPA_PUBLICKEY_LEN_BYTES*8, buf+ZEN_INDCPA_MSG_LEN_BYTES);
	//hash(m||hash(pk))
	pseudohash((2*ZEN_SYM_LEN_BYTES+SEED_LEN_BYTES)*8, buf, (ZEN_INDCPA_MSG_LEN_BYTES+ZEN_SYM_LEN_BYTES)*8, kr);
#endif
	pke_enc(pk, buf, kr+ZEN_SYM_LEN_BYTES, ct);
	memcpy(ss, kr, ZEN_SYM_LEN_BYTES);

	return 0;
}

int kem_dec(
	unsigned char *sk, unsigned long long sk_len_bytes,
	unsigned char *ct, unsigned long long ct_len_bytes,
	unsigned char *ss, unsigned long long *ss_len_bytes)
{
	unsigned int i, mask;
	uint8_t buf[ZEN_INDCPA_MSG_LEN_BYTES+ZEN_SYM_LEN_BYTES];
	uint8_t kr[2*ZEN_SYM_LEN_BYTES+SEED_LEN_BYTES];
	uint8_t zc[ZEN_SHAREDKEY_LEN_BYTES+ZEN_CIPHERTEXT_LEN_BYTES];
	uint8_t ctp[ZEN_CIPHERTEXT_LEN_BYTES];
	uint8_t ssp[2*ZEN_SHAREDKEY_LEN_BYTES];
	uint8_t *pk = sk+ZEN_INDCPA_SECREKEY_LEN_BYTES;

	pke_dec(sk, ct, buf);
	memcpy(buf+ZEN_INDCPA_MSG_LEN_BYTES, sk+ZEN_INDCPA_SECREKEY_LEN_BYTES+ZEN_INDCPA_PUBLICKEY_LEN_BYTES, ZEN_SYM_LEN_BYTES);
#ifdef USE_KECCAK
	shake256(kr, 2 * ZEN_SYM_LEN_BYTES + SEED_LEN_BYTES, buf, (ZEN_INDCPA_MSG_LEN_BYTES + ZEN_SYM_LEN_BYTES));
#else
	pseudohash((2*ZEN_SYM_LEN_BYTES+SEED_LEN_BYTES)*8, buf, (ZEN_INDCPA_MSG_LEN_BYTES+ZEN_SYM_LEN_BYTES)*8, kr);
#endif
	memcpy(zc, sk+ZEN_INDCPA_SECREKEY_LEN_BYTES+ZEN_INDCPA_PUBLICKEY_LEN_BYTES+ZEN_SYM_LEN_BYTES, ZEN_SHAREDKEY_LEN_BYTES);
	memcpy(zc+ZEN_SHAREDKEY_LEN_BYTES, ct, ZEN_CIPHERTEXT_LEN_BYTES);
#ifdef USE_KECCAK
	sha3_256(ssp, zc, (ZEN_SHAREDKEY_LEN_BYTES+ZEN_CIPHERTEXT_LEN_BYTES));
#else
	sm3hash(2*ZEN_SYM_LEN_BYTES*8, zc, (ZEN_SHAREDKEY_LEN_BYTES+ZEN_CIPHERTEXT_LEN_BYTES)*8, ssp);
#endif
	pke_enc(pk, buf, kr+ZEN_SYM_LEN_BYTES, ctp);	

	mask = 0;
	for(i = 0; i < ZEN_CIPHERTEXT_LEN_BYTES; i++)
	{
		mask |= (ct[i] ^ ctp[i]);
	}
	mask = ((mask | (0u - mask)) >> 31);

	for(i = 0; i < ZEN_SHAREDKEY_LEN_BYTES; i++)
	{
		ss[i] = (ssp[i] & (-mask)) | (kr[i] & (~(-mask)));
	}


	return 0;
}