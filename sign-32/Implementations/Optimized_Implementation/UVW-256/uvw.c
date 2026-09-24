#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "macros.h"
#include "uvw.h"

#ifdef USE_API_PKC
    extern DRNG_ctx *uvw_drng;

    static inline void uvw_csprng_read(void *out, size_t length) {
        get_random_number(uvw_drng, out, (unsigned long long)length * 8);
    }

    static inline uint8_t uvw_csprng_get_uint8_t() {
        uint8_t v;
        get_random_number(uvw_drng, &v, 8);
        return v;
    }

    static inline size_t uvw_csprng_get_size_t() {
        size_t v;
        get_random_number(uvw_drng, (unsigned char *)&v, sizeof(size_t) * 8);
        return v;
    }

    static inline void uvw_csprng_shuffle_partial_size_t(size_t *arr, size_t len, size_t steps) {
        for (size_t i = 0; i < steps; i++) {
            size_t j;
            unsigned char buf[sizeof(size_t)];
            get_random_number(uvw_drng, buf, sizeof(size_t) * 8);
            memcpy(&j, buf, sizeof(size_t));
            j = j % (len - i);
            size_t tmp = arr[i];
            arr[i] = arr[i + j];
            arr[i + j] = tmp;
        }
    }

    #define csprng_read(buf, len) uvw_csprng_read(buf, len)
    #define csprng_get_uint8_t() uvw_csprng_get_uint8_t()
    #define csprng_get_size_t() uvw_csprng_get_size_t()
    #define csprng_shuffle_partial_size_t(arr, len, steps) uvw_csprng_shuffle_partial_size_t(arr, len, steps)

    void uvw_hash_init(shake256_ctx *ctx) {
        ctx->msg = NULL;
        ctx->msg_len = 0;
        ctx->xof_buf = NULL;
        ctx->xof_len = 0;
        ctx->xof_pos = 0;
    }

    void uvw_hash_update(shake256_ctx *ctx, const unsigned char *data, unsigned long long len) {
        ctx->msg = realloc(ctx->msg, ctx->msg_len + len);
        memcpy(ctx->msg + ctx->msg_len, data, len);
        ctx->msg_len += len;
    }

    void uvw_hash_read(shake256_ctx *ctx, unsigned char *out, unsigned long long len) {
        if (!ctx->xof_buf) {
            unsigned long long xof_bits = (unsigned long long)(ctx->msg_len) * 8;
            ctx->xof_len = (xof_bits + 7) / 8;
            if (ctx->xof_len < 65536) ctx->xof_len = 65536;
            ctx->xof_buf = malloc(ctx->xof_len);
            pseudoXOF(ctx->xof_len * 8, ctx->msg, xof_bits, ctx->xof_buf);
            ctx->xof_pos = 0;
        }
        if (ctx->xof_pos + len <= ctx->xof_len) {
            memcpy(out, ctx->xof_buf + ctx->xof_pos, len);
            ctx->xof_pos += len;
        } else {
            unsigned long long new_len = ctx->xof_len * 2;
            while (new_len < ctx->xof_pos + len) new_len *= 2;
            ctx->xof_buf = realloc(ctx->xof_buf, new_len);
            unsigned long long xof_bits = (unsigned long long)(ctx->msg_len) * 8;
            pseudoXOF(new_len * 8, ctx->msg, xof_bits, ctx->xof_buf);
            memcpy(out, ctx->xof_buf + ctx->xof_pos, len);
            ctx->xof_pos += len;
            ctx->xof_len = new_len;
        }
    }

    void uvw_hash_free(shake256_ctx *ctx) {
        free(ctx->msg);
        free(ctx->xof_buf);
        ctx->msg = NULL;
        ctx->xof_buf = NULL;
    }

    #define shake256_init(ctx) uvw_hash_init(ctx)
    #define shake256_update(ctx, data, len) uvw_hash_update(ctx, data, len)
    #define shake256_read(ctx, out, len) uvw_hash_read(ctx, out, len)
#else
    #include "csprng.h"
#endif

