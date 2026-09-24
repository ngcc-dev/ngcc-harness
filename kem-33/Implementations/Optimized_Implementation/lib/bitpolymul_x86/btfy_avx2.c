

#include <stdint.h>
#include <string.h>

#include "btfy.h"
#include "cantor_to_gf264.h"

#include "gf264_aesni.h"

static inline unsigned min(unsigned a,unsigned b) { return (a<b)?a:b; }





// (log_n <= 2)
static void btfy_64_small( uint64_t * poly , unsigned log_n , uint64_t scalar_a );
static void ibtfy_64_small( uint64_t * poly , unsigned log_n , uint64_t scalar_a );

// (log_n > 2)
// (n_terms <= SIZE_TBL_CANTOR2X*2)
static void btfy_64_median( uint64_t * poly , unsigned log_n , uint64_t scalar_a );
static void ibtfy_64_median( uint64_t * poly , unsigned log_n , uint64_t scalar_a );

// (log_n > 2)
static void btfy_64_large( uint64_t * poly , unsigned log_n , uint64_t scalar_a );
static void ibtfy_64_large( uint64_t * poly , unsigned log_n , uint64_t scalar_a );



void btfy_64( uint64_t * poly , unsigned log_n , uint64_t scalar_a )
{
	unsigned n_terms = 1<<log_n;
	if (log_n <= 2) {
		btfy_64_small(poly,log_n,scalar_a);
	} else if (n_terms <= SIZE_TBL_CANTOR2X*2) {
		btfy_64_median(poly,log_n,scalar_a);
	} else {
		btfy_64_large(poly,log_n,scalar_a);
	}
}

void ibtfy_64( uint64_t * poly , unsigned log_n , uint64_t scalar_a )
{
	unsigned n_terms = 1<<log_n;
	if (log_n <= 2) {
		ibtfy_64_small(poly,log_n,scalar_a);
	} else if (n_terms <= SIZE_TBL_CANTOR2X*2) {
		ibtfy_64_median(poly,log_n,scalar_a);
	} else {
		ibtfy_64_large(poly,log_n,scalar_a);
	}
}








/////////////////////  butterfly units  //////////////////////


static inline
void butterfly_64_avx2( uint64_t * poly , unsigned unit ,  __m128i a ) /// assert(unit >= 8)
{
	unsigned unit_2= unit/2;
	for(unsigned i=0;i<unit_2;i+=4) {
		__m256i pi_l = _mm256_loadu_si256( (__m256i*)(poly + i) );
		__m256i pi_h = _mm256_loadu_si256( (__m256i*)(poly + unit_2 + i) );
		_mm_prefetch( (__m256i*)(poly + i + 4) , _MM_HINT_T0 );
		_mm_prefetch( (__m256i*)(poly + unit_2 + i + 4) , _MM_HINT_T0 );

		pi_l ^= _gf2ext64_mul_4x1_avx2( pi_h , a );
		pi_h ^= pi_l;

		_mm256_storeu_si256( (__m256i*)(poly + i) , pi_l );
		_mm256_storeu_si256( (__m256i*)(poly + unit_2 + i) , pi_h );
	}
}


//////////////////////////////////


static inline
void i_butterfly_64_avx2( uint64_t * poly , unsigned unit ,  __m128i a ) /// assert(unit >= 8)
{
	unsigned unit_2= unit/2;
	for(unsigned i=0;i<unit_2;i+=4) {
		__m256i pi_l = _mm256_loadu_si256( (__m256i*)(poly + i) );
		__m256i pi_h = _mm256_loadu_si256( (__m256i*)(poly + unit_2 + i) );
		_mm_prefetch( (__m256i*)(poly + i + 4) , _MM_HINT_T0 );
		_mm_prefetch( (__m256i*)(poly + unit_2 + i + 4) , _MM_HINT_T0 );

		pi_h ^= pi_l;
		pi_l ^= _gf2ext64_mul_4x1_avx2( pi_h , a );

		_mm256_storeu_si256( (__m256i*)(poly + i) , pi_l );
		_mm256_storeu_si256( (__m256i*)(poly + unit_2 + i) , pi_h );
	}
}


///////////////////////////////////////////


