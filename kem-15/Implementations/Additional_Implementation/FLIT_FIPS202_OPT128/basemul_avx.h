#ifndef BASEMUL_AVX_H
#define BASEMUL_AVX_H

#include <stdint.h>
#include "params.h"

#define poly_basemul_montgomery_avx KEM_NAMESPACE(_poly_basemul_montgomery_avx)
void poly_basemul_montgomery_avx(int16_t r[N], const int16_t a[N], const int16_t b[N], const int16_t *qdata);

#define poly_baseinv_avx KEM_NAMESPACE(_poly_baseinv_avx)
int poly_baseinv_avx(int16_t b[N], const int16_t a[N], const int16_t *qdata);

#endif
