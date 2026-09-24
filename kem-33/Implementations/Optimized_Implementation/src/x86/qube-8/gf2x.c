/**
 * \file gf2x.c
 * \brief AVX2 implementation of multiplication of two polynomials
 */

#include "gf2x.h"
#include <immintrin.h>
#include <stdint.h>
#include <string.h>
#include "parameters.h"
#include <stdio.h>
#include "polymul.h"
#include "benchmark.h"
#include "btfy.h"
#include "gf264.h"
#include "dencoder.h"
#include "bc_1.h"
//145451 == PARAM_N  147456 == 131072 + 16384 == 2*65536 +4*4096
#define R_FFTFORM_BYTES  (65536 + 4096) 
#define N_PAD_U64  (PARAM_N_MULT>>6) 
#define N_MOD_64   (PARAM_N%64)


/**
 * @brief Compute C(x) = A(x)*B(x)
 * A(x) and B(x) are stored in 128-bit registers
 * This function computes A(x)*B(x) using Karatsuba
 *
 * @param[out] C Pointer to the result
 * @param[in] A Pointer to the polynomial A(x)
 * @param[in] B Pointer to the polynomial B(x)
 */
static inline void karat_mult_1(__m128i *C, __m128i *A, __m128i *B) {
    __m128i D1[2];
    __m128i D0[2], D2[2];
    __m128i Al = _mm_loadu_si128(A);
    __m128i Ah = _mm_loadu_si128(A + 1);
    __m128i Bl = _mm_loadu_si128(B);
    __m128i Bh = _mm_loadu_si128(B + 1);

    //	Compute Al.Bl=D0
    __m128i DD0 = _mm_clmulepi64_si128(Al, Bl, 0);
    __m128i DD2 = _mm_clmulepi64_si128(Al, Bl, 0x11);
    __m128i AAlpAAh = _mm_xor_si128(Al, _mm_shuffle_epi32(Al, 0x4e));
    __m128i BBlpBBh = _mm_xor_si128(Bl, _mm_shuffle_epi32(Bl, 0x4e));
    __m128i DD1 = _mm_xor_si128(_mm_xor_si128(DD0, DD2), _mm_clmulepi64_si128(AAlpAAh, BBlpBBh, 0));
    D0[0] = _mm_xor_si128(DD0, _mm_unpacklo_epi64(_mm_setzero_si128(), DD1));
    D0[1] = _mm_xor_si128(DD2, _mm_unpackhi_epi64(DD1, _mm_setzero_si128()));

    //	Compute Ah.Bh=D2
    DD0 = _mm_clmulepi64_si128(Ah, Bh, 0);
    DD2 = _mm_clmulepi64_si128(Ah, Bh, 0x11);
    AAlpAAh = _mm_xor_si128(Ah, _mm_shuffle_epi32(Ah, 0x4e));
    BBlpBBh = _mm_xor_si128(Bh, _mm_shuffle_epi32(Bh, 0x4e));
    DD1 = _mm_xor_si128(_mm_xor_si128(DD0, DD2), _mm_clmulepi64_si128(AAlpAAh, BBlpBBh, 0));
    D2[0] = _mm_xor_si128(DD0, _mm_unpacklo_epi64(_mm_setzero_si128(), DD1));
    D2[1] = _mm_xor_si128(DD2, _mm_unpackhi_epi64(DD1, _mm_setzero_si128()));

    // Compute AlpAh.BlpBh=D1
    // Initialisation of AlpAh and BlpBh
    __m128i AlpAh = _mm_xor_si128(Al, Ah);
    __m128i BlpBh = _mm_xor_si128(Bl, Bh);
    DD0 = _mm_clmulepi64_si128(AlpAh, BlpBh, 0);
    DD2 = _mm_clmulepi64_si128(AlpAh, BlpBh, 0x11);
    AAlpAAh = _mm_xor_si128(AlpAh, _mm_shuffle_epi32(AlpAh, 0x4e));
    BBlpBBh = _mm_xor_si128(BlpBh, _mm_shuffle_epi32(BlpBh, 0x4e));
    DD1 = _mm_xor_si128(_mm_xor_si128(DD0, DD2), _mm_clmulepi64_si128(AAlpAAh, BBlpBBh, 0));
    D1[0] = _mm_xor_si128(DD0, _mm_unpacklo_epi64(_mm_setzero_si128(), DD1));
    D1[1] = _mm_xor_si128(DD2, _mm_unpackhi_epi64(DD1, _mm_setzero_si128()));

    // Final comutation of C
    __m128i middle = _mm_xor_si128(D0[1], D2[0]);
    C[0] = D0[0];
    C[1] = middle ^ D0[0] ^ D1[0];
    C[2] = middle ^ D1[1] ^ D2[1];
    C[3] = D2[1];
}

