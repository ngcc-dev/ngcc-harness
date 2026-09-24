#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SIG_AlgorithmInstance.h"
#include "auxfunc.h"
#include "drng.h"
#include <sys/random.h>
#include <errno.h>

#if defined(FACTODSA_SO_BUILD)

DRNG_ctx drng_algorithm;

static int factodsa_ngcc_drng_ready = 0;

static int factodsa_ngcc_drng_init_once(void)
{
    unsigned char seed[64];

    if (factodsa_ngcc_drng_ready) {
        return 0;
    }

    ssize_t r = getentropy(seed, sizeof(seed));
    
    if (r != 0) {
        return -1;
    }

    int rc = init_random_number(&drng_algorithm, seed, sizeof seed);

    factodsa_ngcc_drng_ready = 1;
    return rc;
}

#define FACTODSA_NGCC_DRNG_INIT_OR_FAIL()              \
    do {                                               \
        if (factodsa_ngcc_drng_init_once() != 0) {     \
            return -1;                                 \
        }                                              \
    } while (0)

#else

extern DRNG_ctx drng_algorithm;

#define FACTODSA_NGCC_DRNG_INIT_OR_FAIL() ((void)0)

#endif

#define P FACTO_Q
#define FE_BYTES 2
#define FACTO_D (2 * FACTO_N - 1)
#define FACTO_S (FACTO_D - FACTO_M)
#define FACTO_V (2 * FACTO_N)
#define FACTO_Q_TERMS (((FACTO_N + 2) * (FACTO_N + 1) * FACTO_N) / 6)
#define FACTO_R_TERMS ((FACTO_N * (FACTO_N + 1)) / 2)
#define FACTO_QUAD_TERMS ((FACTO_V * (FACTO_V + 1)) / 2)
#define FACTO_CUBIC_TERMS (((FACTO_V + 2) * (FACTO_V + 1) * FACTO_V) / 6)
#define FACTO_PKH_BYTES 32
#define FACTO_RMAX 512
#define FACTO_MAX_FACTORS FACTO_D
#define FACTO_MAX_FACTOR_POWERS FACTO_MAX_FACTORS
#define FACTO_MAX_BIG_WORDS 96
#define FACTO_MAX_SCALES 64

#define SK_SINV_ELEMS (FACTO_V * FACTO_V)
#define SK_U_ELEMS (FACTO_D * FACTO_M)
#define SK_K_ELEMS (FACTO_S * FACTO_D)
#define SK_Q_ELEMS FACTO_Q_TERMS
#define SK_R_ELEMS (FACTO_N * FACTO_R_TERMS)
#define SK_ELEMS (SK_SINV_ELEMS + SK_U_ELEMS + SK_K_ELEMS + SK_Q_ELEMS + SK_R_ELEMS)

#define SK_SINV_OFFSET 0
#define SK_U_OFFSET (SK_SINV_OFFSET + SK_SINV_ELEMS * FE_BYTES)
#define SK_K_OFFSET (SK_U_OFFSET + SK_U_ELEMS * FE_BYTES)
#define SK_Q_OFFSET (SK_K_OFFSET + SK_K_ELEMS * FE_BYTES)
#define SK_R_OFFSET (SK_Q_OFFSET + SK_Q_ELEMS * FE_BYTES)
#define SK_PKH_OFFSET (SK_R_OFFSET + SK_R_ELEMS * FE_BYTES)
#define SK_BYTES (SK_PKH_OFFSET + FACTO_PKH_BYTES)
#define PK_BYTES (FACTO_M * FACTO_CUBIC_TERMS * FE_BYTES)
#define SN_BYTES (FACTO_V * FE_BYTES)

typedef uint16_t fe_t;

typedef struct {
    int deg;
    fe_t c[FACTO_D];
} poly_t;

typedef struct {
    int nwords;
    uint32_t w[FACTO_MAX_BIG_WORDS];
} bigint_t;

typedef struct {
    poly_t f;
    int e;
} factor_power_t;

static unsigned char rnd_pool[4096];
static size_t rnd_pool_pos = sizeof(rnd_pool);

/* -------------------------------------------------------------------------
 * Field arithmetic
 * ------------------------------------------------------------------------- */

static inline fe_t f_add(fe_t a, fe_t b)
{
    uint32_t s = (uint32_t)a + (uint32_t)b;
    if (s >= P) {
        s -= P;
    }
    return (fe_t)s;
}

static inline fe_t f_sub(fe_t a, fe_t b)
{
    if (a >= b) {
        return (fe_t)(a - b);
    }
    return (fe_t)((uint32_t)a + P - (uint32_t)b);
}

static inline fe_t f_neg(fe_t a)
{
    return a ? (fe_t)(P - a) : 0;
}

static inline fe_t f_mul(fe_t a, fe_t b)
{
    return (fe_t)(((uint64_t)a * (uint64_t)b) % P);
}

static fe_t f_pow(fe_t a, uint32_t e)
{
    fe_t r = 1;
    fe_t b = a;

    while (e) {
        if (e & 1u) {
            r = f_mul(r, b);
        }
        e >>= 1;
        if (e) {
            b = f_mul(b, b);
        }
    }
    return r;
}

static fe_t f_inv(fe_t a)
{
    return f_pow(a, P - 2u);
}

static int f_is_square(fe_t a)
{
    if (a == 0) {
        return 1;
    }
    return f_pow(a, (P - 1u) / 2u) == 1;
}

static fe_t f_sqrt(fe_t a)
{
    if (a == 0) {
        return 0;
    }
    return f_pow(a, (P + 1u) / 4u);
}

static void store_fe(unsigned char *p, fe_t x)
{
    p[0] = (unsigned char)(x & 0xffu);
    p[1] = (unsigned char)(x >> 8);
}

