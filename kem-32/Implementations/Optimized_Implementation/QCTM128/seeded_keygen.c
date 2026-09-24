#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scheme_api.h"
#include "seeded_keygen.h"
#include "CryptHash_AlgorithmInstance.h"

#define KEYGEN_MAX_ATTEMPTS 100000U

static scheme_keygen_stats_t last_keygen_stats;

void scheme_reset_last_keygen_stats(void)
{
    memset(&last_keygen_stats, 0, sizeof(last_keygen_stats));
}

const scheme_keygen_stats_t *scheme_last_keygen_stats(void)
{
    return &last_keygen_stats;
}

typedef struct scheme_pair {
    uint64_t h;
    uint32_t index;
} scheme_pair_t;

static size_t scheme_bits_to_bytes(size_t bits)
{
    return (bits + 7U) / 8U;
}

static int scheme_get_bit(const unsigned char *in, size_t bit)
{
    return (in[bit / 8U] >> (bit % 8U)) & 1U;
}

static void scheme_set_bit(unsigned char *out, size_t bit)
{
    out[bit / 8U] |= (unsigned char)(1U << (bit % 8U));
}

static uint64_t scheme_load_bits_le(const unsigned char *in,
                                  size_t bit_offset, size_t bit_len)
{
    uint64_t x = 0;
    size_t i;

    for (i = 0; i < bit_len; i++) {
        if (scheme_get_bit(in, bit_offset + i)) {
            x |= UINT64_C(1) << i;
        }
    }
    return x;
}

static void scheme_copy_bits(unsigned char *out, const unsigned char *in,
                           size_t bit_offset, size_t bit_len)
{
    size_t i;

    memset(out, 0, scheme_bits_to_bytes(bit_len));
    for (i = 0; i < bit_len; i++) {
        if (scheme_get_bit(in, bit_offset + i)) {
            scheme_set_bit(out, i);
        }
    }
}

static int scheme_eta_from_goppa_bits(gf_t eta, int t0, int sigma1,
                                    const unsigned char *bits,
                                    size_t bit_offset, size_t bit_len)
{
    uint32_t idx;
    int m = gf_extd();

    if (eta == NULL || t0 <= 0 || sigma1 < m || bits == NULL ||
        bit_len < (size_t)sigma1 * (size_t)(t0 + 1)) {
        return 0;
    }

    idx = (uint32_t)scheme_load_bits_le(
        bits, bit_offset + (size_t)sigma1 * (size_t)t0, (size_t)m);
    gf_from_index(eta, (gfindex_t)idx);
    return !gf_is_zero(eta);
}

static int scheme_pair_cmp(const void *a, const void *b)
{
    const scheme_pair_t *pa = (const scheme_pair_t *)a;
    const scheme_pair_t *pb = (const scheme_pair_t *)b;

    if (pa->h < pb->h) {
        return -1;
    }
    if (pa->h > pb->h) {
        return 1;
    }
    if (pa->index < pb->index) {
        return -1;
    }
    if (pa->index > pb->index) {
        return 1;
    }
    return 0;
}

static uint32_t scheme_reverse_bits(uint32_t x, unsigned m)
{
    uint32_t y = 0;
    unsigned i;

    for (i = 0; i < m; i++) {
        if (((x >> i) & 1U) != 0) {
            y |= UINT32_C(1) << (m - 1U - i);
        }
    }
    return y;
}

static int scheme_binomial_odd(unsigned n, unsigned k)
{
    return (k & ~n) == 0;
}

static int scheme_shift_from_eta(gf_t shift, gf_t eta)
{
    if (eta == NULL || gf_is_zero(eta)) {
        return 0;
    }
    gf_inv(shift, eta);
    return 1;
}

static void scheme_gf_pow_nonnegative(gf_t out, gf_t x, int e)
{
    int i;

    gf_set_to_unit(out);
    for (i = 0; i < e; i++) {
        gf_mul(out, out, x);
    }
}

static poly_t scheme_x_plus_c_pow_l(int l, gf_t c)
{
    int i;
    gf_t coeff;
    poly_t p;

    if (l <= 0) {
        return NULL;
    }

    p = poly_alloc(l);
    if (p == NULL) {
        return NULL;
    }

    for (i = 0; i <= l; i++) {
        if (scheme_binomial_odd((unsigned)l, (unsigned)i)) {
            scheme_gf_pow_nonnegative(coeff, c, l - i);
            poly_set_coeff(p, i, coeff);
        }
    }
    poly_calcule_deg(p);
    return p;
}

