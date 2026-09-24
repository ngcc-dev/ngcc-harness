
#ifndef _BITPOLY_TO_GF256X2_H_
#define _BITPOLY_TO_GF256X2_H_


#include <stdint.h>

#ifdef  __cplusplus
extern  "C" {
#endif

//// inline functions ////

static const uint16_t _bitpoly_to_gf256x2[8] __attribute__((aligned(32))) = { 0x1, 0x37c, 0x52c, 0xecc4, 0x112a, 0xb016, 0xd9a, 0xf806 };

static inline
uint16_t bitpoly_to_gf256x2( uint8_t bitpoly )
{
  uint16_t r = 0;
  for( int i=0;i<8;i++) {
    r ^= (-(uint16_t)((bitpoly>>i)&1))&_bitpoly_to_gf256x2[i];
  }
  return r;
}

static const uint16_t _gf256x2_to_bitpoly[16] __attribute__((aligned(32))) = { 0x1, 0x50f3, 0xce53, 0x6ee4, 0x6881, 0x4151, 0x409a, 0x8e55, 0xf586, 0x3c79, 0x1464, 0x3961, 0x8ad0, 0xd586, 0x5140, 0xa957 };

static inline
uint16_t gf256x2_to_bitpoly( uint8_t v0, uint8_t v1 )
{
  uint16_t r = 0;
  for( int i=0;i<8;i++) { r ^= (-(uint16_t)((v0>>i)&1))&_gf256x2_to_bitpoly[i]; }
  for( int i=0;i<8;i++) { r ^= (-(uint16_t)((v1>>i)&1))&_gf256x2_to_bitpoly[i+8]; }
  return r;
}

////////////////////////

void bitpoly_to_gf256x2_n( uint8_t * v0 , uint8_t * v1 , const uint8_t * bitpoly , unsigned n );

void gf256x2_to_bitpoly_n( uint8_t * bp0 , uint8_t * bp1 , const uint8_t * v0 , const uint8_t * v1 , unsigned n );


#ifdef  __cplusplus
}
#endif


#endif
