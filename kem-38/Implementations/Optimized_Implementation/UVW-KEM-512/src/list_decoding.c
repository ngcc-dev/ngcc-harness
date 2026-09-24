#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "params.h"
#include "gf_math.h"

#define RS_K 853
#define RS_N 1706
#define RS_M 5
#define RS_L 6
#define POLY_X_SIZE 6300
#define POLY_Y_SIZE (RS_L + 1)
#define MAX_LIST_SIZE 10



typedef struct
{
    gf_elem_t coeffs[POLY_Y_SIZE][POLY_X_SIZE];
    int row_max_x[POLY_Y_SIZE];      /* highest non-zero index in each y-row */
    int cur_max_x;                  /* max over all rows */
    int lod;                        /* (1,k-1)-weighted degree */
    int lod_j;                      /* y-exponent that defines lod */
    int valid;
} inter_poly_t;

typedef struct
{
    gf_elem_t fx[RS_K];
    int distance;
} candidate_msg_t;

static gf_elem_t **COMB_TABLE = NULL;
static gf_elem_t Y_COMB_TABLE[POLY_Y_SIZE][POLY_Y_SIZE];
static int comb_initialized = 0;

/* --------- helper functions --------- */

static gf_elem_t basic_comb_mod_q(int n, int k)
{
    if (k < 0 || k > n) return 0;
    if (k == 0 || k == n) return 1;
    if (k > n / 2) k = n - k;

    gf_elem_t num = 1, den = 1;
    for (int i = 0; i < k; i++) {
        num = gf_mul(num, (gf_elem_t)((n - i) % UVW_Q));
        den = gf_mul(den, (gf_elem_t)((i + 1) % UVW_Q));
    }
    return gf_mul(num, gf_inv(den));
}

static gf_elem_t comb_mod_q(int n, int k)
{
    if (k < 0 || k > n) return 0;
    if (k == 0 || k == n) return 1;
    gf_elem_t res = 1;
    while (n > 0 || k > 0) {
        int ni = n % UVW_Q;
        int ki = k % UVW_Q;
        if (ki > ni) return 0;
        res = gf_mul(res, basic_comb_mod_q(ni, ki));
        n /= UVW_Q;
        k /= UVW_Q;
    }
    return res;
}

static void init_comb_table(void)
{
    if (comb_initialized) return;
    COMB_TABLE = (gf_elem_t **)malloc(POLY_X_SIZE * sizeof(gf_elem_t *));
    for (int i = 0; i < POLY_X_SIZE; i++) {
        COMB_TABLE[i] = (gf_elem_t *)malloc((RS_M + 1) * sizeof(gf_elem_t));
        for (int j = 0; j <= RS_M; j++)
            COMB_TABLE[i][j] = comb_mod_q(i, j);
    }
    for (int n = 0; n < POLY_Y_SIZE; n++)
        for (int k = 0; k < POLY_Y_SIZE; k++)
            Y_COMB_TABLE[n][k] = comb_mod_q(n, k);
    comb_initialized = 1;
}

static inter_poly_t *alloc_poly(void)
{
    inter_poly_t *p = (inter_poly_t *)malloc(sizeof(inter_poly_t));
    memset(p->coeffs, 0, sizeof(p->coeffs));
    for (int j = 0; j < POLY_Y_SIZE; j++)
        p->row_max_x[j] = -1;
    p->cur_max_x = 0;
    p->lod = -1;
    p->lod_j = -1;
    p->valid = 1;
    return p;
}

static void free_poly(inter_poly_t *p)
{
    if (p) free(p);
}

static void copy_poly(inter_poly_t *dst, const inter_poly_t *src)
{
    for (int j = 0; j < POLY_Y_SIZE; j++) {
        int row_max = src->row_max_x[j];
        dst->row_max_x[j] = row_max;
        if (row_max >= 0)
            memcpy(dst->coeffs[j], src->coeffs[j], (row_max + 1) * sizeof(gf_elem_t));
    }
    dst->cur_max_x = src->cur_max_x;
    dst->lod = src->lod;
    dst->lod_j = src->lod_j;
    dst->valid = src->valid;
}

