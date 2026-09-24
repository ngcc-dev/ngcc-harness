// SPDX-License-Identifier: MIT
// Origami reference implementation.

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "origami.h"
#include "origami_gf.h"
#include "symmetric.h"

#define MAX_SIGN_ATTEMPTS 8192
#define ORIGAMI_CROSS_RANK 2
#define GF_STREAM_BUF_BYTES 4096
#define GF_STREAM_MAX_EXTRA (BYTES_DIGEST + BYTES_SALT + 8)

typedef struct {
    uint8_t seed[SEED_LENGTH_PRIVATE];
    size_t seed_len;
    char label[32];
    uint32_t a;
    uint32_t b;
    uint32_t c;
    uint32_t block;
    uint8_t extra[GF_STREAM_MAX_EXTRA];
    size_t extra_len;
    uint8_t buf[GF_STREAM_BUF_BYTES];
    size_t pos;
    size_t len;
    uint8_t byte;
    int have_high;
} gf_stream_t;

static int first_time = 1;
static zone_info_t g_zones[ORIGAMI_TOTAL_ZONES];

static int64_t eligible_affine_positions_total(void);

#ifdef ORIGAMI_ENABLE_STATS
static origami_stats_t g_stats;

static void origami_stats_zone_success(int zone, uint32_t attempts) {
    g_stats.zone_attempts[zone] += attempts;
    if (attempts > 0u) {
        g_stats.zone_retries[zone] += (uint64_t)(attempts - 1u);
    }
    if (attempts > g_stats.zone_max_attempts[zone]) {
        g_stats.zone_max_attempts[zone] = attempts;
    }
}

static void origami_stats_rank_failure(int zone) {
    g_stats.zone_rank_failures[zone]++;
}

static void origami_stats_zone_failure(int zone, uint32_t attempts) {
    g_stats.zone_failures[zone]++;
    if (attempts > g_stats.zone_max_attempts[zone]) {
        g_stats.zone_max_attempts[zone] = attempts;
    }
}

void ORIGAMI_NAMESPACE(stats_reset)(void) {
    memset(&g_stats, 0, sizeof(g_stats));
}

void ORIGAMI_NAMESPACE(stats_snapshot)(origami_stats_t *out) {
    if (out != NULL) {
        *out = g_stats;
    }
}
#endif

static void put_u32_le(uint8_t out[4], uint32_t v) {
    out[0] = (uint8_t)v;
    out[1] = (uint8_t)(v >> 8);
    out[2] = (uint8_t)(v >> 16);
    out[3] = (uint8_t)(v >> 24);
}

static uint32_t load_u32_le(const uint8_t in[4]) {
    return ((uint32_t)in[0]) |
           ((uint32_t)in[1] << 8) |
           ((uint32_t)in[2] << 16) |
           ((uint32_t)in[3] << 24);
}

static void shake_absorb_u32(shake_t *st, uint32_t v) {
    uint8_t tmp[4];
    put_u32_le(tmp, v);
    shake_absorb(st, tmp, sizeof(tmp));
}

static void origami_init(void) {
    if (first_time) {
        init_gf_tables();
        origami_init_zones(g_zones);
        first_time = 0;
    }
}

static int validate_params(void) {
    origami_init();
    for (int z = 0; z < ORIGAMI_TOTAL_ZONES; z++) {
        const zone_info_t *zn = &g_zones[z];
        if (zn->a + zn->b != zn->flat_o) return -1;
        if (zn->b < zn->flat_m + zn->delta) return -2;
        if (zn->l != 2) return -3;
        if (zn->k <= 0 || zn->k > ORIGAMI_MAX_K) return -4;
    }
    if (eligible_affine_positions_total() < (int64_t)NUMGF_RPK) return -5;
    return 0;
}

static void derive_bytes(uint8_t *out, size_t outlen,
                         const uint8_t *seed, size_t seed_len,
                         const char *label,
                         uint32_t a, uint32_t b, uint32_t c,
                         const uint8_t *extra, size_t extra_len) {
    shake_t st;
    shake256_init(&st);
    shake_absorb(&st, seed, seed_len);
    shake_absorb(&st, (const uint8_t *)label, strlen(label));
    shake_absorb_u32(&st, a);
    shake_absorb_u32(&st, b);
    shake_absorb_u32(&st, c);
    if (extra != NULL && extra_len != 0) {
        shake_absorb(&st, extra, extra_len);
    }
    shake_finalize(&st);
    shake_squeeze(out, outlen, &st);
}