void hash_to_vf3(shake256_ctx *ctx, vf3_e **out, const size_t len) {
    uint8_t *buf = malloc(len * sizeof(uint8_t));
    *out = vf3_alloc(len);

    size_t written = 0;
    uint8_t trits[5];
    uint8_t val;
    while (written < len) {
        shake256_read(ctx, &val, 1);
        if (val > 242) continue;
        for (size_t i = 0; i < 5; i++) {
            trits[i] = val % 3;
            val /= 3;
        }

        const size_t w = MIN(5, len - written);
        memcpy(&buf[written], trits, w);
        written += w;
    }

    vf3_set_slice(*out, buf);
    free(buf);
}

void infset(const mf3_e *m, size_t **iset, size_t **co_iset) {
    size_t *cols = malloc(m->n_col * sizeof(size_t));
    mf3_e *m_test = mf3_alloc(m->n_row, m->n_row);

    while (true) {
        for (size_t i = 0; i < m->n_col; i++) cols[i] = i;
        csprng_shuffle_partial_size_t(cols, m->n_col, m->n_row);
        for (size_t k = 0; k < m->n_row; k++) {
            const size_t j = cols[k];
            for (size_t i = 0; i < m->n_row; i++) {
                mf3_setcoeff(m_test, i, k, mf3_coeff(m, i, j));
            }
        }

        mf3_e *dummy;
        const size_t rank = gauss_jordan_elimination(m_test, &dummy, NULL, NULL);
        mf3_free(dummy);
        if (rank == m->n_row) break;
    }

    *co_iset = malloc(m->n_row * sizeof(size_t));
    *iset = malloc((m->n_col - m->n_row) * sizeof(size_t));
    memcpy(*co_iset, cols, m->n_row * sizeof(size_t));
    memcpy(*iset, &cols[m->n_row], (m->n_col - m->n_row) * sizeof(size_t));

    free(cols);
    mf3_free(m_test);
}

void dec_z(const mf3_e *h_z, const vf3_e *s2, vf3_e **e2) {
    size_t counter = 0;

    while (true) {
        counter++;

        const size_t t = csprng_get_size_t() % (h_z->n_col - h_z->n_row + 1);

        size_t *cols = malloc(h_z->n_col * sizeof(size_t));
        for (size_t i = 0; i < h_z->n_col; i++) cols[i] = i;
        csprng_shuffle_partial_size_t(cols, h_z->n_col, h_z->n_row);
        size_t *iset = malloc((h_z->n_col - h_z->n_row) * sizeof(size_t));
        size_t *co_iset = malloc(h_z->n_row * sizeof(size_t));
        memcpy(co_iset, cols, h_z->n_row * sizeof(size_t));
        memcpy(iset, &cols[h_z->n_row], (h_z->n_col - h_z->n_row) * sizeof(size_t));
        free(cols);

        vf3_e *x = vf3_alloc(h_z->n_col);
        csprng_shuffle_partial_size_t(iset, h_z->n_col - h_z->n_row, t);
        for (size_t i = 0; i < t; i++) {
            vf3_set_coeff(iset[i], x, (csprng_get_uint8_t() & 1) + 1);
        }

        dp_e *p = dp_alloc(h_z->n_col);
        for (size_t i = 0; i < h_z->n_row; i++) p->p[co_iset[i]] = i;
        for (size_t i = 0; i < (h_z->n_col - h_z->n_row); i++) p->p[iset[i]] = h_z->n_row + i;
        dp_e *pt = dp_alloc(h_z->n_col);
        dp_transpose_and_copy(pt, p);
        free(iset);
        free(co_iset);

        mf3_e *hp = mf3_alloc(h_z->n_row, h_z->n_col);
        dp_right_product_matrix(hp, p, h_z);
        mf3_e *a = mf3_alloc(h_z->n_row, h_z->n_row);
        mf3_e *b = mf3_alloc(h_z->n_row, h_z->n_col - h_z->n_row);
        for (size_t i = 0; i < h_z->n_row; i++) vf3_vector_split(&a->rows[i], &b->rows[i], &hp->rows[i]);
        mf3_free(hp);

        mf3_e *ai = mf3_alloc(h_z->n_row, h_z->n_row);
        bool ai_result = inverse_matrix(ai, a);
        mf3_free(a);
        if (!ai_result) {
            mf3_free(b);
            continue;
        }

        vf3_e *xp = vf3_alloc(h_z->n_col);
        dp_right_product_vector(xp, p, x);
        dp_free(p);
        vf3_e *z = vf3_alloc(h_z->n_row);
        vf3_e *f = vf3_alloc(h_z->n_col - h_z->n_row);
        vf3_vector_split(z, f, xp);
        vf3_free(xp);
        vf3_free(z);
        vf3_free(x);

        mf3_e *bt = mf3_alloc(h_z->n_col - h_z->n_row, h_z->n_row);
        mf3_transpose_and_copy(bt, b);
        mf3_free(b);

        vf3_e *fbt = vf3_alloc(h_z->n_row);
        mf3_product_vector_matrix(fbt, f, bt);
        mf3_free(bt);

        vf3_e *sfbt = vf3_alloc(h_z->n_row);
        vf3_vector_sub(sfbt, s2, fbt);
        vf3_free(fbt);

        mf3_e *ait = mf3_alloc(h_z->n_row, h_z->n_row);
        mf3_transpose_and_copy(ait, ai);
        mf3_free(ai);

        vf3_e *sfbtait = vf3_alloc(h_z->n_row);
        mf3_product_vector_matrix(sfbtait, sfbt, ait);
        vf3_free(sfbt);
        mf3_free(ait);

        vf3_e *sfbtaitf = vf3_alloc(h_z->n_col);
        vf3_vector_cat(sfbtaitf, sfbtait, f);
        vf3_free(sfbtait);
        vf3_free(f);

        *e2 = vf3_alloc(h_z->n_col);
        dp_right_product_vector(*e2, pt, sfbtaitf);
        dp_free(pt);
        vf3_free(sfbtaitf);

        if (true) {
            return;
        } else {
            vf3_free(*e2);
        }
    }
}

