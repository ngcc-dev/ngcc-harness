
#include "gf264.h"
#include "gf264_aesni.h"

uint64_t gf264_mul( uint64_t a , uint64_t b ) { return gf2ext64_mul_u64( a, b ); }

void gf264v_mul( uint64_t * c, const uint64_t *a , const uint64_t *b , unsigned len )
{
    while( len > 3 ) {
        gf2ext64_mul_4x4_avx2_unaligned( c , a , b );
        c += 4;
        a += 4;
        b += 4;
        len -= 4;
    }
    if( len > 1 ) {
        gf2ext64_mul_2x2_sse_unaligned( c , a , b );
        c += 2;
        a += 2;
        b += 2;
        len -= 2;
    }
    if ( len ) c[0] = gf2ext64_mul_u64( a[0] , b[0] );
}
