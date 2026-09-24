#include "stdint.h"
#include "string.h"

#include "polymul.h"

#include "btfy.h"
#include "gf256.h"
#include "bitpoly_to_gf256x2.h"
#include "bc_8.h"


static inline
unsigned log_floor(unsigned a)
{
    for(unsigned i=0;i<32;i++){
        if(0==a) return i-1;
        a &=  ~(1<<i);
    }
    return 32;
}

// sizeof a_fft = sizeof a * POLYMUL_EXP_RATIO
void polymul_input_transform( uint64_t * a_fft , const uint64_t * a , unsigned n_u64 )
{
    //uint64_t buf0[POLYMUL_MAX_INPUT_U64*4] __attribute__ ((aligned (32)));
    unsigned len = n_u64*8;
    unsigned loglen = log_floor(len);
    uint8_t * a0 = (uint8_t *)a_fft;
    uint8_t * a1 = a0 + len*2;

    // input transform
    memcpy( a0 , a , len );
    bc_8( a0 , len );
    bitpoly_to_gf256x2_n( a0 , a1 , a0 , len );
    // first stage of btfy
    memcpy( a0+len , a0 , len );
    memcpy( a1+len , a1 , len );
    // the rest btfy stages
    btfy_gf256x2( a0 , a1 , loglen , 0 );
    btfy_gf256x2( a0+len , a1+len , loglen , len );
}

// c = a_fft * b_fft
void polymul_output( uint64_t * c , const uint64_t * a_fft , const uint64_t * b_fft , unsigned n_u64 )
{
    uint64_t _c1[POLYMUL_MAX_INPUT_U64*4] __attribute__ ((aligned (32)));
    unsigned len = n_u64*8;
    unsigned loglen = log_floor(len);

    uint8_t * a0 = (uint8_t *)a_fft;
    uint8_t * a1 = a0 + len*2;
    uint8_t * b0 = (uint8_t *)b_fft;
    uint8_t * b1 = b0 + len*2;
    uint8_t * c0 = (uint8_t*)c;
    uint8_t * c1 = (uint8_t*)_c1;

    // multiply
    gf256x2v_mul( c0 , c1 , a0 , a1 , b0 , b1 , len*2 );
    // output transform
    ibtfy_gf256x2( c0 , c1 , loglen+1 , 0 );
    gf256x2_to_bitpoly_n( c0 , c1 , c0 , c1 , 2*len );
    ibc_8( c0 , 2*len );
    ibc_8( c1 , 2*len );
    gf256v_add( c0+1 , c0+1 , c1 , 2*len-1 );
}

void polymul_addfft( uint64_t * c , const uint64_t * a , const uint64_t * b, unsigned n_u64 )
{
    uint64_t a1[POLYMUL_MAX_INPUT_U64*POLYMUL_EXP_RATIO] __attribute__ ((aligned (32)));
    uint64_t b1[POLYMUL_MAX_INPUT_U64*POLYMUL_EXP_RATIO] __attribute__ ((aligned (32)));

    polymul_input_transform( a1 , a , n_u64 );
    polymul_input_transform( b1 , b , n_u64 );
    polymul_output( c , a1 , b1 , n_u64 );
}

