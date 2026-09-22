#ifndef FALCON_FPR_H__
#define FALCON_FPR_H__

/*
 * Fixed-point operations.
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
 */


 /*
 * Active fixed-point formats for the 1024 reference implementation.
 *
 * The signing path uses NTT arithmetic and no longer needs the legacy
 * high-precision FFT-only formats.  The shared fixed-point layer is therefore
 * limited to:
 *  - fpr: signed Q20.43 in a two's-complement uint64_t carrier;
 *  - fpr_q62: signed Q2.62 for twiddles and probabilities;
 *  - fpr_cplx: packed-complex helper for the remaining root FFT code.
 *
 * Since we are manipulating integer that will overflow during some operations,
 * we also define the required 128-bit arithmetic to perform, E.G.
 * multiplications. This representation is supposed to be only intermediary, and
 * shiftted back to the relevant fpr format.
 */


 /******************************************************************************
  *                              128-bit arithmetic                            *
 ******************************************************************************/

typedef struct {
	uint64_t hi;
	uint64_t lo;
} yy_u128;

static inline int64_t
yy_fpr_i64(uint64_t x)
{
	return (int64_t)x;
}

static inline uint64_t
yy_fpr_abs_i64(int64_t x)
{
	uint64_t ux, m;

	ux = (uint64_t)x;
	m = (uint64_t)(x >> 63);
	return (ux ^ m) - m;
}

static inline int64_t
yy_fpr_apply_sign_u64(uint64_t x, uint64_t sign)
{
	uint64_t m;

	m = (uint64_t)0 - sign;
	return (int64_t)((x ^ m) - m);
}

static inline int64_t
yy_i64_rshift_rne(int64_t x, unsigned shift)
{
	uint64_t sign, mag, q, r, half;

	if (shift == 0u) {
		return x;
	}
	sign = (uint64_t)(x >> 63) & 1u;
	mag = yy_fpr_abs_i64(x);
	q = mag >> shift;
	r = mag & ((UINT64_C(1) << shift) - UINT64_C(1));
	half = UINT64_C(1) << (shift - 1u);
	q += (r > half) | ((r == half) & (q & 1u));
	return yy_fpr_apply_sign_u64(q, sign);
}

static inline yy_u128
yy_mul64_wide(uint64_t x, uint64_t y)
{
	uint64_t x0, x1, y0, y1;
	uint64_t p0, p1, p2, p3;
	uint64_t t;
	yy_u128 z;

	x0 = (uint32_t)x;
	x1 = x >> 32;
	y0 = (uint32_t)y;
	y1 = y >> 32;
	p0 = x0 * y0;
	p1 = x0 * y1;
	p2 = x1 * y0;
	p3 = x1 * y1;

	t = (p0 >> 32) + (uint32_t)p1 + (uint32_t)p2;
	z.lo = (p0 & UINT64_C(0xFFFFFFFF)) | (t << 32);
	z.hi = p3 + (p1 >> 32) + (p2 >> 32) + (t >> 32);
	return z;
}

static inline uint64_t
yy_u32_low_nonzero(uint32_t x, unsigned bits)
{
	if (bits == 0u) {
		return 0;
	}
	if (bits >= 32u) {
		return x != 0;
	}
	return (x & ((UINT32_C(1) << bits) - UINT32_C(1))) != 0;
}

static inline uint64_t
yy_u128_get_bit(yy_u128 x, unsigned bit)
{
	return bit < 64u ? ((x.lo >> bit) & 1u)
		: ((x.hi >> (bit - 64u)) & 1u);
}

static inline uint64_t
yy_u128_low_nonzero(yy_u128 x, unsigned bits)
{
	if (bits == 0u) {
		return 0;
	}
	if (bits < 64u) {
		return (x.lo & ((UINT64_C(1) << bits) - UINT64_C(1))) != 0;
	}
	if (bits == 64u) {
		return x.lo != 0;
	}
	return x.lo != 0
		|| (x.hi & ((UINT64_C(1) << (bits - 64u)) - UINT64_C(1))) != 0;
}

static inline uint64_t
yy_u128_rshift(yy_u128 x, unsigned shift)
{
	if (shift == 0u) {
		return x.lo;
	}
	if (shift < 64u) {
		return (x.lo >> shift) | (x.hi << (64u - shift));
	}
	if (shift < 128u) {
		return x.hi >> (shift - 64u);
	}
	return 0;
}