static fe_t load_fe(const unsigned char *p)
{
    return (fe_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static void store_fe_vec(unsigned char *out, const fe_t *in, int n)
{
    for (int i = 0; i < n; i++) {
        store_fe(out + (size_t)i * FE_BYTES, in[i]);
    }
}

static int load_fe_vec(fe_t *out, const unsigned char *in, int n)
{
    for (int i = 0; i < n; i++) {
        out[i] = load_fe(in + (size_t)i * FE_BYTES);
        if (out[i] >= P) {
            return -1;
        }
    }
    return 0;
}

/* -------------------------------------------------------------------------
 * Random generation
 * ------------------------------------------------------------------------- */

static int random_bytes(unsigned char *out, unsigned long long len)
{
    if (get_random_number(&drng_algorithm, out, len * 8ULL) != 0) {
        return -1;
    }
    return 0;
}

static int random_byte_pooled(unsigned char *b)
{
    if (rnd_pool_pos >= sizeof(rnd_pool)) {
        if (random_bytes(rnd_pool, sizeof(rnd_pool)) != 0) {
            return -1;
        }
        rnd_pool_pos = 0;
    }
    *b = rnd_pool[rnd_pool_pos++];
    return 0;
}

static int random_fe(fe_t *x)
{
    unsigned char b0 = 0;
    unsigned char b1 = 0;
    uint32_t v = 0;

    do {
        if (random_byte_pooled(&b0) != 0 || random_byte_pooled(&b1) != 0) {
            return -1;
        }
        v = (uint32_t)b0 | ((uint32_t)b1 << 8);
    } while (v >= P);

    *x = (fe_t)v;
    return 0;
}

static int random_nonzero_fe(fe_t *x)
{
    do {
        if (random_fe(x) != 0) {
            return -1;
        }
    } while (*x == 0);
    return 0;
}

/* -------------------------------------------------------------------------
 * Big integers
 * ------------------------------------------------------------------------- */

static void bigint_set_u32(bigint_t *x, uint32_t v)
{
    memset(x, 0, sizeof(*x));
    x->w[0] = v;
    x->nwords = v ? 1 : 0;
}

static int bigint_mul_small(bigint_t *x, uint32_t m)
{
    uint64_t carry = 0;

    for (int i = 0; i < x->nwords; i++) {
        uint64_t z = (uint64_t)x->w[i] * m + carry;
        x->w[i] = (uint32_t)z;
        carry = z >> 32;
    }

    if (carry) {
        if (x->nwords >= FACTO_MAX_BIG_WORDS) {
            return -1;
        }
        x->w[x->nwords++] = (uint32_t)carry;
    }
    return 0;
}

static void bigint_sub_one(bigint_t *x)
{
    int i = 0;

    while (i < x->nwords) {
        if (x->w[i]) {
            x->w[i]--;
            break;
        }
        x->w[i] = 0xffffffffu;
        i++;
    }

    while (x->nwords > 0 && x->w[x->nwords - 1] == 0) {
        x->nwords--;
    }
}

static void bigint_shr1(bigint_t *x)
{
    uint32_t carry = 0;

    for (int i = x->nwords - 1; i >= 0; i--) {
        uint32_t new_carry = x->w[i] & 1u;
        x->w[i] = (x->w[i] >> 1) | (carry << 31);
        carry = new_carry;
    }

    while (x->nwords > 0 && x->w[x->nwords - 1] == 0) {
        x->nwords--;
    }
}

static int bigint_bitlen(const bigint_t *x)
{
    uint32_t top = 0;
    int bits = 32;

    if (x->nwords == 0) {
        return 0;
    }

    top = x->w[x->nwords - 1];

    while (bits > 0 && ((top >> (bits - 1)) & 1u) == 0) {
        bits--;
    }

    return (x->nwords - 1) * 32 + bits;
}

static int bigint_get_bit(const bigint_t *x, int bit)
{
    int word = bit / 32;
    int offset = bit % 32;

    if (word >= x->nwords) {
        return 0;
    }
    return (x->w[word] >> offset) & 1u;
}

static int make_cz_exp(int d, bigint_t *e)
{
    bigint_set_u32(e, 1);

    for (int i = 0; i < d; i++) {
        if (bigint_mul_small(e, P) != 0) {
            return -1;
        }
    }

    bigint_sub_one(e);
    bigint_shr1(e);
    return 0;
}

/* -------------------------------------------------------------------------
 * Polynomial arithmetic
 * ------------------------------------------------------------------------- */

static void poly_zero(poly_t *a)
{
    memset(a->c, 0, sizeof(a->c));
    a->deg = -1;
}

static void poly_one(poly_t *a)
{
    poly_zero(a);
    a->c[0] = 1;
    a->deg = 0;
}

static void poly_trim(poly_t *a)
{
    int d = FACTO_D - 1;

    while (d >= 0 && a->c[d] == 0) {
        d--;
    }
    a->deg = d;
}

static void poly_copy(poly_t *dst, const poly_t *src)
{
    memcpy(dst, src, sizeof(poly_t));
}

static int poly_is_zero(const poly_t *a)
{
    return a->deg < 0;
}

static int poly_is_one(const poly_t *a)
{
    return a->deg == 0 && a->c[0] == 1;
}

static int poly_equal(const poly_t *a, const poly_t *b)
{
    if (a->deg != b->deg) {
        return 0;
    }
    if (a->deg < 0) {
        return 1;
    }
    for (int i = 0; i <= a->deg; i++) {
        if (a->c[i] != b->c[i]) {
            return 0;
        }
    }
    return 1;
}

static void poly_set_x(poly_t *a)
{
    poly_zero(a);
    a->c[1] = 1;
    a->deg = 1;
}

static void poly_make_monic(poly_t *a)
{
    fe_t inv = 0;

    if (a->deg < 0) {
        return;
    }

    inv = f_inv(a->c[a->deg]);
    for (int i = 0; i <= a->deg; i++) {
        a->c[i] = f_mul(a->c[i], inv);
    }
}

static void poly_sub_inplace(poly_t *a, const poly_t *b)
{
    int maxd = a->deg > b->deg ? a->deg : b->deg;

    for (int i = 0; i <= maxd; i++) {
        a->c[i] = f_sub(a->c[i], b->c[i]);
    }
    poly_trim(a);
}

static void poly_derivative(const poly_t *a, poly_t *d)
{
    poly_zero(d);

    if (a->deg <= 0) {
        return;
    }

    for (int i = 1; i <= a->deg; i++) {
        d->c[i - 1] = f_mul((fe_t)(i % (int)P), a->c[i]);
    }
    poly_trim(d);
}

static int poly_divmod(const poly_t *a, const poly_t *b, poly_t *q, poly_t *r)
{
    fe_t inv_lc = 0;

    if (b->deg < 0) {
        return -1;
    }

    poly_zero(q);
    poly_copy(r, a);

    if (a->deg < b->deg) {
        return 0;
    }

    inv_lc = f_inv(b->c[b->deg]);

    while (r->deg >= b->deg && r->deg >= 0) {
        int sh = r->deg - b->deg;
        fe_t coef = f_mul(r->c[r->deg], inv_lc);

        q->c[sh] = f_add(q->c[sh], coef);
        if (sh > q->deg) {
            q->deg = sh;
        }

        for (int i = 0; i <= b->deg; i++) {
            r->c[i + sh] = f_sub(r->c[i + sh], f_mul(coef, b->c[i]));
        }
        poly_trim(r);
    }

    poly_trim(q);
    return 0;
}

static void poly_mod(const poly_t *a, const poly_t *mod, poly_t *r)
{
    poly_t q;
    poly_divmod(a, mod, &q, r);
}

static void poly_gcd(const poly_t *a, const poly_t *b, poly_t *g)
{
    poly_t A;
    poly_t B;
    poly_t Q;
    poly_t R;

    poly_copy(&A, a);
    poly_copy(&B, b);

    if (poly_is_zero(&A)) {
        poly_copy(g, &B);
        poly_make_monic(g);
        return;
    }

    if (poly_is_zero(&B)) {
        poly_copy(g, &A);
        poly_make_monic(g);
        return;
    }

    while (!poly_is_zero(&B)) {
        poly_divmod(&A, &B, &Q, &R);
        poly_copy(&A, &B);
        poly_copy(&B, &R);
    }

    poly_copy(g, &A);
    poly_make_monic(g);
}

static int poly_mul_raw(const poly_t *a, const poly_t *b, poly_t *out)
{
    poly_zero(out);

    if (a->deg < 0 || b->deg < 0) {
        return 0;
    }

    if (a->deg + b->deg > FACTO_D - 1) {
        return -1;
    }

    for (int i = 0; i <= a->deg; i++) {
        for (int j = 0; j <= b->deg; j++) {
            out->c[i + j] = f_add(out->c[i + j], f_mul(a->c[i], b->c[j]));
        }
    }

    poly_trim(out);
    return 0;
}

static void poly_mul_mod(const poly_t *a, const poly_t *b, const poly_t *mod,
                         poly_t *out)
{
    fe_t tmp[2 * FACTO_D - 1] = {0};
    int tdeg = 0;
    int md = 0;

    if (a->deg < 0 || b->deg < 0) {
        poly_zero(out);
        return;
    }

    tdeg = a->deg + b->deg;

    for (int i = 0; i <= a->deg; i++) {
        for (int j = 0; j <= b->deg; j++) {
            tmp[i + j] = f_add(tmp[i + j], f_mul(a->c[i], b->c[j]));
        }
    }

    md = mod->deg;

    for (int k = tdeg; k >= md; k--) {
        fe_t coef = tmp[k];

        if (!coef) {
            continue;
        }

        for (int j = 0; j < md; j++) {
            tmp[k - md + j] = f_sub(tmp[k - md + j], f_mul(coef, mod->c[j]));
        }
        tmp[k] = 0;
    }

    poly_zero(out);
    for (int i = 0; i < md; i++) {
        out->c[i] = tmp[i];
    }
    poly_trim(out);
}

static void poly_powmod_u32(const poly_t *base, uint32_t exp, const poly_t *mod,
                            poly_t *out)
{
    poly_t result;
    poly_t b;
    poly_t tmp;

    poly_one(&result);
    poly_mod(base, mod, &b);

    while (exp) {
        if (exp & 1u) {
            poly_mul_mod(&result, &b, mod, &tmp);
            poly_copy(&result, &tmp);
        }
        exp >>= 1;
        if (exp) {
            poly_mul_mod(&b, &b, mod, &tmp);
            poly_copy(&b, &tmp);
        }
    }

    poly_copy(out, &result);
}

static void poly_powmod_big(const poly_t *base, const bigint_t *exp,
                            const poly_t *mod, poly_t *out)
{
    poly_t result;
    poly_t b;
    poly_t tmp;
    int bits = 0;

    poly_one(&result);
    poly_mod(base, mod, &b);
    bits = bigint_bitlen(exp);

    for (int i = bits - 1; i >= 0; i--) {
        poly_mul_mod(&result, &result, mod, &tmp);
        poly_copy(&result, &tmp);
        if (bigint_get_bit(exp, i)) {
            poly_mul_mod(&result, &b, mod, &tmp);
            poly_copy(&result, &tmp);
        }
    }

    poly_copy(out, &result);
}

static int random_poly_mod(poly_t *a, int deg_mod)
{
    poly_zero(a);

    if (deg_mod <= 0) {
        return 0;
    }

    for (int i = 0; i < deg_mod; i++) {
        if (random_fe(&a->c[i]) != 0) {
            return -1;
        }
    }
    poly_trim(a);
    return 0;
}

static int poly_pth_root(const poly_t *f, poly_t *root)
{
    poly_zero(root);

    if (f->deg < 0) {
        return -1;
    }

    for (int i = 0; i <= f->deg; i++) {
        if (f->c[i] == 0) {
            continue;
        }
        if ((i % (int)P) != 0) {
            return -2;
        }
        root->c[i / (int)P] = f->c[i];
    }

    poly_trim(root);
    return 0;
}

/* -------------------------------------------------------------------------
 * Polynomial factorisation
 * ------------------------------------------------------------------------- */

static int add_factor_power(factor_power_t *list, int *count,
                            const poly_t *f, int exponent)
{
    poly_t g;

    if (exponent <= 0) {
        return -1;
    }

    poly_copy(&g, f);
    poly_trim(&g);

    if (g.deg <= 0) {
        return 0;
    }

    poly_make_monic(&g);

    for (int i = 0; i < *count; i++) {
        if (poly_equal(&list[i].f, &g)) {
            list[i].e += exponent;
            return 0;
        }
    }

    if (*count >= FACTO_MAX_FACTOR_POWERS) {
        return -2;
    }

    poly_copy(&list[*count].f, &g);
    list[*count].e = exponent;
    (*count)++;
    return 0;
}

static int equal_degree_factor_power(const poly_t *f, int d, int exponent,
                                     factor_power_t *list, int *count)
{
    poly_t stack[FACTO_MAX_FACTORS];
    bigint_t exp;
    int sp = 0;

    if (make_cz_exp(d, &exp) != 0) {
        return -1;
    }

    poly_copy(&stack[sp++], f);

    while (sp > 0) {
        poly_t g;
        int split = 0;

        poly_copy(&g, &stack[--sp]);
        poly_make_monic(&g);

        if (g.deg <= 0) {
            continue;
        }

        if (g.deg == d) {
            if (add_factor_power(list, count, &g, exponent) != 0) {
                return -2;
            }
            continue;
        }

        for (int tries = 0; tries < 1024 && !split; tries++) {
            poly_t a;
            poly_t b;
            poly_t gg;
            poly_t q;
            poly_t r;

            if (tries < (int)P) {
                poly_zero(&a);
                a.c[0] = (fe_t)(tries % (int)P);
                a.c[1] = 1;
                a.deg = 1;
            } else if (random_poly_mod(&a, g.deg) != 0) {
                return -3;
            }

            poly_powmod_big(&a, &exp, &g, &b);

            if (b.deg >= 0) {
                b.c[0] = f_sub(b.c[0], 1);
            } else {
                poly_zero(&b);
                b.c[0] = (fe_t)(P - 1);
                b.deg = 0;
            }

            poly_trim(&b);
            poly_gcd(&g, &b, &gg);

            if (gg.deg > 0 && gg.deg < g.deg) {
                poly_divmod(&g, &gg, &q, &r);
                if (!poly_is_zero(&r)) {
                    continue;
                }
                poly_make_monic(&gg);
                poly_make_monic(&q);
                if (sp + 2 > FACTO_MAX_FACTORS) {
                    return -4;
                }
                stack[sp++] = gg;
                stack[sp++] = q;
                split = 1;
            }
        }

        if (!split) {
            return -5;
        }
    }

    return 0;
}

static int factor_squarefree_irreducible(const poly_t *fin, int exponent,
                                         factor_power_t *list, int *count)
{
    poly_t f;
    poly_t x;
    poly_t h;
    poly_t hp;
    poly_t diff;
    poly_t g;
    poly_t q;
    poly_t r;
    int d = 1;

    poly_copy(&f, fin);
    poly_trim(&f);

    if (f.deg <= 0) {
        return 0;
    }

    poly_make_monic(&f);

    if (f.deg == 1) {
        return add_factor_power(list, count, &f, exponent);
    }

    poly_set_x(&x);
    poly_mod(&x, &f, &h);

    while (f.deg >= 2 * d) {
        poly_powmod_u32(&h, P, &f, &hp);
        poly_copy(&h, &hp);
        poly_copy(&diff, &h);
        poly_sub_inplace(&diff, &x);
        poly_gcd(&f, &diff, &g);

        if (g.deg > 0) {
            if (equal_degree_factor_power(&g, d, exponent, list, count) != 0) {
                return -1;
            }
            poly_divmod(&f, &g, &q, &r);
            if (!poly_is_zero(&r)) {
                return -2;
            }
            poly_copy(&f, &q);
            poly_make_monic(&f);
            if (f.deg <= 0) {
                return 0;
            }
            poly_mod(&h, &f, &hp);
            poly_copy(&h, &hp);
        }
        d++;
    }

    if (f.deg > 0) {
        poly_make_monic(&f);
        if (add_factor_power(list, count, &f, exponent) != 0) {
            return -3;
        }
    }
    return 0;
}

static int factor_poly_recursive(const poly_t *input, int exponent_multiplier,
                                 factor_power_t *list, int *count)
{
    poly_t f;
    poly_t fp;
    poly_t c;
    poly_t w;
    poly_t y;
    poly_t z;
    poly_t q;
    poly_t r;
    poly_t root;
    int i = 1;

    poly_copy(&f, input);
    poly_trim(&f);

    if (f.deg <= 0) {
        return 0;
    }

    poly_make_monic(&f);
    poly_derivative(&f, &fp);

    if (poly_is_zero(&fp)) {
        if (poly_pth_root(&f, &root) != 0) {
            return -1;
        }
        return factor_poly_recursive(&root, exponent_multiplier * (int)P, list, count);
    }

    poly_gcd(&f, &fp, &c);
    poly_divmod(&f, &c, &w, &r);

    if (!poly_is_zero(&r)) {
        return -2;
    }

    while (!poly_is_one(&w)) {
        poly_gcd(&w, &c, &y);
        poly_divmod(&w, &y, &z, &r);

        if (!poly_is_zero(&r)) {
            return -3;
        }

        if (z.deg > 0) {
            if (factor_squarefree_irreducible(&z, exponent_multiplier * i, list, count) != 0) {
                return -4;
            }
        }

        poly_copy(&w, &y);

        if (!poly_is_one(&c)) {
            poly_divmod(&c, &y, &q, &r);
            if (!poly_is_zero(&r)) {
                return -5;
            }
            poly_copy(&c, &q);
        }
        i++;
    }

    if (!poly_is_one(&c)) {
        if (poly_pth_root(&c, &root) != 0) {
            return -6;
        }
        if (factor_poly_recursive(&root, exponent_multiplier * (int)P, list, count) != 0) {
            return -7;
        }
    }

    return 0;
}

static int factor_poly(const poly_t *input, fe_t *unit,
                       factor_power_t *list, int *count)
{
    poly_t f;

    *count = 0;
    poly_copy(&f, input);
    poly_trim(&f);

    if (f.deg < 0) {
        return -1;
    }

    if (f.deg == 0) {
        *unit = f.c[0];
        return 0;
    }

    *unit = f.c[f.deg];
    poly_make_monic(&f);
    return factor_poly_recursive(&f, 1, list, count);
}

static int expand_factor_powers(const factor_power_t *powers, int npowers,
                                poly_t *expanded, int *nexpanded)
{
    int out = 0;

    if (npowers < 0 || npowers > FACTO_MAX_FACTOR_POWERS) {
        return -1;
    }

    for (int i = 0; i < npowers; i++) {
        if (powers[i].e <= 0) {
            return -2;
        }
        for (int j = 0; j < powers[i].e; j++) {
            if (out >= FACTO_MAX_FACTORS) {
                return -3;
            }
            poly_copy(&expanded[out], &powers[i].f);
            out++;
        }
    }

    *nexpanded = out;
    return 0;
}

static int split_factorisation(const poly_t *f, fe_t unit,
                               const poly_t *factors, int nf,
                               poly_t *a, poly_t *b)
{
    int D = f->deg;
    int lower = D - FACTO_N + 1;
    int upper = D;
    int dp[FACTO_D] = {0};
    int prev_s[FACTO_D];
    int prev_i[FACTO_D];
    int chosen[FACTO_MAX_FACTORS] = {0};
    int target = -1;
    int mid = D / 2;

    if (lower < 0) {
        lower = 0;
    }
    if (upper > FACTO_N - 1) {
        upper = FACTO_N - 1;
    }

    for (int i = 0; i < FACTO_D; i++) {
        prev_s[i] = -1;
        prev_i[i] = -1;
    }

    dp[0] = 1;

    for (int i = 0; i < nf; i++) {
        int d = factors[i].deg;
        for (int s = D - d; s >= 0; s--) {
            if (dp[s] && !dp[s + d]) {
                dp[s + d] = 1;
                prev_s[s + d] = s;
                prev_i[s + d] = i;
            }
        }
    }

    for (int radius = 0; radius <= D; radius++) {
        int t1 = mid - radius;
        int t2 = mid + radius;
        if (t1 >= lower && t1 <= upper && dp[t1]) {
            target = t1;
            break;
        }
        if (t2 >= lower && t2 <= upper && dp[t2]) {
            target = t2;
            break;
        }
    }

    if (target < 0) {
        return -1;
    }

    while (target > 0) {
        int i = prev_i[target];
        if (i < 0) {
            return -2;
        }
        chosen[i] = 1;
        target = prev_s[target];
    }

    poly_zero(a);
    a->c[0] = unit;
    a->deg = 0;
    poly_one(b);

    for (int i = 0; i < nf; i++) {
        poly_t tmp;
        if (chosen[i]) {
            if (poly_mul_raw(a, &factors[i], &tmp) != 0) {
                return -3;
            }
            poly_copy(a, &tmp);
        } else {
            if (poly_mul_raw(b, &factors[i], &tmp) != 0) {
                return -4;
            }
            poly_copy(b, &tmp);
        }
    }

    if (a->deg >= FACTO_N || b->deg >= FACTO_N) {
        return -5;
    }
    return 0;
}

static int poly_from_vector(const fe_t *v, poly_t *f)
{
    poly_zero(f);

    for (int i = 0; i < FACTO_D; i++) {
        f->c[i] = v[i];
    }

    poly_trim(f);
    if (f->deg < 0) {
        return -1;
    }
    return 0;
}

/* -------------------------------------------------------------------------
 * Matrix arithmetic
 * ------------------------------------------------------------------------- */

static int random_matrix(fe_t *A, int rows, int cols)
{
    for (int i = 0; i < rows * cols; i++) {
        if (random_fe(&A[i]) != 0) {
            return -1;
        }
    }
    return 0;
}

static int matrix_inverse(const fe_t *A, fe_t *Inv, int n)
{
    fe_t *aug = NULL;
    int width = 2 * n;

    aug = (fe_t *)calloc((size_t)n * (size_t)width, sizeof(fe_t));
    if (!aug) {
        return -2;
    }

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            aug[i * width + j] = A[i * n + j];
        }
        aug[i * width + n + i] = 1;
    }

    for (int col = 0; col < n; col++) {
        int piv = -1;

        for (int r = col; r < n; r++) {
            if (aug[r * width + col] != 0) {
                piv = r;
                break;
            }
        }

        if (piv < 0) {
            free(aug);
            return -1;
        }

        if (piv != col) {
            for (int j = 0; j < width; j++) {
                fe_t t = aug[col * width + j];
                aug[col * width + j] = aug[piv * width + j];
                aug[piv * width + j] = t;
            }
        }

        {
            fe_t invp = f_inv(aug[col * width + col]);
            for (int j = 0; j < width; j++) {
                aug[col * width + j] = f_mul(aug[col * width + j], invp);
            }
        }

        for (int r = 0; r < n; r++) {
            fe_t factor = 0;
            if (r == col) {
                continue;
            }
            factor = aug[r * width + col];
            if (factor == 0) {
                continue;
            }
            for (int j = 0; j < width; j++) {
                aug[r * width + j] = f_sub(aug[r * width + j], f_mul(factor, aug[col * width + j]));
            }
        }
    }

    for (int i = 0; i < n; i++) {
        memcpy(Inv + i * n, aug + i * width + n, (size_t)n * sizeof(fe_t));
    }

    free(aug);
    return 0;
}

