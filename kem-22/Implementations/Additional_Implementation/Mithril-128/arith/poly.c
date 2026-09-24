#include "poly.h"
#ifndef RRLWR_DISABLE_NTT_AVX
#include "ntt.h"
#endif

void poly_ntt32(poly *f, int32_t prime, int32_t primeinv, int32_t fp_zetas[RRLWR_N]) {
#ifndef RRLWR_DISABLE_NTT_AVX
  (void)primeinv;
  (void)fp_zetas;
  ntt_avx(f->coeffs, prime, RRLWR_KEM_NTT_AVX_PARAMS);
#else
  unsigned int len, start, j, k;
  int32_t t, fp_zeta;
  int32_t *fc = f->coeffs;

  k = 1;
  for(len = (RRLWR_N >> 1); len >= 1; len >>= 1) {
    for(start = 0; start < RRLWR_N; start = j + len) {
      fp_zeta = fp_zetas[k++];
      for(j = start; j < start + len; j++) {
        t = montgomery_mul32(fp_zeta, fc[j + len], prime, primeinv);
        fc[j] = conditional_reduce32(fc[j], prime); // Reduce back to [-p, p]
        fc[j + len] = fc[j] - t;
        fc[j] = fc[j] + t;
      }
    }
  }

  for(j = 0; j < RRLWR_N; j++) {
      fc[j] = conditional_reduce32(fc[j], prime); // Reduce back to [-p, p]
  }
#endif
}

void poly_invntt32(poly *f, int32_t prime, int32_t primeinv, int32_t finalconst, int32_t fp_zetas[RRLWR_N]) {
#ifndef RRLWR_DISABLE_NTT_AVX
  (void)primeinv;
  (void)finalconst;
  (void)fp_zetas;
  intt_avx(f->coeffs, prime, RRLWR_KEM_INTT_FUSED_AVX_PARAMS);
#else
  unsigned int start, len, j, k;
  int32_t t, zeta;
  int32_t *fc = f->coeffs;

  k = RRLWR_N-1;
  for(len = 1; len <= (RRLWR_N >> 1); len <<= 1) {
    for(start = 0; start < RRLWR_N; start = j + len) {
      zeta = fp_zetas[k--];
      for(j = start; j < start + len; j++) {
        t = fc[j];
        fc[j] = t + fc[j + len];
        fc[j + len] = fc[j + len] - t;
        fc[j + len] = montgomery_mul32(zeta, fc[j + len], prime, primeinv);
        fc[j] = conditional_reduce32(fc[j], prime); // Reduce back to [-p, p]
      }
    }
  }

  for(j = 0; j < RRLWR_N; j++) {
    fc[j] = montgomery_mul32(fc[j], finalconst, prime, primeinv);
  }
#endif
}
