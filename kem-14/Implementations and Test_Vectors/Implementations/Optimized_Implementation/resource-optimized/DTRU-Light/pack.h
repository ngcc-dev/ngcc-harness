#ifndef PACK_H
#define PACK_H

#include <stdint.h>
#include "poly.h"
#include "params.h"
#include <immintrin.h>
#include <string.h>

void pack_pk(unsigned char *r, const poly *a);
void unpack_pk(poly *r, const unsigned char *a);

void pack_sk(unsigned char *r, const poly *a);
void unpack_sk(poly *r, const unsigned char *a);

// void pack_sk_f(unsigned char *r, const poly *a);
// void unpack_sk_f(poly *r, const unsigned char *a);

void pack_ct(unsigned char *r, const poly *a);
void unpack_decompress_ct(poly *r, const unsigned char *a);

void pack_pk_avx(unsigned char *r, const poly *a);
void unpack_pk_avx(poly *r, const unsigned char *a);
void pack_sk_avx(unsigned char *r, const poly *a);
void unpack_sk_avx(poly *r, const unsigned char *a);
void pack_ct_avx(unsigned char *r, const poly *a);
void unpack_decompress_ct_avx(poly *r, const unsigned char *a);

#endif