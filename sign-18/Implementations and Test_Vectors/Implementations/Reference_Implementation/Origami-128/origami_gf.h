// SPDX-License-Identifier: MIT
// Origami: GF(q) arithmetic and variable-dimension matrix operations
// Reuses GF table logic from the original implementation, adds parameterized matrix ops

#ifndef ORIGAMI_GF_H
#define ORIGAMI_GF_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "origami.h"

typedef uint8_t gf_t;

#if defined(__GNUC__) || defined(__clang__)
#define ORIGAMI_MAYBE_UNUSED __attribute__((unused))
#else
#define ORIGAMI_MAYBE_UNUSED
#endif

// =========================================================
// GF(q) lookup tables
// =========================================================
static gf_t gf_multtab[ORIGAMI_q * ORIGAMI_q];
static gf_t gf_invtab[ORIGAMI_q];
static gf_t gf_addtab[ORIGAMI_q * ORIGAMI_q];

// =========================================================
// Scalar GF operations
// =========================================================
static inline gf_t gf_mult(gf_t a, gf_t b) {
    return gf_multtab[a * ORIGAMI_q + b];
}

static inline gf_t gf_inv(gf_t a) {
    return gf_invtab[a];
}

static inline gf_t gf_add(gf_t a, gf_t b) {
    return gf_addtab[a * ORIGAMI_q + b];
}

static inline void gf_set_add(gf_t *a, gf_t b) {
    *a = gf_addtab[*a * ORIGAMI_q + b];
}

static inline gf_t gf_sub(gf_t a, gf_t b) {
#if ORIGAMI_q != 16
    return gf_addtab[a * ORIGAMI_q + (ORIGAMI_q - b) % ORIGAMI_q];
#else
    return gf_addtab[a * ORIGAMI_q + b];
#endif
}

// =========================================================
// Initialize GF tables (from snova_ref.c, depends only on q)
// =========================================================
static void init_gf_tables(void) {
#if ORIGAMI_q == 16
    // GF(16) = Z2[x]/(x^4+x+1)
    uint8_t F_star[15] = {1, 2, 4, 8, 3, 6, 12, 11, 5, 10, 7, 14, 15, 13, 9};
    for (int i = 0; i < 16; i++) {
        gf_multtab[i] = 0;
        gf_multtab[i * ORIGAMI_q] = 0;
    }
    for (int i = 0; i < ORIGAMI_q - 1; i++)
        for (int j = 0; j < ORIGAMI_q - 1; j++)
            gf_multtab[F_star[i] * ORIGAMI_q + F_star[j]] = F_star[(i + j) % (ORIGAMI_q - 1)];

    for (int i = 0; i < ORIGAMI_q; i++)
        for (int j = 0; j < ORIGAMI_q; j++)
            gf_addtab[i * ORIGAMI_q + j] = (i ^ j);
#else
    // GF(prime)
    for (int i = 0; i < ORIGAMI_q; i++)
        for (int j = 0; j < ORIGAMI_q; j++) {
            gf_multtab[i * ORIGAMI_q + j] = (i * j) % ORIGAMI_q;
            gf_addtab[i * ORIGAMI_q + j] = (i + j) % ORIGAMI_q;
        }
#endif
    // x^(q-2) = x^-1 (Fermat's little theorem)
    for (int i = 0; i < ORIGAMI_q; i++) {
        gf_t val = i;
        for (int j = 3; j < ORIGAMI_q; j++)
            val = gf_mult(val, i);
        gf_invtab[i] = val;
    }
}

// =========================================================
// Variable-dimension matrix operations
// =========================================================

// a[ad x cd] = b[ad x bd] * c[bd x cd]
static inline void gf_mat_mul_dim(gf_t *a, const gf_t *b, const gf_t *c,
                                   int ad, int bd, int cd) {
    for (int i = 0; i < ad; i++)
        for (int j = 0; j < cd; j++) {
            gf_t sum = 0;
            for (int k = 0; k < bd; k++)
                gf_set_add(&sum, gf_mult(b[i * bd + k], c[k * cd + j]));
            a[i * cd + j] = sum;
        }
}