static int random_invertible_matrix(fe_t *A, fe_t *Inv, int n)
{
    int rc = 0;

    for (int tries = 0; tries < 256; tries++) {
        if (random_matrix(A, n, n) != 0) {
            return -3;
        }
        rc = matrix_inverse(A, Inv, n);
        if (rc == 0) {
            return 0;
        }
        if (rc == -2) {
            return -2;
        }
    }
    return -4;
}

static void matrix_vec_mul(const fe_t *A, const fe_t *x, fe_t *y,
                           int rows, int cols)
{
    for (int i = 0; i < rows; i++) {
        fe_t acc = 0;
        for (int j = 0; j < cols; j++) {
            acc = f_add(acc, f_mul(A[i * cols + j], x[j]));
        }
        y[i] = acc;
    }
}

static int compute_fibre_maps(const fe_t *T, fe_t *U, fe_t *K)
{
    fe_t *A = NULL;
    int pivots[FACTO_M];
    int is_pivot[FACTO_D] = {0};
    int free_cols[FACTO_S];
    int rank = 0;
    int width = FACTO_D + FACTO_M;
    int nf = 0;

    A = (fe_t *)calloc((size_t)FACTO_M * (size_t)width, sizeof(fe_t));
    if (!A) {
        return -2;
    }

    for (int i = 0; i < FACTO_M; i++) {
        for (int j = 0; j < FACTO_D; j++) {
            A[i * width + j] = T[i * FACTO_D + j];
        }
        A[i * width + FACTO_D + i] = 1;
    }

    for (int col = 0; col < FACTO_D && rank < FACTO_M; col++) {
        int piv = -1;

        for (int r = rank; r < FACTO_M; r++) {
            if (A[r * width + col] != 0) {
                piv = r;
                break;
            }
        }

        if (piv < 0) {
            continue;
        }

        if (piv != rank) {
            for (int j = 0; j < width; j++) {
                fe_t t = A[rank * width + j];
                A[rank * width + j] = A[piv * width + j];
                A[piv * width + j] = t;
            }
        }

        {
            fe_t invp = f_inv(A[rank * width + col]);
            for (int j = 0; j < width; j++) {
                A[rank * width + j] = f_mul(A[rank * width + j], invp);
            }
        }

        for (int r = 0; r < FACTO_M; r++) {
            fe_t factor = 0;
            if (r == rank) {
                continue;
            }
            factor = A[r * width + col];
            if (!factor) {
                continue;
            }
            for (int j = 0; j < width; j++) {
                A[r * width + j] = f_sub(A[r * width + j], f_mul(factor, A[rank * width + j]));
            }
        }

        pivots[rank] = col;
        is_pivot[col] = 1;
        rank++;
    }

    if (rank != FACTO_M) {
        free(A);
        return -1;
    }

    memset(U, 0, (size_t)FACTO_D * FACTO_M * sizeof(fe_t));
    memset(K, 0, (size_t)FACTO_S * FACTO_D * sizeof(fe_t));

    for (int r = 0; r < FACTO_M; r++) {
        int pc = pivots[r];
        for (int j = 0; j < FACTO_M; j++) {
            U[pc * FACTO_M + j] = A[r * width + FACTO_D + j];
        }
    }

    for (int c = 0; c < FACTO_D; c++) {
        if (!is_pivot[c]) {
            if (nf >= FACTO_S) {
                free(A);
                return -3;
            }
            free_cols[nf++] = c;
        }
    }

    if (nf != FACTO_S) {
        free(A);
        return -4;
    }

    for (int f = 0; f < FACTO_S; f++) {
        int fc = free_cols[f];
        K[f * FACTO_D + fc] = 1;
        for (int r = 0; r < FACTO_M; r++) {
            int pc = pivots[r];
            K[f * FACTO_D + pc] = f_neg(A[r * width + fc]);
        }
    }

    free(A);
    return 0;
}

