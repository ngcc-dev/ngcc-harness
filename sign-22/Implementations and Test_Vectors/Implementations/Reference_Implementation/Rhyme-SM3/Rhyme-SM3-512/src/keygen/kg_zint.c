#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "kg_zint.h"

static void zi_reserve(zint *z, uint32_t cap) {
    if (z->cap >= cap) return;
    uint32_t nc = z->cap ? z->cap : 8;
    while (nc < cap) nc *= 2;
    z->d = realloc(z->d, (size_t)nc * 4);
    z->cap = nc;
}
static void zi_norm(zint *z) {
    while (z->len && z->d[z->len - 1] == 0) z->len--;
    if (z->len == 0) z->sign = 0;
}

void zi_init(zint *z) { z->sign = 0; z->len = 0; z->cap = 0; z->d = NULL; }
void zi_free(zint *z) { free(z->d); zi_init(z); }

void zi_set_i64(zint *z, int64_t v) {
    uint64_t a;
    if (v == 0) { z->sign = 0; z->len = 0; return; }
    if (v < 0) { z->sign = -1; a = (uint64_t)(-(v + 1)) + 1; }
    else { z->sign = 1; a = (uint64_t)v; }
    zi_reserve(z, 2);
    z->d[0] = (uint32_t)a;
    z->d[1] = (uint32_t)(a >> 32);
    z->len = z->d[1] ? 2 : 1;
}

void zi_copy(zint *r, const zint *a) {
    if (r == a) return;
    zi_reserve(r, a->len);
    memcpy(r->d, a->d, (size_t)a->len * 4);
    r->len = a->len;
    r->sign = a->sign;
}

unsigned zi_bitlen(const zint *z) {
    if (z->len == 0) return 0;
    uint32_t t = z->d[z->len - 1];
    unsigned b = 0;
    while (t) { b++; t >>= 1; }
    return (z->len - 1) * 32 + b;
}

int zi_is_zero(const zint *z) { return z->sign == 0; }

int zi_cmp_abs(const zint *a, const zint *b) {
    if (a->len != b->len) return a->len < b->len ? -1 : 1;
    for (uint32_t i = a->len; i-- > 0;) {
        if (a->d[i] != b->d[i]) return a->d[i] < b->d[i] ? -1 : 1;
    }
    return 0;
}

/* |r| = |a| + |b| */
static void abs_add(zint *r, const zint *a, const zint *b) {
    const zint *x = a, *y = b;
    if (x->len < y->len) { const zint *t = x; x = y; y = t; }
    zi_reserve(r, x->len + 1);
    uint64_t carry = 0;
    uint32_t i;
    for (i = 0; i < y->len; i++) {
        uint64_t s = (uint64_t)x->d[i] + y->d[i] + carry;
        r->d[i] = (uint32_t)s;
        carry = s >> 32;
    }
    for (; i < x->len; i++) {
        uint64_t s = (uint64_t)x->d[i] + carry;
        r->d[i] = (uint32_t)s;
        carry = s >> 32;
    }
    r->d[i] = (uint32_t)carry;
    r->len = x->len + 1;
    zi_norm(r);
}

/* |r| = |a| - |b|, requires |a| >= |b| */
static void abs_sub(zint *r, const zint *a, const zint *b) {
    zi_reserve(r, a->len);
    int64_t borrow = 0;
    uint32_t i;
    for (i = 0; i < a->len; i++) {
        int64_t s = (int64_t)a->d[i] - (i < b->len ? b->d[i] : 0) - borrow;
        if (s < 0) { s += ((int64_t)1 << 32); borrow = 1; } else borrow = 0;
        r->d[i] = (uint32_t)s;
    }
    r->len = a->len;
    zi_norm(r);
}

void zi_add(zint *r, const zint *a, const zint *b) {
    if (a->sign == 0) { zi_copy(r, b); return; }
    if (b->sign == 0) { zi_copy(r, a); return; }
    if (a->sign == b->sign) {
        int s = a->sign;
        abs_add(r, a, b);
        r->sign = r->len ? s : 0;
    } else {
        int c = zi_cmp_abs(a, b);
        if (c == 0) { r->sign = 0; r->len = 0; return; }
        if (c > 0) { int s = a->sign; abs_sub(r, a, b); r->sign = r->len ? s : 0; }
        else       { int s = b->sign; abs_sub(r, b, a); r->sign = r->len ? s : 0; }
    }
}

