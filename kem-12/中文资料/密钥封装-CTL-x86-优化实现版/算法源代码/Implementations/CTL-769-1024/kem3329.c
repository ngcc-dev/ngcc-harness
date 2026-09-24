#include "inner.h"
#ifdef CTL_ENABLE_PERF
#include "perf.h"
#endif

#ifdef CTL_AVX2
#include <immintrin.h>
#include "ctl_opt_x86.h"
#endif

/*
 * We use the mod 3329 code here.
 */
#define Q   3329
#include "modgen.c"

static inline uint32_t
mq_set_large(int32_t x)
{
	x %= 3329;
	if (x < 0) {
		x += 3329;
	}
	return mq_tomonty((uint32_t)x);
}

static int64_t
center_mod_i64(int64_t x, int64_t m)
{
	int64_t h, y;

	h = m >> 1;
	y = x % m;
	if (y <= -h) {
		y += m;
	} else if (y > h) {
		y -= m;
	}
	return y;
}

#if defined(CTL_AVX2) && CTL_AVX2
static int64_t
dot_i32_i64_avx2(const int32_t *a, const int32_t *b, size_t len)
{
	size_t u;
	int64_t sum, lanes[4];
	__m256i acc0, acc1;

	acc0 = _mm256_setzero_si256();
	acc1 = _mm256_setzero_si256();
	for (u = 0; u + 8 <= len; u += 8) {
		__m256i va, vb, pa, pb;

		if ((u & 63) == 0 && u + 72 <= len) {
			ctl_asm_prefetch_ro(a + u + 72);
			ctl_asm_prefetch_ro(b + u + 72);
		}
		va = _mm256_loadu_si256((const __m256i *)(const void *)(a + u));
		vb = _mm256_loadu_si256((const __m256i *)(const void *)(b + u));
		pa = _mm256_mul_epi32(va, vb);
		pb = _mm256_mul_epi32(
			_mm256_srli_epi64(va, 32),
			_mm256_srli_epi64(vb, 32));
		acc0 = _mm256_add_epi64(acc0, pa);
		acc1 = _mm256_add_epi64(acc1, pb);
	}

	_mm256_storeu_si256((__m256i *)(void *)lanes, acc0);
	sum = lanes[0] + lanes[1] + lanes[2] + lanes[3];
	_mm256_storeu_si256((__m256i *)(void *)lanes, acc1);
	sum += lanes[0] + lanes[1] + lanes[2] + lanes[3];
	for (; u < len; u ++) {
		sum += (int64_t)a[u] * (int64_t)b[u];
	}
	return sum;
}

static int
poly_mul_negacyclic_i64_avx2_small(int64_t *d,
	const int64_t *a, const int64_t *b, size_t n)
{
	size_t u;
	int32_t *tmp, *a32, *brev;

	tmp = (int32_t *)malloc(2 * n * sizeof *tmp);
	if (tmp == NULL) {
		return 0;
	}
	a32 = tmp;
	brev = tmp + n;
	for (u = 0; u < n; u ++) {
		a32[u] = (int32_t)a[u];
		brev[n - 1 - u] = (int32_t)b[u];
	}
	for (u = 0; u < n; u ++) {
		int64_t pos, neg;

		pos = dot_i32_i64_avx2(a32, brev + n - 1 - u, u + 1);
		neg = dot_i32_i64_avx2(a32 + u + 1, brev, n - u - 1);
		d[u] = pos - neg;
	}
	free(tmp);
	return 1;
}
#endif

static void
poly_mul_negacyclic_i64(int64_t *d,
	const int64_t *a, const int64_t *b, size_t n)
{
	/*
	 * Cache-blocked negacyclic convolution with optional AVX2 inner kernel.
	 * AVX2 path is used only when inputs fit safely into 16-bit signed range
	 * to avoid overflow; otherwise scalar blocked path is used.
	 */
	size_t i, j, bi, t;
	const size_t BLOCK = 64; /* tuneable tile size */

	/* zero output */
	for (i = 0; i < n; i ++) d[i] = 0;

#ifdef CTL_ENABLE_PERF
	PERF_START(perf_poly_mul_negacyclic);
#endif

#if defined(CTL_AVX2) && CTL_AVX2
	{
		int64_t maxa = 0, maxb = 0;

		for (i = 0; i < n; i ++) {
			int64_t va = a[i] < 0 ? -a[i] : a[i];
			int64_t vb = b[i] < 0 ? -b[i] : b[i];
			if (va > maxa) {
				maxa = va;
			}
			if (vb > maxb) {
				maxb = vb;
			}
		}
		if (maxa <= 32767 && maxb <= 32767
			&& poly_mul_negacyclic_i64_avx2_small(d, a, b, n))
		{
#ifdef CTL_ENABLE_PERF
			PERF_END(perf_poly_mul_negacyclic,
				"poly_mul_negacyclic_i64_avx2");
#endif
			return;
		}
	}
#endif

	/* Fallback: scalar blocked */
	for (bi = 0; bi < n; bi += BLOCK) {
		size_t bb = (bi + BLOCK <= n) ? BLOCK : (n - bi);
		int64_t local[BLOCK];
		for (t = 0; t < bb; t ++) local[t] = 0;

		for (i = bi; i < bi + bb; i ++) {
			int64_t ai = a[i];
			for (j = 0; j < n; j ++) {
				size_t k = i + j;
				int64_t z = ai * b[j];
				if (k >= n) {
					size_t idx = k - n;
					if (idx >= bi && idx < bi + bb) {
						local[idx - bi] -= z;
					} else {
						d[idx] -= z;
					}
				} else {
					if (k >= bi && k < bi + bb) {
						local[k - bi] += z;
					} else {
						d[k] += z;
					}
				}
			}
		}

		for (t = 0; t < bb; t ++) d[bi + t] += local[t];
	}

#ifdef CTL_ENABLE_PERF
	PERF_END(perf_poly_mul_negacyclic, "poly_mul_negacyclic_i64");
#endif
}

static inline int32_t
mod3329(int64_t x)
{
	x %= 3329;
	if (x < 0) {
		x += 3329;
	}
	return (int32_t)x;
}

static int32_t
mod3329_inv(int32_t x)
{
	int32_t a, b, u0, u1;

	a = mod3329(x);
	b = 3329;
	u0 = 1;
	u1 = 0;
	while (b != 0) {
		int32_t q, t;

		q = a / b;
		t = a - q * b;
		a = b;
		b = t;
		t = u0 - q * u1;
		u0 = u1;
		u1 = t;
	}
	if (a != 1) {
		return 0;
	}
	return mod3329(u0);
}