static uint32_t scheme_mix32(uint32_t x)
{
    x ^= x >> 16;
    x *= UINT32_C(0x7feb352d);
    x ^= x >> 15;
    x *= UINT32_C(0x846ca68b);
    x ^= x >> 16;
    return x;
}

static void scheme_set_gf_u32(gf_t out, uint32_t value)
{
    out[0] = (gfelt_t)(value & (uint32_t)gf_ord());
}

static poly_t scheme_submission_extension_modulus(int t0, int l,
                                                  int *handled)
{
    gf_t coeff;
    poly_t F;

    *handled = 0;
    if (gf_extd() != 18 || l != 19) {
        return NULL;
    }

    F = poly_alloc(t0);
    if (F == NULL) {
        *handled = 1;
        return NULL;
    }

    poly_set_to_zero(F);
    poly_set_coeff_to_unit(F, t0);

    switch (t0) {
    case 15:
        scheme_set_gf_u32(coeff, UINT32_C(1) << 7);
        poly_set_coeff(F, 0, coeff);
        gf_set_to_unit(coeff);
        poly_set_coeff(F, 7, coeff);
        *handled = 1;
        break;
    case 27:
        scheme_set_gf_u32(coeff, UINT32_C(1) << 1);
        poly_set_coeff(F, 0, coeff);
        *handled = 1;
        break;
    case 53:
        scheme_set_gf_u32(coeff, UINT32_C(1) << 1);
        poly_set_coeff(F, 0, coeff);
        gf_set_to_unit(coeff);
        poly_set_coeff(F, 6, coeff);
        *handled = 1;
        break;
    default:
        poly_free(F);
        return NULL;
    }

    poly_calcule_deg(F);
    if (poly_deg(F) == t0 && poly_degppf(F) == t0) {
        return F;
    }

    poly_free(F);
    return NULL;
}

static poly_t scheme_extension_modulus(int t0, int l)
{
    uint32_t attempt;
    gf_t a;
    int handled = 0;
    poly_t submission_F;

    if (t0 <= 0) {
        return NULL;
    }

    submission_F = scheme_submission_extension_modulus(t0, l, &handled);
    if (handled) {
        return submission_F;
    }

    for (attempt = 0; attempt < 4096U; attempt++) {
        int i;
        poly_t F = poly_alloc(t0);
        if (F == NULL) {
            return NULL;
        }

        poly_set_deg(F, t0);
        poly_set_coeff_to_unit(F, t0);
        for (i = 0; i < t0; i++) {
            uint32_t seed = (uint32_t)gf_extd() * UINT32_C(0x9e3779b1) ^
                            (uint32_t)t0 * UINT32_C(0x85ebca6b) ^
                            (uint32_t)l * UINT32_C(0xc2b2ae35) ^
                            attempt * UINT32_C(0x27d4eb2d) ^
                            (uint32_t)i * UINT32_C(0x165667b1);
            a[0] = scheme_mix32(seed) & (uint32_t)gf_ord();
            if (i == 0 && a[0] == 0) {
                a[0] = 1;
            }
            poly_set_coeff(F, i, a);
        }
        poly_calcule_deg(F);
        if (poly_deg(F) == t0 && poly_degppf(F) == t0) {
            return F;
        }
        poly_free(F);
    }
    return NULL;
}

static void scheme_ext_set(poly_t dst, poly_t src, int t0)
{
    int i;

    poly_set_to_zero(dst);
    for (i = 0; i < t0 && i < src->size; i++) {
        poly_set_coeff(dst, i, poly_coeff(src, i));
    }
    poly_calcule_deg(dst);
}

static void scheme_ext_addto(poly_t dst, poly_t src, int t0)
{
    int i;

    for (i = 0; i < t0; i++) {
        poly_addto_coeff(dst, i, poly_coeff(src, i));
    }
    poly_calcule_deg(dst);
}

