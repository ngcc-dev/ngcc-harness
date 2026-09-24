#include "stdint.h"
#include "string.h"

#include "polymul.h"

#include "benchmark.h"
#include "stdio.h"
#include "ringmul.h"

#include "gf256.h"
///
/// @brief input transform for multiplying two bit-polynomials of size < 24576 bits
///
/// @param a_fft [out] size : 65536 + 16384 bits = 1024+256 u64  = 8192+2048 bytes= 4096+4096+2048 bytes
/// @param a [in] a bit-polynomial of size < 16384 + 8192 bits = 256+128 u64 = 2048+1024 bytes=3072 bytes
void polymul_24576_input( uint64_t * a_fft , const uint64_t * a )
{
    ringmul_s12_input_3072( (uint8_t*)a_fft , ((uint8_t*)a_fft)+4096 ,  (uint8_t*)a );
    memcpy( ((uint8_t*)a_fft)+4096*2 , ((uint8_t*)a) , 2048 );
}


///
/// @brief compute c = a * b. a and b are bit-polynomials of size < 16384+8192 bits.
///
/// @param c [out] product bit-polynomials of size < 32768 + 16384 bits = 768 u64 = 6144=(4096+2048) bytes
/// @param a_fft [in] a transformed bit-polynomial. size : 10240 u64 = 8192+2048 bytes
/// @param b_fft [in] a transformed bit-polynomial. size : 10240 u64 = 8192+2048 bytes
void polymul_24576_mul( uint64_t * c , const uint64_t * a_fft , const uint64_t * b_fft )
{
    polymul_384U64_mul( c , a_fft+(POLYMUL_384U64_FFTSIZE_U64) , b_fft+(POLYMUL_384U64_FFTSIZE_U64) , a_fft , b_fft );
}