static int random_full_rank_T(fe_t *T, fe_t *U, fe_t *K)
{
    int rc = 0;

    for (int tries = 0; tries < 256; tries++) {
        if (random_matrix(T, FACTO_M, FACTO_D) != 0) {
            return -2;
        }
        rc = compute_fibre_maps(T, U, K);
        if (rc == 0) {
            return 0;
        }
        if (rc == -2) {
            return -3;
        }
    }
    return -1;
}

/* -------------------------------------------------------------------------
 * Central maps
 * ------------------------------------------------------------------------- */

static int q_offset(int i)
{
    return (i * (i + 1) * (i + 2)) / 6;
}

static int r_monom_index(int i, int j)
{
    if (i > j) {
        int t = i;
        i = j;
        j = t;
    }
    return i * FACTO_N - (i * (i - 1)) / 2 + (j - i);
}

static int random_Q(fe_t *Q)
{
    int pos = 0;

    for (int i = 0; i < FACTO_N; i++) {
        if (random_nonzero_fe(&Q[pos++]) != 0) {
            return -1;
        }
        for (int u = 0; u < i; u++) {
            if (random_fe(&Q[pos++]) != 0) {
                return -1;
            }
        }
        for (int u = 0; u < i; u++) {
            for (int v = u; v < i; v++) {
                if (random_fe(&Q[pos++]) != 0) {
                    return -1;
                }
            }
        }
    }
    return 0;
}