static inline
void btfy_2stages_64_avx2( uint64_t * poly , unsigned unit , __m256i s1s0_a ) /// assert(unit >= 8)
{
	__m128i si1_a = _mm256_extracti128_si256( s1s0_a , 1 );
	__m128i si0_a0a1 = _mm256_castsi256_si128( s1s0_a );

	unsigned unit_2= unit/2;
	for(unsigned i=0;i<unit_2;i+=4) {
		__m256i p0_l = _mm256_loadu_si256( (__m256i*)(poly + i) );
		__m256i p0_h = _mm256_loadu_si256( (__m256i*)(poly + unit_2 + i) );
		__m256i p1_l = _mm256_loadu_si256( (__m256i*)(poly + unit + i) );
		__m256i p1_h = _mm256_loadu_si256( (__m256i*)(poly + unit + unit_2 + i) );

		_mm_prefetch( (__m256i*)(poly + i + 4) , _MM_HINT_T0 );
		_mm_prefetch( (__m256i*)(poly + unit_2 + i + 4) , _MM_HINT_T0 );
		_mm_prefetch( (__m256i*)(poly + unit + i + 4) , _MM_HINT_T0 );
		_mm_prefetch( (__m256i*)(poly + unit + unit_2 + i + 4) , _MM_HINT_T0 );

		// stage 1
		p0_h ^= _gf2ext64_mul_4x1_avx2( p1_h , si1_a );
		__m128i p1_l_01 = _mm256_castsi256_si128( p1_l );
		__m128i p1_l_23 = _mm256_extracti128_si256( p1_l, 1 );
		__m128i p0_l_r0 = _mm_clmulepi64_si128( p1_l_01 , si1_a , 0x00 );
		__m128i p0_l_r1 = _mm_clmulepi64_si128( p1_l_01 , si1_a , 0x01 );
		__m128i p0_l_r2 = _mm_clmulepi64_si128( p1_l_23 , si1_a , 0x00 );
		__m128i p0_l_r3 = _mm_clmulepi64_si128( p1_l_23 , si1_a , 0x01 );
		p1_h ^= p0_h;
		// delayed:
		// _p0_l_0 = p0_l_0 ^ p0_l_r0;
		// _p0_l_1 = p0_l_1 ^ p0_l_r1;
		// _p0_l_2 = p0_l_2 ^ p0_l_r2;
		// _p0_l_3 = p0_l_3 ^ p0_l_r3;
		// _p1_l_0 = p1_l_0 ^ p0_l_0 ^ p0_l_r0;
		// _p1_l_1 = p1_l_1 ^ p0_l_1 ^ p0_l_r1;
		// _p1_l_2 = p1_l_2 ^ p0_l_2 ^ p0_l_r2;
		// _p1_l_3 = p1_l_3 ^ p0_l_3 ^ p0_l_r3;

		// stage 0
		__m128i p1_h_01 = _mm256_castsi256_si128( p1_h );
		__m128i p1_h_23 = _mm256_extracti128_si256( p1_h, 1 );
		__m128i p1_l_rr0 = _mm_clmulepi64_si128( p1_h_01 , si0_a0a1 , 0x10 );
		__m128i p1_l_rr1 = _mm_clmulepi64_si128( p1_h_01 , si0_a0a1 , 0x11 );
		__m128i p1_l_rr2 = _mm_clmulepi64_si128( p1_h_23 , si0_a0a1 , 0x10 );
		__m128i p1_l_rr3 = _mm_clmulepi64_si128( p1_h_23 , si0_a0a1 , 0x11 );
		p1_l ^= p0_l ^ _gf2ext64_reduce_x4_avx2( p1_l_rr0^p0_l_r0 , p1_l_rr1^p0_l_r1 , p1_l_rr2^p0_l_r2 , p1_l_rr3^p0_l_r3 );
		p1_h ^= p1_l;

		__m128i p0_h_01 = _mm256_castsi256_si128( p0_h );
		__m128i p0_h_23 = _mm256_extracti128_si256( p0_h, 1 );
		__m128i p0_l_rr0 = _mm_clmulepi64_si128( p0_h_01 , si0_a0a1 , 0x00 );
		__m128i p0_l_rr1 = _mm_clmulepi64_si128( p0_h_01 , si0_a0a1 , 0x01 );
		__m128i p0_l_rr2 = _mm_clmulepi64_si128( p0_h_23 , si0_a0a1 , 0x00 );
		__m128i p0_l_rr3 = _mm_clmulepi64_si128( p0_h_23 , si0_a0a1 , 0x01 );
		p0_l ^= _gf2ext64_reduce_x4_avx2( p0_l_rr0^p0_l_r0 , p0_l_rr1^p0_l_r1 , p0_l_rr2^p0_l_r2 , p0_l_rr3^p0_l_r3 );
		p0_h ^= p0_l;

		_mm256_storeu_si256( (__m256i*)(poly + i) , p0_l );
		_mm256_storeu_si256( (__m256i*)(poly + unit_2 + i) , p0_h );
		_mm256_storeu_si256( (__m256i*)(poly + unit + i) , p1_l );
		_mm256_storeu_si256( (__m256i*)(poly + unit + unit_2 + i) , p1_h );
	}
}