static int
poly_deg_i32(const int32_t *a, size_t len)
{
	while (len > 0) {
		if (a[len - 1] != 0) {
			return (int)len - 1;
		}
		len --;
	}
	return -1;
}

static void
poly_reduce_xn1_mod3329(int32_t *d, const int32_t *a, size_t a_len, size_t n)
{
	size_t u;

	memset(d, 0, n * sizeof *d);
	for (u = 0; u < a_len; u ++) {
		if (a[u] == 0) {
			continue;
		}
		if (u < n) {
			d[u] = mod3329((int64_t)d[u] + a[u]);
		} else {
			d[u - n] = mod3329((int64_t)d[u - n] - a[u]);
		}
	}
}

static void
poly_mul_negacyclic_mod3329(int32_t *d,
	const int32_t *a, const int32_t *b, size_t n)
{
	size_t i, j;
	int64_t *acc;
	unsigned logn;
	uint16_t *tmp, *ta, *tb, *tc;

	logn = 0;
	while (((size_t)1 << logn) < n && logn < 11) {
		logn ++;
	}
	if (((size_t)1 << logn) == n && logn >= 1 && logn <= 11) {
		tmp = (uint16_t *)malloc(3 * n * sizeof *tmp);
		if (tmp != NULL) {
			ta = tmp;
			tb = tmp + n;
			tc = tmp + 2 * n;
			for (i = 0; i < n; i ++) {
				ta[i] = (uint16_t)mq_set(mod3329(a[i]));
				tb[i] = (uint16_t)mq_set(mod3329(b[i]));
			}
			NTT(ta, ta, logn);
			NTT(tb, tb, logn);
			mq_poly_mul_ntt(tc, ta, tb, logn);
			iNTT(tc, tc, logn);
			for (i = 0; i < n; i ++) {
				d[i] = (int32_t)mq_unorm(tc[i]);
			}
			free(tmp);
			return;
		}
	}

	acc = (int64_t *)calloc(n, sizeof *acc);
	if (acc == NULL) {
		memset(d, 0, n * sizeof *d);
		return;
	}
	for (i = 0; i < n; i ++) {
		if (a[i] == 0) {
			continue;
		}
		for (j = 0; j < n; j ++) {
			size_t k;
			int64_t z;

			if (b[j] == 0) {
				continue;
			}
			k = i + j;
			z = (int64_t)a[i] * (int64_t)b[j];
			if (k >= n) {
				acc[k - n] -= z;
			} else {
				acc[k] += z;
			}
		}
	}
	for (i = 0; i < n; i ++) {
		d[i] = mod3329(acc[i]);
	}
	free(acc);
}

static void
poly_divmod_mod3329(int32_t *q, int32_t *r,
	const int32_t *a, int da, const int32_t *b, int db, size_t len)
{
	int32_t inv;
	int dr;

	memset(q, 0, len * sizeof *q);
	memmove(r, a, len * sizeof *r);
	if (db < 0) {
		return;
	}
	inv = mod3329_inv(b[db]);
	dr = da;
	while (dr >= db) {
		int32_t f;
		int k, j;

		f = mod3329((int64_t)r[dr] * inv);
		k = dr - db;
		q[k] = f;
		if (f != 0) {
			for (j = 0; j <= db; j ++) {
				r[k + j] = mod3329(
					(int64_t)r[k + j] - (int64_t)f * b[j]);
			}
		}
		while (dr >= 0 && r[dr] == 0) {
			dr --;
		}
	}
}

static int
poly_inv_xn1_mod3329(uint16_t *d, const int8_t *f, unsigned logn)
{
	size_t n, u;
	int32_t *r0, *r1, *r2;
	int32_t *s0, *s1, *s2;
	int32_t *q, *qred, *prod;
	int dr0, dr1;
	int ok;

	n = (size_t)1 << logn;
	r0 = (int32_t *)calloc(n + 1, sizeof *r0);
	r1 = (int32_t *)calloc(n + 1, sizeof *r1);
	r2 = (int32_t *)calloc(n + 1, sizeof *r2);
	s0 = (int32_t *)calloc(n, sizeof *s0);
	s1 = (int32_t *)calloc(n, sizeof *s1);
	s2 = (int32_t *)calloc(n, sizeof *s2);
	q = (int32_t *)calloc(n + 1, sizeof *q);
	qred = (int32_t *)calloc(n, sizeof *qred);
	prod = (int32_t *)calloc(n, sizeof *prod);
	ok = (r0 != NULL && r1 != NULL && r2 != NULL
		&& s0 != NULL && s1 != NULL && s2 != NULL
		&& q != NULL && qred != NULL && prod != NULL);
	if (!ok) {
		free(r0);
		free(r1);
		free(r2);
		free(s0);
		free(s1);
		free(s2);
		free(q);
		free(qred);
		free(prod);
		return 0;
	}

	r0[0] = 1;
	r0[n] = 1;
	for (u = 0; u < n; u ++) {
		r1[u] = mod3329(f[u]);
	}
	s1[0] = 1;
	dr0 = (int)n;
	dr1 = poly_deg_i32(r1, n + 1);
	while (dr1 >= 0) {
		int32_t *tx;
		int dr2;

		poly_divmod_mod3329(q, r2, r0, dr0, r1, dr1, n + 1);
		poly_reduce_xn1_mod3329(qred, q, n + 1, n);
		poly_mul_negacyclic_mod3329(prod, qred, s1, n);
		for (u = 0; u < n; u ++) {
			s2[u] = mod3329((int64_t)s0[u] - prod[u]);
		}
		dr2 = poly_deg_i32(r2, n + 1);
		tx = r0; r0 = r1; r1 = r2; r2 = tx;
		tx = s0; s0 = s1; s1 = s2; s2 = tx;
		dr0 = dr1;
		dr1 = dr2;
	}

	ok = (dr0 == 0 && r0[0] != 0);
	if (ok) {
		int32_t scale;

		scale = mod3329_inv(r0[0]);
		ok = (scale != 0);
		if (ok) {
			for (u = 0; u < n; u ++) {
				d[u] = (uint16_t)mod3329((int64_t)s0[u] * scale);
			}
		}
	}

	free(r0);
	free(r1);
	free(r2);
	free(s0);
	free(s1);
	free(s2);
	free(q);
	free(qred);
	free(prod);
	return ok;
}