/**
 * @brief Compute C(x) = A(x)*B(x)
 *
 * This function computes A(x)*B(x) using Karatsuba
 * A(x) and B(x) are stored in 256-bit registers
 * @param[out] C Pointer to the result
 * @param[in] A Pointer to the polynomial A(x)
 * @param[in] B Pointer to the polynomial B(x)
 */
static inline void karat_mult_2(__m256i *C, __m256i *A, __m256i *B) {
    __m256i D0[2], D1[2], D2[2], SAA, SBB;
    __m128i *A128 = (__m128i *)A, *B128 = (__m128i *)B;

    karat_mult_1((__m128i *)D0, A128, B128);
    karat_mult_1((__m128i *)D2, A128 + 2, B128 + 2);

    SAA = _mm256_xor_si256(A[0], A[1]);
    SBB = _mm256_xor_si256(B[0], B[1]);

    karat_mult_1((__m128i *)D1, (__m128i *)&SAA, (__m128i *)&SBB);
    __m256i middle = _mm256_xor_si256(D0[1], D2[0]);

    C[0] = D0[0];
    C[1] = middle ^ D0[0] ^ D1[0];
    C[2] = middle ^ D1[1] ^ D2[1];
    C[3] = D2[1];
}

/**
 * @brief Compute C(x) = A(x)*B(x)
 *
 * This function computes A(x)*B(x) using Karatsuba
 * A(x) and B(x) are stored in 256-bit registers
 * @param[out] C Pointer to the result
 * @param[in] A Pointer to the polynomial A(x)
 * @param[in] B Pointer to the polynomial B(x)
 */
static inline void karat_mult_4(__m256i *C, __m256i *A, __m256i *B) {
    __m256i D0[4], D1[4], D2[4], SAA[2], SBB[2];

    karat_mult_2(D0, A, B);
    karat_mult_2(D2, A + 2, B + 2);

    SAA[0] = A[0] ^ A[2];
    SBB[0] = B[0] ^ B[2];
    SAA[1] = A[1] ^ A[3];
    SBB[1] = B[1] ^ B[3];

    karat_mult_2(D1, SAA, SBB);

    __m256i middle0 = _mm256_xor_si256(D0[2], D2[0]);
    __m256i middle1 = _mm256_xor_si256(D0[3], D2[1]);

    C[0] = D0[0];
    C[1] = D0[1];
    C[2] = middle0 ^ D0[0] ^ D1[0];
    C[3] = middle1 ^ D0[1] ^ D1[1];
    C[4] = middle0 ^ D1[2] ^ D2[2];
    C[5] = middle1 ^ D1[3] ^ D2[3];
    C[6] = D2[2];
    C[7] = D2[3];
}

/**
 * @brief Compute C(x) = A(x)*B(x)
 *
 * This function computes A(x)*B(x) using Karatsuba
 * A(x) and B(x) are stored in 256-bit registers
 * @param[out] C Pointer to the result
 * @param[in] A Pointer to the polynomial A(x)
 * @param[in] B Pointer to the polynomial B(x)
 */
