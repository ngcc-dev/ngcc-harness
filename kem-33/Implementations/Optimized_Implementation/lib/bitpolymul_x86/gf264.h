
#ifndef _GF264_H_
#define _GF264_H_


#include <stdint.h>


/// X^64 + X^4 + X^3 + X + 1
/// 0x1b
static inline
uint64_t _gf264_mul( uint64_t a , uint64_t b )
{
  uint64_t r = a&(-(b&1));
  for(int i=1;i<64;i++) {
    uint32_t ar = ((a>>63)&1)*0x1b;
    a = (a<<1)^ar;
    b >>=1;
    r ^= a&(-(b&1));
  }
  return r;
}

uint64_t gf264_mul( uint64_t a , uint64_t b );

void gf264v_mul( uint64_t * c, const uint64_t *a , const uint64_t *b , unsigned len );

uint64_t gf264_inv( uint64_t a );

#endif