// a[ad x cd] += b[ad x bd] * c[bd x cd]
static inline void gf_mat_mul_add_dim(gf_t *a, const gf_t *b, const gf_t *c,
                                       int ad, int bd, int cd) {
    for (int i = 0; i < ad; i++)
        for (int j = 0; j < cd; j++) {
            gf_t sum = 0;
            for (int k = 0; k < bd; k++)
                gf_set_add(&sum, gf_mult(b[i * bd + k], c[k * cd + j]));
            gf_set_add(&a[i * cd + j], sum);
        }
}

// Transpose: out[cols x rows] = transpose(in[rows x cols])
static inline void gf_mat_transpose_dim(gf_t *out, const gf_t *in,
                                          int rows, int cols) {
    for (int i = 0; i < rows; i++)
        for (int j = 0; j < cols; j++)
            out[j * rows + i] = in[i * cols + j];
}

// Determinant of dim x dim matrix via Gaussian elimination (in-place on copy)
// Returns determinant value
static gf_t gf_mat_det_dim(const gf_t *mat, int dim) {
    gf_t tmp[dim * dim];
    memcpy(tmp, mat, dim * dim * sizeof(gf_t));

    gf_t det = 1;
    for (int col = 0; col < dim; col++) {
        // Find pivot
        int pivot = -1;
        for (int row = col; row < dim; row++) {
            if (tmp[row * dim + col] != 0) {
                pivot = row;
                break;
            }
        }
        if (pivot < 0) return 0; // singular

        // Swap rows if needed
        if (pivot != col) {
            for (int k = col; k < dim; k++) {
                gf_t t = tmp[col * dim + k];
                tmp[col * dim + k] = tmp[pivot * dim + k];
                tmp[pivot * dim + k] = t;
            }
#if ORIGAMI_q != 16
            // For odd characteristic, swap negates determinant
            det = (ORIGAMI_q - det) % ORIGAMI_q;
#endif
            // For char 2 (q=16), swap doesn't change det sign
        }

        gf_t piv_val = tmp[col * dim + col];
        det = gf_mult(det, piv_val);
        gf_t piv_inv = gf_inv(piv_val);

        // Normalize pivot row
        for (int k = col; k < dim; k++)
            tmp[col * dim + k] = gf_mult(tmp[col * dim + k], piv_inv);

        // Eliminate below
        for (int row = col + 1; row < dim; row++) {
            gf_t factor = tmp[row * dim + col];
            if (factor != 0) {
                for (int k = col; k < dim; k++)
                    tmp[row * dim + k] = gf_sub(tmp[row * dim + k],
                                                 gf_mult(factor, tmp[col * dim + k]));
            }
        }
    }
    return det;
}

// Matrix inverse via augmented [A | I] Gaussian elimination
// inv[dim x dim] = mat[dim x dim]^{-1}
// Returns 0 on success, -1 if singular
static int gf_mat_inv_dim(gf_t *inv, const gf_t *mat, int dim) {
    gf_t aug[dim * 2 * dim];

    // Build augmented matrix [mat | I]
    for (int i = 0; i < dim; i++) {
        for (int j = 0; j < dim; j++) {
            aug[i * 2 * dim + j] = mat[i * dim + j];
            aug[i * 2 * dim + dim + j] = (i == j) ? 1 : 0;
        }
    }

    // Forward elimination
    for (int col = 0; col < dim; col++) {
        int pivot = -1;
        for (int row = col; row < dim; row++) {
            if (aug[row * 2 * dim + col] != 0) {
                pivot = row;
                break;
            }
        }
        if (pivot < 0) return -1; // singular

        if (pivot != col) {
            for (int k = 0; k < 2 * dim; k++) {
                gf_t t = aug[col * 2 * dim + k];
                aug[col * 2 * dim + k] = aug[pivot * 2 * dim + k];
                aug[pivot * 2 * dim + k] = t;
            }
        }

        gf_t piv_inv = gf_inv(aug[col * 2 * dim + col]);
        for (int k = 0; k < 2 * dim; k++)
            aug[col * 2 * dim + k] = gf_mult(aug[col * 2 * dim + k], piv_inv);

        for (int row = 0; row < dim; row++) {
            if (row == col) continue;
            gf_t factor = aug[row * 2 * dim + col];
            if (factor != 0) {
                for (int k = 0; k < 2 * dim; k++)
                    aug[row * 2 * dim + k] = gf_sub(aug[row * 2 * dim + k],
                                                      gf_mult(factor, aug[col * 2 * dim + k]));
            }
        }
    }

    // Extract inverse from right half
    for (int i = 0; i < dim; i++)
        for (int j = 0; j < dim; j++)
            inv[i * dim + j] = aug[i * 2 * dim + dim + j];

    return 0;
}