static inline void karat_mult_8(__m256i *C, __m256i *A, __m256i *B) {
    __m256i D0[8], D1[8], D2[8], SAA[4], SBB[4];

    karat_mult_4(D0, A, B);
    karat_mult_4(D2, A + 4, B + 4);

    for (int32_t i = 0; i < 4; i++) {
        int32_t is = i + 4;
        SAA[i] = A[i] ^ A[is];
        SBB[i] = B[i] ^ B[is];
    }

    karat_mult_4(D1, SAA, SBB);

    for (int32_t i = 0; i < 4; i++) {
        int32_t is = i + 4;
        int32_t is2 = is + 4;
        int32_t is3 = is2 + 4;

        __m256i middle = _mm256_xor_si256(D0[is], D2[i]);

        C[i] = D0[i];
        C[is] = middle ^ D0[i] ^ D1[i];
        C[is2] = middle ^ D1[is] ^ D2[is];
        C[is3] = D2[is];
    }
}

/**
 * @brief Compute C(x) = A(x)*B(x)
 *
 * This function computes A(x)*B(x) using Karatsuba
 * A(x) and B(x) are stored in 256-bit registers
 * @param[out] C Pointer to the result
 * @param[in] A Pointer to the polynomial A(x)
 * @param[in] B Pointer to the polynomial B(x)
 */
static inline void karat_mult_16(__m256i *C, __m256i *A, __m256i *B) {
    __m256i D0[16], D1[16], D2[16], SAA[8], SBB[8];

    karat_mult_8(D0, A, B);
    karat_mult_8(D2, A + 8, B + 8);

    for (int32_t i = 0; i < 8; i++) {
        int32_t is = i + 8;
        SAA[i] = A[i] ^ A[is];
        SBB[i] = B[i] ^ B[is];
    }

    karat_mult_8(D1, SAA, SBB);

    for (int32_t i = 0; i < 8; i++) {
        int32_t is = i + 8;
        int32_t is2 = is + 8;
        int32_t is3 = is2 + 8;

        __m256i middle = _mm256_xor_si256(D0[is], D2[i]);

        C[i] = D0[i];
        C[is] = middle ^ D0[i] ^ D1[i];
        C[is2] = middle ^ D1[is] ^ D2[is];
        C[is3] = D2[is];
    }
}

/**
 * @brief Compute C(x) = A(x)*B(x)
 *
 * This function computes A(x)*B(x) using Karatsuba
 * A(x) and B(x) are stored in 256-bit registers
 * @param[out] C Pointer to the result
 * @param[in] A Pointer to the polynomial A(x)
 * @param[in] B Pointer to the polynomial B(x)
 */
static inline void karat_mult_32(__m256i *C, __m256i *A, __m256i *B) {
    __m256i D0[32], D1[32], D2[32], SAA[16], SBB[16];

    karat_mult_16(D0, A, B);
    karat_mult_16(D2, A + 16, B + 16);

    for (int32_t i = 0; i < 16; i++) {
        int32_t is = i + 16;
        SAA[i] = A[i] ^ A[is];
        SBB[i] = B[i] ^ B[is];
    }

    karat_mult_16(D1, SAA, SBB);

    for (int32_t i = 0; i < 16; i++) {
        int32_t is = i + 16;
        int32_t is2 = is + 16;
        int32_t is3 = is2 + 16;

        __m256i middle = _mm256_xor_si256(D0[is], D2[i]);

        C[i] = D0[i];
        C[is] = middle ^ D0[i] ^ D1[i];
        C[is2] = middle ^ D1[is] ^ D2[is];
        C[is3] = D2[is];
    }
}

/**
 * @brief Compute C(x) = A(x)*B(x)
 *
 * This function computes A(x)*B(x) using Karatsuba
 * A(x) and B(x) are stored in 256-bit registers
 * @param[out] C Pointer to the result
 * @param[in] A Pointer to the polynomial A(x)
 * @param[in] B Pointer to the polynomial B(x)
 */