static int recompute_row_max_x(const inter_poly_t *p, int row, int start_x)
{
    int x = start_x;
    if (x >= POLY_X_SIZE) x = POLY_X_SIZE - 1;
    while (x >= 0 && p->coeffs[row][x] == 0)
        x--;
    return x;
}

static void update_lod_and_max_x(inter_poly_t *p)
{
    int max_w = -1, max_j = -1, current_max_x = -1;
    for (int j = 0; j < POLY_Y_SIZE; j++) {
        int row_max = p->row_max_x[j];
        if (row_max < 0) continue;
        if (row_max > current_max_x) current_max_x = row_max;
        int w = row_max + j * (RS_K - 1);
        if (w > max_w || (w == max_w && j > max_j)) {
            max_w = w; max_j = j;
        }
    }
    p->lod = max_w;
    p->lod_j = max_j;
    p->cur_max_x = (current_max_x >= 0) ? current_max_x : 0;
}

static gf_elem_t hasse_derivative_fast(
    const inter_poly_t *poly,
    int alpha, int beta,
    const gf_elem_t *x_alpha_terms,
    const gf_elem_t *y_factors)
{
    gf_elem_t accum = 0;
    for (int j = beta; j < POLY_Y_SIZE; j++) {
        int row_max = poly->row_max_x[j];
        if (row_max < alpha) continue;
        gf_elem_t y_factor = y_factors[j];
        if (y_factor == 0) continue;
        const gf_elem_t *row = poly->coeffs[j];
        for (int i = alpha; i <= row_max; i++) {
            gf_elem_t coeff = row[i];
            if (coeff == 0) continue;
            gf_elem_t term = gf_mul(coeff, x_alpha_terms[i]);
            term = gf_mul(term, y_factor);
            accum = gf_add(accum, term);
        }
    }
    return accum;
}

static int find_roots_at_zero(const inter_poly_t *Q, gf_elem_t *roots)
{
    int count = 0;
    for (int y = 0; y < UVW_Q; y++) {
        gf_elem_t eval = 0;
        gf_elem_t y_pow = 1;
        for (int j = 0; j < POLY_Y_SIZE; j++) {
            if (Q->coeffs[j][0] != 0)
                eval = gf_add(eval, gf_mul(Q->coeffs[j][0], y_pow));
            y_pow = gf_mul(y_pow, (gf_elem_t)y);
        }
        if (eval == 0)
            roots[count++] = (gf_elem_t)y;
    }
    return count;
}

static void shift_and_update_poly(const inter_poly_t *Q_u, inter_poly_t *Q_v, gf_elem_t alpha)
{
    memset(Q_v->coeffs, 0, sizeof(Q_v->coeffs));
    for (int j = 0; j < POLY_Y_SIZE; j++)
        Q_v->row_max_x[j] = -1;

    gf_elem_t alpha_pow[POLY_Y_SIZE];
    alpha_pow[0] = 1;
    for (int i = 1; i < POLY_Y_SIZE; i++)
        alpha_pow[i] = gf_mul(alpha_pow[i - 1], alpha);

    for (int j = 0; j < POLY_Y_SIZE; j++) {
        int upper = Q_u->cur_max_x + j;
        if (upper >= POLY_X_SIZE) upper = POLY_X_SIZE - 1;
        for (int i = j; i <= upper; i++) {
            gf_elem_t sum = 0;
            for (int beta = j; beta < POLY_Y_SIZE; beta++) {
                int src_x = i - j;
                if (src_x > Q_u->row_max_x[beta]) continue;
                gf_elem_t c = Y_COMB_TABLE[beta][j];
                if (c == 0) continue;
                gf_elem_t term = gf_mul(c, Q_u->coeffs[beta][src_x]);
                term = gf_mul(term, alpha_pow[beta - j]);
                sum = gf_add(sum, term);
            }
            Q_v->coeffs[j][i] = sum;
            if (sum != 0) Q_v->row_max_x[j] = i;
        }
    }

    int h = -1;
    for (int i = 0; i < POLY_X_SIZE; i++) {
        int found = 0;
        for (int j = 0; j < POLY_Y_SIZE; j++) {
            if (Q_v->coeffs[j][i] != 0) { h = i; found = 1; break; }
        }
        if (found) break;
    }

    if (h > 0) {
        for (int j = 0; j < POLY_Y_SIZE; j++) {
            for (int i = 0; i < POLY_X_SIZE - h; i++)
                Q_v->coeffs[j][i] = Q_v->coeffs[j][i + h];
            for (int i = POLY_X_SIZE - h; i < POLY_X_SIZE; i++)
                Q_v->coeffs[j][i] = 0;
            if (Q_v->row_max_x[j] >= 0) Q_v->row_max_x[j] -= h;
        }
    }
    update_lod_and_max_x(Q_v);
}

