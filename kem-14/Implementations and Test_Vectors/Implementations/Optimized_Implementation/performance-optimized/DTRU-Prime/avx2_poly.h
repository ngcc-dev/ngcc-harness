#ifndef AVX2_POLY_H
#define AVX2_POLY_H

#include <immintrin.h>
#include "poly.h"

void poly_fqcsubq_avx2(poly *a);
void poly_add_avx2(poly *c, const poly *a, const poly *b);
void poly_multi_p_avx2(poly *b, const poly *a);

void poly_inverse_avx2(poly *b, const poly *a);

void poly_sample_keygen_f_avx2(poly *a, const unsigned char *buf);
void poly_sample_keygen_g_avx2(poly *a, const unsigned char *buf);
void poly_sample_enc_r_avx2(poly *a, const unsigned char *buf);
void poly_sample_enc_e_avx2(poly *a, const unsigned char *buf);

#endif
