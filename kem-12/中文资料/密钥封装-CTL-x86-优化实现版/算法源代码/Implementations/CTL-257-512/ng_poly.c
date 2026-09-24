#include "ng_inner.h"
#ifdef CTL_ENABLE_PERF
#include "perf.h"
#endif

/* see ng_inner.h */
void
poly_mp_set_small(unsigned logn, uint32_t *restrict d,
	const int8_t *restrict f, uint32_t p)
{
	size_t n = (size_t)1 << logn;
#ifdef CTL_ENABLE_PERF
	PERF_START(perf_poly_mp_set_small);
#endif
	for (size_t u = 0; u < n; u ++) {
		d[u] = mp_set(f[u], p);
	}
#ifdef CTL_ENABLE_PERF
	PERF_END(perf_poly_mp_set_small, "poly_mp_set_small");
#endif
}

/* see ng_inner.h */
void
poly_mp_set(unsigned logn, uint32_t *f, uint32_t p)
{
	size_t n = (size_t)1 << logn;
#ifdef CTL_ENABLE_PERF
	PERF_START(perf_poly_mp_set);
#endif
	for (size_t u = 0; u < n; u ++) {
		uint32_t x = f[u];
		x |= (x & 0x40000000) << 1;
		f[u] = mp_set(*(int32_t *)&x, p);
	}
#ifdef CTL_ENABLE_PERF
	PERF_END(perf_poly_mp_set, "poly_mp_set");
#endif
}

/* see ng_inner.h */
void
poly_mp_norm(unsigned logn, uint32_t *f, uint32_t p)
{
	size_t n = (size_t)1 << logn;
#ifdef CTL_ENABLE_PERF
	PERF_START(perf_poly_mp_norm);
#endif
	for (size_t u = 0; u < n; u ++) {
		f[u] = (uint32_t)mp_norm(f[u], p) & 0x7FFFFFFF;
	}
#ifdef CTL_ENABLE_PERF
	PERF_END(perf_poly_mp_norm, "poly_mp_norm");
#endif
}

/* see ng_inner.h */
int
poly_big_to_small(unsigned logn, int8_t *restrict d,
	const uint32_t *restrict s, int lim)
{
	size_t n = (size_t)1 << logn;
	for (size_t u = 0; u < n; u ++) {
		uint32_t x = s[u];
		x |= (x & 0x40000000) << 1;
		int32_t z = *(int32_t *)&x;
		if (z < -lim || z > lim) {
			return 0;
		}
		d[u] = (int8_t)z;
	}
	return 1;
}

/* see ng_inner.h */
uint32_t
poly_max_bitlength(unsigned logn, const uint32_t *f, size_t flen)
{
	if (flen == 0) {
		return 0;
	}

	size_t n = (size_t)1 << logn;
	uint32_t t = 0;
	uint32_t tk = 0;
	for (size_t u = 0; u < n; u ++, f ++) {
		/* Extend sign bit into a 31-bit mask. */
		uint32_t m = -(f[(flen - 1) << logn] >> 30) & 0x7FFFFFFF;

		/* Get top non-zero sign-adjusted word, with index. */
		uint32_t c = 0;
		uint32_t ck = 0;
		for (size_t v = 0; v < flen; v ++) {
			uint32_t w = f[v << logn] ^ m; /* sign-adjusted word */
			uint32_t nz = ((w - 1) >> 31) - 1;
			c ^= nz & (c ^ w);
			ck ^= nz & (ck ^ (uint32_t)v);
		}

		/* If ck > tk, or tk == ck but c > t, then (c,ck) must
		   replace (t,tk) as current candidate for top word/index. */
		uint32_t rr = tbmask((tk - ck) | (((tk ^ ck) - 1) & (t - c)));
		t ^= rr & (t ^ c);
		tk ^= rr & (tk ^ ck);
	}

	/*
	 * Get bit length of the top word (which has been sign-adjusted)
	 * and return the result.
	 */
	return 31 * tk + 32 - lzcnt(t);
}