static int random_R(fe_t *R)
{
    for (int i = 0; i < FACTO_N * FACTO_R_TERMS; i++) {
        if (random_fe(&R[i]) != 0) {
            return -1;
        }
    }
    return 0;
}

static void eval_R(const fe_t *R, const fe_t *y, fe_t *out)
{
    for (int i = 0; i < FACTO_N; i++) {
        fe_t acc = 0;
        const fe_t *Ri = R + i * FACTO_R_TERMS;
        for (int u = 0; u < FACTO_N; u++) {
            for (int v = u; v < FACTO_N; v++) {
                acc = f_add(acc, f_mul(Ri[r_monom_index(u, v)], f_mul(y[u], y[v])));
            }
        }
        out[i] = acc;
    }
}

static int invert_Q_rec(const fe_t *Q, const fe_t *w, fe_t *x, int i)
{
    int pos = q_offset(i);
    fe_t a = Q[pos++];
    fe_t b = 0;
    fe_t c = 0;
    fe_t rhs = 0;
    fe_t delta = 0;
    fe_t root = 0;
    fe_t inv2a = f_inv(f_add(a, a));

    for (int u = 0; u < i; u++) {
        b = f_add(b, f_mul(Q[pos++], x[u]));
    }

    for (int u = 0; u < i; u++) {
        for (int v = u; v < i; v++) {
            c = f_add(c, f_mul(Q[pos++], f_mul(x[u], x[v])));
        }
    }

    rhs = f_sub(w[i], c);
    delta = f_add(f_mul(b, b), f_mul(f_add(a, a), f_add(rhs, rhs)));

    if (!f_is_square(delta)) {
        return -1;
    }

    root = f_sqrt(delta);
    x[i] = f_mul(f_sub(root, b), inv2a);
    if (i + 1 == FACTO_N || invert_Q_rec(Q, w, x, i + 1) == 0) {
        return 0;
    }

    if (root != 0) {
        x[i] = f_mul(f_sub(f_neg(root), b), inv2a);
        if (i + 1 == FACTO_N || invert_Q_rec(Q, w, x, i + 1) == 0) {
            return 0;
        }
    }

    x[i] = 0;
    return -1;
}