//////////////////////////////////

static inline
void ibtfy_2stages_64_avx2( uint64_t * poly , unsigned unit , __m256i s1s0_a ) /// assert(unit >= 8)
{
	__m128i si1_a = _mm256_extracti128_si256( s1s0_a , 1 );
	__m128i si0_a0a1 = _mm256_castsi256_si128( s1s0_a );
	unsigned unit_2= unit/2;
	for(unsigned i=0;i<unit_2;i+=4) {
		__m256i p0_l = _mm256_loadu_si256( (__m256i*)(poly + i) );
		__m256i p0_h = _mm256_loadu_si256( (__m256i*)(poly + unit_2 + i) );
		__m256i p1_l = _mm256_loadu_si256( (__m256i*)(poly + unit + i) );
		__m256i p1_h = _mm256_loadu_si256( (__m256i*)(poly + unit + unit_2 + i) );

		_mm_prefetch( (__m256i*)(poly + i + 4) , _MM_HINT_T0 );
		_mm_prefetch( (__m256i*)(poly + unit_2 + i + 4) , _MM_HINT_T0 );
		_mm_prefetch( (__m256i*)(poly + unit + i + 4) , _MM_HINT_T0 );
		_mm_prefetch( (__m256i*)(poly + unit + unit_2 + i + 4) , _MM_HINT_T0 );

		// stage 0
		p0_h ^= p0_l;
		p1_h ^= p1_l;

		__m128i p0_h_01 = _mm256_castsi256_si128( p0_h );
		__m128i p0_h_23 = _mm256_extracti128_si256( p0_h, 1 );
		__m128i p0_l_r0 = _mm_clmulepi64_si128( p0_h_01 , si0_a0a1 , 0x00 );
		__m128i p0_l_r1 = _mm_clmulepi64_si128( p0_h_01 , si0_a0a1 , 0x01 );
		__m128i p0_l_r2 = _mm_clmulepi64_si128( p0_h_23 , si0_a0a1 , 0x00 );
		__m128i p0_l_r3 = _mm_clmulepi64_si128( p0_h_23 , si0_a0a1 , 0x01 );

		__m128i p1_h_01 = _mm256_castsi256_si128( p1_h );
		__m128i p1_h_23 = _mm256_extracti128_si256( p1_h, 1 );
		__m128i p1_l_r0 = _mm_clmulepi64_si128( p1_h_01 , si0_a0a1 , 0x10 );
		__m128i p1_l_r1 = _mm_clmulepi64_si128( p1_h_01 , si0_a0a1 , 0x11 );
		__m128i p1_l_r2 = _mm_clmulepi64_si128( p1_h_23 , si0_a0a1 , 0x10 );
		__m128i p1_l_r3 = _mm_clmulepi64_si128( p1_h_23 , si0_a0a1 , 0x11 );
		// delayed:
		// _p0_l_0 = p0_l_0 ^ p0_l_r0;
		// _p0_l_1 = p0_l_1 ^ p0_l_r1;
		// _p0_l_2 = p0_l_2 ^ p0_l_r2;
		// _p0_l_3 = p0_l_3 ^ p0_l_r3;

		// stage 1
		p1_l ^= p0_l ^ _gf2ext64_reduce_x4_avx2( p1_l_r0^p0_l_r0 , p1_l_r1^p0_l_r1 , p1_l_r2^p0_l_r2 , p1_l_r3^p0_l_r3 );
		p1_h ^= p0_h;

		__m128i p1_l_01 = _mm256_castsi256_si128( p1_l );
		__m128i p1_l_23 = _mm256_extracti128_si256( p1_l, 1 );
		__m128i p0_l_rr0 = _mm_clmulepi64_si128( p1_l_01 , si1_a , 0x00 );
		__m128i p0_l_rr1 = _mm_clmulepi64_si128( p1_l_01 , si1_a , 0x01 );
		__m128i p0_l_rr2 = _mm_clmulepi64_si128( p1_l_23 , si1_a , 0x00 );
		__m128i p0_l_rr3 = _mm_clmulepi64_si128( p1_l_23 , si1_a , 0x01 );
		p0_l ^= _gf2ext64_reduce_x4_avx2( p0_l_rr0^p0_l_r0 , p0_l_rr1^p0_l_r1 , p0_l_rr2^p0_l_r2 , p0_l_rr3^p0_l_r3 );
		p0_h ^= _gf2ext64_mul_4x1_avx2( p1_h , si1_a );

		_mm256_storeu_si256( (__m256i*)(poly + i) , p0_l );
		_mm256_storeu_si256( (__m256i*)(poly + unit_2 + i) , p0_h );
		_mm256_storeu_si256( (__m256i*)(poly + unit + i) , p1_l );
		_mm256_storeu_si256( (__m256i*)(poly + unit + unit_2 + i) , p1_h );
	}
}