static int scheme_ext_equal(poly_t a, poly_t b, int t0)
{
    int i;

    for (i = 0; i < t0; i++) {
        if (!gf_eq(poly_coeff(a, i), poly_coeff(b, i))) {
            return 0;
        }
    }
    return 1;
}

static int scheme_ext_is_zero(poly_t a, int t0)
{
    int i;

    for (i = 0; i < t0; i++) {
        if (!gf_is_zero(poly_coeff(a, i))) {
            return 0;
        }
    }
    return 1;
}

static int scheme_ext_is_one(poly_t a, int t0)
{
    int i;

    if (!gf_is_unit(poly_coeff(a, 0))) {
        return 0;
    }
    for (i = 1; i < t0; i++) {
        if (!gf_is_zero(poly_coeff(a, i))) {
            return 0;
        }
    }
    return 1;
}

static void scheme_ext_set_one(poly_t a)
{
    poly_set_to_zero(a);
    poly_set_coeff_to_unit(a, 0);
    poly_set_deg(a, 0);
}

static int scheme_ext_mul(poly_t out, poly_t a, poly_t b, poly_t F, int t0)
{
    poly_t product;

    if (poly_deg(a) < 0 || poly_deg(b) < 0) {
        poly_set_to_zero(out);
        return 1;
    }

    product = poly_mul(a, b);
    if (product == NULL) {
        return 0;
    }
    poly_rem(product, F);
    scheme_ext_set(out, product, t0);
    poly_free(product);
    return 1;
}

static int scheme_ext_square(poly_t out, poly_t a, poly_t F, int t0)
{
    return scheme_ext_mul(out, a, a, F, t0);
}

static int scheme_ext_frobenius_q(poly_t out, poly_t in, poly_t F, int t0, int m)
{
    int i;
    poly_t a = poly_alloc(t0 - 1);
    poly_t b = poly_alloc(t0 - 1);

    if (a == NULL || b == NULL) {
        poly_free(a);
        poly_free(b);
        return 0;
    }

    scheme_ext_set(a, in, t0);
    for (i = 0; i < m; i++) {
        poly_t tmp;
        if (!scheme_ext_square(b, a, F, t0)) {
            poly_free(a);
            poly_free(b);
            return 0;
        }
        tmp = a;
        a = b;
        b = tmp;
    }
    scheme_ext_set(out, a, t0);
    poly_free(a);
    poly_free(b);
    return 1;
}

static int scheme_ext_pow_bits(poly_t out, poly_t base,
                             const unsigned char *bits, int bit_len,
                             poly_t F, int t0)
{
    int i;
    poly_t result = poly_alloc(t0 - 1);
    poly_t power = poly_alloc(t0 - 1);
    poly_t tmp = poly_alloc(t0 - 1);

    if (result == NULL || power == NULL || tmp == NULL) {
        poly_free(result);
        poly_free(power);
        poly_free(tmp);
        return 0;
    }

    scheme_ext_set_one(result);
    scheme_ext_set(power, base, t0);
    for (i = 0; i < bit_len; i++) {
        if (bits[i] != 0) {
            if (!scheme_ext_mul(tmp, result, power, F, t0)) {
                poly_free(result);
                poly_free(power);
                poly_free(tmp);
                return 0;
            }
            scheme_ext_set(result, tmp, t0);
        }
        if (i + 1 < bit_len) {
            if (!scheme_ext_square(tmp, power, F, t0)) {
                poly_free(result);
                poly_free(power);
                poly_free(tmp);
                return 0;
            }
            scheme_ext_set(power, tmp, t0);
        }
    }
    scheme_ext_set(out, result, t0);
    poly_free(result);
    poly_free(power);
    poly_free(tmp);
    return 1;
}

static int scheme_ext_order_divisible_by_l(poly_t beta, poly_t F,
                                         int t0, int m, int l)
{
    int bit_len = m * t0;
    int i;
    int rem = 0;
    unsigned char *exp_bits;
    poly_t value;
    int divisible;

    if (scheme_ext_is_zero(beta, t0) || bit_len <= 0 || l <= 1) {
        return 0;
    }

    exp_bits = calloc((size_t)bit_len, 1);
    value = poly_alloc(t0 - 1);
    if (exp_bits == NULL || value == NULL) {
        free(exp_bits);
        poly_free(value);
        return 0;
    }

    for (i = bit_len - 1; i >= 0; i--) {
        rem = rem * 2 + 1;
        if (rem >= l) {
            exp_bits[i] = 1;
            rem -= l;
        }
    }
    if (rem != 0 || !scheme_ext_pow_bits(value, beta, exp_bits, bit_len,
                                       F, t0)) {
        free(exp_bits);
        poly_free(value);
        return 0;
    }

    divisible = !scheme_ext_is_one(value, t0);
    free(exp_bits);
    poly_free(value);
    return divisible;
}

