#include "canonical.h"

#include <stdlib.h>
#include <string.h>

#include "field.h"
#include "matrixelim.h"
#include "matrixmod.h"
#include "triform.h"

static int vector_is_nonzero(
    const Fq *v,
    int n)
{
  for (int i = 0; i < n; i++)
    if (v[i] != 0)
      return 1;

  return 0;
}

static void vector_list_append(
    Fq *list,
    int index,
    const Fq *v,
    int n)
{
  memcpy(
      list + (size_t)index * (size_t)n,
      v,
      (size_t)n * sizeof(*v));
}

static void vector_list_to_matrix(
    Fq *A,
    const Fq *list,
    int n)
{
  pmod_mat_zero(A, n);

  for (int col = 0; col < n; col++)
    pmod_mat_set_col(
        A,
        col,
        list + (size_t)col * (size_t)n,
        n);
}

static void collect_normalization_anchors(
    Fq *anchors,
    const Fq *transformed,
    int n)
{
  const size_t a_off = 0;
  const size_t b_off = (size_t)(n - 2);
  const size_t c_off = 2u * (size_t)(n - 2);
  const size_t d_off = 3u * (size_t)(n - 2);

  const Fq *M1 = triform_slice_const(transformed, 0, n);
  const Fq *M2 = triform_slice_const(transformed, 1, n);
  const Fq *M5 = triform_slice_const(transformed, 4, n);

  for (int index = 2; index < n; index++)
  {
    const size_t offset = (size_t)(index - 2);

    anchors[a_off + offset] =
        M1[(size_t)index * (size_t)n + 1u];

    anchors[b_off + offset] =
        M1[(size_t)index];

    anchors[c_off + offset] =
        triform_slice_const(transformed, index, n)[1];
  }

  anchors[d_off + 0u] = M1[1];
  anchors[d_off + 1u] = M5[(size_t)n + 2u];
  anchors[d_off + 2u] = M2[2];
  anchors[d_off + 3u] = M2[(size_t)n];
}

static int compute_normalization_diagonals(
    Fq *f,
    Fq *g,
    Fq *h,
    const Fq *transformed,
    int n)
{
  const size_t anchor_count = 3u * (size_t)n - 2u;
  const size_t a_off = 0;
  const size_t b_off = (size_t)(n - 2);
  const size_t c_off = 2u * (size_t)(n - 2);
  const size_t d_off = 3u * (size_t)(n - 2);

  Fq anchors[anchor_count];
  Fq anchor_inverses[anchor_count];

  collect_normalization_anchors(anchors, transformed, n);

  if (GF_batch_inv(anchor_inverses, anchors, anchor_count) != 0)
    return -1;

  const Fq d1 = anchors[d_off + 0u];
  const Fq d2 = anchors[d_off + 1u];
  const Fq d3 = anchors[d_off + 2u];
  const Fq inv_d1 = anchor_inverses[d_off + 0u];
  const Fq inv_d2 = anchor_inverses[d_off + 1u];
  const Fq inv_d3 = anchor_inverses[d_off + 2u];
  const Fq inv_d4 = anchor_inverses[d_off + 3u];

  f[0] = 1;
  g[1] = 1;
  h[0] = inv_d1;

  for (int index = 2; index < n; index++)
  {
    const size_t offset = (size_t)(index - 2);
    const Fq inv_a = anchor_inverses[a_off + offset];
    const Fq inv_b = anchor_inverses[b_off + offset];
    const Fq inv_c = anchor_inverses[c_off + offset];

    f[index] = GF_mul(d1, inv_a);
    g[index] = GF_mul(d1, inv_b);
    h[index] = inv_c;
  }

  const Fq b3 = anchors[b_off + 0u];
  const Fq c5 = anchors[c_off + 2u];

  f[1] = GF_mul(b3, c5);
  f[1] = GF_mul(f[1], inv_d1);
  f[1] = GF_mul(f[1], inv_d2);

  h[1] = GF_mul(b3, inv_d1);
  h[1] = GF_mul(h[1], inv_d3);

  Fq inv_f2 = GF_mul(g[2], h[4]);
  inv_f2 = GF_mul(inv_f2, d2);

  Fq inv_h2 = GF_mul(g[2], d3);

  g[0] = GF_mul(inv_f2, inv_h2);
  g[0] = GF_mul(g[0], inv_d4);

  return 0;
}

static void apply_normalization(
    Fq *out,
    const Fq *transformed,
    const Fq *f,
    const Fq *g,
    const Fq *h,
    int n)
{
  const size_t slice_size = triform_slice_size(n);
  Fq scaled[slice_size];

  for (int slice = 0; slice < n; slice++)
  {
    const Fq *src = triform_slice_const(transformed, slice, n);
    Fq *dst = triform_slice(out, slice, n);

    pmod_mat_diag_scale(scaled, f, src, g, n);

    for (size_t pos = 0; pos < slice_size; pos++)
      dst[pos] = GF_mul(h[slice], scaled[pos]);
  }
}

