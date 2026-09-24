#include "stdint.h"
#include "string.h"

#include "polymul.h"

#include "btfy.h"
#include "gf264.h"
#include "dencoder.h"
#include "bc_1.h"

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
// n_u64: number of u64 terms in polynomial a
void polymul_input_transform( uint64_t * a_fft , const uint64_t * a , unsigned n_u64 )
{
    uint64_t a1[POLYMUL_MAX_INPUT_U64*2] __attribute__ ((aligned (32)));
    unsigned loglen = log_floor(n_u64);
    // input transform
    memcpy( a1 , a , n_u64*8 );
    bc_1( a1 , n_u64*8 );
    encode_64(a_fft,n_u64*2,a1,32);
    btfy_64(a_fft,loglen+1,1ULL<<(32+loglen+1));
}

// c = a_fft * b_fft
// n_u64: number of u64 terms in polynomial a
void polymul_output( uint64_t * c , const uint64_t * a_fft , const uint64_t * b_fft , unsigned n_u64 )
{
    uint64_t a1[POLYMUL_MAX_INPUT_U64*2] __attribute__ ((aligned (32)));
    unsigned loglen = log_floor(n_u64);

    gf264v_mul( a1 , a_fft , b_fft , n_u64*2 );
    // output transform
    ibtfy_64(a1,loglen+1,1ULL<<(32+loglen+1));
    decode_64( c , a1 , n_u64*2 );
    ibc_1( c , n_u64*2*8 );
}

// c = a * b
// n_u64: number of u64 terms in polynomial a
void polymul_fafft( uint64_t * c , const uint64_t * a , const uint64_t * b, unsigned n_u64 )
{
    uint64_t a1[POLYMUL_MAX_INPUT_U64*2] __attribute__ ((aligned (32)));
    uint64_t b1[POLYMUL_MAX_INPUT_U64*2] __attribute__ ((aligned (32)));

    polymul_input_transform( a1 , a , n_u64 );
    polymul_input_transform( b1 , b , n_u64 );
    polymul_output( c , a1 , b1 , n_u64 );
}


