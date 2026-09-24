/**
 * @file fft.h
 * @brief Header file of fft.c
 */

#ifndef MITO_FFT_H
#define MITO_FFT_H

#include <stdint.h>
#include <string.h>

void fft(uint16_t *w, const uint16_t *f, size_t f_coeffs);
void fft_retrieve_error_poly(uint8_t *error, const uint16_t *w);

#endif  // MITO_FFT_H
