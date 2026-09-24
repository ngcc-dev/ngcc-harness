/* Arithmetic validation for opt-in TCHES 2021 NTT AVX2 backend. */
#include "../api.h"
#include "../rng.h"
#include "../viper.h"
#include "../viper_arith.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t xs = 1;
static uint32_t xorshift32(void) { xs ^= xs << 13; xs ^= xs >> 17; xs ^= xs << 5; return xs; }
static uint16_t canon(int64_t x) { return (uint16_t)x & VIPER_Q_MASK; }

static void oracle_mul(vpoly c, const vpoly a, const vpoly b)
{
  int64_t t[2 * VIPER_N] = {0};
  for (size_t i = 0; i < VIPER_N; i++) {
    int64_t ai = (a[i] & VIPER_Q_MASK);
    if (ai >= VIPER_Q / 2) ai -= VIPER_Q;
    for (size_t j = 0; j < VIPER_N; j++) {
      int64_t bj = (b[j] & VIPER_Q_MASK);
      if (bj >= VIPER_Q / 2) bj -= VIPER_Q;
      t[i + j] += ai * bj;
    }
  }
  for (size_t i = 0; i < VIPER_N; i++) c[i] = canon(t[i] - t[i + VIPER_N]);
}

static void random_dense(vpoly a)
{
  for (size_t i = 0; i < VIPER_N; i++) a[i] = (uint16_t)(xorshift32() & VIPER_Q_MASK);
}

static void random_short(vpoly a, int eta)
{
  for (size_t i = 0; i < VIPER_N; i++) {
    int v = (int)(xorshift32() % (unsigned)(2 * eta + 1)) - eta;
    a[i] = canon(v);
  }
}

static void random_vec_dense(vpolyvec v) { for (size_t i = 0; i < VIPER_K; i++) random_dense(v[i]); }
static void random_vec_short(vpolyvec v, int eta) { for (size_t i = 0; i < VIPER_K; i++) random_short(v[i], eta); }
static void random_mat(vpoly A[VIPER_K][VIPER_K]) { for (size_t i = 0; i < VIPER_K; i++) for (size_t j = 0; j < VIPER_K; j++) random_dense(A[i][j]); }

static int16_t centered_u16(uint16_t x)
{
  unsigned y = x & VIPER_QMASK;
  return (int16_t)((y >= VIPER_Q / 2) ? (int)y - VIPER_Q : (int)y);
}

static int cmp_poly(const char *name, const vpoly a, const vpoly b, int iter)
{
  if (memcmp(a, b, sizeof(vpoly)) == 0) return 0;
  for (size_t i = 0; i < VIPER_N; i++) {
    if (a[i] != b[i]) {
      uint16_t diff = (uint16_t)((a[i] - b[i]) & VIPER_QMASK);
      printf("FAIL %s profile=%d iter=%d coeff=%zu got=%u want=%u diff_mod_q=%u centered_diff=%d final_q=%d qmask=0x%x\n",
             name, VIPER_LEVEL, iter, i, a[i], b[i], diff, centered_u16(diff), VIPER_Q, VIPER_QMASK);
      return 1;
    }
  }
  return 1;
}

static int cmp_vec(const char *name, const vpolyvec a, const vpolyvec b, int iter)
{
  for (size_t i = 0; i < VIPER_K; i++) {
    if (cmp_poly(name, a[i], b[i], iter)) return 1;
  }
  return 0;
}

static int pke_kem_check(void)
{
  unsigned char pk[CRYPTO_PUBLICKEYBYTES], sk[CRYPTO_SECRETKEYBYTES];
  unsigned char ct[CRYPTO_CIPHERTEXTBYTES], m[VIPER_MSGBYTES] = {0}, out[VIPER_MSGBYTES];
  unsigned char ss1[CRYPTO_BYTES], ss2[CRYPTO_BYTES];
  unsigned char seed[32] = {3}, coins[VIPER_FALLBACK_KEY_BYTES + VIPER_MU_BYTES] = {4}, rho[32] = {5};
  viper_pke_keypair(pk, sk, seed, rho);
  viper_pke_enc(ct, pk, m, coins);
  viper_pke_dec(out, sk, ct);
  if (memcmp(out, m, sizeof(m)) != 0) return 1;
  if (crypto_kem_keypair(pk, sk) != 0) return 1;
  if (crypto_kem_enc(ct, ss1, pk) != 0) return 1;
  if (crypto_kem_dec(ss2, ct, sk) != 0 || memcmp(ss1, ss2, CRYPTO_BYTES) != 0) return 1;
  return 0;
}

