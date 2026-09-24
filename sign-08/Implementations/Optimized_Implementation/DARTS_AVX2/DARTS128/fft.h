#ifndef FFT__H
#define FFT__H

#include "poly.h"
#include <stdint.h>
#include <immintrin.h>


#if DARTS_MODE == 128 || DARTS_MODE == 256
#define FFT_N 512
#define FFT_LOGN 9
#elif DARTS_MODE == 512
#define FFT_N 1024
#define FFT_LOGN 10
#endif

#define FFT_NAMESPACE(s) darts_fft_##s

typedef struct {
    int32_t real;
    int32_t imag;
} complex_fp32_16;

typedef struct {
  union {
    __m256i vec[FFT_N/8];
    int32_t coeffs[FFT_N];
  } real;
  union {
    __m256i vec[FFT_N/8];
    int32_t coeffs[FFT_N];
  } imag;
} poly_complex_fp32_16;

#define fft_init_and_bitrev FFT_NAMESPACE(fft_init_and_bitrev)
void fft_init_and_bitrev(poly_complex_fp32_16 *r, const poly *x);

#define fft FFT_NAMESPACE(fft)
void fft(poly_complex_fp32_16 *data); 

#define complex_fp_sqabs_add FFT_NAMESPACE(complex_fp_sqabs_add)
void complex_fp_sqabs_add(__m256i *res, const __m256i *real, const __m256i *imag);

#endif