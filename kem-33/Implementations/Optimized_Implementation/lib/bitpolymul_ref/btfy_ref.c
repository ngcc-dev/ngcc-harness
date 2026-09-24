
#include <stdint.h>
#include <string.h>

#include "gf256.h"
#include "cidx_to_gf256x2.h"
#include "btfy.h"


//////////////////////////////////


static inline
void btfy_unit_gf256x2( uint8_t * v0 , uint8_t * v1 , unsigned unit ,  uint16_t a )
{
	unsigned unit_2= unit/2;
    uint8_t a0 = a&0xff;
    uint8_t a1 = a>>8;
	for(unsigned i=0;i<unit_2;i++) {
        uint16_t vh_i_x_a = gf256x2_mul( v0[unit_2+i] , v1[unit_2+i] , a0 , a1 );
		v0[i] ^= (vh_i_x_a&0xff);
        v1[i] ^= (vh_i_x_a>>8);
		v0[unit_2+i] ^= v0[i];
		v1[unit_2+i] ^= v1[i];
    }
}

void btfy_gf256x2( uint8_t * v0 , uint8_t * v1 , unsigned n_stage , uint16_t idx_offset )
{
	if( 0 == n_stage ) return;

	for(int stage=n_stage-1; stage>=0; stage--) {
		unsigned unit = (1<<(stage+1));
		unsigned num = (1<<(((int)n_stage)-1-stage));

		for(unsigned j=0;j<num;j++) {
            unsigned idx = idx_offset+j*unit;
			uint16_t idx_gf = cidx_to_gf256x2(idx>>stage);
            btfy_unit_gf256x2( v0 + j*unit , v1 + j*unit , unit , idx_gf  );
		}
	}
}

/////////////////////////////////////////////////////////

static inline
void ibtfy_unit_gf256x2( uint8_t * v0 , uint8_t * v1 , unsigned unit ,  uint16_t a )
{
	unsigned unit_2= unit/2;
    uint8_t a0 = a&0xff;
    uint8_t a1 = a>>8;
	for(unsigned i=0;i<unit_2;i++) {
		v0[unit_2+i] ^= v0[i];
		v1[unit_2+i] ^= v1[i];
        uint16_t vh_i_x_a = gf256x2_mul( v0[unit_2+i] , v1[unit_2+i] , a0 , a1 );
		v0[i] ^= (vh_i_x_a&0xff);
        v1[i] ^= (vh_i_x_a>>8);
    }
}

void ibtfy_gf256x2( uint8_t * v0 , uint8_t * v1  , unsigned n_stage , uint16_t idx_offset )
{
	if( 0 == n_stage ) return;

	for(int stage=0; stage<(int)n_stage; stage++) {
		unsigned unit = (1<<(stage+1));
		unsigned num = (1<<(((int)n_stage)-1-stage));

		for(unsigned j=0;j<num;j++) {
            unsigned idx = idx_offset+j*unit;
			uint16_t idx_gf = cidx_to_gf256x2(idx>>stage);
            ibtfy_unit_gf256x2( v0 + j*unit , v1 + j*unit , unit , idx_gf  );
		}
	}
}