void zi_sub(zint *r, const zint *a, const zint *b) {
    zint nb = *b;       /* shallow negate trick (b unmodified) */
    nb.sign = -b->sign;
    zi_add(r, a, &nb);
}

void zi_neg(zint *z) { z->sign = -z->sign; }

void zi_mul(zint *r, const zint *a, const zint *b) {
    if (a->sign == 0 || b->sign == 0) { r->sign = 0; r->len = 0; return; }
    uint32_t rl = a->len + b->len;
    uint32_t *tmp = calloc(rl, 4);
    for (uint32_t i = 0; i < a->len; i++) {
        uint64_t carry = 0;
        uint64_t ai = a->d[i];
        for (uint32_t j = 0; j < b->len; j++) {
            uint64_t s = ai * b->d[j] + tmp[i + j] + carry;
            tmp[i + j] = (uint32_t)s;
            carry = s >> 32;
        }
        uint32_t k = i + b->len;
        while (carry) {
            uint64_t s = (uint64_t)tmp[k] + carry;
            tmp[k] = (uint32_t)s;
            carry = s >> 32;
            k++;
        }
    }
    zi_reserve(r, rl);
    memcpy(r->d, tmp, (size_t)rl * 4);
    free(tmp);
    r->len = rl;
    r->sign = a->sign * b->sign;
    zi_norm(r);
}

void zi_addmul_u32(zint *r, const zint *m, uint32_t t) {
    if (t == 0 || m->sign == 0) return;
    zint tmp; zi_init(&tmp);
    zi_reserve(&tmp, m->len + 1);
    uint64_t carry = 0;
    for (uint32_t i = 0; i < m->len; i++) {
        uint64_t s = (uint64_t)m->d[i] * t + carry;
        tmp.d[i] = (uint32_t)s;
        carry = s >> 32;
    }
    tmp.d[m->len] = (uint32_t)carry;
    tmp.len = m->len + 1;
    tmp.sign = m->sign;
    zi_norm(&tmp);
    zi_add(r, r, &tmp);
    zi_free(&tmp);
}

void zi_mul_u32(zint *r, uint32_t t) {
    if (t == 0 || r->sign == 0) { r->sign = 0; r->len = 0; return; }
    zi_reserve(r, r->len + 1);
    uint64_t carry = 0;
    for (uint32_t i = 0; i < r->len; i++) {
        uint64_t s = (uint64_t)r->d[i] * t + carry;
        r->d[i] = (uint32_t)s;
        carry = s >> 32;
    }
    r->d[r->len] = (uint32_t)carry;
    r->len += 1;
    zi_norm(r);
}

uint32_t zi_mod_u32(const zint *z, uint32_t p) {
    uint64_t r = 0;
    for (uint32_t i = z->len; i-- > 0;)
        r = ((r << 32) | z->d[i]) % p;
    if (z->sign < 0 && r != 0) r = p - r;
    return (uint32_t)r;
}

/* slow path: build the full shifted zint and subtract */
static void zi_sub_shifted_slow(zint *r, __int128 v, unsigned shiftbits) {
    int neg = v < 0;
    unsigned __int128 a = neg ? (unsigned __int128)(-v) : (unsigned __int128)v;
    unsigned limb = shiftbits / 32, bit = shiftbits % 32;
    unsigned __int128 sh = a << bit;
    zint t; zi_init(&t);
    zi_reserve(&t, limb + 6);
    memset(t.d, 0, (size_t)(limb + 6) * 4);
    for (unsigned i = 0; i < 5; i++) {
        t.d[limb + i] = (uint32_t)(sh & 0xFFFFFFFFu);
        sh >>= 32;
    }
    t.d[limb + 5] = (uint32_t)sh;
    t.len = limb + 6;
    t.sign = neg ? -1 : 1;
    zi_norm(&t);
    zi_sub(r, r, &t);
    zi_free(&t);
}

/* fast in-place: r -= (v << shiftbits). window add is O(6 + carries);
 * window subtract compares magnitudes first and falls back when |r| < |v<<sh|. */
