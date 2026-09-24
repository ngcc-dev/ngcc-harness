#ifndef API_H
#define API_H

#include "params.h"

#define CRYPTO_SECRETKEYBYTES  WEAVER_SECRETKEYBYTES
#define CRYPTO_PUBLICKEYBYTES  WEAVER_PUBLICKEYBYTES
#define CRYPTO_CIPHERTEXTBYTES WEAVER_CIPHERTEXTBYTES
#define CRYPTO_BYTES           WEAVER_SSBYTES

#if   (WEAVER_MODE == 1)
#ifdef WEAVER_USE_SHAKE
  #define CRYPTO_ALGNAME "WEAVER-640-SHAKE"
#else
  #define CRYPTO_ALGNAME "WEAVER-640"
#endif
#elif (WEAVER_MODE == 3)
#ifdef WEAVER_USE_SHAKE
  #define CRYPTO_ALGNAME "WEAVER-1024-SHAKE"
#else
  #define CRYPTO_ALGNAME "WEAVER-1024"
#endif
#elif (WEAVER_MODE == 5)
#ifdef WEAVER_USE_SHAKE
  #define CRYPTO_ALGNAME "WEAVER-2048-SHAKE"
#else
  #define CRYPTO_ALGNAME "WEAVER-2048"
#endif
#else
  #error "WEAVER_MODE must be in {1,3,5}"
#endif

#define crypto_kem_keypair_derand WEAVER_NAMESPACE(_keypair_derand)
int crypto_kem_keypair_derand(unsigned char *pk, unsigned char *sk, const unsigned char *coins);

#define crypto_kem_enc_derand WEAVER_NAMESPACE(_enc_derand)
int crypto_kem_enc_derand(unsigned char *ct, unsigned char *ss, const unsigned char *pk,
                          const unsigned char coins[WEAVER_KEM_DERAND_COINBYTES]);

#define crypto_kem_dec WEAVER_NAMESPACE(_dec)
int crypto_kem_dec(unsigned char *ss,
                   const unsigned char *ct,
                   const unsigned char *sk);

#ifndef WEAVER_USE_SHAKE
int randombytes(unsigned char *x, unsigned long long xlen);
#endif

#endif
