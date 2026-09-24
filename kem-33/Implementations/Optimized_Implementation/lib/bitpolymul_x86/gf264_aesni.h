
#ifndef _GF264_AESNI_H_
#define _GF264_AESNI_H_


#include <stdint.h>

#include <immintrin.h>


/// X^64 + X^4 + X^3 + X + 1
/// 0x1b
static const uint64_t _gf2ext64_reducer[4] __attribute__((aligned(32)))  = {0x415A776C2D361B00ULL,0x1bULL,0x415A776C2D361B00ULL,0x1bULL};

// bit 64-67 of the return value will be polluted. bit 68-127 will keep the original value.
static inline
__m128i _gf2ext64_reduce_sse( __m128i x0 )
{
	__m128i reducer = _mm_load_si128( (__m128i const*)_gf2ext64_reducer );
	//__m128i *reducer = (__m128i *)_gf2ext128_reducer;
	__m128i r0 = _mm_clmulepi64_si128( x0 , reducer , 0x11 );
	__m128i r1 = _mm_shuffle_epi8( reducer , r0 );
	return x0^r0^_mm_shuffle_epi32( r1 , _MM_SHUFFLE( 3 , 3 , 3 , 2 ) );
}


static inline
__m128i _gf2ext64_reduce_x2_sse( __m128i l0 , __m128i h0 )
{
	__m128i reducer = _mm_load_si128( (__m128i const*)_gf2ext64_reducer );
	//__m128i *reducer = (__m128i *)_gf2ext128_reducer;
	__m128i r0 = _mm_clmulepi64_si128( l0 , reducer , 0x11 );
	__m128i s0 = _mm_clmulepi64_si128( h0 , reducer , 0x11 );
	__m128i r2 = l0 ^ r0;
	__m128i s2 = h0 ^ s0;
	__m128i pr = _mm_unpacklo_epi64( r2 , s2 );

	__m128i rr = _mm_unpackhi_epi64( r0 , s0 );
	__m128i rr2 = _mm_shuffle_epi8( reducer , rr );

	return pr^rr2;
}

static inline
__m256i _gf2ext64_reduce_x4_avx2( __m128i w0 , __m128i x0 , __m128i y0 , __m128i z0 )
{
	__m256i reducer2 = _mm256_load_si256( (__m256i const*)_gf2ext64_reducer );
	//__m128i *reducer = (__m128i *)_gf2ext128_reducer;
	__m128i reducer = _mm256_castsi256_si128( reducer2 );
	__m128i r0 = _mm_clmulepi64_si128( w0 , reducer , 0x11 );
	__m128i s0 = _mm_clmulepi64_si128( x0 , reducer , 0x11 );
	__m128i t0 = _mm_clmulepi64_si128( y0 , reducer , 0x11 );
	__m128i u0 = _mm_clmulepi64_si128( z0 , reducer , 0x11 );
	__m128i r2 = w0 ^ r0;
	__m128i s2 = x0 ^ s0;
	__m128i t2 = y0 ^ t0;
	__m128i u2 = z0 ^ u0;

	__m256i pr2 = _mm256_set_m128i( _mm_unpacklo_epi64( t2 , u2 ) , _mm_unpacklo_epi64( r2 , s2 ) );
	__m256i rr2 = _mm256_set_m128i( _mm_unpackhi_epi64( t0 , u0 ) , _mm_unpackhi_epi64( r0 , s0 ) );
	return pr2 ^ _mm256_shuffle_epi8( reducer2 , rr2 );
}



static inline
__m128i _gf2ext64_mul_sse( __m128i a0 , __m128i b0 )
{
	__m128i c0 = _mm_clmulepi64_si128( a0 , b0 , 0 );
	__m128i c3 = _gf2ext64_reduce_sse( c0 );
	return c3;
}

static inline
__m128i _gf2ext64_mul_hi_sse( __m128i a0 , __m128i b0 )
{
	__m128i c0 = _mm_clmulepi64_si128( a0 , b0 , 1 );
	__m128i c3 = _gf2ext64_reduce_sse( c0 );
	return c3;
}