static poly_t scheme_minpoly_from_beta(poly_t beta, poly_t F, int t0, int m)
{
    int i;
    int r;
    int ok = 0;
    poly_t *coeff = NULL;
    poly_t *next = NULL;
    poly_t current = NULL;
    poly_t conjugate = NULL;
    poly_t product = NULL;
    poly_t M = NULL;

    if (scheme_ext_is_zero(beta, t0)) {
        return NULL;
    }

    coeff = calloc((size_t)t0 + 1U, sizeof(*coeff));
    next = calloc((size_t)t0 + 1U, sizeof(*next));
    if (coeff == NULL || next == NULL) {
        goto cleanup;
    }
    for (i = 0; i <= t0; i++) {
        coeff[i] = poly_alloc(t0 - 1);
        next[i] = poly_alloc(t0 - 1);
        if (coeff[i] == NULL || next[i] == NULL) {
            goto cleanup;
        }
    }

    current = poly_alloc(t0 - 1);
    conjugate = poly_alloc(t0 - 1);
    product = poly_alloc(t0 - 1);
    if (current == NULL || conjugate == NULL || product == NULL) {
        goto cleanup;
    }

    scheme_ext_set_one(coeff[0]);
    scheme_ext_set(current, beta, t0);

    for (r = 0; r < t0; r++) {
        poly_t *swap;
        int j;

        for (j = 0; j <= t0; j++) {
            poly_set_to_zero(next[j]);
        }
        for (j = 0; j <= r; j++) {
            if (!scheme_ext_mul(product, coeff[j], current, F, t0)) {
                goto cleanup;
            }
            scheme_ext_addto(next[j], product, t0);
            scheme_ext_addto(next[j + 1], coeff[j], t0);
        }

        if (!scheme_ext_frobenius_q(conjugate, current, F, t0, m)) {
            goto cleanup;
        }
        if (r + 1 < t0 && scheme_ext_equal(conjugate, beta, t0)) {
            goto cleanup;
        }
        if (r + 1 == t0 && !scheme_ext_equal(conjugate, beta, t0)) {
            goto cleanup;
        }
        scheme_ext_set(current, conjugate, t0);

        swap = coeff;
        coeff = next;
        next = swap;
    }

    M = poly_alloc(t0);
    if (M == NULL) {
        goto cleanup;
    }
    for (i = 0; i <= t0; i++) {
        int j;
        for (j = 1; j < t0; j++) {
            if (!gf_is_zero(poly_coeff(coeff[i], j))) {
                goto cleanup;
            }
        }
        poly_set_coeff(M, i, poly_coeff(coeff[i], 0));
    }
    poly_calcule_deg(M);
    if (poly_deg(M) != t0 || poly_degppf(M) < t0) {
        goto cleanup;
    }
    ok = 1;

cleanup:
    if (coeff != NULL) {
        for (i = 0; i <= t0; i++) {
            poly_free(coeff[i]);
        }
        free(coeff);
    }
    if (next != NULL) {
        for (i = 0; i <= t0; i++) {
            poly_free(next[i]);
        }
        free(next);
    }
    poly_free(current);
    poly_free(conjugate);
    poly_free(product);
    if (!ok) {
        poly_free(M);
        return NULL;
    }
    return M;
}

