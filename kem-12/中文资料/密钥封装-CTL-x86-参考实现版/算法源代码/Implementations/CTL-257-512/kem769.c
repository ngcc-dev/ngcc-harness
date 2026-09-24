#include "inner.h"

/*
 * We use the mod 769 code here.
 */
#define Q   769
#include "modgen.c"

/* see inner.h */
int
ctl_make_public_769(uint16_t *h, const int8_t *f, const int8_t *g,
	unsigned logn, uint32_t *tmp)
{
	size_t u, n;
	uint16_t *ft, *gt;

	n = (size_t)1 << logn;
	ft = (uint16_t *)tmp;
	gt = ft + n;
	for (u = 0; u < n; u ++) {
		ft[u] = mq_set(f[u]);
		gt[u] = mq_set(g[u]);
	}
	NTT(ft, ft, logn);
	if (!mq_poly_inv_ntt(ft, ft, logn)) {
		return 0;
	}
	NTT(gt, gt, logn);
	mq_poly_mul_ntt(ft, ft, gt, logn);
	iNTT(ft, ft, logn);
	for (u = 0; u < n; u ++) {
		h[u] = mq_unorm(ft[u]);
	}
	return 1;
}

/* see inner.h */
uint32_t
ctl_encrypt_769(int8_t *c, const uint8_t *sbuf,
	const uint16_t *h, unsigned logn, uint32_t *tmp)
{
	size_t u, n;
	uint16_t *t1, *t2;
	uint32_t e2norm;

	n = (size_t)1 << logn;
	t1 = (uint16_t *)tmp;
	t2 = t1 + n;

	/*
	 * Get h into t2, in NTT representation.
	 */
	for (u = 0; u < n; u ++) {
		t2[u] = mq_set(h[u]);
	}
	NTT(t2, t2, logn);

	/*
	 * Coefficients of polynomial s are in {0,1}, extracted from sbuf[].
	 */
	for (u = 0; u < n; u ++) {
		t1[u] = mq_set((sbuf[u >> 3] >> (u & 7)) & 1);
	}

	/*
	 * t1 <- h*s
	 * We have the NTT representation of h in t2.
	 */
	NTT(t1, t1, logn);
	mq_poly_mul_ntt(t1, t1, t2, logn);
	iNTT(t1, t1, logn);

	/*
	 * c <- round((h*s mod q)/k)
	 * (for q = 769, we have k = 4).
	 *
	 * Rounding is toward +infty, so that error e = k*c - (h*s mod q)
	 * has coefficients in -1..+2 only.
	 *
	 * We also compute in e2norm the sum of the squares of the coefficients
	 * of 2*e', where e' = e - E(e) = e - 1/2*(1+X+X^2+X^3+...+X^(n-1)).
	 */
	e2norm = 0;
	for (u = 0; u < n; u ++) {
		int y, z, ep;

		y = mq_snorm(t1[u]);
		z = ((y + 386) >> 2) - 96;
		c[u] = z;
		ep = (4 * z - y);
		ep = 2 * ep - 1;
		e2norm += (uint32_t)(ep * ep);
	}

	/*
	 * Ciphertext is acceptable if and only if the norm
	 * of (gamma*s',e') is not greater than
	 * 1.08*sqrt((n/2)*gamma^2). Note that e' and s' are
	 * centered on 0, i.e. e' = e - E(e) and s' = s - E(s).
	 * With k = 4, we have gamma = sqrt(5).
	 *
	 * Coefficients of s are in {0,1}, and E(s) = 1/2, therefore
	 * coefficients of s' are in {-1/2,+1/2}. Thus, the sum
	 * of the squares of the coefficients of gamma*s' is always
	 * n*(1/4)*gamma^2 = 5*n/4.
	 *
	 * Coefficients of e are in {-1,0,1,2} and E(e) = 1/2, therefore
	 * coefficients of e' are in {-3/2,-1/2,+1/2,+3/2}. We computed
	 * the sum of the squares of the coefficients of 2*e' in e2norm.
	 *
	 * Thus, the ciphertext is acceptable if and only if:
	 *    5*n/4 + e2norm/4 <= (1.08*sqrt((n/2)*gamma^2))^2
	 * which we rewrite as:
	 *    5*n + e2norm <= 11.664*n
	 * Note that 11.664*n is not an integer for any of the supported
	 * degrees (2 to 1024). For logn in 1..10:
	 *    floor(11.664*2^logn) = floor(11943 / 2^(10-logn))
	 *
	 * We need the comparison to be constant-time, thus we avoid
	 * using a direct comparison operator (which may be translated
	 * by the C compiler into a conditional jump on some architectures).
	 */
	e2norm = (5u << logn) + e2norm - 1u - (11943u >> (10 - logn));
	return e2norm >> 31;
}