static void derive_public_seed_from_secret(uint8_t pk_seed[SEED_LENGTH_PUBLIC],
                                           const uint8_t sk_seed[SEED_LENGTH_PRIVATE]) {
    derive_bytes(pk_seed, SEED_LENGTH_PUBLIC, sk_seed, SEED_LENGTH_PRIVATE,
                 "pk-seed", (uint32_t)ORIGAMI_PARAM_ID,
                 (uint32_t)ORIGAMI_SECURITY_BITS,
                 (uint32_t)SEED_LENGTH_PUBLIC, NULL, 0);
}
static void derive_coeff_seed_from_secret(uint8_t coeff_seed[SEED_LENGTH_PRIVATE],
                                          const uint8_t sk_seed[SEED_LENGTH_PRIVATE]) {
    derive_bytes(coeff_seed, SEED_LENGTH_PRIVATE, sk_seed, SEED_LENGTH_PRIVATE,
                 "coeff-seed", (uint32_t)ORIGAMI_PARAM_ID,
                 (uint32_t)ORIGAMI_SECURITY_BITS,
                 (uint32_t)SEED_LENGTH_PRIVATE, NULL, 0);
}

typedef struct {
    int64_t total;
    int64_t stride;
    int64_t offset;
} residual_schedule_t;

static int64_t gcd_i64(int64_t a, int64_t b) {
    while (b != 0) {
        int64_t t = a % b;
        a = b;
        b = t;
    }
    return a < 0 ? -a : a;
}

static int64_t eligible_affine_positions_total(void) {
    int64_t total = 0;
    for (int z = 0; z < ORIGAMI_TOTAL_ZONES; z++) {
        const zone_info_t *zn = &g_zones[z];
        const int64_t non_oil = (int64_t)zn->n_offset + (int64_t)zn->flat_v;
        total += (int64_t)zn->flat_m * non_oil * (int64_t)zn->flat_o;
    }
    return total;
}

static int64_t eligible_affine_positions_before_zone(int zone) {
    int64_t total = 0;
    for (int z = 0; z < zone; z++) {
        const zone_info_t *zn = &g_zones[z];
        const int64_t non_oil = (int64_t)zn->n_offset + (int64_t)zn->flat_v;
        total += (int64_t)zn->flat_m * non_oil * (int64_t)zn->flat_o;
    }
    return total;
}

static int64_t residual_stride_for_total(int64_t total) {
    int64_t stride = ((int64_t)ORIGAMI_PARAM_ID << 1) | 1;
    if (total <= 1) return 1;
    stride %= total;
    if ((stride & 1) == 0) stride++;
    if (stride <= 0) stride = 1;
    while (gcd_i64(stride, total) != 1) {
        stride += 2;
        if (stride >= total) stride = 1;
    }
    return stride;
}

static int64_t residual_offset_from_seed(const uint8_t pk_seed[SEED_LENGTH_PUBLIC],
                                         int64_t total) {
    uint64_t acc = 0x9e3779b97f4a7c15ULL ^ (uint64_t)ORIGAMI_PARAM_ID;
    if (total <= 0) return 0;
    for (size_t i = 0; i < SEED_LENGTH_PUBLIC; i++) {
        acc ^= (uint64_t)pk_seed[i] + 0x9e3779b97f4a7c15ULL + (acc << 6) + (acc >> 2);
    }
    return (int64_t)(acc % (uint64_t)total);
}

static void residual_schedule_init(residual_schedule_t *rs,
                                   const uint8_t pk_seed[SEED_LENGTH_PUBLIC]) {
    rs->total = eligible_affine_positions_total();
    rs->stride = residual_stride_for_total(rs->total);
    rs->offset = residual_offset_from_seed(pk_seed, rs->total);
}

static long residual_rank_from_index(const residual_schedule_t *rs, int64_t idx) {
    const int64_t rank = (idx * rs->stride + rs->offset) % rs->total;
    return rank < (int64_t)NUMGF_RPK ? (long)rank : -1L;
}

static int derive_residual_coefficients(uint8_t *R_coeffs,
                                        const uint8_t coeff_seed[SEED_LENGTH_PRIVATE]) {
    uint8_t *bytes = (uint8_t *)malloc((size_t)ORIGAMI_RPK_BYTES);
    if (bytes == NULL) return -1;
    derive_bytes(bytes, (size_t)ORIGAMI_RPK_BYTES, coeff_seed, SEED_LENGTH_PRIVATE,
                 "Rpk-coeff", (uint32_t)ORIGAMI_PARAM_ID,
                 (uint32_t)ORIGAMI_SECURITY_BITS, (uint32_t)NUMGF_RPK,
                 NULL, 0);
    if (expand_gf(R_coeffs, bytes, (size_t)NUMGF_RPK) != 0) {
        free(bytes);
        return -2;
    }
    free(bytes);
    return 0;
}

