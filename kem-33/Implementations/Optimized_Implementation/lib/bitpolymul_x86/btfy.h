
#ifndef _BTFY_H_
#define _BTFY_H_


#include <stdint.h>


#ifdef  __cplusplus
extern  "C" {
#endif



void btfy_64( uint64_t * poly , unsigned log_polylen , uint64_t scalar_a );

void ibtfy_64( uint64_t * poly , unsigned log_polylen , uint64_t scalar_a );



#ifdef  __cplusplus
}
#endif


#endif