int main(int argc, char **argv)
{
  int poly_trials = argc > 1 ? atoi(argv[1]) : 100000;
  int mat_trials = argc > 2 ? atoi(argv[2]) : 256;
  vpoly a, b, got, want;
  vpolyvec x, y, gotv, wantv;
  vpoly A[VIPER_K][VIPER_K];

#if !((defined(VIPER_EXPERIMENTAL_TCHES2021_NTT) && (VIPER_EXPERIMENTAL_TCHES2021_NTT == 1)) || \
      (defined(VIPER_EXPERIMENTAL_TCHES2021_HYBRID) && (VIPER_EXPERIMENTAL_TCHES2021_HYBRID == 1)) || \
      (defined(VIPER_EXPERIMENTAL_TCHES2021_HYBRID_L3L5) && (VIPER_EXPERIMENTAL_TCHES2021_HYBRID_L3L5 == 1)))
  printf("SKIP: rebuild with a TCHES2021 experimental backend macro\n");
  return 77;
#endif

  for (int i = 0; i < poly_trials; i++) {
    random_dense(a);
    random_short(b, VIPER_ETA_S);
    oracle_mul(want, a, b);
    poly_mul_tches2021_ntt_avx(got, a, b);
    if (cmp_poly("poly dense-by-short eta=2", got, want, i)) return 1;
    oracle_mul(want, b, a);
    poly_mul_tches2021_ntt_avx(got, b, a);
    if (cmp_poly("poly short-by-dense eta=2", got, want, i)) return 1;
    random_short(b, 3);
    oracle_mul(want, a, b);
    poly_mul_tches2021_ntt_avx(got, a, b);
    if (cmp_poly("poly dense-by-short eta=3", got, want, i)) return 1;
  }

  const int boundary_dense[] = {0, 1, -1, -(int)(VIPER_Q / 2), (int)(VIPER_Q / 2) - 1};
  const int boundary_short[] = {0, 1, -1, -(int)VIPER_ETA_S, (int)VIPER_ETA_S};
  for (size_t i = 0; i < VIPER_N; i++) {
    a[i] = canon(boundary_dense[i % (sizeof(boundary_dense) / sizeof(boundary_dense[0]))]);
    b[i] = canon(boundary_short[i % (sizeof(boundary_short) / sizeof(boundary_short[0]))]);
  }
  oracle_mul(want, a, b);
  poly_mul_tches2021_ntt_avx(got, a, b);
  if (cmp_poly("poly boundary", got, want, 0)) return 1;

  for (int i = 0; i < mat_trials; i++) {
    random_mat(A);
    random_vec_short(x, VIPER_ETA_S);
    random_vec_dense(y);
    matvec_basebackend_avx(wantv, A, x);
    matvec_tches2021_ntt_avx(gotv, A, x);
    if (cmp_vec("matvec NTT vs current", gotv, wantv, i)) return 1;
    matTvec_basebackend_avx(wantv, A, x);
    matTvec_tches2021_ntt_avx(gotv, A, x);
    if (cmp_vec("matTvec NTT vs current", gotv, wantv, i)) return 1;
    dot_basebackend_avx(want, y, x);
    dot_tches2021_ntt_avx(got, y, x);
    if (cmp_poly("dot dense-by-short NTT vs current", got, want, i)) return 1;
    dot_basebackend_avx(want, x, y);
    dot_tches2021_ntt_avx(got, x, y);
    if (cmp_poly("dot short-by-dense NTT vs current", got, want, i)) return 1;

    poly_mul_basebackend_avx(want, a, x[0]);
    viper_poly_mul(got, a, x[0]);
    if (cmp_poly("selected poly_mul vs current", got, want, i)) return 1;
    matvec_basebackend_avx(wantv, A, x);
    viper_matvec(gotv, A, x);
    if (cmp_vec("selected matvec vs current", gotv, wantv, i)) return 1;
    matTvec_basebackend_avx(wantv, A, x);
    viper_matTvec(gotv, A, x);
    if (cmp_vec("selected matTvec vs current", gotv, wantv, i)) return 1;
    dot_basebackend_avx(want, y, x);
    viper_dot(got, y, x);
    if (cmp_poly("selected dot vs current", got, want, i)) return 1;
  }

  if (pke_kem_check()) {
    printf("FAIL PKE/KEM correctness\n");
    return 1;
  }
#if defined(VIPER_EXPERIMENTAL_TCHES2021_HYBRID) && (VIPER_EXPERIMENTAL_TCHES2021_HYBRID == 1)
  printf("%s TCHES2021 HYBRID validation ok: poly=%d mat=%d\n", VIPER_ALGNAME, poly_trials, mat_trials);
#elif defined(VIPER_EXPERIMENTAL_TCHES2021_HYBRID_L3L5) && (VIPER_EXPERIMENTAL_TCHES2021_HYBRID_L3L5 == 1)
  printf("%s TCHES2021 HYBRID_L3L5 validation ok: poly=%d mat=%d\n", VIPER_ALGNAME, poly_trials, mat_trials);
#else
  printf("%s TCHES2021 NTT validation ok: poly=%d mat=%d crt_bound=%lld crt_half=41296896 centered_range=[%d,%d]\n", VIPER_ALGNAME, poly_trials, mat_trials, (long long)VIPER_K * VIPER_N * (VIPER_Q / 2) * VIPER_ETA_S, -(VIPER_Q / 2), VIPER_Q / 2 - 1);
#endif
  return 0;
}
