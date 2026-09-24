
#ifndef _RINGMUL_H_
#define _RINGMUL_H_


#include <stdint.h>


#ifdef  __cplusplus
extern  "C" {
#endif

/// @brief multiply two polynomials in F216[x]/ x^768
/// @param c0 [out] low  byte of F216. size : 768 bytes
/// @param c1 [out] high byte of F216. size : 768 bytes
/// @param a [in] size : 7686 bytes
/// @param b [in] size : 768 bytes
void ringmul_mul_768( uint8_t * c0 , uint8_t * c1 , const uint8_t * a , const uint8_t * b );

/// @brief transform an input polynomial in F216[x]/s12 into its evaluated values in F256x2
///        s12 = x^4096 + x^256 + x^16 + x
/// @param fft_a0 [out] evaluated values of the input polynomial in low  byte of F256x2.  size: 32768 bits = 4096 byte
/// @param fft_a1 [out] evaluated values of the input polynomial in high byte of F256x2.  size: 32768 bits = 4096 byte
/// @param poly_a [in] F2 polynomial of 24576 bits = (2048+1024) bytes
void ringmul_s12_input_3072( uint8_t * fft_a0 , uint8_t * fft_a1 , const uint8_t * poly_a );

/// @brief transform an input polynomial in F216[x]/s11 into its evaluated values in F256x2
///        s12 = x^4096 + x^256 + x^16 + x
/// @param fft_a0 [out] evaluated values of the input polynomial in low  byte of F256x2.  size: 16384 bits = 2048 byte
/// @param fft_a1 [out] evaluated values of the input polynomial in high byte of F256x2.  size: 16384 bits = 2048 byte
/// @param poly_a [in] F2 polynomial of 11776 bits = (1024+448) bytes
void ringmul_s11_input_1472( uint8_t * fft_a0 , uint8_t * fft_a1 , const uint8_t * poly_a );

/// @brief transform an input polynomial in F216[x]/s13 into its evaluated values in F256x2
///        s13 = x^8192 + x^4096 + x^512 + x^256 + x^32 + x^16 + x^2 + x
/// @param fft_a0 [out] evaluated values of the input polynomial in low  byte of F256x2.  size: 65536 bits = 8192 byte
/// @param fft_a1 [out] evaluated values of the input polynomial in high byte of F256x2.  size: 65536 bits = 8192 byte
/// @param poly_a [in] F2 polynomial of 37376 bits = (4096+576) bytes
void ringmul_s13_input_4672( uint8_t * fft_a0 , uint8_t * fft_a1 , const uint8_t * poly_a );

/// @brief transform an input polynomial in F216[x]/s13 into its evaluated values in F256x2
///        s13 = x^8192 + x^4096 + x^512 + x^256 + x^32 + x^16 + x^2 + x
/// @param fft_a0 [out] evaluated values of the input polynomial in low  byte of F256x2.  size: 65536 bits = 16384 byte
/// @param fft_a1 [out] evaluated values of the input polynomial in high byte of F256x2.  size: 65536 bits = 16384 byte
/// @param poly_a [in] F2 polynomial of 65536 + 6400 bits = (8192+800) bytes
void ringmul_s14_input_8992( uint8_t * fft_a0 , uint8_t * fft_a1 , const uint8_t * poly_a );

///
/// @brief c in F216[x]/s11 = a_fft * b_fft
///
/// @param c0 [out] size : 16384 bits = 2048 byte
/// @param c1 [out] size : 16384 bits = 2048 byte
/// @param a0_fft [in] size : 16384 bits = 2048 byte
/// @param a1_fft [in] size : 16384 bits = 2048 byte
/// @param b0_fft [in] size : 16384 bits = 2048 byte
/// @param b1_fft [in] size : 16384 bits = 2048 byte
void ringmul_s11_mul( uint8_t * c0 , uint8_t * c1 , const uint8_t * a0_fft , const uint8_t * a1_fft , const uint8_t * b0_fft , const uint8_t * b1_fft );



///
/// @brief c in F216[x]/s12 = a_fft * b_fft
///
/// @param c0 [out] size : 32768 bits = 4096 byte
/// @param c1 [out] size : 32768 bits = 4096 byte
/// @param a0_fft [in] size : 3278 bits = 4096 byte
/// @param a1_fft [in] size : 3278 bits = 4096 byte
/// @param b0_fft [in] size : 32768 bits = 4096 byte
/// @param b1_fft [in] size : 32768 bits = 4096 byte
void ringmul_s12_mul( uint8_t * c0 , uint8_t * c1 , const uint8_t * a0_fft , const uint8_t * a1_fft , const uint8_t * b0_fft , const uint8_t * b1_fft );

///
/// @brief c in F216[x]/s13 = a_fft * b_fft
///
/// @param c0 [out] size : 65536 bits = 8192 byte
/// @param c1 [out] size : 65536 bits = 8192 byte
/// @param a0_fft [in] size : 65536 bits = 8192 byte
/// @param a1_fft [in] size : 65536 bits = 8192 byte
/// @param b0_fft [in] size : 65536 bits = 8192 byte
/// @param b1_fft [in] size : 65536 bits = 8192 byte
void ringmul_s13_mul( uint8_t * c0 , uint8_t * c1 , const uint8_t * a0_fft , const uint8_t * a1_fft , const uint8_t * b0_fft , const uint8_t * b1_fft );

/// @param c0 [out] size : 65536 bits = 16384 byte
/// @param c1 [out] size : 65536 bits = 16384 byte
/// @param a0_fft [in] size : 65536 bits = 16384 byte
/// @param a1_fft [in] size : 65536 bits = 16384 byte
/// @param b0_fft [in] size : 65536 bits = 16384 byte
/// @param b1_fft [in] size : 65536 bits = 16384 byte
void ringmul_s14_mul( uint8_t * c0 , uint8_t * c1 , const uint8_t * a0_fft , const uint8_t * a1_fft , const uint8_t * b0_fft , const uint8_t * b1_fft );


/// @brief multiply two polynomials in F216[x]/ x^896
/// @param c0 [out] low  byte of F216. size : 896 bytes
/// @param c1 [out] high byte of F216. size : 896 bytes
/// @param a [in] size : 896 bytes
/// @param b [in] size : 896 bytes
void ringmul_mul_896( uint8_t * c0 , uint8_t * c1 , const uint8_t * a , const uint8_t * b );

/// @brief multiply two polynomials in F216[x]/ x^384
/// @param c0 [out] low  byte of F216. size : 384 bytes
/// @param c1 [out] high byte of F216. size : 384 bytes
/// @param a [in] size : 384 bytes
/// @param b [in] size : 384 bytes
void ringmul_mul_384( uint8_t * c0 , uint8_t * c1 , const uint8_t * a , const uint8_t * b );

/// @brief multiply two polynomials in F216[x]/ x^1152
/// @param c0 [out] low  byte of F216. size : 1152 bytes
/// @param c1 [out] high byte of F216. size : 1152 bytes
/// @param a [in] size : 1152 bytes
/// @param b [in] size : 1152 bytes
void ringmul_mul_1152( uint8_t * c0 , uint8_t * c1 , const uint8_t * a , const uint8_t * b );

/// @brief multiply two polynomials in F216[x]/ x^1664
/// @param c0 [out] low  byte of F216. size : 1664 bytes
/// @param c1 [out] high byte of F216. size : 1664 bytes
/// @param a [in] size : 1664 bytes
/// @param b [in] size : 1664 bytes
void ringmul_mul_1600( uint8_t * c0 , uint8_t * c1 , const uint8_t * a , const uint8_t * b );

/// @brief multiply two polynomials in F216[x]/ x^2048
/// @param c0 [out] low  byte of F216. size : 2048 bytes
/// @param c1 [out] high byte of F216. size : 2048 bytes
/// @param a [in] size : 2048 bytes
/// @param b [in] size : 2048 bytes
void ringmul_mul_2048( uint8_t * c0 , uint8_t * c1 , const uint8_t * a , const uint8_t * b );

#ifdef  __cplusplus
}
#endif


#endif

