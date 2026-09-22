#include "poly_inv_ntt.h"
#include <stdio.h>

#define MOD_Q YUANYANG_Q
#define ROOT YUANYANG_ROOTQ
#define M YUANYANG_NTTLENGTH
#define ONLYS (YUANYANG_D/YUANYANG_NTTLENGTH)

#include <stdlib.h>
#include <string.h>
#include <assert.h>


#if defined(__GNUC__) || defined(__clang__)
#define ASSUME(cond) do { if (!(cond)) __builtin_unreachable(); } while (0)
#else
#define ASSUME(cond) ((void)0)
#endif

#define FREE(x) if((x)) { free((x)); x = NULL; }

static int
is_pow2_size(size_t n)
{
    return n && !(n & (n - 1));
}

static unsigned
log2_size(size_t n)
{
    unsigned r = 0;
    while (n > 1) {
        n >>= 1;
        r++;
    }
    return r;
}

static size_t
bitrev_size(size_t x, unsigned logn)
{
    size_t y = 0;
    while (logn--) {
        y = (y << 1) | (x & 1);
        x >>= 1;
    }
    return y;
}

inline modq_t
mod_reduce_u32(uint32_t x)
{
    uint32_t t = ((x>>11) * ((1<<28)/MOD_Q)) >> 17;
    uint16_t r = x - t * (uint16_t)MOD_Q;
    return r>=MOD_Q ? r-MOD_Q :r;
}

inline modq_t
mod_add(modq_t a, modq_t b)
{
    uint32_t r = (uint32_t)a + b;
    if (r >= MOD_Q)
        r -= MOD_Q;
    return (modq_t)r;
}

inline modq_t
mod_sub(modq_t a, modq_t b)
{
    uint32_t aa = a;
    uint32_t bb = b;
    return (modq_t)(aa >= bb ? aa - bb : aa + MOD_Q - bb);
}

inline modq_t
mod_neg(modq_t a)
{
    return (modq_t)(a ? MOD_Q - a : 0);
}

inline modq_t
mod_mul(modq_t a, modq_t b)
{
    return mod_reduce_u32((uint32_t)a * b);
}

modq_t
mod_pow(modq_t a, uint32_t e)
{
    modq_t r = 1;
    while (e) {
        if (e & 1)
            r = mod_mul(r, a);
        a = mod_mul(a, a);
        e >>= 1;
    }
    return r;
}

/*
static int inv(int a, int m) { return a ? (1 - m * inv(m % a, a)) / a : 0; }
inline modq_t
mod_inv(modq_t a)
{
    int t=inv(a,YUANYANG_Q);
    return t>0 ? t: t+YUANYANG_Q;
}*/

modq_t
mod_inv(modq_t a)
{
	int mod = YUANYANG_Q;
	int u0 = 1, u1 = 0,m=mod;

	a %= m;

	for (;;) {
		long q;

		q = m / a;
		m -= q * a;
		u1 -= q * u0;
		if (m == 0)
			break;

		q = a / m;
		a -= q * m;
		u0 -= q * u1;
		if (a == 0)
			break;
	}

	if (m == 0) {
		u0 %= mod;
		return u0 < 0 ? u0 + mod : u0;
	} else {
		u1 %= mod;
		return u1 < 0 ? u1 + mod : u1;
	}
}


static inline int
poly_deg(const modq_t *a, size_t n)
{
    while (n && a[n - 1] == 0)
        n--;
    return n ? (int)n - 1 : -1;
}

static void
poly_divrem(modq_t *q, modq_t *r,
            const modq_t *a, size_t alen,
            const modq_t *b, size_t blen)
{
	ASSUME(alen<=1+ONLYS);
	ASSUME(blen<=ONLYS);
	int da, db;
	modq_t ib;
	size_t qlen = alen - blen + 1;
    if(qlen==2) {
        q[0]=0;
    }
    else {
        memset(q, 0, qlen * sizeof *q);
    }
	memcpy(r, a, alen * sizeof *r);
	da = alen-1;
	db = blen-1;
	ib = mod_inv(b[db]);
	while (da >= db) {
		int sh = da - db;
		modq_t c = mod_mul(r[da], ib);
		q[sh] = c;
		for (int i = 0; i < db; i++)
			r[i + sh] = mod_sub(r[i + sh], mod_mul(c, b[i]));
		r[da] = 0;
		da = poly_deg(r, (size_t)da);
	}
}