void zi_sub_shifted_i128(zint *r, __int128 v, unsigned shiftbits) {
    if (v == 0) return;
    int vsign = v < 0 ? -1 : 1;
    if (r->sign == 0 || r->sign != vsign) {
        /* magnitude grows: |r| += |v<<sh| with sign = (r->sign != 0 ? r->sign : -vsign) */
        unsigned __int128 a = vsign < 0 ? (unsigned __int128)(-v) : (unsigned __int128)v;
        unsigned limb = shiftbits / 32, bit = shiftbits % 32;
        unsigned __int128 sh = a << bit;
        uint32_t w[6];
        for (unsigned i = 0; i < 6; i++) { w[i] = (uint32_t)sh; sh >>= 32; }
        unsigned need = limb + 7;
        if (r->cap < need + 1) { zi_sub_shifted_slow(r, v, shiftbits); return; }
        if (r->len < need) memset(r->d + r->len, 0, (size_t)(need - r->len) * 4);
        uint64_t carry = 0;
        for (unsigned i = 0; i < 6; i++) {
            uint64_t s2 = (uint64_t)r->d[limb + i] + w[i] + carry;
            r->d[limb + i] = (uint32_t)s2;
            carry = s2 >> 32;
        }
        unsigned k = limb + 6;
        while (carry) {
            uint64_t s2 = (uint64_t)r->d[k] + carry;
            r->d[k] = (uint32_t)s2;
            carry = s2 >> 32;
            k++;
        }
        if (need > r->len) r->len = need;
        if (r->sign == 0) r->sign = -vsign;
        zi_norm(r);
        return;
    }
    /* same sign: magnitude shrinks if |r| >= |v<<sh| */
    unsigned __int128 a = vsign < 0 ? (unsigned __int128)(-v) : (unsigned __int128)v;
    unsigned limb = shiftbits / 32, bit = shiftbits % 32;
    unsigned __int128 sh = a << bit;
    uint32_t w[6];
    for (unsigned i = 0; i < 6; i++) { w[i] = (uint32_t)sh; sh >>= 32; }
    /* compare |r| vs window<<32*limb */
    int cmp = 0;
    if (r->len > limb + 6) cmp = 1;
    else {
        for (int i = 5; i >= 0 && cmp == 0; i--) {
            uint32_t ri = (limb + (unsigned)i < r->len) ? r->d[limb + (unsigned)i] : 0;
            if (ri != w[i]) cmp = ri > w[i] ? 1 : -1;
        }
        if (cmp == 0) {
            for (unsigned i = 0; i < limb && cmp == 0; i++)
                if (r->d[i]) cmp = 1;
        }
    }
    if (cmp < 0) { zi_sub_shifted_slow(r, v, shiftbits); return; }
    if (cmp == 0) { r->sign = 0; r->len = 0; return; }
    int64_t borrow = 0;
    for (unsigned i = 0; i < 6; i++) {
        int64_t s2 = (int64_t)((limb + i < r->len) ? r->d[limb + i] : 0) - w[i] - borrow;
        if (s2 < 0) { s2 += ((int64_t)1 << 32); borrow = 1; } else borrow = 0;
        if (limb + i < r->len) r->d[limb + i] = (uint32_t)s2;
    }
    unsigned k = limb + 6;
    while (borrow && k < r->len) {
        if (r->d[k] == 0) { r->d[k] = 0xFFFFFFFFu; }
        else { r->d[k] -= 1; borrow = 0; }
        k++;
    }
    zi_norm(r);
}

double zi_top_double(const zint *z, unsigned shift) {
    if (z->sign == 0) return 0.0;
    unsigned bl = zi_bitlen(z);
    if (shift >= bl) {
        /* magnitude below window: still return scaled value */
    }
    /* extract bits [shift, shift+63) into u64, then scale */
    unsigned limb = shift / 32, bit = shift % 32;
    uint64_t lo = 0;
    for (int i = 2; i >= 0; i--) {
        uint32_t li = (limb + (unsigned)i < z->len) ? z->d[limb + (unsigned)i] : 0;
        lo = (lo << 32) | li;
    }
    /* lo holds limbs [limb, limb+3); we want >> bit then take low 64 of the 96-bit window */
    /* recompute via 128-bit */
    unsigned __int128 w = 0;
    for (int i = 3; i >= 0; i--) {
        uint32_t li = (limb + (unsigned)i < z->len) ? z->d[limb + (unsigned)i] : 0;
        w = (w << 32) | li;
    }
    w >>= bit;
    uint64_t v = (uint64_t)w;   /* floor(|z|/2^shift) mod 2^64; caller picks shift so it fits */
    double res = (double)v;
    return z->sign < 0 ? -res : res;
}

