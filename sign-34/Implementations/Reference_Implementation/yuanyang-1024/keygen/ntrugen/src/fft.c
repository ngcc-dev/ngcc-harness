/*
 * FFT code.
 *
 * ==========================(LICENSE BEGIN)============================
 *
 * Copyright (c) 2017-2019  Falcon Project
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * ===========================(LICENSE END)=============================
 *
 * @author   Thomas Pornin <thomas.pornin@nccgroup.com>
 */


/*
 * Rules for complex number macros:
 * --------------------------------
 *
 * Operand order is: destination, source1, source2...
 *
 * Each operand is a real and an imaginary part.
 *
 * All overlaps are allowed.
 */

/*
 * Addition of two complex numbers (d = a + b).
 */


#include <math.h>
#include <stddef.h>
#include <stdio.h>

#define FPC_ADD(d_re, d_im, a_re, a_im, b_re, b_im)   do { \
		double fpct_re, fpct_im; \
		fpct_re = (a_re+ b_re); \
		fpct_im = (a_im+ b_im); \
		(d_re) = fpct_re; \
		(d_im) = fpct_im; \
	} while (0)

/*
 * Subtraction of two complex numbers (d = a - b).
 */
#define FPC_SUB(d_re, d_im, a_re, a_im, b_re, b_im)   do { \
		double fpct_re, fpct_im; \
		fpct_re = (a_re- b_re); \
		fpct_im = (a_im- b_im); \
		(d_re) = fpct_re; \
		(d_im) = fpct_im; \
	} while (0)

/*
 * Multplication of two complex numbers (d = a * b).
 */
#define FPC_MUL(d_re, d_im, a_re, a_im, b_re, b_im)   do { \
		double fpct_a_re, fpct_a_im; \
		double fpct_b_re, fpct_b_im; \
		double fpct_d_re, fpct_d_im; \
		double ab_re_re, ab_im_im; \
		fpct_a_re = (a_re); \
		fpct_a_im = (a_im); \
		fpct_b_re = (b_re); \
		fpct_b_im = (b_im); \
		ab_re_re = fpct_a_re* fpct_b_re; \
		ab_im_im = fpct_a_im* fpct_b_im; \
		fpct_d_re = (ab_re_re- ab_im_im); \
		fpct_d_im = ( \
			(fpct_a_re+ fpct_a_im)* \
			(fpct_b_re+ fpct_b_im)); \
		fpct_d_im = ( \
			fpct_d_im- \
			(ab_re_re+ ab_im_im)); \
		(d_re) = fpct_d_re; \
		(d_im) = fpct_d_im; \
	} while (0)

/*
 * Squaring of a complex number (d = a * a).
 */
#define FPC_SQR(d_re, d_im, a_re, a_im)   do { \
		double fpct_a_re, fpct_a_im; \
		double fpct_d_re, fpct_d_im; \
		fpct_a_re = (a_re); \
		fpct_a_im = (a_im); \
		fpct_d_re = double_sub(double_sqr(fpct_a_re), double_sqr(fpct_a_im)); \
		fpct_d_im = double_double(double_mul(fpct_a_re, fpct_a_im)); \
		(d_re) = fpct_d_re; \
		(d_im) = fpct_d_im; \
	} while (0)

/*
 * Inversion of a complex number (d = 1 / a).
 */
#define FPC_INV(d_re, d_im, a_re, a_im)   do { \
		double fpct_a_re, fpct_a_im; \
		double fpct_d_re, fpct_d_im; \
		double fpct_m; \
		fpct_a_re = (a_re); \
		fpct_a_im = (a_im); \
		fpct_m = double_add(double_sqr(fpct_a_re), double_sqr(fpct_a_im)); \
		fpct_m = double_inv(fpct_m); \
		fpct_d_re = double_mul(fpct_a_re, fpct_m); \
		fpct_d_im = double_mul(double_neg(fpct_a_im), fpct_m); \
		(d_re) = fpct_d_re; \
		(d_im) = fpct_d_im; \
	} while (0)

