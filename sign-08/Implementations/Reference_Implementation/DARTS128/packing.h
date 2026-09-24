#ifndef SIGN_PACKING_H
#define SIGN_PACKING_H

#include "params.h"
#include <stdint.h>
#include "polyvec.h"

// 公钥的封装、解封装
#define pack_pk DARTS_NAMESPACE(pack_pk)
void pack_pk(uint8_t pk[CRYPTO_PUBLICKEYBYTES], 
             polyveck *A0, 
             const uint8_t seed[SEEDBYTES]);
#define unpack_pk DARTS_NAMESPACE(unpack_pk)
void unpack_pk(polyveck *A0, 
               uint8_t seed[SEEDBYTES], 
               const uint8_t pk[CRYPTO_PUBLICKEYBYTES]);

// 私钥的封装、解封装
#define pack_sk DARTS_NAMESPACE(pack_sk)
void pack_sk(uint8_t sk[CRYPTO_SECRETKEYBYTES], 
             const uint8_t pk[CRYPTO_PUBLICKEYBYTES], 
             const poly *s0,
             const polyvecl_1 *s1, 
             const polyveck *e, 
             const uint8_t key[SEEDBYTES]);
#define unpack_sk DARTS_NAMESPACE(unpack_sk)
void unpack_sk(polyvecl A[K], // 这里的 A = (A0 | A1) 为 k×l 维矩阵
               poly *s0,
               polyvecl_1 *s,
               polyveck *e, 
               uint8_t *key, 
               const uint8_t sk[CRYPTO_SECRETKEYBYTES]);

// 签名的封装、解封装
#define pack_sig DARTS_NAMESPACE(pack_sig)
int pack_sig(uint8_t sig[CRYPTO_SIGNATUREBYTES], 
             const poly *c, 
             const polyvecl *lowbits_z1, 
             const polyvecl *highbits_z1, 
             const polyveck *h);
#define unpack_sig DARTS_NAMESPACE(unpack_sig)
int unpack_sig(poly *c, 
               polyvecl *highbits_z1, 
               polyvecl *lowbits_z1, 
               polyveck *h, 
               const uint8_t sig[CRYPTO_SIGNATUREBYTES]);

#endif