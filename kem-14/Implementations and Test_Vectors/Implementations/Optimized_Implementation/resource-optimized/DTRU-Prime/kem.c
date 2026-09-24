#include <stddef.h>
#include "randombytes.h"
#include "params.h"
#include "dtru.h"
#include "crypto_hash_sha3256.h"
#include <stdio.h>
#include "api.h"
unsigned char seed[DTRU_SEEDBYTES] = {0};
unsigned long long rand_get_sd_byts()
{
  return DTRU_SEEDBYTES;
}
int rand_init(unsigned char * s, unsigned long long s_byts)
{
  randombytes(s, s_byts);
  for(unsigned long long i = 0; i < s_byts; i ++)
  {
    seed[i] = s[i];
  }
  return 0;
}
int rand_byts(unsigned long long r_byts, unsigned char * r)
{
  crypto_hash_shake256(r, r_byts, seed, DTRU_SEEDBYTES);
  return 0;
}
unsigned long long kem_get_pk_byts()
{
  return DTRU_KEM_PUBLICKEYBYTES;
}
unsigned long long kem_get_sk_byts()
{
  return DTRU_KEM_SECRETKEYBYTES;
}
unsigned long long kem_get_ss_byts()
{
  return DTRU_SHAREDKEYBYTES;
}
unsigned long long kem_get_ct_byts()
{
  return DTRU_KEM_CIPHERTEXTBYTES;
}


int kem_keygen(unsigned char * pk, unsigned long long * pk_byts, unsigned char * sk, unsigned long long * sk_byts)
{
  unsigned int i;
  unsigned char coins[DTRU_COINBYTES_KEYGEN];

  rand_init(seed, DTRU_SEEDBYTES);
  rand_byts(DTRU_COINBYTES_KEYGEN, coins);
  pke_keygen(pk, sk, coins);

  for (i = 0; i < DTRU_PKE_PUBLICKEYBYTES; ++i)
    sk[i + DTRU_PKE_SECRETKEYBYTES] = pk[i];

  randombytes(sk + DTRU_PKE_SECRETKEYBYTES + DTRU_PKE_PUBLICKEYBYTES, DTRU_Z_BYTES);
  *pk_byts = DTRU_KEM_PUBLICKEYBYTES;
  *sk_byts = DTRU_KEM_SECRETKEYBYTES;
  return 0;
}

int kem_enc(unsigned char * pk, unsigned long long pk_byts,
unsigned char * ss, unsigned long long * ss_byts,
unsigned char * ct, unsigned long long * ct_byts)
{
  (void)pk_byts;
  unsigned int i;
  unsigned char buf[DTRU_SHAREDKEYBYTES + DTRU_COINBYTES_ENC], m[DTRU_MSGBYTES];
  //存ID(pk)||m
  unsigned char buf2[DTRU_PREFIXHASHBYTES + DTRU_MSGBYTES];
  //生成消息m
  randombytes(m , DTRU_MSGBYTES);
  //获得前缀哈希
  for (i = 0; i < DTRU_PREFIXHASHBYTES; ++i)
    buf2[i] = pk[i];
  //拼上m
  for (i = 0; i < DTRU_MSGBYTES; ++i)
    buf2[i+DTRU_PREFIXHASHBYTES] = m[i];
  //H(ID(pk)||m)
  crypto_hash_sha3512(buf, buf2, DTRU_PREFIXHASHBYTES + DTRU_MSGBYTES);

  crypto_hash_shake256(buf + DTRU_SHAREDKEYBYTES, DTRU_COINBYTES_ENC, buf + DTRU_SHAREDKEYBYTES, DTRU_SHAREDKEYBYTES);

  pke_enc(ct, pk, m, buf + DTRU_SHAREDKEYBYTES);

  for (i = 0; i < DTRU_SHAREDKEYBYTES; ++i)
    ss[i] = buf[i];
  *ss_byts = DTRU_SHAREDKEYBYTES;
  *ct_byts = DTRU_KEM_CIPHERTEXTBYTES;
  return 0;
}