/*
 * Division of complex numbers (d = a / b).
 */
#define FPC_DIV(d_re, d_im, a_re, a_im, b_re, b_im)   do { \
		fpr fpct_a_re, fpct_a_im; \
		fpr fpct_b_re, fpct_b_im; \
		fpr fpct_d_re, fpct_d_im; \
		fpr ab_re_re, ab_im_im; \
		fpr fpct_m; \
		fpct_a_re = (a_re); \
		fpct_a_im = (a_im); \
		fpct_b_re = (b_re); \
		fpct_b_im = (b_im); \
		fpct_m = fpr_add(fpr_sqr(fpct_b_re), fpr_sqr(fpct_b_im)); \
		fpct_m = fpr_inv(fpct_m); \
		fpct_b_re = fpr_mul(fpct_b_re, fpct_m); \
		fpct_b_im = fpr_mul(fpr_neg(fpct_b_im), fpct_m); \
		ab_re_re = fpr_mul(fpct_a_re, fpct_b_re); \
		ab_im_im = fpr_mul(fpct_a_im, fpct_b_im); \
		fpct_d_re = fpr_sub(ab_re_re, ab_im_im); \
		fpct_d_im = fpr_mul( \
			fpr_add(fpct_a_re, fpct_a_im), \
			fpr_add(fpct_b_re, fpct_b_im)); \
		fpct_d_im = fpr_sub( \
			fpct_d_im, \
			fpr_add(ab_re_re, ab_im_im)); \
		(d_re) = fpct_d_re; \
		(d_im) = fpct_d_im; \
	} while (0)

/*
 * Let w = exp(i*pi/N); w is a primitive 2N-th root of 1. We define the
 * values w_j = w^(2j+1) for all j from 0 to N-1: these are the roots
 * of X^N+1 in the field of complex numbers. A crucial property is that
 * w_{N-1-j} = conj(w_j) = 1/w_j for all j.
 *
 * FFT representation of a polynomial f (taken modulo X^N+1) is the
 * set of values f(w_j). Since f is real, conj(f(w_j)) = f(conj(w_j)),
 * thus f(w_{N-1-j}) = conj(f(w_j)). We thus store only half the values,
 * for j = 0 to N/2-1; the other half can be recomputed easily when (if)
 * needed. A consequence is that FFT representation has the same size
 * as normal representation: N/2 complex numbers use N real numbers (each
 * complex number is the combination of a real and an imaginary part).
 *
 * We use a specific ordering which makes computations easier. Let rev()
 * be the bit-reversal function over log(N) bits. For j in 0..N/2-1, we
 * store the real and imaginary parts of f(w_j) in slots:
 *
 *    Re(f(w_j)) -> slot rev(j)/2
 *    Im(f(w_j)) -> slot rev(j)/2+N/2
 *
 * (Note that rev(j) is even for j < N/2.)
 */

