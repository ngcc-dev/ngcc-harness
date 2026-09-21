#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "params.h"
#include "kem.h"
#include "indcpa.h"
#include "verify.h"
#include "symmetric.h"
//#include "randombytes.h"
#ifndef WEAVER_USE_SHAKE
// USE SM3:
#include "drng.h"
extern DRNG_ctx drng_algorithm;
#else
#include "rng.h"
#endif


#ifdef PK_COMPRESS
#include "invq.h"
#endif

static void compute_ciphertext_tag(uint8_t tag[WEAVER_TAGBYTES],
                                   const uint8_t ss[WEAVER_SSBYTES],
                                   const uint8_t body[WEAVER_CIPHERTEXTBODYBYTES])
{
  uint8_t input[WEAVER_SSBYTES + WEAVER_CIPHERTEXTBODYBYTES];

  memcpy(input, ss, WEAVER_SSBYTES);
  memcpy(input + WEAVER_SSBYTES, body, WEAVER_CIPHERTEXTBODYBYTES);
  shake256(tag, WEAVER_TAGBYTES, input, sizeof(input));
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
  indcpa_keypair_derand(pk, sk, coins); // no longer need pkhash
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
  uint8_t coins[WEAVER_SYMBYTES];
  randombytes(coins, WEAVER_SYMBYTES);
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
  uint8_t buf[WEAVER_INDCPA_MSGBYTES];
  uint8_t *ct_body = ct;
  uint8_t *ct_tag = ct + WEAVER_CIPHERTEXTBODYBYTES;

  memcpy(buf, coins, WEAVER_INDCPA_MSGBYTES); // no longer need pkhash
  /* encryption coins are in coins + WEAVER_INDCPA_MSGBYTES */
  indcpa_enc(ct_body, buf, pk, coins + WEAVER_INDCPA_MSGBYTES);
  compute_ciphertext_tag(ct_tag, buf, ct_body);
  shake256(ss, WEAVER_SSBYTES, buf, sizeof(buf));
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
  uint8_t coins[WEAVER_KEM_DERAND_COINBYTES]; /* coins --> used as encrypted (m,r) for PKE */
  randombytes(coins, WEAVER_KEM_DERAND_COINBYTES);
  crypto_kem_enc_derand(ct, ss, pk, coins);
  return 0;
}

/*************************************************
* Name:        crypto_kem_dec_rigid
*
* Description: Rigid KEM.Decap for AKE.AuthInit (PDF Algorithm 4):
*              ciphertext-tag check; on failure return -1 and do not
*              output the real shared secret (IND-CPAF / ⊥ semantics).
*
* Arguments:   - uint8_t *ss: output shared secret (WEAVER_SSBYTES)
*              - const uint8_t *ct: ciphertext (WEAVER_CIPHERTEXTBYTES)
*              - const uint8_t *sk: secret key (WEAVER_SECRETKEYBYTES)
*
* Returns 0 on success, -1 on decapsulation failure.
**************************************************/
int crypto_kem_dec_rigid(uint8_t *ss,
                         const uint8_t *ct,
                         const uint8_t *sk)
{
  int fail;
  uint8_t buf[WEAVER_INDCPA_MSGBYTES];
  uint8_t expected_tag[WEAVER_TAGBYTES];
  const uint8_t *ct_body = ct;
  const uint8_t *ct_tag = ct + WEAVER_CIPHERTEXTBODYBYTES;

  indcpa_dec(buf, ct_body, sk); // do not need pkhash any more.
  compute_ciphertext_tag(expected_tag, buf, ct_body);
  fail = verify(ct_tag, expected_tag, WEAVER_TAGBYTES);
  if(fail)
    return -1;

  shake256(ss, WEAVER_SSBYTES, buf, sizeof(buf));
  return 0;
}