gfelt_t *seeded_keygen_support(int n, int l, int m, gf_t eta,
                             const unsigned char *bits,
                             size_t bit_offset, size_t bit_len)
{
    uint32_t q;
    uint32_t selected_orbits;
    size_t sigma2;
    size_t i;
    uint32_t orbit_count = 0;
    scheme_pair_t *pairs = NULL;
    unsigned char *visited = NULL;
    gfelt_t *L = NULL;
    gf_t shift;

    if (n <= 0 || l <= 1 || m <= 0 || m >= 31 || n % l != 0 ||
        bits == NULL || gf_card() != (1 << m) ||
        !scheme_shift_from_eta(shift, eta)) {
        return NULL;
    }

    q = UINT32_C(1) << m;
    if ((uint32_t)(n / l) > (q - 1U) / (uint32_t)l ||
        bit_len % (q - 1U) != 0) {
        return NULL;
    }

    sigma2 = bit_len / (q - 1U);
    if (sigma2 < (size_t)(2 * m) || sigma2 > 63U) {
        return NULL;
    }

    selected_orbits = (uint32_t)(n / l);
    pairs = malloc((size_t)(q - 1U) * sizeof(*pairs));
    visited = calloc(q, 1);
    L = malloc((size_t)n * sizeof(*L));
    if (pairs == NULL || visited == NULL || L == NULL) {
        goto fail;
    }

    for (i = 0; i < (size_t)(q - 1U); i++) {
        pairs[i].h = scheme_load_bits_le(bits, bit_offset + i * sigma2, sigma2);
        pairs[i].index = (uint32_t)i + 1U;
    }
    qsort(pairs, (size_t)(q - 1U), sizeof(*pairs), scheme_pair_cmp);

    for (i = 1; i < (size_t)(q - 1U); i++) {
        if (pairs[i - 1U].h == pairs[i].h) {
            goto fail;
        }
    }

    for (i = 0; i < (size_t)(q - 1U) && orbit_count < selected_orbits; i++) {
        uint32_t x = scheme_reverse_bits(pairs[i].index, (unsigned)m);
        gf_t beta;
        gf_t zeta;
        int s;

        if (x == 0 || visited[x] != 0) {
            continue;
        }
        gf_from_index(beta, (gfindex_t)x);
        zeta[0] = gf_exp[gf_ord() / l];

        for (s = 0; s < l; s++) {
            if (gf_is_zero(beta)) {
                goto fail;
            }
            x = (uint32_t)gf_to_index(beta);
            if (x == 0 || x >= q || visited[x] != 0) {
                goto fail;
            }
            L[(size_t)orbit_count * (size_t)l + (size_t)s] =
                beta[0] ^ shift[0];
            visited[x] = 1;
            gf_mul(beta, beta, zeta);
        }
        orbit_count++;
    }

    if (orbit_count != selected_orbits) {
        goto fail;
    }

    free(pairs);
    free(visited);
    return L;

fail:
    free(pairs);
    free(visited);
    free(L);
    return NULL;
}

static poly_t seeded_keygen_goppa_poly_with_mod(int t, int l, int sigma1,
                                              gf_t eta,
                                              const unsigned char *bits,
                                              size_t bit_offset,
                                              size_t bit_len,
                                              poly_t extension_mod)
{
    int i;
    int t0;
    int m = gf_extd();
    poly_t beta = NULL;
    poly_t M = NULL;
    poly_t x_shift_l = NULL;
    poly_t g = NULL;
    gf_t a;
    gf_t shift;

    if (t <= 0 || l <= 0 || sigma1 < m || t % l != 0 ||
        bits == NULL || extension_mod == NULL ||
        !scheme_shift_from_eta(shift, eta)) {
        return NULL;
    }

    t0 = t / l;
    if (bit_len < (size_t)sigma1 * (size_t)t0) {
        return NULL;
    }

    beta = poly_alloc(t0 - 1);
    if (beta == NULL) {
        return NULL;
    }

    for (i = 0; i < t0; i++) {
        uint32_t idx = (uint32_t)scheme_load_bits_le(
            bits, bit_offset + (size_t)i * (size_t)sigma1, (size_t)m);
        gf_from_index(a, (gfindex_t)idx);
        poly_set_coeff(beta, i, a);
    }
    poly_calcule_deg(beta);

    M = scheme_minpoly_from_beta(beta, extension_mod, t0, m);
    if (M == NULL) {
        poly_free(beta);
        return NULL;
    }

    if (!scheme_ext_order_divisible_by_l(beta, extension_mod, t0, m, l)) {
        poly_free(beta);
        poly_free(M);
        return NULL;
    }

    x_shift_l = scheme_x_plus_c_pow_l(l, shift);
    if (x_shift_l == NULL) {
        poly_free(beta);
        poly_free(M);
        return NULL;
    }

    g = poly_compose(M, x_shift_l);
    poly_free(x_shift_l);
    poly_free(beta);
    poly_free(M);
    if (g == NULL) {
        return NULL;
    }
    if (poly_degppf(g) < t) {
        poly_free(g);
        return NULL;
    }
    return g;
}