static int
ctl_decrypt_3329_exact(uint8_t *sbuf, const int16_t *c,
	const int8_t *f, const int8_t *g, const int16_t *F, const int16_t *G,
	const int32_t *w, unsigned logn)
{
	size_t n, u;
	int64_t *fi, *gi, *Fi, *Gi, *wi, *ci, *ones;
	int64_t *cp, *cs, *t1, *t2, *Fd;
	int ok;

	if (logn != 11) {
		return 0;
	}
	n = (size_t)1 << logn;

	fi = (int64_t *)malloc(n * sizeof(*fi));
	gi = (int64_t *)malloc(n * sizeof(*gi));
	Fi = (int64_t *)malloc(n * sizeof(*Fi));
	Gi = (int64_t *)malloc(n * sizeof(*Gi));
	wi = (int64_t *)malloc(n * sizeof(*wi));
	ci = (int64_t *)malloc(n * sizeof(*ci));
	ones = (int64_t *)malloc(n * sizeof(*ones));
	cp = (int64_t *)malloc(n * sizeof(*cp));
	cs = (int64_t *)malloc(n * sizeof(*cs));
	t1 = (int64_t *)malloc(n * sizeof(*t1));
	t2 = (int64_t *)malloc(n * sizeof(*t2));
	Fd = (int64_t *)malloc(n * sizeof(*Fd));
	ok = (fi != NULL && gi != NULL && Fi != NULL && Gi != NULL
		&& wi != NULL && ci != NULL && ones != NULL
		&& cp != NULL && cs != NULL && t1 != NULL
		&& t2 != NULL && Fd != NULL);
	if (!ok) {
		free(fi);
		free(gi);
		free(Fi);
		free(Gi);
		free(wi);
		free(ci);
		free(ones);
		free(cp);
		free(cs);
		free(t1);
		free(t2);
		free(Fd);
		return 0;
	}

	for (u = 0; u < n; u ++) {
		fi[u] = f[u];
		gi[u] = g[u];
		Fi[u] = F[u];
		Gi[u] = G[u];
		wi[u] = w[u];
		ci[u] = c[u];
		ones[u] = 1;
	}

	poly_mul_negacyclic_i64(cp, fi, ci, n);
	for (u = 0; u < n; u ++) {
		cp[u] *= 16;
	}
	poly_mul_negacyclic_i64(t1, fi, ones, n);
	poly_mul_negacyclic_i64(t2, gi, ones, n);
	for (u = 0; u < n; u ++) {
		cp[u] = center_mod_i64(cp[u] - t1[u] - t2[u], 6658);
	}

	poly_mul_negacyclic_i64(cs, Fi, ci, n);
	for (u = 0; u < n; u ++) {
		cs[u] *= 16 * 64513;
	}
	poly_mul_negacyclic_i64(t1, Fi, ones, n);
	poly_mul_negacyclic_i64(t2, Gi, ones, n);
	for (u = 0; u < n; u ++) {
		cs[u] -= 64513 * (t1[u] + t2[u]);
	}
	poly_mul_negacyclic_i64(t1, cp, wi, n);
	for (u = 0; u < n; u ++) {
		cs[u] = center_mod_i64(cs[u] - t1[u], 429527554);
	}

	poly_mul_negacyclic_i64(Fd, fi, wi, n);
	for (u = 0; u < n; u ++) {
		Fd[u] = 64513 * Fi[u] - Fd[u];
	}

	poly_mul_negacyclic_i64(t1, Fd, cp, n);
	poly_mul_negacyclic_i64(t2, fi, cs, n);

	memset(sbuf, 0, (n + 7) >> 3);
	for (u = 0; u < n; u ++) {
		sbuf[u >> 3] |= (uint8_t)((t1[u] > t2[u]) << (u & 7));
	}

	free(fi);
	free(gi);
	free(Fi);
	free(Gi);
	free(wi);
	free(ci);
	free(ones);
	free(cp);
	free(cs);
	free(t1);
	free(t2);
	free(Fd);
	return 1;
}

/* see inner.h */
int
ctl_make_public_3329(uint16_t *h, const int8_t *f, const int8_t *g,
	unsigned logn, uint32_t *tmp)
{
	size_t u, n;
	int32_t *gp, *hp;
	int ok;

	(void)tmp;
	n = (size_t)1 << logn;
	gp = (int32_t *)calloc(n, sizeof *gp);
	hp = (int32_t *)calloc(n, sizeof *hp);
	ok = (gp != NULL && hp != NULL);
	if (!ok) {
		free(gp);
		free(hp);
		return 0;
	}
	for (u = 0; u < n; u ++) {
		gp[u] = mod3329(g[u]);
	}
	ok = poly_inv_xn1_mod3329(h, f, logn);
	if (ok) {
		for (u = 0; u < n; u ++) {
			hp[u] = h[u];
		}
		poly_mul_negacyclic_mod3329(gp, gp, hp, n);
		for (u = 0; u < n; u ++) {
			h[u] = (uint16_t)gp[u];
		}
	}
	free(gp);
	free(hp);
	return ok;
}

