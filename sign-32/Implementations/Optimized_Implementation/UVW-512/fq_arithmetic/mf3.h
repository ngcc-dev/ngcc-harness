/******************************************************************************
WAVE -- Code-Based Digital Signature Scheme
Copyright (c) 2023 The Wave Team
contact: wave-contact@inria.fr

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
******************************************************************************/


#ifndef WAVE2_MF3_H
#define WAVE2_MF3_H

#include <stdbool.h>
#include <stdint.h>

#include "types_f3.h"
#include "vf3.h"

/**
 * Allocate a new ternary matrix. All entries will be set to 0.
 * @param nr_rows number of rows of the matrix
 * @param nr_cols number of columns of the matrix
 * @return a pointer to the new matrix
 */
mf3_e *mf3_alloc(const size_t nr_rows, const size_t nr_cols);

/**
 * Deallocate a ternary matrix.
 * @param m a valid pointer to a matrix (which will be freed)
 */
void mf3_free(mf3_e *m);

/**
 * Copy the entries from one matrix into another.
 * @param dest a valid pointer to the destination matrix (which will be
 * modified)
 * @param src a valid pointer to the source matrix
 */
void mf3_set(mf3_e *dest, const mf3_e *src);

/**
 * Allocate and fill a new copy of a matrix.
 * @param M a valid pointer to the matrix to be copied
 * @return a pointer to the new matrix (with the same dimensions and entries as
 * M)
 */
mf3_e *mf3_copy(const mf3_e *M);

/**
 * Set any extra entries (beyond the official size) of a matrix to 0.
 * @param M a valid pointer to a matrix
 */
void mf3_clean(mf3_e *M);

/**
 * Fill a matrix with random trits.
 * @param e a valid pointer to a matrix (must be already allocated)
 */
void mf3_random(mf3_e *e);

/**
 * Replace a matrix with its transpose.
 * @param m a valid pointer to a matrix (which will be freed and replaced with
 * the transpose)
 */
void mf3_transpose(mf3_e *m);

/**
 * Fill a matrix with the entries of the transpose of another matrix.
 * Assumes tr->n_row = m->n_col and tr->n_col = m->n_row
 * @param tr a valid pointer to a matrix that can hold the transpose
 * @param m  a valid pointer to the matrix to be transposed
 */
void mf3_transpose_and_copy(mf3_e *tr, const mf3_e *m);

/**
 * Compute a matrix-vector product: s^t = He^t.
 * Assumes s->size = H->n_row and e->size = H->n_col.
 * @param s a valid pointer to a vector to hold the result
 * @param H a valid pointer to a matrix
 * @param e a valid pointer to a vector
 */
void mf3_product_matrix_vector(vf3_e *s, const mf3_e *H, const vf3_e *e);

/**
 * Compute a vector-matrix product: y = xG.
 * Assumes y->size = G->n_col and x->size = G->n_row
 * @param y a valid pointer to a vector to hold the result
 * @param x a valid pointer to a vector
 * @param g a valid pointer to a matrix
 */
void mf3_product_vector_matrix(vf3_e *y, const vf3_e *x, const mf3_e *G);

/**
 * Extend a matrix downwards with rows of zeroes.
 * The matrix is reallocated, and the new rows are allocated and zeroed.
 * @param H a pointer to a valid matrix
 * @param nr_stack number of new zero rows to be added
 */
void mf3_stack_zeros(mf3_e *H, size_t nr_stack);

/**
 * Extend a matrix H downwards with a copy of the rows of another matrix M.
 * Assumes H->n_col = M->n_col.
 * H will be reallocated, and copies of the rows of M will be allocated.
 * @param H a valid pointer to the matrix to be extended
 * @param M a valid pointer to the matrix to be stacked under H
 */
void mf3_stack(mf3_e *H, const mf3_e *M);

/**
 * Check equality of (entries of) two matrices.
 * @param H a pointer to a valid matrix
 * @param M a pointer to a valid matrix
 * @return `true` if the matrices are equal, otherwise `false`.
 */