///
/// @brief compute c = a * b. a and b are bit-polynomials of size < 16384+8192 bits.
///
/// @param c [out] product bit-polynomials of size < 32768 + 16384 bits = 768 u64 = 6144=(4096+2048) bytes
/// @param a [in] the input bit-polynomial. size : < 384 u64 = 3072 bytes
/// @param b [in] the input bit-polynomial. size : < 384 u64 = 3072 bytes
/// @param a_fft [in] a transformed partial bit-polynomial. size : 1024 u64 = 8192 bytes
/// @param b_fft [in] a transformed partial bit-polynomial. size : 1024 u64 = 8192 bytes
void polymul_384U64_mul( uint64_t * _c , const uint64_t * _a , const uint64_t * _b , const uint64_t * _a_fft , const uint64_t * _b_fft )
{
#define LEN0 (4096)
#define LEN1 (2048)
    uint8_t * c = (uint8_t*)_c;
    uint8_t * a = (uint8_t*)_a;
    uint8_t * b = (uint8_t*)_b;
    const uint8_t * a_fft = (const uint8_t*)_a_fft;
    const uint8_t * b_fft = (const uint8_t*)_b_fft;
    uint8_t pc_m2048_l[LEN1+32];
    uint8_t pc_m2048_h[LEN1+32];
    ringmul_mul_2048( pc_m2048_l , pc_m2048_h , a , b );
    pc_m2048_l[LEN1] = 0;
    pc_m2048_h[LEN1] = 0;
    uint8_t pc_s12_l[LEN0+LEN1+32];
    uint8_t pc_s12_h[LEN0+LEN1+32];
    ringmul_s12_mul( pc_s12_l , pc_s12_h , a_fft , a_fft+LEN0 , b_fft , b_fft+LEN0 );
    memset( pc_s12_l+LEN0 , 0 , LEN1+32 );
    memset( pc_s12_h+LEN0 , 0 , LEN1+32 );

    //static const int invs12[] = {0, 15, 30, 45, 60, 75, 90, 105, 120, 135, 150, 165, 180, 195, 210, 225, 240, 270, 300, 330, 360};

    static const int invs12[] = {0, 15, 30, 45, 60, 75, 90, 105, 120, 135, 150, 165, 180, 195, 210, 225, 240, 270, 300, 330, 360, 390, 420, 450, 480, 525, 540, 585, 600, 645, 660, 705, 720, 780, 840, 900, 960, 1035, 1050, 1065, 1080, 1155, 1170, 1185, 1200, 1290, 1320, 1410, 1440, 1545, 1560, 1665, 1680, 1800, 1920};

    
    uint8_t k_s12_div_x_l[LEN1];
    uint8_t k_s12_div_x_h[LEN1];

    //  #k*(s12/x) = (pc_m2048+pc_ms12)/x   mod  x^2048
    //  k_s12_div_x = xor_list(pc_m2048,pc_ms12)[1:2048+1]
    gf256v_add( k_s12_div_x_l, pc_s12_l+1 , pc_m2048_l+1 , LEN1 );
    gf256v_add( k_s12_div_x_h, pc_s12_h+1 , pc_m2048_h+1 , LEN1 );

    uint8_t k_l[LEN1];
    uint8_t k_h[LEN1];
    // #k = (s12/x)^-1 * k_s12_div_x
    // k = poly_mul_gf2poly( k_s12_div_x , invs12 )[:2048]
    memcpy( k_l , k_s12_div_x_l , LEN1 );
    memcpy( k_h , k_s12_div_x_h , LEN1 );
    for( int i = 1 ; i < (int)(sizeof(invs12)/sizeof(int)) ; i++ ) {
        int raise_deg = invs12[i];
        gf256v_add( k_l+raise_deg , k_l+raise_deg , k_s12_div_x_l , LEN1-raise_deg );
        gf256v_add( k_h+raise_deg , k_h+raise_deg , k_s12_div_x_h , LEN1-raise_deg );
    }

    //# f = pc_ms12 + k*s12   mod (lcm(x^2048, x^4096+x^256+x^16+x))
    //f = xor_list( pc_ms12 , poly_mul_gf2poly( k , [4096,256,16,1] ) )
    uint8_t * f_l= pc_s12_l;
    uint8_t * f_h= pc_s12_h;
    { int r_deg = 1;    gf256v_add( f_l+r_deg , f_l+r_deg , k_l , LEN1 ); gf256v_add( f_h+r_deg , f_h+r_deg , k_h , LEN1 ); }
    { int r_deg = 16;   gf256v_add( f_l+r_deg , f_l+r_deg , k_l , LEN1 ); gf256v_add( f_h+r_deg , f_h+r_deg , k_h , LEN1 ); }
    { int r_deg = 256;  gf256v_add( f_l+r_deg , f_l+r_deg , k_l , LEN1 ); gf256v_add( f_h+r_deg , f_h+r_deg , k_h , LEN1 ); }
    { int r_deg = 4096; gf256v_add( f_l+r_deg , f_l+r_deg , k_l , LEN1 ); gf256v_add( f_h+r_deg , f_h+r_deg , k_h , LEN1 ); }

    // rr = poly_mod_gf2poly( f , [4096+2048-1,256+2048-1,16+2048-1,1+2048-1] )  # lcm(x^2048, x^4096+x^256+x^16+x)
    uint8_t red_l = f_l[LEN0+LEN1-1]; f_l[LEN0+LEN1-1] = 0;
    uint8_t red_h = f_h[LEN0+LEN1-1]; f_h[LEN0+LEN1-1] = 0;
    f_l[256+LEN1-1] ^= red_l;
    f_h[256+LEN1-1] ^= red_h;
    f_l[16+LEN1-1]  ^= red_l;
    f_h[16+LEN1-1]  ^= red_h;
    f_l[1+LEN1-1]   ^= red_l;
    f_h[1+LEN1-1]   ^= red_h;

    c[0] = f_l[0];
    gf256v_add( c+1 , f_l+1 , f_h , LEN0+LEN1-1 );
#undef LEN0
#undef LEN1
}

/////////////////////
///
/// @brief input transform for multiplying two bit-polynomials of size < 19264 bits
///
/// @param a_fft [out] size : 32768 (4*8192)+ 7168(2*3584) bits = 512+112 u64  = 4096+896 bytes= 2048+2048+896 bytes
/// @param a [in] a bit-polynomial of size < 8192+3584 bits = 128+56 u64 = 1024+448 bytes=1472 bytes
void polymul_11776_input( uint64_t * a_fft , const uint64_t * a )
{
    ringmul_s11_input_1472( (uint8_t*)a_fft , ((uint8_t*)a_fft)+2048 ,  (uint8_t*)a );
    memcpy( ((uint8_t*)a_fft)+2048*2 , ((uint8_t*)a) , 896 );
}