static int
poly_mul_mod_xs_root(modq_t *out,
                     const modq_t *a,
                     const modq_t *b,
                     size_t s,
                     modq_t root)
{
    size_t i, j;
    modq_t *acc;

    memset(out, 0, s * sizeof *out);
    if (s == 1) {
        out[0] = mod_mul(a[0], b[0]);
        return 0;
    }

    acc = calloc(2 * s - 1, sizeof *acc);
    if (!acc)
        return -1;

    for (i = 0; i < s; i++) {
        if (a[i] == 0)
            continue;
        for (j = 0; j < s; j++) {
            if (b[j] == 0)
                continue;
            acc[i + j] = mod_add(acc[i + j], mod_mul(a[i], b[j]));
        }
    }

    for (i = 2 * s - 2; i >= s; i--) {
        if (acc[i])
            acc[i - s] = mod_add(acc[i - s], mod_mul(acc[i], root));
        if (i == s)
            break;
    }

    memcpy(out, acc, s * sizeof *out);
    free(acc);
    return 0;
}

static void
poly_sub_mul_plain(modq_t *d, size_t dlen,
                   const modq_t *a, size_t alen,
                   const modq_t *b, size_t blen,
                   const modq_t *c, size_t clen)
{
    ASSUME(alen<=ONLYS);
	ASSUME(blen<=ONLYS);
    ASSUME(clen<=ONLYS);
    ASSUME(dlen<=ONLYS);
	size_t i, j, n;
	n =  alen;
	memcpy(d, a, n * sizeof *d);
	memset(d + n, 0, (dlen - n) * sizeof *d);
	for (i = 0; i < blen; i++) {
		modq_t bi = b[i];
		for (j = 0; j < clen; j++) {
			modq_t cj = c[j];
			d[i + j] = mod_sub(d[i + j], mod_mul(bi, cj));
		}
	}
}

static int
poly_inv_mod_xs_root(modq_t *out,
                     const modq_t *a,
                     size_t s,
                     modq_t root,
                     modq_t* work)
{
	size_t i, r0len, r1len, t0len, t1len;
	modq_t *r0, *r1, *q, *rem, *t0, *t1, *tn, *tt;
    ASSUME(s==ONLYS);
	if (poly_deg(a, s) < 0)
		return -1;

    modq_t *w = work;
    r0 = w; w += s + 1;
    r1 = w; w += s + 1;
    q = w; w += s + 1;
    rem = w; w += s + 1;
    t0 = w; w += s;
    t1 = w; w += s;
    tn = w; w += s;
	memset(r0, 0, (s + 1) * sizeof *r0);
	memset(r1, 0, (s + 1) * sizeof *r1);
	memset(t0, 0, s * sizeof *t0);
	memset(t1, 0, s * sizeof *t1);
	r0[0] = mod_neg(root);
	r0[s] = 1;
	memcpy(r1, a, s * sizeof *r1);
	t1[0] = 1;
	r0len = s + 1;
	r1len = (size_t)poly_deg(r1, s) + 1;
	t0len = 0;
	t1len = 1;
    while (r1len > 1) {
        size_t qlen, tnlen;
        int rd;
        poly_divrem(q, rem, r0, r0len, r1, r1len); 
        qlen = r0len - r1len + 1;
        tnlen = qlen+t1len-1;
        poly_sub_mul_plain(tn, tnlen, t0, t0len, q, qlen, t1, t1len);
        rd = poly_deg(rem, r1len-1);
        tt = r0; r0 = r1; r1 = rem; rem = tt;
        tt = t0; t0 = t1; t1 = tn; tn = tt;
        r0len = r1len;
        r1len = rd + 1;
        t0len = t1len;
        t1len = tnlen;
    }
	if (r1len == 0)
		return -1;
	modq_t c = mod_inv(r1[0]);
	for (i = 0; i < t1len; i++)
		out[i] = mod_mul(t1[i], c);
	for (; i < s; i++)
		out[i] = 0;
	return 0;
}

