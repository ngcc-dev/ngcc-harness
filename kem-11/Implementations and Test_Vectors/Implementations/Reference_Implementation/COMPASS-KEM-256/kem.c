#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "params.h"
#include "kem.h"
#include "indcpa.h"
#include "verify.h"
#include "symmetric.h"
#include "drng.h"
extern DRNG_ctx drng_algorithm;
/*************************************************
* Name:        crypto_kem_keypair_derand
*
* Description: Generates public and private key
*              for CCA-secure COMPASS_KEM key encapsulation mechanism
*
* Arguments:   - uint8_t *pk: pointer to output public key
*                (an already allocated array of COMPASS_KEM_PUBLICKEYBYTES bytes)
*              - uint8_t *sk: pointer to output private key
*                (an already allocated array of COMPASS_KEM_SECRETKEYBYTES bytes)
*              - uint8_t *coins: pointer to input randomness
*                (an already allocated array filled with 2*COMPASS_KEM_SYMBYTES random bytes)
**
* Returns 0 (success)
**************************************************/
int crypto_kem_keypair_derand(uint8_t *pk,
                              uint8_t *sk,
                              const uint8_t *coins)
{
  indcpa_keypair_derand(pk, sk, coins);
  memcpy(sk+COMPASS_KEM_INDCPA_SECRETKEYBYTES, pk, COMPASS_KEM_PUBLICKEYBYTES);
  hash_h(sk+COMPASS_KEM_SECRETKEYBYTES-2*COMPASS_KEM_SYMBYTES, pk, COMPASS_KEM_PUBLICKEYBYTES);
  /* Value z for pseudo-random output on reject */
  memcpy(sk+COMPASS_KEM_SECRETKEYBYTES-COMPASS_KEM_SYMBYTES, coins+COMPASS_KEM_SYMBYTES, COMPASS_KEM_SYMBYTES);
  return 0;
}

/*************************************************
* Name:        crypto_kem_keypair
*
* Description: Generates public and private key
*              for CCA-secure COMPASS_KEM key encapsulation mechanism
*
* Arguments:   - uint8_t *pk: pointer to output public key
*                (an already allocated array of COMPASS_KEM_PUBLICKEYBYTES bytes)
*              - uint8_t *sk: pointer to output private key
*                (an already allocated array of COMPASS_KEM_SECRETKEYBYTES bytes)
*
* Returns 0 (success)
**************************************************/
int crypto_kem_keypair(uint8_t *pk,
                       uint8_t *sk)
{
  uint8_t coins[2*COMPASS_KEM_SYMBYTES];
  get_random_number(&drng_algorithm, coins, 2 * COMPASS_KEM_SYMBYTES * 8);
  // randombytes(coins, 2*COMPASS_KEM_SYMBYTES);
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
*                (an already allocated array of COMPASS_KEM_CIPHERTEXTBYTES bytes)
*              - uint8_t *ss: pointer to output shared secret
*                (an already allocated array of COMPASS_KEM_SSBYTES bytes)
*              - const uint8_t *pk: pointer to input public key
*                (an already allocated array of COMPASS_KEM_PUBLICKEYBYTES bytes)
*              - const uint8_t *coins: pointer to input randomness
*                (an already allocated array filled with COMPASS_KEM_SYMBYTES random bytes)
**
* Returns 0 (success)
**************************************************/
int crypto_kem_enc_derand(uint8_t *ct,
                          uint8_t *ss,
                          const uint8_t *pk,
                          const uint8_t *coins)
{
  uint8_t buf[2*COMPASS_KEM_SYMBYTES];
  /* Will contain key, coins */
  uint8_t kr[2*COMPASS_KEM_SYMBYTES];

  memcpy(buf, coins, COMPASS_KEM_SYMBYTES);

  /* Multitarget countermeasure for coins + contributory KEM */
  hash_h(buf+COMPASS_KEM_SYMBYTES, pk, COMPASS_KEM_PUBLICKEYBYTES);
  hash_g(kr, buf, 2*COMPASS_KEM_SYMBYTES);

  /* coins are in kr+COMPASS_KEM_SYMBYTES */
  indcpa_enc(ct, buf, pk, kr+COMPASS_KEM_SYMBYTES);

  memcpy(ss,kr,COMPASS_KEM_SYMBYTES);
  return 0;
}

/*************************************************
* Name:        crypto_kem_enc
*
* Description: Generates cipher text and shared
*              secret for given public key
*
* Arguments:   - uint8_t *ct: pointer to output cipher text
*                (an already allocated array of COMPASS_KEM_CIPHERTEXTBYTES bytes)
*              - uint8_t *ss: pointer to output shared secret
*                (an already allocated array of COMPASS_KEM_SSBYTES bytes)
*              - const uint8_t *pk: pointer to input public key
*                (an already allocated array of COMPASS_KEM_PUBLICKEYBYTES bytes)
*
* Returns 0 (success)
**************************************************/
int crypto_kem_enc(uint8_t *ct,
                   uint8_t *ss,
                   const uint8_t *pk)
{
  uint8_t coins[COMPASS_KEM_SYMBYTES];
  get_random_number(&drng_algorithm, coins, COMPASS_KEM_SYMBYTES * 8);
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
*                (an already allocated array of COMPASS_KEM_SSBYTES bytes)
*              - const uint8_t *ct: pointer to input cipher text
*                (an already allocated array of COMPASS_KEM_CIPHERTEXTBYTES bytes)
*              - const uint8_t *sk: pointer to input private key
*                (an already allocated array of COMPASS_KEM_SECRETKEYBYTES bytes)
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
  uint8_t buf[2*COMPASS_KEM_SYMBYTES];
  /* Will contain key, coins */
  uint8_t kr[2*COMPASS_KEM_SYMBYTES];
//  uint8_t cmp[COMPASS_KEM_CIPHERTEXTBYTES+COMPASS_KEM_SYMBYTES];
  uint8_t cmp[COMPASS_KEM_CIPHERTEXTBYTES];
  const uint8_t *pk = sk+COMPASS_KEM_INDCPA_SECRETKEYBYTES;

  indcpa_dec(buf, ct, sk);

  /* Multitarget countermeasure for coins + contributory KEM */
  memcpy(buf+COMPASS_KEM_SYMBYTES, sk+COMPASS_KEM_SECRETKEYBYTES-2*COMPASS_KEM_SYMBYTES, COMPASS_KEM_SYMBYTES);
  hash_g(kr, buf, 2*COMPASS_KEM_SYMBYTES);

  /* coins are in kr+COMPASS_KEM_SYMBYTES */
  indcpa_enc(cmp, buf, pk, kr+COMPASS_KEM_SYMBYTES);

  fail = verify(ct, cmp, COMPASS_KEM_CIPHERTEXTBYTES);

  /* Compute rejection key */
  rkprf(ss,sk+COMPASS_KEM_SECRETKEYBYTES-COMPASS_KEM_SYMBYTES,ct);

  /* Copy true key to return buffer if fail is false */
  cmov(ss,kr,COMPASS_KEM_SYMBYTES,!fail);

  return 0;
}