/* see ng_inner.h */
void
poly_big_to_fixed(unsigned logn, fxr *restrict d, const uint32_t *restrict f,
	size_t len, uint32_t sc)
{
	size_t n = (size_t)1 << logn;
	if (len == 0) {
		memset(d, 0, n * sizeof *d);
		return;
	}

	/*
	 * We split the bit length into sch and scl such that:
	 *   sc = 31*sch + scl
	 * We also want scl in the 1..31 range, not 0..30. It may happen
	 * that sch becomes -1, which will "wrap around" (harmlessly).
	 *
	 * For each coefficient, we need three words, each with a given
	 * left shift (negative for a right shift):
	 *    sch-1   1 - scl
	 *    sch     32 - scl
	 *    sch+1   63 - scl
	 */
	uint32_t sch, scl;
	DIVREM31(sch, scl, sc);
	uint32_t z = (scl - 1) >> 31;
	sch -= z;
	scl |= 31 & -z;

	uint32_t t0 = (uint32_t)(sch - 1) & 0xFFFFFF;
	uint32_t t1 = sch & 0xFFFFFF;
	uint32_t t2 = (uint32_t)(sch + 1) & 0xFFFFFF;

	for (size_t u = 0; u < n; u ++, f ++) {
		uint32_t w0, w1, w2, ws, xl, xh;

		w0 = 0;
		w1 = 0;
		w2 = 0;
		for (size_t v = 0; v < len; v ++) {
			uint32_t t, w;

			w = f[v << logn];
			t = (uint32_t)v & 0xFFFFFF;
			w0 |= w & -((uint32_t)((t ^ t0) - 1) >> 31);
			w1 |= w & -((uint32_t)((t ^ t1) - 1) >> 31);
			w2 |= w & -((uint32_t)((t ^ t2) - 1) >> 31);
		}

		/*
		 * If there were not enough words for the requested
		 * scaling, then we must supply copies with the proper
		 * sign.
		 */
		ws = -(f[(len - 1) << logn] >> 30) >> 1;
		w0 |= ws & -((uint32_t)((uint32_t)len - sch) >> 31);
		w1 |= ws & -((uint32_t)((uint32_t)len - sch - 1) >> 31);
		w2 |= ws & -((uint32_t)((uint32_t)len - sch - 2) >> 31);

		/*
		 * Assemble the 64-bit value with the shifts. We assume
		 * that shifts on 32-bit values are constant-time with
		 * regard to the shift count (this should be true on all
		 * modern architectures; the last notable arch on which
		 * shift timing depended on the count was the Pentium IV).
		 *
		 * Since the shift count (scl) is guaranteed to be in 1..31,
		 * we do not have special cases to handle.
		 *
		 * We must sign-extend w2 to ensure the sign bit is properly
		 * set in the fnr value.
		 */
		w2 |= (uint32_t)(w2 & 0x40000000) << 1;
		xl = (w0 >> (scl - 1)) | (w1 << (32 - scl));
		xh = (w1 >> scl) | (w2 << (31 - scl));
		d[u] = fxr_of_scaled32((uint64_t)xl | ((uint64_t)xh << 32));
	}
}

