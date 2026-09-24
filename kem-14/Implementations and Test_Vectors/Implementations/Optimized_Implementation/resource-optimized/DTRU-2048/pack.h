#ifndef PACK_H
#define PACK_H

#include <stdint.h>
#include "poly.h"
#include "params.h"

void pack_pk(unsigned char *r, const poly *a);
void unpack_pk(poly *r, const unsigned char *a);

void pack_sk(unsigned char *r, const poly *a);
void unpack_sk(poly *r, const unsigned char *a);

void pack_ct(unsigned char *r, const poly *a);
void unpack_decompress_ct(poly *r, const unsigned char *a);

/* AVX2 implementations */
void pack_sk_avx2(unsigned char *r, const poly *a);
void unpack_sk_avx2(poly *r, const unsigned char *a);

/* 12-bit 打包（PK_PACK_OPT=0），输出 DTRU_N * DTRU_LOGQ / 8 = 3072 bytes。
 * 与 PK_PACK_OPT=1 的熵编码版本（3012 bytes）不兼容。 */
void pack_pk_avx2(unsigned char *r, const poly *a);
void unpack_pk_avx2(poly *r, const unsigned char *a);

#endif