static int invert_Q(const fe_t *Q, const fe_t *w, fe_t *x)
{
    memset(x, 0, FACTO_N * sizeof(fe_t));
    return invert_Q_rec(Q, w, x, 0);
}

/* -------------------------------------------------------------------------
 * Hashing and public cubic map
 * ------------------------------------------------------------------------- */

static void sm3_digest(const unsigned char *in, unsigned long long in_len,
                       unsigned char out[FACTO_PKH_BYTES])
{
    sm3hash(256, in, in_len * 8ULL, out);
}

static int xof_field(const unsigned char *pkh, const unsigned char *m,
                     unsigned long long m_len, fe_t *h)
{
    const unsigned char label[] = "FDSA-H-128";
    unsigned long long base_len = 0;
    unsigned long long off = 0;
    unsigned char *buf = NULL;
    unsigned char block[128] = {0};
    int got = 0;
    uint32_t ctr = 0;

    base_len = sizeof(label) - 1 + FACTO_PKH_BYTES + m_len + 4;
    buf = (unsigned char *)malloc((size_t)base_len);
    if (!buf) {
        return -2;
    }

    memcpy(buf + off, label, sizeof(label) - 1);
    off += sizeof(label) - 1;
    memcpy(buf + off, pkh, FACTO_PKH_BYTES);
    off += FACTO_PKH_BYTES;
    memcpy(buf + off, m, (size_t)m_len);
    off += m_len;

    while (got < FACTO_M) {
        buf[off + 0] = (unsigned char)(ctr >> 24);
        buf[off + 1] = (unsigned char)(ctr >> 16);
        buf[off + 2] = (unsigned char)(ctr >> 8);
        buf[off + 3] = (unsigned char)ctr;

        if (pseudoXOF(sizeof(block) * 8ULL, buf, base_len * 8ULL, block) != 0) {
            free(buf);
            return -3;
        }

        for (unsigned int i = 0; i + 1 < sizeof(block) && got < FACTO_M; i += 2) {
            uint32_t v = (uint32_t)block[i] | ((uint32_t)block[i + 1] << 8);
            if (v < P) {
                h[got++] = (fe_t)v;
            }
        }

        ctr++;
        if (ctr == 0) {
            free(buf);
            return -4;
        }
    }

    free(buf);
    return 0;
}

static int quad_index(int i, int j)
{
    if (i > j) {
        int t = i;
        i = j;
        j = t;
    }
    return i * FACTO_V - (i * (i - 1)) / 2 + (j - i);
}

static int cubic_index(int i, int j, int k)
{
    int idx = 0;

    if (i > j) {
        int t = i;
        i = j;
        j = t;
    }
    if (j > k) {
        int t = j;
        j = k;
        k = t;
    }
    if (i > j) {
        int t = i;
        i = j;
        j = t;
    }

    for (int a = 0; a < i; a++) {
        idx += ((FACTO_V - a + 1) * (FACTO_V - a)) / 2;
    }
    for (int b = i; b < j; b++) {
        idx += FACTO_V - b;
    }
    idx += k - j;
    return idx;
}

static void add_linlin_to_quad(fe_t *row, fe_t coeff, const fe_t *A,
                               const fe_t *B, const uint16_t *qidx)
{
    if (!coeff) {
        return;
    }

    for (int i = 0; i < FACTO_V; i++) {
        fe_t ai = A[i];
        if (!ai) {
            continue;
        }
        for (int j = 0; j < FACTO_V; j++) {
            fe_t v = f_mul(coeff, f_mul(ai, B[j]));
            if (v) {
                int idx = qidx[i * FACTO_V + j];
                row[idx] = f_add(row[idx], v);
            }
        }
    }
}

static void add_quad_to_cubic(fe_t *row, const fe_t *A,
                              const fe_t *B, const uint16_t *tidx)
{
    int q = 0;

    for (int i = 0; i < FACTO_V; i++) {
        for (int j = i; j < FACTO_V; j++, q++) {
            fe_t aij = A[q];
            if (!aij) {
                continue;
            }
            for (int k = 0; k < FACTO_V; k++) {
                fe_t v = f_mul(aij, B[k]);
                if (v) {
                    int idx = tidx[(i * FACTO_V + j) * FACTO_V + k];
                    row[idx] = f_add(row[idx], v);
                }
            }
        }
    }
}