poly_t seeded_keygen_goppa_poly(int t, int l, int sigma1, gf_t eta,
                              const unsigned char *bits,
                              size_t bit_offset, size_t bit_len)
{
    int t0;
    poly_t extension_mod;
    poly_t g;

    if (t <= 0 || l <= 0 || t % l != 0) {
        return NULL;
    }
    t0 = t / l;
    extension_mod = scheme_extension_modulus(t0, l);
    if (extension_mod == NULL) {
        return NULL;
    }
    g = seeded_keygen_goppa_poly_with_mod(t, l, sigma1, eta, bits, bit_offset,
                                        bit_len, extension_mod);
    poly_free(extension_mod);
    return g;
}

goppa_t scheme_keygen_seeded(int n, int l, int m, int t,
                           gf_t eta_out,
                           const unsigned char *seed, size_t seed_len,
                           unsigned char **pk,
                           unsigned char *decode_map,
                           unsigned char *delta_out,
                           unsigned char *fallback_s)
{
    size_t q;
    size_t ell_bits;
    size_t support_bits;
    size_t goppa_bits;
    size_t total_bits;
    size_t stream_len;
    size_t offset;
    size_t pk_len;
    unsigned char *delta = NULL;
    unsigned char *delta_next = NULL;
    unsigned char *stream = NULL;
    poly_t extension_mod = NULL;
    int trace_keygen = 0;
    uint32_t attempt;
    int alloc_pk;

    scheme_reset_last_keygen_stats();
    trace_keygen =
        getenv("LOCALLY_QUASI_CYCLIC_TWISTED_MCELIECE_TRACE_KEYGEN") != NULL;

    if (!goppa_check_params(n, m, l, t) || eta_out == NULL || seed == NULL ||
        seed_len != KEYGEN_SEED_BYTES || t % l != 0) {
        return NULL;
    }

    q = (size_t)1U << (unsigned)m;
    ell_bits = seed_len * 8U;
    support_bits = (size_t)(2 * m) * (q - 1U);
    goppa_bits = (size_t)m * (size_t)((t / l) + 1);
    total_bits = (size_t)n + support_bits + goppa_bits + ell_bits;
    stream_len = scheme_bits_to_bytes(total_bits);
    pk_len = PUBLICKEY_BYTES;

    if (stream_len > (size_t)2147483647) {
        return NULL;
    }

    delta = malloc(seed_len);
    delta_next = malloc(seed_len);
    stream = malloc(stream_len);
    if (delta == NULL || delta_next == NULL || stream == NULL) {
        free(delta);
        free(delta_next);
        free(stream);
        return NULL;
    }
    memcpy(delta, seed, seed_len);

    alloc_pk = ((pk != NULL) && (*pk == NULL));
    if (alloc_pk) {
        *pk = malloc(pk_len);
        if (*pk == NULL) {
            free(delta);
            free(delta_next);
            free(stream);
            return NULL;
        }
    }

    extension_mod = scheme_extension_modulus(t / l, l);
    if (extension_mod == NULL) {
        if (alloc_pk) {
            free(*pk);
            *pk = NULL;
        }
        free(delta);
        free(delta_next);
        free(stream);
        return NULL;
    }
    if (trace_keygen) {
        fprintf(stderr,
                "keygen: n=%d l=%d m=%d t=%d t0=%d stream_len=%zu pk=%s\n",
                n, l, m, t, t / l, stream_len, pk == NULL ? "no" : "yes");
        fflush(stderr);
    }

    for (attempt = 0; attempt < KEYGEN_MAX_ATTEMPTS; attempt++) {
        gfelt_t *L = NULL;
        poly_t g = NULL;
        goppa_t gamma = NULL;
        gf_t eta;
        size_t support_offset;
        size_t goppa_offset;

        last_keygen_stats.attempts_used = (unsigned long)attempt + 1UL;
        if (trace_keygen &&
            (attempt < 5U || (attempt % 100U) == 0U)) {
            fprintf(stderr, "keygen: attempt=%u shake\n", attempt + 1U);
            fflush(stderr);
        }

        if (CryptHash((int)(8U * stream_len), delta,
                      (unsigned long long)(8U * seed_len), stream) != 0) {
            if (alloc_pk) {
                free(*pk);
                *pk = NULL;
            }
            poly_free(extension_mod);
            free(delta);
            free(delta_next);
            free(stream);
            return NULL;
        }
        if (trace_keygen &&
            (attempt < 5U || (attempt % 100U) == 0U)) {
            fprintf(stderr, "keygen: attempt=%u support\n", attempt + 1U);
            fflush(stderr);
        }

        offset = 0;
        if (fallback_s != NULL) {
            scheme_copy_bits(fallback_s, stream, offset, (size_t)n);
        }
        offset += (size_t)n;

        scheme_copy_bits(delta_next, stream,
                       total_bits - ell_bits, ell_bits);

        support_offset = offset;
        goppa_offset = support_offset + support_bits;
        if (!scheme_eta_from_goppa_bits(eta, t / l, m, stream, goppa_offset,
                                        goppa_bits)) {
            last_keygen_stats.goppa_poly_rejects++;
            memcpy(delta, delta_next, seed_len);
            continue;
        }
        if (trace_keygen &&
            (attempt < 5U || (attempt % 100U) == 0U)) {
            fprintf(stderr, "keygen: attempt=%u goppa_poly\n", attempt + 1U);
            fflush(stderr);
        }

        g = seeded_keygen_goppa_poly_with_mod(t, l, m, eta, stream, goppa_offset,
                                            goppa_bits, extension_mod);
        if (g == NULL) {
            last_keygen_stats.goppa_poly_rejects++;
            memcpy(delta, delta_next, seed_len);
            continue;
        }

        L = seeded_keygen_support(n, l, m, eta, stream, support_offset,
                                  support_bits);
        if (L == NULL) {
            last_keygen_stats.support_rejects++;
            poly_free(g);
            memcpy(delta, delta_next, seed_len);
            continue;
        }

        if (pk != NULL) {
            if (trace_keygen &&
                (attempt < 5U || (attempt % 100U) == 0U)) {
                fprintf(stderr, "keygen: attempt=%u goppa_keygen\n",
                        attempt + 1U);
                fflush(stderr);
            }
            memset(*pk, 0, pk_len);
            if (goppa_keygen(L, g, n, l, t, eta, *pk, decode_map) != CODIMENSION) {
                last_keygen_stats.systematic_form_rejects++;
                free(L);
                poly_free(g);
                memcpy(delta, delta_next, seed_len);
                continue;
            }
            if (trace_keygen &&
                (attempt < 5U || (attempt % 100U) == 0U)) {
                fprintf(stderr, "keygen: attempt=%u goppa_keygen_done\n",
                        attempt + 1U);
                fflush(stderr);
            }
        }

        if (trace_keygen &&
            (attempt < 5U || (attempt % 100U) == 0U)) {
            fprintf(stderr, "keygen: attempt=%u goppa_init\n", attempt + 1U);
            fflush(stderr);
        }
        gamma = goppa_init(L, g, n, t, l, NULL, eta);
        if (gamma == NULL) {
            last_keygen_stats.goppa_init_rejects++;
            free(L);
            poly_free(g);
            memcpy(delta, delta_next, seed_len);
            continue;
        }

        if (delta_out != NULL) {
            memcpy(delta_out, delta, seed_len);
        }
        gf_set(eta_out, eta);
        last_keygen_stats.success = 1;
        if (trace_keygen) {
            fprintf(stderr, "keygen: attempt=%u success\n", attempt + 1U);
            fflush(stderr);
        }
        poly_free(extension_mod);
        free(delta);
        free(delta_next);
        free(stream);
        return gamma;
    }

    if (alloc_pk) {
        free(*pk);
        *pk = NULL;
    }
    free(delta);
    free(delta_next);
    free(stream);
    poly_free(extension_mod);
    return NULL;
}