int64_t zi_to_i64(const zint *z) {
    uint64_t v = 0;
    if (z->len > 0) v = z->d[0];
    if (z->len > 1) v |= (uint64_t)z->d[1] << 32;
    int64_t r = (int64_t)v;
    return z->sign < 0 ? -r : r;
}

/* extended binary gcd (HAC 14.61): g = gcd(x,y), u*x0 + v*y0 = g (x0=|a|, y0=|b|).
 * then fix signs for the original signed a, b. */
void zi_xgcd(zint *g, zint *u, zint *v, const zint *a, const zint *b) {
    zint x, y, gg, A, B, C, D, t1, t2;
    zi_init(&x); zi_init(&y); zi_init(&gg);
    zi_init(&A); zi_init(&B); zi_init(&C); zi_init(&D);
    zi_init(&t1); zi_init(&t2);
    zi_copy(&x, a); x.sign = 1;
    zi_copy(&y, b); y.sign = 1;
    zi_set_i64(&gg, 1);

    /* strip common factors of 2 */
    while (x.len && y.len && !(x.d[0] & 1) && !(y.d[0] & 1)) {
        /* x >>= 1; y >>= 1; gg <<= 1 */
        for (uint32_t i = 0; i < x.len; i++)
            x.d[i] = (x.d[i] >> 1) | ((i + 1 < x.len ? x.d[i+1] : 0) << 31);
        zi_norm(&x);
        for (uint32_t i = 0; i < y.len; i++)
            y.d[i] = (y.d[i] >> 1) | ((i + 1 < y.len ? y.d[i+1] : 0) << 31);
        zi_norm(&y);
        zi_mul_u32(&gg, 2);
    }
    zint x0, y0; zi_init(&x0); zi_init(&y0);
    zi_copy(&x0, &x); zi_copy(&y0, &y);

    zint uu, vv;  /* working copies of x, y reused as 'u' and 'v' in HAC */
    zi_init(&uu); zi_init(&vv);
    zi_copy(&uu, &x); zi_copy(&vv, &y);
    zi_set_i64(&A, 1); zi_set_i64(&B, 0);
    zi_set_i64(&C, 0); zi_set_i64(&D, 1);

#define HALVE(Z) do { \
        for (uint32_t _i = 0; _i < (Z).len; _i++) \
            (Z).d[_i] = ((Z).d[_i] >> 1) | ((_i + 1 < (Z).len ? (Z).d[_i+1] : 0) << 31); \
        zi_norm(&(Z)); \
    } while (0)

    while (uu.sign != 0) {
        while (uu.len && !(uu.d[0] & 1)) {
            HALVE(uu);
            if ((A.len && (A.d[0] & 1)) || (B.len && (B.d[0] & 1))) {
                zi_add(&A, &A, &y0);
                zi_sub(&B, &B, &x0);
            }
            HALVE(A); HALVE(B);
            /* halving signed: magnitude halving is wrong if odd; ensured even by adjust */
        }
        while (vv.len && !(vv.d[0] & 1)) {
            HALVE(vv);
            if ((C.len && (C.d[0] & 1)) || (D.len && (D.d[0] & 1))) {
                zi_add(&C, &C, &y0);
                zi_sub(&D, &D, &x0);
            }
            HALVE(C); HALVE(D);
        }
        if (zi_cmp_abs(&uu, &vv) >= 0) {
            zi_sub(&uu, &uu, &vv);
            zi_sub(&A, &A, &C);
            zi_sub(&B, &B, &D);
        } else {
            zi_sub(&vv, &vv, &uu);
            zi_sub(&C, &C, &A);
            zi_sub(&D, &D, &B);
        }
    }
    /* g = gg * vv ; u = C ; v = D, for |a|,|b| */
    zi_mul(g, &gg, &vv);
    zi_copy(u, &C);
    zi_copy(v, &D);
    if (a->sign < 0) zi_neg(u);
    if (b->sign < 0) zi_neg(v);

    zi_free(&x); zi_free(&y); zi_free(&gg);
    zi_free(&A); zi_free(&B); zi_free(&C); zi_free(&D);
    zi_free(&t1); zi_free(&t2);
    zi_free(&x0); zi_free(&y0);
    zi_free(&uu); zi_free(&vv);
#undef HALVE
}


