#include "poly.h"

void poly_ntt32(poly *f, int32_t prime, int32_t primeinv, int32_t fp_zetas[RRLWR_N]) {
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
}

void poly_invntt32(poly *f, int32_t prime, int32_t primeinv, int32_t finalconst, int32_t fp_zetas[RRLWR_N]) {
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
}

void poly_basemul32(poly *r, poly *f, poly *g, int32_t prime, int32_t primeinv) {
  for(unsigned int i = 0; i < RRLWR_N; i++) {
    r->coeffs[i] = montgomery_mul32(f->coeffs[i], g->coeffs[i], prime, primeinv);
  }
}

void poly_add32(poly *r, poly *f, poly *g, int32_t prime) {
  for(unsigned int i = 0; i < RRLWR_N; i++) {
    r->coeffs[i] = add32(f->coeffs[i], g->coeffs[i], prime);
  }
}

void poly_add(poly *r, poly *f, poly *g) {
  for(unsigned int i = 0; i < RRLWR_N; i++) {
    r->coeffs[i] = f->coeffs[i] + g->coeffs[i];
  }
}

void poly_sub32(poly *r, poly *f, poly *g, int32_t prime) {
  for(unsigned int i = 0; i < RRLWR_N; i++) {
    r->coeffs[i] = sub32(f->coeffs[i], g->coeffs[i], prime);
  }
}

void poly_sub(poly *r, poly *f, poly *g) {
  for(unsigned int i = 0; i < RRLWR_N; i++) {
    r->coeffs[i] = f->coeffs[i] - g->coeffs[i];
  }
}

/// @brief Reduce the input modulo 2**d into signed interval [-2**d, 2**d-1]
void poly_reduce_pow2(poly *r, poly *f, int32_t d) {
  int32_t pow2div2 = (int32_t)1 << (d-1); // 2^d/2
  int32_t pow2 = (int32_t)1 << d;         // 2^d
  for(unsigned int i = 0; i < RRLWR_N; i++) {
    r->coeffs[i] = ((f->coeffs[i] + pow2div2) & (pow2-1)) - pow2div2;
  }
}

/// @brief Reduce the input modulo prime into signed interval [-(prime-1)/2, (prime-1)/2-1]
void poly_conditional_final_reduce32(poly *r, int32_t prime) {
  for(unsigned int i = 0; i < RRLWR_N; i++) {
    r->coeffs[i] = conditional_final_reduce32(r->coeffs[i], prime);
  }
}

void poly_round_xtoy(poly *r, const poly *f, int32_t x, int32_t y) {
  for(unsigned int i = 0; i < RRLWR_N; i++) {
    r->coeffs[i] = f->coeffs[i] + ((int32_t)1 << (x-(y+1))); // Add constant x/(2*y)
    r->coeffs[i] >>= (x-y);                                  // Divide by x/y and floor
    r->coeffs[i] &= ((int32_t)1 << y)-1;                     // Reduce mod y
  }
}

void poly_compress(poly *r, int32_t x) {
  for(unsigned int i = 0; i < RRLWR_N; i++) {
    r->coeffs[i] >>= x;
  }
}

void poly_decompress(poly *r, int32_t x) {
  for(unsigned int i = 0; i < RRLWR_N; i++) {
    r->coeffs[i] <<= x;
  }
}