/* see inner.h */
uint32_t
ctl_encrypt_3329(int16_t *c, const uint8_t *sbuf,
	const uint16_t *h, unsigned logn, uint32_t *tmp)
{
	size_t u, n;
	int32_t *sp, *hp, *tp;
	uint32_t e2norm;

	(void)tmp;
	n = (size_t)1 << logn;
	sp = (int32_t *)calloc(n, sizeof *sp);
	hp = (int32_t *)calloc(n, sizeof *hp);
	tp = (int32_t *)calloc(n, sizeof *tp);
	if (sp == NULL || hp == NULL || tp == NULL) {
		free(sp);
		free(hp);
		free(tp);
		return 0;
	}
	for (u = 0; u < n; u ++) {
		sp[u] = (sbuf[u >> 3] >> (u & 7)) & 1;
		hp[u] = h[u];
	}
	poly_mul_negacyclic_mod3329(tp, hp, sp, n);

	/*
	 * c <- round((h*s mod q)/k)
	 * (for q = 3329, we have k = 8).
	 *
	 * Rounding is toward +infty, so that error e = k*c - (h*s mod q)
	 * has coefficients in 0..7.
	 *
	 * We also compute in e2norm the sum of the squares of the coefficients
	 * of 2*e', where e' = e - E(e) = e - 1/2*(1+X+X^2+X^3+...+X^(n-1)).
	 */
	e2norm = 0;
	for (u = 0; u < n; u ++) {
		int y, z, ep;

		y = tp[u];
		if (y > 1664) {
			y -= 3329;
		}
		z = ((y + 1668) >> 3) - 208;
		c[u] = z;
		ep = (8 * z - y);
		ep = 2 * ep - 1;
		e2norm += (uint32_t)(ep * ep);
	}

	/*
	 * Ciphertext is acceptable if and only if the norm
	 * of (gamma*s',e') is not greater than
	 * 1.08*sqrt((n/2)*gamma^2). Note that e' and s' are
	 * centered on 0, i.e. e' = e - E(e) and s' = s - E(s).
	 * With k = 8, we have gamma^2 = (k^2 - 1)/3 = 21.
	 *
	 * Coefficients of s are in {0,1}, and E(s) = 1/2, therefore
	 * coefficients of s' are in {-1/2,+1/2}. Thus, the sum
	 * of the squares of the coefficients of gamma*s' is always
	 * 21*n/4.
	 *
	 * Coefficients of e are in {0,1,2,3,4,5,6,7} and E(e) = 7/2, therefore
	 * coefficients of e' are in {-7/2,-5/2,-3/2,-1/2,+1/2,+3/2,+5/2,+7/2}.
	 * We computed
	 * the sum of the squares of the coefficients of 2*e' in e2norm.
	 */
	{
		uint64_t lhs, rhs;

		lhs = ((uint64_t)21 << logn) + (uint64_t)e2norm;
		rhs = (489888ull * (uint64_t)n) / 10000ull;
		free(sp);
		free(hp);
		free(tp);
		return lhs <= rhs;
	}
}