///////////////////////////////////////////

static inline
void btfy_s1s0_x4( uint64_t *ptr , __m128i s1_a , __m256i s0_a )
{
	__m256i t0123 = _mm256_loadu_si256( (__m256i*)ptr );
	__m256i t4567 = _mm256_loadu_si256( (__m256i*)(ptr + 4) );

	__m128i t01 = _mm256_castsi256_si128(t0123);
	__m128i t23 = _mm256_extracti128_si256(t0123, 1);
	__m128i t45 = _mm256_castsi256_si128(t4567);
	__m128i t67 = _mm256_extracti128_si256(t4567, 1);

	// stage 1
	__m128i t2s1a = _mm_clmulepi64_si128( t23 , s1_a , 0x0 );
	__m128i t3s1a = _mm_clmulepi64_si128( t23 , s1_a , 0x1 );
	__m128i t6s1a = _mm_clmulepi64_si128( t67 , s1_a , 0x10 );
	__m128i t7s1a = _mm_clmulepi64_si128( t67 , s1_a , 0x11 );
	__m128i t3t7s1a = _gf2ext64_reduce_x2_sse( t3s1a , t7s1a );

	__m128i mask_low64 = _mm_set_epi64x(0, -1LL);

	__m128i t0 = t2s1a ^ (t01&mask_low64);               // data in full 128
	__m128i t1 = t3t7s1a ^ _mm_srli_si128(t01, 8);       // data in low 64 bit
	__m128i t2 = t0 ^ (t23&mask_low64);                  // data in full 128
	__m128i t3 = t1 ^ _mm_srli_si128(t23, 8);            // data in low 64 bit

	__m128i t4 = t6s1a ^ (t45&mask_low64);               // data in full 128
	__m128i t5 = t3t7s1a ^ t45;                          // data in high 64 bit
	__m128i t6 = t4 ^ (t67&mask_low64);                  // data in full 128
	__m128i t7 = t5 ^ t67;                               // data in high 64 bit

	// stage 0
	__m128i s0a_01 = _mm256_castsi256_si128(s0_a);
	__m128i s0a_23 = _mm256_extracti128_si256(s0_a, 1);

	__m128i t1s0a = _mm_clmulepi64_si128( t1 , s0a_01 , 0x00 );
	__m128i t3s0a = _mm_clmulepi64_si128( t3 , s0a_01 , 0x10 );
	__m128i t5s0a = _mm_clmulepi64_si128( t5 , s0a_23 , 0x01 );
	__m128i t7s0a = _mm_clmulepi64_si128( t7 , s0a_23 , 0x11 );

	t0 ^= t1s0a;
	t2 ^= t3s0a;
	t4 ^= t5s0a;
	t6 ^= t7s0a;
	__m256i t0426 = _gf2ext64_reduce_x4_avx2( t0 , t4 , t2 , t6 );
	__m128i t04 = _mm256_castsi256_si128(t0426);
	__m128i t26 = _mm256_extracti128_si256(t0426, 1);

	t1 ^= t04;  // data in low64 bit
	t3 ^= t26;  // data in low64 bit
	t5 ^= t04;  // data in high64 bit
	t7 ^= t26;  // data in high64 bit

	__m256i _t0123 = _mm256_set_m128i( _mm_unpacklo_epi64(t26,t3) , _mm_unpacklo_epi64(t04,t1) );
	__m256i _t4567 = _mm256_set_m128i( _mm_unpackhi_epi64(t26,t7) , _mm_unpackhi_epi64(t04,t5) );

	_mm256_storeu_si256( (__m256i*)ptr , _t0123 );
	_mm256_storeu_si256( (__m256i*)(ptr + 4) , _t4567 );
}