static void rr_dfs(
    inter_poly_t *Q,
    int depth,
    gf_elem_t *current_fx,
    candidate_msg_t *candidates,
    int *cand_count)
{
    if (*cand_count >= MAX_LIST_SIZE) {
        return;
    }

    if (depth == RS_K) {
        if (*cand_count < MAX_LIST_SIZE) {
            memcpy(candidates[*cand_count].fx, current_fx, RS_K * sizeof(gf_elem_t));
            (*cand_count)++;
        }
        return;
    }

    /* dynamic allocation to avoid stack overflow */
    gf_elem_t *roots = (gf_elem_t *)malloc(UVW_Q * sizeof(gf_elem_t));
    if (roots == NULL) return;

    int root_num = find_roots_at_zero(Q, roots);
    for (int i = 0; i < root_num; i++) {
        if (*cand_count >= MAX_LIST_SIZE) {
            break;
        }
        current_fx[depth] = roots[i];
        inter_poly_t *next_Q = alloc_poly();
        shift_and_update_poly(Q, next_Q, roots[i]);
        rr_dfs(next_Q, depth + 1, current_fx, candidates, cand_count);
        free_poly(next_Q);
    }
    free(roots);
}

static void free_poly_array(inter_poly_t **polys, int count)
{
    for (int i = 0; i < count; i++) free_poly(polys[i]);
}