/* see inner.h */
void
ctl_decrypt_3329(uint8_t *sbuf, const int16_t *c,
	const int8_t *f, const int8_t *g, const int16_t *F, const int16_t *G,
	const int32_t *w, unsigned logn, uint32_t *tmp)
{
	if (ctl_decrypt_3329_exact(sbuf, c, f, g, F, G, w, logn)) {
		return;
	}

	/*
	 * Decapsulation algorithm:
	 *
	 *   c <- k*c
	 *   c' <- (Q*f*c - f*ones - g*ones) mod q*Q
	 *   c'' <- (q'*Q*F*c - q'*F*ones - q'*G*ones - c'*w) mod q*q'*Q
	 *   e' = (-Gd*c' + g*c'') / (q*q'*Q)
	 *   s' = (Fd*c' - f*c'') / (q*q'*Q)
	 *   e = e' + (1/2)*ones
	 *   s = s' + (1/2)*ones
	 *
	 * If the ciphertext is correct, then it must be that all
	 * coefficients of s are in {0,1}, and all coefficients of e are
	 * in -((k/2)-1)..(k/2) (for k even).
	 *
	 * When q = 3329, we have Q = 2 and k = 8. This simplifies some
	 * things:
	 *
	 *   - f*ones mod 2  is a constant polynomial: all its
	 *     coefficients are equal to the "parity" of f, i.e. the sum
	 *     of its coefficients mod 2. The keygen implementation
	 *     produces only polynomials f and g with odd parity, but an
	 *     externally provided key may have f or g with an even
	 *     parity (not both can be even, though, otherwise the NTRU
	 *     equation g*F - f*G = q has no solution).
	 *
	 *   - We compute c' modulo q, and modulo Q = 2. Since f*ones
	 *     and g*ones are constant polynomials modulo 2, and
	 *     Q*f*c = 0 mod Q, c' mod Q is a constant polynomial (all
	 *     coefficients are 0, or all coefficients are 1). We
	 *     get c' mod q, and adjust each coefficient in order to have
	 *     the correct parity.
	 *
	 *   - c'' is computed modulo q, q' and Q. Modulo Q = 2:
	 *        q'*Q*F*c = 0 mod Q
	 *        q'*F*ones mod Q  is a constant polynomial
	 *        q'*G*ones mod Q  is a constant polynomial
	 *        c' mod Q  is a constant polynomial, either 0 or ones;
	 *        thus, c'*w is a constant polynomial modulo Q.
	 *
	 *     Modulo q', q'*Q*F*c, q'*F*ones and q'*G*ones are zero,
	 *     so only c'*w has to be computed modulo q'.
	 *
	 *     Modulo q, all elements must be computed.
	 *
	 *     When we have c'' modulo q, q' and Q, we use the CRT to get
	 *     the value modulo q*q'*Q.
	 *
	 * Once we have c' and c'', we can compute 2*q*q'*Q*e' and
	 * 2*q*q'*Q*s'. These are nominally plain integers, but we do
	 * not need the full values; we just want to distinguish the
	 * possible values for the coefficients of s', which are in
	 * {-1/2,+1/2}. Thus, we can do the computation modulo any prime
	 * p which is not 2, q or q'; in practice, we use the "other q"
	 * (i.e. 257) since we already have the code for computations
	 * modulo that prime.
	 */
	size_t u, n;
	uint16_t *t1, *t2, *t3, *t4;
	unsigned par_fg, par_FG, par_w, cp2, cs2;

	n = (size_t)1 << logn;

	t1 = (uint16_t *)tmp;
	t2 = t1 + n;
	t3 = t2 + n;
	t4 = t3 + n;

	/*
	 * In the CTL specification, algorithm 3.2 ("Decode") expects
	 * as polynomial 'c' what is really 'k*c' in algorithm 4.3
	 * ("Decapsulate"). With q = 3329, we have k = 8. Moreover, we
	 * want Q*c, with Q = 2; thus, we compute 16*c here.
	 */
	for (u = 0; u < n; u ++) {
		t1[u] = mq_set(16 * c[u]);
	}

	/*
	 * We have Q*c in t1; convert it to NTT (mod q).
	 */
	NTT(t1, t1, logn);

	/*
	 * Get NTT representations of f and f+g, in t2 and t3, respectively.
	 */
	for (u = 0; u < n; u ++) {
		t2[u] = mq_set(f[u]);
		t3[u] = mq_set(f[u] + g[u]);
	}
	NTT(t2, t2, logn);
	NTT(t3, t3, logn);

	/*
	 * t2 <- Q*f*c mod q  (NTT)
	 */
	mq_poly_mul_ntt(t2, t1, t2, logn);

	/*
	 * t2 <- c' mod q  (NTT)
	 * t3 is scratch
	 */
	mq_poly_mul_ones_ntt(t3, t3, logn);
	mq_poly_sub(t2, t2, t3, logn);

	/*
	 * t1 <- q'*Q*F*c mod q  (NTT)
	 * t3 <- q'*F mod q      (NTT)
	 */
	for (u = 0; u < n; u ++) {
		t3[u] = mq_set_large((int32_t)F[u] * (64513 % 3329));
	}
	NTT(t3, t3, logn);
	mq_poly_mul_ntt(t1, t1, t3, logn);

	/*
	 * t1 <- q'*Q*F*c - q'*F*ones mod q  (NTT)
	 * t3 is scratch
	 */
	mq_poly_mul_ones_ntt(t3, t3, logn);
	mq_poly_sub(t1, t1, t3, logn);

	/*
	 * t1 <- q'*Q*F*c - q'*F*ones - q'*G*ones mod q  (NTT)
	 */
	for (u = 0; u < n; u ++) {
		t3[u] = mq_set_large((int32_t)G[u] * (64513 % 3329));
	}
	NTT(t3, t3, logn);
	mq_poly_mul_ones_ntt(t3, t3, logn);
	mq_poly_sub(t1, t1, t3, logn);

	/*
	 * t1 <- q'*Q*F*c - q'*F*ones - q'*G*ones - c'*w mod q  (NTT)
	 */
	for (u = 0; u < n; u ++) {
		t3[u] = mq_set(w[u]);
	}
	NTT(t3, t3, logn);
	mq_poly_mul_ntt(t3, t2, t3, logn);
	mq_poly_sub(t1, t1, t3, logn);

	/*
	 * We won't need NTT representations for c' and c'' any more, so
	 * we convert back to normal (but still Montgomery).
	 */
	iNTT(t1, t1, logn);
	iNTT(t2, t2, logn);

	/*
	 * We now have c'' and c' mod q, in t1 and t2, respectively (both
	 * are in normal representation).
	 *
	 * The actual values (with plain integer coefficients) of c'
	 * and c'' are the results of modular reduction and normalization
	 * of c' and c'' modulo q*Q and q*q'*Q, respectively. We must
	 * thus get c' modulo Q, and c'' modulo Q and modulo q', in order
	 * to access the actual coefficients.
	 *
	 *
	 * Since Q = 2, there are shortcuts for computing modulo Q. The
	 * product of any polynomial by the 'ones' polynomial yields
	 * either zero, or the 'ones' polynomial, depending on the
	 * "parity" of the source polynomial (the sum of its
	 * coefficients modulo 2). Therefore, modulo Q:
	 *
	 *   c' = Q*f*c - (f+g)*ones mod 2 = parity(f+g)*ones modulo 2
	 *   c'' = q'*Q*F*c - q'*(F+G)*ones - c'*w mod 2
	 *       = (parity(F+G) + parity(f+g)*parity(w) mod 2)*ones
	 *
	 * Therefore, we only need to get the parities of f, g, F, G and
	 * w to get c' and c'' modulo Q.
	 *
	 * Note that our keygen algorithm only produces f and g such
	 * that parity(f) = 1 and parity(g) = 1. Thus, for our private
	 * keys, we have parity(f+g) = 0. However, this may not be the
	 * case for other, externally generated private keys (the NTRU
	 * equation g*F - f*G = q cannot be solved for an odd q if f and
	 * g both have parity 0, but it is possible to have one of f or
	 * g to be of parity 0). Therefore, we apply the complete
	 * treatment below (the extra cost is negligible in practice).
	 */
	par_fg = 0;
	par_FG = 0;
	par_w = 0;
	for (u = 0; u < n; u ++) {
		par_fg += (unsigned)f[u] + (unsigned)g[u];
		par_FG += (unsigned)F[u] + (unsigned)G[u];
		par_w += (unsigned)w[u];
	}
	par_fg &= 1;
	par_FG &= 1;
	par_w &= 1;

	cp2 = par_fg;
	cs2 = par_FG ^ (par_fg & par_w);

	/*
	 * Set t2 to coefficients of c'. We have c' mod q in t2, so we
	 * just need to adjust them to account for the parity we just
	 * computed in cp2:
	 *  - coefficient must be normalized to -384..+384
	 *  - if the parity is wrong, we must add 3329 (if the value is
	 *    negative or zero) or subtract 3329 (if the value is strictly
	 *    positive), so that the result is in -768..+3329.
	 */
	for (u = 0; u < n; u ++) {
		uint32_t x;

		/*
		 * Get next coefficient of c' into x, normalized in
		 * -384..+384.
		 */
		x = (uint32_t)mq_snorm(t2[u]);

		/*
		 * Adjust x to get the right value modulo 2: if it has
		 * the wrong parity, then we must add or subtract 3329.
		 * We add 3329 if the value is negative or zero, subtract
		 * 3329 if it is negative.
		 */
		x += -(uint32_t)((x ^ cp2) & 1u)
			& -(uint32_t)3329
			& (((x - 1) >> 16) & (2 * 3329));

		/*
		 * We write back x as an unsigned 16-bit integer, but with
		 * two's complement notation for negative values, which will
		 * be fine since uint16_t and int16_t have compatible
		 * memory layouts, and ctl_polyqp_mulneg() expects int16_t
		 * values.
		 */
		t2[u] = (uint16_t)x;
	}

	/*
	 * We want:
	 *   c'' = -c'*w mod q'
	 * We compute that value into t3.
	 */
	ctl_polyqp_mulneg((int16_t *)t3, (int16_t *)t2, w, logn,
		(uint32_t *)t4);

	/*
	 * At that point:
	 *    t1    c'' mod q  (normal representation, Montgomery)
	 *    t2    c'         (normal representation, signed)
	 *    t3    c'' mod q' (normal representation, signed)
	 *    cs2   c'' mod 2  (to multiply by 'ones')
	 * We now compute c'' itself in normal representation and signed
	 * integers by combining the coefficients modulo q, q' and 2,
	 * stored in t1[], t3[] and cs2, respectively. This uses the CRT.
	 */
	for (u = 0; u < n; u ++) {
		uint32_t y0, y1, t;
		int32_t z;
		int32_t z257;

		/*
		 * If:
		 *    y = y0 mod q
		 *    y = y1 mod q'
		 * Then:
		 *    y = ((1/q') * (y0 - y1) mod q) * q' + y1
		 * We need y0 and y1 in 0..q-1 and 0..q'-1, respectively.
		 */
		y0 = mq_unorm(t1[u]);
		y1 = (uint32_t)*(int16_t *)&t3[u];
		y1 += 64513 & (y1 >> 16);

		t = mq_unorm(mq_montymul(
			mq_inv(mq_set(64513 % 3329)),
			mq_set((int32_t)y0 - (int32_t)y1)));
		z = (int32_t)((uint64_t)t * 64513u + y1);
		if (z == 0) {
			z = 214763777;
		}
		if (((unsigned)z & 1u) != cs2) {
			z -= 214763777;
		}
		z257 = z % 257;
		if (z257 < 0) {
			z257 += 257;
		}
		t1[u] = m257_tomonty((uint32_t)z257);
	}

	/*
	 * We now have c' and c'' in t2 and t1, respectively. c' uses
	 * signed integers; c'' is in Montgomery representation modulo 257.
	 * We convert c' the Montgomery representation modulo 257 as
	 * well.
	 */
	for (u = 0; u < n; u ++) {
		int32_t z;

		z = *(int16_t *)&t2[u];
		z %= 257;
		if (z < 0) {
			z += 257;
		}
		t2[u] = m257_tomonty((uint32_t)z);
	}

	/*
	 * We need to compute:
	 *   Fd = q'*F - f*w
	 *   s' = 1/(q*q'*Q) (Fd*c' - f*c'')
	 *   s = s' + (1/2)*ones
	 *
	 * We use the mod 257 code to obtain q*q'*Q*s'.
	 */
	ctl_finish_decapsulate_257_i16(t2, t1, f, F, w, logn, (uint32_t *)t3);

	/*
	 * If the ciphertext is correct and the decapsulation worked well,
	 * then s' has coefficients in {-1/2,1/2} and the coefficients of
	 * s are obtained by adding 1/2. We have the coefficients of
	 * q*q'*Q*s' in t2[], in Montgomery representation modulo 257:
	 *
	 *    s'   s   t2[]
	 *  -1/2   0    72    (-q*q'*Q/2 = -q*q' = 72 mod 257)
	 *  +1/2   1   185    (+q*q'*Q/2 = +q*q' = 185 mod 257)
	 *
	 * Therefore, we just need to look at the least significant bit
	 * of each value in t2[] to get the coefficients of s.
	 */
	memset(sbuf, 0, (n + 7) >> 3);
	for (u = 0; u < n; u ++) {
		sbuf[u >> 3] |= (t2[u] & 1) << (u & 7);
	}
}