static inline uint64_t
yy_u128_rshift_rne(yy_u128 x, unsigned shift)
{
	uint64_t z, guard, sticky;

	z = yy_u128_rshift(x, shift);
	if (shift == 0u) {
		return z;
	}
	guard = yy_u128_get_bit(x, shift - 1u);
	sticky = yy_u128_low_nonzero(x, shift - 1u);
	z += guard & (sticky | (z & 1u));
	return z;
}

static inline uint64_t
yy_mul64_rshift_rne(uint64_t x, uint64_t y, unsigned shift)
{
	uint64_t x0, x1, y0, y1;
	uint64_t p0, p1, p2, p3;
	uint64_t t, hi, z, guard, sticky;
	uint32_t lo0, lo1;

	if (shift == 0u) {
		return x * y;
	}
	if (shift >= 64u) {
		return yy_u128_rshift_rne(yy_mul64_wide(x, y), shift);
	}

	/*
	 * Compute the rounded value of (x*y)/2^shift without materializing the
	 * complete yy_u128 product.  The 32-bit product limbs are:
	 *
	 *   x*y = lo0 + lo1*2^32 + hi*2^64
	 *
	 * where lo0 is the low limb, lo1 includes the carry from p0 into the
	 * cross terms, and hi is the high 64 bits.  For the shifts used by the
	 * active fpr/q62 code, the final result fits in uint64_t, so we
	 * only need the kept 64-bit window plus the guard bit and a sticky flag
	 * from the discarded low limbs.  This is equivalent to
	 * yy_u128_rshift_rne(yy_mul64_wide(x, y), shift), but avoids storing the
	 * full 128-bit value and re-decoding it in the generic shifter.
	 */
	x0 = (uint32_t)x;
	x1 = x >> 32;
	y0 = (uint32_t)y;
	y1 = y >> 32;
	p0 = x0 * y0;
	p1 = x0 * y1;
	p2 = x1 * y0;
	p3 = x1 * y1;

	t = (p0 >> 32) + (uint32_t)p1 + (uint32_t)p2;
	lo0 = (uint32_t)p0;
	lo1 = (uint32_t)t;
	hi = p3 + (p1 >> 32) + (p2 >> 32) + (t >> 32);

	if (shift < 32u) {
		z = ((uint64_t)lo0 >> shift)
			| ((uint64_t)lo1 << (32u - shift))
			| (hi << (64u - shift));
		guard = (lo0 >> (shift - 1u)) & 1u;
		sticky = yy_u32_low_nonzero(lo0, shift - 1u);
	} else if (shift == 32u) {
		z = (uint64_t)lo1 | (hi << 32);
		guard = (lo0 >> 31) & 1u;
		sticky = (lo0 & UINT32_C(0x7FFFFFFF)) != 0;
	} else {
		unsigned s;

		s = shift - 32u;
		z = ((uint64_t)lo1 >> s) | (hi << (64u - shift));
		guard = (lo1 >> (s - 1u)) & 1u;
		sticky = (lo0 != 0) | yy_u32_low_nonzero(lo1, s - 1u);
	}
	z += guard & (sticky | (z & 1u));
	return z;
}

static inline uint64_t
yy_mul64_hi(uint64_t x, uint64_t y)
{
	return yy_mul64_wide(x, y).hi;
}

static inline uint64_t
yy_mul_q63(uint64_t x, uint64_t y)
{
	return yy_mul64_rshift_rne(x, y, 63u);
}

 /******************************************************************************
  *                          Default Q20.43 fixed-point                        *
  *****************************************************************************/

typedef struct {
	uint64_t v;
} fpr;

#define YUANYANG_FPR_FRAC_BITS   43u
#define YUANYANG_FPR_ONE_RAW     (UINT64_C(1) << YUANYANG_FPR_FRAC_BITS)
#define YUANYANG_FPR_FRAC_MASK   (YUANYANG_FPR_ONE_RAW - UINT64_C(1))

#define YY_FPR_RAW(x) { UINT64_C(x) }

