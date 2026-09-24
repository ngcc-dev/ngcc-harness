#ifndef POLY_H
#define POLY_H

#include <stdint.h>
#include "params.h"

typedef struct
{
  int16_t coeffs[DTRU_N];
} poly;

void poly_fqcsubq(poly *a);
void poly_add(poly *c, const poly *a, const poly *b);
void poly_multi_p(poly *b, const poly *a);

void poly_sample_keygen_f(poly *a, const unsigned char *buf);
void poly_sample_keygen_g(poly *a, const unsigned char *buf);
void poly_sample_enc_r(poly *a, const unsigned char *buf);
void poly_sample_enc_e(poly *a, const unsigned char *buf);

void poly_inverse(poly *b, const poly *a);
void poly_inverse_opt(poly *b, const poly *a);

void poly_encode_compress(poly *c, const poly *sigma, const unsigned char *msg);
void poly_decode(unsigned char *msg, const poly *cf);

#endif
