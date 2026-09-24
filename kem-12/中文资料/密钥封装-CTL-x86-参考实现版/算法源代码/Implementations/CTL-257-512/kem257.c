#include "inner.h"

/*
 * We use the mod 257 code here.
 */
#define Q   257
#include "modgen.c"

static inline uint32_t
mq_set_large_i16(int32_t x)
{
	x %= 257;
	if (x < 0) {
		x += 257;
	}
	return mq_tomonty((uint32_t)x);
}

/* see inner.h */
int
ctl_make_public_257(uint16_t *h, const int8_t *f, const int8_t *g,
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
ctl_encrypt_257(int8_t *c, const uint8_t *sbuf,
	const uint16_t *h, unsigned logn, uint32_t *tmp)
{
	size_t u, n;
	uint16_t *t1, *t2;

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
	 * Coefficients of polynomial s are {0,1}, extracted from sbuf[].
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
	 * (for q = 257, we have k = 2).
	 *
	 * Rounding is toward +infty, so that error e = k*c - (h*s mod q)
	 * has coefficients in {0, 1} only.
	 */
	for (u = 0; u < n; u ++) {
		c[u] = ((mq_snorm(t1[u]) + 129) >> 1) - 64;
	}

	/*
	 * Ciphertext is acceptable if and only if the norm
	 * of (gamma*s',e') is not greater than
	 * 1.08*sqrt((n/2)*gamma^2). Note that e' and s' are
	 * centered on 0, i.e. e' = e - E(e) and s' = s - E(s).
	 * With k = 2, we have gamma = 1, and all coefficients
	 * of e' and s' can only be 1/2 or -1/2. Therefore,
	 * the norm of (gamma*s',e') can only be sqrt(n/2),
	 * therefore always below the bound.
	 */
	return 1;
}

/* see inner.h */
void
ctl_decrypt_257(uint8_t *sbuf, const int8_t *c,
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
	 * When q = 257, we have Q = 2 and k = 2. This simplifies some
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
	 * possible values for the coefficients of s' and e' (these
	 * coefficients can only be 0 or 1). Thus, we can do the
	 * computation modulo any prime p which is not 2, q or q'; in
	 * practice, we use the "other q" (i.e. 769) since we already
	 * have the code for computations modulo that prime.
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
	 * ("Decapsulate"). With q = 257, we have k = 2. Moreover, we
	 * want Q*c, with Q = 2; thus, we compute 4*c here.
	 */
	for (u = 0; u < n; u ++) {
		t1[u] = mq_set(4 * c[u]);
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
		t3[u] = mq_set((int32_t)F[u] * (64513 % 257));
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
		t3[u] = mq_set((int32_t)G[u] * (64513 % 257));
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
	 *  - coefficient must be normalized to -128..+128
	 *  - if the parity is wrong, we must add 257 (if the value is
	 *    negative or zero) or subtract 257 (if the value is strictly
	 *    positive), so that the result is in -256..+257.
	 */
	for (u = 0; u < n; u ++) {
		uint32_t x;

		/*
		 * Get next coefficient of c' into x, normalized in
		 * -128..+128.
		 */
		x = (uint32_t)mq_snorm(t2[u]);

		/*
		 * Adjust x to get the right value modulo 2: if it has
		 * the wrong parity, then we must add or subtract 257.
		 * We add 257 if the value is negative or zero, subtract
		 * 257 if it is negative.
		 */
		x += -(uint32_t)((x ^ cp2) & 1u)
			& -(uint32_t)257
			& (((x - 1) >> 16) & (2 * 257));

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
		 * The Montgomery representation of 1/q' mod q is 43
		 * (with q = 257 and q' = 64513). We add 64764 = 252*257 to
		 * ensure that the value provided to mq_montyred() is in
		 * the proper range (max value will be 43*(256+64764)).
		 */
		x = mq_montyred(43 * (64764 + y0 - y1));

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
		 * If x = 0, set it to q*q' = 16579841.
		 */
		x += 16579841 & -((uint32_t)(x - 1) >> 31);

		/*
		 * Adjust parity to get value modulo 2*q*q': we subtract
		 * q*q' = 16579841 if the value has the wrong parity.
		 */
		x -= 16579841 & -(uint32_t)((x & 1) ^ cs2);

		/*
		 * Value is now in -8289920..+8289920, which is the correct
		 * normalized range. Since we will hand it over to the
		 * module that computes modulo 769, we pre-reduce it
		 * modulo 769. We ensure a positive value by adding
		 * 8290589 = 769 * 10781.
		 */
		t1[u] = m769_tomonty(x + 8290589);
	}

	/*
	 * We now have c' and c'' in t2 and t1, respectively. c' uses
	 * signed integers; c'' is in Montgomery representation modulo 769.
	 * We convert c' the Montgomery representation modulo 769 as
	 * well.
	 */
	for (u = 0; u < n; u ++) {
		t2[u] = m769_tomonty(*(int16_t *)&t2[u] + 769);
	}

	/*
	 * We need to compute:
	 *   Fd = q'*F - f*w
	 *   s' = 1/(q*q'*Q) (Fd*c' - f*c'')
	 *   s = s' + (1/2)*ones
	 *
	 * We use the mod 769 code to obtain q*q'*Q*s'.
	 */
	ctl_finish_decapsulate_769(t2, t1, f, F, w, logn, (uint32_t *)t3);

	/*
	 * If the ciphertext is correct and the decapsulation worked well,
	 * then s' has coefficients in {-1/2,1/2} and the coefficients of
	 * s are obtained by adding 1/2. We have the coefficients of
	 * q*q'*Q*s' in t2[], in Montgomery representation modulo 769:
	 *
	 *    s'   s   t2[]
	 *  -1/2   0    26    (-q*q'*Q/2 = 568 mod 769)
	 *  +1/2   1   743    (+q*q'*Q/2 = 201 mod 769)
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
ctl_finish_decapsulate_257(uint16_t *cp, uint16_t *cs,
	const int8_t *f, const int8_t *F, const int32_t *w, unsigned logn,
	uint32_t *tmp)
{
	/*
	 * Formulas:
	 *   Fd = q'*F - f*w
	 *   q*q'*Q*s = Fd*c' - f*c''
	 * We q' = 3329.
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
		 * 196862 = 257 * 766.
		 * For the int16 variant, F may use the full signed 16-bit
		 * range, so we need a much larger positive offset before
		 * passing the value to mq_tomonty().
		 */
		t2[u] = mq_tomonty((int32_t)F[u] * (64513 % 257) + 196862);
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
void
ctl_finish_decapsulate_257_i16(uint16_t *cp, uint16_t *cs,
	const int8_t *f, const int16_t *F, const int32_t *w, unsigned logn,
	uint32_t *tmp)
{
	size_t u, n;
	uint16_t *t1, *t2;

	n = (size_t)1 << logn;
	t1 = (uint16_t *)tmp;
	t2 = t1 + n;

	NTT(cp, cp, logn);
	NTT(cs, cs, logn);

	for (u = 0; u < n; u ++) {
		t1[u] = mq_set(f[u]);
		t2[u] = mq_set(w[u]);
	}
	NTT(t1, t1, logn);
	NTT(t2, t2, logn);

	mq_poly_mul_ntt(cs, cs, t1, logn);
	mq_poly_mul_ntt(t1, t1, t2, logn);

	for (u = 0; u < n; u ++) {
		t2[u] = mq_set_large_i16((int32_t)F[u] * (64513 % 257));
	}
	NTT(t2, t2, logn);

	mq_poly_sub(t1, t2, t1, logn);
	mq_poly_mul_ntt(t1, t1, cp, logn);
	mq_poly_sub(t1, t1, cs, logn);
	iNTT(cp, t1, logn);
}

/* see inner.h */
int
ctl_rebuild_G_257(int8_t *G,
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
 * Encoding format: 8 values modulo 257 are encoded over 65 bits.
 * Value x_i is split into its 4 low-order bits (xl_i, value in 0..15)
 * and its 5 high-order bits (xh_i, value in 0..16). First 32 bits
 * are the xl_i; then remaining 33 bits are the xh_i (digits in base-17).
 *
 * Eight 65-bit blocks are encoded by grouping the high bits into a common
 * final byte.
 *
 * For small degrees:
 *
 *  - If degree is 2, then encoding is over three bytes, 10 bits per value.
 *  - If degree is 4, then encoding is over five bytes, 10 bits per value.
 *  - If degree is 8, 16 or 32, then one, two or four 65-bit blocks are
 *    used; the ignored bits in the last byte are set to zero.
 */

/*
 * Encode eight values x[0]..x[7] into 65 bits (bits 0..31 are returned
 * in *lo, bits 32..63 in *hi, bit 64 is the function return value).
 */
static inline uint32_t
encode8(uint32_t *lo, uint32_t *hi, const uint16_t *x)
{
	int i;
	uint32_t wl, wh, r, t;

	wl = 0;
	for (i = 0; i < 8; i ++) {
		wl |= (uint32_t)(x[i] & 0x0F) << (4 * i);
	}
	*lo = wl;
	wh = 0;
	for (i = 7; i > 0; i --) {
		wh = (wh * 17) + (uint32_t)(x[i] >> 4);
	}

	/*
	 * Final iteration is:
	 *   *hi = wh*17 + (x[0] >> 4)
	 * which can be written as:
	 *   *hi = wh*16 + wh + (x[0] >> 4)
	 * There will be a carry into the top bit (bit 64 of the final
	 * output) if and only if one of the following holds:
	 *  - wh >= 2^28
	 *  - adding wh + (x[0] >> 4) triggers a carry
	 * Note that wh + (x[0] >> 4) <= 410338688 < 2^29.
	 */

	r = wh >> 28;
	t = wh + (x[0] >> 4);
	wh = (wh << 4) + t;
	r |= ((uint32_t)(wh - t) & ~wh) >> 31;
	*hi = wh;
	return r;
}


/*
 * Decode a 65-bit word (lo is bits 0..31, hi is bits 32..63, tt is bit 64)
 * into eight values. Returned value is 1 on success, 0 on error.
 */
static uint32_t
decode8(uint16_t *x, uint32_t lo, uint32_t hi, uint32_t tt)
{
	/*
	 * Useful facts:
	 *    256 = 1 mod 17
	 *    65536 = 1 mod 17
	 *    2^32 = 1 mod 17
	 *    for any x in 0..69631, (61681 * x) >> 20 = floor(x / 17)
	 *    17 * 4042322161 = 1 mod 2^32
	 */
	uint32_t a;
	int i;

	/*
	 * First iteration: we need to compute the quotient and
	 * remainder of (hi + (tt << 32)) divided by 17.
	 *
	 * We first compute the remainder.
	 */
	a = (hi & 0xFFFF) + (hi >> 16) + tt;   /* 0 <= a <= 131071 */
	a = (a & 0xFF) + (a >> 8);             /* 0 <= a <= 766 */
	a -= 17 * ((61681 * a) >> 20);         /* 0 <= a <= 16 */
	x[0] = (lo & 0x0F) + (a << 4);

	/*
	 * Subtract the remainder, then do the exact division by multiplying
	 * by the inverse of 17 modulo 2^32. We do all computations modulo
	 * 2^32, since we know that the result will be lower than 2^32 (we
	 * don't have to care about the value of tt here, it is implictly
	 * taken into account in the remainder).
	 */
	hi = (hi - a) * 4042322161u;

	/*
	 * Next four iterations are similar, except that we work on a
	 * 32-bit value now (no tt).
	 */
	for (i = 1; i <= 4; i ++) {
		a = (hi & 0xFFFF) + (hi >> 16);   /* 0 <= a <= 131070 */
		a = (a & 0xFF) + (a >> 8);        /* 0 <= a <= 766 */
		a -= 17 * ((61681 * a) >> 20);    /* 0 <= a <= 16 */
		x[i] = ((lo >> (4 * i)) & 0x0F) + (a << 4);
		hi = (hi - a) * 4042322161u;
	}

	/*
	 * At that point, max value for hi is 6049. The next two iterations
	 * can be done with simpler computations.
	 */
	for (i = 5; i <= 6; i ++) {
		a = (61681 * hi) >> 20;
		x[i] = ((lo >> (4 * i)) & 0x0F) + ((hi - 17 * a) << 4);
		hi = a;
	}

	/*
	 * We use the final value for 'hi' as-is; if it is off-range, this
	 * will be detected in the final test.
	 */
	x[7] = (lo >> 28) + (hi << 4);

	/*
	 * Decoding is correct if all decoded values are less than 257.
	 */
	a = (uint32_t)-1;
	for (i = 0; i < 8; i ++) {
		a &= (uint32_t)(x[i] - 257);
	}
	return a >> 31;
}


/* see inner.h */
size_t
ctl_encode_257(void *out, size_t max_out_len,
	const uint16_t *x, unsigned logn)
{
	size_t out_len, u, v, n;
	uint8_t *buf;
	uint32_t xb;
	int j;

	switch (logn) {
	case 1:
		out_len = 3;
		break;
	case 2:
		out_len = 5;
		break;
	default:
		out_len = (((1u << (logn - 3)) + 7) >> 3) + (1u << logn);
		break;
	}
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
	case 3:
	case 4:
	case 5:
		n = (size_t)1 << logn;
		v = 0;
		xb = 0;
		j = 0;
		for (u = 0; u < n; u += 8) {
			uint32_t lo, hi;

			xb |= encode8(&lo, &hi, x + u) << j;
			enc32le(buf + v, lo);
			enc32le(buf + v + 4, hi);
			v += 8;
			if (++ j == 8) {
				buf[v ++] = xb;
				xb = 0;
				j = 0;
			}
		}
		if (j != 0) {
			buf[v ++] = xb;
		}
		return v;
	default:
		n = (size_t)1 << logn;
		v = 0;
		for (u = 0; u < n; u += 64) {
			xb = 0;
			for (j = 0; j < 8; j ++) {
				uint32_t lo, hi;

				xb |= encode8(&lo, &hi, x + u + (j << 3)) << j;
				enc32le(buf + v + (j << 3), lo);
				enc32le(buf + v + (j << 3) + 4, hi);
			}
			buf[v + 64] = xb;
			v += 65;
		}
		return v;
	}
}

/* see inner.h */
size_t
ctl_decode_257(uint16_t *x, unsigned logn,
	const void *in, size_t max_in_len)
{
	const uint8_t *buf;
	uint32_t r, xb;
	size_t in_len, u, n;

	buf = in;
	switch (logn) {
	case 1:
		if (max_in_len < 3) {
			return 0;
		}
		x[0] = buf[0] | ((buf[1] & 0x03) << 8);
		x[1] = (buf[1] >> 2) | (buf[2] << 6);
		in_len = 3;
		r = (((uint32_t)x[0] - 257) & ((uint32_t)x[1] - 257)) >> 31;
		break;
	case 2:
		if (max_in_len < 5) {
			return 0;
		}
		x[0] = buf[0] | ((buf[1] & 0x03) << 8);
		x[1] = (buf[1] >> 2) | ((buf[2] & 0x0F) << 6);
		x[2] = (buf[2] >> 4) | ((buf[3] & 0x3F) << 4);
		x[3] = (buf[3] >> 6) | (buf[4] << 2);
		in_len = 5;
		r = (((uint32_t)x[0] - 257)
			& ((uint32_t)x[1] - 257)
			& ((uint32_t)x[2] - 257)
			& ((uint32_t)x[3] - 257)) >> 31;
		break;
	case 3:
	case 4:
	case 5:
		n = (size_t)1 << logn;
		in_len = n + 1;
		if (max_in_len < in_len) {
			return 0;
		}
		xb = buf[n];

		/* Ignored bits in xb must be zero. */
		r = (uint32_t)((xb >> (1u << (logn - 3))) - 1) >> 31;

		/* Decode 65-bit blocks. */
		for (u = 0; u < n; u += 8) {
			r &= decode8(x + u,
				dec32le(buf + u),
				dec32le(buf + u + 4),
				(xb >> ((unsigned)u >> 3)) & 1);
		}
		break;

	default:
		n = (size_t)1 << logn;
		in_len = n + ((size_t)1 << (logn - 6));
		if (max_in_len < in_len) {
			return 0;
		}
		r = 1;
		for (u = 0; u < n; u += 64) {
			size_t v;

			xb = buf[64];
			for (v = 0; v < 64; v += 8) {
				r &= decode8(x + u + v,
					dec32le(buf + v),
					dec32le(buf + v + 4),
					(xb >> ((unsigned)v >> 3)) & 1);
			}
			buf += 65;
		}
		break;
	}

	return in_len & -(size_t)r;
}

/*
 * Ciphertext encoding: this is similar to public keys, except that
 * values are in the -64..+64 range. We first add 64 to get a positive
 * value (0..128). Each value will be split into its low 3 bits and high
 * 5 bits; the high bits are in 0..16 only. Values are grouped into
 * chunks of eight, that encode into 7 bytes + 1 bit.
 *
 * For small degrees:
 *
 *  - If degree is 2, then encoding is over two bytes, 8 bits per value.
 *  - If degree is 4, then encoding is over four bytes, 8 bits per value.
 *  - If degree is 8, 16 or 32, then one, two or four 57-bit blocks are
 *    used; the ignored bits in the last byte are set to zero.
 */

/*
 * Encode eight ciphertext values x[0]..x[7] into 57 bits (bits 0..23 are
 * returned in *lo, bits 24..55 in *hi, bit 56 is the function return value).
 */
static inline uint32_t
encode_ct_8(uint32_t *lo, uint32_t *hi, const int8_t *x)
{
	int i;
	uint32_t wl, wh, r, t;

	wl = 0;
	for (i = 0; i < 8; i ++) {
		wl |= (uint32_t)((uint8_t)x[i] & 0x07) << (3 * i);
	}
	*lo = wl;
	wh = 0;
	for (i = 7; i > 0; i --) {
		wh = (wh * 17) + (uint32_t)((x[i] + 64) >> 3);
	}

	/*
	 * Final iteration is:
	 *   *hi = wh*17 + (x[0] >> 3)
	 * which can be written as:
	 *   *hi = wh*16 + wh + (x[0] >> 3)
	 * There will be a carry into the top bit (bit 64 of the final
	 * output) if and only if one of the following holds:
	 *  - wh >= 2^28
	 *  - adding wh + (x[0] >> 3) triggers a carry
	 * Note that wh + (x[0] >> 3) <= 410338688 < 2^29.
	 */

	r = wh >> 28;
	t = wh + ((x[0] + 64) >> 3);
	wh = (wh << 4) + t;
	r |= ((uint32_t)(wh - t) & ~wh) >> 31;
	*hi = wh;
	return r;
}


/*
 * Decode a 57-bit word (lo is bits 0..23, hi is bits 24..55, tt is bit 56)
 * into eight values. Returned value is 1 on success, 0 on error.
 */
static uint32_t
decode_ct_8(int8_t *x, uint32_t lo, uint32_t hi, uint32_t tt)
{
	/*
	 * Useful facts:
	 *    256 = 1 mod 17
	 *    65536 = 1 mod 17
	 *    2^32 = 1 mod 17
	 *    for any x in 0..69631, (61681 * x) >> 20 = floor(x / 17)
	 *    17 * 4042322161 = 1 mod 2^32
	 */
	uint32_t a;
	int i;

	/*
	 * First iteration: we need to compute the quotient and
	 * remainder of (hi + (tt << 32)) divided by 17.
	 *
	 * We first compute the remainder.
	 */
	a = (hi & 0xFFFF) + (hi >> 16) + tt;   /* 0 <= a <= 131071 */
	a = (a & 0xFF) + (a >> 8);             /* 0 <= a <= 766 */
	a -= 17 * ((61681 * a) >> 20);         /* 0 <= a <= 16 */
	x[0] = (lo & 0x07) + (a << 3) - 64;

	/*
	 * Subtract the remainder, then do the exact division by multiplying
	 * by the inverse of 17 modulo 2^32. We do all computations modulo
	 * 2^32, since we know that the result will be lower than 2^32 (we
	 * don't have to care about the value of tt here, it is implictly
	 * taken into account in the remainder).
	 */
	hi = (hi - a) * 4042322161u;

	/*
	 * Next four iterations are similar, except that we work on a
	 * 32-bit value now (no tt).
	 */
	for (i = 1; i <= 4; i ++) {
		a = (hi & 0xFFFF) + (hi >> 16);   /* 0 <= a <= 131070 */
		a = (a & 0xFF) + (a >> 8);        /* 0 <= a <= 766 */
		a -= 17 * ((61681 * a) >> 20);    /* 0 <= a <= 16 */
		x[i] = ((lo >> (3 * i)) & 0x07) + (a << 3) - 64;
		hi = (hi - a) * 4042322161u;
	}

	/*
	 * At that point, max value for hi is 6049. The next two iterations
	 * can be done with simpler computations.
	 */
	for (i = 5; i <= 6; i ++) {
		a = (61681 * hi) >> 20;
		x[i] = ((lo >> (3 * i)) & 0x07) + ((hi - 17 * a) << 3) - 64;
		hi = a;
	}

	/*
	 * We use the final value for 'hi' as-is; if it is off-range, this
	 * will be detected in the final test.
	 */
	x[7] = (lo >> 21) + (hi << 3) - 64;

	/*
	 * Decoding is correct if all decoded values are in -64..+64.
	 * Since the values were computed by subtracting 64 from a nonnegative
	 * integer, they cannot be lower than -64, but they could be higher
	 * than +64.
	 */
	a = (uint32_t)-1;
	for (i = 0; i < 8; i ++) {
		a &= (uint32_t)(x[i] - 65);
	}
	return a >> 31;
}


/* see inner.h */
size_t
ctl_encode_ciphertext_257(void *out, size_t max_out_len,
	const int8_t *c, unsigned logn)
{
	size_t out_len, u, v, n;
	uint8_t *buf;
	uint32_t xb;
	int j;

	switch (logn) {
	case 1:
		out_len = 2;
		break;
	case 2:
		out_len = 4;
		break;
	default:
		out_len = (((1u << (logn - 3)) + 7) >> 3) + (1u << logn)
			- (1u << (logn - 3));
		break;
	}
	if (out == NULL) {
		return out_len;
	}
	if (max_out_len < out_len) {
		return 0;
	}

	buf = out;
	switch (logn) {
	case 1:
		buf[0] = (uint8_t)(c[0] + 64);
		buf[1] = (uint8_t)(c[1] + 64);
		return 2;
	case 2:
		buf[0] = (uint8_t)(c[0] + 64);
		buf[1] = (uint8_t)(c[1] + 64);
		buf[2] = (uint8_t)(c[2] + 64);
		buf[3] = (uint8_t)(c[3] + 64);
		return 4;
	case 3:
	case 4:
	case 5:
		n = (size_t)1 << logn;
		v = 0;
		xb = 0;
		j = 0;
		for (u = 0; u < n; u += 8) {
			uint32_t lo, hi;

			xb |= encode_ct_8(&lo, &hi, c + u) << j;
			enc24le(buf + v, lo);
			enc32le(buf + v + 3, hi);
			v += 7;
			if (++ j == 8) {
				buf[v ++] = xb;
				xb = 0;
				j = 0;
			}
		}
		if (j != 0) {
			buf[v ++] = xb;
		}
		return v;
	default:
		n = (size_t)1 << logn;
		v = 0;
		for (u = 0; u < n; u += 64) {
			xb = 0;
			for (j = 0; j < 8; j ++) {
				uint32_t lo, hi;

				xb |= encode_ct_8(&lo, &hi,
					c + u + (j << 3)) << j;
				enc24le(buf + v, lo);
				enc32le(buf + v + 3, hi);
				v += 7;
			}
			buf[v ++] = xb;
		}
		return v;
	}
}

/* see inner.h */
size_t
ctl_decode_ciphertext_257(int8_t *c, unsigned logn,
	const void *in, size_t max_in_len)
{
	const uint8_t *buf;
	uint32_t r, xb;
	size_t in_len, u, n;

	buf = in;
	switch (logn) {
	case 1:
	case 2:
		in_len = (size_t)1 << logn;
		if (max_in_len < in_len) {
			return 0;
		}
		r = 1;
		for (u = 0; u < in_len; u ++) {
			uint8_t tt;

			tt = (uint8_t)(buf[u] - 64);
			c[u] = *(int8_t *)&tt;
			r &= (uint32_t)(buf[u] - 129) >> 31;
		}
		break;
	case 3:
	case 4:
	case 5:
		n = (size_t)1 << logn;
		in_len = 1 + ((size_t)7 << (logn - 3));
		if (max_in_len < in_len) {
			return 0;
		}
		xb = buf[in_len - 1];

		/* Ignored bits in xb must be zero. */
		r = (uint32_t)((xb >> (1u << (logn - 3))) - 1) >> 31;

		/* Decode 57-bit blocks. */
		for (u = 0; u < n; u += 8) {
			size_t v;

			v = u - (u >> 3);
			r &= decode_ct_8(c + u,
				dec24le(buf + v),
				dec32le(buf + v + 3),
				(xb >> ((unsigned)u >> 3)) & 1);
		}
		break;

	default:
		n = (size_t)1 << logn;
		in_len = ((size_t)7 << (logn - 3)) + ((size_t)1 << (logn - 6));
		if (max_in_len < in_len) {
			return 0;
		}
		r = 1;
		for (u = 0; u < n; u += 64) {
			size_t v;

			xb = buf[56];
			for (v = 0; v < 64; v += 8) {
				size_t v2;

				v2 = v - (v >> 3);
				r &= decode_ct_8(c + u + v,
					dec24le(buf + v2),
					dec32le(buf + v2 + 3),
					(xb >> ((unsigned)v >> 3)) & 1);
			}
			buf += 57;
		}
		break;
	}

	return in_len & -(size_t)r;
}