/* see ng_inner.h */
void
poly_sub_scaled(unsigned logn,
	uint32_t *restrict F, size_t Flen,
	const uint32_t *restrict f, size_t flen,
	const int32_t *restrict k, uint32_t sc)
{
	if (flen == 0) {
		return;
	}
	uint32_t sch, scl;
	DIVREM31(sch, scl, sc);
	if (sch >= Flen) {
		return;
	}
	F += (size_t)sch << logn;
	Flen -= sch;
	switch (logn) {
	case 1: {
		uint32_t t0 = 0;
		uint32_t t1 = 0;
		uint32_t signf0 = -(f[(flen << 1) - 2] >> 30) >> 1;
		uint32_t signf1 = -(f[(flen << 1) - 1] >> 30) >> 1;
		int32_t k0 = k[0];
		int32_t k1 = k[1];
		int64_t cc0 = 0;
		int64_t cc1 = 0;
		for (size_t u = 0; u < Flen; u ++) {
			/*
			 * Next word, shifted.
			 */
			uint32_t f0, f1;
			if (u < flen) {
				f0 = f[(u << 1) + 0];
				f1 = f[(u << 1) + 1];
			} else {
				f0 = signf0;
				f1 = signf1;
			}
			uint32_t fs0 = ((f0 << scl) & 0x7FFFFFFF) | t0;
			uint32_t fs1 = ((f1 << scl) & 0x7FFFFFFF) | t1;
			t0 = f0 >> (31 - scl);
			t1 = f1 >> (31 - scl);

			uint32_t F0 = F[(u << 1) + 0];
			uint32_t F1 = F[(u << 1) + 1];
			int64_t z0 = (int64_t)F0 + cc0
				- (int64_t)fs0 * (int64_t)k0
				+ (int64_t)fs1 * (int64_t)k1;
			int64_t z1 = (int64_t)F1 + cc1
				- (int64_t)fs0 * (int64_t)k1
				- (int64_t)fs1 * (int64_t)k0;
			F[(u << 1) + 0] = (uint32_t)z0 & 0x7FFFFFFF;
			F[(u << 1) + 1] = (uint32_t)z1 & 0x7FFFFFFF;
			cc0 = z0 >> 31;
			cc1 = z1 >> 31;
		}
		return;
	}

	case 2: {
		uint32_t t0 = 0;
		uint32_t t1 = 0;
		uint32_t t2 = 0;
		uint32_t t3 = 0;
		uint32_t signf0 = -(f[(flen << 2) - 4] >> 30) >> 1;
		uint32_t signf1 = -(f[(flen << 2) - 3] >> 30) >> 1;
		uint32_t signf2 = -(f[(flen << 2) - 2] >> 30) >> 1;
		uint32_t signf3 = -(f[(flen << 2) - 1] >> 30) >> 1;
		int32_t k0 = k[0];
		int32_t k1 = k[1];
		int32_t k2 = k[2];
		int32_t k3 = k[3];
		int64_t cc0 = 0;
		int64_t cc1 = 0;
		int64_t cc2 = 0;
		int64_t cc3 = 0;
		for (size_t u = 0; u < Flen; u ++) {
			/*
			 * Next word, shifted.
			 */
			uint32_t f0, f1, f2, f3;
			if (u < flen) {
				f0 = f[(u << 2) + 0];
				f1 = f[(u << 2) + 1];
				f2 = f[(u << 2) + 2];
				f3 = f[(u << 2) + 3];
			} else {
				f0 = signf0;
				f1 = signf1;
				f2 = signf2;
				f3 = signf3;
			}
			uint32_t fs0 = ((f0 << scl) & 0x7FFFFFFF) | t0;
			uint32_t fs1 = ((f1 << scl) & 0x7FFFFFFF) | t1;
			uint32_t fs2 = ((f2 << scl) & 0x7FFFFFFF) | t2;
			uint32_t fs3 = ((f3 << scl) & 0x7FFFFFFF) | t3;
			t0 = f0 >> (31 - scl);
			t1 = f1 >> (31 - scl);
			t2 = f2 >> (31 - scl);
			t3 = f3 >> (31 - scl);

			uint32_t F0 = F[(u << 2) + 0];
			uint32_t F1 = F[(u << 2) + 1];
			uint32_t F2 = F[(u << 2) + 2];
			uint32_t F3 = F[(u << 2) + 3];
			int64_t z0 = (int64_t)F0 + cc0
				- (int64_t)fs0 * (int64_t)k0
				+ (int64_t)fs1 * (int64_t)k3
				+ (int64_t)fs2 * (int64_t)k2
				+ (int64_t)fs3 * (int64_t)k1;
			int64_t z1 = (int64_t)F1 + cc1
				- (int64_t)fs0 * (int64_t)k1
				- (int64_t)fs1 * (int64_t)k0
				+ (int64_t)fs2 * (int64_t)k3
				+ (int64_t)fs3 * (int64_t)k2;
			int64_t z2 = (int64_t)F2 + cc2
				- (int64_t)fs0 * (int64_t)k2
				- (int64_t)fs1 * (int64_t)k1
				- (int64_t)fs2 * (int64_t)k0
				+ (int64_t)fs3 * (int64_t)k3;
			int64_t z3 = (int64_t)F3 + cc3
				- (int64_t)fs0 * (int64_t)k3
				- (int64_t)fs1 * (int64_t)k2
				- (int64_t)fs2 * (int64_t)k1
				- (int64_t)fs3 * (int64_t)k0;
			F[(u << 2) + 0] = (uint32_t)z0 & 0x7FFFFFFF;
			F[(u << 2) + 1] = (uint32_t)z1 & 0x7FFFFFFF;
			F[(u << 2) + 2] = (uint32_t)z2 & 0x7FFFFFFF;
			F[(u << 2) + 3] = (uint32_t)z3 & 0x7FFFFFFF;
			cc0 = z0 >> 31;
			cc1 = z1 >> 31;
			cc2 = z2 >> 31;
			cc3 = z3 >> 31;
		}
		return;
	}

	case 3: {
		uint32_t t0 = 0;
		uint32_t t1 = 0;
		uint32_t t2 = 0;
		uint32_t t3 = 0;
		uint32_t t4 = 0;
		uint32_t t5 = 0;
		uint32_t t6 = 0;
		uint32_t t7 = 0;
		uint32_t signf0 = -(f[(flen << 3) - 8] >> 30) >> 1;
		uint32_t signf1 = -(f[(flen << 3) - 7] >> 30) >> 1;
		uint32_t signf2 = -(f[(flen << 3) - 6] >> 30) >> 1;
		uint32_t signf3 = -(f[(flen << 3) - 5] >> 30) >> 1;
		uint32_t signf4 = -(f[(flen << 3) - 4] >> 30) >> 1;
		uint32_t signf5 = -(f[(flen << 3) - 3] >> 30) >> 1;
		uint32_t signf6 = -(f[(flen << 3) - 2] >> 30) >> 1;
		uint32_t signf7 = -(f[(flen << 3) - 1] >> 30) >> 1;
		int32_t k0 = k[0];
		int32_t k1 = k[1];
		int32_t k2 = k[2];
		int32_t k3 = k[3];
		int32_t k4 = k[4];
		int32_t k5 = k[5];
		int32_t k6 = k[6];
		int32_t k7 = k[7];
		int64_t cc0 = 0;
		int64_t cc1 = 0;
		int64_t cc2 = 0;
		int64_t cc3 = 0;
		int64_t cc4 = 0;
		int64_t cc5 = 0;
		int64_t cc6 = 0;
		int64_t cc7 = 0;
		for (size_t u = 0; u < Flen; u ++) {
			/*
			 * Next word, shifted.
			 */
			uint32_t f0, f1, f2, f3, f4, f5, f6, f7;
			if (u < flen) {
				f0 = f[(u << 3) + 0];
				f1 = f[(u << 3) + 1];
				f2 = f[(u << 3) + 2];
				f3 = f[(u << 3) + 3];
				f4 = f[(u << 3) + 4];
				f5 = f[(u << 3) + 5];
				f6 = f[(u << 3) + 6];
				f7 = f[(u << 3) + 7];
			} else {
				f0 = signf0;
				f1 = signf1;
				f2 = signf2;
				f3 = signf3;
				f4 = signf4;
				f5 = signf5;
				f6 = signf6;
				f7 = signf7;
			}
			uint32_t fs0 = ((f0 << scl) & 0x7FFFFFFF) | t0;
			uint32_t fs1 = ((f1 << scl) & 0x7FFFFFFF) | t1;
			uint32_t fs2 = ((f2 << scl) & 0x7FFFFFFF) | t2;
			uint32_t fs3 = ((f3 << scl) & 0x7FFFFFFF) | t3;
			uint32_t fs4 = ((f4 << scl) & 0x7FFFFFFF) | t4;
			uint32_t fs5 = ((f5 << scl) & 0x7FFFFFFF) | t5;
			uint32_t fs6 = ((f6 << scl) & 0x7FFFFFFF) | t6;
			uint32_t fs7 = ((f7 << scl) & 0x7FFFFFFF) | t7;
			t0 = f0 >> (31 - scl);
			t1 = f1 >> (31 - scl);
			t2 = f2 >> (31 - scl);
			t3 = f3 >> (31 - scl);
			t4 = f4 >> (31 - scl);
			t5 = f5 >> (31 - scl);
			t6 = f6 >> (31 - scl);
			t7 = f7 >> (31 - scl);

			uint32_t F0 = F[(u << 3) + 0];
			uint32_t F1 = F[(u << 3) + 1];
			uint32_t F2 = F[(u << 3) + 2];
			uint32_t F3 = F[(u << 3) + 3];
			uint32_t F4 = F[(u << 3) + 4];
			uint32_t F5 = F[(u << 3) + 5];
			uint32_t F6 = F[(u << 3) + 6];
			uint32_t F7 = F[(u << 3) + 7];
			int64_t z0 = (int64_t)F0 + cc0
				- (int64_t)fs0 * (int64_t)k0
				+ (int64_t)fs1 * (int64_t)k7
				+ (int64_t)fs2 * (int64_t)k6
				+ (int64_t)fs3 * (int64_t)k5
				+ (int64_t)fs4 * (int64_t)k4
				+ (int64_t)fs5 * (int64_t)k3
				+ (int64_t)fs6 * (int64_t)k2
				+ (int64_t)fs7 * (int64_t)k1;
			int64_t z1 = (int64_t)F1 + cc1
				- (int64_t)fs0 * (int64_t)k1
				- (int64_t)fs1 * (int64_t)k0
				+ (int64_t)fs2 * (int64_t)k7
				+ (int64_t)fs3 * (int64_t)k6
				+ (int64_t)fs4 * (int64_t)k5
				+ (int64_t)fs5 * (int64_t)k4
				+ (int64_t)fs6 * (int64_t)k3
				+ (int64_t)fs7 * (int64_t)k2;
			int64_t z2 = (int64_t)F2 + cc2
				- (int64_t)fs0 * (int64_t)k2
				- (int64_t)fs1 * (int64_t)k1
				- (int64_t)fs2 * (int64_t)k0
				+ (int64_t)fs3 * (int64_t)k7
				+ (int64_t)fs4 * (int64_t)k6
				+ (int64_t)fs5 * (int64_t)k5
				+ (int64_t)fs6 * (int64_t)k4
				+ (int64_t)fs7 * (int64_t)k3;
			int64_t z3 = (int64_t)F3 + cc3
				- (int64_t)fs0 * (int64_t)k3
				- (int64_t)fs1 * (int64_t)k2
				- (int64_t)fs2 * (int64_t)k1
				- (int64_t)fs3 * (int64_t)k0
				+ (int64_t)fs4 * (int64_t)k7
				+ (int64_t)fs5 * (int64_t)k6
				+ (int64_t)fs6 * (int64_t)k5
				+ (int64_t)fs7 * (int64_t)k4;
			int64_t z4 = (int64_t)F4 + cc4
				- (int64_t)fs0 * (int64_t)k4
				- (int64_t)fs1 * (int64_t)k3
				- (int64_t)fs2 * (int64_t)k2
				- (int64_t)fs3 * (int64_t)k1
				- (int64_t)fs4 * (int64_t)k0
				+ (int64_t)fs5 * (int64_t)k7
				+ (int64_t)fs6 * (int64_t)k6
				+ (int64_t)fs7 * (int64_t)k5;
			int64_t z5 = (int64_t)F5 + cc5
				- (int64_t)fs0 * (int64_t)k5
				- (int64_t)fs1 * (int64_t)k4
				- (int64_t)fs2 * (int64_t)k3
				- (int64_t)fs3 * (int64_t)k2
				- (int64_t)fs4 * (int64_t)k1
				- (int64_t)fs5 * (int64_t)k0
				+ (int64_t)fs6 * (int64_t)k7
				+ (int64_t)fs7 * (int64_t)k6;
			int64_t z6 = (int64_t)F6 + cc6
				- (int64_t)fs0 * (int64_t)k6
				- (int64_t)fs1 * (int64_t)k5
				- (int64_t)fs2 * (int64_t)k4
				- (int64_t)fs3 * (int64_t)k3
				- (int64_t)fs4 * (int64_t)k2
				- (int64_t)fs5 * (int64_t)k1
				- (int64_t)fs6 * (int64_t)k0
				+ (int64_t)fs7 * (int64_t)k7;
			int64_t z7 = (int64_t)F7 + cc7
				- (int64_t)fs0 * (int64_t)k7
				- (int64_t)fs1 * (int64_t)k6
				- (int64_t)fs2 * (int64_t)k5
				- (int64_t)fs3 * (int64_t)k4
				- (int64_t)fs4 * (int64_t)k3
				- (int64_t)fs5 * (int64_t)k2
				- (int64_t)fs6 * (int64_t)k1
				- (int64_t)fs7 * (int64_t)k0;
			F[(u << 3) + 0] = (uint32_t)z0 & 0x7FFFFFFF;
			F[(u << 3) + 1] = (uint32_t)z1 & 0x7FFFFFFF;
			F[(u << 3) + 2] = (uint32_t)z2 & 0x7FFFFFFF;
			F[(u << 3) + 3] = (uint32_t)z3 & 0x7FFFFFFF;
			F[(u << 3) + 4] = (uint32_t)z4 & 0x7FFFFFFF;
			F[(u << 3) + 5] = (uint32_t)z5 & 0x7FFFFFFF;
			F[(u << 3) + 6] = (uint32_t)z6 & 0x7FFFFFFF;
			F[(u << 3) + 7] = (uint32_t)z7 & 0x7FFFFFFF;
			cc0 = z0 >> 31;
			cc1 = z1 >> 31;
			cc2 = z2 >> 31;
			cc3 = z3 >> 31;
			cc4 = z4 >> 31;
			cc5 = z5 >> 31;
			cc6 = z6 >> 31;
			cc7 = z7 >> 31;
		}
		return;
	}
	}
	size_t n = (size_t)1 << logn;
	for (size_t u = 0; u < n; u ++) {
		int32_t kf = -k[u];
		uint32_t *x = F + u;
		for (size_t v = 0; v < n; v ++) {
			zint_add_scaled_mul_small(
				x, Flen, f + v, flen, n, kf, 0, scl);
			if (u + v == n - 1) {
				x = F;
				kf = -kf;
			} else {
				x ++;
			}
		}
	}
}

