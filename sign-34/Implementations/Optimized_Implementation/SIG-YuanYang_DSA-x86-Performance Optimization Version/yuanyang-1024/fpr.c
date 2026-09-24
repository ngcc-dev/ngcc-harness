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
		/* The fixed-point polynomial uses two's-complement wrap semantics. */
		y = (int64_t)((uint64_t)c_q62[u - 1u]
			+ (uint64_t)yy_mul_q62(x_q62, y));
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
	{ INT64_C(0x0000000000000000) }, { INT64_C(0x0000000000000000) }, { INT64_C(0x0000000000000000) }, { INT64_C(0x4000000000000000) },
	{ INT64_C(0x2d413cccfe779921) }, { INT64_C(0x2d413cccfe779921) }, { -INT64_C(0x2d413cccfe779921) }, { INT64_C(0x2d413cccfe779921) },
	{ INT64_C(0x3b20d79e651a8c51) }, { INT64_C(0x187de2a6aea962d2) }, { -INT64_C(0x187de2a6aea962d2) }, { INT64_C(0x3b20d79e651a8c51) },
	{ INT64_C(0x187de2a6aea962d2) }, { INT64_C(0x3b20d79e651a8c51) }, { -INT64_C(0x3b20d79e651a8c51) }, { INT64_C(0x187de2a6aea962d2) },
	{ INT64_C(0x3ec52f9feeb96056) }, { INT64_C(0x0c7c5c1e34d3055b) }, { -INT64_C(0x0c7c5c1e34d3055b) }, { INT64_C(0x3ec52f9feeb96056) },
	{ INT64_C(0x238e76735cd190d9) }, { INT64_C(0x3536cc521d434606) }, { -INT64_C(0x3536cc521d434606) }, { INT64_C(0x238e76735cd190d9) },
	{ INT64_C(0x3536cc521d434606) }, { INT64_C(0x238e76735cd190d9) }, { -INT64_C(0x238e76735cd190d9) }, { INT64_C(0x3536cc521d434606) },
	{ INT64_C(0x0c7c5c1e34d3055b) }, { INT64_C(0x3ec52f9feeb96056) }, { -INT64_C(0x3ec52f9feeb96056) }, { INT64_C(0x0c7c5c1e34d3055b) },
	{ INT64_C(0x3fb11b47a24a4b3c) }, { INT64_C(0x0645e9af0a6d0af8) }, { -INT64_C(0x0645e9af0a6d0af8) }, { INT64_C(0x3fb11b47a24a4b3c) },
	{ INT64_C(0x2899e64a123bac30) }, { INT64_C(0x317900d62a2e816a) }, { -INT64_C(0x317900d62a2e816a) }, { INT64_C(0x2899e64a123bac30) },
	{ INT64_C(0x387165e3017b61a4) }, { INT64_C(0x1e2b5d3806f63b1e) }, { -INT64_C(0x1e2b5d3806f63b1e) }, { INT64_C(0x387165e3017b61a4) },
	{ INT64_C(0x1294062ed59f05a9) }, { INT64_C(0x3d3e82ad8c5bb4bb) }, { -INT64_C(0x3d3e82ad8c5bb4bb) }, { INT64_C(0x1294062ed59f05a9) },
	{ INT64_C(0x3d3e82ad8c5bb4bb) }, { INT64_C(0x1294062ed59f05a9) }, { -INT64_C(0x1294062ed59f05a9) }, { INT64_C(0x3d3e82ad8c5bb4bb) },
	{ INT64_C(0x1e2b5d3806f63b1e) }, { INT64_C(0x387165e3017b61a4) }, { -INT64_C(0x387165e3017b61a4) }, { INT64_C(0x1e2b5d3806f63b1e) },
	{ INT64_C(0x317900d62a2e816a) }, { INT64_C(0x2899e64a123bac30) }, { -INT64_C(0x2899e64a123bac30) }, { INT64_C(0x317900d62a2e816a) },
	{ INT64_C(0x0645e9af0a6d0af8) }, { INT64_C(0x3fb11b47a24a4b3c) }, { -INT64_C(0x3fb11b47a24a4b3c) }, { INT64_C(0x0645e9af0a6d0af8) },
	{ INT64_C(0x3fec43c6f2dafbc7) }, { INT64_C(0x0323ecbe21bb027d) }, { -INT64_C(0x0323ecbe21bb027d) }, { INT64_C(0x3fec43c6f2dafbc7) },
	{ INT64_C(0x2afad26919d93f45) }, { INT64_C(0x2f6bbe44d55f5dbc) }, { -INT64_C(0x2f6bbe44d55f5dbc) }, { INT64_C(0x2afad26919d93f45) },
	{ INT64_C(0x39daf5e8798ee5e2) }, { INT64_C(0x1b5d1009e15cc02b) }, { -INT64_C(0x1b5d1009e15cc02b) }, { INT64_C(0x39daf5e8798ee5e2) },
	{ INT64_C(0x158f9a75ab1fdcfe) }, { INT64_C(0x3c424209ed0dc97f) }, { -INT64_C(0x3c424209ed0dc97f) }, { INT64_C(0x158f9a75ab1fdcfe) },
	{ INT64_C(0x3e14fdf72461ae55) }, { INT64_C(0x0f8cfcbd90af8d58) }, { -INT64_C(0x0f8cfcbd90af8d58) }, { INT64_C(0x3e14fdf72461ae55) },
	{ INT64_C(0x20e70f3245ffdb2d) }, { INT64_C(0x36e5068a32dc7b22) }, { -INT64_C(0x36e5068a32dc7b22) }, { INT64_C(0x20e70f3245ffdb2d) },
	{ INT64_C(0x3367c08fe70e8168) }, { INT64_C(0x261feff9c2e069c2) }, { -INT64_C(0x261feff9c2e069c2) }, { INT64_C(0x3367c08fe70e8168) },
	{ INT64_C(0x0964083747309d11) }, { INT64_C(0x3f4eaafe114a2d43) }, { -INT64_C(0x3f4eaafe114a2d43) }, { INT64_C(0x0964083747309d11) },
	{ INT64_C(0x3f4eaafe114a2d43) }, { INT64_C(0x0964083747309d11) }, { -INT64_C(0x0964083747309d11) }, { INT64_C(0x3f4eaafe114a2d43) },
	{ INT64_C(0x261feff9c2e069c2) }, { INT64_C(0x3367c08fe70e8168) }, { -INT64_C(0x3367c08fe70e8168) }, { INT64_C(0x261feff9c2e069c2) },
	{ INT64_C(0x36e5068a32dc7b22) }, { INT64_C(0x20e70f3245ffdb2d) }, { -INT64_C(0x20e70f3245ffdb2d) }, { INT64_C(0x36e5068a32dc7b22) },
	{ INT64_C(0x0f8cfcbd90af8d58) }, { INT64_C(0x3e14fdf72461ae55) }, { -INT64_C(0x3e14fdf72461ae55) }, { INT64_C(0x0f8cfcbd90af8d58) },
	{ INT64_C(0x3c424209ed0dc97f) }, { INT64_C(0x158f9a75ab1fdcfe) }, { -INT64_C(0x158f9a75ab1fdcfe) }, { INT64_C(0x3c424209ed0dc97f) },
	{ INT64_C(0x1b5d1009e15cc02b) }, { INT64_C(0x39daf5e8798ee5e2) }, { -INT64_C(0x39daf5e8798ee5e2) }, { INT64_C(0x1b5d1009e15cc02b) },
	{ INT64_C(0x2f6bbe44d55f5dbc) }, { INT64_C(0x2afad26919d93f45) }, { -INT64_C(0x2afad26919d93f45) }, { INT64_C(0x2f6bbe44d55f5dbc) },
	{ INT64_C(0x0323ecbe21bb027d) }, { INT64_C(0x3fec43c6f2dafbc7) }, { -INT64_C(0x3fec43c6f2dafbc7) }, { INT64_C(0x0323ecbe21bb027d) },
	{ INT64_C(0x3ffb10c1099a1976) }, { INT64_C(0x0192155f7a3667e0) }, { -INT64_C(0x0192155f7a3667e0) }, { INT64_C(0x3ffb10c1099a1976) },
	{ INT64_C(0x2c216eaa3a59bdb7) }, { INT64_C(0x2e5a106fdfff2c87) }, { -INT64_C(0x2e5a106fdfff2c87) }, { INT64_C(0x2c216eaa3a59bdb7) },
	{ INT64_C(0x3a8269a29b927359) }, { INT64_C(0x19ef7943a8ed8a2e) }, { -INT64_C(0x19ef7943a8ed8a2e) }, { INT64_C(0x3a8269a29b927359) },
	{ INT64_C(0x17088530fa459eaf) }, { INT64_C(0x3bb6276d998478c2) }, { -INT64_C(0x3bb6276d998478c2) }, { INT64_C(0x17088530fa459eaf) },
	{ INT64_C(0x3e71e758c9cb118a) }, { INT64_C(0x0e05c1353f27b17e) }, { -INT64_C(0x0e05c1353f27b17e) }, { INT64_C(0x3e71e758c9cb118a) },
	{ INT64_C(0x223d66a836964508) }, { INT64_C(0x361214b02a03ff37) }, { -INT64_C(0x361214b02a03ff37) }, { INT64_C(0x223d66a836964508) },
	{ INT64_C(0x34534f408c4f03bb) }, { INT64_C(0x24da0a99ba25bd51) }, { -INT64_C(0x24da0a99ba25bd51) }, { INT64_C(0x34534f408c4f03bb) },
	{ INT64_C(0x0af10a22459fe32a) }, { INT64_C(0x3f0ec9f4e297526b) }, { -INT64_C(0x3f0ec9f4e297526b) }, { INT64_C(0x0af10a22459fe32a) },
	{ INT64_C(0x3f84c8e1c33fa68f) }, { INT64_C(0x07d59395aa5cc38d) }, { -INT64_C(0x07d59395aa5cc38d) }, { INT64_C(0x3f84c8e1c33fa68f) },
	{ INT64_C(0x275ff45240a17279) }, { INT64_C(0x3274449324c7f69f) }, { -INT64_C(0x3274449324c7f69f) }, { INT64_C(0x275ff45240a17279) },
	{ INT64_C(0x37af8158df2a533f) }, { INT64_C(0x1f8ba4dbf89ab9fb) }, { -INT64_C(0x1f8ba4dbf89ab9fb) }, { INT64_C(0x37af8158df2a533f) },
	{ INT64_C(0x1111d262b1f67761) }, { INT64_C(0x3dae81ced092c67a) }, { -INT64_C(0x3dae81ced092c67a) }, { INT64_C(0x1111d262b1f67761) },
	{ INT64_C(0x3cc511d891c223dd) }, { INT64_C(0x14135c9417660143) }, { -INT64_C(0x14135c9417660143) }, { INT64_C(0x3cc511d891c223dd) },
	{ INT64_C(0x1cc66e9931c45e17) }, { INT64_C(0x392a96426823e9ed) }, { -INT64_C(0x392a96426823e9ed) }, { INT64_C(0x1cc66e9931c45e17) },
	{ INT64_C(0x30761c17ff2edba4) }, { INT64_C(0x29cd9577c7cbd228) }, { -INT64_C(0x29cd9577c7cbd228) }, { INT64_C(0x30761c17ff2edba4) },
	{ INT64_C(0x04b54824b3867d73) }, { INT64_C(0x3fd39b5a0310742a) }, { -INT64_C(0x3fd39b5a0310742a) }, { INT64_C(0x04b54824b3867d73) },
	{ INT64_C(0x3fd39b5a0310742a) }, { INT64_C(0x04b54824b3867d73) }, { -INT64_C(0x04b54824b3867d73) }, { INT64_C(0x3fd39b5a0310742a) },
	{ INT64_C(0x29cd9577c7cbd228) }, { INT64_C(0x30761c17ff2edba4) }, { -INT64_C(0x30761c17ff2edba4) }, { INT64_C(0x29cd9577c7cbd228) },
	{ INT64_C(0x392a96426823e9ed) }, { INT64_C(0x1cc66e9931c45e17) }, { -INT64_C(0x1cc66e9931c45e17) }, { INT64_C(0x392a96426823e9ed) },
	{ INT64_C(0x14135c9417660143) }, { INT64_C(0x3cc511d891c223dd) }, { -INT64_C(0x3cc511d891c223dd) }, { INT64_C(0x14135c9417660143) },
	{ INT64_C(0x3dae81ced092c67a) }, { INT64_C(0x1111d262b1f67761) }, { -INT64_C(0x1111d262b1f67761) }, { INT64_C(0x3dae81ced092c67a) },
	{ INT64_C(0x1f8ba4dbf89ab9fb) }, { INT64_C(0x37af8158df2a533f) }, { -INT64_C(0x37af8158df2a533f) }, { INT64_C(0x1f8ba4dbf89ab9fb) },
	{ INT64_C(0x3274449324c7f69f) }, { INT64_C(0x275ff45240a17279) }, { -INT64_C(0x275ff45240a17279) }, { INT64_C(0x3274449324c7f69f) },
	{ INT64_C(0x07d59395aa5cc38d) }, { INT64_C(0x3f84c8e1c33fa68f) }, { -INT64_C(0x3f84c8e1c33fa68f) }, { INT64_C(0x07d59395aa5cc38d) },
	{ INT64_C(0x3f0ec9f4e297526b) }, { INT64_C(0x0af10a22459fe32a) }, { -INT64_C(0x0af10a22459fe32a) }, { INT64_C(0x3f0ec9f4e297526b) },
	{ INT64_C(0x24da0a99ba25bd51) }, { INT64_C(0x34534f408c4f03bb) }, { -INT64_C(0x34534f408c4f03bb) }, { INT64_C(0x24da0a99ba25bd51) },
	{ INT64_C(0x361214b02a03ff37) }, { INT64_C(0x223d66a836964508) }, { -INT64_C(0x223d66a836964508) }, { INT64_C(0x361214b02a03ff37) },
	{ INT64_C(0x0e05c1353f27b17e) }, { INT64_C(0x3e71e758c9cb118a) }, { -INT64_C(0x3e71e758c9cb118a) }, { INT64_C(0x0e05c1353f27b17e) },
	{ INT64_C(0x3bb6276d998478c2) }, { INT64_C(0x17088530fa459eaf) }, { -INT64_C(0x17088530fa459eaf) }, { INT64_C(0x3bb6276d998478c2) },
	{ INT64_C(0x19ef7943a8ed8a2e) }, { INT64_C(0x3a8269a29b927359) }, { -INT64_C(0x3a8269a29b927359) }, { INT64_C(0x19ef7943a8ed8a2e) },
	{ INT64_C(0x2e5a106fdfff2c87) }, { INT64_C(0x2c216eaa3a59bdb7) }, { -INT64_C(0x2c216eaa3a59bdb7) }, { INT64_C(0x2e5a106fdfff2c87) },
	{ INT64_C(0x0192155f7a3667e0) }, { INT64_C(0x3ffb10c1099a1976) }, { -INT64_C(0x3ffb10c1099a1976) }, { INT64_C(0x0192155f7a3667e0) },
	{ INT64_C(0x3ffec42d3725b6af) }, { INT64_C(0x00c90e8fe6f63c23) }, { -INT64_C(0x00c90e8fe6f63c23) }, { INT64_C(0x3ffec42d3725b6af) },
	{ INT64_C(0x2cb2324be0f07ae2) }, { INT64_C(0x2dce88a9d5515d12) }, { -INT64_C(0x2dce88a9d5515d12) }, { INT64_C(0x2cb2324be0f07ae2) },
	{ INT64_C(0x3ad2c2e793cd1586) }, { INT64_C(0x19372a63bc93d72d) }, { -INT64_C(0x19372a63bc93d72d) }, { INT64_C(0x3ad2c2e793cd1586) },
	{ INT64_C(0x17c3a9311dcce702) }, { INT64_C(0x3b6ca4c471413595) }, { -INT64_C(0x3b6ca4c471413595) }, { INT64_C(0x17c3a9311dcce702) },
	{ INT64_C(0x3e9cc076165e599c) }, { INT64_C(0x0d415012d802284f) }, { -INT64_C(0x0d415012d802284f) }, { INT64_C(0x3e9cc076165e599c) },
	{ INT64_C(0x22e69ac7bdb69141) }, { INT64_C(0x35a5793c43aa215c) }, { -INT64_C(0x35a5793c43aa215c) }, { INT64_C(0x22e69ac7bdb69141) },
	{ INT64_C(0x34c6123605f5c386) }, { INT64_C(0x2434f33267d6b163) }, { -INT64_C(0x2434f33267d6b163) }, { INT64_C(0x34c6123605f5c386) },
	{ INT64_C(0x0bb6ecef285f98a4) }, { INT64_C(0x3eeb33474240eec2) }, { -INT64_C(0x3eeb33474240eec2) }, { INT64_C(0x0bb6ecef285f98a4) },
	{ INT64_C(0x3f9c2bfadb4cf5a9) }, { INT64_C(0x070de171e7b0b53d) }, { -INT64_C(0x070de171e7b0b53d) }, { INT64_C(0x3f9c2bfadb4cf5a9) },
	{ INT64_C(0x27fdb2a68aada89b) }, { INT64_C(0x31f79947df2819d2) }, { -INT64_C(0x31f79947df2819d2) }, { INT64_C(0x27fdb2a68aada89b) },
	{ INT64_C(0x3811884ce4aa921b) }, { INT64_C(0x1edc1952ef78d589) }, { -INT64_C(0x1edc1952ef78d589) }, { INT64_C(0x3811884ce4aa921b) },
	{ INT64_C(0x11d3443f4cdb3dd2) }, { INT64_C(0x3d77b191be16e872) }, { -INT64_C(0x3d77b191be16e872) }, { INT64_C(0x11d3443f4cdb3dd2) },
	{ INT64_C(0x3d02f75699a2198c) }, { INT64_C(0x135410c2e18151b1) }, { -INT64_C(0x135410c2e18151b1) }, { INT64_C(0x3d02f75699a2198c) },
	{ INT64_C(0x1d79775b86e38955) }, { INT64_C(0x38cf166910e7363b) }, { -INT64_C(0x38cf166910e7363b) }, { INT64_C(0x1d79775b86e38955) },
	{ INT64_C(0x30f8801f745d7d69) }, { INT64_C(0x293489373612716c) }, { -INT64_C(0x293489373612716c) }, { INT64_C(0x30f8801f745d7d69) },
	{ INT64_C(0x057db402a6a90630) }, { INT64_C(0x3fc395f97ab61234) }, { -INT64_C(0x3fc395f97ab61234) }, { INT64_C(0x057db402a6a90630) },
	{ INT64_C(0x3fe12acb1ce35a81) }, { INT64_C(0x03ecadcf3f041bfe) }, { -INT64_C(0x03ecadcf3f041bfe) }, { INT64_C(0x3fe12acb1ce35a81) },
	{ INT64_C(0x2a65052546ab2b98) }, { INT64_C(0x2ff1d9c6ae2ee132) }, { -INT64_C(0x2ff1d9c6ae2ee132) }, { INT64_C(0x2a65052546ab2b98) },
	{ INT64_C(0x3983e1e7f9f8b879) }, { INT64_C(0x1c1249d8011ee6a0) }, { -INT64_C(0x1c1249d8011ee6a0) }, { INT64_C(0x3983e1e7f9f8b879) },
	{ INT64_C(0x14d1e24278e76a25) }, { INT64_C(0x3c84d4965782fcd4) }, { -INT64_C(0x3c84d4965782fcd4) }, { INT64_C(0x14d1e24278e76a25) },
	{ INT64_C(0x3de2f147c8e784b2) }, { INT64_C(0x104fb80e37fdadff) }, { -INT64_C(0x104fb80e37fdadff) }, { INT64_C(0x3de2f147c8e784b2) },
	{ INT64_C(0x2039f90e987d6db3) }, { INT64_C(0x374b54ce6b21a4bf) }, { -INT64_C(0x374b54ce6b21a4bf) }, { INT64_C(0x2039f90e987d6db3) },
	{ INT64_C(0x32eefde98fae8375) }, { INT64_C(0x26c0b1620cb3e570) }, { -INT64_C(0x26c0b1620cb3e570) }, { INT64_C(0x32eefde98fae8375) },
	{ INT64_C(0x089cf8676d7abb56) }, { INT64_C(0x3f6af2e32bae8247) }, { -INT64_C(0x3f6af2e32bae8247) }, { INT64_C(0x089cf8676d7abb56) },
	{ INT64_C(0x3f2ff2499213350f) }, { INT64_C(0x0a2abb58949f2ced) }, { -INT64_C(0x0a2abb58949f2ced) }, { INT64_C(0x3f2ff2499213350f) },
	{ INT64_C(0x257db64bf5e7d3ef) }, { INT64_C(0x33de87de535f286c) }, { -INT64_C(0x33de87de535f286c) }, { INT64_C(0x257db64bf5e7d3ef) },
	{ INT64_C(0x367c9a7deaae230a) }, { INT64_C(0x2192e09abb131d39) }, { -INT64_C(0x2192e09abb131d39) }, { INT64_C(0x367c9a7deaae230a) },
	{ INT64_C(0x0ec9a7f2a2a188af) }, { INT64_C(0x3e44a5eeec75b370) }, { -INT64_C(0x3e44a5eeec75b370) }, { INT64_C(0x0ec9a7f2a2a188af) },
	{ INT64_C(0x3bfd5cc45b7c5557) }, { INT64_C(0x164c7ddd3f27c611) }, { -INT64_C(0x164c7ddd3f27c611) }, { INT64_C(0x3bfd5cc45b7c5557) },
	{ INT64_C(0x1aa6c82b6d3fc98b) }, { INT64_C(0x3a2fcee87c6bb7ef) }, { -INT64_C(0x3a2fcee87c6bb7ef) }, { INT64_C(0x1aa6c82b6d3fc98b) },
	{ INT64_C(0x2ee3cebe06e4c257) }, { INT64_C(0x2b8ef77cca031883) }, { -INT64_C(0x2b8ef77cca031883) }, { INT64_C(0x2ee3cebe06e4c257) },
	{ INT64_C(0x025b0caeb28ab9a3) }, { INT64_C(0x3ff4e5dffdeeb93a) }, { -INT64_C(0x3ff4e5dffdeeb93a) }, { INT64_C(0x025b0caeb28ab9a3) },
	{ INT64_C(0x3ff4e5dffdeeb93a) }, { INT64_C(0x025b0caeb28ab9a3) }, { -INT64_C(0x025b0caeb28ab9a3) }, { INT64_C(0x3ff4e5dffdeeb93a) },
	{ INT64_C(0x2b8ef77cca031883) }, { INT64_C(0x2ee3cebe06e4c257) }, { -INT64_C(0x2ee3cebe06e4c257) }, { INT64_C(0x2b8ef77cca031883) },
	{ INT64_C(0x3a2fcee87c6bb7ef) }, { INT64_C(0x1aa6c82b6d3fc98b) }, { -INT64_C(0x1aa6c82b6d3fc98b) }, { INT64_C(0x3a2fcee87c6bb7ef) },
	{ INT64_C(0x164c7ddd3f27c611) }, { INT64_C(0x3bfd5cc45b7c5557) }, { -INT64_C(0x3bfd5cc45b7c5557) }, { INT64_C(0x164c7ddd3f27c611) },
	{ INT64_C(0x3e44a5eeec75b370) }, { INT64_C(0x0ec9a7f2a2a188af) }, { -INT64_C(0x0ec9a7f2a2a188af) }, { INT64_C(0x3e44a5eeec75b370) },
	{ INT64_C(0x2192e09abb131d39) }, { INT64_C(0x367c9a7deaae230a) }, { -INT64_C(0x367c9a7deaae230a) }, { INT64_C(0x2192e09abb131d39) },
	{ INT64_C(0x33de87de535f286c) }, { INT64_C(0x257db64bf5e7d3ef) }, { -INT64_C(0x257db64bf5e7d3ef) }, { INT64_C(0x33de87de535f286c) },
	{ INT64_C(0x0a2abb58949f2ced) }, { INT64_C(0x3f2ff2499213350f) }, { -INT64_C(0x3f2ff2499213350f) }, { INT64_C(0x0a2abb58949f2ced) },
	{ INT64_C(0x3f6af2e32bae8247) }, { INT64_C(0x089cf8676d7abb56) }, { -INT64_C(0x089cf8676d7abb56) }, { INT64_C(0x3f6af2e32bae8247) },
	{ INT64_C(0x26c0b1620cb3e570) }, { INT64_C(0x32eefde98fae8375) }, { -INT64_C(0x32eefde98fae8375) }, { INT64_C(0x26c0b1620cb3e570) },
	{ INT64_C(0x374b54ce6b21a4bf) }, { INT64_C(0x2039f90e987d6db3) }, { -INT64_C(0x2039f90e987d6db3) }, { INT64_C(0x374b54ce6b21a4bf) },
	{ INT64_C(0x104fb80e37fdadff) }, { INT64_C(0x3de2f147c8e784b2) }, { -INT64_C(0x3de2f147c8e784b2) }, { INT64_C(0x104fb80e37fdadff) },
	{ INT64_C(0x3c84d4965782fcd4) }, { INT64_C(0x14d1e24278e76a25) }, { -INT64_C(0x14d1e24278e76a25) }, { INT64_C(0x3c84d4965782fcd4) },
	{ INT64_C(0x1c1249d8011ee6a0) }, { INT64_C(0x3983e1e7f9f8b879) }, { -INT64_C(0x3983e1e7f9f8b879) }, { INT64_C(0x1c1249d8011ee6a0) },
	{ INT64_C(0x2ff1d9c6ae2ee132) }, { INT64_C(0x2a65052546ab2b98) }, { -INT64_C(0x2a65052546ab2b98) }, { INT64_C(0x2ff1d9c6ae2ee132) },
	{ INT64_C(0x03ecadcf3f041bfe) }, { INT64_C(0x3fe12acb1ce35a81) }, { -INT64_C(0x3fe12acb1ce35a81) }, { INT64_C(0x03ecadcf3f041bfe) },
	{ INT64_C(0x3fc395f97ab61234) }, { INT64_C(0x057db402a6a90630) }, { -INT64_C(0x057db402a6a90630) }, { INT64_C(0x3fc395f97ab61234) },
	{ INT64_C(0x293489373612716c) }, { INT64_C(0x30f8801f745d7d69) }, { -INT64_C(0x30f8801f745d7d69) }, { INT64_C(0x293489373612716c) },
	{ INT64_C(0x38cf166910e7363b) }, { INT64_C(0x1d79775b86e38955) }, { -INT64_C(0x1d79775b86e38955) }, { INT64_C(0x38cf166910e7363b) },
	{ INT64_C(0x135410c2e18151b1) }, { INT64_C(0x3d02f75699a2198c) }, { -INT64_C(0x3d02f75699a2198c) }, { INT64_C(0x135410c2e18151b1) },
	{ INT64_C(0x3d77b191be16e872) }, { INT64_C(0x11d3443f4cdb3dd2) }, { -INT64_C(0x11d3443f4cdb3dd2) }, { INT64_C(0x3d77b191be16e872) },
	{ INT64_C(0x1edc1952ef78d589) }, { INT64_C(0x3811884ce4aa921b) }, { -INT64_C(0x3811884ce4aa921b) }, { INT64_C(0x1edc1952ef78d589) },
	{ INT64_C(0x31f79947df2819d2) }, { INT64_C(0x27fdb2a68aada89b) }, { -INT64_C(0x27fdb2a68aada89b) }, { INT64_C(0x31f79947df2819d2) },
	{ INT64_C(0x070de171e7b0b53d) }, { INT64_C(0x3f9c2bfadb4cf5a9) }, { -INT64_C(0x3f9c2bfadb4cf5a9) }, { INT64_C(0x070de171e7b0b53d) },
	{ INT64_C(0x3eeb33474240eec2) }, { INT64_C(0x0bb6ecef285f98a4) }, { -INT64_C(0x0bb6ecef285f98a4) }, { INT64_C(0x3eeb33474240eec2) },
	{ INT64_C(0x2434f33267d6b163) }, { INT64_C(0x34c6123605f5c386) }, { -INT64_C(0x34c6123605f5c386) }, { INT64_C(0x2434f33267d6b163) },
	{ INT64_C(0x35a5793c43aa215c) }, { INT64_C(0x22e69ac7bdb69141) }, { -INT64_C(0x22e69ac7bdb69141) }, { INT64_C(0x35a5793c43aa215c) },
	{ INT64_C(0x0d415012d802284f) }, { INT64_C(0x3e9cc076165e599c) }, { -INT64_C(0x3e9cc076165e599c) }, { INT64_C(0x0d415012d802284f) },
	{ INT64_C(0x3b6ca4c471413595) }, { INT64_C(0x17c3a9311dcce702) }, { -INT64_C(0x17c3a9311dcce702) }, { INT64_C(0x3b6ca4c471413595) },
	{ INT64_C(0x19372a63bc93d72d) }, { INT64_C(0x3ad2c2e793cd1586) }, { -INT64_C(0x3ad2c2e793cd1586) }, { INT64_C(0x19372a63bc93d72d) },
	{ INT64_C(0x2dce88a9d5515d12) }, { INT64_C(0x2cb2324be0f07ae2) }, { -INT64_C(0x2cb2324be0f07ae2) }, { INT64_C(0x2dce88a9d5515d12) },
	{ INT64_C(0x00c90e8fe6f63c23) }, { INT64_C(0x3ffec42d3725b6af) }, { -INT64_C(0x3ffec42d3725b6af) }, { INT64_C(0x00c90e8fe6f63c23) },
	{ INT64_C(0x3fffb10b1d15249b) }, { INT64_C(0x006487c3f99c01c4) }, { -INT64_C(0x006487c3f99c01c4) }, { INT64_C(0x3fffb10b1d15249b) },
	{ INT64_C(0x2cf9ef09235e200c) }, { INT64_C(0x2d881ae78304ea25) }, { -INT64_C(0x2d881ae78304ea25) }, { INT64_C(0x2cf9ef09235e200c) },
	{ INT64_C(0x3afa160571d9f2c0) }, { INT64_C(0x18daa52ec8a4afd2) }, { -INT64_C(0x18daa52ec8a4afd2) }, { INT64_C(0x3afa160571d9f2c0) },
	{ INT64_C(0x1820e3b04eaac3f3) }, { INT64_C(0x3b470752cd130f54) }, { -INT64_C(0x3b470752cd130f54) }, { INT64_C(0x1820e3b04eaac3f3) },
	{ INT64_C(0x3eb14562f13d0848) }, { INT64_C(0x0cdee5f96e21b333) }, { -INT64_C(0x0cdee5f96e21b333) }, { INT64_C(0x3eb14562f13d0848) },
	{ INT64_C(0x233ab413e5736fda) }, { INT64_C(0x356e64b22d81a8d4) }, { -INT64_C(0x356e64b22d81a8d4) }, { INT64_C(0x233ab413e5736fda) },
	{ INT64_C(0x34feb0a53fcd3934) }, { INT64_C(0x23e1e117790c35de) }, { -INT64_C(0x23e1e117790c35de) }, { INT64_C(0x34feb0a53fcd3934) },
	{ INT64_C(0x0c19b3744e3262dd) }, { INT64_C(0x3ed87efbeb776e61) }, { -INT64_C(0x3ed87efbeb776e61) }, { INT64_C(0x0c19b3744e3262dd) },
	{ INT64_C(0x3fa6f228441708a9) }, { INT64_C(0x06a9edc9125700de) }, { -INT64_C(0x06a9edc9125700de) }, { INT64_C(0x3fa6f228441708a9) },
	{ INT64_C(0x284bfe2f1cd762be) }, { INT64_C(0x31b88a662d319824) }, { -INT64_C(0x31b88a662d319824) }, { INT64_C(0x284bfe2f1cd762be) },
	{ INT64_C(0x3841bc7f52e35f26) }, { INT64_C(0x1e83e0eaf85113d1) }, { -INT64_C(0x1e83e0eaf85113d1) }, { INT64_C(0x3841bc7f52e35f26) },
	{ INT64_C(0x1233bbabc3bb7166) }, { INT64_C(0x3d5b65d1cf511b37) }, { -INT64_C(0x3d5b65d1cf511b37) }, { INT64_C(0x1233bbabc3bb7166) },
	{ INT64_C(0x3d21086c3befe4e7) }, { INT64_C(0x12f422daec0386a3) }, { -INT64_C(0x12f422daec0386a3) }, { INT64_C(0x3d21086c3befe4e7) },
	{ INT64_C(0x1dd28f1481cc57f1) }, { INT64_C(0x38a0840256d20dd4) }, { -INT64_C(0x38a0840256d20dd4) }, { INT64_C(0x1dd28f1481cc57f1) },
	{ INT64_C(0x3138fd349ba954ee) }, { INT64_C(0x28e76a3730e68e39) }, { -INT64_C(0x28e76a3730e68e39) }, { INT64_C(0x3138fd349ba954ee) },
	{ INT64_C(0x05e1d61a9756c856) }, { INT64_C(0x3fbaa73fe3e8ab95) }, { -INT64_C(0x3fbaa73fe3e8ab95) }, { INT64_C(0x05e1d61a9756c856) },
	{ INT64_C(0x3fe7061f1aaeb79b) }, { INT64_C(0x038851a2581afc5a) }, { -INT64_C(0x038851a2581afc5a) }, { INT64_C(0x3fe7061f1aaeb79b) },
	{ INT64_C(0x2ab020712ea26ea3) }, { INT64_C(0x2faf06d9867b6446) }, { -INT64_C(0x2faf06d9867b6446) }, { INT64_C(0x2ab020712ea26ea3) },
	{ INT64_C(0x39afb3131665ebc2) }, { INT64_C(0x1bb7cf2304bd0134) }, { -INT64_C(0x1bb7cf2304bd0134) }, { INT64_C(0x39afb3131665ebc2) },
	{ INT64_C(0x1530d880af3c2381) }, { INT64_C(0x3c63d5d0e19c4991) }, { -INT64_C(0x3c63d5d0e19c4991) }, { INT64_C(0x1530d880af3c2381) },
	{ INT64_C(0x3dfc4418172bd8e4) }, { INT64_C(0x0fee6e0d6ff6fc5a) }, { -INT64_C(0x0fee6e0d6ff6fc5a) }, { INT64_C(0x3dfc4418172bd8e4) },
	{ INT64_C(0x2090ac4d5c4434dd) }, { INT64_C(0x371871a4ea09d175) }, { -INT64_C(0x371871a4ea09d175) }, { INT64_C(0x2090ac4d5c4434dd) },
	{ INT64_C(0x332b9e5db01a445e) }, { INT64_C(0x2670801a191cad2a) }, { -INT64_C(0x2670801a191cad2a) }, { INT64_C(0x332b9e5db01a445e) },
	{ INT64_C(0x09008b6a763de75b) }, { INT64_C(0x3f5d1d1c8d9f75b1) }, { -INT64_C(0x3f5d1d1c8d9f75b1) }, { INT64_C(0x09008b6a763de75b) },
	{ INT64_C(0x3f3f9cab5b65907d) }, { INT64_C(0x09c76dd866c689dd) }, { -INT64_C(0x09c76dd866c689dd) }, { INT64_C(0x3f3f9cab5b65907d) },
	{ INT64_C(0x25cf01c7d1d42d27) }, { INT64_C(0x33a363ebd501aae3) }, { -INT64_C(0x33a363ebd501aae3) }, { INT64_C(0x25cf01c7d1d42d27) },
	{ INT64_C(0x36b113fd242809c4) }, { INT64_C(0x213d20e82f8bc101) }, { -INT64_C(0x213d20e82f8bc101) }, { INT64_C(0x36b113fd242809c4) },
	{ INT64_C(0x0f2b650f080d0da9) }, { INT64_C(0x3e2d1ea7ee40b9db) }, { -INT64_C(0x3e2d1ea7ee40b9db) }, { INT64_C(0x0f2b650f080d0da9) },
	{ INT64_C(0x3c201994530157e0) }, { INT64_C(0x15ee27379ea69359) }, { -INT64_C(0x15ee27379ea69359) }, { INT64_C(0x3c201994530157e0) },
	{ INT64_C(0x1b020d6c7f400914) }, { INT64_C(0x3a05a9fd657b248d) }, { -INT64_C(0x3a05a9fd657b248d) }, { INT64_C(0x1b020d6c7f400914) },
	{ INT64_C(0x2f2800ae9eabc97b) }, { INT64_C(0x2b451a54bae4a0ac) }, { -INT64_C(0x2b451a54bae4a0ac) }, { INT64_C(0x2f2800ae9eabc97b) },
	{ INT64_C(0x02bf801a5219a86d) }, { INT64_C(0x3ff0e3b5b703be63) }, { -INT64_C(0x3ff0e3b5b703be63) }, { INT64_C(0x02bf801a5219a86d) },
	{ INT64_C(0x3ff84a3be3a7f05f) }, { INT64_C(0x01f693731d1cf010) }, { -INT64_C(0x01f693731d1cf010) }, { INT64_C(0x3ff84a3be3a7f05f) },
	{ INT64_C(0x2bd8692b06e0e878) }, { INT64_C(0x2e9f291b51a51a01) }, { -INT64_C(0x2e9f291b51a51a01) }, { INT64_C(0x2bd8692b06e0e878) },
	{ INT64_C(0x3a596441c1df3d84) }, { INT64_C(0x1a4b4127dea1e490) }, { -INT64_C(0x1a4b4127dea1e490) }, { INT64_C(0x3a596441c1df3d84) },
	{ INT64_C(0x16aa9d7dc77e16b2) }, { INT64_C(0x3bda0befbc8fb36a) }, { -INT64_C(0x3bda0befbc8fb36a) }, { INT64_C(0x16aa9d7dc77e16b2) },
	{ INT64_C(0x3e5b939211353a0b) }, { INT64_C(0x0e67c65989594312) }, { -INT64_C(0x0e67c65989594312) }, { INT64_C(0x3e5b939211353a0b) },
	{ INT64_C(0x21e84d76551cfb22) }, { INT64_C(0x36479a8e00276857) }, { -INT64_C(0x36479a8e00276857) }, { INT64_C(0x21e84d76551cfb22) },
	{ INT64_C(0x34192bd575f26d10) }, { INT64_C(0x252c0e4ec5395056) }, { -INT64_C(0x252c0e4ec5395056) }, { INT64_C(0x34192bd575f26d10) },
	{ INT64_C(0x0a8defc2cbe2f8fd) }, { INT64_C(0x3f1fabff5c83b59d) }, { -INT64_C(0x3f1fabff5c83b59d) }, { INT64_C(0x0a8defc2cbe2f8fd) },
	{ INT64_C(0x3f782c2fc8830bf5) }, { INT64_C(0x08395023dd418e92) }, { -INT64_C(0x08395023dd418e92) }, { INT64_C(0x3f782c2fc8830bf5) },
	{ INT64_C(0x2710830bbfd64398) }, { INT64_C(0x32b1dfc91cdbad55) }, { -INT64_C(0x32b1dfc91cdbad55) }, { INT64_C(0x2710830bbfd64398) },
	{ INT64_C(0x377daf892701d40e) }, { INT64_C(0x1fe2f64be7120fb6) }, { -INT64_C(0x1fe2f64be7120fb6) }, { INT64_C(0x377daf892701d40e) },
	{ INT64_C(0x10b0d9cfdbdb9014) }, { INT64_C(0x3dc905c4b53b7792) }, { -INT64_C(0x3dc905c4b53b7792) }, { INT64_C(0x10b0d9cfdbdb9014) },
	{ INT64_C(0x3ca53e08e53ff8c8) }, { INT64_C(0x1472b8a5571053c0) }, { -INT64_C(0x1472b8a5571053c0) }, { INT64_C(0x3ca53e08e53ff8c8) },
	{ INT64_C(0x1c6c7f4997000a90) }, { INT64_C(0x395782d3417200e2) }, { -INT64_C(0x395782d3417200e2) }, { INT64_C(0x1c6c7f4997000a90) },
	{ INT64_C(0x303436676af59751) }, { INT64_C(0x2a19813eb341365a) }, { -INT64_C(0x2a19813eb341365a) }, { INT64_C(0x303436676af59751) },
	{ INT64_C(0x0451004d35c26ca0) }, { INT64_C(0x3fdab1d96ce78786) }, { -INT64_C(0x3fdab1d96ce78786) }, { INT64_C(0x0451004d35c26ca0) },
	{ INT64_C(0x3fcbe75e5c7280d9) }, { INT64_C(0x0519845e49c8256b) }, { -INT64_C(0x0519845e49c8256b) }, { INT64_C(0x3fcbe75e5c7280d9) },
	{ INT64_C(0x2981428bd8000812) }, { INT64_C(0x30b78a35d2b198a3) }, { -INT64_C(0x30b78a35d2b198a3) }, { INT64_C(0x2981428bd8000812) },
	{ INT64_C(0x38fd1ca44679e636) }, { INT64_C(0x1d2016e8e9db5ac7) }, { -INT64_C(0x1d2016e8e9db5ac7) }, { INT64_C(0x38fd1ca44679e636) },
	{ INT64_C(0x13b3cefa0414b77d) }, { INT64_C(0x3ce44fb6d52e8891) }, { -INT64_C(0x3ce44fb6d52e8891) }, { INT64_C(0x13b3cefa0414b77d) },
	{ INT64_C(0x3d9365a7877f0846) }, { INT64_C(0x1172a0d776517724) }, { -INT64_C(0x1172a0d776517724) }, { INT64_C(0x3d9365a7877f0846) },
	{ INT64_C(0x1f3405963fd06742) }, { INT64_C(0x37e0c9c2a6efba24) }, { -INT64_C(0x37e0c9c2a6efba24) }, { INT64_C(0x1f3405963fd06742) },
	{ INT64_C(0x32362cdfa93b43d9) }, { INT64_C(0x27af04718b06877f) }, { -INT64_C(0x27af04718b06877f) }, { INT64_C(0x32362cdfa93b43d9) },
	{ INT64_C(0x0771c3b2eba7f245) }, { INT64_C(0x3f90c8d9fd6e4299) }, { -INT64_C(0x3f90c8d9fd6e4299) }, { INT64_C(0x0771c3b2eba7f245) },
	{ INT64_C(0x3efd4c53cc7adcdd) }, { INT64_C(0x0b5409827b25591f) }, { -INT64_C(0x0b5409827b25591f) }, { INT64_C(0x3efd4c53cc7adcdd) },
	{ INT64_C(0x2487abf731583e71) }, { INT64_C(0x348cf1902335908e) }, { -INT64_C(0x348cf1902335908e) }, { INT64_C(0x2487abf731583e71) },
	{ INT64_C(0x35dc09687828e763) }, { INT64_C(0x22922b5e66d9d67d) }, { -INT64_C(0x22922b5e66d9d67d) }, { INT64_C(0x35dc09687828e763) },
	{ INT64_C(0x0da399779eb39137) }, { INT64_C(0x3e87a10bff25b938) }, { -INT64_C(0x3e87a10bff25b938) }, { INT64_C(0x0da399779eb39137) },
	{ INT64_C(0x3b91af968204c05b) }, { INT64_C(0x1766340f2418f64b) }, { -INT64_C(0x1766340f2418f64b) }, { INT64_C(0x3b91af968204c05b) },
	{ INT64_C(0x1993716141bdfebb) }, { INT64_C(0x3aaadea5d27d6140) }, { -INT64_C(0x3aaadea5d27d6140) }, { INT64_C(0x1993716141bdfebb) },
	{ INT64_C(0x2e1485662edaf38a) }, { INT64_C(0x2c6a07463837d222) }, { -INT64_C(0x2c6a07463837d222) }, { INT64_C(0x2e1485662edaf38a) },
	{ INT64_C(0x012d936bbe30efd3) }, { INT64_C(0x3ffd396896a34257) }, { -INT64_C(0x3ffd396896a34257) }, { INT64_C(0x012d936bbe30efd3) },
	{ INT64_C(0x3ffd396896a34257) }, { INT64_C(0x012d936bbe30efd3) }, { -INT64_C(0x012d936bbe30efd3) }, { INT64_C(0x3ffd396896a34257) },
	{ INT64_C(0x2c6a07463837d222) }, { INT64_C(0x2e1485662edaf38a) }, { -INT64_C(0x2e1485662edaf38a) }, { INT64_C(0x2c6a07463837d222) },
	{ INT64_C(0x3aaadea5d27d6140) }, { INT64_C(0x1993716141bdfebb) }, { -INT64_C(0x1993716141bdfebb) }, { INT64_C(0x3aaadea5d27d6140) },
	{ INT64_C(0x1766340f2418f64b) }, { INT64_C(0x3b91af968204c05b) }, { -INT64_C(0x3b91af968204c05b) }, { INT64_C(0x1766340f2418f64b) },
	{ INT64_C(0x3e87a10bff25b938) }, { INT64_C(0x0da399779eb39137) }, { -INT64_C(0x0da399779eb39137) }, { INT64_C(0x3e87a10bff25b938) },
	{ INT64_C(0x22922b5e66d9d67d) }, { INT64_C(0x35dc09687828e763) }, { -INT64_C(0x35dc09687828e763) }, { INT64_C(0x22922b5e66d9d67d) },
	{ INT64_C(0x348cf1902335908e) }, { INT64_C(0x2487abf731583e71) }, { -INT64_C(0x2487abf731583e71) }, { INT64_C(0x348cf1902335908e) },
	{ INT64_C(0x0b5409827b25591f) }, { INT64_C(0x3efd4c53cc7adcdd) }, { -INT64_C(0x3efd4c53cc7adcdd) }, { INT64_C(0x0b5409827b25591f) },
	{ INT64_C(0x3f90c8d9fd6e4299) }, { INT64_C(0x0771c3b2eba7f245) }, { -INT64_C(0x0771c3b2eba7f245) }, { INT64_C(0x3f90c8d9fd6e4299) },
	{ INT64_C(0x27af04718b06877f) }, { INT64_C(0x32362cdfa93b43d9) }, { -INT64_C(0x32362cdfa93b43d9) }, { INT64_C(0x27af04718b06877f) },
	{ INT64_C(0x37e0c9c2a6efba24) }, { INT64_C(0x1f3405963fd06742) }, { -INT64_C(0x1f3405963fd06742) }, { INT64_C(0x37e0c9c2a6efba24) },
	{ INT64_C(0x1172a0d776517724) }, { INT64_C(0x3d9365a7877f0846) }, { -INT64_C(0x3d9365a7877f0846) }, { INT64_C(0x1172a0d776517724) },
	{ INT64_C(0x3ce44fb6d52e8891) }, { INT64_C(0x13b3cefa0414b77d) }, { -INT64_C(0x13b3cefa0414b77d) }, { INT64_C(0x3ce44fb6d52e8891) },
	{ INT64_C(0x1d2016e8e9db5ac7) }, { INT64_C(0x38fd1ca44679e636) }, { -INT64_C(0x38fd1ca44679e636) }, { INT64_C(0x1d2016e8e9db5ac7) },
	{ INT64_C(0x30b78a35d2b198a3) }, { INT64_C(0x2981428bd8000812) }, { -INT64_C(0x2981428bd8000812) }, { INT64_C(0x30b78a35d2b198a3) },
	{ INT64_C(0x0519845e49c8256b) }, { INT64_C(0x3fcbe75e5c7280d9) }, { -INT64_C(0x3fcbe75e5c7280d9) }, { INT64_C(0x0519845e49c8256b) },
	{ INT64_C(0x3fdab1d96ce78786) }, { INT64_C(0x0451004d35c26ca0) }, { -INT64_C(0x0451004d35c26ca0) }, { INT64_C(0x3fdab1d96ce78786) },
	{ INT64_C(0x2a19813eb341365a) }, { INT64_C(0x303436676af59751) }, { -INT64_C(0x303436676af59751) }, { INT64_C(0x2a19813eb341365a) },
	{ INT64_C(0x395782d3417200e2) }, { INT64_C(0x1c6c7f4997000a90) }, { -INT64_C(0x1c6c7f4997000a90) }, { INT64_C(0x395782d3417200e2) },
	{ INT64_C(0x1472b8a5571053c0) }, { INT64_C(0x3ca53e08e53ff8c8) }, { -INT64_C(0x3ca53e08e53ff8c8) }, { INT64_C(0x1472b8a5571053c0) },
	{ INT64_C(0x3dc905c4b53b7792) }, { INT64_C(0x10b0d9cfdbdb9014) }, { -INT64_C(0x10b0d9cfdbdb9014) }, { INT64_C(0x3dc905c4b53b7792) },
	{ INT64_C(0x1fe2f64be7120fb6) }, { INT64_C(0x377daf892701d40e) }, { -INT64_C(0x377daf892701d40e) }, { INT64_C(0x1fe2f64be7120fb6) },
	{ INT64_C(0x32b1dfc91cdbad55) }, { INT64_C(0x2710830bbfd64398) }, { -INT64_C(0x2710830bbfd64398) }, { INT64_C(0x32b1dfc91cdbad55) },
	{ INT64_C(0x08395023dd418e92) }, { INT64_C(0x3f782c2fc8830bf5) }, { -INT64_C(0x3f782c2fc8830bf5) }, { INT64_C(0x08395023dd418e92) },
	{ INT64_C(0x3f1fabff5c83b59d) }, { INT64_C(0x0a8defc2cbe2f8fd) }, { -INT64_C(0x0a8defc2cbe2f8fd) }, { INT64_C(0x3f1fabff5c83b59d) },
	{ INT64_C(0x252c0e4ec5395056) }, { INT64_C(0x34192bd575f26d10) }, { -INT64_C(0x34192bd575f26d10) }, { INT64_C(0x252c0e4ec5395056) },
	{ INT64_C(0x36479a8e00276857) }, { INT64_C(0x21e84d76551cfb22) }, { -INT64_C(0x21e84d76551cfb22) }, { INT64_C(0x36479a8e00276857) },
	{ INT64_C(0x0e67c65989594312) }, { INT64_C(0x3e5b939211353a0b) }, { -INT64_C(0x3e5b939211353a0b) }, { INT64_C(0x0e67c65989594312) },
	{ INT64_C(0x3bda0befbc8fb36a) }, { INT64_C(0x16aa9d7dc77e16b2) }, { -INT64_C(0x16aa9d7dc77e16b2) }, { INT64_C(0x3bda0befbc8fb36a) },
	{ INT64_C(0x1a4b4127dea1e490) }, { INT64_C(0x3a596441c1df3d84) }, { -INT64_C(0x3a596441c1df3d84) }, { INT64_C(0x1a4b4127dea1e490) },
	{ INT64_C(0x2e9f291b51a51a01) }, { INT64_C(0x2bd8692b06e0e878) }, { -INT64_C(0x2bd8692b06e0e878) }, { INT64_C(0x2e9f291b51a51a01) },
	{ INT64_C(0x01f693731d1cf010) }, { INT64_C(0x3ff84a3be3a7f05f) }, { -INT64_C(0x3ff84a3be3a7f05f) }, { INT64_C(0x01f693731d1cf010) },
	{ INT64_C(0x3ff0e3b5b703be63) }, { INT64_C(0x02bf801a5219a86d) }, { -INT64_C(0x02bf801a5219a86d) }, { INT64_C(0x3ff0e3b5b703be63) },
	{ INT64_C(0x2b451a54bae4a0ac) }, { INT64_C(0x2f2800ae9eabc97b) }, { -INT64_C(0x2f2800ae9eabc97b) }, { INT64_C(0x2b451a54bae4a0ac) },
	{ INT64_C(0x3a05a9fd657b248d) }, { INT64_C(0x1b020d6c7f400914) }, { -INT64_C(0x1b020d6c7f400914) }, { INT64_C(0x3a05a9fd657b248d) },
	{ INT64_C(0x15ee27379ea69359) }, { INT64_C(0x3c201994530157e0) }, { -INT64_C(0x3c201994530157e0) }, { INT64_C(0x15ee27379ea69359) },
	{ INT64_C(0x3e2d1ea7ee40b9db) }, { INT64_C(0x0f2b650f080d0da9) }, { -INT64_C(0x0f2b650f080d0da9) }, { INT64_C(0x3e2d1ea7ee40b9db) },
	{ INT64_C(0x213d20e82f8bc101) }, { INT64_C(0x36b113fd242809c4) }, { -INT64_C(0x36b113fd242809c4) }, { INT64_C(0x213d20e82f8bc101) },
	{ INT64_C(0x33a363ebd501aae3) }, { INT64_C(0x25cf01c7d1d42d27) }, { -INT64_C(0x25cf01c7d1d42d27) }, { INT64_C(0x33a363ebd501aae3) },
	{ INT64_C(0x09c76dd866c689dd) }, { INT64_C(0x3f3f9cab5b65907d) }, { -INT64_C(0x3f3f9cab5b65907d) }, { INT64_C(0x09c76dd866c689dd) },
	{ INT64_C(0x3f5d1d1c8d9f75b1) }, { INT64_C(0x09008b6a763de75b) }, { -INT64_C(0x09008b6a763de75b) }, { INT64_C(0x3f5d1d1c8d9f75b1) },
	{ INT64_C(0x2670801a191cad2a) }, { INT64_C(0x332b9e5db01a445e) }, { -INT64_C(0x332b9e5db01a445e) }, { INT64_C(0x2670801a191cad2a) },
	{ INT64_C(0x371871a4ea09d175) }, { INT64_C(0x2090ac4d5c4434dd) }, { -INT64_C(0x2090ac4d5c4434dd) }, { INT64_C(0x371871a4ea09d175) },
	{ INT64_C(0x0fee6e0d6ff6fc5a) }, { INT64_C(0x3dfc4418172bd8e4) }, { -INT64_C(0x3dfc4418172bd8e4) }, { INT64_C(0x0fee6e0d6ff6fc5a) },
	{ INT64_C(0x3c63d5d0e19c4991) }, { INT64_C(0x1530d880af3c2381) }, { -INT64_C(0x1530d880af3c2381) }, { INT64_C(0x3c63d5d0e19c4991) },
	{ INT64_C(0x1bb7cf2304bd0134) }, { INT64_C(0x39afb3131665ebc2) }, { -INT64_C(0x39afb3131665ebc2) }, { INT64_C(0x1bb7cf2304bd0134) },
	{ INT64_C(0x2faf06d9867b6446) }, { INT64_C(0x2ab020712ea26ea3) }, { -INT64_C(0x2ab020712ea26ea3) }, { INT64_C(0x2faf06d9867b6446) },
	{ INT64_C(0x038851a2581afc5a) }, { INT64_C(0x3fe7061f1aaeb79b) }, { -INT64_C(0x3fe7061f1aaeb79b) }, { INT64_C(0x038851a2581afc5a) },
	{ INT64_C(0x3fbaa73fe3e8ab95) }, { INT64_C(0x05e1d61a9756c856) }, { -INT64_C(0x05e1d61a9756c856) }, { INT64_C(0x3fbaa73fe3e8ab95) },
	{ INT64_C(0x28e76a3730e68e39) }, { INT64_C(0x3138fd349ba954ee) }, { -INT64_C(0x3138fd349ba954ee) }, { INT64_C(0x28e76a3730e68e39) },
	{ INT64_C(0x38a0840256d20dd4) }, { INT64_C(0x1dd28f1481cc57f1) }, { -INT64_C(0x1dd28f1481cc57f1) }, { INT64_C(0x38a0840256d20dd4) },
	{ INT64_C(0x12f422daec0386a3) }, { INT64_C(0x3d21086c3befe4e7) }, { -INT64_C(0x3d21086c3befe4e7) }, { INT64_C(0x12f422daec0386a3) },
	{ INT64_C(0x3d5b65d1cf511b37) }, { INT64_C(0x1233bbabc3bb7166) }, { -INT64_C(0x1233bbabc3bb7166) }, { INT64_C(0x3d5b65d1cf511b37) },
	{ INT64_C(0x1e83e0eaf85113d1) }, { INT64_C(0x3841bc7f52e35f26) }, { -INT64_C(0x3841bc7f52e35f26) }, { INT64_C(0x1e83e0eaf85113d1) },
	{ INT64_C(0x31b88a662d319824) }, { INT64_C(0x284bfe2f1cd762be) }, { -INT64_C(0x284bfe2f1cd762be) }, { INT64_C(0x31b88a662d319824) },
	{ INT64_C(0x06a9edc9125700de) }, { INT64_C(0x3fa6f228441708a9) }, { -INT64_C(0x3fa6f228441708a9) }, { INT64_C(0x06a9edc9125700de) },
	{ INT64_C(0x3ed87efbeb776e61) }, { INT64_C(0x0c19b3744e3262dd) }, { -INT64_C(0x0c19b3744e3262dd) }, { INT64_C(0x3ed87efbeb776e61) },
	{ INT64_C(0x23e1e117790c35de) }, { INT64_C(0x34feb0a53fcd3934) }, { -INT64_C(0x34feb0a53fcd3934) }, { INT64_C(0x23e1e117790c35de) },
	{ INT64_C(0x356e64b22d81a8d4) }, { INT64_C(0x233ab413e5736fda) }, { -INT64_C(0x233ab413e5736fda) }, { INT64_C(0x356e64b22d81a8d4) },
	{ INT64_C(0x0cdee5f96e21b333) }, { INT64_C(0x3eb14562f13d0848) }, { -INT64_C(0x3eb14562f13d0848) }, { INT64_C(0x0cdee5f96e21b333) },
	{ INT64_C(0x3b470752cd130f54) }, { INT64_C(0x1820e3b04eaac3f3) }, { -INT64_C(0x1820e3b04eaac3f3) }, { INT64_C(0x3b470752cd130f54) },
	{ INT64_C(0x18daa52ec8a4afd2) }, { INT64_C(0x3afa160571d9f2c0) }, { -INT64_C(0x3afa160571d9f2c0) }, { INT64_C(0x18daa52ec8a4afd2) },
	{ INT64_C(0x2d881ae78304ea25) }, { INT64_C(0x2cf9ef09235e200c) }, { -INT64_C(0x2cf9ef09235e200c) }, { INT64_C(0x2d881ae78304ea25) },
	{ INT64_C(0x006487c3f99c01c4) }, { INT64_C(0x3fffb10b1d15249b) }, { -INT64_C(0x3fffb10b1d15249b) }, { INT64_C(0x006487c3f99c01c4) },
	{ INT64_C(0x3fffec42c43a03a5) }, { INT64_C(0x003243f17d994975) }, { -INT64_C(0x003243f17d994975) }, { INT64_C(0x3fffec42c43a03a5) },
	{ INT64_C(0x2d1da3d54338e27f) }, { INT64_C(0x2d64b9da5fc53753) }, { -INT64_C(0x2d64b9da5fc53753) }, { INT64_C(0x2d1da3d54338e27f) },
	{ INT64_C(0x3b0d89088b489ef4) }, { INT64_C(0x18ac4b86d5ed4446) }, { -INT64_C(0x18ac4b86d5ed4446) }, { INT64_C(0x3b0d89088b489ef4) },
	{ INT64_C(0x184f6aaaf3903f2e) }, { INT64_C(0x3b3401bb167a8c1c) }, { -INT64_C(0x3b3401bb167a8c1c) }, { INT64_C(0x184f6aaaf3903f2e) },
	{ INT64_C(0x3ebb4dda86d0b956) }, { INT64_C(0x0cada4f4db157cf7) }, { -INT64_C(0x0cada4f4db157cf7) }, { INT64_C(0x3ebb4dda86d0b956) },
	{ INT64_C(0x2364a02e26e77d27) }, { INT64_C(0x3552a8f459731c06) }, { -INT64_C(0x3552a8f459731c06) }, { INT64_C(0x2364a02e26e77d27) },
	{ INT64_C(0x351acedca8b5a3f2) }, { INT64_C(0x23b836c9b890e43e) }, { -INT64_C(0x23b836c9b890e43e) }, { INT64_C(0x351acedca8b5a3f2) },
	{ INT64_C(0x0c4b0b93e20c0214) }, { INT64_C(0x3eceeaad1079dc3e) }, { -INT64_C(0x3eceeaad1079dc3e) }, { INT64_C(0x0c4b0b93e20c0214) },
	{ INT64_C(0x3fac1a5b4eb93c13) }, { INT64_C(0x0677edbac92a170f) }, { -INT64_C(0x0677edbac92a170f) }, { INT64_C(0x3fac1a5b4eb93c13) },
	{ INT64_C(0x2872feb65486ff47) }, { INT64_C(0x3198d4ea308ca90f) }, { -INT64_C(0x3198d4ea308ca90f) }, { INT64_C(0x2872feb65486ff47) },
	{ INT64_C(0x3859a29263c7e24b) }, { INT64_C(0x1e57a86d3cd824da) }, { -INT64_C(0x1e57a86d3cd824da) }, { INT64_C(0x3859a29263c7e24b) },
	{ INT64_C(0x1263e6995554ba47) }, { INT64_C(0x3d4d0727ccafffae) }, { -INT64_C(0x3d4d0727ccafffae) }, { INT64_C(0x1263e6995554ba47) },
	{ INT64_C(0x3d2fd86c02d660ae) }, { INT64_C(0x12c41a4e95452067) }, { -INT64_C(0x12c41a4e95452067) }, { INT64_C(0x3d2fd86c02d660ae) },
	{ INT64_C(0x1dfeff66a941dd96) }, { INT64_C(0x3889066283800849) }, { -INT64_C(0x3889066283800849) }, { INT64_C(0x1dfeff66a941dd96) },
	{ INT64_C(0x31590e3dbc3b0f33) }, { INT64_C(0x28c0b4d256654491) }, { -INT64_C(0x28c0b4d256654491) }, { INT64_C(0x31590e3dbc3b0f33) },
	{ INT64_C(0x0613e1c4b04c2822) }, { INT64_C(0x3fb5f4ea28a7180f) }, { -INT64_C(0x3fb5f4ea28a7180f) }, { INT64_C(0x0613e1c4b04c2822) },
	{ INT64_C(0x3fe9b8a9637da51f) }, { INT64_C(0x03562037abf062d5) }, { -INT64_C(0x03562037abf062d5) }, { INT64_C(0x3fe9b8a9637da51f) },
	{ INT64_C(0x2ad586a32ec93d04) }, { INT64_C(0x2f8d7139c5a6658c) }, { -INT64_C(0x2f8d7139c5a6658c) }, { INT64_C(0x2ad586a32ec93d04) },
	{ INT64_C(0x39c5664f3340bfa7) }, { INT64_C(0x1b8a7814fd569367) }, { -INT64_C(0x1b8a7814fd569367) }, { INT64_C(0x39c5664f3340bfa7) },
	{ INT64_C(0x15604012f467b468) }, { INT64_C(0x3c531e887232f43d) }, { -INT64_C(0x3c531e887232f43d) }, { INT64_C(0x15604012f467b468) },
	{ INT64_C(0x3e08b4299ee7172d) }, { INT64_C(0x0fbdba405e9c00cd) }, { -INT64_C(0x0fbdba405e9c00cd) }, { INT64_C(0x3e08b4299ee7172d) },
	{ INT64_C(0x20bbe7d863716dc4) }, { INT64_C(0x36fecd0dcf25d2c4) }, { -INT64_C(0x36fecd0dcf25d2c4) }, { INT64_C(0x20bbe7d863716dc4) },
	{ INT64_C(0x3349bf48560d639f) }, { INT64_C(0x264843d8934c3eb3) }, { -INT64_C(0x264843d8934c3eb3) }, { INT64_C(0x3349bf48560d639f) },
	{ INT64_C(0x09324ca6fe9a04b5) }, { INT64_C(0x3f55f79619fbb70e) }, { -INT64_C(0x3f55f79619fbb70e) }, { INT64_C(0x09324ca6fe9a04b5) },
	{ INT64_C(0x3f473758f42f21cd) }, { INT64_C(0x0995bdfca28b53a5) }, { -INT64_C(0x0995bdfca28b53a5) }, { INT64_C(0x3f473758f42f21cd) },
	{ INT64_C(0x25f7849688202a3b) }, { INT64_C(0x3385a221e0eb84ed) }, { -INT64_C(0x3385a221e0eb84ed) }, { INT64_C(0x25f7849688202a3b) },
	{ INT64_C(0x36cb1e29fb788e2a) }, { INT64_C(0x2112224065611826) }, { -INT64_C(0x2112224065611826) }, { INT64_C(0x36cb1e29fb788e2a) },
	{ INT64_C(0x0f5c35a316f1a3c8) }, { INT64_C(0x3e212179131ebd23) }, { -INT64_C(0x3e212179131ebd23) }, { INT64_C(0x0f5c35a316f1a3c8) },
	{ INT64_C(0x3c31405fb8cdb284) }, { INT64_C(0x15bee78b9db3b61e) }, { -INT64_C(0x15bee78b9db3b61e) }, { INT64_C(0x3c31405fb8cdb284) },
	{ INT64_C(0x1b2f971db319727f) }, { INT64_C(0x39f061d19c8cf542) }, { -INT64_C(0x39f061d19c8cf542) }, { INT64_C(0x1b2f971db319727f) },
	{ INT64_C(0x2f49ee0f7f2fa49e) }, { INT64_C(0x2b2003abee47be6a) }, { -INT64_C(0x2b2003abee47be6a) }, { INT64_C(0x2f49ee0f7f2fa49e) },
	{ INT64_C(0x02f1b754b0e8d0bb) }, { INT64_C(0x3feea7763722c7a1) }, { -INT64_C(0x3feea7763722c7a1) }, { INT64_C(0x02f1b754b0e8d0bb) },
	{ INT64_C(0x3ff9c139c54cf142) }, { INT64_C(0x01c454f4ce53b1c9) }, { -INT64_C(0x01c454f4ce53b1c9) }, { INT64_C(0x3ff9c139c54cf142) },
	{ INT64_C(0x2bfcf97bcad41f02) }, { INT64_C(0x2e7cab1c0f32836d) }, { -INT64_C(0x2e7cab1c0f32836d) }, { INT64_C(0x2bfcf97bcad41f02) },
	{ INT64_C(0x3a6df8f797f7b723) }, { INT64_C(0x1a1d6543b50abfe0) }, { -INT64_C(0x1a1d6543b50abfe0) }, { INT64_C(0x3a6df8f797f7b723) },
	{ INT64_C(0x16d998638a0cb5fe) }, { INT64_C(0x3bc82c1edb1b02ce) }, { -INT64_C(0x3bc82c1edb1b02ce) }, { INT64_C(0x16d998638a0cb5fe) },
	{ INT64_C(0x3e66d0b4755de07b) }, { INT64_C(0x0e36c829aeba6e72) }, { -INT64_C(0x0e36c829aeba6e72) }, { INT64_C(0x3e66d0b4755de07b) },
	{ INT64_C(0x2212e491a152ad43) }, { INT64_C(0x362ce8549945e923) }, { -INT64_C(0x362ce8549945e923) }, { INT64_C(0x2212e491a152ad43) },
	{ INT64_C(0x34364da5814ebc26) }, { INT64_C(0x250317de9a797387) }, { -INT64_C(0x250317de9a797387) }, { INT64_C(0x34364da5814ebc26) },
	{ INT64_C(0x0abf80432a65ef19) }, { INT64_C(0x3f174e6f9696ef13) }, { -INT64_C(0x3f174e6f9696ef13) }, { INT64_C(0x0abf80432a65ef19) },
	{ INT64_C(0x3f7e8e1e151b0f4a) }, { INT64_C(0x08077456b7dc2d96) }, { -INT64_C(0x08077456b7dc2d96) }, { INT64_C(0x3f7e8e1e151b0f4a) },
	{ INT64_C(0x273847c7ac6051ec) }, { INT64_C(0x329321c75894d86a) }, { -INT64_C(0x329321c75894d86a) }, { INT64_C(0x273847c7ac6051ec) },
	{ INT64_C(0x3796a9961a464e01) }, { INT64_C(0x1fb7575c24d2ddd5) }, { -INT64_C(0x1fb7575c24d2ddd5) }, { INT64_C(0x3796a9961a464e01) },
	{ INT64_C(0x10e15b4e1749cda9) }, { INT64_C(0x3dbbd6d40f0ca162) }, { -INT64_C(0x3dbbd6d40f0ca162) }, { INT64_C(0x10e15b4e1749cda9) },
	{ INT64_C(0x3cb53aaa08cfa65f) }, { INT64_C(0x144310dc8936f063) }, { -INT64_C(0x144310dc8936f063) }, { INT64_C(0x3cb53aaa08cfa65f) },
	{ INT64_C(0x1c997fc3865388a7) }, { INT64_C(0x39411e33738891af) }, { -INT64_C(0x39411e33738891af) }, { INT64_C(0x1c997fc3865388a7) },
	{ INT64_C(0x30553827ea8bfda7) }, { INT64_C(0x29f3984b99490ca4) }, { -INT64_C(0x29f3984b99490ca4) }, { INT64_C(0x30553827ea8bfda7) },
	{ INT64_C(0x0483259d3b511320) }, { INT64_C(0x3fd73a4a60821e1a) }, { -INT64_C(0x3fd73a4a60821e1a) }, { INT64_C(0x0483259d3b511320) },
	{ INT64_C(0x3fcfd50a905ac04d) }, { INT64_C(0x04e767c4b168a641) }, { -INT64_C(0x04e767c4b168a641) }, { INT64_C(0x3fcfd50a905ac04d) },
	{ INT64_C(0x29a778dab13efeb7) }, { INT64_C(0x3096e2235f07f34e) }, { -INT64_C(0x3096e2235f07f34e) }, { INT64_C(0x29a778dab13efeb7) },
	{ INT64_C(0x3913eb0e05382616) }, { INT64_C(0x1cf34baee1cd20bc) }, { -INT64_C(0x1cf34baee1cd20bc) }, { INT64_C(0x3913eb0e05382616) },
	{ INT64_C(0x13e39be96ec27160) }, { INT64_C(0x3cd4c38abaa74e5a) }, { -INT64_C(0x3cd4c38abaa74e5a) }, { INT64_C(0x13e39be96ec27160) },
	{ INT64_C(0x3da106bd33201229) }, { INT64_C(0x11423eefc6937849) }, { -INT64_C(0x11423eefc6937849) }, { INT64_C(0x3da106bd33201229) },
	{ INT64_C(0x1f5fdee656cda2e8) }, { INT64_C(0x37c836c222a98101) }, { -INT64_C(0x37c836c222a98101) }, { INT64_C(0x1f5fdee656cda2e8) },
	{ INT64_C(0x3255483f8b5029d3) }, { INT64_C(0x27878893038a2f25) }, { -INT64_C(0x27878893038a2f25) }, { INT64_C(0x3255483f8b5029d3) },
	{ INT64_C(0x07a3adff792a8fae) }, { INT64_C(0x3f8adc76fb35ebbf) }, { -INT64_C(0x3f8adc76fb35ebbf) }, { INT64_C(0x07a3adff792a8fae) },
	{ INT64_C(0x3f061e94818c1831) }, { INT64_C(0x0b228d418ec1869b) }, { -INT64_C(0x0b228d418ec1869b) }, { INT64_C(0x3f061e94818c1831) },
	{ INT64_C(0x24b0e69976e22027) }, { INT64_C(0x34703094b2778b22) }, { -INT64_C(0x34703094b2778b22) }, { INT64_C(0x24b0e69976e22027) },
	{ INT64_C(0x35f71fb13eaf6c04) }, { INT64_C(0x2267d39fdc4a9d98) }, { -INT64_C(0x2267d39fdc4a9d98) }, { INT64_C(0x35f71fb13eaf6c04) },
	{ INT64_C(0x0dd4b19a78aed651) }, { INT64_C(0x3e7cd7783778ca2b) }, { -INT64_C(0x3e7cd7783778ca2b) }, { INT64_C(0x0dd4b19a78aed651) },
	{ INT64_C(0x3ba3fde71522b344) }, { INT64_C(0x173763c9261091d6) }, { -INT64_C(0x173763c9261091d6) }, { INT64_C(0x3ba3fde71522b344) },
	{ INT64_C(0x19c17d440df9f232) }, { INT64_C(0x3a96b63630ea47c1) }, { -INT64_C(0x3a96b63630ea47c1) }, { INT64_C(0x19c17d440df9f232) },
	{ INT64_C(0x2e37592c1c837d92) }, { INT64_C(0x2c45c89fd845fe7e) }, { -INT64_C(0x2c45c89fd845fe7e) }, { INT64_C(0x2e37592c1c837d92) },
	{ INT64_C(0x015fd4d21fab225e) }, { INT64_C(0x3ffc38d0e196eec5) }, { -INT64_C(0x3ffc38d0e196eec5) }, { INT64_C(0x015fd4d21fab225e) },
	{ INT64_C(0x3ffe12878a77a15c) }, { INT64_C(0x00fb514b55ccbe54) }, { -INT64_C(0x00fb514b55ccbe54) }, { INT64_C(0x3ffe12878a77a15c) },
	{ INT64_C(0x2c8e2a86fea6b542) }, { INT64_C(0x2df1953392b6ea7d) }, { -INT64_C(0x2df1953392b6ea7d) }, { INT64_C(0x2c8e2a86fea6b542) },
	{ INT64_C(0x3abee2e51114fd4c) }, { INT64_C(0x196555b7ab948f5f) }, { -INT64_C(0x196555b7ab948f5f) }, { INT64_C(0x3abee2e51114fd4c) },
	{ INT64_C(0x1794f5e613dfae30) }, { INT64_C(0x3b7f3c872aeb34ec) }, { -INT64_C(0x3b7f3c872aeb34ec) }, { INT64_C(0x1794f5e613dfae30) },
	{ INT64_C(0x3e92440d795768e9) }, { INT64_C(0x0d7278eaf9dcd5b5) }, { -INT64_C(0x0d7278eaf9dcd5b5) }, { INT64_C(0x3e92440d795768e9) },
	{ INT64_C(0x22bc6dc9b7c5784f) }, { INT64_C(0x35c0d1e68bd9ddb3) }, { -INT64_C(0x35c0d1e68bd9ddb3) }, { INT64_C(0x22bc6dc9b7c5784f) },
	{ INT64_C(0x34a9922121ea4646) }, { INT64_C(0x245e5acc5827c315) }, { -INT64_C(0x245e5acc5827c315) }, { INT64_C(0x34a9922121ea4646) },
	{ INT64_C(0x0b857ec684627fa5) }, { INT64_C(0x3ef453383464545c) }, { -INT64_C(0x3ef453383464545c) }, { INT64_C(0x0b857ec684627fa5) },
	{ INT64_C(0x3f968e072286a889) }, { INT64_C(0x073fd4cecc1f704c) }, { -INT64_C(0x073fd4cecc1f704c) }, { INT64_C(0x3f968e072286a889) },
	{ INT64_C(0x27d667d57c0d0018) }, { INT64_C(0x3216f286aebdfc2a) }, { -INT64_C(0x3216f286aebdfc2a) }, { INT64_C(0x27d667d57c0d0018) },
	{ INT64_C(0x37f93a4b43628dfd) }, { INT64_C(0x1f081906bff7fdad) }, { -INT64_C(0x1f081906bff7fdad) }, { INT64_C(0x37f93a4b43628dfd) },
	{ INT64_C(0x11a2f7fbe8f24359) }, { INT64_C(0x3d859e9635ed640a) }, { -INT64_C(0x3d859e9635ed640a) }, { INT64_C(0x11a2f7fbe8f24359) },
	{ INT64_C(0x3cf3b6534a2cb3a3) }, { INT64_C(0x1383f5e353b6aa96) }, { -INT64_C(0x1383f5e353b6aa96) }, { INT64_C(0x3cf3b6534a2cb3a3) },
	{ INT64_C(0x1d4cd02ba8609cd9) }, { INT64_C(0x38e62b133d55adcd) }, { -INT64_C(0x38e62b133d55adcd) }, { INT64_C(0x1d4cd02ba8609cd9) },
	{ INT64_C(0x30d8143b35432a0c) }, { INT64_C(0x295af2a2ce45e177) }, { -INT64_C(0x295af2a2ce45e177) }, { INT64_C(0x30d8143b35432a0c) },
	{ INT64_C(0x054b9dd293534287) }, { INT64_C(0x3fc7d257d3b10c36) }, { -INT64_C(0x3fc7d257d3b10c36) }, { INT64_C(0x054b9dd293534287) },
	{ INT64_C(0x3fde020504c3224f) }, { INT64_C(0x041ed853918c18ec) }, { -INT64_C(0x041ed853918c18ec) }, { INT64_C(0x3fde020504c3224f) },
	{ INT64_C(0x2a3f5039b3354bff) }, { INT64_C(0x301316eadca5f4c9) }, { -INT64_C(0x301316eadca5f4c9) }, { INT64_C(0x2a3f5039b3354bff) },
	{ INT64_C(0x396dc41401b53158) }, { INT64_C(0x1c3f6d4726312967) }, { -INT64_C(0x1c3f6d4726312967) }, { INT64_C(0x396dc41401b53158) },
	{ INT64_C(0x14a253d11b82f2ed) }, { INT64_C(0x3c951bff039cbca2) }, { -INT64_C(0x3c951bff039cbca2) }, { INT64_C(0x14a253d11b82f2ed) },
	{ INT64_C(0x3dd60e98a14a88ed) }, { INT64_C(0x10804e05eb661e55) }, { -INT64_C(0x10804e05eb661e55) }, { INT64_C(0x3dd60e98a14a88ed) },
	{ INT64_C(0x200e81905705c149) }, { INT64_C(0x376493416d8819e3) }, { -INT64_C(0x376493416d8819e3) }, { INT64_C(0x200e81905705c149) },
	{ INT64_C(0x32d07e857affc1de) }, { INT64_C(0x26e8a63702ff1aa2) }, { -INT64_C(0x26e8a63702ff1aa2) }, { INT64_C(0x32d07e857affc1de) },
	{ INT64_C(0x086b26de5933c2e9) }, { INT64_C(0x3f71a31acd5b6d8c) }, { -INT64_C(0x3f71a31acd5b6d8c) }, { INT64_C(0x086b26de5933c2e9) },
	{ INT64_C(0x3f27e29f0b580f18) }, { INT64_C(0x0a5c58bfbcfd4437) }, { -INT64_C(0x0a5c58bfbcfd4437) }, { INT64_C(0x3f27e29f0b580f18) },
	{ INT64_C(0x2554edd0f5d6b010) }, { INT64_C(0x33fbe9e26293bc8f) }, { -INT64_C(0x33fbe9e26293bc8f) }, { INT64_C(0x2554edd0f5d6b010) },
	{ INT64_C(0x36622b4be6f7e6ef) }, { INT64_C(0x21bda17097896b3d) }, { -INT64_C(0x21bda17097896b3d) }, { INT64_C(0x36622b4be6f7e6ef) },
	{ INT64_C(0x0e98bba6965ef726) }, { INT64_C(0x3e502ff88c139b11) }, { -INT64_C(0x3e502ff88c139b11) }, { INT64_C(0x0e98bba6965ef726) },
	{ INT64_C(0x3bebc6d5374b37c4) }, { INT64_C(0x167b949cad63ca97) }, { -INT64_C(0x167b949cad63ca97) }, { INT64_C(0x3bebc6d5374b37c4) },
	{ INT64_C(0x1a790cd3dbf31ad0) }, { INT64_C(0x3a44ab8dcb49c117) }, { -INT64_C(0x3a44ab8dcb49c117) }, { INT64_C(0x1a790cd3dbf31ad0) },
	{ INT64_C(0x2ec18a58608ec777) }, { INT64_C(0x2bb3bdce7c68b7c4) }, { -INT64_C(0x2bb3bdce7c68b7c4) }, { INT64_C(0x2ec18a58608ec777) },
	{ INT64_C(0x0228d0bb68582fd7) }, { INT64_C(0x3ff6abc84bfb5cb7) }, { -INT64_C(0x3ff6abc84bfb5cb7) }, { INT64_C(0x0228d0bb68582fd7) },
	{ INT64_C(0x3ff2f8841180282a) }, { INT64_C(0x028d472dff0c2f22) }, { -INT64_C(0x028d472dff0c2f22) }, { INT64_C(0x3ff2f8841180282a) },
	{ INT64_C(0x2b6a164c9ee89054) }, { INT64_C(0x2f05f6372166b561) }, { -INT64_C(0x2f05f6372166b561) }, { INT64_C(0x2b6a164c9ee89054) },
	{ INT64_C(0x3a1ace5eb3a5711c) }, { INT64_C(0x1ad473125cdc08ed) }, { -INT64_C(0x1ad473125cdc08ed) }, { INT64_C(0x3a1ace5eb3a5711c) },
	{ INT64_C(0x161d595c88c2027e) }, { INT64_C(0x3c0ecdb2501ea8fa) }, { -INT64_C(0x3c0ecdb2501ea8fa) }, { INT64_C(0x161d595c88c2027e) },
	{ INT64_C(0x3e38f57c508e10a8) }, { INT64_C(0x0efa8b1f8084cce0) }, { -INT64_C(0x0efa8b1f8084cce0) }, { INT64_C(0x3e38f57c508e10a8) },
	{ INT64_C(0x21680b0f1f0bd729) }, { INT64_C(0x3696e813bcf24a7d) }, { -INT64_C(0x3696e813bcf24a7d) }, { INT64_C(0x21680b0f1f0bd729) },
	{ INT64_C(0x33c105db6848e42b) }, { INT64_C(0x25a667a69d376f79) }, { -INT64_C(0x25a667a69d376f79) }, { INT64_C(0x33c105db6848e42b) },
	{ INT64_C(0x09f917abeda4498e) }, { INT64_C(0x3f37daf9f7bc5193) }, { -INT64_C(0x3f37daf9f7bc5193) }, { INT64_C(0x09f917abeda4498e) },
	{ INT64_C(0x3f641b8d03aa9900) }, { INT64_C(0x08cec4a05f12739f) }, { -INT64_C(0x08cec4a05f12739f) }, { INT64_C(0x3f641b8d03aa9900) },
	{ INT64_C(0x2698a4a5829bc2ac) }, { INT64_C(0x330d5de28aeb2728) }, { -INT64_C(0x330d5de28aeb2728) }, { INT64_C(0x2698a4a5829bc2ac) },
	{ INT64_C(0x3731f43fb22abcab) }, { INT64_C(0x20655cabdb7b2b21) }, { -INT64_C(0x20655cabdb7b2b21) }, { INT64_C(0x3731f43fb22abcab) },
	{ INT64_C(0x101f1806b9fdd1af) }, { INT64_C(0x3defadca39477f2e) }, { -INT64_C(0x3defadca39477f2e) }, { INT64_C(0x101f1806b9fdd1af) },
	{ INT64_C(0x3c7467d8eb9d0a29) }, { INT64_C(0x150163dc19704785) }, { -INT64_C(0x150163dc19704785) }, { INT64_C(0x3c7467d8eb9d0a29) },
	{ INT64_C(0x1be51517ffc0d94b) }, { INT64_C(0x3999dc4185bd3f16) }, { -INT64_C(0x3999dc4185bd3f16) }, { INT64_C(0x1be51517ffc0d94b) },
	{ INT64_C(0x2fd07f0f606d0c37) }, { INT64_C(0x2a8a9fea2b3bf759) }, { -INT64_C(0x2a8a9fea2b3bf759) }, { INT64_C(0x2fd07f0f606d0c37) },
	{ INT64_C(0x03ba80df3011d921) }, { INT64_C(0x3fe42c29c263d911) }, { -INT64_C(0x3fe42c29c263d911) }, { INT64_C(0x03ba80df3011d921) },
	{ INT64_C(0x3fbf3245ee660e69) }, { INT64_C(0x05afc6cf9e6c4a2f) }, { -INT64_C(0x05afc6cf9e6c4a2f) }, { INT64_C(0x3fbf3245ee660e69) },
	{ INT64_C(0x290e0660c123fdd2) }, { INT64_C(0x3118cdce903741a9) }, { -INT64_C(0x3118cdce903741a9) }, { INT64_C(0x290e0660c123fdd2) },
	{ INT64_C(0x38b7deb3fdf0b3e1) }, { INT64_C(0x1da60c5cfa10d8de) }, { -INT64_C(0x1da60c5cfa10d8de) }, { INT64_C(0x38b7deb3fdf0b3e1) },
	{ INT64_C(0x13241fb638baaf08) }, { INT64_C(0x3d1212b75ac04953) }, { -INT64_C(0x3d1212b75ac04953) }, { INT64_C(0x13241fb638baaf08) },
	{ INT64_C(0x3d699ea2b7102233) }, { INT64_C(0x12038583d727bdc7) }, { -INT64_C(0x12038583d727bdc7) }, { INT64_C(0x3d699ea2b7102233) },
	{ INT64_C(0x1eb00695f2562055) }, { INT64_C(0x3829b3b88cbcaee7) }, { -INT64_C(0x3829b3b88cbcaee7) }, { INT64_C(0x1eb00695f2562055) },
	{ INT64_C(0x31d8213690d8918d) }, { INT64_C(0x2824e4cc7a21188e) }, { -INT64_C(0x2824e4cc7a21188e) }, { INT64_C(0x31d8213690d8918d) },
	{ INT64_C(0x06dbe9bb0e3d931e) }, { INT64_C(0x3fa1a2b1b0c10ecc) }, { -INT64_C(0x3fa1a2b1b0c10ecc) }, { INT64_C(0x06dbe9bb0e3d931e) },
	{ INT64_C(0x3ee1ec8696fd6e46) }, { INT64_C(0x0be853dde9658dc6) }, { -INT64_C(0x0be853dde9658dc6) }, { INT64_C(0x3ee1ec8696fd6e46) },
	{ INT64_C(0x240b7542eac1ac6e) }, { INT64_C(0x34e271bd3ac1e907) }, { -INT64_C(0x34e271bd3ac1e907) }, { INT64_C(0x240b7542eac1ac6e) },
	{ INT64_C(0x3589ff7a7df58fa0) }, { INT64_C(0x2310b23e748dc991) }, { -INT64_C(0x2310b23e748dc991) }, { INT64_C(0x3589ff7a7df58fa0) },
	{ INT64_C(0x0d101f0d8c18ed1c) }, { INT64_C(0x3ea7163f5e5a0df5) }, { -INT64_C(0x3ea7163f5e5a0df5) }, { INT64_C(0x0d101f0d8c18ed1c) },
	{ INT64_C(0x3b59e859cd157180) }, { INT64_C(0x17f24dd37341e425) }, { -INT64_C(0x17f24dd37341e425) }, { INT64_C(0x3b59e859cd157180) },
	{ INT64_C(0x1908ef81ef7bd153) }, { INT64_C(0x3ae67ea1181bfd80) }, { -INT64_C(0x3ae67ea1181bfd80) }, { INT64_C(0x1908ef81ef7bd153) },
	{ INT64_C(0x2dab5fde955f993d) }, { INT64_C(0x2cd61e7ea5670c8f) }, { -INT64_C(0x2cd61e7ea5670c8f) }, { INT64_C(0x2dab5fde955f993d) },
	{ INT64_C(0x0096cb587284b817) }, { INT64_C(0x3fff4e592f189d6f) }, { -INT64_C(0x3fff4e592f189d6f) }, { INT64_C(0x0096cb587284b817) },
	{ INT64_C(0x3fff4e592f189d6f) }, { INT64_C(0x0096cb587284b817) }, { -INT64_C(0x0096cb587284b817) }, { INT64_C(0x3fff4e592f189d6f) },
	{ INT64_C(0x2cd61e7ea5670c8f) }, { INT64_C(0x2dab5fde955f993d) }, { -INT64_C(0x2dab5fde955f993d) }, { INT64_C(0x2cd61e7ea5670c8f) },
	{ INT64_C(0x3ae67ea1181bfd80) }, { INT64_C(0x1908ef81ef7bd153) }, { -INT64_C(0x1908ef81ef7bd153) }, { INT64_C(0x3ae67ea1181bfd80) },
	{ INT64_C(0x17f24dd37341e425) }, { INT64_C(0x3b59e859cd157180) }, { -INT64_C(0x3b59e859cd157180) }, { INT64_C(0x17f24dd37341e425) },
	{ INT64_C(0x3ea7163f5e5a0df5) }, { INT64_C(0x0d101f0d8c18ed1c) }, { -INT64_C(0x0d101f0d8c18ed1c) }, { INT64_C(0x3ea7163f5e5a0df5) },
	{ INT64_C(0x2310b23e748dc991) }, { INT64_C(0x3589ff7a7df58fa0) }, { -INT64_C(0x3589ff7a7df58fa0) }, { INT64_C(0x2310b23e748dc991) },
	{ INT64_C(0x34e271bd3ac1e907) }, { INT64_C(0x240b7542eac1ac6e) }, { -INT64_C(0x240b7542eac1ac6e) }, { INT64_C(0x34e271bd3ac1e907) },
	{ INT64_C(0x0be853dde9658dc6) }, { INT64_C(0x3ee1ec8696fd6e46) }, { -INT64_C(0x3ee1ec8696fd6e46) }, { INT64_C(0x0be853dde9658dc6) },
	{ INT64_C(0x3fa1a2b1b0c10ecc) }, { INT64_C(0x06dbe9bb0e3d931e) }, { -INT64_C(0x06dbe9bb0e3d931e) }, { INT64_C(0x3fa1a2b1b0c10ecc) },
	{ INT64_C(0x2824e4cc7a21188e) }, { INT64_C(0x31d8213690d8918d) }, { -INT64_C(0x31d8213690d8918d) }, { INT64_C(0x2824e4cc7a21188e) },
	{ INT64_C(0x3829b3b88cbcaee7) }, { INT64_C(0x1eb00695f2562055) }, { -INT64_C(0x1eb00695f2562055) }, { INT64_C(0x3829b3b88cbcaee7) },
	{ INT64_C(0x12038583d727bdc7) }, { INT64_C(0x3d699ea2b7102233) }, { -INT64_C(0x3d699ea2b7102233) }, { INT64_C(0x12038583d727bdc7) },
	{ INT64_C(0x3d1212b75ac04953) }, { INT64_C(0x13241fb638baaf08) }, { -INT64_C(0x13241fb638baaf08) }, { INT64_C(0x3d1212b75ac04953) },
	{ INT64_C(0x1da60c5cfa10d8de) }, { INT64_C(0x38b7deb3fdf0b3e1) }, { -INT64_C(0x38b7deb3fdf0b3e1) }, { INT64_C(0x1da60c5cfa10d8de) },
	{ INT64_C(0x3118cdce903741a9) }, { INT64_C(0x290e0660c123fdd2) }, { -INT64_C(0x290e0660c123fdd2) }, { INT64_C(0x3118cdce903741a9) },
	{ INT64_C(0x05afc6cf9e6c4a2f) }, { INT64_C(0x3fbf3245ee660e69) }, { -INT64_C(0x3fbf3245ee660e69) }, { INT64_C(0x05afc6cf9e6c4a2f) },
	{ INT64_C(0x3fe42c29c263d911) }, { INT64_C(0x03ba80df3011d921) }, { -INT64_C(0x03ba80df3011d921) }, { INT64_C(0x3fe42c29c263d911) },
	{ INT64_C(0x2a8a9fea2b3bf759) }, { INT64_C(0x2fd07f0f606d0c37) }, { -INT64_C(0x2fd07f0f606d0c37) }, { INT64_C(0x2a8a9fea2b3bf759) },
	{ INT64_C(0x3999dc4185bd3f16) }, { INT64_C(0x1be51517ffc0d94b) }, { -INT64_C(0x1be51517ffc0d94b) }, { INT64_C(0x3999dc4185bd3f16) },
	{ INT64_C(0x150163dc19704785) }, { INT64_C(0x3c7467d8eb9d0a29) }, { -INT64_C(0x3c7467d8eb9d0a29) }, { INT64_C(0x150163dc19704785) },
	{ INT64_C(0x3defadca39477f2e) }, { INT64_C(0x101f1806b9fdd1af) }, { -INT64_C(0x101f1806b9fdd1af) }, { INT64_C(0x3defadca39477f2e) },
	{ INT64_C(0x20655cabdb7b2b21) }, { INT64_C(0x3731f43fb22abcab) }, { -INT64_C(0x3731f43fb22abcab) }, { INT64_C(0x20655cabdb7b2b21) },
	{ INT64_C(0x330d5de28aeb2728) }, { INT64_C(0x2698a4a5829bc2ac) }, { -INT64_C(0x2698a4a5829bc2ac) }, { INT64_C(0x330d5de28aeb2728) },
	{ INT64_C(0x08cec4a05f12739f) }, { INT64_C(0x3f641b8d03aa9900) }, { -INT64_C(0x3f641b8d03aa9900) }, { INT64_C(0x08cec4a05f12739f) },
	{ INT64_C(0x3f37daf9f7bc5193) }, { INT64_C(0x09f917abeda4498e) }, { -INT64_C(0x09f917abeda4498e) }, { INT64_C(0x3f37daf9f7bc5193) },
	{ INT64_C(0x25a667a69d376f79) }, { INT64_C(0x33c105db6848e42b) }, { -INT64_C(0x33c105db6848e42b) }, { INT64_C(0x25a667a69d376f79) },
	{ INT64_C(0x3696e813bcf24a7d) }, { INT64_C(0x21680b0f1f0bd729) }, { -INT64_C(0x21680b0f1f0bd729) }, { INT64_C(0x3696e813bcf24a7d) },
	{ INT64_C(0x0efa8b1f8084cce0) }, { INT64_C(0x3e38f57c508e10a8) }, { -INT64_C(0x3e38f57c508e10a8) }, { INT64_C(0x0efa8b1f8084cce0) },
	{ INT64_C(0x3c0ecdb2501ea8fa) }, { INT64_C(0x161d595c88c2027e) }, { -INT64_C(0x161d595c88c2027e) }, { INT64_C(0x3c0ecdb2501ea8fa) },
	{ INT64_C(0x1ad473125cdc08ed) }, { INT64_C(0x3a1ace5eb3a5711c) }, { -INT64_C(0x3a1ace5eb3a5711c) }, { INT64_C(0x1ad473125cdc08ed) },
	{ INT64_C(0x2f05f6372166b561) }, { INT64_C(0x2b6a164c9ee89054) }, { -INT64_C(0x2b6a164c9ee89054) }, { INT64_C(0x2f05f6372166b561) },
	{ INT64_C(0x028d472dff0c2f22) }, { INT64_C(0x3ff2f8841180282a) }, { -INT64_C(0x3ff2f8841180282a) }, { INT64_C(0x028d472dff0c2f22) },
	{ INT64_C(0x3ff6abc84bfb5cb7) }, { INT64_C(0x0228d0bb68582fd7) }, { -INT64_C(0x0228d0bb68582fd7) }, { INT64_C(0x3ff6abc84bfb5cb7) },
	{ INT64_C(0x2bb3bdce7c68b7c4) }, { INT64_C(0x2ec18a58608ec777) }, { -INT64_C(0x2ec18a58608ec777) }, { INT64_C(0x2bb3bdce7c68b7c4) },
	{ INT64_C(0x3a44ab8dcb49c117) }, { INT64_C(0x1a790cd3dbf31ad0) }, { -INT64_C(0x1a790cd3dbf31ad0) }, { INT64_C(0x3a44ab8dcb49c117) },
	{ INT64_C(0x167b949cad63ca97) }, { INT64_C(0x3bebc6d5374b37c4) }, { -INT64_C(0x3bebc6d5374b37c4) }, { INT64_C(0x167b949cad63ca97) },
	{ INT64_C(0x3e502ff88c139b11) }, { INT64_C(0x0e98bba6965ef726) }, { -INT64_C(0x0e98bba6965ef726) }, { INT64_C(0x3e502ff88c139b11) },
	{ INT64_C(0x21bda17097896b3d) }, { INT64_C(0x36622b4be6f7e6ef) }, { -INT64_C(0x36622b4be6f7e6ef) }, { INT64_C(0x21bda17097896b3d) },
	{ INT64_C(0x33fbe9e26293bc8f) }, { INT64_C(0x2554edd0f5d6b010) }, { -INT64_C(0x2554edd0f5d6b010) }, { INT64_C(0x33fbe9e26293bc8f) },
	{ INT64_C(0x0a5c58bfbcfd4437) }, { INT64_C(0x3f27e29f0b580f18) }, { -INT64_C(0x3f27e29f0b580f18) }, { INT64_C(0x0a5c58bfbcfd4437) },
	{ INT64_C(0x3f71a31acd5b6d8c) }, { INT64_C(0x086b26de5933c2e9) }, { -INT64_C(0x086b26de5933c2e9) }, { INT64_C(0x3f71a31acd5b6d8c) },
	{ INT64_C(0x26e8a63702ff1aa2) }, { INT64_C(0x32d07e857affc1de) }, { -INT64_C(0x32d07e857affc1de) }, { INT64_C(0x26e8a63702ff1aa2) },
	{ INT64_C(0x376493416d8819e3) }, { INT64_C(0x200e81905705c149) }, { -INT64_C(0x200e81905705c149) }, { INT64_C(0x376493416d8819e3) },
	{ INT64_C(0x10804e05eb661e55) }, { INT64_C(0x3dd60e98a14a88ed) }, { -INT64_C(0x3dd60e98a14a88ed) }, { INT64_C(0x10804e05eb661e55) },
	{ INT64_C(0x3c951bff039cbca2) }, { INT64_C(0x14a253d11b82f2ed) }, { -INT64_C(0x14a253d11b82f2ed) }, { INT64_C(0x3c951bff039cbca2) },
	{ INT64_C(0x1c3f6d4726312967) }, { INT64_C(0x396dc41401b53158) }, { -INT64_C(0x396dc41401b53158) }, { INT64_C(0x1c3f6d4726312967) },
	{ INT64_C(0x301316eadca5f4c9) }, { INT64_C(0x2a3f5039b3354bff) }, { -INT64_C(0x2a3f5039b3354bff) }, { INT64_C(0x301316eadca5f4c9) },
	{ INT64_C(0x041ed853918c18ec) }, { INT64_C(0x3fde020504c3224f) }, { -INT64_C(0x3fde020504c3224f) }, { INT64_C(0x041ed853918c18ec) },
	{ INT64_C(0x3fc7d257d3b10c36) }, { INT64_C(0x054b9dd293534287) }, { -INT64_C(0x054b9dd293534287) }, { INT64_C(0x3fc7d257d3b10c36) },
	{ INT64_C(0x295af2a2ce45e177) }, { INT64_C(0x30d8143b35432a0c) }, { -INT64_C(0x30d8143b35432a0c) }, { INT64_C(0x295af2a2ce45e177) },
	{ INT64_C(0x38e62b133d55adcd) }, { INT64_C(0x1d4cd02ba8609cd9) }, { -INT64_C(0x1d4cd02ba8609cd9) }, { INT64_C(0x38e62b133d55adcd) },
	{ INT64_C(0x1383f5e353b6aa96) }, { INT64_C(0x3cf3b6534a2cb3a3) }, { -INT64_C(0x3cf3b6534a2cb3a3) }, { INT64_C(0x1383f5e353b6aa96) },
	{ INT64_C(0x3d859e9635ed640a) }, { INT64_C(0x11a2f7fbe8f24359) }, { -INT64_C(0x11a2f7fbe8f24359) }, { INT64_C(0x3d859e9635ed640a) },
	{ INT64_C(0x1f081906bff7fdad) }, { INT64_C(0x37f93a4b43628dfd) }, { -INT64_C(0x37f93a4b43628dfd) }, { INT64_C(0x1f081906bff7fdad) },
	{ INT64_C(0x3216f286aebdfc2a) }, { INT64_C(0x27d667d57c0d0018) }, { -INT64_C(0x27d667d57c0d0018) }, { INT64_C(0x3216f286aebdfc2a) },
	{ INT64_C(0x073fd4cecc1f704c) }, { INT64_C(0x3f968e072286a889) }, { -INT64_C(0x3f968e072286a889) }, { INT64_C(0x073fd4cecc1f704c) },
	{ INT64_C(0x3ef453383464545c) }, { INT64_C(0x0b857ec684627fa5) }, { -INT64_C(0x0b857ec684627fa5) }, { INT64_C(0x3ef453383464545c) },
	{ INT64_C(0x245e5acc5827c315) }, { INT64_C(0x34a9922121ea4646) }, { -INT64_C(0x34a9922121ea4646) }, { INT64_C(0x245e5acc5827c315) },
	{ INT64_C(0x35c0d1e68bd9ddb3) }, { INT64_C(0x22bc6dc9b7c5784f) }, { -INT64_C(0x22bc6dc9b7c5784f) }, { INT64_C(0x35c0d1e68bd9ddb3) },
	{ INT64_C(0x0d7278eaf9dcd5b5) }, { INT64_C(0x3e92440d795768e9) }, { -INT64_C(0x3e92440d795768e9) }, { INT64_C(0x0d7278eaf9dcd5b5) },
	{ INT64_C(0x3b7f3c872aeb34ec) }, { INT64_C(0x1794f5e613dfae30) }, { -INT64_C(0x1794f5e613dfae30) }, { INT64_C(0x3b7f3c872aeb34ec) },
	{ INT64_C(0x196555b7ab948f5f) }, { INT64_C(0x3abee2e51114fd4c) }, { -INT64_C(0x3abee2e51114fd4c) }, { INT64_C(0x196555b7ab948f5f) },
	{ INT64_C(0x2df1953392b6ea7d) }, { INT64_C(0x2c8e2a86fea6b542) }, { -INT64_C(0x2c8e2a86fea6b542) }, { INT64_C(0x2df1953392b6ea7d) },
	{ INT64_C(0x00fb514b55ccbe54) }, { INT64_C(0x3ffe12878a77a15c) }, { -INT64_C(0x3ffe12878a77a15c) }, { INT64_C(0x00fb514b55ccbe54) },
	{ INT64_C(0x3ffc38d0e196eec5) }, { INT64_C(0x015fd4d21fab225e) }, { -INT64_C(0x015fd4d21fab225e) }, { INT64_C(0x3ffc38d0e196eec5) },
	{ INT64_C(0x2c45c89fd845fe7e) }, { INT64_C(0x2e37592c1c837d92) }, { -INT64_C(0x2e37592c1c837d92) }, { INT64_C(0x2c45c89fd845fe7e) },
	{ INT64_C(0x3a96b63630ea47c1) }, { INT64_C(0x19c17d440df9f232) }, { -INT64_C(0x19c17d440df9f232) }, { INT64_C(0x3a96b63630ea47c1) },
	{ INT64_C(0x173763c9261091d6) }, { INT64_C(0x3ba3fde71522b344) }, { -INT64_C(0x3ba3fde71522b344) }, { INT64_C(0x173763c9261091d6) },
	{ INT64_C(0x3e7cd7783778ca2b) }, { INT64_C(0x0dd4b19a78aed651) }, { -INT64_C(0x0dd4b19a78aed651) }, { INT64_C(0x3e7cd7783778ca2b) },
	{ INT64_C(0x2267d39fdc4a9d98) }, { INT64_C(0x35f71fb13eaf6c04) }, { -INT64_C(0x35f71fb13eaf6c04) }, { INT64_C(0x2267d39fdc4a9d98) },
	{ INT64_C(0x34703094b2778b22) }, { INT64_C(0x24b0e69976e22027) }, { -INT64_C(0x24b0e69976e22027) }, { INT64_C(0x34703094b2778b22) },
	{ INT64_C(0x0b228d418ec1869b) }, { INT64_C(0x3f061e94818c1831) }, { -INT64_C(0x3f061e94818c1831) }, { INT64_C(0x0b228d418ec1869b) },
	{ INT64_C(0x3f8adc76fb35ebbf) }, { INT64_C(0x07a3adff792a8fae) }, { -INT64_C(0x07a3adff792a8fae) }, { INT64_C(0x3f8adc76fb35ebbf) },
	{ INT64_C(0x27878893038a2f25) }, { INT64_C(0x3255483f8b5029d3) }, { -INT64_C(0x3255483f8b5029d3) }, { INT64_C(0x27878893038a2f25) },
	{ INT64_C(0x37c836c222a98101) }, { INT64_C(0x1f5fdee656cda2e8) }, { -INT64_C(0x1f5fdee656cda2e8) }, { INT64_C(0x37c836c222a98101) },
	{ INT64_C(0x11423eefc6937849) }, { INT64_C(0x3da106bd33201229) }, { -INT64_C(0x3da106bd33201229) }, { INT64_C(0x11423eefc6937849) },
	{ INT64_C(0x3cd4c38abaa74e5a) }, { INT64_C(0x13e39be96ec27160) }, { -INT64_C(0x13e39be96ec27160) }, { INT64_C(0x3cd4c38abaa74e5a) },
	{ INT64_C(0x1cf34baee1cd20bc) }, { INT64_C(0x3913eb0e05382616) }, { -INT64_C(0x3913eb0e05382616) }, { INT64_C(0x1cf34baee1cd20bc) },
	{ INT64_C(0x3096e2235f07f34e) }, { INT64_C(0x29a778dab13efeb7) }, { -INT64_C(0x29a778dab13efeb7) }, { INT64_C(0x3096e2235f07f34e) },
	{ INT64_C(0x04e767c4b168a641) }, { INT64_C(0x3fcfd50a905ac04d) }, { -INT64_C(0x3fcfd50a905ac04d) }, { INT64_C(0x04e767c4b168a641) },
	{ INT64_C(0x3fd73a4a60821e1a) }, { INT64_C(0x0483259d3b511320) }, { -INT64_C(0x0483259d3b511320) }, { INT64_C(0x3fd73a4a60821e1a) },
	{ INT64_C(0x29f3984b99490ca4) }, { INT64_C(0x30553827ea8bfda7) }, { -INT64_C(0x30553827ea8bfda7) }, { INT64_C(0x29f3984b99490ca4) },
	{ INT64_C(0x39411e33738891af) }, { INT64_C(0x1c997fc3865388a7) }, { -INT64_C(0x1c997fc3865388a7) }, { INT64_C(0x39411e33738891af) },
	{ INT64_C(0x144310dc8936f063) }, { INT64_C(0x3cb53aaa08cfa65f) }, { -INT64_C(0x3cb53aaa08cfa65f) }, { INT64_C(0x144310dc8936f063) },
	{ INT64_C(0x3dbbd6d40f0ca162) }, { INT64_C(0x10e15b4e1749cda9) }, { -INT64_C(0x10e15b4e1749cda9) }, { INT64_C(0x3dbbd6d40f0ca162) },
	{ INT64_C(0x1fb7575c24d2ddd5) }, { INT64_C(0x3796a9961a464e01) }, { -INT64_C(0x3796a9961a464e01) }, { INT64_C(0x1fb7575c24d2ddd5) },
	{ INT64_C(0x329321c75894d86a) }, { INT64_C(0x273847c7ac6051ec) }, { -INT64_C(0x273847c7ac6051ec) }, { INT64_C(0x329321c75894d86a) },
	{ INT64_C(0x08077456b7dc2d96) }, { INT64_C(0x3f7e8e1e151b0f4a) }, { -INT64_C(0x3f7e8e1e151b0f4a) }, { INT64_C(0x08077456b7dc2d96) },
	{ INT64_C(0x3f174e6f9696ef13) }, { INT64_C(0x0abf80432a65ef19) }, { -INT64_C(0x0abf80432a65ef19) }, { INT64_C(0x3f174e6f9696ef13) },
	{ INT64_C(0x250317de9a797387) }, { INT64_C(0x34364da5814ebc26) }, { -INT64_C(0x34364da5814ebc26) }, { INT64_C(0x250317de9a797387) },
	{ INT64_C(0x362ce8549945e923) }, { INT64_C(0x2212e491a152ad43) }, { -INT64_C(0x2212e491a152ad43) }, { INT64_C(0x362ce8549945e923) },
	{ INT64_C(0x0e36c829aeba6e72) }, { INT64_C(0x3e66d0b4755de07b) }, { -INT64_C(0x3e66d0b4755de07b) }, { INT64_C(0x0e36c829aeba6e72) },
	{ INT64_C(0x3bc82c1edb1b02ce) }, { INT64_C(0x16d998638a0cb5fe) }, { -INT64_C(0x16d998638a0cb5fe) }, { INT64_C(0x3bc82c1edb1b02ce) },
	{ INT64_C(0x1a1d6543b50abfe0) }, { INT64_C(0x3a6df8f797f7b723) }, { -INT64_C(0x3a6df8f797f7b723) }, { INT64_C(0x1a1d6543b50abfe0) },
	{ INT64_C(0x2e7cab1c0f32836d) }, { INT64_C(0x2bfcf97bcad41f02) }, { -INT64_C(0x2bfcf97bcad41f02) }, { INT64_C(0x2e7cab1c0f32836d) },
	{ INT64_C(0x01c454f4ce53b1c9) }, { INT64_C(0x3ff9c139c54cf142) }, { -INT64_C(0x3ff9c139c54cf142) }, { INT64_C(0x01c454f4ce53b1c9) },
	{ INT64_C(0x3feea7763722c7a1) }, { INT64_C(0x02f1b754b0e8d0bb) }, { -INT64_C(0x02f1b754b0e8d0bb) }, { INT64_C(0x3feea7763722c7a1) },
	{ INT64_C(0x2b2003abee47be6a) }, { INT64_C(0x2f49ee0f7f2fa49e) }, { -INT64_C(0x2f49ee0f7f2fa49e) }, { INT64_C(0x2b2003abee47be6a) },
	{ INT64_C(0x39f061d19c8cf542) }, { INT64_C(0x1b2f971db319727f) }, { -INT64_C(0x1b2f971db319727f) }, { INT64_C(0x39f061d19c8cf542) },
	{ INT64_C(0x15bee78b9db3b61e) }, { INT64_C(0x3c31405fb8cdb284) }, { -INT64_C(0x3c31405fb8cdb284) }, { INT64_C(0x15bee78b9db3b61e) },
	{ INT64_C(0x3e212179131ebd23) }, { INT64_C(0x0f5c35a316f1a3c8) }, { -INT64_C(0x0f5c35a316f1a3c8) }, { INT64_C(0x3e212179131ebd23) },
	{ INT64_C(0x2112224065611826) }, { INT64_C(0x36cb1e29fb788e2a) }, { -INT64_C(0x36cb1e29fb788e2a) }, { INT64_C(0x2112224065611826) },
	{ INT64_C(0x3385a221e0eb84ed) }, { INT64_C(0x25f7849688202a3b) }, { -INT64_C(0x25f7849688202a3b) }, { INT64_C(0x3385a221e0eb84ed) },
	{ INT64_C(0x0995bdfca28b53a5) }, { INT64_C(0x3f473758f42f21cd) }, { -INT64_C(0x3f473758f42f21cd) }, { INT64_C(0x0995bdfca28b53a5) },
	{ INT64_C(0x3f55f79619fbb70e) }, { INT64_C(0x09324ca6fe9a04b5) }, { -INT64_C(0x09324ca6fe9a04b5) }, { INT64_C(0x3f55f79619fbb70e) },
	{ INT64_C(0x264843d8934c3eb3) }, { INT64_C(0x3349bf48560d639f) }, { -INT64_C(0x3349bf48560d639f) }, { INT64_C(0x264843d8934c3eb3) },
	{ INT64_C(0x36fecd0dcf25d2c4) }, { INT64_C(0x20bbe7d863716dc4) }, { -INT64_C(0x20bbe7d863716dc4) }, { INT64_C(0x36fecd0dcf25d2c4) },
	{ INT64_C(0x0fbdba405e9c00cd) }, { INT64_C(0x3e08b4299ee7172d) }, { -INT64_C(0x3e08b4299ee7172d) }, { INT64_C(0x0fbdba405e9c00cd) },
	{ INT64_C(0x3c531e887232f43d) }, { INT64_C(0x15604012f467b468) }, { -INT64_C(0x15604012f467b468) }, { INT64_C(0x3c531e887232f43d) },
	{ INT64_C(0x1b8a7814fd569367) }, { INT64_C(0x39c5664f3340bfa7) }, { -INT64_C(0x39c5664f3340bfa7) }, { INT64_C(0x1b8a7814fd569367) },
	{ INT64_C(0x2f8d7139c5a6658c) }, { INT64_C(0x2ad586a32ec93d04) }, { -INT64_C(0x2ad586a32ec93d04) }, { INT64_C(0x2f8d7139c5a6658c) },
	{ INT64_C(0x03562037abf062d5) }, { INT64_C(0x3fe9b8a9637da51f) }, { -INT64_C(0x3fe9b8a9637da51f) }, { INT64_C(0x03562037abf062d5) },
	{ INT64_C(0x3fb5f4ea28a7180f) }, { INT64_C(0x0613e1c4b04c2822) }, { -INT64_C(0x0613e1c4b04c2822) }, { INT64_C(0x3fb5f4ea28a7180f) },
	{ INT64_C(0x28c0b4d256654491) }, { INT64_C(0x31590e3dbc3b0f33) }, { -INT64_C(0x31590e3dbc3b0f33) }, { INT64_C(0x28c0b4d256654491) },
	{ INT64_C(0x3889066283800849) }, { INT64_C(0x1dfeff66a941dd96) }, { -INT64_C(0x1dfeff66a941dd96) }, { INT64_C(0x3889066283800849) },
	{ INT64_C(0x12c41a4e95452067) }, { INT64_C(0x3d2fd86c02d660ae) }, { -INT64_C(0x3d2fd86c02d660ae) }, { INT64_C(0x12c41a4e95452067) },
	{ INT64_C(0x3d4d0727ccafffae) }, { INT64_C(0x1263e6995554ba47) }, { -INT64_C(0x1263e6995554ba47) }, { INT64_C(0x3d4d0727ccafffae) },
	{ INT64_C(0x1e57a86d3cd824da) }, { INT64_C(0x3859a29263c7e24b) }, { -INT64_C(0x3859a29263c7e24b) }, { INT64_C(0x1e57a86d3cd824da) },
	{ INT64_C(0x3198d4ea308ca90f) }, { INT64_C(0x2872feb65486ff47) }, { -INT64_C(0x2872feb65486ff47) }, { INT64_C(0x3198d4ea308ca90f) },
	{ INT64_C(0x0677edbac92a170f) }, { INT64_C(0x3fac1a5b4eb93c13) }, { -INT64_C(0x3fac1a5b4eb93c13) }, { INT64_C(0x0677edbac92a170f) },
	{ INT64_C(0x3eceeaad1079dc3e) }, { INT64_C(0x0c4b0b93e20c0214) }, { -INT64_C(0x0c4b0b93e20c0214) }, { INT64_C(0x3eceeaad1079dc3e) },
	{ INT64_C(0x23b836c9b890e43e) }, { INT64_C(0x351acedca8b5a3f2) }, { -INT64_C(0x351acedca8b5a3f2) }, { INT64_C(0x23b836c9b890e43e) },
	{ INT64_C(0x3552a8f459731c06) }, { INT64_C(0x2364a02e26e77d27) }, { -INT64_C(0x2364a02e26e77d27) }, { INT64_C(0x3552a8f459731c06) },
	{ INT64_C(0x0cada4f4db157cf7) }, { INT64_C(0x3ebb4dda86d0b956) }, { -INT64_C(0x3ebb4dda86d0b956) }, { INT64_C(0x0cada4f4db157cf7) },
	{ INT64_C(0x3b3401bb167a8c1c) }, { INT64_C(0x184f6aaaf3903f2e) }, { -INT64_C(0x184f6aaaf3903f2e) }, { INT64_C(0x3b3401bb167a8c1c) },
	{ INT64_C(0x18ac4b86d5ed4446) }, { INT64_C(0x3b0d89088b489ef4) }, { -INT64_C(0x3b0d89088b489ef4) }, { INT64_C(0x18ac4b86d5ed4446) },
	{ INT64_C(0x2d64b9da5fc53753) }, { INT64_C(0x2d1da3d54338e27f) }, { -INT64_C(0x2d1da3d54338e27f) }, { INT64_C(0x2d64b9da5fc53753) },
	{ INT64_C(0x003243f17d994975) }, { INT64_C(0x3fffec42c43a03a5) }, { -INT64_C(0x3fffec42c43a03a5) }, { INT64_C(0x003243f17d994975) },
};