bool mf3_equal(const mf3_e *H, const mf3_e *M);

/**
 * Set all entries of a matrix to zero.
 * @param M a valid pointer to a matrix
 */
void mf3_set_to_zero(mf3_e *M);

/**
 * Set a matrix to identity matrix.
 * If M->n_row != M->n_col, only left-upper MIN(M->n_row, M->n_col) rows and columns are set.
 * @param M a valid pointer to a matrix
 */
void mf3_set_to_identity(mf3_e *M);

/**
 * Check if a matrix is a identity matrix.
 * @param M a valid pointer to a matrix
 * @return `true` if the matrix is a identity matrix, otherwise `false`.
 */
bool mf3_is_identity(mf3_e *M);

/**
 * Check if a matrix is a identity matrix.
 * If M->n_row != M->n_col, only left-upper MIN(M->n_row, M->n_col) rows and columns are checked.
 * @param M a valid pointer to a matrix
 * @return `true` if the matrix is a identity matrix, otherwise `false`.
 */
bool mf3_is_identity_loose(mf3_e *M);

/**
 * Print the contents of the vectors in a matrix.
 * @param M a valid pointer to a matrix
 */
void mf3_print(const mf3_e *M);

/**
 * Set the (i,j)-th entry of a matrix to a given trit value (represented as an
 * integer in {0,1,2}) Assumes 0 <= i < M->n_row and 0 <= j < M->n_col
 * @param M a valid pointer to a matrix
 * @param i an integer (row index)
 * @param j an integer (column index)
 * @param a an integer in {0,1,2} representing a single trit
 */
static inline void mf3_setcoeff(mf3_e *M, const size_t i, const size_t j, const uint8_t a) {
  vf3_set_coeff(j, &M->rows[i], a);
}

/**
 * Add a given trit value (represented as an integer in {0,1,2}) to the (i,j)-th
 * entry of a matrix. Assumes 0 <= i < M->n_row and 0 <= j < M->n_col
 * @param M a valid pointer to a matrix
 * @param i an integer (row index)
 * @param j an integer (column index)
 * @param a an integer in {0,1,2} representing a single trit
 */
static inline void mf3_addcoeff(mf3_e *M, const size_t i, const size_t j, const uint8_t a) {
  vf3_add_coeff(j, &M->rows[i], a);
}

/**
 * Get the (i,j)-th entry of a matrix, represented as an integer in {0,1,2}
 * Assumes 0 <= i < M->n_row and 0 <= j < M->n_col
 * @param M a valid pointer to a matrix
 * @param i an integer (row index)
 * @param j an integer (column index)
 * @return a an integer in {0,1,2} representing a single trit
 */
static inline uint8_t mf3_coeff(const mf3_e *M, const size_t i, const size_t j) {
  return vf3_get_element(j, &M->rows[i]);
}

/**
 * Compute the negative of a matrix: a = -b.
 * Assumes a->n_row = b->n_row and a->n_col = b->n_col
 * @param a a valid pointer to a matrix (which will be modified to hold the
 * result)
 * @param b a valid pointer to the matrix to be negated
 */
static inline void mf3_matrix_neg(mf3_e *a, const mf3_e *b) {
  for (size_t i = 0; i < b->n_row; i++) {
    vf3_vector_neg(&a->rows[i], &b->rows[i]);
  }
}

/**
 * Negate a matrix in-place.
 * @param b a valid pointer to a matrix (which will be modified to hold its own
 * negative)
 */
static inline void mf3_matrix_neg_inplace(mf3_e *b) {
  for (size_t i = 0; i < b->n_row; i++) {
    vf3_vector_neg_inplace(&b->rows[i]);
  }
}

/**
 * Compute the sum of matrices c = a + b.
 * Assumes a->n_row = b->n_row = c->n_row and a->n_col = b->n_col = c->n_col.
 * @param c a valid pointer to a matrix (which will be modified to hold the
 * result)
 * @param a a valid pointer to a matrix
 * @param b a valid pointer to a matrix
 */