static int build_public_key(const fe_t *S, const fe_t *T, const fe_t *Q,
                            const fe_t *R, unsigned char *pk)
{
    uint16_t *qidx = NULL;
    uint16_t *tidx = NULL;
    fe_t *A = NULL;
    fe_t *C = NULL;

    qidx = (uint16_t *)malloc((size_t)FACTO_V * FACTO_V * sizeof(uint16_t));
    tidx = (uint16_t *)malloc((size_t)FACTO_V * FACTO_V * FACTO_V * sizeof(uint16_t));
    A = (fe_t *)calloc((size_t)FACTO_N * FACTO_QUAD_TERMS, sizeof(fe_t));
    C = (fe_t *)calloc((size_t)FACTO_D * FACTO_CUBIC_TERMS, sizeof(fe_t));
    if (!qidx || !tidx || !A || !C) {
        free(qidx);
        free(tidx);
        free(A);
        free(C);
        return -2;
    }

    for (int i = 0; i < FACTO_V; i++) {
        for (int j = 0; j < FACTO_V; j++) {
            qidx[i * FACTO_V + j] = (uint16_t)quad_index(i, j);
            for (int k = 0; k < FACTO_V; k++) {
                tidx[(i * FACTO_V + j) * FACTO_V + k] = (uint16_t)cubic_index(i, j, k);
            }
        }
    }

    for (int a = 0; a < FACTO_N; a++) {
        fe_t *Arow = A + (size_t)a * FACTO_QUAD_TERMS;
        int qpos = q_offset(a);
        const fe_t *L1a = S + (size_t)a * FACTO_V;

        add_linlin_to_quad(Arow, Q[qpos++], L1a, L1a, qidx);

        for (int u = 0; u < a; u++) {
            const fe_t *Lu = S + (size_t)u * FACTO_V;
            add_linlin_to_quad(Arow, Q[qpos++], Lu, L1a, qidx);
        }

        for (int u = 0; u < a; u++) {
            for (int v = u; v < a; v++) {
                const fe_t *Lu = S + (size_t)u * FACTO_V;
                const fe_t *Lv = S + (size_t)v * FACTO_V;
                add_linlin_to_quad(Arow, Q[qpos++], Lu, Lv, qidx);
            }
        }

        for (int u = 0; u < FACTO_N; u++) {
            for (int v = u; v < FACTO_N; v++) {
                const fe_t *Lu = S + (size_t)(FACTO_N + u) * FACTO_V;
                const fe_t *Lv = S + (size_t)(FACTO_N + v) * FACTO_V;
                fe_t rc = R[a * FACTO_R_TERMS + r_monom_index(u, v)];
                add_linlin_to_quad(Arow, rc, Lu, Lv, qidx);
            }
        }
    }

    for (int l = 0; l < FACTO_D; l++) {
        fe_t *Crow = C + (size_t)l * FACTO_CUBIC_TERMS;
        int amin = l - (FACTO_N - 1);
        int amax = l;

        if (amin < 0) {
            amin = 0;
        }
        if (amax > FACTO_N - 1) {
            amax = FACTO_N - 1;
        }

        for (int a = amin; a <= amax; a++) {
            int b = l - a;
            const fe_t *Yb = S + (size_t)(FACTO_N + b) * FACTO_V;
            const fe_t *Arow = A + (size_t)a * FACTO_QUAD_TERMS;
            add_quad_to_cubic(Crow, Arow, Yb, tidx);
        }
    }

    memset(pk, 0, PK_BYTES);

    for (int r = 0; r < FACTO_M; r++) {
        fe_t *Prow = (fe_t *)calloc(FACTO_CUBIC_TERMS, sizeof(fe_t));
        if (!Prow) {
            free(qidx);
            free(tidx);
            free(A);
            free(C);
            return -3;
        }

        for (int l = 0; l < FACTO_D; l++) {
            fe_t trl = T[r * FACTO_D + l];
            const fe_t *Crow = C + (size_t)l * FACTO_CUBIC_TERMS;

            if (!trl) {
                continue;
            }

            for (int q = 0; q < FACTO_CUBIC_TERMS; q++) {
                Prow[q] = f_add(Prow[q], f_mul(trl, Crow[q]));
            }
        }

        for (int q = 0; q < FACTO_CUBIC_TERMS; q++) {
            store_fe(pk + ((size_t)r * FACTO_CUBIC_TERMS + q) * FE_BYTES, Prow[q]);
        }

        free(Prow);
    }

    free(qidx);
    free(tidx);
    free(A);
    free(C);
    return 0;
}

static int eval_public_key(const unsigned char *pk, const fe_t *z, fe_t *out)
{
    int qidx = 0;

    for (int r = 0; r < FACTO_M; r++) {
        out[r] = 0;
    }

    for (int i = 0; i < FACTO_V; i++) {
        for (int j = i; j < FACTO_V; j++) {
            fe_t zij = f_mul(z[i], z[j]);
            for (int k = j; k < FACTO_V; k++, qidx++) {
                fe_t zijk = f_mul(zij, z[k]);
                if (!zijk) {
                    continue;
                }
                for (int r = 0; r < FACTO_M; r++) {
                    fe_t c = load_fe(pk + ((size_t)r * FACTO_CUBIC_TERMS + qidx) * FE_BYTES);
                    if (c >= P) {
                        return -1;
                    }
                    if (c) {
                        out[r] = f_add(out[r], f_mul(c, zijk));
                    }
                }
            }
        }
    }

    return 0;
}

/* -------------------------------------------------------------------------
 * Secret-key packing
 * ------------------------------------------------------------------------- */

static void sk_store_fe_array(unsigned char *sk, size_t off, const fe_t *x, int n)
{
    for (int i = 0; i < n; i++) {
        store_fe(sk + off + (size_t)i * FE_BYTES, x[i]);
    }
}

static int sk_load_fe_array(const unsigned char *sk, size_t off, fe_t *x, int n)
{
    for (int i = 0; i < n; i++) {
        x[i] = load_fe(sk + off + (size_t)i * FE_BYTES);
        if (x[i] >= P) {
            return -1;
        }
    }
    return 0;
}

/* -------------------------------------------------------------------------
 * Signing helpers
 * ------------------------------------------------------------------------- */

static void sample_completion(const fe_t *U, const fe_t *K, const fe_t *h, fe_t *c)
{
    matrix_vec_mul(U, h, c, FACTO_D, FACTO_M);

    for (int s = 0; s < FACTO_S; s++) {
        fe_t beta = 0;
        random_fe(&beta);
        for (int i = 0; i < FACTO_D; i++) {
            c[i] = f_add(c[i], f_mul(beta, K[s * FACTO_D + i]));
        }
    }
}

static void poly_to_nvec(const poly_t *f, fe_t *v)
{
    memset(v, 0, FACTO_N * sizeof(fe_t));
    for (int i = 0; i <= f->deg && i < FACTO_N; i++) {
        v[i] = f->c[i];
    }
}

static int try_split_solution(const fe_t *Sinv, const fe_t *Q, const fe_t *R,
                              const poly_t *a, const poly_t *b, fe_t scale,
                              fe_t *zvec)
{
    fe_t X[FACTO_N] = {0};
    fe_t Y[FACTO_N] = {0};
    fe_t W[FACTO_N] = {0};
    fe_t RY[FACTO_N] = {0};
    fe_t target[FACTO_N] = {0};
    fe_t XY[FACTO_V] = {0};
    fe_t ainv = f_inv(scale);

    poly_to_nvec(a, W);
    poly_to_nvec(b, Y);

    for (int i = 0; i < FACTO_N; i++) {
        W[i] = f_mul(scale, W[i]);
        Y[i] = f_mul(ainv, Y[i]);
    }

    eval_R(R, Y, RY);

    for (int i = 0; i < FACTO_N; i++) {
        target[i] = f_sub(W[i], RY[i]);
    }

    if (invert_Q(Q, target, X) != 0) {
        return -1;
    }

    for (int i = 0; i < FACTO_N; i++) {
        XY[i] = X[i];
        XY[FACTO_N + i] = Y[i];
    }

    matrix_vec_mul(Sinv, XY, zvec, FACTO_V, FACTO_V);
    return 0;
}

/* -------------------------------------------------------------------------
 * Public SIG API
 * ------------------------------------------------------------------------- */

unsigned long long sig_get_pk_len_bytes(void)
{
    return PK_BYTES;
}

unsigned long long sig_get_sk_len_bytes(void)
{
    return SK_BYTES;
}

unsigned long long sig_get_sn_len_bytes(void)
{
    return SN_BYTES;
}

