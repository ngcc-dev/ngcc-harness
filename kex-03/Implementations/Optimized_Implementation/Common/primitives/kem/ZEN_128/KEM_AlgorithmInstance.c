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
Copyright (c) 2026 Yu Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
File Description: Declares the ZEN key-encapsulation mechanism layer for the optimized ZEN-128 instance.
*/

#include "KEM_AlgorithmInstance.h"
#include "drng.h"
#include "params.h"

#include <string.h>
#include "pke.h"
#include "auxfunc.h"

// DRNG_ctx for generating pseudorandom numbers within the KEM scheme
extern DRNG_ctx drng_algorithm;

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
	if (pk == NULL || pk_len_bytes == NULL || sk == NULL || sk_len_bytes == NULL) {
        return -1;
    }

    *pk_len_bytes = kem_get_pk_len_bytes();
    *sk_len_bytes = kem_get_sk_len_bytes();
	pke_keygen_derand(pk, sk, seed);
	for(i = 0; i < ZEN_INDCPA_PUBLICKEY_LEN_BYTES; i++)
	{
		sk[i+ZEN_INDCPA_SECREKEY_LEN_BYTES] = pk[i];
	}
	sm3hash(2*ZEN_SYM_LEN_BYTES*8, pk, ZEN_INDCPA_PUBLICKEY_LEN_BYTES*8, sk+ZEN_INDCPA_SECREKEY_LEN_BYTES+ZEN_INDCPA_PUBLICKEY_LEN_BYTES);
	get_random_number(&drng_algorithm, sk+ZEN_INDCPA_SECREKEY_LEN_BYTES+ZEN_INDCPA_PUBLICKEY_LEN_BYTES+ZEN_SYM_LEN_BYTES, ZEN_SHAREDKEY_LEN_BYTES*8);
	
	return 0;
}

int kem_keygen(
	unsigned char *pk, unsigned long long *pk_len_bytes,
	unsigned char *sk, unsigned long long *sk_len_bytes)
{
	unsigned int i;
	if (pk == NULL || pk_len_bytes == NULL || sk == NULL || sk_len_bytes == NULL) {
        return -1;
    }

    *pk_len_bytes = kem_get_pk_len_bytes();
    *sk_len_bytes = kem_get_sk_len_bytes();
	pke_keygen(pk, sk);
	for(i = 0; i < ZEN_INDCPA_PUBLICKEY_LEN_BYTES; i++)
	{
		sk[i+ZEN_INDCPA_SECREKEY_LEN_BYTES] = pk[i];
	}
	sm3hash(2*ZEN_SYM_LEN_BYTES*8, pk, ZEN_INDCPA_PUBLICKEY_LEN_BYTES*8, sk+ZEN_INDCPA_SECREKEY_LEN_BYTES+ZEN_INDCPA_PUBLICKEY_LEN_BYTES);
	get_random_number(&drng_algorithm, sk+ZEN_INDCPA_SECREKEY_LEN_BYTES+ZEN_INDCPA_PUBLICKEY_LEN_BYTES+ZEN_SYM_LEN_BYTES, ZEN_SHAREDKEY_LEN_BYTES*8);
	
	return 0;
}

int kem_enc(
	unsigned char *pk, unsigned long long pk_len_bytes,
	unsigned char *ss, unsigned long long *ss_len_bytes,
	unsigned char *ct, unsigned long long *ct_len_bytes)
{
	ALIGN32 uint8_t buf[ZEN_INDCPA_MSG_LEN_BYTES+2*ZEN_SYM_LEN_BYTES];
	ALIGN32 uint8_t kr[2*ZEN_SYM_LEN_BYTES+SEED_LEN_BYTES];
	if (pk == NULL || ss == NULL || ss_len_bytes == NULL || ct == NULL || ct_len_bytes == NULL) {
        return -1;
    }
    if (pk_len_bytes != kem_get_pk_len_bytes()) {
        return -2;
    }

    *ss_len_bytes = kem_get_ss_len_bytes();
    *ct_len_bytes = kem_get_ct_len_bytes();
	//m
	get_random_number(&drng_algorithm, buf, ZEN_INDCPA_MSG_LEN_BYTES*8);
	//hash(pk)
	sm3hash(2*ZEN_SYM_LEN_BYTES*8, pk, ZEN_INDCPA_PUBLICKEY_LEN_BYTES*8, buf+ZEN_INDCPA_MSG_LEN_BYTES);
	//hash(m||hash(pk))
	pseudohash((2*ZEN_SYM_LEN_BYTES+SEED_LEN_BYTES)*8, buf, (ZEN_INDCPA_MSG_LEN_BYTES+ZEN_SYM_LEN_BYTES)*8, kr);
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
	ALIGN32 uint8_t buf[ZEN_INDCPA_MSG_LEN_BYTES+ZEN_SYM_LEN_BYTES];
	ALIGN32 uint8_t kr[2*ZEN_SYM_LEN_BYTES+SEED_LEN_BYTES];
	ALIGN32 uint8_t zc[ZEN_SHAREDKEY_LEN_BYTES+ZEN_CIPHERTEXT_LEN_BYTES];
	ALIGN32 uint8_t ctp[ZEN_CIPHERTEXT_LEN_BYTES];
	ALIGN32 uint8_t ssp[2*ZEN_SHAREDKEY_LEN_BYTES];
	ALIGN32 uint8_t *pk = sk+ZEN_INDCPA_SECREKEY_LEN_BYTES;
	if (sk == NULL || ct == NULL || ss == NULL || ss_len_bytes == NULL) {
        return -2;
    }
    if (sk_len_bytes != kem_get_sk_len_bytes() || ct_len_bytes != kem_get_ct_len_bytes()) {
        return -3;
    }
	*ss_len_bytes = kem_get_ss_len_bytes();
	pke_dec(sk, ct, buf);
	memcpy(buf+ZEN_INDCPA_MSG_LEN_BYTES, sk+ZEN_INDCPA_SECREKEY_LEN_BYTES+ZEN_INDCPA_PUBLICKEY_LEN_BYTES, ZEN_SYM_LEN_BYTES);
	pseudohash((2*ZEN_SYM_LEN_BYTES+SEED_LEN_BYTES)*8, buf, (ZEN_INDCPA_MSG_LEN_BYTES+ZEN_SYM_LEN_BYTES)*8, kr);

	memcpy(zc, sk+ZEN_INDCPA_SECREKEY_LEN_BYTES+ZEN_INDCPA_PUBLICKEY_LEN_BYTES+ZEN_SYM_LEN_BYTES, ZEN_SHAREDKEY_LEN_BYTES);
	memcpy(zc+ZEN_SHAREDKEY_LEN_BYTES, ct, ZEN_CIPHERTEXT_LEN_BYTES);

	sm3hash(2*ZEN_SYM_LEN_BYTES*8, zc, (ZEN_SHAREDKEY_LEN_BYTES+ZEN_CIPHERTEXT_LEN_BYTES)*8, ssp);

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