int canonical_build_uvw_vartime(
    Fq *U,
    Fq *V,
    Fq *W,
    const Fq *M,
    const Fq *u1,
    int n)
{
  if (U == NULL ||
      V == NULL ||
      W == NULL ||
      M == NULL ||
      u1 == NULL)
    return -1;

  if (n < 1 || n > TRINE_n)
    return -1;

  if (!vector_is_nonzero(u1, n))
    return -1;

  Fq LU[n * n];
  Fq LV[n * n];
  Fq LW[n * n];
  Fq current_v[n];
  Fq current_w[n];
  Fq next_u[n];
  Fq next_v[n];
  Fq matrix_tmp[n * n];

  int u_count = 1;
  int v_count = 0;
  int w_count = 0;

  vector_list_append(LU, 0, u1, n);

  if (triform_phi_u_lker_corank1_vartime(
          current_v,
          M,
          u1,
          n) != 0)
    return -1;

  for (int i = 0; i < n; i++)
  {
    if (!pmod_vec_extends_independent_set_vartime(
            current_v,
            LV,
            v_count,
            n))
      return -1;

    vector_list_append(LV, v_count, current_v, n);
    v_count++;

    if (triform_phi_v_rker_corank1_vartime(
            current_w,
            M,
            current_v,
            n) != 0)
      return -1;

    if (!pmod_vec_extends_independent_set_vartime(
            current_w,
            LW,
            w_count,
            n))
      return -1;

    vector_list_append(LW, w_count, current_w, n);
    w_count++;

    if (i == n - 1)
      break;

    if (triform_matrix_at_w_lker_corank1_vartime(
            next_u,
            M,
            current_w,
            n) != 0)
      return -1;

    /*
     * This both verifies Phi_U(next_u) has corank one and caches
     * v_{i+1}, avoiding a second elimination in the next iteration.
     */
    if (triform_phi_u_lker_corank1_vartime(
            next_v,
            M,
            next_u,
            n) != 0)
      return -1;

    if (!pmod_vec_extends_independent_set_vartime(
            next_u,
            LU,
            u_count,
            n))
      return -1;

    vector_list_append(LU, u_count, next_u, n);
    u_count++;

    memcpy(current_v, next_v, (size_t)n * sizeof(*current_v));
  }

  if (u_count != n || v_count != n || w_count != n)
    return -1;

  vector_list_to_matrix(matrix_tmp, LU, n);
  if (!pmod_mat_is_invertible_vartime(matrix_tmp, n))
    return -1;

  vector_list_to_matrix(matrix_tmp, LV, n);
  if (!pmod_mat_is_invertible_vartime(matrix_tmp, n))
    return -1;

  vector_list_to_matrix(matrix_tmp, LW, n);
  if (!pmod_mat_is_invertible_vartime(matrix_tmp, n))
    return -1;

  vector_list_to_matrix(U, LU, n);
  vector_list_to_matrix(V, LV, n);
  vector_list_to_matrix(W, LW, n);

  return 0;
}

int canonical_diagonal_normalize_vartime(
    Fq *out,
    const Fq *M,
    const Fq *U,
    const Fq *V,
    const Fq *W,
    int n)
{
  if (out == NULL ||
      M == NULL ||
      U == NULL ||
      V == NULL ||
      W == NULL)
    return -1;

  if (n < 5 || n > TRINE_n)
    return -1;

  Fq *transformed = malloc(triform_element_count(n) * sizeof(*transformed));
  if (transformed == NULL)
    return -1;

  Fq f[n];
  Fq g[n];
  Fq h[n];
  int ret = -1;

  triform_action_pullback(transformed, M, U, V, W, n);

  if (compute_normalization_diagonals(f, g, h, transformed, n) != 0)
    goto cleanup;

  apply_normalization(out, transformed, f, g, h, n);
  ret = 0;

cleanup:
  free(transformed);
  return ret;
}

int canonical_form_vartime(
    Fq *out,
    const Fq *M,
    const Fq *u1,
    int n)
{
  if (out == NULL ||
      M == NULL ||
      u1 == NULL)
    return -1;

  if (n < 5 || n > TRINE_n)
    return -1;

  Fq U[n * n];
  Fq V[n * n];
  Fq W[n * n];

  if (canonical_build_uvw_vartime(U, V, W, M, u1, n) != 0)
    return -1;

  return canonical_diagonal_normalize_vartime(out, M, U, V, W, n);
}