void dec_xy(const mf3_e *h_xy, const vf3_e *s1, const vf3_e *e2, vf3_e **e1, const size_t w) {
    size_t counter = 0;

    size_t *iset, *co_iset;
    mf3_e *ai, *bt;
    dp_e *p, *pt;

    while (true) {
        size_t *cols = malloc(h_xy->n_col * sizeof(size_t));
        for (size_t i = 0; i < h_xy->n_col; i++) cols[i] = i;
        csprng_shuffle_partial_size_t(cols, h_xy->n_col, h_xy->n_row);
        iset = malloc((h_xy->n_col - h_xy->n_row) * sizeof(size_t));
        co_iset = malloc(h_xy->n_row * sizeof(size_t));
        memcpy(co_iset, cols, h_xy->n_row * sizeof(size_t));
        memcpy(iset, &cols[h_xy->n_row], (h_xy->n_col - h_xy->n_row) * sizeof(size_t));
        free(cols);

        p = dp_alloc(h_xy->n_col);
        for (size_t i = 0; i < h_xy->n_row; i++) p->p[co_iset[i]] = i;
        for (size_t i = 0; i < (h_xy->n_col - h_xy->n_row); i++) p->p[iset[i]] = h_xy->n_row + i;
        pt = dp_alloc(h_xy->n_col);
        dp_transpose_and_copy(pt, p);

        mf3_e *hp = mf3_alloc(h_xy->n_row, h_xy->n_col);
        dp_right_product_matrix(hp, p, h_xy);
        mf3_e *a = mf3_alloc(h_xy->n_row, h_xy->n_row);
        mf3_e *b = mf3_alloc(h_xy->n_row, h_xy->n_col - h_xy->n_row);
        for (size_t i = 0; i < h_xy->n_row; i++) vf3_vector_split(&a->rows[i], &b->rows[i], &hp->rows[i]);
        mf3_free(hp);

        ai = mf3_alloc(h_xy->n_row, h_xy->n_row);
        bool ai_result = inverse_matrix(ai, a);
        mf3_free(a);
        if (ai_result) {
            bt = mf3_alloc(h_xy->n_col - h_xy->n_row, h_xy->n_row);
            mf3_transpose_and_copy(bt, b);
            mf3_free(b);
            break;
        }
        mf3_free(b);
        mf3_free(ai);
        dp_free(p);
        dp_free(pt);
        free(iset);
        free(co_iset);
    }

    while (true) {
        counter++;

        vf3_e *x = vf3_alloc(h_xy->n_col);
        for (size_t i = 0; i < h_xy->n_col - h_xy->n_row; i++) {
            uint8_t e2_coeff = vf3_get_element(iset[i], e2);
            vf3_set_coeff(iset[i], x, e2_coeff ? e2_coeff : ((csprng_get_uint8_t() & 1) + 1));
        }

        vf3_e *xp = vf3_alloc(h_xy->n_col);
        dp_right_product_vector(xp, p, x);
        vf3_e *z = vf3_alloc(h_xy->n_row);
        vf3_e *f = vf3_alloc(h_xy->n_col - h_xy->n_row);
        vf3_vector_split(z, f, xp);
        vf3_free(xp);
        vf3_free(z);
        vf3_free(x);

        vf3_e *fbt = vf3_alloc(h_xy->n_row);
        mf3_product_vector_matrix(fbt, f, bt);

        vf3_e *sfbt = vf3_alloc(h_xy->n_row);
        vf3_vector_sub(sfbt, s1, fbt);
        vf3_free(fbt);

        mf3_e *ait = mf3_alloc(h_xy->n_row, h_xy->n_row);
        mf3_transpose_and_copy(ait, ai);

        vf3_e *sfbtait = vf3_alloc(h_xy->n_row);
        mf3_product_vector_matrix(sfbtait, sfbt, ait);
        vf3_free(sfbt);
        mf3_free(ait);

        vf3_e *sfbtaitf = vf3_alloc(h_xy->n_col);
        vf3_vector_cat(sfbtaitf, sfbtait, f);
        vf3_free(sfbtait);
        vf3_free(f);

        *e1 = vf3_alloc(h_xy->n_col);
        dp_right_product_vector(*e1, pt, sfbtaitf);
        vf3_free(sfbtaitf);

        vf3_e *e1e2 = vf3_alloc(h_xy->n_col);
        vf3_vector_add(e1e2, *e1, e2);

        vf3_e *e1e1e2 = vf3_alloc(h_xy->n_col * 2);
        vf3_vector_cat(e1e1e2, *e1, e1e2);
        vf3_free(e1e2);

        const size_t ew = vf3_hamming_weight(e1e1e2);
        vf3_free(e1e1e2);

        if (true && ew == w) {
            free(iset);
            free(co_iset);
            mf3_free(ai);
            mf3_free(bt);
            dp_free(p);
            dp_free(pt);
            return;
        } else {
            vf3_free(*e1);
        }
    }
}

