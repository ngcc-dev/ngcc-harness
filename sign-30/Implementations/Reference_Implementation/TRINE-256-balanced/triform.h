#ifndef TRIFORM_H
#define TRIFORM_H

#include <stddef.h>

#include "params.h"

/*
 * Storage:
 *   M[slice * n * n + row * n + col]
 *
 * Preconditions:
 *   1 <= n && n <= TRINE_n
 *   all pointers refer to sufficiently large valid buffers
 *   all field elements are canonical
 */

static inline size_t triform_slice_size(int n)
{
  return (size_t)n * (size_t)n;
}

static inline size_t triform_element_count(int n)
{
  return (size_t)n * (size_t)n * (size_t)n;
}

static inline size_t triform_index(
    int slice,
    int row,
    int col,
    int n)
{
  return (size_t)slice * triform_slice_size(n)
       + (size_t)row * (size_t)n
       + (size_t)col;
}

static inline Fq *triform_slice(
    Fq *M,
    int slice,
    int n)
{
  return M + (size_t)slice * triform_slice_size(n);
}

static inline const Fq *triform_slice_const(
    const Fq *M,
    int slice,
    int n)
{
  return M + (size_t)slice * triform_slice_size(n);
}

void triform_matrix_at_w(
    Fq *out,
    const Fq *M,
    const Fq *w,
    int n);

void triform_phi_u(
    Fq *phi_u,
    const Fq *M,
    const Fq *u,
    int n);

void triform_phi_v(
    Fq *phi_v,
    const Fq *M,
    const Fq *v,
    int n);

Fq triform_eval(
    const Fq *M,
    const Fq *u,
    const Fq *v,
    const Fq *w,
    int n);

int triform_phi_u_lker_corank1_vartime(
    Fq *v,
    const Fq *M,
    const Fq *u,
    int n);

int triform_phi_v_rker_corank1_vartime(
    Fq *w,
    const Fq *M,
    const Fq *v,
    int n);

int triform_matrix_at_w_lker_corank1_vartime(
    Fq *u_next,
    const Fq *M,
    const Fq *w,
    int n);

/*
 * New independent operator. It does not replace legacy phi().
 *
 *   phi_out(x, y, z) = phi_M(Ax, By, Cz)
 *
 * Therefore:
 *
 *   out_j = A^T * (sum_i C[i,j] M_i) * B.
 *
 * out must not overlap M, A, B, or C.
 */
void triform_action_pullback(
    Fq *out,
    const Fq *M,
    const Fq *A,
    const Fq *B,
    const Fq *C,
    int n);

#endif
