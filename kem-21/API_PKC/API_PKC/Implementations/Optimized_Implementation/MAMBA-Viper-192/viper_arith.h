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
void poly_mul_tches2021_ntt_avx(vpoly c, const vpoly a, const vpoly b);
void matvec_tches2021_ntt_avx(vpolyvec out, vpoly A[VIPER_K][VIPER_K], const vpolyvec s);
void matTvec_tches2021_ntt_avx(vpolyvec out, vpoly A[VIPER_K][VIPER_K], const vpolyvec s);
void dot_tches2021_ntt_avx(vpoly out, const vpolyvec a, const vpolyvec b);

#if defined(VIPER_EXPERIMENTAL_TCHES2021_NTT) && (VIPER_EXPERIMENTAL_TCHES2021_NTT == 1)
#define VIPER_TCHES2021_NTT_ENABLED 1
#else
#define VIPER_TCHES2021_NTT_ENABLED 0
#endif
#if defined(VIPER_EXPERIMENTAL_TCHES2021_HYBRID) && (VIPER_EXPERIMENTAL_TCHES2021_HYBRID == 1)
#define VIPER_TCHES2021_HYBRID_ENABLED 1
#else
#define VIPER_TCHES2021_HYBRID_ENABLED 0
#endif
#if defined(VIPER_EXPERIMENTAL_TCHES2021_HYBRID_L3L5) && (VIPER_EXPERIMENTAL_TCHES2021_HYBRID_L3L5 == 1)
#define VIPER_TCHES2021_HYBRID_L3L5_ENABLED 1
#else
#define VIPER_TCHES2021_HYBRID_L3L5_ENABLED 0
#endif

#if (VIPER_TCHES2021_NTT_ENABLED + VIPER_TCHES2021_HYBRID_ENABLED + VIPER_TCHES2021_HYBRID_L3L5_ENABLED) > 1
#error "Select at most one TCHES2021 experimental backend: NTT, HYBRID, or HYBRID_L3L5"
#endif

#if defined(VIPER_ARITH_AVX) && VIPER_TCHES2021_NTT_ENABLED
#define viper_poly_mul poly_mul_tches2021_ntt_avx
#define viper_matvec matvec_tches2021_ntt_avx
#define viper_matTvec matTvec_tches2021_ntt_avx
#define viper_dot dot_tches2021_ntt_avx
#define VIPER_POLY_MUL_ROUTE "poly_mul_tches2021_ntt_avx"
#define VIPER_MATVEC_ROUTE "matvec_tches2021_ntt_avx"
#define VIPER_MATTVEC_ROUTE "matTvec_tches2021_ntt_avx"
#define VIPER_DOT_ROUTE "dot_tches2021_ntt_avx"
#elif defined(VIPER_ARITH_AVX) && VIPER_TCHES2021_HYBRID_ENABLED
#define viper_poly_mul poly_mul_basebackend_avx
#define viper_matvec matvec_tches2021_ntt_avx
#define viper_matTvec matTvec_tches2021_ntt_avx
#define viper_dot dot_basebackend_avx
#define VIPER_POLY_MUL_ROUTE "poly_mul_basebackend_avx"
#define VIPER_MATVEC_ROUTE "matvec_tches2021_ntt_avx"
#define VIPER_MATTVEC_ROUTE "matTvec_tches2021_ntt_avx"
#define VIPER_DOT_ROUTE "dot_basebackend_avx"
#elif defined(VIPER_ARITH_AVX) && VIPER_TCHES2021_HYBRID_L3L5_ENABLED && (VIPER_LEVEL == 192 || VIPER_LEVEL == 256)
#define viper_poly_mul poly_mul_basebackend_avx
#define viper_matvec matvec_tches2021_ntt_avx
#define viper_matTvec matTvec_tches2021_ntt_avx
#define viper_dot dot_basebackend_avx
#define VIPER_POLY_MUL_ROUTE "poly_mul_basebackend_avx"
#define VIPER_MATVEC_ROUTE "matvec_tches2021_ntt_avx"
#define VIPER_MATTVEC_ROUTE "matTvec_tches2021_ntt_avx"
#define VIPER_DOT_ROUTE "dot_basebackend_avx"
#elif defined(VIPER_ARITH_AVX)
#define viper_poly_mul poly_mul_basebackend_avx
#define viper_matvec matvec_basebackend_avx
#define viper_matTvec matTvec_basebackend_avx
#define viper_dot dot_basebackend_avx
#define VIPER_POLY_MUL_ROUTE "poly_mul_basebackend_avx"
#define VIPER_MATVEC_ROUTE "matvec_basebackend_avx"
#define VIPER_MATTVEC_ROUTE "matTvec_basebackend_avx"
#define VIPER_DOT_ROUTE "dot_basebackend_avx"
#else
#define viper_poly_mul poly_mul_basebackend_ref
#define viper_matvec matvec_basebackend_ref
#define viper_matTvec matTvec_basebackend_ref
#define viper_dot dot_basebackend_ref
#define VIPER_POLY_MUL_ROUTE "poly_mul_basebackend_ref"
#define VIPER_MATVEC_ROUTE "matvec_basebackend_ref"
#define VIPER_MATTVEC_ROUTE "matTvec_basebackend_ref"
#define VIPER_DOT_ROUTE "dot_basebackend_ref"
#endif

#if defined(VIPER_ARITH_AVX)
#define VIPER_ARITH_AVX_ENABLED 1
#else
#define VIPER_ARITH_AVX_ENABLED 0
#endif

static inline void viper_backend_report(FILE *out)
{
  fprintf(out, "REPORT,VIPER_LEVEL,%d\n", VIPER_LEVEL);
  fprintf(out, "REPORT,VIPER_ARITH_AVX,%d\n", VIPER_ARITH_AVX_ENABLED);
  fprintf(out, "REPORT,VIPER_EXPERIMENTAL_TCHES2021_NTT,%d\n", VIPER_TCHES2021_NTT_ENABLED);
  fprintf(out, "REPORT,VIPER_EXPERIMENTAL_TCHES2021_HYBRID,%d\n", VIPER_TCHES2021_HYBRID_ENABLED);
  fprintf(out, "REPORT,VIPER_EXPERIMENTAL_TCHES2021_HYBRID_L3L5,%d\n", VIPER_TCHES2021_HYBRID_L3L5_ENABLED);
  fprintf(out, "REPORT,resolved_poly_mul_route,%s\n", VIPER_POLY_MUL_ROUTE);
  fprintf(out, "REPORT,resolved_matvec_route,%s\n", VIPER_MATVEC_ROUTE);
  fprintf(out, "REPORT,resolved_matTvec_route,%s\n", VIPER_MATTVEC_ROUTE);
  fprintf(out, "REPORT,resolved_dot_route,%s\n", VIPER_DOT_ROUTE);
}

#endif