///
/// @brief compute c = a * b. a and b are bit-polynomials of size < 8192+3584 bits.
///
/// @param c [out] product bit-polynomials of size < 16384 + 7168 bits = 368 u64 = 2944=(2048+896) bytes
/// @param a_fft [in] a transformed bit-polynomial. size : 624 u64 = 4096+896 bytes
/// @param b_fft [in] a transformed bit-polynomial. size : 624 u64 = 4096+896 bytes
void polymul_11776_mul( uint64_t * c , const uint64_t * a_fft , const uint64_t * b_fft )
{
    polymul_184U64_mul( c , a_fft+(POLYMUL_184U64_FFTSIZE_U64) , b_fft+(POLYMUL_184U64_FFTSIZE_U64) , a_fft , b_fft );
}


///
/// @brief compute c = a * b. a and b are bit-polynomials of size < 16384+7168 bits.
///
/// @param c [out] product bit-polynomials of size < 16384+7168 bits = 256+112 u64 = 2944=(2048+896) bytes
/// @param a [in] the input bit-polynomial. size : < 184 u64 = 1472 bytes
/// @param b [in] the input bit-polynomial. size : < 184 u64 = 1472 bytes
/// @param a_fft [in] a transformed partial bit-polynomial. size : 256 u64 = 2048 bytes
/// @param b_fft [in] a transformed partial bit-polynomial. size : 256 u64 = 2048 bytes
void polymul_184U64_mul( uint64_t * _c , const uint64_t * _a , const uint64_t * _b , const uint64_t * _a_fft , const uint64_t * _b_fft )
{
#define LEN0 (2048)
#define LEN1 (896)
    uint8_t * c = (uint8_t*)_c;
    uint8_t * a = (uint8_t*)_a;
    uint8_t * b = (uint8_t*)_b;
    const uint8_t * a_fft = (const uint8_t*)_a_fft;
    const uint8_t * b_fft = (const uint8_t*)_b_fft;
    uint8_t pc_m896_l[LEN1+32];
    uint8_t pc_m896_h[LEN1+32];
    ringmul_mul_896( pc_m896_l , pc_m896_h , a , b );
    pc_m896_l[LEN1] = 0;
    pc_m896_h[LEN1] = 0;
    uint8_t pc_s11_l[LEN0+LEN1+32];
    uint8_t pc_s11_h[LEN0+LEN1+32];
    ringmul_s11_mul( pc_s11_l , pc_s11_h , a_fft , a_fft+LEN0 , b_fft , b_fft+LEN0 );
    memset( pc_s11_l+LEN0 , 0 , LEN1+32 );
    memset( pc_s11_h+LEN0 , 0 , LEN1+32 );

    
    static const int invs11[] = {0, 1, 2, 4, 8, 15, 16, 17, 19, 23, 30, 31, 32, 34, 38, 45, 46, 47, 49, 53, 60, 61, 62, 64, 68, 75, 76, 77, 79, 83, 90, 91, 92, 94, 98, 105, 106, 107, 109, 113, 120, 121, 122, 124, 128, 135, 136, 137, 139, 143, 150, 151, 152, 154, 158, 165, 166, 167, 169, 173, 180, 181, 182, 184, 188, 195, 196, 197, 199, 203, 210, 211, 212, 214, 218, 225, 226, 227, 229, 233, 240, 241, 242, 244, 248, 256, 270, 272, 274, 278, 286, 300, 302, 304, 308, 316, 330, 332, 334, 338, 346, 360, 362, 364, 368, 376, 390, 392, 394, 398, 406, 420, 422, 424, 428, 436, 450, 452, 454, 458, 466, 480, 482, 484, 488, 496, 512, 525, 529, 533, 540, 541, 544, 548, 556, 557, 572, 585, 589, 593, 600, 601, 604, 608, 616, 617, 632, 645, 649, 653, 660, 661, 664, 668, 676, 677, 692, 705, 709, 713, 720, 721, 724, 728, 736, 737, 752, 780, 784, 788, 796, 812, 840, 844, 848, 856, 872};
    uint8_t k_s11_div_x_l[LEN1];
    uint8_t k_s11_div_x_h[LEN1];
   

    //  #k*(s11/x) = (pc_m896+pc_ms11)/x   mod  x^896
    //  k_s11_div_x = xor_list(pc_m896,pc_ms11)[1:896+1]
    gf256v_add( k_s11_div_x_l, pc_s11_l+1 , pc_m896_l+1 , LEN1 );
    gf256v_add( k_s11_div_x_h, pc_s11_h+1 , pc_m896_h+1 , LEN1 );
    
    uint8_t k_l[LEN1];
    uint8_t k_h[LEN1];
    // #k = (s11/x)^-1 * k_s11_div_x
    // k = poly_mul_gf2poly( k_s11_div_x , invs11 )[:896]
    memcpy( k_l , k_s11_div_x_l , LEN1 );
    memcpy( k_h , k_s11_div_x_h , LEN1 );
    for( int i = 1 ; i < (int)(sizeof(invs11)/sizeof(int)) ; i++ ) {
        int raise_deg = invs11[i];
        gf256v_add( k_l+raise_deg , k_l+raise_deg , k_s11_div_x_l , LEN1-raise_deg );
        gf256v_add( k_h+raise_deg , k_h+raise_deg , k_s11_div_x_h , LEN1-raise_deg );
    }
    
    //# f = pc_ms11+ k*s11   mod (lcm(x^896, s11 = x^2048+x^1024 +x^512+ x^256 + x^8 + x^4+x^2+x))
    //f = xor_list( pc_ms11 , poly_mul_gf2poly( k , [2048,1024,512,256,8,4,2,1] ) )
    uint8_t * f_l= pc_s11_l;
    uint8_t * f_h= pc_s11_h;
    { int r_deg = 1;    gf256v_add( f_l+r_deg , f_l+r_deg , k_l , LEN1 ); gf256v_add( f_h+r_deg , f_h+r_deg , k_h , LEN1 ); }
    { int r_deg = 2;    gf256v_add( f_l+r_deg , f_l+r_deg , k_l , LEN1 ); gf256v_add( f_h+r_deg , f_h+r_deg , k_h , LEN1 ); }
    { int r_deg = 4;    gf256v_add( f_l+r_deg , f_l+r_deg , k_l , LEN1 ); gf256v_add( f_h+r_deg , f_h+r_deg , k_h , LEN1 ); }
    { int r_deg = 8;    gf256v_add( f_l+r_deg , f_l+r_deg , k_l , LEN1 ); gf256v_add( f_h+r_deg , f_h+r_deg , k_h , LEN1 ); }
    { int r_deg = 256;    gf256v_add( f_l+r_deg , f_l+r_deg , k_l , LEN1 ); gf256v_add( f_h+r_deg , f_h+r_deg , k_h , LEN1 ); }
    { int r_deg = 512;    gf256v_add( f_l+r_deg , f_l+r_deg , k_l , LEN1 ); gf256v_add( f_h+r_deg , f_h+r_deg , k_h , LEN1 ); }
    { int r_deg = 1024;    gf256v_add( f_l+r_deg , f_l+r_deg , k_l , LEN1 ); gf256v_add( f_h+r_deg , f_h+r_deg , k_h , LEN1 ); }
    { int r_deg = 2048;    gf256v_add( f_l+r_deg , f_l+r_deg , k_l , LEN1 ); gf256v_add( f_h+r_deg , f_h+r_deg , k_h , LEN1 ); }
    
    // rr = poly_mod_gf2poly( f , [2048+896-1,256+896-1,8+896-1,4+896-1,2+896-1,1+896-1] )  # lcm(x^896, x^2048+x^256+x^8+x^4+x^2+x)
    uint8_t red_l = f_l[LEN0+LEN1-1]; f_l[LEN0+LEN1-1] = 0;
    uint8_t red_h = f_h[LEN0+LEN1-1]; f_h[LEN0+LEN1-1] = 0;
    f_l[1024+LEN1-1] ^= red_l;
    f_h[1024+LEN1-1] ^= red_h;
    f_l[512+LEN1-1] ^= red_l;
    f_h[512+LEN1-1] ^= red_h;
    f_l[256+LEN1-1] ^= red_l;
    f_h[256+LEN1-1] ^= red_h;
    f_l[8+LEN1-1]  ^= red_l;
    f_h[8+LEN1-1]  ^= red_h;
    f_l[4+LEN1-1]  ^= red_l;
    f_h[4+LEN1-1]  ^= red_h;
    f_l[2+LEN1-1]  ^= red_l;
    f_h[2+LEN1-1]  ^= red_h;
    f_l[1+LEN1-1]   ^= red_l;
    f_h[1+LEN1-1]   ^= red_h;
    
    c[0] = f_l[0];
    gf256v_add( c+1 , f_l+1 , f_h , LEN0+LEN1-1 );
#undef LEN0
#undef LEN1

}