// Matrix-vector multiply: out[dim] = mat[dim x dim] * vec[dim]
static inline void gf_mat_vec_mul(gf_t *out, const gf_t *mat, const gf_t *vec, int dim) {
    for (int i = 0; i < dim; i++) {
        gf_t sum = 0;
        for (int j = 0; j < dim; j++)
            gf_set_add(&sum, gf_mult(mat[i * dim + j], vec[j]));
        out[i] = sum;
    }
}

// Solve linear system A*x = b via Gaussian elimination on augmented [A|b]
// A is dim x dim, b and x are dim-vectors
// Returns 0 on success, -1 if singular
static ORIGAMI_MAYBE_UNUSED int gf_solve_linear(gf_t *x, const gf_t *A, const gf_t *b, int dim) {
    gf_t aug[dim * (dim + 1)];

    // Build augmented matrix [A | b]
    for (int i = 0; i < dim; i++) {
        for (int j = 0; j < dim; j++)
            aug[i * (dim + 1) + j] = A[i * dim + j];
        aug[i * (dim + 1) + dim] = b[i];
    }

    // Forward elimination with partial pivoting
    for (int col = 0; col < dim; col++) {
        int pivot = -1;
        for (int row = col; row < dim; row++) {
            if (aug[row * (dim + 1) + col] != 0) {
                pivot = row;
                break;
            }
        }
        if (pivot < 0) return -1;

        if (pivot != col) {
            for (int k = col; k <= dim; k++) {
                gf_t t = aug[col * (dim + 1) + k];
                aug[col * (dim + 1) + k] = aug[pivot * (dim + 1) + k];
                aug[pivot * (dim + 1) + k] = t;
            }
        }

        gf_t piv_inv = gf_inv(aug[col * (dim + 1) + col]);
        for (int k = col; k <= dim; k++)
            aug[col * (dim + 1) + k] = gf_mult(aug[col * (dim + 1) + k], piv_inv);

        for (int row = col + 1; row < dim; row++) {
            gf_t factor = aug[row * (dim + 1) + col];
            if (factor != 0) {
                for (int k = col; k <= dim; k++)
                    aug[row * (dim + 1) + k] = gf_sub(aug[row * (dim + 1) + k],
                                                        gf_mult(factor, aug[col * (dim + 1) + k]));
            }
        }
    }

    // Back substitution
    for (int i = dim - 1; i >= 0; i--) {
        gf_t sum = 0;
        for (int k = i + 1; k < dim; k++)
            gf_set_add(&sum, gf_mult(aug[i * (dim + 1) + k], x[k]));
        x[i] = gf_sub(aug[i * (dim + 1) + dim], sum);
    }

    return 0;
}

// =========================================================
// Byte/GF conversion utilities (from snova_ref.c, q-only)
// =========================================================
static void convert_bytes_to_GF(gf_t *gf_array, const uint8_t *byte_array, size_t num) {
#if ORIGAMI_q != 16
    for (size_t idx = 0; idx < num; idx++)
        gf_array[idx] = byte_array[idx] % ORIGAMI_q;
#else
    for (size_t idx = 0; idx < num / 2; idx++) {
        gf_array[2 * idx] = (byte_array[idx] & 0xf) % ORIGAMI_q;
        gf_array[2 * idx + 1] = (byte_array[idx] >> 4) % ORIGAMI_q;
    }
    if (num & 1)
        gf_array[num - 1] = (byte_array[num / 2] & 0xf) % ORIGAMI_q;
#endif
}

static void compress_gf(uint8_t *byte_array, const gf_t *gf_array, size_t num) {
    size_t idx = 0;
    size_t out_idx = 0;
    size_t num_bytes = BYTES_GF(num);

    do {
        uint64_t val = 0;
        uint64_t fact = 1;
        int i = 0;
        while (i < PACK_GF && idx < num) {
            val += fact * (gf_array[idx] % ORIGAMI_q);
            idx++;
            i++;
            fact *= ORIGAMI_q;
        }
        i = (i + 1) / 2;
        int j = 0;
        while (j < PACK_BYTES && out_idx < num_bytes) {
            byte_array[out_idx] = val & 0xff;
            out_idx++;
            val >>= 8;
            j++;
        }
    } while (idx < num);
}

