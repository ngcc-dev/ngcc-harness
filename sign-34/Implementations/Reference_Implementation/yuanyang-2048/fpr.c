/*
 * Fixed-point operations.
 *
 * This file implements the non-inline functions declared in
 * fpr.h, as well as the constants for FFT / iFFT.
 *
 * ==========================(LICENSE BEGIN)============================
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
 */

#include "yuanyang_inner.h"

static unsigned
yy_clz64(uint64_t x)
{
	unsigned n;
	uint64_t seen;

	n = 0;
	seen = 0;
	for (int i = 63; i >= 0; i--) {
		uint64_t bit;

		bit = (x >> (unsigned)i) & 1u;
		n += (unsigned)((seen | bit) ^ 1u);
		seen |= bit;
	}
	return n;
}

static fpr
fpr_abs(fpr x)
{
	int64_t y;
	fpr r;

	y = yy_fpr_i64(x.v);
	r.v = yy_fpr_abs_i64(y);
	return r;
}

static uint64_t
yy_ct_eq_u64(uint64_t x, uint64_t y)
{
	uint64_t z;

	z = x ^ y;
	return ((z | (uint64_t)-z) >> 63) ^ 1u;
}

static fpr
fpr_select(uint64_t ctl, fpr if_true, fpr if_false)
{
	uint64_t m;
	fpr r;

	m = (uint64_t)0 - ctl;
	r.v = (if_true.v & m) | (if_false.v & ~m);
	return r;
}

static uint64_t
yy_select_u64(uint64_t ctl, uint64_t if_true, uint64_t if_false)
{
	uint64_t m;

	m = (uint64_t)0 - ctl;
	return (if_true & m) | (if_false & ~m);
}

#define NEWTON_RAPHSON_ITER 8
#define NEWTON_RAPHSON_ITER_SQRT 9

fpr
fpr_div(fpr x, fpr y)
{
	static const fpr c48_17 = YY_FPR_RAW(0x0000169696969697);
	static const fpr c32_17 = YY_FPR_RAW(0x00000f0f0f0f0f0f);
	int64_t xi, yi;
	uint64_t sx, sy, ay;
	unsigned top;
	int shift;
	fpr b, inv, r;

	xi = yy_fpr_i64(x.v);
	yi = yy_fpr_i64(y.v);
	sx = (uint64_t)(xi >> 63) & 1u;
	sy = (uint64_t)(yi >> 63) & 1u;
	ay = yy_fpr_abs_i64(yi);
	if (ay == 0) {
		return fpr_zero;
	}

	/* Normalize |y| into [0.5, 1). */
	top = 63u - yy_clz64(ay);
	shift = (int)top - ((int)YUANYANG_FPR_FRAC_BITS - 1);
	if (shift >= 0) {
		b.v = ay >> (unsigned)shift;
	} else {
		b.v = ay << (unsigned)-shift;
	}

	inv = fpr_sub(c48_17, fpr_mul(c32_17, b));
	for (unsigned i = 0; i < NEWTON_RAPHSON_ITER; i++) {
		fpr e;

		e = fpr_sub(fpr_one, fpr_mul(b, inv));
		inv = fpr_add(inv, fpr_mul(inv, e));
	}
	if (shift >= 0) {
		inv = fpr_div2e(inv, (unsigned)shift);
	} else {
		inv = fpr_mul2e(inv, (unsigned)-shift);
	}

	r = fpr_mul(fpr_abs(x), inv);
	r.v = (uint64_t)yy_fpr_apply_sign_u64(r.v, sx ^ sy);
	return r;
}

#define YY_ISQRT_FRAC_BITS  62

/* Compute 1/sqrt(x) using Newton approximation.
 * Given a good approximation, it converges quadratically, so only few
 * iterations are needed. We only handle values around 2500 to 3500, so we can
 * give start with a very accurate approximation.
 * We may need to adjust the initial value for other parameter sets The interval
 * comes from the context of call to sqrt in YuanYang:
*  - In sample UnifCrown, we compute sqrt(lower_radius_sq +
*    rho*radius_sq_span) which is well bounded.
*  - In the cholesky decomposition of sigma_p - I, we compute two square
*    root, on values that can be bounded too.
 * We compute 1/sqrt(x), then multiply the result by x to avoid, using
 * divisions during the iterative approximation.
 */
fpr
fpr_sqrt(fpr x)
{
	uint64_t y;
	fpr z;
	/*
	 * Seed = 1/64 in Q2.62.
	 * Safe for x in [2500, 3400].
	 */
	y = UINT64_C(0x0100000000000000);
	for (unsigned i = 0; i < NEWTON_RAPHSON_ITER_SQRT; i++) {
		uint64_t y2, xy2, t;

		/* y2 = y*y, still Q2.62 */
		y2 = yy_mul64_rshift_rne(y, y, YY_ISQRT_FRAC_BITS);
		/*
		 * xy2 = x * y^2.
		 * x.v is Q20.43 that is less than 2^{12}, y2 is Q2.62 that is less than 2^{-12}.
		 * Shift by 43 to keep Q2.62.
		 */
		xy2 = yy_mul64_rshift_rne(x.v, y2, YUANYANG_FPR_FRAC_BITS);

		/* t = (3 - x*y^2) / 2, in Q2.62 */
		t = ((UINT64_C(3) << YY_ISQRT_FRAC_BITS) - xy2) >> 1;

		/* y = y * t, still Q2.62 */
		y = yy_mul64_rshift_rne(y, t, YY_ISQRT_FRAC_BITS);
	}

	/*
	 * r0 = x * (1/sqrt(x)).
	 * x is Q20.43, y is Q2.62, result is Q20.43.
	 */
	z.v = yy_mul64_rshift_rne(x.v, y, YY_ISQRT_FRAC_BITS);

	return z;
}

#if 0
/* This is a more accurate but slower implementations of sqrt, kept just in case
and for small case testing */

static yy_u128
yy_u128_lshift_u64(uint64_t x, unsigned shift)
{
	yy_u128 z;

	if (shift == 0u) {
		z.hi = 0;
		z.lo = x;
	} else if (shift < 64u) {
		z.hi = x >> (64u - shift);
		z.lo = x << shift;
	} else {
		z.hi = x << (shift - 64u);
		z.lo = 0;
	}
	return z;
}

static int
yy_u128_cmp(yy_u128 x, yy_u128 y)
{
	if (x.hi != y.hi) {
		return x.hi < y.hi ? -1 : 1;
	}
	if (x.lo != y.lo) {
		return x.lo < y.lo ? -1 : 1;
	}
	return 0;
}

static yy_u128
yy_u128_sub(yy_u128 x, yy_u128 y)
{
	yy_u128 z;

	z.lo = x.lo - y.lo;
	z.hi = x.hi - y.hi - (x.lo < y.lo);
	return z;
}

static fpr
fpr_sqrt_safe(fpr x)
{
	int64_t xi;
	uint64_t ax, r;
	yy_u128 n, sq, rem;
	fpr y;

	xi = yy_fpr_i64(x.v);
	if (xi <= 0) {
		return fpr_zero;
	}
	ax = (uint64_t)xi;
	n = yy_u128_lshift_u64(ax, YUANYANG_FPR_FRAC_BITS);
	r = 0;
	for (int i = 63; i >= 0; i--) {
		uint64_t cand, le;

		cand = r | (UINT64_C(1) << (unsigned)i);
		sq = yy_mul64_wide(cand, cand);
		le = (uint64_t)(yy_u128_cmp(sq, n) <= 0);
		r = yy_select_u64(le, cand, r);
	}
	sq = yy_mul64_wide(r, r);
	rem = yy_u128_sub(n, sq);
	r += (uint64_t)(yy_u128_cmp(rem, (yy_u128){ 0, r }) > 0);
	y.v = r;
	return y;
}
#endif

uint64_t
fpr_to_q63(fpr x)
{
	int64_t y;

	y = yy_fpr_i64(x.v);
	if (y <= 0) {
		return 0;
	}
	if ((uint64_t)y >= YUANYANG_FPR_ONE_RAW) {
		return UINT64_C(0x8000000000000000);
	}
	return (uint64_t)y << (63u - YUANYANG_FPR_FRAC_BITS);
}

fpr
fpr_from_q63(uint64_t x)
{
	fpr y;

	y.v = x >> (63u - YUANYANG_FPR_FRAC_BITS);
	return y;
}

uint64_t
fpr_expm_p63(fpr x)
{
	static const uint64_t C[] = {
		UINT64_C(0x00000004741183A3),
		UINT64_C(0x00000036548CFC06),
		UINT64_C(0x0000024FDCBF140A),
		UINT64_C(0x0000171D939DE045),
		UINT64_C(0x0000D00CF58F6F84),
		UINT64_C(0x000680681CF796E3),
		UINT64_C(0x002D82D8305B0FEA),
		UINT64_C(0x011111110E066FD0),
		UINT64_C(0x0555555555070F00),
		UINT64_C(0x155555555581FF00),
		UINT64_C(0x400000000002B400),
		UINT64_C(0x7FFFFFFFFFFF4800),
		UINT64_C(0x8000000000000000)
	};
	uint64_t z, y;

	z = fpr_to_q63(x);
	y = C[0];
	for (unsigned u = 1; u < (sizeof C) / sizeof(C[0]); u++) {
		y = C[u] - yy_mul_q63(z, y);
	}
	return y;
}

fpr_q62
fpr_expm_p62(fpr x)
{
	return fpr_q62_from_q63(fpr_expm_p63(x));
}

/*
 * Reduced-domain Q62 trig approximations.
 *
 * The public fpr type remains Q20.43, but the reduced angle and polynomial
 * value are carried in Q62, with Q62 products inside Horner evaluation.  This
 * keeps the approximation error within one Q62 unit on [-pi/4, pi/4] before the
 * final conversion back to fpr.
 */
#define YY_TRIG_Q62_FRAC         62u
#define YY_TRIG_PI_OVER_FOUR_Q62 INT64_C(3622009729038561280)

#define NB_SIN_COEFFS 7
static const int64_t yy_sin_q62_d13_odd[NB_SIN_COEFFS] = {
	INT64_C(4611686018427387904),
	-INT64_C(768614336404561920),
	INT64_C(38430716820168704),
	-INT64_C(915017066565632),
	INT64_C(12708567678976),
	-INT64_C(115526041600),
	INT64_C(733016064)
};
#define NB_COS_COEFFS 7
static const int64_t yy_cos_q62_d12_even[NB_COS_COEFFS] = {
	INT64_C(4611686018427387904),
	-INT64_C(2305843009213661184),
	INT64_C(192153584100245504),
	-INT64_C(6405119461318656),
	INT64_C(114377092980736),
	-INT64_C(1270760869888),
	INT64_C(9514119168)
};

static int64_t
yy_mul_q62(int64_t x, int64_t y)
{
	uint64_t sx, sy, mag;

	sx = (uint64_t)(x >> 63) & 1u;
	sy = (uint64_t)(y >> 63) & 1u;
	mag = yy_mul64_rshift_rne(yy_fpr_abs_i64(x),
		yy_fpr_abs_i64(y), YY_TRIG_Q62_FRAC);
	return yy_fpr_apply_sign_u64(mag, sx ^ sy);
}

static int64_t
yy_horner_q62(const int64_t *c_q62, size_t n, int64_t x_q62)
{
	int64_t y;

	y = c_q62[n - 1u];
	for (size_t u = n - 1u; u > 0; u--) {
		y = c_q62[u - 1u] + yy_mul_q62(x_q62, y);
	}
	return y;
}

static int64_t
sin_reduced(int64_t x)
{
	uint64_t sin;

	sin = yy_mul64_rshift_rne((uint64_t)x, (uint64_t)x,
		YY_TRIG_Q62_FRAC);
	sin = yy_horner_q62(yy_sin_q62_d13_odd, NB_SIN_COEFFS, sin);
	sin = yy_mul64_rshift_rne((uint64_t)x, sin, YY_TRIG_Q62_FRAC);
	return yy_i64_rshift_rne(sin,
		YY_TRIG_Q62_FRAC - YUANYANG_FPR_FRAC_BITS);
}

static int64_t
cos_reduced(int64_t x)
{
	uint64_t cos;

	cos = yy_mul64_rshift_rne((uint64_t)x, (uint64_t)x,
		YY_TRIG_Q62_FRAC);
	cos = yy_horner_q62(yy_cos_q62_d12_even, NB_COS_COEFFS, cos);
	return yy_i64_rshift_rne(cos,
		YY_TRIG_Q62_FRAC - YUANYANG_FPR_FRAC_BITS);
}

/*
 * Reduce the angle by quadrant and half-quadrant, then compute sine and cosine
 * on the reduced angle.
*/
static void
sincos_2pi_scaled(uint64_t frac, unsigned cycle_bits, fpr *s, fpr *c)
{
	uint64_t rem, quad, quarter, upper_half;
	uint64_t x0, x1, xr;
	uint64_t q1, q2, q3;
	int64_t x_q62;
	fpr sp, cp, nsp, ncp;
	fpr bsp, bcp;
	fpr s0, s1, s2, s3;
	fpr c0, c1, c2, c3;

	quad = frac >> (cycle_bits - 2u);
	quarter = UINT64_C(1) << (cycle_bits - 2u);
	rem = frac & (quarter - 1u);
	upper_half = rem >> (cycle_bits - 3u);

	/*
	 * Reduce each quadrant to the nearest axis.  The polynomial now only sees
	 * an angle in [0, pi/4], instead of the full [0, pi/2] quadrant.
	 */
	x0 = rem;
	x1 = quarter - rem;
	xr = yy_select_u64(upper_half, x1, x0);
	x_q62 = (int64_t)yy_mul64_rshift_rne(xr,
		(uint64_t)YY_TRIG_PI_OVER_FOUR_Q62, cycle_bits - 3u);
	sp.v = sin_reduced(x_q62);
	cp.v = cos_reduced(x_q62);
	bsp = fpr_select(upper_half, cp, sp);
	bcp = fpr_select(upper_half, sp, cp);
	sp = bsp;
	cp = bcp;
	nsp = fpr_neg(sp);
	ncp = fpr_neg(cp);

	s0 = sp;
	c0 = cp;
	s1 = cp;
	c1 = nsp;
	s2 = nsp;
	c2 = ncp;
	s3 = ncp;
	c3 = sp;

	q1 = yy_ct_eq_u64(quad, 1u);
	q2 = yy_ct_eq_u64(quad, 2u);
	q3 = yy_ct_eq_u64(quad, 3u);
	*s = fpr_select(q3, s3,
		fpr_select(q2, s2, fpr_select(q1, s1, s0)));
	*c = fpr_select(q3, c3,
		fpr_select(q2, c2, fpr_select(q1, c1, c0)));
}

fpr_q62
fpr_cos_2pi_q62(fpr x)
{
	fpr_q62 s;
	/*
	 * Compute x*x in high precision: we switch to Q2.62 format for the internal
	 * computation
	 */
	uint64_t x_q62 = yy_mul64_rshift_rne(x.v, x.v, 24u);
	s.v = yy_horner_q62(yy_cos_q62_d12_even, NB_COS_COEFFS, x_q62);
	return s;
}

fpr
fpr_sin_2pi(fpr x)
{
	fpr s, c;

	fpr_sincos_2pi_u53((uint64_t)fpr_rint_to_scaled(x, 53), &s, &c);
	(void)c;
	return s;
}

fpr
fpr_cos_2pi(fpr x)
{
	fpr s, c;

	fpr_sincos_2pi_u53((uint64_t)fpr_rint_to_scaled(x, 53), &s, &c);
	(void)s;
	return c;
}

/* Compute both sin and cos of x, store it in s and c respectively. */
void
fpr_sincos_2pi_u53(uint64_t x, fpr *s, fpr *c)
{
	sincos_2pi_scaled(x & UINT64_C(0x1FFFFFFFFFFFFF), 53u, s, c);
}

/* Compute both sin and cos of x/4, store it in s and c respectively. */
void
fpr_sincos_2pi_u53_div4(uint64_t x, fpr *s, fpr *c)
{
	sincos_2pi_scaled(x & UINT64_C(0x1FFFFFFFFFFFFF), 55u, s, c);
}

/*
 * Table of roots for FFT embedding, but stored as signed Q2.62 for FXP
 * FFT/iFFT twiddle multiplication.  Generated from the native
 * decimal literals with round-to-nearest-even.
 */