////////////////////////

/// @brief input transform for multiplying two bit-polynomials of size < 37376 bits
///
/// @param a_fft [out] size : 131072 + 9216 bits = 2048+144 u64 = 16384+1152 bytes
/// @param a [in] a bit-polynomial of size < 32768 + 4608 bits = 584 u64 = 4096+576 bytes
void polymul_37376_input( uint64_t * a_fft , const uint64_t * a )
{
    ringmul_s13_input_4672( (uint8_t*)a_fft , ((uint8_t*)a_fft)+8192 ,  (uint8_t*)a );
    memcpy( ((uint8_t*)a_fft)+8192*2 , ((uint8_t*)a) , 1152 );
}



/// @brief input transform for multiplying two bit-polynomials of size < 71936 bits
///
/// @param a_fft [out] size : 262144 + 12800 bits = 4096+200 u64 = 32768+1600 bytes
/// @param a [in] a bit-polynomial of size < 65536 + 6400 bits = 1124 u64 = 8192+800 bytes
void polymul_71936_input( uint64_t * a_fft , const uint64_t * a )
{
    ringmul_s14_input_8992( (uint8_t*)a_fft , ((uint8_t*)a_fft)+16384 ,  (uint8_t*)a );
    memcpy( ((uint8_t*)a_fft)+16384*2 , ((uint8_t*)a) , 1600 );
}