static inline
void ibtfy_s0s1_x4( uint64_t *ptr , __m128i s1_a , __m256i s0_a )
{
	__m256i t0123 = _mm256_loadu_si256( (__m256i*)ptr );
	__m256i t4567 = _mm256_loadu_si256( (__m256i*)(ptr + 4) );

	__m256i t0426 = _mm256_unpacklo_epi64( t0123 , t4567 );
	__m256i t1537 = _mm256_unpackhi_epi64( t0123 , t4567 );

	// stage 0
	__m128i s0a_01 = _mm256_castsi256_si128(s0_a);
	__m128i s0a_23 = _mm256_extracti128_si256(s0_a, 1);

	t1537 ^= t0426;

	__m128i t04 = _mm256_castsi256_si128(t0426);
	__m128i t26 = _mm256_extracti128_si256(t0426, 1);
	__m128i t15 = _mm256_castsi256_si128(t1537);
	__m128i t37 = _mm256_extracti128_si256(t1537, 1);

	__m128i t1s0a = _mm_clmulepi64_si128( t15 , s0a_01 , 0x00 ); // add to t0
	__m128i t3s0a = _mm_clmulepi64_si128( t37 , s0a_01 , 0x10 ); // add to t2
	__m128i t5s0a = _mm_clmulepi64_si128( t15 , s0a_23 , 0x01 ); // add to t4
	__m128i t7s0a = _mm_clmulepi64_si128( t37 , s0a_23 , 0x11 ); // add to t6

	// delayed
	// _t0 = t0 ^ t1s0a;
	// _t4 = t4 ^ t5s0a;
	// _t2 = t2 ^ t3s0a;
	// _t6 = t6 ^ t7s0a;

	// stage 1
	t37 ^= t15;
	// __t2 ^= _t2 ^ _t0 = t2 ^ t0 ^ t3s0a ^ t1s0a;
	// __t6 ^= _t6 ^ _t4 = t6 ^ t4 ^ t7s0a ^ t5s0a;
	t26 = t26 ^ t04 ^ _gf2ext64_reduce_x2_sse( t3s0a ^ t1s0a , t7s0a ^ t5s0a );

	__m128i t2s1a = _mm_clmulepi64_si128( t26 , s1_a , 0x0 ); // add to t0
	__m128i t3s1a = _mm_clmulepi64_si128( t37 , s1_a , 0x0 ); // add to t1
	__m128i t6s1a = _mm_clmulepi64_si128( t26 , s1_a , 0x11 ); // add to t4
	__m128i t7s1a = _mm_clmulepi64_si128( t37 , s1_a , 0x11 ); // add to t5

	__m256i r0415 = _gf2ext64_reduce_x4_avx2( t2s1a^t1s0a , t6s1a^t5s0a , t3s1a , t7s1a );
	t04 ^= _mm256_castsi256_si128(r0415);
	t15 ^= _mm256_extracti128_si256(r0415, 1);

	__m256i _t0426 = _mm256_set_m128i( t26 , t04 );
	__m256i _t1537 = _mm256_set_m128i( t37 , t15 );

	_mm256_storeu_si256( (__m256i*)ptr , _mm256_unpacklo_epi64(_t0426,_t1537) );
	_mm256_storeu_si256( (__m256i*)(ptr + 4) , _mm256_unpackhi_epi64(_t0426,_t1537) );
}



/////////////////////  butterfly for all constants si(...) found in precomputed tables ////////////////////////////////////

static
void btfy_64_median( uint64_t * poly , unsigned log_n , uint64_t scalar_a )
{
	// assert( log_n > 2 )
	// assert( n_terms <= SIZE_TBL_CANTOR2X*2 )

	unsigned n_terms = 1<<log_n;
	unsigned si=log_n-1;

	// if si is even
	if (0==(si&1)) {
		unsigned unit = 1<<(si+1);
		unsigned num  = 1<<(log_n-(si+1)); // n_terms / unit

		uint64_t extra_a = cantor_to_gf264(scalar_a>>si);
		for(unsigned j=0;j<num;j++) {
			__m128i xmm_a = _mm_cvtsi64_si128( cantor_to_gf264_2x[j]^extra_a );
			butterfly_64_avx2( poly + j*unit , unit , xmm_a );
		}
		si -= 1;
	}
	for( ; si>1; si -= 2 ) {
		unsigned unit = 1<<(si);
		unsigned num  = 1<<(log_n-(si)); // n_terms / unit

		uint64_t extra_a1 = cantor_to_gf264(scalar_a>>si);
		uint64_t extra_a0 = cantor_to_gf264(scalar_a>>(si-1));
		__m256i extra_a1a0 = _mm256_set_epi64x( extra_a1 , extra_a1 , extra_a0 , extra_a0 );
		for(unsigned j=0;j<num;j+=2) {
			__m256i a1a0 = _mm256_load_si256( (__m256i*)&btfy_consts_2stages[j<<1] ) ^ extra_a1a0;
			btfy_2stages_64_avx2( poly + j*unit , unit , a1a0 );
		}
	}
	// s1 and s0
	do {
		unsigned num_s0 = n_terms/2;
		__m128i extra_s1_a = _mm_set1_epi64x( cantor_to_gf264(scalar_a>>1) );
		__m256i extra_s0_a = _mm256_set1_epi64x( cantor_to_gf264(scalar_a) );
		for(unsigned j=0;j<num_s0;j+=4) {
			__m128i s1_a = _mm_load_si128( (__m128i*) &cantor_to_gf264_2x[j>>1] )^extra_s1_a;
			__m256i s0_a = _mm256_load_si256( (__m256i*) &cantor_to_gf264_2x[j] )^extra_s0_a;
			btfy_s1s0_x4( poly + j*2 , s1_a , s0_a );
		}
	} while(0);
}

