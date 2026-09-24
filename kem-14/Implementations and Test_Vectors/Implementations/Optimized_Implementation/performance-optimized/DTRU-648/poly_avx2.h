#ifndef POLY_AVX2_H
#define POLY_AVX2_H

#include "poly.h"

void poly_freeze_avx2_impl(poly *a);
void poly_add_avx2_impl(poly *c, const poly *a, const poly *b);
void poly_multi_p_avx2_impl(poly *b, const poly *a);
#endif