uvw_param uvw_param_from_level(const size_t level) {
    switch (level) {
        case 0:
            return (uvw_param){ lambda: 8, n: 20, k: 10, k1: 7, k2: 3, w: 18 };
        case 1:
            return (uvw_param){ lambda: 128, n: 9700, k: 4850, k1: 3250, k2: 1600, w: 8633 };
        case 2:
            return (uvw_param){ lambda: 256, n: 19200, k: 9600, k1: 6433, k2: 3167, w: 17088 };
        case 3:
            return (uvw_param){ lambda: 512, n: 39000, k: 19500, k1: 13066, k2: 6434, w: 34710 };
    }
    PANIC("Unknown param level %lu", level);
    return (uvw_param){ };
}

uvw_keypair uvw_keygen(const uvw_param param) {
    uvw_keypair keypair;

    keypair.sk.param = keypair.pk.param = param;
    keypair.sk.sti = mf3_alloc(param.n - param.k, param.n - param.k);
    keypair.sk.h_x = mf3_alloc(param.n / 2 - param.k1, param.n / 2);
    keypair.sk.h_y = mf3_alloc(param.n / 2 - param.k1, param.n / 2);
    keypair.sk.h_z = mf3_alloc(param.n / 2 - param.k2, param.n / 2);
    keypair.sk.d = dp_alloc(param.n);
    keypair.pk.r = mf3_alloc(param.n - param.k, param.k);

    mf3_random(keypair.sk.h_x);
    mf3_random(keypair.sk.h_y);
    mf3_random(keypair.sk.h_z);

    dp_e *d = dp_alloc(param.n);
    dp_random_non_zero(d);
    dp_shuffle(d);

    mf3_e *h_z_neg = mf3_alloc(param.n / 2 - param.k2, param.n / 2);
    mf3_matrix_neg(h_z_neg, keypair.sk.h_z);
    mf3_e *h_sk_left = mf3_alloc(param.n - param.k, param.n);
    for (size_t i = 0; i < param.n / 2 - param.k1; i++) {
        vf3_vector_cat(
            &h_sk_left->rows[i],
            &keypair.sk.h_x->rows[i],
            &keypair.sk.h_y->rows[i]
        );
    }
    for (size_t i = param.n / 2 - param.k1; i < param.n - param.k; i++) {
        vf3_vector_cat(
            &h_sk_left->rows[i],
            &h_z_neg->rows[i - (param.n / 2 - param.k1)],
            &keypair.sk.h_z->rows[i - (param.n / 2 - param.k1)]
        );
    }

    mf3_e *h_sk = mf3_alloc(param.n - param.k, param.n);
    dp_right_product_matrix(h_sk, d, h_sk_left);

    mf3_e *h_sk_tilde, *s;
    dp_e *p;
    gauss_jordan_elimination(h_sk, &h_sk_tilde, &s, &p);

    mf3_e *st = mf3_alloc(param.n - param.k, param.n - param.k);
    mf3_transpose_and_copy(st, s);
    mf3_free(s);
    inverse_matrix(keypair.sk.sti, st);
    mf3_free(st);

    mf3_e *ir = mf3_alloc(param.n - param.k, param.n);
    dp_right_product_matrix(ir, p, h_sk_tilde);
    vf3_e *dummy = vf3_alloc(param.n - param.k);
    for (size_t i = 0; i < param.n - param.k; i++) {
        vf3_vector_split(dummy, &keypair.pk.r->rows[i], &ir->rows[i]);
    }
    vf3_free(dummy);

    dp_product(keypair.sk.d, d, p);

    mf3_free(h_z_neg);
    mf3_free(h_sk_left);
    mf3_free(h_sk);
    mf3_free(h_sk_tilde);
    dp_free(d);
    dp_free(p);
    mf3_free(ir);

    return keypair;
}