static inline void karat_mult_64(__m256i *C, __m256i *A, __m256i *B) {
    __m256i D0[64], D1[64], D2[64], SAA[32], SBB[32];

    karat_mult_32(D0, A, B);
    karat_mult_32(D2, A + 32, B + 32);

    for (int32_t i = 0; i < 32; i++) {
        int32_t is = i + 32;
        SAA[i] = A[i] ^ A[is];
        SBB[i] = B[i] ^ B[is];
    }

    karat_mult_32(D1, SAA, SBB);

    for (int32_t i = 0; i < 32; i++) {
        int32_t is = i + 32;
        int32_t is2 = is + 32;
        int32_t is3 = is2 + 32;

        __m256i middle = _mm256_xor_si256(D0[is], D2[i]);

        C[i] = D0[i];
        C[is] = middle ^ D0[i] ^ D1[i];
        C[is2] = middle ^ D1[is] ^ D2[is];
        C[is3] = D2[is];
    }
}

///
/// @brief input transform for multiplying two bit-polynomials of size < (32768 + 16384) bits
///
/// @param a_fft [out] size : transformed values of the partial input polynomial: 65536=1024 u64
/// @param a [in] a bit-polynomial of size < 32768 + 16384 = 768 u64 = 512+256 u64
static inline void polymul_2304U64_input( uint64_t * a_fft , const uint64_t * a )
{
	uint64_t fft_end[2048];             // 2048 u64
    memcpy( fft_end , a , 2048*8 );     // 2048 u64
    bc_1( fft_end , 2048*8 );           // 2048 u64
    encode_64(a_fft,4096,fft_end,32);   // 4096 = 2*2048
    btfy_64(a_fft,11+1,1ULL<<(32+11+1));// 11=log2048
}

///
/// @brief input transform for multiplying two bit-polynomials of size < 49152 bits
///
/// @param a_fft [out] size : 262144 bits + 2304 u64 = 4096 u64 + 2304 u64
/// @param a [in] a bit-polynomial of size < 131072 + 16384 = 2304 u64
static inline void polymul_147456_input( uint64_t * a_fft , const uint64_t * a )
{
    polymul_2304U64_input( a_fft , a );
    uint64_t * fft_end = a_fft + 4096;  // offset 4096 u64
    memcpy( fft_end , a , 2304*8 ); // remain 2304u64
}

///
/// @brief compute c = a * b. a and b are bit-polynomials of size < 32768 + 16384 bits.
///
/// @param c [out] product bit-polynomials of size < 65536 + 32768 bits
/// @param a [in] the input bit-polynomial. size : < 768 u64
/// @param b [in] the input bit-polynomial. size : < 768 u64
/// @param a_fft [in] a transformed partial bit-polynomial. size : 1024 u64
/// @param b_fft [in] a transformed partial bit-polynomial. size : 1024 u64
static inline void polymul_2304U64_mul( uint64_t * c , const uint64_t * a , const uint64_t * b , const uint64_t * a_fft , const uint64_t * b_fft )
{
	uint64_t temp[4096];                       //lenth 8192 u64
    gf264v_mul( temp , a_fft , b_fft , 4096 ); 
    // output transform
    ibtfy_64( temp,11+1,1ULL<<(32+11+1));
    decode_64( c , temp , 4096 );
    ibc_1( c , 4096*8 );

    __m256i _a[576]; memcpy( _a , a , 576*32 ); //576 u64
    __m256i _b[576]; memcpy( _b , b , 576*32 );
    
    const __m256i * a_poly = _a;
    const __m256i * b_poly = _b;
    const __m256i * a_rem = a_poly + 512;  //split 576 to 512 + 64
    const __m256i * b_rem = b_poly + 512; 

	__m256i tmp0[128]; // 128 for 2 64-degree multiplication
    __m256i tmp1[128];
    __m256i * cc = (__m256i*) (c); // c=a*b 

    {
        karat_mult_64( tmp0 , a_rem , b_rem ); //lenth 64 u256
        for (int j=0;j<128;j++) { _mm256_storeu_si256( cc+1024+j , tmp0[j] ); } //hightest 128-degree
    }
    for( int i=0;i<512;i+=64) { //a_poly is a 128-degree polynomal 
        karat_mult_64( tmp0 , a_poly + i , b_rem );//lenth 64 u256
        karat_mult_64( tmp1 , b_poly + i , a_rem );
        for (int j=0;j<128;j++) {
            __m256i tmp = _mm256_loadu_si256( cc+512+i+j ) ^ tmp0[j]^tmp1[j]; //128 is a block = 2 64-degree multiplication
            _mm256_storeu_si256( cc+512+i+j , tmp );
        }
    }

}

