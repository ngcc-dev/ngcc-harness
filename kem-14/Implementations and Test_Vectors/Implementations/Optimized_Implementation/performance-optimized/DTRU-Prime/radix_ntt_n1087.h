#ifndef RADIX_NTT_N1087_H
#define RADIX_NTT_N1087_H

#include <stdint.h>
#include "poly.h"
#include "align.h"

#define N_N1087 2304 // 2**8 * 3**2
#define Q_N1087 16776961 // 2^24 - 2^8 + 1
#define QINV_N1087 65793 // q^(-1) mod 2^32

typedef ALIGNED_INT32(N_N1087) nttpoly_n1087;

void poly_radix_ntt_n1087(poly *c, const poly *a, const poly *b);

void poly_extend(nttpoly_n1087 *b, const poly *a);
void poly_extract(poly *b, const nttpoly_n1087 *a);

void poly_ntt(nttpoly_n1087 *a);
void poly_invntt(nttpoly_n1087 *a);
void poly_basemul(nttpoly_n1087 *c, const nttpoly_n1087 *a, const nttpoly_n1087 *b);

int32_t pseudomersenne_reduce_single_n1087(int32_t a);
int32_t montgomery_reduce_n1087(int64_t a);

#endif