int
partial_ntt_make_twiddles(modq_t *tw, size_t m, modq_t psi)
{
    unsigned logm;

    if (!tw || !is_pow2_size(m))
        return -1;

    memset(tw, 0, PARTIAL_NTT_TWIDDLES_LEN(m) * sizeof *tw);
    tw[0] = 1;
    tw[m] = 1;
    if (m == 1) {
        tw[2 * m] = mod_neg(1);
        return 0;
    }

    logm = log2_size(m);
    for (size_t k = 1; k < m; k++) {
        modq_t z = mod_pow(psi, (uint32_t)bitrev_size(k, logm));
        tw[k] = z;
        tw[m + k] = mod_inv(z);
    }
    for (size_t j = 0; j < m; j++) {
        size_t e = (bitrev_size(j, logm) << 1) + 1;
        tw[2 * m + j] = mod_pow(psi, (uint32_t)e);
    }
    return 0;
}

int
partial_ntt_inplace(modq_t *a, size_t n, size_t m,
                    const modq_t *tw)
{
    size_t t, mm,s=n/m;

    if (!a || !tw || !is_pow2_size(n) || !is_pow2_size(m) || !is_pow2_size(s))
        return -1;
    if (m * s != n)
        return -1;
    if (m == 1)
        return 0;

    t = m;
    for (mm = 1; mm < m; mm <<= 1) {
        size_t ht = t >> 1;
        for (size_t i1 = 0; i1 < mm; i1++) {
            size_t j1 = i1 * t;
            modq_t z = tw[mm + i1];
            for (size_t j = j1; j < j1 + ht; j++) {
                modq_t *x = a + j * s;
                modq_t *y = a + (j + ht) * s;
                for (size_t u = 0; u < s; u++) {
                    modq_t xu = x[u];
                    modq_t tmp = mod_mul(y[u],z);
                    x[u] = mod_add(xu, tmp);
                    y[u] = mod_sub(xu, tmp);
                }
            }
        }
        t = ht;
    }

    return 0;
}


int
partial_intt_inplace(modq_t *a, size_t n, size_t m,
                     const modq_t *tw)
{
    size_t s, t;
    modq_t invm;

    if (!a || !tw)
        return -1;
    if (!is_pow2_size(n) || !is_pow2_size(m))
        return -1;
    if (m == 0 || n % m != 0)
        return -1;

    s = n / m;
    if (!is_pow2_size(s))
        return -1;

    if (m == 1)
        return 0;

    /*
     * Inverse of the first log2(m) forward stages.
     *
     * Input layout:
     *
     *     a[j*s + u] = coeff_u(f mod (x^s - root_j))
     *
     * Output:
     *
     *     ordinary coefficient order modulo x^n + 1
     */
    t = s;
    for (size_t mm = m; mm > 1; mm >>= 1) {
        size_t hm = mm >> 1;
        size_t dt = t << 1;

        for (size_t i = 0, j1 = 0; i < hm; i++, j1 += dt) {
            modq_t z = tw[m + hm + i];

            for (size_t j = j1; j < j1 + t; j++) {
                modq_t u = a[j];
                modq_t v = a[j + t];

                a[j] = mod_add(u, v);
                a[j + t] = mod_mul(mod_sub(u, v), z);
            }
        }

        t = dt;
    }

    invm = mod_inv((modq_t)m);
    for (size_t i = 0; i < n; i++)
        a[i] = mod_mul(a[i], invm);

    return 0;
}

int
partial_ntt(modq_t *out, const modq_t *f, size_t n, size_t m,
            const modq_t *tw)
{
    if (!out || !f)
        return -1;
    memmove(out, f, n * sizeof *out);
    return partial_ntt_inplace(out, n, m, tw);
}

