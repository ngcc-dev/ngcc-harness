/* Viper opt-in wrapper for the TCHES 2021 ntt-polymul Saber AVX2 NTT backend.
 * Upstream attribution/license: see README.Viper.md and LICENSE.ntt-polymul.
 */

#include "viper_tches2021_ntt.h"
#include "poly.h"
#include "polyvec.h"
#include <stddef.h>
#include <stdint.h>
#if defined(VIPER_TCHES_NTT_STAGE_PROFILE) && (VIPER_TCHES_NTT_STAGE_PROFILE == 1)
#include <stdio.h>
#include <x86intrin.h>
#define VIPER_STAGE_T0(name) uint64_t name = __rdtsc()
#define VIPER_STAGE_PRINT(op, stage, start) \
  fprintf(stderr, "NTT_STAGE,%s,%s,%llu\n", (op), (stage), (unsigned long long)(__rdtsc() - (start)))
#else
#define VIPER_STAGE_T0(name) (void)0
#define VIPER_STAGE_PRINT(op, stage, start) (void)0
#endif

static void viper_to_tches_poly(poly *out, const vpoly in)
{
  for (size_t i = 0; i < VIPER_N; i++) {
    uint16_t x = in[i] & VIPER_Q_MASK;
    out->coeffs[i] = (int16_t)((x >= VIPER_Q / 2) ? (int)x - VIPER_Q : (int)x);
  }
}

static void tches_to_viper_poly(vpoly out, const poly *in)
{
  for (size_t i = 0; i < VIPER_N; i++) {
    out[i] = (uint16_t)in->coeffs[i] & VIPER_Q_MASK;
  }
}

static void viper_to_tches_polyvec(polyvec *out, const vpolyvec in)
{
  for (size_t i = 0; i < VIPER_K; i++) {
    viper_to_tches_poly(&out->vec[i], in[i]);
  }
}

static void tches_to_viper_polyvec(vpolyvec out, const polyvec *in)
{
  for (size_t i = 0; i < VIPER_K; i++) {
    tches_to_viper_poly(out[i], &in->vec[i]);
  }
}

static void viper_to_tches_matrix(polyvec out[VIPER_K], vpoly in[VIPER_K][VIPER_K])
{
  for (size_t i = 0; i < VIPER_K; i++) {
    for (size_t j = 0; j < VIPER_K; j++) {
      viper_to_tches_poly(&out[i].vec[j], in[i][j]);
    }
  }
}

void poly_mul_tches2021_ntt_avx(vpoly c, const vpoly a, const vpoly b)
{
  poly ta, tb, tc;
  VIPER_STAGE_T0(t0);
  viper_to_tches_poly(&ta, a);
  viper_to_tches_poly(&tb, b);
  VIPER_STAGE_PRINT("poly_mul", "wrapper_q_to_tches", t0);
  VIPER_STAGE_T0(t1);
  poly_mul(&tc, &ta, &tb);
  VIPER_STAGE_PRINT("poly_mul", "core_aux_ntt_crt", t1);
  VIPER_STAGE_T0(t2);
  tches_to_viper_poly(c, &tc);
  VIPER_STAGE_PRINT("poly_mul", "wrapper_tches_to_q", t2);
}

void matvec_tches2021_ntt_avx(vpolyvec out, vpoly A[VIPER_K][VIPER_K], const vpolyvec s)
{
  polyvec tA[VIPER_K];
  polyvec ts, tout;
  VIPER_STAGE_T0(t0);
  viper_to_tches_matrix(tA, A);
  VIPER_STAGE_PRINT("matvec", "wrapper_matrix_q_to_tches", t0);
  VIPER_STAGE_T0(t1);
  viper_to_tches_polyvec(&ts, s);
  VIPER_STAGE_PRINT("matvec", "wrapper_secret_q_to_tches", t1);
  VIPER_STAGE_T0(t2);
  polyvec_matrix_vector_mul(&tout, tA, &ts, 0);
  VIPER_STAGE_PRINT("matvec", "core_aux_ntt_crt", t2);
  VIPER_STAGE_T0(t3);
  tches_to_viper_polyvec(out, &tout);
  VIPER_STAGE_PRINT("matvec", "wrapper_tches_to_q", t3);
}

void matTvec_tches2021_ntt_avx(vpolyvec out, vpoly A[VIPER_K][VIPER_K], const vpolyvec s)
{
  polyvec tA[VIPER_K];
  polyvec ts, tout;
  VIPER_STAGE_T0(t0);
  viper_to_tches_matrix(tA, A);
  VIPER_STAGE_PRINT("matTvec", "wrapper_matrix_q_to_tches", t0);
  VIPER_STAGE_T0(t1);
  viper_to_tches_polyvec(&ts, s);
  VIPER_STAGE_PRINT("matTvec", "wrapper_secret_q_to_tches", t1);
  VIPER_STAGE_T0(t2);
  polyvec_matrix_vector_mul(&tout, tA, &ts, 1);
  VIPER_STAGE_PRINT("matTvec", "core_aux_ntt_crt", t2);
  VIPER_STAGE_T0(t3);
  tches_to_viper_polyvec(out, &tout);
  VIPER_STAGE_PRINT("matTvec", "wrapper_tches_to_q", t3);
}

void dot_tches2021_ntt_avx(vpoly out, const vpolyvec a, const vpolyvec b)
{
  polyvec ta, tb;
  poly tout;
  VIPER_STAGE_T0(t0);
  viper_to_tches_polyvec(&ta, a);
  VIPER_STAGE_PRINT("dot", "wrapper_a_q_to_tches", t0);
  VIPER_STAGE_T0(t1);
  viper_to_tches_polyvec(&tb, b);
  VIPER_STAGE_PRINT("dot", "wrapper_b_q_to_tches", t1);
  VIPER_STAGE_T0(t2);
  polyvec_iprod(&tout, &ta, &tb);
  VIPER_STAGE_PRINT("dot", "core_aux_ntt_crt", t2);
  VIPER_STAGE_T0(t3);
  tches_to_viper_poly(out, &tout);
  VIPER_STAGE_PRINT("dot", "wrapper_tches_to_q", t3);
}