/* see inner.h */
void
ctl_finish_decapsulate_3329(uint16_t *cp, uint16_t *cs,
	const int8_t *f, const int16_t *F, const int32_t *w, unsigned logn,
	uint32_t *tmp)
{
	/*
	 * Formulas:
	 *   Fd = q'*F - f*w
	 *   q*q'*Q*s = Fd*c' - f*c''
	 * We use q' = 64513.
	 */
	size_t u, n;
	uint16_t *t1, *t2;

	n = (size_t)1 << logn;
	t1 = (uint16_t *)tmp;
	t2 = t1 + n;

	/*
	 * Convert c' and c'' to NTT.
	 */
	NTT(cp, cp, logn);
	NTT(cs, cs, logn);

	/*
	 * Load f in t1 and w in t2 and convert both to NTT.
	 */
	for (u = 0; u < n; u ++) {
		t1[u] = mq_set(f[u]);
		t2[u] = mq_set(w[u]);
	}
	NTT(t1, t1, logn);
	NTT(t2, t2, logn);

	/*
	 * cs <- f*c''  (NTT)
	 * t1 <- f*w    (NTT)
	 */
	mq_poly_mul_ntt(cs, cs, t1, logn);
	mq_poly_mul_ntt(t1, t1, t2, logn);

	/*
	 * t2 <- q'*F  (NTT)
	 */
	for (u = 0; u < n; u ++) {
		t2[u] = mq_set_large((int32_t)F[u] * (64513 % 3329));
	}
	NTT(t2, t2, logn);

	/*
	 * t1 <- Fd = q'*F - f*w  (NTT)
	 */
	mq_poly_sub(t1, t2, t1, logn);

	/*
	 * cp <- Fd*c' - f*c''    (normal representation)
	 */
	mq_poly_mul_ntt(t1, t1, cp, logn);
	mq_poly_sub(t1, t1, cs, logn);
	iNTT(cp, t1, logn);
}

/* see inner.h */
int
ctl_rebuild_G_3329_i16(int16_t *G,
	const int8_t *f, const int8_t *g, const int16_t *F,
	unsigned logn, uint32_t *tmp)
{
	size_t u, n;
	uint16_t *finv;
	int32_t *gp, *Fp, *Gi;
	int lim;
	int ok;

	(void)tmp;
	n = (size_t)1 << logn;
	finv = (uint16_t *)calloc(n, sizeof *finv);
	gp = (int32_t *)calloc(n, sizeof *gp);
	Fp = (int32_t *)calloc(n, sizeof *Fp);
	Gi = (int32_t *)calloc(n, sizeof *Gi);
	ok = (finv != NULL && gp != NULL && Fp != NULL && Gi != NULL);
	if (!ok) {
		free(finv);
		free(gp);
		free(Fp);
		free(Gi);
		return 0;
	}
	for (u = 0; u < n; u ++) {
		gp[u] = mod3329(g[u]);
		Fp[u] = mod3329(F[u]);
	}
	ok = poly_inv_xn1_mod3329(finv, f, logn);
	if (ok) {
		for (u = 0; u < n; u ++) {
			Gi[u] = finv[u];
		}
		poly_mul_negacyclic_mod3329(gp, gp, Fp, n);
		poly_mul_negacyclic_mod3329(Gi, gp, Gi, n);
	}

	/*
	 * Get back the coefficients of G. The coefficients are verified
	 * to remain within the expected bound.
	 */
	lim = (1 << (ctl_max_FG_bits[logn] - 1)) - 1;
	if (ok) {
		for (u = 0; u < n; u ++) {
			int x;

			x = Gi[u];
			if (x > 1664) {
				x -= 3329;
			}
			if (x < -lim || x > +lim) {
				ok = 0;
				break;
			}
			G[u] = (int16_t)x;
		}
	}
	free(finv);
	free(gp);
	free(Fp);
	free(Gi);
	return ok;
}