static void gf_stream_init(gf_stream_t *gs,
                           const uint8_t *seed, size_t seed_len,
                           const char *label,
                           uint32_t a, uint32_t b, uint32_t c,
                           const uint8_t *extra, size_t extra_len) {
    memset(gs, 0, sizeof(*gs));
    if (seed_len > sizeof(gs->seed)) seed_len = sizeof(gs->seed);
    memcpy(gs->seed, seed, seed_len);
    gs->seed_len = seed_len;
    strncpy(gs->label, label, sizeof(gs->label) - 1);
    gs->a = a;
    gs->b = b;
    gs->c = c;
    if (extra != NULL && extra_len != 0) {
        if (extra_len > sizeof(gs->extra)) extra_len = sizeof(gs->extra);
        memcpy(gs->extra, extra, extra_len);
        gs->extra_len = extra_len;
    }
}

static void gf_stream_refill(gf_stream_t *gs) {
    uint8_t stream_extra[GF_STREAM_MAX_EXTRA + 4];
    put_u32_le(stream_extra, gs->block++);
    if (gs->extra_len != 0) {
        memcpy(stream_extra + 4, gs->extra, gs->extra_len);
    }
    derive_bytes(gs->buf, sizeof(gs->buf), gs->seed, gs->seed_len, gs->label,
                 gs->a, gs->b, gs->c, stream_extra, gs->extra_len + 4);
    gs->pos = 0;
    gs->len = sizeof(gs->buf);
    gs->byte = 0;
    gs->have_high = 0;
}

static gf_t gf_stream_next(gf_stream_t *gs) {
    gf_t out;
    if (gs->have_high) {
        out = (gf_t)(gs->byte >> 4);
        gs->have_high = 0;
        return out;
    }
    if (gs->pos >= gs->len) {
        gf_stream_refill(gs);
    }
    gs->byte = gs->buf[gs->pos++];
    out = (gf_t)(gs->byte & 0x0f);
    gs->have_high = 1;
    return out;
}

static void build_zone_system(gf_t *D, gf_t *A, const gf_t *secret_y,
                              const ph_expanded_SK *skx, int zone,
                              const int *known_vars, int known_len,
                              const int *w_vars, const int *u_vars);

static void gen_permutation(uint16_t *perm, uint16_t *perm_inv, int n,
                            const uint8_t *seed, size_t seed_len,
                            const char *label, uint32_t domain) {
    uint8_t rnd[2];
    shake_t st;

    for (int i = 0; i < n; i++) {
        perm[i] = (uint16_t)i;
    }

    shake256_init(&st);
    shake_absorb(&st, seed, seed_len);
    shake_absorb(&st, (const uint8_t *)label, strlen(label));
    shake_absorb_u32(&st, domain);
    shake_finalize(&st);

    for (int i = n - 1; i > 0; i--) {
        shake_squeeze(rnd, sizeof(rnd), &st);
        int j = (int)((((uint32_t)rnd[0]) | ((uint32_t)rnd[1] << 8)) % (uint32_t)(i + 1));
        uint16_t t = perm[i];
        perm[i] = perm[j];
        perm[j] = t;
    }

    if (perm_inv != NULL) {
        for (int i = 0; i < n; i++) {
            perm_inv[perm[i]] = (uint16_t)i;
        }
    }
}

static int gen_invertible_matrix_copy(gf_t *mat, gf_t *mat_inv, int dim,
                                      const uint8_t *seed, size_t seed_len,
                                      const char *label,
                                      uint32_t zone, uint32_t copy) {
    uint8_t bytes[ORIGAMI_MAX_L2];
    for (uint32_t attempt = 0; attempt < 1024; attempt++) {
        derive_bytes(bytes, (size_t)(dim * dim), seed, seed_len, label,
                     zone, copy, attempt, NULL, 0);
        convert_bytes_to_GF(mat, bytes, (size_t)(dim * dim));
        if (gf_mat_det_dim(mat, dim) != 0) {
            if (mat_inv != NULL && gf_mat_inv_dim(mat_inv, mat, dim) != 0) {
                return -1;
            }
            return 0;
        }
    }
    return -1;
}