static
void ibtfy_64_median( uint64_t * poly , unsigned log_n , uint64_t scalar_a )
{
	// assert( log_n > 2 )
	// assert( n_terms <= SIZE_TBL_CANTOR2X*2 )
	unsigned n_terms = 1<<log_n;

	// s1 and s0
	do {
		unsigned num_s0 = n_terms/2;
		__m128i extra_s1_a = _mm_set1_epi64x( cantor_to_gf264(scalar_a>>1) );
		__m256i extra_s0_a = _mm256_set1_epi64x( cantor_to_gf264(scalar_a) );
		for(unsigned j=0;j<num_s0;j+=4) {
			__m128i s1_a = _mm_load_si128( (__m128i*) &cantor_to_gf264_2x[j>>1] )^extra_s1_a;
			__m256i s0_a = _mm256_load_si256( (__m256i*) &cantor_to_gf264_2x[j] )^extra_s0_a;
			ibtfy_s0s1_x4( poly + j*2 , s1_a , s0_a );
		}
	} while(0);

	unsigned si = 2;
	for( ; si+1<log_n; si += 2 ) {
		unsigned unit = 1<<(si+1);
		unsigned num  = 1<<(log_n-(si+1)); // n_terms / unit

		uint64_t extra_a1 = cantor_to_gf264(scalar_a>>(si+1));
		uint64_t extra_a0 = cantor_to_gf264(scalar_a>>(si));
		__m256i extra_a1a0 = _mm256_set_epi64x( extra_a1 , extra_a1 , extra_a0 , extra_a0 );
		for(unsigned j=0;j<num;j+=2) {
			__m256i a1a0 = _mm256_load_si256( (__m256i*)&btfy_consts_2stages[j<<1] ) ^ extra_a1a0;
			ibtfy_2stages_64_avx2( poly + j*unit , unit , a1a0 );
		}
	}
	if ( si<log_n ) {
		unsigned unit = 1<<(si+1);
		unsigned num  = 1<<(log_n-(si+1)); // n_terms / unit

		uint64_t extra_a = cantor_to_gf264(scalar_a>>si);
		for(unsigned j=0;j<num;j++) {
			__m128i xmm_a = _mm_cvtsi64_si128( cantor_to_gf264_2x[j]^extra_a );
			i_butterfly_64_avx2( poly + j*unit , unit , xmm_a );
		}
	}
}


/////////////////////  butterfly of very large sizes  //////////////////////


