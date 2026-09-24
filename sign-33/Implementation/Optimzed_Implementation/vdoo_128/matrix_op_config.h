#ifndef MATRIX_OP_CONFIG_H
#define MATRIX_OP_CONFIG_H

#include "vdoo_config.h"
#include "parallel_matrix_op.h"

#if VDOO_Q == 16
#define batch_trimat_madd batch_trimat_madd_gf16
#define batch_trimatTr_madd batch_trimatTr_madd_gf16
#define batch_2trimat_madd batch_2trimat_madd_gf16
#define batch_matTr_madd batch_matTr_madd_gf16
#define batch_bmatTr_madd batch_bmatTr_madd_gf16
#define batch_mat_madd batch_mat_madd_gf16
#define batch_quad_trimat_eval batch_quad_trimat_eval_gf16
#define batch_quad_recmat_eval batch_quad_recmat_eval_gf16
#define batch_S_FT_madd batch_S_FT_madd_gf16
#else
#define batch_trimat_madd batch_trimat_madd_gf256
#define batch_trimatTr_madd batch_trimatTr_madd_gf256
#define batch_2trimat_madd batch_2trimat_madd_gf256
#define batch_matTr_madd batch_matTr_madd_gf256
#define batch_bmatTr_madd batch_bmatTr_madd_gf256
#define batch_mat_madd batch_mat_madd_gf256
#define batch_quad_trimat_eval batch_quad_trimat_eval_gf256
#define batch_quad_recmat_eval batch_quad_recmat_eval_gf256
#define batch_S_FT_madd batch_S_FT_madd_gf256
#endif

#endif /* !MATRIX_OP_CONFIG_H */