static int expand_gf(gf_t *gf_array, const uint8_t *byte_array, size_t num) {
    size_t num_bytes = BYTES_GF(num);
    size_t idx = 0;
    size_t out_idx = 0;
    int res = 0;

    do {
        uint64_t val = 0;
        int i = 0;
        while (i < PACK_BYTES && idx < num_bytes) {
            val = val ^ ((uint64_t)(byte_array[idx]) << (8 * i));
            idx++;
            i++;
        }
        int j = 0;
        while (j < PACK_GF && out_idx < num) {
            gf_array[out_idx] = val % ORIGAMI_q;
            val = val / ORIGAMI_q;
            out_idx++;
            j++;
        }
        res |= (int)val;
    } while (out_idx < num);

#if ORIGAMI_q == 16
    if (num & 1)
        return byte_array[num / 2] & 0xF0;
#endif
    return res;
}

// =========================================================
// Characteristic polynomial via Faddeev-LeVerrier algorithm
// Input:  mat[dim x dim] over GF(q)
// Output: poly[dim+1] where poly[k] is coefficient of x^k
//         poly[dim] = 1 (monic)
//         charpoly(x) = x^dim + poly[dim-1]*x^{dim-1} + ... + poly[0]
// =========================================================
static ORIGAMI_MAYBE_UNUSED void gf_charpoly(gf_t *poly, const gf_t *mat, int dim) {
    // Faddeev-LeVerrier: c_k = -1/k * tr(M * C_{k-1})
    // where C_0 = I, C_k = M*C_{k-1} + c_k*I
    gf_t C[dim * dim];
    gf_t MC[dim * dim];

    // C_0 = I
    memset(C, 0, dim * dim * sizeof(gf_t));
    for (int i = 0; i < dim; i++)
        C[i * dim + i] = 1;

    poly[dim] = 1; // monic

    for (int k = 1; k <= dim; k++) {
        // MC = mat * C
        gf_mat_mul_dim(MC, mat, C, dim, dim, dim);

        // trace of MC
        gf_t tr = 0;
        for (int i = 0; i < dim; i++)
            gf_set_add(&tr, MC[i * dim + i]);

        // c_k = -tr / k  (in GF(q))
        // For prime fields: -tr/k = (q - tr) * inv(k)
        // For GF(16) (char 2): -tr = tr, and we need inv(k mod 2)
#if ORIGAMI_q == 16
        // In characteristic 2, Faddeev-LeVerrier doesn't work for even k.
        // For small dim (l <= 4), we use a different approach below.
        // This branch should not be reached for GF(16).
        gf_t ck = 0;
        if ((k & 1) == 1) {
            ck = tr; // -1/k = 1/1 = 1 when k is odd in char 2
        }
#else
        gf_t k_gf = k % ORIGAMI_q;
        gf_t neg_tr = (ORIGAMI_q - tr) % ORIGAMI_q;
        gf_t ck = gf_mult(neg_tr, gf_inv(k_gf));
#endif
        poly[dim - k] = ck;

        // C_k = MC + c_k * I
        memcpy(C, MC, dim * dim * sizeof(gf_t));
        for (int i = 0; i < dim; i++)
            gf_set_add(&C[i * dim + i], ck);
    }
}

