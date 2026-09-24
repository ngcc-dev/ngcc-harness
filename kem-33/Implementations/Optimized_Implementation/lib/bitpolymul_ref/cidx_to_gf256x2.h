
#ifndef _CANTOR_TO_GF256X2_H_
#define _CANTOR_TO_GF256X2_H_


#include <stdint.h>

#ifdef  __cplusplus
extern  "C" {
#endif

static const uint16_t _cidx_to_gf256x2[16] __attribute__((aligned(32))) = {
  0x1, 0xbc, 0x5c, 0xc, 0xae, 0x5a, 0xe, 0x84,
  0x1f6, 0xbc5c, 0x5c18, 0xc7e, 0xae46, 0x5a68, 0xe02, 0x8456,
};

static inline
uint16_t cidx_to_gf256x2( uint16_t cidx )
{
  uint16_t r = 0;
  while( cidx ) {
    r ^= _cidx_to_gf256x2[ __builtin_ctz(cidx) ];
    cidx &= (cidx-1);
  }
  return r;
}

#ifdef  __cplusplus
}
#endif


#endif