/*
 * Pre-computed constants related to the parameter set
*/
static const fpr fpr_inverse_of_q = YY_FPR_RAW(0x000000007500a0e1);
static const fpr fpr_yuanyang_inv_2sqrsigma_sig = YY_FPR_RAW(0x000000001eb9595b);
static const fpr fpr_inv_sigma_sq = YY_FPR_RAW(0x000000003a4b55f2);
static const fpr fpr_yuanyang_lower_radius_sq = YY_FPR_RAW(0x007c8113404ea4a4);
static const fpr fpr_yuanyang_radius_sq_span = YY_FPR_RAW(0x00439ff318fc504c);
static const fpr fpr_yuanyang_inv_2sqreta = YY_FPR_RAW(0x000004376629febf);
static const fpr fpr_yuanyang_inv_2sqr_large_sampler_sigma = YY_FPR_RAW(0x0000004376629fec);

/*
* Pre computed constants for testing only
*/
static const fpr fpr_yuanyang_eta = YY_FPR_RAW(0x000007cac083126f);
static const fpr fpr_yuanyang_large_sampler_sigma = YY_FPR_RAW(0x00001f2b020c49ba);
static const fpr fpr_yuanyang_sigma_sig = YY_FPR_RAW(0x0002e2f5c28f5c29);
static const fpr fpr_yuanyang_sigma = YY_FPR_RAW(0x0002f6af92a943d0);
static const fpr fpr_yuanyang_sigma_sq = YY_FPR_RAW(0x01190e861d86a051);
static const fpr fpr_yuanyang_inv_2sqrsigma = YY_FPR_RAW(2109591050039);
static const fpr fpr_yuanyang_alpha = YY_FPR_RAW(0x00000b3333333333);
static const fpr fpr_yuanyang_lower_radius = YY_FPR_RAW(0x0001f8f5c28f5c29);
static const fpr fpr_yuanyang_upper_radius = YY_FPR_RAW(0x00027347ae147ae1);
static const fpr fpr_yuanyang_pairgen_norm_min = YY_FPR_RAW(0x004771cbc14e5e0c);
static const fpr fpr_yuanyang_pairgen_norm_max = YY_FPR_RAW(0x011276147ae1479c);

/* Constants unrelated to YuanYang */
static const fpr fpr_log2 = YY_FPR_RAW(0x0000058b90bfbe8e);
static const fpr fpr_inv_log2 = YY_FPR_RAW(0x00000b8aa3b295c1);
static const fpr fpr_zero = YY_FPR_RAW(0x0000000000000000);
static const fpr fpr_one = YY_FPR_RAW(0x0000080000000000);

static inline fpr
fpr_of(int64_t i)
{
	fpr x;

	x.v = (uint64_t)i << YUANYANG_FPR_FRAC_BITS;
	return x;
}

static inline fpr
fpr_scaled(int64_t i, int sc)
{
	fpr x;

	x = fpr_of(i);
	if (sc >= 0) {
		x.v <<= (unsigned)sc;
	} else {
		int64_t y;

		y = yy_fpr_i64(x.v);
		y >>= (unsigned)-sc;
		x.v = (uint64_t)y;
	}
	return x;
}

static inline fpr
fpr_from_scaled_i64(int64_t i, unsigned bits)
{
	fpr x;

	if (bits <= YUANYANG_FPR_FRAC_BITS) {
		x.v = (uint64_t)i << (YUANYANG_FPR_FRAC_BITS - bits);
	} else {
		int64_t y;
		unsigned shift;

		y = i;
		shift = bits - YUANYANG_FPR_FRAC_BITS;
		y = shift >= 63u ? (y >> 63) : (y >> shift);
		x.v = (uint64_t)y;
	}
	return x;
}

static inline fpr
fpr_add(fpr x, fpr y)
{
	x.v += y.v;
	return x;
}

static inline fpr
fpr_sub(fpr x, fpr y)
{
	x.v -= y.v;
	return x;
}

static inline fpr
fpr_neg(fpr x)
{
	x.v = (uint64_t)-x.v;
	return x;
}

static inline fpr
fpr_div2e(fpr x, unsigned n)
{
	int64_t y;

	if (n == 0u) {
		return x;
	}
	y = x.v + (int64_t)(UINT64_C(1) << (n - 1u));
	y >>= n;
	x.v = (uint64_t)y;
	return x;
}

static inline fpr
fpr_mul2e(fpr x, unsigned n)
{
	x.v <<= n;
	return x;
}

static inline fpr
fpr_double(fpr x)
{
	return fpr_mul2e(x, 1);
}

