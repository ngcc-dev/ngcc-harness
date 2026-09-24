#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "params.h"
#include "kem.h"
#include "indcpa.h"
#include "verify.h"
#include "symmetric.h"

#ifndef WEAVER_USE_SHAKE
#include "drng.h"
extern DRNG_ctx drng_algorithm;
int randombytes(unsigned char *x, unsigned long long xlen)
{
  return get_random_number(&drng_algorithm, x, xlen * 8);
}
#else
#include "rng.h"
#endif

#ifdef PK_COMPRESS
#include "invq.h"
#endif

static void kem_enc_derand_fill_msg(uint8_t buf[WEAVER_INDCPA_MSGBYTES],
                                    const uint8_t coins[WEAVER_KEM_DERAND_COINBYTES])
{
  memcpy(buf, coins, WEAVER_INDCPA_MSGBYTES);
}

/*************************************************
* Name:        crypto_kem_keypair_derand
*
* Description: Generates public and private key
*              for CCA-secure Kyber key encapsulation mechanism
*
* Arguments:   - uint8_t *pk: pointer to output public key
*                (an already allocated array of WEAVER_PUBLICKEYBYTES bytes)
*              - uint8_t *sk: pointer to output private key
*                (an already allocated array of WEAVER_SECRETKEYBYTES bytes)
*              - uint8_t *coins: pointer to input randomness
*                (an already allocated array filled with 2*WEAVER_SYMBYTES random bytes)
**
* Returns 0 (success)
**************************************************/
int crypto_kem_keypair_derand(uint8_t *pk,
                              uint8_t *sk,
                              const uint8_t *coins)
{
  indcpa_keypair_derand(pk, sk, coins);
  memcpy(sk + WEAVER_INDCPA_SECRETKEYBYTES, pk, WEAVER_PUBLICKEYBYTES);
  hash_h(sk + WEAVER_SK_HPK_OFFSET, pk, WEAVER_PUBLICKEYBYTES);
  /* Value z for pseudo-random output on reject */
  memcpy(sk + WEAVER_SK_Z_OFFSET, coins + WEAVER_SYMBYTES, WEAVER_SYMBYTES);
  return 0;
}

/*************************************************
* Name:        crypto_kem_keypair
*
* Description: Generates public and private key
*              for CCA-secure Kyber key encapsulation mechanism
*
* Arguments:   - uint8_t *pk: pointer to output public key
*                (an already allocated array of WEAVER_PUBLICKEYBYTES bytes)
*              - uint8_t *sk: pointer to output private key
*                (an already allocated array of WEAVER_SECRETKEYBYTES bytes)
*
* Returns 0 (success)
**************************************************/
int crypto_kem_keypair(uint8_t *pk,
                       uint8_t *sk)
{
  uint8_t coins[2*WEAVER_SYMBYTES];
  randombytes(coins, 2*WEAVER_SYMBYTES);
  crypto_kem_keypair_derand(pk, sk, coins);
  return 0;
}

/*************************************************
* Name:        crypto_kem_enc_derand
*
* Description: Generates cipher text and shared
*              secret for given public key
*
* Arguments:   - uint8_t *ct: pointer to output cipher text
*                (an already allocated array of WEAVER_CIPHERTEXTBYTES bytes)
*              - uint8_t *ss: pointer to output shared secret
*                (an already allocated array of WEAVER_SSBYTES bytes)
*              - const uint8_t *pk: pointer to input public key
*                (an already allocated array of WEAVER_PUBLICKEYBYTES bytes)
*              - const uint8_t *coins: FO message m (WEAVER_INDCPA_MSGBYTES bytes)
**
* Returns 0 (success)
**************************************************/
int crypto_kem_enc_derand(uint8_t *ct,
                          uint8_t *ss,
                          const uint8_t *pk,
                          const uint8_t coins[WEAVER_KEM_DERAND_COINBYTES])
{
  uint8_t buf[WEAVER_INDCPA_MSGBYTES + WEAVER_HBYTES];
  /* Will contain shared-key material || encryption coins */
  uint8_t kr[WEAVER_GBYTES];

  kem_enc_derand_fill_msg(buf, coins);

  /* Multitarget countermeasure for coins + contributory KEM */
  hash_h(buf + WEAVER_INDCPA_MSGBYTES, pk, WEAVER_PUBLICKEYBYTES);
  hash_g(kr, buf, sizeof(buf));

  /* encryption coins are in kr + WEAVER_SSBYTES */
  indcpa_enc(ct, buf, pk, kr + WEAVER_SSBYTES);

  memcpy(ss, kr, WEAVER_SSBYTES);
  return 0;
}

/*************************************************
* Name:        crypto_kem_enc
*
* Description: Generates cipher text and shared
*              secret for given public key
*
* Arguments:   - uint8_t *ct: pointer to output cipher text
*                (an already allocated array of WEAVER_CIPHERTEXTBYTES bytes)
*              - uint8_t *ss: pointer to output shared secret
*                (an already allocated array of WEAVER_SSBYTES bytes)
*              - const uint8_t *pk: pointer to input public key
*                (an already allocated array of WEAVER_PUBLICKEYBYTES bytes)
*
* Returns 0 (success)
**************************************************/
int crypto_kem_enc(uint8_t *ct,
                   uint8_t *ss,
                   const uint8_t *pk)
{
  uint8_t coins[WEAVER_INDCPA_MSGBYTES]; /* coins --> used as encrypted m for PKE */
  randombytes(coins, WEAVER_INDCPA_MSGBYTES);
  crypto_kem_enc_derand(ct, ss, pk, coins);
  return 0;
}

/*************************************************
* Name:        crypto_kem_dec
*
* Description: Generates shared secret for given
*              cipher text and private key
*
* Arguments:   - uint8_t *ss: pointer to output shared secret
*                (an already allocated array of WEAVER_SSBYTES bytes)
*              - const uint8_t *ct: pointer to input cipher text
*                (an already allocated array of WEAVER_CIPHERTEXTBYTES bytes)
*              - const uint8_t *sk: pointer to input private key
*                (an already allocated array of WEAVER_SECRETKEYBYTES bytes)
*
* Returns 0.
*
* On failure, ss will contain a pseudo-random value.
**************************************************/
int crypto_kem_dec(uint8_t *ss,
                   const uint8_t *ct,
                   const uint8_t *sk)
{
  int fail;
  uint8_t buf[WEAVER_INDCPA_MSGBYTES + WEAVER_HBYTES];
  /* Will contain shared-key material || encryption coins */
  uint8_t kr[WEAVER_GBYTES];
  uint8_t cmp[WEAVER_CIPHERTEXTBYTES];
  const uint8_t *pk = sk + WEAVER_INDCPA_SECRETKEYBYTES;

  indcpa_dec(buf, ct, sk);

  /* Multitarget countermeasure for coins + contributory KEM */
  memcpy(buf + WEAVER_INDCPA_MSGBYTES, sk + WEAVER_SK_HPK_OFFSET, WEAVER_HBYTES);
  hash_g(kr, buf, sizeof(buf));

  /* encryption coins are in kr + WEAVER_SSBYTES */
  indcpa_enc(cmp, buf, pk, kr + WEAVER_SSBYTES);

  fail = verify(ct, cmp, WEAVER_CIPHERTEXTBYTES);

  /* Compute rejection key */
  rkprf(ss, sk + WEAVER_SK_Z_OFFSET, ct);

  /* Copy true key to return buffer if fail is false */
  cmov(ss, kr, WEAVER_SSBYTES, !fail);

  return 0;
}