uvw_signature uvw_sign(const uvw_secret_key sk, shake256_ctx *ctx) {
    const uvw_param param = sk.param;
    uvw_signature sign;
    sign.param = param;

    const size_t r_len = param.lambda / 8 * sizeof(uint8_t);
    sign.r = malloc(r_len);
    csprng_read(sign.r, r_len);
    shake256_update(ctx, sign.r, r_len);
    vf3_e *s; hash_to_vf3(ctx, &s, param.n - param.k);

    vf3_e *s1s2 = vf3_alloc(param.n - param.k);
    mf3_product_vector_matrix(s1s2, s, sk.sti);
    vf3_free(s);
    vf3_e *s1 = vf3_alloc(param.n / 2 - param.k1);
    vf3_e *s2 = vf3_alloc(param.n / 2 - param.k2);
    vf3_vector_split(s1, s2, s1s2);
    vf3_free(s1s2);

    vf3_e *e2;
    dec_z(sk.h_z, s2, &e2);

    mf3_e *hxhy = mf3_alloc(param.n / 2 - param.k1, param.n / 2);
    mf3_matrix_add(hxhy, sk.h_x, sk.h_y);
    mf3_e *hyt = mf3_alloc(param.n / 2, param.n / 2 - param.k1);
    mf3_transpose_and_copy(hyt, sk.h_y);
    vf3_e *e2hyt = vf3_alloc(param.n / 2 - param.k1);
    mf3_product_vector_matrix(e2hyt, e2, hyt);
    mf3_free(hyt);
    vf3_e *s1e2hyt = vf3_alloc(param.n / 2 - param.k1);
    vf3_vector_sub(s1e2hyt, s1, e2hyt);
    vf3_free(e2hyt);
    vf3_e *e1;
    dec_xy(hxhy, s1e2hyt, e2, &e1, param.w);
    mf3_free(hxhy);

    vf3_e *e1e2 = vf3_alloc(param.n / 2);
    vf3_vector_add(e1e2, e1, e2);
    vf3_free(e2);
    vf3_e *e1e1e2 = vf3_alloc(param.n);
    vf3_vector_cat(e1e1e2, e1, e1e2);
    vf3_free(e1e2);
    vf3_free(e1);

    vf3_e *fe = vf3_alloc(param.n);
    dp_right_product_vector(fe, sk.d, e1e1e2);
    size_t fe_hw = vf3_hamming_weight(fe);
    fprintf(stderr, "DBG_SIGN: e1e1e2_hw=%zu fe_hw=%zu diff=%zu\n", vf3_hamming_weight(e1e1e2), fe_hw, vf3_hamming_weight(e1e1e2) - fe_hw);
    vf3_free(e1e1e2);

    vf3_e *f = vf3_alloc(param.n - param.k);
    sign.e = vf3_alloc(param.k);
    vf3_vector_split(f, sign.e, fe);
    size_t d_zero = 0;
    for (size_t i = 0; i < param.n; i++) if (vf3_get_element(i, sk.d->d) == 0) d_zero++;
    size_t p_dup = 0;
    for (size_t i = 0; i < param.n; i++) for (size_t j = i+1; j < param.n; j++) if (sk.d->p[i] == sk.d->p[j]) p_dup++;
    fprintf(stderr, "DBG_SIGN: e1e1e2_hw=%zu f_hw=%zu e_hw=%zu sum=%zu w=%zu d_zeros=%zu p_dups=%zu\n", fe_hw, vf3_hamming_weight(f), vf3_hamming_weight(sign.e), vf3_hamming_weight(f) + vf3_hamming_weight(sign.e), param.w, d_zero, p_dup);
    vf3_free(fe);
    vf3_free(f);

    return sign;
}

