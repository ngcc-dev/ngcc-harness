#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "kem.h"
#include "rng.h"
#include "scheme_api.h"
#include "goppa.h"
#include "m2e.h"
#include "seeded_keygen.h"
#include "CryptHash_AlgorithmInstance.h"
#if __has_include("qctm_portable.h")
#include "qctm_portable.h"
#else
#include "../qctm_portable.h"
#endif

static int debug_last_error[ERROR_WEIGHT];
static int debug_last_error_valid = 0;

static int trace_dec_enabled(void)
{
    return getenv("LOCALLY_QUASI_CYCLIC_TWISTED_MCELIECE_TRACE_DEC") != NULL;
}

static int profile_dec_enabled(void)
{
    const char *profile = getenv("QCTM_PROFILE_DEC");

    return profile != NULL && profile[0] != '\0' && profile[0] != '0';
}

static double monotonic_ms(void)
{
    return qctm_now_ms();
}

void xor(unsigned char *a, unsigned char *b, int length)
{
    int i;

    for (i = 0; i < length; i++) {
        a[i] ^= b[i];
    }
}

static int scheme_public_t_aligned(void)
{
    return (SYSTEMATIC_QC_ROWS % ORDER) == 0 &&
           ((LENGTH - SYSTEMATIC_QC_ROWS) % ORDER) == 0 &&
           SYSTEMATIC_TAIL_ROWS == 2 &&
           CODIMENSION == MATGEN_TOTAL_ROWS &&
           PUBLICKEY_BITS == PUBLIC_T_BITS;
}

static int pk_bit(const unsigned char *pk, int bit)
{
    return (pk[(size_t)bit / 8U] >> (bit % 8)) & 1U;
}

static int shifted_public_position(int public_pos, int shift)
{
    int block = public_pos / ORDER;
    int in_block = public_pos % ORDER;

    return block * ORDER + ((in_block + ORDER - shift) % ORDER);
}

static int hybrid_pk_t_bit(const unsigned char *pk, int row, int col)
{
    if (row < SYSTEMATIC_QC_ROWS) {
        int base_block = row / ORDER;
        int shift = row % ORDER;
        int public_pos = col;
        int src_public_pos = shifted_public_position(public_pos, shift);

        return pk_bit(pk, base_block * PUBLIC_T_COLUMNS + src_public_pos);
    }
    return pk_bit(pk, PSI_T1_BITS +
                  (row - SYSTEMATIC_QC_ROWS) * PUBLIC_T_COLUMNS + col);
}

static void syndrome_xor_bytes(unsigned char *dst, const unsigned char *src)
{
    size_t i = 0;

    for (; i + sizeof(uint64_t) <= (size_t)SYNDROME_BYTES;
         i += sizeof(uint64_t)) {
        uint64_t a;
        uint64_t b;

        memcpy(&a, dst + i, sizeof(a));
        memcpy(&b, src + i, sizeof(b));
        a ^= b;
        memcpy(dst + i, &a, sizeof(a));
    }
    for (; i < (size_t)SYNDROME_BYTES; i++) {
        dst[i] ^= src[i];
    }
}

static void syndrome_toggle_bit(unsigned char *s, int bit)
{
    s[bit / 8] ^= (unsigned char)(1U << (bit % 8));
}

static int build_encrypt_column_cache(unsigned char *cols,
                                      const unsigned char *pk)
{
    int col;

    if (cols == NULL || pk == NULL) {
        return FAIL;
    }
    memset(cols, 0, (size_t)PUBLIC_T_COLUMNS * (size_t)SYNDROME_BYTES);
    for (col = 0; col < PUBLIC_T_COLUMNS; col++) {
        int row;
        unsigned char *dst = cols + (size_t)col * (size_t)SYNDROME_BYTES;

        for (row = 0; row < CODIMENSION; row++) {
            if (hybrid_pk_t_bit(pk, row, col)) {
                syndrome_toggle_bit(dst, row);
            }
        }
    }
    return SUCCESS;
}

