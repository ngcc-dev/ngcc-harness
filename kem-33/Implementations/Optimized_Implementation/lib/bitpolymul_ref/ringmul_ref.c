#include "stdint.h"
#include "string.h"

#include "ringmul.h"


static inline
uint16_t _mul_8( uint8_t a , uint8_t b )
{
    uint16_t r = 0;
    for( int i = 0 ; i < 8 ; i++ )
    {
        if( b & (1<<i) )
            r ^= ((uint16_t)a) << i;
    }
    return r;
}

/// @brief multiply two polynomials in F216[x]/ x^2048
/// @param c0 [out] low  byte of F216. size : 2048 bytes
/// @param c1 [out] high byte of F216. size : 2048 bytes
/// @param a [in] size : 2048 bytes
/// @param b [in] size : 2048 bytes
void ringmul_mul_2048( uint8_t * c0 , uint8_t * c1 , const uint8_t * a , const uint8_t * b )
{
#define LEN (2048)
    uint16_t t0[LEN] = {0};
    for(int i=0;i<LEN;i++)
    {
        for(int j=0;j<LEN;j++)
        {
            if( i+j >= LEN ) break;
            t0[i+j] ^= _mul_8( a[i] , b[j] );
        }
    }
    for(int i=0;i<LEN;i++)
    {
        c0[i] = t0[i] & 0xff;
        c1[i] = (t0[i] >> 8);
    }
#undef LEN
}



/// @brief multiply two polynomials in F216[x]/ x^896
/// @param c0 [out] low  byte of F216. size : 896 bytes
/// @param c1 [out] high byte of F216. size : 896 bytes
/// @param a [in] size : 896 bytes
/// @param b [in] size : 896 bytes
void ringmul_mul_896( uint8_t * c0 , uint8_t * c1 , const uint8_t * a , const uint8_t * b )
{
#define LEN (896)
    uint16_t t0[LEN] = {0};
    for(int i=0;i<LEN;i++)
    {
        for(int j=0;j<LEN;j++)
        {
            if( i+j >= LEN ) break;
            t0[i+j] ^= _mul_8( a[i] , b[j] );
        }
    }
    for(int i=0;i<LEN;i++)
    {
        c0[i] = t0[i] & 0xff;
        c1[i] = (t0[i] >> 8);
    }
#undef LEN
}

/// @brief multiply two polynomials in F216[x]/ x^1152
/// @param c0 [out] low  byte of F216. size : 1152 bytes
/// @param c1 [out] high byte of F216. size : 1152 bytes
/// @param a [in] size : 1152 bytes
/// @param b [in] size : 1152 bytes
void ringmul_mul_1152( uint8_t * c0 , uint8_t * c1 , const uint8_t * a , const uint8_t * b )
{
#define LEN (1152)
    uint16_t t0[LEN] = {0};
    for(int i=0;i<LEN;i++)
    {
        for(int j=0;j<LEN;j++)
        {
            if( i+j >= LEN ) break;
            t0[i+j] ^= _mul_8( a[i] , b[j] );
        }
    }
    for(int i=0;i<LEN;i++)
    {
        c0[i] = t0[i] & 0xff;
        c1[i] = (t0[i] >> 8);
    }
#undef LEN
}

/// @brief multiply two polynomials in F216[x]/ x^1600
/// @param c0 [out] low  byte of F216. size : 1600 bytes
/// @param c1 [out] high byte of F216. size : 1600 bytes
/// @param a [in] size : 1600 bytes
/// @param b [in] size : 1600 bytes
void ringmul_mul_1600( uint8_t * c0 , uint8_t * c1 , const uint8_t * a , const uint8_t * b )
{
#define LEN (1600)
    uint16_t t0[LEN] = {0};
    for(int i=0;i<LEN;i++)
    {
        for(int j=0;j<LEN;j++)
        {
            if( i+j >= LEN ) break;
            t0[i+j] ^= _mul_8( a[i] , b[j] );
        }
    }
    for(int i=0;i<LEN;i++)
    {
        c0[i] = t0[i] & 0xff;
        c1[i] = (t0[i] >> 8);
    }
#undef LEN
}



