#ifndef YUANYANG_512_KEYGEN_FFT_H
#define YUANYANG_512_KEYGEN_FFT_H

#include <stddef.h>
#include <stdint.h>

#include "yuanyang_inner.h"

typedef struct {
	double re;
	double im;
} yykg_cplx;

static inline yykg_cplx
yykg_c_make(double re, double im)
{
	yykg_cplx z;

	z.re = re;
	z.im = im;
	return z;
}

static inline yykg_cplx
yykg_c_add(yykg_cplx a, yykg_cplx b)
{
	return yykg_c_make(a.re + b.re, a.im + b.im);
}

static inline yykg_cplx
yykg_c_sub(yykg_cplx a, yykg_cplx b)
{
	return yykg_c_make(a.re - b.re, a.im - b.im);
}

static inline yykg_cplx
yykg_c_neg(yykg_cplx a)
{
	return yykg_c_make(-a.re, -a.im);
}

static inline yykg_cplx
yykg_c_conj(yykg_cplx a)
{
	return yykg_c_make(a.re, -a.im);
}

static inline yykg_cplx
yykg_c_mul(yykg_cplx a, yykg_cplx b)
{
	return yykg_c_make(
		a.re * b.re - a.im * b.im,
		a.re * b.im + a.im * b.re);
}

static inline yykg_cplx
yykg_c_div_real(yykg_cplx a, double den)
{
	return yykg_c_make(a.re / den, a.im / den);
}

static inline double
yykg_c_abs2(yykg_cplx a)
{
	return a.re * a.re + a.im * a.im;
}

static inline yykg_cplx
yykg_fft_get(const double a[YUANYANG_D], size_t u)
{
	size_t hn;

	hn = YUANYANG_D >> 1;
	return yykg_c_make(a[u], a[u + hn]);
}

static inline void
yykg_fft_set(double a[YUANYANG_D], size_t u, yykg_cplx z)
{
	size_t hn;

	hn = YUANYANG_D >> 1;
	a[u] = z.re;
	a[u + hn] = z.im;
}

void yykg_fft(double *f, unsigned logn);
void yykg_ifft(double *f, unsigned logn);

void yykg_poly_split_fft(
	double *restrict f0, double *restrict f1,
	const double *restrict f, unsigned logn);

void yykg_poly_to_fft_i8(
	double dst[YUANYANG_D], const int8_t src[YUANYANG_D]);
void yykg_poly_to_fft_double(
	double dst[YUANYANG_D], const double src[YUANYANG_D]);
void yykg_poly_from_fft(
	double dst[YUANYANG_D], const double src_fft[YUANYANG_D]);

#endif