int
partial_intt(modq_t *f, const modq_t *in, size_t n, size_t m,
             const modq_t *tw)
{
    if (!f || !in)
        return -1;
    memmove(f, in, n * sizeof *f);
    return partial_intt_inplace(f, n, m, tw);
}

static int
pointwise_mul_full(modq_t *c, const modq_t *a, const modq_t *b,
                   size_t m, size_t s, const modq_t *tw)
{
    for (size_t j = 0; j < m; j++) {
        modq_t root = (m == 1) ? mod_neg(1) : tw[2 * m + j];

        if (poly_mul_mod_xs_root(c + j * s,
                                 a + j * s,
                                 b + j * s,
                                 s, root) < 0)
            return -1;
    }

    return 0;
}

int
poly_mul_xn1_q(modq_t *c, const modq_t *a, const modq_t *b, size_t n)
{
    size_t m=M;
    size_t s=n/m;
    modq_t *tw, *ta, *tb;
    int r = -1;

    if (!c || !a || !b)
        return -1;

    tw = malloc(PARTIAL_NTT_TWIDDLES_LEN(m) * sizeof *tw);
    ta = malloc(n * sizeof *ta);
    tb = malloc(n * sizeof *tb);
    if (!tw || !ta || !tb)
        goto done;

    if (partial_ntt_make_twiddles(tw, m, ROOT) < 0)
        goto done;
    memcpy(ta, a, n * sizeof *ta);
    memcpy(tb, b, n * sizeof *tb);
    if (partial_ntt_inplace(ta, n, m,  tw) < 0)
        goto done;
    if (partial_ntt_inplace(tb, n, m, tw) < 0)
        goto done;
    if (pointwise_mul_full(c, ta, tb, m, s, tw) < 0)
        goto done;
    if (partial_intt_inplace(c, n, m, tw) < 0)
        goto done;
    r = 0;

done:
    free(tw); free(ta); free(tb);
    return r;
}

static int
pointwise_inv_full(modq_t *out, const modq_t *in,
                   size_t m, size_t s, const modq_t *tw)
{
    modq_t *work=malloc((7*s+4)*sizeof(modq_t));
    if(!work)
        return -1;
    for (size_t j = 0; j < m; j++) {
        modq_t root = (m == 1) ? mod_neg(1) : tw[2 * m + j];

        if (poly_inv_mod_xs_root(out + j * s,
                                 in  + j * s,
                                 s, root,work) < 0)
            return -1;
    }
    free(work);
    return 0;
}

int
poly_inv_xn1_q(modq_t *out, const modq_t *f, size_t n)
{
    size_t m = M, s;
    modq_t *tw, *eval, *ieva;
    int r = -1;

    if (!out || !f)
        return -1;
    if (!is_pow2_size(n) || !is_pow2_size(m))
        return -1;
    if (m == 0 || n % m != 0)
        return -1;

    s = n / m;
    if (!is_pow2_size(s))
        return -1;

    tw = malloc(PARTIAL_NTT_TWIDDLES_LEN(m) * sizeof *tw);
    eval = malloc(n * sizeof *eval);
    ieva = malloc(n * sizeof *ieva);
    if (!tw || !eval || !ieva)
        goto done;

    if (partial_ntt_make_twiddles(tw, m, ROOT) < 0)
        goto done;

    memcpy(eval, f, n * sizeof *eval);

    if (partial_ntt_inplace(eval, n, m, tw) < 0)
        goto done;

    /*
     * eval block j is:
     *
     *     eval[j*s + 0 ... j*s + s-1]
     *
     * representing f mod (x^s - root_j).
     */
    if (pointwise_inv_full(ieva, eval, m, s, tw) < 0)
        goto done;

    if (partial_intt_inplace(ieva, n, m, tw) < 0)
        goto done;

    memcpy(out, ieva, n * sizeof *out);
    r = 0;

done:
    free(tw);
    free(eval);
    free(ieva);
    return r;
}