///
/// @brief compute c = a * b. a and b are bit-polynomials of size < 32768+4096 bits.
///
/// @param c [out] product bit-polynomials of size < 65536 + 9216 bits = 1168 u64 = 8192+1152 bytes
/// @param a_fft [in] a transformed bit-polynomial. size : 2176 u64 = 16384+1152 bytes
/// @param b_fft [in] a transformed bit-polynomial. size : 2176 u64 = 16384+1152 bytes
void polymul_37376_mul( uint64_t * c , const uint64_t * a_fft , const uint64_t * b_fft )
{
    polymul_584U64_mul( c , a_fft+(POLYMUL_584U64_FFTSIZE_U64) , b_fft+(POLYMUL_584U64_FFTSIZE_U64) , a_fft , b_fft );
}

///
/// @brief compute c = a * b. a and b are bit-polynomials of size < 32768+4096 bits.
///
/// @param c [out] product bit-polynomials of size < 131072 + 12800 bits = 2248 u64 = 16384+1600 bytes
/// @param a_fft [in] a transformed bit-polynomial. size : 4296 u64 = 32768+1600 bytes
/// @param b_fft [in] a transformed bit-polynomial. size : 4296 u64 = 32768+1600 bytes
void polymul_71936_mul( uint64_t * c , const uint64_t * a_fft , const uint64_t * b_fft )
{
    polymul_1124U64_mul( c , a_fft+(POLYMUL_1124U64_FFTSIZE_U64) , b_fft+(POLYMUL_1124U64_FFTSIZE_U64) , a_fft , b_fft );
}