static inline
__m128i _gf2ext64_mul_2x1_sse( __m128i a0a1 , __m128i b0 )
{
	__m128i c0 = _mm_clmulepi64_si128( a0a1 , b0 , 0 );
	__m128i c1 = _mm_clmulepi64_si128( a0a1 , b0 , 1 );
	__m128i c3 = _gf2ext64_reduce_x2_sse( c0 , c1 );
	return c3;
}

static inline
__m128i _gf2ext64_mul_2x2_sse( __m128i a0a1 , __m128i b0b1 )
{
	__m128i c0 = _mm_clmulepi64_si128( a0a1 , b0b1 , 0 );
	__m128i c1 = _mm_clmulepi64_si128( a0a1 , b0b1 , 0x11 );
	__m128i c3 = _gf2ext64_reduce_x2_sse( c0 , c1 );
	return c3;
}

static inline
__m256i _gf2ext64_mul_4x1_avx2( __m256i a , __m128i b0 )
{
	__m128i al = _mm256_castsi256_si128( a );
	__m128i ah = _mm256_extracti128_si256( a , 1 );

	__m128i c0 = _mm_clmulepi64_si128( al , b0 , 0 );
	__m128i c1 = _mm_clmulepi64_si128( al , b0 , 1 );
	__m128i c2 = _mm_clmulepi64_si128( ah , b0 , 0 );
	__m128i c3 = _mm_clmulepi64_si128( ah , b0 , 1 );

	return _gf2ext64_reduce_x4_avx2( c0 , c1 , c2 , c3 );
}

static inline
__m256i _gf2ext64_mul_2x1_x2_avx2( __m256i a , __m128i b01 )
{
	__m128i al = _mm256_castsi256_si128( a );
	__m128i ah = _mm256_extracti128_si256( a , 1 );

	__m128i c0 = _mm_clmulepi64_si128( al , b01 , 0 );
	__m128i c1 = _mm_clmulepi64_si128( al , b01 , 1 );
	__m128i c2 = _mm_clmulepi64_si128( ah , b01 , 0x10 );
	__m128i c3 = _mm_clmulepi64_si128( ah , b01 , 0x11 );

	return _gf2ext64_reduce_x4_avx2( c0 , c1 , c2 , c3 );
}

static inline
__m256i _gf2ext64_mul_4x4_avx2( __m256i a , __m256i b )
{
	__m128i al = _mm256_castsi256_si128( a );
	__m128i bl = _mm256_castsi256_si128( b );
	__m128i ah = _mm256_extracti128_si256( a , 1 );
	__m128i bh = _mm256_extracti128_si256( b , 1 );

	__m128i c0 = _mm_clmulepi64_si128( al , bl , 0 );
	__m128i c1 = _mm_clmulepi64_si128( al , bl , 0x11 );
	__m128i c2 = _mm_clmulepi64_si128( ah , bh , 0 );
	__m128i c3 = _mm_clmulepi64_si128( ah , bh , 0x11 );

	return _gf2ext64_reduce_x4_avx2( c0 , c1 , c2 , c3 );
}


static inline
uint64_t gf2ext64_mul_u64( uint64_t a , uint64_t b )
{
	__m128i a0, b0;
	a0 = _mm_cvtsi64_si128( a );
	b0 = _mm_cvtsi64_si128( b );
	__m128i c0 = _gf2ext64_mul_sse( a0 , b0 );

	return _mm_extract_epi64( c0 , 0 );
}


static inline
void gf2ext64_mul_2x2_sse_unaligned( uint64_t * c , const uint64_t * a , const uint64_t * b )
{
	__m128i a0a1 = _mm_loadu_si128( (__m128i const *)a );
	__m128i b0b1 = _mm_loadu_si128( (__m128i const *)b );
	__m128i c0c1 = _gf2ext64_mul_2x2_sse( a0a1 , b0b1 );

	_mm_storeu_si128((__m128i*) c , c0c1 );
}