static inline void mf3_matrix_add(mf3_e *c, const mf3_e *a, const mf3_e *b) {
  for (size_t i = 0; i < b->n_row; i++) {
    vf3_vector_add(&c->rows[i], &a->rows[i], &b->rows[i]);
  }
}

/**
 * Compute the sum of matrices a = a + b in-place (overwriting a with the
 * result). Assumes a->n_row = b->n_row and a->n_col = b->n_col.
 * @param a a valid pointer to a matrix (which will be modified)
 * @param b a valid pointer to a matrix
 */
static inline void mf3_matrix_add_inplace(mf3_e *a, const mf3_e *b) {
  for (size_t i = 0; i < b->n_row; i++) {
    vf3_vector_add_inplace(&a->rows[i], &b->rows[i]);
  }
}

/**
 * Compute the difference of two matrices: c = a - b.
 * Assumes a->n_row = b->n_row = c->n_row and a->n_col = b->n_col = c->n_col.
 * @param c a valid pointer to a matrix (which will be modified to hold the
 * result)
 * @param a a valid pointer to a matrix
 * @param b a valid pointer to a matrix
 */
static inline void mf3_vector_sub(mf3_e *c, const mf3_e *a, const mf3_e *b) {
  for (size_t i = 0; i < b->n_row; i++) {
    vf3_vector_sub(&c->rows[i], &a->rows[i], &b->rows[i]);
  }
}

/**
 * Subtract one matrix from another in-place: a = a - b.
 * Assumes a->n_row = b->n_row and a->n_col = b->n_col.
 * @param a a valid pointer to a matrix (which will be modified to hold the
 * result)
 * @param b a valid pointer to a matrix
 */
static inline void mf3_vector_sub_inplace(mf3_e *a, const mf3_e *b) {
  for (size_t i = 0; i < b->n_row; i++) {
    vf3_vector_sub_inplace(&a->rows[i], &b->rows[i]);
  }
}

/**
 * Fill a matrix with the entries provided in row-major order.
 * Assumes data length is m->n_row * m->n_col
 * @param m a valid pointer to a matrix
 * @param data Entries provided in row-major order
 */
static inline void mf3_set_row_slice(mf3_e *m, const uint8_t *data) {
  for (size_t i = 0; i < m->n_row; i++) {
    for (size_t j = 0; j < m->n_col; j++) {
      mf3_setcoeff(m, i, j, data[i * m->n_col + j]);
    }
  }
}

/**
 * Fill a matrix with the entries provided in column-major order.
 * Assumes data length is m->n_row * m->n_col
 * @param m a valid pointer to a matrix
 * @param data Entries provided in column-major order
 */
static inline void mf3_set_column_slice(mf3_e *m, const uint8_t *data) {
  for (size_t j = 0; j < m->n_col; j++) {
    for (size_t i = 0; i < m->n_row; i++) {
      mf3_setcoeff(m, i, j, data[j * m->n_row + i]);
    }
  }
}

/**
 * Swap two rows of a matrix.
 * @param m a valid pointer to a matrix
 * @param i the i-th and j-th row will be swapped
 * @param j the i-th and j-th row will be swapped
 */
static inline void mf3_swap_rows(mf3_e *m, const size_t i, const size_t j) {
  vf3_swap(&m->rows[i], &m->rows[j]);
}

/**
 * Read a ternary matrix from a file (in internal format)
 * @param m a valid pointer to a matrix
 * @param f a stream open for reading
 * @return 0 on failure, read size on success
 */
size_t mf3_read(mf3_e *m, FILE *f);

/**
 * Write a ternary matrix to a file (in internal format)
 * @param m a valid pointer to a matrix
 * @param f a stream open for writing
 * @return 0 on failure, written size on success
 */
size_t mf3_write(mf3_e *m, FILE *f);

#endif  // WAVE2_MF3_H