///
/// @brief compute c = a * b. a and b are bit-polynomials of size < 32768 + 16384 bits.
///
/// @param c [out] product bit-polynomials of size < 65536 +32768 bits
/// @param a_fft [in] a transformed bit-polynomial. size : 1696 u64 = 1024 u64 + 768 u64
/// @param b_fft [in] a transformed bit-polynomial. size : 1696 u64 = 1024 u64 + 768 u64
static inline void polymul_147456_mul( uint64_t * c , const uint64_t * a_fft , const uint64_t * b_fft )
{
	polymul_2304U64_mul( c , a_fft+4096 , b_fft+4096 , a_fft , b_fft );//offset 1024u64
}


///
/// Arithmetic operations for the quotient ring :=  gf2[x]/(x^N-1)
///
static inline void ring_to_fftform( uint8_t *v_fft , const uint8_t *v );
static inline void ring_mul_fftformx2( uint8_t *c, const uint8_t *a_fft, const uint8_t *b_fft );


void ring_to_fftform( uint8_t *v_fft , const uint8_t *v )
{
    uint64_t tmp1[N_PAD_U64];
    for(int i=VEC_N_SIZE_BYTES/8;i<N_PAD_U64;i++) { tmp1[i] = 0; }//convert v to tmp1 with zero padding
    memcpy( tmp1 , v , VEC_N_SIZE_BYTES );
    polymul_147456_input((uint64_t *)v_fft , tmp1 );
}

static inline void ring_reduce(uint8_t * _c , const uint64_t *temp_c)
{
    uint64_t c[VEC_N_SIZE_64];
    for(int i=0;i<VEC_N_SIZE_64;i++) {
        c[i] = temp_c[i] ^ ((temp_c[VEC_N_SIZE_64-1+i]>>N_MOD_64) | (temp_c[VEC_N_SIZE_64+i]<<(64-N_MOD_64)));
    }
    c[VEC_N_SIZE_64-1] &= BITMASK(PARAM_N, 64);
    memcpy( _c , c , VEC_N_SIZE_64*8 );
}

void ring_mul_fftformx2( uint8_t *c, const uint8_t *a_fft, const uint8_t *b_fft )
{
    uint64_t temp_c[N_PAD_U64*2];
    polymul_147456_mul( temp_c ,(uint64_t *)a_fft ,(uint64_t *)b_fft );
    ring_reduce(c, temp_c);
}

/**
 * @brief Carry-less multiplication mod (X^PARAM_N - 1).
 *
 * Computes o = a1 * a2, each operand of VEC_N_SIZE_64 words, then reduces.
 *
 * @param[out] o   Result buffer, size VEC_N_SIZE_64 words.
 * @param[in]  a1  Operand polynomial a(x).
 * @param[in]  a2  Operand polynomial b(x).
 */
void vect_mul(__m256i *o, const __m256i *v1, const __m256i *v2) {
    uint64_t a_fft[R_FFTFORM_BYTES/8];
    uint64_t b_fft[R_FFTFORM_BYTES/8];
    
    ring_to_fftform( (uint8_t *)a_fft , (uint8_t *)v1 );
    ring_to_fftform( (uint8_t *)b_fft , (uint8_t *)v2 );
    
    ring_mul_fftformx2( o , (uint8_t *)a_fft , (uint8_t *)b_fft );
}




