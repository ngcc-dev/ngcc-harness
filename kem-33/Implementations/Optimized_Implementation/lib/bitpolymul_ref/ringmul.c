#include "stdint.h"
#include "string.h"

#include "ringmul.h"

#include "btfy.h"
#include "gf256.h"
#include "bitpoly_to_gf256x2.h"
#include "bc_8.h"
#include <stdio.h>
#include "benchmark.h"
/// @brief transform an input polynomial in F216[x]/s12 into its evaluated values in F256x2
///        s12 = x^4096 + x^256 + x^16 + x
/// @param fft_a0 [out] evaluated values of the input polynomial in low  byte of F256x2.  size: 32768 bits = 4096 byte
/// @param fft_a1 [out] evaluated values of the input polynomial in high byte of F256x2.  size: 32768 bits = 4096 byte
/// @param poly_a [in] F2 polynomial of 24576 bits = (2048+1024) bytes
void ringmul_s12_input_3072( uint8_t * fft_a0 , uint8_t * fft_a1 , const uint8_t * poly_a )
{
#define LEN (4096)
#define LEN_INP (3072)
#define LEN_2 (LEN/2)
#define LEN_TAIL (LEN_INP-LEN_2)
#define LOGLEN_2 (11)

    uint8_t a0[LEN] __attribute__ ((aligned (32)));
    memcpy( a0 , poly_a , LEN_INP );
    memset( a0+LEN_INP , 0 , LEN-LEN_INP );

    // input transform
    bc_8( a0 , LEN );  // XXX: can be optimized since a0 is 0 padded

    // convert to gf256x2
    bitpoly_to_gf256x2_n( fft_a0 , fft_a1 , a0 , LEN_INP );

    // first stage of btfy
    gf256v_add( fft_a0+LEN_2 , fft_a0+LEN_2 , fft_a0 , LEN_TAIL );
    gf256v_add( fft_a1+LEN_2 , fft_a1+LEN_2 , fft_a1 , LEN_TAIL );
    memcpy( fft_a0+LEN_INP , fft_a0+LEN_TAIL , LEN_2-LEN_TAIL );
    memcpy( fft_a1+LEN_INP , fft_a1+LEN_TAIL , LEN_2-LEN_TAIL );

    // the rest btfy stages
    btfy_gf256x2( fft_a0 , fft_a1 , LOGLEN_2 , 0 );
    btfy_gf256x2( fft_a0+LEN_2 , fft_a1+LEN_2 , LOGLEN_2 , LEN_2 );

#undef LEN
#undef LEN_INP
#undef LEN_2
#undef LEN_TAIL
#undef LOGLEN_2
}


/// @brief transform an input polynomial in F216[x]/s11 into its evaluated values in F256x2
///        s12 = x^4096 + x^256 + x^16 + x
/// @param fft_a0 [out] evaluated values of the input polynomial in low  byte of F256x2.  size: 16384 bits = 2048 byte
/// @param fft_a1 [out] evaluated values of the input polynomial in high byte of F256x2.  size: 16384 bits = 2048 byte
/// @param poly_a [in] F2 polynomial of 11776 bits = (1024+448) bytes
void ringmul_s11_input_1472( uint8_t * fft_a0 , uint8_t * fft_a1 , const uint8_t * poly_a )
{
#define LEN (2048)
#define LEN_INP (1472)
#define LEN_2 (LEN/2)
#define LEN_TAIL (LEN_INP-LEN_2)
#define LOGLEN_2 (10)

    uint8_t a0[LEN] __attribute__ ((aligned (32)));
    memcpy( a0 , poly_a , LEN_INP );
    memset( a0+LEN_INP , 0 , LEN-LEN_INP );

    // input transform
    bc_8( a0 , LEN );  // XXX: can be optimized since a0 is 0 padded

    // convert to gf256x2
    bitpoly_to_gf256x2_n( fft_a0 , fft_a1 , a0 , LEN_INP );

    // first stage of btfy
    gf256v_add( fft_a0+LEN_2 , fft_a0+LEN_2 , fft_a0 , LEN_TAIL );
    gf256v_add( fft_a1+LEN_2 , fft_a1+LEN_2 , fft_a1 , LEN_TAIL );
    memcpy( fft_a0+LEN_INP , fft_a0+LEN_TAIL , LEN_2-LEN_TAIL );
    memcpy( fft_a1+LEN_INP , fft_a1+LEN_TAIL , LEN_2-LEN_TAIL );

    // the rest btfy stages
    btfy_gf256x2( fft_a0 , fft_a1 , LOGLEN_2 , 0 );
    btfy_gf256x2( fft_a0+LEN_2 , fft_a1+LEN_2 , LOGLEN_2 , LEN_2 );

#undef LEN
#undef LEN_INP
#undef LEN_2
#undef LEN_TAIL
#undef LOGLEN_2
}


