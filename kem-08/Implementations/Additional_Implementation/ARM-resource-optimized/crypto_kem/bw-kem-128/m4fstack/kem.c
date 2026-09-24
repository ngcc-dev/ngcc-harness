#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "params.h"
#include "kem.h"
#include "indcpa.h"
#include "verify.h"
#include "symmetric.h"
#include "randombytes.h"
int crypto_kem_keypair_derand(uint8_t *pk,
                              uint8_t *sk,
                              const uint8_t *coins)
{
  indcpa_keypair_derand(pk, sk, coins);
  memcpy(sk+KYBER_INDCPA_SECRETKEYBYTES, pk, KYBER_PUBLICKEYBYTES);

  // hash_h(sk+KYBER_SECRETKEYBYTES-2*KYBER_SYMBYTES, pk, KYBER_PUBLICKEYBYTES);
  memcpy(sk+KYBER_SECRETKEYBYTES-PREFIXHASHBYTES-KYBER_SYMBYTES,pk,PREFIXHASHBYTES);

  /* Value z for pseudo-random output on reject */
  memcpy(sk+KYBER_SECRETKEYBYTES-KYBER_SYMBYTES, coins+KYBER_SYMBYTES, KYBER_SYMBYTES);
  return 0;
}

int crypto_kem_keypair(uint8_t *pk,
                       uint8_t *sk)
{
  uint8_t coins[2*KYBER_SYMBYTES];
  randombytes(coins, 2*KYBER_SYMBYTES);
  crypto_kem_keypair_derand(pk, sk, coins);
  return 0;
}

int crypto_kem_enc_derand(uint8_t *ct,
                          uint8_t *ss,
                          const uint8_t *pk,
                          const uint8_t *coins)
{
  uint8_t buf[KYBER_INDCPA_MSGBYTES+PREFIXHASHBYTES];
  /* kr = K || r with K = kr[0..15] and r = kr[16..31]. */
  uint8_t kr[2*KYBER_SYMBYTES];

  memcpy(buf, coins, KYBER_INDCPA_MSGBYTES);

  /* Multitarget countermeasure for coins + contributory KEM */
  // hash_h(buf+KYBER_SYMBYTES, pk, KYBER_PUBLICKEYBYTES);
  memcpy(buf+KYBER_INDCPA_MSGBYTES,pk,PREFIXHASHBYTES);
  hash_g(kr, buf, KYBER_INDCPA_MSGBYTES+PREFIXHASHBYTES);

  /* r occupies kr + KYBER_SYMBYTES. */
  indcpa_enc(ct, buf, pk, kr+KYBER_SYMBYTES);

  memcpy(ss,kr,KYBER_SSBYTES);
  return 0;
}

int crypto_kem_enc(uint8_t *ct,
                   uint8_t *ss,
                   const uint8_t *pk)
{
  uint8_t coins[KYBER_INDCPA_MSGBYTES];
  randombytes(coins, KYBER_INDCPA_MSGBYTES);
  crypto_kem_enc_derand(ct, ss, pk, coins);
  return 0;
}

int crypto_kem_dec(uint8_t *ss,
                   const uint8_t *ct,
                   const uint8_t *sk)
{
  int fail;
  uint8_t buf[KYBER_INDCPA_MSGBYTES+PREFIXHASHBYTES];
  /* kr = K || r with K = kr[0..15] and r = kr[16..31]. */
  uint8_t kr[2*KYBER_SYMBYTES];
  const uint8_t *pk = sk+KYBER_INDCPA_SECRETKEYBYTES;

  indcpa_dec(buf, ct, sk);

  /* Multitarget countermeasure for coins + contributory KEM */
  memcpy(buf+KYBER_INDCPA_MSGBYTES, sk+KYBER_SECRETKEYBYTES-PREFIXHASHBYTES-KYBER_SYMBYTES, PREFIXHASHBYTES);
  hash_g(kr, buf, KYBER_INDCPA_MSGBYTES+PREFIXHASHBYTES);

  /* r occupies kr + KYBER_SYMBYTES. */
  fail = indcpa_enc_cmp(ct, buf, pk, kr+KYBER_SYMBYTES);

  /* Compute rejection key */
  rkprf(ss,sk+KYBER_SECRETKEYBYTES-KYBER_SYMBYTES,ct);

  /* Copy true key to return buffer if fail is false */
  cmov(ss,kr,KYBER_SSBYTES,!fail);

  return 0;
}