static
void btfy_64_large( uint64_t * poly , unsigned log_n , uint64_t scalar_a )
{
	// assert( log_n > 2 )

	unsigned n_terms = 1<<log_n;
	unsigned si=log_n-1;

	// if si is even
	if (0==(si&1)) {
		unsigned unit = 1<<(si+1);
		unsigned num  = 1<<(log_n-(si+1)); // n_terms / unit

		uint64_t extra_a = cantor_to_gf264(scalar_a>>si);
		unsigned step_size = min(SIZE_TBL_CANTOR2X,num);
		for(unsigned j=0;j<num;j+=step_size) {  // process constants outside of the cantor_to_gf264_2x[]
			//unsigned idx = ((j+k)<<(si+1)) + scalar_a
			uint64_t extra_a_j = extra_a ^ cantor_to_gf264(j<<1);
			for(unsigned k=0;k<step_size;k++) {
				__m128i xmm_a = _mm_cvtsi64_si128( cantor_to_gf264_2x[k]^extra_a_j );
				butterfly_64_avx2( poly + (j+k)*unit , unit , xmm_a );
			}
		}
		si -= 1;
	}
	for( ; si>1; si -= 2 ) {
		unsigned unit = 1<<(si);
		unsigned num  = 1<<(log_n-(si)); // n_terms / unit
		uint64_t extra_a1 =  cantor_to_gf264(scalar_a>>si);
		uint64_t extra_a0 =  cantor_to_gf264(scalar_a>>(si-1));

		unsigned step_size = min(SIZE_TBL_CANTOR2X,num);
		for(unsigned j=0;j<num;j+=step_size) {  // process constants outside of the cantor_to_gf264_2x[]
			//unsigned idx = ((j+k)<<(si+1)) + scalar_a
			uint64_t extra_a1_j = extra_a1 ^ cantor_to_gf264(j);
			uint64_t extra_a0_j = extra_a0 ^ cantor_to_gf264(j<<1);
			__m256i extra_a1a0 = _mm256_set_epi64x( extra_a1_j , extra_a1_j , extra_a0_j , extra_a0_j );
			for(unsigned k=0;k<step_size;k+=2) {
				__m256i a1a0 = _mm256_load_si256( (__m256i*)&btfy_consts_2stages[k<<1] ) ^ extra_a1a0;
				btfy_2stages_64_avx2( poly + (j+k)*unit , unit , a1a0 );
			}
		}
	}
	// s1 and s0
	do {
		unsigned num_s0 = n_terms/2;
		unsigned step_size = min(SIZE_TBL_CANTOR2X,num_s0);
		uint64_t s1_a = cantor_to_gf264(scalar_a>>1);
		uint64_t s0_a = cantor_to_gf264(scalar_a);

		for(unsigned j=0;j<num_s0;j+=step_size) {
			//unsigned idx = ((j+k)<<(si+1)) + scalar_a
			__m128i s1_a_j = _mm_set1_epi64x( s1_a ^ cantor_to_gf264(j) );
			__m256i s0_a_j = _mm256_set1_epi64x( s0_a ^ cantor_to_gf264(j<<1) );

			for(unsigned k=0;k<step_size;k+=4) {
				__m128i s1_a = _mm_load_si128( (__m128i*) &cantor_to_gf264_2x[k>>1] )^s1_a_j;
				__m256i s0_a = _mm256_load_si256( (__m256i*) &cantor_to_gf264_2x[k] )^s0_a_j;

				btfy_s1s0_x4( poly + (j+k)*2 , s1_a , s0_a );
			}
		}
	} while(0);
}

static
void ibtfy_64_large( uint64_t * poly , unsigned log_n , uint64_t scalar_a )
{
	// assert( log_n > 2 )
	unsigned n_terms = 1<<log_n;

	// s1 and s0
	do {
		unsigned num_s0 = n_terms/2;
		unsigned step_size = min(SIZE_TBL_CANTOR2X,num_s0);
		uint64_t s1_a = cantor_to_gf264(scalar_a>>1);
		uint64_t s0_a = cantor_to_gf264(scalar_a);

		for(unsigned j=0;j<num_s0;j+=step_size) {
			//unsigned idx = ((j+k)<<(si+1)) + scalar_a
			__m128i s1_a_j = _mm_set1_epi64x( s1_a ^ cantor_to_gf264(j) );
			__m256i s0_a_j = _mm256_set1_epi64x( s0_a ^ cantor_to_gf264(j<<1) );

			for(unsigned k=0;k<step_size;k+=4) {
				__m128i s1_a = _mm_load_si128( (__m128i*) &cantor_to_gf264_2x[k>>1] )^s1_a_j;
				__m256i s0_a = _mm256_load_si256( (__m256i*) &cantor_to_gf264_2x[k] )^s0_a_j;

				ibtfy_s0s1_x4( poly + (j+k)*2 , s1_a , s0_a );
			}
		}
	} while(0);
	unsigned si = 2;
	for( ; si+1<log_n; si += 2 ) {
		unsigned unit = 1<<(si+1);
		unsigned num  = 1<<(log_n-(si+1)); // n_terms / unit
		uint64_t extra_a1 =  cantor_to_gf264(scalar_a>>(si+1));
		uint64_t extra_a0 =  cantor_to_gf264(scalar_a>>(si));

		unsigned step_size = min(SIZE_TBL_CANTOR2X,num);
		for(unsigned j=0;j<num;j+=step_size) {  // process constants outside of the cantor_to_gf264_2x[]
			//unsigned idx = ((j+k)<<(si+1)) + scalar_a
			uint64_t extra_a1_j = extra_a1 ^ cantor_to_gf264(j);
			uint64_t extra_a0_j = extra_a0 ^ cantor_to_gf264(j<<1);
			__m256i extra_a1a0 = _mm256_set_epi64x( extra_a1_j , extra_a1_j , extra_a0_j , extra_a0_j );
			for(unsigned k=0;k<step_size;k+=2) {
				__m256i a1a0 = _mm256_load_si256( (__m256i*)&btfy_consts_2stages[k<<1] ) ^ extra_a1a0;
				ibtfy_2stages_64_avx2( poly + (j+k)*unit , unit , a1a0 );
			}
		}
	}
	if ( si<log_n ) {
		unsigned unit = 1<<(si+1);
		unsigned num  = 1<<(log_n-(si+1)); // n_terms / unit
		uint64_t extra_a = cantor_to_gf264(scalar_a>>si);
		unsigned step_size = min(SIZE_TBL_CANTOR2X,num);
		for(unsigned j=0;j<num;j+=step_size) {  // process constants outside of the cantor_to_gf264_2x[]
			//unsigned idx = ((j+k)<<(si+1)) + scalar_a
			uint64_t extra_a_j = extra_a ^ cantor_to_gf264(j<<1);
			for(unsigned k=0;k<step_size;k++) {
				__m128i xmm_a = _mm_cvtsi64_si128( cantor_to_gf264_2x[k]^extra_a_j );
				i_butterfly_64_avx2( poly + (j+k)*unit , unit , xmm_a );
			}
		}
	}
}