static int gen_hidden_algebra(ph_expanded_SK *skx, int zone, int copy) {
    const zone_info_t *zn = &g_zones[zone];
    const int l = zn->l;
    const int l2 = zn->l2;
    gf_t S[ORIGAMI_MAX_L2];
    gf_t C[ORIGAMI_MAX_L2];
    gf_t C_inv[ORIGAMI_MAX_L2];
    gf_t tmp[ORIGAMI_MAX_L2];
    gf_t T[ORIGAMI_MAX_L2];
    uint8_t bytes[ORIGAMI_MAX_L2];

    for (uint32_t attempt = 0; attempt < 2048; attempt++) {
        derive_bytes(bytes, (size_t)l2, skx->sk_seed, SEED_LENGTH_PRIVATE,
                     "embed-S", (uint32_t)zone, (uint32_t)copy,
                     attempt, NULL, 0);
        convert_bytes_to_GF(S, bytes, (size_t)l2);

        gf_t poly[ORIGAMI_MAX_L + 1];
        gf_charpoly_interp(poly, S, l);
        if (gf_poly_is_irreducible(poly, l)) {
            break;
        }
        if (attempt == 2047) return -1;
    }

    if (gen_invertible_matrix_copy(C, C_inv, l, skx->sk_seed,
                                   SEED_LENGTH_PRIVATE, "embed-C",
                                   (uint32_t)zone, (uint32_t)copy) != 0) {
        return -1;
    }

    gf_mat_mul_dim(tmp, C_inv, S, l, l, l);
    gf_mat_mul_dim(T, tmp, C, l, l, l);

    memset(skx->T_powers[zone][copy], 0, sizeof(skx->T_powers[zone][copy]));
    for (int i = 0; i < l; i++) {
        skx->T_powers[zone][copy][0][i * l + i] = 1;
    }
    memcpy(skx->T_powers[zone][copy][1], T, (size_t)l2);
    memcpy(skx->C[zone][copy], C, (size_t)l2);
    memcpy(skx->C_inv[zone][copy], C_inv, (size_t)l2);
    return 0;
}

static void sample_k_elements(gf_t *out, int elements, const ph_expanded_SK *skx,
                              int zone, gf_stream_t *gs) {
    const zone_info_t *zn = &g_zones[zone];
    const int l = zn->l;
    const int l2 = zn->l2;
    const int elements_per_copy = zn->v;
    for (int e = 0; e < elements; e++) {
        gf_t acc[ORIGAMI_MAX_L2] = {0, 0, 0, 0};
        int copy = 0;
        if (elements_per_copy > 0) {
            copy = e / elements_per_copy;
            if (copy >= zn->k) copy = zn->k - 1;
        }
        for (int p = 0; p < l; p++) {
            gf_t coeff = gf_stream_next(gs);
            for (int t = 0; t < l2; t++) {
                gf_set_add(&acc[t], gf_mult(coeff, skx->T_powers[zone][copy][p][t]));
            }
        }
        memcpy(out + e * l2, acc, (size_t)l2);
    }
}

static void build_zone_lists(const ph_expanded_SK *skx, int zone,
                             int *known_vars, int *known_len,
                             int *w_vars, int *u_vars) {
    const zone_info_t *zn = &g_zones[zone];
    int pos = 0;
    const int oil_base = zn->n_offset + zn->flat_v;

    for (int i = 0; i < zn->n_offset; i++) {
        known_vars[pos++] = i;
    }
    for (int i = 0; i < zn->flat_v; i++) {
        known_vars[pos++] = zn->n_offset + i;
    }
    for (int i = 0; i < zn->a; i++) {
        u_vars[i] = oil_base + skx->rho[zone][i];
        known_vars[pos++] = u_vars[i];
    }
    for (int i = 0; i < zn->b; i++) {
        w_vars[i] = oil_base + skx->rho[zone][zn->a + i];
    }
    *known_len = pos;
}

static void public_affine_stream_for_eq(gf_stream_t *gs,
                                        const uint8_t pk_seed[SEED_LENGTH_PUBLIC],
                                        int eq) {
    gf_stream_init(gs, pk_seed, SEED_LENGTH_PUBLIC, "P_affine",
                   (uint32_t)eq, (uint32_t)ORIGAMI_N,
                   (uint32_t)ORIGAMI_PARAM_ID, NULL, 0);
}

static int derive_public_permutation(uint16_t secret_to_public[ORIGAMI_N],
                                     uint16_t public_to_secret[ORIGAMI_N],
                                     const uint8_t pk_seed[SEED_LENGTH_PUBLIC]) {
    gen_permutation(secret_to_public, public_to_secret, ORIGAMI_N,
                    pk_seed, SEED_LENGTH_PUBLIC, "secret-to-public", 0);
    return 0;
}

