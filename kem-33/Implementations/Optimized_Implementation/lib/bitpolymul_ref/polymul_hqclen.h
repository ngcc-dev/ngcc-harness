
#ifndef _POLYMUL_HQCLEN_H_
#define _POLYMUL_HQCLEN_H_


#include <stdint.h>

#include "polymul.h"

#ifdef  __cplusplus
extern  "C" {
#endif

#define POLYMUL_1124U64_FFTSIZE_U64  (4096)

///
/// @brief compute c = a * b. a and b are bit-polynomials of size < 32768+4096 bits.
///
/// @param c [out] product bit-polynomials of size < 131072 + 12800 bits = 2248 u64 = 16384+1600 bytes
/// @param a_fft [in] a transformed bit-polynomial. size : 4296 u64 = 32768+1600 bytes
/// @param b_fft [in] a transformed bit-polynomial. size : 4296 u64 = 32768+1600 bytes
void polymul_71936_mul( uint64_t * c , const uint64_t * a_fft , const uint64_t * b_fft );

#define POLYMUL_71936_FFTSIZE_BYTE  (32768 + 1600)


#define POLYMUL_24576_FFTSIZE_BYTE  (8192 + 2048)
///
/// @brief input transform for multiplying two bit-polynomials of size < 24576 bits
///
/// @param a_fft [out] size : 65536 + 16384 bits = 1024+256 u64  = 8192+2048 bytes= 4096+4096+2048 bytes
/// @param a [in] a bit-polynomial of size < 16384 + 8192 bits = 256+128 u64 = 2048+1024 bytes=3072 bytes
void polymul_24576_input( uint64_t * a_fft , const uint64_t * a );

///
/// @brief compute c = a * b. a and b are bit-polynomials of size < 16384+8192 bits.
///
/// @param c [out] product bit-polynomials of size < 32768 + 16384 bits = 768 u64 = 6144=(4096+2048) bytes
/// @param a_fft [in] a transformed bit-polynomial. size : 10240 u64 = 8192+2048 bytes
/// @param b_fft [in] a transformed bit-polynomial. size : 10240 u64 = 8192+2048 bytes
void polymul_24576_mul( uint64_t * c , const uint64_t * a_fft , const uint64_t * b_fft );


///
/// @brief compute c = a * b. a and b are bit-polynomials of size < 16384+8192 bits.
///
/// @param c [out] product bit-polynomials of size < 32768 + 16384 bits = 768 u64 = 6144=(4096+2048) bytes
/// @param a [in] the input bit-polynomial. size : < 384 u64 = 3072 bytes
/// @param b [in] the input bit-polynomial. size : < 384 u64 = 3072 bytes
/// @param a_fft [in] a transformed partial bit-polynomial. size : 1024 u64 = 8192 bytes
/// @param b_fft [in] a transformed partial bit-polynomial. size : 1024 u64 = 8192 bytes
void polymul_384U64_mul( uint64_t * _c , const uint64_t * _a , const uint64_t * _b , const uint64_t * _a_fft , const uint64_t * _b_fft );
#define POLYMUL_384U64_FFTSIZE_U64  (1024)


#define POLYMUL_11776_FFTSIZE_BYTE  (4096 + 896)
///
/// @brief input transform for multiplying two bit-polynomials of size < 11776 bits
///
/// @param a_fft [out] size : 32768 + 7168 bits = 512+112 u64  = 4096+896 bytes
/// @param a [in] a bit-polynomial of size < 8192 + 3584 bits = 128+56 u64 = 1024+448 bytes
void polymul_11776_input( uint64_t * a_fft , const uint64_t * a );

///
/// @brief compute c = a * b. a and b are bit-polynomials of size < 16384+1536 bits.
///
/// @param c [out] product bit-polynomials of size < 16384 + 7168 bits = 368 u64 = 2944=(2048+896) bytes
/// @param a_fft [in] a transformed bit-polynomial. size : 624 u64 = 4096+896 bytes
/// @param b_fft [in] a transformed bit-polynomial. size : 624 u64 = 4096+896 bytes
void polymul_11776_mul( uint64_t * c , const uint64_t * a_fft , const uint64_t * b_fft );

// target : 17669 bits
#define POLYMUL_184U64_FFTSIZE_U64  (512)



///
/// @brief input transform for multiplying two bit-polynomials of size < (16384+1536) bits
///
/// @param a_fft [out] size : transformed values of the partial input polynomial: 65536 bits = 1024 u64  = 8192 bytes
/// @param a [in] a bit-polynomial of size < 16384 + 1536 bits = 256+24 u64 = 2048+192 bytes
void polymul_280U64_input( uint64_t * a_fft , const uint64_t * a );

///
/// @brief compute c = a * b. a and b are bit-polynomials of size < 16384+1536 bits.
///
/// @param c [out] product bit-polynomials of size < 32768 + 3072 bits = 512+48 u64 = 4480=(4096+384) bytes
/// @param a [in] the input bit-polynomial. size : < 280 u64 = 2240 bytes
/// @param b [in] the input bit-polynomial. size : < 280 u64 = 2240 bytes
/// @param a_fft [in] a transformed partial bit-polynomial. size : 1024 u64 = 8192 bytes
/// @param b_fft [in] a transformed partial bit-polynomial. size : 1024 u64 = 8192 bytes
void polymul_280U64_mul( uint64_t * c , const uint64_t * a , const uint64_t * b , const uint64_t * a_fft , const uint64_t * b_fft );


#define POLYMUL_37376_FFTSIZE_BYTE  (16384 + 1152)

///
/// @brief input transform for multiplying two bit-polynomials of size < 37376 bits
///
/// @param a_fft [out] size : 131072 + 9216 bits = 2048+144 u64 = 16384+1152 bytes
/// @param a [in] a bit-polynomial of size < 32768 + 4608 bits = 584 u64 = 4096+576 bytes
void polymul_37376_input( uint64_t * a_fft , const uint64_t * a );

/// @brief input transform for multiplying two bit-polynomials of size < 71936 bits
///
/// @param a_fft [out] size : 262144 + 12800 bits = 4096+200 u64 = 32768+1600 bytes
/// @param a [in] a bit-polynomial of size < 65536 + 6400 bits = 1124 u64 = 8192+800 bytes
void polymul_71936_input( uint64_t * a_fft , const uint64_t * a );

///
/// @brief compute c = a * b. a and b are bit-polynomials of size < 32768+4096 bits.
///
/// @param c [out] product bit-polynomials of size < 65536 + 9216 bits = 1168 u64 = 8192+1152 bytes
/// @param a_fft [in] a transformed bit-polynomial. size : 2176 u64 = 16384+1152 bytes
/// @param b_fft [in] a transformed bit-polynomial. size : 2176 u64 = 16384+1152 bytes
void polymul_37376_mul( uint64_t * c , const uint64_t * a_fft , const uint64_t * b_fft );


// target : 35851 bits
#define POLYMUL_584U64_FFTSIZE_U64  (2048)


///
/// @brief compute c = a * b. a and b are bit-polynomials of size < 16384+7168 bits.
///
/// @param c [out] product bit-polynomials of size < 16384+7168 bits = 256+112 u64 = 2944=(2048+896) bytes
/// @param a [in] the input bit-polynomial. size : < 184 u64 = 1472 bytes
/// @param b [in] the input bit-polynomial. size : < 184 u64 = 1472 bytes
/// @param a_fft [in] a transformed partial bit-polynomial. size : 256 u64 = 2048 bytes
/// @param b_fft [in] a transformed partial bit-polynomial. size : 256 u64 = 2048 bytes
void polymul_184U64_mul( uint64_t * _c , const uint64_t * _a , const uint64_t * _b , const uint64_t * _a_fft , const uint64_t * _b_fft );


///
/// @brief input transform for multiplying two bit-polynomials of size < 32768+4096 bits
///
/// @param a_fft [out] size : transformed values of the partial input polynomial: 2048 u64  = 16384 bytes
/// @param a [in] a bit-polynomial of size < 32768 + 4096 bits = 512+64 u64 = 4608=(4096+512) bytes
void polymul_576U64_input( uint64_t * a_fft , const uint64_t * a );

///
/// @brief compute c = a * b. a and b are bit-polynomials of size < 32768+4096 bits.
///
/// @param c [out] product bit-polynomials of size < 1024+128 u64 = 8976=(8192+1024) bytes
/// @param a [in] the input bit-polynomial. size : < 512+64 u64 = 4608 bytes
/// @param b [in] the input bit-polynomial. size : < 512+64 u64 = 4608 bytes
/// @param a_fft [in] a transformed partial bit-polynomial. size : 2048 u64 = 16384 bytes
/// @param b_fft [in] a transformed partial bit-polynomial. size : 2048 u64 = 16384 bytes
void polymul_576U64_mul( uint64_t * c , const uint64_t * a , const uint64_t * b , const uint64_t * a_fft , const uint64_t * b_fft );


///
/// @brief compute c = a * b. a and b are bit-polynomials of size < 65536+6656 bits.
///
/// @param c [out] product bit-polynomials of size < 2048+200 u64 = (16384+1600) bytes
/// @param a [in] the input bit-polynomial. size : < 1024+104 u64 = 9024 bytes
/// @param b [in] the input bit-polynomial. size : < 1024+104 u64 = 9024 bytes
/// @param a_fft [in] a transformed partial bit-polynomial. size : 4096 u64 = 32768 bytes
/// @param b_fft [in] a transformed partial bit-polynomial. size : 4096 u64 = 32768 bytes
void polymul_1124U64_mul( uint64_t * _c , const uint64_t * _a , const uint64_t * _b , const uint64_t * _a_fft , const uint64_t * _b_fft );



///
/// @brief compute c = a * b. a and b are bit-polynomials of size < 32768+4608 bits.
///
/// @param c [out] product bit-polynomials of size < 1024+144 u64 = 9344=(8192+1152) bytes
/// @param a [in] the input bit-polynomial. size : < 512+64 u64 = 4608 bytes
/// @param b [in] the input bit-polynomial. size : < 512+64 u64 = 4608 bytes
/// @param a_fft [in] a transformed partial bit-polynomial. size : 2048 u64 = 16384 bytes
/// @param b_fft [in] a transformed partial bit-polynomial. size : 2048 u64 = 16384 bytes
void polymul_584U64_mul( uint64_t * _c , const uint64_t * _a , const uint64_t * _b , const uint64_t * _a_fft , const uint64_t * _b_fft );
#ifdef  __cplusplus
}
#endif


#endif