/////////////////////  butterfly of very small sizes  //////////////////////


static inline
void btfy_s1_x1( uint64_t * poly4 , uint64_t extra_a ) {
	__m128i p01 = _mm_loadu_si128( (__m128i*)poly4 );
	__m128i p23 = _mm_loadu_si128( (__m128i*)(poly4+2) );
	__m128i eaea = _mm_set1_epi64x( extra_a );

	p01 ^= _gf2ext64_mul_2x1_sse( p23 , eaea );
	p23 ^= p01;

	_mm_storeu_si128( (__m128i*)poly4 , p01 );
	_mm_storeu_si128( (__m128i*)(poly4+2) , p23 );
}

static inline
void i_btfy_s1_x1( uint64_t * poly4 , uint64_t extra_a ) {
	__m128i p01 = _mm_loadu_si128( (__m128i*)poly4 );
	__m128i p23 = _mm_loadu_si128( (__m128i*)(poly4+2) );
	__m128i eaea = _mm_set1_epi64x( extra_a );

	p23 ^= p01;
	p01 ^= _gf2ext64_mul_2x1_sse( p23 , eaea );

	_mm_storeu_si128( (__m128i*)poly4 , p01 );
	_mm_storeu_si128( (__m128i*)(poly4+2) , p23 );
}

static inline
void btfy_s0_x1( uint64_t * poly2 , uint64_t extra_a ) {
	__m128i p01 = _mm_loadu_si128( (__m128i*)poly2 );
	__m128i eaea = _mm_set1_epi64x( extra_a );

	p01 ^= _gf2ext64_mul_hi_sse( p01 , eaea );
	p01 ^= _mm_slli_si128( p01 , 8 );

	_mm_storeu_si128( (__m128i*)poly2 , p01 );
}

static inline
void i_btfy_s0_x1( uint64_t * poly2 , uint64_t extra_a ) {
	__m128i p01 = _mm_loadu_si128( (__m128i*)poly2 );
	__m128i eaea = _mm_set1_epi64x( extra_a );

	p01 ^= _mm_slli_si128( p01 , 8 );
	p01 ^= _gf2ext64_mul_hi_sse( p01 , eaea );

	_mm_storeu_si128( (__m128i*)poly2 , p01 );
}


static
void btfy_64_small( uint64_t * poly , unsigned log_n , uint64_t scalar_a )
{
	if( 1 == log_n ) { btfy_s0_x1( poly , cantor_to_gf264(scalar_a) ); }
	else if( 2 == log_n ) {
		btfy_s1_x1( poly , cantor_to_gf264(scalar_a>>1) );
		uint64_t extra_a = cantor_to_gf264(scalar_a);
		btfy_s0_x1( poly   , extra_a );
		btfy_s0_x1( poly+2 , extra_a^cantor_basis[1] );
	}
}

static
void ibtfy_64_small( uint64_t * poly , unsigned log_n , uint64_t scalar_a )
{
	if( 1 == log_n ) { i_btfy_s0_x1( poly , cantor_to_gf264(scalar_a) ); }
	else if( 2 == log_n ) {
		uint64_t extra_a = cantor_to_gf264(scalar_a);
		i_btfy_s0_x1( poly   , extra_a );
		i_btfy_s0_x1( poly+2 , extra_a^cantor_basis[1] );
		i_btfy_s1_x1( poly , cantor_to_gf264(scalar_a>>1) );
	}
}