/// @brief transform an input polynomial in F216[x]/s13 into its evaluated values in F256x2
///        s13 = x^8192 + x^4096 + x^512 + x^256 + x^32 + x^16 + x^2 + x
/// @param fft_a0 [out] evaluated values of the input polynomial in low  byte of F256x2.  size: 65536 bits = 16384 byte
/// @param fft_a1 [out] evaluated values of the input polynomial in high byte of F256x2.  size: 65536 bits = 16384 byte
/// @param poly_a [in] F2 polynomial of 65536 + 6400 bits = (8192+800) bytes
void ringmul_s14_input_8992( uint8_t * fft_a0 , uint8_t * fft_a1 , const uint8_t * poly_a )
{
#define LEN (16384)
#define LEN_INP (8992)
#define LEN_2 (LEN/2)
#define LEN_TAIL (LEN_INP-LEN_2)
#define LOGLEN_2 (13)

    uint8_t a0[LEN] __attribute__ ((aligned (32)));
    memcpy( a0 , poly_a , LEN_INP );
    memset( a0+LEN_INP , 0 , LEN-LEN_INP );

    // input transform
    bc_8( a0 , LEN );  // XXX: can be optimized since a0 is 0 padded

    // convert to gf256x2
    bitpoly_to_gf256x2_n( fft_a0 , fft_a1 , a0 , LEN_INP );

    // first stage of btfy
    gf256v_add( fft_a0+LEN_2 , fft_a0+LEN_2 , fft_a0 , LEN_TAIL );
    gf256v_add( fft_a1+LEN_2 , fft_a1+LEN_2 , fft_a1 , LEN_TAIL );
    memcpy( fft_a0+LEN_INP , fft_a0+LEN_TAIL , LEN_2-LEN_TAIL );
    memcpy( fft_a1+LEN_INP , fft_a1+LEN_TAIL , LEN_2-LEN_TAIL );

    // the rest btfy stages
    btfy_gf256x2( fft_a0 , fft_a1 , LOGLEN_2 , 0 );
    btfy_gf256x2( fft_a0+LEN_2 , fft_a1+LEN_2 , LOGLEN_2 , LEN_2 );

#undef LEN
#undef LEN_INP
#undef LEN_2
#undef LEN_TAIL
#undef LOGLEN_2
}

/// @brief transform an input polynomial in F216[x]/s13 into its evaluated values in F256x2
///        s13 = x^8192 + x^4096 + x^512 + x^256 + x^32 + x^16 + x^2 + x
/// @param fft_a0 [out] evaluated values of the input polynomial in low  byte of F256x2.  size: 65536 bits = 8192 byte
/// @param fft_a1 [out] evaluated values of the input polynomial in high byte of F256x2.  size: 65536 bits = 8192 byte
/// @param poly_a [in] F2 polynomial of 37376 bits = (4096+576) bytes
void ringmul_s13_input_4672( uint8_t * fft_a0 , uint8_t * fft_a1 , const uint8_t * poly_a )
{
#define LEN (8192)
#define LEN_INP (4672)
#define LEN_2 (LEN/2)
#define LEN_TAIL (LEN_INP-LEN_2)
#define LOGLEN_2 (12)

    uint8_t a0[LEN] __attribute__ ((aligned (32)));
    memcpy( a0 , poly_a , LEN_INP );
    memset( a0+LEN_INP , 0 , LEN-LEN_INP );

    // input transform
    bc_8( a0 , LEN );  // XXX: can be optimized since a0 is 0 padded

    // convert to gf256x2
    bitpoly_to_gf256x2_n( fft_a0 , fft_a1 , a0 , LEN_INP );

    // first stage of btfy
    gf256v_add( fft_a0+LEN_2 , fft_a0+LEN_2 , fft_a0 , LEN_TAIL );
    gf256v_add( fft_a1+LEN_2 , fft_a1+LEN_2 , fft_a1 , LEN_TAIL );
    memcpy( fft_a0+LEN_INP , fft_a0+LEN_TAIL , LEN_2-LEN_TAIL );
    memcpy( fft_a1+LEN_INP , fft_a1+LEN_TAIL , LEN_2-LEN_TAIL );

    // the rest btfy stages
    btfy_gf256x2( fft_a0 , fft_a1 , LOGLEN_2 , 0 );
    btfy_gf256x2( fft_a0+LEN_2 , fft_a1+LEN_2 , LOGLEN_2 , LEN_2 );

#undef LEN
#undef LEN_INP
#undef LEN_2
#undef LEN_TAIL
#undef LOGLEN_2
}