/*
 * Encoding format: 5 values modulo 3329 are encoded over 48 bits.
 * Value x_i is split into its 3 low-order bits (xl_i, value in 0..7)
 * and its 7 high-order bits (xh_i, value in 0..96). First 15 bits
 * are the xl_i; then remaining 33 bits are the xh_i (digits in base-97).
 *
 * Remaining values (0 to 4) are encoded with 10 bits per value.
 */

/*
 * Encode five values x[0]..x[4] into 48 bits (bits 0..31 are returned
 * in *lo, bits 32..47 are the function return value).
 */
static inline uint32_t
encode5(uint32_t *lo, const uint16_t *x)
{
	uint32_t wl, wh, tt, z;
	int i;

	wl = 0;
	for (i = 0; i < 5; i ++) {
		wl |= (uint32_t)(x[i] & 0x07) << (3 * i);
	}

	wh = 0;
	for (i = 4; i > 0; i --) {
		wh = (wh * 97) + (uint32_t)(x[i] >> 3);
	}

	/*
	 * Final iteration may overflow into the extra 'tt' bit.
	 * At that point, wh <= 97^4-1, and 48*wh < 2^32.
	 */
	z = wh + (uint32_t)(x[0] >> 3);
	wh *= 48;
	tt = wh >> 31;
	wh <<= 1;
	wh += z;
	tt |= ((uint32_t)(wh - z) & ~wh) >> 31;

	/*
	 * We have bits 0..14 in wl, bits 15..46 in wh, and bit 47 in tt.
	 */
	*lo = wl | (wh << 15);
	return (wh >> 17) | (tt << 15);
}


/* see inner.h */
size_t
ctl_encode_3329(void *out, size_t max_out_len,
    const uint16_t *x, unsigned logn)
{
    size_t n, out_len, i;
    uint8_t *buf;
    uint64_t acc;
    unsigned acc_len;

    n = (size_t)1 << logn;
    out_len = (n / 4) * 47 / 8;

    if (out == NULL) {
        return out_len;
    }
    if (max_out_len < out_len) {
        return 0;
    }

    buf = out;
    acc = 0;
    acc_len = 0;

    for (i = 0; i < n; i += 4) {
        uint32_t high_part;
        uint32_t low_part;
        uint16_t x0, x1, x2, x3;
        uint8_t h0, h1, h2, h3;

        x0 = x[i];
        x1 = x[i + 1];
        x2 = x[i + 2];
        x3 = x[i + 3];

        h0 = (uint8_t)(x0 >> 6);
        h1 = (uint8_t)(x1 >> 6);
        h2 = (uint8_t)(x2 >> 6);
        h3 = (uint8_t)(x3 >> 6);

        high_part = (uint32_t)h0 + 53U * ((uint32_t)h1 + 53U * ((uint32_t)h2 + 53U * (uint32_t)h3));

        low_part = ((uint32_t)(x0 & 0x3F))
            | (((uint32_t)(x1 & 0x3F)) << 6)
            | (((uint32_t)(x2 & 0x3F)) << 12)
            | (((uint32_t)(x3 & 0x3F)) << 18);

        acc = (acc << 47) | ((uint64_t)high_part << 24) | low_part;
        acc_len += 47;

        while (acc_len >= 8) {
            acc_len -= 8;
            *buf++ = (uint8_t)(acc >> acc_len);
        }
    }

    if (acc_len > 0) {
        *buf++ = (uint8_t)(acc << (8 - acc_len));
    }

    return out_len;
}

/* see inner.h */
size_t
ctl_decode_3329(uint16_t *x, unsigned logn,
    const void *in, size_t max_in_len)
{
    size_t n, in_len, i;
    const uint8_t *buf;
    uint64_t acc;
    unsigned acc_len;
    uint32_t r;

    n = (size_t)1 << logn;
    in_len = (n / 4) * 47 / 8;

    if (max_in_len < in_len) {
        return 0;
    }

    buf = in;
    acc = 0;
    acc_len = 0;
    r = 1;

    for (i = 0; i < n; i += 4) {
        uint64_t group;
        uint32_t high_part, low_part;
        uint8_t h0, h1, h2, h3;
        uint16_t x0, x1, x2, x3;

        while (acc_len < 47) {
            acc = (acc << 8) | *buf++;
            acc_len += 8;
        }

        acc_len -= 47;
        group = (acc >> acc_len) & ((1ULL << 47) - 1);

        high_part = (uint32_t)(group >> 24);
        low_part = (uint32_t)(group & ((1ULL << 24) - 1));

        h3 = (uint8_t)(high_part / (53U * 53U * 53U));
        high_part %= 53U * 53U * 53U;
        h2 = (uint8_t)(high_part / (53U * 53U));
        high_part %= 53U * 53U;
        h1 = (uint8_t)(high_part / 53U);
        h0 = (uint8_t)(high_part % 53U);

        x0 = (uint16_t)((uint16_t)h0 << 6) | ((low_part >> 0) & 0x3F);
        x1 = (uint16_t)((uint16_t)h1 << 6) | ((low_part >> 6) & 0x3F);
        x2 = (uint16_t)((uint16_t)h2 << 6) | ((low_part >> 12) & 0x3F);
        x3 = (uint16_t)((uint16_t)h3 << 6) | ((low_part >> 18) & 0x3F);

        x[i] = x0;
        x[i + 1] = x1;
        x[i + 2] = x2;
        x[i + 3] = x3;

        r &= (uint32_t)(x0 - 3329) >> 31;
        r &= (uint32_t)(x1 - 3329) >> 31;
        r &= (uint32_t)(x2 - 3329) >> 31;
        r &= (uint32_t)(x3 - 3329) >> 31;
    }

    acc &= ((1ULL << acc_len) - 1);
    r &= ~(acc | -acc);

    return in_len & -(size_t)r;
}


/*
 * Ciphertext encoding format: 5 values are encoded over 38 bits.
 * Each value is in -96..+96; we first add 96 to get a value in 0..192.
 * Values are then encoded in base 193; 193^5 is slightly below 2^38.
 *
 * Remaining values (0 to 4) are encoded with 8 bits per value.
 */

/*
 * Encode five values x[0]..x[4] into 38 bits (bits 0..31 are returned
 * in *lo, bits 32..37 are the function return value).
 */
