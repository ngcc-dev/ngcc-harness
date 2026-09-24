#pragma once

#include "tsuov_params.h"
#include "stdint.h"
#include <stddef.h>

typedef uint8_t Fq ;

#if TSUOV_q == 31
inline static int Fq_reduction(int Z){
      Z = (Z & TSUOV_q) + ((Z & ~TSUOV_q) >> TSUOV_ceil_log_2_q) ;
  int C = ((Z + 1) & ~TSUOV_q) ;
      Z += (C>>TSUOV_ceil_log_2_q) ;
      Z -= C ;
  return Z ;
}
#else
#  error "unsupported TSUOV_q"
#endif

inline static Fq Fq_add(Fq X, Fq Y){ return (Fq)Fq_reduction((int)X+(int)Y); }
inline static Fq Fq_sub(Fq X, Fq Y){ return (Fq)Fq_reduction((int)X-(int)Y+TSUOV_q) ; }
inline static Fq Fq_mul(Fq X, Fq Y){ return (Fq)Fq_reduction((int)X*(int)Y); }
inline static Fq Fq_inv(Fq X){
    extern Fq Fq_inv_table[TSUOV_q] ;
    return Fq_inv_table[X] ;
}

#if TSUOV_L == 2
static inline void phi_t2_q31_planar(uint8_t *data, uint8_t *dst, size_t n_pairs) {
    for (size_t i = 0; i < n_pairs; i++) {
        uint8_t a0 = data[i];
        uint8_t a1 = data[i + n_pairs];
        dst[i] = Fq_add(a0, a1);
        dst[i + n_pairs] = Fq_mul(8, a1);
    }
}
#else
#  error "unsupported TSUOV_L"
#endif

inline static void store_bits(
  int             x,
  const int       num_bits,
  uint8_t       * pool,
  size_t        * pool_bits
){
  int    shift = (int)(*pool_bits &  7) ;
  size_t index = (*pool_bits >> 3) ;
  int    mask  = (1<<num_bits) - 1 ;
  x &= mask ;
  x <<= shift ;
  uint8_t x0 = (x & 0xFF) ;
  if(shift == 0){
    pool[index] = x0 ;
  }else{
    pool[index] |= x0 ;
  }
  if(shift + num_bits > 8){
    uint8_t x1 = (x >> 8) ;
    pool[index+1] = x1 ;
  }
  *pool_bits += (size_t) num_bits ;
}

inline static int restore_bits(
  const uint8_t * pool,
  size_t        * pool_bits,
  const int       num_bits
){
  int    shift = (int)(*pool_bits &  7) ;
  size_t index = (*pool_bits >> 3) ;
  int    mask  = (1<<num_bits) - 1 ;

  int x        = ((int) pool[index])
               | (((shift + num_bits > 8) ? (int)pool[index+1] : 0) << 8) ;
  x >>= shift ;
  x &= mask ;
  *pool_bits += (size_t) num_bits ;
  return x ;
}

inline static void store_Fq(
  int             x,
  uint8_t       * pool,
  size_t        * pool_bits
){
  store_bits(x, TSUOV_ceil_log_2_q, pool, pool_bits) ;
}

inline static Fq restore_Fq(
  const uint8_t * pool,
  size_t * pool_bits
){
  return (Fq) restore_bits(pool, pool_bits, TSUOV_ceil_log_2_q) ;
}
