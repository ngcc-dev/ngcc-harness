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

#endif