/* see ng_inner.h */
void
poly_sub_scaled_ntt(unsigned logn, uint32_t *restrict F, size_t Flen,
	const uint32_t *restrict f, size_t flen,
	const int32_t *restrict k, uint32_t sc, uint32_t *restrict tmp)
{

	size_t n = (size_t)1 << logn;
	size_t tlen = flen + 1;
	uint32_t *gm = tmp;
	uint32_t *igm = gm + n;
	uint32_t *fk = igm + n;
	uint32_t *t1 = fk + (tlen << logn);
	uint32_t sch, scl;
	DIVREM31(sch, scl, sc);

	/*
	 * Compute k*f in fk[], in RNS notation.
	 * f is assumed to be already in RNS+NTT over flen+1 words.
	 */
	for (size_t u = 0; u < tlen; u ++) {
		uint32_t p = PRIMES[u].p;
		uint32_t p0i = PRIMES[u].p0i;
		uint32_t R2 = PRIMES[u].R2;
		mp_mkgmigm(logn, gm, igm, PRIMES[u].g, PRIMES[u].ig, p, p0i);
		for (size_t v = 0; v < n; v ++) {
			t1[v] = mp_set(k[v], p);
		}
		mp_NTT(logn, t1, gm, p, p0i);

		const uint32_t *fs = f + (u << logn);
		uint32_t *ff = fk + (u << logn);
		for (size_t v = 0; v < n; v ++) {
			ff[v] = mp_montymul(
				mp_montymul(t1[v], fs[v], p, p0i), R2, p, p0i);
		}
		mp_iNTT(logn, ff, igm, p, p0i);
	}

	/*
	 * Rebuild k*f.
	 */
	zint_rebuild_CRT(fk, tlen, n, 1, 1, t1);

	/*
	 * Subtract k*f, scaled, from F.
	 */
	for (size_t u = 0; u < n; u ++) {
		zint_sub_scaled(F + u, Flen, fk + u, tlen, n, sch, scl);
	}
}