static int encode_public_residual(uint8_t *rpk, const ph_expanded_SK *skx) {
    if (skx->R_coeffs == NULL) return -1;
    compress_gf(rpk, skx->R_coeffs, (size_t)NUMGF_RPK);
    return 0;
}

static int decode_public_residual(ph_expanded_PK *pkx, const uint8_t *rpk) {
    pkx->R_coeffs = (uint8_t *)malloc((size_t)NUMGF_RPK);
    if (pkx->R_coeffs == NULL) return -1;
    if (expand_gf(pkx->R_coeffs, rpk, (size_t)NUMGF_RPK) != 0) {
        free(pkx->R_coeffs);
        pkx->R_coeffs = NULL;
        return -2;
    }
    if (derive_public_permutation(pkx->secret_to_public, pkx->public_to_secret,
                                  pkx->pk_seed) != 0) {
        free(pkx->R_coeffs);
        pkx->R_coeffs = NULL;
        return -3;
    }
    return 0;
}

static void evaluate_public_map(gf_t *result, const ph_expanded_PK *pkx,
                                const gf_t *public_y) {
    gf_t secret_y[ORIGAMI_N];
    residual_schedule_t rs;

    origami_init();
    memset(result, 0, (size_t)ORIGAMI_M * sizeof(gf_t));
    memset(secret_y, 0, sizeof(secret_y));
    for (int i = 0; i < ORIGAMI_N; i++) {
        secret_y[pkx->public_to_secret[i]] = public_y[i];
    }
    residual_schedule_init(&rs, pkx->pk_seed);

    for (int zone = 0; zone < ORIGAMI_TOTAL_ZONES; zone++) {
        const zone_info_t *zn = &g_zones[zone];
        const int non_oil_count = zn->n_offset + zn->flat_v;
        const int oil_start = zn->n_offset + zn->flat_v;
        const int64_t zone_base = eligible_affine_positions_before_zone(zone);

        for (int eq = 0; eq < zn->flat_m; eq++) {
            gf_stream_t gs;
            const int eq_global = zn->m_offset + eq;
            const int64_t eq_base = zone_base +
                (int64_t)eq * (int64_t)non_oil_count * (int64_t)zn->flat_o;
            gf_t acc = 0;
            public_affine_stream_for_eq(&gs, pkx->pk_seed, eq_global);

            for (int non = 0; non < non_oil_count; non++) {
                const gf_t y_non = secret_y[non];
                for (int oil_rel = 0; oil_rel < zn->flat_o; oil_rel++) {
                    gf_t coeff = gf_stream_next(&gs);
                    const int64_t idx = eq_base +
                        (int64_t)non * (int64_t)zn->flat_o + (int64_t)oil_rel;
                    const long rank = residual_rank_from_index(&rs, idx);
                    if (rank >= 0) {
                        coeff = pkx->R_coeffs[rank];
                    }
                    if (coeff == 0 || y_non == 0) continue;
                    gf_set_add(&acc, gf_mult(coeff,
                                             gf_mult(y_non, secret_y[oil_start + oil_rel])));
                }
            }
            result[zn->m_offset + eq] = acc;
        }
    }
}
static void hash_to_field(gf_t target[ORIGAMI_M],
                          const uint8_t *digest, size_t len_digest,
                          const uint8_t salt[BYTES_SALT]) {
    uint8_t bytes[BYTES_HASH];
    shake_t st;

    shake256_init(&st);
    shake_absorb(&st, (const uint8_t *)"target", 6);
    shake_absorb(&st, digest, len_digest);
    shake_absorb(&st, salt, BYTES_SALT);
    shake_finalize(&st);
    shake_squeeze(bytes, sizeof(bytes), &st);
    (void)expand_gf(target, bytes, ORIGAMI_M);
}

static void sample_zone_known(gf_t *secret_y, const ph_expanded_SK *skx,
                              int zone, int attempt,
                              const int *u_vars,
                              const uint8_t *digest, size_t len_digest,
                              const uint8_t salt[BYTES_SALT]) {
    const zone_info_t *zn = &g_zones[zone];
    uint8_t extra[BYTES_DIGEST + BYTES_SALT];
    size_t extra_len = 0;
    gf_stream_t gs;

    memcpy(extra + extra_len, digest, len_digest);
    extra_len += len_digest;
    memcpy(extra + extra_len, salt, BYTES_SALT);
    extra_len += BYTES_SALT;

    gf_stream_init(&gs, skx->sk_seed, SEED_LENGTH_PRIVATE, "sign-known",
                   (uint32_t)zone, (uint32_t)attempt, 0, extra, extra_len);

    if (zn->flat_v != 0) {
        sample_k_elements(secret_y + zn->n_offset,
                          zn->flat_v / zn->l2, skx, zone, &gs);
    }
    for (int i = 0; i < zn->a; i++) {
        gf_t u = gf_stream_next(&gs);
        if (i == 0 && u == 0) u = 1;
        secret_y[u_vars[i]] = u;
    }
}