static int get_encrypt_column_cache(const unsigned char *pk,
                                    unsigned char **cols_out)
{
    static unsigned char *cached_cols = NULL;
    static unsigned char *cached_pk = NULL;
    static int cached_valid = 0;
    int cache_hit;

    *cols_out = NULL;
    if (getenv("QCTM_DISABLE_ENC_CACHE") != NULL) {
        return FAIL;
    }

    if (cached_cols == NULL) {
        cached_cols = malloc((size_t)PUBLIC_T_COLUMNS *
                             (size_t)SYNDROME_BYTES);
        if (cached_cols == NULL) {
            return FAIL;
        }
        cached_valid = 0;
    }
    if (cached_pk == NULL) {
        cached_pk = malloc(PUBLICKEY_BYTES);
        if (cached_pk == NULL) {
            return FAIL;
        }
        cached_valid = 0;
    }

    cache_hit = cached_valid &&
                memcmp(cached_pk, pk, PUBLICKEY_BYTES) == 0;
    if (!cache_hit) {
        if (build_encrypt_column_cache(cached_cols, pk) != SUCCESS) {
            cached_valid = 0;
            return FAIL;
        }
        memcpy(cached_pk, pk, PUBLICKEY_BYTES);
        cached_valid = 1;
    }

    *cols_out = cached_cols;
    return SUCCESS;
}

static void poly_to_bytes(unsigned char *out, poly_t p, int degree)
{
    int i;
    int j;

    memset(out, 0, FULL_SYNDROME_BYTES);
    for (i = 0; i < degree; i++) {
        gfindex_t x = gf_to_index(poly_coeff(p, i));

        for (j = 0; j < gf_extd(); j++) {
            if ((x >> j) & 1U) {
                int bit = i * gf_extd() + j;

                out[bit / 8] ^= (unsigned char)(1U << (bit % 8));
            }
        }
    }
}

static void bytes_to_poly(const unsigned char *in, poly_t p, int degree)
{
    int i;
    int j;

    poly_set_to_zero(p);
    for (i = 0; i < degree; i++) {
        gfindex_t x = 0;
        gf_t a;

        for (j = 0; j < gf_extd(); j++) {
            int bit = i * gf_extd() + j;

            if ((in[bit / 8] >> (bit % 8)) & 1U) {
                x ^= (gfindex_t)(1U << j);
            }
        }
        gf_from_index(a, x);
        poly_set_coeff(p, i, a);
    }
    poly_calcule_deg(p);
}

static int debug_sun_syndrome_to_sui_definition(poly_t R, poly_t S, poly_t g)
{
    int i;
    int j;
    int t;
    gf_t coeff;
    gf_t term;

    if (R == NULL || S == NULL || g == NULL) {
        return 0;
    }
    t = poly_deg(g);
    if (t <= 0) {
        return 0;
    }

    poly_set_to_zero(R);
    for (i = 0; i < t; i++) {
        gf_set_to_zero(coeff);
        for (j = i + 1; j <= t; j++) {
            gf_mul(term, poly_coeff(g, j), poly_coeff(S, j - 1 - i));
            gf_add(coeff, coeff, term);
        }
        poly_set_coeff(R, i, coeff);
    }
    poly_calcule_deg(R);
    return 1;
}

static int sun_syndrome_bytes_to_definition(unsigned char *out,
                                            const unsigned char *sun,
                                            poly_t g)
{
    poly_t sun_poly;
    poly_t def_poly;

    sun_poly = poly_alloc(GOPPA_DEGREE);
    def_poly = poly_alloc(GOPPA_DEGREE);
    if (sun_poly == NULL || def_poly == NULL) {
        poly_free(sun_poly);
        poly_free(def_poly);
        return FAIL;
    }

    bytes_to_poly(sun, sun_poly, GOPPA_DEGREE);
    if (!debug_sun_syndrome_to_sui_definition(def_poly, sun_poly, g)) {
        poly_free(sun_poly);
        poly_free(def_poly);
        return FAIL;
    }
    poly_to_bytes(out, def_poly, GOPPA_DEGREE);

    poly_free(sun_poly);
    poly_free(def_poly);
    return SUCCESS;
}

