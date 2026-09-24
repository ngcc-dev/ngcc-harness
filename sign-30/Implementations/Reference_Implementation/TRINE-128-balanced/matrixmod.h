#ifndef MATRIXMOD_H
#define MATRIXMOD_H

#include <stdio.h>
#include <stdint.h>

#include "field.h"

#define pmod_mat_entry(M, M_r, M_c, r, c) M[(M_c)*(r)+(c)]

#define pmod_mat_set_entry(M, M_r, M_c, r, c, v) (M[(M_c)*(r)+(c)] = v)

void pmod_mat_print(const Fq *M, int M_r, int M_c);
void pmod_mat_fprint(FILE *stream, const Fq *M, int M_r, int M_c);

void pmod_mat_zero(Fq *A, int n);
void pmod_mat_copy(Fq *dst, const Fq *src, int n);
void pmod_mat_identity(Fq *A, int n);

void pmod_mat_transpose(
    Fq *AT,
    const Fq *A,
    int n);

void pmod_mat_mul(
    Fq *C,
    const Fq *A,
    const Fq *B,
    int n);

void pmod_mat_mul_rect(
    Fq *C,
    int C_r,
    int C_c,
    const Fq *A,
    int A_r,
    int A_c,
    const Fq *B,
    int B_r,
    int B_c);

void pmod_mat_vec_mul(
    Fq *out,
    const Fq *A,
    const Fq *v,
    int n);

void pmod_mat_transpose_vec_mul(
    Fq *out,
    const Fq *A,
    const Fq *v,
    int n);

void pmod_mat_set_col(
    Fq *A,
    int col,
    const Fq *v,
    int n);

void pmod_mat_get_col(
    Fq *v,
    const Fq *A,
    int col,
    int n);

void pmod_mat_linear_combination(
    Fq *out,
    const Fq *matrices,
    const Fq *coeffs,
    int count,
    int n);

void pmod_mat_diag_scale(
    Fq *out,
    const Fq *left_diag,
    const Fq *A,
    const Fq *right_diag,
    int n);

/*
 * Legacy MEDS systematic-form helpers.
 * Do not use for rank or kernel computation.
 */
int pmod_mat_syst_ct(Fq *M, int M_r, int M_c);
int pmod_mat_row_echelon_ct(Fq *M, int M_r, int M_c);
int pmod_mat_back_substitution_ct(Fq *M, int M_r, int M_c);

int pmod_mat_inv(Fq *B, const Fq *A, int A_r, int A_c);

#endif