/* see inner.h */
void
ctl_decrypt_769(uint8_t *sbuf, const int8_t *c,
	const int8_t *f, const int8_t *g, const int8_t *F, const int8_t *G,
	const int32_t *w, unsigned logn, uint32_t *tmp)
{
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
	 * When q = 769, we have Q = 2 and k = 4. This simplifies some
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
	 * ("Decapsulate"). With q = 769, we have k = 4. Moreover, we
	 * want Q*c, with Q = 2; thus, we compute 8*c here.
	 */
	for (u = 0; u < n; u ++) {
		t1[u] = mq_set(8 * c[u]);
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
		t3[u] = mq_set((int32_t)F[u] * (64513 % 769));
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
		t3[u] = mq_set((int32_t)G[u] * (64513 % 769));
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
	 *  - if the parity is wrong, we must add 769 (if the value is
	 *    negative or zero) or subtract 769 (if the value is strictly
	 *    positive), so that the result is in -768..+769.
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
		 * the wrong parity, then we must add or subtract 769.
		 * We add 769 if the value is negative or zero, subtract
		 * 769 if it is negative.
		 */
		x += -(uint32_t)((x ^ cp2) & 1u)
			& -(uint32_t)769
			& (((x - 1) >> 16) & (2 * 769));

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
		uint32_t y0, y1, x;

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

		/*
		 * The Montgomery representation of 1/q' mod q is 602
		 * (with q = 769 and q' = 64513). We add 64596 = 84*769 to
		 * ensure that the value provided to mq_montyred() is in
		 * the proper range (max value will be 602*(768+64596)).
		 */
		x = mq_montyred(602 * (64596 + y0 - y1));

		/*
		 * Value x is in 1..q range. We need to normalize value
		 * q to 0.
		 */
		x &= (uint32_t)(x - Q) >> 16;

		/*
		 * Compute value modulo q*q', in 0..q*q'-1 range.
		 */
		x = (x * 64513) + (uint32_t)y1;

		/*
		 * If x = 0, set it to q*q' = 49610497.
		 */
		x += 49610497 & -((uint32_t)(x - 1) >> 31);

		/*
		 * Adjust parity to get value modulo 2*q*q': we subtract
		 * q*q' = 49610497 if the value has the wrong parity.
		 */
		x -= 49610497 & -(uint32_t)((x & 1) ^ cs2);

		/*
		 * Value is now in -49610496..+49610497, which is the
		 * correct normalized range. Since we will hand it over
		 * to the module that computes modulo 257, we pre-reduce
		 * it modulo 257. We ensure a positive value by adding
		 * 49610252 = 257 * 193036.
		 */
		t1[u] = m257_tomonty(x + 49610252);
	}

	/*
	 * We now have c' and c'' in t2 and t1, respectively. c' uses
	 * signed integers; c'' is in Montgomery representation modulo 257.
	 * We convert c' the Montgomery representation modulo 257 as
	 * well.
	 */
	for (u = 0; u < n; u ++) {
		t2[u] = m257_tomonty(*(int16_t *)&t2[u] + 257);
	}

	/*
	 * We need to compute:
	 *   Fd = q'*F - f*w
	 *   s' = 1/(q*q'*Q) (Fd*c' - f*c'')
	 *   s = s' + (1/2)*ones
	 *
	 * We use the mod 257 code to obtain q*q'*Q*s'.
	 */
	ctl_finish_decapsulate_257(t2, t1, f, F, w, logn, (uint32_t *)t3);

	/*
	 * If the ciphertext is correct and the decapsulation worked well,
	 * then s' has coefficients in {-1/2,1/2} and the coefficients of
	 * s are obtained by adding 1/2. We have the coefficients of
	 * q*q'*Q*s' in t2[], in Montgomery representation modulo 257:
	 *
	 *    s'   s   t2[]
	 *  -1/2   0    12    (-q*q'*Q/2 = 12 mod 257)
	 *  +1/2   1   245    (+q*q'*Q/2 = 245 mod 257)
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
ctl_finish_decapsulate_769(uint16_t *cp, uint16_t *cs,
	const int8_t *f, const int8_t *F, const int32_t *w, unsigned logn,
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
		/*
		 * 87666 = 769 * 114.
		 * This addition ensures that (q' mod q)*F[u] becomes a
		 * positive integer, thus in range for mq_tomonty().
		 */
		t2[u] = mq_tomonty((int32_t)F[u] * (64513 % 769) + 87666);
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
ctl_rebuild_G_769(int8_t *G,
	const int8_t *f, const int8_t *g, const int8_t *F,
	unsigned logn, uint32_t *tmp)
{
	size_t u, n;
	uint16_t *t1, *t2;
	int lim;

	n = (size_t)1 << logn;
	t1 = (uint16_t *)tmp;
	t2 = t1 + n;

	/*
	 * Load g in t1 and F in t2.
	 */
	for (u = 0; u < n; u ++) {
		t1[u] = mq_set(g[u]);
		t2[u] = mq_set(F[u]);
	}

	/*
	 * Get g*F into t1, in NTT representation.
	 */
	NTT(t1, t1, logn);
	NTT(t2, t2, logn);
	mq_poly_mul_ntt(t1, t1, t2, logn);

	/*
	 * Load f in t2.
	 */
	for (u = 0; u < n; u ++) {
		t2[u] = mq_set(f[u]);
	}

	/*
	 * Compute G in t1 (NTT).
	 */
	NTT(t2, t2, logn);
	if (!mq_poly_inv_ntt(t2, t2, logn)) {
		return 0;
	}
	mq_poly_mul_ntt(t1, t1, t2, logn);

	/*
	 * Get back the coefficients of G. The coefficients are verified
	 * to remain within the expected bound.
	 */
	iNTT(t1, t1, logn);
	lim = (1 << (ctl_max_FG_bits[logn] - 1)) - 1;
	for (u = 0; u < n; u ++) {
		int x;

		x = mq_snorm(t1[u]);
		if (x < -lim || x > +lim) {
			return 0;
		}
		G[u] = (int8_t)x;
	}
	return 1;
}

/*
 * Encoding format: 5 values modulo 769 are encoded over 48 bits.
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


/*
 * Decode a 48-bit word (lo is bits 0..31, hi is bits 32..47)
 * into five values. Returned value is 1 on success, 0 on error.
 */
static uint32_t
decode5(uint16_t *x, uint32_t lo, uint32_t hi)
{
	/*
	 * Useful facts:
	 *    512 = 27 mod 97
	 *    2^19 = 3 mod 97
	 *    for any x in 0..57519, (43241 * x) >> 22 = floor(x / 97)
	 *    97 * 1594008481 = 1 mod 2^32
	 */
	uint32_t a;
	int i;

	/*
	 * First element: we need to compute the quotient and
	 * remainder of the high 33-bit chunk divided by 97.
	 *
	 * We first compute the remainder.
	 */
	a = (lo >> 15) + ((hi & 0x03) << 17)
		+ ((hi >> 2) * 3);             /* 0 <= a <= 573436 */
	a = (a & 0x1FF) + ((a >> 9) * 27);     /* 0 <= a <= 30723 */
	a -= 97 * ((43241 * a) >> 22);
	x[0] = (lo & 0x07) + (a << 3);

	/*
	 * Subtract the remainder from the high 33-bit chunk, and divide by
	 * 97. We can work modulo 2^32 since the division is exact, and the
	 * result fits on 32 bits. From now on, we keep the chunk in 'hi'.
	 */
	hi = (hi << 17) | (lo >> 15);
	hi = (hi - a) * 1594008481u;

	/*
	 * Second element is similar, except that we work on a
	 * 32-bit value now, stored in 'hi' only. Maximum value for 'hi'
	 * is floor((2^33-1) / 97) = 88556026.
	 */
	a = (hi & 0xFFFF) + ((hi >> 16) * 61);  /* 0 <= a <= 147945 */
	a = (a & 0x1FF) + ((a >> 9) * 27);      /* 0 <= a <= 8286 */
	a -= 97 * ((43241 * a) >> 22);
	x[1] = ((lo >> 3) & 0x07) + (a << 3);
	hi = (hi - a) * 1594008481u;

	/*
	 * Third element: maximum value for 'hi' is
	 * floor((2^33-1) / (97^2)) = 912948.
	 */
	a = (hi & 0x0FFF) + ((hi >> 12) * 22);  /* 0 <= a <= 8978 */
	a -= 97 * ((43241 * a) >> 22);
	x[2] = ((lo >> 6) & 0x07) + (a << 3);
	hi = (hi - a) * 1594008481u;

	/*
	 * Fourth element: now, hi <= 9411. We can use a simpler approach.
	 */
	a = (43241 * hi) >> 22;
	x[3] = ((lo >> 9) & 0x07) + ((hi - (97 * a)) << 3);
	hi = a;

	/*
	 * Fifth element: now, hi <= 97. We simply use its value; if it
	 * is off-range, this will be detected in the final test.
	 */
	x[4] = ((lo >> 12) & 0x07) + (hi << 3);

	/*
	 * Decoding is correct if all decoded values are less than 769.
	 */
	a = (uint32_t)-1;
	for (i = 0; i < 5; i ++) {
		a &= (uint32_t)(x[i] - 769);
	}
	return a >> 31;
}


/* see inner.h */
size_t
ctl_encode_769(void *out, size_t max_out_len,
	const uint16_t *x, unsigned logn)
{
	size_t out_len, u, v, n;
	uint8_t *buf;
	uint32_t acc;
	int acc_len;

	n = (size_t)1 << logn;
	out_len = 6 * (n / 5) + (n % 5) + 1;
	if (out == NULL) {
		return out_len;
	}
	if (max_out_len < out_len) {
		return 0;
	}

	buf = out;
	switch (logn) {
	case 1:
		buf[0] = (uint8_t)x[0];
		buf[1] = (uint8_t)((x[0] >> 8) | ((unsigned)x[1] << 2));
		buf[2] = (uint8_t)(x[1] >> 6);
		return 3;
	case 2:
		buf[0] = (uint8_t)x[0];
		buf[1] = (uint8_t)((x[0] >> 8) | ((unsigned)x[1] << 2));
		buf[2] = (uint8_t)((x[1] >> 6) | ((unsigned)x[2] << 4));
		buf[3] = (uint8_t)((x[2] >> 4) | ((unsigned)x[3] << 6));
		buf[4] = (uint8_t)(x[3] >> 2);
		return 5;
	default:
		v = 0;
		u = 0;
		for (; (u + 5) <= n; u += 5) {
			uint32_t lo, hi;

			hi = encode5(&lo, x + u);
			enc32le(buf + v, lo);
			enc16le(buf + v + 4, hi);
			v += 6;
		}
		acc = 0;
		acc_len = 0;
		while (u < n) {
			acc |= (uint32_t)x[u ++] << acc_len;
			buf[v ++] = (uint8_t)acc;
			acc >>= 8;
			acc_len += 2;
		}
		if (acc_len != 0) {
			buf[v ++] = (uint8_t)acc;
		}
		return v;
	}
}

/* see inner.h */
size_t
ctl_decode_769(uint16_t *x, unsigned logn,
	const void *in, size_t max_in_len)
{
	const uint8_t *buf;
	uint32_t r;
	size_t in_len, u, n;

	buf = in;
	n = (size_t)1 << logn;
	in_len = 6 * (n / 5) + (n % 5) + 1;
	if (max_in_len < in_len) {
		return 0;
	}
	u = 0;
	r = 1;
	for (; (u + 5) <= n; u += 5) {
		r &= decode5(x + u, dec32le(buf), dec16le(buf + 4));
		buf += 6;
	}
	if (u < n) {
		uint32_t acc;
		int acc_len;

		acc = *buf ++;
		acc_len = 8;
		while (u < n) {
			uint32_t z;

			acc |= (uint32_t)(*buf ++) << acc_len;
			z = acc & 0x3FF;
			acc >>= 10;
			acc_len -= 2;
			x[u ++] = z;
			r &= (uint32_t)(z - 769) >> 31;
		}
		r &= (uint32_t)(acc - 1) >> 31;
	}

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


/*
 * Decode a 38-bit word (lo is bits 0..31, hi is bits 32..37)
 * into five values. Returned value is 1 on success, 0 on error.
 */
static uint32_t
decode_ct_5(int8_t *c, uint32_t lo, uint32_t hi)
{
	/*
	 * Useful facts:
	 *
	 *   2^8 = 63 mod 193
	 *   2^16 = 109 mod 193
	 *   2^24 = 112 mod 193
	 *   2^32 = 108 mod 193
	 *   (x * 43465) >> 23 == floor(x / 193)  for all x in 0..61372.
	 *
	 * General processing is the following:
	 *
	 *  - Input is an integer z.
	 *  - We find a = z mod 193, which yields the next ciphertext value.
	 *  - z <- (z - a) / 193. This is an exact division, and the result
	 *    fits on 32 bits, so we can compute it by multiplying z
	 *    with 2425655105, which is the inverse of 193 modulo 2^32.
	 *    We can even do that by working with z mod 2^32.
	 */
	uint32_t a;

	/*
	 * First iteration: lo + 2^32*hi <= 2^38-1
	 */
	a = (lo & 0xFF)
		+ 63 * ((lo >> 8) & 0xFF)
		+ 109 * ((lo >> 16) & 0xFF)
		+ 112 * ((lo >> 24) & 0xFF)
		+ 108 * hi;                    /* a = 0..79479 */
	a = (a & 0x3FF) + 59 * (a >> 10);      /* a = 0..5565 */
	a -= 193 * ((a * 43465) >> 23);
	c[0] = (int)a - 96;
	lo = (lo - a) * 2425655105u;

	/*
	 * Second iteration: lo <= floor((2^38-1) / 193) = 1424237859
	 */
	a = (lo & 0xFF)
		+ 63 * ((lo >> 8) & 0xFF)
		+ 109 * ((lo >> 16) & 0xFF)
		+ 112 * ((lo >> 24) & 0xFF);   /* a = 0..53522 */
	a -= 193 * ((a * 43465) >> 23);
	c[1] = (int)a - 96;
	lo = (lo - a) * 2425655105u;

	/*
	 * Third iteration: lo <= floor((2^38-1) / 193^2) = 7379470
	 */
	a = (lo & 0xFF)
		+ 63 * ((lo >> 8) & 0xFF)
		+ 109 * ((lo >> 16) & 0xFF);   /* a = 0..28527 */
	a -= 193 * ((a * 43465) >> 23);
	c[2] = (int)a - 96;
	lo = (lo - a) * 2425655105u;

	/*
	 * Fourth iteration: lo <= floor((2^38-1) / 193^3) = 38235
	 */
	a = (lo * 43465) >> 23;
	c[3] = (int)(lo - 193 * a) - 96;
	lo = a;

	/*
	 * Fifth iteration: lo <= 198.
	 * Decoding is correct if and only if lo <= 192 at this point.
	 */
	c[4] = (int)lo - 96;
	return (uint32_t)(lo - 193) >> 31;
}


/* see inner.h */
size_t
ctl_encode_ciphertext_769(void *out, size_t max_out_len,
	const int8_t *c, unsigned logn)
{
	size_t out_len, u, v, n;
	uint8_t *buf;
	uint32_t acc;
	int acc_len;

	n = (size_t)1 << logn;
	out_len = (((n / 5) * 38) + 8 * (n % 5) + 7) >> 3;
	if (out == NULL) {
		return out_len;
	}
	if (max_out_len < out_len) {
		return 0;
	}

	/*
	 * buf = output buffer
	 * v = current offset into output buffer
	 * acc = buffered bits
	 * acc_len = number of buffered bits (0..31)
	 * Note: unused bits in acc are zero.
	 */
	buf = out;
	v = 0;
	acc = 0;
	acc_len = 0;
	u = 0;


	/*
	 * Encode full groups of 5 values; each yields 38 bits.
	 */
	for (; (u + 5) <= n; u += 5) {
		uint32_t lo, hi;

		hi = encode_ct_5(&lo, c + u);
		acc |= (lo << acc_len);
		enc32le(buf + v, acc);
		v += 4;
		acc = (lo >> (31 - acc_len)) >> 1;
		acc |= hi << acc_len;
		acc_len += 6;
		if (acc_len >= 32) {
			enc32le(buf + v, acc);
			v += 4;
			acc_len -= 32;
			acc = hi >> (6 - acc_len);
		}
	}

	/*
	 * Encode remaining values, 8 bits per value.
	 */
	for (; u < n; u ++) {
		uint32_t w;

		w = (uint32_t)(c[u] + 96);
		acc |= w << acc_len;
		acc_len += 8;
		if (acc_len >= 32) {
			enc32le(buf + v, acc);
			v += 4;
			acc_len -= 32;
			acc = w >> (8 - acc_len);
		}
	}

	/*
	 * Flush buffered bits.
	 */
	while (acc_len > 0) {
		buf[v ++] = (uint8_t)acc;
		acc >>= 8;
		acc_len -= 8;
	}
	return v;
}

/* see inner.h */
size_t
ctl_decode_ciphertext_769(int8_t *c, unsigned logn,
	const void *in, size_t max_in_len)
{
	const uint8_t *buf;
	uint32_t r, acc;
	size_t in_len, u, n;
	int acc_len;

	buf = in;
	n = (size_t)1 << logn;
	in_len = (((n / 5) * 38) + 8 * (n % 5) + 7) >> 3;
	if (max_in_len < in_len) {
		return 0;
	}
	u = 0;

	r = 1;

	/*
	 * Decode all blocks of 20 values (19 bytes exactly).
	 */
	for (; (u + 20) <= n; u += 20) {
		uint32_t w0, w1, w2, w3, w4;

		w0 = dec32le(buf);
		w1 = dec32le(buf + 4);
		w2 = dec32le(buf + 8);
		w3 = dec32le(buf + 12);
		w4 = dec24le(buf + 16);
		r &= decode_ct_5(c + u,
			w0, w1 & 0x3F);
		r &= decode_ct_5(c + u + 5,
			(w1 >> 6) | (w2 << 26), (w2 >> 6) & 0x3F);
		r &= decode_ct_5(c + u + 10,
			(w2 >> 12) | (w3 << 20), (w3 >> 12) & 0x3F);
		r &= decode_ct_5(c + u + 15,
			(w3 >> 18) | (w4 << 14), w4 >> 18);
		buf += 19;
	}

	/*
	 * Decode the full blocks of 5 values (38 bits). This requires
	 * some bit buffering management.
	 */
	acc = 0;
	acc_len = 0;
	for (; u + 5 <= n; u += 5) {
		uint32_t lo, hi;

		lo = acc;
		acc = dec32le(buf);
		buf += 4;
		lo |= (acc << acc_len);
		acc >>= acc_len;
		if (acc_len < 6) {
			acc |= (uint32_t)(*buf ++) << acc_len;
			acc_len += 8;
		}
		hi = acc & 0x3F;
		acc >>= 6;
		acc_len -= 6;
		r &= decode_ct_5(c + u, lo, hi);
	}

	/*
	 * Remaining values use 8 bits each.
	 */
	for (; u < n; u ++) {
		uint32_t w;

		if (acc_len < 8) {
			acc |= (uint32_t)(*buf ++) << acc_len;
		} else {
			acc_len -= 8;
		}
		w = acc & 0xFF;
		acc >>= 8;
		c[u] = (int)w - 96;
		r &= (uint32_t)(w - 193) >> 31;
	}

	/*
	 * There may be a few unused bits; if any is non-zero, then the
	 * input is invalid.
	 */
	r &= (uint32_t)(acc - 1) >> 31;

	return in_len & -(size_t)r;
}
