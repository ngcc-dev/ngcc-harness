
#ifndef _CANTOR_TO_GF264_H_
#define _CANTOR_TO_GF264_H_


#include <stdint.h>

// 1024*16
#define SIZE_TBL_CANTOR2X  (1<<14)

#ifdef  __cplusplus
extern  "C" {
#endif

extern const uint64_t cantor_basis[64];

extern const uint64_t cantor_to_gf264_2x[SIZE_TBL_CANTOR2X];

extern const uint64_t btfy_consts_2stages[SIZE_TBL_CANTOR2X*2];

static inline
uint64_t cantor_to_gf264( uint64_t gf_in_cantor )
{
  uint64_t r = 0;
  while( gf_in_cantor ) {
    r ^= cantor_basis[ __builtin_ctzll(gf_in_cantor) ];
    gf_in_cantor &= (gf_in_cantor-1);
  }
  return r;
}

uint64_t index_to_gf264( uint64_t fft_position_index );

#ifdef  __cplusplus
}
#endif


#endif