static void build_zone_system(gf_t *D, gf_t *A, const gf_t *secret_y,
                              const ph_expanded_SK *skx, int zone,
                              const int *known_vars, int known_len,
                              const int *w_vars, const int *u_vars) {
    const zone_info_t *zn = &g_zones[zone];
    residual_schedule_t rs;
    const int non_oil_count = zn->n_offset + zn->flat_v;
    const int oil_start = zn->n_offset + zn->flat_v;
    const int64_t zone_base = eligible_affine_positions_before_zone(zone);

    (void)known_vars;
    (void)known_len;
    (void)w_vars;
    (void)u_vars;

    memset(D, 0, (size_t)zn->flat_m * sizeof(gf_t));
    memset(A, 0, (size_t)zn->flat_m * (size_t)zn->b * sizeof(gf_t));
    residual_schedule_init(&rs, skx->pk_seed);

    for (int eq = 0; eq < zn->flat_m; eq++) {
        gf_stream_t gs;
        const int eq_global = zn->m_offset + eq;
        const int64_t eq_base = zone_base +
            (int64_t)eq * (int64_t)non_oil_count * (int64_t)zn->flat_o;
        public_affine_stream_for_eq(&gs, skx->pk_seed, eq_global);

        for (int non = 0; non < non_oil_count; non++) {
            const gf_t y_non = secret_y[non];
            for (int oil_rel = 0; oil_rel < zn->flat_o; oil_rel++) {
                gf_t coeff = gf_stream_next(&gs);
                const int64_t idx = eq_base +
                    (int64_t)non * (int64_t)zn->flat_o + (int64_t)oil_rel;
                const long rank = residual_rank_from_index(&rs, idx);
                if (rank >= 0) {
                    coeff = skx->R_coeffs[rank];
                }
                if (coeff == 0 || y_non == 0) continue;

                const int oil_var = oil_start + oil_rel;
                const int part = skx->rho_inv[zone][oil_rel];
                const gf_t scaled = gf_mult(coeff, y_non);
                if (part < zn->a) {
                    gf_set_add(&D[eq], gf_mult(scaled, secret_y[oil_var]));
                } else {
                    const int w_col = part - zn->a;
                    gf_set_add(&A[eq * zn->b + w_col], scaled);
                }
            }
        }
    }
}

static int solve_rect_random(gf_t *x, const gf_t *A, const gf_t *rhs,
                             int rows, int cols,
                             const uint8_t *seed, size_t seed_len,
                             int zone, int attempt,
                             const uint8_t *digest, size_t len_digest,
                             const uint8_t salt[BYTES_SALT]) {
    gf_t aug[ORIGAMI_MAX_FLAT_M * (ORIGAMI_MAX_FLAT_O + 1)];
    int pivot_col[ORIGAMI_MAX_FLAT_M];
    int is_pivot[ORIGAMI_MAX_FLAT_O];
    uint8_t extra[BYTES_DIGEST + BYTES_SALT];
    size_t extra_len = 0;
    gf_stream_t gs;
    int rank = 0;

    memset(is_pivot, 0, sizeof(is_pivot));
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            aug[r * (cols + 1) + c] = A[r * cols + c];
        }
        aug[r * (cols + 1) + cols] = rhs[r];
    }

    for (int col = 0; col < cols && rank < rows; col++) {
        int pivot = -1;
        for (int r = rank; r < rows; r++) {
            if (aug[r * (cols + 1) + col] != 0) {
                pivot = r;
                break;
            }
        }
        if (pivot < 0) continue;

        if (pivot != rank) {
            for (int c = col; c <= cols; c++) {
                gf_t t = aug[rank * (cols + 1) + c];
                aug[rank * (cols + 1) + c] = aug[pivot * (cols + 1) + c];
                aug[pivot * (cols + 1) + c] = t;
            }
        }

        gf_t inv = gf_inv(aug[rank * (cols + 1) + col]);
        for (int c = col; c <= cols; c++) {
            aug[rank * (cols + 1) + c] = gf_mult(aug[rank * (cols + 1) + c], inv);
        }

        for (int r = 0; r < rows; r++) {
            if (r == rank) continue;
            gf_t factor = aug[r * (cols + 1) + col];
            if (factor == 0) continue;
            for (int c = col; c <= cols; c++) {
                gf_t term = gf_mult(factor, aug[rank * (cols + 1) + c]);
                aug[r * (cols + 1) + c] = gf_sub(aug[r * (cols + 1) + c], term);
            }
        }

        pivot_col[rank] = col;
        is_pivot[col] = 1;
        rank++;
    }

    if (rank != rows) return -1;

    memset(x, 0, (size_t)cols * sizeof(gf_t));
    memcpy(extra + extra_len, digest, len_digest);
    extra_len += len_digest;
    memcpy(extra + extra_len, salt, BYTES_SALT);
    extra_len += BYTES_SALT;
    gf_stream_init(&gs, seed, seed_len, "kernel-sample",
                   (uint32_t)zone, (uint32_t)attempt, 0, extra, extra_len);

    for (int c = 0; c < cols; c++) {
        if (!is_pivot[c]) {
            x[c] = gf_stream_next(&gs);
        }
    }

    for (int r = rows - 1; r >= 0; r--) {
        int pc = pivot_col[r];
        gf_t sum = 0;
        for (int c = pc + 1; c < cols; c++) {
            gf_set_add(&sum, gf_mult(aug[r * (cols + 1) + c], x[c]));
        }
        x[pc] = gf_sub(aug[r * (cols + 1) + cols], sum);
    }

    return 0;
}

