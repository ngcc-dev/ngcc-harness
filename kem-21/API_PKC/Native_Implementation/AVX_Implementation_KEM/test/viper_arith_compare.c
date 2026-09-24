/* MAMBA-Viper implementation and implementation support layer where applicable. */
#include "../viper.h"
#include "../viper_arith.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int fail(const char *name, int iter) { printf("FAIL %s iter=%d\n", name, iter); return 1; }

static uint32_t xs = 1;
static uint32_t xorshift32(void) { xs ^= xs << 13; xs ^= xs >> 17; xs ^= xs << 5; return xs; }
static void random_poly(vpoly a) { for (size_t i = 0; i < VIPER_N; i++) a[i] = (uint16_t)(xorshift32() & VIPER_Q_MASK); }
static uint16_t canon_i(int v) { return (uint16_t)v & VIPER_Q_MASK; }
static void random_short_poly(vpoly a) { for (size_t i = 0; i < VIPER_N; i++) a[i] = canon_i((int)(xorshift32() % (2 * VIPER_ETA_S + 1)) - VIPER_ETA_S); }
static void random_short_vec(vpolyvec v) { for (size_t i = 0; i < VIPER_K; i++) random_short_poly(v[i]); }
static void random_vec(vpolyvec v) { for (size_t i = 0; i < VIPER_K; i++) random_poly(v[i]); }
static void random_mat(vpoly A[VIPER_K][VIPER_K]) { for (size_t i = 0; i < VIPER_K; i++) for (size_t j = 0; j < VIPER_K; j++) random_poly(A[i][j]); }

int main(int argc, char **argv) {
  int poly_trials = argc > 1 ? atoi(argv[1]) : 100000;
  int mat_trials = argc > 2 ? atoi(argv[2]) : 256;
  vpoly a, b, refp, avxp;
  vpolyvec x, y, refr, avxr;
  vpoly A[VIPER_K][VIPER_K];
  for (int i = 0; i < poly_trials; i++) {
    random_poly(a); random_poly(b);
    poly_mul_basebackend_ref(refp, a, b);
    poly_mul_basebackend_avx(avxp, a, b);
    if (memcmp(refp, avxp, sizeof(refp))) return fail("poly_mul_basebackend_avx vs ref", i);
#if defined(VIPER_EXPERIMENTAL_TCHES2021_NTT) && (VIPER_EXPERIMENTAL_TCHES2021_NTT == 1)
    random_short_poly(b);
    poly_mul_basebackend_ref(refp, a, b);
    viper_poly_mul(avxp, a, b);
    if (memcmp(refp, avxp, sizeof(refp))) return fail("viper_poly_mul selected NTT dense-by-short vs ref", i);
#endif
  }
  for (int i = 0; i < mat_trials; i++) {
    random_mat(A); random_vec(x); random_vec(y);
    matvec_basebackend_ref(refr, A, x); matvec_basebackend_avx(avxr, A, x);
    if (memcmp(refr, avxr, sizeof(refr))) return fail("matvec_basebackend_avx vs ref", i);
#if defined(VIPER_EXPERIMENTAL_TCHES2021_NTT) && (VIPER_EXPERIMENTAL_TCHES2021_NTT == 1)
    random_short_vec(x);
    matvec_basebackend_ref(refr, A, x);
    viper_matvec(avxr, A, x);
    if (memcmp(refr, avxr, sizeof(refr))) return fail("viper_matvec selected NTT vs ref", i);
#endif
    matTvec_basebackend_ref(refr, A, x); matTvec_basebackend_avx(avxr, A, x);
    if (memcmp(refr, avxr, sizeof(refr))) return fail("matTvec_basebackend_avx vs ref", i);
#if defined(VIPER_EXPERIMENTAL_TCHES2021_NTT) && (VIPER_EXPERIMENTAL_TCHES2021_NTT == 1)
    matTvec_basebackend_ref(refr, A, x);
    viper_matTvec(avxr, A, x);
    if (memcmp(refr, avxr, sizeof(refr))) return fail("viper_matTvec selected NTT vs ref", i);
#endif
    dot_basebackend_ref(refp, x, y); dot_basebackend_avx(avxp, x, y);
    if (memcmp(refp, avxp, sizeof(refp))) return fail("dot_basebackend_avx vs ref", i);
#if defined(VIPER_EXPERIMENTAL_TCHES2021_NTT) && (VIPER_EXPERIMENTAL_TCHES2021_NTT == 1)
    dot_basebackend_ref(refp, y, x);
    viper_dot(avxp, y, x);
    if (memcmp(refp, avxp, sizeof(refp))) return fail("viper_dot selected NTT dense-by-short vs ref", i);
#endif
  }
  printf("%s avx arithmetic compare ok: poly=%d mat=%d\n", VIPER_ALGNAME, poly_trials, mat_trials);
  return 0;
}