static int compare_debug_full_syndrome(const unsigned char *actual,
                                       gfelt_t *L, poly_t g, gf_t eta)
{
    int i;
    int diff = 0;
    poly_t acc;
    poly_t col;
    poly_t sun_col;
    poly_t def_col;
    unsigned char expected[FULL_SYNDROME_BYTES];

    if (!debug_last_error_valid) {
        return 0;
    }

    acc = poly_alloc(GOPPA_DEGREE);
    col = poly_alloc(GOPPA_DEGREE);
    sun_col = poly_alloc(GOPPA_DEGREE);
    def_col = poly_alloc(GOPPA_DEGREE);
    if (acc == NULL || col == NULL || sun_col == NULL || def_col == NULL) {
        poly_free(acc);
        poly_free(col);
        poly_free(sun_col);
        poly_free(def_col);
        return -1;
    }

    poly_set_to_zero(acc);
    for (i = 0; i < ERROR_WEIGHT; i++) {
        int j;

        poly_syndrome_twisted_explicit_row(col, L + debug_last_error[i],
                                           g, eta, DEFAULT_TWIST_ROW);
        poly_set_to_zero(sun_col);
        for (j = 0; j < GOPPA_DEGREE - 1; j++) {
            poly_set_coeff(sun_col, j, poly_coeff(col, j));
        }
        poly_set_coeff_to_unit(sun_col, GOPPA_DEGREE - 1);
        poly_calcule_deg(sun_col);
        if (!debug_sun_syndrome_to_sui_definition(def_col, sun_col, g)) {
            poly_free(acc);
            poly_free(col);
            poly_free(sun_col);
            poly_free(def_col);
            return -1;
        }
        for (j = 0; j < GOPPA_DEGREE; j++) {
            poly_addto_coeff(acc, j, poly_coeff(def_col, j));
        }
    }
    poly_calcule_deg(acc);
    poly_to_bytes(expected, acc, GOPPA_DEGREE);
    for (i = 0; i < FULL_SYNDROME_BYTES; i++) {
        if (expected[i] != actual[i]) {
            diff++;
        }
    }
    poly_free(acc);
    poly_free(col);
    poly_free(sun_col);
    poly_free(def_col);
    return diff;
}

static void error_to_vector(unsigned char *e, const int *error)
{
    int i;

    memset(e, 0, BITS_TO_BYTES(LENGTH));
    for (i = 0; i < ERROR_WEIGHT; i++) {
        e[error[i] / 8] ^= (unsigned char)(1U << (error[i] % 8));
    }
}

static void set_bit_msb(unsigned char *out, size_t bit, int value)
{
    if (value) {
        out[bit / 8U] |= (unsigned char)(1U << (7U - (bit % 8U)));
    }
}

static int get_bit_msb(const unsigned char *in, size_t bit)
{
    return (in[bit / 8U] >> (7U - (bit % 8U))) & 1U;
}

static int get_bit_lsb(const unsigned char *in, size_t bit)
{
    return (in[bit / 8U] >> (bit % 8U)) & 1U;
}

static void set_bit_lsb(unsigned char *out, size_t bit, int value)
{
    if (value) {
        out[bit / 8U] |= (unsigned char)(1U << (bit % 8U));
    }
}

static void append_field_element_msb(unsigned char *out, size_t *bitpos,
                                     gf_t a)
{
    int j;
    gfindex_t x = gf_to_index(a);

    for (j = 0; j < EXT_DEGREE; j++) {
        set_bit_msb(out, *bitpos, (int)((x >> j) & 1U));
        (*bitpos)++;
    }
}

static void read_field_element_msb(gf_t a, const unsigned char *in,
                                   size_t *bitpos)
{
    int j;
    gfindex_t x = 0;

    for (j = 0; j < EXT_DEGREE; j++) {
        if (get_bit_msb(in, *bitpos)) {
            x |= (gfindex_t)(1U << j);
        }
        (*bitpos)++;
    }
    gf_from_index(a, x);
}

static int serialize_gamma_prime(unsigned char *out, poly_t g, gfelt_t *L)
{
    int i;
    size_t bitpos = 0;

    if (out == NULL || g == NULL || L == NULL) {
        return FAIL;
    }

    memset(out, 0, SECRETKEY_GAMMA_BYTES);
    for (i = 0; i < GOPPA_DEGREE; i++) {
        append_field_element_msb(out, &bitpos, poly_coeff(g, i));
    }
    for (i = 0; i < LENGTH; i++) {
        append_field_element_msb(out, &bitpos, L + i);
    }

    return bitpos == (size_t)SECRETKEY_GAMMA_BITS ? SUCCESS : FAIL;
}