static inline fpr
fpr_mul(fpr x, fpr y)
{
	int64_t xi, yi;
	uint64_t sx, sy, mag;

	xi = yy_fpr_i64(x.v);
	yi = yy_fpr_i64(y.v);
	sx = (uint64_t)(xi >> 63) & 1u;
	sy = (uint64_t)(yi >> 63) & 1u;
	mag = yy_mul64_rshift_rne(yy_fpr_abs_i64(xi),
		yy_fpr_abs_i64(yi), YUANYANG_FPR_FRAC_BITS);
	x.v = (uint64_t)yy_fpr_apply_sign_u64(mag, sx ^ sy);
	return x;
}

static inline fpr
fpr_sqr(fpr x)
{
	return fpr_mul(x, x);
}

static inline fpr
fpr_from_fpr_rne(fpr x)
{
	return x;
}

static inline fpr
fpr_to_fpr_rne(fpr x)
{
	return x;
}

static inline fpr
fpr_of_i64(int64_t x)
{
	return fpr_of(x);
}

static inline fpr
fpr_div2e_rne(fpr x, unsigned shift)
{
	return fpr_div2e(x, shift);
}

static inline fpr
fpr_mul_fpr(fpr x, fpr y)
{
	return fpr_mul(x, y);
}

static inline fpr
fpr_mul_fpr_to_fpr(fpr x, fpr y)
{
	return fpr_mul(x, y);
}

static inline int64_t
fpr_rint(fpr x)
{
	int64_t y;
	uint64_t sign, mag, q, r, half;

	y = yy_fpr_i64(x.v);
	sign = (uint64_t)(y >> 63) & 1u;
	mag = yy_fpr_abs_i64(y);
	q = mag >> YUANYANG_FPR_FRAC_BITS;
	r = mag & YUANYANG_FPR_FRAC_MASK;
	half = UINT64_C(1) << (YUANYANG_FPR_FRAC_BITS - 1u);
	q += (r > half) | ((r == half) & (q & 1u));
	return yy_fpr_apply_sign_u64(q, sign);
}

static inline int64_t
fpr_rint_to_scaled(fpr x, unsigned bits)
{
	int64_t y;
	uint64_t sign, mag, q;

	y = yy_fpr_i64(x.v);
	sign = (uint64_t)(y >> 63) & 1u;
	mag = yy_fpr_abs_i64(y);
	if (bits <= YUANYANG_FPR_FRAC_BITS) {
		unsigned shift;

		shift = YUANYANG_FPR_FRAC_BITS - bits;
		if (shift == 0u) {
			q = mag;
		} else {
			uint64_t r, half;

			q = mag >> shift;
			r = mag & ((UINT64_C(1) << shift) - UINT64_C(1));
			half = UINT64_C(1) << (shift - 1u);
			q += (r > half) | ((r == half) & (q & 1u));
		}
	} else {
		q = mag << (bits - YUANYANG_FPR_FRAC_BITS);
	}
	return yy_fpr_apply_sign_u64(q, sign);
}

static inline int64_t
fpr_floor(fpr x)
{
	return yy_fpr_i64(x.v) >> YUANYANG_FPR_FRAC_BITS;
}

static inline int64_t
fpr_trunc(fpr x)
{
	int64_t y;
	uint64_t sign, mag, q;

	y = yy_fpr_i64(x.v);
	sign = (uint64_t)(y >> 63) & 1u;
	mag = yy_fpr_abs_i64(y);
	q = mag >> YUANYANG_FPR_FRAC_BITS;
	return yy_fpr_apply_sign_u64(q, sign);
}

/* see fpr.c */
fpr fpr_div(fpr x, fpr y);

static inline fpr
fpr_inv(fpr x)
{
	return fpr_div(fpr_one, x);
}

/* see fpr.c */
fpr fpr_sqrt(fpr x);

static inline int
fpr_lt(fpr x, fpr y)
{
	return yy_fpr_i64(x.v) < yy_fpr_i64(y.v);
}

 /******************************************************************************
  *                       High-precision Q1.62 fixed-point                     *
  *****************************************************************************/
typedef struct {
	int64_t v;
} fpr_q62;

#define YUANYANG_FPR_Q62_FRAC_BITS   62u
#define YUANYANG_FPR_Q62_ONE_RAW     INT64_C(0x4000000000000000)

#define YY_FPR_Q62_RAW(x)   { INT64_C(x) }