void double_negacyclic_FFT(double *f, unsigned logn,const double *twiddles)
{
	/*
	 * FFT algorithm in bit-reversal order uses the following
	 * iterative algorithm:
	 *
	 *   t = N
	 *   for m = 1; m < N; m *= 2:
	 *       ht = t/2
	 *       for i1 = 0; i1 < m; i1 ++:
	 *           j1 = i1 * t
	 *           s = GM[m + i1]
	 *           for j = j1; j < (j1 + ht); j ++:
	 *               x = f[j]
	 *               y = s * f[j + ht]
	 *               f[j] = x + y
	 *               f[j + ht] = x - y
	 *       t = ht
	 *
	 * GM[k] contains w^rev(k) for primitive root w = exp(i*pi/N).
	 *
	 * In the description above, f[] is supposed to contain complex
	 * numbers. In our in-memory representation, the real and
	 * imaginary parts of f[k] are in array slots k and k+N/2.
	 *
	 * We only keep the first half of the complex numbers. We can
	 * see that after the first iteration, the first and second halves
	 * of the array of complex numbers have separate lives, so we
	 * simply ignore the second part.
	 */

	unsigned u;
	size_t t, n, hn, m;

	/*
	 * First iteration: compute f[j] + i * f[j+N/2] for all j < N/2
	 * (because GM[1] = w^rev(1) = w^(N/2) = i).
	 * In our chosen representation, this is a no-op: everything is
	 * already where it should be.
	 */

	/*
	 * Subsequent iterations are truncated to use only the first
	 * half of values.
	 */
	n = (size_t)1 << logn;
	hn = n >> 1;
	t = hn;
	for (u = 1, m = 2; u < logn; u ++, m <<= 1) {
		size_t ht, hm, i1, j1;

		ht = t >> 1;
		hm = m >> 1;
		for (i1 = 0, j1 = 0; i1 < hm; i1 ++, j1 += t) {
			size_t j, j2;

			j2 = j1 + ht;
			double s_re, s_im;

			s_re = twiddles[((m + i1) << 1) + 0];
			s_im = twiddles[((m + i1) << 1) + 1];
			for (j = j1; j < j2; j ++) {
				double x_re, x_im, y_re, y_im;

				x_re = f[j];
				x_im = f[j + hn];
				y_re = f[j + ht];
				y_im = f[j + ht + hn];
				FPC_MUL(y_re, y_im, y_re, y_im, s_re, s_im);
				FPC_ADD(f[j], f[j + hn],
					x_re, x_im, y_re, y_im);
				FPC_SUB(f[j + ht], f[j + ht + hn],
					x_re, x_im, y_re, y_im);
			}
		}
		t = ht;
	}
}

/* see yuanyang_inner.h */
void double_negacyclic_iFFT(double *f, unsigned logn,const double *twiddles)
{
	/*
	 * Inverse FFT algorithm in bit-reversal order uses the following
	 * iterative algorithm:
	 *
	 *   t = 1
	 *   for m = N; m > 1; m /= 2:
	 *       hm = m/2
	 *       dt = t*2
	 *       for i1 = 0; i1 < hm; i1 ++:
	 *           j1 = i1 * dt
	 *           s = iGM[hm + i1]
	 *           for j = j1; j < (j1 + t); j ++:
	 *               x = f[j]
	 *               y = f[j + t]
	 *               f[j] = x + y
	 *               f[j + t] = s * (x - y)
	 *       t = dt
	 *   for i1 = 0; i1 < N; i1 ++:
	 *       f[i1] = f[i1] / N
	 *
	 * iGM[k] contains (1/w)^rev(k) for primitive root w = exp(i*pi/N)
	 * (actually, iGM[k] = 1/GM[k] = conj(GM[k])).
	 *
	 * In the main loop (not counting the final division loop), in
	 * all iterations except the last, the first and second half of f[]
	 * (as an array of complex numbers) are separate. In our chosen
	 * representation, we do not keep the second half.
	 *
	 * The last iteration recombines the recomputed half with the
	 * implicit half, and should yield only real numbers since the
	 * target polynomial is real; moreover, s = i at that step.
	 * Thus, when considering x and y:
	 *    y = conj(x) since the final f[j] must be real
	 *    Therefore, f[j] is filled with 2*Re(x), and f[j + t] is
	 *    filled with 2*Im(x).
	 * But we already have Re(x) and Im(x) in array slots j and j+t
	 * in our chosen representation. That last iteration is thus a
	 * simple doubling of the values in all the array.
	 *
	 * We make the last iteration a no-op by tweaking the final
	 * division into a division by N/2, not N.
	 */
	size_t u, n, hn, t, m;

	n = (size_t)1 << logn;
	t = 1;
	m = n;
	hn = n >> 1;
	for (u = logn; u > 1; u --) {
		size_t hm, dt, i1, j1;

		hm = m >> 1;
		dt = t << 1;
		for (i1 = 0, j1 = 0; j1 < hn; i1 ++, j1 += dt) {
			size_t j, j2;

			j2 = j1 + t;
			double s_re, s_im;

			s_re = twiddles[((hm + i1) << 1) + 0];
			s_im = -(twiddles[((hm + i1) << 1) + 1]);
			for (j = j1; j < j2; j ++) {
				double x_re, x_im, y_re, y_im;

				x_re = f[j];
				x_im = f[j + hn];
				y_re = f[j + t];
				y_im = f[j + t + hn];
				FPC_ADD(f[j], f[j + hn],
					x_re, x_im, y_re, y_im);
				FPC_SUB(x_re, x_im, x_re, x_im, y_re, y_im);
				FPC_MUL(f[j + t], f[j + t + hn],
					x_re, x_im, s_re, s_im);
			}
		}
		t = dt;
		m = hm;
	}

	/*
	 * Last iteration is a no-op, provided that we divide by N/2
	 * instead of N. We need to make a special case for logn = 0.
	 */
	if (logn > 0) {
		for (u = 0; u < n; u ++) {
			f[u] = f[u]/(1<<(logn-1));
		}
	}
}

