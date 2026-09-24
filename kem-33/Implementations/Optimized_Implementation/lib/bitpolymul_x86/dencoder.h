
#ifndef _DENCODE_H_
#define _DENCODE_H_


#include <stdint.h>


#ifdef  __cplusplus
extern  "C" {
#endif


// sizeof output = 2 x sizeof input // assert(n_u64>=4)
// previous signature: void encode_64( uint64_t * rfx , const uint64_t * fx , unsigned n_u64 );
// now, n_u64 describes the number of 64-bit elements in the output instead of input.
// usually, src_bits = 32 for nomial cases of bit-polynomial multiplication.
void encode_64( uint64_t * rfx , unsigned n_u64, const uint64_t * fx , unsigned src_bits );

// sizeof output = sizeof input // assert(n_u64>=8)
void decode_64( uint64_t * rfx , const uint64_t * fx , unsigned n_u64 );




#ifdef  __cplusplus
}
#endif


#endif
