#ifndef _BC_1_H_
#define _BC_1_H_

#include "stdint.h"


//
// libaray for basis conversion
// computation unit: 1 bit
//


void bc_1_256( void *poly , unsigned n_256bit );

void ibc_1_256( void *poly , unsigned n_256bit );


/////////////////////////////////////////

// n_byte >= 32
void bc_1( void * poly , unsigned n_byte );

void ibc_1( void * poly , unsigned n_byte );


#endif
