#include "triform.h"

#include "field.h"
#include "matrixelim.h"
#include "matrixmod.h"

void triform_matrix_at_w(
    Fq *out,
    const Fq *M,
    const Fq *w,
    int n)
{
  pmod_mat_linear_combination(out, M, w, n, n);
}

void triform_phi_u(
    Fq *phi_u,
    const Fq *M,
    const Fq *u,
    int n)
{
  Fq column[n];

  for (int slice = 0; slice < n; slice++)
  {
    const Fq *M_slice = triform_slice_const(M, slice, n);
    pmod_mat_transpose_vec_mul(column, M_slice, u, n);
    pmod_mat_set_col(phi_u, slice, column, n);
  }
}

void triform_phi_v(
    Fq *phi_v,
    const Fq *M,
    const Fq *v,
    int n)
{
  Fq column[n];

  for (int slice = 0; slice < n; slice++)
  {
    const Fq *M_slice = triform_slice_const(M, slice, n);
    pmod_mat_vec_mul(column, M_slice, v, n);
    pmod_mat_set_col(phi_v, slice, column, n);
  }
}

Fq triform_eval(
    const Fq *M,
    const Fq *u,
    const Fq *v,
    const Fq *w,
    int n)
{
  Fq Mv[n];
  Fq result = 0;

  for (int slice = 0; slice < n; slice++)
  {
    const Fq *M_slice = triform_slice_const(M, slice, n);
    Fq bilinear_value = 0;

    pmod_mat_vec_mul(Mv, M_slice, v, n);

    for (int row = 0; row < n; row++)
      bilinear_value = GF_add(
          bilinear_value,
          GF_mul(u[row], Mv[row]));

    result = GF_add(
        result,
        GF_mul(w[slice], bilinear_value));
  }

  return result;
}

int triform_phi_u_lker_corank1_vartime(
    Fq *v,
    const Fq *M,
    const Fq *u,
    int n)
{
  Fq phi_u[n * n];

  triform_phi_u(phi_u, M, u, n);

  return pmod_mat_left_kernel_corank1_vartime(v, phi_u, n);
}

int triform_phi_v_rker_corank1_vartime(
    Fq *w,
    const Fq *M,
    const Fq *v,
    int n)
{
  Fq phi_v[n * n];

  triform_phi_v(phi_v, M, v, n);

  return pmod_mat_right_kernel_corank1_vartime(w, phi_v, n);
}

int triform_matrix_at_w_lker_corank1_vartime(
    Fq *u_next,
    const Fq *M,
    const Fq *w,
    int n)
{
  Fq matrix_at_w[n * n];

  triform_matrix_at_w(matrix_at_w, M, w, n);

  return pmod_mat_left_kernel_corank1_vartime(u_next, matrix_at_w, n);
}

void triform_action_pullback(
    Fq *out,
    const Fq *M,
    const Fq *A,
    const Fq *B,
    const Fq *C,
    int n)
{
  Fq A_transpose[n * n];
  Fq coefficients[n];
  Fq combined[n * n];
  Fq left_product[n * n];

  pmod_mat_transpose(A_transpose, A, n);

  for (int out_slice = 0; out_slice < n; out_slice++)
  {
    for (int in_slice = 0; in_slice < n; in_slice++)
      coefficients[in_slice] = C[in_slice * n + out_slice];

    pmod_mat_linear_combination(combined, M, coefficients, n, n);
    pmod_mat_mul(left_product, A_transpose, combined, n);
    pmod_mat_mul(triform_slice(out, out_slice, n), left_product, B, n);
  }
}