static size_t
bitrev_size(size_t x, unsigned logn)
{
	size_t r = 0;

	while (logn -- > 0) {
		r = (r << 1) | (x & 1);
		x >>= 1;
	}
	return r;
}

/*
 * tw must have room for 2*N doubles, with N = 2^logn.
 *
 * For all k in 0..N-1:
 *
 *     tw[2*k + 0] = Re(w^rev(k))
 *     tw[2*k + 1] = Im(w^rev(k))
 *
 * where w = exp(i*pi/N), and rev() is bit-reversal over logn bits.
 *
 * The inverse FFT uses the conjugates by negating the imaginary part.
 */
void
make_twiddles(unsigned logn, double *tw)
{
	size_t n, k;
	double pi, scale;

	n = (size_t)1 << logn;
	pi = acos(-1.0);
	scale = pi / (double)n;

	for (k = 0; k < n; k ++) {
		size_t r;
		double a;

		r = bitrev_size(k, logn);
		a = scale * (double)r;

		tw[(k << 1) + 0] = cos(a);
		tw[(k << 1) + 1] = sin(a);
	}
}

void
double_FFT_fold4(const double *f, unsigned logn, double *nf)
{
	size_t m, hn, i;

	m = (size_t)1 << (logn - 2);  /* output size: n/4 */
	hn = m << 1;                 /* input half-size: n/2 */

	for (i = 0; i < (m >> 1); i ++) {
		size_t j;
		double x_re, x_im, y_re, y_im;

		j = 4 * i;

		x_re = f[j + 0];
		x_im = f[j + 0 + hn];

		y_re = f[j + 1];
		y_im = f[j + 1 + hn];
		FPC_MUL(x_re, x_im, x_re, x_im, y_re, y_im);

		y_re = f[j + 2];
		y_im = f[j + 2 + hn];
		FPC_MUL(x_re, x_im, x_re, x_im, y_re, y_im);

		y_re = f[j + 3];
		y_im = f[j + 3 + hn];
		FPC_MUL(x_re, x_im, x_re, x_im, y_re, y_im);

		nf[i] = x_re;
		nf[i + (m >> 1)] = x_im;
	}
}

/*
void
Zf(poly_mul_fft)(
	fpr *restrict a, const fpr *restrict b, unsigned logn)
{
	size_t n, hn, u;

	n = (size_t)1 << logn;
	hn = n >> 1;
	for (u = 0; u < hn; u ++) {
		fpr a_re, a_im, b_re, b_im;

		a_re = a[u];
		a_im = a[u + hn];
		b_re = b[u];
		b_im = b[u + hn];
		FPC_MUL(a[u], a[u + hn], a_re, a_im, b_re, b_im);
	}
}

void
Zf(poly_mulconst)(fpr *a, fpr x, unsigned logn)
{
	size_t n, u;

	n = (size_t)1 << logn;
	for (u = 0; u < n; u ++) {
		a[u] = fpr_mul(a[u], x);
	}
}
*/
