#include <stdint.h>
#include "inverse.h"
#include "params.h"
#include "reduce.h"

static inline int16_t fq_freeze_9(int32_t x)
{
  int32_t r;
  const int16_t half_q = (DTRU_Q - 1) >> 1;

  r = x - (int32_t)(((int64_t)x * BARRETT_V) >> 26) * DTRU_Q;
  if (r > half_q)
    r -= DTRU_Q;
  if (r > half_q)
    r -= DTRU_Q;
  if (r < -half_q)
    r += DTRU_Q;
  if (r < -half_q)
    r += DTRU_Q;

  return (int16_t)r;
}

static inline int16_t fq_add_9(int32_t a, int32_t b)
{
  return fq_freeze_9(a + b);
}

static inline int16_t fq_sub_9(int32_t a, int32_t b)
{
  return fq_freeze_9(a - b);
}

static inline int16_t fq_mul_9(int32_t a, int32_t b)
{
  return fq_freeze_9(a * b);
}

static inline int int16_nonzero_mask_9(int16_t x)
{
  uint16_t u = (uint16_t)x;
  uint32_t w = u;

  w = -w;
  w >>= 31;
  return -(int)w;
}

static inline int int16_negative_mask_9(int16_t x)
{
  return -((int)((uint16_t)x >> 15));
}

static inline int16_t fqinv_3457_9(int16_t a)
{
  int16_t t = 1;

  a = fqmul(a, 867);

#define FQINV_STEP(bit)            \
  do {                             \
    if (bit)                       \
      t = fqmul(t, a);             \
    a = fqmul(a, a);               \
  } while (0)

  FQINV_STEP(1);
  FQINV_STEP(1);
  FQINV_STEP(1);
  FQINV_STEP(1);
  FQINV_STEP(1);
  FQINV_STEP(1);
  FQINV_STEP(1);
  FQINV_STEP(0);
  FQINV_STEP(1);
  FQINV_STEP(0);
  FQINV_STEP(1);
  FQINV_STEP(1);

#undef FQINV_STEP

  return t;
}

#define SWAP_ENTRY(a, b, idx)        \
  do {                               \
    t = swap & ((a)[idx] ^ (b)[idx]);\
    (a)[idx] ^= t;                   \
    (b)[idx] ^= t;                   \
  } while (0)