static int deserialize_gamma_prime(const unsigned char *in, poly_t g,
                                   gfelt_t *L, gf_t eta)
{
    int i;
    size_t bitpos = 0;

    if (in == NULL || g == NULL || L == NULL || eta == NULL) {
        return FAIL;
    }

    poly_set_to_zero(g);
    for (i = 0; i < GOPPA_DEGREE; i++) {
        read_field_element_msb(poly_coeff(g, i), in, &bitpos);
    }
    poly_set_coeff_to_unit(g, GOPPA_DEGREE);
    poly_set_deg(g, GOPPA_DEGREE);
    for (i = 0; i < LENGTH; i++) {
        read_field_element_msb(L + i, in, &bitpos);
    }
    if (bitpos != (size_t)SECRETKEY_GAMMA_BITS ||
        gf_is_zero(poly_coeff(g, GOPPA_DEGREE - 1))) {
        return FAIL;
    }
    gf_inv(eta, poly_coeff(g, GOPPA_DEGREE - 1));
    return SUCCESS;
}

static int build_dec_gamma_from_sk(const unsigned char *sk, goppa_t *gamma_out)
{
    goppa_t gamma;
    gfelt_t *L;
    poly_t g;
    gf_t eta;

    L = malloc((size_t)LENGTH * sizeof(*L));
    if (L == NULL) {
        return FAIL;
    }

    g = poly_alloc(GOPPA_DEGREE);
    if (g == NULL) {
        free(L);
        return FAIL;
    }
    if (deserialize_gamma_prime(sk, g, L, eta) != SUCCESS) {
        free(L);
        poly_free(g);
        return FAIL;
    }

    gamma = goppa_init(L, g, LENGTH, GOPPA_DEGREE, ORDER, NULL, eta);
    if (gamma == NULL) {
        free(L);
        poly_free(g);
        return FAIL;
    }

    *gamma_out = gamma;
    return SUCCESS;
}

static goppa_t cached_dec_gamma = NULL;
static unsigned char cached_dec_gamma_key[SECRETKEY_GAMMA_BYTES];
static int cached_dec_gamma_valid = 0;

static int dec_gamma_cache_disabled(void)
{
    const char *disable = getenv("QCTM_DISABLE_DEC_GAMMA_CACHE");

    return disable != NULL && disable[0] != '\0' && disable[0] != '0';
}

static int cache_dec_gamma_from_sk(const unsigned char *sk, goppa_t gamma)
{
    if (sk == NULL || gamma == NULL || dec_gamma_cache_disabled()) {
        return FAIL;
    }

    if (cached_dec_gamma != NULL && cached_dec_gamma != gamma) {
        free_goppa(cached_dec_gamma);
    }
    cached_dec_gamma = gamma;
    memcpy(cached_dec_gamma_key, sk, SECRETKEY_GAMMA_BYTES);
    cached_dec_gamma_valid = 1;
    return SUCCESS;
}

static int get_dec_gamma(const unsigned char *sk, goppa_t *gamma_out,
                         int *owned_out)
{
    goppa_t gamma = NULL;

    *gamma_out = NULL;
    *owned_out = 0;

    if (!dec_gamma_cache_disabled() && cached_dec_gamma_valid &&
        memcmp(cached_dec_gamma_key, sk, SECRETKEY_GAMMA_BYTES) == 0) {
        *gamma_out = cached_dec_gamma;
        return SUCCESS;
    }

    if (build_dec_gamma_from_sk(sk, &gamma) != SUCCESS) {
        return FAIL;
    }

    if (!dec_gamma_cache_disabled() &&
        cache_dec_gamma_from_sk(sk, gamma) == SUCCESS) {
        *gamma_out = cached_dec_gamma;
        return SUCCESS;
    }

    *gamma_out = gamma;
    *owned_out = 1;
    return SUCCESS;
}

static void pack_bits_msb_from_lsb(unsigned char *out,
                                   const unsigned char *in,
                                   size_t bit_len)
{
    size_t i;

    memset(out, 0, BITS_TO_BYTES(bit_len));
    for (i = 0; i < bit_len; i++) {
        set_bit_msb(out, i, get_bit_lsb(in, i));
    }
}

