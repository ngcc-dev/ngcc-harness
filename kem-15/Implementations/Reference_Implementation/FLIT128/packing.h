#ifndef KEM_PACKING_H
#define KEM_PACKING_H

#include "params.h"
#include <stdint.h>
#include "poly.h"
#include "decode.h"

// 公钥的封装、解封装
#define pack_pk KEM_NAMESPACE(pack_pk)
void pack_pk(uint8_t pk[KEM_CPAPKE_PUBLICKEYBYTES], const poly *h);
#define unpack_pk KEM_NAMESPACE(unpack_pk)
void unpack_pk(poly *h, const uint8_t pk[KEM_CPAPKE_PUBLICKEYBYTES]);

// 私钥的封装、解封装
#define pack_sk KEM_NAMESPACE(pack_sk)
void pack_sk(uint8_t sk[KEM_CPAPKE_SECRETKEYBYTES], const poly *f, const poly_f1 *f1);
#define unpack_sk KEM_NAMESPACE(unpack_sk)
void unpack_sk(poly *f, poly_f1 *f1, const uint8_t sk[KEM_CPAPKE_SECRETKEYBYTES]);

#endif