///
/// @brief compute c = a * b. a and b are bit-polynomials of size < 65536+6400 bits.
///
/// @param c [out] product bit-polynomials of size < 2048+20 u64 = (16384+1600) bytes
/// @param a [in] the input bit-polynomial. size : < 1024+200 u64 = 8992 bytes
/// @param b [in] the input bit-polynomial. size : < 1024+200 u64 = 8992 bytes
/// @param a_fft [in] a transformed partial bit-polynomial. size : 4096 u64 = 32768 bytes
/// @param b_fft [in] a transformed partial bit-polynomial. size : 4096 u64 = 32768 bytes
void polymul_1124U64_mul( uint64_t * _c , const uint64_t * _a , const uint64_t * _b , const uint64_t * _a_fft , const uint64_t * _b_fft )
{
#define LEN0 (16384)
#define LEN1 (1600)
    uint8_t * c = (uint8_t*)_c;
    uint8_t * a = (uint8_t*)_a;
    uint8_t * b = (uint8_t*)_b;
    const uint8_t * a_fft = (const uint8_t*)_a_fft;
    const uint8_t * b_fft = (const uint8_t*)_b_fft;
    uint8_t pc_m1600_l[LEN1+32];
    uint8_t pc_m1600_h[LEN1+32];
    ringmul_mul_1600( pc_m1600_l , pc_m1600_h , a , b );
    pc_m1600_l[LEN1] = 0;
    pc_m1600_h[LEN1] = 0;
    uint8_t pc_s14_l[LEN0+LEN1+32];
    uint8_t pc_s14_h[LEN0+LEN1+32];
    ringmul_s14_mul( pc_s14_l , pc_s14_h , a_fft , a_fft+LEN0 , b_fft , b_fft+LEN0 );
    memset( pc_s14_l+LEN0 , 0 , LEN1+32 );
    memset( pc_s14_h+LEN0 , 0 , LEN1+32 );

    static const int invs14[] = {0, 3, 6, 9, 12, 18, 24, 33, 36, 48, 66, 72, 96, 129, 132, 144, 192, 258, 264, 288, 384, 513, 516, 528, 576, 768, 1026, 1032, 1056, 1152, 1536};
    uint8_t k_s14_div_x_l[LEN1];
    uint8_t k_s14_div_x_h[LEN1];

    //  #k*(s14/x) = (pc_m1600+pc_ms14)/x   mod  x^1600
    //  k_s14_div_x = xor_list(pc_m1600,pc_ms14)[1:1600+1]
    gf256v_add( k_s14_div_x_l, pc_s14_l+1 , pc_m1600_l+1 , LEN1 );
    gf256v_add( k_s14_div_x_h, pc_s14_h+1 , pc_m1600_h+1 , LEN1 );

    uint8_t k_l[LEN1];
    uint8_t k_h[LEN1];
    // #k = (s14/x)^-1 * k_s14_div_x
    // k = poly_mul_gf2poly( k_s14_div_x , invs14 )[:1664]
    memcpy( k_l , k_s14_div_x_l , LEN1 );
    memcpy( k_h , k_s14_div_x_h , LEN1 );
    for( int i = 1 ; i < (int)(sizeof(invs14)/sizeof(int)) ; i++ ) {
        int raise_deg = invs14[i];
        gf256v_add( k_l+raise_deg , k_l+raise_deg , k_s14_div_x_l , LEN1-raise_deg );
        gf256v_add( k_h+raise_deg , k_h+raise_deg , k_s14_div_x_h , LEN1-raise_deg );
    }

    //# f = pc_ms14 + k*s14   mod (lcm(x^1664, s13))
    //f = xor_list( pc_ms13 , poly_mul_gf2poly( k , [16384,4096,1024,256,64,16,4,1] ) )
    uint8_t * f_l= pc_s14_l;
    uint8_t * f_h= pc_s14_h;
    { int r_deg = 1;     gf256v_add( f_l+r_deg , f_l+r_deg , k_l , LEN1 ); gf256v_add( f_h+r_deg , f_h+r_deg , k_h , LEN1 ); }
    { int r_deg = 4;     gf256v_add( f_l+r_deg , f_l+r_deg , k_l , LEN1 ); gf256v_add( f_h+r_deg , f_h+r_deg , k_h , LEN1 ); }
    { int r_deg = 16;    gf256v_add( f_l+r_deg , f_l+r_deg , k_l , LEN1 ); gf256v_add( f_h+r_deg , f_h+r_deg , k_h , LEN1 ); }
    { int r_deg = 64;    gf256v_add( f_l+r_deg , f_l+r_deg , k_l , LEN1 ); gf256v_add( f_h+r_deg , f_h+r_deg , k_h , LEN1 ); }
    { int r_deg = 256;   gf256v_add( f_l+r_deg , f_l+r_deg , k_l , LEN1 ); gf256v_add( f_h+r_deg , f_h+r_deg , k_h , LEN1 ); }
    { int r_deg = 1024;  gf256v_add( f_l+r_deg , f_l+r_deg , k_l , LEN1 ); gf256v_add( f_h+r_deg , f_h+r_deg , k_h , LEN1 ); }
    { int r_deg = 4096;  gf256v_add( f_l+r_deg , f_l+r_deg , k_l , LEN1 ); gf256v_add( f_h+r_deg , f_h+r_deg , k_h , LEN1 ); }
    { int r_deg = 16384; gf256v_add( f_l+r_deg , f_l+r_deg , k_l , LEN1 ); gf256v_add( f_h+r_deg , f_h+r_deg , k_h , LEN1 ); }

    // rr = poly_mod_gf2poly( f , [16384+1600-1,4096+1600-1,1024+1600-1,256+1600-1,64+1600-1,16+1600-1,4+1600-1,1+1600-1] )  # lcm(x^1600, s14 )
    uint8_t red_l = f_l[LEN0+LEN1-1]; f_l[LEN0+LEN1-1] = 0;
    uint8_t red_h = f_h[LEN0+LEN1-1]; f_h[LEN0+LEN1-1] = 0;
    f_l[4096+LEN1-1] ^= red_l;
    f_h[4096+LEN1-1] ^= red_h;
    f_l[1024+LEN1-1] ^= red_l;
    f_h[1024+LEN1-1] ^= red_h;
    f_l[256+LEN1-1] ^= red_l;
    f_h[256+LEN1-1] ^= red_h;
    f_l[64+LEN1-1] ^= red_l;
    f_h[64+LEN1-1] ^= red_h;
    f_l[16+LEN1-1] ^= red_l;
    f_h[16+LEN1-1] ^= red_h;
    f_l[4+LEN1-1] ^= red_l;
    f_h[4+LEN1-1] ^= red_h;
    f_l[1+LEN1-1] ^= red_l;
    f_h[1+LEN1-1] ^= red_h;

    c[0] = f_l[0];
    gf256v_add( c+1 , f_l+1 , f_h , LEN0+LEN1-1 );
#undef LEN0
#undef LEN1
}


