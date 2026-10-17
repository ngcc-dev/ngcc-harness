/* Drop-in replacement for gauss.c:inverse_matrix used ONLY to speed up the
   statistics experiment.  The inverse of a square matrix is unique and the
   invertibility verdict is unique, so outputs (and therefore every signature
   and DRBG draw) are identical to the frozen routine.  FASTINV_CHECK=1
   compares against the original on every call. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "fq_arithmetic/mf3.h"
#include "fq_arithmetic/vf3.h"
bool inverse_matrix_orig(mf3_e *inv, mf3_e const *mat);
static inline void addmul(uint64_t *x0, uint64_t *x1, const uint64_t *y0s, const uint64_t *y1s, int c, size_t a, size_t b) {
    uint64_t mneg = -(uint64_t)(c > 1);
    for (size_t i = a; i < b; i++) {
        uint64_t y0 = y0s[i], y1 = (y1s[i] ^ mneg) & y0;
        uint64_t tmp = x1[i] ^ y0;
        x1[i] = (x0[i] ^ y1) & tmp;
        x0[i] = (x0[i] ^ y0) | (tmp ^ y1);
    }
}
static bool fast_inverse(mf3_e *inv, mf3_e const *mat) {
    size_t d = mat->n_row; if (d != mat->n_col) return false;
    size_t wl = (d + 63) / 64, W = 2 * wl;            /* left words | right words */
    uint64_t *R0 = calloc(d * W, 8), *R1 = calloc(d * W, 8);
    for (size_t i = 0; i < d; i++) {
        memcpy(R0 + i * W, mat->rows[i].r0, wl * 8); memcpy(R1 + i * W, mat->rows[i].r1, wl * 8);
        if (d % 64) { R0[i*W + wl-1] &= (1ULL << (d % 64)) - 1; R1[i*W + wl-1] &= (1ULL << (d % 64)) - 1; }
        R0[i * W + wl + i / 64] |= 1ULL << (i % 64);
    }
    size_t *rowp = malloc(d * sizeof(size_t)); for (size_t i = 0; i < d; i++) rowp[i] = i;
    bool ok = true;
    for (size_t c = 0; c < d; c++) {
        size_t cw = c / 64; uint64_t bit = 1ULL << (c % 64), piv = SIZE_MAX;
        for (size_t r = c; r < d; r++) if (R0[rowp[r] * W + cw] & bit) { piv = r; break; }
        if (piv == SIZE_MAX) { ok = false; break; }
        size_t t = rowp[c]; rowp[c] = rowp[piv]; rowp[piv] = t;
        uint64_t *p0 = R0 + rowp[c] * W, *p1 = R1 + rowp[c] * W;
        if (p1[cw] & bit) for (size_t i = cw; i < W; i++) p1[i] ^= p0[i];   /* scale by 2 = negate */
        for (size_t r = 0; r < d; r++) {
            if (r == c) continue;
            uint64_t *x0 = R0 + rowp[r] * W, *x1 = R1 + rowp[r] * W;
            if (!(x0[cw] & bit)) continue;
            int v = (x1[cw] & bit) ? 2 : 1;            /* x_c; subtract v*row_p == add (3-v)*row_p */
            addmul(x0, x1, p0, p1, 3 - v, cw, W);
        }
    }
    if (ok) for (size_t i = 0; i < d; i++) {
        vf3_e *o = &inv->rows[i];
        memset(o->r0, 0, o->alloc * 8); memset(o->r1, 0, o->alloc * 8);
        memcpy(o->r0, R0 + rowp[i] * W + wl, wl * 8); memcpy(o->r1, R1 + rowp[i] * W + wl, wl * 8);
    }
    free(R0); free(R1); free(rowp);
    return ok;
}
bool inverse_matrix(mf3_e *inv, mf3_e const *mat) {
    bool ok = fast_inverse(inv, mat);
    if (getenv("FASTINV_CHECK")) {
        mf3_e *ref = mf3_alloc(mat->n_row, mat->n_col);
        bool ok2 = inverse_matrix_orig(ref, mat);
        int same = (ok == ok2);
        if (ok && ok2) for (size_t i = 0; i < mat->n_row && same; i++)
            for (size_t j = 0; j < mat->n_col; j++) if (vf3_get_element(j, &ref->rows[i]) != vf3_get_element(j, &inv->rows[i])) { same = 0; break; }
        fprintf(stderr, "FASTINV_CHECK d=%zu ok=%d orig_ok=%d identical=%d\n", mat->n_row, ok, ok2, same);
        mf3_free(ref);
    }
    return ok;
}
