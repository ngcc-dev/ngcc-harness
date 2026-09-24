#include "dp.h"
#include "vf3.h"
#include "mf3.h"
#include "../macros.h"

#include <stdio.h>
#include <stdlib.h>

#ifdef USE_API_PKC
    #include "../drng.h"
    extern DRNG_ctx *uvw_drng;

    static inline void dp_csprng_read(void *out, size_t length) {
        get_random_number(uvw_drng, out, (unsigned long long)length * 8);
    }

    static inline void dp_csprng_shuffle_size_t(size_t *arr, size_t len) {
        for (size_t i = 0; i < len; i++) {
            unsigned char buf[sizeof(size_t)];
            get_random_number(uvw_drng, buf, sizeof(size_t) * 8);
            size_t j;
            memcpy(&j, buf, sizeof(size_t));
            j = j % (len - i);
            size_t tmp = arr[i];
            arr[i] = arr[i + j];
            arr[i + j] = tmp;
        }
    }
    #define csprng_read(buf, len) dp_csprng_read(buf, len)
    #define csprng_shuffle_size_t(arr, len) dp_csprng_shuffle_size_t(arr, len)
#else
    #include "../csprng.h"
#endif

void dp_init(dp_e *dp, const size_t length) {
    dp->d = vf3_alloc(length);
    vf3_vector_constant(dp->d, 1);
    dp->p = malloc(length * sizeof(size_t));
    for (size_t i = 0; i < length; i++) dp->p[i] = i;
}

dp_e *dp_alloc(const size_t length) {
    dp_e *dp = malloc(sizeof(dp_e));
    dp_init(dp, length);
    return dp;
}

void dp_free(dp_e *dp) {
    vf3_free(dp->d);
    free(dp->p);
    free(dp);
}

void dp_set_diag(dp_e *dp, const uint8_t *data) {
    for (size_t i = 0; i < dp->d->size; i++) {
        vf3_set_coeff(i, dp->d, data[i]);
    }
}

void dp_set_perm(dp_e *dp, const size_t *data) {
    memcpy(dp->p, data, sizeof(size_t) * dp->d->size);
}

void dp_print(dp_e *dp) {
    for (size_t i = 0; i < dp->d->size; i++) {
        for (size_t j = 0; j < dp->d->size; j++) {
            if (dp->p[i] == j) {
                printf("%d,", vf3_get_element(i, dp->d));
            } else {
                printf("_,");
            }
        }
        putchar('\n');
    }
    putchar('\n');
}

void dp_random(dp_e *dp) {
    vf3_random(dp->d);
}

void dp_random_non_zero(dp_e *dp) {
    vf3_random_non_zero(dp->d);
}

void dp_shuffle(dp_e *dp) {
    csprng_shuffle_size_t(dp->p, dp->d->size);
}

bool dp_equal(const dp_e *a, const dp_e *b) {
    return (
        (a->d->size == b->d->size) &&
        vf3_equal(a->d, b->d) &&
        !memcmp(a->p, b->p, a->d->size * sizeof(size_t))
    );
}

void dp_product(dp_e *c, const dp_e *a, const dp_e *b) {
    for (size_t i = 0; i < c->d->size; i++) {
        vf3_set_coeff(i, c->d, (vf3_get_element(i, a->d) * vf3_get_element(a->p[i], b->d)) % 3);
        c->p[i] = b->p[a->p[i]];
    }
}

void dp_left_product_matrix(mf3_e *r, const dp_e *dp, const mf3_e *m) {
    // 左乘一个对角置换矩阵，dp[i,j]=k，表示r[i,*]=k*m[j,*]
    for (size_t i = 0; i < dp->d->size; i++) {
        vf3_copy(&r->rows[i], &m->rows[dp->p[i]]);
        vf3_vector_scalarmul_inplace(&r->rows[i], vf3_get_element(i, dp->d));
    }
}

void dp_right_product_matrix(mf3_e *r, const dp_e *dp, const mf3_e *m) {
    for (size_t i = 0; i < m->n_col; i++) {
        size_t dst_col = dp->p[i];
        uint8_t d_val = vf3_get_element(i, dp->d);
        for (size_t row = 0; row < m->n_row; row++) {
            uint8_t val = (mf3_coeff(m, row, i) * d_val) % 3;
            mf3_setcoeff(r, row, dst_col, val);
        }
    }
}

void dp_right_product_vector(vf3_e *y, const dp_e *dp, const vf3_e *x) {
    for (size_t i = 0; i < dp->d->size; i++) {
        vf3_set_coeff(dp->p[i], y, (vf3_get_element(i, dp->d) * vf3_get_element(i, x)) % 3);
    }
}

void dp_swap_rows(dp_e *dp, const size_t i, const size_t j) {
    // 1 0 0     1 0 0
    // 0 2 0 <-> 0 0 3
    // 0 0 3     0 2 0
    const uint8_t x = vf3_get_element(i, dp->d);
    vf3_set_coeff(i, dp->d, vf3_get_element(j, dp->d));
    vf3_set_coeff(j, dp->d, x);
    SWAP(dp->p[i], dp->p[j], size_t);
}

void dp_swap_columns(dp_e *dp, const size_t i, const size_t j) {
    // 1 0 0     1 0 0
    // 0 2 0 <-> 0 0 2
    // 0 0 3     0 3 0
    size_t ii = 0, jj = 0;
    while (i != dp->p[ii]) ii++;
    while (j != dp->p[jj]) jj++;
    SWAP(dp->p[ii], dp->p[jj], size_t);
}

void dp_transpose_and_copy(dp_e *dpt, const dp_e *dp) {
    for (size_t i = 0; i < dp->d->size; i++) {
        vf3_set_coeff(dp->p[i], dpt->d, vf3_get_element(i, dp->d));
        dpt->p[dp->p[i]] = i;
    }
}

size_t dp_read(dp_e *dp, FILE *f) {
    size_t s = 0;
    s += vf3_read(dp->d, f);
    for (size_t i = 0; i < dp->d->size; i++) {
        uint16_t x;
        s += fread(&x, sizeof(uint16_t), 1, f);
        dp->p[i] = x;
    }
    return s;
}

size_t dp_write(dp_e *dp, FILE *f) {
    size_t s = 0;
    s += vf3_write(dp->d, f);
    for (size_t i = 0; i < dp->d->size; i++) {
        uint16_t x = dp->p[i];
        s += fwrite(&x, sizeof(uint16_t), 1, f);
    }
    return s;
}