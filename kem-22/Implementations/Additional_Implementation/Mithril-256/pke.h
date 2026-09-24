#ifndef PKE_H
#define PKE_H

#include "parameters.h"
#include "ring.h"
#include "uniform.h"

#ifdef __cplusplus
extern "C"
{
#endif

  void poly_subp(poly *r, const poly *f, const poly *g);
  void poly_add_msg(poly *r, const unsigned char msg[RRLWR_PKE_MESSAGE_LEN]);
  uint8_t ct_cmp(const unsigned char *c1, const unsigned char *c2);

  int pke_keygen(unsigned char pk[RRLWR_PKE_PK_LEN], unsigned char sk[RRLWR_PKE_SK_LEN],
                 const unsigned char seedA[RRLWR_PKE_SEED_A_LEN], const unsigned char seedS[RRLWR_SEED_S_LEN]);
  int pke_encrypt(unsigned char ct[RRLWR_PKE_CT_LEN], const unsigned char pk[RRLWR_PKE_PK_LEN],
                  const unsigned char m[RRLWR_PKE_MESSAGE_LEN], const unsigned char seedSp[RRLWR_SEED_S_LEN]);
  int pke_decrypt(unsigned char m[RRLWR_PKE_MESSAGE_LEN], 
                  const unsigned char ct[RRLWR_PKE_CT_LEN], const unsigned char sk[RRLWR_PKE_SK_LEN]);

#ifdef __cplusplus
}
#endif

#endif