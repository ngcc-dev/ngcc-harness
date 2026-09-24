#ifndef MATRIXELIM_H
#define MATRIXELIM_H

#include "matrixmod.h"

int pmod_mat_rank_vartime(
    const Fq *A,
    int n);

int pmod_mat_is_invertible_vartime(
    const Fq *A,
    int n);

int pmod_mat_inv_vartime(
    Fq *A_inv,
    const Fq *A,
    int n);

int pmod_mat_right_kernel_corank1_vartime(
    Fq *kernel,
    const Fq *A,
    int n);

int pmod_mat_left_kernel_corank1_vartime(
    Fq *kernel,
    const Fq *A,
    int n);

int pmod_vec_in_span_vartime(
    const Fq *v,
    const Fq *basis,
    int basis_count,
    int n);

int pmod_vec_extends_independent_set_vartime(
    const Fq *v,
    const Fq *basis,
    int basis_count,
    int n);

#endif
