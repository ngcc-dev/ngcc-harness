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
#include "drng.h"
#include "dtru.h"
#include "params.h"
#include "auxfunc.h"

// DRNG_ctx for generating pseudorandom numbers within the KEM scheme
extern DRNG_ctx drng_algorithm;

// The following should be used to get pseudorandom numbers
// get_random_number(&drng_algorithm, random_number, random_number_len_bits);


unsigned long long kem_get_pk_len_bytes(void)
{
	return DTRU_KEM_PUBLICKEYBYTES;
}

unsigned long long kem_get_sk_len_bytes(void)
{
	return DTRU_KEM_SECRETKEYBYTES;
}

unsigned long long kem_get_ss_len_bytes(void)
{
	return DTRU_SHAREDKEYBYTES;
}

unsigned long long kem_get_ct_len_bytes(void)
{
	return DTRU_KEM_CIPHERTEXTBYTES;
}

int kem_keygen(
	unsigned char *pk, unsigned long long *pk_len_bytes,
	unsigned char *sk, unsigned long long *sk_len_bytes)
{
	unsigned int i;
  unsigned char coins[DTRU_COINBYTES_KEYGEN];

  do
  {
	get_random_number(&drng_algorithm,coins,DTRU_COINBYTES_KEYGEN*8);
  } while (pke_keygen(pk, sk, coins));

  for (i = 0; i < DTRU_PKE_PUBLICKEYBYTES; ++i)
    sk[i + DTRU_PKE_SECRETKEYBYTES] = pk[i];
  get_random_number(&drng_algorithm,sk + DTRU_PKE_SECRETKEYBYTES + DTRU_PKE_PUBLICKEYBYTES,DTRU_Z_BYTES * 8);
  *pk_len_bytes = DTRU_KEM_PUBLICKEYBYTES;
  *sk_len_bytes = DTRU_KEM_SECRETKEYBYTES;
  return 0;
}

int kem_enc(
	unsigned char *pk, unsigned long long pk_len_bytes,
	unsigned char *ss, unsigned long long *ss_len_bytes,
	unsigned char *ct, unsigned long long *ct_len_bytes)
{
	unsigned int i;
  unsigned char buf[DTRU_SHAREDKEYBYTES + DTRU_COINBYTES_ENC], m[DTRU_MSGBYTES];
  //buf2存ID(pk)||M
  unsigned char buf2[DTRU_PREFIXHASHBYTES + DTRU_MSGBYTES + 1];
  get_random_number(&drng_algorithm,m, DTRU_MSGBYTES*8);

  for (i = 0; i < DTRU_PREFIXHASHBYTES && i < pk_len_bytes; ++i)
    buf2[i] = pk[i];
  for (i = 0; i < DTRU_MSGBYTES; ++i)
    buf2[i+DTRU_PREFIXHASHBYTES] = m[i];

  buf2[DTRU_PREFIXHASHBYTES + DTRU_MSGBYTES] = 0;
  pseudohash(512,buf2,(DTRU_PREFIXHASHBYTES + DTRU_MSGBYTES + 1) * 8 ,buf);
  buf2[DTRU_PREFIXHASHBYTES + DTRU_MSGBYTES] = 1;
  pseudohash(512,buf2,(DTRU_PREFIXHASHBYTES + DTRU_MSGBYTES + 1) * 8 ,buf+ DTRU_SHAREDKEYBYTES);
  pseudoXOF(DTRU_COINBYTES_ENC * 8,buf + DTRU_SHAREDKEYBYTES,DTRU_SHAREDKEYBYTES * 8,buf + DTRU_SHAREDKEYBYTES);
  pke_enc(ct, pk, m, buf + DTRU_SHAREDKEYBYTES);

  for (i = 0; i < DTRU_SHAREDKEYBYTES; ++i)
    ss[i] = buf[i];

  *ss_len_bytes = DTRU_SHAREDKEYBYTES;
  *ct_len_bytes = DTRU_KEM_CIPHERTEXTBYTES;
  return 0;
}

int kem_dec(
	unsigned char *sk, unsigned long long sk_len_bytes,
	unsigned char *ct, unsigned long long ct_len_bytes,
	unsigned char *ss, unsigned long long *ss_len_bytes)
{
	unsigned int i;
  //buf存放导出密钥及采样需要的coin，buf2存~K，由于是SHA3_512的输出，需要等于64字节
  unsigned char buf[DTRU_SHAREDKEYBYTES + DTRU_COINBYTES_ENC], buf2[SHA3512_OUTPUTBYTES], m[DTRU_PREFIXHASHBYTES + DTRU_MSGBYTES + 1];
  unsigned char ct2[DTRU_PKE_CIPHERTEXTBYTES + DTRU_Z_BYTES + DTRU_PREFIXHASHBYTES];
  int16_t t;
  uint32_t fail;

  pke_dec(m + DTRU_PREFIXHASHBYTES, ct, sk);

  for (i = 0; i < DTRU_PREFIXHASHBYTES; ++i)
    m[i] = sk[i + DTRU_PKE_SECRETKEYBYTES];

  m[DTRU_PREFIXHASHBYTES + DTRU_MSGBYTES] = 0;
  pseudohash(512,m,(DTRU_PREFIXHASHBYTES + DTRU_MSGBYTES + 1) * 8 ,buf);
  m[DTRU_PREFIXHASHBYTES + DTRU_MSGBYTES] = 1;
  pseudohash(512,m,(DTRU_PREFIXHASHBYTES + DTRU_MSGBYTES + 1) * 8 ,buf+ DTRU_SHAREDKEYBYTES);

  pseudoXOF(DTRU_COINBYTES_ENC * 8,buf + DTRU_SHAREDKEYBYTES,DTRU_SHAREDKEYBYTES *8,buf + DTRU_SHAREDKEYBYTES);

  pke_enc(ct2, sk + DTRU_PKE_SECRETKEYBYTES, m + DTRU_PREFIXHASHBYTES, buf + DTRU_SHAREDKEYBYTES);

  t = 0;
  for (i = 0; i < DTRU_PKE_CIPHERTEXTBYTES; ++i)
    t |= ct[i] ^ ct2[i];

  fail = (uint16_t)t;
  fail = (-fail) >> 31;


  //concatenate c
  for(i = 0; i < ct_len_bytes; ++i)
  {
    ct2[i] = ct[i];
  }
  //concatenate z
  for(i = 0; i < DTRU_Z_BYTES && i < sk_len_bytes; ++i)
  {
    ct2[i+ct_len_bytes] = sk[i + DTRU_PKE_SECRETKEYBYTES + DTRU_PKE_PUBLICKEYBYTES];
  }
  //H(c,z)
  pseudohash(512,ct2,(ct_len_bytes + DTRU_Z_BYTES) * 8 ,buf2);

  for (i = 0; i < DTRU_SHAREDKEYBYTES; ++i)
    ss[i] = buf[i] ^ ((-fail) & (buf[i] ^ buf2[i]));

  *ss_len_bytes = DTRU_SHAREDKEYBYTES;
  return fail;
}