/*
 * Pre-computed constants related to the parameter set
*/
static const fpr_q62 fpr_q62_yuanyang_theta_den[2] = {
	YY_FPR_Q62_RAW(0x0000000207f55527),
	YY_FPR_Q62_RAW(0x0000000000000000)
};
static const fpr_q62 fpr_q62_yuanyang_weak_inv_C =
	YY_FPR_Q62_RAW(0x3d5b52c055a94849);

/* Constants unrelated to YuanYang */
static const fpr_q62 fpr_q62_one = YY_FPR_Q62_RAW(0x4000000000000000);

static inline fpr_q62
fpr_q62_raw(int64_t x)
{
	fpr_q62 r;

	r.v = x;
	return r;
}

static inline int64_t
fpr_q62_to_raw(fpr_q62 x)
{
	return x.v;
}

static inline fpr_q62
fpr_q62_add(fpr_q62 x, fpr_q62 y)
{
	x.v += y.v;
	return x;
}

static inline fpr_q62
fpr_q62_sub(fpr_q62 x, fpr_q62 y)
{
	x.v -= y.v;
	return x;
}

static inline fpr_q62
fpr_q62_neg(fpr_q62 x)
{
	x.v = -x.v;
	return x;
}

static inline fpr_q62
fpr_q62_mul(fpr_q62 x, fpr_q62 y)
{
	uint64_t sx, sy, mag;

	sx = (uint64_t)(x.v >> 63) & 1u;
	sy = (uint64_t)(y.v >> 63) & 1u;
	mag = yy_mul64_rshift_rne(yy_fpr_abs_i64(x.v),
		yy_fpr_abs_i64(y.v), YUANYANG_FPR_Q62_FRAC_BITS);
	return fpr_q62_raw(yy_fpr_apply_sign_u64(mag, sx ^ sy));
}

static inline fpr_q62
fpr_q62_sqr(fpr_q62 x)
{
	return fpr_q62_mul(x, x);
}

static inline fpr_q62
fpr_q62_clamp01(fpr_q62 x)
{
	if (x.v <= 0) {
		return (fpr_q62)YY_FPR_Q62_RAW(0);
	}
	if (x.v > YUANYANG_FPR_Q62_ONE_RAW) {
		return (fpr_q62)YY_FPR_Q62_RAW(0x4000000000000000);
	}
	return x;
}

static inline fpr
fpr_q62_to_fpr(fpr_q62 x)
{
	return fpr_from_scaled_i64(x.v, YUANYANG_FPR_Q62_FRAC_BITS);
}

static inline uint64_t
fpr_q62_to_q63(fpr_q62 x)
{
	if (x.v <= 0) {
		return 0;
	}
	if (x.v >= YUANYANG_FPR_Q62_ONE_RAW) {
		return UINT64_C(0x8000000000000000);
	}
	return (uint64_t)x.v << 1;
}

static inline fpr_q62
fpr_q62_from_q63(uint64_t x)
{
	fpr_q62 y;

	y.v = (int64_t)(x >> 1);
	y.v += (int64_t)(x & 1u);
	return y;
}

static inline uint64_t
fpr_q62_to_u62(fpr_q62 x)
{
	if (x.v <= 0) {
		return 0;
	}
	if (x.v >= YUANYANG_FPR_Q62_ONE_RAW) {
		return UINT64_C(1) << 62;
	}
	return (uint64_t)x.v;
}

static inline fpr_q62
fpr_q62_from_u62(uint64_t x)
{
	if (x >= (UINT64_C(1) << 62)) {
		return (fpr_q62)YY_FPR_Q62_RAW(0x4000000000000000);
	}
	return fpr_q62_raw((int64_t)x);
}

static inline fpr
fpr_mul_q62(fpr x, fpr_q62 y)
{
	int64_t xi;
	uint64_t sx, sy, mag;

	xi = yy_fpr_i64(x.v);
	sx = (uint64_t)(xi >> 63) & 1u;
	sy = (uint64_t)(y.v >> 63) & 1u;
	mag = yy_mul64_rshift_rne(yy_fpr_abs_i64(xi),
		yy_fpr_abs_i64(y.v), YUANYANG_FPR_Q62_FRAC_BITS);
	x.v = (uint64_t)yy_fpr_apply_sign_u64(mag, sx ^ sy);
	return x;
}

 /******************************************************************************
  *                       Fixed-point complex (Q20.43)                         *
  *****************************************************************************/

typedef struct {
	fpr re;
	fpr im;
} fpr_cplx;

