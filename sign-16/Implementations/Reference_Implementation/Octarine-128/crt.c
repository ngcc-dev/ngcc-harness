#include "crt.h"

int32_t rrlwr_sign_zetas1[RRLWR_N] = RRLWR_SIGN_ZETAS1;
int32_t rrlwr_sign_zetas2[RRLWR_N] = RRLWR_SIGN_ZETAS2;

/// @brief Conditional reduction based on the sign bit of r after an addition or subtraction
/// @param[in] r in [-2*m, 2*m]
/// @return r mod p in [-m, m]
static int64_t conditional_reduce64(int64_t r, int64_t modulus) {
  int64_t sign_mask = -((r >> 63) & 1);                // -1 if r < 0,  0 if r >= 0
  int64_t shift = ((2*modulus) & sign_mask) - modulus; //  m if r < 0, -m if r >= 0
  r += shift;                                          // Reduced to interval [-m, m]

  return r;
}

/// @brief Conditional reduction based on the sign bit of r to a unique representation
/// @param[in] r in [-p+1, p]
/// @return r mod p in [-(p-1)/2+1, (p-1)/2]
static int64_t conditional_final_reduce64(int64_t r, int64_t modulus) {
  int64_t offset = (modulus-1) >> 1;                                      // (p-1)/2
  int64_t sign_mask_neg = -(((r+(offset-1)) >> 63) & 1);                  // -1 if r < -(p-1)/2+1,  0 if r >= -(p-1)/2+1
  int64_t sign_mask_pos = -(((r-(offset+1)) >> 63) & 1);                  // -1 if r <  (p-1)/2+1,  0 if r >=  (p-1)/2+1
  int64_t shift = (modulus & sign_mask_neg) - (modulus & ~sign_mask_pos); //  p if r < -(p-1)/2+1, -p if r >=  (p-1)/2+1, 0 otherwise
  r += shift;

  return r;
}

/// @brief Compute [ a*P mod (P1*P2) ], where 
///        * a is the size of Pi, 
///        * P = Pi*(Pi^-1 mod Pj), for i,j in {1,2}, 
///        * R = 2^64, 
///        * Th = floor(P*R/(p1*p2)) >> 32, 
///        * P1P2 = P1*P2.
///        The output is in the signed interval [-P1*P2, P1*P2], which requires a conditional subtraction to fully reduce
static int64_t crt_mulconst(int32_t a, int64_t P, uint32_t Th, int64_t p1p2) {
  int32_t v;
  int64_t v64, aP1;

  v = ((int64_t)a*Th) >> 32; // Take higher bits of 32-bit multiplication
  aP1 = a*P;                 // a*P1
  v64 = (int64_t)v*p1p2;     // v*(p1*p2)

  return aP1 - v64;
}

/// @brief Compute the unique integer C mod P1*P2 such that C = A mod P1 and C = B mod P2
///        The value of C lies in the signed interval [-P1*P2/2, P1*P2-1]
int64_t crt(int32_t a, int32_t b) {
  int64_t A, B, C;

  A = crt_mulconst(a, RRLWR_SIGN_P2INV_MODP1, RRLWR_SIGN_T1H, RRLWR_SIGN_P1P2);
  B = crt_mulconst(b, RRLWR_SIGN_P1INV_MODP2, RRLWR_SIGN_T2H, RRLWR_SIGN_P1P2);
  C = A + B; // C in [-2p1p2, 2p1p1], follow by a double conditional reduction
  C = conditional_reduce64(C, RRLWR_SIGN_P1P2);
  C = conditional_final_reduce64(C, RRLWR_SIGN_P1P2);

  return C;
}

/// @brief CRT on a full polynomial, including the reduction modulo q to [-q/2, q/2-1]
void poly_crt(poly *r, const poly *f, const poly *g) {
  for(unsigned int i = 0; i < RRLWR_N; i++) {
    int64_t r64 = crt(f->coeffs[i], g->coeffs[i]);
    r->coeffs[i] = (int32_t)(((r64 + (RRLWR_SIGN_Q >> 1)) & (RRLWR_SIGN_Q-1)) - (RRLWR_SIGN_Q >> 1));
  }
}

void ring_crt(ring_element *r, const ring_element *f, const ring_element *g) {
  for(unsigned int i = 0; i < RRLWR_K; i++) {
    poly_crt(&r->x[i], &f->x[i], &g->x[i]);
  }
}

/// @brief Forward NTT for both CRT domains
void ring_ntt32x2(ring_elementx2 *r, const ring_element *f) {

  // Create 2 copies of f
  for(unsigned int i = 0; i < RRLWR_K; i++) {
    for(unsigned int j = 0; j < RRLWR_N; j++) {
      r->xp1.x[i].coeffs[j] = f->x[i].coeffs[j];
      r->xp2.x[i].coeffs[j] = f->x[i].coeffs[j];
    }
  }

  // Apply NTT for separate primes
  ring_ntt32(&r->xp1, RRLWR_SIGN_PRIME1, RRLWR_SIGN_PRIME1INV, rrlwr_sign_zetas1);
  ring_ntt32(&r->xp2, RRLWR_SIGN_PRIME2, RRLWR_SIGN_PRIME2INV, rrlwr_sign_zetas2);
}

/// @brief Multiplication and inverse NTT for both CRT domains
void ring_mul_invntt32x2(ring_element *r, ring_elementx2 *a, ring_elementx2 *b) {
  ring_elementx2 rx2;

  ring_mul_invntt32((poly*)&rx2.xp1.x, &a->xp1, &b->xp1, RRLWR_K, RRLWR_SIGN_PRIME1, RRLWR_SIGN_PRIME1INV, RRLWR_SIGN_NTTINV_FINALCONST1, RRLWR_SIGN_RMODPRIME1, RRLWR_SIGN_2RMODPRIME1, rrlwr_sign_zetas1);
  ring_mul_invntt32((poly*)&rx2.xp2.x, &a->xp2, &b->xp2, RRLWR_K, RRLWR_SIGN_PRIME2, RRLWR_SIGN_PRIME2INV, RRLWR_SIGN_NTTINV_FINALCONST2, RRLWR_SIGN_RMODPRIME2, RRLWR_SIGN_2RMODPRIME2, rrlwr_sign_zetas2);
  ring_crt(r, &rx2.xp1, &rx2.xp2);
}

/// @brief Full multiplication with forward and inverse NTT for both CRT domains
void ring_mul32x2(ring_element *r, const ring_element *f, const ring_element *g) {
  ring_elementx2 fx2, gx2;

  ring_ntt32x2(&fx2, f);
  ring_ntt32x2(&gx2, g);
  ring_mul_invntt32x2(r, &fx2, &gx2);
}