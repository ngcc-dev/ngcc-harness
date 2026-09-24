#ifndef VIPER_ARITH_H
#define VIPER_ARITH_H

/* MAMBA-Viper implementation and implementation support layer where applicable. */

#include "viper.h"
#include <stdio.h>

void poly_mul_basebackend_ref(vpoly c, const vpoly a, const vpoly b);
void matvec_basebackend_ref(vpolyvec out, vpoly A[VIPER_K][VIPER_K], const vpolyvec s);
void matTvec_basebackend_ref(vpolyvec out, vpoly A[VIPER_K][VIPER_K], const vpolyvec s);
void dot_basebackend_ref(vpoly out, const vpolyvec a, const vpolyvec b);
void poly_mul_basebackend_avx(vpoly c, const vpoly a, const vpoly b);
void matvec_basebackend_avx(vpolyvec out, vpoly A[VIPER_K][VIPER_K], const vpolyvec s);
void matTvec_basebackend_avx(vpolyvec out, vpoly A[VIPER_K][VIPER_K], const vpolyvec s);
void dot_basebackend_avx(vpoly out, const vpolyvec a, const vpolyvec b);

#ifdef VIPER_ARITH_AVX
#define viper_poly_mul poly_mul_basebackend_avx
#define viper_matvec matvec_basebackend_avx
#define viper_matTvec matTvec_basebackend_avx
#define viper_dot dot_basebackend_avx
#define VIPER_POLY_MUL_ROUTE "poly_mul_basebackend_avx"
#define VIPER_MATVEC_ROUTE "matvec_basebackend_avx"
#define VIPER_MATTVEC_ROUTE "matTvec_basebackend_avx"
#define VIPER_DOT_ROUTE "dot_basebackend_avx"
#define VIPER_ARITH_AVX_ENABLED 1
#else
#define viper_poly_mul poly_mul_basebackend_ref
#define viper_matvec matvec_basebackend_ref
#define viper_matTvec matTvec_basebackend_ref
#define viper_dot dot_basebackend_ref
#define VIPER_POLY_MUL_ROUTE "poly_mul_basebackend_ref"
#define VIPER_MATVEC_ROUTE "matvec_basebackend_ref"
#define VIPER_MATTVEC_ROUTE "matTvec_basebackend_ref"
#define VIPER_DOT_ROUTE "dot_basebackend_ref"
#define VIPER_ARITH_AVX_ENABLED 0
#endif

static inline void viper_backend_report(FILE *out)
{
  fprintf(out, "REPORT,VIPER_LEVEL,%d\n", VIPER_LEVEL);
  fprintf(out, "REPORT,VIPER_ARITH_AVX,%d\n", VIPER_ARITH_AVX_ENABLED);
  fprintf(out, "REPORT,VIPER_EXPERIMENTAL_TCHES2021_NTT,0\n");
  fprintf(out, "REPORT,VIPER_EXPERIMENTAL_TCHES2021_HYBRID,0\n");
  fprintf(out, "REPORT,VIPER_EXPERIMENTAL_TCHES2021_HYBRID_L3L5,0\n");
  fprintf(out, "REPORT,resolved_poly_mul_route,%s\n", VIPER_POLY_MUL_ROUTE);
  fprintf(out, "REPORT,resolved_matvec_route,%s\n", VIPER_MATVEC_ROUTE);
  fprintf(out, "REPORT,resolved_matTvec_route,%s\n", VIPER_MATTVEC_ROUTE);
  fprintf(out, "REPORT,resolved_dot_route,%s\n", VIPER_DOT_ROUTE);
}

#endif
