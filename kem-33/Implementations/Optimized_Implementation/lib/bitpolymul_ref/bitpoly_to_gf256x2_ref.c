
#include "bitpoly_to_gf256x2.h"

void bitpoly_to_gf256x2_n( uint8_t * v0 , uint8_t * v1 , const uint8_t * bitpoly , unsigned n )
{
    for(unsigned i=0;i<n;i++){
        uint16_t r = bitpoly_to_gf256x2( bitpoly[i] );
        v0[i] = r&0xff;
        v1[i] = r>>8;
    }

}

void gf256x2_to_bitpoly_n( uint8_t * bp0 , uint8_t * bp1 , const uint8_t * v0 , const uint8_t * v1 , unsigned n )
{
    for(unsigned i=0;i<n;i++) {
        uint16_t r = gf256x2_to_bitpoly( v0[i], v1[i] );
        bp0[i] = r&0xff;
        bp1[i] = r>>8;
    }
}