/* see ng_inner.h */
void
poly_sub_kf_scaled_depth1(unsigned logn_top,
	uint32_t *restrict F, size_t FGlen,
	uint32_t *restrict k, uint32_t sc,
	const int8_t *restrict f,
	uint32_t *restrict tmp)
{
	unsigned logn = logn_top - 1;
	size_t n = (size_t)1 << logn;
	size_t hn = n >> 1;
	uint32_t *gm = tmp;
	uint32_t *t1 = gm + n;
	uint32_t *t2 = t1 + n;

	/*
	 * Step 1: convert F to RNS. Since FGlen is equal to 1 or 2,
	 * we do it with some specialized code. We assume that the RNS
	 * representation does not lose information (i.e. each signed
	 * coefficient is lower than (p0*p1)/2, with FGlen = 2 and the two
	 * prime moduli are p0 and p1).
	 */
	if (FGlen == 1) {
		uint32_t p = PRIMES[0].p;
		for (size_t u = 0; u < n; u ++) {
			uint32_t xf = F[u];
			xf |= (xf & 0x40000000) << 1;
			F[u] = mp_set(*(int32_t *)&xf, p);
		}
	} else {
		uint32_t p0 = PRIMES[0].p;
		uint32_t p0_0i = PRIMES[0].p0i;
		uint32_t z0 = mp_half(PRIMES[0].R2, p0);
		uint32_t p1 = PRIMES[1].p;
		uint32_t p1_0i = PRIMES[1].p0i;
		uint32_t z1 = mp_half(PRIMES[1].R2, p1);
		for (size_t u = 0; u < n; u ++) {
			uint32_t xl, xh, yl0, yh0, r0, yl1, yh1, r1;

			xl = F[u];
			xh = F[u + n] | ((F[u + n] & 0x40000000) << 1);
			yl0 = xl - (p0 & ~tbmask(xl - p0));
			yh0 = mp_set(*(int32_t *)&xh, p0);
			r0 = mp_add(yl0, mp_montymul(yh0, z0, p0, p0_0i), p0);
			yl1 = xl - (p1 & ~tbmask(xl - p1));
			yh1 = mp_set(*(int32_t *)&xh, p1);
			r1 = mp_add(yl1, mp_montymul(yh1, z1, p1, p1_0i), p1);
			F[u] = r0;
			F[u + n] = r1;
		}
	}

	/*
	 * Step 2: for FGlen small primes, convert F to RNS+NTT,
	 * and subtract (2^sc)*ft. The ft polynomial is computed
	 * in RNS+NTT dynamically.
	 */
	for (size_t u = 0; u < FGlen; u ++) {
		uint32_t p = PRIMES[u].p;
		uint32_t p0i = PRIMES[u].p0i;
		uint32_t R2 = PRIMES[u].R2;
		uint32_t R3 = mp_montymul(R2, R2, p, p0i);
		mp_mkgm(logn, gm, PRIMES[u].g, p, p0i);

		/*
		 * k <- (2^sc)*k (and into NTT).
		 */
		uint32_t scv = mp_montymul(
			(uint32_t)1 << (sc & 31), R2, p, p0i);
		for (uint32_t m = sc >> 5; m > 0; m --) {
			scv = mp_montymul(scv, R2, p, p0i);
		}
		for (size_t v = 0; v < n; v ++) {
			uint32_t x = mp_set(*(int32_t *)&k[v], p);
			k[v] = mp_montymul(scv, x, p, p0i);
		}
		mp_NTT(logn, k, gm, p, p0i);

		/*
		 * Convert F to NTT.
		 */
		uint32_t *Fu = F + (u << logn);
		mp_NTT(logn, Fu, gm, p, p0i);

		/*
		 * Given the top-level f, we obtain ft = N(f) (the f at
		 * depth 1) with:
		 *    f = f_e(X^2) + X*f_o(X^2)
		 * with f_e and f_o being modulo X^n+1. Then:
		 *    N(f) = f_e^2 - X*f_o^2
		 * The NTT representation of X is obtained from the gm[] tab:
		 *    NTT(X)[2*j + 0] = gm[j + n/2]
		 *    NTT(X)[2*j + 1] = -NTT(X)[2*j + 0]
		 * Note that the values in gm[] are in Montgomery
		 * representation.
		 */
		for (size_t v = 0; v < n; v ++) {
			t1[v] = mp_set(f[(v << 1) + 0], p);
			t2[v] = mp_set(f[(v << 1) + 1], p);
		}
		mp_NTT(logn, t1, gm, p, p0i);
		mp_NTT(logn, t2, gm, p, p0i);
		for (size_t v = 0; v < hn; v ++) {
			uint32_t xe0 = t1[(v << 1) + 0];
			uint32_t xe1 = t1[(v << 1) + 1];
			uint32_t xo0 = t2[(v << 1) + 0];
			uint32_t xo1 = t2[(v << 1) + 1];
			uint32_t xv0 = gm[hn + v];
			uint32_t xv1 = p - xv0;
			xe0 = mp_montymul(xe0, xe0, p, p0i);
			xe1 = mp_montymul(xe1, xe1, p, p0i);
			xo0 = mp_montymul(xo0, xo0, p, p0i);
			xo1 = mp_montymul(xo1, xo1, p, p0i);
			uint32_t xf0 = mp_sub(xe0,
				mp_montymul(xo0, xv0, p, p0i), p);
			uint32_t xf1 = mp_sub(xe1,
				mp_montymul(xo1, xv1, p, p0i), p);

			uint32_t xkf0 = mp_montymul(
				mp_montymul(xf0, k[(v << 1) + 0], p, p0i),
				R3, p, p0i);
			uint32_t xkf1 = mp_montymul(
				mp_montymul(xf1, k[(v << 1) + 1], p, p0i),
				R3, p, p0i);
			Fu[(v << 1) + 0] = mp_sub(Fu[(v << 1) + 0], xkf0, p);
			Fu[(v << 1) + 1] = mp_sub(Fu[(v << 1) + 1], xkf1, p);
		}

		/*
		 * Convert back F to RNS.
		 */
		mp_mkigm(logn, t1, PRIMES[u].ig, p, p0i);
		mp_iNTT(logn, Fu, t1, p, p0i);

		/*
		 * We replaced k (plain 32-bit) with (2^sc)*k (NTT). We must
		 * put it back to its initial value if there should be another
		 * iteration.
		 */
		if ((u + 1) < FGlen) {
			mp_iNTT(logn, k, t1, p, p0i);
			scv = (uint32_t)1 << (-sc & 31);
			for (uint32_t m = sc >> 5; m > 0; m --) {
				scv = mp_montymul(scv, 1, p, p0i);
			}
			for (size_t v = 0; v < n; v ++) {
				k[v] = (uint32_t)mp_norm(
					mp_montymul(scv, k[v], p, p0i), p);
			}
		}
	}

	/*
	 * Output F is in RNS (non-NTT), but we want plain integers.
	 */
	if (FGlen == 1) {
		uint32_t p = PRIMES[0].p;
		for (size_t u = 0; u < n; u ++) {
			F[u] = (uint32_t)mp_norm(F[u], p) & 0x7FFFFFFF;
		}
	} else {
		uint32_t p0 = PRIMES[0].p;
		uint32_t p1 = PRIMES[1].p;
		uint32_t p1_0i = PRIMES[1].p0i;
		uint32_t s = PRIMES[1].s;
		uint64_t pp = (uint64_t)p0 * (uint64_t)p1;
		uint64_t hpp = pp >> 1;
		for (size_t u = 0; u < n; u ++) {
			/*
			 * Apply CRT with two primes on the coefficient of F.
			 */
			uint32_t x0 = F[u];      /* mod p0 */
			uint32_t x1 = F[u + n];  /* mod p1 */
			uint32_t x0m1 = x0 - (p1 & ~tbmask(x0 - p1));
			uint32_t y = mp_montymul(
				mp_sub(x1, x0m1, p1), s, p1, p1_0i);
			uint64_t z = (uint64_t)x0 + (uint64_t)p0 * (uint64_t)y;
			z -= pp & -((hpp - z) >> 63);
			F[u] = (uint32_t)z & 0x7FFFFFFF;
			F[u + n] = (uint32_t)(z >> 31) & 0x7FFFFFFF;
		}
	}
}