int uvw_rs_list_decode(const gf_elem_t *receive_codeword, const gf_elem_t *support_set, gf_elem_t *de_codeword)
{
    init_comb_table();

    /* allocate initial G polynomials */
    inter_poly_t *G[POLY_Y_SIZE];
    for (int j = 0; j < POLY_Y_SIZE; j++) {
        G[j] = alloc_poly();
        G[j]->coeffs[j][0] = 1;
        G[j]->row_max_x[j] = 0;
        update_lod_and_max_x(G[j]);
    }
    inter_poly_t *G_min_copy = alloc_poly();

    /* allocate temporary arrays on heap to avoid stack overflow */
    gf_elem_t *x_pow = (gf_elem_t *)malloc(POLY_X_SIZE * sizeof(gf_elem_t));
    gf_elem_t *r_pow = (gf_elem_t *)malloc(POLY_Y_SIZE * sizeof(gf_elem_t));
    candidate_msg_t *candidates = (candidate_msg_t *)malloc(MAX_LIST_SIZE * sizeof(candidate_msg_t));
    gf_elem_t *current_fx = (gf_elem_t *)malloc(RS_K * sizeof(gf_elem_t));

    if (!x_pow || !r_pow || !candidates || !current_fx) {
        free(x_pow); free(r_pow); free(candidates); free(current_fx);
        free_poly(G_min_copy);
        free_poly_array(G, POLY_Y_SIZE);
        return 0;
    }

    /* interpolation step */
    for (int count = 0; count < RS_N; count++) {
        gf_elem_t x_i = support_set[count];
        gf_elem_t r_i = receive_codeword[count];

        x_pow[0] = 1;
        for (int i = 1; i < POLY_X_SIZE; i++)
            x_pow[i] = gf_mul(x_pow[i - 1], x_i);
        r_pow[0] = 1;
        for (int i = 1; i < POLY_Y_SIZE; i++)
            r_pow[i] = gf_mul(r_pow[i - 1], r_i);

        gf_elem_t x_alpha_terms[RS_M][POLY_X_SIZE];
        for (int alpha = 0; alpha < RS_M; alpha++) {
            for (int i = alpha; i < POLY_X_SIZE; i++) {
                x_alpha_terms[alpha][i] = gf_mul(COMB_TABLE[i][alpha], x_pow[i - alpha]);
            }
        }

        gf_elem_t y_factors[RS_M][POLY_Y_SIZE];
        for (int beta = 0; beta < RS_M; beta++) {
            for (int j = 0; j < POLY_Y_SIZE; j++) {
                if (j < beta) {
                    y_factors[beta][j] = 0;
                } else {
                    gf_elem_t y_comb = Y_COMB_TABLE[j][beta];
                    y_factors[beta][j] = (y_comb == 0) ? 0 : gf_mul(y_comb, r_pow[j - beta]);
                }
            }
        }

        for (int alpha = 0; alpha < RS_M; alpha++) {
            for (int beta = 0; beta < (RS_M - alpha); beta++) {
                gf_elem_t deltas[POLY_Y_SIZE] = {0};
                int min_lod_idx = -1, min_lod = 0x7FFFFFFF, min_lod_j = -1;

                for (int h = 0; h < POLY_Y_SIZE; h++) {
                    if (!G[h]->valid) continue;
                    deltas[h] = hasse_derivative_fast(G[h], alpha, beta,
                                                      x_alpha_terms[alpha],
                                                      y_factors[beta]);
                    if (deltas[h] != 0) {
                        if (G[h]->lod < min_lod || (G[h]->lod == min_lod && G[h]->lod_j < min_lod_j)) {
                            min_lod = G[h]->lod;
                            min_lod_j = G[h]->lod_j;
                            min_lod_idx = h;
                        }
                    }
                }
                if (min_lod_idx == -1) continue;

                copy_poly(G_min_copy, G[min_lod_idx]);
                gf_elem_t delta_min = deltas[min_lod_idx];

                for (int h = 0; h < POLY_Y_SIZE; h++) {
                    if (!G[h]->valid || deltas[h] == 0) continue;

                    if (h != min_lod_idx) {
                        int new_cur_max_x = -1;
                        gf_elem_t delta_h = deltas[h];
                        for (int j = 0; j < POLY_Y_SIZE; j++) {
                            int row_max_h   = G[h]->row_max_x[j];
                            int row_max_min = G_min_copy->row_max_x[j];
                            int max_x = (row_max_h > row_max_min) ? row_max_h : row_max_min;
                            if (max_x < 0) { G[h]->row_max_x[j] = -1; continue; }
                            gf_elem_t *dst_row = G[h]->coeffs[j];
                            const gf_elem_t *min_row = G_min_copy->coeffs[j];

                            int overlap = (row_max_h < row_max_min) ? row_max_h : row_max_min;
                            for (int i = 0; i <= overlap; i++) {
                                gf_elem_t term1 = gf_mul(delta_min, dst_row[i]);
                                gf_elem_t term2 = gf_mul(delta_h, min_row[i]);
                                dst_row[i] = gf_sub(term1, term2);
                            }
                            for (int i = overlap + 1; i <= row_max_h; i++) {
                                dst_row[i] = gf_mul(delta_min, dst_row[i]);
                            }
                            for (int i = overlap + 1; i <= row_max_min; i++) {
                                dst_row[i] = gf_sub(0, gf_mul(delta_h, min_row[i]));
                            }
                            if (row_max_h != row_max_min) {
                                G[h]->row_max_x[j] = max_x;
                            } else {
                                G[h]->row_max_x[j] = recompute_row_max_x(G[h], j, max_x);
                            }
                            if (G[h]->row_max_x[j] > new_cur_max_x) new_cur_max_x = G[h]->row_max_x[j];
                        }
                        G[h]->cur_max_x = (new_cur_max_x >= 0) ? new_cur_max_x : 0;
                    }
                    else {
                        for (int j = 0; j < POLY_Y_SIZE; j++) {
                            int row_max = G_min_copy->row_max_x[j];
                            if (row_max < 0) { G[h]->row_max_x[j] = -1; continue; }
                            int clear_upto = row_max + 1;
                            if (clear_upto >= POLY_X_SIZE) clear_upto = POLY_X_SIZE - 1;
                            for (int i = 0; i <= clear_upto; i++)
                                G[h]->coeffs[j][i] = 0;

                            for (int i = 0; i <= row_max; i++) {
                                gf_elem_t coeff = G_min_copy->coeffs[j][i];
                                if (coeff == 0) continue;
                                gf_elem_t term_x = gf_mul(delta_min, coeff);
                                gf_elem_t term_c = gf_mul(term_x, x_i);
                                if (i + 1 < POLY_X_SIZE)
                                    G[h]->coeffs[j][i + 1] = gf_add(G[h]->coeffs[j][i + 1], term_x);
                                G[h]->coeffs[j][i] = gf_sub(G[h]->coeffs[j][i], term_c);
                            }
                            G[h]->row_max_x[j] = clear_upto;
                            while (G[h]->row_max_x[j] >= 0 &&
                                   G[h]->coeffs[j][G[h]->row_max_x[j]] == 0) {
                                G[h]->row_max_x[j]--;
                            }
                        }
                        G[h]->cur_max_x = 0;
                        for (int j = 0; j < POLY_Y_SIZE; j++) {
                            if (G[h]->row_max_x[j] > G[h]->cur_max_x) {
                                G[h]->cur_max_x = G[h]->row_max_x[j];
                            }
                        }
                    }
                    update_lod_and_max_x(G[h]);
                }
            }
        }
    }

    /* pick the polynomial with the smallest lod */
    int best_q_idx = 0, min_lod = 0x7FFFFFFF;
    for (int j = 0; j < POLY_Y_SIZE; j++) {
        if (G[j]->valid && G[j]->lod < min_lod) {
            min_lod = G[j]->lod;
            best_q_idx = j;
        }
    }

    int cand_count = 0;
    rr_dfs(G[best_q_idx], 0, current_fx, candidates, &cand_count);

    free_poly(G_min_copy);
    free(x_pow); free(r_pow);

    if (cand_count > 0) {
        int best_cand_idx = 0, min_err_dist = 0x7FFFFFFF;
        for (int c = 0; c < cand_count; c++) {
            int err_dist = 0;
            for (int i = 0; i < RS_N; i++) {
                gf_elem_t eval = 0, x_power = 1;
                for (int d = 0; d < RS_K; d++) {
                    eval = gf_add(eval, gf_mul(candidates[c].fx[d], x_power));
                    x_power = gf_mul(x_power, support_set[i]);
                }
                if (eval != receive_codeword[i]) err_dist++;
            }
            if (err_dist < min_err_dist) {
                min_err_dist = err_dist;
                best_cand_idx = c;
            }
        }

        if (min_err_dist <= UVW_W) {
            memcpy(de_codeword, candidates[best_cand_idx].fx, RS_K * sizeof(gf_elem_t));
            free_poly_array(G, POLY_Y_SIZE);
            free(candidates); free(current_fx);
            return cand_count;
        }
    }

    free_poly_array(G, POLY_Y_SIZE);
    free(candidates); free(current_fx);
    return cand_count;
}