int rq_inverse_euclid9_ref(int16_t finv[ROOT_DIMENSION], const int16_t f[ROOT_DIMENSION], int16_t zeta)
{
  int16_t Phi[ROOT_DIMENSION + 1] = {1, 0, 0, 0, 0, 0, 0, 0, 0, zeta};
  int16_t V[ROOT_DIMENSION + 1] = {0};
  int16_t S[ROOT_DIMENSION + 1] = {1, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  int16_t F[ROOT_DIMENSION + 1] = {f[8], f[7], f[6], f[5], f[4], f[3], f[2], f[1], f[0], 0};
  int16_t nextF[ROOT_DIMENSION + 1];
  int16_t nextS[ROOT_DIMENSION + 1];
  int i;
  int loop;
  int swap;
  int t;
  int Delta = 1;
  int32_t Phi0;
  int32_t F0;
  int16_t scale;

  for (loop = 0; loop < 2 * ROOT_DIMENSION - 1; ++loop)
  {
    V[9] = V[8];
    V[8] = V[7];
    V[7] = V[6];
    V[6] = V[5];
    V[5] = V[4];
    V[4] = V[3];
    V[3] = V[2];
    V[2] = V[1];
    V[1] = V[0];
    V[0] = 0;

    swap = int16_negative_mask_9((int16_t)-Delta) & int16_nonzero_mask_9(F[0]);

    SWAP_ENTRY(Phi, F, 0);
    SWAP_ENTRY(Phi, F, 1);
    SWAP_ENTRY(Phi, F, 2);
    SWAP_ENTRY(Phi, F, 3);
    SWAP_ENTRY(Phi, F, 4);
    SWAP_ENTRY(Phi, F, 5);
    SWAP_ENTRY(Phi, F, 6);
    SWAP_ENTRY(Phi, F, 7);
    SWAP_ENTRY(Phi, F, 8);
    SWAP_ENTRY(Phi, F, 9);

    SWAP_ENTRY(V, S, 0);
    SWAP_ENTRY(V, S, 1);
    SWAP_ENTRY(V, S, 2);
    SWAP_ENTRY(V, S, 3);
    SWAP_ENTRY(V, S, 4);
    SWAP_ENTRY(V, S, 5);
    SWAP_ENTRY(V, S, 6);
    SWAP_ENTRY(V, S, 7);
    SWAP_ENTRY(V, S, 8);
    SWAP_ENTRY(V, S, 9);

    Delta ^= swap & (Delta ^ -Delta);
    Delta++;

    Phi0 = Phi[0];
    F0 = F[0];

    nextF[0] = fq_freeze_9(Phi0 * F[1] - F0 * Phi[1]);
    nextF[1] = fq_freeze_9(Phi0 * F[2] - F0 * Phi[2]);
    nextF[2] = fq_freeze_9(Phi0 * F[3] - F0 * Phi[3]);
    nextF[3] = fq_freeze_9(Phi0 * F[4] - F0 * Phi[4]);
    nextF[4] = fq_freeze_9(Phi0 * F[5] - F0 * Phi[5]);
    nextF[5] = fq_freeze_9(Phi0 * F[6] - F0 * Phi[6]);
    nextF[6] = fq_freeze_9(Phi0 * F[7] - F0 * Phi[7]);
    nextF[7] = fq_freeze_9(Phi0 * F[8] - F0 * Phi[8]);
    nextF[8] = fq_freeze_9(Phi0 * F[9] - F0 * Phi[9]);
    nextF[9] = 0;

    nextS[0] = fq_freeze_9(Phi0 * S[0] - F0 * V[0]);
    nextS[1] = fq_freeze_9(Phi0 * S[1] - F0 * V[1]);
    nextS[2] = fq_freeze_9(Phi0 * S[2] - F0 * V[2]);
    nextS[3] = fq_freeze_9(Phi0 * S[3] - F0 * V[3]);
    nextS[4] = fq_freeze_9(Phi0 * S[4] - F0 * V[4]);
    nextS[5] = fq_freeze_9(Phi0 * S[5] - F0 * V[5]);
    nextS[6] = fq_freeze_9(Phi0 * S[6] - F0 * V[6]);
    nextS[7] = fq_freeze_9(Phi0 * S[7] - F0 * V[7]);
    nextS[8] = fq_freeze_9(Phi0 * S[8] - F0 * V[8]);
    nextS[9] = fq_freeze_9(Phi0 * S[9] - F0 * V[9]);

    Phi[0] = Phi[0];
    F[0] = nextF[0];
    F[1] = nextF[1];
    F[2] = nextF[2];
    F[3] = nextF[3];
    F[4] = nextF[4];
    F[5] = nextF[5];
    F[6] = nextF[6];
    F[7] = nextF[7];
    F[8] = nextF[8];
    F[9] = nextF[9];

    S[0] = nextS[0];
    S[1] = nextS[1];
    S[2] = nextS[2];
    S[3] = nextS[3];
    S[4] = nextS[4];
    S[5] = nextS[5];
    S[6] = nextS[6];
    S[7] = nextS[7];
    S[8] = nextS[8];
    S[9] = nextS[9];
  }

  scale = Phi[0];
  scale += (scale >> 15) & DTRU_Q;
  scale = fqinv_3457_9(scale);

  for (i = 0; i < ROOT_DIMENSION; ++i)
    finv[i] = fq_freeze_9(scale * (int32_t)V[ROOT_DIMENSION - 1 - i]);

  return int16_nonzero_mask_9((int16_t)Delta);
}

#undef SWAP_ENTRY

static inline void mul_y_3(int16_t c[3], const int16_t a[3], int16_t lambda)
{
  int16_t c0 = fq_mul_9(lambda, a[2]);

  c[2] = a[1];
  c[1] = a[0];
  c[0] = c0;
}

static inline void basemul3_mod_y3_zeta_c(int16_t c[3],
                                          const int16_t a[3],
                                          const int16_t b[3],
                                          int16_t lambda)
{
  int16_t d0 = fq_mul_9(a[0], b[0]);
  int16_t d1 = fq_add_9(fq_mul_9(a[0], b[1]), fq_mul_9(a[1], b[0]));
  int16_t d2 = fq_add_9(fq_add_9(fq_mul_9(a[0], b[2]), fq_mul_9(a[1], b[1])),
                        fq_mul_9(a[2], b[0]));
  int16_t d3 = fq_add_9(fq_mul_9(a[1], b[2]), fq_mul_9(a[2], b[1]));
  int16_t d4 = fq_mul_9(a[2], b[2]);

  c[0] = fq_add_9(d0, fq_mul_9(lambda, d3));
  c[1] = fq_add_9(d1, fq_mul_9(lambda, d4));
  c[2] = d2;
}

#if defined(__arm__) || defined(__thumb__)
extern void basemul3_mod_y3_zeta_asm(int16_t c[3],
                                      const int16_t a[3],
                                      const int16_t b[3],
                                      int16_t lambda);
#define basemul3_mod_y3_zeta basemul3_mod_y3_zeta_asm
#else
#define basemul3_mod_y3_zeta basemul3_mod_y3_zeta_c
#endif

static inline int inverse3_mod_y3_zeta(int16_t b[3],
                                       const int16_t a[3],
                                       int16_t lambda)
{
  int16_t adj[3];
  int16_t den;
  int16_t scale;
  int16_t a0a0 = fq_mul_9(a[0], a[0]);
  int16_t a1a1 = fq_mul_9(a[1], a[1]);
  int16_t a2a2 = fq_mul_9(a[2], a[2]);
  int16_t a0a1 = fq_mul_9(a[0], a[1]);
  int16_t a0a2 = fq_mul_9(a[0], a[2]);
  int16_t a1a2 = fq_mul_9(a[1], a[2]);

  adj[0] = fq_sub_9(a0a0, fq_mul_9(lambda, a1a2));
  adj[1] = fq_sub_9(fq_mul_9(lambda, a2a2), a0a1);
  adj[2] = fq_sub_9(a1a1, a0a2);

  den = fq_add_9(fq_mul_9(a[0], adj[0]),
                 fq_mul_9(lambda, fq_add_9(fq_mul_9(a[1], adj[2]),
                                           fq_mul_9(a[2], adj[1]))));
  if (den == 0)
  {
    b[0] = 0;
    b[1] = 0;
    b[2] = 0;
    return -1;
  }

  scale = den;
  scale += (scale >> 15) & DTRU_Q;
  scale = fqinv_3457_9(scale);

  b[0] = fq_mul_9(adj[0], scale);
  b[1] = fq_mul_9(adj[1], scale);
  b[2] = fq_mul_9(adj[2], scale);
  return 0;
}

int rq_inverse_asm_9(int16_t finv[ROOT_DIMENSION], const int16_t f[ROOT_DIMENSION], int16_t zeta)
{
  int16_t lambda = fq_freeze_9(-zeta);
  int16_t a0[3] = {f[0], f[3], f[6]};
  int16_t a1[3] = {f[1], f[4], f[7]};
  int16_t a2[3] = {f[2], f[5], f[8]};
  int16_t b0[3], b1[3], b2[3];
  int16_t den[3], den_inv[3];
  int16_t t0[3], t1[3], t2[3], t3[3];
  int i;

  basemul3_mod_y3_zeta(t0, a0, a0, lambda);
  basemul3_mod_y3_zeta(t1, a1, a2, lambda);
  mul_y_3(t2, t1, lambda);
  for (i = 0; i < 3; ++i)
    b0[i] = fq_sub_9(t0[i], t2[i]);

  basemul3_mod_y3_zeta(t0, a2, a2, lambda);
  mul_y_3(t2, t0, lambda);
  basemul3_mod_y3_zeta(t1, a0, a1, lambda);
  for (i = 0; i < 3; ++i)
    b1[i] = fq_sub_9(t2[i], t1[i]);

  basemul3_mod_y3_zeta(t0, a1, a1, lambda);
  basemul3_mod_y3_zeta(t1, a0, a2, lambda);
  for (i = 0; i < 3; ++i)
    b2[i] = fq_sub_9(t0[i], t1[i]);

  basemul3_mod_y3_zeta(den, a0, b0, lambda);
  basemul3_mod_y3_zeta(t0, a1, b2, lambda);
  basemul3_mod_y3_zeta(t1, a2, b1, lambda);
  for (i = 0; i < 3; ++i)
    t3[i] = fq_add_9(t0[i], t1[i]);
  mul_y_3(t2, t3, lambda);
  for (i = 0; i < 3; ++i)
    den[i] = fq_add_9(den[i], t2[i]);

  if (inverse3_mod_y3_zeta(den_inv, den, lambda) != 0)
  {
    for (i = 0; i < ROOT_DIMENSION; ++i)
      finv[i] = 0;
    return -1;
  }

  basemul3_mod_y3_zeta(t0, b0, den_inv, lambda);
  basemul3_mod_y3_zeta(t1, b1, den_inv, lambda);
  basemul3_mod_y3_zeta(t2, b2, den_inv, lambda);

  for (i = 0; i < 3; ++i)
  {
    finv[3 * i + 0] = t0[i];
    finv[3 * i + 1] = t1[i];
    finv[3 * i + 2] = t2[i];
  }

  return 0;
}
