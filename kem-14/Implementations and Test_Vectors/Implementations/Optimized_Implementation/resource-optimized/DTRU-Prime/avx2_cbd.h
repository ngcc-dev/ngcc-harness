#ifndef CBD_AVX2_H
#define CBD_AVX2_H

#include <stdint.h>
#include <immintrin.h>
#include <string.h>
#include "params.h"
#include "poly.h"

void cbd1_avx2_intrinsic(poly *r, const uint8_t buf[DTRU_CBD1_BYTES]);
void cbd2_avx2_intrinsic(poly *r, const uint8_t buf[DTRU_CBD2_BYTES]);
void cbd1_avx2_fast(poly *r, const uint8_t buf[DTRU_CBD1_BYTES]);
void cbd2_avx2_fast(poly *r, const uint8_t buf[DTRU_CBD2_BYTES]);
#endif
