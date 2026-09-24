
#ifndef _POLYMUL_H_
#define _POLYMUL_H_


#include <stdint.h>


#ifdef  __cplusplus
extern  "C" {
#endif


#define POLYMUL_MAX_INPUT_U64   (1024)
#define POLYMUL_EXP_RATIO  (4)

// sizeof a_fft = sizeof a * POLYMUL_EXP_RATIO
void polymul_input_transform( uint64_t * a_fft , const uint64_t * a , unsigned n_u64 );

// c = a_fft * b_fft
void polymul_output( uint64_t * c , const uint64_t * a_fft , const uint64_t * b_fft , unsigned n_u64 );

// c = a * b
void polymul_addfft( uint64_t * c , const uint64_t * a , const uint64_t * b, unsigned n_u64 );


#ifdef  __cplusplus
}
#endif

#include <polymul_hqclen.h>

#endif