/* ============================================================
 * Lehmer-accelerated GCD (no Bezout).  For coprimality testing.
 * Standard scheme (Knuth 4.5.2 / Cohen 1.4.2): collapse many Euclid steps
 * using the top words, then apply a 2x2 transform to the full integers.
 * Falls back to an exact big-int remainder step when the single-precision
 * cofactor matrix cannot be advanced, so the result is always the true gcd.
 * ============================================================ */

/* extract the top 'bits' (<=63) of |z| aligned to its MSB, as a uint64. */
static uint64_t zi_topbits(const zint *z, unsigned want, unsigned *outbl) {
    unsigned bl = zi_bitlen(z);
    *outbl = bl;
    if (bl == 0) return 0;
    unsigned sh = (bl > want) ? bl - want : 0;
    /* gather bits [sh, sh+want) */
    unsigned limb = sh / 32, bit = sh % 32;
    unsigned __int128 w = 0;
    for (int i = 3; i >= 0; i--) {
        uint32_t li = (limb + (unsigned)i < z->len) ? z->d[limb + (unsigned)i] : 0;
        w = (w << 32) | li;
    }
    w >>= bit;
    return (uint64_t)w;
}

/* exact x = |x| mod |y| (y!=0) via shifted binary subtraction (O(N * bits)). */
static void zi_mod_exact(zint *x, const zint *y) {
    zint yy; zi_init(&yy); zi_copy(&yy, y); yy.sign = 1;
    x->sign = (x->len ? 1 : 0);
    while (zi_cmp_abs(x, &yy) >= 0) {
        unsigned xb = zi_bitlen(x), yb = zi_bitlen(&yy);
        unsigned sh = xb - yb;
        /* try to subtract yy<<sh; if that exceeds x, use sh-1 */
        zint t; zi_init(&t); zi_copy(&t, &yy);
        /* shift t left by sh bits */
        unsigned full = sh / 32, rem = sh % 32;
        if (rem) zi_mul_u32(&t, (uint32_t)(1u << rem));
        if (full) {
            zint ts; zi_init(&ts); zi_reserve(&ts, t.len + full);
            for (unsigned i = 0; i < full; i++) ts.d[i] = 0;
            for (uint32_t i = 0; i < t.len; i++) ts.d[i + full] = t.d[i];
            ts.len = t.len + full; ts.sign = 1; zi_norm(&ts);
            zi_copy(&t, &ts); zi_free(&ts);
        }
        if (zi_cmp_abs(&t, x) > 0) {
            /* yy<<sh too big: halve it once (sh-1) */
            /* divide t by 2 */
            for (uint32_t i = 0; i < t.len; i++)
                t.d[i] = (t.d[i] >> 1) | ((i + 1 < t.len ? t.d[i+1] : 0) << 31);
            zi_norm(&t);
        }
        zi_sub(x, x, &t);
        x->sign = (x->len ? 1 : 0);
        zi_free(&t);
    }
    zi_free(&yy);
}

/* r *= m (64-bit multiplier). */
static void mul_u64(zint *r, uint64_t m) {
    if (m == 0) { r->sign = 0; r->len = 0; return; }
    uint32_t lo = (uint32_t)m, hi = (uint32_t)(m >> 32);
    if (hi == 0) { zi_mul_u32(r, lo); return; }
    zint rhi; zi_init(&rhi); zi_copy(&rhi, r);
    zi_mul_u32(r, lo);
    zi_mul_u32(&rhi, hi);
    /* r += rhi << 32 */
    zint sh; zi_init(&sh); zi_reserve(&sh, rhi.len + 1);
    sh.d[0] = 0;
    for (uint32_t i = 0; i < rhi.len; i++) sh.d[i + 1] = rhi.d[i];
    sh.len = rhi.len + 1; sh.sign = rhi.sign ? 1 : 0; zi_norm(&sh);
    zi_add(r, r, &sh);
    zi_free(&rhi); zi_free(&sh);
}

