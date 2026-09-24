#ifndef FFT__H
#define FFT__H

#include "poly.h"
#include "params.h"
#include <stdint.h>

#if SIGN_MODE == 128 || SIGN_MODE == 256
#define FFT_N 512
#define FFT_LOGN 9
#elif SIGN_MODE == 512
#define FFT_N 1024
#define FFT_LOGN 10
#endif

#define FFT_NAMESPACE(s) sign_fft_##s

typedef struct {
    int32_t real;
    int32_t imag;
} complex_fp32_16;

#define fft_init_and_bitrev FFT_NAMESPACE(ff1_init_and_bitrev)
void fft_init_and_bitrev(complex_fp32_16 r[N], const poly *x);

#define fft FFT_NAMESPACE(fft)
void fft(complex_fp32_16 data[N]); 

#define complex_fp_sqabs FFT_NAMESPACE(complex_fp_sqabs)
int64_t complex_fp_sqabs(complex_fp32_16 x); // 计算复数的平方模

#endif