static void unpack_bits_lsb_from_msb(unsigned char *out,
                                     const unsigned char *in,
                                     size_t bit_len)
{
    size_t i;

    memset(out, 0, BITS_TO_BYTES(bit_len));
    for (i = 0; i < bit_len; i++) {
        set_bit_lsb(out, i, get_bit_msb(in, i));
    }
}

static int hash_session_key(unsigned char *ss, unsigned char b,
                            const unsigned char *e,
                            const unsigned char *ct)
{
    size_t e_bytes = BITS_TO_BYTES(LENGTH);
    size_t in_len = 1U + e_bytes + CIPHERTEXT_BYTES;
    unsigned char in[1U + BITS_TO_BYTES(LENGTH) + CIPHERTEXT_BYTES];

    if (in_len > sizeof(in) || in_len > (size_t)4294967295U) {
        return FAIL;
    }

    in[0] = b;
    memcpy(in + 1, e, e_bytes);
    memcpy(in + 1 + e_bytes, ct, CIPHERTEXT_BYTES);
    if (CryptHash((int)(8U * CRYPTO_BYTES), in,
                  (unsigned long long)(8U * in_len), ss) != 0) {
        return FAIL;
    }
    return SUCCESS;
}

int qctm_precompute_public_key(IN unsigned char *pk)
{
    unsigned char *cached_cols = NULL;

    if (pk == NULL || !scheme_public_t_aligned()) {
        return FAIL;
    }
    if (get_encrypt_column_cache(pk, &cached_cols) != SUCCESS) {
        return FAIL;
    }
    return SUCCESS;
}

int qctm_precompute_secret_key(IN unsigned char *sk)
{
    goppa_t gamma = NULL;
    int gamma_owned = 0;
    unsigned char zero_syndrome[SYNDROME_BYTES];
    unsigned char sun_syndrome[FULL_SYNDROME_BYTES];
    int ok;

    if (sk == NULL || !scheme_public_t_aligned()) {
        return FAIL;
    }

    gf_init(EXT_DEGREE);
    if (get_dec_gamma(sk, &gamma, &gamma_owned) != SUCCESS) {
        return FAIL;
    }

    memset(zero_syndrome, 0, sizeof(zero_syndrome));
    memset(sun_syndrome, 0, sizeof(sun_syndrome));
    ok = goppa_public_syndrome_to_sun_syndrome(gamma->L, gamma->g, LENGTH,
                                               ORDER, GOPPA_DEGREE,
                                               gamma->eta,
                                               sk + SECRETKEY_PK_OFFSET,
                                               zero_syndrome, sun_syndrome);
    if (gamma_owned) {
        free_goppa(gamma);
    }
    return ok ? SUCCESS : FAIL;
}

int qctm_precompute_keypair(IN unsigned char *pk, IN unsigned char *sk)
{
    if (qctm_precompute_public_key(pk) != SUCCESS) {
        return FAIL;
    }
    return qctm_precompute_secret_key(sk);
}

int encrypt_nied(OUT unsigned char *syndrome, IN int *e, unsigned char *pk)
{
    int i;
    unsigned char syn_aux[SYNDROME_BYTES];
    unsigned char *cached_cols = NULL;

    if (!scheme_public_t_aligned()) {
        return FAIL;
    }

    memset(syn_aux, 0, sizeof(syn_aux));
    (void)get_encrypt_column_cache(pk, &cached_cols);

    for (i = 0; i < ERROR_WEIGHT; ++i) {
        if (e[i] < SYSTEMATIC_QC_ROWS) {
            syndrome_toggle_bit(syn_aux, e[i]);
        } else if (cached_cols != NULL) {
            int col = e[i] - SYSTEMATIC_QC_ROWS;

            syndrome_xor_bytes(syn_aux,
                               cached_cols + (size_t)col *
                               (size_t)SYNDROME_BYTES);
        } else {
            int col = e[i] - SYSTEMATIC_QC_ROWS;
            int row;

            for (row = 0; row < CODIMENSION; row++) {
                if (hybrid_pk_t_bit(pk, row, col)) {
                    syndrome_toggle_bit(syn_aux, row);
                }
            }
        }
    }

    memcpy(syndrome, syn_aux, SYNDROME_BYTES);

    return SUCCESS;
}