/* see ng_inner.h */
int
poly_is_invertible(unsigned logn, const int8_t *restrict f,
	uint32_t p, uint32_t p0i, uint32_t s,
	uint32_t r, uint32_t rm, unsigned rs, uint32_t *restrict tmp)
{
	size_t n = (size_t)1 << logn;
	uint32_t *t1 = tmp;
	uint32_t *t2 = t1 + n;
	mp_mkgm(logn, t1, s, p, p0i);
	for (size_t u = 0; u < n; u ++) {
		t2[u] = mp_set(f[u], p);
	}
	mp_NTT(logn, t2, t1, p, p0i);
	uint32_t b = 0;
	for (size_t u = 0; u < n; u ++) {
		/*
		 * Reduce coefficient u modulo r, and set the top bit of b
		 * if the result is zero.
		 */
		uint32_t x = t2[u];
		uint32_t y = (uint32_t)(((uint64_t)x * (uint64_t)rm) >> rs);
		x -= r * y;
		b |= x - 1;
	}

	/*
	 * If any of the NTT coefficients was zero, then the top bit of b
	 * is 1, and the polynomial is not invertible. Otherwise, the top bit
	 * of b is 0 and the polynomial is invertible.
	 */
	return 1 - (b >> 31);
}

/* see ng_inner.h */
int
poly_is_invertible_ext(unsigned logn, const int8_t *restrict f,
	uint32_t r1, uint32_t r2, uint32_t p, uint32_t p0i, uint32_t s,
	uint32_t r1m, unsigned r1s, uint32_t r2m, unsigned r2s,
	uint32_t *restrict tmp)
{
	size_t n = (size_t)1 << logn;
	uint32_t *t1 = tmp;
	uint32_t *t2 = t1 + n;
	mp_mkgm(logn, t1, s, p, p0i);
	for (size_t u = 0; u < n; u ++) {
		t2[u] = mp_set(f[u], p);
	}
	mp_NTT(logn, t2, t1, p, p0i);
	uint32_t b = 0;
	for (size_t u = 0; u < n; u ++) {
		uint32_t x = t2[u];

		/*
		 * Reduce coefficient u modulo r1, and set the top bit of b
		 * if the result is zero.
		 */
		uint32_t y1 = (uint32_t)(((uint64_t)x * (uint64_t)r1m) >> r1s);
		b |= (x - r1 * y1) - 1;

		/*
		 * Idem for r2.
		 */
		uint32_t y2 = (uint32_t)(((uint64_t)x * (uint64_t)r2m) >> r2s);
		b |= (x - r2 * y2) - 1;
	}

	/*
	 * If any of the NTT coefficients was zero, then the top bit of b
	 * is 1, and the polynomial is not invertible. Otherwise, the top bit
	 * of b is 0 and the polynomial is invertible.
	 */
	return 1 - (b >> 31);
}

/* see ng_inner.h */
uint32_t
poly_sqnorm(unsigned logn, const int8_t *f)
{
	size_t n = (size_t)1 << logn;
	uint32_t s = 0;
	for (size_t u = 0; u < n; u ++) {
		int32_t x = f[u];
		s += (uint32_t)(x * x);
	}
	return s;
}