static inline fpr_cplx
fpr_c_make(fpr re, fpr im)
{
	fpr_cplx z;

	z.re = re;
	z.im = im;
	return z;
}

static inline fpr_cplx
fpr_c_add(fpr_cplx a, fpr_cplx b)
{
	return fpr_c_make(fpr_add(a.re, b.re), fpr_add(a.im, b.im));
}

static inline fpr_cplx
fpr_c_sub(fpr_cplx a, fpr_cplx b)
{
	return fpr_c_make(fpr_sub(a.re, b.re), fpr_sub(a.im, b.im));
}

static inline fpr_cplx
fpr_c_neg(fpr_cplx z)
{
	return fpr_c_make(fpr_neg(z.re), fpr_neg(z.im));
}

static inline fpr_cplx
fpr_c_conj(fpr_cplx z)
{
	return fpr_c_make(z.re, fpr_neg(z.im));
}

static inline fpr
fpr_c_abs2(fpr_cplx z)
{
	return fpr_add(fpr_sqr(z.re), fpr_sqr(z.im));
}

static inline fpr_cplx
fpr_c_mul(fpr_cplx a, fpr_cplx b)
{
	fpr rr, ii, ri;

	rr = fpr_mul(a.re, b.re);
	ii = fpr_mul(a.im, b.im);
	ri = fpr_sub(
		fpr_mul(fpr_add(a.re, a.im), fpr_add(b.re, b.im)),
		fpr_add(rr, ii));
	return fpr_c_make(fpr_sub(rr, ii), ri);
}

static inline fpr_cplx
fpr_c_conj_mul(fpr_cplx a, fpr_cplx b)
{
	return fpr_c_mul(fpr_c_conj(a), b);
}

static inline fpr_cplx
fpr_c_mul_q62(fpr_cplx a, fpr_q62 re, fpr_q62 im)
{
	fpr rr, ii, ri, ir;

	rr = fpr_mul_q62(a.re, re);
	ii = fpr_mul_q62(a.im, im);
	ri = fpr_mul_q62(a.re, im);
	ir = fpr_mul_q62(a.im, re);
	return fpr_c_make(fpr_sub(rr, ii), fpr_add(ri, ir));
}

static inline fpr_cplx
fpr_c_mul_fpr(fpr_cplx a, fpr re, fpr im)
{
	fpr rr, ii, ri, ir;

	rr = fpr_mul(a.re, re);
	ii = fpr_mul(a.im, im);
	ri = fpr_mul(a.re, im);
	ir = fpr_mul(a.im, re);
	return fpr_c_make(fpr_sub(rr, ii), fpr_add(ri, ir));
}

static inline fpr_cplx
fpr_fft_get(const fpr *f, size_t slot)
{
	return fpr_c_make(f[slot], f[slot + (YUANYANG_D >> 1)]);
}

static inline void
fpr_fft_set(fpr *f, size_t slot, fpr_cplx z)
{
	f[slot] = z.re;
	f[slot + (YUANYANG_D >> 1)] = z.im;
}

static inline void
fpr_poly_from_fpr(fpr *dst, const fpr *src, size_t n)
{
	for (size_t u = 0; u < n; u++) {
		dst[u] = src[u];
	}
}

static inline void
fpr_poly_from_i8(fpr *dst, const int8_t *src, size_t n)
{
	for (size_t u = 0; u < n; u++) {
		dst[u] = fpr_of(src[u]);
	}
}

static inline void
fpr_poly_to_fpr(fpr *dst, const fpr *src, size_t n)
{
	for (size_t u = 0; u < n; u++) {
		dst[u] = src[u];
	}
}

/* see fpr.c */
uint64_t fpr_expm_p63(fpr x);
fpr_q62 fpr_expm_p62(fpr x);
uint64_t fpr_to_q63(fpr x);
fpr fpr_from_q63(uint64_t x);
fpr fpr_sin_2pi(fpr x);
fpr fpr_cos_2pi(fpr x);
fpr_q62 fpr_cos_2pi_q62(fpr x);
void fpr_sincos_2pi_u53(uint64_t x, fpr *s, fpr *c);
void fpr_sincos_2pi_u53_div4(uint64_t x, fpr *s, fpr *c);

#define fpr_gm_tab_q62   Zf(fpr_gm_tab_q62)
extern const fpr_q62 fpr_gm_tab_q62[];


#endif