bool uvw_verify(const uvw_public_key pk, shake256_ctx *ctx, const uvw_signature sign) {
    if (memcmp(&pk.param, &sign.param, sizeof(uvw_param))) return false;

    const uvw_param param = pk.param;

    const size_t r_len = param.lambda / 8 * sizeof(uint8_t);
    shake256_update(ctx, sign.r, r_len);
    vf3_e *s; hash_to_vf3(ctx, &s, param.n - param.k);

    mf3_e *rt = mf3_alloc(param.k, param.n - param.k);
    mf3_transpose_and_copy(rt, pk.r);

    vf3_e *ert = vf3_alloc(param.n - param.k);
    mf3_product_vector_matrix(ert, sign.e, rt);
    vf3_vector_sub_inplace(ert, s);

    const size_t ew = vf3_hamming_weight(sign.e);
    const size_t ertw = vf3_hamming_weight(ert);
    fprintf(stderr, "DBG_VFY: ew=%zu ertw=%zu sum=%zu w=%zu match=%d\n", ew, ertw, ew + ertw, param.w, (ew + ertw == param.w));
    bool result = param.w == ew + ertw;

    vf3_free(s);
    vf3_free(ert);
    mf3_free(rt);

    return result;
}

void uvw_free_secret_key(uvw_secret_key sk) {
    mf3_free(sk.sti);
    mf3_free(sk.h_x);
    mf3_free(sk.h_y);
    mf3_free(sk.h_z);
    dp_free(sk.d);
}

void uvw_free_public_key(uvw_public_key pk) {
    mf3_free(pk.r);
}

void uvw_free_signature(uvw_signature sign) {
    free(sign.r);
    vf3_free(sign.e);
}