// =========================================================
// Characteristic polynomial via direct determinant: det(xI - A)
// Works for all fields including GF(16) (char 2).
// Evaluates det(xI - A) at x = 0, 1, ..., dim points,
// then uses Lagrange interpolation.
// =========================================================
static void gf_charpoly_interp(gf_t *poly, const gf_t *mat, int dim) {
    // We need dim+1 evaluation points since charpoly has degree dim.
    // Evaluate det(x*I - mat) for x = 0, 1, ..., dim
    int n_pts = dim + 1;
    gf_t vals[n_pts];
    gf_t points[n_pts];
    gf_t tmp[dim * dim];

    for (int p = 0; p < n_pts; p++) {
        points[p] = p % ORIGAMI_q;
        // Build x*I - mat. In characteristic two, subtraction is addition.
        for (int i = 0; i < dim; i++)
            for (int j = 0; j < dim; j++) {
#if ORIGAMI_q == 16
                gf_t entry = mat[i * dim + j];
#else
                gf_t entry = (ORIGAMI_q - mat[i * dim + j]) % ORIGAMI_q;
#endif
                if (i == j)
                    gf_set_add(&entry, points[p]);
                tmp[i * dim + j] = entry;
            }
        vals[p] = gf_mat_det_dim(tmp, dim);
    }

    // Lagrange interpolation to recover polynomial coefficients
    // poly[k] = coefficient of x^k
    memset(poly, 0, (dim + 1) * sizeof(gf_t));

    for (int i = 0; i < n_pts; i++) {
        // Compute Lagrange basis polynomial L_i(x) = product_{j!=i} (x - x_j)/(x_i - x_j)
        // and accumulate vals[i] * L_i into poly

        // First compute the denominator: product_{j!=i} (x_i - x_j)
        gf_t denom = 1;
        for (int j = 0; j < n_pts; j++) {
            if (j == i) continue;
            denom = gf_mult(denom, gf_sub(points[i], points[j]));
        }
        gf_t scale = gf_mult(vals[i], gf_inv(denom));

        // Compute L_i(x) as a polynomial by successive multiplication
        // L_i(x) = product_{j!=i} (x - x_j)
        gf_t basis[dim + 2]; // degree up to dim
        memset(basis, 0, (dim + 2) * sizeof(gf_t));
        basis[0] = 1; // start with constant 1
        int deg = 0;

        for (int j = 0; j < n_pts; j++) {
            if (j == i) continue;
            // Multiply basis by (x - x_j)
            gf_t neg_xj = gf_sub(0, points[j]);
            // Shift up (multiply by x) and add -x_j * old
            for (int k = deg + 1; k >= 1; k--)
                basis[k] = gf_add(basis[k - 1], gf_mult(neg_xj, basis[k]));
            basis[0] = gf_mult(neg_xj, basis[0]);
            deg++;
        }

        // Accumulate scale * basis into poly
        for (int k = 0; k <= dim; k++)
            gf_set_add(&poly[k], gf_mult(scale, basis[k]));
    }
}

// =========================================================
// Test if a monic polynomial over GF(q) is irreducible.
// poly[0..deg] with poly[deg] = 1 (monic).
// For small deg (<=4) and small q (<=31), exhaustive check suffices:
//   - degree 1: always irreducible
//   - degree 2,3: irreducible iff no root in F_q
//   - degree 4: irreducible iff no root in F_q AND no degree-2 factor
// =========================================================
static int gf_poly_is_irreducible(const gf_t *poly, int deg) {
    if (deg <= 0) return 0;
    if (deg == 1) return 1;

    // Check for roots: evaluate poly at each element of F_q
    for (int x = 0; x < ORIGAMI_q; x++) {
        gf_t val = 0;
        gf_t xpow = 1;
        for (int k = 0; k <= deg; k++) {
            gf_set_add(&val, gf_mult(poly[k], xpow));
            xpow = gf_mult(xpow, (gf_t)x);
        }
        if (val == 0) return 0; // has a root -> reducible
    }

    // For degree 2 or 3, no roots means irreducible
    if (deg <= 3) return 1;

    // For degree 4: also need to check there's no quadratic factor
    // Try all monic degree-2 polynomials x^2 + a*x + b
    // and check if they divide poly
    for (int a = 0; a < ORIGAMI_q; a++) {
        for (int b = 0; b < ORIGAMI_q; b++) {
            // Polynomial division: poly / (x^2 + a*x + b)
            // If remainder is zero, poly is reducible
            gf_t rem[deg + 1];
            memcpy(rem, poly, (deg + 1) * sizeof(gf_t));

            // Long division from highest degree down
            int ok = 1;
            for (int i = deg; i >= 2; i--) {
                gf_t lead = rem[i];
                if (lead == 0) continue;
                // Subtract lead * x^{i-2} * (x^2 + a*x + b)
                rem[i] = gf_sub(rem[i], lead); // should become 0
                rem[i - 1] = gf_sub(rem[i - 1], gf_mult(lead, (gf_t)a));
                rem[i - 2] = gf_sub(rem[i - 2], gf_mult(lead, (gf_t)b));
            }
            // Check if remainder (degree 0 and 1) is zero
            if (rem[0] == 0 && rem[1] == 0) {
                // Found a degree-2 factor -> reducible
                return 0;
            }
            (void)ok;
        }
    }

    return 1;
}

#endif // ORIGAMI_GF_H