///
/// @brief compute c = a * b. a and b are bit-polynomials of size < 32768+4608 bits.
///
/// @param c [out] product bit-polynomials of size < 1024+144 u64 = 9344=(8192+1152) bytes
/// @param a [in] the input bit-polynomial. size : < 512+64 u64 = 4608 bytes
/// @param b [in] the input bit-polynomial. size : < 512+64 u64 = 4608 bytes
/// @param a_fft [in] a transformed partial bit-polynomial. size : 2048 u64 = 16384 bytes
/// @param b_fft [in] a transformed partial bit-polynomial. size : 2048 u64 = 16384 bytes
void polymul_584U64_mul( uint64_t * _c , const uint64_t * _a , const uint64_t * _b , const uint64_t * _a_fft , const uint64_t * _b_fft )
{
#define LEN0 (8192)
#define LEN1 (1152)
    uint8_t * c = (uint8_t*)_c;
    uint8_t * a = (uint8_t*)_a;
    uint8_t * b = (uint8_t*)_b;
    const uint8_t * a_fft = (const uint8_t*)_a_fft;
    const uint8_t * b_fft = (const uint8_t*)_b_fft;
    uint8_t pc_m1152_l[LEN1+32];
    uint8_t pc_m1152_h[LEN1+32];
    //////////////////////////////////////////////////////////
    
    ringmul_mul_1152( pc_m1152_l , pc_m1152_h , a , b );
    pc_m1152_l[LEN1] = 0;
    pc_m1152_h[LEN1] = 0;
    uint8_t pc_s13_l[LEN0+LEN1+32];
    uint8_t pc_s13_h[LEN0+LEN1+32];
    ringmul_s13_mul( pc_s13_l , pc_s13_h , a_fft , a_fft+LEN0 , b_fft , b_fft+LEN0 );
    memset( pc_s13_l+LEN0 , 0 , LEN1+32 );
    memset( pc_s13_h+LEN0 , 0 , LEN1+32 );

    static const int invs13[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 16, 18, 20, 22, 24, 26, 28, 32, 33, 36, 37, 40, 41, 44, 48, 52, 56, 64, 65, 66, 67, 72, 73, 74, 80, 82, 88, 96, 97, 104, 112, 128, 129, 130, 131, 132, 133, 134, 144, 146, 148, 160, 161, 164, 176, 192, 193, 194, 208, 224, 256, 258, 260, 262, 264, 266, 268, 288, 292, 296, 320, 322, 328, 352, 384, 386, 388, 416, 448, 512, 513, 516, 517, 520, 521, 524, 528, 532, 536, 576, 577, 584, 592, 640, 641, 644, 656, 704, 768, 772, 776, 832, 896, 1024, 1025, 1026, 1027, 1032, 1033, 1034, 1040, 1042, 1048, 1056, 1057, 1064, 1072};
    uint8_t k_s13_div_x_l[LEN1];
    uint8_t k_s13_div_x_h[LEN1];

    //  #k*(s13/x) = (pc_m1152+pc_ms13)/x   mod  x^1152
    //  k_s13_div_x = xor_list(pc_m1152,pc_ms13)[1:1152+1]
    gf256v_add( k_s13_div_x_l, pc_s13_l+1 , pc_m1152_l+1 , LEN1 );
    gf256v_add( k_s13_div_x_h, pc_s13_h+1 , pc_m1152_h+1 , LEN1 );

    uint8_t k_l[LEN1];
    uint8_t k_h[LEN1];
    // #k = (s13/x)^-1 * k_s13_div_x
    // k = poly_mul_gf2poly( k_s13_div_x , invs13 )[:1152]
    memcpy( k_l , k_s13_div_x_l , LEN1 );
    memcpy( k_h , k_s13_div_x_h , LEN1 );
    for( int i = 1 ; i < (int)(sizeof(invs13)/sizeof(int)) ; i++ ) {
        int raise_deg = invs13[i];
        gf256v_add( k_l+raise_deg , k_l+raise_deg , k_s13_div_x_l , LEN1-raise_deg );
        gf256v_add( k_h+raise_deg , k_h+raise_deg , k_s13_div_x_h , LEN1-raise_deg );
    }

    //# f = pc_ms13 + k*s13   mod (lcm(x^1024, s13))
    //f = xor_list( pc_ms13 , poly_mul_gf2poly( k , [8192,4096,512,256,32,16,2,1] ) )
    uint8_t * f_l= pc_s13_l;
    uint8_t * f_h= pc_s13_h;
    { int r_deg = 1;    gf256v_add( f_l+r_deg , f_l+r_deg , k_l , LEN1 ); gf256v_add( f_h+r_deg , f_h+r_deg , k_h , LEN1 ); }
    { int r_deg = 2;    gf256v_add( f_l+r_deg , f_l+r_deg , k_l , LEN1 ); gf256v_add( f_h+r_deg , f_h+r_deg , k_h , LEN1 ); }
    { int r_deg = 16;   gf256v_add( f_l+r_deg , f_l+r_deg , k_l , LEN1 ); gf256v_add( f_h+r_deg , f_h+r_deg , k_h , LEN1 ); }
    { int r_deg = 32;   gf256v_add( f_l+r_deg , f_l+r_deg , k_l , LEN1 ); gf256v_add( f_h+r_deg , f_h+r_deg , k_h , LEN1 ); }
    { int r_deg = 256;  gf256v_add( f_l+r_deg , f_l+r_deg , k_l , LEN1 ); gf256v_add( f_h+r_deg , f_h+r_deg , k_h , LEN1 ); }
    { int r_deg = 512;  gf256v_add( f_l+r_deg , f_l+r_deg , k_l , LEN1 ); gf256v_add( f_h+r_deg , f_h+r_deg , k_h , LEN1 ); }
    { int r_deg = 4096; gf256v_add( f_l+r_deg , f_l+r_deg , k_l , LEN1 ); gf256v_add( f_h+r_deg , f_h+r_deg , k_h , LEN1 ); }
    { int r_deg = 8192; gf256v_add( f_l+r_deg , f_l+r_deg , k_l , LEN1 ); gf256v_add( f_h+r_deg , f_h+r_deg , k_h , LEN1 ); }

    // rr = poly_mod_gf2poly( f , [4096+384-1,256+384-1,16+384-1,1+384-1] )  # lcm(x^1152, s13 )
    uint8_t red_l = f_l[LEN0+LEN1-1]; f_l[LEN0+LEN1-1] = 0;
    uint8_t red_h = f_h[LEN0+LEN1-1]; f_h[LEN0+LEN1-1] = 0;
    f_l[4096+LEN1-1] ^= red_l;
    f_h[4096+LEN1-1] ^= red_h;
    f_l[512+LEN1-1] ^= red_l;
    f_h[512+LEN1-1] ^= red_h;
    f_l[256+LEN1-1] ^= red_l;
    f_h[256+LEN1-1] ^= red_h;
    f_l[32+LEN1-1] ^= red_l;
    f_h[32+LEN1-1] ^= red_h;
    f_l[16+LEN1-1] ^= red_l;
    f_h[16+LEN1-1] ^= red_h;
    f_l[2+LEN1-1] ^= red_l;
    f_h[2+LEN1-1] ^= red_h;
    f_l[1+LEN1-1] ^= red_l;
    f_h[1+LEN1-1] ^= red_h;

    c[0] = f_l[0];
    gf256v_add( c+1 , f_l+1 , f_h , LEN0+LEN1-1 );
#undef LEN0
#undef LEN1
}