void uvw_write_param(const uvw_param param, FILE *f) {
    uint16_t x;
    x = param.lambda; fwrite(&x, sizeof(uint16_t), 1, f);
    x = param.n; fwrite(&x, sizeof(uint16_t), 1, f);
    x = param.k; fwrite(&x, sizeof(uint16_t), 1, f);
    x = param.k1; fwrite(&x, sizeof(uint16_t), 1, f);
    x = param.k2; fwrite(&x, sizeof(uint16_t), 1, f);
    x = param.w; fwrite(&x, sizeof(uint16_t), 1, f);
}

void uvw_write_secret_key(const uvw_secret_key sk, FILE *f) {
    fwrite("UVWSSECK", sizeof(uint8_t), 8, f);
    uvw_write_param(sk.param, f);
    mf3_write(sk.sti, f);
    mf3_write(sk.h_x, f);
    mf3_write(sk.h_y, f);
    mf3_write(sk.h_z, f);
    dp_write(sk.d, f);
}

void uvw_write_public_key(const uvw_public_key pk, FILE *f) {
    fwrite("UVWSPUBK", sizeof(uint8_t), 8, f);
    uvw_write_param(pk.param, f);
    mf3_write(pk.r, f);
}

void uvw_write_signature(const uvw_signature sign, FILE *f) {
    fwrite("UVWSSIGN", sizeof(uint8_t), 8, f);
    uvw_write_param(sign.param, f);
    fwrite(sign.r, sizeof(uint8_t), sign.param.lambda / 8, f);
    vf3_write(sign.e, f);
}

uvw_param uvw_read_param(FILE *f) {
    uvw_param param;
    uint16_t x;
    fread(&x, sizeof(uint16_t), 1, f); param.lambda = x;
    fread(&x, sizeof(uint16_t), 1, f); param.n = x;
    fread(&x, sizeof(uint16_t), 1, f); param.k = x;
    fread(&x, sizeof(uint16_t), 1, f); param.k1 = x;
    fread(&x, sizeof(uint16_t), 1, f); param.k2 = x;
    fread(&x, sizeof(uint16_t), 1, f); param.w = x;
    return param;
}

uvw_secret_key uvw_read_secret_key(FILE *f) {
    uint8_t h[8];
    fread(h, sizeof(uint8_t), 8, f);
    if (memcmp(h, "UVWSSECK", 8)) PANIC("Incorrect secret key header");
    uvw_param param = uvw_read_param(f);
    uvw_secret_key sk;
    sk.param = param;
    sk.sti = mf3_alloc(param.n - param.k, param.n - param.k); mf3_read(sk.sti, f);
    sk.h_x = mf3_alloc(param.n / 2 - param.k1, param.n / 2); mf3_read(sk.h_x, f);
    sk.h_y = mf3_alloc(param.n / 2 - param.k1, param.n / 2); mf3_read(sk.h_y, f);
    sk.h_z = mf3_alloc(param.n / 2 - param.k2, param.n / 2); mf3_read(sk.h_z, f);
    sk.d = dp_alloc(param.n); dp_read(sk.d, f);
    return sk;
}

uvw_public_key uvw_read_public_key(FILE *f) {
    uint8_t h[8];
    fread(h, sizeof(uint8_t), 8, f);
    if (memcmp(h, "UVWSPUBK", 8)) PANIC("Incorrect public key header");
    uvw_param param = uvw_read_param(f);
    uvw_public_key pk;
    pk.param = param;
    pk.r = mf3_alloc(param.n - param.k, param.k); mf3_read(pk.r, f);
    return pk;
}

uvw_signature uvw_read_signature(FILE *f) {
    uint8_t h[8];
    fread(h, sizeof(uint8_t), 8, f);
    if (memcmp(h, "UVWSSIGN", 8)) PANIC("Incorrect signature header");
    uvw_param param = uvw_read_param(f);
    uvw_signature sign;
    sign.param = param;
    sign.r = malloc(sizeof(uint8_t) * param.lambda / 8); fread(sign.r, sizeof(uint8_t), param.lambda / 8, f);
    sign.e = vf3_alloc(param.k); vf3_read(sign.e, f);
    return sign;
}