const fpr_q62 fpr_gm_tab_q62[] = {
	{ INT64_C(0x0000000000000000) }, { INT64_C(0x0000000000000000) }, { INT64_C(0x000000000000011a) }, { INT64_C(0x4000000000000000) },
	{ INT64_C(0x2d413cccfe779a00) }, { INT64_C(0x2d413cccfe779800) }, { -INT64_C(0x2d413cccfe779800) }, { INT64_C(0x2d413cccfe779a00) },
	{ INT64_C(0x3b20d79e651a8c00) }, { INT64_C(0x187de2a6aea96300) }, { -INT64_C(0x187de2a6aea96200) }, { INT64_C(0x3b20d79e651a8c00) },
	{ INT64_C(0x187de2a6aea96400) }, { INT64_C(0x3b20d79e651a8c00) }, { -INT64_C(0x3b20d79e651a8c00) }, { INT64_C(0x187de2a6aea96500) },
	{ INT64_C(0x3ec52f9feeb96000) }, { INT64_C(0x0c7c5c1e34d30500) }, { -INT64_C(0x0c7c5c1e34d30400) }, { INT64_C(0x3ec52f9feeb96000) },
	{ INT64_C(0x238e76735cd19200) }, { INT64_C(0x3536cc521d434600) }, { -INT64_C(0x3536cc521d434800) }, { INT64_C(0x238e76735cd19000) },
	{ INT64_C(0x3536cc521d434600) }, { INT64_C(0x238e76735cd19000) }, { -INT64_C(0x238e76735cd18c00) }, { INT64_C(0x3536cc521d434a00) },
	{ INT64_C(0x0c7c5c1e34d30680) }, { INT64_C(0x3ec52f9feeb96000) }, { -INT64_C(0x3ec52f9feeb96000) }, { INT64_C(0x0c7c5c1e34d30b80) },
	{ INT64_C(0x3fb11b47a24a4c00) }, { INT64_C(0x0645e9af0a6d0b00) }, { -INT64_C(0x0645e9af0a6d0bc0) }, { INT64_C(0x3fb11b47a24a4c00) },
	{ INT64_C(0x2899e64a123bac00) }, { INT64_C(0x317900d62a2e8200) }, { -INT64_C(0x317900d62a2e8200) }, { INT64_C(0x2899e64a123bac00) },
	{ INT64_C(0x387165e3017b6200) }, { INT64_C(0x1e2b5d3806f63b00) }, { -INT64_C(0x1e2b5d3806f63c00) }, { INT64_C(0x387165e3017b6200) },
	{ INT64_C(0x1294062ed59f0500) }, { INT64_C(0x3d3e82ad8c5bb600) }, { -INT64_C(0x3d3e82ad8c5bb400) }, { INT64_C(0x1294062ed59f0600) },
	{ INT64_C(0x3d3e82ad8c5bb400) }, { INT64_C(0x1294062ed59f0500) }, { -INT64_C(0x1294062ed59f0200) }, { INT64_C(0x3d3e82ad8c5bb600) },
	{ INT64_C(0x1e2b5d3806f63e00) }, { INT64_C(0x387165e3017b6000) }, { -INT64_C(0x387165e3017b6000) }, { INT64_C(0x1e2b5d3806f63f00) },
	{ INT64_C(0x317900d62a2e8200) }, { INT64_C(0x2899e64a123bac00) }, { -INT64_C(0x2899e64a123baa00) }, { INT64_C(0x317900d62a2e8400) },
	{ INT64_C(0x0645e9af0a6d0e00) }, { INT64_C(0x3fb11b47a24a4a00) }, { -INT64_C(0x3fb11b47a24a4a00) }, { INT64_C(0x0645e9af0a6d0f00) },
	{ INT64_C(0x3fec43c6f2dafc00) }, { INT64_C(0x0323ecbe21bb0280) }, { -INT64_C(0x0323ecbe21bb0260) }, { INT64_C(0x3fec43c6f2dafc00) },
	{ INT64_C(0x2afad26919d93e00) }, { INT64_C(0x2f6bbe44d55f5e00) }, { -INT64_C(0x2f6bbe44d55f5a00) }, { INT64_C(0x2afad26919d94200) },
	{ INT64_C(0x39daf5e8798ee600) }, { INT64_C(0x1b5d1009e15cc000) }, { -INT64_C(0x1b5d1009e15cbc00) }, { INT64_C(0x39daf5e8798ee800) },
	{ INT64_C(0x158f9a75ab1fdd00) }, { INT64_C(0x3c424209ed0dca00) }, { -INT64_C(0x3c424209ed0dc800) }, { INT64_C(0x158f9a75ab1fe200) },
	{ INT64_C(0x3e14fdf72461ae00) }, { INT64_C(0x0f8cfcbd90af8d00) }, { -INT64_C(0x0f8cfcbd90af8d00) }, { INT64_C(0x3e14fdf72461ae00) },
	{ INT64_C(0x20e70f3245ffda00) }, { INT64_C(0x36e5068a32dc7c00) }, { -INT64_C(0x36e5068a32dc7a00) }, { INT64_C(0x20e70f3245ffdc00) },
	{ INT64_C(0x3367c08fe70e8200) }, { INT64_C(0x261feff9c2e06a00) }, { -INT64_C(0x261feff9c2e06a00) }, { INT64_C(0x3367c08fe70e8200) },
	{ INT64_C(0x0964083747309d00) }, { INT64_C(0x3f4eaafe114a2e00) }, { -INT64_C(0x3f4eaafe114a2e00) }, { INT64_C(0x0964083747309e00) },
	{ INT64_C(0x3f4eaafe114a2e00) }, { INT64_C(0x0964083747309d00) }, { -INT64_C(0x0964083747309b00) }, { INT64_C(0x3f4eaafe114a2e00) },
	{ INT64_C(0x261feff9c2e06c00) }, { INT64_C(0x3367c08fe70e8000) }, { -INT64_C(0x3367c08fe70e8000) }, { INT64_C(0x261feff9c2e06c00) },
	{ INT64_C(0x36e5068a32dc7c00) }, { INT64_C(0x20e70f3245ffda00) }, { -INT64_C(0x20e70f3245ffda00) }, { INT64_C(0x36e5068a32dc7c00) },
	{ INT64_C(0x0f8cfcbd90af8f00) }, { INT64_C(0x3e14fdf72461ae00) }, { -INT64_C(0x3e14fdf72461ae00) }, { INT64_C(0x0f8cfcbd90af9080) },
	{ INT64_C(0x3c424209ed0dca00) }, { INT64_C(0x158f9a75ab1fdd00) }, { -INT64_C(0x158f9a75ab1fdb00) }, { INT64_C(0x3c424209ed0dca00) },
	{ INT64_C(0x1b5d1009e15cc200) }, { INT64_C(0x39daf5e8798ee600) }, { -INT64_C(0x39daf5e8798ee600) }, { INT64_C(0x1b5d1009e15cbf00) },
	{ INT64_C(0x2f6bbe44d55f5e00) }, { INT64_C(0x2afad26919d93e00) }, { -INT64_C(0x2afad26919d94000) }, { INT64_C(0x2f6bbe44d55f5c00) },
	{ INT64_C(0x0323ecbe21bb0480) }, { INT64_C(0x3fec43c6f2dafc00) }, { -INT64_C(0x3fec43c6f2dafc00) }, { INT64_C(0x0323ecbe21bb01a0) },
	{ INT64_C(0x3ffb10c1099a1a00) }, { INT64_C(0x0192155f7a3667e0) }, { -INT64_C(0x0192155f7a366540) }, { INT64_C(0x3ffb10c1099a1a00) },
	{ INT64_C(0x2c216eaa3a59be00) }, { INT64_C(0x2e5a106fdfff2c00) }, { -INT64_C(0x2e5a106fdfff2a00) }, { INT64_C(0x2c216eaa3a59c000) },
	{ INT64_C(0x3a8269a29b927400) }, { INT64_C(0x19ef7943a8ed8a00) }, { -INT64_C(0x19ef7943a8ed8800) }, { INT64_C(0x3a8269a29b927400) },
	{ INT64_C(0x17088530fa45a100) }, { INT64_C(0x3bb6276d99847800) }, { -INT64_C(0x3bb6276d99847800) }, { INT64_C(0x17088530fa45a200) },
	{ INT64_C(0x3e71e758c9cb1200) }, { INT64_C(0x0e05c1353f27b180) }, { -INT64_C(0x0e05c1353f27af00) }, { INT64_C(0x3e71e758c9cb1200) },
	{ INT64_C(0x223d66a836964600) }, { INT64_C(0x361214b02a03fe00) }, { -INT64_C(0x361214b02a040000) }, { INT64_C(0x223d66a836964400) },
	{ INT64_C(0x34534f408c4f0400) }, { INT64_C(0x24da0a99ba25be00) }, { -INT64_C(0x24da0a99ba25be00) }, { INT64_C(0x34534f408c4f0400) },
	{ INT64_C(0x0af10a22459fe580) }, { INT64_C(0x3f0ec9f4e2975200) }, { -INT64_C(0x3f0ec9f4e2975200) }, { INT64_C(0x0af10a22459fe300) },
	{ INT64_C(0x3f84c8e1c33fa600) }, { INT64_C(0x07d59395aa5cc380) }, { -INT64_C(0x07d59395aa5cc2c0) }, { INT64_C(0x3f84c8e1c33fa600) },
	{ INT64_C(0x275ff45240a17200) }, { INT64_C(0x3274449324c7f600) }, { -INT64_C(0x3274449324c7f600) }, { INT64_C(0x275ff45240a17400) },
	{ INT64_C(0x37af8158df2a5400) }, { INT64_C(0x1f8ba4dbf89aba00) }, { -INT64_C(0x1f8ba4dbf89ab900) }, { INT64_C(0x37af8158df2a5400) },
	{ INT64_C(0x1111d262b1f67800) }, { INT64_C(0x3dae81ced092c600) }, { -INT64_C(0x3dae81ced092c600) }, { INT64_C(0x1111d262b1f67900) },
	{ INT64_C(0x3cc511d891c22400) }, { INT64_C(0x14135c9417660200) }, { -INT64_C(0x14135c9417660000) }, { INT64_C(0x3cc511d891c22400) },
	{ INT64_C(0x1cc66e9931c45e00) }, { INT64_C(0x392a96426823ea00) }, { -INT64_C(0x392a96426823e800) }, { INT64_C(0x1cc66e9931c46300) },
	{ INT64_C(0x30761c17ff2edc00) }, { INT64_C(0x29cd9577c7cbd200) }, { -INT64_C(0x29cd9577c7cbce00) }, { INT64_C(0x30761c17ff2ede00) },
	{ INT64_C(0x04b54824b3867e00) }, { INT64_C(0x3fd39b5a03107400) }, { -INT64_C(0x3fd39b5a03107400) }, { INT64_C(0x04b54824b3868300) },
	{ INT64_C(0x3fd39b5a03107400) }, { INT64_C(0x04b54824b3867d80) }, { -INT64_C(0x04b54824b3867bc0) }, { INT64_C(0x3fd39b5a03107400) },
	{ INT64_C(0x29cd9577c7cbd200) }, { INT64_C(0x30761c17ff2eda00) }, { -INT64_C(0x30761c17ff2edc00) }, { INT64_C(0x29cd9577c7cbd000) },
	{ INT64_C(0x392a96426823ea00) }, { INT64_C(0x1cc66e9931c45d00) }, { -INT64_C(0x1cc66e9931c46000) }, { INT64_C(0x392a96426823e800) },
	{ INT64_C(0x14135c9417660300) }, { INT64_C(0x3cc511d891c22400) }, { -INT64_C(0x3cc511d891c22400) }, { INT64_C(0x14135c9417660000) },
	{ INT64_C(0x3dae81ced092c600) }, { INT64_C(0x1111d262b1f67700) }, { -INT64_C(0x1111d262b1f67600) }, { INT64_C(0x3dae81ced092c600) },
	{ INT64_C(0x1f8ba4dbf89abb00) }, { INT64_C(0x37af8158df2a5200) }, { -INT64_C(0x37af8158df2a5200) }, { INT64_C(0x1f8ba4dbf89abc00) },
	{ INT64_C(0x3274449324c7f800) }, { INT64_C(0x275ff45240a17200) }, { -INT64_C(0x275ff45240a17000) }, { INT64_C(0x3274449324c7f800) },
	{ INT64_C(0x07d59395aa5cc500) }, { INT64_C(0x3f84c8e1c33fa600) }, { -INT64_C(0x3f84c8e1c33fa600) }, { INT64_C(0x07d59395aa5cc640) },
	{ INT64_C(0x3f0ec9f4e2975200) }, { INT64_C(0x0af10a22459fe300) }, { -INT64_C(0x0af10a22459fe380) }, { INT64_C(0x3f0ec9f4e2975200) },
	{ INT64_C(0x24da0a99ba25be00) }, { INT64_C(0x34534f408c4f0400) }, { -INT64_C(0x34534f408c4f0200) }, { INT64_C(0x24da0a99ba25c000) },
	{ INT64_C(0x361214b02a040000) }, { INT64_C(0x223d66a836964400) }, { -INT64_C(0x223d66a836964200) }, { INT64_C(0x361214b02a040200) },
	{ INT64_C(0x0e05c1353f27b100) }, { INT64_C(0x3e71e758c9cb1200) }, { -INT64_C(0x3e71e758c9cb1000) }, { INT64_C(0x0e05c1353f27b600) },
	{ INT64_C(0x3bb6276d99847a00) }, { INT64_C(0x17088530fa459e00) }, { -INT64_C(0x17088530fa459f00) }, { INT64_C(0x3bb6276d99847800) },
	{ INT64_C(0x19ef7943a8ed8a00) }, { INT64_C(0x3a8269a29b927400) }, { -INT64_C(0x3a8269a29b927400) }, { INT64_C(0x19ef7943a8ed8b00) },
	{ INT64_C(0x2e5a106fdfff2e00) }, { INT64_C(0x2c216eaa3a59bc00) }, { -INT64_C(0x2c216eaa3a59be00) }, { INT64_C(0x2e5a106fdfff2c00) },
	{ INT64_C(0x0192155f7a366770) }, { INT64_C(0x3ffb10c1099a1a00) }, { -INT64_C(0x3ffb10c1099a1a00) }, { INT64_C(0x0192155f7a366890) },
	{ INT64_C(0x3ffec42d3725b600) }, { INT64_C(0x00c90e8fe6f63c20) }, { -INT64_C(0x00c90e8fe6f63a48) }, { INT64_C(0x3ffec42d3725b600) },
	{ INT64_C(0x2cb2324be0f07c00) }, { INT64_C(0x2dce88a9d5515c00) }, { -INT64_C(0x2dce88a9d5515c00) }, { INT64_C(0x2cb2324be0f07c00) },
	{ INT64_C(0x3ad2c2e793cd1600) }, { INT64_C(0x19372a63bc93d700) }, { -INT64_C(0x19372a63bc93d500) }, { INT64_C(0x3ad2c2e793cd1600) },
	{ INT64_C(0x17c3a9311dcce800) }, { INT64_C(0x3b6ca4c471413400) }, { -INT64_C(0x3b6ca4c471413400) }, { INT64_C(0x17c3a9311dccea00) },
	{ INT64_C(0x3e9cc076165e5a00) }, { INT64_C(0x0d415012d8022880) }, { -INT64_C(0x0d415012d8022680) }, { INT64_C(0x3e9cc076165e5a00) },
	{ INT64_C(0x22e69ac7bdb69200) }, { INT64_C(0x35a5793c43aa2000) }, { -INT64_C(0x35a5793c43aa2200) }, { INT64_C(0x22e69ac7bdb69000) },
	{ INT64_C(0x34c6123605f5c400) }, { INT64_C(0x2434f33267d6b000) }, { -INT64_C(0x2434f33267d6b200) }, { INT64_C(0x34c6123605f5c200) },
	{ INT64_C(0x0bb6ecef285f9a80) }, { INT64_C(0x3eeb33474240ee00) }, { -INT64_C(0x3eeb33474240ee00) }, { INT64_C(0x0bb6ecef285f9780) },
	{ INT64_C(0x3f9c2bfadb4cf600) }, { INT64_C(0x070de171e7b0b540) }, { -INT64_C(0x070de171e7b0b540) }, { INT64_C(0x3f9c2bfadb4cf600) },
	{ INT64_C(0x27fdb2a68aadaa00) }, { INT64_C(0x31f79947df281800) }, { -INT64_C(0x31f79947df281a00) }, { INT64_C(0x27fdb2a68aada800) },
	{ INT64_C(0x3811884ce4aa9200) }, { INT64_C(0x1edc1952ef78d500) }, { -INT64_C(0x1edc1952ef78d500) }, { INT64_C(0x3811884ce4aa9200) },
	{ INT64_C(0x11d3443f4cdb3d00) }, { INT64_C(0x3d77b191be16e800) }, { -INT64_C(0x3d77b191be16e800) }, { INT64_C(0x11d3443f4cdb3f00) },
	{ INT64_C(0x3d02f75699a21a00) }, { INT64_C(0x135410c2e1815200) }, { -INT64_C(0x135410c2e1815200) }, { INT64_C(0x3d02f75699a21a00) },
	{ INT64_C(0x1d79775b86e38900) }, { INT64_C(0x38cf166910e73600) }, { -INT64_C(0x38cf166910e73400) }, { INT64_C(0x1d79775b86e38d00) },
	{ INT64_C(0x30f8801f745d7e00) }, { INT64_C(0x2934893736127000) }, { -INT64_C(0x2934893736126e00) }, { INT64_C(0x30f8801f745d8000) },
	{ INT64_C(0x057db402a6a90600) }, { INT64_C(0x3fc395f97ab61200) }, { -INT64_C(0x3fc395f97ab61200) }, { INT64_C(0x057db402a6a90b00) },
	{ INT64_C(0x3fe12acb1ce35a00) }, { INT64_C(0x03ecadcf3f041c00) }, { -INT64_C(0x03ecadcf3f041b20) }, { INT64_C(0x3fe12acb1ce35a00) },
	{ INT64_C(0x2a65052546ab2c00) }, { INT64_C(0x2ff1d9c6ae2ee000) }, { -INT64_C(0x2ff1d9c6ae2ede00) }, { INT64_C(0x2a65052546ab3000) },
	{ INT64_C(0x3983e1e7f9f8b800) }, { INT64_C(0x1c1249d8011ee700) }, { -INT64_C(0x1c1249d8011ee200) }, { INT64_C(0x3983e1e7f9f8ba00) },
	{ INT64_C(0x14d1e24278e76b00) }, { INT64_C(0x3c84d4965782fc00) }, { -INT64_C(0x3c84d4965782fa00) }, { INT64_C(0x14d1e24278e77000) },
	{ INT64_C(0x3de2f147c8e78400) }, { INT64_C(0x104fb80e37fdae00) }, { -INT64_C(0x104fb80e37fdad00) }, { INT64_C(0x3de2f147c8e78400) },
	{ INT64_C(0x2039f90e987d6e00) }, { INT64_C(0x374b54ce6b21a400) }, { -INT64_C(0x374b54ce6b21a400) }, { INT64_C(0x2039f90e987d7000) },
	{ INT64_C(0x32eefde98fae8400) }, { INT64_C(0x26c0b1620cb3e600) }, { -INT64_C(0x26c0b1620cb3e400) }, { INT64_C(0x32eefde98fae8400) },
	{ INT64_C(0x089cf8676d7abc00) }, { INT64_C(0x3f6af2e32bae8200) }, { -INT64_C(0x3f6af2e32bae8200) }, { INT64_C(0x089cf8676d7abd00) },
	{ INT64_C(0x3f2ff24992133600) }, { INT64_C(0x0a2abb58949f2d00) }, { -INT64_C(0x0a2abb58949f2a00) }, { INT64_C(0x3f2ff24992133600) },
	{ INT64_C(0x257db64bf5e7d400) }, { INT64_C(0x33de87de535f2800) }, { -INT64_C(0x33de87de535f2600) }, { INT64_C(0x257db64bf5e7d600) },
	{ INT64_C(0x367c9a7deaae2400) }, { INT64_C(0x2192e09abb131e00) }, { -INT64_C(0x2192e09abb131a00) }, { INT64_C(0x367c9a7deaae2400) },
	{ INT64_C(0x0ec9a7f2a2a18b80) }, { INT64_C(0x3e44a5eeec75b200) }, { -INT64_C(0x3e44a5eeec75b200) }, { INT64_C(0x0ec9a7f2a2a18c80) },
	{ INT64_C(0x3bfd5cc45b7c5600) }, { INT64_C(0x164c7ddd3f27c600) }, { -INT64_C(0x164c7ddd3f27c300) }, { INT64_C(0x3bfd5cc45b7c5600) },
	{ INT64_C(0x1aa6c82b6d3fcc00) }, { INT64_C(0x3a2fcee87c6bb600) }, { -INT64_C(0x3a2fcee87c6bb800) }, { INT64_C(0x1aa6c82b6d3fc900) },
	{ INT64_C(0x2ee3cebe06e4c200) }, { INT64_C(0x2b8ef77cca031800) }, { -INT64_C(0x2b8ef77cca031800) }, { INT64_C(0x2ee3cebe06e4c200) },
	{ INT64_C(0x025b0caeb28abc80) }, { INT64_C(0x3ff4e5dffdeeba00) }, { -INT64_C(0x3ff4e5dffdeeba00) }, { INT64_C(0x025b0caeb28ab9a0) },
	{ INT64_C(0x3ff4e5dffdeeba00) }, { INT64_C(0x025b0caeb28ab9a0) }, { -INT64_C(0x025b0caeb28aba40) }, { INT64_C(0x3ff4e5dffdeeba00) },
	{ INT64_C(0x2b8ef77cca031a00) }, { INT64_C(0x2ee3cebe06e4c200) }, { -INT64_C(0x2ee3cebe06e4c000) }, { INT64_C(0x2b8ef77cca031c00) },
	{ INT64_C(0x3a2fcee87c6bb800) }, { INT64_C(0x1aa6c82b6d3fc900) }, { -INT64_C(0x1aa6c82b6d3fc600) }, { INT64_C(0x3a2fcee87c6bba00) },
	{ INT64_C(0x164c7ddd3f27c500) }, { INT64_C(0x3bfd5cc45b7c5600) }, { -INT64_C(0x3bfd5cc45b7c5400) }, { INT64_C(0x164c7ddd3f27ca00) },
	{ INT64_C(0x3e44a5eeec75b400) }, { INT64_C(0x0ec9a7f2a2a18880) }, { -INT64_C(0x0ec9a7f2a2a18900) }, { INT64_C(0x3e44a5eeec75b400) },
	{ INT64_C(0x2192e09abb131c00) }, { INT64_C(0x367c9a7deaae2400) }, { -INT64_C(0x367c9a7deaae2400) }, { INT64_C(0x2192e09abb131e00) },
	{ INT64_C(0x33de87de535f2800) }, { INT64_C(0x257db64bf5e7d400) }, { -INT64_C(0x257db64bf5e7d400) }, { INT64_C(0x33de87de535f2800) },
	{ INT64_C(0x0a2abb58949f2c00) }, { INT64_C(0x3f2ff24992133600) }, { -INT64_C(0x3f2ff24992133600) }, { INT64_C(0x0a2abb58949f2d80) },
	{ INT64_C(0x3f6af2e32bae8200) }, { INT64_C(0x089cf8676d7abb00) }, { -INT64_C(0x089cf8676d7aba00) }, { INT64_C(0x3f6af2e32bae8200) },
	{ INT64_C(0x26c0b1620cb3e600) }, { INT64_C(0x32eefde98fae8200) }, { -INT64_C(0x32eefde98fae8200) }, { INT64_C(0x26c0b1620cb3e800) },
	{ INT64_C(0x374b54ce6b21a600) }, { INT64_C(0x2039f90e987d6e00) }, { -INT64_C(0x2039f90e987d6c00) }, { INT64_C(0x374b54ce6b21a600) },
	{ INT64_C(0x104fb80e37fdaf00) }, { INT64_C(0x3de2f147c8e78400) }, { -INT64_C(0x3de2f147c8e78400) }, { INT64_C(0x104fb80e37fdb000) },
	{ INT64_C(0x3c84d4965782fc00) }, { INT64_C(0x14d1e24278e76a00) }, { -INT64_C(0x14d1e24278e76900) }, { INT64_C(0x3c84d4965782fe00) },
	{ INT64_C(0x1c1249d8011ee800) }, { INT64_C(0x3983e1e7f9f8b800) }, { -INT64_C(0x3983e1e7f9f8ba00) }, { INT64_C(0x1c1249d8011ee500) },
	{ INT64_C(0x2ff1d9c6ae2ee200) }, { INT64_C(0x2a65052546ab2c00) }, { -INT64_C(0x2a65052546ab2e00) }, { INT64_C(0x2ff1d9c6ae2ee000) },
	{ INT64_C(0x03ecadcf3f041d40) }, { INT64_C(0x3fe12acb1ce35a00) }, { -INT64_C(0x3fe12acb1ce35a00) }, { INT64_C(0x03ecadcf3f041a60) },
	{ INT64_C(0x3fc395f97ab61200) }, { INT64_C(0x057db402a6a90640) }, { -INT64_C(0x057db402a6a903c0) }, { INT64_C(0x3fc395f97ab61200) },
	{ INT64_C(0x2934893736127200) }, { INT64_C(0x30f8801f745d7e00) }, { -INT64_C(0x30f8801f745d7e00) }, { INT64_C(0x2934893736127000) },
	{ INT64_C(0x38cf166910e73600) }, { INT64_C(0x1d79775b86e38900) }, { -INT64_C(0x1d79775b86e38a00) }, { INT64_C(0x38cf166910e73600) },
	{ INT64_C(0x135410c2e1815400) }, { INT64_C(0x3d02f75699a21800) }, { -INT64_C(0x3d02f75699a21a00) }, { INT64_C(0x135410c2e1815100) },
	{ INT64_C(0x3d77b191be16e800) }, { INT64_C(0x11d3443f4cdb3d00) }, { -INT64_C(0x11d3443f4cdb3b00) }, { INT64_C(0x3d77b191be16ea00) },
	{ INT64_C(0x1edc1952ef78d700) }, { INT64_C(0x3811884ce4aa9200) }, { -INT64_C(0x3811884ce4aa9000) }, { INT64_C(0x1edc1952ef78d800) },
	{ INT64_C(0x31f79947df281a00) }, { INT64_C(0x27fdb2a68aada800) }, { -INT64_C(0x27fdb2a68aada600) }, { INT64_C(0x31f79947df281c00) },
	{ INT64_C(0x070de171e7b0b780) }, { INT64_C(0x3f9c2bfadb4cf600) }, { -INT64_C(0x3f9c2bfadb4cf600) }, { INT64_C(0x070de171e7b0b880) },
	{ INT64_C(0x3eeb33474240ee00) }, { INT64_C(0x0bb6ecef285f9880) }, { -INT64_C(0x0bb6ecef285f9800) }, { INT64_C(0x3eeb33474240ee00) },
	{ INT64_C(0x2434f33267d6b200) }, { INT64_C(0x34c6123605f5c400) }, { -INT64_C(0x34c6123605f5c000) }, { INT64_C(0x2434f33267d6b600) },
	{ INT64_C(0x35a5793c43aa2200) }, { INT64_C(0x22e69ac7bdb69200) }, { -INT64_C(0x22e69ac7bdb68e00) }, { INT64_C(0x35a5793c43aa2400) },
	{ INT64_C(0x0d415012d8022880) }, { INT64_C(0x3e9cc076165e5a00) }, { -INT64_C(0x3e9cc076165e5800) }, { INT64_C(0x0d415012d8022d80) },
	{ INT64_C(0x3b6ca4c471413600) }, { INT64_C(0x17c3a9311dcce700) }, { -INT64_C(0x17c3a9311dcce600) }, { INT64_C(0x3b6ca4c471413600) },
	{ INT64_C(0x19372a63bc93d700) }, { INT64_C(0x3ad2c2e793cd1600) }, { -INT64_C(0x3ad2c2e793cd1600) }, { INT64_C(0x19372a63bc93d800) },
	{ INT64_C(0x2dce88a9d5515c00) }, { INT64_C(0x2cb2324be0f07c00) }, { -INT64_C(0x2cb2324be0f07a00) }, { INT64_C(0x2dce88a9d5515e00) },
	{ INT64_C(0x00c90e8fe6f63c78) }, { INT64_C(0x3ffec42d3725b600) }, { -INT64_C(0x3ffec42d3725b600) }, { INT64_C(0x00c90e8fe6f63d98) },
	{ INT64_C(0x3fffb10b1d152400) }, { INT64_C(0x006487c3f99c01c4) }, { -INT64_C(0x006487c3f99c0048) }, { INT64_C(0x3fffb10b1d152400) },
	{ INT64_C(0x2cf9ef09235e2000) }, { INT64_C(0x2d881ae78304ea00) }, { -INT64_C(0x2d881ae78304ec00) }, { INT64_C(0x2cf9ef09235e1e00) },
	{ INT64_C(0x3afa160571d9f400) }, { INT64_C(0x18daa52ec8a4af00) }, { -INT64_C(0x18daa52ec8a4ae00) }, { INT64_C(0x3afa160571d9f400) },
	{ INT64_C(0x1820e3b04eaac500) }, { INT64_C(0x3b470752cd130e00) }, { -INT64_C(0x3b470752cd131000) }, { INT64_C(0x1820e3b04eaac200) },
	{ INT64_C(0x3eb14562f13d0800) }, { INT64_C(0x0cdee5f96e21b300) }, { -INT64_C(0x0cdee5f96e21b180) }, { INT64_C(0x3eb14562f13d0800) },
	{ INT64_C(0x233ab413e5737000) }, { INT64_C(0x356e64b22d81a800) }, { -INT64_C(0x356e64b22d81a800) }, { INT64_C(0x233ab413e5737200) },
	{ INT64_C(0x34feb0a53fcd3a00) }, { INT64_C(0x23e1e117790c3600) }, { -INT64_C(0x23e1e117790c3400) }, { INT64_C(0x34feb0a53fcd3a00) },
	{ INT64_C(0x0c19b3744e326400) }, { INT64_C(0x3ed87efbeb776e00) }, { -INT64_C(0x3ed87efbeb776e00) }, { INT64_C(0x0c19b3744e326580) },
	{ INT64_C(0x3fa6f22844170800) }, { INT64_C(0x06a9edc912570100) }, { -INT64_C(0x06a9edc912570140) }, { INT64_C(0x3fa6f22844170800) },
	{ INT64_C(0x284bfe2f1cd76400) }, { INT64_C(0x31b88a662d319800) }, { -INT64_C(0x31b88a662d319600) }, { INT64_C(0x284bfe2f1cd76600) },
	{ INT64_C(0x3841bc7f52e36000) }, { INT64_C(0x1e83e0eaf8511300) }, { -INT64_C(0x1e83e0eaf8511000) }, { INT64_C(0x3841bc7f52e36000) },
	{ INT64_C(0x1233bbabc3bb7100) }, { INT64_C(0x3d5b65d1cf511c00) }, { -INT64_C(0x3d5b65d1cf511a00) }, { INT64_C(0x1233bbabc3bb7600) },
	{ INT64_C(0x3d21086c3befe600) }, { INT64_C(0x12f422daec038600) }, { -INT64_C(0x12f422daec038700) }, { INT64_C(0x3d21086c3befe400) },
	{ INT64_C(0x1dd28f1481cc5700) }, { INT64_C(0x38a0840256d20e00) }, { -INT64_C(0x38a0840256d20e00) }, { INT64_C(0x1dd28f1481cc5800) },
	{ INT64_C(0x3138fd349ba95600) }, { INT64_C(0x28e76a3730e68e00) }, { -INT64_C(0x28e76a3730e68e00) }, { INT64_C(0x3138fd349ba95400) },
	{ INT64_C(0x05e1d61a9756c7c0) }, { INT64_C(0x3fbaa73fe3e8ac00) }, { -INT64_C(0x3fbaa73fe3e8ac00) }, { INT64_C(0x05e1d61a9756c8c0) },
	{ INT64_C(0x3fe7061f1aaeb800) }, { INT64_C(0x038851a2581afc40) }, { -INT64_C(0x038851a2581afbe0) }, { INT64_C(0x3fe7061f1aaeb800) },
	{ INT64_C(0x2ab020712ea26e00) }, { INT64_C(0x2faf06d9867b6400) }, { -INT64_C(0x2faf06d9867b6400) }, { INT64_C(0x2ab020712ea27000) },
	{ INT64_C(0x39afb3131665ec00) }, { INT64_C(0x1bb7cf2304bd0100) }, { -INT64_C(0x1bb7cf2304bd0000) }, { INT64_C(0x39afb3131665ec00) },
	{ INT64_C(0x1530d880af3c2400) }, { INT64_C(0x3c63d5d0e19c4a00) }, { -INT64_C(0x3c63d5d0e19c4a00) }, { INT64_C(0x1530d880af3c2500) },
	{ INT64_C(0x3dfc4418172bda00) }, { INT64_C(0x0fee6e0d6ff6fc00) }, { -INT64_C(0x0fee6e0d6ff6fb80) }, { INT64_C(0x3dfc4418172bda00) },
	{ INT64_C(0x2090ac4d5c443400) }, { INT64_C(0x371871a4ea09d200) }, { -INT64_C(0x371871a4ea09ce00) }, { INT64_C(0x2090ac4d5c443a00) },
	{ INT64_C(0x332b9e5db01a4400) }, { INT64_C(0x2670801a191cae00) }, { -INT64_C(0x2670801a191caa00) }, { INT64_C(0x332b9e5db01a4800) },
	{ INT64_C(0x09008b6a763de780) }, { INT64_C(0x3f5d1d1c8d9f7600) }, { -INT64_C(0x3f5d1d1c8d9f7400) }, { INT64_C(0x09008b6a763ded00) },
	{ INT64_C(0x3f3f9cab5b659000) }, { INT64_C(0x09c76dd866c68a00) }, { -INT64_C(0x09c76dd866c68780) }, { INT64_C(0x3f3f9cab5b659000) },
	{ INT64_C(0x25cf01c7d1d42e00) }, { INT64_C(0x33a363ebd501aa00) }, { -INT64_C(0x33a363ebd501ac00) }, { INT64_C(0x25cf01c7d1d42c00) },
	{ INT64_C(0x36b113fd24280a00) }, { INT64_C(0x213d20e82f8bc200) }, { -INT64_C(0x213d20e82f8bc200) }, { INT64_C(0x36b113fd24280a00) },
	{ INT64_C(0x0f2b650f080d1000) }, { INT64_C(0x3e2d1ea7ee40ba00) }, { -INT64_C(0x3e2d1ea7ee40ba00) }, { INT64_C(0x0f2b650f080d0d00) },
	{ INT64_C(0x3c20199453015800) }, { INT64_C(0x15ee27379ea69300) }, { -INT64_C(0x15ee27379ea69100) }, { INT64_C(0x3c20199453015800) },
	{ INT64_C(0x1b020d6c7f400b00) }, { INT64_C(0x3a05a9fd657b2400) }, { -INT64_C(0x3a05a9fd657b2400) }, { INT64_C(0x1b020d6c7f400c00) },
	{ INT64_C(0x2f2800ae9eabca00) }, { INT64_C(0x2b451a54bae4a000) }, { -INT64_C(0x2b451a54bae49e00) }, { INT64_C(0x2f2800ae9eabcc00) },
	{ INT64_C(0x02bf801a5219aae0) }, { INT64_C(0x3ff0e3b5b703be00) }, { -INT64_C(0x3ff0e3b5b703be00) }, { INT64_C(0x02bf801a5219ac00) },
	{ INT64_C(0x3ff84a3be3a7f000) }, { INT64_C(0x01f693731d1cf010) }, { -INT64_C(0x01f693731d1ced10) }, { INT64_C(0x3ff84a3be3a7f000) },
	{ INT64_C(0x2bd8692b06e0e800) }, { INT64_C(0x2e9f291b51a51a00) }, { -INT64_C(0x2e9f291b51a51a00) }, { INT64_C(0x2bd8692b06e0e800) },
	{ INT64_C(0x3a596441c1df3e00) }, { INT64_C(0x1a4b4127dea1e400) }, { -INT64_C(0x1a4b4127dea1e200) }, { INT64_C(0x3a596441c1df3e00) },
	{ INT64_C(0x16aa9d7dc77e1900) }, { INT64_C(0x3bda0befbc8fb200) }, { -INT64_C(0x3bda0befbc8fb400) }, { INT64_C(0x16aa9d7dc77e1700) },
	{ INT64_C(0x3e5b939211353a00) }, { INT64_C(0x0e67c65989594300) }, { -INT64_C(0x0e67c65989594000) }, { INT64_C(0x3e5b939211353a00) },
	{ INT64_C(0x21e84d76551cfe00) }, { INT64_C(0x36479a8e00276600) }, { -INT64_C(0x36479a8e00276600) }, { INT64_C(0x21e84d76551cfe00) },
	{ INT64_C(0x34192bd575f26e00) }, { INT64_C(0x252c0e4ec5395000) }, { -INT64_C(0x252c0e4ec5394e00) }, { INT64_C(0x34192bd575f27000) },
	{ INT64_C(0x0a8defc2cbe2fc00) }, { INT64_C(0x3f1fabff5c83b600) }, { -INT64_C(0x3f1fabff5c83b400) }, { INT64_C(0x0a8defc2cbe2fd00) },
	{ INT64_C(0x3f782c2fc8830c00) }, { INT64_C(0x08395023dd418e80) }, { -INT64_C(0x08395023dd418d80) }, { INT64_C(0x3f782c2fc8830c00) },
	{ INT64_C(0x2710830bbfd64400) }, { INT64_C(0x32b1dfc91cdbae00) }, { -INT64_C(0x32b1dfc91cdbaa00) }, { INT64_C(0x2710830bbfd64800) },
	{ INT64_C(0x377daf892701d400) }, { INT64_C(0x1fe2f64be7121000) }, { -INT64_C(0x1fe2f64be7120b00) }, { INT64_C(0x377daf892701d600) },
	{ INT64_C(0x10b0d9cfdbdb9100) }, { INT64_C(0x3dc905c4b53b7800) }, { -INT64_C(0x3dc905c4b53b7600) }, { INT64_C(0x10b0d9cfdbdb9600) },
	{ INT64_C(0x3ca53e08e53ff800) }, { INT64_C(0x1472b8a557105400) }, { -INT64_C(0x1472b8a557105300) }, { INT64_C(0x3ca53e08e53ffa00) },
	{ INT64_C(0x1c6c7f4997000b00) }, { INT64_C(0x395782d341720000) }, { -INT64_C(0x395782d341720000) }, { INT64_C(0x1c6c7f4997000c00) },
	{ INT64_C(0x303436676af59800) }, { INT64_C(0x2a19813eb3413600) }, { -INT64_C(0x2a19813eb3413600) }, { INT64_C(0x303436676af59800) },
	{ INT64_C(0x0451004d35c26d80) }, { INT64_C(0x3fdab1d96ce78800) }, { -INT64_C(0x3fdab1d96ce78800) }, { INT64_C(0x0451004d35c26ec0) },
	{ INT64_C(0x3fcbe75e5c728000) }, { INT64_C(0x0519845e49c82580) }, { -INT64_C(0x0519845e49c82380) }, { INT64_C(0x3fcbe75e5c728200) },
	{ INT64_C(0x2981428bd8000a00) }, { INT64_C(0x30b78a35d2b19800) }, { -INT64_C(0x30b78a35d2b19600) }, { INT64_C(0x2981428bd8000a00) },
	{ INT64_C(0x38fd1ca44679e600) }, { INT64_C(0x1d2016e8e9db5b00) }, { -INT64_C(0x1d2016e8e9db5900) }, { INT64_C(0x38fd1ca44679e800) },
	{ INT64_C(0x13b3cefa0414b900) }, { INT64_C(0x3ce44fb6d52e8800) }, { -INT64_C(0x3ce44fb6d52e8800) }, { INT64_C(0x13b3cefa0414ba00) },
	{ INT64_C(0x3d9365a7877f0800) }, { INT64_C(0x1172a0d776517700) }, { -INT64_C(0x1172a0d776517500) }, { INT64_C(0x3d9365a7877f0800) },
	{ INT64_C(0x1f3405963fd06900) }, { INT64_C(0x37e0c9c2a6efba00) }, { -INT64_C(0x37e0c9c2a6efba00) }, { INT64_C(0x1f3405963fd06600) },
	{ INT64_C(0x32362cdfa93b4400) }, { INT64_C(0x27af04718b068600) }, { -INT64_C(0x27af04718b068800) }, { INT64_C(0x32362cdfa93b4200) },
	{ INT64_C(0x0771c3b2eba7f440) }, { INT64_C(0x3f90c8d9fd6e4200) }, { -INT64_C(0x3f90c8d9fd6e4200) }, { INT64_C(0x0771c3b2eba7f140) },
	{ INT64_C(0x3efd4c53cc7adc00) }, { INT64_C(0x0b5409827b255900) }, { -INT64_C(0x0b5409827b255900) }, { INT64_C(0x3efd4c53cc7adc00) },
	{ INT64_C(0x2487abf731584000) }, { INT64_C(0x348cf19023359000) }, { -INT64_C(0x348cf19023359000) }, { INT64_C(0x2487abf731583e00) },
	{ INT64_C(0x35dc09687828e800) }, { INT64_C(0x22922b5e66d9d600) }, { -INT64_C(0x22922b5e66d9d600) }, { INT64_C(0x35dc09687828e800) },
	{ INT64_C(0x0da399779eb39100) }, { INT64_C(0x3e87a10bff25ba00) }, { -INT64_C(0x3e87a10bff25ba00) }, { INT64_C(0x0da399779eb39200) },
	{ INT64_C(0x3b91af968204c000) }, { INT64_C(0x1766340f2418f600) }, { -INT64_C(0x1766340f2418f600) }, { INT64_C(0x3b91af968204c000) },
	{ INT64_C(0x1993716141bdfe00) }, { INT64_C(0x3aaadea5d27d6200) }, { -INT64_C(0x3aaadea5d27d6000) }, { INT64_C(0x1993716141be0300) },
	{ INT64_C(0x2e1485662edaf400) }, { INT64_C(0x2c6a07463837d200) }, { -INT64_C(0x2c6a07463837ce00) }, { INT64_C(0x2e1485662edaf600) },
	{ INT64_C(0x012d936bbe30efd0) }, { INT64_C(0x3ffd396896a34200) }, { -INT64_C(0x3ffd396896a34200) }, { INT64_C(0x012d936bbe30f4e0) },
	{ INT64_C(0x3ffd396896a34200) }, { INT64_C(0x012d936bbe30efd0) }, { -INT64_C(0x012d936bbe30ed90) }, { INT64_C(0x3ffd396896a34200) },
	{ INT64_C(0x2c6a07463837d200) }, { INT64_C(0x2e1485662edaf400) }, { -INT64_C(0x2e1485662edaf400) }, { INT64_C(0x2c6a07463837d200) },
	{ INT64_C(0x3aaadea5d27d6200) }, { INT64_C(0x1993716141bdfe00) }, { -INT64_C(0x1993716141bdfc00) }, { INT64_C(0x3aaadea5d27d6200) },
	{ INT64_C(0x1766340f2418f800) }, { INT64_C(0x3b91af968204c000) }, { -INT64_C(0x3b91af968204c000) }, { INT64_C(0x1766340f2418f500) },
	{ INT64_C(0x3e87a10bff25ba00) }, { INT64_C(0x0da399779eb39100) }, { -INT64_C(0x0da399779eb38f00) }, { INT64_C(0x3e87a10bff25ba00) },
	{ INT64_C(0x22922b5e66d9d800) }, { INT64_C(0x35dc09687828e600) }, { -INT64_C(0x35dc09687828e600) }, { INT64_C(0x22922b5e66d9d800) },
	{ INT64_C(0x348cf19023359000) }, { INT64_C(0x2487abf731583e00) }, { -INT64_C(0x2487abf731583c00) }, { INT64_C(0x348cf19023359200) },
	{ INT64_C(0x0b5409827b255b00) }, { INT64_C(0x3efd4c53cc7adc00) }, { -INT64_C(0x3efd4c53cc7adc00) }, { INT64_C(0x0b5409827b255c80) },
	{ INT64_C(0x3f90c8d9fd6e4200) }, { INT64_C(0x0771c3b2eba7f200) }, { -INT64_C(0x0771c3b2eba7f200) }, { INT64_C(0x3f90c8d9fd6e4200) },
	{ INT64_C(0x27af04718b068800) }, { INT64_C(0x32362cdfa93b4400) }, { -INT64_C(0x32362cdfa93b4000) }, { INT64_C(0x27af04718b068c00) },
	{ INT64_C(0x37e0c9c2a6efba00) }, { INT64_C(0x1f3405963fd06800) }, { -INT64_C(0x1f3405963fd06300) }, { INT64_C(0x37e0c9c2a6efbc00) },
	{ INT64_C(0x1172a0d776517700) }, { INT64_C(0x3d9365a7877f0800) }, { -INT64_C(0x3d9365a7877f0600) }, { INT64_C(0x1172a0d776517c00) },
	{ INT64_C(0x3ce44fb6d52e8800) }, { INT64_C(0x13b3cefa0414b700) }, { -INT64_C(0x13b3cefa0414b700) }, { INT64_C(0x3ce44fb6d52e8800) },
	{ INT64_C(0x1d2016e8e9db5b00) }, { INT64_C(0x38fd1ca44679e600) }, { -INT64_C(0x38fd1ca44679e600) }, { INT64_C(0x1d2016e8e9db5c00) },
	{ INT64_C(0x30b78a35d2b19800) }, { INT64_C(0x2981428bd8000800) }, { -INT64_C(0x2981428bd8000800) }, { INT64_C(0x30b78a35d2b19a00) },
	{ INT64_C(0x0519845e49c82580) }, { INT64_C(0x3fcbe75e5c728000) }, { -INT64_C(0x3fcbe75e5c728000) }, { INT64_C(0x0519845e49c826c0) },
	{ INT64_C(0x3fdab1d96ce78800) }, { INT64_C(0x0451004d35c26c80) }, { -INT64_C(0x0451004d35c26b40) }, { INT64_C(0x3fdab1d96ce78800) },
	{ INT64_C(0x2a19813eb3413600) }, { INT64_C(0x303436676af59600) }, { -INT64_C(0x303436676af59600) }, { INT64_C(0x2a19813eb3413800) },
	{ INT64_C(0x395782d341720200) }, { INT64_C(0x1c6c7f4997000a00) }, { -INT64_C(0x1c6c7f4997000900) }, { INT64_C(0x395782d341720200) },
	{ INT64_C(0x1472b8a557105500) }, { INT64_C(0x3ca53e08e53ff800) }, { -INT64_C(0x3ca53e08e53ff800) }, { INT64_C(0x1472b8a557105600) },
	{ INT64_C(0x3dc905c4b53b7800) }, { INT64_C(0x10b0d9cfdbdb9000) }, { -INT64_C(0x10b0d9cfdbdb8f00) }, { INT64_C(0x3dc905c4b53b7800) },
	{ INT64_C(0x1fe2f64be7121000) }, { INT64_C(0x377daf892701d400) }, { -INT64_C(0x377daf892701d600) }, { INT64_C(0x1fe2f64be7120e00) },
	{ INT64_C(0x32b1dfc91cdbae00) }, { INT64_C(0x2710830bbfd64400) }, { -INT64_C(0x2710830bbfd64600) }, { INT64_C(0x32b1dfc91cdbac00) },
	{ INT64_C(0x08395023dd418f80) }, { INT64_C(0x3f782c2fc8830c00) }, { -INT64_C(0x3f782c2fc8830c00) }, { INT64_C(0x08395023dd418d00) },
	{ INT64_C(0x3f1fabff5c83b600) }, { INT64_C(0x0a8defc2cbe2f900) }, { -INT64_C(0x0a8defc2cbe2f980) }, { INT64_C(0x3f1fabff5c83b600) },
	{ INT64_C(0x252c0e4ec5395000) }, { INT64_C(0x34192bd575f26c00) }, { -INT64_C(0x34192bd575f26e00) }, { INT64_C(0x252c0e4ec5395000) },
	{ INT64_C(0x36479a8e00276800) }, { INT64_C(0x21e84d76551cfa00) }, { -INT64_C(0x21e84d76551cfc00) }, { INT64_C(0x36479a8e00276800) },
	{ INT64_C(0x0e67c65989594200) }, { INT64_C(0x3e5b939211353a00) }, { -INT64_C(0x3e5b939211353a00) }, { INT64_C(0x0e67c65989594300) },
	{ INT64_C(0x3bda0befbc8fb400) }, { INT64_C(0x16aa9d7dc77e1600) }, { -INT64_C(0x16aa9d7dc77e1700) }, { INT64_C(0x3bda0befbc8fb400) },
	{ INT64_C(0x1a4b4127dea1e400) }, { INT64_C(0x3a596441c1df3e00) }, { -INT64_C(0x3a596441c1df3c00) }, { INT64_C(0x1a4b4127dea1e800) },
	{ INT64_C(0x2e9f291b51a51a00) }, { INT64_C(0x2bd8692b06e0e800) }, { -INT64_C(0x2bd8692b06e0e600) }, { INT64_C(0x2e9f291b51a51c00) },
	{ INT64_C(0x01f693731d1cef40) }, { INT64_C(0x3ff84a3be3a7f000) }, { -INT64_C(0x3ff84a3be3a7f000) }, { INT64_C(0x01f693731d1cf460) },
	{ INT64_C(0x3ff0e3b5b703be00) }, { INT64_C(0x02bf801a5219a860) }, { -INT64_C(0x02bf801a5219a8a0) }, { INT64_C(0x3ff0e3b5b703be00) },
	{ INT64_C(0x2b451a54bae4a200) }, { INT64_C(0x2f2800ae9eabc800) }, { -INT64_C(0x2f2800ae9eabca00) }, { INT64_C(0x2b451a54bae4a000) },
	{ INT64_C(0x3a05a9fd657b2400) }, { INT64_C(0x1b020d6c7f400900) }, { -INT64_C(0x1b020d6c7f400900) }, { INT64_C(0x3a05a9fd657b2400) },
	{ INT64_C(0x15ee27379ea69300) }, { INT64_C(0x3c20199453015800) }, { -INT64_C(0x3c20199453015800) }, { INT64_C(0x15ee27379ea69400) },
	{ INT64_C(0x3e2d1ea7ee40ba00) }, { INT64_C(0x0f2b650f080d0d80) }, { -INT64_C(0x0f2b650f080d0e00) }, { INT64_C(0x3e2d1ea7ee40ba00) },
	{ INT64_C(0x213d20e82f8bc000) }, { INT64_C(0x36b113fd24280a00) }, { -INT64_C(0x36b113fd24280800) }, { INT64_C(0x213d20e82f8bc400) },
	{ INT64_C(0x33a363ebd501ac00) }, { INT64_C(0x25cf01c7d1d42c00) }, { -INT64_C(0x25cf01c7d1d42a00) }, { INT64_C(0x33a363ebd501ae00) },
	{ INT64_C(0x09c76dd866c68980) }, { INT64_C(0x3f3f9cab5b659000) }, { -INT64_C(0x3f3f9cab5b659000) }, { INT64_C(0x09c76dd866c68e80) },
	{ INT64_C(0x3f5d1d1c8d9f7600) }, { INT64_C(0x09008b6a763de700) }, { -INT64_C(0x09008b6a763de580) }, { INT64_C(0x3f5d1d1c8d9f7600) },
	{ INT64_C(0x2670801a191cae00) }, { INT64_C(0x332b9e5db01a4400) }, { -INT64_C(0x332b9e5db01a4600) }, { INT64_C(0x2670801a191cac00) },
	{ INT64_C(0x371871a4ea09d200) }, { INT64_C(0x2090ac4d5c443400) }, { -INT64_C(0x2090ac4d5c443600) }, { INT64_C(0x371871a4ea09d000) },
	{ INT64_C(0x0fee6e0d6ff6fe00) }, { INT64_C(0x3dfc4418172bd800) }, { -INT64_C(0x3dfc4418172bda00) }, { INT64_C(0x0fee6e0d6ff6fb00) },
	{ INT64_C(0x3c63d5d0e19c4a00) }, { INT64_C(0x1530d880af3c2400) }, { -INT64_C(0x1530d880af3c2200) }, { INT64_C(0x3c63d5d0e19c4a00) },
	{ INT64_C(0x1bb7cf2304bd0200) }, { INT64_C(0x39afb3131665ec00) }, { -INT64_C(0x39afb3131665ea00) }, { INT64_C(0x1bb7cf2304bd0300) },
	{ INT64_C(0x2faf06d9867b6600) }, { INT64_C(0x2ab020712ea26e00) }, { -INT64_C(0x2ab020712ea26c00) }, { INT64_C(0x2faf06d9867b6600) },
	{ INT64_C(0x038851a2581afe00) }, { INT64_C(0x3fe7061f1aaeb800) }, { -INT64_C(0x3fe7061f1aaeb800) }, { INT64_C(0x038851a2581aff20) },
	{ INT64_C(0x3fbaa73fe3e8ac00) }, { INT64_C(0x05e1d61a9756c840) }, { -INT64_C(0x05e1d61a9756c580) }, { INT64_C(0x3fbaa73fe3e8ac00) },
	{ INT64_C(0x28e76a3730e68e00) }, { INT64_C(0x3138fd349ba95400) }, { -INT64_C(0x3138fd349ba95200) }, { INT64_C(0x28e76a3730e69000) },
	{ INT64_C(0x38a0840256d20e00) }, { INT64_C(0x1dd28f1481cc5800) }, { -INT64_C(0x1dd28f1481cc5500) }, { INT64_C(0x38a0840256d21000) },
	{ INT64_C(0x12f422daec038900) }, { INT64_C(0x3d21086c3befe400) }, { -INT64_C(0x3d21086c3befe400) }, { INT64_C(0x12f422daec038a00) },
	{ INT64_C(0x3d5b65d1cf511c00) }, { INT64_C(0x1233bbabc3bb7200) }, { -INT64_C(0x1233bbabc3bb6f00) }, { INT64_C(0x3d5b65d1cf511c00) },
	{ INT64_C(0x1e83e0eaf8511600) }, { INT64_C(0x3841bc7f52e35e00) }, { -INT64_C(0x3841bc7f52e36000) }, { INT64_C(0x1e83e0eaf8511300) },
	{ INT64_C(0x31b88a662d319800) }, { INT64_C(0x284bfe2f1cd76200) }, { -INT64_C(0x284bfe2f1cd76400) }, { INT64_C(0x31b88a662d319800) },
	{ INT64_C(0x06a9edc912570380) }, { INT64_C(0x3fa6f22844170800) }, { -INT64_C(0x3fa6f22844170800) }, { INT64_C(0x06a9edc9125700c0) },
	{ INT64_C(0x3ed87efbeb776e00) }, { INT64_C(0x0c19b3744e326280) }, { -INT64_C(0x0c19b3744e326200) }, { INT64_C(0x3ed87efbeb776e00) },
	{ INT64_C(0x23e1e117790c3600) }, { INT64_C(0x34feb0a53fcd3a00) }, { -INT64_C(0x34feb0a53fcd3800) }, { INT64_C(0x23e1e117790c3800) },
	{ INT64_C(0x356e64b22d81a800) }, { INT64_C(0x233ab413e5737000) }, { -INT64_C(0x233ab413e5736e00) }, { INT64_C(0x356e64b22d81aa00) },
	{ INT64_C(0x0cdee5f96e21b400) }, { INT64_C(0x3eb14562f13d0800) }, { -INT64_C(0x3eb14562f13d0800) }, { INT64_C(0x0cdee5f96e21b500) },
	{ INT64_C(0x3b470752cd131000) }, { INT64_C(0x1820e3b04eaac400) }, { -INT64_C(0x1820e3b04eaac300) }, { INT64_C(0x3b470752cd131000) },
	{ INT64_C(0x18daa52ec8a4b000) }, { INT64_C(0x3afa160571d9f200) }, { -INT64_C(0x3afa160571d9f000) }, { INT64_C(0x18daa52ec8a4b500) },
	{ INT64_C(0x2d881ae78304ea00) }, { INT64_C(0x2cf9ef09235e2000) }, { -INT64_C(0x2cf9ef09235e1c00) }, { INT64_C(0x2d881ae78304ee00) },
	{ INT64_C(0x006487c3f99c027c) }, { INT64_C(0x3fffb10b1d152400) }, { -INT64_C(0x3fffb10b1d152400) }, { INT64_C(0x006487c3f99c0798) },
	{ INT64_C(0x3fffec42c43a0400) }, { INT64_C(0x003243f17d994974) }, { -INT64_C(0x003243f17d994a2a) }, { INT64_C(0x3fffec42c43a0400) },
	{ INT64_C(0x2d1da3d54338e400) }, { INT64_C(0x2d64b9da5fc53600) }, { -INT64_C(0x2d64b9da5fc53400) }, { INT64_C(0x2d1da3d54338e600) },
	{ INT64_C(0x3b0d89088b48a000) }, { INT64_C(0x18ac4b86d5ed4400) }, { -INT64_C(0x18ac4b86d5ed4500) }, { INT64_C(0x3b0d89088b489e00) },
	{ INT64_C(0x184f6aaaf3903e00) }, { INT64_C(0x3b3401bb167a8c00) }, { -INT64_C(0x3b3401bb167a8a00) }, { INT64_C(0x184f6aaaf3904300) },
	{ INT64_C(0x3ebb4dda86d0ba00) }, { INT64_C(0x0cada4f4db157c80) }, { -INT64_C(0x0cada4f4db157d80) }, { INT64_C(0x3ebb4dda86d0ba00) },
	{ INT64_C(0x2364a02e26e77e00) }, { INT64_C(0x3552a8f459731c00) }, { -INT64_C(0x3552a8f459731c00) }, { INT64_C(0x2364a02e26e77e00) },
	{ INT64_C(0x351acedca8b5a400) }, { INT64_C(0x23b836c9b890e400) }, { -INT64_C(0x23b836c9b890e400) }, { INT64_C(0x351acedca8b5a400) },
	{ INT64_C(0x0c4b0b93e20c0100) }, { INT64_C(0x3eceeaad1079dc00) }, { -INT64_C(0x3eceeaad1079dc00) }, { INT64_C(0x0c4b0b93e20c0280) },
	{ INT64_C(0x3fac1a5b4eb93c00) }, { INT64_C(0x0677edbac92a1700) }, { -INT64_C(0x0677edbac92a15c0) }, { INT64_C(0x3fac1a5b4eb93c00) },
	{ INT64_C(0x2872feb654870000) }, { INT64_C(0x3198d4ea308ca800) }, { -INT64_C(0x3198d4ea308ca800) }, { INT64_C(0x2872feb654870000) },
	{ INT64_C(0x3859a29263c7e200) }, { INT64_C(0x1e57a86d3cd82400) }, { -INT64_C(0x1e57a86d3cd82300) }, { INT64_C(0x3859a29263c7e400) },
	{ INT64_C(0x1263e6995554bb00) }, { INT64_C(0x3d4d0727ccb00000) }, { -INT64_C(0x3d4d0727ccb00000) }, { INT64_C(0x1263e6995554bc00) },
	{ INT64_C(0x3d2fd86c02d66000) }, { INT64_C(0x12c41a4e95452000) }, { -INT64_C(0x12c41a4e95451f00) }, { INT64_C(0x3d2fd86c02d66200) },
	{ INT64_C(0x1dfeff66a941de00) }, { INT64_C(0x3889066283800800) }, { -INT64_C(0x3889066283800a00) }, { INT64_C(0x1dfeff66a941dc00) },
	{ INT64_C(0x31590e3dbc3b1000) }, { INT64_C(0x28c0b4d256654400) }, { -INT64_C(0x28c0b4d256654600) }, { INT64_C(0x31590e3dbc3b0e00) },
	{ INT64_C(0x0613e1c4b04c2940) }, { INT64_C(0x3fb5f4ea28a71800) }, { -INT64_C(0x3fb5f4ea28a71800) }, { INT64_C(0x0613e1c4b04c2680) },
	{ INT64_C(0x3fe9b8a9637da600) }, { INT64_C(0x03562037abf062e0) }, { -INT64_C(0x03562037abf06080) }, { INT64_C(0x3fe9b8a9637da600) },
	{ INT64_C(0x2ad586a32ec93c00) }, { INT64_C(0x2f8d7139c5a66600) }, { -INT64_C(0x2f8d7139c5a66600) }, { INT64_C(0x2ad586a32ec93c00) },
	{ INT64_C(0x39c5664f3340c000) }, { INT64_C(0x1b8a7814fd569300) }, { -INT64_C(0x1b8a7814fd569500) }, { INT64_C(0x39c5664f3340c000) },
	{ INT64_C(0x15604012f467b600) }, { INT64_C(0x3c531e887232f400) }, { -INT64_C(0x3c531e887232f400) }, { INT64_C(0x15604012f467b400) },
	{ INT64_C(0x3e08b4299ee71800) }, { INT64_C(0x0fbdba405e9c0080) }, { -INT64_C(0x0fbdba405e9bfe80) }, { INT64_C(0x3e08b4299ee71800) },
	{ INT64_C(0x20bbe7d863717000) }, { INT64_C(0x36fecd0dcf25d200) }, { -INT64_C(0x36fecd0dcf25d200) }, { INT64_C(0x20bbe7d863717000) },
	{ INT64_C(0x3349bf48560d6400) }, { INT64_C(0x264843d8934c3e00) }, { -INT64_C(0x264843d8934c3c00) }, { INT64_C(0x3349bf48560d6600) },
	{ INT64_C(0x09324ca6fe9a0700) }, { INT64_C(0x3f55f79619fbb600) }, { -INT64_C(0x3f55f79619fbb600) }, { INT64_C(0x09324ca6fe9a0800) },
	{ INT64_C(0x3f473758f42f2200) }, { INT64_C(0x0995bdfca28b5380) }, { -INT64_C(0x0995bdfca28b5380) }, { INT64_C(0x3f473758f42f2200) },
	{ INT64_C(0x25f7849688202a00) }, { INT64_C(0x3385a221e0eb8600) }, { -INT64_C(0x3385a221e0eb8200) }, { INT64_C(0x25f7849688202e00) },
	{ INT64_C(0x36cb1e29fb788e00) }, { INT64_C(0x2112224065611800) }, { -INT64_C(0x2112224065611400) }, { INT64_C(0x36cb1e29fb789000) },
	{ INT64_C(0x0f5c35a316f1a400) }, { INT64_C(0x3e212179131ebe00) }, { -INT64_C(0x3e212179131ebc00) }, { INT64_C(0x0f5c35a316f1a900) },
	{ INT64_C(0x3c31405fb8cdb200) }, { INT64_C(0x15bee78b9db3b600) }, { -INT64_C(0x15bee78b9db3b600) }, { INT64_C(0x3c31405fb8cdb200) },
	{ INT64_C(0x1b2f971db3197200) }, { INT64_C(0x39f061d19c8cf600) }, { -INT64_C(0x39f061d19c8cf400) }, { INT64_C(0x1b2f971db3197300) },
	{ INT64_C(0x2f49ee0f7f2fa400) }, { INT64_C(0x2b2003abee47be00) }, { -INT64_C(0x2b2003abee47be00) }, { INT64_C(0x2f49ee0f7f2fa600) },
	{ INT64_C(0x02f1b754b0e8d100) }, { INT64_C(0x3feea7763722c800) }, { -INT64_C(0x3feea7763722c800) }, { INT64_C(0x02f1b754b0e8d220) },
	{ INT64_C(0x3ff9c139c54cf200) }, { INT64_C(0x01c454f4ce53b1c0) }, { -INT64_C(0x01c454f4ce53b100) }, { INT64_C(0x3ff9c139c54cf200) },
	{ INT64_C(0x2bfcf97bcad42000) }, { INT64_C(0x2e7cab1c0f328400) }, { -INT64_C(0x2e7cab1c0f328000) }, { INT64_C(0x2bfcf97bcad42200) },
	{ INT64_C(0x3a6df8f797f7b800) }, { INT64_C(0x1a1d6543b50ac000) }, { -INT64_C(0x1a1d6543b50abf00) }, { INT64_C(0x3a6df8f797f7b800) },
	{ INT64_C(0x16d998638a0cb600) }, { INT64_C(0x3bc82c1edb1b0200) }, { -INT64_C(0x3bc82c1edb1b0000) }, { INT64_C(0x16d998638a0cbb00) },
	{ INT64_C(0x3e66d0b4755de000) }, { INT64_C(0x0e36c829aeba6e00) }, { -INT64_C(0x0e36c829aeba6d80) }, { INT64_C(0x3e66d0b4755de000) },
	{ INT64_C(0x2212e491a152ae00) }, { INT64_C(0x362ce8549945e800) }, { -INT64_C(0x362ce8549945e800) }, { INT64_C(0x2212e491a152ae00) },
	{ INT64_C(0x34364da5814ebc00) }, { INT64_C(0x250317de9a797400) }, { -INT64_C(0x250317de9a797200) }, { INT64_C(0x34364da5814ebc00) },
	{ INT64_C(0x0abf80432a65f000) }, { INT64_C(0x3f174e6f9696ee00) }, { -INT64_C(0x3f174e6f9696ee00) }, { INT64_C(0x0abf80432a65f100) },
	{ INT64_C(0x3f7e8e1e151b1000) }, { INT64_C(0x08077456b7dc2d80) }, { -INT64_C(0x08077456b7dc2a80) }, { INT64_C(0x3f7e8e1e151b1000) },
	{ INT64_C(0x273847c7ac605200) }, { INT64_C(0x329321c75894d800) }, { -INT64_C(0x329321c75894d600) }, { INT64_C(0x273847c7ac605400) },
	{ INT64_C(0x3796a9961a464e00) }, { INT64_C(0x1fb7575c24d2de00) }, { -INT64_C(0x1fb7575c24d2db00) }, { INT64_C(0x3796a9961a465000) },
	{ INT64_C(0x10e15b4e1749d000) }, { INT64_C(0x3dbbd6d40f0ca000) }, { -INT64_C(0x3dbbd6d40f0ca000) }, { INT64_C(0x10e15b4e1749d100) },
	{ INT64_C(0x3cb53aaa08cfa600) }, { INT64_C(0x144310dc8936f000) }, { -INT64_C(0x144310dc8936ee00) }, { INT64_C(0x3cb53aaa08cfa800) },
	{ INT64_C(0x1c997fc386538b00) }, { INT64_C(0x39411e3373889000) }, { -INT64_C(0x39411e3373889200) }, { INT64_C(0x1c997fc386538800) },
	{ INT64_C(0x30553827ea8bfe00) }, { INT64_C(0x29f3984b99490c00) }, { -INT64_C(0x29f3984b99490e00) }, { INT64_C(0x30553827ea8bfe00) },
	{ INT64_C(0x0483259d3b5115c0) }, { INT64_C(0x3fd73a4a60821e00) }, { -INT64_C(0x3fd73a4a60821e00) }, { INT64_C(0x0483259d3b511300) },
	{ INT64_C(0x3fcfd50a905ac000) }, { INT64_C(0x04e767c4b168a640) }, { -INT64_C(0x04e767c4b168a680) }, { INT64_C(0x3fcfd50a905ac000) },
	{ INT64_C(0x29a778dab13f0000) }, { INT64_C(0x3096e2235f07f200) }, { -INT64_C(0x3096e2235f07f400) }, { INT64_C(0x29a778dab13f0000) },
	{ INT64_C(0x3913eb0e05382600) }, { INT64_C(0x1cf34baee1cd2100) }, { -INT64_C(0x1cf34baee1cd2100) }, { INT64_C(0x3913eb0e05382600) },
	{ INT64_C(0x13e39be96ec27100) }, { INT64_C(0x3cd4c38abaa74e00) }, { -INT64_C(0x3cd4c38abaa74e00) }, { INT64_C(0x13e39be96ec27200) },
	{ INT64_C(0x3da106bd33201200) }, { INT64_C(0x11423eefc6937800) }, { -INT64_C(0x11423eefc6937800) }, { INT64_C(0x3da106bd33201200) },
	{ INT64_C(0x1f5fdee656cda200) }, { INT64_C(0x37c836c222a98200) }, { -INT64_C(0x37c836c222a97e00) }, { INT64_C(0x1f5fdee656cda700) },
	{ INT64_C(0x3255483f8b502a00) }, { INT64_C(0x27878893038a2e00) }, { -INT64_C(0x27878893038a2c00) }, { INT64_C(0x3255483f8b502c00) },
	{ INT64_C(0x07a3adff792a8f80) }, { INT64_C(0x3f8adc76fb35ec00) }, { -INT64_C(0x3f8adc76fb35ec00) }, { INT64_C(0x07a3adff792a9480) },
	{ INT64_C(0x3f061e94818c1800) }, { INT64_C(0x0b228d418ec18680) }, { -INT64_C(0x0b228d418ec18480) }, { INT64_C(0x3f061e94818c1800) },
	{ INT64_C(0x24b0e69976e22200) }, { INT64_C(0x34703094b2778a00) }, { -INT64_C(0x34703094b2778c00) }, { INT64_C(0x24b0e69976e21e00) },
	{ INT64_C(0x35f71fb13eaf6c00) }, { INT64_C(0x2267d39fdc4a9c00) }, { -INT64_C(0x2267d39fdc4aa000) }, { INT64_C(0x35f71fb13eaf6c00) },
	{ INT64_C(0x0dd4b19a78aed800) }, { INT64_C(0x3e7cd7783778ca00) }, { -INT64_C(0x3e7cd7783778ca00) }, { INT64_C(0x0dd4b19a78aed500) },
	{ INT64_C(0x3ba3fde71522b400) }, { INT64_C(0x173763c926109200) }, { -INT64_C(0x173763c926109000) }, { INT64_C(0x3ba3fde71522b400) },
	{ INT64_C(0x19c17d440df9f400) }, { INT64_C(0x3a96b63630ea4800) }, { -INT64_C(0x3a96b63630ea4600) }, { INT64_C(0x19c17d440df9f500) },
	{ INT64_C(0x2e37592c1c837e00) }, { INT64_C(0x2c45c89fd845fe00) }, { -INT64_C(0x2c45c89fd845fc00) }, { INT64_C(0x2e37592c1c838000) },
	{ INT64_C(0x015fd4d21fab2420) }, { INT64_C(0x3ffc38d0e196ee00) }, { -INT64_C(0x3ffc38d0e196ee00) }, { INT64_C(0x015fd4d21fab2540) },
	{ INT64_C(0x3ffe12878a77a200) }, { INT64_C(0x00fb514b55ccbe50) }, { -INT64_C(0x00fb514b55ccbe48) }, { INT64_C(0x3ffe12878a77a200) },
	{ INT64_C(0x2c8e2a86fea6b600) }, { INT64_C(0x2df1953392b6ea00) }, { -INT64_C(0x2df1953392b6e800) }, { INT64_C(0x2c8e2a86fea6b800) },
	{ INT64_C(0x3abee2e51114fe00) }, { INT64_C(0x196555b7ab948f00) }, { -INT64_C(0x196555b7ab948f00) }, { INT64_C(0x3abee2e51114fe00) },
	{ INT64_C(0x1794f5e613dfae00) }, { INT64_C(0x3b7f3c872aeb3600) }, { -INT64_C(0x3b7f3c872aeb3400) }, { INT64_C(0x1794f5e613dfb300) },
	{ INT64_C(0x3e92440d79576800) }, { INT64_C(0x0d7278eaf9dcd580) }, { -INT64_C(0x0d7278eaf9dcd580) }, { INT64_C(0x3e92440d79576800) },
	{ INT64_C(0x22bc6dc9b7c57a00) }, { INT64_C(0x35c0d1e68bd9dc00) }, { -INT64_C(0x35c0d1e68bd9de00) }, { INT64_C(0x22bc6dc9b7c57800) },
	{ INT64_C(0x34a9922121ea4800) }, { INT64_C(0x245e5acc5827c200) }, { -INT64_C(0x245e5acc5827c200) }, { INT64_C(0x34a9922121ea4600) },
	{ INT64_C(0x0b857ec684627f80) }, { INT64_C(0x3ef4533834645400) }, { -INT64_C(0x3ef4533834645400) }, { INT64_C(0x0b857ec684628080) },
	{ INT64_C(0x3f968e072286a800) }, { INT64_C(0x073fd4cecc1f7040) }, { -INT64_C(0x073fd4cecc1f6e40) }, { INT64_C(0x3f968e072286a800) },
	{ INT64_C(0x27d667d57c0d0200) }, { INT64_C(0x3216f286aebdfc00) }, { -INT64_C(0x3216f286aebdfa00) }, { INT64_C(0x27d667d57c0d0200) },
	{ INT64_C(0x37f93a4b43628e00) }, { INT64_C(0x1f081906bff7fd00) }, { -INT64_C(0x1f081906bff7fc00) }, { INT64_C(0x37f93a4b43629000) },
	{ INT64_C(0x11a2f7fbe8f24500) }, { INT64_C(0x3d859e9635ed6400) }, { -INT64_C(0x3d859e9635ed6400) }, { INT64_C(0x11a2f7fbe8f24600) },
	{ INT64_C(0x3cf3b6534a2cb400) }, { INT64_C(0x1383f5e353b6aa00) }, { -INT64_C(0x1383f5e353b6a800) }, { INT64_C(0x3cf3b6534a2cb400) },
	{ INT64_C(0x1d4cd02ba8609e00) }, { INT64_C(0x38e62b133d55ae00) }, { -INT64_C(0x38e62b133d55ae00) }, { INT64_C(0x1d4cd02ba8609c00) },
	{ INT64_C(0x30d8143b35432a00) }, { INT64_C(0x295af2a2ce45e000) }, { -INT64_C(0x295af2a2ce45e200) }, { INT64_C(0x30d8143b35432a00) },
	{ INT64_C(0x054b9dd293534480) }, { INT64_C(0x3fc7d257d3b10c00) }, { -INT64_C(0x3fc7d257d3b10c00) }, { INT64_C(0x054b9dd293534180) },
	{ INT64_C(0x3fde020504c32200) }, { INT64_C(0x041ed853918c1900) }, { -INT64_C(0x041ed853918c15c0) }, { INT64_C(0x3fde020504c32200) },
	{ INT64_C(0x2a3f5039b3354c00) }, { INT64_C(0x301316eadca5f400) }, { -INT64_C(0x301316eadca5f600) }, { INT64_C(0x2a3f5039b3354c00) },
	{ INT64_C(0x396dc41401b53200) }, { INT64_C(0x1c3f6d4726312900) }, { -INT64_C(0x1c3f6d4726312a00) }, { INT64_C(0x396dc41401b53200) },
	{ INT64_C(0x14a253d11b82f600) }, { INT64_C(0x3c951bff039cbc00) }, { -INT64_C(0x3c951bff039cbc00) }, { INT64_C(0x14a253d11b82f300) },
	{ INT64_C(0x3dd60e98a14a8800) }, { INT64_C(0x10804e05eb661e00) }, { -INT64_C(0x10804e05eb661b00) }, { INT64_C(0x3dd60e98a14a8a00) },
	{ INT64_C(0x200e81905705c400) }, { INT64_C(0x376493416d881800) }, { -INT64_C(0x376493416d881800) }, { INT64_C(0x200e81905705c400) },
	{ INT64_C(0x32d07e857affc200) }, { INT64_C(0x26e8a63702ff1a00) }, { -INT64_C(0x26e8a63702ff1800) }, { INT64_C(0x32d07e857affc400) },
	{ INT64_C(0x086b26de5933c600) }, { INT64_C(0x3f71a31acd5b6e00) }, { -INT64_C(0x3f71a31acd5b6e00) }, { INT64_C(0x086b26de5933c700) },
	{ INT64_C(0x3f27e29f0b581000) }, { INT64_C(0x0a5c58bfbcfd4400) }, { -INT64_C(0x0a5c58bfbcfd4300) }, { INT64_C(0x3f27e29f0b581000) },
	{ INT64_C(0x2554edd0f5d6b000) }, { INT64_C(0x33fbe9e26293bc00) }, { -INT64_C(0x33fbe9e26293ba00) }, { INT64_C(0x2554edd0f5d6b400) },
	{ INT64_C(0x36622b4be6f7e800) }, { INT64_C(0x21bda17097896a00) }, { -INT64_C(0x21bda17097896600) }, { INT64_C(0x36622b4be6f7ea00) },
	{ INT64_C(0x0e98bba6965ef800) }, { INT64_C(0x3e502ff88c139a00) }, { -INT64_C(0x3e502ff88c139a00) }, { INT64_C(0x0e98bba6965efd00) },
	{ INT64_C(0x3bebc6d5374b3800) }, { INT64_C(0x167b949cad63ca00) }, { -INT64_C(0x167b949cad63c900) }, { INT64_C(0x3bebc6d5374b3800) },
	{ INT64_C(0x1a790cd3dbf31b00) }, { INT64_C(0x3a44ab8dcb49c000) }, { -INT64_C(0x3a44ab8dcb49c000) }, { INT64_C(0x1a790cd3dbf31c00) },
	{ INT64_C(0x2ec18a58608ec800) }, { INT64_C(0x2bb3bdce7c68b800) }, { -INT64_C(0x2bb3bdce7c68b600) }, { INT64_C(0x2ec18a58608ec800) },
	{ INT64_C(0x0228d0bb685830e0) }, { INT64_C(0x3ff6abc84bfb5c00) }, { -INT64_C(0x3ff6abc84bfb5c00) }, { INT64_C(0x0228d0bb68583200) },
	{ INT64_C(0x3ff2f88411802800) }, { INT64_C(0x028d472dff0c2f20) }, { -INT64_C(0x028d472dff0c2da0) }, { INT64_C(0x3ff2f88411802800) },
	{ INT64_C(0x2b6a164c9ee89200) }, { INT64_C(0x2f05f6372166b400) }, { -INT64_C(0x2f05f6372166b600) }, { INT64_C(0x2b6a164c9ee88e00) },
	{ INT64_C(0x3a1ace5eb3a57200) }, { INT64_C(0x1ad473125cdc0800) }, { -INT64_C(0x1ad473125cdc0b00) }, { INT64_C(0x3a1ace5eb3a57000) },
	{ INT64_C(0x161d595c88c20400) }, { INT64_C(0x3c0ecdb2501ea800) }, { -INT64_C(0x3c0ecdb2501eaa00) }, { INT64_C(0x161d595c88c20100) },
	{ INT64_C(0x3e38f57c508e1000) }, { INT64_C(0x0efa8b1f8084cd00) }, { -INT64_C(0x0efa8b1f8084cb00) }, { INT64_C(0x3e38f57c508e1200) },
	{ INT64_C(0x21680b0f1f0bd800) }, { INT64_C(0x3696e813bcf24a00) }, { -INT64_C(0x3696e813bcf24a00) }, { INT64_C(0x21680b0f1f0bda00) },
	{ INT64_C(0x33c105db6848e400) }, { INT64_C(0x25a667a69d376e00) }, { -INT64_C(0x25a667a69d376e00) }, { INT64_C(0x33c105db6848e600) },
	{ INT64_C(0x09f917abeda44b00) }, { INT64_C(0x3f37daf9f7bc5200) }, { -INT64_C(0x3f37daf9f7bc5200) }, { INT64_C(0x09f917abeda44c00) },
	{ INT64_C(0x3f641b8d03aa9a00) }, { INT64_C(0x08cec4a05f127380) }, { -INT64_C(0x08cec4a05f127400) }, { INT64_C(0x3f641b8d03aa9800) },
	{ INT64_C(0x2698a4a5829bc400) }, { INT64_C(0x330d5de28aeb2600) }, { -INT64_C(0x330d5de28aeb2400) }, { INT64_C(0x2698a4a5829bc600) },
	{ INT64_C(0x3731f43fb22abe00) }, { INT64_C(0x20655cabdb7b2a00) }, { -INT64_C(0x20655cabdb7b2800) }, { INT64_C(0x3731f43fb22abe00) },
	{ INT64_C(0x101f1806b9fdd100) }, { INT64_C(0x3defadca39478000) }, { -INT64_C(0x3defadca39477e00) }, { INT64_C(0x101f1806b9fdd600) },
	{ INT64_C(0x3c7467d8eb9d0a00) }, { INT64_C(0x150163dc19704700) }, { -INT64_C(0x150163dc19704800) }, { INT64_C(0x3c7467d8eb9d0a00) },
	{ INT64_C(0x1be51517ffc0d900) }, { INT64_C(0x3999dc4185bd4000) }, { -INT64_C(0x3999dc4185bd3e00) }, { INT64_C(0x1be51517ffc0da00) },
	{ INT64_C(0x2fd07f0f606d0c00) }, { INT64_C(0x2a8a9fea2b3bf600) }, { -INT64_C(0x2a8a9fea2b3bf800) }, { INT64_C(0x2fd07f0f606d0c00) },
	{ INT64_C(0x03ba80df3011d8a0) }, { INT64_C(0x3fe42c29c263da00) }, { -INT64_C(0x3fe42c29c263da00) }, { INT64_C(0x03ba80df3011d9c0) },
	{ INT64_C(0x3fbf3245ee660e00) }, { INT64_C(0x05afc6cf9e6c4a40) }, { -INT64_C(0x05afc6cf9e6c4980) }, { INT64_C(0x3fbf3245ee660e00) },
	{ INT64_C(0x290e0660c123fe00) }, { INT64_C(0x3118cdce90374200) }, { -INT64_C(0x3118cdce90374000) }, { INT64_C(0x290e0660c123fe00) },
	{ INT64_C(0x38b7deb3fdf0b400) }, { INT64_C(0x1da60c5cfa10d800) }, { -INT64_C(0x1da60c5cfa10d800) }, { INT64_C(0x38b7deb3fdf0b400) },
	{ INT64_C(0x13241fb638baaf00) }, { INT64_C(0x3d1212b75ac04a00) }, { -INT64_C(0x3d1212b75ac04800) }, { INT64_C(0x13241fb638bab000) },
	{ INT64_C(0x3d699ea2b7102200) }, { INT64_C(0x12038583d727bd00) }, { -INT64_C(0x12038583d727bd00) }, { INT64_C(0x3d699ea2b7102200) },
	{ INT64_C(0x1eb00695f2562000) }, { INT64_C(0x3829b3b88cbcae00) }, { -INT64_C(0x3829b3b88cbcac00) }, { INT64_C(0x1eb00695f2562500) },
	{ INT64_C(0x31d8213690d89200) }, { INT64_C(0x2824e4cc7a211800) }, { -INT64_C(0x2824e4cc7a211400) }, { INT64_C(0x31d8213690d89400) },
	{ INT64_C(0x06dbe9bb0e3d9380) }, { INT64_C(0x3fa1a2b1b0c10e00) }, { -INT64_C(0x3fa1a2b1b0c10e00) }, { INT64_C(0x06dbe9bb0e3d98c0) },
	{ INT64_C(0x3ee1ec8696fd6e00) }, { INT64_C(0x0be853dde9658d80) }, { -INT64_C(0x0be853dde9658b00) }, { INT64_C(0x3ee1ec8696fd6e00) },
	{ INT64_C(0x240b7542eac1ac00) }, { INT64_C(0x34e271bd3ac1e800) }, { -INT64_C(0x34e271bd3ac1ea00) }, { INT64_C(0x240b7542eac1ac00) },
	{ INT64_C(0x3589ff7a7df59000) }, { INT64_C(0x2310b23e748dca00) }, { -INT64_C(0x2310b23e748dca00) }, { INT64_C(0x3589ff7a7df59000) },
	{ INT64_C(0x0d101f0d8c18ef80) }, { INT64_C(0x3ea7163f5e5a0e00) }, { -INT64_C(0x3ea7163f5e5a0e00) }, { INT64_C(0x0d101f0d8c18ec80) },
	{ INT64_C(0x3b59e859cd157200) }, { INT64_C(0x17f24dd37341e300) }, { -INT64_C(0x17f24dd37341e200) }, { INT64_C(0x3b59e859cd157200) },
	{ INT64_C(0x1908ef81ef7bd300) }, { INT64_C(0x3ae67ea1181bfc00) }, { -INT64_C(0x3ae67ea1181bfc00) }, { INT64_C(0x1908ef81ef7bd400) },
	{ INT64_C(0x2dab5fde955f9a00) }, { INT64_C(0x2cd61e7ea5670c00) }, { -INT64_C(0x2cd61e7ea5670a00) }, { INT64_C(0x2dab5fde955f9c00) },
	{ INT64_C(0x0096cb587284baa0) }, { INT64_C(0x3fff4e592f189e00) }, { -INT64_C(0x3fff4e592f189e00) }, { INT64_C(0x0096cb587284bbb8) },
	{ INT64_C(0x3fff4e592f189e00) }, { INT64_C(0x0096cb587284b818) }, { -INT64_C(0x0096cb587284b868) }, { INT64_C(0x3fff4e592f189e00) },
	{ INT64_C(0x2cd61e7ea5670e00) }, { INT64_C(0x2dab5fde955f9800) }, { -INT64_C(0x2dab5fde955f9a00) }, { INT64_C(0x2cd61e7ea5670c00) },
	{ INT64_C(0x3ae67ea1181bfe00) }, { INT64_C(0x1908ef81ef7bd100) }, { -INT64_C(0x1908ef81ef7bd100) }, { INT64_C(0x3ae67ea1181bfe00) },
	{ INT64_C(0x17f24dd37341e400) }, { INT64_C(0x3b59e859cd157200) }, { -INT64_C(0x3b59e859cd157200) }, { INT64_C(0x17f24dd37341e500) },
	{ INT64_C(0x3ea7163f5e5a0e00) }, { INT64_C(0x0d101f0d8c18ed00) }, { -INT64_C(0x0d101f0d8c18ed80) }, { INT64_C(0x3ea7163f5e5a0e00) },
	{ INT64_C(0x2310b23e748dca00) }, { INT64_C(0x3589ff7a7df58e00) }, { -INT64_C(0x3589ff7a7df58e00) }, { INT64_C(0x2310b23e748dce00) },
	{ INT64_C(0x34e271bd3ac1ea00) }, { INT64_C(0x240b7542eac1ac00) }, { -INT64_C(0x240b7542eac1aa00) }, { INT64_C(0x34e271bd3ac1ec00) },
	{ INT64_C(0x0be853dde9658d80) }, { INT64_C(0x3ee1ec8696fd6e00) }, { -INT64_C(0x3ee1ec8696fd6e00) }, { INT64_C(0x0be853dde9659280) },
	{ INT64_C(0x3fa1a2b1b0c10e00) }, { INT64_C(0x06dbe9bb0e3d9300) }, { -INT64_C(0x06dbe9bb0e3d9180) }, { INT64_C(0x3fa1a2b1b0c10e00) },
	{ INT64_C(0x2824e4cc7a211a00) }, { INT64_C(0x31d8213690d89000) }, { -INT64_C(0x31d8213690d89200) }, { INT64_C(0x2824e4cc7a211800) },
	{ INT64_C(0x3829b3b88cbcb000) }, { INT64_C(0x1eb00695f2562000) }, { -INT64_C(0x1eb00695f2562200) }, { INT64_C(0x3829b3b88cbcae00) },
	{ INT64_C(0x12038583d727bf00) }, { INT64_C(0x3d699ea2b7102200) }, { -INT64_C(0x3d699ea2b7102200) }, { INT64_C(0x12038583d727bc00) },
	{ INT64_C(0x3d1212b75ac04a00) }, { INT64_C(0x13241fb638baaf00) }, { -INT64_C(0x13241fb638baad00) }, { INT64_C(0x3d1212b75ac04a00) },
	{ INT64_C(0x1da60c5cfa10da00) }, { INT64_C(0x38b7deb3fdf0b400) }, { -INT64_C(0x38b7deb3fdf0b200) }, { INT64_C(0x1da60c5cfa10db00) },
	{ INT64_C(0x3118cdce90374200) }, { INT64_C(0x290e0660c123fe00) }, { -INT64_C(0x290e0660c123fc00) }, { INT64_C(0x3118cdce90374400) },
	{ INT64_C(0x05afc6cf9e6c4bc0) }, { INT64_C(0x3fbf3245ee660e00) }, { -INT64_C(0x3fbf3245ee660e00) }, { INT64_C(0x05afc6cf9e6c4d00) },
	{ INT64_C(0x3fe42c29c263da00) }, { INT64_C(0x03ba80df3011d900) }, { -INT64_C(0x03ba80df3011d660) }, { INT64_C(0x3fe42c29c263da00) },
	{ INT64_C(0x2a8a9fea2b3bf800) }, { INT64_C(0x2fd07f0f606d0c00) }, { -INT64_C(0x2fd07f0f606d0a00) }, { INT64_C(0x2a8a9fea2b3bfa00) },
	{ INT64_C(0x3999dc4185bd4000) }, { INT64_C(0x1be51517ffc0d900) }, { -INT64_C(0x1be51517ffc0d700) }, { INT64_C(0x3999dc4185bd4000) },
	{ INT64_C(0x150163dc19704a00) }, { INT64_C(0x3c7467d8eb9d0a00) }, { -INT64_C(0x3c7467d8eb9d0800) }, { INT64_C(0x150163dc19704b00) },
	{ INT64_C(0x3defadca39478000) }, { INT64_C(0x101f1806b9fdd200) }, { -INT64_C(0x101f1806b9fdcf00) }, { INT64_C(0x3defadca39478000) },
	{ INT64_C(0x20655cabdb7b2e00) }, { INT64_C(0x3731f43fb22abc00) }, { -INT64_C(0x3731f43fb22abc00) }, { INT64_C(0x20655cabdb7b2a00) },
	{ INT64_C(0x330d5de28aeb2800) }, { INT64_C(0x2698a4a5829bc200) }, { -INT64_C(0x2698a4a5829bc400) }, { INT64_C(0x330d5de28aeb2600) },
	{ INT64_C(0x08cec4a05f127600) }, { INT64_C(0x3f641b8d03aa9800) }, { -INT64_C(0x3f641b8d03aa9a00) }, { INT64_C(0x08cec4a05f127380) },
	{ INT64_C(0x3f37daf9f7bc5200) }, { INT64_C(0x09f917abeda44980) }, { -INT64_C(0x09f917abeda44900) }, { INT64_C(0x3f37daf9f7bc5200) },
	{ INT64_C(0x25a667a69d377000) }, { INT64_C(0x33c105db6848e400) }, { -INT64_C(0x33c105db6848e400) }, { INT64_C(0x25a667a69d377000) },
	{ INT64_C(0x3696e813bcf24a00) }, { INT64_C(0x21680b0f1f0bd800) }, { -INT64_C(0x21680b0f1f0bd600) }, { INT64_C(0x3696e813bcf24c00) },
	{ INT64_C(0x0efa8b1f8084cd80) }, { INT64_C(0x3e38f57c508e1000) }, { -INT64_C(0x3e38f57c508e1000) }, { INT64_C(0x0efa8b1f8084ce80) },
	{ INT64_C(0x3c0ecdb2501ea800) }, { INT64_C(0x161d595c88c20300) }, { -INT64_C(0x161d595c88c20200) }, { INT64_C(0x3c0ecdb2501eaa00) },
	{ INT64_C(0x1ad473125cdc0900) }, { INT64_C(0x3a1ace5eb3a57000) }, { -INT64_C(0x3a1ace5eb3a56e00) }, { INT64_C(0x1ad473125cdc0e00) },
	{ INT64_C(0x2f05f6372166b600) }, { INT64_C(0x2b6a164c9ee89000) }, { -INT64_C(0x2b6a164c9ee88c00) }, { INT64_C(0x2f05f6372166b800) },
	{ INT64_C(0x028d472dff0c2fc0) }, { INT64_C(0x3ff2f88411802800) }, { -INT64_C(0x3ff2f88411802800) }, { INT64_C(0x028d472dff0c34e0) },
	{ INT64_C(0x3ff6abc84bfb5c00) }, { INT64_C(0x0228d0bb68582fc0) }, { -INT64_C(0x0228d0bb68582ea0) }, { INT64_C(0x3ff6abc84bfb5c00) },
	{ INT64_C(0x2bb3bdce7c68b800) }, { INT64_C(0x2ec18a58608ec800) }, { -INT64_C(0x2ec18a58608ec600) }, { INT64_C(0x2bb3bdce7c68ba00) },
	{ INT64_C(0x3a44ab8dcb49c200) }, { INT64_C(0x1a790cd3dbf31a00) }, { -INT64_C(0x1a790cd3dbf31900) }, { INT64_C(0x3a44ab8dcb49c200) },
	{ INT64_C(0x167b949cad63cb00) }, { INT64_C(0x3bebc6d5374b3800) }, { -INT64_C(0x3bebc6d5374b3800) }, { INT64_C(0x167b949cad63cc00) },
	{ INT64_C(0x3e502ff88c139c00) }, { INT64_C(0x0e98bba6965ef700) }, { -INT64_C(0x0e98bba6965ef600) }, { INT64_C(0x3e502ff88c139c00) },
	{ INT64_C(0x21bda17097896c00) }, { INT64_C(0x36622b4be6f7e600) }, { -INT64_C(0x36622b4be6f7e800) }, { INT64_C(0x21bda17097896a00) },
	{ INT64_C(0x33fbe9e26293bc00) }, { INT64_C(0x2554edd0f5d6b000) }, { -INT64_C(0x2554edd0f5d6b200) }, { INT64_C(0x33fbe9e26293bc00) },
	{ INT64_C(0x0a5c58bfbcfd4500) }, { INT64_C(0x3f27e29f0b580e00) }, { -INT64_C(0x3f27e29f0b581000) }, { INT64_C(0x0a5c58bfbcfd4280) },
	{ INT64_C(0x3f71a31acd5b6e00) }, { INT64_C(0x086b26de5933c300) }, { -INT64_C(0x086b26de5933c380) }, { INT64_C(0x3f71a31acd5b6e00) },
	{ INT64_C(0x26e8a63702ff1c00) }, { INT64_C(0x32d07e857affc200) }, { -INT64_C(0x32d07e857affc200) }, { INT64_C(0x26e8a63702ff1a00) },
	{ INT64_C(0x376493416d881a00) }, { INT64_C(0x200e81905705c000) }, { -INT64_C(0x200e81905705c200) }, { INT64_C(0x376493416d881a00) },
	{ INT64_C(0x10804e05eb661d00) }, { INT64_C(0x3dd60e98a14a8a00) }, { -INT64_C(0x3dd60e98a14a8800) }, { INT64_C(0x10804e05eb661e00) },
	{ INT64_C(0x3c951bff039cbc00) }, { INT64_C(0x14a253d11b82f300) }, { -INT64_C(0x14a253d11b82f300) }, { INT64_C(0x3c951bff039cbc00) },
	{ INT64_C(0x1c3f6d4726312800) }, { INT64_C(0x396dc41401b53200) }, { -INT64_C(0x396dc41401b53000) }, { INT64_C(0x1c3f6d4726312d00) },
	{ INT64_C(0x301316eadca5f600) }, { INT64_C(0x2a3f5039b3354c00) }, { -INT64_C(0x2a3f5039b3354a00) }, { INT64_C(0x301316eadca5f800) },
	{ INT64_C(0x041ed853918c1800) }, { INT64_C(0x3fde020504c32200) }, { -INT64_C(0x3fde020504c32200) }, { INT64_C(0x041ed853918c1d40) },
	{ INT64_C(0x3fc7d257d3b10c00) }, { INT64_C(0x054b9dd293534280) }, { -INT64_C(0x054b9dd293534240) }, { INT64_C(0x3fc7d257d3b10c00) },
	{ INT64_C(0x295af2a2ce45e200) }, { INT64_C(0x30d8143b35432a00) }, { -INT64_C(0x30d8143b35432600) }, { INT64_C(0x295af2a2ce45e600) },
	{ INT64_C(0x38e62b133d55ae00) }, { INT64_C(0x1d4cd02ba8609c00) }, { -INT64_C(0x1d4cd02ba8609900) }, { INT64_C(0x38e62b133d55b000) },
	{ INT64_C(0x1383f5e353b6ab00) }, { INT64_C(0x3cf3b6534a2cb400) }, { -INT64_C(0x3cf3b6534a2cb200) }, { INT64_C(0x1383f5e353b6af00) },
	{ INT64_C(0x3d859e9635ed6400) }, { INT64_C(0x11a2f7fbe8f24300) }, { -INT64_C(0x11a2f7fbe8f24300) }, { INT64_C(0x3d859e9635ed6400) },
	{ INT64_C(0x1f081906bff7fe00) }, { INT64_C(0x37f93a4b43628e00) }, { -INT64_C(0x37f93a4b43628e00) }, { INT64_C(0x1f081906bff7fe00) },
	{ INT64_C(0x3216f286aebdfc00) }, { INT64_C(0x27d667d57c0d0000) }, { -INT64_C(0x27d667d57c0d0000) }, { INT64_C(0x3216f286aebdfc00) },
	{ INT64_C(0x073fd4cecc1f7080) }, { INT64_C(0x3f968e072286a800) }, { -INT64_C(0x3f968e072286a800) }, { INT64_C(0x073fd4cecc1f7180) },
	{ INT64_C(0x3ef4533834645400) }, { INT64_C(0x0b857ec684627f80) }, { -INT64_C(0x0b857ec684627d80) }, { INT64_C(0x3ef4533834645400) },
	{ INT64_C(0x245e5acc5827c200) }, { INT64_C(0x34a9922121ea4600) }, { -INT64_C(0x34a9922121ea4400) }, { INT64_C(0x245e5acc5827c600) },
	{ INT64_C(0x35c0d1e68bd9de00) }, { INT64_C(0x22bc6dc9b7c57800) }, { -INT64_C(0x22bc6dc9b7c57600) }, { INT64_C(0x35c0d1e68bd9e000) },
	{ INT64_C(0x0d7278eaf9dcd780) }, { INT64_C(0x3e92440d79576800) }, { -INT64_C(0x3e92440d79576800) }, { INT64_C(0x0d7278eaf9dcd900) },
	{ INT64_C(0x3b7f3c872aeb3600) }, { INT64_C(0x1794f5e613dfae00) }, { -INT64_C(0x1794f5e613dfac00) }, { INT64_C(0x3b7f3c872aeb3600) },
	{ INT64_C(0x196555b7ab949100) }, { INT64_C(0x3abee2e51114fc00) }, { -INT64_C(0x3abee2e51114fe00) }, { INT64_C(0x196555b7ab948e00) },
	{ INT64_C(0x2df1953392b6ea00) }, { INT64_C(0x2c8e2a86fea6b600) }, { -INT64_C(0x2c8e2a86fea6b600) }, { INT64_C(0x2df1953392b6ea00) },
	{ INT64_C(0x00fb514b55ccc078) }, { INT64_C(0x3ffe12878a77a200) }, { -INT64_C(0x3ffe12878a77a200) }, { INT64_C(0x00fb514b55ccbd98) },
	{ INT64_C(0x3ffc38d0e196ee00) }, { INT64_C(0x015fd4d21fab2260) }, { -INT64_C(0x015fd4d21fab21f0) }, { INT64_C(0x3ffc38d0e196ee00) },
	{ INT64_C(0x2c45c89fd845fe00) }, { INT64_C(0x2e37592c1c837e00) }, { -INT64_C(0x2e37592c1c837c00) }, { INT64_C(0x2c45c89fd8460000) },
	{ INT64_C(0x3a96b63630ea4800) }, { INT64_C(0x19c17d440df9f200) }, { -INT64_C(0x19c17d440df9f200) }, { INT64_C(0x3a96b63630ea4800) },
	{ INT64_C(0x173763c926109200) }, { INT64_C(0x3ba3fde71522b400) }, { -INT64_C(0x3ba3fde71522b200) }, { INT64_C(0x173763c926109300) },
	{ INT64_C(0x3e7cd7783778ca00) }, { INT64_C(0x0dd4b19a78aed680) }, { -INT64_C(0x0dd4b19a78aed600) }, { INT64_C(0x3e7cd7783778ca00) },
	{ INT64_C(0x2267d39fdc4a9e00) }, { INT64_C(0x35f71fb13eaf6c00) }, { -INT64_C(0x35f71fb13eaf6a00) }, { INT64_C(0x2267d39fdc4aa200) },
	{ INT64_C(0x34703094b2778a00) }, { INT64_C(0x24b0e69976e22000) }, { -INT64_C(0x24b0e69976e21c00) }, { INT64_C(0x34703094b2778e00) },
	{ INT64_C(0x0b228d418ec18700) }, { INT64_C(0x3f061e94818c1800) }, { -INT64_C(0x3f061e94818c1800) }, { INT64_C(0x0b228d418ec18c00) },
	{ INT64_C(0x3f8adc76fb35ec00) }, { INT64_C(0x07a3adff792a8f80) }, { -INT64_C(0x07a3adff792a8d40) }, { INT64_C(0x3f8adc76fb35ec00) },
	{ INT64_C(0x27878893038a3000) }, { INT64_C(0x3255483f8b502a00) }, { -INT64_C(0x3255483f8b502a00) }, { INT64_C(0x27878893038a2e00) },
	{ INT64_C(0x37c836c222a98000) }, { INT64_C(0x1f5fdee656cda300) }, { -INT64_C(0x1f5fdee656cda400) }, { INT64_C(0x37c836c222a98000) },
	{ INT64_C(0x11423eefc6937a00) }, { INT64_C(0x3da106bd33201200) }, { -INT64_C(0x3da106bd33201200) }, { INT64_C(0x11423eefc6937800) },
	{ INT64_C(0x3cd4c38abaa74e00) }, { INT64_C(0x13e39be96ec27100) }, { -INT64_C(0x13e39be96ec26f00) }, { INT64_C(0x3cd4c38abaa75000) },
	{ INT64_C(0x1cf34baee1cd2300) }, { INT64_C(0x3913eb0e05382600) }, { -INT64_C(0x3913eb0e05382400) }, { INT64_C(0x1cf34baee1cd2400) },
	{ INT64_C(0x3096e2235f07f400) }, { INT64_C(0x29a778dab13efe00) }, { -INT64_C(0x29a778dab13efc00) }, { INT64_C(0x3096e2235f07f600) },
	{ INT64_C(0x04e767c4b168a880) }, { INT64_C(0x3fcfd50a905ac000) }, { -INT64_C(0x3fcfd50a905ac000) }, { INT64_C(0x04e767c4b168a9c0) },
	{ INT64_C(0x3fd73a4a60821e00) }, { INT64_C(0x0483259d3b511300) }, { -INT64_C(0x0483259d3b5113c0) }, { INT64_C(0x3fd73a4a60821e00) },
	{ INT64_C(0x29f3984b99490e00) }, { INT64_C(0x30553827ea8bfe00) }, { -INT64_C(0x30553827ea8bfc00) }, { INT64_C(0x29f3984b99491000) },
	{ INT64_C(0x39411e3373889200) }, { INT64_C(0x1c997fc386538800) }, { -INT64_C(0x1c997fc386538500) }, { INT64_C(0x39411e3373889400) },
	{ INT64_C(0x144310dc8936f000) }, { INT64_C(0x3cb53aaa08cfa600) }, { -INT64_C(0x3cb53aaa08cfa600) }, { INT64_C(0x144310dc8936f400) },
	{ INT64_C(0x3dbbd6d40f0ca200) }, { INT64_C(0x10e15b4e1749cd00) }, { -INT64_C(0x10e15b4e1749ce00) }, { INT64_C(0x3dbbd6d40f0ca200) },
	{ INT64_C(0x1fb7575c24d2dd00) }, { INT64_C(0x3796a9961a464e00) }, { -INT64_C(0x3796a9961a464e00) }, { INT64_C(0x1fb7575c24d2de00) },
	{ INT64_C(0x329321c75894d800) }, { INT64_C(0x273847c7ac605200) }, { -INT64_C(0x273847c7ac605200) }, { INT64_C(0x329321c75894d800) },
	{ INT64_C(0x08077456b7dc2d00) }, { INT64_C(0x3f7e8e1e151b1000) }, { -INT64_C(0x3f7e8e1e151b1000) }, { INT64_C(0x08077456b7dc2e00) },
	{ INT64_C(0x3f174e6f9696f000) }, { INT64_C(0x0abf80432a65ef00) }, { -INT64_C(0x0abf80432a65ed80) }, { INT64_C(0x3f174e6f9696f000) },
	{ INT64_C(0x250317de9a797400) }, { INT64_C(0x34364da5814ebc00) }, { -INT64_C(0x34364da5814eba00) }, { INT64_C(0x250317de9a797600) },
	{ INT64_C(0x362ce8549945ea00) }, { INT64_C(0x2212e491a152ac00) }, { -INT64_C(0x2212e491a152ac00) }, { INT64_C(0x362ce8549945ea00) },
	{ INT64_C(0x0e36c829aeba6f80) }, { INT64_C(0x3e66d0b4755de000) }, { -INT64_C(0x3e66d0b4755de000) }, { INT64_C(0x0e36c829aeba7080) },
	{ INT64_C(0x3bc82c1edb1b0400) }, { INT64_C(0x16d998638a0cb500) }, { -INT64_C(0x16d998638a0cb400) }, { INT64_C(0x3bc82c1edb1b0400) },
	{ INT64_C(0x1a1d6543b50ac100) }, { INT64_C(0x3a6df8f797f7b600) }, { -INT64_C(0x3a6df8f797f7b800) }, { INT64_C(0x1a1d6543b50abe00) },
	{ INT64_C(0x2e7cab1c0f328400) }, { INT64_C(0x2bfcf97bcad41e00) }, { -INT64_C(0x2bfcf97bcad42000) }, { INT64_C(0x2e7cab1c0f328200) },
	{ INT64_C(0x01c454f4ce53b330) }, { INT64_C(0x3ff9c139c54cf200) }, { -INT64_C(0x3ff9c139c54cf200) }, { INT64_C(0x01c454f4ce53b050) },
	{ INT64_C(0x3feea7763722c800) }, { INT64_C(0x02f1b754b0e8d0a0) }, { -INT64_C(0x02f1b754b0e8cec0) }, { INT64_C(0x3feea7763722c800) },
	{ INT64_C(0x2b2003abee47c000) }, { INT64_C(0x2f49ee0f7f2fa400) }, { -INT64_C(0x2f49ee0f7f2fa200) }, { INT64_C(0x2b2003abee47c000) },
	{ INT64_C(0x39f061d19c8cf600) }, { INT64_C(0x1b2f971db3197200) }, { -INT64_C(0x1b2f971db3197000) }, { INT64_C(0x39f061d19c8cf600) },
	{ INT64_C(0x15bee78b9db3b800) }, { INT64_C(0x3c31405fb8cdb200) }, { -INT64_C(0x3c31405fb8cdb200) }, { INT64_C(0x15bee78b9db3b900) },
	{ INT64_C(0x3e212179131ebe00) }, { INT64_C(0x0f5c35a316f1a400) }, { -INT64_C(0x0f5c35a316f1a200) }, { INT64_C(0x3e212179131ebe00) },
	{ INT64_C(0x2112224065611a00) }, { INT64_C(0x36cb1e29fb788e00) }, { -INT64_C(0x36cb1e29fb788e00) }, { INT64_C(0x2112224065611600) },
	{ INT64_C(0x3385a221e0eb8600) }, { INT64_C(0x25f7849688202a00) }, { -INT64_C(0x25f7849688202c00) }, { INT64_C(0x3385a221e0eb8400) },
	{ INT64_C(0x0995bdfca28b5580) }, { INT64_C(0x3f473758f42f2200) }, { -INT64_C(0x3f473758f42f2200) }, { INT64_C(0x0995bdfca28b5280) },
	{ INT64_C(0x3f55f79619fbb800) }, { INT64_C(0x09324ca6fe9a0500) }, { -INT64_C(0x09324ca6fe9a0480) }, { INT64_C(0x3f55f79619fbb800) },
	{ INT64_C(0x264843d8934c4000) }, { INT64_C(0x3349bf48560d6200) }, { -INT64_C(0x3349bf48560d6400) }, { INT64_C(0x264843d8934c4000) },
	{ INT64_C(0x36fecd0dcf25d400) }, { INT64_C(0x20bbe7d863716c00) }, { -INT64_C(0x20bbe7d863716e00) }, { INT64_C(0x36fecd0dcf25d200) },
	{ INT64_C(0x0fbdba405e9c0080) }, { INT64_C(0x3e08b4299ee71800) }, { -INT64_C(0x3e08b4299ee71600) }, { INT64_C(0x0fbdba405e9c0180) },
	{ INT64_C(0x3c531e887232f400) }, { INT64_C(0x15604012f467b400) }, { -INT64_C(0x15604012f467b400) }, { INT64_C(0x3c531e887232f400) },
	{ INT64_C(0x1b8a7814fd569300) }, { INT64_C(0x39c5664f3340c000) }, { -INT64_C(0x39c5664f3340be00) }, { INT64_C(0x1b8a7814fd569800) },
	{ INT64_C(0x2f8d7139c5a66600) }, { INT64_C(0x2ad586a32ec93c00) }, { -INT64_C(0x2ad586a32ec93a00) }, { INT64_C(0x2f8d7139c5a66800) },
	{ INT64_C(0x03562037abf062c0) }, { INT64_C(0x3fe9b8a9637da600) }, { -INT64_C(0x3fe9b8a9637da400) }, { INT64_C(0x03562037abf067c0) },
	{ INT64_C(0x3fb5f4ea28a71800) }, { INT64_C(0x0613e1c4b04c2800) }, { -INT64_C(0x0613e1c4b04c2740) }, { INT64_C(0x3fb5f4ea28a71800) },
	{ INT64_C(0x28c0b4d256654400) }, { INT64_C(0x31590e3dbc3b0e00) }, { -INT64_C(0x31590e3dbc3b0c00) }, { INT64_C(0x28c0b4d256654800) },
	{ INT64_C(0x3889066283800800) }, { INT64_C(0x1dfeff66a941dd00) }, { -INT64_C(0x1dfeff66a941d900) }, { INT64_C(0x3889066283800a00) },
	{ INT64_C(0x12c41a4e95452100) }, { INT64_C(0x3d2fd86c02d66000) }, { -INT64_C(0x3d2fd86c02d65e00) }, { INT64_C(0x12c41a4e95452600) },
	{ INT64_C(0x3d4d0727ccb00000) }, { INT64_C(0x1263e6995554ba00) }, { -INT64_C(0x1263e6995554b900) }, { INT64_C(0x3d4d0727ccb00000) },
	{ INT64_C(0x1e57a86d3cd82500) }, { INT64_C(0x3859a29263c7e200) }, { -INT64_C(0x3859a29263c7e200) }, { INT64_C(0x1e57a86d3cd82600) },
	{ INT64_C(0x3198d4ea308caa00) }, { INT64_C(0x2872feb654870000) }, { -INT64_C(0x2872feb65486fe00) }, { INT64_C(0x3198d4ea308caa00) },
	{ INT64_C(0x0677edbac92a1800) }, { INT64_C(0x3fac1a5b4eb93c00) }, { -INT64_C(0x3fac1a5b4eb93c00) }, { INT64_C(0x0677edbac92a1900) },
	{ INT64_C(0x3eceeaad1079dc00) }, { INT64_C(0x0c4b0b93e20c0200) }, { -INT64_C(0x0c4b0b93e20bff00) }, { INT64_C(0x3eceeaad1079dc00) },
	{ INT64_C(0x23b836c9b890e400) }, { INT64_C(0x351acedca8b5a400) }, { -INT64_C(0x351acedca8b5a200) }, { INT64_C(0x23b836c9b890e800) },
	{ INT64_C(0x3552a8f459731c00) }, { INT64_C(0x2364a02e26e77c00) }, { -INT64_C(0x2364a02e26e77a00) }, { INT64_C(0x3552a8f459731e00) },
	{ INT64_C(0x0cada4f4db157f80) }, { INT64_C(0x3ebb4dda86d0b800) }, { -INT64_C(0x3ebb4dda86d0b800) }, { INT64_C(0x0cada4f4db158100) },
	{ INT64_C(0x3b3401bb167a8c00) }, { INT64_C(0x184f6aaaf3903f00) }, { -INT64_C(0x184f6aaaf3903c00) }, { INT64_C(0x3b3401bb167a8e00) },
	{ INT64_C(0x18ac4b86d5ed4700) }, { INT64_C(0x3b0d89088b489e00) }, { -INT64_C(0x3b0d89088b48a000) }, { INT64_C(0x18ac4b86d5ed4400) },
	{ INT64_C(0x2d64b9da5fc53800) }, { INT64_C(0x2d1da3d54338e200) }, { -INT64_C(0x2d1da3d54338e200) }, { INT64_C(0x2d64b9da5fc53600) },
	{ INT64_C(0x003243f17d994c5e) }, { INT64_C(0x3fffec42c43a0400) }, { -INT64_C(0x3fffec42c43a0400) }, { INT64_C(0x003243f17d994978) },
	{ INT64_C(0x3ffffb10b0ddcc00) }, { INT64_C(0x001921faaee6472d) }, { -INT64_C(0x001921faaee644fb) }, { INT64_C(0x3ffffb10b0ddcc00) },
	{ INT64_C(0x2d2f73cd0d283800) }, { INT64_C(0x2d52fed25905d200) }, { -INT64_C(0x2d52fed25905d400) }, { INT64_C(0x2d2f73cd0d283800) },
	{ INT64_C(0x3b1734e1df396800) }, { INT64_C(0x189518fbff098e00) }, { -INT64_C(0x189518fbff098c00) }, { INT64_C(0x3b1734e1df396a00) },
	{ INT64_C(0x1866a88a792ea200) }, { INT64_C(0x3b2a713ca0853600) }, { -INT64_C(0x3b2a713ca0853600) }, { INT64_C(0x1866a88a792e9f00) },
	{ INT64_C(0x3ec04393e2918400) }, { INT64_C(0x0c950181e40ca780) }, { -INT64_C(0x0c950181e40ca580) }, { INT64_C(0x3ec04393e2918400) },
	{ INT64_C(0x23798e0d0088d000) }, { INT64_C(0x3544bebeb5dcc000) }, { -INT64_C(0x3544bebeb5dcbe00) }, { INT64_C(0x23798e0d0088d200) },
	{ INT64_C(0x3528d1b0b6415c00) }, { INT64_C(0x23a3595e02598400) }, { -INT64_C(0x23a3595e02598200) }, { INT64_C(0x3528d1b0b6415c00) },
	{ INT64_C(0x0c63b4cd9a654180) }, { INT64_C(0x3eca11fde8f7a800) }, { -INT64_C(0x3eca11fde8f7a800) }, { INT64_C(0x0c63b4cd9a654280) },
	{ INT64_C(0x3fae9fba81577e00) }, { INT64_C(0x065eec32aae95bc0) }, { -INT64_C(0x065eec32aae95bc0) }, { INT64_C(0x3fae9fba81577e00) },
	{ INT64_C(0x288675a022f63a00) }, { INT64_C(0x3188eeb1f4e39400) }, { -INT64_C(0x3188eeb1f4e39200) }, { INT64_C(0x288675a022f63e00) },
	{ INT64_C(0x38658893ec106200) }, { INT64_C(0x1e418527dc4ffa00) }, { -INT64_C(0x1e418527dc4ff600) }, { INT64_C(0x38658893ec106400) },
	{ INT64_C(0x127bf7d0f2c34600) }, { INT64_C(0x3d45c9a425800000) }, { -INT64_C(0x3d45c9a425800000) }, { INT64_C(0x127bf7d0f2c34b00) },
	{ INT64_C(0x3d373245208df000) }, { INT64_C(0x12ac11af48357200) }, { -INT64_C(0x12ac11af48357200) }, { INT64_C(0x3d373245208df000) },
	{ INT64_C(0x1e1530a12779f400) }, { INT64_C(0x387d3a7dcfa4c400) }, { -INT64_C(0x387d3a7dcfa4c400) }, { INT64_C(0x1e1530a12779f500) },
	{ INT64_C(0x31690b594548d000) }, { INT64_C(0x28ad50b122e08c00) }, { -INT64_C(0x28ad50b122e08c00) }, { INT64_C(0x31690b594548d000) },
	{ INT64_C(0x062ce633c30e3840) }, { INT64_C(0x3fb38d024f8f1800) }, { -INT64_C(0x3fb38d024f8f1800) }, { INT64_C(0x062ce633c30e3940) },
	{ INT64_C(0x3feb0325dc06b000) }, { INT64_C(0x033d06bad32790e0) }, { -INT64_C(0x033d06bad3278fa0) }, { INT64_C(0x3feb0325dc06b000) },
	{ INT64_C(0x2ae82fd5176eb800) }, { INT64_C(0x2f7c9b68a744f600) }, { -INT64_C(0x2f7c9b68a744f600) }, { INT64_C(0x2ae82fd5176eba00) },
	{ INT64_C(0x39d0329106899c00) }, { INT64_C(0x1b73c62d52062400) }, { -INT64_C(0x1b73c62d52062300) }, { INT64_C(0x39d0329106899c00) },
	{ INT64_C(0x1577eeec151e4800) }, { INT64_C(0x3c4ab4ef4c777000) }, { -INT64_C(0x3c4ab4ef4c777000) }, { INT64_C(0x1577eeec151e4900) },
	{ INT64_C(0x3e0eddd95bc1f400) }, { INT64_C(0x0fa55cb3ce4fae00) }, { -INT64_C(0x0fa55cb3ce4fad00) }, { INT64_C(0x3e0eddd95bc1f400) },
	{ INT64_C(0x20d17e0d23805200) }, { INT64_C(0x36f1ee0893469600) }, { -INT64_C(0x36f1ee0893469800) }, { INT64_C(0x20d17e0d23805000) },
	{ INT64_C(0x3358c3e1a9c48e00) }, { INT64_C(0x26341cdb46bc8a00) }, { -INT64_C(0x26341cdb46bc8c00) }, { INT64_C(0x3358c3e1a9c48c00) },
	{ INT64_C(0x094b2b2695caa000) }, { INT64_C(0x3f52562c00caf800) }, { -INT64_C(0x3f52562c00caf800) }, { INT64_C(0x094b2b2695ca9d00) },
	{ INT64_C(0x3f4af60cdc4ea800) }, { INT64_C(0x097ce3d53d395780) }, { -INT64_C(0x097ce3d53d395880) }, { INT64_C(0x3f4af60cdc4ea800) },
	{ INT64_C(0x260bbd3724352a00) }, { INT64_C(0x3376b550be413000) }, { -INT64_C(0x3376b550be413000) }, { INT64_C(0x260bbd3724352800) },
	{ INT64_C(0x36d81694ab582e00) }, { INT64_C(0x20fc9b4477820400) }, { -INT64_C(0x20fc9b4477820600) }, { INT64_C(0x36d81694ab582e00) },
	{ INT64_C(0x0f749a6168036300) }, { INT64_C(0x3e1b148206f38a00) }, { -INT64_C(0x3e1b148206f38a00) }, { INT64_C(0x0f749a6168036400) },
	{ INT64_C(0x3c39c5d9a181b000) }, { INT64_C(0x15a742ac0ff78d00) }, { -INT64_C(0x15a742ac0ff78e00) }, { INT64_C(0x3c39c5d9a181b000) },
	{ INT64_C(0x1b4655ae2bf75700) }, { INT64_C(0x39e5b053e3681c00) }, { -INT64_C(0x39e5b053e3681a00) }, { INT64_C(0x1b4655ae2bf75b00) },
	{ INT64_C(0x2f5ad9d0e9b76c00) }, { INT64_C(0x2b0d6e5c5659fa00) }, { -INT64_C(0x2b0d6e5c5659f800) }, { INT64_C(0x2f5ad9d0e9b76e00) },
	{ INT64_C(0x030ad24576a27760) }, { INT64_C(0x3fed7a8c76889a00) }, { -INT64_C(0x3fed7a8c76889a00) }, { INT64_C(0x030ad24576a27c60) },
	{ INT64_C(0x3ffa6dec48a01800) }, { INT64_C(0x01ab354b1504fca0) }, { -INT64_C(0x01ab354b1504fce0) }, { INT64_C(0x3ffa6dec48a01800) },
	{ INT64_C(0x2c0f3778b55bf600) }, { INT64_C(0x2e6b615a40136200) }, { -INT64_C(0x2e6b615a40136400) }, { INT64_C(0x2c0f3778b55bf400) },
	{ INT64_C(0x3a7835cf3e569c00) }, { INT64_C(0x1a067145664d5700) }, { -INT64_C(0x1a067145664d5700) }, { INT64_C(0x3a7835cf3e569c00) },
	{ INT64_C(0x16f1108f1bc9c500) }, { INT64_C(0x3bbf2e619506f600) }, { -INT64_C(0x3bbf2e619506f600) }, { INT64_C(0x16f1108f1bc9c600) },
	{ INT64_C(0x3e6c60d6cf8aac00) }, { INT64_C(0x0e1e45c625ceeb80) }, { -INT64_C(0x0e1e45c625ceec00) }, { INT64_C(0x3e6c60d6cf8aac00) },
	{ INT64_C(0x2228283f26aa8a00) }, { INT64_C(0x361f82aeba67b600) }, { -INT64_C(0x361f82aeba67b400) }, { INT64_C(0x2228283f26aa8e00) },
	{ INT64_C(0x3444d27ac5997200) }, { INT64_C(0x24ee94152c2c2400) }, { -INT64_C(0x24ee94152c2c2200) }, { INT64_C(0x3444d27ac5997400) },
	{ INT64_C(0x0ad84608c971c000) }, { INT64_C(0x3f13110f46d90800) }, { -INT64_C(0x3f13110f46d90800) }, { INT64_C(0x0ad84608c971c500) },
	{ INT64_C(0x3f81b0657e087400) }, { INT64_C(0x07ee8492c1eb6140) }, { -INT64_C(0x07ee8492c1eb5fc0) }, { INT64_C(0x3f81b0657e087400) },
	{ INT64_C(0x274c2114a974b000) }, { INT64_C(0x3283b7125c74ca00) }, { -INT64_C(0x3283b7125c74cc00) }, { INT64_C(0x274c2114a974ae00) },
	{ INT64_C(0x37a319c1b833ac00) }, { INT64_C(0x1fa1808c6cf7e000) }, { -INT64_C(0x1fa1808c6cf7e300) }, { INT64_C(0x37a319c1b833aa00) },
	{ INT64_C(0x10f998277733f900) }, { INT64_C(0x3db531137fd0d200) }, { -INT64_C(0x3db531137fd0d400) }, { INT64_C(0x10f998277733f600) },
	{ INT64_C(0x3cbd2af03d7e3800) }, { INT64_C(0x142b38466e292800) }, { -INT64_C(0x142b38466e292600) }, { INT64_C(0x3cbd2af03d7e3a00) },
	{ INT64_C(0x1caff964a0421f00) }, { INT64_C(0x3935dea437a90000) }, { -INT64_C(0x3935dea437a8fe00) }, { INT64_C(0x1caff964a0422000) },
	{ INT64_C(0x3065addb4747c000) }, { INT64_C(0x29e09a1c50b35400) }, { -INT64_C(0x29e09a1c50b35400) }, { INT64_C(0x3065addb4747c000) },
	{ INT64_C(0x049c373bf7f11840) }, { INT64_C(0x3fd56fbe38c00a00) }, { -INT64_C(0x3fd56fbe38c00a00) }, { INT64_C(0x049c373bf7f11940) },
	{ INT64_C(0x3fd1bd1e07aeb800) }, { INT64_C(0x04ce5853907fe940) }, { -INT64_C(0x04ce5853907fe6c0) }, { INT64_C(0x3fd1bd1e07aeb800) },
	{ INT64_C(0x29ba8a60ed60c800) }, { INT64_C(0x308682db8999c400) }, { -INT64_C(0x308682db8999c200) }, { INT64_C(0x29ba8a60ed60cc00) },
	{ INT64_C(0x391f450fc2660a00) }, { INT64_C(0x1cdcdf5dc440ce00) }, { -INT64_C(0x1cdcdf5dc440cb00) }, { INT64_C(0x391f450fc2660c00) },
	{ INT64_C(0x13fb7dc932cfa700) }, { INT64_C(0x3cccef61cda63c00) }, { -INT64_C(0x3cccef61cda63c00) }, { INT64_C(0x13fb7dc932cfa800) },
	{ INT64_C(0x3da7c9070938a200) }, { INT64_C(0x112a09fc0b1b1200) }, { -INT64_C(0x112a09fc0b1b0f00) }, { INT64_C(0x3da7c9070938a200) },
	{ INT64_C(0x1f75c44e26a85500) }, { INT64_C(0x37bbe059a5730400) }, { -INT64_C(0x37bbe059a5730600) }, { INT64_C(0x1f75c44e26a85200) },
	{ INT64_C(0x3264ca4c13639e00) }, { INT64_C(0x2773c17d633c3000) }, { -INT64_C(0x2773c17d633c3000) }, { INT64_C(0x3264ca4c13639c00) },
	{ INT64_C(0x07bca16349d58940) }, { INT64_C(0x3f87d7926a8a9800) }, { -INT64_C(0x3f87d7926a8a9800) }, { INT64_C(0x07bca16349d58640) },
	{ INT64_C(0x3f0a792112b2bc00) }, { INT64_C(0x0b09cc8bcd374a00) }, { -INT64_C(0x0b09cc8bcd374980) }, { INT64_C(0x3f0a792112b2bc00) },
	{ INT64_C(0x24c57b6f6f2b4400) }, { INT64_C(0x3461c3f4997f0200) }, { -INT64_C(0x3461c3f4997f0200) }, { INT64_C(0x24c57b6f6f2b4600) },
	{ INT64_C(0x36049e5afa495200) }, { INT64_C(0x22529fc98a6a1400) }, { -INT64_C(0x22529fc98a6a1200) }, { INT64_C(0x36049e5afa495200) },
	{ INT64_C(0x0ded3a7ac2b19880) }, { INT64_C(0x3e77643989fc8e00) }, { -INT64_C(0x3e77643989fc8e00) }, { INT64_C(0x0ded3a7ac2b19980) },
	{ INT64_C(0x3bad17444cf44a00) }, { INT64_C(0x171ff6458782ec00) }, { -INT64_C(0x171ff6458782eb00) }, { INT64_C(0x3bad17444cf44a00) },
	{ INT64_C(0x19d87d4207b0ac00) }, { INT64_C(0x3a8c94701ce48200) }, { -INT64_C(0x3a8c94701ce48000) }, { INT64_C(0x19d87d4207b0b100) },
	{ INT64_C(0x2e48b85f9a925400) }, { INT64_C(0x2c339f0d8aae0400) }, { -INT64_C(0x2c339f0d8aae0000) }, { INT64_C(0x2e48b85f9a925600) },
	{ INT64_C(0x0178f535ddc9f0f0) }, { INT64_C(0x3ffba9b7ef1ea400) }, { -INT64_C(0x3ffba9b7ef1ea400) }, { INT64_C(0x0178f535ddc9f610) },
	{ INT64_C(0x3ffe7049911ede00) }, { INT64_C(0x00e22fff0f2456c8) }, { -INT64_C(0x00e22fff0f2453d0) }, { INT64_C(0x3ffe7049911ede00) },
	{ INT64_C(0x2ca031da50510000) }, { INT64_C(0x2de012783ea9bc00) }, { -INT64_C(0x2de012783ea9bc00) }, { INT64_C(0x2ca031da5050fe00) },
	{ INT64_C(0x3ac8d76eae9bca00) }, { INT64_C(0x194e420137bce300) }, { -INT64_C(0x194e420137bce000) }, { INT64_C(0x3ac8d76eae9bca00) },
	{ INT64_C(0x17ac515ee2b17500) }, { INT64_C(0x3b75f53b836f1800) }, { -INT64_C(0x3b75f53b836f1800) }, { INT64_C(0x17ac515ee2b17200) },
	{ INT64_C(0x3e9787154b8d2800) }, { INT64_C(0x0d59e586737f2200) }, { -INT64_C(0x0d59e586737f1f00) }, { INT64_C(0x3e9787154b8d2800) },
	{ INT64_C(0x22d186f804aeca00) }, { INT64_C(0x35b329b565d2f000) }, { -INT64_C(0x35b329b565d2f000) }, { INT64_C(0x22d186f804aecc00) },
	{ INT64_C(0x34b7d63c3106ea00) }, { INT64_C(0x2449a9cbaa905c00) }, { -INT64_C(0x2449a9cbaa905800) }, { INT64_C(0x34b7d63c3106ec00) },
	{ INT64_C(0x0b9e36c02aff0800) }, { INT64_C(0x3eefc81a0d152000) }, { -INT64_C(0x3eefc81a0d152000) }, { INT64_C(0x0b9e36c02aff0980) },
	{ INT64_C(0x3f9961e864754600) }, { INT64_C(0x0726dbad85971080) }, { -INT64_C(0x0726dbad85970fc0) }, { INT64_C(0x3f9961e864754600) },
	{ INT64_C(0x27ea1051e3d1a600) }, { INT64_C(0x320749c2cca26600) }, { -INT64_C(0x320749c2cca26200) }, { INT64_C(0x27ea1051e3d1aa00) },
	{ INT64_C(0x3805659de3cc8200) }, { INT64_C(0x1ef21b8fafd3b500) }, { -INT64_C(0x1ef21b8fafd3b000) }, { INT64_C(0x3805659de3cc8400) },
	{ INT64_C(0x11bb1f7b99948100) }, { INT64_C(0x3d7eacd1d5e5de00) }, { -INT64_C(0x3d7eacd1d5e5dc00) }, { INT64_C(0x11bb1f7b99948600) },
	{ INT64_C(0x3cfb5b88adb0a000) }, { INT64_C(0x136c04d27a4edf00) }, { -INT64_C(0x136c04d27a4ede00) }, { INT64_C(0x3cfb5b88adb0a000) },
	{ INT64_C(0x1d632607ac9aaa00) }, { INT64_C(0x38daa520683d6200) }, { -INT64_C(0x38daa520683d6200) }, { INT64_C(0x1d632607ac9aab00) },
	{ INT64_C(0x30e84df2b9ab7800) }, { INT64_C(0x2947c11bd93dac00) }, { -INT64_C(0x2947c11bd93dac00) }, { INT64_C(0x30e84df2b9ab7a00) },
	{ INT64_C(0x0564a9551226f300) }, { INT64_C(0x3fc5b91377fe1000) }, { -INT64_C(0x3fc5b91377fe1000) }, { INT64_C(0x0564a9551226f400) },
	{ INT64_C(0x3fdf9b54e08ac400) }, { INT64_C(0x0405c360cefd0840) }, { -INT64_C(0x0405c360cefd0680) }, { INT64_C(0x3fdf9b54e08ac400) },
	{ INT64_C(0x2a522df2df071000) }, { INT64_C(0x30027c0c71cf2800) }, { -INT64_C(0x30027c0c71cf2800) }, { INT64_C(0x2a522df2df071200) },
	{ INT64_C(0x3978d76c71a21600) }, { INT64_C(0x1c28ddbb6cf14500) }, { -INT64_C(0x1c28ddbb6cf14300) }, { INT64_C(0x3978d76c71a21600) },
	{ INT64_C(0x14ba1ca2eca31e00) }, { INT64_C(0x3c8cfcf5e6be4400) }, { -INT64_C(0x3c8cfcf5e6be4400) }, { INT64_C(0x14ba1ca2eca31f00) },
	{ INT64_C(0x3ddc84b54d613400) }, { INT64_C(0x1068044deab00200) }, { -INT64_C(0x1068044deab00000) }, { INT64_C(0x3ddc84b54d613400) },
	{ INT64_C(0x20243fc9eada5a00) }, { INT64_C(0x3757f84c5ccb0c00) }, { -INT64_C(0x3757f84c5ccb0c00) }, { INT64_C(0x20243fc9eada5600) },
	{ INT64_C(0x32dfc223bbf9c000) }, { INT64_C(0x26d4aecb05063600) }, { -INT64_C(0x26d4aecb05063800) }, { INT64_C(0x32dfc223bbf9be00) },
	{ INT64_C(0x0884104afc105a00) }, { INT64_C(0x3f6e4fe30fe38c00) }, { -INT64_C(0x3f6e4fe30fe38e00) }, { INT64_C(0x0884104afc105700) },
	{ INT64_C(0x3f2bef5343d88e00) }, { INT64_C(0x0a438ad6c266fa80) }, { -INT64_C(0x0a438ad6c266fa00) }, { INT64_C(0x3f2bef5343d88e00) },
	{ INT64_C(0x256954f0eec98800) }, { INT64_C(0x33ed3ce158eb7800) }, { -INT64_C(0x33ed3ce158eb7800) }, { INT64_C(0x256954f0eec98800) },
	{ INT64_C(0x366f67176a981200) }, { INT64_C(0x21a8439e07825600) }, { -INT64_C(0x21a8439e07825600) }, { INT64_C(0x366f67176a981200) },
	{ INT64_C(0x0eb132ee9f93f180) }, { INT64_C(0x3e4a6fc14e3f3e00) }, { -INT64_C(0x3e4a6fc14e3f3e00) }, { INT64_C(0x0eb132ee9f93f280) },
	{ INT64_C(0x3bf4966c424e3a00) }, { INT64_C(0x16640af6f03d9e00) }, { -INT64_C(0x16640af6f03d9e00) }, { INT64_C(0x3bf4966c424e3a00) },
	{ INT64_C(0x1a8fec8bf5b16600) }, { INT64_C(0x3a3a41b88182b800) }, { -INT64_C(0x3a3a41b88182b600) }, { INT64_C(0x1a8fec8bf5b16a00) },
	{ INT64_C(0x2ed2b027736b2800) }, { INT64_C(0x2ba15e02dda3a800) }, { -INT64_C(0x2ba15e02dda3a600) }, { INT64_C(0x2ed2b027736b2a00) },
	{ INT64_C(0x0241eee19d6238c0) }, { INT64_C(0x3ff5cdc2aad33000) }, { -INT64_C(0x3ff5cdc2aad33000) }, { INT64_C(0x0241eee19d623de0) },
	{ INT64_C(0x3ff3f42069106e00) }, { INT64_C(0x02742a1ec8435f80) }, { -INT64_C(0x02742a1ec8435f00) }, { INT64_C(0x3ff3f42069106e00) },
	{ INT64_C(0x2b7c8a3f17f30200) }, { INT64_C(0x2ef4e6197721fe00) }, { -INT64_C(0x2ef4e6197721fe00) }, { INT64_C(0x2b7c8a3f17f30200) },
	{ INT64_C(0x3a25531f58821800) }, { INT64_C(0x1abd9faebc398000) }, { -INT64_C(0x1abd9faebc398000) }, { INT64_C(0x3a25531f58821800) },
	{ INT64_C(0x1634ed533be58e00) }, { INT64_C(0x3c0619dc286b7000) }, { -INT64_C(0x3c0619dc286b6e00) }, { INT64_C(0x1634ed533be58f00) },
	{ INT64_C(0x3e3ed2824b3af400) }, { INT64_C(0x0ee21aaeda00ca80) }, { -INT64_C(0x0ee21aaeda00ca00) }, { INT64_C(0x3e3ed2824b3af400) },
	{ INT64_C(0x217d7869fe8c8c00) }, { INT64_C(0x3689c57d5e14c200) }, { -INT64_C(0x3689c57d5e14be00) }, { INT64_C(0x217d7869fe8c9000) },
	{ INT64_C(0x33cfcadb968b7a00) }, { INT64_C(0x259211dee69cae00) }, { -INT64_C(0x259211dee69caa00) }, { INT64_C(0x33cfcadb968b7e00) },
	{ INT64_C(0x0a11ea490720b380) }, { INT64_C(0x3f33eb8157a92c00) }, { -INT64_C(0x3f33eb8157a92c00) }, { INT64_C(0x0a11ea490720b880) },
	{ INT64_C(0x3f678c1ba5833200) }, { INT64_C(0x08b5df2fd62c3280) }, { -INT64_C(0x08b5df2fd62c3000) }, { INT64_C(0x3f678c1ba5833200) },
	{ INT64_C(0x26acadff2f335e00) }, { INT64_C(0x32fe31d49cb93a00) }, { -INT64_C(0x32fe31d49cb93c00) }, { INT64_C(0x26acadff2f335c00) },
	{ INT64_C(0x373ea8c98b7ff800) }, { INT64_C(0x204fad5b0650fc00) }, { -INT64_C(0x204fad5b0650fc00) }, { INT64_C(0x373ea8c98b7ff800) },
	{ INT64_C(0x1037694a928cae00) }, { INT64_C(0x3de9544f16406400) }, { -INT64_C(0x3de9544f16406400) }, { INT64_C(0x1037694a928cab00) },
	{ INT64_C(0x3c7ca2e197f88c00) }, { INT64_C(0x14e9a4ac15d52000) }, { -INT64_C(0x14e9a4ac15d51e00) }, { INT64_C(0x3c7ca2e197f88c00) },
	{ INT64_C(0x1bfbb1a05e0ede00) }, { INT64_C(0x398ee384e6d81000) }, { -INT64_C(0x398ee384e6d81000) }, { INT64_C(0x1bfbb1a05e0edf00) },
	{ INT64_C(0x2fe1301c2272f000) }, { INT64_C(0x2a77d5ce02557c00) }, { -INT64_C(0x2a77d5ce02557a00) }, { INT64_C(0x2fe1301c2272f200) },
	{ INT64_C(0x03d397a2bfeaa760) }, { INT64_C(0x3fe2b0677c32ca00) }, { -INT64_C(0x3fe2b0677c32ca00) }, { INT64_C(0x03d397a2bfeaa880) },
	{ INT64_C(0x3fc1690a3037c600) }, { INT64_C(0x0596bdd7743e1ec0) }, { -INT64_C(0x0596bdd7743e1f80) }, { INT64_C(0x3fc1690a3037c400) },
	{ INT64_C(0x29214af7db79be00) }, { INT64_C(0x3108aabee5f4d400) }, { -INT64_C(0x3108aabee5f4d200) }, { INT64_C(0x29214af7db79c200) },
	{ INT64_C(0x38c37eeeff989000) }, { INT64_C(0x1d8fc423c62a2500) }, { -INT64_C(0x1d8fc423c62a2200) }, { INT64_C(0x38c37eeeff989200) },
	{ INT64_C(0x133c19b83af20600) }, { INT64_C(0x3d0a89bbe1a0ea00) }, { -INT64_C(0x3d0a89bbe1a0e800) }, { INT64_C(0x133c19b83af20b00) },
	{ INT64_C(0x3d70acd7021e5400) }, { INT64_C(0x11eb6643499fbb00) }, { -INT64_C(0x11eb6643499fbc00) }, { INT64_C(0x3d70acd7021e5200) },
	{ INT64_C(0x1ec61253e3c61a00) }, { INT64_C(0x381da25666e5d400) }, { -INT64_C(0x381da25666e5d200) }, { INT64_C(0x1ec61253e3c61b00) },
	{ INT64_C(0x31e7e11851b35800) }, { INT64_C(0x28114ed069818000) }, { -INT64_C(0x28114ed069818000) }, { INT64_C(0x31e7e11851b35600) },
	{ INT64_C(0x06f4e61fcc7e82c0) }, { INT64_C(0x3f9eec3e18ef4600) }, { -INT64_C(0x3f9eec3e18ef4600) }, { INT64_C(0x06f4e61fcc7e8400) },
	{ INT64_C(0x3ee694c088c4f200) }, { INT64_C(0x0bcfa14facf08e80) }, { -INT64_C(0x0bcfa14facf08d00) }, { INT64_C(0x3ee694c088c4f400) },
	{ INT64_C(0x24203703c1b4f000) }, { INT64_C(0x34d4460c6ec47400) }, { -INT64_C(0x34d4460c6ec47400) }, { INT64_C(0x24203703c1b4f000) },
	{ INT64_C(0x3597c07d41ce7600) }, { INT64_C(0x22fba935a2c2c200) }, { -INT64_C(0x22fba935a2c2c200) }, { INT64_C(0x3597c07d41ce7600) },
	{ INT64_C(0x0d28b893f1ed5f80) }, { INT64_C(0x3ea1f02f0b8d7000) }, { -INT64_C(0x3ea1f02f0b8d7000) }, { INT64_C(0x0d28b893f1ed6080) },
	{ INT64_C(0x3b634b2364187000) }, { INT64_C(0x17dafd592ba62100) }, { -INT64_C(0x17dafd592ba62000) }, { INT64_C(0x3b634b2364187000) },
	{ INT64_C(0x19200ee2c9be9800) }, { INT64_C(0x3adca54e390a8e00) }, { -INT64_C(0x3adca54e390a8e00) }, { INT64_C(0x19200ee2c9be9600) },
	{ INT64_C(0x2dbcf7cb0b103a00) }, { INT64_C(0x2cc42bd8e9d72200) }, { -INT64_C(0x2cc42bd8e9d72400) }, { INT64_C(0x2dbcf7cb0b103800) },
	{ INT64_C(0x00afed01bd603078) }, { INT64_C(0x3fff0e326f9c5e00) }, { -INT64_C(0x3fff0e326f9c5e00) }, { INT64_C(0x00afed01bd602d90) },
	{ INT64_C(0x3fff84a16bb5de00) }, { INT64_C(0x007da997e68a8f14) }, { -INT64_C(0x007da997e68a8c84) }, { INT64_C(0x3fff84a16bb5de00) },
	{ INT64_C(0x2ce80a3a4f12da00) }, { INT64_C(0x2d99c0e72acf3c00) }, { -INT64_C(0x2d99c0e72acf3a00) }, { INT64_C(0x2ce80a3a4f12dc00) },
	{ INT64_C(0x3af04edeac2f8c00) }, { INT64_C(0x18f1cc44bea32900) }, { -INT64_C(0x18f1cc44bea32700) }, { INT64_C(0x3af04edeac2f8c00) },
	{ INT64_C(0x18099a9c5c362f00) }, { INT64_C(0x3b507c691ec27600) }, { -INT64_C(0x3b507c691ec27600) }, { INT64_C(0x18099a9c5c363000) },
	{ INT64_C(0x3eac32a643812000) }, { INT64_C(0x0cf7838371ad1a80) }, { -INT64_C(0x0cf7838371ad1780) }, { INT64_C(0x3eac32a643812200) },
	{ INT64_C(0x2325b5def4a70e00) }, { INT64_C(0x357c3636171b5800) }, { -INT64_C(0x357c3636171b5800) }, { INT64_C(0x2325b5def4a70c00) },
	{ INT64_C(0x34f095463a7eaa00) }, { INT64_C(0x23f6adf3166f7200) }, { -INT64_C(0x23f6adf3166f7200) }, { INT64_C(0x34f095463a7eaa00) },
	{ INT64_C(0x0c0104960eba2800) }, { INT64_C(0x3edd3a9a24c59400) }, { -INT64_C(0x3edd3a9a24c59400) }, { INT64_C(0x0c0104960eba2500) },
	{ INT64_C(0x3fa44f5537aa7600) }, { INT64_C(0x06c2ec4787555640) }, { -INT64_C(0x06c2ec4787555580) }, { INT64_C(0x3fa44f5537aa7600) },
	{ INT64_C(0x28387497b7544e00) }, { INT64_C(0x31c859a50a5c1200) }, { -INT64_C(0x31c859a50a5c1200) }, { INT64_C(0x28387497b7544e00) },
	{ INT64_C(0x3835bc7179c33600) }, { INT64_C(0x1e99f61c817eda00) }, { -INT64_C(0x1e99f61c817eda00) }, { INT64_C(0x3835bc7179c33600) },
	{ INT64_C(0x121ba1fd3d262300) }, { INT64_C(0x3d6286f5f3766800) }, { -INT64_C(0x3d6286f5f3766800) }, { INT64_C(0x121ba1fd3d262400) },
	{ INT64_C(0x3d199247db871000) }, { INT64_C(0x130c22c08d6a1300) }, { -INT64_C(0x130c22c08d6a1300) }, { INT64_C(0x3d199247db871000) },
	{ INT64_C(0x1dbc5003b2edf800) }, { INT64_C(0x38ac35b9d6e87c00) }, { -INT64_C(0x38ac35b9d6e87a00) }, { INT64_C(0x1dbc5003b2edfd00) },
	{ INT64_C(0x3128e94bf6150800) }, { INT64_C(0x28fabb74dfbc0200) }, { -INT64_C(0x28fabb74dfbbfe00) }, { INT64_C(0x3128e94bf6150a00) },
	{ INT64_C(0x05c8cee748db9cc0) }, { INT64_C(0x3fbcf1ad0ca7fa00) }, { -INT64_C(0x3fbcf1ad0ca7fa00) }, { INT64_C(0x05c8cee748dba1c0) },
	{ INT64_C(0x3fe59e11b4e64200) }, { INT64_C(0x03a169886df23a00) }, { -INT64_C(0x03a169886df23860) }, { INT64_C(0x3fe59e11b4e64200) },
	{ INT64_C(0x2a9d6376db971e00) }, { INT64_C(0x2fbfc6a2fb126e00) }, { -INT64_C(0x2fbfc6a2fb127000) }, { INT64_C(0x2a9d6376db971c00) },
	{ INT64_C(0x39a4cc1c2583cc00) }, { INT64_C(0x1bce744262deee00) }, { -INT64_C(0x1bce744262def100) }, { INT64_C(0x39a4cc1c2583ca00) },
	{ INT64_C(0x15191fceda3c3600) }, { INT64_C(0x3c6c237d975ed200) }, { -INT64_C(0x3c6c237d975ed200) }, { INT64_C(0x15191fceda3c3400) },
	{ INT64_C(0x3df5fdb837516e00) }, { INT64_C(0x1006c4466e54af00) }, { -INT64_C(0x1006c4466e54ae00) }, { INT64_C(0x3df5fdb837516e00) },
	{ INT64_C(0x207b06fdbfe6e000) }, { INT64_C(0x37253732d4b6fe00) }, { -INT64_C(0x37253732d4b6fe00) }, { INT64_C(0x207b06fdbfe6e200) },
	{ INT64_C(0x331c8211034bfe00) }, { INT64_C(0x268495581defac00) }, { -INT64_C(0x268495581defaa00) }, { INT64_C(0x331c8211034c0000) },
	{ INT64_C(0x08e7a8b531503d00) }, { INT64_C(0x3f60a137cdefb600) }, { -INT64_C(0x3f60a137cdefb600) }, { INT64_C(0x08e7a8b531503e00) },
	{ INT64_C(0x3f3bc0b2d6ef4600) }, { INT64_C(0x09e043851c1ffb00) }, { -INT64_C(0x09e043851c1ffb80) }, { INT64_C(0x3f3bc0b2d6ef4600) },
	{ INT64_C(0x25bab79ff6ec2c00) }, { INT64_C(0x33b238e00fab4c00) }, { -INT64_C(0x33b238e00fab4a00) }, { INT64_C(0x25bab79ff6ec2e00) },
	{ INT64_C(0x36a4023f00b92400) }, { INT64_C(0x2152988d6a7a1600) }, { -INT64_C(0x2152988d6a7a1200) }, { INT64_C(0x36a4023f00b92600) },
	{ INT64_C(0x0f12f940d15b4180) }, { INT64_C(0x3e330edde3e90c00) }, { -INT64_C(0x3e330edde3e90a00) }, { INT64_C(0x0f12f940d15b4680) },
	{ INT64_C(0x3c1778457b069200) }, { INT64_C(0x1605c1fcc88f6300) }, { -INT64_C(0x1605c1fcc88f6400) }, { INT64_C(0x3c1778457b069000) },
	{ INT64_C(0x1aeb4252ca07ab00) }, { INT64_C(0x3a1040a82d175a00) }, { -INT64_C(0x3a1040a82d175800) }, { INT64_C(0x1aeb4252ca07ac00) },
	{ INT64_C(0x2f16ff146414a400) }, { INT64_C(0x2b579ba83761b000) }, { -INT64_C(0x2b579ba83761b200) }, { INT64_C(0x2f16ff146414a200) },
	{ INT64_C(0x02a663d877741ce0) }, { INT64_C(0x3ff1f30b1e0b1600) }, { -INT64_C(0x3ff1f30b1e0b1600) }, { INT64_C(0x02a663d877741e00) },
	{ INT64_C(0x3ff77ff0bf2a2a00) }, { INT64_C(0x020fb23ff308b000) }, { -INT64_C(0x020fb23ff308afe0) }, { INT64_C(0x3ff77ff0bf2a2a00) },
	{ INT64_C(0x2bc616dcd0efe800) }, { INT64_C(0x2eb05d5373465000) }, { -INT64_C(0x2eb05d5373464e00) }, { INT64_C(0x2bc616dcd0efea00) },
	{ INT64_C(0x3a4f0c66bea5f800) }, { INT64_C(0x1a622906a70b6300) }, { -INT64_C(0x1a622906a70b6300) }, { INT64_C(0x3a4f0c66bea5f800) },
	{ INT64_C(0x16931acad55f5100) }, { INT64_C(0x3be2ee00964a6800) }, { -INT64_C(0x3be2ee00964a6600) }, { INT64_C(0x16931acad55f5500) },
	{ INT64_C(0x3e55e693c2ea0e00) }, { INT64_C(0x0e80421e4ce2fc80) }, { -INT64_C(0x0e80421e4ce2fc80) }, { INT64_C(0x3e55e693c2ea0e00) },
	{ INT64_C(0x21d2fa0f1fa47600) }, { INT64_C(0x3654e71d6a3e6600) }, { -INT64_C(0x3654e71d6a3e6600) }, { INT64_C(0x21d2fa0f1fa47600) },
	{ INT64_C(0x340a8edf2cf76a00) }, { INT64_C(0x254080ef3087f600) }, { -INT64_C(0x254080ef3087f800) }, { INT64_C(0x340a8edf2cf76a00) },
	{ INT64_C(0x0a75250fb125b080) }, { INT64_C(0x3f23cc2d88712200) }, { -INT64_C(0x3f23cc2d88712200) }, { INT64_C(0x0a75250fb125b180) },
	{ INT64_C(0x3f74ec89e0d16a00) }, { INT64_C(0x08523c255c5f8e80) }, { -INT64_C(0x08523c255c5f8c80) }, { INT64_C(0x3f74ec89e0d16c00) },
	{ INT64_C(0x26fc97a2f25fc400) }, { INT64_C(0x32c13311275b1800) }, { -INT64_C(0x32c13311275b1600) }, { INT64_C(0x26fc97a2f25fc600) },
	{ INT64_C(0x377125ababb5aa00) }, { INT64_C(0x1ff8be6537615e00) }, { -INT64_C(0x1ff8be6537615d00) }, { INT64_C(0x377125ababb5ac00) },
	{ INT64_C(0x109895327b466000) }, { INT64_C(0x3dcf8ef2c3b93a00) }, { -INT64_C(0x3dcf8ef2c3b93a00) }, { INT64_C(0x109895327b466100) },
	{ INT64_C(0x3c9d31b06d815e00) }, { INT64_C(0x148a87d0b07fd700) }, { -INT64_C(0x148a87d0b07fd500) }, { INT64_C(0x3c9d31b06d815e00) },
	{ INT64_C(0x1c55f877b2353900) }, { INT64_C(0x3962a7e05f70fa00) }, { -INT64_C(0x3962a7e05f70fc00) }, { INT64_C(0x1c55f877b2353600) },
	{ INT64_C(0x3023aa5f5f29ba00) }, { INT64_C(0x2a2c6bfcac03ae00) }, { -INT64_C(0x2a2c6bfcac03b000) }, { INT64_C(0x3023aa5f5f29b800) },
	{ INT64_C(0x0437eca3a8807bc0) }, { INT64_C(0x3fdc5edbc8ab7400) }, { -INT64_C(0x3fdc5edbc8ab7400) }, { INT64_C(0x0437eca3a88078c0) },
	{ INT64_C(0x3fc9e1c63af49400) }, { INT64_C(0x0532917f06ea2d80) }, { -INT64_C(0x0532917f06ea2a40) }, { INT64_C(0x3fc9e1c63af49400) },
	{ INT64_C(0x296e1dc91f70d200) }, { INT64_C(0x30c7d2fb67b2b400) }, { -INT64_C(0x30c7d2fb67b2b400) }, { INT64_C(0x296e1dc91f70d200) },
	{ INT64_C(0x38f1a83fc944ce00) }, { INT64_C(0x1d3675caebf96200) }, { -INT64_C(0x1d3675caebf96300) }, { INT64_C(0x38f1a83fc944ce00) },
	{ INT64_C(0x139be3f1bc8af200) }, { INT64_C(0x3cec07b79ce9d000) }, { -INT64_C(0x3cec07b79ce9d200) }, { INT64_C(0x139be3f1bc8aef00) },
	{ INT64_C(0x3d8c86ddcc061200) }, { INT64_C(0x118acdc3f4873a00) }, { -INT64_C(0x118acdc3f4873700) }, { INT64_C(0x3d8c86ddcc061200) },
	{ INT64_C(0x1f1e11b4bbc35e00) }, { INT64_C(0x37ed0656e3d8b400) }, { -INT64_C(0x37ed0656e3d8b400) }, { INT64_C(0x1f1e11b4bbc35f00) },
	{ INT64_C(0x322693911b472e00) }, { INT64_C(0x27c2b9345b724400) }, { -INT64_C(0x27c2b9345b724200) }, { INT64_C(0x322693911b473000) },
	{ INT64_C(0x0758ccd1e1633c00) }, { INT64_C(0x3f93b0578522bc00) }, { -INT64_C(0x3f93b0578522bc00) }, { INT64_C(0x0758ccd1e1633d00) },
	{ INT64_C(0x3ef8d4a104d08000) }, { INT64_C(0x0b6cc50604646d00) }, { -INT64_C(0x0b6cc50604646c00) }, { INT64_C(0x3ef8d4a104d08000) },
	{ INT64_C(0x247306313fbfac00) }, { INT64_C(0x349b45e70bd32e00) }, { -INT64_C(0x349b45e70bd32a00) }, { INT64_C(0x247306313fbfb000) },
	{ INT64_C(0x35ce71cd9a96ce00) }, { INT64_C(0x22a74f4017e8a600) }, { -INT64_C(0x22a74f4017e8a200) }, { INT64_C(0x35ce71cd9a96d000) },
	{ INT64_C(0x0d8b0a3ca0e51600) }, { INT64_C(0x3e8cf75f6f77be00) }, { -INT64_C(0x3e8cf75f6f77be00) }, { INT64_C(0x0d8b0a3ca0e51b00) },
	{ INT64_C(0x3b887aa5f968a600) }, { INT64_C(0x177d96ca4b73a600) }, { -INT64_C(0x177d96ca4b73a500) }, { INT64_C(0x3b887aa5f968a600) },
	{ INT64_C(0x197c6583890fc300) }, { INT64_C(0x3ab4e54c443ce600) }, { -INT64_C(0x3ab4e54c443ce600) }, { INT64_C(0x197c6583890fc400) },
	{ INT64_C(0x2e0310d91e2dcc00) }, { INT64_C(0x2c7c1c54b3af5400) }, { -INT64_C(0x2c7c1c54b3af5200) }, { INT64_C(0x2e0310d91e2dcc00) },
	{ INT64_C(0x01147270dad71440) }, { INT64_C(0x3ffdaae731a56e00) }, { -INT64_C(0x3ffdaae731a56e00) }, { INT64_C(0x01147270dad71560) },
	{ INT64_C(0x3ffcbe0bcaf1b600) }, { INT64_C(0x0146b4381fce81c0) }, { -INT64_C(0x0146b4381fce8260) }, { INT64_C(0x3ffcbe0bcaf1b600) },
	{ INT64_C(0x2c57eb5e561bfc00) }, { INT64_C(0x2e25f2d813a31a00) }, { -INT64_C(0x2e25f2d813a31800) }, { INT64_C(0x2c57eb5e561bfe00) },
	{ INT64_C(0x3aa0cef347a5b200) }, { INT64_C(0x19aa794d47c9ee00) }, { -INT64_C(0x19aa794d47c9ef00) }, { INT64_C(0x3aa0cef347a5b200) },
	{ INT64_C(0x174ecdb8390a3d00) }, { INT64_C(0x3b9adb5759474a00) }, { -INT64_C(0x3b9adb5759474800) }, { INT64_C(0x174ecdb8390a4200) },
	{ INT64_C(0x3e824113fb15b800) }, { INT64_C(0x0dbc269829b66d00) }, { -INT64_C(0x0dbc269829b66e00) }, { INT64_C(0x3e824113fb15b800) },
	{ INT64_C(0x227d0227e72d8200) }, { INT64_C(0x35e998b50bf6e800) }, { -INT64_C(0x35e998b50bf6e600) }, { INT64_C(0x227d0227e72d8200) },
	{ INT64_C(0x347e951e9dc5a400) }, { INT64_C(0x249c4c1afdcf9200) }, { -INT64_C(0x249c4c1afdcf9200) }, { INT64_C(0x347e951e9dc5a400) },
	{ INT64_C(0x0b3b4c3fb90b5b00) }, { INT64_C(0x3f01ba4fdb040800) }, { -INT64_C(0x3f01ba4fdb040800) }, { INT64_C(0x0b3b4c3fb90b5c80) },
	{ INT64_C(0x3f8dd78efe110e00) }, { INT64_C(0x078ab96e115fddc0) }, { -INT64_C(0x078ab96e115fdc40) }, { INT64_C(0x3f8dd78efe110e00) },
	{ INT64_C(0x279b499014c0ec00) }, { INT64_C(0x3245be6ff0c8d400) }, { -INT64_C(0x3245be6ff0c8d400) }, { INT64_C(0x279b499014c0ee00) },
	{ INT64_C(0x37d484906fbcda00) }, { INT64_C(0x1f49f4a7e9772900) }, { -INT64_C(0x1f49f4a7e9772800) }, { INT64_C(0x37d484906fbcda00) },
	{ INT64_C(0x115a713a28b9da00) }, { INT64_C(0x3d9a3af2591e2000) }, { -INT64_C(0x3d9a3af2591e2000) }, { INT64_C(0x115a713a28b9db00) },
	{ INT64_C(0x3cdc8e5223b44e00) }, { INT64_C(0x13cbb6f87a146e00) }, { -INT64_C(0x13cbb6f87a146c00) }, { INT64_C(0x3cdc8e5223b44e00) },
	{ INT64_C(0x1d09b389152ec200) }, { INT64_C(0x3908883ef0bec200) }, { -INT64_C(0x3908883ef0bec400) }, { INT64_C(0x1d09b389152ebf00) },
	{ INT64_C(0x30a739ecf9205200) }, { INT64_C(0x299460e804314600) }, { -INT64_C(0x299460e804314800) }, { INT64_C(0x30a739ecf9205000) },
	{ INT64_C(0x0500767438e94340) }, { INT64_C(0x3fcde31fe8590800) }, { -INT64_C(0x3fcde31fe8590800) }, { INT64_C(0x0500767438e94040) },
	{ INT64_C(0x3fd8fafe339ff800) }, { INT64_C(0x046a134c5b53e280) }, { -INT64_C(0x046a134c5b53e000) }, { INT64_C(0x3fd8fafe339ff800) },
	{ INT64_C(0x2a069002b3bda600) }, { INT64_C(0x3044bb0072c9ca00) }, { -INT64_C(0x3044bb0072c9ca00) }, { INT64_C(0x2a069002b3bda600) },
	{ INT64_C(0x394c54ee5fb23200) }, { INT64_C(0x1c8301b95b40c200) }, { -INT64_C(0x1c8301b95b40c300) }, { INT64_C(0x394c54ee5fb23000) },
	{ INT64_C(0x145ae652bb280200) }, { INT64_C(0x3cad41072d1d6200) }, { -INT64_C(0x3cad41072d1d6200) }, { INT64_C(0x145ae652bb27ff00) },
	{ INT64_C(0x3dc2730f77d7b800) }, { INT64_C(0x10c91bda4f158d00) }, { -INT64_C(0x10c91bda4f158b00) }, { INT64_C(0x3dc2730f77d7ba00) },
	{ INT64_C(0x1fcd2947c1ff5a00) }, { INT64_C(0x378a30d7f06c3800) }, { -INT64_C(0x378a30d7f06c3800) }, { INT64_C(0x1fcd2947c1ff5b00) },
	{ INT64_C(0x32a284afb8866a00) }, { INT64_C(0x2724686e58fe0800) }, { -INT64_C(0x2724686e58fe0600) }, { INT64_C(0x32a284afb8866c00) },
	{ INT64_C(0x082062ddb3ba4100) }, { INT64_C(0x3f7b620c04302c00) }, { -INT64_C(0x3f7b620c04302c00) }, { INT64_C(0x082062ddb3ba4280) },
	{ INT64_C(0x3f1b82152a701a00) }, { INT64_C(0x0aa6b8d53a778100) }, { -INT64_C(0x0aa6b8d53a778100) }, { INT64_C(0x3f1b82152a701a00) },
	{ INT64_C(0x251795f2db297600) }, { INT64_C(0x3427c0c2fc9d8e00) }, { -INT64_C(0x3427c0c2fc9d8a00) }, { INT64_C(0x251795f2db297a00) },
	{ INT64_C(0x363a459fb5b98c00) }, { INT64_C(0x21fd9ba2ee0a6e00) }, { -INT64_C(0x21fd9ba2ee0a6a00) }, { INT64_C(0x363a459fb5b99000) },
	{ INT64_C(0x0e4f485c12535500) }, { INT64_C(0x3e6136f296e33e00) }, { -INT64_C(0x3e6136f296e33e00) }, { INT64_C(0x0e4f485c12535a00) },
	{ INT64_C(0x3bd120a408cb9c00) }, { INT64_C(0x16c21cb1e3977100) }, { -INT64_C(0x16c21cb1e3977100) }, { INT64_C(0x3bd120a408cb9c00) },
	{ INT64_C(0x1a34553b0afee500) }, { INT64_C(0x3a63b31d3ca07400) }, { -INT64_C(0x3a63b31d3ca07400) }, { INT64_C(0x1a34553b0afee600) },
	{ INT64_C(0x2e8dedb2a2dab200) }, { INT64_C(0x2beab4b64aedf000) }, { -INT64_C(0x2beab4b64aedf000) }, { INT64_C(0x2e8dedb2a2dab200) },
	{ INT64_C(0x01dd7458c64ab3e0) }, { INT64_C(0x3ff90aa99a427400) }, { -INT64_C(0x3ff90aa99a427400) }, { INT64_C(0x01dd7458c64ab500) },
	{ INT64_C(0x3fefca840641f200) }, { INT64_C(0x02d89befafab7080) }, { -INT64_C(0x02d89befafab6fa0) }, { INT64_C(0x3fefca840641f200) },
	{ INT64_C(0x2b32925503ff7800) }, { INT64_C(0x2f38fb0331cadc00) }, { -INT64_C(0x2f38fb0331cad800) }, { INT64_C(0x2b32925503ff7c00) },
	{ INT64_C(0x39fb0a5ffed5a800) }, { INT64_C(0x1b18d45bf8aca600) }, { -INT64_C(0x1b18d45bf8aca100) }, { INT64_C(0x39fb0a5ffed5aa00) },
	{ INT64_C(0x15d68910aee68700) }, { INT64_C(0x3c28b19d835b0600) }, { -INT64_C(0x3c28b19d835b0400) }, { INT64_C(0x15d68910aee68c00) },
	{ INT64_C(0x3e2724db5a049c00) }, { INT64_C(0x0f43ce86607ed780) }, { -INT64_C(0x0f43ce86607ed700) }, { INT64_C(0x3e2724db5a049c00) },
	{ INT64_C(0x2127a422bdc03000) }, { INT64_C(0x36be1d4c234a6400) }, { -INT64_C(0x36be1d4c234a6200) }, { INT64_C(0x2127a422bdc03200) },
	{ INT64_C(0x3394870101d5b600) }, { INT64_C(0x25e3461b0cee7400) }, { -INT64_C(0x25e3461b0cee7200) }, { INT64_C(0x3394870101d5b800) },
	{ INT64_C(0x09ae96a9a206d980) }, { INT64_C(0x3f436ee2ecc3aa00) }, { -INT64_C(0x3f436ee2ecc3aa00) }, { INT64_C(0x09ae96a9a206da80) },
	{ INT64_C(0x3f598f3bcd889e00) }, { INT64_C(0x09196cbc576a0980) }, { -INT64_C(0x09196cbc576a0700) }, { INT64_C(0x3f598f3bcd889e00) },
	{ INT64_C(0x265c64ee8cfad600) }, { INT64_C(0x333ab2c63ccba800) }, { -INT64_C(0x333ab2c63ccba800) }, { INT64_C(0x265c64ee8cfad800) },
	{ INT64_C(0x370ba397ea580400) }, { INT64_C(0x20a64c975a090600) }, { -INT64_C(0x20a64c975a090200) }, { INT64_C(0x370ba397ea580600) },
	{ INT64_C(0x0fd6155f7fac0280) }, { INT64_C(0x3e0280e8e11da600) }, { -INT64_C(0x3e0280e8e11da600) }, { INT64_C(0x0fd6155f7fac0380) },
	{ INT64_C(0x3c5b7ed41223b200) }, { INT64_C(0x15488dedeff3be00) }, { -INT64_C(0x15488dedeff3bb00) }, { INT64_C(0x3c5b7ed41223b200) },
	{ INT64_C(0x1ba125bd63584000) }, { INT64_C(0x39ba9124a9fcc800) }, { -INT64_C(0x39ba9124a9fcca00) }, { INT64_C(0x1ba125bd63583e00) },
	{ INT64_C(0x2f9e3fb597e2f200) }, { INT64_C(0x2ac2d6d6409cdc00) }, { -INT64_C(0x2ac2d6d6409cde00) }, { INT64_C(0x2f9e3fb597e2f200) },
	{ INT64_C(0x036f3930cd3171e0) }, { INT64_C(0x3fe86451bc36ee00) }, { -INT64_C(0x3fe86451bc36ee00) }, { INT64_C(0x036f3930cd316f00) },
	{ INT64_C(0x3fb852fece975200) }, { INT64_C(0x05fadc65adcbb900) }, { -INT64_C(0x05fadc65adcbb900) }, { INT64_C(0x3fb852fece975200) },
	{ INT64_C(0x28d412aaaf41e400) }, { INT64_C(0x31490986063aa000) }, { -INT64_C(0x31490986063aa000) }, { INT64_C(0x28d412aaaf41e200) },
	{ INT64_C(0x3894c98f4b589800) }, { INT64_C(0x1de8c98bf86bd500) }, { -INT64_C(0x1de8c98bf86bd500) }, { INT64_C(0x3894c98f4b589800) },
	{ INT64_C(0x12dc200907fe5100) }, { INT64_C(0x3d28752355696800) }, { -INT64_C(0x3d28752355696800) }, { INT64_C(0x12dc200907fe5200) },
	{ INT64_C(0x3d543b376415e600) }, { INT64_C(0x124bd28bb3767200) }, { -INT64_C(0x124bd28bb3767200) }, { INT64_C(0x3d543b376415e600) },
	{ INT64_C(0x1e6dc704be97e200) }, { INT64_C(0x384db3e03e5d5e00) }, { -INT64_C(0x384db3e03e5d5c00) }, { INT64_C(0x1e6dc704be97e600) },
	{ INT64_C(0x31a8b37c697bf400) }, { INT64_C(0x285f818fa75b7600) }, { -INT64_C(0x285f818fa75b7400) }, { INT64_C(0x31a8b37c697bf600) },
	{ INT64_C(0x0690ee4389fcf100) }, { INT64_C(0x3fa98b2a6df5c800) }, { -INT64_C(0x3fa98b2a6df5c800) }, { INT64_C(0x0690ee4389fcf600) },
	{ INT64_C(0x3ed3b9aca5ebbc00) }, { INT64_C(0x0c326074d95b6780) }, { -INT64_C(0x0c326074d95b6600) }, { INT64_C(0x3ed3b9aca5ebbc00) },
	{ INT64_C(0x23cd0eb347c0e800) }, { INT64_C(0x350cc3d81dc26600) }, { -INT64_C(0x350cc3d81dc26800) }, { INT64_C(0x23cd0eb347c0e600) },
	{ INT64_C(0x35608af0e2affe00) }, { INT64_C(0x234facda0a2dcc00) }, { -INT64_C(0x234facda0a2dce00) }, { INT64_C(0x35608af0e2affc00) },
	{ INT64_C(0x0cc646734d3d7780) }, { INT64_C(0x3eb64e749f45b200) }, { -INT64_C(0x3eb64e749f45b200) }, { INT64_C(0x0cc646734d3d7500) },
	{ INT64_C(0x3b3d89184d63e600) }, { INT64_C(0x1838290bb359c800) }, { -INT64_C(0x1838290bb359c600) }, { INT64_C(0x3b3d89184d63e800) },
	{ INT64_C(0x18c37a439f885100) }, { INT64_C(0x3b03d413e716c400) }, { -INT64_C(0x3b03d413e716c400) }, { INT64_C(0x18c37a439f885200) },
	{ INT64_C(0x2d766de256bc0c00) }, { INT64_C(0x2d0bcce85fddde00) }, { -INT64_C(0x2d0bcce85fdddc00) }, { INT64_C(0x2d766de256bc0c00) },
	{ INT64_C(0x004b65e08be65cc4) }, { INT64_C(0x3fffd3963c5d1a00) }, { -INT64_C(0x3fffd3963c5d1a00) }, { INT64_C(0x004b65e08be65ddc) },
	{ INT64_C(0x3fffd3963c5d1a00) }, { INT64_C(0x004b65e08be65af0) }, { -INT64_C(0x004b65e08be65a90) }, { INT64_C(0x3fffd3963c5d1a00) },
	{ INT64_C(0x2d0bcce85fddde00) }, { INT64_C(0x2d766de256bc0a00) }, { -INT64_C(0x2d766de256bc0a00) }, { INT64_C(0x2d0bcce85fdde000) },
	{ INT64_C(0x3b03d413e716c600) }, { INT64_C(0x18c37a439f884f00) }, { -INT64_C(0x18c37a439f884f00) }, { INT64_C(0x3b03d413e716c600) },
	{ INT64_C(0x1838290bb359c900) }, { INT64_C(0x3b3d89184d63e600) }, { -INT64_C(0x3b3d89184d63e600) }, { INT64_C(0x1838290bb359ca00) },
	{ INT64_C(0x3eb64e749f45b200) }, { INT64_C(0x0cc646734d3d7600) }, { -INT64_C(0x0cc646734d3d7580) }, { INT64_C(0x3eb64e749f45b200) },
	{ INT64_C(0x234facda0a2dce00) }, { INT64_C(0x35608af0e2affe00) }, { -INT64_C(0x35608af0e2affc00) }, { INT64_C(0x234facda0a2dd200) },
	{ INT64_C(0x350cc3d81dc26600) }, { INT64_C(0x23cd0eb347c0e800) }, { -INT64_C(0x23cd0eb347c0e200) }, { INT64_C(0x350cc3d81dc26a00) },
	{ INT64_C(0x0c326074d95b6880) }, { INT64_C(0x3ed3b9aca5ebbc00) }, { -INT64_C(0x3ed3b9aca5ebbc00) }, { INT64_C(0x0c326074d95b6d80) },
	{ INT64_C(0x3fa98b2a6df5c800) }, { INT64_C(0x0690ee4389fcf100) }, { -INT64_C(0x0690ee4389fceec0) }, { INT64_C(0x3fa98b2a6df5c800) },
	{ INT64_C(0x285f818fa75b7600) }, { INT64_C(0x31a8b37c697bf200) }, { -INT64_C(0x31a8b37c697bf400) }, { INT64_C(0x285f818fa75b7600) },
	{ INT64_C(0x384db3e03e5d5e00) }, { INT64_C(0x1e6dc704be97e200) }, { -INT64_C(0x1e6dc704be97e400) }, { INT64_C(0x384db3e03e5d5e00) },
	{ INT64_C(0x124bd28bb3767400) }, { INT64_C(0x3d543b376415e600) }, { -INT64_C(0x3d543b376415e600) }, { INT64_C(0x124bd28bb3767100) },
	{ INT64_C(0x3d28752355696800) }, { INT64_C(0x12dc200907fe5100) }, { -INT64_C(0x12dc200907fe4f00) }, { INT64_C(0x3d28752355696800) },
	{ INT64_C(0x1de8c98bf86bd700) }, { INT64_C(0x3894c98f4b589800) }, { -INT64_C(0x3894c98f4b589600) }, { INT64_C(0x1de8c98bf86bd800) },
	{ INT64_C(0x31490986063aa000) }, { INT64_C(0x28d412aaaf41e200) }, { -INT64_C(0x28d412aaaf41e000) }, { INT64_C(0x31490986063aa200) },
	{ INT64_C(0x05fadc65adcbbb40) }, { INT64_C(0x3fb852fece975200) }, { -INT64_C(0x3fb852fece975200) }, { INT64_C(0x05fadc65adcbbc40) },
	{ INT64_C(0x3fe86451bc36ee00) }, { INT64_C(0x036f3930cd316f00) }, { -INT64_C(0x036f3930cd316fa0) }, { INT64_C(0x3fe86451bc36ee00) },
	{ INT64_C(0x2ac2d6d6409cde00) }, { INT64_C(0x2f9e3fb597e2f000) }, { -INT64_C(0x2f9e3fb597e2ee00) }, { INT64_C(0x2ac2d6d6409ce000) },
	{ INT64_C(0x39ba9124a9fcca00) }, { INT64_C(0x1ba125bd63583e00) }, { -INT64_C(0x1ba125bd63583b00) }, { INT64_C(0x39ba9124a9fccc00) },
	{ INT64_C(0x15488dedeff3be00) }, { INT64_C(0x3c5b7ed41223b200) }, { -INT64_C(0x3c5b7ed41223b000) }, { INT64_C(0x15488dedeff3c200) },
	{ INT64_C(0x3e0280e8e11da600) }, { INT64_C(0x0fd6155f7fabff80) }, { -INT64_C(0x0fd6155f7fac0080) }, { INT64_C(0x3e0280e8e11da600) },
	{ INT64_C(0x20a64c975a090400) }, { INT64_C(0x370ba397ea580400) }, { -INT64_C(0x370ba397ea580400) }, { INT64_C(0x20a64c975a090600) },
	{ INT64_C(0x333ab2c63ccbaa00) }, { INT64_C(0x265c64ee8cfad600) }, { -INT64_C(0x265c64ee8cfad600) }, { INT64_C(0x333ab2c63ccbaa00) },
	{ INT64_C(0x09196cbc576a0900) }, { INT64_C(0x3f598f3bcd889e00) }, { -INT64_C(0x3f598f3bcd889e00) }, { INT64_C(0x09196cbc576a0a80) },
	{ INT64_C(0x3f436ee2ecc3aa00) }, { INT64_C(0x09ae96a9a206d900) }, { -INT64_C(0x09ae96a9a206d780) }, { INT64_C(0x3f436ee2ecc3ac00) },
	{ INT64_C(0x25e3461b0cee7400) }, { INT64_C(0x3394870101d5b600) }, { -INT64_C(0x3394870101d5b600) }, { INT64_C(0x25e3461b0cee7400) },
	{ INT64_C(0x36be1d4c234a6400) }, { INT64_C(0x2127a422bdc03000) }, { -INT64_C(0x2127a422bdc02e00) }, { INT64_C(0x36be1d4c234a6400) },
	{ INT64_C(0x0f43ce86607ed900) }, { INT64_C(0x3e2724db5a049c00) }, { -INT64_C(0x3e2724db5a049c00) }, { INT64_C(0x0f43ce86607eda00) },
	{ INT64_C(0x3c28b19d835b0800) }, { INT64_C(0x15d68910aee68600) }, { -INT64_C(0x15d68910aee68500) }, { INT64_C(0x3c28b19d835b0800) },
	{ INT64_C(0x1b18d45bf8aca700) }, { INT64_C(0x39fb0a5ffed5a800) }, { -INT64_C(0x39fb0a5ffed5a800) }, { INT64_C(0x1b18d45bf8aca400) },
	{ INT64_C(0x2f38fb0331cadc00) }, { INT64_C(0x2b32925503ff7600) }, { -INT64_C(0x2b32925503ff7a00) }, { INT64_C(0x2f38fb0331cada00) },
	{ INT64_C(0x02d89befafab71e0) }, { INT64_C(0x3fefca840641f200) }, { -INT64_C(0x3fefca840641f200) }, { INT64_C(0x02d89befafab6f00) },
	{ INT64_C(0x3ff90aa99a427400) }, { INT64_C(0x01dd7458c64ab390) }, { -INT64_C(0x01dd7458c64ab1b0) }, { INT64_C(0x3ff90aa99a427400) },
	{ INT64_C(0x2beab4b64aedf200) }, { INT64_C(0x2e8dedb2a2dab200) }, { -INT64_C(0x2e8dedb2a2dab000) }, { INT64_C(0x2beab4b64aedf200) },
	{ INT64_C(0x3a63b31d3ca07400) }, { INT64_C(0x1a34553b0afee500) }, { -INT64_C(0x1a34553b0afee300) }, { INT64_C(0x3a63b31d3ca07600) },
	{ INT64_C(0x16c21cb1e3977300) }, { INT64_C(0x3bd120a408cb9c00) }, { -INT64_C(0x3bd120a408cb9a00) }, { INT64_C(0x16c21cb1e3977400) },
	{ INT64_C(0x3e6136f296e33e00) }, { INT64_C(0x0e4f485c12535480) }, { -INT64_C(0x0e4f485c12535280) }, { INT64_C(0x3e6136f296e33e00) },
	{ INT64_C(0x21fd9ba2ee0a6e00) }, { INT64_C(0x363a459fb5b98c00) }, { -INT64_C(0x363a459fb5b98e00) }, { INT64_C(0x21fd9ba2ee0a6c00) },
	{ INT64_C(0x3427c0c2fc9d8e00) }, { INT64_C(0x251795f2db297400) }, { -INT64_C(0x251795f2db297800) }, { INT64_C(0x3427c0c2fc9d8c00) },
	{ INT64_C(0x0aa6b8d53a778300) }, { INT64_C(0x3f1b82152a701a00) }, { -INT64_C(0x3f1b82152a701a00) }, { INT64_C(0x0aa6b8d53a778000) },
	{ INT64_C(0x3f7b620c04302c00) }, { INT64_C(0x082062ddb3ba3f00) }, { -INT64_C(0x082062ddb3ba3f00) }, { INT64_C(0x3f7b620c04302c00) },
	{ INT64_C(0x2724686e58fe0a00) }, { INT64_C(0x32a284afb8866800) }, { -INT64_C(0x32a284afb8866a00) }, { INT64_C(0x2724686e58fe0800) },
	{ INT64_C(0x378a30d7f06c3a00) }, { INT64_C(0x1fcd2947c1ff5700) }, { -INT64_C(0x1fcd2947c1ff5800) }, { INT64_C(0x378a30d7f06c3800) },
	{ INT64_C(0x10c91bda4f158d00) }, { INT64_C(0x3dc2730f77d7b800) }, { -INT64_C(0x3dc2730f77d7b800) }, { INT64_C(0x10c91bda4f158f00) },
	{ INT64_C(0x3cad41072d1d6200) }, { INT64_C(0x145ae652bb280000) }, { -INT64_C(0x145ae652bb280000) }, { INT64_C(0x3cad41072d1d6200) },
	{ INT64_C(0x1c8301b95b40c200) }, { INT64_C(0x394c54ee5fb23200) }, { -INT64_C(0x394c54ee5fb22e00) }, { INT64_C(0x1c8301b95b40c600) },
	{ INT64_C(0x3044bb0072c9ca00) }, { INT64_C(0x2a069002b3bda400) }, { -INT64_C(0x2a069002b3bda200) }, { INT64_C(0x3044bb0072c9cc00) },
	{ INT64_C(0x046a134c5b53e240) }, { INT64_C(0x3fd8fafe339ff800) }, { -INT64_C(0x3fd8fafe339ff800) }, { INT64_C(0x046a134c5b53e780) },
	{ INT64_C(0x3fcde31fe8590800) }, { INT64_C(0x0500767438e941c0) }, { -INT64_C(0x0500767438e94100) }, { INT64_C(0x3fcde31fe8590800) },
	{ INT64_C(0x299460e804314600) }, { INT64_C(0x30a739ecf9205200) }, { -INT64_C(0x30a739ecf9204e00) }, { INT64_C(0x299460e804314a00) },
	{ INT64_C(0x3908883ef0bec200) }, { INT64_C(0x1d09b389152ec100) }, { -INT64_C(0x1d09b389152ebc00) }, { INT64_C(0x3908883ef0bec600) },
	{ INT64_C(0x13cbb6f87a146f00) }, { INT64_C(0x3cdc8e5223b44e00) }, { -INT64_C(0x3cdc8e5223b44c00) }, { INT64_C(0x13cbb6f87a147300) },
	{ INT64_C(0x3d9a3af2591e2000) }, { INT64_C(0x115a713a28b9d900) }, { -INT64_C(0x115a713a28b9d800) }, { INT64_C(0x3d9a3af2591e2000) },
	{ INT64_C(0x1f49f4a7e9772a00) }, { INT64_C(0x37d484906fbcda00) }, { -INT64_C(0x37d484906fbcd800) }, { INT64_C(0x1f49f4a7e9772b00) },
	{ INT64_C(0x3245be6ff0c8d400) }, { INT64_C(0x279b499014c0ec00) }, { -INT64_C(0x279b499014c0ea00) }, { INT64_C(0x3245be6ff0c8d600) },
	{ INT64_C(0x078ab96e115fde80) }, { INT64_C(0x3f8dd78efe110e00) }, { -INT64_C(0x3f8dd78efe110e00) }, { INT64_C(0x078ab96e115fdfc0) },
	{ INT64_C(0x3f01ba4fdb040800) }, { INT64_C(0x0b3b4c3fb90b5c00) }, { -INT64_C(0x0b3b4c3fb90b5900) }, { INT64_C(0x3f01ba4fdb040800) },
	{ INT64_C(0x249c4c1afdcf9200) }, { INT64_C(0x347e951e9dc5a400) }, { -INT64_C(0x347e951e9dc5a200) }, { INT64_C(0x249c4c1afdcf9600) },
	{ INT64_C(0x35e998b50bf6e600) }, { INT64_C(0x227d0227e72d8200) }, { -INT64_C(0x227d0227e72d8000) }, { INT64_C(0x35e998b50bf6e800) },
	{ INT64_C(0x0dbc269829b67000) }, { INT64_C(0x3e824113fb15b800) }, { -INT64_C(0x3e824113fb15b600) }, { INT64_C(0x0dbc269829b67100) },
	{ INT64_C(0x3b9adb5759474a00) }, { INT64_C(0x174ecdb8390a3e00) }, { -INT64_C(0x174ecdb8390a3b00) }, { INT64_C(0x3b9adb5759474c00) },
	{ INT64_C(0x19aa794d47c9f100) }, { INT64_C(0x3aa0cef347a5b000) }, { -INT64_C(0x3aa0cef347a5b200) }, { INT64_C(0x19aa794d47c9ee00) },
	{ INT64_C(0x2e25f2d813a31a00) }, { INT64_C(0x2c57eb5e561bfa00) }, { -INT64_C(0x2c57eb5e561bfc00) }, { INT64_C(0x2e25f2d813a31a00) },
	{ INT64_C(0x0146b4381fce8490) }, { INT64_C(0x3ffcbe0bcaf1b600) }, { -INT64_C(0x3ffcbe0bcaf1b600) }, { INT64_C(0x0146b4381fce81b0) },
	{ INT64_C(0x3ffdaae731a56e00) }, { INT64_C(0x01147270dad71320) }, { -INT64_C(0x01147270dad71210) }, { INT64_C(0x3ffdaae731a56e00) },
	{ INT64_C(0x2c7c1c54b3af5400) }, { INT64_C(0x2e0310d91e2dca00) }, { -INT64_C(0x2e0310d91e2dca00) }, { INT64_C(0x2c7c1c54b3af5600) },
	{ INT64_C(0x3ab4e54c443ce600) }, { INT64_C(0x197c6583890fc200) }, { -INT64_C(0x197c6583890fc100) }, { INT64_C(0x3ab4e54c443ce800) },
	{ INT64_C(0x177d96ca4b73a700) }, { INT64_C(0x3b887aa5f968a600) }, { -INT64_C(0x3b887aa5f968a600) }, { INT64_C(0x177d96ca4b73a800) },
	{ INT64_C(0x3e8cf75f6f77be00) }, { INT64_C(0x0d8b0a3ca0e51500) }, { -INT64_C(0x0d8b0a3ca0e51400) }, { INT64_C(0x3e8cf75f6f77c000) },
	{ INT64_C(0x22a74f4017e8a800) }, { INT64_C(0x35ce71cd9a96ce00) }, { -INT64_C(0x35ce71cd9a96ce00) }, { INT64_C(0x22a74f4017e8a400) },
	{ INT64_C(0x349b45e70bd32e00) }, { INT64_C(0x247306313fbfaa00) }, { -INT64_C(0x247306313fbfac00) }, { INT64_C(0x349b45e70bd32c00) },
	{ INT64_C(0x0b6cc50604646e80) }, { INT64_C(0x3ef8d4a104d08000) }, { -INT64_C(0x3ef8d4a104d08000) }, { INT64_C(0x0b6cc50604646b80) },
	{ INT64_C(0x3f93b0578522bc00) }, { INT64_C(0x0758ccd1e1633900) }, { -INT64_C(0x0758ccd1e16339c0) }, { INT64_C(0x3f93b0578522bc00) },
	{ INT64_C(0x27c2b9345b724600) }, { INT64_C(0x322693911b472e00) }, { -INT64_C(0x322693911b472e00) }, { INT64_C(0x27c2b9345b724400) },
	{ INT64_C(0x37ed0656e3d8b600) }, { INT64_C(0x1f1e11b4bbc35c00) }, { -INT64_C(0x1f1e11b4bbc35c00) }, { INT64_C(0x37ed0656e3d8b600) },
	{ INT64_C(0x118acdc3f4873900) }, { INT64_C(0x3d8c86ddcc061200) }, { -INT64_C(0x3d8c86ddcc061200) }, { INT64_C(0x118acdc3f4873a00) },
	{ INT64_C(0x3cec07b79ce9d200) }, { INT64_C(0x139be3f1bc8aef00) }, { -INT64_C(0x139be3f1bc8af000) }, { INT64_C(0x3cec07b79ce9d200) },
	{ INT64_C(0x1d3675caebf96100) }, { INT64_C(0x38f1a83fc944d000) }, { -INT64_C(0x38f1a83fc944cc00) }, { INT64_C(0x1d3675caebf96600) },
	{ INT64_C(0x30c7d2fb67b2b400) }, { INT64_C(0x296e1dc91f70d200) }, { -INT64_C(0x296e1dc91f70d000) }, { INT64_C(0x30c7d2fb67b2b600) },
	{ INT64_C(0x0532917f06ea2c80) }, { INT64_C(0x3fc9e1c63af49400) }, { -INT64_C(0x3fc9e1c63af49400) }, { INT64_C(0x0532917f06ea3180) },
	{ INT64_C(0x3fdc5edbc8ab7400) }, { INT64_C(0x0437eca3a8807980) }, { -INT64_C(0x0437eca3a8807980) }, { INT64_C(0x3fdc5edbc8ab7400) },
	{ INT64_C(0x2a2c6bfcac03ae00) }, { INT64_C(0x3023aa5f5f29b800) }, { -INT64_C(0x3023aa5f5f29b400) }, { INT64_C(0x2a2c6bfcac03b200) },
	{ INT64_C(0x3962a7e05f70fa00) }, { INT64_C(0x1c55f877b2353700) }, { -INT64_C(0x1c55f877b2353300) }, { INT64_C(0x3962a7e05f70fc00) },
	{ INT64_C(0x148a87d0b07fd700) }, { INT64_C(0x3c9d31b06d815e00) }, { -INT64_C(0x3c9d31b06d815c00) }, { INT64_C(0x148a87d0b07fdc00) },
	{ INT64_C(0x3dcf8ef2c3b93c00) }, { INT64_C(0x109895327b465e00) }, { -INT64_C(0x109895327b465e00) }, { INT64_C(0x3dcf8ef2c3b93c00) },
	{ INT64_C(0x1ff8be6537615f00) }, { INT64_C(0x377125ababb5aa00) }, { -INT64_C(0x377125ababb5aa00) }, { INT64_C(0x1ff8be6537616000) },
	{ INT64_C(0x32c13311275b1800) }, { INT64_C(0x26fc97a2f25fc400) }, { -INT64_C(0x26fc97a2f25fc400) }, { INT64_C(0x32c13311275b1800) },
	{ INT64_C(0x08523c255c5f8e80) }, { INT64_C(0x3f74ec89e0d16a00) }, { -INT64_C(0x3f74ec89e0d16a00) }, { INT64_C(0x08523c255c5f9000) },
	{ INT64_C(0x3f23cc2d88712200) }, { INT64_C(0x0a75250fb125b080) }, { -INT64_C(0x0a75250fb125ae80) }, { INT64_C(0x3f23cc2d88712400) },
	{ INT64_C(0x254080ef3087f800) }, { INT64_C(0x340a8edf2cf76a00) }, { -INT64_C(0x340a8edf2cf76800) }, { INT64_C(0x254080ef3087fa00) },
	{ INT64_C(0x3654e71d6a3e6600) }, { INT64_C(0x21d2fa0f1fa47600) }, { -INT64_C(0x21d2fa0f1fa47400) }, { INT64_C(0x3654e71d6a3e6800) },
	{ INT64_C(0x0e80421e4ce2fe80) }, { INT64_C(0x3e55e693c2ea0e00) }, { -INT64_C(0x3e55e693c2ea0e00) }, { INT64_C(0x0e80421e4ce2ff80) },
	{ INT64_C(0x3be2ee00964a6800) }, { INT64_C(0x16931acad55f5100) }, { -INT64_C(0x16931acad55f4f00) }, { INT64_C(0x3be2ee00964a6800) },
	{ INT64_C(0x1a622906a70b6500) }, { INT64_C(0x3a4f0c66bea5f600) }, { -INT64_C(0x3a4f0c66bea5f800) }, { INT64_C(0x1a622906a70b6200) },
	{ INT64_C(0x2eb05d5373465200) }, { INT64_C(0x2bc616dcd0efe600) }, { -INT64_C(0x2bc616dcd0efe800) }, { INT64_C(0x2eb05d5373465200) },
	{ INT64_C(0x020fb23ff308b220) }, { INT64_C(0x3ff77ff0bf2a2a00) }, { -INT64_C(0x3ff77ff0bf2a2a00) }, { INT64_C(0x020fb23ff308af40) },
	{ INT64_C(0x3ff1f30b1e0b1600) }, { INT64_C(0x02a663d877741d60) }, { -INT64_C(0x02a663d877741ac0) }, { INT64_C(0x3ff1f30b1e0b1600) },
	{ INT64_C(0x2b579ba83761b200) }, { INT64_C(0x2f16ff146414a200) }, { -INT64_C(0x2f16ff146414a000) }, { INT64_C(0x2b579ba83761b400) },
	{ INT64_C(0x3a1040a82d175800) }, { INT64_C(0x1aeb4252ca07ab00) }, { -INT64_C(0x1aeb4252ca07a900) }, { INT64_C(0x3a1040a82d175a00) },
	{ INT64_C(0x1605c1fcc88f6600) }, { INT64_C(0x3c1778457b069000) }, { -INT64_C(0x3c1778457b069000) }, { INT64_C(0x1605c1fcc88f6700) },
	{ INT64_C(0x3e330edde3e90c00) }, { INT64_C(0x0f12f940d15b4200) }, { -INT64_C(0x0f12f940d15b3f80) }, { INT64_C(0x3e330edde3e90c00) },
	{ INT64_C(0x2152988d6a7a1800) }, { INT64_C(0x36a4023f00b92200) }, { -INT64_C(0x36a4023f00b92400) }, { INT64_C(0x2152988d6a7a1600) },
	{ INT64_C(0x33b238e00fab4c00) }, { INT64_C(0x25bab79ff6ec2c00) }, { -INT64_C(0x25bab79ff6ec2c00) }, { INT64_C(0x33b238e00fab4c00) },
	{ INT64_C(0x09e043851c1ffd80) }, { INT64_C(0x3f3bc0b2d6ef4600) }, { -INT64_C(0x3f3bc0b2d6ef4600) }, { INT64_C(0x09e043851c1ffa80) },
	{ INT64_C(0x3f60a137cdefb600) }, { INT64_C(0x08e7a8b531503b80) }, { -INT64_C(0x08e7a8b531503b00) }, { INT64_C(0x3f60a137cdefb600) },
	{ INT64_C(0x268495581defac00) }, { INT64_C(0x331c8211034bfe00) }, { -INT64_C(0x331c8211034bfe00) }, { INT64_C(0x268495581defae00) },
	{ INT64_C(0x37253732d4b6fe00) }, { INT64_C(0x207b06fdbfe6e000) }, { -INT64_C(0x207b06fdbfe6de00) }, { INT64_C(0x37253732d4b6fe00) },
	{ INT64_C(0x1006c4466e54b000) }, { INT64_C(0x3df5fdb837516c00) }, { -INT64_C(0x3df5fdb837516c00) }, { INT64_C(0x1006c4466e54b100) },
	{ INT64_C(0x3c6c237d975ed200) }, { INT64_C(0x15191fceda3c3500) }, { -INT64_C(0x15191fceda3c3400) }, { INT64_C(0x3c6c237d975ed200) },
	{ INT64_C(0x1bce744262deef00) }, { INT64_C(0x39a4cc1c2583ca00) }, { -INT64_C(0x39a4cc1c2583c800) }, { INT64_C(0x1bce744262def400) },
	{ INT64_C(0x2fbfc6a2fb126e00) }, { INT64_C(0x2a9d6376db971e00) }, { -INT64_C(0x2a9d6376db971a00) }, { INT64_C(0x2fbfc6a2fb127200) },
	{ INT64_C(0x03a169886df23aa0) }, { INT64_C(0x3fe59e11b4e64200) }, { -INT64_C(0x3fe59e11b4e64200) }, { INT64_C(0x03a169886df23fc0) },
	{ INT64_C(0x3fbcf1ad0ca7fa00) }, { INT64_C(0x05c8cee748db9c40) }, { -INT64_C(0x05c8cee748db9a80) }, { INT64_C(0x3fbcf1ad0ca7fa00) },
	{ INT64_C(0x28fabb74dfbc0200) }, { INT64_C(0x3128e94bf6150600) }, { -INT64_C(0x3128e94bf6150800) }, { INT64_C(0x28fabb74dfbc0000) },
	{ INT64_C(0x38ac35b9d6e87c00) }, { INT64_C(0x1dbc5003b2edf800) }, { -INT64_C(0x1dbc5003b2edfa00) }, { INT64_C(0x38ac35b9d6e87a00) },
	{ INT64_C(0x130c22c08d6a1500) }, { INT64_C(0x3d199247db871000) }, { -INT64_C(0x3d199247db871000) }, { INT64_C(0x130c22c08d6a1200) },
	{ INT64_C(0x3d6286f5f3766800) }, { INT64_C(0x121ba1fd3d262300) }, { -INT64_C(0x121ba1fd3d262100) }, { INT64_C(0x3d6286f5f3766a00) },
	{ INT64_C(0x1e99f61c817edc00) }, { INT64_C(0x3835bc7179c33400) }, { -INT64_C(0x3835bc7179c33400) }, { INT64_C(0x1e99f61c817edd00) },
	{ INT64_C(0x31c859a50a5c1400) }, { INT64_C(0x28387497b7544e00) }, { -INT64_C(0x28387497b7544c00) }, { INT64_C(0x31c859a50a5c1400) },
	{ INT64_C(0x06c2ec47875557c0) }, { INT64_C(0x3fa44f5537aa7600) }, { -INT64_C(0x3fa44f5537aa7600) }, { INT64_C(0x06c2ec47875558c0) },
	{ INT64_C(0x3edd3a9a24c59400) }, { INT64_C(0x0c0104960eba2580) }, { -INT64_C(0x0c0104960eba2580) }, { INT64_C(0x3edd3a9a24c59400) },
	{ INT64_C(0x23f6adf3166f7200) }, { INT64_C(0x34f095463a7ea800) }, { -INT64_C(0x34f095463a7ea800) }, { INT64_C(0x23f6adf3166f7400) },
	{ INT64_C(0x357c3636171b5800) }, { INT64_C(0x2325b5def4a70c00) }, { -INT64_C(0x2325b5def4a70a00) }, { INT64_C(0x357c3636171b5a00) },
	{ INT64_C(0x0cf7838371ad1a00) }, { INT64_C(0x3eac32a643812000) }, { -INT64_C(0x3eac32a643812000) }, { INT64_C(0x0cf7838371ad1f00) },
	{ INT64_C(0x3b507c691ec27600) }, { INT64_C(0x18099a9c5c362d00) }, { -INT64_C(0x18099a9c5c362d00) }, { INT64_C(0x3b507c691ec27600) },
	{ INT64_C(0x18f1cc44bea32900) }, { INT64_C(0x3af04edeac2f8c00) }, { -INT64_C(0x3af04edeac2f8a00) }, { INT64_C(0x18f1cc44bea32a00) },
	{ INT64_C(0x2d99c0e72acf3c00) }, { INT64_C(0x2ce80a3a4f12d800) }, { -INT64_C(0x2ce80a3a4f12d800) }, { INT64_C(0x2d99c0e72acf3c00) },
	{ INT64_C(0x007da997e68a8eb8) }, { INT64_C(0x3fff84a16bb5de00) }, { -INT64_C(0x3fff84a16bb5de00) }, { INT64_C(0x007da997e68a8fd0) },
	{ INT64_C(0x3fff0e326f9c5e00) }, { INT64_C(0x00afed01bd602f00) }, { -INT64_C(0x00afed01bd602e40) }, { INT64_C(0x3fff0e326f9c5e00) },
	{ INT64_C(0x2cc42bd8e9d72200) }, { INT64_C(0x2dbcf7cb0b103800) }, { -INT64_C(0x2dbcf7cb0b103600) }, { INT64_C(0x2cc42bd8e9d72600) },
	{ INT64_C(0x3adca54e390a8e00) }, { INT64_C(0x19200ee2c9be9700) }, { -INT64_C(0x19200ee2c9be9600) }, { INT64_C(0x3adca54e390a8e00) },
	{ INT64_C(0x17dafd592ba62200) }, { INT64_C(0x3b634b2364187000) }, { -INT64_C(0x3b634b2364186e00) }, { INT64_C(0x17dafd592ba62700) },
	{ INT64_C(0x3ea1f02f0b8d7200) }, { INT64_C(0x0d28b893f1ed5e00) }, { -INT64_C(0x0d28b893f1ed5d80) }, { INT64_C(0x3ea1f02f0b8d7200) },
	{ INT64_C(0x22fba935a2c2c200) }, { INT64_C(0x3597c07d41ce7600) }, { -INT64_C(0x3597c07d41ce7400) }, { INT64_C(0x22fba935a2c2c400) },
	{ INT64_C(0x34d4460c6ec47600) }, { INT64_C(0x24203703c1b4f000) }, { -INT64_C(0x24203703c1b4ee00) }, { INT64_C(0x34d4460c6ec47600) },
	{ INT64_C(0x0bcfa14facf08f00) }, { INT64_C(0x3ee694c088c4f200) }, { -INT64_C(0x3ee694c088c4f200) }, { INT64_C(0x0bcfa14facf09000) },
	{ INT64_C(0x3f9eec3e18ef4600) }, { INT64_C(0x06f4e61fcc7e8380) }, { -INT64_C(0x06f4e61fcc7e80c0) }, { INT64_C(0x3f9eec3e18ef4800) },
	{ INT64_C(0x28114ed069818000) }, { INT64_C(0x31e7e11851b35600) }, { -INT64_C(0x31e7e11851b35400) }, { INT64_C(0x28114ed069818200) },
	{ INT64_C(0x381da25666e5d200) }, { INT64_C(0x1ec61253e3c61b00) }, { -INT64_C(0x1ec61253e3c61800) }, { INT64_C(0x381da25666e5d400) },
	{ INT64_C(0x11eb6643499fbe00) }, { INT64_C(0x3d70acd7021e5200) }, { -INT64_C(0x3d70acd7021e5200) }, { INT64_C(0x11eb6643499fbf00) },
	{ INT64_C(0x3d0a89bbe1a0ea00) }, { INT64_C(0x133c19b83af20700) }, { -INT64_C(0x133c19b83af20400) }, { INT64_C(0x3d0a89bbe1a0ea00) },
	{ INT64_C(0x1d8fc423c62a2700) }, { INT64_C(0x38c37eeeff988e00) }, { -INT64_C(0x38c37eeeff989000) }, { INT64_C(0x1d8fc423c62a2500) },
	{ INT64_C(0x3108aabee5f4d600) }, { INT64_C(0x29214af7db79be00) }, { -INT64_C(0x29214af7db79be00) }, { INT64_C(0x3108aabee5f4d600) },
	{ INT64_C(0x0596bdd7743e21c0) }, { INT64_C(0x3fc1690a3037c400) }, { -INT64_C(0x3fc1690a3037c600) }, { INT64_C(0x0596bdd7743e1ec0) },
	{ INT64_C(0x3fe2b0677c32ca00) }, { INT64_C(0x03d397a2bfeaa4e0) }, { -INT64_C(0x03d397a2bfeaa520) }, { INT64_C(0x3fe2b0677c32ca00) },
	{ INT64_C(0x2a77d5ce02557e00) }, { INT64_C(0x2fe1301c2272ee00) }, { -INT64_C(0x2fe1301c2272f000) }, { INT64_C(0x2a77d5ce02557c00) },
	{ INT64_C(0x398ee384e6d81200) }, { INT64_C(0x1bfbb1a05e0edc00) }, { -INT64_C(0x1bfbb1a05e0edc00) }, { INT64_C(0x398ee384e6d81200) },
	{ INT64_C(0x14e9a4ac15d52100) }, { INT64_C(0x3c7ca2e197f88c00) }, { -INT64_C(0x3c7ca2e197f88c00) }, { INT64_C(0x14e9a4ac15d52200) },
	{ INT64_C(0x3de9544f16406400) }, { INT64_C(0x1037694a928cac00) }, { -INT64_C(0x1037694a928cac00) }, { INT64_C(0x3de9544f16406400) },
	{ INT64_C(0x204fad5b0650fa00) }, { INT64_C(0x373ea8c98b7ff800) }, { -INT64_C(0x373ea8c98b7ff600) }, { INT64_C(0x204fad5b0650fe00) },
	{ INT64_C(0x32fe31d49cb93c00) }, { INT64_C(0x26acadff2f335c00) }, { -INT64_C(0x26acadff2f335a00) }, { INT64_C(0x32fe31d49cb93e00) },
	{ INT64_C(0x08b5df2fd62c3280) }, { INT64_C(0x3f678c1ba5833200) }, { -INT64_C(0x3f678c1ba5833200) }, { INT64_C(0x08b5df2fd62c3780) },
	{ INT64_C(0x3f33eb8157a92c00) }, { INT64_C(0x0a11ea490720b300) }, { -INT64_C(0x0a11ea490720b180) }, { INT64_C(0x3f33eb8157a92e00) },
	{ INT64_C(0x259211dee69cb000) }, { INT64_C(0x33cfcadb968b7a00) }, { -INT64_C(0x33cfcadb968b7c00) }, { INT64_C(0x259211dee69cae00) },
	{ INT64_C(0x3689c57d5e14c200) }, { INT64_C(0x217d7869fe8c8c00) }, { -INT64_C(0x217d7869fe8c8e00) }, { INT64_C(0x3689c57d5e14c000) },
	{ INT64_C(0x0ee21aaeda00cc00) }, { INT64_C(0x3e3ed2824b3af400) }, { -INT64_C(0x3e3ed2824b3af400) }, { INT64_C(0x0ee21aaeda00c900) },
	{ INT64_C(0x3c0619dc286b7000) }, { INT64_C(0x1634ed533be58e00) }, { -INT64_C(0x1634ed533be58c00) }, { INT64_C(0x3c0619dc286b7000) },
	{ INT64_C(0x1abd9faebc398200) }, { INT64_C(0x3a25531f58821800) }, { -INT64_C(0x3a25531f58821800) }, { INT64_C(0x1abd9faebc398300) },
	{ INT64_C(0x2ef4e61977220000) }, { INT64_C(0x2b7c8a3f17f30000) }, { -INT64_C(0x2b7c8a3f17f30000) }, { INT64_C(0x2ef4e61977220000) },
	{ INT64_C(0x02742a1ec8436140) }, { INT64_C(0x3ff3f42069106e00) }, { -INT64_C(0x3ff3f42069106e00) }, { INT64_C(0x02742a1ec8436260) },
	{ INT64_C(0x3ff5cdc2aad33000) }, { INT64_C(0x0241eee19d6238c0) }, { -INT64_C(0x0241eee19d623680) }, { INT64_C(0x3ff5cdc2aad33000) },
	{ INT64_C(0x2ba15e02dda3aa00) }, { INT64_C(0x2ed2b027736b2600) }, { -INT64_C(0x2ed2b027736b2800) }, { INT64_C(0x2ba15e02dda3a800) },
	{ INT64_C(0x3a3a41b88182b800) }, { INT64_C(0x1a8fec8bf5b16600) }, { -INT64_C(0x1a8fec8bf5b16400) }, { INT64_C(0x3a3a41b88182ba00) },
	{ INT64_C(0x16640af6f03da000) }, { INT64_C(0x3bf4966c424e3a00) }, { -INT64_C(0x3bf4966c424e3c00) }, { INT64_C(0x16640af6f03d9d00) },
	{ INT64_C(0x3e4a6fc14e3f3e00) }, { INT64_C(0x0eb132ee9f93f180) }, { -INT64_C(0x0eb132ee9f93ef80) }, { INT64_C(0x3e4a6fc14e3f3e00) },
	{ INT64_C(0x21a8439e07825800) }, { INT64_C(0x366f67176a981000) }, { -INT64_C(0x366f67176a981000) }, { INT64_C(0x21a8439e07825a00) },
	{ INT64_C(0x33ed3ce158eb7800) }, { INT64_C(0x256954f0eec98800) }, { -INT64_C(0x256954f0eec98600) }, { INT64_C(0x33ed3ce158eb7a00) },
	{ INT64_C(0x0a438ad6c266fc80) }, { INT64_C(0x3f2bef5343d88e00) }, { -INT64_C(0x3f2bef5343d88e00) }, { INT64_C(0x0a438ad6c266fd80) },
	{ INT64_C(0x3f6e4fe30fe38e00) }, { INT64_C(0x0884104afc105800) }, { -INT64_C(0x0884104afc105780) }, { INT64_C(0x3f6e4fe30fe38e00) },
	{ INT64_C(0x26d4aecb05063600) }, { INT64_C(0x32dfc223bbf9c000) }, { -INT64_C(0x32dfc223bbf9bc00) }, { INT64_C(0x26d4aecb05063a00) },
	{ INT64_C(0x3757f84c5ccb0c00) }, { INT64_C(0x20243fc9eada5800) }, { -INT64_C(0x20243fc9eada5400) }, { INT64_C(0x3757f84c5ccb0e00) },
	{ INT64_C(0x1068044deab00200) }, { INT64_C(0x3ddc84b54d613400) }, { -INT64_C(0x3ddc84b54d613200) }, { INT64_C(0x1068044deab00700) },
	{ INT64_C(0x3c8cfcf5e6be4600) }, { INT64_C(0x14ba1ca2eca31c00) }, { -INT64_C(0x14ba1ca2eca31c00) }, { INT64_C(0x3c8cfcf5e6be4600) },
	{ INT64_C(0x1c28ddbb6cf14500) }, { INT64_C(0x3978d76c71a21600) }, { -INT64_C(0x3978d76c71a21400) }, { INT64_C(0x1c28ddbb6cf14600) },
	{ INT64_C(0x30027c0c71cf2a00) }, { INT64_C(0x2a522df2df071000) }, { -INT64_C(0x2a522df2df070e00) }, { INT64_C(0x30027c0c71cf2a00) },
	{ INT64_C(0x0405c360cefd0880) }, { INT64_C(0x3fdf9b54e08ac400) }, { -INT64_C(0x3fdf9b54e08ac400) }, { INT64_C(0x0405c360cefd09c0) },
	{ INT64_C(0x3fc5b91377fe1000) }, { INT64_C(0x0564a9551226f200) }, { -INT64_C(0x0564a9551226f0c0) }, { INT64_C(0x3fc5b91377fe1000) },
	{ INT64_C(0x2947c11bd93dae00) }, { INT64_C(0x30e84df2b9ab7800) }, { -INT64_C(0x30e84df2b9ab7800) }, { INT64_C(0x2947c11bd93dae00) },
	{ INT64_C(0x38daa520683d6400) }, { INT64_C(0x1d632607ac9aa900) }, { -INT64_C(0x1d632607ac9aa800) }, { INT64_C(0x38daa520683d6400) },
	{ INT64_C(0x136c04d27a4ee000) }, { INT64_C(0x3cfb5b88adb0a000) }, { -INT64_C(0x3cfb5b88adb09e00) }, { INT64_C(0x136c04d27a4ee100) },
	{ INT64_C(0x3d7eacd1d5e5de00) }, { INT64_C(0x11bb1f7b99948000) }, { -INT64_C(0x11bb1f7b99947f00) }, { INT64_C(0x3d7eacd1d5e5de00) },
	{ INT64_C(0x1ef21b8fafd3b500) }, { INT64_C(0x3805659de3cc8000) }, { -INT64_C(0x3805659de3cc8200) }, { INT64_C(0x1ef21b8fafd3b300) },
	{ INT64_C(0x320749c2cca26600) }, { INT64_C(0x27ea1051e3d1a400) }, { -INT64_C(0x27ea1051e3d1a600) }, { INT64_C(0x320749c2cca26400) },
	{ INT64_C(0x0726dbad859711c0) }, { INT64_C(0x3f9961e864754600) }, { -INT64_C(0x3f9961e864754600) }, { INT64_C(0x0726dbad85970f00) },
	{ INT64_C(0x3eefc81a0d152000) }, { INT64_C(0x0b9e36c02aff0500) }, { -INT64_C(0x0b9e36c02aff0600) }, { INT64_C(0x3eefc81a0d152000) },
	{ INT64_C(0x2449a9cbaa905c00) }, { INT64_C(0x34b7d63c3106e800) }, { -INT64_C(0x34b7d63c3106ea00) }, { INT64_C(0x2449a9cbaa905c00) },
	{ INT64_C(0x35b329b565d2f200) }, { INT64_C(0x22d186f804aec800) }, { -INT64_C(0x22d186f804aeca00) }, { INT64_C(0x35b329b565d2f000) },
	{ INT64_C(0x0d59e586737f2180) }, { INT64_C(0x3e9787154b8d2800) }, { -INT64_C(0x3e9787154b8d2800) }, { INT64_C(0x0d59e586737f2280) },
	{ INT64_C(0x3b75f53b836f1a00) }, { INT64_C(0x17ac515ee2b17200) }, { -INT64_C(0x17ac515ee2b17300) }, { INT64_C(0x3b75f53b836f1800) },
	{ INT64_C(0x194e420137bce200) }, { INT64_C(0x3ac8d76eae9bca00) }, { -INT64_C(0x3ac8d76eae9bc800) }, { INT64_C(0x194e420137bce600) },
	{ INT64_C(0x2de012783ea9bc00) }, { INT64_C(0x2ca031da5050fe00) }, { -INT64_C(0x2ca031da5050fc00) }, { INT64_C(0x2de012783ea9be00) },
	{ INT64_C(0x00e22fff0f245608) }, { INT64_C(0x3ffe7049911ede00) }, { -INT64_C(0x3ffe7049911edc00) }, { INT64_C(0x00e22fff0f245b20) },
	{ INT64_C(0x3ffba9b7ef1ea400) }, { INT64_C(0x0178f535ddc9f030) }, { -INT64_C(0x0178f535ddc9eec0) }, { INT64_C(0x3ffba9b7ef1ea400) },
	{ INT64_C(0x2c339f0d8aae0600) }, { INT64_C(0x2e48b85f9a925200) }, { -INT64_C(0x2e48b85f9a925400) }, { INT64_C(0x2c339f0d8aae0400) },
	{ INT64_C(0x3a8c94701ce48400) }, { INT64_C(0x19d87d4207b0ab00) }, { -INT64_C(0x19d87d4207b0aa00) }, { INT64_C(0x3a8c94701ce48400) },
	{ INT64_C(0x171ff6458782ed00) }, { INT64_C(0x3bad17444cf44a00) }, { -INT64_C(0x3bad17444cf44c00) }, { INT64_C(0x171ff6458782ea00) },
	{ INT64_C(0x3e77643989fc8e00) }, { INT64_C(0x0ded3a7ac2b19800) }, { -INT64_C(0x0ded3a7ac2b19680) }, { INT64_C(0x3e77643989fc9000) },
	{ INT64_C(0x22529fc98a6a1400) }, { INT64_C(0x36049e5afa495200) }, { -INT64_C(0x36049e5afa495200) }, { INT64_C(0x22529fc98a6a1600) },
	{ INT64_C(0x3461c3f4997f0200) }, { INT64_C(0x24c57b6f6f2b4400) }, { -INT64_C(0x24c57b6f6f2b4400) }, { INT64_C(0x3461c3f4997f0400) },
	{ INT64_C(0x0b09cc8bcd374b80) }, { INT64_C(0x3f0a792112b2bc00) }, { -INT64_C(0x3f0a792112b2bc00) }, { INT64_C(0x0b09cc8bcd374d00) },
	{ INT64_C(0x3f87d7926a8a9800) }, { INT64_C(0x07bca16349d586c0) }, { -INT64_C(0x07bca16349d58700) }, { INT64_C(0x3f87d7926a8a9800) },
	{ INT64_C(0x2773c17d633c3000) }, { INT64_C(0x3264ca4c13639c00) }, { -INT64_C(0x3264ca4c13639a00) }, { INT64_C(0x2773c17d633c3200) },
	{ INT64_C(0x37bbe059a5730600) }, { INT64_C(0x1f75c44e26a85200) }, { -INT64_C(0x1f75c44e26a85000) }, { INT64_C(0x37bbe059a5730600) },
	{ INT64_C(0x112a09fc0b1b1200) }, { INT64_C(0x3da7c9070938a200) }, { -INT64_C(0x3da7c9070938a000) }, { INT64_C(0x112a09fc0b1b1600) },
	{ INT64_C(0x3cccef61cda63c00) }, { INT64_C(0x13fb7dc932cfa400) }, { -INT64_C(0x13fb7dc932cfa500) }, { INT64_C(0x3cccef61cda63c00) },
	{ INT64_C(0x1cdcdf5dc440cd00) }, { INT64_C(0x391f450fc2660c00) }, { -INT64_C(0x391f450fc2660a00) }, { INT64_C(0x1cdcdf5dc440ce00) },
	{ INT64_C(0x308682db8999c400) }, { INT64_C(0x29ba8a60ed60c800) }, { -INT64_C(0x29ba8a60ed60c800) }, { INT64_C(0x308682db8999c400) },
	{ INT64_C(0x04ce5853907fe900) }, { INT64_C(0x3fd1bd1e07aeb800) }, { -INT64_C(0x3fd1bd1e07aeb600) }, { INT64_C(0x04ce5853907fea00) },
	{ INT64_C(0x3fd56fbe38c00a00) }, { INT64_C(0x049c373bf7f116c0) }, { -INT64_C(0x049c373bf7f11600) }, { INT64_C(0x3fd56fbe38c00a00) },
	{ INT64_C(0x29e09a1c50b35600) }, { INT64_C(0x3065addb4747c000) }, { -INT64_C(0x3065addb4747be00) }, { INT64_C(0x29e09a1c50b35600) },
	{ INT64_C(0x3935dea437a90000) }, { INT64_C(0x1caff964a0421d00) }, { -INT64_C(0x1caff964a0421d00) }, { INT64_C(0x3935dea437a90000) },
	{ INT64_C(0x142b38466e292800) }, { INT64_C(0x3cbd2af03d7e3800) }, { -INT64_C(0x3cbd2af03d7e3800) }, { INT64_C(0x142b38466e292900) },
	{ INT64_C(0x3db531137fd0d400) }, { INT64_C(0x10f998277733f700) }, { -INT64_C(0x10f998277733f700) }, { INT64_C(0x3db531137fd0d400) },
	{ INT64_C(0x1fa1808c6cf7e100) }, { INT64_C(0x37a319c1b833aa00) }, { -INT64_C(0x37a319c1b833a800) }, { INT64_C(0x1fa1808c6cf7e600) },
	{ INT64_C(0x3283b7125c74ca00) }, { INT64_C(0x274c2114a974ae00) }, { -INT64_C(0x274c2114a974aa00) }, { INT64_C(0x3283b7125c74ce00) },
	{ INT64_C(0x07ee8492c1eb6200) }, { INT64_C(0x3f81b0657e087400) }, { -INT64_C(0x3f81b0657e087200) }, { INT64_C(0x07ee8492c1eb6700) },
	{ INT64_C(0x3f13110f46d90800) }, { INT64_C(0x0ad84608c971c080) }, { -INT64_C(0x0ad84608c971be00) }, { INT64_C(0x3f13110f46d90800) },
	{ INT64_C(0x24ee94152c2c2600) }, { INT64_C(0x3444d27ac5997200) }, { -INT64_C(0x3444d27ac5997200) }, { INT64_C(0x24ee94152c2c2400) },
	{ INT64_C(0x361f82aeba67b600) }, { INT64_C(0x2228283f26aa8a00) }, { -INT64_C(0x2228283f26aa8a00) }, { INT64_C(0x361f82aeba67b600) },
	{ INT64_C(0x0e1e45c625ceee00) }, { INT64_C(0x3e6c60d6cf8aaa00) }, { -INT64_C(0x3e6c60d6cf8aac00) }, { INT64_C(0x0e1e45c625ceeb80) },
	{ INT64_C(0x3bbf2e619506f600) }, { INT64_C(0x16f1108f1bc9c500) }, { -INT64_C(0x16f1108f1bc9c300) }, { INT64_C(0x3bbf2e619506f800) },
	{ INT64_C(0x1a067145664d5900) }, { INT64_C(0x3a7835cf3e569c00) }, { -INT64_C(0x3a7835cf3e569c00) }, { INT64_C(0x1a067145664d5a00) },
	{ INT64_C(0x2e6b615a40136400) }, { INT64_C(0x2c0f3778b55bf400) }, { -INT64_C(0x2c0f3778b55bf200) }, { INT64_C(0x2e6b615a40136600) },
	{ INT64_C(0x01ab354b1504ff20) }, { INT64_C(0x3ffa6dec48a01800) }, { -INT64_C(0x3ffa6dec48a01800) }, { INT64_C(0x01ab354b15050030) },
	{ INT64_C(0x3fed7a8c76889a00) }, { INT64_C(0x030ad24576a27820) }, { -INT64_C(0x030ad24576a27520) }, { INT64_C(0x3fed7a8c76889a00) },
	{ INT64_C(0x2b0d6e5c5659fc00) }, { INT64_C(0x2f5ad9d0e9b76a00) }, { -INT64_C(0x2f5ad9d0e9b76c00) }, { INT64_C(0x2b0d6e5c5659fa00) },
	{ INT64_C(0x39e5b053e3681c00) }, { INT64_C(0x1b4655ae2bf75700) }, { -INT64_C(0x1b4655ae2bf75800) }, { INT64_C(0x39e5b053e3681a00) },
	{ INT64_C(0x15a742ac0ff79000) }, { INT64_C(0x3c39c5d9a181b000) }, { -INT64_C(0x3c39c5d9a181b000) }, { INT64_C(0x15a742ac0ff78e00) },
	{ INT64_C(0x3e1b148206f38a00) }, { INT64_C(0x0f749a6168036380) }, { -INT64_C(0x0f749a6168036080) }, { INT64_C(0x3e1b148206f38a00) },
	{ INT64_C(0x20fc9b4477820800) }, { INT64_C(0x36d81694ab582c00) }, { -INT64_C(0x36d81694ab582c00) }, { INT64_C(0x20fc9b4477820800) },
	{ INT64_C(0x3376b550be413200) }, { INT64_C(0x260bbd3724352800) }, { -INT64_C(0x260bbd3724352600) }, { INT64_C(0x3376b550be413200) },
	{ INT64_C(0x097ce3d53d395a80) }, { INT64_C(0x3f4af60cdc4ea800) }, { -INT64_C(0x3f4af60cdc4ea800) }, { INT64_C(0x097ce3d53d395b80) },
	{ INT64_C(0x3f52562c00caf800) }, { INT64_C(0x094b2b2695ca9f00) }, { -INT64_C(0x094b2b2695ca9e00) }, { INT64_C(0x3f52562c00caf800) },
	{ INT64_C(0x26341cdb46bc8a00) }, { INT64_C(0x3358c3e1a9c48e00) }, { -INT64_C(0x3358c3e1a9c48a00) }, { INT64_C(0x26341cdb46bc8e00) },
	{ INT64_C(0x36f1ee0893469600) }, { INT64_C(0x20d17e0d23805000) }, { -INT64_C(0x20d17e0d23804c00) }, { INT64_C(0x36f1ee0893469a00) },
	{ INT64_C(0x0fa55cb3ce4faf00) }, { INT64_C(0x3e0eddd95bc1f400) }, { -INT64_C(0x3e0eddd95bc1f200) }, { INT64_C(0x0fa55cb3ce4fb400) },
	{ INT64_C(0x3c4ab4ef4c777000) }, { INT64_C(0x1577eeec151e4700) }, { -INT64_C(0x1577eeec151e4600) }, { INT64_C(0x3c4ab4ef4c777200) },
	{ INT64_C(0x1b73c62d52062500) }, { INT64_C(0x39d0329106899c00) }, { -INT64_C(0x39d0329106899c00) }, { INT64_C(0x1b73c62d52062600) },
	{ INT64_C(0x2f7c9b68a744f800) }, { INT64_C(0x2ae82fd5176eb800) }, { -INT64_C(0x2ae82fd5176eb800) }, { INT64_C(0x2f7c9b68a744f800) },
	{ INT64_C(0x033d06bad32791e0) }, { INT64_C(0x3feb0325dc06b000) }, { -INT64_C(0x3feb0325dc06b000) }, { INT64_C(0x033d06bad32792e0) },
	{ INT64_C(0x3fb38d024f8f1800) }, { INT64_C(0x062ce633c30e3800) }, { -INT64_C(0x062ce633c30e3600) }, { INT64_C(0x3fb38d024f8f1800) },
	{ INT64_C(0x28ad50b122e08e00) }, { INT64_C(0x31690b594548ce00) }, { -INT64_C(0x31690b594548ce00) }, { INT64_C(0x28ad50b122e08e00) },
	{ INT64_C(0x387d3a7dcfa4c400) }, { INT64_C(0x1e1530a12779f400) }, { -INT64_C(0x1e1530a12779f200) }, { INT64_C(0x387d3a7dcfa4c600) },
	{ INT64_C(0x12ac11af48357400) }, { INT64_C(0x3d373245208dee00) }, { -INT64_C(0x3d373245208dee00) }, { INT64_C(0x12ac11af48357500) },
	{ INT64_C(0x3d45c9a425800000) }, { INT64_C(0x127bf7d0f2c34600) }, { -INT64_C(0x127bf7d0f2c34400) }, { INT64_C(0x3d45c9a425800200) },
	{ INT64_C(0x1e418527dc4ffc00) }, { INT64_C(0x38658893ec106000) }, { -INT64_C(0x38658893ec106200) }, { INT64_C(0x1e418527dc4ff900) },
	{ INT64_C(0x3188eeb1f4e39600) }, { INT64_C(0x288675a022f63a00) }, { -INT64_C(0x288675a022f63c00) }, { INT64_C(0x3188eeb1f4e39400) },
	{ INT64_C(0x065eec32aae95e00) }, { INT64_C(0x3fae9fba81577e00) }, { -INT64_C(0x3fae9fba81577e00) }, { INT64_C(0x065eec32aae95b00) },
	{ INT64_C(0x3eca11fde8f7aa00) }, { INT64_C(0x0c63b4cd9a653f00) }, { -INT64_C(0x0c63b4cd9a653f00) }, { INT64_C(0x3eca11fde8f7aa00) },
	{ INT64_C(0x23a3595e02598400) }, { INT64_C(0x3528d1b0b6415a00) }, { -INT64_C(0x3528d1b0b6415c00) }, { INT64_C(0x23a3595e02598400) },
	{ INT64_C(0x3544bebeb5dcc000) }, { INT64_C(0x23798e0d0088ce00) }, { -INT64_C(0x23798e0d0088d000) }, { INT64_C(0x3544bebeb5dcc000) },
	{ INT64_C(0x0c950181e40ca780) }, { INT64_C(0x3ec04393e2918400) }, { -INT64_C(0x3ec04393e2918400) }, { INT64_C(0x0c950181e40ca880) },
	{ INT64_C(0x3b2a713ca0853600) }, { INT64_C(0x1866a88a792ea000) }, { -INT64_C(0x1866a88a792ea000) }, { INT64_C(0x3b2a713ca0853600) },
	{ INT64_C(0x189518fbff098e00) }, { INT64_C(0x3b1734e1df396800) }, { -INT64_C(0x3b1734e1df396600) }, { INT64_C(0x189518fbff099300) },
	{ INT64_C(0x2d52fed25905d400) }, { INT64_C(0x2d2f73cd0d283800) }, { -INT64_C(0x2d2f73cd0d283600) }, { INT64_C(0x2d52fed25905d600) },
	{ INT64_C(0x001921faaee64730) }, { INT64_C(0x3ffffb10b0ddcc00) }, { -INT64_C(0x3ffffb10b0ddcc00) }, { INT64_C(0x001921faaee64c4a) },
};