int decrypt_nied(IN unsigned char *syndrome, OUT int *error,
                 IN unsigned char *sk, IN unsigned char *pk)
{
    int i;
    int trace_dec = trace_dec_enabled();
    goppa_t gamma = NULL;
    int gamma_owned = 0;
    unsigned char sun_syndrome[FULL_SYNDROME_BYTES];
    unsigned char explicit_syndrome[FULL_SYNDROME_BYTES];
    int profile_dec = profile_dec_enabled();
    double t_start = 0.0;
    double t_after_deser = 0.0;
    double t_after_map = 0.0;
    double t_after_init = 0.0;
    double t_after_convert = 0.0;
    double t_after_decode = 0.0;

    if (profile_dec) {
        t_start = monotonic_ms();
    }

    if (!scheme_public_t_aligned()) {
        if (trace_dec) {
            fprintf(stderr, "dec: public T alignment failed\n");
        }
        return FAIL;
    }

    gf_init(EXT_DEGREE);

    if (get_dec_gamma(sk, &gamma, &gamma_owned) != SUCCESS) {
        return FAIL;
    }
    if (profile_dec) {
        t_after_deser = monotonic_ms();
    }

    if (!goppa_public_syndrome_to_sun_syndrome(gamma->L, gamma->g, LENGTH,
                                               ORDER, GOPPA_DEGREE,
                                               gamma->eta, pk, syndrome,
                                               sun_syndrome)) {
        if (trace_dec) {
            fprintf(stderr, "dec: direct public syndrome mapping failed\n");
        }
        if (gamma_owned) {
            free_goppa(gamma);
        }
        return FAIL;
    }
    if (profile_dec) {
        t_after_map = monotonic_ms();
    }

    if (profile_dec) {
        t_after_init = monotonic_ms();
    }

    if (trace_dec) {
        fprintf(stderr, "dec: direct public syndrome mapping\n");
        fflush(stderr);
    }
    if (sun_syndrome_bytes_to_definition(explicit_syndrome, sun_syndrome,
                                         gamma->g) != SUCCESS) {
        if (gamma_owned) {
            free_goppa(gamma);
        }
        return FAIL;
    }
    if (profile_dec) {
        t_after_convert = monotonic_ms();
    }
    if (trace_dec && debug_last_error_valid) {
        int diff = compare_debug_full_syndrome(explicit_syndrome,
                                               gamma->L, gamma->g,
                                               gamma->eta);

        fprintf(stderr, "dec: full_syndrome_diff_bytes=%d\n", diff);
        fflush(stderr);
    }

    gamma->explicit_syndrome_input = 2;
    if (trace_dec) {
        fprintf(stderr, "dec: goppa_decode\n");
        fflush(stderr);
    }
    i = goppa_decode(explicit_syndrome, error, gamma);
    if (profile_dec) {
        t_after_decode = monotonic_ms();
        fprintf(stderr,
                "qctm_dec_profile_ms deserialize=%.3f map=%.3f init=%.3f convert=%.3f decode=%.3f total=%.3f\n",
                t_after_deser - t_start,
                t_after_map - t_after_deser,
                t_after_init - t_after_map,
                t_after_convert - t_after_init,
                t_after_decode - t_after_convert,
                t_after_decode - t_start);
        fflush(stderr);
    }

    if (gamma_owned) {
        free_goppa(gamma);
    }

    if (i != ERROR_WEIGHT) {
        if (trace_dec) {
            fprintf(stderr, "dec: decoded_weight=%d expected=%d\n",
                    i, ERROR_WEIGHT);
        }
        return FAIL;
    }
    if (trace_dec) {
        fprintf(stderr, "dec: success weight=%d\n", i);
    }
    return SUCCESS;
}

