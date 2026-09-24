
#ifndef _BTFY_H_
#define _BTFY_H_


#include <stdint.h>


#ifdef  __cplusplus
extern  "C" {
#endif



void btfy_gf256x2( uint8_t * v0 , uint8_t * v1 , unsigned n_stage , uint16_t idx_offset );

void ibtfy_gf256x2( uint8_t * v0 , uint8_t * v1  , unsigned n_stage , uint16_t idx_offset );



#ifdef  __cplusplus
}
#endif


#endif