static inline
void gf2ext64_mul_4x4_avx2_unaligned( uint64_t * c , const uint64_t * a , const uint64_t * b )
{
	__m256i as = _mm256_loadu_si256( (__m256i const *)a );
	__m256i bs = _mm256_loadu_si256( (__m256i const *)b );
	__m256i cs = _gf2ext64_mul_4x4_avx2( as , bs );

	_mm256_storeu_si256((__m256i*) c , cs );
}




////////////////////////////////////////////////////////////////////////////////////


// s7 = x^128 + x^64 + x^32 + x16 + x8 + x4 + x2 + x
// s6 = x^64  + x16 + x4 + x
static const uint64_t _s6_s7[2] __attribute__((aligned(32)))  = {0x100010116ULL,0x10012ULL};


#if 0
static inline
__m256i div_s7_s6( __m256i a )
{
	__m128i r_s7 = _mm_load_si128( (__m128i const*)_s6_s7 );
	__m128i a1 = _mm256_extracti128_si256( a, 1 );
	__m128i a0 = _mm256_castsi256_si128( a );

	__m128i a1h_s7 = _mm_clmulepi64_si128( a1 , r_s7 , 1 );
	a1 ^= _mm_srli_si128( a1^a1h_s7 , 8 );
	a0 ^= _mm_slli_si128( a1h_s7 , 8 );

	a0 ^= _mm_slli_si128( a1 , 8 );
	a0 ^= _mm_clmulepi64_si128( a1 , r_s7 , 0 );

	__m128i a1rd = _mm_srli_si128(a1^_mm_srli_epi16(a1,15)^_mm_srli_epi16(a1,12),6);
	__m128i a0rd = _mm_srli_si128(a0^_mm_srli_epi16(a0,15)^_mm_srli_epi16(a0,12),6);
	a1 ^= _mm_clmulepi64_si128( a1^a1rd , r_s7 , 0x11 );
	a0 ^= _mm_clmulepi64_si128( a0^a0rd , r_s7 , 0x11 );

	__m256i r = _mm256_castsi128_si256( a0 );
	return _mm256_inserti128_si256( r , a1 , 1 );
}
#else
static inline
__m256i div_s7( __m256i a )
{
	__m128i r_s7 = _mm_load_si128( (__m128i const*)_s6_s7 );
	__m128i a1 = _mm256_extracti128_si256( a, 1 );
	__m128i a0 = _mm256_castsi256_si128( a );

	__m128i a1h_s7 = _mm_clmulepi64_si128( a1 , r_s7 , 1 );
	a1 ^= _mm_srli_si128( a1^a1h_s7 , 8 );
	a0 ^= _mm_slli_si128( a1h_s7 , 8 );

	a0 ^= _mm_slli_si128( a1 , 8 );
	a0 ^= _mm_clmulepi64_si128( a1 , r_s7 , 0 );

	__m256i r = _mm256_castsi128_si256( a0 );
	return _mm256_inserti128_si256( r , a1 , 1 );
}
#endif

static inline
__m256i exp_s7( __m256i a )
{
	__m128i r_s7 = _mm_load_si128( (__m128i const*)_s6_s7 );
	__m128i a1 = _mm256_extracti128_si256( a, 1 );
	__m128i a0 = _mm256_castsi256_si128( a );

	a0 ^= _mm_clmulepi64_si128( a1 , r_s7 , 0 );
	a0 ^= _mm_slli_si128( a1 , 8 );
	__m128i a1h_s7 = _mm_clmulepi64_si128( a1 , r_s7 , 1 );
	a0 ^= _mm_slli_si128( a1h_s7 , 8 );
	a1 ^= _mm_srli_si128( a1^a1h_s7 , 8 );

	__m256i r = _mm256_castsi128_si256( a0 );
	return _mm256_inserti128_si256( r , a1 , 1 );
}


#endif