int sig_keygen(unsigned char *pk, unsigned long long *pk_len_bytes,
               unsigned char *sk, unsigned long long *sk_len_bytes)
{

    FACTODSA_NGCC_DRNG_INIT_OR_FAIL();

    fe_t *S = NULL;
    fe_t *Sinv = NULL;
    fe_t *T = NULL;
    fe_t *U = NULL;
    fe_t *K = NULL;
    fe_t *Q = NULL;
    fe_t *R = NULL;
    int rc = 0;

    rnd_pool_pos = sizeof(rnd_pool);

    if (!pk || !sk || !pk_len_bytes || !sk_len_bytes) {
        return -1;
    }

    S = (fe_t *)malloc((size_t)FACTO_V * FACTO_V * sizeof(fe_t));
    Sinv = (fe_t *)malloc((size_t)FACTO_V * FACTO_V * sizeof(fe_t));
    T = (fe_t *)malloc((size_t)FACTO_M * FACTO_D * sizeof(fe_t));
    U = (fe_t *)malloc((size_t)FACTO_D * FACTO_M * sizeof(fe_t));
    K = (fe_t *)malloc((size_t)FACTO_S * FACTO_D * sizeof(fe_t));
    Q = (fe_t *)malloc((size_t)FACTO_Q_TERMS * sizeof(fe_t));
    R = (fe_t *)malloc((size_t)FACTO_N * FACTO_R_TERMS * sizeof(fe_t));

    if (!S || !Sinv || !T || !U || !K || !Q || !R) {
        rc = -2;
        goto cleanup;
    }

    if (random_invertible_matrix(S, Sinv, FACTO_V) != 0) {
        rc = -3;
        goto cleanup;
    }

    if (random_full_rank_T(T, U, K) != 0) {
        rc = -4;
        goto cleanup;
    }

    if (random_Q(Q) != 0 || random_R(R) != 0) {
        rc = -5;
        goto cleanup;
    }

    if (build_public_key(S, T, Q, R, pk) != 0) {
        rc = -6;
        goto cleanup;
    }

    sk_store_fe_array(sk, SK_SINV_OFFSET, Sinv, SK_SINV_ELEMS);
    sk_store_fe_array(sk, SK_U_OFFSET, U, SK_U_ELEMS);
    sk_store_fe_array(sk, SK_K_OFFSET, K, SK_K_ELEMS);
    sk_store_fe_array(sk, SK_Q_OFFSET, Q, SK_Q_ELEMS);
    sk_store_fe_array(sk, SK_R_OFFSET, R, SK_R_ELEMS);
    sm3_digest(pk, PK_BYTES, sk + SK_PKH_OFFSET);

    *pk_len_bytes = PK_BYTES;
    *sk_len_bytes = SK_BYTES;

cleanup:
    free(S);
    free(Sinv);
    free(T);
    free(U);
    free(K);
    free(Q);
    free(R);
    return rc;
}

int sig_sign(unsigned char *sk, unsigned long long sk_len_bytes,
             unsigned char *m, unsigned long long m_len_bytes,
             unsigned char *sn, unsigned long long *sn_len_bytes)
{

    FACTODSA_NGCC_DRNG_INIT_OR_FAIL();

    fe_t Sinv[SK_SINV_ELEMS];
    fe_t U[SK_U_ELEMS];
    fe_t K[SK_K_ELEMS];
    fe_t Q[SK_Q_ELEMS];
    fe_t R[SK_R_ELEMS];
    const unsigned char *pkh = NULL;
    fe_t h[FACTO_M] = {0};
    fe_t c[FACTO_D] = {0};
    fe_t zvec[FACTO_V] = {0};
    poly_t f;
    poly_t expanded_factors[FACTO_MAX_FACTORS];
    poly_t a;
    poly_t b;
    factor_power_t factor_powers[FACTO_MAX_FACTOR_POWERS];
    fe_t unit = 0;
    int nf_powers = 0;
    int nf_expanded = 0;

    rnd_pool_pos = sizeof(rnd_pool);

    if (!sk || !m || !sn || !sn_len_bytes) {
        return -1;
    }

    if (sk_len_bytes != SK_BYTES) {
        return -2;
    }

    if (sk_load_fe_array(sk, SK_SINV_OFFSET, Sinv, SK_SINV_ELEMS) != 0 ||
        sk_load_fe_array(sk, SK_U_OFFSET, U, SK_U_ELEMS) != 0 ||
        sk_load_fe_array(sk, SK_K_OFFSET, K, SK_K_ELEMS) != 0 ||
        sk_load_fe_array(sk, SK_Q_OFFSET, Q, SK_Q_ELEMS) != 0 ||
        sk_load_fe_array(sk, SK_R_OFFSET, R, SK_R_ELEMS) != 0) {
        return -3;
    }

    pkh = sk + SK_PKH_OFFSET;

    if (xof_field(pkh, m, m_len_bytes, h) != 0) {
        return -4;
    }

    for (int trial = 0; trial < FACTO_RMAX; trial++) {
        sample_completion(U, K, h, c);

        if (poly_from_vector(c, &f) != 0) {
            continue;
        }

        nf_powers = 0;
        nf_expanded = 0;

        if (factor_poly(&f, &unit, factor_powers, &nf_powers) != 0) {
            continue;
        }

        if (expand_factor_powers(factor_powers, nf_powers, expanded_factors, &nf_expanded) != 0) {
            continue;
        }

        if (split_factorisation(&f, unit, expanded_factors, nf_expanded, &a, &b) != 0) {
            continue;
        }

        for (int sc = 0; sc < FACTO_MAX_SCALES; sc++) {
            fe_t scale = 1;
            if (sc != 0 && random_nonzero_fe(&scale) != 0) {
                return -5;
            }
            if (try_split_solution(Sinv, Q, R, &a, &b, scale, zvec) == 0) {
                store_fe_vec(sn, zvec, FACTO_V);
                *sn_len_bytes = SN_BYTES;
                return 0;
            }
        }
    }

    return -6;
}

int sig_verify(unsigned char *pk, unsigned long long pk_len_bytes,
               unsigned char *sn, unsigned long long sn_len_bytes,
               unsigned char *m, unsigned long long m_len_bytes)
{
    fe_t zvec[FACTO_V] = {0};
    fe_t h[FACTO_M] = {0};
    fe_t pz[FACTO_M] = {0};
    unsigned char pkh[FACTO_PKH_BYTES] = {0};

    if (!pk || !sn || !m) {
        return -2;
    }

    if (pk_len_bytes != PK_BYTES || sn_len_bytes != SN_BYTES) {
        return -2;
    }

    if (load_fe_vec(zvec, sn, FACTO_V) != 0) {
        return -1;
    }

    for (unsigned long long i = 0; i + 1 < pk_len_bytes; i += FE_BYTES) {
        if (load_fe(pk + i) >= P) {
            return -1;
        }
    }

    sm3_digest(pk, pk_len_bytes, pkh);

    if (xof_field(pkh, m, m_len_bytes, h) != 0) {
        return -2;
    }

    if (eval_public_key(pk, zvec, pz) != 0) {
        return -2;
    }

    for (int i = 0; i < FACTO_M; i++) {
        if (pz[i] != h[i]) {
            return -1;
        }
    }

    return 0;
}
