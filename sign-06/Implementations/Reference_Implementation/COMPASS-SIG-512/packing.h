#ifndef PACKING_H
#define PACKING_H

#include <stdint.h>
#include "params.h"
#include "polyvec.h"

#define pack_pk COMPASS_SIG_NAMESPACE(pack_pk)
void pack_pk(uint8_t pk[CRYPTO_PUBLICKEYBYTES], const uint8_t rho[SEEDBYTES], const polyveck *t1);

#define pack_sk COMPASS_SIG_NAMESPACE(pack_sk)
void pack_sk(uint8_t sk[CRYPTO_SECRETKEYBYTES],
             const uint8_t rho[SEEDBYTES],
             const uint8_t tr[TRBYTES],
             const uint8_t key[SEEDBYTES],
             const polyvecl *s1,
             const polyveck *s2,
             const polyveck *t0);

#define pack_sig COMPASS_SIG_NAMESPACE(pack_sig)
void pack_sig(uint8_t sig[CRYPTO_BYTES],
              const uint8_t c[CTILDEBYTES],
              const polyvecl *z);

#define unpack_pk COMPASS_SIG_NAMESPACE(unpack_pk)
void unpack_pk(uint8_t rho[SEEDBYTES], polyveck *t1, const uint8_t pk[CRYPTO_PUBLICKEYBYTES]);

#define unpack_sk COMPASS_SIG_NAMESPACE(unpack_sk)
void unpack_sk(uint8_t rho[SEEDBYTES],
               uint8_t tr[TRBYTES],
               uint8_t key[SEEDBYTES],
               polyvecl *s1,
               polyveck *s2,
               polyveck *t0,
               const uint8_t sk[CRYPTO_SECRETKEYBYTES]);

#define unpack_sig COMPASS_SIG_NAMESPACE(unpack_sig)
int unpack_sig(uint8_t c[CTILDEBYTES],
               polyvecl *z,
               const uint8_t sig[CRYPTO_BYTES]);

#endif