void zi_gcd(zint *g, const zint *a, const zint *b) {
    zint x, y, t, nx, ny; zi_init(&x); zi_init(&y); zi_init(&t);
    zi_init(&nx); zi_init(&ny);
    zi_copy(&x, a); x.sign = (x.len ? 1 : 0);
    zi_copy(&y, b); y.sign = (y.len ? 1 : 0);
    if (x.sign == 0) { zi_copy(g, &y); goto done; }
    if (y.sign == 0) { zi_copy(g, &x); goto done; }
    if (zi_cmp_abs(&x, &y) < 0) { zi_copy(&t, &x); zi_copy(&x, &y); zi_copy(&y, &t); }

    while (y.sign != 0) {
        unsigned xb = zi_bitlen(&x), yb = zi_bitlen(&y);
        if (xb - yb > 40 || xb <= 64) {
            /* magnitudes far apart, or small enough: one exact remainder step */
            zi_mod_exact(&x, &y);
            zi_copy(&t, &x); zi_copy(&x, &y); zi_copy(&y, &t);
            continue;
        }
        /* Lehmer: take aligned top ~62 bits of x and y (same shift = xb-62). */
        unsigned sh = xb - 62;
        uint64_t xt, yt; unsigned d1, d2;
        xt = zi_topbits(&x, 62, &d1);
        /* y aligned to the SAME shift as x (not its own MSB) */
        {
            unsigned limb = sh / 32, bit = sh % 32;
            unsigned __int128 w = 0;
            for (int i = 3; i >= 0; i--) {
                uint32_t li = (limb + (unsigned)i < y.len) ? y.d[limb + (unsigned)i] : 0;
                w = (w << 32) | li;
            }
            w >>= bit; yt = (uint64_t)w; (void)d2;
        }
        (void)d1;
        /* single-precision Euclid with Lehmer cofactor matrix [[A,B],[C,D]]:
         * (x,y) ~ (xt,yt); maintain x = A*X+B*Y, y = C*X+D*Y in window. */
        int64_t A = 1, B = 0, C = 0, D = 1;
        uint64_t u = xt, w2 = yt;
        int steps = 0;
        for (;;) {
            if (w2 + (uint64_t)C == 0 || w2 + (uint64_t)D == 0) break;
            uint64_t q = u / w2;
            /* Lehmer/Cohen safety test on cofactors */
            int64_t nA = A - (int64_t)q * C;
            int64_t nB = B - (int64_t)q * D;
            uint64_t nu = u - q * w2;
            /* guard: stop if the linear-combination approximation may be wrong */
            if (nu < (uint64_t)(-nA < 0 ? -nA : nA) ||
                (w2 - nu) < (uint64_t)((D - nB) < 0 ? -(D - nB) : (D - nB)))
                break;
            A = C; B = D; C = nA; D = nB;
            u = w2; w2 = nu;
            steps++;
            if (steps > 60) break;
        }
        if (steps == 0) {
            /* couldn't advance in single precision: exact step */
            zi_mod_exact(&x, &y);
            zi_copy(&t, &x); zi_copy(&x, &y); zi_copy(&y, &t);
            continue;
        }
        /* apply matrix: (x', y') = (A*x + B*y, C*x + D*y), then take abs.
         * entries A,B,C,D are small (fit int64). Compute via u32 mul + add/sub. */
        /* nx = A*x + B*y ; ny = C*x + D*y  (signed combos) */
        #define COMBO(dst, M1, src1, M2, src2) do {                         \
            zint p1, p2; zi_init(&p1); zi_init(&p2);                         \
            int64_t m1 = (M1), m2 = (M2);                                   \
            zi_copy(&p1, (src1));                                           \
            if (m1 < 0) { zi_neg(&p1); m1 = -m1; }                          \
            mul_u64(&p1, (uint64_t)m1);                                     \
            zi_copy(&p2, (src2));                                           \
            if (m2 < 0) { zi_neg(&p2); m2 = -m2; }                          \
            mul_u64(&p2, (uint64_t)m2);                                     \
            zi_add((dst), &p1, &p2);                                        \
            zi_free(&p1); zi_free(&p2);                                     \
        } while (0)
        COMBO(&nx, A, &x, B, &y);
        COMBO(&ny, C, &x, D, &y);
        #undef COMBO
        nx.sign = nx.len ? nx.sign : 0;
        ny.sign = ny.len ? ny.sign : 0;
        /* x,y <- |nx|,|ny| */
        zi_copy(&x, &nx); x.sign = (x.len ? 1 : 0);
        zi_copy(&y, &ny); y.sign = (y.len ? 1 : 0);
        if (zi_cmp_abs(&x, &y) < 0) { zi_copy(&t, &x); zi_copy(&x, &y); zi_copy(&y, &t); }
    }
    zi_copy(g, &x);
done:
    zi_free(&x); zi_free(&y); zi_free(&t); zi_free(&nx); zi_free(&ny);
}