static inline uint32_t
encode_ct_5(uint32_t *lo, const int8_t *c)
{
	uint32_t wl, wh, x, y;

	/*
	 * First four values fit in 30 bits.
	 */
	wl = (uint32_t)(c[0] + 96)
		+ 193 * (uint32_t)(c[1] + 96)
		+ (193 * 193) * (uint32_t)(c[2] + 96)
		+ (193 * 193 * 193) * (uint32_t)(c[3] + 96);

	/*
	 * We want to add 193^4 * x (with x = c[4] + 96, in 0..192).
	 * Multiplication result may be above 2^32, so we need to split
	 * things in order to remain on pure 32-bit code. Note that:
	 *
	 *   193^4 = 0x52B36301 = (0x52B363 * 256) + 1
	 *
	 * Therefore, for 0 <= x <= 192:
	 *
	 *   x * (193^4) = (0x52B363 * x) * 256 + x
	 *
	 * Note that 0x52B363 * x will be less than 2^30, and the final
	 * addition of x cannot propagate any carry into bits 8+.
	 */
	x = (uint32_t)(c[4] + 96);
	y = 0x52B363 * x;
	wh = y >> 24;
	y = (y << 8) + x;

	/*
	 * Addition of y with wl may trigger a carry. Note that wl, at
	 * this point, is at most 193^4 - 1, which is lower than 2^30;
	 * thus, if a carry occurs, then the result will be less than
	 * 2^30 as well. We know a carry occured if and only if bit 31
	 * of y is zero AND bit 31 of y - wl is 1.
	 */
	y += wl;
	wh += (uint32_t)((y - wl) & ~y) >> 31;

	*lo = y;
	return wh;
}


/* see inner.h */
size_t
ctl_encode_ciphertext_3329(void *out, size_t max_out_len,
    const int16_t *c, unsigned logn)
{
    size_t n, out_len, i;
    uint8_t *buf;
    uint64_t acc;
    unsigned acc_len;

    n = (size_t)1 << logn;
    out_len = (n / 4) * 36 / 8;

    if (out == NULL) {
        return out_len;
    }
    if (max_out_len < out_len) {
        return 0;
    }

    buf = out;
    acc = 0;
    acc_len = 0;

    for (i = 0; i < n; i += 4) {
        uint32_t high_part;
        uint32_t low_part;
        int16_t c0, c1, c2, c3;
        uint8_t h0, h1, h2, h3;
        uint8_t l0, l1, l2, l3;
        uint32_t z0, z1, z2, z3;

        c0 = c[i];
        c1 = c[i + 1];
        c2 = c[i + 2];
        c3 = c[i + 3];

        if (c0 < -208 || c0 > 208 || c1 < -208 || c1 > 208 ||
            c2 < -208 || c2 > 208 || c3 < -208 || c3 > 208) {
            return 0;
        }

        z0 = (uint32_t)(c0 + 208);
        z1 = (uint32_t)(c1 + 208);
        z2 = (uint32_t)(c2 + 208);
        z3 = (uint32_t)(c3 + 208);

        h0 = (uint8_t)(z0 >> 6);
        h1 = (uint8_t)(z1 >> 6);
        h2 = (uint8_t)(z2 >> 6);
        h3 = (uint8_t)(z3 >> 6);

        l0 = (uint8_t)(z0 & 0x3F);
        l1 = (uint8_t)(z1 & 0x3F);
        l2 = (uint8_t)(z2 & 0x3F);
        l3 = (uint8_t)(z3 & 0x3F);

        high_part = h0 + 7U * (h1 + 7U * (h2 + 7U * h3));

        low_part = l0 | (l1 << 6) | (l2 << 12) | (l3 << 18);

        acc = (acc << 36) | ((uint64_t)high_part << 24) | low_part;
        acc_len += 36;

        while (acc_len >= 8) {
            acc_len -= 8;
            *buf++ = (uint8_t)(acc >> acc_len);
        }
    }

    if (acc_len > 0) {
        *buf++ = (uint8_t)(acc << (8 - acc_len));
    }

    return out_len;
}


/* see inner.h */
size_t
ctl_decode_ciphertext_3329(int16_t *c, unsigned logn,
    const void *in, size_t max_in_len)
{
    size_t n, in_len, i;
    const uint8_t *buf;
    uint64_t acc;
    unsigned acc_len;
    uint32_t r;

    n = (size_t)1 << logn;
    in_len = (n / 4) * 36 / 8;

    if (max_in_len < in_len) {
        return 0;
    }

    buf = in;
    acc = 0;
    acc_len = 0;
    r = 1;

    for (i = 0; i < n; i += 4) {
        uint64_t group;
        uint32_t high_part, low_part;
        uint8_t h0, h1, h2, h3;
        uint8_t l0, l1, l2, l3;
        int32_t z0, z1, z2, z3;

        while (acc_len < 36) {
            acc = (acc << 8) | *buf++;
            acc_len += 8;
        }

        acc_len -= 36;
        group = (acc >> acc_len) & ((1ULL << 36) - 1);

        high_part = (uint32_t)(group >> 24);
        low_part = (uint32_t)(group & ((1ULL << 24) - 1));

        h3 = (uint8_t)(high_part / (7U * 7U * 7U));
        high_part %= 7U * 7U * 7U;
        h2 = (uint8_t)(high_part / (7U * 7U));
        high_part %= 7U * 7U;
        h1 = (uint8_t)(high_part / 7U);
        h0 = (uint8_t)(high_part % 7U);

        l0 = (uint8_t)(low_part >> 0) & 0x3F;
        l1 = (uint8_t)(low_part >> 6) & 0x3F;
        l2 = (uint8_t)(low_part >> 12) & 0x3F;
        l3 = (uint8_t)(low_part >> 18) & 0x3F;

        z0 = (int32_t)h0 * 64 + (int32_t)l0;
        z1 = (int32_t)h1 * 64 + (int32_t)l1;
        z2 = (int32_t)h2 * 64 + (int32_t)l2;
        z3 = (int32_t)h3 * 64 + (int32_t)l3;

        r &= (uint32_t)((z0 - 417) >> 31);
        r &= (uint32_t)((z1 - 417) >> 31);
        r &= (uint32_t)((z2 - 417) >> 31);
        r &= (uint32_t)((z3 - 417) >> 31);

        c[i] = (int16_t)z0 - 208;
        c[i + 1] = (int16_t)z1 - 208;
        c[i + 2] = (int16_t)z2 - 208;
        c[i + 3] = (int16_t)z3 - 208;
    }

    acc &= ((1ULL << acc_len) - 1);
    r &= ~(acc | -acc);

    return in_len & -(size_t)r;
}
