#include "fprime.h"

/// @brief Montgomery reduction for a 32-bit prime modulus p with R = 2^32
/// @param[in] a 64-bit input
/// @param[in] p 32-bit modulus
/// @param[in] pinv -p^-1 mod 2^32
/// @return An integer in [-p+1,p] congruent to a*R^-1 mod p (no final reduction)
static int32_t montgomery_reduce32(int64_t a, int32_t prime, int32_t primeinv)
{
  int32_t t;

  t = (int32_t)a*primeinv;
  t = (a + (int64_t)t*prime) >> 32;
  return t;
}

/// @brief Montgomery multiplication for a 32-bit prime modulus p
/// @param[in] a 32-bit input
/// @param[in] b 32-bit input
/// @param[in] p 32-bit modulus
/// @param[in] pinv -p^-1 mod 2^32
/// @return (a*b) mod p
int32_t montgomery_mul32(int32_t a, int32_t b, int32_t prime, int32_t primeinv) {
  return montgomery_reduce32((int64_t)a*b, prime, primeinv);
}

/// @brief Conditional reduction based on the sign bit of r after an addition or subtraction
/// @param[in] r in [-2*p, 2*p]
/// @return r mod p in [-p, p]
int32_t conditional_reduce32(int32_t r, int32_t prime) {
  int32_t sign_mask = -((r >> 31) & 1);            // -1 if r < 0,  0 if r >= 0
  int32_t shift = ((2*prime) & sign_mask) - prime; //  p if r < 0, -p if r >= 0
  r += shift;                                      // Reduced to interval [-p, p]

  return r;
}

/// @brief Conditional reduction based on the sign bit of r to a unique representation
/// @param[in] r in [-p+1, p]
/// @return r mod p in [-(p-1)/2+1, (p-1)/2]
int32_t conditional_final_reduce32(int32_t r, int32_t prime) {
  int32_t offset = (prime-1) >> 1;                                    // (p-1)/2
  int32_t sign_mask_neg = -(((r+(offset-1)) >> 31) & 1);              // -1 if r < -(p-1)/2+1,  0 if r >= -(p-1)/2+1
  int32_t sign_mask_pos = -(((r-(offset+1)) >> 31) & 1);              // -1 if r <  (p-1)/2+1,  0 if r >=  (p-1)/2+1
  int32_t shift = (prime & sign_mask_neg) - (prime & ~sign_mask_pos); //  p if r < -(p-1)/2+1, -p if r >=  (p-1)/2+1, 0 otherwise
  r += shift;

  return r;
}

/// @brief Modular addition modulo p
/// @param[in] a in [-p, p]
/// @param[in] b in [-p, p]
/// @return (a+b) mod p in [-p, p]
int32_t add32(int32_t a, int32_t b, int32_t prime) {
  return conditional_reduce32(a + b, prime);
}

/// @brief Modular subtraction modulo p
/// @param[in] a in [-p, p]
/// @param[in] b in [-p, p]
/// @return (a-b) mod p in [-p, p]
int32_t sub32(int32_t a, int32_t b, int32_t prime) {
  return conditional_reduce32(a - b, prime);
}