///
/// @brief c in F216[x]/s11 = a_fft * b_fft
///
/// @param c0 [out] size : 16384 bits = 2048 byte
/// @param c1 [out] size : 16384 bits = 2048 byte
/// @param a0_fft [in] size : 16384 bits = 2048 byte
/// @param a1_fft [in] size : 16384 bits = 2048 byte
/// @param b0_fft [in] size : 16384 bits = 2048 byte
/// @param b1_fft [in] size : 16384 bits = 2048 byte
void ringmul_s11_mul( uint8_t * c0 , uint8_t * c1 , const uint8_t * a0_fft , const uint8_t * a1_fft , const uint8_t * b0_fft , const uint8_t * b1_fft )
{
#define LEN (2048)
#define LOGLEN (11)

    // multiply
    gf256x2v_mul( c0 , c1 , a0_fft , a1_fft , b0_fft , b1_fft , LEN );
    // output transform
    ibtfy_gf256x2( c0 , c1 , LOGLEN , 0 );
    gf256x2_to_bitpoly_n( c0 , c1 , c0 , c1 , LEN );
    ibc_8( c0 , LEN );
    ibc_8( c1 , LEN );

#undef LEN
#undef LOGLEN
}

///
/// @brief c in F216[x]/s12 = a_fft * b_fft
///
/// @param c0 [out] size : 32768 bits = 4096 byte
/// @param c1 [out] size : 32768 bits = 4096 byte
/// @param a0_fft [in] size : 3278 bits = 4096 byte
/// @param a1_fft [in] size : 3278 bits = 4096 byte
/// @param b0_fft [in] size : 32768 bits = 4096 byte
/// @param b1_fft [in] size : 32768 bits = 4096 byte
void ringmul_s12_mul( uint8_t * c0 , uint8_t * c1 , const uint8_t * a0_fft , const uint8_t * a1_fft , const uint8_t * b0_fft , const uint8_t * b1_fft )
{
#define LEN (4096)
#define LOGLEN (12)

    // multiply
    gf256x2v_mul( c0 , c1 , a0_fft , a1_fft , b0_fft , b1_fft , LEN );
    // output transform
    ibtfy_gf256x2( c0 , c1 , LOGLEN , 0 );
    gf256x2_to_bitpoly_n( c0 , c1 , c0 , c1 , LEN );
    ibc_8( c0 , LEN );
    ibc_8( c1 , LEN );

#undef LEN
#undef LOGLEN
}

///
/// @brief c in F216[x]/s13 = a_fft * b_fft
///
/// @param c0 [out] size : 65536 bits = 8192 byte
/// @param c1 [out] size : 65536 bits = 8192 byte
/// @param a0_fft [in] size : 65536 bits = 8192 byte
/// @param a1_fft [in] size : 65536 bits = 8192 byte
/// @param b0_fft [in] size : 65536 bits = 8192 byte
/// @param b1_fft [in] size : 65536 bits = 8192 byte
void ringmul_s13_mul( uint8_t * c0 , uint8_t * c1 , const uint8_t * a0_fft , const uint8_t * a1_fft , const uint8_t * b0_fft , const uint8_t * b1_fft )
{
#define LEN (8192)
#define LOGLEN (13)

    // multiply
    gf256x2v_mul( c0 , c1 , a0_fft , a1_fft , b0_fft , b1_fft , LEN );
    // output transform
    ibtfy_gf256x2( c0 , c1 , LOGLEN , 0 );
    gf256x2_to_bitpoly_n( c0 , c1 , c0 , c1 , LEN );
    ibc_8( c0 , LEN );
    ibc_8( c1 , LEN );

#undef LEN
#undef LOGLEN
}

///
/// @brief c in F216[x]/s13 = a_fft * b_fft
///
/// @param c0 [out] size : 65536 bits = 16384 byte
/// @param c1 [out] size : 65536 bits = 16384 byte
/// @param a0_fft [in] size : 65536 bits = 16384 byte
/// @param a1_fft [in] size : 65536 bits = 16384 byte
/// @param b0_fft [in] size : 65536 bits = 16384 byte
/// @param b1_fft [in] size : 65536 bits = 16384 byte
void ringmul_s14_mul( uint8_t * c0 , uint8_t * c1 , const uint8_t * a0_fft , const uint8_t * a1_fft , const uint8_t * b0_fft , const uint8_t * b1_fft )
{
#define LEN (16384)
#define LOGLEN (14)

    // multiply
    gf256x2v_mul( c0 , c1 , a0_fft , a1_fft , b0_fft , b1_fft , LEN );
    // output transform
    ibtfy_gf256x2( c0 , c1 , LOGLEN , 0 );
    gf256x2_to_bitpoly_n( c0 , c1 , c0 , c1 , LEN );
    ibc_8( c0 , LEN );
    ibc_8( c1 , LEN );

#undef LEN
#undef LOGLEN
}






