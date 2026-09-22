#ifndef YUANYANG_1024_FFT_UTILS_H
#define YUANYANG_1024_FFT_UTILS_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "yuanyang_inner.h"

typedef struct {
	fpr re;
	fpr im;
} yuanyang_cplx;

static inline yuanyang_cplx
yuanyang_c_make(fpr re, fpr im)
{
	yuanyang_cplx z;

	z.re = re;
	z.im = im;
	return z;
}

static inline yuanyang_cplx
yuanyang_c_add(yuanyang_cplx a, yuanyang_cplx b)
{
	return yuanyang_c_make(fpr_add(a.re, b.re), fpr_add(a.im, b.im));
}

static inline yuanyang_cplx
yuanyang_c_sub(yuanyang_cplx a, yuanyang_cplx b)
{
	return yuanyang_c_make(fpr_sub(a.re, b.re), fpr_sub(a.im, b.im));
}

static inline yuanyang_cplx
yuanyang_c_neg(yuanyang_cplx a)
{
	return yuanyang_c_make(fpr_neg(a.re), fpr_neg(a.im));
}

static inline yuanyang_cplx
yuanyang_c_conj(yuanyang_cplx a)
{
	return yuanyang_c_make(a.re, fpr_neg(a.im));
}

/*
 * Same packed-complex multiplication formula as Falcon's FPC_MUL macro,
 * exposed here as a yuanyang-local helper usable across translation units.
 */
static inline yuanyang_cplx
yuanyang_c_mul(yuanyang_cplx a, yuanyang_cplx b)
{
	fpr rr, ii, ri;

	rr = fpr_mul(a.re, b.re);
	ii = fpr_mul(a.im, b.im);
	ri = fpr_sub(
		fpr_mul(fpr_add(a.re, a.im), fpr_add(b.re, b.im)),
		fpr_add(rr, ii));
	return yuanyang_c_make(fpr_sub(rr, ii), ri);
}

static inline yuanyang_cplx
yuanyang_c_div_real(yuanyang_cplx a, fpr den)
{
	fpr inv;

	inv = fpr_inv(den);
	return yuanyang_c_make(fpr_mul(a.re, inv), fpr_mul(a.im, inv));
}

static inline yuanyang_cplx
yuanyang_c_scale(yuanyang_cplx a, fpr scalar)
{
	return yuanyang_c_make(fpr_mul(a.re, scalar), fpr_mul(a.im, scalar));
}

static inline fpr
yuanyang_c_abs2(yuanyang_cplx a)
{
	return fpr_add(fpr_sqr(a.re), fpr_sqr(a.im));
}

static inline yuanyang_cplx
yuanyang_fft_get(const fpr a[YUANYANG_D], size_t u)
{
	size_t hn;

	hn = YUANYANG_D >> 1;
	return yuanyang_c_make(a[u], a[u + hn]);
}

static inline void
yuanyang_fft_set(fpr a[YUANYANG_D], size_t u, yuanyang_cplx z)
{
	size_t hn;

	hn = YUANYANG_D >> 1;
	a[u] = z.re;
	a[u + hn] = z.im;
}

static inline void
yuanyang_poly_to_fft_i8(fpr dst[YUANYANG_D], const int8_t src[YUANYANG_D])
{
	for (size_t u = 0; u < YUANYANG_D; u++) {
		dst[u] = fpr_of(src[u]);
	}
	Zf(FFT)(dst, YUANYANG_LOGD);
}

static inline void
yuanyang_poly_to_fft_i32(fpr dst[YUANYANG_D], const int32_t src[YUANYANG_D])
{
	for (size_t u = 0; u < YUANYANG_D; u++) {
		dst[u] = fpr_of(src[u]);
	}
	Zf(FFT)(dst, YUANYANG_LOGD);
}

static inline void
yuanyang_poly_to_fft_u16(fpr dst[YUANYANG_D], const uint16_t src[YUANYANG_D])
{
	for (size_t u = 0; u < YUANYANG_D; u++) {
		dst[u] = fpr_of(src[u]);
	}
	Zf(FFT)(dst, YUANYANG_LOGD);
}

static inline void
yuanyang_poly_to_fft_fpr(fpr dst[YUANYANG_D], const fpr src[YUANYANG_D])
{
	memcpy(dst, src, sizeof(fpr) * YUANYANG_D);
	Zf(FFT)(dst, YUANYANG_LOGD);
}

static inline void
yuanyang_poly_from_fft(fpr dst[YUANYANG_D], const fpr src_fft[YUANYANG_D])
{
	memcpy(dst, src_fft, sizeof(fpr) * YUANYANG_D);
	Zf(iFFT)(dst, YUANYANG_LOGD);
}

static inline void
yuanyang_round_i32_poly_from_fft(
	int32_t dst[YUANYANG_D], const fpr src_fft[YUANYANG_D])
{
	fpr tmp[YUANYANG_D];

	yuanyang_poly_from_fft(tmp, src_fft);
	for (size_t u = 0; u < YUANYANG_D; u++) {
		dst[u] = (int32_t)fpr_rint(tmp[u]);
	}
}

#endif
