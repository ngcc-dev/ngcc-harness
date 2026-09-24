/*
Copyright (c) 2026 Ying Liu.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
File Description: Declares AVX2 NTT arithmetic routines for the optimized POLARLAC-512 instance.
*/

#ifndef POLARLAC_NTT_AVX2_H
#define POLARLAC_NTT_AVX2_H

#include <stdint.h>

int NTT_AVX2(int16_t *a);
int INTT_AVX2_Lazy(int16_t *a);
int mul_mod(int16_t *a, int16_t *b, int16_t *c);
int lazyincom_mul_mod_avx2(int16_t *a, int16_t *b, int16_t *c);
int incom_mul_add2_avx2(int16_t *out,
                        const int16_t *a0, const int16_t *b0,
                        const int16_t *a1, const int16_t *b1);
int poly_mul_ntt_avx2_lazy(const uint16_t *a, const uint16_t *s, uint16_t *b);

#endif
