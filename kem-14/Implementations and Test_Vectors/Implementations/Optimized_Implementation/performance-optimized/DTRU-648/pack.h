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

void pack_sk_avx2(unsigned char *r, const poly *a);
void unpack_sk_avx2(poly *r, const unsigned char *a);

/* 12-bit PK format only: use these helpers with PK_PACK_OPT=0. */
void pack_pk_avx2(unsigned char *r, const poly *a);
void unpack_pk_avx2(poly *r, const unsigned char *a);

#endif
