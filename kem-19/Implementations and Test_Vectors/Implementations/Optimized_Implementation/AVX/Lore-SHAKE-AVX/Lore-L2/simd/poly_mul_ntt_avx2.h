#ifndef POLY_MUL_NTT_AVX2_H
#define POLY_MUL_NTT_AVX2_H

#ifdef LORE_USE_AVX2_POLYMUL_NTT
#include <stdint.h>

void poly_mul_ntt_avx2(int16_t r[512], const int16_t a[512], const int16_t b[512]);

#endif
#endif
