#include "gf256.h"

void gf256v_add( uint8_t * r , const uint8_t * a , const uint8_t * b , unsigned len )
{
    for(unsigned i=0;i<len;i++) r[i] = a[i]^b[i];
}

void gf256v_mul( uint8_t * r , const uint8_t * a , const uint8_t * b , unsigned len )
{
    for(unsigned i=0;i<len;i++) r[i] = gf256_mul( a[i] , b[i] );
}

void gf256x2v_mul( uint8_t * c0 , uint8_t * c1 , 
    const uint8_t * a0 , const uint8_t * a1 , const uint8_t * b0 , const uint8_t * b1 , unsigned len )
{
    for(unsigned i=0;i<len;i++) {
        uint8_t r0 = gf256_mul( a0[i] , b0[i] );
        uint8_t r2 = gf256_mul( a1[i] , b1[i] );
        uint8_t r1 = gf256_mul( a0[i]^a1[i] , b0[i]^b1[i] )^r0;
        c0[i] = r0 ^ gf256_mul( r2 , 0x20 );
        c1[i] = r1;
    }
}