int ORIGAMI_NAMESPACE(genkeys)(uint8_t *pk, uint8_t *sk, const uint8_t *seed) {
    ph_expanded_SK skx;

    if (validate_params() != 0) return -1;
    if (ORIGAMI_NAMESPACE(sk_expand)(&skx, seed) != 0) return -2;

    put_u32_le(pk, (uint32_t)ORIGAMI_PARAM_ID);
    memcpy(pk + 4, skx.pk_seed, SEED_LENGTH_PUBLIC);
    if (encode_public_residual(pk + 4 + SEED_LENGTH_PUBLIC, &skx) != 0) {
        ORIGAMI_NAMESPACE(sk_free)(&skx);
        return -3;
    }
    memcpy(sk, seed, BYTES_SK);

    ORIGAMI_NAMESPACE(sk_free)(&skx);
    return 0;
}
int ORIGAMI_NAMESPACE(sk_expand)(ph_expanded_SK *skx, const uint8_t *sk) {
    origami_init();
    memset(skx, 0, sizeof(*skx));
    memcpy(skx->sk_seed, sk, SEED_LENGTH_PRIVATE);
    derive_coeff_seed_from_secret(skx->coeff_seed, skx->sk_seed);
    derive_public_seed_from_secret(skx->pk_seed, skx->sk_seed);
    derive_public_permutation(skx->secret_to_public, skx->public_to_secret,
                              skx->pk_seed);
    skx->R_coeffs = (uint8_t *)malloc((size_t)NUMGF_RPK);
    if (skx->R_coeffs == NULL) return -1;
    if (derive_residual_coefficients(skx->R_coeffs, skx->coeff_seed) != 0) {
        free(skx->R_coeffs);
        skx->R_coeffs = NULL;
        return -1;
    }

    for (int z = 0; z < ORIGAMI_TOTAL_ZONES; z++) {
        const zone_info_t *zn = &g_zones[z];
        gen_permutation(skx->rho[z], skx->rho_inv[z], zn->flat_o,
                        skx->sk_seed, SEED_LENGTH_PRIVATE, "rho", (uint32_t)z);
        for (int copy = 0; copy < zn->k; copy++) {
            if (gen_hidden_algebra(skx, z, copy) != 0) {
                ORIGAMI_NAMESPACE(sk_free)(skx);
                return -1;
            }
        }
    }
    return 0;
}

void ORIGAMI_NAMESPACE(sk_free)(ph_expanded_SK *skx) {
    if (skx != NULL) {
        free(skx->R_coeffs);
        skx->R_coeffs = NULL;
    }
}