int kem_dec(
unsigned char * sk, unsigned long long sk_byts,
unsigned char * ct, unsigned long long ct_byts,
unsigned char * ss, unsigned long long * ss_byts)
{
  (void)sk_byts;
  (void)ct_byts;
  unsigned int i;
  //buf存放导出密钥及采样需要的coin，buf2存~K，由于是SHA3_512的输出，需要等于64字节
  unsigned char buf[DTRU_SHAREDKEYBYTES + DTRU_COINBYTES_ENC], buf2[SHA3512_OUTPUTBYTES], m[DTRU_MSGBYTES];
  unsigned char buf3[1+ DTRU_PREFIXHASHBYTES +  DTRU_MSGBYTES];
  unsigned char ct2[DTRU_PKE_CIPHERTEXTBYTES + DTRU_Z_BYTES];
  int16_t t;
  int32_t fail;
  //解密得到m
  pke_dec(m, ct, sk);

  //获得ID(pk)||m
  //获得前缀哈希
  for (i = 0; i < DTRU_PREFIXHASHBYTES; ++i)
    buf3[i] = sk[i + DTRU_PKE_SECRETKEYBYTES];
  //拼上m
  for (i = 0; i < DTRU_MSGBYTES; ++i)
    buf3[i+DTRU_PREFIXHASHBYTES] = m[i];
  //H(ID(pk)||m)
  crypto_hash_sha3512(buf, buf3, DTRU_PREFIXHASHBYTES + DTRU_MSGBYTES);

  crypto_hash_shake256(buf + DTRU_SHAREDKEYBYTES, DTRU_COINBYTES_ENC, buf + DTRU_SHAREDKEYBYTES, DTRU_SHAREDKEYBYTES);
  pke_enc(ct2, sk + DTRU_PKE_SECRETKEYBYTES, m, buf + DTRU_SHAREDKEYBYTES);

  t = 0;
  for (i = 0; i < DTRU_PKE_CIPHERTEXTBYTES; ++i)
    t |= ct[i] ^ ct2[i];

  fail = (uint16_t)t;
  fail = (-fail) >> 31;

   //concatenate c
  for(i = 0; i < DTRU_KEM_CIPHERTEXTBYTES; ++i)
  {
    ct2[i] = ct[i];
  }
  //concatenate z
  for(i = 0; i <  DTRU_Z_BYTES; ++i)
  {
    ct2[i+DTRU_KEM_CIPHERTEXTBYTES] = sk[i + DTRU_PKE_SECRETKEYBYTES + DTRU_PKE_PUBLICKEYBYTES];
  }
  //H(c,z)
  crypto_hash_sha3512(buf2, ct2, DTRU_KEM_CIPHERTEXTBYTES + DTRU_SHAREDKEYBYTES);
   
  for (i = 0; i < DTRU_SHAREDKEYBYTES; ++i)
    ss[i] = buf[i] ^ ((-fail) & (buf[i] ^ buf2[i]));
  *ss_byts = DTRU_SHAREDKEYBYTES;
  return fail;
}

int kem_keygen_avx2(unsigned char * pk, unsigned long long * pk_byts, unsigned char * sk, unsigned long long * sk_byts)
{
  return kem_keygen(pk, pk_byts, sk, sk_byts);
}

int kem_enc_avx2(unsigned char * pk, unsigned long long pk_byts,
unsigned char * ss, unsigned long long * ss_byts,
unsigned char * ct, unsigned long long * ct_byts)
{
  return kem_enc(pk, pk_byts, ss, ss_byts, ct, ct_byts);
}

int kem_dec_avx2(
unsigned char * sk, unsigned long long sk_byts,
unsigned char * ct, unsigned long long ct_byts,
unsigned char * ss, unsigned long long * ss_byts)
{
  return kem_dec(sk, sk_byts, ct, ct_byts, ss, ss_byts);
}