int crypto_kem_keypair(OUT unsigned char *pk, OUT unsigned char *sk)
{
    goppa_t gamma = NULL;
    unsigned char *sk_start = sk;
    unsigned char keygen_seed[KEYGEN_SEED_BYTES];
    unsigned char fallback_s[BITS_TO_BYTES(LENGTH)];
    unsigned char zero_syndrome[SYNDROME_BYTES];
    unsigned char sun_syndrome_check[FULL_SYNDROME_BYTES];
    gf_t eta;
    int attempt;

    if (!scheme_public_t_aligned()) {
        return FAIL;
    }

    memset(pk, 0, PUBLICKEY_BYTES);
    memset(fallback_s, 0, sizeof(fallback_s));
    memset(zero_syndrome, 0, sizeof(zero_syndrome));
    gf_init(EXT_DEGREE);
    gf_set_to_zero(eta);

    for (attempt = 0; attempt < 64; attempt++) {
        memset(pk, 0, PUBLICKEY_BYTES);
        memset(fallback_s, 0, sizeof(fallback_s));
        memset(sun_syndrome_check, 0, sizeof(sun_syndrome_check));
        randombytes(keygen_seed, sizeof(keygen_seed));
#ifdef LOCALLY_QUASI_CYCLIC_TWISTED_MCELIECE_BASELINE
        if (!goppa_fixed_eta(ORDER, eta)) {
            return FAIL;
        }
        randombytes(fallback_s, sizeof(fallback_s));
        gamma = goppa_keygen_rand(LENGTH, ORDER, EXT_DEGREE, GOPPA_DEGREE,
                                  eta, &pk, NULL);
#else
        gamma = scheme_keygen_seeded(LENGTH, ORDER, EXT_DEGREE, GOPPA_DEGREE,
                                   eta, keygen_seed, sizeof(keygen_seed), &pk,
                                   NULL, NULL, fallback_s);
#endif
        if (gamma != NULL) {
            if (goppa_public_syndrome_to_sun_syndrome(gamma->L, gamma->g,
                                                      LENGTH, ORDER,
                                                      GOPPA_DEGREE,
                                                      gamma->eta, pk,
                                                      zero_syndrome,
                                                      sun_syndrome_check)) {
                break;
            }
        }
        if (gamma != NULL) {
            free_goppa(gamma);
            gamma = NULL;
        }
    }
    if (gamma == NULL) {
        return FAIL;
    }

    if (serialize_gamma_prime(sk, gamma->g, gamma->L) != SUCCESS) {
        free_goppa(gamma);
        return FAIL;
    }
    sk += SECRETKEY_GAMMA_BYTES;

    memcpy(sk, pk, PUBLICKEY_BYTES);
    sk += PUBLICKEY_BYTES;

    pack_bits_msb_from_lsb(sk, fallback_s, LENGTH);

    if (qctm_precompute_public_key(pk) != SUCCESS) {
        free_goppa(gamma);
        return FAIL;
    }

    if (cache_dec_gamma_from_sk(sk_start, gamma) == SUCCESS) {
        gamma = NULL;
    }
    if (gamma != NULL) {
        free_goppa(gamma);
    }
    return SUCCESS;
}

int crypto_kem_enc(OUT unsigned char *ct, OUT unsigned char *ss,
                   IN unsigned char *pk)
{
    int error[ERROR_WEIGHT];
    unsigned char e[BITS_TO_BYTES(LENGTH)];
    int status;

    status = fixed_weight_random(error);
    if (status == SUCCESS && trace_dec_enabled()) {
        memcpy(debug_last_error, error, sizeof(debug_last_error));
        debug_last_error_valid = 1;
    }
    if (status == SUCCESS) {
        status = encrypt_nied(ct, error, (unsigned char *)pk);
    }
    if (status == SUCCESS) {
        error_to_vector(e, error);
        status = hash_session_key(ss, 1, e, ct);
    }

    return status;
}

int crypto_kem_dec(OUT unsigned char *ss, IN unsigned char *ct,
                   IN unsigned char *sk)
{
    int error[ERROR_WEIGHT];
    unsigned char e[BITS_TO_BYTES(LENGTH)];
    const unsigned char *fallback_s;
    unsigned char b = 1;
    int status;

    fallback_s = sk + SECRETKEY_S_OFFSET;
    if (decrypt_nied((unsigned char *)ct, error, sk,
                     sk + SECRETKEY_PK_OFFSET) == SUCCESS) {
        error_to_vector(e, error);
    } else {
        unpack_bits_lsb_from_msb(e, fallback_s, LENGTH);
        b = 0;
    }

    status = hash_session_key(ss, b, e, ct);
    return status;
}