int ORIGAMI_NAMESPACE(sign)(const ph_expanded_SK *skx, uint8_t *sig,
                            const uint8_t *digest, size_t len_digest,
                            const uint8_t *salt) {
    gf_t target[ORIGAMI_M];
    gf_t secret_y[ORIGAMI_N];
    gf_t public_y[ORIGAMI_N];
    gf_t D[ORIGAMI_MAX_FLAT_M];
    gf_t A[ORIGAMI_MAX_FLAT_M * ORIGAMI_MAX_FLAT_O];
    gf_t rhs[ORIGAMI_MAX_FLAT_M];
    gf_t W[ORIGAMI_MAX_FLAT_O];

    origami_init();
    if (len_digest > BYTES_DIGEST) len_digest = BYTES_DIGEST;
    memset(secret_y, 0, sizeof(secret_y));
    hash_to_field(target, digest, len_digest, salt);

    for (int zone = 0; zone < ORIGAMI_TOTAL_ZONES; zone++) {
        const zone_info_t *zn = &g_zones[zone];
        int known_vars[ORIGAMI_MAX_KNOWN];
        int w_vars[ORIGAMI_MAX_FLAT_O];
        int u_vars[ORIGAMI_MAX_FLAT_O];
        int known_len = 0;
        int solved = 0;
#ifdef ORIGAMI_ENABLE_STATS
        uint32_t zone_attempts = 0;
#endif

        build_zone_lists(skx, zone, known_vars, &known_len, w_vars, u_vars);

        for (int attempt = 0; attempt < MAX_SIGN_ATTEMPTS; attempt++) {
#ifdef ORIGAMI_ENABLE_STATS
            zone_attempts = (uint32_t)attempt + 1u;
#endif
            sample_zone_known(secret_y, skx, zone, attempt, u_vars,
                              digest, len_digest, salt);
            build_zone_system(D, A, secret_y, skx, zone,
                              known_vars, known_len, w_vars, u_vars);

            for (int i = 0; i < zn->flat_m; i++) {
                rhs[i] = gf_sub(target[zn->m_offset + i], D[i]);
            }
            const int solve_rc = solve_rect_random(W, A, rhs, zn->flat_m, zn->b,
                                                   skx->sk_seed, SEED_LENGTH_PRIVATE,
                                                   zone, attempt, digest,
                                                   len_digest, salt);
            if (solve_rc == 0) {
                for (int i = 0; i < zn->b; i++) {
                    secret_y[w_vars[i]] = W[i];
                }
#ifdef ORIGAMI_ENABLE_STATS
                origami_stats_zone_success(zone, zone_attempts);
#endif
                solved = 1;
                break;
            }
#ifdef ORIGAMI_ENABLE_STATS
            origami_stats_rank_failure(zone);
#endif
        }

        if (!solved) {
#ifdef ORIGAMI_ENABLE_STATS
            origami_stats_zone_failure(zone, zone_attempts);
#endif
            return -1;
        }
    }

#ifdef ORIGAMI_ENABLE_STATS
    g_stats.signatures++;
#endif
    for (int i = 0; i < ORIGAMI_N; i++) {
        public_y[skx->secret_to_public[i]] = secret_y[i];
    }
    compress_gf(sig, public_y, ORIGAMI_N);
    memcpy(sig + BYTES_GF(ORIGAMI_N), salt, BYTES_SALT);
    return 0;
}

int ORIGAMI_NAMESPACE(pk_expand)(ph_expanded_PK *pkx, const uint8_t *pk) {
    origami_init();
    memset(pkx, 0, sizeof(*pkx));
    pkx->param_id = load_u32_le(pk);
    if (pkx->param_id != (uint32_t)ORIGAMI_PARAM_ID) return -1;
    memcpy(pkx->pk_seed, pk + 4, SEED_LENGTH_PUBLIC);
    if (decode_public_residual(pkx, pk + 4 + SEED_LENGTH_PUBLIC) != 0) {
        return -2;
    }
    return 0;
}

void ORIGAMI_NAMESPACE(pk_free)(ph_expanded_PK *pkx) {
    if (pkx != NULL) {
        free(pkx->R_coeffs);
        pkx->R_coeffs = NULL;
    }
}
int ORIGAMI_NAMESPACE(verify)(const ph_expanded_PK *pkx, const uint8_t *sig,
                              const uint8_t *digest, size_t len_digest) {
    gf_t sigma[ORIGAMI_N];
    gf_t expected[ORIGAMI_M];
    gf_t result[ORIGAMI_M];
    const uint8_t *salt = sig + BYTES_GF(ORIGAMI_N);

    origami_init();
    if (len_digest > BYTES_DIGEST) len_digest = BYTES_DIGEST;
    if (expand_gf(sigma, sig, ORIGAMI_N) != 0) return -1;
    hash_to_field(expected, digest, len_digest, salt);
    evaluate_public_map(result, pkx, sigma);

    for (int i = 0; i < ORIGAMI_M; i++) {
        if (result[i] != expected[i]) return -1;
    }
    return 0;
}
