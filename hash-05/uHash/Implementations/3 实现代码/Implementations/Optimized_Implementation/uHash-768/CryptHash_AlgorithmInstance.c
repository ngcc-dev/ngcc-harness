
/*
* Date:2026 03 02
* The speed of uHash with linear key schedule
* This is the CF512
* Note: This version omit the Add Constant
* 后续优化，可以考虑，其实Key Schedule部分的值是一样的，是不是可以在S盒的拆分那里减少一下cycle
*
*
* Update Date:
* Bug:
* (1)Key Schedule and Ki to KKi, i=0,1,2,3,4,5,6,7
*
* Update Date: 2026 03 30
* The result of uHash512 is correct
*
* Update Date: 2026 04 20
* The compress function in the L-th Block is update
* This code is corresponding to the basic implementation
*
* Update Date:2026 04 29
* This code is the c not cpp code
*
*/
#include <stdio.h>
#include <memory.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <malloc.h>

#include <emmintrin.h>//sse2 header file(include sse header file)  
#include <pmmintrin.h> //SSE3(include emmintrin.h)  
#include <tmmintrin.h>//SSSE3(include pmmintrin.h)  
#include <smmintrin.h>//SSE4.1(include tmmintrin.h)  
#include <nmmintrin.h>//SSE4.2(include smmintrin.h)  
#include <intrin.h>//(include immintrin.h)  

#include <wmmintrin.h>//AES and PCLMULQDQ intrinsics
#include <immintrin.h>//Intel-specific intrinsics(AVX)
#include <intrin.h>//(include immintrin.h) 
#include <intrin.h>
#include <immintrin.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <string.h>
//#include <openssl/evp.h>   // OpenSSL SHA-3 接口
//
// 
// #include <openssl/provider.h>
#include <intrin.h>

#include "CryptHash_AlgorithmInstance.h"

#pragma intrinsic(__rdtsc)


#define BlockSize 32//2*HalfBlockSize
#define HalfBlockSize 16// The length of one brunch
typedef unsigned long long uint64_t;
#define ROUND 13


// ==================== Windows 完全兼容的 aligned 分配 ====================
#if defined(_MSC_VER) || defined(_WIN32)
#include <malloc.h>        // 关键！_aligned_malloc 在这里面
#endif

static uint64_t checksum = 0x12345678ULL;

static inline void* my_aligned_alloc(size_t size, size_t alignment)
{
#if defined(_MSC_VER) || defined(_WIN32)
	return _aligned_malloc(size, alignment);
#else
	// 给 Linux/macOS 留个后门（如果你以后在 Linux 上编译）
	void* p;
	if (posix_memalign(&p, alignment, size) != 0) return nullptr;
	return p;
#endif
}

static inline void my_aligned_free(void* ptr)
{
#if defined(_MSC_VER) || defined(_WIN32)
	_aligned_free(ptr);
#else
	free(ptr);
#endif
}
static inline void mfence()
{
	_mm_mfence();
}

// ==================== VS x64 兼容 RDTSC ====================
static inline uint64_t rdtsc_begin()
{
	_mm_mfence();       // 同步指令
	unsigned long long val = __rdtsc();
	return val;
}

static inline uint64_t rdtsc_end()
{
	unsigned long long val = __rdtsc();
	_mm_lfence();       // 同步指令
	return val;
}

// ==================== VS 兼容 blackhole ====================
static inline void blackhole(const uint8_t* p, size_t n)
{
	uint64_t sink = 0;
	for (size_t i = 0; i < n; ++i)
		sink ^= p[i];
	checksum ^= sink;

	_ReadWriteBarrier();  // VS 内存屏障，禁止优化

	static int once = 1;
	if (once) {
		printf(" [blackhole triggered, checksum = %016llx]\n", checksum);
		once = 0;
	}
}








unsigned char Subkey[41][64];


unsigned char rc[40][32] = { 0x99,0x88,0x88,0xcc,0xcc,0x99,0xdd,0xdd,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0xff,0x00,0xee,0x44,0xaa,0x11,0xbb,0x55,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x22,0x11,0x33,0x55,0x77,0x00,0x66,0x44,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x88,0x33,0x99,0x77,0xdd,0x22,0xcc,0x66,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0xcc,0x77,0xdd,0x33,0x99,0x66,0x88,0x22,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x44,0xff,0x55,0xbb,0x11,0xee,0x00,0xaa,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x55,0xee,0x44,0xaa,0x00,0xff,0x11,0xbb,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x77,0xcc,0x66,0x88,0x22,0xdd,0x33,0x99,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x33,0x99,0x22,0xdd,0x66,0x88,0x77,0xcc,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0xbb,0x33,0xaa,0x77,0xee,0x22,0xff,0x66,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0xaa,0x77,0xbb,0x33,0xff,0x66,0xee,0x22,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x88,0xee,0x99,0xaa,0xdd,0xff,0xcc,0xbb,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0xdd,0xcc,0xcc,0x88,0x88,0xdd,0x99,0x99,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x77,0x88,0x66,0xcc,0x22,0x99,0x33,0xdd,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x33,0x00,0x22,0x44,0x66,0x11,0x77,0x55,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0xaa,0x11,0xbb,0x55,0xff,0x00,0xee,0x44,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x88,0x22,0x99,0x66,0xdd,0x33,0xcc,0x77,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0xcc,0x55,0xdd,0x11,0x99,0x44,0x88,0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x44,0xaa,0x55,0xee,0x11,0xbb,0x00,0xff,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x55,0x55,0x44,0x11,0x00,0x44,0x11,0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x66,0xbb,0x77,0xff,0x33,0xaa,0x22,0xee,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x11,0x77,0x00,0x33,0x44,0x66,0x55,0x22,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0xee,0xff,0xff,0xbb,0xbb,0xee,0xaa,0xaa,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x11,0xff,0x00,0xbb,0x44,0xee,0x55,0xaa,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0xff,0xff,0xee,0xbb,0xaa,0xee,0xbb,0xaa,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,//25
0x33,0xff,0x22,0xbb,0x66,0xee,0x77,0xaa,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,//26
0xbb,0xff,0xaa,0xbb,0xee,0xee,0xff,0xaa,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,//27
0xbb,0xee,0xaa,0xaa,0xee,0xff,0xff,0xbb,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,//28
0xbb,0xcc,0xaa,0x88,0xee,0xdd,0xff,0x99,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,//29
0xbb,0x99,0xaa,0xdd,0xee,0x88,0xff,0xcc,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,//30
0xbb,0x22,0xaa,0x66,0xee,0x33,0xff,0x77,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,//31
0xaa,0x55,0xbb,0x11,0xff,0x44,0xee,0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,//32
0x88,0xbb,0x99,0xff,0xdd,0xaa,0xcc,0xee,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,//33
0xdd,0x77,0xcc,0x33,0x88,0x66,0x99,0x22,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,//34
0x66,0xff,0x77,0xbb,0x33,0xee,0x22,0xaa,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,//35
0x11,0xee,0x00,0xaa,0x44,0xff,0x55,0xbb,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,//36
0xff,0xdd,0xee,0x99,0xaa,0xcc,0xbb,0x88,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,//37
0x33,0xaa,0x22,0xee,0x66,0xbb,0x77,0xff,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,//38
0xbb,0x44,0xaa,0x00,0xee,0x55,0xff,0x11,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,//39
0xaa,0x99,0xbb,0xdd,0xff,0x88,0xee,0xcc,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0 ,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0 };//40


unsigned char rrc[40][32] = { 0x9,0x8,0x8,0xc,0xc,0x9,0xd,0xd,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
0xf,0x0,0xe,0x4,0xa,0x1,0xb,0x5,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
0x2,0x1,0x3,0x5,0x7,0x0,0x6,0x4,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
0x8,0x3,0x9,0x7,0xd,0x2,0xc,0x6,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
0xc,0x7,0xd,0x3,0x9,0x6,0x8,0x2,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
0x4,0xf,0x5,0xb,0x1,0xe,0x0,0xa,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
0x5,0xe,0x4,0xa,0x0,0xf,0x1,0xb,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
0x7,0xc,0x6,0x8,0x2,0xd,0x3,0x9,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
0x3,0x9,0x2,0xd,0x6,0x8,0x7,0xc,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
0xb,0x3,0xa,0x7,0xe,0x2,0xf,0x6,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
0xa,0x7,0xb,0x3,0xf,0x6,0xe,0x2,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
0x8,0xe,0x9,0xa,0xd,0xf,0xc,0xb,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
0xd,0xc,0xc,0x8,0x8,0xd,0x9,0x9,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
0x7,0x8,0x6,0xc,0x2,0x9,0x3,0xd,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
0x3,0x0,0x2,0x4,0x6,0x1,0x7,0x5,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
0xa,0x1,0xb,0x5,0xf,0x0,0xe,0x4,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
0x8,0x2,0x9,0x6,0xd,0x3,0xc,0x7,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
0xc,0x5,0xd,0x1,0x9,0x4,0x8,0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
0x4,0xa,0x5,0xe,0x1,0xb,0x0,0xf,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
0x5,0x5,0x4,0x1,0x0,0x4,0x1,0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
0x6,0xb,0x7,0xf,0x3,0xa,0x2,0xe,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
0x1,0x7,0x0,0x3,0x4,0x6,0x5,0x2,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
0xe,0xf,0xf,0xb,0xb,0xe,0xa,0xa,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
0x1,0xf,0x0,0xb,0x4,0xe,0x5,0xa,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
0xF,0xF,0xE,0xB,0xA,0xE,0xB,0xA,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,//25
0x3,0xF,0x2,0xB,0x6,0xE,0x7,0xA,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,//26
0xB,0xF,0xA,0xB,0xE,0xE,0xF,0xA,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,//27
0xB,0xE,0xA,0xA,0xE,0xF,0xF,0xB,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,//28
0xB,0xC,0xA,0x8,0xE,0xD,0xF,0x9,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,//29
0xB,0x9,0xA,0xD,0xE,0x8,0xF,0xC,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,//30
0xB,0x2,0xA,0x6,0xE,0x3,0xF,0x7,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,//31
0xA,0x5,0xB,0x1,0xF,0x4,0xE,0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,//32
0x8,0xB,0x9,0xF,0xD,0xA,0xC,0xE,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,//33
0xD,0x7,0xC,0x3,0x8,0x6,0x9,0x2,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,//34
0x6,0xF,0x7,0xB,0x3,0xE,0x2,0xA,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,//35
0x1,0xE,0x0,0xA,0x4,0xF,0x5,0xB,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,//36
0xF,0xD,0xE,0x9,0xA,0xC,0xB,0x8,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,//37
0x3,0xA,0x2,0xE,0x6,0xB,0x7,0xF,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,//38
0xB,0x4,0xA,0x0,0xE,0x5,0xF,0x1,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,//39
0xA,0x9,0xB,0xD,0xF,0x8,0xE,0xC,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 ,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };//40


static const uint32_t RC[40][8] = {
0x0908080c,0x0c090d0d,0x0, 0x0,0x0, 0x0,0x0, 0x0,
0x0f000e04,0x0a010b05,0x0, 0x0,0x0, 0x0,0x0, 0x0,
0x02010305,0x07000604,0x0, 0x0, 0x0, 0x0,0x0, 0x0,
0x08030907,0x0d020c06,0x0, 0x0, 0x0, 0x0,0x0, 0x0,
0x0c070d03,0x09060802,0x0, 0x0,0x0, 0x0,0x0, 0x0,
0x040f050b,0x010e000a,0x0, 0x0, 0x0, 0x0,0x0, 0x0,
0x050e040a,0x000f010b,0x0, 0x0, 0x0, 0x0,0x0, 0x0,
0x070c0608,0x020d0309,0x0, 0x0,0x0, 0x0,0x0, 0x0,
0x0309020d,0x0608070c,0x0, 0x0, 0x0, 0x0,0x0, 0x0,
0x0b030a07,0x0e020f06,0x0, 0x0, 0x0, 0x0,0x0, 0x0,
0x0a070b03,0x0f060e02,0x0, 0x0, 0x0, 0x0,0x0, 0x0,
0x080e090a,0x0d0f0c0b,0x0, 0x0, 0x0, 0x0,0x0, 0x0,
0x0d0c0c08,0x080d0909,0x0, 0x0,0x0, 0x0,0x0, 0x0,
0x0708060c,0x0209030d,0x0, 0x0, 0x0, 0x0,0x0, 0x0,
0x03000204,0x06010705,0x0, 0x0,0x0, 0x0,0x0, 0x0,
0x0a010b05,0x0f000e04,0x0, 0x0, 0x0, 0x0,0x0, 0x0,
0x08020906,0x0d030c07,0x0, 0x0 };//40


//m is the m1,m2, u1=u1_x||u1_y (the 256-bit plaintext ), u2=u2_x||u2_y, u3=u3_x||u3_y,  u2||u3||m2||m3 (the 1024-bit key)
//u1是并行，u2，u3，m1，m2都是单行，及u1=(0xaa,0xaa,...),u2,u3,m1,m2=(0x0a,0x0a,...)

int CF_512(const unsigned char* m, __m256i* u1_x, __m256i* u1_y, __m256i* u2_x, __m256i* u2_y, int round)
{
	//首先，将m1，m2,m3进行分割
	__m256i S_L4 = _mm256_setr_epi8(0x30, 0x00, 0x60, 0x20, 0x50, 0x40, 0xf0, 0xe0, 0xa0, 0x80, 0x70, 0x90, 0x10, 0xc0, 0xd0, 0xb0, 0x30, 0x00, 0x60, 0x20, 0x50, 0x40, 0xf0, 0xe0, 0xa0, 0x80, 0x70, 0x90, 0x10, 0xc0, 0xd0, 0xb0);
	__m256i S = _mm256_setr_epi8(0x3, 0x0, 0x6, 0x2, 0x5, 0x4, 0xf, 0xe, 0xa, 0x8, 0x7, 0x9, 0x1, 0xc, 0xd, 0xb, 0x3, 0x0, 0x6, 0x2, 0x5, 0x4, 0xf, 0xe, 0xa, 0x8, 0x7, 0x9, 0x1, 0xc, 0xd, 0xb);
	__m256i S_Inv = _mm256_setr_epi8(0xc, 0xa, 0xe, 0xd, 0x1, 0xf, 0xb, 0x0, 0x7, 0x2, 0x5, 0x4, 0x3, 0x6, 0x9, 0x8, 0xc, 0xa, 0xe, 0xd, 0x1, 0xf, 0xb, 0x0, 0x7, 0x2, 0x5, 0x4, 0x3, 0x6, 0x9, 0x8);

	__m256i con = _mm256_set1_epi8(0x0f);//取右4-bit
	__m256i con_L4 = _mm256_set1_epi8(0xf0);//取左4-bit
	__m256i A1 = _mm256_setr_epi8(1, 2, 3, 4, 5, 6, 7, 0, 9, 10, 11, 12, 13, 14, 15, 8, 1, 2, 3, 4, 5, 6, 7, 0, 9, 10, 11, 12, 13, 14, 15, 8);
	__m256i A2 = _mm256_setr_epi8(2, 3, 4, 5, 6, 7, 0, 1, 10, 11, 12, 13, 14, 15, 8, 9, 2, 3, 4, 5, 6, 7, 0, 1, 10, 11, 12, 13, 14, 15, 8, 9);
	__m256i A3 = _mm256_setr_epi8(5, 6, 7, 0, 1, 2, 3, 4, 13, 14, 15, 8, 9, 10, 11, 12, 5, 6, 7, 0, 1, 2, 3, 4, 13, 14, 15, 8, 9, 10, 11, 12);
	__m256i PL = _mm256_setr_epi8(12, 2, 0, 10, 14, 6, 13, 7, 11, 1, 8, 15, 9, 3, 5, 4, 3, 10, 4, 15, 14, 5, 7, 13, 12, 0, 11, 1, 9, 2, 6, 8);
	__m256i PR = _mm256_setr_epi8(0, 2, 1, 12, 11, 8, 13, 7, 10, 4, 6, 5, 3, 14, 9, 15, 6, 2, 11, 7, 8, 15, 4, 12, 13, 1, 0, 3, 9, 14, 10, 5);
	__m256i A4 = _mm256_setr_epi8(3, 4, 5, 6, 7, 0, 1, 2, 11, 12, 13, 14, 15, 8, 9, 10, 3, 4, 5, 6, 7, 0, 1, 2, 11, 12, 13, 14, 15, 8, 9, 10);

	__m256i c1 = _mm256_setr_epi8(0, 0x80, 1, 0x80, 2, 0x80, 3, 0x80, 4, 0x80, 5, 0x80, 6, 0x80, 7, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
	__m256i c2 = _mm256_setr_epi8(0x80, 0, 0x80, 1, 0x80, 2, 0x80, 3, 0x80, 4, 0x80, 5, 0x80, 6, 0x80, 7, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
	__m256i c3 = _mm256_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 8, 0x80, 9, 0x80, 10, 0x80, 11, 0x80, 12, 0x80, 13, 0x80, 14, 0x80, 15, 0x80);
	__m256i c4 = _mm256_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 8, 0x80, 9, 0x80, 10, 0x80, 11, 0x80, 12, 0x80, 13, 0x80, 14, 0x80, 15);

	//__m256i con = _mm256_set1_epi8(0x0f);//取右4-bit
	__m256i c5 = _mm256_setr_epi8(0, 2, 4, 6, 8, 10, 12, 14, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
	__m256i c6 = _mm256_setr_epi8(1, 3, 5, 7, 9, 11, 13, 15, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
	__m256i c7 = _mm256_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0, 2, 4, 6, 8, 10, 12, 14, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
	__m256i c8 = _mm256_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 1, 3, 5, 7, 9, 11, 13, 15, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
	__m256i c55 = _mm256_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0, 2, 4, 6, 8, 10, 12, 14, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
	__m256i c66 = _mm256_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 1, 3, 5, 7, 9, 11, 13, 15, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
	__m256i c77 = _mm256_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0, 2, 4, 6, 8, 10, 12, 14);
	__m256i c88 = _mm256_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 1, 3, 5, 7, 9, 11, 13, 15);

	//G=[23, 3, 31, 2, 0, 20, 21, 15, 13, 29, 14, 30, 12, 28, 22, 1, 17, 24, 8, 5, 27, 16, 6, 11, 25, 18, 19, 10, 7, 9, 26, 4] (3cycle 8x32+ 1cycle two 16x4)
	//F = [11, 3, 14, 2, 9, 10, 12, 0, 4, 13, 7, 8, 15, 6, 5, 1, 16, 19, 27, 22, 31, 26, 20, 30, 23, 29, 25, 18, 24, 21, 28, 17](1 cycle two 16x4)

	__m256i F = _mm256_setr_epi8(11, 3, 14, 2, 9, 10, 12, 0, 4, 13, 7, 8, 15, 6, 5, 1, 0, 3, 11, 6, 15, 10, 4, 14, 7, 13, 9, 2, 8, 5, 12, 1);
	//__m256i P2 = _mm256_setr_epi8(8, 0, 6, 5, 3, 11, 7, 2, 14, 1, 15, 4, 10, 12, 13, 9, 13, 10, 1, 3, 12, 9, 14, 7, 6, 2, 15, 11, 8, 5, 4, 0);
	//G=G_Nibble + [2,4,5,6,0,1,3,7]
	__m256i G_Nibble = _mm256_setr_epi8(11, 3, 15, 2, 0, 8, 9, 7, 5, 13, 6, 14, 4, 12, 10, 1, 9, 12, 4, 1, 15, 8, 2, 7, 13, 10, 11, 6, 3, 5, 14, 0);
	__m256i G_Word = _mm256_setr_epi32(0, 3, 5, 7, 1, 2, 4, 6);

	int i;

	__m256i k, t1, t2, t3, t4, t5, t6, t7, t8;
	__m256i K0, K1, K0_L4, K1_L4, K2, K3, K2_L4, K3_L4, state_1_3, state_1_4, k_1;
	__m256i K4, K5, K4_L4, K5_L4, state_2_3, state_2_4, k_2;
	__m256i K6, K7, K6_L4, K7_L4, state_3_3, state_3_4, k_3;
	__m256i tG, k1, k3, tK1, tK2, tK3;
	__m256i KK0, KK1, KK2, KK3, KK4, KK5, KK6, KK7;

	__m256i state1, state2, state3, k_m;
	__m256i state1_1, state2_1, state3_1, k_m_1;
	__m256i state1_L4, state2_L4, state1_L4_R4, state2_L4_R4, state1_R4, state2_R4, k_L4, k_R4, k_L4_R4;
	__m256i Value1, state1_R4_L4, state2_R4_L4;
	__m256i Value2;
	__m256i u1_x_pre, u1_y_pre, u2_x_pre, u2_y_pre, u1_x_R4, u1_y_R4;
	unsigned char value1[32] = { 0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x10 };
	Value1 = _mm256_loadu_si256((__m256i*)value1);
	//unsigned char value2[32] = { 0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x2 };
	//Value2 = _mm256_loadu_si256((__m256i*)value2);
	//u2_x(K0),u2_y(K1)

	K0 = *u2_x;//K0=u2_x=(0x0a,0x0a,...)
	K1 = *u2_y;//K1=u2_y=(0x0a,0x0a,...)




	K0_L4 = _mm256_slli_epi16(K0, 4);
	KK0 = _mm256_xor_si256(K0, K0_L4);//KK0=(0xaa,0xaa,...)

	K1_L4 = _mm256_slli_epi16(K1, 4);
	KK1 = _mm256_xor_si256(K1, K1_L4);//KK1=(0xaa,0xaa,...)





	//m1_x(K4)/m1_y(K5)
//m1_x=(0x0a,0x0a,...)
//m1_y=(0x0a,0x0a,...)
	K3 = _mm256_loadu_si256((__m256i*)(m));
	K2 = _mm256_srli_epi16(K3, 4);
	K2 = _mm256_and_si256(K2, con);
	K3 = _mm256_and_si256(K3, con);

	state_2_3 = _mm256_permute2x128_si256(K2, K2, 1);
	state_2_4 = _mm256_permute2x128_si256(K3, K3, 1);
	t1 = _mm256_shuffle_epi8(K2, c1);
	t2 = _mm256_shuffle_epi8(state_2_3, c3);
	t3 = _mm256_shuffle_epi8(K3, c2);
	t4 = _mm256_shuffle_epi8(state_2_4, c4);
	t5 = _mm256_shuffle_epi8(state_2_3, c1);
	t6 = _mm256_shuffle_epi8(K2, c3);
	t7 = _mm256_shuffle_epi8(state_2_4, c2);
	t8 = _mm256_shuffle_epi8(K3, c4);

	K2 = _mm256_xor_si256(t1, t2);
	state_2_3 = _mm256_xor_si256(t3, t4);
	K2 = _mm256_xor_si256(K2, state_2_3);

	K3 = _mm256_xor_si256(t5, t6);
	state_2_4 = _mm256_xor_si256(t7, t8);
	K3 = _mm256_xor_si256(K3, state_2_4);


	K2_L4 = _mm256_slli_epi16(K2, 4);
	KK2 = _mm256_xor_si256(K2, K2_L4);//KK4=(0xaa,0xaa,...)

	K3_L4 = _mm256_slli_epi16(K3, 4);
	KK3 = _mm256_xor_si256(K3, K3_L4);//KK5=(0xaa,0xaa,...)




	//m1_x(K4)/m1_y(K5)
	//m1_x=(0x0a,0x0a,...)
	//m1_y=(0x0a,0x0a,...)
	K5 = _mm256_loadu_si256((__m256i*)(m + 32));
	K4 = _mm256_srli_epi16(K5, 4);
	K4 = _mm256_and_si256(K4, con);
	K5 = _mm256_and_si256(K5, con);

	state_2_3 = _mm256_permute2x128_si256(K4, K4, 1);
	state_2_4 = _mm256_permute2x128_si256(K5, K5, 1);
	t1 = _mm256_shuffle_epi8(K4, c1);
	t2 = _mm256_shuffle_epi8(state_2_3, c3);
	t3 = _mm256_shuffle_epi8(K5, c2);
	t4 = _mm256_shuffle_epi8(state_2_4, c4);
	t5 = _mm256_shuffle_epi8(state_2_3, c1);
	t6 = _mm256_shuffle_epi8(K4, c3);
	t7 = _mm256_shuffle_epi8(state_2_4, c2);
	t8 = _mm256_shuffle_epi8(K5, c4);

	K4 = _mm256_xor_si256(t1, t2);
	state_2_3 = _mm256_xor_si256(t3, t4);
	K4 = _mm256_xor_si256(K4, state_2_3);

	K5 = _mm256_xor_si256(t5, t6);
	state_2_4 = _mm256_xor_si256(t7, t8);
	K5 = _mm256_xor_si256(K5, state_2_4);


	K4_L4 = _mm256_slli_epi16(K4, 4);
	KK4 = _mm256_xor_si256(K4, K4_L4);//KK4=(0xaa,0xaa,...)

	K5_L4 = _mm256_slli_epi16(K5, 4);
	KK5 = _mm256_xor_si256(K5, K5_L4);//KK5=(0xaa,0xaa,...)






	//m2_x(K6)/m2_y(K7)
	//m2_x=(0x0a,0x0a,...)
	//m2_y=(0x0a,0x0a,...)
	K7 = _mm256_loadu_si256((__m256i*)(m + 64));
	K6 = _mm256_srli_epi16(K7, 4);
	K6 = _mm256_and_si256(K6, con);
	K7 = _mm256_and_si256(K7, con);

	state_3_3 = _mm256_permute2x128_si256(K6, K6, 1);
	state_3_4 = _mm256_permute2x128_si256(K7, K7, 1);
	t1 = _mm256_shuffle_epi8(K6, c1);
	t2 = _mm256_shuffle_epi8(state_3_3, c3);
	t3 = _mm256_shuffle_epi8(K7, c2);
	t4 = _mm256_shuffle_epi8(state_3_4, c4);
	t5 = _mm256_shuffle_epi8(state_3_3, c1);
	t6 = _mm256_shuffle_epi8(K6, c3);
	t7 = _mm256_shuffle_epi8(state_3_4, c2);
	t8 = _mm256_shuffle_epi8(K7, c4);

	K6 = _mm256_xor_si256(t1, t2);
	state_3_3 = _mm256_xor_si256(t3, t4);
	K6 = _mm256_xor_si256(K6, state_3_3);

	K7 = _mm256_xor_si256(t5, t6);
	state_3_4 = _mm256_xor_si256(t7, t8);
	K7 = _mm256_xor_si256(K7, state_3_4);

	K6_L4 = _mm256_slli_epi16(K6, 4);
	KK6 = _mm256_xor_si256(K6, K6_L4);//KK6=(0xaa,0xaa,...)

	K7_L4 = _mm256_slli_epi16(K7, 4);
	KK7 = _mm256_xor_si256(K7, K7_L4);//KK7=(0xaa,0xaa,...)






	//取u1_x和u1_y的R4位
	u1_x_R4 = _mm256_and_si256(*u1_x, con);
	u1_y_R4 = _mm256_and_si256(*u1_y, con);
	//明文部分的赋值

	//第一个大块和第二个大块的加密输入,第1个分支在右4位，第2个分支左4位
	state1 = *u1_x;//u1_x=(0xaa,0xaa,....)
	state2 = *u1_y;//u1_y=(0xaa,0xaa,....)

	//u1和u2的值还得再处理一下
	//state1,state2=0x(aa,0xaa,...)
	//u1和u2是按照(0xaa,0xaa,0xaa)来存储的，不是按照(0x0a,0x0a,0x0a,)来存储的
	state2 = _mm256_xor_si256(state2, Value1);//区分两个块不同的明文输入








	//第三个块加密输入
	//state1_1 = u1_x_R4;//u1_x=(0xaa,0xaa,....)
	//state2_1 = u1_y_R4;//u1_y=(0xaa,0xaa,....)

	//state1_1 = _mm256_and_si256(u1_x, con);//state1_1=0x(0a,0x0a,...)
	//state2_1 = _mm256_and_si256(u1_y, con);//state2_1=0x(0a,0x0a,...)

	//u1和u2是按照(0xaa,0xaa,0xaa)来存储的，不是按照(0x0a,0x0a,0x0a,)来存储的
	//state2_1 = _mm256_xor_si256(state2_1, Value2);//区分两个块不同的明文输入


	//第1个step不需要更新轮密钥
	//The first round
	state1 = _mm256_xor_si256(state1, KK0);
	state2 = _mm256_xor_si256(state2, KK1);
	state1_L4 = _mm256_and_si256(state1, con_L4);
	state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);
	state1_R4 = _mm256_and_si256(state1, con);
	state1_L4 = _mm256_shuffle_epi8(S_L4, state1_L4_R4);
	state1_R4 = _mm256_shuffle_epi8(S, state1_R4);
	state1 = _mm256_xor_si256(state1_L4, state1_R4);

	state2_L4 = _mm256_and_si256(state2, con_L4);
	state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);
	state2_R4 = _mm256_and_si256(state2, con);
	state2_L4 = _mm256_shuffle_epi8(S_L4, state2_L4_R4);
	state2_R4 = _mm256_shuffle_epi8(S, state2_R4);
	state2 = _mm256_xor_si256(state2_L4, state2_R4);

	//state2 = _mm256_shuffle_epi8(S, state2);

	state2 = _mm256_xor_si256(state2, state1);

	k_m = _mm256_shuffle_epi8(state2, A1);
	state1 = _mm256_xor_si256(state1, k_m);

	k_m = _mm256_shuffle_epi8(state1, A2);
	state2 = _mm256_xor_si256(state2, k_m);

	k_m = _mm256_shuffle_epi8(state2, A2);
	state1 = _mm256_xor_si256(state1, k_m);

	k_m = _mm256_shuffle_epi8(state1, A3);
	state2 = _mm256_xor_si256(state2, k_m);

	state1 = _mm256_xor_si256(state1, state2);

	/*
	The difference with uBlock round function
	*/
	state3 = _mm256_permute2x128_si256(state1, state1, 1);//Left and right change
	state1 = _mm256_shuffle_epi8(state3, PL);//Byte permutation within 128-bit

	state2 = _mm256_shuffle_epi8(state2, PR);//Byte permutation within 128-bit






	//The second round
	state1 = _mm256_xor_si256(state1, KK2);
	state2 = _mm256_xor_si256(state2, KK3);
	state1_L4 = _mm256_and_si256(state1, con_L4);
	state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);
	state1_R4 = _mm256_and_si256(state1, con);
	state1_L4 = _mm256_shuffle_epi8(S_L4, state1_L4_R4);
	state1_R4 = _mm256_shuffle_epi8(S, state1_R4);
	state1 = _mm256_xor_si256(state1_L4, state1_R4);

	state2_L4 = _mm256_and_si256(state2, con_L4);
	state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);
	state2_R4 = _mm256_and_si256(state2, con);
	state2_L4 = _mm256_shuffle_epi8(S_L4, state2_L4_R4);
	state2_R4 = _mm256_shuffle_epi8(S, state2_R4);
	state2 = _mm256_xor_si256(state2_L4, state2_R4);

	//state2 = _mm256_shuffle_epi8(S, state2);

	state2 = _mm256_xor_si256(state2, state1);

	k_m = _mm256_shuffle_epi8(state2, A1);
	state1 = _mm256_xor_si256(state1, k_m);

	k_m = _mm256_shuffle_epi8(state1, A2);
	state2 = _mm256_xor_si256(state2, k_m);

	k_m = _mm256_shuffle_epi8(state2, A2);
	state1 = _mm256_xor_si256(state1, k_m);

	k_m = _mm256_shuffle_epi8(state1, A3);
	state2 = _mm256_xor_si256(state2, k_m);

	state1 = _mm256_xor_si256(state1, state2);

	/*
	The difference with uBlock round function
	*/
	state3 = _mm256_permute2x128_si256(state1, state1, 1);//Left and right change
	state1 = _mm256_shuffle_epi8(state3, PL);//Byte permutation within 128-bit

	state2 = _mm256_shuffle_epi8(state2, PR);//Byte permutation within 128-bit





	//The third round
	state1 = _mm256_xor_si256(state1, KK4);
	state2 = _mm256_xor_si256(state2, KK5);
	state1_L4 = _mm256_and_si256(state1, con_L4);
	state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);
	state1_R4 = _mm256_and_si256(state1, con);
	state1_L4 = _mm256_shuffle_epi8(S_L4, state1_L4_R4);
	state1_R4 = _mm256_shuffle_epi8(S, state1_R4);
	state1 = _mm256_xor_si256(state1_L4, state1_R4);

	state2_L4 = _mm256_and_si256(state2, con_L4);
	state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);
	state2_R4 = _mm256_and_si256(state2, con);
	state2_L4 = _mm256_shuffle_epi8(S_L4, state2_L4_R4);
	state2_R4 = _mm256_shuffle_epi8(S, state2_R4);
	state2 = _mm256_xor_si256(state2_L4, state2_R4);

	//state2 = _mm256_shuffle_epi8(S, state2);

	state2 = _mm256_xor_si256(state2, state1);

	k_m = _mm256_shuffle_epi8(state2, A1);
	state1 = _mm256_xor_si256(state1, k_m);

	k_m = _mm256_shuffle_epi8(state1, A2);
	state2 = _mm256_xor_si256(state2, k_m);

	k_m = _mm256_shuffle_epi8(state2, A2);
	state1 = _mm256_xor_si256(state1, k_m);

	k_m = _mm256_shuffle_epi8(state1, A3);
	state2 = _mm256_xor_si256(state2, k_m);

	state1 = _mm256_xor_si256(state1, state2);

	/*
	The difference with uBlock round function
	*/
	state3 = _mm256_permute2x128_si256(state1, state1, 1);//Left and right change
	state1 = _mm256_shuffle_epi8(state3, PL);//Byte permutation within 128-bit

	state2 = _mm256_shuffle_epi8(state2, PR);//Byte permutation within 128-bit






	//The fourth round
	state1 = _mm256_xor_si256(state1, KK6);
	state2 = _mm256_xor_si256(state2, KK7);
	state1_L4 = _mm256_and_si256(state1, con_L4);
	state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);
	state1_R4 = _mm256_and_si256(state1, con);
	state1_L4 = _mm256_shuffle_epi8(S_L4, state1_L4_R4);
	state1_R4 = _mm256_shuffle_epi8(S, state1_R4);
	state1 = _mm256_xor_si256(state1_L4, state1_R4);

	state2_L4 = _mm256_and_si256(state2, con_L4);
	state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);
	state2_R4 = _mm256_and_si256(state2, con);
	state2_L4 = _mm256_shuffle_epi8(S_L4, state2_L4_R4);
	state2_R4 = _mm256_shuffle_epi8(S, state2_R4);
	state2 = _mm256_xor_si256(state2_L4, state2_R4);

	//state2 = _mm256_shuffle_epi8(S, state2);

	state2 = _mm256_xor_si256(state2, state1);

	k_m = _mm256_shuffle_epi8(state2, A1);
	state1 = _mm256_xor_si256(state1, k_m);

	k_m = _mm256_shuffle_epi8(state1, A2);
	state2 = _mm256_xor_si256(state2, k_m);

	k_m = _mm256_shuffle_epi8(state2, A2);
	state1 = _mm256_xor_si256(state1, k_m);

	k_m = _mm256_shuffle_epi8(state1, A3);
	state2 = _mm256_xor_si256(state2, k_m);

	state1 = _mm256_xor_si256(state1, state2);

	/*
	The difference with uBlock round function
	*/
	state3 = _mm256_permute2x128_si256(state1, state1, 1);//Left and right change
	state1 = _mm256_shuffle_epi8(state3, PL);//Byte permutation within 128-bit

	state2 = _mm256_shuffle_epi8(state2, PR);//Byte permutation within 128-bit






	//这里的round其实代表的是step

	for (i = 1; i < round; i++)
	{
		//The first round
		tK1 = KK0;
		tK2 = KK1;
		__m256i RoundConst = _mm256_setr_epi8(rc[i - 1][0], rc[i - 1][1], rc[i - 1][2], rc[i - 1][3], rc[i - 1][4], rc[i - 1][5], rc[i - 1][6], rc[i - 1][7], rc[i - 1][8], rc[i - 1][9], rc[i - 1][10], rc[i - 1][11], rc[i - 1][12], rc[i - 1][13], rc[i - 1][14], rc[i - 1][15], rc[i - 1][16], rc[i - 1][17], rc[i - 1][18], rc[i - 1][19], rc[i - 1][20], rc[i - 1][21], rc[i - 1][22], rc[i - 1][23], rc[i - 1][24], rc[i - 1][25], rc[i - 1][26], rc[i - 1][27], rc[i - 1][28], rc[i - 1][29], rc[i - 1][30], rc[i - 1][31]);
		//_mm256_setr_epi32(RC[i - 1][0], RC[i - 1][1], RC[i - 1][2], RC[i - 1][3], RC[i - 1][4], RC[i - 1][5], RC[i - 1][6], RC[i - 1][7]);


		//@@@@@@Bug is in here@@@@@@@
		KK0 = _mm256_xor_si256(KK2, RoundConst);
		KK0 = _mm256_shuffle_epi8(KK0, F);//the output after function P2
		//K0 = _mm256_shuffle_epi8(K2, F);//the output after function P2
		tG = _mm256_permutevar8x32_epi32(KK5, G_Word);
		KK1 = _mm256_shuffle_epi8(tG, G_Nibble);//the output after function G
		//得到K0，K1=（0x0a,0x0a,...）后生成KK0,KK1=(0xaa,0xaa,...)
		//K0_L4 = _mm256_slli_epi16(K0, 4);
		//KK0 = _mm256_xor_si256(K0, K0_L4);//KK0=(0xaa,0xaa,...)

		//K1_L4 = _mm256_slli_epi16(K1, 4);
		//KK1 = _mm256_xor_si256(K1, K1_L4);//KK1=(0xaa,0xaa,...)




		state1 = _mm256_xor_si256(state1, KK0);
		state2 = _mm256_xor_si256(state2, KK1);
		state1_L4 = _mm256_and_si256(state1, con_L4);
		state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);
		state1_R4 = _mm256_and_si256(state1, con);
		state1_L4 = _mm256_shuffle_epi8(S_L4, state1_L4_R4);
		state1_R4 = _mm256_shuffle_epi8(S, state1_R4);
		state1 = _mm256_xor_si256(state1_L4, state1_R4);

		state2_L4 = _mm256_and_si256(state2, con_L4);
		state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);
		state2_R4 = _mm256_and_si256(state2, con);
		state2_L4 = _mm256_shuffle_epi8(S_L4, state2_L4_R4);
		state2_R4 = _mm256_shuffle_epi8(S, state2_R4);
		state2 = _mm256_xor_si256(state2_L4, state2_R4);

		//state2 = _mm256_shuffle_epi8(S, state2);

		state2 = _mm256_xor_si256(state2, state1);

		k_m = _mm256_shuffle_epi8(state2, A1);
		state1 = _mm256_xor_si256(state1, k_m);

		k_m = _mm256_shuffle_epi8(state1, A2);
		state2 = _mm256_xor_si256(state2, k_m);

		k_m = _mm256_shuffle_epi8(state2, A2);
		state1 = _mm256_xor_si256(state1, k_m);

		k_m = _mm256_shuffle_epi8(state1, A3);
		state2 = _mm256_xor_si256(state2, k_m);

		state1 = _mm256_xor_si256(state1, state2);

		/*
		The difference with uBlock round function
		*/
		state3 = _mm256_permute2x128_si256(state1, state1, 1);//Left and right change
		state1 = _mm256_shuffle_epi8(state3, PL);//Byte permutation within 128-bit

		state2 = _mm256_shuffle_epi8(state2, PR);//Byte permutation within 128-bit





		//The second round
		KK2 = _mm256_shuffle_epi8(tK2, F);//the output after function P2
		tK2 = KK3;
		tG = _mm256_permutevar8x32_epi32(tK1, G_Word);
		KK3 = _mm256_shuffle_epi8(tG, G_Nibble);//the output after function G
		//得到K0，K1=（0x0a,0x0a,...）后生成KK0,KK1=(0xaa,0xaa,...)
		//K2_L4 = _mm256_slli_epi16(K2, 4);
		//KK2 = _mm256_xor_si256(K2, K2_L4);//KK0=(0xaa,0xaa,...)

		//K3_L4 = _mm256_slli_epi16(K3, 4);
		//KK3 = _mm256_xor_si256(K3, K3_L4);//KK1=(0xaa,0xaa,...)




		state1 = _mm256_xor_si256(state1, KK2);
		state2 = _mm256_xor_si256(state2, KK3);
		state1_L4 = _mm256_and_si256(state1, con_L4);
		state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);
		state1_R4 = _mm256_and_si256(state1, con);
		state1_L4 = _mm256_shuffle_epi8(S_L4, state1_L4_R4);
		state1_R4 = _mm256_shuffle_epi8(S, state1_R4);
		state1 = _mm256_xor_si256(state1_L4, state1_R4);

		state2_L4 = _mm256_and_si256(state2, con_L4);
		state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);
		state2_R4 = _mm256_and_si256(state2, con);
		state2_L4 = _mm256_shuffle_epi8(S_L4, state2_L4_R4);
		state2_R4 = _mm256_shuffle_epi8(S, state2_R4);
		state2 = _mm256_xor_si256(state2_L4, state2_R4);

		//state2 = _mm256_shuffle_epi8(S, state2);

		state2 = _mm256_xor_si256(state2, state1);

		k_m = _mm256_shuffle_epi8(state2, A1);
		state1 = _mm256_xor_si256(state1, k_m);

		k_m = _mm256_shuffle_epi8(state1, A2);
		state2 = _mm256_xor_si256(state2, k_m);

		k_m = _mm256_shuffle_epi8(state2, A2);
		state1 = _mm256_xor_si256(state1, k_m);

		k_m = _mm256_shuffle_epi8(state1, A3);
		state2 = _mm256_xor_si256(state2, k_m);

		state1 = _mm256_xor_si256(state1, state2);

		/*
		The difference with uBlock round function
		*/
		state3 = _mm256_permute2x128_si256(state1, state1, 1);//Left and right change
		state1 = _mm256_shuffle_epi8(state3, PL);//Byte permutation within 128-bit

		state2 = _mm256_shuffle_epi8(state2, PR);//Byte permutation within 128-bit






		//The third round
		tK1 = KK4;
		KK4 = _mm256_shuffle_epi8(tK2, F);//the output after function P2

		tG = _mm256_permutevar8x32_epi32(KK6, G_Word);
		KK5 = _mm256_shuffle_epi8(tG, G_Nibble);//the output after function G
		//得到K0，K1=（0x0a,0x0a,...）后生成KK0,KK1=(0xaa,0xaa,...)
		//K4_L4 = _mm256_slli_epi16(K4, 4);
		//KK4 = _mm256_xor_si256(K4, K4_L4);//KK0=(0xaa,0xaa,...)

		//K5_L4 = _mm256_slli_epi16(K5, 4);
		//KK5 = _mm256_xor_si256(K5, K5_L4);//KK1=(0xaa,0xaa,...)




		state1 = _mm256_xor_si256(state1, KK4);
		state2 = _mm256_xor_si256(state2, KK5);
		state1_L4 = _mm256_and_si256(state1, con_L4);
		state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);
		state1_R4 = _mm256_and_si256(state1, con);
		state1_L4 = _mm256_shuffle_epi8(S_L4, state1_L4_R4);
		state1_R4 = _mm256_shuffle_epi8(S, state1_R4);
		state1 = _mm256_xor_si256(state1_L4, state1_R4);

		state2_L4 = _mm256_and_si256(state2, con_L4);
		state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);
		state2_R4 = _mm256_and_si256(state2, con);
		state2_L4 = _mm256_shuffle_epi8(S_L4, state2_L4_R4);
		state2_R4 = _mm256_shuffle_epi8(S, state2_R4);
		state2 = _mm256_xor_si256(state2_L4, state2_R4);

		//state2 = _mm256_shuffle_epi8(S, state2);

		state2 = _mm256_xor_si256(state2, state1);

		k_m = _mm256_shuffle_epi8(state2, A1);
		state1 = _mm256_xor_si256(state1, k_m);

		k_m = _mm256_shuffle_epi8(state1, A2);
		state2 = _mm256_xor_si256(state2, k_m);

		k_m = _mm256_shuffle_epi8(state2, A2);
		state1 = _mm256_xor_si256(state1, k_m);

		k_m = _mm256_shuffle_epi8(state1, A3);
		state2 = _mm256_xor_si256(state2, k_m);

		state1 = _mm256_xor_si256(state1, state2);

		/*
		The difference with uBlock round function
		*/
		state3 = _mm256_permute2x128_si256(state1, state1, 1);//Left and right change
		state1 = _mm256_shuffle_epi8(state3, PL);//Byte permutation within 128-bit

		state2 = _mm256_shuffle_epi8(state2, PR);//Byte permutation within 128-bit







		//The fourth round

		KK6 = _mm256_shuffle_epi8(KK7, F);//the output after function P2

		tG = _mm256_permutevar8x32_epi32(tK1, G_Word);
		KK7 = _mm256_shuffle_epi8(tG, G_Nibble);//the output after function G
		//得到K0，K1=（0x0a,0x0a,...）后生成KK0,KK1=(0xaa,0xaa,...)
		//K6_L4 = _mm256_slli_epi16(K6, 4);
		//KK6 = _mm256_xor_si256(K6, K6_L4);//KK0=(0xaa,0xaa,...)

		//K7_L4 = _mm256_slli_epi16(K7, 4);
		//KK7 = _mm256_xor_si256(K7, K7_L4);//KK1=(0xaa,0xaa,...)



		state1 = _mm256_xor_si256(state1, KK6);
		state2 = _mm256_xor_si256(state2, KK7);
		state1_L4 = _mm256_and_si256(state1, con_L4);
		state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);
		state1_R4 = _mm256_and_si256(state1, con);
		state1_L4 = _mm256_shuffle_epi8(S_L4, state1_L4_R4);
		state1_R4 = _mm256_shuffle_epi8(S, state1_R4);
		state1 = _mm256_xor_si256(state1_L4, state1_R4);

		state2_L4 = _mm256_and_si256(state2, con_L4);
		state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);
		state2_R4 = _mm256_and_si256(state2, con);
		state2_L4 = _mm256_shuffle_epi8(S_L4, state2_L4_R4);
		state2_R4 = _mm256_shuffle_epi8(S, state2_R4);
		state2 = _mm256_xor_si256(state2_L4, state2_R4);

		//state2 = _mm256_shuffle_epi8(S, state2);

		state2 = _mm256_xor_si256(state2, state1);

		k_m = _mm256_shuffle_epi8(state2, A1);
		state1 = _mm256_xor_si256(state1, k_m);

		k_m = _mm256_shuffle_epi8(state1, A2);
		state2 = _mm256_xor_si256(state2, k_m);

		k_m = _mm256_shuffle_epi8(state2, A2);
		state1 = _mm256_xor_si256(state1, k_m);

		k_m = _mm256_shuffle_epi8(state1, A3);
		state2 = _mm256_xor_si256(state2, k_m);

		state1 = _mm256_xor_si256(state1, state2);

		/*
		The difference with uBlock round function
		*/
		state3 = _mm256_permute2x128_si256(state1, state1, 1);//Left and right change
		state1 = _mm256_shuffle_epi8(state3, PL);//Byte permutation within 128-bit

		state2 = _mm256_shuffle_epi8(state2, PR);//Byte permutation within 128-bit




	}


	//*** Modify***
	//Update in V20, removing the white key XOR in the end of the round function
	//state1 = _mm256_xor_si256(state1, KK0);
	//state2 = _mm256_xor_si256(state2, KK1);





	/*
	* ==========================================================================
	* 反馈操作，完成后输出u1=（0xaa,...）,u2,u3=(0x0a,....)
	* ==========================================================================
	*/
	//添加反馈操作，放到前面减少指令
	state1 = _mm256_xor_si256(state1, *u1_x);
	state2 = _mm256_xor_si256(state2, *u1_y);




	state1_L4 = _mm256_and_si256(state1, con_L4);
	state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);

	state1_R4 = _mm256_and_si256(state1, con);
	state1_R4_L4 = _mm256_slli_epi16(state1_R4, 4);

	state2_L4 = _mm256_and_si256(state2, con_L4);
	state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);

	state2_R4 = _mm256_and_si256(state2, con);
	state2_R4_L4 = _mm256_slli_epi16(state2_R4, 4);




	*u1_x = _mm256_xor_si256(state1_R4, state1_R4_L4);//u1_x,u1_y=(0xaa,0xaa,...)
	*u1_y = _mm256_xor_si256(state2_R4, state2_R4_L4);

	*u2_x = state1_L4_R4;//u2_x,u2_y=(0x0a,0x0a,...)
	*u2_y = state2_L4_R4;









	return 0;
}



void CF_512_Final(unsigned char* m, __m256i* u1_x, __m256i* u1_y, __m256i* u2_x, __m256i* u2_y, int round)
{
	//首先，将m1，m2,m3进行分割
	__m256i S_L4 = _mm256_setr_epi8(0x30, 0x00, 0x60, 0x20, 0x50, 0x40, 0xf0, 0xe0, 0xa0, 0x80, 0x70, 0x90, 0x10, 0xc0, 0xd0, 0xb0, 0x30, 0x00, 0x60, 0x20, 0x50, 0x40, 0xf0, 0xe0, 0xa0, 0x80, 0x70, 0x90, 0x10, 0xc0, 0xd0, 0xb0);
	__m256i S = _mm256_setr_epi8(0x3, 0x0, 0x6, 0x2, 0x5, 0x4, 0xf, 0xe, 0xa, 0x8, 0x7, 0x9, 0x1, 0xc, 0xd, 0xb, 0x3, 0x0, 0x6, 0x2, 0x5, 0x4, 0xf, 0xe, 0xa, 0x8, 0x7, 0x9, 0x1, 0xc, 0xd, 0xb);
	__m256i S_Inv = _mm256_setr_epi8(0xc, 0xa, 0xe, 0xd, 0x1, 0xf, 0xb, 0x0, 0x7, 0x2, 0x5, 0x4, 0x3, 0x6, 0x9, 0x8, 0xc, 0xa, 0xe, 0xd, 0x1, 0xf, 0xb, 0x0, 0x7, 0x2, 0x5, 0x4, 0x3, 0x6, 0x9, 0x8);

	__m256i con = _mm256_set1_epi8(0x0f);//取右4-bit
	__m256i con_L4 = _mm256_set1_epi8(0xf0);//取左4-bit
	__m256i A1 = _mm256_setr_epi8(1, 2, 3, 4, 5, 6, 7, 0, 9, 10, 11, 12, 13, 14, 15, 8, 1, 2, 3, 4, 5, 6, 7, 0, 9, 10, 11, 12, 13, 14, 15, 8);
	__m256i A2 = _mm256_setr_epi8(2, 3, 4, 5, 6, 7, 0, 1, 10, 11, 12, 13, 14, 15, 8, 9, 2, 3, 4, 5, 6, 7, 0, 1, 10, 11, 12, 13, 14, 15, 8, 9);
	__m256i A3 = _mm256_setr_epi8(5, 6, 7, 0, 1, 2, 3, 4, 13, 14, 15, 8, 9, 10, 11, 12, 5, 6, 7, 0, 1, 2, 3, 4, 13, 14, 15, 8, 9, 10, 11, 12);
	__m256i PL = _mm256_setr_epi8(12, 2, 0, 10, 14, 6, 13, 7, 11, 1, 8, 15, 9, 3, 5, 4, 3, 10, 4, 15, 14, 5, 7, 13, 12, 0, 11, 1, 9, 2, 6, 8);
	__m256i PR = _mm256_setr_epi8(0, 2, 1, 12, 11, 8, 13, 7, 10, 4, 6, 5, 3, 14, 9, 15, 6, 2, 11, 7, 8, 15, 4, 12, 13, 1, 0, 3, 9, 14, 10, 5);
	__m256i A4 = _mm256_setr_epi8(3, 4, 5, 6, 7, 0, 1, 2, 11, 12, 13, 14, 15, 8, 9, 10, 3, 4, 5, 6, 7, 0, 1, 2, 11, 12, 13, 14, 15, 8, 9, 10);

	__m256i c1 = _mm256_setr_epi8(0, 0x80, 1, 0x80, 2, 0x80, 3, 0x80, 4, 0x80, 5, 0x80, 6, 0x80, 7, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
	__m256i c2 = _mm256_setr_epi8(0x80, 0, 0x80, 1, 0x80, 2, 0x80, 3, 0x80, 4, 0x80, 5, 0x80, 6, 0x80, 7, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
	__m256i c3 = _mm256_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 8, 0x80, 9, 0x80, 10, 0x80, 11, 0x80, 12, 0x80, 13, 0x80, 14, 0x80, 15, 0x80);
	__m256i c4 = _mm256_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 8, 0x80, 9, 0x80, 10, 0x80, 11, 0x80, 12, 0x80, 13, 0x80, 14, 0x80, 15);


	__m256i c5 = _mm256_setr_epi8(0, 2, 4, 6, 8, 10, 12, 14, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
	__m256i c6 = _mm256_setr_epi8(1, 3, 5, 7, 9, 11, 13, 15, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
	__m256i c7 = _mm256_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0, 2, 4, 6, 8, 10, 12, 14, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
	__m256i c8 = _mm256_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 1, 3, 5, 7, 9, 11, 13, 15, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
	__m256i c55 = _mm256_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0, 2, 4, 6, 8, 10, 12, 14, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
	__m256i c66 = _mm256_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 1, 3, 5, 7, 9, 11, 13, 15, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
	__m256i c77 = _mm256_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0, 2, 4, 6, 8, 10, 12, 14);
	__m256i c88 = _mm256_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 1, 3, 5, 7, 9, 11, 13, 15);

	//G=[23, 3, 31, 2, 0, 20, 21, 15, 13, 29, 14, 30, 12, 28, 22, 1, 17, 24, 8, 5, 27, 16, 6, 11, 25, 18, 19, 10, 7, 9, 26, 4] (3cycle 8x32+ 1cycle two 16x4)
	//F = [11, 3, 14, 2, 9, 10, 12, 0, 4, 13, 7, 8, 15, 6, 5, 1, 16, 19, 27, 22, 31, 26, 20, 30, 23, 29, 25, 18, 24, 21, 28, 17](1 cycle two 16x4)

	__m256i F = _mm256_setr_epi8(11, 3, 14, 2, 9, 10, 12, 0, 4, 13, 7, 8, 15, 6, 5, 1, 0, 3, 11, 6, 15, 10, 4, 14, 7, 13, 9, 2, 8, 5, 12, 1);
	//__m256i P2 = _mm256_setr_epi8(8, 0, 6, 5, 3, 11, 7, 2, 14, 1, 15, 4, 10, 12, 13, 9, 13, 10, 1, 3, 12, 9, 14, 7, 6, 2, 15, 11, 8, 5, 4, 0);
	//G=G_Nibble + [2,4,5,6,0,1,3,7]
	__m256i G_Nibble = _mm256_setr_epi8(11, 3, 15, 2, 0, 8, 9, 7, 5, 13, 6, 14, 4, 12, 10, 1, 9, 12, 4, 1, 15, 8, 2, 7, 13, 10, 11, 6, 3, 5, 14, 0);
	__m256i G_Word = _mm256_setr_epi32(0, 3, 5, 7, 1, 2, 4, 6);


	int i;

	__m256i k, t1, t2, t3, t4, t5, t6, t7, t8;
	__m256i K0, K1, K0_L4, K1_L4, K2, K3, K2_L4, K3_L4, state_1_3, state_1_4, k_1;
	__m256i K4, K5, K4_L4, K5_L4, state_2_3, state_2_4, k_2;
	__m256i K6, K7, K6_L4, K7_L4, state_3_3, state_3_4, k_3;
	__m256i tG, k1, k3, tK1, tK2, tK3;
	__m256i KK0, KK1, KK2, KK3, KK4, KK5, KK6, KK7;

	__m256i state1, state2, state3, k_m;
	__m256i state1_1, state2_1, state3_1, k_m_1;
	__m256i state1_L4, state2_L4, state1_L4_R4, state2_L4_R4, state1_R4, state2_R4, k_L4, k_R4, k_L4_R4;
	__m256i Value1, state1_R4_L4, state2_R4_L4;
	__m256i Value2, ValueK;
	__m256i u1_x_pre, u1_y_pre, u2_x_pre, u2_y_pre, u1_x_R4, u1_y_R4;
	//unsigned char value1[32] = { 0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x54 };

	//*** Modify1***
	//The Value1 is the same as the first l-1 block
	unsigned char value1[32] = { 0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x10 };

	Value1 = _mm256_loadu_si256((__m256i*)value1);


	//*** Modify2***
	//The difference is in the master Key in the final step
	unsigned char valueK[32] = { 0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x10,0x0f };

	ValueK = _mm256_loadu_si256((__m256i*)valueK);

	//unsigned char value2[32] = { 0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x2 };
	//Value2 = _mm256_loadu_si256((__m256i*)value2);
	//u2_x(K0),u2_y(K1)

	K0 = *u2_x;//K0=u2_x=(0x0a,0x0a,...)
	K1 = *u2_y;//K1=u2_y=(0x0a,0x0a,...)




	K0_L4 = _mm256_slli_epi16(K0, 4);
	KK0 = _mm256_xor_si256(K0, K0_L4);//KK0=(0xaa,0xaa,...)

	K1_L4 = _mm256_slli_epi16(K1, 4);
	KK1 = _mm256_xor_si256(K1, K1_L4);//KK1=(0xaa,0xaa,...)





	//m1_x(K4)/m1_y(K5)
//m1_x=(0x0a,0x0a,...)
//m1_y=(0x0a,0x0a,...)
	K3 = _mm256_loadu_si256((__m256i*)(m));
	K2 = _mm256_srli_epi16(K3, 4);
	K2 = _mm256_and_si256(K2, con);
	K3 = _mm256_and_si256(K3, con);

	state_2_3 = _mm256_permute2x128_si256(K2, K2, 1);
	state_2_4 = _mm256_permute2x128_si256(K3, K3, 1);
	t1 = _mm256_shuffle_epi8(K2, c1);
	t2 = _mm256_shuffle_epi8(state_2_3, c3);
	t3 = _mm256_shuffle_epi8(K3, c2);
	t4 = _mm256_shuffle_epi8(state_2_4, c4);
	t5 = _mm256_shuffle_epi8(state_2_3, c1);
	t6 = _mm256_shuffle_epi8(K2, c3);
	t7 = _mm256_shuffle_epi8(state_2_4, c2);
	t8 = _mm256_shuffle_epi8(K3, c4);

	K2 = _mm256_xor_si256(t1, t2);
	state_2_3 = _mm256_xor_si256(t3, t4);
	K2 = _mm256_xor_si256(K2, state_2_3);

	K3 = _mm256_xor_si256(t5, t6);
	state_2_4 = _mm256_xor_si256(t7, t8);
	K3 = _mm256_xor_si256(K3, state_2_4);


	K2_L4 = _mm256_slli_epi16(K2, 4);
	KK2 = _mm256_xor_si256(K2, K2_L4);//KK4=(0xaa,0xaa,...)

	K3_L4 = _mm256_slli_epi16(K3, 4);
	KK3 = _mm256_xor_si256(K3, K3_L4);//KK5=(0xaa,0xaa,...)






	//m1_x(K4)/m1_y(K5)
	//m1_x=(0x0a,0x0a,...)
	//m1_y=(0x0a,0x0a,...)
	K5 = _mm256_loadu_si256((__m256i*)(m + 32));
	K4 = _mm256_srli_epi16(K5, 4);
	K4 = _mm256_and_si256(K4, con);
	K5 = _mm256_and_si256(K5, con);

	state_2_3 = _mm256_permute2x128_si256(K4, K4, 1);
	state_2_4 = _mm256_permute2x128_si256(K5, K5, 1);
	t1 = _mm256_shuffle_epi8(K4, c1);
	t2 = _mm256_shuffle_epi8(state_2_3, c3);
	t3 = _mm256_shuffle_epi8(K5, c2);
	t4 = _mm256_shuffle_epi8(state_2_4, c4);
	t5 = _mm256_shuffle_epi8(state_2_3, c1);
	t6 = _mm256_shuffle_epi8(K4, c3);
	t7 = _mm256_shuffle_epi8(state_2_4, c2);
	t8 = _mm256_shuffle_epi8(K5, c4);

	K4 = _mm256_xor_si256(t1, t2);
	state_2_3 = _mm256_xor_si256(t3, t4);
	K4 = _mm256_xor_si256(K4, state_2_3);

	K5 = _mm256_xor_si256(t5, t6);
	state_2_4 = _mm256_xor_si256(t7, t8);
	K5 = _mm256_xor_si256(K5, state_2_4);


	K4_L4 = _mm256_slli_epi16(K4, 4);
	KK4 = _mm256_xor_si256(K4, K4_L4);//KK4=(0xaa,0xaa,...)

	K5_L4 = _mm256_slli_epi16(K5, 4);
	KK5 = _mm256_xor_si256(K5, K5_L4);//KK5=(0xaa,0xaa,...)





	//m2_x(K6)/m2_y(K7)
	//m2_x=(0x0a,0x0a,...)
	//m2_y=(0x0a,0x0a,...)
	K7 = _mm256_loadu_si256((__m256i*)(m + 64));
	K6 = _mm256_srli_epi16(K7, 4);
	K6 = _mm256_and_si256(K6, con);
	K7 = _mm256_and_si256(K7, con);

	state_3_3 = _mm256_permute2x128_si256(K6, K6, 1);
	state_3_4 = _mm256_permute2x128_si256(K7, K7, 1);
	t1 = _mm256_shuffle_epi8(K6, c1);
	t2 = _mm256_shuffle_epi8(state_3_3, c3);
	t3 = _mm256_shuffle_epi8(K7, c2);
	t4 = _mm256_shuffle_epi8(state_3_4, c4);
	t5 = _mm256_shuffle_epi8(state_3_3, c1);
	t6 = _mm256_shuffle_epi8(K6, c3);
	t7 = _mm256_shuffle_epi8(state_3_4, c2);
	t8 = _mm256_shuffle_epi8(K7, c4);

	K6 = _mm256_xor_si256(t1, t2);
	state_3_3 = _mm256_xor_si256(t3, t4);
	K6 = _mm256_xor_si256(K6, state_3_3);

	K7 = _mm256_xor_si256(t5, t6);
	state_3_4 = _mm256_xor_si256(t7, t8);
	K7 = _mm256_xor_si256(K7, state_3_4);

	K6_L4 = _mm256_slli_epi16(K6, 4);
	KK6 = _mm256_xor_si256(K6, K6_L4);//KK6=(0xaa,0xaa,...)

	K7_L4 = _mm256_slli_epi16(K7, 4);
	KK7 = _mm256_xor_si256(K7, K7_L4);//KK7=(0xaa,0xaa,...)

	//***Modify2***, the difference is shown in the master key in different brunch
	KK7 = _mm256_xor_si256(KK7, ValueK);






	//取u1_x和u1_y的R4位
	u1_x_R4 = _mm256_and_si256(*u1_x, con);
	u1_y_R4 = _mm256_and_si256(*u1_y, con);
	//明文部分的赋值

	//第一个大块和第二个大块的加密输入,第1个分支在右4位，第2个分支左4位
	state1 = *u1_x;//u1_x=(0xaa,0xaa,....)
	state2 = *u1_y;//u1_y=(0xaa,0xaa,....)

	//u1和u2的值还得再处理一下
	//state1,state2=0x(aa,0xaa,...)
	//u1和u2是按照(0xaa,0xaa,0xaa)来存储的，不是按照(0x0a,0x0a,0x0a,)来存储的
	state2 = _mm256_xor_si256(state2, Value1);//区分两个块不同的明文输入









	//第三个块加密输入
	//state1_1 = u1_x_R4;//u1_x=(0xaa,0xaa,....)
	//state2_1 = u1_y_R4;//u1_y=(0xaa,0xaa,....)

	//state1_1 = _mm256_and_si256(u1_x, con);//state1_1=0x(0a,0x0a,...)
	//state2_1 = _mm256_and_si256(u1_y, con);//state2_1=0x(0a,0x0a,...)

	//u1和u2是按照(0xaa,0xaa,0xaa)来存储的，不是按照(0x0a,0x0a,0x0a,)来存储的
	//state2_1 = _mm256_xor_si256(state2_1, Value2);//区分两个块不同的明文输入


	//第1个step不需要更新轮密钥
	//The first round
	state1 = _mm256_xor_si256(state1, KK0);
	state2 = _mm256_xor_si256(state2, KK1);
	state1_L4 = _mm256_and_si256(state1, con_L4);
	state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);
	state1_R4 = _mm256_and_si256(state1, con);
	state1_L4 = _mm256_shuffle_epi8(S_L4, state1_L4_R4);
	state1_R4 = _mm256_shuffle_epi8(S, state1_R4);
	state1 = _mm256_xor_si256(state1_L4, state1_R4);

	state2_L4 = _mm256_and_si256(state2, con_L4);
	state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);
	state2_R4 = _mm256_and_si256(state2, con);
	state2_L4 = _mm256_shuffle_epi8(S_L4, state2_L4_R4);
	state2_R4 = _mm256_shuffle_epi8(S, state2_R4);
	state2 = _mm256_xor_si256(state2_L4, state2_R4);

	//state2 = _mm256_shuffle_epi8(S, state2);

	state2 = _mm256_xor_si256(state2, state1);

	k_m = _mm256_shuffle_epi8(state2, A1);
	state1 = _mm256_xor_si256(state1, k_m);

	k_m = _mm256_shuffle_epi8(state1, A2);
	state2 = _mm256_xor_si256(state2, k_m);

	k_m = _mm256_shuffle_epi8(state2, A2);
	state1 = _mm256_xor_si256(state1, k_m);

	k_m = _mm256_shuffle_epi8(state1, A3);
	state2 = _mm256_xor_si256(state2, k_m);

	state1 = _mm256_xor_si256(state1, state2);

	/*
	The difference with uBlock round function
	*/
	state3 = _mm256_permute2x128_si256(state1, state1, 1);//Left and right change
	state1 = _mm256_shuffle_epi8(state3, PL);//Byte permutation within 128-bit

	state2 = _mm256_shuffle_epi8(state2, PR);//Byte permutation within 128-bit






	//The second round
	state1 = _mm256_xor_si256(state1, KK2);
	state2 = _mm256_xor_si256(state2, KK3);
	state1_L4 = _mm256_and_si256(state1, con_L4);
	state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);
	state1_R4 = _mm256_and_si256(state1, con);
	state1_L4 = _mm256_shuffle_epi8(S_L4, state1_L4_R4);
	state1_R4 = _mm256_shuffle_epi8(S, state1_R4);
	state1 = _mm256_xor_si256(state1_L4, state1_R4);

	state2_L4 = _mm256_and_si256(state2, con_L4);
	state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);
	state2_R4 = _mm256_and_si256(state2, con);
	state2_L4 = _mm256_shuffle_epi8(S_L4, state2_L4_R4);
	state2_R4 = _mm256_shuffle_epi8(S, state2_R4);
	state2 = _mm256_xor_si256(state2_L4, state2_R4);

	//state2 = _mm256_shuffle_epi8(S, state2);

	state2 = _mm256_xor_si256(state2, state1);

	k_m = _mm256_shuffle_epi8(state2, A1);
	state1 = _mm256_xor_si256(state1, k_m);

	k_m = _mm256_shuffle_epi8(state1, A2);
	state2 = _mm256_xor_si256(state2, k_m);

	k_m = _mm256_shuffle_epi8(state2, A2);
	state1 = _mm256_xor_si256(state1, k_m);

	k_m = _mm256_shuffle_epi8(state1, A3);
	state2 = _mm256_xor_si256(state2, k_m);

	state1 = _mm256_xor_si256(state1, state2);

	/*
	The difference with uBlock round function
	*/
	state3 = _mm256_permute2x128_si256(state1, state1, 1);//Left and right change
	state1 = _mm256_shuffle_epi8(state3, PL);//Byte permutation within 128-bit

	state2 = _mm256_shuffle_epi8(state2, PR);//Byte permutation within 128-bit





	//The third round
	state1 = _mm256_xor_si256(state1, KK4);
	state2 = _mm256_xor_si256(state2, KK5);
	state1_L4 = _mm256_and_si256(state1, con_L4);
	state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);
	state1_R4 = _mm256_and_si256(state1, con);
	state1_L4 = _mm256_shuffle_epi8(S_L4, state1_L4_R4);
	state1_R4 = _mm256_shuffle_epi8(S, state1_R4);
	state1 = _mm256_xor_si256(state1_L4, state1_R4);

	state2_L4 = _mm256_and_si256(state2, con_L4);
	state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);
	state2_R4 = _mm256_and_si256(state2, con);
	state2_L4 = _mm256_shuffle_epi8(S_L4, state2_L4_R4);
	state2_R4 = _mm256_shuffle_epi8(S, state2_R4);
	state2 = _mm256_xor_si256(state2_L4, state2_R4);

	//state2 = _mm256_shuffle_epi8(S, state2);

	state2 = _mm256_xor_si256(state2, state1);

	k_m = _mm256_shuffle_epi8(state2, A1);
	state1 = _mm256_xor_si256(state1, k_m);

	k_m = _mm256_shuffle_epi8(state1, A2);
	state2 = _mm256_xor_si256(state2, k_m);

	k_m = _mm256_shuffle_epi8(state2, A2);
	state1 = _mm256_xor_si256(state1, k_m);

	k_m = _mm256_shuffle_epi8(state1, A3);
	state2 = _mm256_xor_si256(state2, k_m);

	state1 = _mm256_xor_si256(state1, state2);

	/*
	The difference with uBlock round function
	*/
	state3 = _mm256_permute2x128_si256(state1, state1, 1);//Left and right change
	state1 = _mm256_shuffle_epi8(state3, PL);//Byte permutation within 128-bit

	state2 = _mm256_shuffle_epi8(state2, PR);//Byte permutation within 128-bit






	//The fourth round
	state1 = _mm256_xor_si256(state1, KK6);
	state2 = _mm256_xor_si256(state2, KK7);
	state1_L4 = _mm256_and_si256(state1, con_L4);
	state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);
	state1_R4 = _mm256_and_si256(state1, con);
	state1_L4 = _mm256_shuffle_epi8(S_L4, state1_L4_R4);
	state1_R4 = _mm256_shuffle_epi8(S, state1_R4);
	state1 = _mm256_xor_si256(state1_L4, state1_R4);

	state2_L4 = _mm256_and_si256(state2, con_L4);
	state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);
	state2_R4 = _mm256_and_si256(state2, con);
	state2_L4 = _mm256_shuffle_epi8(S_L4, state2_L4_R4);
	state2_R4 = _mm256_shuffle_epi8(S, state2_R4);
	state2 = _mm256_xor_si256(state2_L4, state2_R4);

	//state2 = _mm256_shuffle_epi8(S, state2);

	state2 = _mm256_xor_si256(state2, state1);

	k_m = _mm256_shuffle_epi8(state2, A1);
	state1 = _mm256_xor_si256(state1, k_m);

	k_m = _mm256_shuffle_epi8(state1, A2);
	state2 = _mm256_xor_si256(state2, k_m);

	k_m = _mm256_shuffle_epi8(state2, A2);
	state1 = _mm256_xor_si256(state1, k_m);

	k_m = _mm256_shuffle_epi8(state1, A3);
	state2 = _mm256_xor_si256(state2, k_m);

	state1 = _mm256_xor_si256(state1, state2);

	/*
	The difference with uBlock round function
	*/
	state3 = _mm256_permute2x128_si256(state1, state1, 1);//Left and right change
	state1 = _mm256_shuffle_epi8(state3, PL);//Byte permutation within 128-bit

	state2 = _mm256_shuffle_epi8(state2, PR);//Byte permutation within 128-bit







	//这里的round其实代表的是step

	for (i = 1; i < round; i++)
	{
		//The first round
		tK1 = KK0;
		tK2 = KK1;
		__m256i RoundConst = _mm256_setr_epi8(rc[i - 1][0], rc[i - 1][1], rc[i - 1][2], rc[i - 1][3], rc[i - 1][4], rc[i - 1][5], rc[i - 1][6], rc[i - 1][7], rc[i - 1][8], rc[i - 1][9], rc[i - 1][10], rc[i - 1][11], rc[i - 1][12], rc[i - 1][13], rc[i - 1][14], rc[i - 1][15], rc[i - 1][16], rc[i - 1][17], rc[i - 1][18], rc[i - 1][19], rc[i - 1][20], rc[i - 1][21], rc[i - 1][22], rc[i - 1][23], rc[i - 1][24], rc[i - 1][25], rc[i - 1][26], rc[i - 1][27], rc[i - 1][28], rc[i - 1][29], rc[i - 1][30], rc[i - 1][31]);
		//_mm256_setr_epi32(RC[i - 1][0], RC[i - 1][1], RC[i - 1][2], RC[i - 1][3], RC[i - 1][4], RC[i - 1][5], RC[i - 1][6], RC[i - 1][7]);








		//@@@@@@Bug is in here@@@@@@@
		KK0 = _mm256_xor_si256(KK2, RoundConst);
		KK0 = _mm256_shuffle_epi8(KK0, F);//the output after function P2
		//K0 = _mm256_shuffle_epi8(K2, F);//the output after function P2
		tG = _mm256_permutevar8x32_epi32(KK5, G_Word);
		KK1 = _mm256_shuffle_epi8(tG, G_Nibble);//the output after function G
		//得到K0，K1=（0x0a,0x0a,...）后生成KK0,KK1=(0xaa,0xaa,...)
		//K0_L4 = _mm256_slli_epi16(K0, 4);
		//KK0 = _mm256_xor_si256(K0, K0_L4);//KK0=(0xaa,0xaa,...)

		//K1_L4 = _mm256_slli_epi16(K1, 4);
		//KK1 = _mm256_xor_si256(K1, K1_L4);//KK1=(0xaa,0xaa,...)







		state1 = _mm256_xor_si256(state1, KK0);
		state2 = _mm256_xor_si256(state2, KK1);
		state1_L4 = _mm256_and_si256(state1, con_L4);
		state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);
		state1_R4 = _mm256_and_si256(state1, con);
		state1_L4 = _mm256_shuffle_epi8(S_L4, state1_L4_R4);
		state1_R4 = _mm256_shuffle_epi8(S, state1_R4);
		state1 = _mm256_xor_si256(state1_L4, state1_R4);

		state2_L4 = _mm256_and_si256(state2, con_L4);
		state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);
		state2_R4 = _mm256_and_si256(state2, con);
		state2_L4 = _mm256_shuffle_epi8(S_L4, state2_L4_R4);
		state2_R4 = _mm256_shuffle_epi8(S, state2_R4);
		state2 = _mm256_xor_si256(state2_L4, state2_R4);

		//state2 = _mm256_shuffle_epi8(S, state2);

		state2 = _mm256_xor_si256(state2, state1);

		k_m = _mm256_shuffle_epi8(state2, A1);
		state1 = _mm256_xor_si256(state1, k_m);

		k_m = _mm256_shuffle_epi8(state1, A2);
		state2 = _mm256_xor_si256(state2, k_m);

		k_m = _mm256_shuffle_epi8(state2, A2);
		state1 = _mm256_xor_si256(state1, k_m);

		k_m = _mm256_shuffle_epi8(state1, A3);
		state2 = _mm256_xor_si256(state2, k_m);

		state1 = _mm256_xor_si256(state1, state2);

		/*
		The difference with uBlock round function
		*/
		state3 = _mm256_permute2x128_si256(state1, state1, 1);//Left and right change
		state1 = _mm256_shuffle_epi8(state3, PL);//Byte permutation within 128-bit

		state2 = _mm256_shuffle_epi8(state2, PR);//Byte permutation within 128-bit







		//The second round
		KK2 = _mm256_shuffle_epi8(tK2, F);//the output after function P2
		tK2 = KK3;
		tG = _mm256_permutevar8x32_epi32(tK1, G_Word);
		KK3 = _mm256_shuffle_epi8(tG, G_Nibble);//the output after function G
		//得到K0，K1=（0x0a,0x0a,...）后生成KK0,KK1=(0xaa,0xaa,...)
		//K2_L4 = _mm256_slli_epi16(K2, 4);
		//KK2 = _mm256_xor_si256(K2, K2_L4);//KK0=(0xaa,0xaa,...)

		//K3_L4 = _mm256_slli_epi16(K3, 4);
		//KK3 = _mm256_xor_si256(K3, K3_L4);//KK1=(0xaa,0xaa,...)




		state1 = _mm256_xor_si256(state1, KK2);
		state2 = _mm256_xor_si256(state2, KK3);
		state1_L4 = _mm256_and_si256(state1, con_L4);
		state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);
		state1_R4 = _mm256_and_si256(state1, con);
		state1_L4 = _mm256_shuffle_epi8(S_L4, state1_L4_R4);
		state1_R4 = _mm256_shuffle_epi8(S, state1_R4);
		state1 = _mm256_xor_si256(state1_L4, state1_R4);

		state2_L4 = _mm256_and_si256(state2, con_L4);
		state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);
		state2_R4 = _mm256_and_si256(state2, con);
		state2_L4 = _mm256_shuffle_epi8(S_L4, state2_L4_R4);
		state2_R4 = _mm256_shuffle_epi8(S, state2_R4);
		state2 = _mm256_xor_si256(state2_L4, state2_R4);

		//state2 = _mm256_shuffle_epi8(S, state2);

		state2 = _mm256_xor_si256(state2, state1);

		k_m = _mm256_shuffle_epi8(state2, A1);
		state1 = _mm256_xor_si256(state1, k_m);

		k_m = _mm256_shuffle_epi8(state1, A2);
		state2 = _mm256_xor_si256(state2, k_m);

		k_m = _mm256_shuffle_epi8(state2, A2);
		state1 = _mm256_xor_si256(state1, k_m);

		k_m = _mm256_shuffle_epi8(state1, A3);
		state2 = _mm256_xor_si256(state2, k_m);

		state1 = _mm256_xor_si256(state1, state2);

		/*
		The difference with uBlock round function
		*/
		state3 = _mm256_permute2x128_si256(state1, state1, 1);//Left and right change
		state1 = _mm256_shuffle_epi8(state3, PL);//Byte permutation within 128-bit

		state2 = _mm256_shuffle_epi8(state2, PR);//Byte permutation within 128-bit





		//The third round
		tK1 = KK4;
		KK4 = _mm256_shuffle_epi8(tK2, F);//the output after function P2

		tG = _mm256_permutevar8x32_epi32(KK6, G_Word);
		KK5 = _mm256_shuffle_epi8(tG, G_Nibble);//the output after function G
		//得到K0，K1=（0x0a,0x0a,...）后生成KK0,KK1=(0xaa,0xaa,...)
		//K4_L4 = _mm256_slli_epi16(K4, 4);
		//KK4 = _mm256_xor_si256(K4, K4_L4);//KK0=(0xaa,0xaa,...)

		//K5_L4 = _mm256_slli_epi16(K5, 4);
		//KK5 = _mm256_xor_si256(K5, K5_L4);//KK1=(0xaa,0xaa,...)




		state1 = _mm256_xor_si256(state1, KK4);
		state2 = _mm256_xor_si256(state2, KK5);
		state1_L4 = _mm256_and_si256(state1, con_L4);
		state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);
		state1_R4 = _mm256_and_si256(state1, con);
		state1_L4 = _mm256_shuffle_epi8(S_L4, state1_L4_R4);
		state1_R4 = _mm256_shuffle_epi8(S, state1_R4);
		state1 = _mm256_xor_si256(state1_L4, state1_R4);

		state2_L4 = _mm256_and_si256(state2, con_L4);
		state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);
		state2_R4 = _mm256_and_si256(state2, con);
		state2_L4 = _mm256_shuffle_epi8(S_L4, state2_L4_R4);
		state2_R4 = _mm256_shuffle_epi8(S, state2_R4);
		state2 = _mm256_xor_si256(state2_L4, state2_R4);

		//state2 = _mm256_shuffle_epi8(S, state2);

		state2 = _mm256_xor_si256(state2, state1);

		k_m = _mm256_shuffle_epi8(state2, A1);
		state1 = _mm256_xor_si256(state1, k_m);

		k_m = _mm256_shuffle_epi8(state1, A2);
		state2 = _mm256_xor_si256(state2, k_m);

		k_m = _mm256_shuffle_epi8(state2, A2);
		state1 = _mm256_xor_si256(state1, k_m);

		k_m = _mm256_shuffle_epi8(state1, A3);
		state2 = _mm256_xor_si256(state2, k_m);

		state1 = _mm256_xor_si256(state1, state2);

		/*
		The difference with uBlock round function
		*/
		state3 = _mm256_permute2x128_si256(state1, state1, 1);//Left and right change
		state1 = _mm256_shuffle_epi8(state3, PL);//Byte permutation within 128-bit

		state2 = _mm256_shuffle_epi8(state2, PR);//Byte permutation within 128-bit





		//The fourth round

		KK6 = _mm256_shuffle_epi8(KK7, F);//the output after function P2

		tG = _mm256_permutevar8x32_epi32(tK1, G_Word);
		KK7 = _mm256_shuffle_epi8(tG, G_Nibble);//the output after function G
		//得到K0，K1=（0x0a,0x0a,...）后生成KK0,KK1=(0xaa,0xaa,...)
		//K6_L4 = _mm256_slli_epi16(K6, 4);
		//KK6 = _mm256_xor_si256(K6, K6_L4);//KK0=(0xaa,0xaa,...)

		//K7_L4 = _mm256_slli_epi16(K7, 4);
		//KK7 = _mm256_xor_si256(K7, K7_L4);//KK1=(0xaa,0xaa,...)



		state1 = _mm256_xor_si256(state1, KK6);
		state2 = _mm256_xor_si256(state2, KK7);
		state1_L4 = _mm256_and_si256(state1, con_L4);
		state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);
		state1_R4 = _mm256_and_si256(state1, con);
		state1_L4 = _mm256_shuffle_epi8(S_L4, state1_L4_R4);
		state1_R4 = _mm256_shuffle_epi8(S, state1_R4);
		state1 = _mm256_xor_si256(state1_L4, state1_R4);

		state2_L4 = _mm256_and_si256(state2, con_L4);
		state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);
		state2_R4 = _mm256_and_si256(state2, con);
		state2_L4 = _mm256_shuffle_epi8(S_L4, state2_L4_R4);
		state2_R4 = _mm256_shuffle_epi8(S, state2_R4);
		state2 = _mm256_xor_si256(state2_L4, state2_R4);

		//state2 = _mm256_shuffle_epi8(S, state2);

		state2 = _mm256_xor_si256(state2, state1);

		k_m = _mm256_shuffle_epi8(state2, A1);
		state1 = _mm256_xor_si256(state1, k_m);

		k_m = _mm256_shuffle_epi8(state1, A2);
		state2 = _mm256_xor_si256(state2, k_m);

		k_m = _mm256_shuffle_epi8(state2, A2);
		state1 = _mm256_xor_si256(state1, k_m);

		k_m = _mm256_shuffle_epi8(state1, A3);
		state2 = _mm256_xor_si256(state2, k_m);

		state1 = _mm256_xor_si256(state1, state2);

		/*
		The difference with uBlock round function
		*/
		state3 = _mm256_permute2x128_si256(state1, state1, 1);//Left and right change
		state1 = _mm256_shuffle_epi8(state3, PL);//Byte permutation within 128-bit

		state2 = _mm256_shuffle_epi8(state2, PR);//Byte permutation within 128-bit






	}

	//*** Modify***
	//Update in V20, removing the white key XOR in the end of the round function
	//state1 = _mm256_xor_si256(state1, KK0);
	//state2 = _mm256_xor_si256(state2, KK1);





	/*
	* ==========================================================================
	* 反馈操作，完成后输出u1=（0xaa,...）,u2,u3=(0x0a,....)
	* ==========================================================================
	*/
	//添加反馈操作，放到前面减少指令
	state1 = _mm256_xor_si256(state1, *u1_x);
	state2 = _mm256_xor_si256(state2, *u1_y);





	state1_L4 = _mm256_and_si256(state1, con_L4);
	state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);

	state1_R4 = _mm256_and_si256(state1, con);
	state1_R4_L4 = _mm256_slli_epi16(state1_R4, 4);

	state2_L4 = _mm256_and_si256(state2, con_L4);
	state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);

	state2_R4 = _mm256_and_si256(state2, con);
	state2_R4_L4 = _mm256_slli_epi16(state2_R4, 4);




	*u1_x = _mm256_xor_si256(state1_R4, state1_R4_L4);//u1_x,u1_y=(0xaa,0xaa,...)
	*u1_y = _mm256_xor_si256(state2_R4, state2_R4_L4);

	*u2_x = state1_L4_R4;//u2_x,u2_y=(0x0a,0x0a,...)
	*u2_y = state2_L4_R4;



	return;
}




//m is the m1,m2, u1=u1_x||u1_y (the 256-bit plaintext ), uu1=uu1_x||uu1z-y,u2=u2_x||u2_y, u3=u3_x||u3_y,  u2||u3||m2||m3 (the 1024-bit key)
//u1是并行，uu1,u2，u3，m1，m2都是单行，及u1=(0xaa,0xaa,...),uu1,u2,u3,m1,m2=(0x0a,0x0a,...)
//


int CF_768(const unsigned char* m, __m256i* u1_x, __m256i* u1_y, __m256i* u2_x, __m256i* u2_y, __m256i* u3_x, __m256i* u3_y, int round)
{
	//首先，将m1，m2,m3进行分割
	__m256i S_L4 = _mm256_setr_epi8(0x30, 0x00, 0x60, 0x20, 0x50, 0x40, 0xf0, 0xe0, 0xa0, 0x80, 0x70, 0x90, 0x10, 0xc0, 0xd0, 0xb0, 0x30, 0x00, 0x60, 0x20, 0x50, 0x40, 0xf0, 0xe0, 0xa0, 0x80, 0x70, 0x90, 0x10, 0xc0, 0xd0, 0xb0);
	__m256i S = _mm256_setr_epi8(0x3, 0x0, 0x6, 0x2, 0x5, 0x4, 0xf, 0xe, 0xa, 0x8, 0x7, 0x9, 0x1, 0xc, 0xd, 0xb, 0x3, 0x0, 0x6, 0x2, 0x5, 0x4, 0xf, 0xe, 0xa, 0x8, 0x7, 0x9, 0x1, 0xc, 0xd, 0xb);
	__m256i S_Inv = _mm256_setr_epi8(0xc, 0xa, 0xe, 0xd, 0x1, 0xf, 0xb, 0x0, 0x7, 0x2, 0x5, 0x4, 0x3, 0x6, 0x9, 0x8, 0xc, 0xa, 0xe, 0xd, 0x1, 0xf, 0xb, 0x0, 0x7, 0x2, 0x5, 0x4, 0x3, 0x6, 0x9, 0x8);

	__m256i con = _mm256_set1_epi8(0x0f);//取右4-bit
	__m256i con_L4 = _mm256_set1_epi8(0xf0);//取左4-bit
	__m256i A1 = _mm256_setr_epi8(1, 2, 3, 4, 5, 6, 7, 0, 9, 10, 11, 12, 13, 14, 15, 8, 1, 2, 3, 4, 5, 6, 7, 0, 9, 10, 11, 12, 13, 14, 15, 8);
	__m256i A2 = _mm256_setr_epi8(2, 3, 4, 5, 6, 7, 0, 1, 10, 11, 12, 13, 14, 15, 8, 9, 2, 3, 4, 5, 6, 7, 0, 1, 10, 11, 12, 13, 14, 15, 8, 9);
	__m256i A3 = _mm256_setr_epi8(5, 6, 7, 0, 1, 2, 3, 4, 13, 14, 15, 8, 9, 10, 11, 12, 5, 6, 7, 0, 1, 2, 3, 4, 13, 14, 15, 8, 9, 10, 11, 12);
	__m256i PL = _mm256_setr_epi8(12, 2, 0, 10, 14, 6, 13, 7, 11, 1, 8, 15, 9, 3, 5, 4, 3, 10, 4, 15, 14, 5, 7, 13, 12, 0, 11, 1, 9, 2, 6, 8);
	__m256i PR = _mm256_setr_epi8(0, 2, 1, 12, 11, 8, 13, 7, 10, 4, 6, 5, 3, 14, 9, 15, 6, 2, 11, 7, 8, 15, 4, 12, 13, 1, 0, 3, 9, 14, 10, 5);
	__m256i A4 = _mm256_setr_epi8(3, 4, 5, 6, 7, 0, 1, 2, 11, 12, 13, 14, 15, 8, 9, 10, 3, 4, 5, 6, 7, 0, 1, 2, 11, 12, 13, 14, 15, 8, 9, 10);

	__m256i c1 = _mm256_setr_epi8(0, 0x80, 1, 0x80, 2, 0x80, 3, 0x80, 4, 0x80, 5, 0x80, 6, 0x80, 7, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
	__m256i c2 = _mm256_setr_epi8(0x80, 0, 0x80, 1, 0x80, 2, 0x80, 3, 0x80, 4, 0x80, 5, 0x80, 6, 0x80, 7, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
	__m256i c3 = _mm256_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 8, 0x80, 9, 0x80, 10, 0x80, 11, 0x80, 12, 0x80, 13, 0x80, 14, 0x80, 15, 0x80);
	__m256i c4 = _mm256_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 8, 0x80, 9, 0x80, 10, 0x80, 11, 0x80, 12, 0x80, 13, 0x80, 14, 0x80, 15);


	__m256i c5 = _mm256_setr_epi8(0, 2, 4, 6, 8, 10, 12, 14, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
	__m256i c6 = _mm256_setr_epi8(1, 3, 5, 7, 9, 11, 13, 15, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
	__m256i c7 = _mm256_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0, 2, 4, 6, 8, 10, 12, 14, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
	__m256i c8 = _mm256_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 1, 3, 5, 7, 9, 11, 13, 15, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
	__m256i c55 = _mm256_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0, 2, 4, 6, 8, 10, 12, 14, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
	__m256i c66 = _mm256_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 1, 3, 5, 7, 9, 11, 13, 15, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
	__m256i c77 = _mm256_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0, 2, 4, 6, 8, 10, 12, 14);
	__m256i c88 = _mm256_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 1, 3, 5, 7, 9, 11, 13, 15);

	//G=[23, 3, 31, 2, 0, 20, 21, 15, 13, 29, 14, 30, 12, 28, 22, 1, 17, 24, 8, 5, 27, 16, 6, 11, 25, 18, 19, 10, 7, 9, 26, 4] (3cycle 8x32+ 1cycle two 16x4)
	//F = [11, 3, 14, 2, 9, 10, 12, 0, 4, 13, 7, 8, 15, 6, 5, 1, 16, 19, 27, 22, 31, 26, 20, 30, 23, 29, 25, 18, 24, 21, 28, 17](1 cycle two 16x4)

	__m256i F = _mm256_setr_epi8(11, 3, 14, 2, 9, 10, 12, 0, 4, 13, 7, 8, 15, 6, 5, 1, 0, 3, 11, 6, 15, 10, 4, 14, 7, 13, 9, 2, 8, 5, 12, 1);
	//__m256i P2 = _mm256_setr_epi8(8, 0, 6, 5, 3, 11, 7, 2, 14, 1, 15, 4, 10, 12, 13, 9, 13, 10, 1, 3, 12, 9, 14, 7, 6, 2, 15, 11, 8, 5, 4, 0);
	//G=G_Nibble + [2,4,5,6,0,1,3,7]
	__m256i G_Nibble = _mm256_setr_epi8(11, 3, 15, 2, 0, 8, 9, 7, 5, 13, 6, 14, 4, 12, 10, 1, 9, 12, 4, 1, 15, 8, 2, 7, 13, 10, 11, 6, 3, 5, 14, 0);
	__m256i G_Word = _mm256_setr_epi32(0, 3, 5, 7, 1, 2, 4, 6);


	int i;

	__m256i k, t1, t2, t3, t4, t5, t6, t7, t8;
	__m256i K0, K1, K0_L4, K1_L4, K2, K3, K2_L4, K3_L4, state_1_3, state_1_4, k_1;
	__m256i K4, K5, K4_L4, K5_L4, state_2_3, state_2_4, k_2;
	__m256i K6, K7, K6_L4, K7_L4, state_3_3, state_3_4, k_3;
	__m256i tG, k1, k3, tK1, tK2, tK3;
	__m256i KK0, KK1, KK2, KK3, KK4, KK5, KK6, KK7;

	__m256i state1, state2, state3, k_m;
	__m256i state1_1, state2_1, state3_1, k_m_1;
	__m256i state1_L4, state2_L4, state1_L4_R4, state2_L4_R4, state1_R4, state2_R4, k_L4, k_R4, k_L4_R4;
	__m256i Value1, state1_R4_L4, state2_R4_L4;
	__m256i Value2;
	__m256i u1_x_pre, u1_y_pre, u2_x_pre, u2_y_pre, u1_x_R4, u1_y_R4;

	__m256i SSstate1, SSstate2, SSstate3;


	//unsigned char value1[32] = { 0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x10 };

	Value1 = _mm256_setr_epi8(0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x10);


	Value2 = _mm256_setr_epi8(0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x02);


	//_mm256_loadu_si256((__m256i*)value1);
	//unsigned char value2[32] = { 0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x2 };
	//Value2 = _mm256_loadu_si256((__m256i*)value2);
	//u2_x(K0),u2_y(K1)

	K0 = *u2_x;//K0=u2_x=(0x0a,0x0a,...)
	K1 = *u2_y;//K1=u2_y=(0x0a,0x0a,...)




	K0_L4 = _mm256_slli_epi16(K0, 4);
	KK0 = _mm256_xor_si256(K0, K0_L4);//KK0=(0xaa,0xaa,...)

	K1_L4 = _mm256_slli_epi16(K1, 4);
	KK1 = _mm256_xor_si256(K1, K1_L4);//KK1=(0xaa,0xaa,...)


	K2 = *u3_x;//K2=u3_x=(0x0a,0x0a,...)
	K3 = *u3_y;//K3=u3_y=(0x0a,0x0a,...)




	K2_L4 = _mm256_slli_epi16(K2, 4);
	KK2 = _mm256_xor_si256(K2, K2_L4);//KK0=(0xaa,0xaa,...)

	K3_L4 = _mm256_slli_epi16(K3, 4);
	KK3 = _mm256_xor_si256(K3, K3_L4);//KK1=(0xaa,0xaa,...)












	//m1_x(K4)/m1_y(K5)
	//m1_x=(0x0a,0x0a,...)
	//m1_y=(0x0a,0x0a,...)
	K5 = _mm256_loadu_si256((__m256i*)(m));
	K4 = _mm256_srli_epi16(K5, 4);
	K4 = _mm256_and_si256(K4, con);
	K5 = _mm256_and_si256(K5, con);

	state_2_3 = _mm256_permute2x128_si256(K4, K4, 1);
	state_2_4 = _mm256_permute2x128_si256(K5, K5, 1);
	t1 = _mm256_shuffle_epi8(K4, c1);
	t2 = _mm256_shuffle_epi8(state_2_3, c3);
	t3 = _mm256_shuffle_epi8(K5, c2);
	t4 = _mm256_shuffle_epi8(state_2_4, c4);
	t5 = _mm256_shuffle_epi8(state_2_3, c1);
	t6 = _mm256_shuffle_epi8(K4, c3);
	t7 = _mm256_shuffle_epi8(state_2_4, c2);
	t8 = _mm256_shuffle_epi8(K5, c4);

	K4 = _mm256_xor_si256(t1, t2);
	state_2_3 = _mm256_xor_si256(t3, t4);
	K4 = _mm256_xor_si256(K4, state_2_3);

	K5 = _mm256_xor_si256(t5, t6);
	state_2_4 = _mm256_xor_si256(t7, t8);
	K5 = _mm256_xor_si256(K5, state_2_4);


	K4_L4 = _mm256_slli_epi16(K4, 4);
	KK4 = _mm256_xor_si256(K4, K4_L4);//KK4=(0xaa,0xaa,...)

	K5_L4 = _mm256_slli_epi16(K5, 4);
	KK5 = _mm256_xor_si256(K5, K5_L4);//KK5=(0xaa,0xaa,...)






	//m2_x(K6)/m2_y(K7)
	//m2_x=(0x0a,0x0a,...)
	//m2_y=(0x0a,0x0a,...)
	K7 = _mm256_loadu_si256((__m256i*)(m + 32));
	K6 = _mm256_srli_epi16(K7, 4);
	K6 = _mm256_and_si256(K6, con);
	K7 = _mm256_and_si256(K7, con);

	state_3_3 = _mm256_permute2x128_si256(K6, K6, 1);
	state_3_4 = _mm256_permute2x128_si256(K7, K7, 1);
	t1 = _mm256_shuffle_epi8(K6, c1);
	t2 = _mm256_shuffle_epi8(state_3_3, c3);
	t3 = _mm256_shuffle_epi8(K7, c2);
	t4 = _mm256_shuffle_epi8(state_3_4, c4);
	t5 = _mm256_shuffle_epi8(state_3_3, c1);
	t6 = _mm256_shuffle_epi8(K6, c3);
	t7 = _mm256_shuffle_epi8(state_3_4, c2);
	t8 = _mm256_shuffle_epi8(K7, c4);

	K6 = _mm256_xor_si256(t1, t2);
	state_3_3 = _mm256_xor_si256(t3, t4);
	K6 = _mm256_xor_si256(K6, state_3_3);

	K7 = _mm256_xor_si256(t5, t6);
	state_3_4 = _mm256_xor_si256(t7, t8);
	K7 = _mm256_xor_si256(K7, state_3_4);

	K6_L4 = _mm256_slli_epi16(K6, 4);
	KK6 = _mm256_xor_si256(K6, K6_L4);//KK6=(0xaa,0xaa,...)

	K7_L4 = _mm256_slli_epi16(K7, 4);
	KK7 = _mm256_xor_si256(K7, K7_L4);//KK7=(0xaa,0xaa,...)






	//取u1_x和u1_y的R4位
	u1_x_R4 = _mm256_and_si256(*u1_x, con);//即uu1_x,uu1_y
	u1_y_R4 = _mm256_and_si256(*u1_y, con);
	//明文部分的赋值

	//第一个大块和第二个大块的加密输入,第1个分支在右4位，第2个分支左4位
	state1 = *u1_x;//u1_x=(0xaa,0xaa,....)
	state2 = *u1_y;//u1_y=(0xaa,0xaa,....)

	//第一个大块和第二个大块的加密输入,第1个分支在右4位，第2个分支左4位
	SSstate1 = u1_x_R4;//u1_x_R4=(0x0a,0x0a,....)
	SSstate2 = u1_y_R4;//u1_y_R4=(0x0a,0x0a,....)

	//u1和u2的值还得再处理一下
	//state1,state2=0x(aa,0xaa,...)
	//u1和u2是按照(0xaa,0xaa,0xaa)来存储的，不是按照(0x0a,0x0a,0x0a,)来存储的
	state2 = _mm256_xor_si256(state2, Value1);//区分两个块不同的明文输入

	SSstate2 = _mm256_xor_si256(SSstate2, Value2);//区分两个块不同的明文输入









	//第三个块加密输入
	//state1_1 = u1_x_R4;//u1_x=(0xaa,0xaa,....)
	//state2_1 = u1_y_R4;//u1_y=(0xaa,0xaa,....)

	//state1_1 = _mm256_and_si256(u1_x, con);//state1_1=0x(0a,0x0a,...)
	//state2_1 = _mm256_and_si256(u1_y, con);//state2_1=0x(0a,0x0a,...)

	//u1和u2是按照(0xaa,0xaa,0xaa)来存储的，不是按照(0x0a,0x0a,0x0a,)来存储的
	//state2_1 = _mm256_xor_si256(state2_1, Value2);//区分两个块不同的明文输入


	//第1个step不需要更新轮密钥
	//The first round
	state1 = _mm256_xor_si256(state1, KK0);
	state2 = _mm256_xor_si256(state2, KK1);
	state1_L4 = _mm256_and_si256(state1, con_L4);
	state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);
	state1_R4 = _mm256_and_si256(state1, con);
	state1_L4 = _mm256_shuffle_epi8(S_L4, state1_L4_R4);
	state1_R4 = _mm256_shuffle_epi8(S, state1_R4);
	state1 = _mm256_xor_si256(state1_L4, state1_R4);

	state2_L4 = _mm256_and_si256(state2, con_L4);
	state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);
	state2_R4 = _mm256_and_si256(state2, con);
	state2_L4 = _mm256_shuffle_epi8(S_L4, state2_L4_R4);
	state2_R4 = _mm256_shuffle_epi8(S, state2_R4);
	state2 = _mm256_xor_si256(state2_L4, state2_R4);

	//state2 = _mm256_shuffle_epi8(S, state2);

	state2 = _mm256_xor_si256(state2, state1);

	k_m = _mm256_shuffle_epi8(state2, A1);
	state1 = _mm256_xor_si256(state1, k_m);

	k_m = _mm256_shuffle_epi8(state1, A2);
	state2 = _mm256_xor_si256(state2, k_m);

	k_m = _mm256_shuffle_epi8(state2, A2);
	state1 = _mm256_xor_si256(state1, k_m);

	k_m = _mm256_shuffle_epi8(state1, A3);
	state2 = _mm256_xor_si256(state2, k_m);

	state1 = _mm256_xor_si256(state1, state2);

	/*
	The difference with uBlock round function
	*/
	state3 = _mm256_permute2x128_si256(state1, state1, 1);//Left and right change
	state1 = _mm256_shuffle_epi8(state3, PL);//Byte permutation within 128-bit

	state2 = _mm256_shuffle_epi8(state2, PR);//Byte permutation within 128-bit




	//The third Brunch 
	//The first round
	SSstate1 = _mm256_xor_si256(SSstate1, K0);
	SSstate2 = _mm256_xor_si256(SSstate2, K1);
	//SSstate1_L4 = _mm256_and_si256(state1, con_L4);
	//state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);
	//state1_R4 = _mm256_and_si256(state1, con);
	//state1_L4 = _mm256_shuffle_epi8(S_L4, state1_L4_R4);
	SSstate1 = _mm256_shuffle_epi8(S, SSstate1);
	//state1 = _mm256_xor_si256(state1_L4, state1_R4);

	//state2_L4 = _mm256_and_si256(state2, con_L4);
	//state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);
	//state2_R4 = _mm256_and_si256(state2, con);
	//state2_L4 = _mm256_shuffle_epi8(S_L4, state2_L4_R4);
	SSstate2 = _mm256_shuffle_epi8(S, SSstate2);
	//state2 = _mm256_xor_si256(state2_L4, state2_R4);

	//state2 = _mm256_shuffle_epi8(S, state2);

	SSstate2 = _mm256_xor_si256(SSstate2, SSstate1);

	k_m = _mm256_shuffle_epi8(SSstate2, A1);
	SSstate1 = _mm256_xor_si256(SSstate1, k_m);

	k_m = _mm256_shuffle_epi8(SSstate1, A2);
	SSstate2 = _mm256_xor_si256(SSstate2, k_m);

	k_m = _mm256_shuffle_epi8(SSstate2, A2);
	SSstate1 = _mm256_xor_si256(SSstate1, k_m);

	k_m = _mm256_shuffle_epi8(SSstate1, A3);
	SSstate2 = _mm256_xor_si256(SSstate2, k_m);

	SSstate1 = _mm256_xor_si256(SSstate1, SSstate2);

	/*
	The difference with uBlock round function
	*/
	SSstate3 = _mm256_permute2x128_si256(SSstate1, SSstate1, 1);//Left and right change
	SSstate1 = _mm256_shuffle_epi8(SSstate3, PL);//Byte permutation within 128-bit

	SSstate2 = _mm256_shuffle_epi8(SSstate2, PR);//Byte permutation within 128-bit







	//The second round
	state1 = _mm256_xor_si256(state1, KK2);
	state2 = _mm256_xor_si256(state2, KK3);
	state1_L4 = _mm256_and_si256(state1, con_L4);
	state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);
	state1_R4 = _mm256_and_si256(state1, con);
	state1_L4 = _mm256_shuffle_epi8(S_L4, state1_L4_R4);
	state1_R4 = _mm256_shuffle_epi8(S, state1_R4);
	state1 = _mm256_xor_si256(state1_L4, state1_R4);

	state2_L4 = _mm256_and_si256(state2, con_L4);
	state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);
	state2_R4 = _mm256_and_si256(state2, con);
	state2_L4 = _mm256_shuffle_epi8(S_L4, state2_L4_R4);
	state2_R4 = _mm256_shuffle_epi8(S, state2_R4);
	state2 = _mm256_xor_si256(state2_L4, state2_R4);

	//state2 = _mm256_shuffle_epi8(S, state2);

	state2 = _mm256_xor_si256(state2, state1);

	k_m = _mm256_shuffle_epi8(state2, A1);
	state1 = _mm256_xor_si256(state1, k_m);

	k_m = _mm256_shuffle_epi8(state1, A2);
	state2 = _mm256_xor_si256(state2, k_m);

	k_m = _mm256_shuffle_epi8(state2, A2);
	state1 = _mm256_xor_si256(state1, k_m);

	k_m = _mm256_shuffle_epi8(state1, A3);
	state2 = _mm256_xor_si256(state2, k_m);

	state1 = _mm256_xor_si256(state1, state2);

	/*
	The difference with uBlock round function
	*/
	state3 = _mm256_permute2x128_si256(state1, state1, 1);//Left and right change
	state1 = _mm256_shuffle_epi8(state3, PL);//Byte permutation within 128-bit

	state2 = _mm256_shuffle_epi8(state2, PR);//Byte permutation within 128-bit


	//The third Brunch 
	//The second round
	SSstate1 = _mm256_xor_si256(SSstate1, K2);
	SSstate2 = _mm256_xor_si256(SSstate2, K3);
	//SSstate1_L4 = _mm256_and_si256(state1, con_L4);
	//state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);
	//state1_R4 = _mm256_and_si256(state1, con);
	//state1_L4 = _mm256_shuffle_epi8(S_L4, state1_L4_R4);
	SSstate1 = _mm256_shuffle_epi8(S, SSstate1);
	//state1 = _mm256_xor_si256(state1_L4, state1_R4);

	//state2_L4 = _mm256_and_si256(state2, con_L4);
	//state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);
	//state2_R4 = _mm256_and_si256(state2, con);
	//state2_L4 = _mm256_shuffle_epi8(S_L4, state2_L4_R4);
	SSstate2 = _mm256_shuffle_epi8(S, SSstate2);
	//state2 = _mm256_xor_si256(state2_L4, state2_R4);

	//state2 = _mm256_shuffle_epi8(S, state2);

	SSstate2 = _mm256_xor_si256(SSstate2, SSstate1);

	k_m = _mm256_shuffle_epi8(SSstate2, A1);
	SSstate1 = _mm256_xor_si256(SSstate1, k_m);

	k_m = _mm256_shuffle_epi8(SSstate1, A2);
	SSstate2 = _mm256_xor_si256(SSstate2, k_m);

	k_m = _mm256_shuffle_epi8(SSstate2, A2);
	SSstate1 = _mm256_xor_si256(SSstate1, k_m);

	k_m = _mm256_shuffle_epi8(SSstate1, A3);
	SSstate2 = _mm256_xor_si256(SSstate2, k_m);

	SSstate1 = _mm256_xor_si256(SSstate1, SSstate2);


	/*
The difference with uBlock round function
*/
	SSstate3 = _mm256_permute2x128_si256(SSstate1, SSstate1, 1);//Left and right change
	SSstate1 = _mm256_shuffle_epi8(SSstate3, PL);//Byte permutation within 128-bit

	SSstate2 = _mm256_shuffle_epi8(SSstate2, PR);//Byte permutation within 128-bit





	//The third round
	state1 = _mm256_xor_si256(state1, KK4);
	state2 = _mm256_xor_si256(state2, KK5);
	state1_L4 = _mm256_and_si256(state1, con_L4);
	state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);
	state1_R4 = _mm256_and_si256(state1, con);
	state1_L4 = _mm256_shuffle_epi8(S_L4, state1_L4_R4);
	state1_R4 = _mm256_shuffle_epi8(S, state1_R4);
	state1 = _mm256_xor_si256(state1_L4, state1_R4);

	state2_L4 = _mm256_and_si256(state2, con_L4);
	state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);
	state2_R4 = _mm256_and_si256(state2, con);
	state2_L4 = _mm256_shuffle_epi8(S_L4, state2_L4_R4);
	state2_R4 = _mm256_shuffle_epi8(S, state2_R4);
	state2 = _mm256_xor_si256(state2_L4, state2_R4);

	//state2 = _mm256_shuffle_epi8(S, state2);

	state2 = _mm256_xor_si256(state2, state1);

	k_m = _mm256_shuffle_epi8(state2, A1);
	state1 = _mm256_xor_si256(state1, k_m);

	k_m = _mm256_shuffle_epi8(state1, A2);
	state2 = _mm256_xor_si256(state2, k_m);

	k_m = _mm256_shuffle_epi8(state2, A2);
	state1 = _mm256_xor_si256(state1, k_m);

	k_m = _mm256_shuffle_epi8(state1, A3);
	state2 = _mm256_xor_si256(state2, k_m);

	state1 = _mm256_xor_si256(state1, state2);

	/*
	The difference with uBlock round function
	*/
	state3 = _mm256_permute2x128_si256(state1, state1, 1);//Left and right change
	state1 = _mm256_shuffle_epi8(state3, PL);//Byte permutation within 128-bit

	state2 = _mm256_shuffle_epi8(state2, PR);//Byte permutation within 128-bit


	//The third Brunch 
	 //The third round
	SSstate1 = _mm256_xor_si256(SSstate1, K4);
	SSstate2 = _mm256_xor_si256(SSstate2, K5);
	//SSstate1_L4 = _mm256_and_si256(state1, con_L4);
	//state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);
	//state1_R4 = _mm256_and_si256(state1, con);
	//state1_L4 = _mm256_shuffle_epi8(S_L4, state1_L4_R4);
	SSstate1 = _mm256_shuffle_epi8(S, SSstate1);
	//state1 = _mm256_xor_si256(state1_L4, state1_R4);

	//state2_L4 = _mm256_and_si256(state2, con_L4);
	//state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);
	//state2_R4 = _mm256_and_si256(state2, con);
	//state2_L4 = _mm256_shuffle_epi8(S_L4, state2_L4_R4);
	SSstate2 = _mm256_shuffle_epi8(S, SSstate2);
	//state2 = _mm256_xor_si256(state2_L4, state2_R4);

	//state2 = _mm256_shuffle_epi8(S, state2);

	SSstate2 = _mm256_xor_si256(SSstate2, SSstate1);

	k_m = _mm256_shuffle_epi8(SSstate2, A1);
	SSstate1 = _mm256_xor_si256(SSstate1, k_m);

	k_m = _mm256_shuffle_epi8(SSstate1, A2);
	SSstate2 = _mm256_xor_si256(SSstate2, k_m);

	k_m = _mm256_shuffle_epi8(SSstate2, A2);
	SSstate1 = _mm256_xor_si256(SSstate1, k_m);

	k_m = _mm256_shuffle_epi8(SSstate1, A3);
	SSstate2 = _mm256_xor_si256(SSstate2, k_m);

	SSstate1 = _mm256_xor_si256(SSstate1, SSstate2);


	/*
The difference with uBlock round function
*/
	SSstate3 = _mm256_permute2x128_si256(SSstate1, SSstate1, 1);//Left and right change
	SSstate1 = _mm256_shuffle_epi8(SSstate3, PL);//Byte permutation within 128-bit

	SSstate2 = _mm256_shuffle_epi8(SSstate2, PR);//Byte permutation within 128-bit






	//The fourth round
	state1 = _mm256_xor_si256(state1, KK6);
	state2 = _mm256_xor_si256(state2, KK7);
	state1_L4 = _mm256_and_si256(state1, con_L4);
	state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);
	state1_R4 = _mm256_and_si256(state1, con);
	state1_L4 = _mm256_shuffle_epi8(S_L4, state1_L4_R4);
	state1_R4 = _mm256_shuffle_epi8(S, state1_R4);
	state1 = _mm256_xor_si256(state1_L4, state1_R4);

	state2_L4 = _mm256_and_si256(state2, con_L4);
	state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);
	state2_R4 = _mm256_and_si256(state2, con);
	state2_L4 = _mm256_shuffle_epi8(S_L4, state2_L4_R4);
	state2_R4 = _mm256_shuffle_epi8(S, state2_R4);
	state2 = _mm256_xor_si256(state2_L4, state2_R4);

	//state2 = _mm256_shuffle_epi8(S, state2);

	state2 = _mm256_xor_si256(state2, state1);

	k_m = _mm256_shuffle_epi8(state2, A1);
	state1 = _mm256_xor_si256(state1, k_m);

	k_m = _mm256_shuffle_epi8(state1, A2);
	state2 = _mm256_xor_si256(state2, k_m);

	k_m = _mm256_shuffle_epi8(state2, A2);
	state1 = _mm256_xor_si256(state1, k_m);

	k_m = _mm256_shuffle_epi8(state1, A3);
	state2 = _mm256_xor_si256(state2, k_m);

	state1 = _mm256_xor_si256(state1, state2);

	/*
	The difference with uBlock round function
	*/
	state3 = _mm256_permute2x128_si256(state1, state1, 1);//Left and right change
	state1 = _mm256_shuffle_epi8(state3, PL);//Byte permutation within 128-bit

	state2 = _mm256_shuffle_epi8(state2, PR);//Byte permutation within 128-bit


	//The third Brunch 
	//The fourth round
	SSstate1 = _mm256_xor_si256(SSstate1, K6);
	SSstate2 = _mm256_xor_si256(SSstate2, K7);
	//SSstate1_L4 = _mm256_and_si256(state1, con_L4);
	//state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);
	//state1_R4 = _mm256_and_si256(state1, con);
	//state1_L4 = _mm256_shuffle_epi8(S_L4, state1_L4_R4);
	SSstate1 = _mm256_shuffle_epi8(S, SSstate1);
	//state1 = _mm256_xor_si256(state1_L4, state1_R4);

	//state2_L4 = _mm256_and_si256(state2, con_L4);
	//state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);
	//state2_R4 = _mm256_and_si256(state2, con);
	//state2_L4 = _mm256_shuffle_epi8(S_L4, state2_L4_R4);
	SSstate2 = _mm256_shuffle_epi8(S, SSstate2);
	//state2 = _mm256_xor_si256(state2_L4, state2_R4);

	//state2 = _mm256_shuffle_epi8(S, state2);

	SSstate2 = _mm256_xor_si256(SSstate2, SSstate1);

	k_m = _mm256_shuffle_epi8(SSstate2, A1);
	SSstate1 = _mm256_xor_si256(SSstate1, k_m);

	k_m = _mm256_shuffle_epi8(SSstate1, A2);
	SSstate2 = _mm256_xor_si256(SSstate2, k_m);

	k_m = _mm256_shuffle_epi8(SSstate2, A2);
	SSstate1 = _mm256_xor_si256(SSstate1, k_m);

	k_m = _mm256_shuffle_epi8(SSstate1, A3);
	SSstate2 = _mm256_xor_si256(SSstate2, k_m);

	SSstate1 = _mm256_xor_si256(SSstate1, SSstate2);


	/*
The difference with uBlock round function
*/
	SSstate3 = _mm256_permute2x128_si256(SSstate1, SSstate1, 1);//Left and right change
	SSstate1 = _mm256_shuffle_epi8(SSstate3, PL);//Byte permutation within 128-bit

	SSstate2 = _mm256_shuffle_epi8(SSstate2, PR);//Byte permutation within 128-bit






	//这里的round其实代表的是step

	for (i = 1; i < round; i++)
	{
		//The first round
		tK1 = K0;
		tK2 = K1;
		__m256i RoundConst = _mm256_setr_epi8(rrc[i - 1][0], rrc[i - 1][1], rrc[i - 1][2], rrc[i - 1][3], rrc[i - 1][4], rrc[i - 1][5], rrc[i - 1][6], rrc[i - 1][7], rrc[i - 1][8], rrc[i - 1][9], rrc[i - 1][10], rrc[i - 1][11], rrc[i - 1][12], rrc[i - 1][13], rrc[i - 1][14], rrc[i - 1][15], rrc[i - 1][16], rrc[i - 1][17], rrc[i - 1][18], rrc[i - 1][19], rrc[i - 1][20], rrc[i - 1][21], rrc[i - 1][22], rrc[i - 1][23], rrc[i - 1][24], rrc[i - 1][25], rrc[i - 1][26], rrc[i - 1][27], rrc[i - 1][28], rrc[i - 1][29], rrc[i - 1][30], rrc[i - 1][31]);
		//__m256i RoundConst = _mm256_setr_epi8(rc[i - 1][0], rc[i - 1][1], rc[i - 1][2], rc[i - 1][3], rc[i - 1][4], rc[i - 1][5], rc[i - 1][6], rc[i - 1][7], rc[i - 1][8], rc[i - 1][9], rc[i - 1][10], rc[i - 1][11], rc[i - 1][12], rc[i - 1][13], rc[i - 1][14], rc[i - 1][15], rc[i - 1][16], rc[i - 1][17], rc[i - 1][18], rc[i - 1][19], rc[i - 1][20], rc[i - 1][21], rc[i - 1][22], rc[i - 1][23], rc[i - 1][24], rc[i - 1][25], rc[i - 1][26], rc[i - 1][27], rc[i - 1][28], rc[i - 1][29], rc[i - 1][30], rc[i - 1][31]);
		//_mm256_setr_epi32(RC[i - 1][0], RC[i - 1][1], RC[i - 1][2], RC[i - 1][3], RC[i - 1][4], RC[i - 1][5], RC[i - 1][6], RC[i - 1][7]);










		//@@@@@@Bug is in here@@@@@@@
		K0 = _mm256_xor_si256(K2, RoundConst);
		K0 = _mm256_shuffle_epi8(K0, F);//the output after function P2
		//K0 = _mm256_shuffle_epi8(K2, F);//the output after function P2
		tG = _mm256_permutevar8x32_epi32(K5, G_Word);
		K1 = _mm256_shuffle_epi8(tG, G_Nibble);//the output after function G
		//得到K0，K1=（0x0a,0x0a,...）后生成KK0,KK1=(0xaa,0xaa,...)
		K0_L4 = _mm256_slli_epi16(K0, 4);
		KK0 = _mm256_xor_si256(K0, K0_L4);//KK0=(0xaa,0xaa,...)

		K1_L4 = _mm256_slli_epi16(K1, 4);
		KK1 = _mm256_xor_si256(K1, K1_L4);//KK1=(0xaa,0xaa,...)








		state1 = _mm256_xor_si256(state1, KK0);
		state2 = _mm256_xor_si256(state2, KK1);
		state1_L4 = _mm256_and_si256(state1, con_L4);
		state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);
		state1_R4 = _mm256_and_si256(state1, con);
		state1_L4 = _mm256_shuffle_epi8(S_L4, state1_L4_R4);
		state1_R4 = _mm256_shuffle_epi8(S, state1_R4);
		state1 = _mm256_xor_si256(state1_L4, state1_R4);

		state2_L4 = _mm256_and_si256(state2, con_L4);
		state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);
		state2_R4 = _mm256_and_si256(state2, con);
		state2_L4 = _mm256_shuffle_epi8(S_L4, state2_L4_R4);
		state2_R4 = _mm256_shuffle_epi8(S, state2_R4);
		state2 = _mm256_xor_si256(state2_L4, state2_R4);

		//state2 = _mm256_shuffle_epi8(S, state2);

		state2 = _mm256_xor_si256(state2, state1);

		k_m = _mm256_shuffle_epi8(state2, A1);
		state1 = _mm256_xor_si256(state1, k_m);

		k_m = _mm256_shuffle_epi8(state1, A2);
		state2 = _mm256_xor_si256(state2, k_m);

		k_m = _mm256_shuffle_epi8(state2, A2);
		state1 = _mm256_xor_si256(state1, k_m);

		k_m = _mm256_shuffle_epi8(state1, A3);
		state2 = _mm256_xor_si256(state2, k_m);

		state1 = _mm256_xor_si256(state1, state2);

		/*
		The difference with uBlock round function
		*/
		state3 = _mm256_permute2x128_si256(state1, state1, 1);//Left and right change
		state1 = _mm256_shuffle_epi8(state3, PL);//Byte permutation within 128-bit

		state2 = _mm256_shuffle_epi8(state2, PR);//Byte permutation within 128-bit


		//The third Brunch 
		//The first round
		SSstate1 = _mm256_xor_si256(SSstate1, K0);
		SSstate2 = _mm256_xor_si256(SSstate2, K1);
		//SSstate1_L4 = _mm256_and_si256(state1, con_L4);
		//state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);
		//state1_R4 = _mm256_and_si256(state1, con);
		//state1_L4 = _mm256_shuffle_epi8(S_L4, state1_L4_R4);
		SSstate1 = _mm256_shuffle_epi8(S, SSstate1);
		//state1 = _mm256_xor_si256(state1_L4, state1_R4);

		//state2_L4 = _mm256_and_si256(state2, con_L4);
		//state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);
		//state2_R4 = _mm256_and_si256(state2, con);
		//state2_L4 = _mm256_shuffle_epi8(S_L4, state2_L4_R4);
		SSstate2 = _mm256_shuffle_epi8(S, SSstate2);
		//state2 = _mm256_xor_si256(state2_L4, state2_R4);

		//state2 = _mm256_shuffle_epi8(S, state2);

		SSstate2 = _mm256_xor_si256(SSstate2, SSstate1);

		k_m = _mm256_shuffle_epi8(SSstate2, A1);
		SSstate1 = _mm256_xor_si256(SSstate1, k_m);

		k_m = _mm256_shuffle_epi8(SSstate1, A2);
		SSstate2 = _mm256_xor_si256(SSstate2, k_m);

		k_m = _mm256_shuffle_epi8(SSstate2, A2);
		SSstate1 = _mm256_xor_si256(SSstate1, k_m);

		k_m = _mm256_shuffle_epi8(SSstate1, A3);
		SSstate2 = _mm256_xor_si256(SSstate2, k_m);

		SSstate1 = _mm256_xor_si256(SSstate1, SSstate2);


		/*
The difference with uBlock round function
*/
		SSstate3 = _mm256_permute2x128_si256(SSstate1, SSstate1, 1);//Left and right change
		SSstate1 = _mm256_shuffle_epi8(SSstate3, PL);//Byte permutation within 128-bit

		SSstate2 = _mm256_shuffle_epi8(SSstate2, PR);//Byte permutation within 128-bit







		//The second round
		K2 = _mm256_shuffle_epi8(tK2, F);//the output after function P2
		tK2 = K3;
		tG = _mm256_permutevar8x32_epi32(tK1, G_Word);
		K3 = _mm256_shuffle_epi8(tG, G_Nibble);//the output after function G
		//得到K0，K1=（0x0a,0x0a,...）后生成KK0,KK1=(0xaa,0xaa,...)
		K2_L4 = _mm256_slli_epi16(K2, 4);
		KK2 = _mm256_xor_si256(K2, K2_L4);//KK0=(0xaa,0xaa,...)

		K3_L4 = _mm256_slli_epi16(K3, 4);
		KK3 = _mm256_xor_si256(K3, K3_L4);//KK1=(0xaa,0xaa,...)



		state1 = _mm256_xor_si256(state1, KK2);
		state2 = _mm256_xor_si256(state2, KK3);
		state1_L4 = _mm256_and_si256(state1, con_L4);
		state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);
		state1_R4 = _mm256_and_si256(state1, con);
		state1_L4 = _mm256_shuffle_epi8(S_L4, state1_L4_R4);
		state1_R4 = _mm256_shuffle_epi8(S, state1_R4);
		state1 = _mm256_xor_si256(state1_L4, state1_R4);

		state2_L4 = _mm256_and_si256(state2, con_L4);
		state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);
		state2_R4 = _mm256_and_si256(state2, con);
		state2_L4 = _mm256_shuffle_epi8(S_L4, state2_L4_R4);
		state2_R4 = _mm256_shuffle_epi8(S, state2_R4);
		state2 = _mm256_xor_si256(state2_L4, state2_R4);

		//state2 = _mm256_shuffle_epi8(S, state2);

		state2 = _mm256_xor_si256(state2, state1);

		k_m = _mm256_shuffle_epi8(state2, A1);
		state1 = _mm256_xor_si256(state1, k_m);

		k_m = _mm256_shuffle_epi8(state1, A2);
		state2 = _mm256_xor_si256(state2, k_m);

		k_m = _mm256_shuffle_epi8(state2, A2);
		state1 = _mm256_xor_si256(state1, k_m);

		k_m = _mm256_shuffle_epi8(state1, A3);
		state2 = _mm256_xor_si256(state2, k_m);

		state1 = _mm256_xor_si256(state1, state2);

		/*
		The difference with uBlock round function
		*/
		state3 = _mm256_permute2x128_si256(state1, state1, 1);//Left and right change
		state1 = _mm256_shuffle_epi8(state3, PL);//Byte permutation within 128-bit

		state2 = _mm256_shuffle_epi8(state2, PR);//Byte permutation within 128-bit


		//The third Brunch 
		//The second round
		SSstate1 = _mm256_xor_si256(SSstate1, K2);
		SSstate2 = _mm256_xor_si256(SSstate2, K3);
		//SSstate1_L4 = _mm256_and_si256(state1, con_L4);
		//state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);
		//state1_R4 = _mm256_and_si256(state1, con);
		//state1_L4 = _mm256_shuffle_epi8(S_L4, state1_L4_R4);
		SSstate1 = _mm256_shuffle_epi8(S, SSstate1);
		//state1 = _mm256_xor_si256(state1_L4, state1_R4);

		//state2_L4 = _mm256_and_si256(state2, con_L4);
		//state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);
		//state2_R4 = _mm256_and_si256(state2, con);
		//state2_L4 = _mm256_shuffle_epi8(S_L4, state2_L4_R4);
		SSstate2 = _mm256_shuffle_epi8(S, SSstate2);
		//state2 = _mm256_xor_si256(state2_L4, state2_R4);

		//state2 = _mm256_shuffle_epi8(S, state2);

		SSstate2 = _mm256_xor_si256(SSstate2, SSstate1);

		k_m = _mm256_shuffle_epi8(SSstate2, A1);
		SSstate1 = _mm256_xor_si256(SSstate1, k_m);

		k_m = _mm256_shuffle_epi8(SSstate1, A2);
		SSstate2 = _mm256_xor_si256(SSstate2, k_m);

		k_m = _mm256_shuffle_epi8(SSstate2, A2);
		SSstate1 = _mm256_xor_si256(SSstate1, k_m);

		k_m = _mm256_shuffle_epi8(SSstate1, A3);
		SSstate2 = _mm256_xor_si256(SSstate2, k_m);

		SSstate1 = _mm256_xor_si256(SSstate1, SSstate2);

		/*
The difference with uBlock round function
*/
		SSstate3 = _mm256_permute2x128_si256(SSstate1, SSstate1, 1);//Left and right change
		SSstate1 = _mm256_shuffle_epi8(SSstate3, PL);//Byte permutation within 128-bit

		SSstate2 = _mm256_shuffle_epi8(SSstate2, PR);//Byte permutation within 128-bit





		//The third round
		tK1 = K4;
		K4 = _mm256_shuffle_epi8(tK2, F);//the output after function P2

		tG = _mm256_permutevar8x32_epi32(K6, G_Word);
		K5 = _mm256_shuffle_epi8(tG, G_Nibble);//the output after function G
		//得到K0，K1=（0x0a,0x0a,...）后生成KK0,KK1=(0xaa,0xaa,...)
		K4_L4 = _mm256_slli_epi16(K4, 4);
		KK4 = _mm256_xor_si256(K4, K4_L4);//KK0=(0xaa,0xaa,...)

		K5_L4 = _mm256_slli_epi16(K5, 4);
		KK5 = _mm256_xor_si256(K5, K5_L4);//KK1=(0xaa,0xaa,...)




		state1 = _mm256_xor_si256(state1, KK4);
		state2 = _mm256_xor_si256(state2, KK5);
		state1_L4 = _mm256_and_si256(state1, con_L4);
		state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);
		state1_R4 = _mm256_and_si256(state1, con);
		state1_L4 = _mm256_shuffle_epi8(S_L4, state1_L4_R4);
		state1_R4 = _mm256_shuffle_epi8(S, state1_R4);
		state1 = _mm256_xor_si256(state1_L4, state1_R4);

		state2_L4 = _mm256_and_si256(state2, con_L4);
		state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);
		state2_R4 = _mm256_and_si256(state2, con);
		state2_L4 = _mm256_shuffle_epi8(S_L4, state2_L4_R4);
		state2_R4 = _mm256_shuffle_epi8(S, state2_R4);
		state2 = _mm256_xor_si256(state2_L4, state2_R4);

		//state2 = _mm256_shuffle_epi8(S, state2);

		state2 = _mm256_xor_si256(state2, state1);

		k_m = _mm256_shuffle_epi8(state2, A1);
		state1 = _mm256_xor_si256(state1, k_m);

		k_m = _mm256_shuffle_epi8(state1, A2);
		state2 = _mm256_xor_si256(state2, k_m);

		k_m = _mm256_shuffle_epi8(state2, A2);
		state1 = _mm256_xor_si256(state1, k_m);

		k_m = _mm256_shuffle_epi8(state1, A3);
		state2 = _mm256_xor_si256(state2, k_m);

		state1 = _mm256_xor_si256(state1, state2);

		/*
		The difference with uBlock round function
		*/
		state3 = _mm256_permute2x128_si256(state1, state1, 1);//Left and right change
		state1 = _mm256_shuffle_epi8(state3, PL);//Byte permutation within 128-bit

		state2 = _mm256_shuffle_epi8(state2, PR);//Byte permutation within 128-bit


		//The third Brunch 
		//The third round
		SSstate1 = _mm256_xor_si256(SSstate1, K4);
		SSstate2 = _mm256_xor_si256(SSstate2, K5);
		//SSstate1_L4 = _mm256_and_si256(state1, con_L4);
		//state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);
		//state1_R4 = _mm256_and_si256(state1, con);
		//state1_L4 = _mm256_shuffle_epi8(S_L4, state1_L4_R4);
		SSstate1 = _mm256_shuffle_epi8(S, SSstate1);
		//state1 = _mm256_xor_si256(state1_L4, state1_R4);

		//state2_L4 = _mm256_and_si256(state2, con_L4);
		//state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);
		//state2_R4 = _mm256_and_si256(state2, con);
		//state2_L4 = _mm256_shuffle_epi8(S_L4, state2_L4_R4);
		SSstate2 = _mm256_shuffle_epi8(S, SSstate2);
		//state2 = _mm256_xor_si256(state2_L4, state2_R4);

		//state2 = _mm256_shuffle_epi8(S, state2);

		SSstate2 = _mm256_xor_si256(SSstate2, SSstate1);

		k_m = _mm256_shuffle_epi8(SSstate2, A1);
		SSstate1 = _mm256_xor_si256(SSstate1, k_m);

		k_m = _mm256_shuffle_epi8(SSstate1, A2);
		SSstate2 = _mm256_xor_si256(SSstate2, k_m);

		k_m = _mm256_shuffle_epi8(SSstate2, A2);
		SSstate1 = _mm256_xor_si256(SSstate1, k_m);

		k_m = _mm256_shuffle_epi8(SSstate1, A3);
		SSstate2 = _mm256_xor_si256(SSstate2, k_m);

		SSstate1 = _mm256_xor_si256(SSstate1, SSstate2);


		/*
The difference with uBlock round function
*/
		SSstate3 = _mm256_permute2x128_si256(SSstate1, SSstate1, 1);//Left and right change
		SSstate1 = _mm256_shuffle_epi8(SSstate3, PL);//Byte permutation within 128-bit

		SSstate2 = _mm256_shuffle_epi8(SSstate2, PR);//Byte permutation within 128-bit





		//The fourth round

		K6 = _mm256_shuffle_epi8(K7, F);//the output after function P2

		tG = _mm256_permutevar8x32_epi32(tK1, G_Word);
		K7 = _mm256_shuffle_epi8(tG, G_Nibble);//the output after function G
		//得到K0，K1=（0x0a,0x0a,...）后生成KK0,KK1=(0xaa,0xaa,...)
		K6_L4 = _mm256_slli_epi16(K6, 4);
		KK6 = _mm256_xor_si256(K6, K6_L4);//KK0=(0xaa,0xaa,...)

		K7_L4 = _mm256_slli_epi16(K7, 4);
		KK7 = _mm256_xor_si256(K7, K7_L4);//KK1=(0xaa,0xaa,...)




		state1 = _mm256_xor_si256(state1, KK6);
		state2 = _mm256_xor_si256(state2, KK7);
		state1_L4 = _mm256_and_si256(state1, con_L4);
		state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);
		state1_R4 = _mm256_and_si256(state1, con);
		state1_L4 = _mm256_shuffle_epi8(S_L4, state1_L4_R4);
		state1_R4 = _mm256_shuffle_epi8(S, state1_R4);
		state1 = _mm256_xor_si256(state1_L4, state1_R4);

		state2_L4 = _mm256_and_si256(state2, con_L4);
		state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);
		state2_R4 = _mm256_and_si256(state2, con);
		state2_L4 = _mm256_shuffle_epi8(S_L4, state2_L4_R4);
		state2_R4 = _mm256_shuffle_epi8(S, state2_R4);
		state2 = _mm256_xor_si256(state2_L4, state2_R4);

		//state2 = _mm256_shuffle_epi8(S, state2);

		state2 = _mm256_xor_si256(state2, state1);

		k_m = _mm256_shuffle_epi8(state2, A1);
		state1 = _mm256_xor_si256(state1, k_m);

		k_m = _mm256_shuffle_epi8(state1, A2);
		state2 = _mm256_xor_si256(state2, k_m);

		k_m = _mm256_shuffle_epi8(state2, A2);
		state1 = _mm256_xor_si256(state1, k_m);

		k_m = _mm256_shuffle_epi8(state1, A3);
		state2 = _mm256_xor_si256(state2, k_m);

		state1 = _mm256_xor_si256(state1, state2);

		/*
		The difference with uBlock round function
		*/
		state3 = _mm256_permute2x128_si256(state1, state1, 1);//Left and right change
		state1 = _mm256_shuffle_epi8(state3, PL);//Byte permutation within 128-bit

		state2 = _mm256_shuffle_epi8(state2, PR);//Byte permutation within 128-bit

		//The third Brunch 
		 //The fourth round
		SSstate1 = _mm256_xor_si256(SSstate1, K6);
		SSstate2 = _mm256_xor_si256(SSstate2, K7);
		//SSstate1_L4 = _mm256_and_si256(state1, con_L4);
		//state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);
		//state1_R4 = _mm256_and_si256(state1, con);
		//state1_L4 = _mm256_shuffle_epi8(S_L4, state1_L4_R4);
		SSstate1 = _mm256_shuffle_epi8(S, SSstate1);
		//state1 = _mm256_xor_si256(state1_L4, state1_R4);

		//state2_L4 = _mm256_and_si256(state2, con_L4);
		//state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);
		//state2_R4 = _mm256_and_si256(state2, con);
		//state2_L4 = _mm256_shuffle_epi8(S_L4, state2_L4_R4);
		SSstate2 = _mm256_shuffle_epi8(S, SSstate2);
		//state2 = _mm256_xor_si256(state2_L4, state2_R4);

		//state2 = _mm256_shuffle_epi8(S, state2);

		SSstate2 = _mm256_xor_si256(SSstate2, SSstate1);

		k_m = _mm256_shuffle_epi8(SSstate2, A1);
		SSstate1 = _mm256_xor_si256(SSstate1, k_m);

		k_m = _mm256_shuffle_epi8(SSstate1, A2);
		SSstate2 = _mm256_xor_si256(SSstate2, k_m);

		k_m = _mm256_shuffle_epi8(SSstate2, A2);
		SSstate1 = _mm256_xor_si256(SSstate1, k_m);

		k_m = _mm256_shuffle_epi8(SSstate1, A3);
		SSstate2 = _mm256_xor_si256(SSstate2, k_m);

		SSstate1 = _mm256_xor_si256(SSstate1, SSstate2);


		/*
The difference with uBlock round function
*/
		SSstate3 = _mm256_permute2x128_si256(SSstate1, SSstate1, 1);//Left and right change
		SSstate1 = _mm256_shuffle_epi8(SSstate3, PL);//Byte permutation within 128-bit

		SSstate2 = _mm256_shuffle_epi8(SSstate2, PR);//Byte permutation within 128-bit





	}

	//*** Modify 1***
	// Removing the white key xor in the end of the round function
	//state1 = _mm256_xor_si256(state1, KK0);
	//state2 = _mm256_xor_si256(state2, KK1);

	//SSstate1 = _mm256_xor_si256(SSstate1, K0);
	//SSstate2 = _mm256_xor_si256(SSstate2, K1);




	/*
	* ==========================================================================
	* 反馈操作，完成后输出u1=（0xaa,...）,u2,u3=(0x0a,....)
	* ==========================================================================
	*/
	//添加反馈操作，放到前面减少指令
	state1 = _mm256_xor_si256(state1, *u1_x);
	state2 = _mm256_xor_si256(state2, *u1_y);

	SSstate1 = _mm256_xor_si256(SSstate1, u1_x_R4);
	SSstate2 = _mm256_xor_si256(SSstate2, u1_y_R4);





	state1_L4 = _mm256_and_si256(state1, con_L4);
	state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);

	state1_R4 = _mm256_and_si256(state1, con);
	state1_R4_L4 = _mm256_slli_epi16(state1_R4, 4);

	state2_L4 = _mm256_and_si256(state2, con_L4);
	state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);

	state2_R4 = _mm256_and_si256(state2, con);
	state2_R4_L4 = _mm256_slli_epi16(state2_R4, 4);




	*u1_x = _mm256_xor_si256(state1_R4, state1_R4_L4);//u1_x,u1_y=(0xaa,0xaa,...)
	*u1_y = _mm256_xor_si256(state2_R4, state2_R4_L4);

	*u2_x = state1_L4_R4;//u2_x,u2_y=(0x0a,0x0a,...)
	*u2_y = state2_L4_R4;

	*u3_x = SSstate1;
	*u3_y = SSstate2;





	return 0;
}



void CF_768_Final(unsigned char* m, __m256i* u1_x, __m256i* u1_y, __m256i* u2_x, __m256i* u2_y, __m256i* u3_x, __m256i* u3_y, int round)
{
	//首先，将m1，m2,m3进行分割
	__m256i S_L4 = _mm256_setr_epi8(0x30, 0x00, 0x60, 0x20, 0x50, 0x40, 0xf0, 0xe0, 0xa0, 0x80, 0x70, 0x90, 0x10, 0xc0, 0xd0, 0xb0, 0x30, 0x00, 0x60, 0x20, 0x50, 0x40, 0xf0, 0xe0, 0xa0, 0x80, 0x70, 0x90, 0x10, 0xc0, 0xd0, 0xb0);
	__m256i S = _mm256_setr_epi8(0x3, 0x0, 0x6, 0x2, 0x5, 0x4, 0xf, 0xe, 0xa, 0x8, 0x7, 0x9, 0x1, 0xc, 0xd, 0xb, 0x3, 0x0, 0x6, 0x2, 0x5, 0x4, 0xf, 0xe, 0xa, 0x8, 0x7, 0x9, 0x1, 0xc, 0xd, 0xb);
	__m256i S_Inv = _mm256_setr_epi8(0xc, 0xa, 0xe, 0xd, 0x1, 0xf, 0xb, 0x0, 0x7, 0x2, 0x5, 0x4, 0x3, 0x6, 0x9, 0x8, 0xc, 0xa, 0xe, 0xd, 0x1, 0xf, 0xb, 0x0, 0x7, 0x2, 0x5, 0x4, 0x3, 0x6, 0x9, 0x8);

	__m256i con = _mm256_set1_epi8(0x0f);//取右4-bit
	__m256i con_L4 = _mm256_set1_epi8(0xf0);//取左4-bit
	__m256i A1 = _mm256_setr_epi8(1, 2, 3, 4, 5, 6, 7, 0, 9, 10, 11, 12, 13, 14, 15, 8, 1, 2, 3, 4, 5, 6, 7, 0, 9, 10, 11, 12, 13, 14, 15, 8);
	__m256i A2 = _mm256_setr_epi8(2, 3, 4, 5, 6, 7, 0, 1, 10, 11, 12, 13, 14, 15, 8, 9, 2, 3, 4, 5, 6, 7, 0, 1, 10, 11, 12, 13, 14, 15, 8, 9);
	__m256i A3 = _mm256_setr_epi8(5, 6, 7, 0, 1, 2, 3, 4, 13, 14, 15, 8, 9, 10, 11, 12, 5, 6, 7, 0, 1, 2, 3, 4, 13, 14, 15, 8, 9, 10, 11, 12);
	__m256i PL = _mm256_setr_epi8(12, 2, 0, 10, 14, 6, 13, 7, 11, 1, 8, 15, 9, 3, 5, 4, 3, 10, 4, 15, 14, 5, 7, 13, 12, 0, 11, 1, 9, 2, 6, 8);
	__m256i PR = _mm256_setr_epi8(0, 2, 1, 12, 11, 8, 13, 7, 10, 4, 6, 5, 3, 14, 9, 15, 6, 2, 11, 7, 8, 15, 4, 12, 13, 1, 0, 3, 9, 14, 10, 5);
	__m256i A4 = _mm256_setr_epi8(3, 4, 5, 6, 7, 0, 1, 2, 11, 12, 13, 14, 15, 8, 9, 10, 3, 4, 5, 6, 7, 0, 1, 2, 11, 12, 13, 14, 15, 8, 9, 10);

	__m256i c1 = _mm256_setr_epi8(0, 0x80, 1, 0x80, 2, 0x80, 3, 0x80, 4, 0x80, 5, 0x80, 6, 0x80, 7, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
	__m256i c2 = _mm256_setr_epi8(0x80, 0, 0x80, 1, 0x80, 2, 0x80, 3, 0x80, 4, 0x80, 5, 0x80, 6, 0x80, 7, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
	__m256i c3 = _mm256_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 8, 0x80, 9, 0x80, 10, 0x80, 11, 0x80, 12, 0x80, 13, 0x80, 14, 0x80, 15, 0x80);
	__m256i c4 = _mm256_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 8, 0x80, 9, 0x80, 10, 0x80, 11, 0x80, 12, 0x80, 13, 0x80, 14, 0x80, 15);


	__m256i c5 = _mm256_setr_epi8(0, 2, 4, 6, 8, 10, 12, 14, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
	__m256i c6 = _mm256_setr_epi8(1, 3, 5, 7, 9, 11, 13, 15, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
	__m256i c7 = _mm256_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0, 2, 4, 6, 8, 10, 12, 14, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
	__m256i c8 = _mm256_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 1, 3, 5, 7, 9, 11, 13, 15, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
	__m256i c55 = _mm256_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0, 2, 4, 6, 8, 10, 12, 14, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
	__m256i c66 = _mm256_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 1, 3, 5, 7, 9, 11, 13, 15, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
	__m256i c77 = _mm256_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0, 2, 4, 6, 8, 10, 12, 14);
	__m256i c88 = _mm256_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 1, 3, 5, 7, 9, 11, 13, 15);

	//G=[23, 3, 31, 2, 0, 20, 21, 15, 13, 29, 14, 30, 12, 28, 22, 1, 17, 24, 8, 5, 27, 16, 6, 11, 25, 18, 19, 10, 7, 9, 26, 4] (3cycle 8x32+ 1cycle two 16x4)
	//F = [11, 3, 14, 2, 9, 10, 12, 0, 4, 13, 7, 8, 15, 6, 5, 1, 16, 19, 27, 22, 31, 26, 20, 30, 23, 29, 25, 18, 24, 21, 28, 17](1 cycle two 16x4)

	__m256i F = _mm256_setr_epi8(11, 3, 14, 2, 9, 10, 12, 0, 4, 13, 7, 8, 15, 6, 5, 1, 0, 3, 11, 6, 15, 10, 4, 14, 7, 13, 9, 2, 8, 5, 12, 1);
	//__m256i P2 = _mm256_setr_epi8(8, 0, 6, 5, 3, 11, 7, 2, 14, 1, 15, 4, 10, 12, 13, 9, 13, 10, 1, 3, 12, 9, 14, 7, 6, 2, 15, 11, 8, 5, 4, 0);
	//G=G_Nibble + [2,4,5,6,0,1,3,7]
	__m256i G_Nibble = _mm256_setr_epi8(11, 3, 15, 2, 0, 8, 9, 7, 5, 13, 6, 14, 4, 12, 10, 1, 9, 12, 4, 1, 15, 8, 2, 7, 13, 10, 11, 6, 3, 5, 14, 0);
	__m256i G_Word = _mm256_setr_epi32(0, 3, 5, 7, 1, 2, 4, 6);


	int i;

	__m256i k, t1, t2, t3, t4, t5, t6, t7, t8;
	__m256i K0, K1, K0_L4, K1_L4, K2, K3, K2_L4, K3_L4, state_1_3, state_1_4, k_1;
	__m256i K4, K5, K4_L4, K5_L4, state_2_3, state_2_4, k_2;
	__m256i K6, K7, K6_L4, K7_L4, state_3_3, state_3_4, k_3;
	__m256i tG, k1, k3, tK1, tK2, tK3, tGG, kk1, kk3, tKK1, tKK2, tKK3;
	__m256i KK0, KK1, KK2, KK3, KK4, KK5, KK6, KK7;

	__m256i state1, state2, state3, k_m;
	__m256i state1_1, state2_1, state3_1, k_m_1;
	__m256i state1_L4, state2_L4, state1_L4_R4, state2_L4_R4, state1_R4, state2_R4, k_L4, k_R4, k_L4_R4;
	__m256i Value1, state1_R4_L4, state2_R4_L4;
	__m256i Value2, ValueK1, ValueK2;
	__m256i u1_x_pre, u1_y_pre, u2_x_pre, u2_y_pre, u1_x_R4, u1_y_R4;

	__m256i SSstate1, SSstate2, SSstate3;


	unsigned char value1[32] = { 0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x10 };

	//*** Modify 2 ***
	//The counter in the plaintext are same as the first l-1 block
	Value1 = _mm256_setr_epi8(0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x10);


	Value2 = _mm256_setr_epi8(0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x02);


	//*** Modify 3 ***
	//The difference in the last block, i,e. the l-th block is in the XOR value in the master key
	ValueK1 = _mm256_setr_epi8(0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x10, 0x0f);


	ValueK2 = _mm256_setr_epi8(0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x01, 0x01);


	//_mm256_loadu_si256((__m256i*)value1);
	//unsigned char value2[32] = { 0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x2 };
	//Value2 = _mm256_loadu_si256((__m256i*)value2);
	//u2_x(K0),u2_y(K1)

	K0 = *u2_x;//K0=u2_x=(0x0a,0x0a,...)
	K1 = *u2_y;//K1=u2_y=(0x0a,0x0a,...)




	K0_L4 = _mm256_slli_epi16(K0, 4);
	KK0 = _mm256_xor_si256(K0, K0_L4);//KK0=(0xaa,0xaa,...)

	K1_L4 = _mm256_slli_epi16(K1, 4);
	KK1 = _mm256_xor_si256(K1, K1_L4);//KK1=(0xaa,0xaa,...)


	K2 = *u3_x;//K2=u3_x=(0x0a,0x0a,...)
	K3 = *u3_y;//K3=u3_y=(0x0a,0x0a,...)




	K2_L4 = _mm256_slli_epi16(K2, 4);
	KK2 = _mm256_xor_si256(K2, K2_L4);//KK0=(0xaa,0xaa,...)

	K3_L4 = _mm256_slli_epi16(K3, 4);
	KK3 = _mm256_xor_si256(K3, K3_L4);//KK1=(0xaa,0xaa,...)





	//m1_x(K4)/m1_y(K5)
	//m1_x=(0x0a,0x0a,...)
	//m1_y=(0x0a,0x0a,...)
	K5 = _mm256_loadu_si256((__m256i*)(m));
	K4 = _mm256_srli_epi16(K5, 4);
	K4 = _mm256_and_si256(K4, con);
	K5 = _mm256_and_si256(K5, con);

	state_2_3 = _mm256_permute2x128_si256(K4, K4, 1);
	state_2_4 = _mm256_permute2x128_si256(K5, K5, 1);
	t1 = _mm256_shuffle_epi8(K4, c1);
	t2 = _mm256_shuffle_epi8(state_2_3, c3);
	t3 = _mm256_shuffle_epi8(K5, c2);
	t4 = _mm256_shuffle_epi8(state_2_4, c4);
	t5 = _mm256_shuffle_epi8(state_2_3, c1);
	t6 = _mm256_shuffle_epi8(K4, c3);
	t7 = _mm256_shuffle_epi8(state_2_4, c2);
	t8 = _mm256_shuffle_epi8(K5, c4);

	K4 = _mm256_xor_si256(t1, t2);
	state_2_3 = _mm256_xor_si256(t3, t4);
	K4 = _mm256_xor_si256(K4, state_2_3);

	K5 = _mm256_xor_si256(t5, t6);
	state_2_4 = _mm256_xor_si256(t7, t8);
	K5 = _mm256_xor_si256(K5, state_2_4);


	K4_L4 = _mm256_slli_epi16(K4, 4);
	KK4 = _mm256_xor_si256(K4, K4_L4);//KK4=(0xaa,0xaa,...)

	K5_L4 = _mm256_slli_epi16(K5, 4);
	KK5 = _mm256_xor_si256(K5, K5_L4);//KK5=(0xaa,0xaa,...)






	//m2_x(K6)/m2_y(K7)
	//m2_x=(0x0a,0x0a,...)
	//m2_y=(0x0a,0x0a,...)
	K7 = _mm256_loadu_si256((__m256i*)(m + 32));
	K6 = _mm256_srli_epi16(K7, 4);
	K6 = _mm256_and_si256(K6, con);
	K7 = _mm256_and_si256(K7, con);

	state_3_3 = _mm256_permute2x128_si256(K6, K6, 1);
	state_3_4 = _mm256_permute2x128_si256(K7, K7, 1);
	t1 = _mm256_shuffle_epi8(K6, c1);
	t2 = _mm256_shuffle_epi8(state_3_3, c3);
	t3 = _mm256_shuffle_epi8(K7, c2);
	t4 = _mm256_shuffle_epi8(state_3_4, c4);
	t5 = _mm256_shuffle_epi8(state_3_3, c1);
	t6 = _mm256_shuffle_epi8(K6, c3);
	t7 = _mm256_shuffle_epi8(state_3_4, c2);
	t8 = _mm256_shuffle_epi8(K7, c4);

	K6 = _mm256_xor_si256(t1, t2);
	state_3_3 = _mm256_xor_si256(t3, t4);
	K6 = _mm256_xor_si256(K6, state_3_3);

	K7 = _mm256_xor_si256(t5, t6);
	state_3_4 = _mm256_xor_si256(t7, t8);
	K7 = _mm256_xor_si256(K7, state_3_4);

	K6_L4 = _mm256_slli_epi16(K6, 4);
	KK6 = _mm256_xor_si256(K6, K6_L4);//KK6=(0xaa,0xaa,...)

	K7_L4 = _mm256_slli_epi16(K7, 4);
	KK7 = _mm256_xor_si256(K7, K7_L4);//KK7=(0xaa,0xaa,...)

	KK7 = _mm256_xor_si256(KK7, ValueK1);//KK7=(0xaa,0xaa,...)
	K7 = _mm256_xor_si256(K7, ValueK2);//KK7=(0xaa,0xaa,...)







	//取u1_x和u1_y的R4位
	u1_x_R4 = _mm256_and_si256(*u1_x, con);//即uu1_x,uu1_y
	u1_y_R4 = _mm256_and_si256(*u1_y, con);
	//明文部分的赋值

	//第一个大块和第二个大块的加密输入,第1个分支在右4位，第2个分支左4位
	state1 = *u1_x;//u1_x=(0xaa,0xaa,....)
	state2 = *u1_y;//u1_y=(0xaa,0xaa,....)

	//第一个大块和第二个大块的加密输入,第1个分支在右4位，第2个分支左4位
	SSstate1 = u1_x_R4;//u1_x_R4=(0x0a,0x0a,....)
	SSstate2 = u1_y_R4;//u1_y_R4=(0x0a,0x0a,....)

	//u1和u2的值还得再处理一下
	//state1,state2=0x(aa,0xaa,...)
	//u1和u2是按照(0xaa,0xaa,0xaa)来存储的，不是按照(0x0a,0x0a,0x0a,)来存储的
	state2 = _mm256_xor_si256(state2, Value1);//区分两个块不同的明文输入

	SSstate2 = _mm256_xor_si256(SSstate2, Value2);//区分两个块不同的明文输入









	//第三个块加密输入
	//state1_1 = u1_x_R4;//u1_x=(0xaa,0xaa,....)
	//state2_1 = u1_y_R4;//u1_y=(0xaa,0xaa,....)

	//state1_1 = _mm256_and_si256(u1_x, con);//state1_1=0x(0a,0x0a,...)
	//state2_1 = _mm256_and_si256(u1_y, con);//state2_1=0x(0a,0x0a,...)

	//u1和u2是按照(0xaa,0xaa,0xaa)来存储的，不是按照(0x0a,0x0a,0x0a,)来存储的
	//state2_1 = _mm256_xor_si256(state2_1, Value2);//区分两个块不同的明文输入


	//第1个step不需要更新轮密钥
	//The first round
	state1 = _mm256_xor_si256(state1, KK0);
	state2 = _mm256_xor_si256(state2, KK1);
	state1_L4 = _mm256_and_si256(state1, con_L4);
	state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);
	state1_R4 = _mm256_and_si256(state1, con);
	state1_L4 = _mm256_shuffle_epi8(S_L4, state1_L4_R4);
	state1_R4 = _mm256_shuffle_epi8(S, state1_R4);
	state1 = _mm256_xor_si256(state1_L4, state1_R4);

	state2_L4 = _mm256_and_si256(state2, con_L4);
	state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);
	state2_R4 = _mm256_and_si256(state2, con);
	state2_L4 = _mm256_shuffle_epi8(S_L4, state2_L4_R4);
	state2_R4 = _mm256_shuffle_epi8(S, state2_R4);
	state2 = _mm256_xor_si256(state2_L4, state2_R4);

	//state2 = _mm256_shuffle_epi8(S, state2);

	state2 = _mm256_xor_si256(state2, state1);

	k_m = _mm256_shuffle_epi8(state2, A1);
	state1 = _mm256_xor_si256(state1, k_m);

	k_m = _mm256_shuffle_epi8(state1, A2);
	state2 = _mm256_xor_si256(state2, k_m);

	k_m = _mm256_shuffle_epi8(state2, A2);
	state1 = _mm256_xor_si256(state1, k_m);

	k_m = _mm256_shuffle_epi8(state1, A3);
	state2 = _mm256_xor_si256(state2, k_m);

	state1 = _mm256_xor_si256(state1, state2);

	/*
	The difference with uBlock round function
	*/
	state3 = _mm256_permute2x128_si256(state1, state1, 1);//Left and right change
	state1 = _mm256_shuffle_epi8(state3, PL);//Byte permutation within 128-bit

	state2 = _mm256_shuffle_epi8(state2, PR);//Byte permutation within 128-bit




	//The third Brunch 
	//The first round
	SSstate1 = _mm256_xor_si256(SSstate1, K0);
	SSstate2 = _mm256_xor_si256(SSstate2, K1);
	//SSstate1_L4 = _mm256_and_si256(state1, con_L4);
	//state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);
	//state1_R4 = _mm256_and_si256(state1, con);
	//state1_L4 = _mm256_shuffle_epi8(S_L4, state1_L4_R4);
	SSstate1 = _mm256_shuffle_epi8(S, SSstate1);
	//state1 = _mm256_xor_si256(state1_L4, state1_R4);

	//state2_L4 = _mm256_and_si256(state2, con_L4);
	//state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);
	//state2_R4 = _mm256_and_si256(state2, con);
	//state2_L4 = _mm256_shuffle_epi8(S_L4, state2_L4_R4);
	SSstate2 = _mm256_shuffle_epi8(S, SSstate2);
	//state2 = _mm256_xor_si256(state2_L4, state2_R4);

	//state2 = _mm256_shuffle_epi8(S, state2);

	SSstate2 = _mm256_xor_si256(SSstate2, SSstate1);

	k_m = _mm256_shuffle_epi8(SSstate2, A1);
	SSstate1 = _mm256_xor_si256(SSstate1, k_m);

	k_m = _mm256_shuffle_epi8(SSstate1, A2);
	SSstate2 = _mm256_xor_si256(SSstate2, k_m);

	k_m = _mm256_shuffle_epi8(SSstate2, A2);
	SSstate1 = _mm256_xor_si256(SSstate1, k_m);

	k_m = _mm256_shuffle_epi8(SSstate1, A3);
	SSstate2 = _mm256_xor_si256(SSstate2, k_m);

	SSstate1 = _mm256_xor_si256(SSstate1, SSstate2);

	/*
	The difference with uBlock round function
	*/
	SSstate3 = _mm256_permute2x128_si256(SSstate1, SSstate1, 1);//Left and right change
	SSstate1 = _mm256_shuffle_epi8(SSstate3, PL);//Byte permutation within 128-bit

	SSstate2 = _mm256_shuffle_epi8(SSstate2, PR);//Byte permutation within 128-bit







	//The second round
	state1 = _mm256_xor_si256(state1, KK2);
	state2 = _mm256_xor_si256(state2, KK3);
	state1_L4 = _mm256_and_si256(state1, con_L4);
	state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);
	state1_R4 = _mm256_and_si256(state1, con);
	state1_L4 = _mm256_shuffle_epi8(S_L4, state1_L4_R4);
	state1_R4 = _mm256_shuffle_epi8(S, state1_R4);
	state1 = _mm256_xor_si256(state1_L4, state1_R4);

	state2_L4 = _mm256_and_si256(state2, con_L4);
	state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);
	state2_R4 = _mm256_and_si256(state2, con);
	state2_L4 = _mm256_shuffle_epi8(S_L4, state2_L4_R4);
	state2_R4 = _mm256_shuffle_epi8(S, state2_R4);
	state2 = _mm256_xor_si256(state2_L4, state2_R4);

	//state2 = _mm256_shuffle_epi8(S, state2);

	state2 = _mm256_xor_si256(state2, state1);

	k_m = _mm256_shuffle_epi8(state2, A1);
	state1 = _mm256_xor_si256(state1, k_m);

	k_m = _mm256_shuffle_epi8(state1, A2);
	state2 = _mm256_xor_si256(state2, k_m);

	k_m = _mm256_shuffle_epi8(state2, A2);
	state1 = _mm256_xor_si256(state1, k_m);

	k_m = _mm256_shuffle_epi8(state1, A3);
	state2 = _mm256_xor_si256(state2, k_m);

	state1 = _mm256_xor_si256(state1, state2);

	/*
	The difference with uBlock round function
	*/
	state3 = _mm256_permute2x128_si256(state1, state1, 1);//Left and right change
	state1 = _mm256_shuffle_epi8(state3, PL);//Byte permutation within 128-bit

	state2 = _mm256_shuffle_epi8(state2, PR);//Byte permutation within 128-bit


	//The third Brunch 
	//The second round
	SSstate1 = _mm256_xor_si256(SSstate1, K2);
	SSstate2 = _mm256_xor_si256(SSstate2, K3);
	//SSstate1_L4 = _mm256_and_si256(state1, con_L4);
	//state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);
	//state1_R4 = _mm256_and_si256(state1, con);
	//state1_L4 = _mm256_shuffle_epi8(S_L4, state1_L4_R4);
	SSstate1 = _mm256_shuffle_epi8(S, SSstate1);
	//state1 = _mm256_xor_si256(state1_L4, state1_R4);

	//state2_L4 = _mm256_and_si256(state2, con_L4);
	//state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);
	//state2_R4 = _mm256_and_si256(state2, con);
	//state2_L4 = _mm256_shuffle_epi8(S_L4, state2_L4_R4);
	SSstate2 = _mm256_shuffle_epi8(S, SSstate2);
	//state2 = _mm256_xor_si256(state2_L4, state2_R4);

	//state2 = _mm256_shuffle_epi8(S, state2);

	SSstate2 = _mm256_xor_si256(SSstate2, SSstate1);

	k_m = _mm256_shuffle_epi8(SSstate2, A1);
	SSstate1 = _mm256_xor_si256(SSstate1, k_m);

	k_m = _mm256_shuffle_epi8(SSstate1, A2);
	SSstate2 = _mm256_xor_si256(SSstate2, k_m);

	k_m = _mm256_shuffle_epi8(SSstate2, A2);
	SSstate1 = _mm256_xor_si256(SSstate1, k_m);

	k_m = _mm256_shuffle_epi8(SSstate1, A3);
	SSstate2 = _mm256_xor_si256(SSstate2, k_m);

	SSstate1 = _mm256_xor_si256(SSstate1, SSstate2);


	/*
The difference with uBlock round function
*/
	SSstate3 = _mm256_permute2x128_si256(SSstate1, SSstate1, 1);//Left and right change
	SSstate1 = _mm256_shuffle_epi8(SSstate3, PL);//Byte permutation within 128-bit

	SSstate2 = _mm256_shuffle_epi8(SSstate2, PR);//Byte permutation within 128-bit





	//The third round
	state1 = _mm256_xor_si256(state1, KK4);
	state2 = _mm256_xor_si256(state2, KK5);
	state1_L4 = _mm256_and_si256(state1, con_L4);
	state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);
	state1_R4 = _mm256_and_si256(state1, con);
	state1_L4 = _mm256_shuffle_epi8(S_L4, state1_L4_R4);
	state1_R4 = _mm256_shuffle_epi8(S, state1_R4);
	state1 = _mm256_xor_si256(state1_L4, state1_R4);

	state2_L4 = _mm256_and_si256(state2, con_L4);
	state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);
	state2_R4 = _mm256_and_si256(state2, con);
	state2_L4 = _mm256_shuffle_epi8(S_L4, state2_L4_R4);
	state2_R4 = _mm256_shuffle_epi8(S, state2_R4);
	state2 = _mm256_xor_si256(state2_L4, state2_R4);

	//state2 = _mm256_shuffle_epi8(S, state2);

	state2 = _mm256_xor_si256(state2, state1);

	k_m = _mm256_shuffle_epi8(state2, A1);
	state1 = _mm256_xor_si256(state1, k_m);

	k_m = _mm256_shuffle_epi8(state1, A2);
	state2 = _mm256_xor_si256(state2, k_m);

	k_m = _mm256_shuffle_epi8(state2, A2);
	state1 = _mm256_xor_si256(state1, k_m);

	k_m = _mm256_shuffle_epi8(state1, A3);
	state2 = _mm256_xor_si256(state2, k_m);

	state1 = _mm256_xor_si256(state1, state2);

	/*
	The difference with uBlock round function
	*/
	state3 = _mm256_permute2x128_si256(state1, state1, 1);//Left and right change
	state1 = _mm256_shuffle_epi8(state3, PL);//Byte permutation within 128-bit

	state2 = _mm256_shuffle_epi8(state2, PR);//Byte permutation within 128-bit


	//The third Brunch 
	 //The third round
	SSstate1 = _mm256_xor_si256(SSstate1, K4);
	SSstate2 = _mm256_xor_si256(SSstate2, K5);
	//SSstate1_L4 = _mm256_and_si256(state1, con_L4);
	//state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);
	//state1_R4 = _mm256_and_si256(state1, con);
	//state1_L4 = _mm256_shuffle_epi8(S_L4, state1_L4_R4);
	SSstate1 = _mm256_shuffle_epi8(S, SSstate1);
	//state1 = _mm256_xor_si256(state1_L4, state1_R4);

	//state2_L4 = _mm256_and_si256(state2, con_L4);
	//state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);
	//state2_R4 = _mm256_and_si256(state2, con);
	//state2_L4 = _mm256_shuffle_epi8(S_L4, state2_L4_R4);
	SSstate2 = _mm256_shuffle_epi8(S, SSstate2);
	//state2 = _mm256_xor_si256(state2_L4, state2_R4);

	//state2 = _mm256_shuffle_epi8(S, state2);

	SSstate2 = _mm256_xor_si256(SSstate2, SSstate1);

	k_m = _mm256_shuffle_epi8(SSstate2, A1);
	SSstate1 = _mm256_xor_si256(SSstate1, k_m);

	k_m = _mm256_shuffle_epi8(SSstate1, A2);
	SSstate2 = _mm256_xor_si256(SSstate2, k_m);

	k_m = _mm256_shuffle_epi8(SSstate2, A2);
	SSstate1 = _mm256_xor_si256(SSstate1, k_m);

	k_m = _mm256_shuffle_epi8(SSstate1, A3);
	SSstate2 = _mm256_xor_si256(SSstate2, k_m);

	SSstate1 = _mm256_xor_si256(SSstate1, SSstate2);


	/*
The difference with uBlock round function
*/
	SSstate3 = _mm256_permute2x128_si256(SSstate1, SSstate1, 1);//Left and right change
	SSstate1 = _mm256_shuffle_epi8(SSstate3, PL);//Byte permutation within 128-bit

	SSstate2 = _mm256_shuffle_epi8(SSstate2, PR);//Byte permutation within 128-bit






	//The fourth round
	state1 = _mm256_xor_si256(state1, KK6);
	state2 = _mm256_xor_si256(state2, KK7);
	state1_L4 = _mm256_and_si256(state1, con_L4);
	state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);
	state1_R4 = _mm256_and_si256(state1, con);
	state1_L4 = _mm256_shuffle_epi8(S_L4, state1_L4_R4);
	state1_R4 = _mm256_shuffle_epi8(S, state1_R4);
	state1 = _mm256_xor_si256(state1_L4, state1_R4);

	state2_L4 = _mm256_and_si256(state2, con_L4);
	state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);
	state2_R4 = _mm256_and_si256(state2, con);
	state2_L4 = _mm256_shuffle_epi8(S_L4, state2_L4_R4);
	state2_R4 = _mm256_shuffle_epi8(S, state2_R4);
	state2 = _mm256_xor_si256(state2_L4, state2_R4);

	//state2 = _mm256_shuffle_epi8(S, state2);

	state2 = _mm256_xor_si256(state2, state1);

	k_m = _mm256_shuffle_epi8(state2, A1);
	state1 = _mm256_xor_si256(state1, k_m);

	k_m = _mm256_shuffle_epi8(state1, A2);
	state2 = _mm256_xor_si256(state2, k_m);

	k_m = _mm256_shuffle_epi8(state2, A2);
	state1 = _mm256_xor_si256(state1, k_m);

	k_m = _mm256_shuffle_epi8(state1, A3);
	state2 = _mm256_xor_si256(state2, k_m);

	state1 = _mm256_xor_si256(state1, state2);

	/*
	The difference with uBlock round function
	*/
	state3 = _mm256_permute2x128_si256(state1, state1, 1);//Left and right change
	state1 = _mm256_shuffle_epi8(state3, PL);//Byte permutation within 128-bit

	state2 = _mm256_shuffle_epi8(state2, PR);//Byte permutation within 128-bit


	//The third Brunch 
	//The fourth round
	SSstate1 = _mm256_xor_si256(SSstate1, K6);
	SSstate2 = _mm256_xor_si256(SSstate2, K7);
	//SSstate1_L4 = _mm256_and_si256(state1, con_L4);
	//state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);
	//state1_R4 = _mm256_and_si256(state1, con);
	//state1_L4 = _mm256_shuffle_epi8(S_L4, state1_L4_R4);
	SSstate1 = _mm256_shuffle_epi8(S, SSstate1);
	//state1 = _mm256_xor_si256(state1_L4, state1_R4);

	//state2_L4 = _mm256_and_si256(state2, con_L4);
	//state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);
	//state2_R4 = _mm256_and_si256(state2, con);
	//state2_L4 = _mm256_shuffle_epi8(S_L4, state2_L4_R4);
	SSstate2 = _mm256_shuffle_epi8(S, SSstate2);
	//state2 = _mm256_xor_si256(state2_L4, state2_R4);

	//state2 = _mm256_shuffle_epi8(S, state2);

	SSstate2 = _mm256_xor_si256(SSstate2, SSstate1);

	k_m = _mm256_shuffle_epi8(SSstate2, A1);
	SSstate1 = _mm256_xor_si256(SSstate1, k_m);

	k_m = _mm256_shuffle_epi8(SSstate1, A2);
	SSstate2 = _mm256_xor_si256(SSstate2, k_m);

	k_m = _mm256_shuffle_epi8(SSstate2, A2);
	SSstate1 = _mm256_xor_si256(SSstate1, k_m);

	k_m = _mm256_shuffle_epi8(SSstate1, A3);
	SSstate2 = _mm256_xor_si256(SSstate2, k_m);

	SSstate1 = _mm256_xor_si256(SSstate1, SSstate2);


	/*
The difference with uBlock round function
*/
	SSstate3 = _mm256_permute2x128_si256(SSstate1, SSstate1, 1);//Left and right change
	SSstate1 = _mm256_shuffle_epi8(SSstate3, PL);//Byte permutation within 128-bit

	SSstate2 = _mm256_shuffle_epi8(SSstate2, PR);//Byte permutation within 128-bit






	//这里的round其实代表的是step

	for (i = 1; i < round; i++)
	{
		//The first round
		tK1 = K0;
		tK2 = K1;
		__m256i RoundConst = _mm256_setr_epi8(rrc[i - 1][0], rrc[i - 1][1], rrc[i - 1][2], rrc[i - 1][3], rrc[i - 1][4], rrc[i - 1][5], rrc[i - 1][6], rrc[i - 1][7], rrc[i - 1][8], rrc[i - 1][9], rrc[i - 1][10], rrc[i - 1][11], rrc[i - 1][12], rrc[i - 1][13], rrc[i - 1][14], rrc[i - 1][15], rrc[i - 1][16], rrc[i - 1][17], rrc[i - 1][18], rrc[i - 1][19], rrc[i - 1][20], rrc[i - 1][21], rrc[i - 1][22], rrc[i - 1][23], rrc[i - 1][24], rrc[i - 1][25], rrc[i - 1][26], rrc[i - 1][27], rrc[i - 1][28], rrc[i - 1][29], rrc[i - 1][30], rrc[i - 1][31]);
		//_mm256_setr_epi32(RC[i - 1][0], RC[i - 1][1], RC[i - 1][2], RC[i - 1][3], RC[i - 1][4], RC[i - 1][5], RC[i - 1][6], RC[i - 1][7]);

		//@@@@@@Bug is in here@@@@@@@
		K0 = _mm256_xor_si256(K2, RoundConst);
		K0 = _mm256_shuffle_epi8(K0, F);//the output after function P2
		//K0 = _mm256_shuffle_epi8(K2, F);//the output after function P2
		tG = _mm256_permutevar8x32_epi32(K5, G_Word);
		K1 = _mm256_shuffle_epi8(tG, G_Nibble);//the output after function G

		//*** Modify 2***
		//The difference in the last block is in the xor value for master key
		tKK1 = KK0;
		tKK2 = KK1;
		__m256i RoundConst1 = _mm256_setr_epi8(rc[i - 1][0], rc[i - 1][1], rc[i - 1][2], rc[i - 1][3], rc[i - 1][4], rc[i - 1][5], rc[i - 1][6], rc[i - 1][7], rc[i - 1][8], rc[i - 1][9], rc[i - 1][10], rc[i - 1][11], rc[i - 1][12], rc[i - 1][13], rc[i - 1][14], rc[i - 1][15], rc[i - 1][16], rc[i - 1][17], rc[i - 1][18], rc[i - 1][19], rc[i - 1][20], rc[i - 1][21], rc[i - 1][22], rc[i - 1][23], rc[i - 1][24], rc[i - 1][25], rc[i - 1][26], rc[i - 1][27], rc[i - 1][28], rc[i - 1][29], rc[i - 1][30], rc[i - 1][31]);
		//_mm256_setr_epi32(RC[i - 1][0], RC[i - 1][1], RC[i - 1][2], RC[i - 1][3], RC[i - 1][4], RC[i - 1][5], RC[i - 1][6], RC[i - 1][7]);

		//@@@@@@Bug is in here@@@@@@@
		KK0 = _mm256_xor_si256(KK2, RoundConst1);
		KK0 = _mm256_shuffle_epi8(KK0, F);//the output after function P2
		//K0 = _mm256_shuffle_epi8(K2, F);//the output after function P2
		tGG = _mm256_permutevar8x32_epi32(KK5, G_Word);
		KK1 = _mm256_shuffle_epi8(tGG, G_Nibble);//the output after function G

		//得到K0，K1=（0x0a,0x0a,...）后生成KK0,KK1=(0xaa,0xaa,...)
		//K0_L4 = _mm256_slli_epi16(K0, 4);
		//KK0 = _mm256_xor_si256(K0, K0_L4);//KK0=(0xaa,0xaa,...)

		//K1_L4 = _mm256_slli_epi16(K1, 4);
		//KK1 = _mm256_xor_si256(K1, K1_L4);//KK1=(0xaa,0xaa,...)








		state1 = _mm256_xor_si256(state1, KK0);
		state2 = _mm256_xor_si256(state2, KK1);
		state1_L4 = _mm256_and_si256(state1, con_L4);
		state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);
		state1_R4 = _mm256_and_si256(state1, con);
		state1_L4 = _mm256_shuffle_epi8(S_L4, state1_L4_R4);
		state1_R4 = _mm256_shuffle_epi8(S, state1_R4);
		state1 = _mm256_xor_si256(state1_L4, state1_R4);

		state2_L4 = _mm256_and_si256(state2, con_L4);
		state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);
		state2_R4 = _mm256_and_si256(state2, con);
		state2_L4 = _mm256_shuffle_epi8(S_L4, state2_L4_R4);
		state2_R4 = _mm256_shuffle_epi8(S, state2_R4);
		state2 = _mm256_xor_si256(state2_L4, state2_R4);

		//state2 = _mm256_shuffle_epi8(S, state2);

		state2 = _mm256_xor_si256(state2, state1);

		k_m = _mm256_shuffle_epi8(state2, A1);
		state1 = _mm256_xor_si256(state1, k_m);

		k_m = _mm256_shuffle_epi8(state1, A2);
		state2 = _mm256_xor_si256(state2, k_m);

		k_m = _mm256_shuffle_epi8(state2, A2);
		state1 = _mm256_xor_si256(state1, k_m);

		k_m = _mm256_shuffle_epi8(state1, A3);
		state2 = _mm256_xor_si256(state2, k_m);

		state1 = _mm256_xor_si256(state1, state2);

		/*
		The difference with uBlock round function
		*/
		state3 = _mm256_permute2x128_si256(state1, state1, 1);//Left and right change
		state1 = _mm256_shuffle_epi8(state3, PL);//Byte permutation within 128-bit

		state2 = _mm256_shuffle_epi8(state2, PR);//Byte permutation within 128-bit


		//The third Brunch 
		//The first round
		SSstate1 = _mm256_xor_si256(SSstate1, K0);
		SSstate2 = _mm256_xor_si256(SSstate2, K1);
		//SSstate1_L4 = _mm256_and_si256(state1, con_L4);
		//state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);
		//state1_R4 = _mm256_and_si256(state1, con);
		//state1_L4 = _mm256_shuffle_epi8(S_L4, state1_L4_R4);
		SSstate1 = _mm256_shuffle_epi8(S, SSstate1);
		//state1 = _mm256_xor_si256(state1_L4, state1_R4);

		//state2_L4 = _mm256_and_si256(state2, con_L4);
		//state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);
		//state2_R4 = _mm256_and_si256(state2, con);
		//state2_L4 = _mm256_shuffle_epi8(S_L4, state2_L4_R4);
		SSstate2 = _mm256_shuffle_epi8(S, SSstate2);
		//state2 = _mm256_xor_si256(state2_L4, state2_R4);

		//state2 = _mm256_shuffle_epi8(S, state2);

		SSstate2 = _mm256_xor_si256(SSstate2, SSstate1);

		k_m = _mm256_shuffle_epi8(SSstate2, A1);
		SSstate1 = _mm256_xor_si256(SSstate1, k_m);

		k_m = _mm256_shuffle_epi8(SSstate1, A2);
		SSstate2 = _mm256_xor_si256(SSstate2, k_m);

		k_m = _mm256_shuffle_epi8(SSstate2, A2);
		SSstate1 = _mm256_xor_si256(SSstate1, k_m);

		k_m = _mm256_shuffle_epi8(SSstate1, A3);
		SSstate2 = _mm256_xor_si256(SSstate2, k_m);

		SSstate1 = _mm256_xor_si256(SSstate1, SSstate2);


		/*
The difference with uBlock round function
*/
		SSstate3 = _mm256_permute2x128_si256(SSstate1, SSstate1, 1);//Left and right change
		SSstate1 = _mm256_shuffle_epi8(SSstate3, PL);//Byte permutation within 128-bit

		SSstate2 = _mm256_shuffle_epi8(SSstate2, PR);//Byte permutation within 128-bit







		//The second round
		K2 = _mm256_shuffle_epi8(tK2, F);//the output after function P2
		tK2 = K3;
		tG = _mm256_permutevar8x32_epi32(tK1, G_Word);
		K3 = _mm256_shuffle_epi8(tG, G_Nibble);//the output after function G

		//*** Modify 3***
		// The difference in the last block is the XOR value of the master key
		// 
		//The second round
		KK2 = _mm256_shuffle_epi8(tKK2, F);//the output after function P2
		tKK2 = KK3;
		tGG = _mm256_permutevar8x32_epi32(tKK1, G_Word);
		KK3 = _mm256_shuffle_epi8(tGG, G_Nibble);//the output after function G

		//得到K0，K1=（0x0a,0x0a,...）后生成KK0,KK1=(0xaa,0xaa,...)
		//K2_L4 = _mm256_slli_epi16(K2, 4);
		//KK2 = _mm256_xor_si256(K2, K2_L4);//KK0=(0xaa,0xaa,...)

		//K3_L4 = _mm256_slli_epi16(K3, 4);
		//KK3 = _mm256_xor_si256(K3, K3_L4);//KK1=(0xaa,0xaa,...)



		state1 = _mm256_xor_si256(state1, KK2);
		state2 = _mm256_xor_si256(state2, KK3);
		state1_L4 = _mm256_and_si256(state1, con_L4);
		state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);
		state1_R4 = _mm256_and_si256(state1, con);
		state1_L4 = _mm256_shuffle_epi8(S_L4, state1_L4_R4);
		state1_R4 = _mm256_shuffle_epi8(S, state1_R4);
		state1 = _mm256_xor_si256(state1_L4, state1_R4);

		state2_L4 = _mm256_and_si256(state2, con_L4);
		state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);
		state2_R4 = _mm256_and_si256(state2, con);
		state2_L4 = _mm256_shuffle_epi8(S_L4, state2_L4_R4);
		state2_R4 = _mm256_shuffle_epi8(S, state2_R4);
		state2 = _mm256_xor_si256(state2_L4, state2_R4);

		//state2 = _mm256_shuffle_epi8(S, state2);

		state2 = _mm256_xor_si256(state2, state1);

		k_m = _mm256_shuffle_epi8(state2, A1);
		state1 = _mm256_xor_si256(state1, k_m);

		k_m = _mm256_shuffle_epi8(state1, A2);
		state2 = _mm256_xor_si256(state2, k_m);

		k_m = _mm256_shuffle_epi8(state2, A2);
		state1 = _mm256_xor_si256(state1, k_m);

		k_m = _mm256_shuffle_epi8(state1, A3);
		state2 = _mm256_xor_si256(state2, k_m);

		state1 = _mm256_xor_si256(state1, state2);

		/*
		The difference with uBlock round function
		*/
		state3 = _mm256_permute2x128_si256(state1, state1, 1);//Left and right change
		state1 = _mm256_shuffle_epi8(state3, PL);//Byte permutation within 128-bit

		state2 = _mm256_shuffle_epi8(state2, PR);//Byte permutation within 128-bit


		//The third Brunch 
		//The second round
		SSstate1 = _mm256_xor_si256(SSstate1, K2);
		SSstate2 = _mm256_xor_si256(SSstate2, K3);
		//SSstate1_L4 = _mm256_and_si256(state1, con_L4);
		//state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);
		//state1_R4 = _mm256_and_si256(state1, con);
		//state1_L4 = _mm256_shuffle_epi8(S_L4, state1_L4_R4);
		SSstate1 = _mm256_shuffle_epi8(S, SSstate1);
		//state1 = _mm256_xor_si256(state1_L4, state1_R4);

		//state2_L4 = _mm256_and_si256(state2, con_L4);
		//state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);
		//state2_R4 = _mm256_and_si256(state2, con);
		//state2_L4 = _mm256_shuffle_epi8(S_L4, state2_L4_R4);
		SSstate2 = _mm256_shuffle_epi8(S, SSstate2);
		//state2 = _mm256_xor_si256(state2_L4, state2_R4);

		//state2 = _mm256_shuffle_epi8(S, state2);

		SSstate2 = _mm256_xor_si256(SSstate2, SSstate1);

		k_m = _mm256_shuffle_epi8(SSstate2, A1);
		SSstate1 = _mm256_xor_si256(SSstate1, k_m);

		k_m = _mm256_shuffle_epi8(SSstate1, A2);
		SSstate2 = _mm256_xor_si256(SSstate2, k_m);

		k_m = _mm256_shuffle_epi8(SSstate2, A2);
		SSstate1 = _mm256_xor_si256(SSstate1, k_m);

		k_m = _mm256_shuffle_epi8(SSstate1, A3);
		SSstate2 = _mm256_xor_si256(SSstate2, k_m);

		SSstate1 = _mm256_xor_si256(SSstate1, SSstate2);

		/*
The difference with uBlock round function
*/
		SSstate3 = _mm256_permute2x128_si256(SSstate1, SSstate1, 1);//Left and right change
		SSstate1 = _mm256_shuffle_epi8(SSstate3, PL);//Byte permutation within 128-bit

		SSstate2 = _mm256_shuffle_epi8(SSstate2, PR);//Byte permutation within 128-bit





		//The third round
		tK1 = K4;
		K4 = _mm256_shuffle_epi8(tK2, F);//the output after function P2

		tG = _mm256_permutevar8x32_epi32(K6, G_Word);
		K5 = _mm256_shuffle_epi8(tG, G_Nibble);//the output after function G
		//得到K0，K1=（0x0a,0x0a,...）后生成KK0,KK1=(0xaa,0xaa,...)


		//*** Modify 3***
		// The difference in the last block is the XOR value for the master key
		//The third round
		tKK1 = KK4;
		KK4 = _mm256_shuffle_epi8(tKK2, F);//the output after function P2

		tGG = _mm256_permutevar8x32_epi32(KK6, G_Word);
		KK5 = _mm256_shuffle_epi8(tGG, G_Nibble);//the output after function G


		//K4_L4 = _mm256_slli_epi16(K4, 4);
		//KK4 = _mm256_xor_si256(K4, K4_L4);//KK0=(0xaa,0xaa,...)

		//K5_L4 = _mm256_slli_epi16(K5, 4);
		//KK5 = _mm256_xor_si256(K5, K5_L4);//KK1=(0xaa,0xaa,...)




		state1 = _mm256_xor_si256(state1, KK4);
		state2 = _mm256_xor_si256(state2, KK5);
		state1_L4 = _mm256_and_si256(state1, con_L4);
		state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);
		state1_R4 = _mm256_and_si256(state1, con);
		state1_L4 = _mm256_shuffle_epi8(S_L4, state1_L4_R4);
		state1_R4 = _mm256_shuffle_epi8(S, state1_R4);
		state1 = _mm256_xor_si256(state1_L4, state1_R4);

		state2_L4 = _mm256_and_si256(state2, con_L4);
		state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);
		state2_R4 = _mm256_and_si256(state2, con);
		state2_L4 = _mm256_shuffle_epi8(S_L4, state2_L4_R4);
		state2_R4 = _mm256_shuffle_epi8(S, state2_R4);
		state2 = _mm256_xor_si256(state2_L4, state2_R4);

		//state2 = _mm256_shuffle_epi8(S, state2);

		state2 = _mm256_xor_si256(state2, state1);

		k_m = _mm256_shuffle_epi8(state2, A1);
		state1 = _mm256_xor_si256(state1, k_m);

		k_m = _mm256_shuffle_epi8(state1, A2);
		state2 = _mm256_xor_si256(state2, k_m);

		k_m = _mm256_shuffle_epi8(state2, A2);
		state1 = _mm256_xor_si256(state1, k_m);

		k_m = _mm256_shuffle_epi8(state1, A3);
		state2 = _mm256_xor_si256(state2, k_m);

		state1 = _mm256_xor_si256(state1, state2);

		/*
		The difference with uBlock round function
		*/
		state3 = _mm256_permute2x128_si256(state1, state1, 1);//Left and right change
		state1 = _mm256_shuffle_epi8(state3, PL);//Byte permutation within 128-bit

		state2 = _mm256_shuffle_epi8(state2, PR);//Byte permutation within 128-bit


		//The third Brunch 
		//The third round
		SSstate1 = _mm256_xor_si256(SSstate1, K4);
		SSstate2 = _mm256_xor_si256(SSstate2, K5);
		//SSstate1_L4 = _mm256_and_si256(state1, con_L4);
		//state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);
		//state1_R4 = _mm256_and_si256(state1, con);
		//state1_L4 = _mm256_shuffle_epi8(S_L4, state1_L4_R4);
		SSstate1 = _mm256_shuffle_epi8(S, SSstate1);
		//state1 = _mm256_xor_si256(state1_L4, state1_R4);

		//state2_L4 = _mm256_and_si256(state2, con_L4);
		//state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);
		//state2_R4 = _mm256_and_si256(state2, con);
		//state2_L4 = _mm256_shuffle_epi8(S_L4, state2_L4_R4);
		SSstate2 = _mm256_shuffle_epi8(S, SSstate2);
		//state2 = _mm256_xor_si256(state2_L4, state2_R4);

		//state2 = _mm256_shuffle_epi8(S, state2);

		SSstate2 = _mm256_xor_si256(SSstate2, SSstate1);

		k_m = _mm256_shuffle_epi8(SSstate2, A1);
		SSstate1 = _mm256_xor_si256(SSstate1, k_m);

		k_m = _mm256_shuffle_epi8(SSstate1, A2);
		SSstate2 = _mm256_xor_si256(SSstate2, k_m);

		k_m = _mm256_shuffle_epi8(SSstate2, A2);
		SSstate1 = _mm256_xor_si256(SSstate1, k_m);

		k_m = _mm256_shuffle_epi8(SSstate1, A3);
		SSstate2 = _mm256_xor_si256(SSstate2, k_m);

		SSstate1 = _mm256_xor_si256(SSstate1, SSstate2);


		/*
The difference with uBlock round function
*/
		SSstate3 = _mm256_permute2x128_si256(SSstate1, SSstate1, 1);//Left and right change
		SSstate1 = _mm256_shuffle_epi8(SSstate3, PL);//Byte permutation within 128-bit

		SSstate2 = _mm256_shuffle_epi8(SSstate2, PR);//Byte permutation within 128-bit





		//The fourth round

		K6 = _mm256_shuffle_epi8(K7, F);//the output after function P2

		tG = _mm256_permutevar8x32_epi32(tK1, G_Word);
		K7 = _mm256_shuffle_epi8(tG, G_Nibble);//the output after function G
		//得到K0，K1=（0x0a,0x0a,...）后生成KK0,KK1=(0xaa,0xaa,...)


		//*** Modify 3 ***
		// The difference in the last block is the XOR value of the master key
		//The fourth round
		KK6 = _mm256_shuffle_epi8(KK7, F);//the output after function P2

		tGG = _mm256_permutevar8x32_epi32(tKK1, G_Word);
		KK7 = _mm256_shuffle_epi8(tGG, G_Nibble);//the output after function G
		//得到K0，K1=（0x0a,0x0a,...）后生成KK0,KK1=(0xaa,0xaa,...)



		//K6_L4 = _mm256_slli_epi16(K6, 4);
		//KK6 = _mm256_xor_si256(K6, K6_L4);//KK0=(0xaa,0xaa,...)

		//K7_L4 = _mm256_slli_epi16(K7, 4);
		//KK7 = _mm256_xor_si256(K7, K7_L4);//KK1=(0xaa,0xaa,...)




		state1 = _mm256_xor_si256(state1, KK6);
		state2 = _mm256_xor_si256(state2, KK7);
		state1_L4 = _mm256_and_si256(state1, con_L4);
		state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);
		state1_R4 = _mm256_and_si256(state1, con);
		state1_L4 = _mm256_shuffle_epi8(S_L4, state1_L4_R4);
		state1_R4 = _mm256_shuffle_epi8(S, state1_R4);
		state1 = _mm256_xor_si256(state1_L4, state1_R4);

		state2_L4 = _mm256_and_si256(state2, con_L4);
		state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);
		state2_R4 = _mm256_and_si256(state2, con);
		state2_L4 = _mm256_shuffle_epi8(S_L4, state2_L4_R4);
		state2_R4 = _mm256_shuffle_epi8(S, state2_R4);
		state2 = _mm256_xor_si256(state2_L4, state2_R4);

		//state2 = _mm256_shuffle_epi8(S, state2);

		state2 = _mm256_xor_si256(state2, state1);

		k_m = _mm256_shuffle_epi8(state2, A1);
		state1 = _mm256_xor_si256(state1, k_m);

		k_m = _mm256_shuffle_epi8(state1, A2);
		state2 = _mm256_xor_si256(state2, k_m);

		k_m = _mm256_shuffle_epi8(state2, A2);
		state1 = _mm256_xor_si256(state1, k_m);

		k_m = _mm256_shuffle_epi8(state1, A3);
		state2 = _mm256_xor_si256(state2, k_m);

		state1 = _mm256_xor_si256(state1, state2);

		/*
		The difference with uBlock round function
		*/
		state3 = _mm256_permute2x128_si256(state1, state1, 1);//Left and right change
		state1 = _mm256_shuffle_epi8(state3, PL);//Byte permutation within 128-bit

		state2 = _mm256_shuffle_epi8(state2, PR);//Byte permutation within 128-bit

		//The third Brunch 
		 //The fourth round
		SSstate1 = _mm256_xor_si256(SSstate1, K6);
		SSstate2 = _mm256_xor_si256(SSstate2, K7);
		//SSstate1_L4 = _mm256_and_si256(state1, con_L4);
		//state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);
		//state1_R4 = _mm256_and_si256(state1, con);
		//state1_L4 = _mm256_shuffle_epi8(S_L4, state1_L4_R4);
		SSstate1 = _mm256_shuffle_epi8(S, SSstate1);
		//state1 = _mm256_xor_si256(state1_L4, state1_R4);

		//state2_L4 = _mm256_and_si256(state2, con_L4);
		//state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);
		//state2_R4 = _mm256_and_si256(state2, con);
		//state2_L4 = _mm256_shuffle_epi8(S_L4, state2_L4_R4);
		SSstate2 = _mm256_shuffle_epi8(S, SSstate2);
		//state2 = _mm256_xor_si256(state2_L4, state2_R4);

		//state2 = _mm256_shuffle_epi8(S, state2);

		SSstate2 = _mm256_xor_si256(SSstate2, SSstate1);

		k_m = _mm256_shuffle_epi8(SSstate2, A1);
		SSstate1 = _mm256_xor_si256(SSstate1, k_m);

		k_m = _mm256_shuffle_epi8(SSstate1, A2);
		SSstate2 = _mm256_xor_si256(SSstate2, k_m);

		k_m = _mm256_shuffle_epi8(SSstate2, A2);
		SSstate1 = _mm256_xor_si256(SSstate1, k_m);

		k_m = _mm256_shuffle_epi8(SSstate1, A3);
		SSstate2 = _mm256_xor_si256(SSstate2, k_m);

		SSstate1 = _mm256_xor_si256(SSstate1, SSstate2);


		/*
The difference with uBlock round function
*/
		SSstate3 = _mm256_permute2x128_si256(SSstate1, SSstate1, 1);//Left and right change
		SSstate1 = _mm256_shuffle_epi8(SSstate3, PL);//Byte permutation within 128-bit

		SSstate2 = _mm256_shuffle_epi8(SSstate2, PR);//Byte permutation within 128-bit





	}

	//*** Modify 1***
	// Removing the white key xor in the end of the round function
	//state1 = _mm256_xor_si256(state1, KK0);
	//state2 = _mm256_xor_si256(state2, KK1);

	//SSstate1 = _mm256_xor_si256(SSstate1, K0);
	//SSstate2 = _mm256_xor_si256(SSstate2, K1);



	/*
	* ==========================================================================
	* 反馈操作，完成后输出u1=（0xaa,...）,u2,u3=(0x0a,....)
	* ==========================================================================
	*/
	//添加反馈操作，放到前面减少指令
	state1 = _mm256_xor_si256(state1, *u1_x);
	state2 = _mm256_xor_si256(state2, *u1_y);

	SSstate1 = _mm256_xor_si256(SSstate1, u1_x_R4);
	SSstate2 = _mm256_xor_si256(SSstate2, u1_y_R4);




	state1_L4 = _mm256_and_si256(state1, con_L4);
	state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);

	state1_R4 = _mm256_and_si256(state1, con);
	state1_R4_L4 = _mm256_slli_epi16(state1_R4, 4);

	state2_L4 = _mm256_and_si256(state2, con_L4);
	state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);

	state2_R4 = _mm256_and_si256(state2, con);
	state2_R4_L4 = _mm256_slli_epi16(state2_R4, 4);




	*u1_x = _mm256_xor_si256(state1_R4, state1_R4_L4);//u1_x,u1_y=(0xaa,0xaa,...)
	*u1_y = _mm256_xor_si256(state2_R4, state2_R4_L4);

	*u2_x = state1_L4_R4;//u2_x,u2_y=(0x0a,0x0a,...)
	*u2_y = state2_L4_R4;

	*u3_x = SSstate1;
	*u3_y = SSstate2;








	return;
}




int CF_1024(const unsigned char* m, __m256i* u1_x, __m256i* u1_y, __m256i* u2_x, __m256i* u2_y, __m256i* u3_x, __m256i* u3_y, __m256i* u4_x, __m256i* u4_y, int round)
{
	//首先，将m1，m2,m3进行分割
	__m256i S_L4 = _mm256_setr_epi8(0x30, 0x00, 0x60, 0x20, 0x50, 0x40, 0xf0, 0xe0, 0xa0, 0x80, 0x70, 0x90, 0x10, 0xc0, 0xd0, 0xb0, 0x30, 0x00, 0x60, 0x20, 0x50, 0x40, 0xf0, 0xe0, 0xa0, 0x80, 0x70, 0x90, 0x10, 0xc0, 0xd0, 0xb0);
	__m256i S = _mm256_setr_epi8(0x3, 0x0, 0x6, 0x2, 0x5, 0x4, 0xf, 0xe, 0xa, 0x8, 0x7, 0x9, 0x1, 0xc, 0xd, 0xb, 0x3, 0x0, 0x6, 0x2, 0x5, 0x4, 0xf, 0xe, 0xa, 0x8, 0x7, 0x9, 0x1, 0xc, 0xd, 0xb);
	__m256i S_Inv = _mm256_setr_epi8(0xc, 0xa, 0xe, 0xd, 0x1, 0xf, 0xb, 0x0, 0x7, 0x2, 0x5, 0x4, 0x3, 0x6, 0x9, 0x8, 0xc, 0xa, 0xe, 0xd, 0x1, 0xf, 0xb, 0x0, 0x7, 0x2, 0x5, 0x4, 0x3, 0x6, 0x9, 0x8);

	__m256i con = _mm256_set1_epi8(0x0f);//取右4-bit
	__m256i con_L4 = _mm256_set1_epi8(0xf0);//取左4-bit
	__m256i A1 = _mm256_setr_epi8(1, 2, 3, 4, 5, 6, 7, 0, 9, 10, 11, 12, 13, 14, 15, 8, 1, 2, 3, 4, 5, 6, 7, 0, 9, 10, 11, 12, 13, 14, 15, 8);
	__m256i A2 = _mm256_setr_epi8(2, 3, 4, 5, 6, 7, 0, 1, 10, 11, 12, 13, 14, 15, 8, 9, 2, 3, 4, 5, 6, 7, 0, 1, 10, 11, 12, 13, 14, 15, 8, 9);
	__m256i A3 = _mm256_setr_epi8(5, 6, 7, 0, 1, 2, 3, 4, 13, 14, 15, 8, 9, 10, 11, 12, 5, 6, 7, 0, 1, 2, 3, 4, 13, 14, 15, 8, 9, 10, 11, 12);
	__m256i PL = _mm256_setr_epi8(12, 2, 0, 10, 14, 6, 13, 7, 11, 1, 8, 15, 9, 3, 5, 4, 3, 10, 4, 15, 14, 5, 7, 13, 12, 0, 11, 1, 9, 2, 6, 8);
	__m256i PR = _mm256_setr_epi8(0, 2, 1, 12, 11, 8, 13, 7, 10, 4, 6, 5, 3, 14, 9, 15, 6, 2, 11, 7, 8, 15, 4, 12, 13, 1, 0, 3, 9, 14, 10, 5);
	__m256i A4 = _mm256_setr_epi8(3, 4, 5, 6, 7, 0, 1, 2, 11, 12, 13, 14, 15, 8, 9, 10, 3, 4, 5, 6, 7, 0, 1, 2, 11, 12, 13, 14, 15, 8, 9, 10);

	__m256i c1 = _mm256_setr_epi8(0, 0x80, 1, 0x80, 2, 0x80, 3, 0x80, 4, 0x80, 5, 0x80, 6, 0x80, 7, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
	__m256i c2 = _mm256_setr_epi8(0x80, 0, 0x80, 1, 0x80, 2, 0x80, 3, 0x80, 4, 0x80, 5, 0x80, 6, 0x80, 7, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
	__m256i c3 = _mm256_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 8, 0x80, 9, 0x80, 10, 0x80, 11, 0x80, 12, 0x80, 13, 0x80, 14, 0x80, 15, 0x80);
	__m256i c4 = _mm256_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 8, 0x80, 9, 0x80, 10, 0x80, 11, 0x80, 12, 0x80, 13, 0x80, 14, 0x80, 15);


	__m256i c5 = _mm256_setr_epi8(0, 2, 4, 6, 8, 10, 12, 14, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
	__m256i c6 = _mm256_setr_epi8(1, 3, 5, 7, 9, 11, 13, 15, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
	__m256i c7 = _mm256_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0, 2, 4, 6, 8, 10, 12, 14, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
	__m256i c8 = _mm256_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 1, 3, 5, 7, 9, 11, 13, 15, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
	__m256i c55 = _mm256_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0, 2, 4, 6, 8, 10, 12, 14, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
	__m256i c66 = _mm256_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 1, 3, 5, 7, 9, 11, 13, 15, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
	__m256i c77 = _mm256_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0, 2, 4, 6, 8, 10, 12, 14);
	__m256i c88 = _mm256_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 1, 3, 5, 7, 9, 11, 13, 15);

	//G=[23, 3, 31, 2, 0, 20, 21, 15, 13, 29, 14, 30, 12, 28, 22, 1, 17, 24, 8, 5, 27, 16, 6, 11, 25, 18, 19, 10, 7, 9, 26, 4] (3cycle 8x32+ 1cycle two 16x4)
	//F = [11, 3, 14, 2, 9, 10, 12, 0, 4, 13, 7, 8, 15, 6, 5, 1, 16, 19, 27, 22, 31, 26, 20, 30, 23, 29, 25, 18, 24, 21, 28, 17](1 cycle two 16x4)

	__m256i F = _mm256_setr_epi8(11, 3, 14, 2, 9, 10, 12, 0, 4, 13, 7, 8, 15, 6, 5, 1, 0, 3, 11, 6, 15, 10, 4, 14, 7, 13, 9, 2, 8, 5, 12, 1);
	//__m256i P2 = _mm256_setr_epi8(8, 0, 6, 5, 3, 11, 7, 2, 14, 1, 15, 4, 10, 12, 13, 9, 13, 10, 1, 3, 12, 9, 14, 7, 6, 2, 15, 11, 8, 5, 4, 0);
	//G=G_Nibble + [2,4,5,6,0,1,3,7]
	__m256i G_Nibble = _mm256_setr_epi8(11, 3, 15, 2, 0, 8, 9, 7, 5, 13, 6, 14, 4, 12, 10, 1, 9, 12, 4, 1, 15, 8, 2, 7, 13, 10, 11, 6, 3, 5, 14, 0);
	__m256i G_Word = _mm256_setr_epi32(0, 3, 5, 7, 1, 2, 4, 6);


	int i;

	__m256i k, t1, t2, t3, t4, t5, t6, t7, t8;
	__m256i K0, K1, K0_L4, K1_L4, K2, K3, K2_L4, K3_L4, state_1_3, state_1_4, k_1;
	__m256i K4, K5, K4_L4, K5_L4, state_2_3, state_2_4, k_2;
	__m256i K6, K7, K6_L4, K7_L4, state_3_3, state_3_4, k_3;
	__m256i tG, k1, k3, tK1, tK2, tK3;
	__m256i KK0, KK1, KK2, KK3, KK4, KK5, KK6, KK7;

	__m256i state1, state2, state3, SSstate1, SSstate2, SSstate3, k_m;
	__m256i state1_1, state2_1, state3_1, k_m_1;
	__m256i state1_L4, state2_L4, state1_L4_R4, state2_L4_R4, state1_R4, state2_R4, k_L4, k_R4, k_L4_R4;
	__m256i SSstate1_L4, SSstate2_L4, SSstate1_L4_R4, SSstate2_L4_R4, SSstate1_R4, SSstate2_R4, SSstate1_R4_L4, SSstate2_R4_L4;
	__m256i Value1, state1_R4_L4, state2_R4_L4;
	__m256i Value2;
	__m256i u1_x_pre, u1_y_pre, u2_x_pre, u2_y_pre, u1_x_R4, u1_y_R4;
	unsigned char value1[32] = { 0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x54 };
	Value1 = _mm256_setr_epi8(0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x10);

	Value2 = _mm256_setr_epi8(0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x32);


	//K0 and K1<--(u2_x,u2_y)
	K0 = *u2_x;//K0=u2_x=(0x0a,0x0a,...)
	K1 = *u2_y;//K1=u2_y=(0x0a,0x0a,...)
	K0_L4 = _mm256_slli_epi16(K0, 4);
	KK0 = _mm256_xor_si256(K0, K0_L4);//KK0=(0xaa,0xaa,...)

	K1_L4 = _mm256_slli_epi16(K1, 4);
	KK1 = _mm256_xor_si256(K1, K1_L4);//KK1=(0xaa,0xaa,...)


	//K2 and K3<--(u3_x,u3_y)
	K2 = *u3_x;//K0=u2_x=(0x0a,0x0a,...)
	K3 = *u3_y;//K1=u2_y=(0x0a,0x0a,...)
	K2_L4 = _mm256_slli_epi16(K2, 4);
	KK2 = _mm256_xor_si256(K2, K2_L4);//KK0=(0xaa,0xaa,...)

	K3_L4 = _mm256_slli_epi16(K3, 4);
	KK3 = _mm256_xor_si256(K3, K3_L4);//KK1=(0xaa,0xaa,...)


	//K4 and K5<--(u3_x,u3_y)
	K4 = *u4_x;//K0=u2_x=(0x0a,0x0a,...)
	K5 = *u4_y;//K1=u2_y=(0x0a,0x0a,...)
	K4_L4 = _mm256_slli_epi16(K4, 4);
	KK4 = _mm256_xor_si256(K4, K4_L4);//KK0=(0xaa,0xaa,...)

	K5_L4 = _mm256_slli_epi16(K5, 4);
	KK5 = _mm256_xor_si256(K5, K5_L4);//KK1=(0xaa,0xaa,...)


	//m2_x(K6)/m2_y(K7)
	//m2_x=(0x0a,0x0a,...)
	//m2_y=(0x0a,0x0a,...)
	K7 = _mm256_loadu_si256((__m256i*)(m));
	K6 = _mm256_srli_epi16(K7, 4);
	K6 = _mm256_and_si256(K6, con);
	K7 = _mm256_and_si256(K7, con);

	state_3_3 = _mm256_permute2x128_si256(K6, K6, 1);
	state_3_4 = _mm256_permute2x128_si256(K7, K7, 1);
	t1 = _mm256_shuffle_epi8(K6, c1);
	t2 = _mm256_shuffle_epi8(state_3_3, c3);
	t3 = _mm256_shuffle_epi8(K7, c2);
	t4 = _mm256_shuffle_epi8(state_3_4, c4);
	t5 = _mm256_shuffle_epi8(state_3_3, c1);
	t6 = _mm256_shuffle_epi8(K6, c3);
	t7 = _mm256_shuffle_epi8(state_3_4, c2);
	t8 = _mm256_shuffle_epi8(K7, c4);

	K6 = _mm256_xor_si256(t1, t2);
	state_3_3 = _mm256_xor_si256(t3, t4);
	K6 = _mm256_xor_si256(K6, state_3_3);

	K7 = _mm256_xor_si256(t5, t6);
	state_3_4 = _mm256_xor_si256(t7, t8);
	K7 = _mm256_xor_si256(K7, state_3_4);

	K6_L4 = _mm256_slli_epi16(K6, 4);
	KK6 = _mm256_xor_si256(K6, K6_L4);//KK6=(0xaa,0xaa,...)

	K7_L4 = _mm256_slli_epi16(K7, 4);
	KK7 = _mm256_xor_si256(K7, K7_L4);//KK7=(0xaa,0xaa,...)






	//取u1_x和u1_y的R4位
	u1_x_R4 = _mm256_and_si256(*u1_x, con);
	u1_y_R4 = _mm256_and_si256(*u1_y, con);
	//明文部分的赋值

	//第一个大块和第二个大块的加密输入,第1个分支在右4位，第2个分支左4位
	state1 = *u1_x;//u1_x=(0xaa,0xaa,....)
	state2 = *u1_y;//u1_y=(0xaa,0xaa,....)

	//u1和u2的值还得再处理一下
	//state1,state2=0x(aa,0xaa,...)
	//u1和u2是按照(0xaa,0xaa,0xaa)来存储的，不是按照(0x0a,0x0a,0x0a,)来存储的
	state2 = _mm256_xor_si256(state2, Value1);//区分两个块不同的明文输入


	//第3个大块和第4个大块的加密输入,第3个分支在右4位，第4个分支左4位
	SSstate1 = *u1_x;//u1_x=(0xaa,0xaa,....)
	SSstate2 = *u1_y;//u1_y=(0xaa,0xaa,....)

	//u1和u2的值还得再处理一下
	//state1,state2=0x(aa,0xaa,...)
	//u1和u2是按照(0xaa,0xaa,0xaa)来存储的，不是按照(0x0a,0x0a,0x0a,)来存储的
	SSstate2 = _mm256_xor_si256(SSstate2, Value2);//区分两个块不同的明文输入





	//第1个step不需要更新轮密钥
	//The first round B1 and B2
	state1 = _mm256_xor_si256(state1, KK0);
	state2 = _mm256_xor_si256(state2, KK1);
	state1_L4 = _mm256_and_si256(state1, con_L4);
	state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);
	state1_R4 = _mm256_and_si256(state1, con);
	state1_L4 = _mm256_shuffle_epi8(S_L4, state1_L4_R4);
	state1_R4 = _mm256_shuffle_epi8(S, state1_R4);
	state1 = _mm256_xor_si256(state1_L4, state1_R4);

	state2_L4 = _mm256_and_si256(state2, con_L4);
	state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);
	state2_R4 = _mm256_and_si256(state2, con);
	state2_L4 = _mm256_shuffle_epi8(S_L4, state2_L4_R4);
	state2_R4 = _mm256_shuffle_epi8(S, state2_R4);
	state2 = _mm256_xor_si256(state2_L4, state2_R4);

	//state2 = _mm256_shuffle_epi8(S, state2);

	state2 = _mm256_xor_si256(state2, state1);

	k_m = _mm256_shuffle_epi8(state2, A1);
	state1 = _mm256_xor_si256(state1, k_m);

	k_m = _mm256_shuffle_epi8(state1, A2);
	state2 = _mm256_xor_si256(state2, k_m);

	k_m = _mm256_shuffle_epi8(state2, A2);
	state1 = _mm256_xor_si256(state1, k_m);

	k_m = _mm256_shuffle_epi8(state1, A3);
	state2 = _mm256_xor_si256(state2, k_m);

	state1 = _mm256_xor_si256(state1, state2);

	/*
	The difference with uBlock round function
	*/
	state3 = _mm256_permute2x128_si256(state1, state1, 1);//Left and right change
	state1 = _mm256_shuffle_epi8(state3, PL);//Byte permutation within 128-bit

	state2 = _mm256_shuffle_epi8(state2, PR);//Byte permutation within 128-bit



	//B3 and B4
	SSstate1 = _mm256_xor_si256(SSstate1, KK0);
	SSstate2 = _mm256_xor_si256(SSstate2, KK1);
	SSstate1_L4 = _mm256_and_si256(SSstate1, con_L4);
	SSstate1_L4_R4 = _mm256_srli_epi16(SSstate1_L4, 4);
	SSstate1_R4 = _mm256_and_si256(SSstate1, con);
	SSstate1_L4 = _mm256_shuffle_epi8(S_L4, SSstate1_L4_R4);
	SSstate1_R4 = _mm256_shuffle_epi8(S, SSstate1_R4);
	SSstate1 = _mm256_xor_si256(SSstate1_L4, SSstate1_R4);

	SSstate2_L4 = _mm256_and_si256(SSstate2, con_L4);
	SSstate2_L4_R4 = _mm256_srli_epi16(SSstate2_L4, 4);
	SSstate2_R4 = _mm256_and_si256(SSstate2, con);
	SSstate2_L4 = _mm256_shuffle_epi8(S_L4, SSstate2_L4_R4);
	SSstate2_R4 = _mm256_shuffle_epi8(S, SSstate2_R4);
	SSstate2 = _mm256_xor_si256(SSstate2_L4, SSstate2_R4);

	//state2 = _mm256_shuffle_epi8(S, state2);

	SSstate2 = _mm256_xor_si256(SSstate2, SSstate1);

	k_m = _mm256_shuffle_epi8(SSstate2, A1);
	SSstate1 = _mm256_xor_si256(SSstate1, k_m);

	k_m = _mm256_shuffle_epi8(SSstate1, A2);
	SSstate2 = _mm256_xor_si256(SSstate2, k_m);

	k_m = _mm256_shuffle_epi8(SSstate2, A2);
	SSstate1 = _mm256_xor_si256(SSstate1, k_m);

	k_m = _mm256_shuffle_epi8(SSstate1, A3);
	SSstate2 = _mm256_xor_si256(SSstate2, k_m);

	SSstate1 = _mm256_xor_si256(SSstate1, SSstate2);

	/*
	The difference with uBlock round function
	*/
	SSstate3 = _mm256_permute2x128_si256(SSstate1, SSstate1, 1);//Left and right change
	SSstate1 = _mm256_shuffle_epi8(SSstate3, PL);//Byte permutation within 128-bit

	SSstate2 = _mm256_shuffle_epi8(SSstate2, PR);//Byte permutation within 128-bit




	//The second round
	state1 = _mm256_xor_si256(state1, KK2);
	state2 = _mm256_xor_si256(state2, KK3);
	state1_L4 = _mm256_and_si256(state1, con_L4);
	state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);
	state1_R4 = _mm256_and_si256(state1, con);
	state1_L4 = _mm256_shuffle_epi8(S_L4, state1_L4_R4);
	state1_R4 = _mm256_shuffle_epi8(S, state1_R4);
	state1 = _mm256_xor_si256(state1_L4, state1_R4);

	state2_L4 = _mm256_and_si256(state2, con_L4);
	state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);
	state2_R4 = _mm256_and_si256(state2, con);
	state2_L4 = _mm256_shuffle_epi8(S_L4, state2_L4_R4);
	state2_R4 = _mm256_shuffle_epi8(S, state2_R4);
	state2 = _mm256_xor_si256(state2_L4, state2_R4);

	//state2 = _mm256_shuffle_epi8(S, state2);

	state2 = _mm256_xor_si256(state2, state1);

	k_m = _mm256_shuffle_epi8(state2, A1);
	state1 = _mm256_xor_si256(state1, k_m);

	k_m = _mm256_shuffle_epi8(state1, A2);
	state2 = _mm256_xor_si256(state2, k_m);

	k_m = _mm256_shuffle_epi8(state2, A2);
	state1 = _mm256_xor_si256(state1, k_m);

	k_m = _mm256_shuffle_epi8(state1, A3);
	state2 = _mm256_xor_si256(state2, k_m);

	state1 = _mm256_xor_si256(state1, state2);

	/*
	The difference with uBlock round function
	*/
	state3 = _mm256_permute2x128_si256(state1, state1, 1);//Left and right change
	state1 = _mm256_shuffle_epi8(state3, PL);//Byte permutation within 128-bit

	state2 = _mm256_shuffle_epi8(state2, PR);//Byte permutation within 128-bit


	//B3 and B4
	SSstate1 = _mm256_xor_si256(SSstate1, KK2);
	SSstate2 = _mm256_xor_si256(SSstate2, KK3);
	SSstate1_L4 = _mm256_and_si256(SSstate1, con_L4);
	SSstate1_L4_R4 = _mm256_srli_epi16(SSstate1_L4, 4);
	SSstate1_R4 = _mm256_and_si256(SSstate1, con);
	SSstate1_L4 = _mm256_shuffle_epi8(S_L4, SSstate1_L4_R4);
	SSstate1_R4 = _mm256_shuffle_epi8(S, SSstate1_R4);
	SSstate1 = _mm256_xor_si256(SSstate1_L4, SSstate1_R4);

	SSstate2_L4 = _mm256_and_si256(SSstate2, con_L4);
	SSstate2_L4_R4 = _mm256_srli_epi16(SSstate2_L4, 4);
	SSstate2_R4 = _mm256_and_si256(SSstate2, con);
	SSstate2_L4 = _mm256_shuffle_epi8(S_L4, SSstate2_L4_R4);
	SSstate2_R4 = _mm256_shuffle_epi8(S, SSstate2_R4);
	SSstate2 = _mm256_xor_si256(SSstate2_L4, SSstate2_R4);

	//state2 = _mm256_shuffle_epi8(S, state2);

	SSstate2 = _mm256_xor_si256(SSstate2, SSstate1);

	k_m = _mm256_shuffle_epi8(SSstate2, A1);
	SSstate1 = _mm256_xor_si256(SSstate1, k_m);

	k_m = _mm256_shuffle_epi8(SSstate1, A2);
	SSstate2 = _mm256_xor_si256(SSstate2, k_m);

	k_m = _mm256_shuffle_epi8(SSstate2, A2);
	SSstate1 = _mm256_xor_si256(SSstate1, k_m);

	k_m = _mm256_shuffle_epi8(SSstate1, A3);
	SSstate2 = _mm256_xor_si256(SSstate2, k_m);

	SSstate1 = _mm256_xor_si256(SSstate1, SSstate2);

	/*
	The difference with uBlock round function
	*/
	SSstate3 = _mm256_permute2x128_si256(SSstate1, SSstate1, 1);//Left and right change
	SSstate1 = _mm256_shuffle_epi8(SSstate3, PL);//Byte permutation within 128-bit

	SSstate2 = _mm256_shuffle_epi8(SSstate2, PR);//Byte permutation within 128-bit


	//The third round
	state1 = _mm256_xor_si256(state1, KK4);
	state2 = _mm256_xor_si256(state2, KK5);
	state1_L4 = _mm256_and_si256(state1, con_L4);
	state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);
	state1_R4 = _mm256_and_si256(state1, con);
	state1_L4 = _mm256_shuffle_epi8(S_L4, state1_L4_R4);
	state1_R4 = _mm256_shuffle_epi8(S, state1_R4);
	state1 = _mm256_xor_si256(state1_L4, state1_R4);

	state2_L4 = _mm256_and_si256(state2, con_L4);
	state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);
	state2_R4 = _mm256_and_si256(state2, con);
	state2_L4 = _mm256_shuffle_epi8(S_L4, state2_L4_R4);
	state2_R4 = _mm256_shuffle_epi8(S, state2_R4);
	state2 = _mm256_xor_si256(state2_L4, state2_R4);

	//state2 = _mm256_shuffle_epi8(S, state2);

	state2 = _mm256_xor_si256(state2, state1);

	k_m = _mm256_shuffle_epi8(state2, A1);
	state1 = _mm256_xor_si256(state1, k_m);

	k_m = _mm256_shuffle_epi8(state1, A2);
	state2 = _mm256_xor_si256(state2, k_m);

	k_m = _mm256_shuffle_epi8(state2, A2);
	state1 = _mm256_xor_si256(state1, k_m);

	k_m = _mm256_shuffle_epi8(state1, A3);
	state2 = _mm256_xor_si256(state2, k_m);

	state1 = _mm256_xor_si256(state1, state2);

	/*
	The difference with uBlock round function
	*/
	state3 = _mm256_permute2x128_si256(state1, state1, 1);//Left and right change
	state1 = _mm256_shuffle_epi8(state3, PL);//Byte permutation within 128-bit

	state2 = _mm256_shuffle_epi8(state2, PR);//Byte permutation within 128-bit


	//B3 and B4
	SSstate1 = _mm256_xor_si256(SSstate1, KK4);
	SSstate2 = _mm256_xor_si256(SSstate2, KK5);
	SSstate1_L4 = _mm256_and_si256(SSstate1, con_L4);
	SSstate1_L4_R4 = _mm256_srli_epi16(SSstate1_L4, 4);
	SSstate1_R4 = _mm256_and_si256(SSstate1, con);
	SSstate1_L4 = _mm256_shuffle_epi8(S_L4, SSstate1_L4_R4);
	SSstate1_R4 = _mm256_shuffle_epi8(S, SSstate1_R4);
	SSstate1 = _mm256_xor_si256(SSstate1_L4, SSstate1_R4);

	SSstate2_L4 = _mm256_and_si256(SSstate2, con_L4);
	SSstate2_L4_R4 = _mm256_srli_epi16(SSstate2_L4, 4);
	SSstate2_R4 = _mm256_and_si256(SSstate2, con);
	SSstate2_L4 = _mm256_shuffle_epi8(S_L4, SSstate2_L4_R4);
	SSstate2_R4 = _mm256_shuffle_epi8(S, SSstate2_R4);
	SSstate2 = _mm256_xor_si256(SSstate2_L4, SSstate2_R4);

	//state2 = _mm256_shuffle_epi8(S, state2);

	SSstate2 = _mm256_xor_si256(SSstate2, SSstate1);

	k_m = _mm256_shuffle_epi8(SSstate2, A1);
	SSstate1 = _mm256_xor_si256(SSstate1, k_m);

	k_m = _mm256_shuffle_epi8(SSstate1, A2);
	SSstate2 = _mm256_xor_si256(SSstate2, k_m);

	k_m = _mm256_shuffle_epi8(SSstate2, A2);
	SSstate1 = _mm256_xor_si256(SSstate1, k_m);

	k_m = _mm256_shuffle_epi8(SSstate1, A3);
	SSstate2 = _mm256_xor_si256(SSstate2, k_m);

	SSstate1 = _mm256_xor_si256(SSstate1, SSstate2);

	/*
	The difference with uBlock round function
	*/
	SSstate3 = _mm256_permute2x128_si256(SSstate1, SSstate1, 1);//Left and right change
	SSstate1 = _mm256_shuffle_epi8(SSstate3, PL);//Byte permutation within 128-bit

	SSstate2 = _mm256_shuffle_epi8(SSstate2, PR);//Byte permutation within 128-bit



	//The fourth round
	state1 = _mm256_xor_si256(state1, KK6);
	state2 = _mm256_xor_si256(state2, KK7);
	state1_L4 = _mm256_and_si256(state1, con_L4);
	state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);
	state1_R4 = _mm256_and_si256(state1, con);
	state1_L4 = _mm256_shuffle_epi8(S_L4, state1_L4_R4);
	state1_R4 = _mm256_shuffle_epi8(S, state1_R4);
	state1 = _mm256_xor_si256(state1_L4, state1_R4);

	state2_L4 = _mm256_and_si256(state2, con_L4);
	state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);
	state2_R4 = _mm256_and_si256(state2, con);
	state2_L4 = _mm256_shuffle_epi8(S_L4, state2_L4_R4);
	state2_R4 = _mm256_shuffle_epi8(S, state2_R4);
	state2 = _mm256_xor_si256(state2_L4, state2_R4);

	//state2 = _mm256_shuffle_epi8(S, state2);

	state2 = _mm256_xor_si256(state2, state1);

	k_m = _mm256_shuffle_epi8(state2, A1);
	state1 = _mm256_xor_si256(state1, k_m);

	k_m = _mm256_shuffle_epi8(state1, A2);
	state2 = _mm256_xor_si256(state2, k_m);

	k_m = _mm256_shuffle_epi8(state2, A2);
	state1 = _mm256_xor_si256(state1, k_m);

	k_m = _mm256_shuffle_epi8(state1, A3);
	state2 = _mm256_xor_si256(state2, k_m);

	state1 = _mm256_xor_si256(state1, state2);

	/*
	The difference with uBlock round function
	*/
	state3 = _mm256_permute2x128_si256(state1, state1, 1);//Left and right change
	state1 = _mm256_shuffle_epi8(state3, PL);//Byte permutation within 128-bit

	state2 = _mm256_shuffle_epi8(state2, PR);//Byte permutation within 128-bit


	//B3 and B4
	SSstate1 = _mm256_xor_si256(SSstate1, KK6);
	SSstate2 = _mm256_xor_si256(SSstate2, KK7);
	SSstate1_L4 = _mm256_and_si256(SSstate1, con_L4);
	SSstate1_L4_R4 = _mm256_srli_epi16(SSstate1_L4, 4);
	SSstate1_R4 = _mm256_and_si256(SSstate1, con);
	SSstate1_L4 = _mm256_shuffle_epi8(S_L4, SSstate1_L4_R4);
	SSstate1_R4 = _mm256_shuffle_epi8(S, SSstate1_R4);
	SSstate1 = _mm256_xor_si256(SSstate1_L4, SSstate1_R4);

	SSstate2_L4 = _mm256_and_si256(SSstate2, con_L4);
	SSstate2_L4_R4 = _mm256_srli_epi16(SSstate2_L4, 4);
	SSstate2_R4 = _mm256_and_si256(SSstate2, con);
	SSstate2_L4 = _mm256_shuffle_epi8(S_L4, SSstate2_L4_R4);
	SSstate2_R4 = _mm256_shuffle_epi8(S, SSstate2_R4);
	SSstate2 = _mm256_xor_si256(SSstate2_L4, SSstate2_R4);

	//state2 = _mm256_shuffle_epi8(S, state2);

	SSstate2 = _mm256_xor_si256(SSstate2, SSstate1);

	k_m = _mm256_shuffle_epi8(SSstate2, A1);
	SSstate1 = _mm256_xor_si256(SSstate1, k_m);

	k_m = _mm256_shuffle_epi8(SSstate1, A2);
	SSstate2 = _mm256_xor_si256(SSstate2, k_m);

	k_m = _mm256_shuffle_epi8(SSstate2, A2);
	SSstate1 = _mm256_xor_si256(SSstate1, k_m);

	k_m = _mm256_shuffle_epi8(SSstate1, A3);
	SSstate2 = _mm256_xor_si256(SSstate2, k_m);

	SSstate1 = _mm256_xor_si256(SSstate1, SSstate2);

	/*
	The difference with uBlock round function
	*/
	SSstate3 = _mm256_permute2x128_si256(SSstate1, SSstate1, 1);//Left and right change
	SSstate1 = _mm256_shuffle_epi8(SSstate3, PL);//Byte permutation within 128-bit

	SSstate2 = _mm256_shuffle_epi8(SSstate2, PR);//Byte permutation within 128-bit





	//这里的round其实代表的是step

	for (i = 1; i < round; i++)
	{
		//The first round
		tK1 = K0;
		tK2 = K1;
		__m256i RoundConst = _mm256_setr_epi8(rrc[i - 1][0], rrc[i - 1][1], rrc[i - 1][2], rrc[i - 1][3], rrc[i - 1][4], rrc[i - 1][5], rrc[i - 1][6], rrc[i - 1][7], rrc[i - 1][8], rrc[i - 1][9], rrc[i - 1][10], rrc[i - 1][11], rrc[i - 1][12], rrc[i - 1][13], rrc[i - 1][14], rrc[i - 1][15], rrc[i - 1][16], rrc[i - 1][17], rrc[i - 1][18], rrc[i - 1][19], rrc[i - 1][20], rrc[i - 1][21], rrc[i - 1][22], rrc[i - 1][23], rrc[i - 1][24], rrc[i - 1][25], rrc[i - 1][26], rrc[i - 1][27], rrc[i - 1][28], rrc[i - 1][29], rrc[i - 1][30], rrc[i - 1][31]);

		//__m256i RoundConst = _mm256_setr_epi8(rc[i - 1][0], rc[i - 1][1], rc[i - 1][2], rc[i - 1][3], rc[i - 1][4], rc[i - 1][5], rc[i - 1][6], rc[i - 1][7], rc[i - 1][8], rc[i - 1][9], rc[i - 1][10], rc[i - 1][11], rc[i - 1][12], rc[i - 1][13], rc[i - 1][14], rc[i - 1][15], rc[i - 1][16], rc[i - 1][17], rc[i - 1][18], rc[i - 1][19], rc[i - 1][20], rc[i - 1][21], rc[i - 1][22], rc[i - 1][23], rc[i - 1][24], rc[i - 1][25], rc[i - 1][26], rc[i - 1][27], rc[i - 1][28], rc[i - 1][29], rc[i - 1][30], rc[i - 1][31]);
		//_mm256_setr_epi32(RC[i - 1][0], RC[i - 1][1], RC[i - 1][2], RC[i - 1][3], RC[i - 1][4], RC[i - 1][5], RC[i - 1][6], RC[i - 1][7]);



		//@@@@@@Bug is in here@@@@@@@
		K0 = _mm256_xor_si256(K2, RoundConst);
		K0 = _mm256_shuffle_epi8(K0, F);//the output after function P2
		//K0 = _mm256_shuffle_epi8(K2, F);//the output after function P2
		tG = _mm256_permutevar8x32_epi32(K5, G_Word);
		K1 = _mm256_shuffle_epi8(tG, G_Nibble);//the output after function G
		//得到K0，K1=（0x0a,0x0a,...）后生成KK0,KK1=(0xaa,0xaa,...)
		K0_L4 = _mm256_slli_epi16(K0, 4);
		KK0 = _mm256_xor_si256(K0, K0_L4);//KK0=(0xaa,0xaa,...)

		K1_L4 = _mm256_slli_epi16(K1, 4);
		KK1 = _mm256_xor_si256(K1, K1_L4);//KK1=(0xaa,0xaa,...)







		state1 = _mm256_xor_si256(state1, KK0);
		state2 = _mm256_xor_si256(state2, KK1);
		state1_L4 = _mm256_and_si256(state1, con_L4);
		state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);
		state1_R4 = _mm256_and_si256(state1, con);
		state1_L4 = _mm256_shuffle_epi8(S_L4, state1_L4_R4);
		state1_R4 = _mm256_shuffle_epi8(S, state1_R4);
		state1 = _mm256_xor_si256(state1_L4, state1_R4);

		state2_L4 = _mm256_and_si256(state2, con_L4);
		state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);
		state2_R4 = _mm256_and_si256(state2, con);
		state2_L4 = _mm256_shuffle_epi8(S_L4, state2_L4_R4);
		state2_R4 = _mm256_shuffle_epi8(S, state2_R4);
		state2 = _mm256_xor_si256(state2_L4, state2_R4);

		//state2 = _mm256_shuffle_epi8(S, state2);

		state2 = _mm256_xor_si256(state2, state1);

		k_m = _mm256_shuffle_epi8(state2, A1);
		state1 = _mm256_xor_si256(state1, k_m);

		k_m = _mm256_shuffle_epi8(state1, A2);
		state2 = _mm256_xor_si256(state2, k_m);

		k_m = _mm256_shuffle_epi8(state2, A2);
		state1 = _mm256_xor_si256(state1, k_m);

		k_m = _mm256_shuffle_epi8(state1, A3);
		state2 = _mm256_xor_si256(state2, k_m);

		state1 = _mm256_xor_si256(state1, state2);

		/*
		The difference with uBlock round function
		*/
		state3 = _mm256_permute2x128_si256(state1, state1, 1);//Left and right change
		state1 = _mm256_shuffle_epi8(state3, PL);//Byte permutation within 128-bit

		state2 = _mm256_shuffle_epi8(state2, PR);//Byte permutation within 128-bit



		//B3 and B4
		SSstate1 = _mm256_xor_si256(SSstate1, KK0);
		SSstate2 = _mm256_xor_si256(SSstate2, KK1);
		SSstate1_L4 = _mm256_and_si256(SSstate1, con_L4);
		SSstate1_L4_R4 = _mm256_srli_epi16(SSstate1_L4, 4);
		SSstate1_R4 = _mm256_and_si256(SSstate1, con);
		SSstate1_L4 = _mm256_shuffle_epi8(S_L4, SSstate1_L4_R4);
		SSstate1_R4 = _mm256_shuffle_epi8(S, SSstate1_R4);
		SSstate1 = _mm256_xor_si256(SSstate1_L4, SSstate1_R4);

		SSstate2_L4 = _mm256_and_si256(SSstate2, con_L4);
		SSstate2_L4_R4 = _mm256_srli_epi16(SSstate2_L4, 4);
		SSstate2_R4 = _mm256_and_si256(SSstate2, con);
		SSstate2_L4 = _mm256_shuffle_epi8(S_L4, SSstate2_L4_R4);
		SSstate2_R4 = _mm256_shuffle_epi8(S, SSstate2_R4);
		SSstate2 = _mm256_xor_si256(SSstate2_L4, SSstate2_R4);

		//state2 = _mm256_shuffle_epi8(S, state2);

		SSstate2 = _mm256_xor_si256(SSstate2, SSstate1);

		k_m = _mm256_shuffle_epi8(SSstate2, A1);
		SSstate1 = _mm256_xor_si256(SSstate1, k_m);

		k_m = _mm256_shuffle_epi8(SSstate1, A2);
		SSstate2 = _mm256_xor_si256(SSstate2, k_m);

		k_m = _mm256_shuffle_epi8(SSstate2, A2);
		SSstate1 = _mm256_xor_si256(SSstate1, k_m);

		k_m = _mm256_shuffle_epi8(SSstate1, A3);
		SSstate2 = _mm256_xor_si256(SSstate2, k_m);

		SSstate1 = _mm256_xor_si256(SSstate1, SSstate2);

		/*
		The difference with uBlock round function
		*/
		SSstate3 = _mm256_permute2x128_si256(SSstate1, SSstate1, 1);//Left and right change
		SSstate1 = _mm256_shuffle_epi8(SSstate3, PL);//Byte permutation within 128-bit

		SSstate2 = _mm256_shuffle_epi8(SSstate2, PR);//Byte permutation within 128-bit





		//The second round
		K2 = _mm256_shuffle_epi8(tK2, F);//the output after function P2
		tK2 = K3;
		tG = _mm256_permutevar8x32_epi32(tK1, G_Word);
		K3 = _mm256_shuffle_epi8(tG, G_Nibble);//the output after function G
		//得到K0，K1=（0x0a,0x0a,...）后生成KK0,KK1=(0xaa,0xaa,...)
		K2_L4 = _mm256_slli_epi16(K2, 4);
		KK2 = _mm256_xor_si256(K2, K2_L4);//KK0=(0xaa,0xaa,...)

		K3_L4 = _mm256_slli_epi16(K3, 4);
		KK3 = _mm256_xor_si256(K3, K3_L4);//KK1=(0xaa,0xaa,...)




		state1 = _mm256_xor_si256(state1, KK2);
		state2 = _mm256_xor_si256(state2, KK3);
		state1_L4 = _mm256_and_si256(state1, con_L4);
		state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);
		state1_R4 = _mm256_and_si256(state1, con);
		state1_L4 = _mm256_shuffle_epi8(S_L4, state1_L4_R4);
		state1_R4 = _mm256_shuffle_epi8(S, state1_R4);
		state1 = _mm256_xor_si256(state1_L4, state1_R4);

		state2_L4 = _mm256_and_si256(state2, con_L4);
		state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);
		state2_R4 = _mm256_and_si256(state2, con);
		state2_L4 = _mm256_shuffle_epi8(S_L4, state2_L4_R4);
		state2_R4 = _mm256_shuffle_epi8(S, state2_R4);
		state2 = _mm256_xor_si256(state2_L4, state2_R4);

		//state2 = _mm256_shuffle_epi8(S, state2);

		state2 = _mm256_xor_si256(state2, state1);

		k_m = _mm256_shuffle_epi8(state2, A1);
		state1 = _mm256_xor_si256(state1, k_m);

		k_m = _mm256_shuffle_epi8(state1, A2);
		state2 = _mm256_xor_si256(state2, k_m);

		k_m = _mm256_shuffle_epi8(state2, A2);
		state1 = _mm256_xor_si256(state1, k_m);

		k_m = _mm256_shuffle_epi8(state1, A3);
		state2 = _mm256_xor_si256(state2, k_m);

		state1 = _mm256_xor_si256(state1, state2);

		/*
		The difference with uBlock round function
		*/
		state3 = _mm256_permute2x128_si256(state1, state1, 1);//Left and right change
		state1 = _mm256_shuffle_epi8(state3, PL);//Byte permutation within 128-bit

		state2 = _mm256_shuffle_epi8(state2, PR);//Byte permutation within 128-bit


		//B3 and B4
		SSstate1 = _mm256_xor_si256(SSstate1, KK2);
		SSstate2 = _mm256_xor_si256(SSstate2, KK3);
		SSstate1_L4 = _mm256_and_si256(SSstate1, con_L4);
		SSstate1_L4_R4 = _mm256_srli_epi16(SSstate1_L4, 4);
		SSstate1_R4 = _mm256_and_si256(SSstate1, con);
		SSstate1_L4 = _mm256_shuffle_epi8(S_L4, SSstate1_L4_R4);
		SSstate1_R4 = _mm256_shuffle_epi8(S, SSstate1_R4);
		SSstate1 = _mm256_xor_si256(SSstate1_L4, SSstate1_R4);

		SSstate2_L4 = _mm256_and_si256(SSstate2, con_L4);
		SSstate2_L4_R4 = _mm256_srli_epi16(SSstate2_L4, 4);
		SSstate2_R4 = _mm256_and_si256(SSstate2, con);
		SSstate2_L4 = _mm256_shuffle_epi8(S_L4, SSstate2_L4_R4);
		SSstate2_R4 = _mm256_shuffle_epi8(S, SSstate2_R4);
		SSstate2 = _mm256_xor_si256(SSstate2_L4, SSstate2_R4);

		//state2 = _mm256_shuffle_epi8(S, state2);

		SSstate2 = _mm256_xor_si256(SSstate2, SSstate1);

		k_m = _mm256_shuffle_epi8(SSstate2, A1);
		SSstate1 = _mm256_xor_si256(SSstate1, k_m);

		k_m = _mm256_shuffle_epi8(SSstate1, A2);
		SSstate2 = _mm256_xor_si256(SSstate2, k_m);

		k_m = _mm256_shuffle_epi8(SSstate2, A2);
		SSstate1 = _mm256_xor_si256(SSstate1, k_m);

		k_m = _mm256_shuffle_epi8(SSstate1, A3);
		SSstate2 = _mm256_xor_si256(SSstate2, k_m);

		SSstate1 = _mm256_xor_si256(SSstate1, SSstate2);

		/*
		The difference with uBlock round function
		*/
		SSstate3 = _mm256_permute2x128_si256(SSstate1, SSstate1, 1);//Left and right change
		SSstate1 = _mm256_shuffle_epi8(SSstate3, PL);//Byte permutation within 128-bit

		SSstate2 = _mm256_shuffle_epi8(SSstate2, PR);//Byte permutation within 128-bit



		//The third round
		tK1 = K4;
		K4 = _mm256_shuffle_epi8(tK2, F);//the output after function P2

		tG = _mm256_permutevar8x32_epi32(K6, G_Word);
		K5 = _mm256_shuffle_epi8(tG, G_Nibble);//the output after function G
		//得到K0，K1=（0x0a,0x0a,...）后生成KK0,KK1=(0xaa,0xaa,...)
		K4_L4 = _mm256_slli_epi16(K4, 4);
		KK4 = _mm256_xor_si256(K4, K4_L4);//KK0=(0xaa,0xaa,...)

		K5_L4 = _mm256_slli_epi16(K5, 4);
		KK5 = _mm256_xor_si256(K5, K5_L4);//KK1=(0xaa,0xaa,...)




		state1 = _mm256_xor_si256(state1, KK4);
		state2 = _mm256_xor_si256(state2, KK5);
		state1_L4 = _mm256_and_si256(state1, con_L4);
		state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);
		state1_R4 = _mm256_and_si256(state1, con);
		state1_L4 = _mm256_shuffle_epi8(S_L4, state1_L4_R4);
		state1_R4 = _mm256_shuffle_epi8(S, state1_R4);
		state1 = _mm256_xor_si256(state1_L4, state1_R4);

		state2_L4 = _mm256_and_si256(state2, con_L4);
		state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);
		state2_R4 = _mm256_and_si256(state2, con);
		state2_L4 = _mm256_shuffle_epi8(S_L4, state2_L4_R4);
		state2_R4 = _mm256_shuffle_epi8(S, state2_R4);
		state2 = _mm256_xor_si256(state2_L4, state2_R4);

		//state2 = _mm256_shuffle_epi8(S, state2);

		state2 = _mm256_xor_si256(state2, state1);

		k_m = _mm256_shuffle_epi8(state2, A1);
		state1 = _mm256_xor_si256(state1, k_m);

		k_m = _mm256_shuffle_epi8(state1, A2);
		state2 = _mm256_xor_si256(state2, k_m);

		k_m = _mm256_shuffle_epi8(state2, A2);
		state1 = _mm256_xor_si256(state1, k_m);

		k_m = _mm256_shuffle_epi8(state1, A3);
		state2 = _mm256_xor_si256(state2, k_m);

		state1 = _mm256_xor_si256(state1, state2);

		/*
		The difference with uBlock round function
		*/
		state3 = _mm256_permute2x128_si256(state1, state1, 1);//Left and right change
		state1 = _mm256_shuffle_epi8(state3, PL);//Byte permutation within 128-bit

		state2 = _mm256_shuffle_epi8(state2, PR);//Byte permutation within 128-bit


		//B3 and B4
		SSstate1 = _mm256_xor_si256(SSstate1, KK4);
		SSstate2 = _mm256_xor_si256(SSstate2, KK5);
		SSstate1_L4 = _mm256_and_si256(SSstate1, con_L4);
		SSstate1_L4_R4 = _mm256_srli_epi16(SSstate1_L4, 4);
		SSstate1_R4 = _mm256_and_si256(SSstate1, con);
		SSstate1_L4 = _mm256_shuffle_epi8(S_L4, SSstate1_L4_R4);
		SSstate1_R4 = _mm256_shuffle_epi8(S, SSstate1_R4);
		SSstate1 = _mm256_xor_si256(SSstate1_L4, SSstate1_R4);

		SSstate2_L4 = _mm256_and_si256(SSstate2, con_L4);
		SSstate2_L4_R4 = _mm256_srli_epi16(SSstate2_L4, 4);
		SSstate2_R4 = _mm256_and_si256(SSstate2, con);
		SSstate2_L4 = _mm256_shuffle_epi8(S_L4, SSstate2_L4_R4);
		SSstate2_R4 = _mm256_shuffle_epi8(S, SSstate2_R4);
		SSstate2 = _mm256_xor_si256(SSstate2_L4, SSstate2_R4);

		//state2 = _mm256_shuffle_epi8(S, state2);

		SSstate2 = _mm256_xor_si256(SSstate2, SSstate1);

		k_m = _mm256_shuffle_epi8(SSstate2, A1);
		SSstate1 = _mm256_xor_si256(SSstate1, k_m);

		k_m = _mm256_shuffle_epi8(SSstate1, A2);
		SSstate2 = _mm256_xor_si256(SSstate2, k_m);

		k_m = _mm256_shuffle_epi8(SSstate2, A2);
		SSstate1 = _mm256_xor_si256(SSstate1, k_m);

		k_m = _mm256_shuffle_epi8(SSstate1, A3);
		SSstate2 = _mm256_xor_si256(SSstate2, k_m);

		SSstate1 = _mm256_xor_si256(SSstate1, SSstate2);

		/*
		The difference with uBlock round function
		*/
		SSstate3 = _mm256_permute2x128_si256(SSstate1, SSstate1, 1);//Left and right change
		SSstate1 = _mm256_shuffle_epi8(SSstate3, PL);//Byte permutation within 128-bit

		SSstate2 = _mm256_shuffle_epi8(SSstate2, PR);//Byte permutation within 128-bit


		//The fourth round

		K6 = _mm256_shuffle_epi8(K7, F);//the output after function P2

		tG = _mm256_permutevar8x32_epi32(tK1, G_Word);
		K7 = _mm256_shuffle_epi8(tG, G_Nibble);//the output after function G
		//得到K0，K1=（0x0a,0x0a,...）后生成KK0,KK1=(0xaa,0xaa,...)
		K6_L4 = _mm256_slli_epi16(K6, 4);
		KK6 = _mm256_xor_si256(K6, K6_L4);//KK0=(0xaa,0xaa,...)

		K7_L4 = _mm256_slli_epi16(K7, 4);
		KK7 = _mm256_xor_si256(K7, K7_L4);//KK1=(0xaa,0xaa,...)



		state1 = _mm256_xor_si256(state1, KK6);
		state2 = _mm256_xor_si256(state2, KK7);
		state1_L4 = _mm256_and_si256(state1, con_L4);
		state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);
		state1_R4 = _mm256_and_si256(state1, con);
		state1_L4 = _mm256_shuffle_epi8(S_L4, state1_L4_R4);
		state1_R4 = _mm256_shuffle_epi8(S, state1_R4);
		state1 = _mm256_xor_si256(state1_L4, state1_R4);

		state2_L4 = _mm256_and_si256(state2, con_L4);
		state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);
		state2_R4 = _mm256_and_si256(state2, con);
		state2_L4 = _mm256_shuffle_epi8(S_L4, state2_L4_R4);
		state2_R4 = _mm256_shuffle_epi8(S, state2_R4);
		state2 = _mm256_xor_si256(state2_L4, state2_R4);

		//state2 = _mm256_shuffle_epi8(S, state2);

		state2 = _mm256_xor_si256(state2, state1);

		k_m = _mm256_shuffle_epi8(state2, A1);
		state1 = _mm256_xor_si256(state1, k_m);

		k_m = _mm256_shuffle_epi8(state1, A2);
		state2 = _mm256_xor_si256(state2, k_m);

		k_m = _mm256_shuffle_epi8(state2, A2);
		state1 = _mm256_xor_si256(state1, k_m);

		k_m = _mm256_shuffle_epi8(state1, A3);
		state2 = _mm256_xor_si256(state2, k_m);

		state1 = _mm256_xor_si256(state1, state2);

		/*
		The difference with uBlock round function
		*/
		state3 = _mm256_permute2x128_si256(state1, state1, 1);//Left and right change
		state1 = _mm256_shuffle_epi8(state3, PL);//Byte permutation within 128-bit

		state2 = _mm256_shuffle_epi8(state2, PR);//Byte permutation within 128-bit


		//B3 and B4
		SSstate1 = _mm256_xor_si256(SSstate1, KK6);
		SSstate2 = _mm256_xor_si256(SSstate2, KK7);
		SSstate1_L4 = _mm256_and_si256(SSstate1, con_L4);
		SSstate1_L4_R4 = _mm256_srli_epi16(SSstate1_L4, 4);
		SSstate1_R4 = _mm256_and_si256(SSstate1, con);
		SSstate1_L4 = _mm256_shuffle_epi8(S_L4, SSstate1_L4_R4);
		SSstate1_R4 = _mm256_shuffle_epi8(S, SSstate1_R4);
		SSstate1 = _mm256_xor_si256(SSstate1_L4, SSstate1_R4);

		SSstate2_L4 = _mm256_and_si256(SSstate2, con_L4);
		SSstate2_L4_R4 = _mm256_srli_epi16(SSstate2_L4, 4);
		SSstate2_R4 = _mm256_and_si256(SSstate2, con);
		SSstate2_L4 = _mm256_shuffle_epi8(S_L4, SSstate2_L4_R4);
		SSstate2_R4 = _mm256_shuffle_epi8(S, SSstate2_R4);
		SSstate2 = _mm256_xor_si256(SSstate2_L4, SSstate2_R4);

		//state2 = _mm256_shuffle_epi8(S, state2);

		SSstate2 = _mm256_xor_si256(SSstate2, SSstate1);

		k_m = _mm256_shuffle_epi8(SSstate2, A1);
		SSstate1 = _mm256_xor_si256(SSstate1, k_m);

		k_m = _mm256_shuffle_epi8(SSstate1, A2);
		SSstate2 = _mm256_xor_si256(SSstate2, k_m);

		k_m = _mm256_shuffle_epi8(SSstate2, A2);
		SSstate1 = _mm256_xor_si256(SSstate1, k_m);

		k_m = _mm256_shuffle_epi8(SSstate1, A3);
		SSstate2 = _mm256_xor_si256(SSstate2, k_m);

		SSstate1 = _mm256_xor_si256(SSstate1, SSstate2);

		/*
		The difference with uBlock round function
		*/
		SSstate3 = _mm256_permute2x128_si256(SSstate1, SSstate1, 1);//Left and right change
		SSstate1 = _mm256_shuffle_epi8(SSstate3, PL);//Byte permutation within 128-bit

		SSstate2 = _mm256_shuffle_epi8(SSstate2, PR);//Byte permutation within 128-bit




	}

	//*** Modify 1***
	//Remove the XOR white key in the end of the round function
	//state1 = _mm256_xor_si256(state1, KK0);
	//state2 = _mm256_xor_si256(state2, KK1);

	//SSstate1 = _mm256_xor_si256(SSstate1, KK0);
	//SSstate2 = _mm256_xor_si256(SSstate2, KK1);





	/*
	* ==========================================================================
	* 反馈操作，完成后输出u1=（0xaa,...）,u2,u3=(0x0a,....)
	* ==========================================================================
	*/
	//添加反馈操作，放到前面减少指令
	state1 = _mm256_xor_si256(state1, *u1_x);
	state2 = _mm256_xor_si256(state2, *u1_y);

	SSstate1 = _mm256_xor_si256(SSstate1, *u1_x);
	SSstate2 = _mm256_xor_si256(SSstate2, *u1_y);




	state1_L4 = _mm256_and_si256(state1, con_L4);
	state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);

	state1_R4 = _mm256_and_si256(state1, con);
	state1_R4_L4 = _mm256_slli_epi16(state1_R4, 4);

	state2_L4 = _mm256_and_si256(state2, con_L4);
	state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);

	state2_R4 = _mm256_and_si256(state2, con);
	state2_R4_L4 = _mm256_slli_epi16(state2_R4, 4);




	*u1_x = _mm256_xor_si256(state1_R4, state1_R4_L4);//u1_x,u1_y=(0xaa,0xaa,...)
	*u1_y = _mm256_xor_si256(state2_R4, state2_R4_L4);

	*u2_x = state1_L4_R4;//u2_x,u2_y=(0x0a,0x0a,...)
	*u2_y = state2_L4_R4;



	SSstate1_L4 = _mm256_and_si256(SSstate1, con_L4);
	SSstate1_L4_R4 = _mm256_srli_epi16(SSstate1_L4, 4);

	SSstate1_R4 = _mm256_and_si256(SSstate1, con);
	//SSstate1_R4_L4 = _mm256_slli_epi16(SSstate1_R4, 4);

	SSstate2_L4 = _mm256_and_si256(SSstate2, con_L4);
	SSstate2_L4_R4 = _mm256_srli_epi16(SSstate2_L4, 4);

	SSstate2_R4 = _mm256_and_si256(SSstate2, con);
	//SSstate2_R4_L4 = _mm256_slli_epi16(SSstate2_R4, 4);




	*u3_x = SSstate1_R4;//_mm256_xor_si256(SSstate1_R4, SSstate1_R4_L4);//u1_x,u1_y=(0xaa,0xaa,...)
	*u3_y = SSstate2_R4;//_mm256_xor_si256(SSstate2_R4, SSstate2_R4_L4);

	*u4_x = SSstate1_L4_R4;//u2_x,u2_y=(0x0a,0x0a,...)
	*u4_y = SSstate2_L4_R4;



	return 0;
}



void CF_1024_Final(unsigned char* m, __m256i* u1_x, __m256i* u1_y, __m256i* u2_x, __m256i* u2_y, __m256i* u3_x, __m256i* u3_y, __m256i* u4_x, __m256i* u4_y, int round)
{
	//首先，将m1，m2,m3进行分割
	__m256i S_L4 = _mm256_setr_epi8(0x30, 0x00, 0x60, 0x20, 0x50, 0x40, 0xf0, 0xe0, 0xa0, 0x80, 0x70, 0x90, 0x10, 0xc0, 0xd0, 0xb0, 0x30, 0x00, 0x60, 0x20, 0x50, 0x40, 0xf0, 0xe0, 0xa0, 0x80, 0x70, 0x90, 0x10, 0xc0, 0xd0, 0xb0);
	__m256i S = _mm256_setr_epi8(0x3, 0x0, 0x6, 0x2, 0x5, 0x4, 0xf, 0xe, 0xa, 0x8, 0x7, 0x9, 0x1, 0xc, 0xd, 0xb, 0x3, 0x0, 0x6, 0x2, 0x5, 0x4, 0xf, 0xe, 0xa, 0x8, 0x7, 0x9, 0x1, 0xc, 0xd, 0xb);
	__m256i S_Inv = _mm256_setr_epi8(0xc, 0xa, 0xe, 0xd, 0x1, 0xf, 0xb, 0x0, 0x7, 0x2, 0x5, 0x4, 0x3, 0x6, 0x9, 0x8, 0xc, 0xa, 0xe, 0xd, 0x1, 0xf, 0xb, 0x0, 0x7, 0x2, 0x5, 0x4, 0x3, 0x6, 0x9, 0x8);

	__m256i con = _mm256_set1_epi8(0x0f);//取右4-bit
	__m256i con_L4 = _mm256_set1_epi8(0xf0);//取左4-bit
	__m256i A1 = _mm256_setr_epi8(1, 2, 3, 4, 5, 6, 7, 0, 9, 10, 11, 12, 13, 14, 15, 8, 1, 2, 3, 4, 5, 6, 7, 0, 9, 10, 11, 12, 13, 14, 15, 8);
	__m256i A2 = _mm256_setr_epi8(2, 3, 4, 5, 6, 7, 0, 1, 10, 11, 12, 13, 14, 15, 8, 9, 2, 3, 4, 5, 6, 7, 0, 1, 10, 11, 12, 13, 14, 15, 8, 9);
	__m256i A3 = _mm256_setr_epi8(5, 6, 7, 0, 1, 2, 3, 4, 13, 14, 15, 8, 9, 10, 11, 12, 5, 6, 7, 0, 1, 2, 3, 4, 13, 14, 15, 8, 9, 10, 11, 12);
	__m256i PL = _mm256_setr_epi8(12, 2, 0, 10, 14, 6, 13, 7, 11, 1, 8, 15, 9, 3, 5, 4, 3, 10, 4, 15, 14, 5, 7, 13, 12, 0, 11, 1, 9, 2, 6, 8);
	__m256i PR = _mm256_setr_epi8(0, 2, 1, 12, 11, 8, 13, 7, 10, 4, 6, 5, 3, 14, 9, 15, 6, 2, 11, 7, 8, 15, 4, 12, 13, 1, 0, 3, 9, 14, 10, 5);
	__m256i A4 = _mm256_setr_epi8(3, 4, 5, 6, 7, 0, 1, 2, 11, 12, 13, 14, 15, 8, 9, 10, 3, 4, 5, 6, 7, 0, 1, 2, 11, 12, 13, 14, 15, 8, 9, 10);

	__m256i c1 = _mm256_setr_epi8(0, 0x80, 1, 0x80, 2, 0x80, 3, 0x80, 4, 0x80, 5, 0x80, 6, 0x80, 7, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
	__m256i c2 = _mm256_setr_epi8(0x80, 0, 0x80, 1, 0x80, 2, 0x80, 3, 0x80, 4, 0x80, 5, 0x80, 6, 0x80, 7, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
	__m256i c3 = _mm256_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 8, 0x80, 9, 0x80, 10, 0x80, 11, 0x80, 12, 0x80, 13, 0x80, 14, 0x80, 15, 0x80);
	__m256i c4 = _mm256_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 8, 0x80, 9, 0x80, 10, 0x80, 11, 0x80, 12, 0x80, 13, 0x80, 14, 0x80, 15);


	__m256i c5 = _mm256_setr_epi8(0, 2, 4, 6, 8, 10, 12, 14, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
	__m256i c6 = _mm256_setr_epi8(1, 3, 5, 7, 9, 11, 13, 15, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
	__m256i c7 = _mm256_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0, 2, 4, 6, 8, 10, 12, 14, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
	__m256i c8 = _mm256_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 1, 3, 5, 7, 9, 11, 13, 15, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
	__m256i c55 = _mm256_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0, 2, 4, 6, 8, 10, 12, 14, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
	__m256i c66 = _mm256_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 1, 3, 5, 7, 9, 11, 13, 15, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
	__m256i c77 = _mm256_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0, 2, 4, 6, 8, 10, 12, 14);
	__m256i c88 = _mm256_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 1, 3, 5, 7, 9, 11, 13, 15);

	//G=[23, 3, 31, 2, 0, 20, 21, 15, 13, 29, 14, 30, 12, 28, 22, 1, 17, 24, 8, 5, 27, 16, 6, 11, 25, 18, 19, 10, 7, 9, 26, 4] (3cycle 8x32+ 1cycle two 16x4)
	//F = [11, 3, 14, 2, 9, 10, 12, 0, 4, 13, 7, 8, 15, 6, 5, 1, 16, 19, 27, 22, 31, 26, 20, 30, 23, 29, 25, 18, 24, 21, 28, 17](1 cycle two 16x4)

	__m256i F = _mm256_setr_epi8(11, 3, 14, 2, 9, 10, 12, 0, 4, 13, 7, 8, 15, 6, 5, 1, 0, 3, 11, 6, 15, 10, 4, 14, 7, 13, 9, 2, 8, 5, 12, 1);
	//__m256i P2 = _mm256_setr_epi8(8, 0, 6, 5, 3, 11, 7, 2, 14, 1, 15, 4, 10, 12, 13, 9, 13, 10, 1, 3, 12, 9, 14, 7, 6, 2, 15, 11, 8, 5, 4, 0);
	//G=G_Nibble + [2,4,5,6,0,1,3,7]
	__m256i G_Nibble = _mm256_setr_epi8(11, 3, 15, 2, 0, 8, 9, 7, 5, 13, 6, 14, 4, 12, 10, 1, 9, 12, 4, 1, 15, 8, 2, 7, 13, 10, 11, 6, 3, 5, 14, 0);
	__m256i G_Word = _mm256_setr_epi32(0, 3, 5, 7, 1, 2, 4, 6);


	int i;

	__m256i k, t1, t2, t3, t4, t5, t6, t7, t8;
	__m256i K0, K1, K0_L4, K1_L4, K2, K3, K2_L4, K3_L4, state_1_3, state_1_4, k_1;
	__m256i K4, K5, K4_L4, K5_L4, state_2_3, state_2_4, k_2;
	__m256i K6, K7, K6_L4, K7_L4, state_3_3, state_3_4, k_3;
	__m256i tG, k1, k3, tK1, tK2, tK3;
	__m256i KK0, KK1, KK2, KK3, KK4, KK5, KK6, KK7;

	__m256i dtG, dk1, dk3, dtK1, dtK2, dtK3;
	__m256i dKK0, dKK1, dKK2, dKK3, dKK4, dKK5, dKK6, dKK7;

	__m256i state1, state2, state3, SSstate1, SSstate2, SSstate3, k_m;
	__m256i state1_1, state2_1, state3_1, k_m_1;
	__m256i state1_L4, state2_L4, state1_L4_R4, state2_L4_R4, state1_R4, state2_R4, k_L4, k_R4, k_L4_R4;
	__m256i SSstate1_L4, SSstate2_L4, SSstate1_L4_R4, SSstate2_L4_R4, SSstate1_R4, SSstate2_R4, SSstate1_R4_L4, SSstate2_R4_L4;
	__m256i Value1, state1_R4_L4, state2_R4_L4;
	__m256i Value2, ValueK1, ValueK2;
	__m256i u1_x_pre, u1_y_pre, u2_x_pre, u2_y_pre, u1_x_R4, u1_y_R4;
	unsigned char value1[32] = { 0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x54 };

	//*** Modify 2***
	//The counter in the every brunch are same as the first l-1 block
	Value1 = _mm256_setr_epi8(0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x10);

	Value2 = _mm256_setr_epi8(0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x32);

	//*** Modify 2***
	//The counter in the every brunch are same as the first L-1 block
	ValueK1 = _mm256_setr_epi8(0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x10, 0x0f);

	ValueK2 = _mm256_setr_epi8(0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x11, 0x21);


	//K0 and K1<--(u2_x,u2_y)
	K0 = *u2_x;//K0=u2_x=(0x0a,0x0a,...)
	K1 = *u2_y;//K1=u2_y=(0x0a,0x0a,...)
	K0_L4 = _mm256_slli_epi16(K0, 4);
	KK0 = _mm256_xor_si256(K0, K0_L4);//KK0=(0xaa,0xaa,...)

	K1_L4 = _mm256_slli_epi16(K1, 4);
	KK1 = _mm256_xor_si256(K1, K1_L4);//KK1=(0xaa,0xaa,...)

	//*** Modify 3**
	dKK0 = KK0;
	dKK1 = KK1;

	//K2 and K3<--(u3_x,u3_y)
	K2 = *u3_x;//K0=u2_x=(0x0a,0x0a,...)
	K3 = *u3_y;//K1=u2_y=(0x0a,0x0a,...)
	K2_L4 = _mm256_slli_epi16(K2, 4);
	KK2 = _mm256_xor_si256(K2, K2_L4);//KK0=(0xaa,0xaa,...)

	K3_L4 = _mm256_slli_epi16(K3, 4);
	KK3 = _mm256_xor_si256(K3, K3_L4);//KK1=(0xaa,0xaa,...)

	//*** Modify 3**
	dKK2 = KK2;
	dKK3 = KK3;

	//K4 and K5<--(u3_x,u3_y)
	K4 = *u4_x;//K0=u2_x=(0x0a,0x0a,...)
	K5 = *u4_y;//K1=u2_y=(0x0a,0x0a,...)
	K4_L4 = _mm256_slli_epi16(K4, 4);
	KK4 = _mm256_xor_si256(K4, K4_L4);//KK0=(0xaa,0xaa,...)

	K5_L4 = _mm256_slli_epi16(K5, 4);
	KK5 = _mm256_xor_si256(K5, K5_L4);//KK1=(0xaa,0xaa,...)

	//*** Modify 3**
	dKK4 = KK4;
	dKK5 = KK5;


	//m2_x(K6)/m2_y(K7)
	//m2_x=(0x0a,0x0a,...)
	//m2_y=(0x0a,0x0a,...)
	K7 = _mm256_loadu_si256((__m256i*)(m));
	K6 = _mm256_srli_epi16(K7, 4);
	K6 = _mm256_and_si256(K6, con);
	K7 = _mm256_and_si256(K7, con);

	state_3_3 = _mm256_permute2x128_si256(K6, K6, 1);
	state_3_4 = _mm256_permute2x128_si256(K7, K7, 1);
	t1 = _mm256_shuffle_epi8(K6, c1);
	t2 = _mm256_shuffle_epi8(state_3_3, c3);
	t3 = _mm256_shuffle_epi8(K7, c2);
	t4 = _mm256_shuffle_epi8(state_3_4, c4);
	t5 = _mm256_shuffle_epi8(state_3_3, c1);
	t6 = _mm256_shuffle_epi8(K6, c3);
	t7 = _mm256_shuffle_epi8(state_3_4, c2);
	t8 = _mm256_shuffle_epi8(K7, c4);

	K6 = _mm256_xor_si256(t1, t2);
	state_3_3 = _mm256_xor_si256(t3, t4);
	K6 = _mm256_xor_si256(K6, state_3_3);

	K7 = _mm256_xor_si256(t5, t6);
	state_3_4 = _mm256_xor_si256(t7, t8);
	K7 = _mm256_xor_si256(K7, state_3_4);

	K6_L4 = _mm256_slli_epi16(K6, 4);
	KK6 = _mm256_xor_si256(K6, K6_L4);//KK6=(0xaa,0xaa,...)

	K7_L4 = _mm256_slli_epi16(K7, 4);
	KK7 = _mm256_xor_si256(K7, K7_L4);//KK7=(0xaa,0xaa,...)

	//*** Modify 3**
	dKK6 = KK6;
	dKK7 = _mm256_xor_si256(KK7, ValueK2);//KK7=(0xaa,0xaa,...)
	KK7 = _mm256_xor_si256(KK7, ValueK1);//KK7=(0xaa,0xaa,...)








	//取u1_x和u1_y的R4位
	u1_x_R4 = _mm256_and_si256(*u1_x, con);
	u1_y_R4 = _mm256_and_si256(*u1_y, con);
	//明文部分的赋值

	//第一个大块和第二个大块的加密输入,第1个分支在右4位，第2个分支左4位
	state1 = *u1_x;//u1_x=(0xaa,0xaa,....)
	state2 = *u1_y;//u1_y=(0xaa,0xaa,....)

	//u1和u2的值还得再处理一下
	//state1,state2=0x(aa,0xaa,...)
	//u1和u2是按照(0xaa,0xaa,0xaa)来存储的，不是按照(0x0a,0x0a,0x0a,)来存储的
	state2 = _mm256_xor_si256(state2, Value1);//区分两个块不同的明文输入


	//第3个大块和第4个大块的加密输入,第3个分支在右4位，第4个分支左4位
	SSstate1 = *u1_x;//u1_x=(0xaa,0xaa,....)
	SSstate2 = *u1_y;//u1_y=(0xaa,0xaa,....)

	//u1和u2的值还得再处理一下
	//state1,state2=0x(aa,0xaa,...)
	//u1和u2是按照(0xaa,0xaa,0xaa)来存储的，不是按照(0x0a,0x0a,0x0a,)来存储的
	SSstate2 = _mm256_xor_si256(SSstate2, Value2);//区分两个块不同的明文输入





	//第1个step不需要更新轮密钥
	//The first round B1 and B2
	state1 = _mm256_xor_si256(state1, KK0);
	state2 = _mm256_xor_si256(state2, KK1);
	state1_L4 = _mm256_and_si256(state1, con_L4);
	state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);
	state1_R4 = _mm256_and_si256(state1, con);
	state1_L4 = _mm256_shuffle_epi8(S_L4, state1_L4_R4);
	state1_R4 = _mm256_shuffle_epi8(S, state1_R4);
	state1 = _mm256_xor_si256(state1_L4, state1_R4);

	state2_L4 = _mm256_and_si256(state2, con_L4);
	state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);
	state2_R4 = _mm256_and_si256(state2, con);
	state2_L4 = _mm256_shuffle_epi8(S_L4, state2_L4_R4);
	state2_R4 = _mm256_shuffle_epi8(S, state2_R4);
	state2 = _mm256_xor_si256(state2_L4, state2_R4);

	//state2 = _mm256_shuffle_epi8(S, state2);

	state2 = _mm256_xor_si256(state2, state1);

	k_m = _mm256_shuffle_epi8(state2, A1);
	state1 = _mm256_xor_si256(state1, k_m);

	k_m = _mm256_shuffle_epi8(state1, A2);
	state2 = _mm256_xor_si256(state2, k_m);

	k_m = _mm256_shuffle_epi8(state2, A2);
	state1 = _mm256_xor_si256(state1, k_m);

	k_m = _mm256_shuffle_epi8(state1, A3);
	state2 = _mm256_xor_si256(state2, k_m);

	state1 = _mm256_xor_si256(state1, state2);

	/*
	The difference with uBlock round function
	*/
	state3 = _mm256_permute2x128_si256(state1, state1, 1);//Left and right change
	state1 = _mm256_shuffle_epi8(state3, PL);//Byte permutation within 128-bit

	state2 = _mm256_shuffle_epi8(state2, PR);//Byte permutation within 128-bit



	//B3 and B4
	SSstate1 = _mm256_xor_si256(SSstate1, dKK0);
	SSstate2 = _mm256_xor_si256(SSstate2, dKK1);
	SSstate1_L4 = _mm256_and_si256(SSstate1, con_L4);
	SSstate1_L4_R4 = _mm256_srli_epi16(SSstate1_L4, 4);
	SSstate1_R4 = _mm256_and_si256(SSstate1, con);
	SSstate1_L4 = _mm256_shuffle_epi8(S_L4, SSstate1_L4_R4);
	SSstate1_R4 = _mm256_shuffle_epi8(S, SSstate1_R4);
	SSstate1 = _mm256_xor_si256(SSstate1_L4, SSstate1_R4);

	SSstate2_L4 = _mm256_and_si256(SSstate2, con_L4);
	SSstate2_L4_R4 = _mm256_srli_epi16(SSstate2_L4, 4);
	SSstate2_R4 = _mm256_and_si256(SSstate2, con);
	SSstate2_L4 = _mm256_shuffle_epi8(S_L4, SSstate2_L4_R4);
	SSstate2_R4 = _mm256_shuffle_epi8(S, SSstate2_R4);
	SSstate2 = _mm256_xor_si256(SSstate2_L4, SSstate2_R4);

	//state2 = _mm256_shuffle_epi8(S, state2);

	SSstate2 = _mm256_xor_si256(SSstate2, SSstate1);

	k_m = _mm256_shuffle_epi8(SSstate2, A1);
	SSstate1 = _mm256_xor_si256(SSstate1, k_m);

	k_m = _mm256_shuffle_epi8(SSstate1, A2);
	SSstate2 = _mm256_xor_si256(SSstate2, k_m);

	k_m = _mm256_shuffle_epi8(SSstate2, A2);
	SSstate1 = _mm256_xor_si256(SSstate1, k_m);

	k_m = _mm256_shuffle_epi8(SSstate1, A3);
	SSstate2 = _mm256_xor_si256(SSstate2, k_m);

	SSstate1 = _mm256_xor_si256(SSstate1, SSstate2);

	/*
	The difference with uBlock round function
	*/
	SSstate3 = _mm256_permute2x128_si256(SSstate1, SSstate1, 1);//Left and right change
	SSstate1 = _mm256_shuffle_epi8(SSstate3, PL);//Byte permutation within 128-bit

	SSstate2 = _mm256_shuffle_epi8(SSstate2, PR);//Byte permutation within 128-bit




	//The second round
	state1 = _mm256_xor_si256(state1, KK2);
	state2 = _mm256_xor_si256(state2, KK3);
	state1_L4 = _mm256_and_si256(state1, con_L4);
	state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);
	state1_R4 = _mm256_and_si256(state1, con);
	state1_L4 = _mm256_shuffle_epi8(S_L4, state1_L4_R4);
	state1_R4 = _mm256_shuffle_epi8(S, state1_R4);
	state1 = _mm256_xor_si256(state1_L4, state1_R4);

	state2_L4 = _mm256_and_si256(state2, con_L4);
	state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);
	state2_R4 = _mm256_and_si256(state2, con);
	state2_L4 = _mm256_shuffle_epi8(S_L4, state2_L4_R4);
	state2_R4 = _mm256_shuffle_epi8(S, state2_R4);
	state2 = _mm256_xor_si256(state2_L4, state2_R4);

	//state2 = _mm256_shuffle_epi8(S, state2);

	state2 = _mm256_xor_si256(state2, state1);

	k_m = _mm256_shuffle_epi8(state2, A1);
	state1 = _mm256_xor_si256(state1, k_m);

	k_m = _mm256_shuffle_epi8(state1, A2);
	state2 = _mm256_xor_si256(state2, k_m);

	k_m = _mm256_shuffle_epi8(state2, A2);
	state1 = _mm256_xor_si256(state1, k_m);

	k_m = _mm256_shuffle_epi8(state1, A3);
	state2 = _mm256_xor_si256(state2, k_m);

	state1 = _mm256_xor_si256(state1, state2);

	/*
	The difference with uBlock round function
	*/
	state3 = _mm256_permute2x128_si256(state1, state1, 1);//Left and right change
	state1 = _mm256_shuffle_epi8(state3, PL);//Byte permutation within 128-bit

	state2 = _mm256_shuffle_epi8(state2, PR);//Byte permutation within 128-bit


	//B3 and B4
	SSstate1 = _mm256_xor_si256(SSstate1, dKK2);
	SSstate2 = _mm256_xor_si256(SSstate2, dKK3);
	SSstate1_L4 = _mm256_and_si256(SSstate1, con_L4);
	SSstate1_L4_R4 = _mm256_srli_epi16(SSstate1_L4, 4);
	SSstate1_R4 = _mm256_and_si256(SSstate1, con);
	SSstate1_L4 = _mm256_shuffle_epi8(S_L4, SSstate1_L4_R4);
	SSstate1_R4 = _mm256_shuffle_epi8(S, SSstate1_R4);
	SSstate1 = _mm256_xor_si256(SSstate1_L4, SSstate1_R4);

	SSstate2_L4 = _mm256_and_si256(SSstate2, con_L4);
	SSstate2_L4_R4 = _mm256_srli_epi16(SSstate2_L4, 4);
	SSstate2_R4 = _mm256_and_si256(SSstate2, con);
	SSstate2_L4 = _mm256_shuffle_epi8(S_L4, SSstate2_L4_R4);
	SSstate2_R4 = _mm256_shuffle_epi8(S, SSstate2_R4);
	SSstate2 = _mm256_xor_si256(SSstate2_L4, SSstate2_R4);

	//state2 = _mm256_shuffle_epi8(S, state2);

	SSstate2 = _mm256_xor_si256(SSstate2, SSstate1);

	k_m = _mm256_shuffle_epi8(SSstate2, A1);
	SSstate1 = _mm256_xor_si256(SSstate1, k_m);

	k_m = _mm256_shuffle_epi8(SSstate1, A2);
	SSstate2 = _mm256_xor_si256(SSstate2, k_m);

	k_m = _mm256_shuffle_epi8(SSstate2, A2);
	SSstate1 = _mm256_xor_si256(SSstate1, k_m);

	k_m = _mm256_shuffle_epi8(SSstate1, A3);
	SSstate2 = _mm256_xor_si256(SSstate2, k_m);

	SSstate1 = _mm256_xor_si256(SSstate1, SSstate2);

	/*
	The difference with uBlock round function
	*/
	SSstate3 = _mm256_permute2x128_si256(SSstate1, SSstate1, 1);//Left and right change
	SSstate1 = _mm256_shuffle_epi8(SSstate3, PL);//Byte permutation within 128-bit

	SSstate2 = _mm256_shuffle_epi8(SSstate2, PR);//Byte permutation within 128-bit


	//The third round
	state1 = _mm256_xor_si256(state1, KK4);
	state2 = _mm256_xor_si256(state2, KK5);
	state1_L4 = _mm256_and_si256(state1, con_L4);
	state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);
	state1_R4 = _mm256_and_si256(state1, con);
	state1_L4 = _mm256_shuffle_epi8(S_L4, state1_L4_R4);
	state1_R4 = _mm256_shuffle_epi8(S, state1_R4);
	state1 = _mm256_xor_si256(state1_L4, state1_R4);

	state2_L4 = _mm256_and_si256(state2, con_L4);
	state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);
	state2_R4 = _mm256_and_si256(state2, con);
	state2_L4 = _mm256_shuffle_epi8(S_L4, state2_L4_R4);
	state2_R4 = _mm256_shuffle_epi8(S, state2_R4);
	state2 = _mm256_xor_si256(state2_L4, state2_R4);

	//state2 = _mm256_shuffle_epi8(S, state2);

	state2 = _mm256_xor_si256(state2, state1);

	k_m = _mm256_shuffle_epi8(state2, A1);
	state1 = _mm256_xor_si256(state1, k_m);

	k_m = _mm256_shuffle_epi8(state1, A2);
	state2 = _mm256_xor_si256(state2, k_m);

	k_m = _mm256_shuffle_epi8(state2, A2);
	state1 = _mm256_xor_si256(state1, k_m);

	k_m = _mm256_shuffle_epi8(state1, A3);
	state2 = _mm256_xor_si256(state2, k_m);

	state1 = _mm256_xor_si256(state1, state2);

	/*
	The difference with uBlock round function
	*/
	state3 = _mm256_permute2x128_si256(state1, state1, 1);//Left and right change
	state1 = _mm256_shuffle_epi8(state3, PL);//Byte permutation within 128-bit

	state2 = _mm256_shuffle_epi8(state2, PR);//Byte permutation within 128-bit


	//B3 and B4
	SSstate1 = _mm256_xor_si256(SSstate1, dKK4);
	SSstate2 = _mm256_xor_si256(SSstate2, dKK5);
	SSstate1_L4 = _mm256_and_si256(SSstate1, con_L4);
	SSstate1_L4_R4 = _mm256_srli_epi16(SSstate1_L4, 4);
	SSstate1_R4 = _mm256_and_si256(SSstate1, con);
	SSstate1_L4 = _mm256_shuffle_epi8(S_L4, SSstate1_L4_R4);
	SSstate1_R4 = _mm256_shuffle_epi8(S, SSstate1_R4);
	SSstate1 = _mm256_xor_si256(SSstate1_L4, SSstate1_R4);

	SSstate2_L4 = _mm256_and_si256(SSstate2, con_L4);
	SSstate2_L4_R4 = _mm256_srli_epi16(SSstate2_L4, 4);
	SSstate2_R4 = _mm256_and_si256(SSstate2, con);
	SSstate2_L4 = _mm256_shuffle_epi8(S_L4, SSstate2_L4_R4);
	SSstate2_R4 = _mm256_shuffle_epi8(S, SSstate2_R4);
	SSstate2 = _mm256_xor_si256(SSstate2_L4, SSstate2_R4);

	//state2 = _mm256_shuffle_epi8(S, state2);

	SSstate2 = _mm256_xor_si256(SSstate2, SSstate1);

	k_m = _mm256_shuffle_epi8(SSstate2, A1);
	SSstate1 = _mm256_xor_si256(SSstate1, k_m);

	k_m = _mm256_shuffle_epi8(SSstate1, A2);
	SSstate2 = _mm256_xor_si256(SSstate2, k_m);

	k_m = _mm256_shuffle_epi8(SSstate2, A2);
	SSstate1 = _mm256_xor_si256(SSstate1, k_m);

	k_m = _mm256_shuffle_epi8(SSstate1, A3);
	SSstate2 = _mm256_xor_si256(SSstate2, k_m);

	SSstate1 = _mm256_xor_si256(SSstate1, SSstate2);

	/*
	The difference with uBlock round function
	*/
	SSstate3 = _mm256_permute2x128_si256(SSstate1, SSstate1, 1);//Left and right change
	SSstate1 = _mm256_shuffle_epi8(SSstate3, PL);//Byte permutation within 128-bit

	SSstate2 = _mm256_shuffle_epi8(SSstate2, PR);//Byte permutation within 128-bit



	//The fourth round
	state1 = _mm256_xor_si256(state1, KK6);
	state2 = _mm256_xor_si256(state2, KK7);
	state1_L4 = _mm256_and_si256(state1, con_L4);
	state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);
	state1_R4 = _mm256_and_si256(state1, con);
	state1_L4 = _mm256_shuffle_epi8(S_L4, state1_L4_R4);
	state1_R4 = _mm256_shuffle_epi8(S, state1_R4);
	state1 = _mm256_xor_si256(state1_L4, state1_R4);

	state2_L4 = _mm256_and_si256(state2, con_L4);
	state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);
	state2_R4 = _mm256_and_si256(state2, con);
	state2_L4 = _mm256_shuffle_epi8(S_L4, state2_L4_R4);
	state2_R4 = _mm256_shuffle_epi8(S, state2_R4);
	state2 = _mm256_xor_si256(state2_L4, state2_R4);

	//state2 = _mm256_shuffle_epi8(S, state2);

	state2 = _mm256_xor_si256(state2, state1);

	k_m = _mm256_shuffle_epi8(state2, A1);
	state1 = _mm256_xor_si256(state1, k_m);

	k_m = _mm256_shuffle_epi8(state1, A2);
	state2 = _mm256_xor_si256(state2, k_m);

	k_m = _mm256_shuffle_epi8(state2, A2);
	state1 = _mm256_xor_si256(state1, k_m);

	k_m = _mm256_shuffle_epi8(state1, A3);
	state2 = _mm256_xor_si256(state2, k_m);

	state1 = _mm256_xor_si256(state1, state2);

	/*
	The difference with uBlock round function
	*/
	state3 = _mm256_permute2x128_si256(state1, state1, 1);//Left and right change
	state1 = _mm256_shuffle_epi8(state3, PL);//Byte permutation within 128-bit

	state2 = _mm256_shuffle_epi8(state2, PR);//Byte permutation within 128-bit


	//B3 and B4
	SSstate1 = _mm256_xor_si256(SSstate1, dKK6);
	SSstate2 = _mm256_xor_si256(SSstate2, dKK7);
	SSstate1_L4 = _mm256_and_si256(SSstate1, con_L4);
	SSstate1_L4_R4 = _mm256_srli_epi16(SSstate1_L4, 4);
	SSstate1_R4 = _mm256_and_si256(SSstate1, con);
	SSstate1_L4 = _mm256_shuffle_epi8(S_L4, SSstate1_L4_R4);
	SSstate1_R4 = _mm256_shuffle_epi8(S, SSstate1_R4);
	SSstate1 = _mm256_xor_si256(SSstate1_L4, SSstate1_R4);

	SSstate2_L4 = _mm256_and_si256(SSstate2, con_L4);
	SSstate2_L4_R4 = _mm256_srli_epi16(SSstate2_L4, 4);
	SSstate2_R4 = _mm256_and_si256(SSstate2, con);
	SSstate2_L4 = _mm256_shuffle_epi8(S_L4, SSstate2_L4_R4);
	SSstate2_R4 = _mm256_shuffle_epi8(S, SSstate2_R4);
	SSstate2 = _mm256_xor_si256(SSstate2_L4, SSstate2_R4);

	//state2 = _mm256_shuffle_epi8(S, state2);

	SSstate2 = _mm256_xor_si256(SSstate2, SSstate1);

	k_m = _mm256_shuffle_epi8(SSstate2, A1);
	SSstate1 = _mm256_xor_si256(SSstate1, k_m);

	k_m = _mm256_shuffle_epi8(SSstate1, A2);
	SSstate2 = _mm256_xor_si256(SSstate2, k_m);

	k_m = _mm256_shuffle_epi8(SSstate2, A2);
	SSstate1 = _mm256_xor_si256(SSstate1, k_m);

	k_m = _mm256_shuffle_epi8(SSstate1, A3);
	SSstate2 = _mm256_xor_si256(SSstate2, k_m);

	SSstate1 = _mm256_xor_si256(SSstate1, SSstate2);

	/*
	The difference with uBlock round function
	*/
	SSstate3 = _mm256_permute2x128_si256(SSstate1, SSstate1, 1);//Left and right change
	SSstate1 = _mm256_shuffle_epi8(SSstate3, PL);//Byte permutation within 128-bit

	SSstate2 = _mm256_shuffle_epi8(SSstate2, PR);//Byte permutation within 128-bit





	//这里的round其实代表的是step

	for (i = 1; i < round; i++)
	{
		//*** Modify 3******************

		//The first round
		tK1 = KK0;
		tK2 = KK1;
		__m256i RoundConst = _mm256_setr_epi8(rc[i - 1][0], rc[i - 1][1], rc[i - 1][2], rc[i - 1][3], rc[i - 1][4], rc[i - 1][5], rc[i - 1][6], rc[i - 1][7], rc[i - 1][8], rc[i - 1][9], rc[i - 1][10], rc[i - 1][11], rc[i - 1][12], rc[i - 1][13], rc[i - 1][14], rc[i - 1][15], rc[i - 1][16], rc[i - 1][17], rc[i - 1][18], rc[i - 1][19], rc[i - 1][20], rc[i - 1][21], rc[i - 1][22], rc[i - 1][23], rc[i - 1][24], rc[i - 1][25], rc[i - 1][26], rc[i - 1][27], rc[i - 1][28], rc[i - 1][29], rc[i - 1][30], rc[i - 1][31]);
		//_mm256_setr_epi32(RC[i - 1][0], RC[i - 1][1], RC[i - 1][2], RC[i - 1][3], RC[i - 1][4], RC[i - 1][5], RC[i - 1][6], RC[i - 1][7]);
		//@@@@@@Bug is in here@@@@@@@
		KK0 = _mm256_xor_si256(KK2, RoundConst);
		KK0 = _mm256_shuffle_epi8(KK0, F);//the output after function P2
		//K0 = _mm256_shuffle_epi8(K2, F);//the output after function P2
		tG = _mm256_permutevar8x32_epi32(KK5, G_Word);
		KK1 = _mm256_shuffle_epi8(tG, G_Nibble);//the output after function G


		//The first round
		dtK1 = dKK0;
		dtK2 = dKK1;
		//__m256i RoundConst = _mm256_setr_epi8(rc[i - 1][0], rc[i - 1][1], rc[i - 1][2], rc[i - 1][3], rc[i - 1][4], rc[i - 1][5], rc[i - 1][6], rc[i - 1][7], rc[i - 1][8], rc[i - 1][9], rc[i - 1][10], rc[i - 1][11], rc[i - 1][12], rc[i - 1][13], rc[i - 1][14], rc[i - 1][15], rc[i - 1][16], rc[i - 1][17], rc[i - 1][18], rc[i - 1][19], rc[i - 1][20], rc[i - 1][21], rc[i - 1][22], rc[i - 1][23], rc[i - 1][24], rc[i - 1][25], rc[i - 1][26], rc[i - 1][27], rc[i - 1][28], rc[i - 1][29], rc[i - 1][30], rc[i - 1][31]);
		//_mm256_setr_epi32(RC[i - 1][0], RC[i - 1][1], RC[i - 1][2], RC[i - 1][3], RC[i - 1][4], RC[i - 1][5], RC[i - 1][6], RC[i - 1][7]);
		//@@@@@@Bug is in here@@@@@@@
		dKK0 = _mm256_xor_si256(dKK2, RoundConst);
		dKK0 = _mm256_shuffle_epi8(dKK0, F);//the output after function P2
		//K0 = _mm256_shuffle_epi8(K2, F);//the output after function P2
		dtG = _mm256_permutevar8x32_epi32(dKK5, G_Word);
		dKK1 = _mm256_shuffle_epi8(dtG, G_Nibble);//the output after function G
		//得到K0，K1=（0x0a,0x0a,...）后生成KK0,KK1=(0xaa,0xaa,...)


		//K0_L4 = _mm256_slli_epi16(K0, 4);
		//KK0 = _mm256_xor_si256(K0, K0_L4);//KK0=(0xaa,0xaa,...)

		//K1_L4 = _mm256_slli_epi16(K1, 4);
		//KK1 = _mm256_xor_si256(K1, K1_L4);//KK1=(0xaa,0xaa,...)







		state1 = _mm256_xor_si256(state1, KK0);
		state2 = _mm256_xor_si256(state2, KK1);
		state1_L4 = _mm256_and_si256(state1, con_L4);
		state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);
		state1_R4 = _mm256_and_si256(state1, con);
		state1_L4 = _mm256_shuffle_epi8(S_L4, state1_L4_R4);
		state1_R4 = _mm256_shuffle_epi8(S, state1_R4);
		state1 = _mm256_xor_si256(state1_L4, state1_R4);

		state2_L4 = _mm256_and_si256(state2, con_L4);
		state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);
		state2_R4 = _mm256_and_si256(state2, con);
		state2_L4 = _mm256_shuffle_epi8(S_L4, state2_L4_R4);
		state2_R4 = _mm256_shuffle_epi8(S, state2_R4);
		state2 = _mm256_xor_si256(state2_L4, state2_R4);

		//state2 = _mm256_shuffle_epi8(S, state2);

		state2 = _mm256_xor_si256(state2, state1);

		k_m = _mm256_shuffle_epi8(state2, A1);
		state1 = _mm256_xor_si256(state1, k_m);

		k_m = _mm256_shuffle_epi8(state1, A2);
		state2 = _mm256_xor_si256(state2, k_m);

		k_m = _mm256_shuffle_epi8(state2, A2);
		state1 = _mm256_xor_si256(state1, k_m);

		k_m = _mm256_shuffle_epi8(state1, A3);
		state2 = _mm256_xor_si256(state2, k_m);

		state1 = _mm256_xor_si256(state1, state2);

		/*
		The difference with uBlock round function
		*/
		state3 = _mm256_permute2x128_si256(state1, state1, 1);//Left and right change
		state1 = _mm256_shuffle_epi8(state3, PL);//Byte permutation within 128-bit

		state2 = _mm256_shuffle_epi8(state2, PR);//Byte permutation within 128-bit



		//B3 and B4
		SSstate1 = _mm256_xor_si256(SSstate1, dKK0);
		SSstate2 = _mm256_xor_si256(SSstate2, dKK1);
		SSstate1_L4 = _mm256_and_si256(SSstate1, con_L4);
		SSstate1_L4_R4 = _mm256_srli_epi16(SSstate1_L4, 4);
		SSstate1_R4 = _mm256_and_si256(SSstate1, con);
		SSstate1_L4 = _mm256_shuffle_epi8(S_L4, SSstate1_L4_R4);
		SSstate1_R4 = _mm256_shuffle_epi8(S, SSstate1_R4);
		SSstate1 = _mm256_xor_si256(SSstate1_L4, SSstate1_R4);

		SSstate2_L4 = _mm256_and_si256(SSstate2, con_L4);
		SSstate2_L4_R4 = _mm256_srli_epi16(SSstate2_L4, 4);
		SSstate2_R4 = _mm256_and_si256(SSstate2, con);
		SSstate2_L4 = _mm256_shuffle_epi8(S_L4, SSstate2_L4_R4);
		SSstate2_R4 = _mm256_shuffle_epi8(S, SSstate2_R4);
		SSstate2 = _mm256_xor_si256(SSstate2_L4, SSstate2_R4);

		//state2 = _mm256_shuffle_epi8(S, state2);

		SSstate2 = _mm256_xor_si256(SSstate2, SSstate1);

		k_m = _mm256_shuffle_epi8(SSstate2, A1);
		SSstate1 = _mm256_xor_si256(SSstate1, k_m);

		k_m = _mm256_shuffle_epi8(SSstate1, A2);
		SSstate2 = _mm256_xor_si256(SSstate2, k_m);

		k_m = _mm256_shuffle_epi8(SSstate2, A2);
		SSstate1 = _mm256_xor_si256(SSstate1, k_m);

		k_m = _mm256_shuffle_epi8(SSstate1, A3);
		SSstate2 = _mm256_xor_si256(SSstate2, k_m);

		SSstate1 = _mm256_xor_si256(SSstate1, SSstate2);

		/*
		The difference with uBlock round function
		*/
		SSstate3 = _mm256_permute2x128_si256(SSstate1, SSstate1, 1);//Left and right change
		SSstate1 = _mm256_shuffle_epi8(SSstate3, PL);//Byte permutation within 128-bit

		SSstate2 = _mm256_shuffle_epi8(SSstate2, PR);//Byte permutation within 128-bit




		//*** Modify 3 ******

		//The second round
		KK2 = _mm256_shuffle_epi8(tK2, F);//the output after function P2
		tK2 = KK3;
		tG = _mm256_permutevar8x32_epi32(tK1, G_Word);
		KK3 = _mm256_shuffle_epi8(tG, G_Nibble);//the output after function G


		//The second round
		dKK2 = _mm256_shuffle_epi8(dtK2, F);//the output after function P2
		dtK2 = dKK3;
		dtG = _mm256_permutevar8x32_epi32(dtK1, G_Word);
		dKK3 = _mm256_shuffle_epi8(dtG, G_Nibble);//the output after function G
		//得到K0，K1=（0x0a,0x0a,...）后生成KK0,KK1=(0xaa,0xaa,...)



		//K2_L4 = _mm256_slli_epi16(K2, 4);
		//KK2 = _mm256_xor_si256(K2, K2_L4);//KK0=(0xaa,0xaa,...)

		//K3_L4 = _mm256_slli_epi16(K3, 4);
		//KK3 = _mm256_xor_si256(K3, K3_L4);//KK1=(0xaa,0xaa,...)




		state1 = _mm256_xor_si256(state1, KK2);
		state2 = _mm256_xor_si256(state2, KK3);
		state1_L4 = _mm256_and_si256(state1, con_L4);
		state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);
		state1_R4 = _mm256_and_si256(state1, con);
		state1_L4 = _mm256_shuffle_epi8(S_L4, state1_L4_R4);
		state1_R4 = _mm256_shuffle_epi8(S, state1_R4);
		state1 = _mm256_xor_si256(state1_L4, state1_R4);

		state2_L4 = _mm256_and_si256(state2, con_L4);
		state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);
		state2_R4 = _mm256_and_si256(state2, con);
		state2_L4 = _mm256_shuffle_epi8(S_L4, state2_L4_R4);
		state2_R4 = _mm256_shuffle_epi8(S, state2_R4);
		state2 = _mm256_xor_si256(state2_L4, state2_R4);

		//state2 = _mm256_shuffle_epi8(S, state2);

		state2 = _mm256_xor_si256(state2, state1);

		k_m = _mm256_shuffle_epi8(state2, A1);
		state1 = _mm256_xor_si256(state1, k_m);

		k_m = _mm256_shuffle_epi8(state1, A2);
		state2 = _mm256_xor_si256(state2, k_m);

		k_m = _mm256_shuffle_epi8(state2, A2);
		state1 = _mm256_xor_si256(state1, k_m);

		k_m = _mm256_shuffle_epi8(state1, A3);
		state2 = _mm256_xor_si256(state2, k_m);

		state1 = _mm256_xor_si256(state1, state2);

		/*
		The difference with uBlock round function
		*/
		state3 = _mm256_permute2x128_si256(state1, state1, 1);//Left and right change
		state1 = _mm256_shuffle_epi8(state3, PL);//Byte permutation within 128-bit

		state2 = _mm256_shuffle_epi8(state2, PR);//Byte permutation within 128-bit


		//B3 and B4
		SSstate1 = _mm256_xor_si256(SSstate1, dKK2);
		SSstate2 = _mm256_xor_si256(SSstate2, dKK3);
		SSstate1_L4 = _mm256_and_si256(SSstate1, con_L4);
		SSstate1_L4_R4 = _mm256_srli_epi16(SSstate1_L4, 4);
		SSstate1_R4 = _mm256_and_si256(SSstate1, con);
		SSstate1_L4 = _mm256_shuffle_epi8(S_L4, SSstate1_L4_R4);
		SSstate1_R4 = _mm256_shuffle_epi8(S, SSstate1_R4);
		SSstate1 = _mm256_xor_si256(SSstate1_L4, SSstate1_R4);

		SSstate2_L4 = _mm256_and_si256(SSstate2, con_L4);
		SSstate2_L4_R4 = _mm256_srli_epi16(SSstate2_L4, 4);
		SSstate2_R4 = _mm256_and_si256(SSstate2, con);
		SSstate2_L4 = _mm256_shuffle_epi8(S_L4, SSstate2_L4_R4);
		SSstate2_R4 = _mm256_shuffle_epi8(S, SSstate2_R4);
		SSstate2 = _mm256_xor_si256(SSstate2_L4, SSstate2_R4);

		//state2 = _mm256_shuffle_epi8(S, state2);

		SSstate2 = _mm256_xor_si256(SSstate2, SSstate1);

		k_m = _mm256_shuffle_epi8(SSstate2, A1);
		SSstate1 = _mm256_xor_si256(SSstate1, k_m);

		k_m = _mm256_shuffle_epi8(SSstate1, A2);
		SSstate2 = _mm256_xor_si256(SSstate2, k_m);

		k_m = _mm256_shuffle_epi8(SSstate2, A2);
		SSstate1 = _mm256_xor_si256(SSstate1, k_m);

		k_m = _mm256_shuffle_epi8(SSstate1, A3);
		SSstate2 = _mm256_xor_si256(SSstate2, k_m);

		SSstate1 = _mm256_xor_si256(SSstate1, SSstate2);

		/*
		The difference with uBlock round function
		*/
		SSstate3 = _mm256_permute2x128_si256(SSstate1, SSstate1, 1);//Left and right change
		SSstate1 = _mm256_shuffle_epi8(SSstate3, PL);//Byte permutation within 128-bit

		SSstate2 = _mm256_shuffle_epi8(SSstate2, PR);//Byte permutation within 128-bit



		//*** Modify 3 ******
		// 
		//The third round
		tK1 = KK4;
		KK4 = _mm256_shuffle_epi8(tK2, F);//the output after function P2

		tG = _mm256_permutevar8x32_epi32(KK6, G_Word);
		KK5 = _mm256_shuffle_epi8(tG, G_Nibble);//the output after function G

		dtK1 = dKK4;
		dKK4 = _mm256_shuffle_epi8(dtK2, F);//the output after function P2

		dtG = _mm256_permutevar8x32_epi32(dKK6, G_Word);
		dKK5 = _mm256_shuffle_epi8(dtG, G_Nibble);//the output after function G


		//得到K0，K1=（0x0a,0x0a,...）后生成KK0,KK1=(0xaa,0xaa,...)
		//K4_L4 = _mm256_slli_epi16(K4, 4);
		//KK4 = _mm256_xor_si256(K4, K4_L4);//KK0=(0xaa,0xaa,...)

		//K5_L4 = _mm256_slli_epi16(K5, 4);
		//KK5 = _mm256_xor_si256(K5, K5_L4);//KK1=(0xaa,0xaa,...)




		state1 = _mm256_xor_si256(state1, KK4);
		state2 = _mm256_xor_si256(state2, KK5);
		state1_L4 = _mm256_and_si256(state1, con_L4);
		state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);
		state1_R4 = _mm256_and_si256(state1, con);
		state1_L4 = _mm256_shuffle_epi8(S_L4, state1_L4_R4);
		state1_R4 = _mm256_shuffle_epi8(S, state1_R4);
		state1 = _mm256_xor_si256(state1_L4, state1_R4);

		state2_L4 = _mm256_and_si256(state2, con_L4);
		state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);
		state2_R4 = _mm256_and_si256(state2, con);
		state2_L4 = _mm256_shuffle_epi8(S_L4, state2_L4_R4);
		state2_R4 = _mm256_shuffle_epi8(S, state2_R4);
		state2 = _mm256_xor_si256(state2_L4, state2_R4);

		//state2 = _mm256_shuffle_epi8(S, state2);

		state2 = _mm256_xor_si256(state2, state1);

		k_m = _mm256_shuffle_epi8(state2, A1);
		state1 = _mm256_xor_si256(state1, k_m);

		k_m = _mm256_shuffle_epi8(state1, A2);
		state2 = _mm256_xor_si256(state2, k_m);

		k_m = _mm256_shuffle_epi8(state2, A2);
		state1 = _mm256_xor_si256(state1, k_m);

		k_m = _mm256_shuffle_epi8(state1, A3);
		state2 = _mm256_xor_si256(state2, k_m);

		state1 = _mm256_xor_si256(state1, state2);

		/*
		The difference with uBlock round function
		*/
		state3 = _mm256_permute2x128_si256(state1, state1, 1);//Left and right change
		state1 = _mm256_shuffle_epi8(state3, PL);//Byte permutation within 128-bit

		state2 = _mm256_shuffle_epi8(state2, PR);//Byte permutation within 128-bit


		//B3 and B4
		SSstate1 = _mm256_xor_si256(SSstate1, dKK4);
		SSstate2 = _mm256_xor_si256(SSstate2, dKK5);
		SSstate1_L4 = _mm256_and_si256(SSstate1, con_L4);
		SSstate1_L4_R4 = _mm256_srli_epi16(SSstate1_L4, 4);
		SSstate1_R4 = _mm256_and_si256(SSstate1, con);
		SSstate1_L4 = _mm256_shuffle_epi8(S_L4, SSstate1_L4_R4);
		SSstate1_R4 = _mm256_shuffle_epi8(S, SSstate1_R4);
		SSstate1 = _mm256_xor_si256(SSstate1_L4, SSstate1_R4);

		SSstate2_L4 = _mm256_and_si256(SSstate2, con_L4);
		SSstate2_L4_R4 = _mm256_srli_epi16(SSstate2_L4, 4);
		SSstate2_R4 = _mm256_and_si256(SSstate2, con);
		SSstate2_L4 = _mm256_shuffle_epi8(S_L4, SSstate2_L4_R4);
		SSstate2_R4 = _mm256_shuffle_epi8(S, SSstate2_R4);
		SSstate2 = _mm256_xor_si256(SSstate2_L4, SSstate2_R4);

		//state2 = _mm256_shuffle_epi8(S, state2);

		SSstate2 = _mm256_xor_si256(SSstate2, SSstate1);

		k_m = _mm256_shuffle_epi8(SSstate2, A1);
		SSstate1 = _mm256_xor_si256(SSstate1, k_m);

		k_m = _mm256_shuffle_epi8(SSstate1, A2);
		SSstate2 = _mm256_xor_si256(SSstate2, k_m);

		k_m = _mm256_shuffle_epi8(SSstate2, A2);
		SSstate1 = _mm256_xor_si256(SSstate1, k_m);

		k_m = _mm256_shuffle_epi8(SSstate1, A3);
		SSstate2 = _mm256_xor_si256(SSstate2, k_m);

		SSstate1 = _mm256_xor_si256(SSstate1, SSstate2);

		/*
		The difference with uBlock round function
		*/
		SSstate3 = _mm256_permute2x128_si256(SSstate1, SSstate1, 1);//Left and right change
		SSstate1 = _mm256_shuffle_epi8(SSstate3, PL);//Byte permutation within 128-bit

		SSstate2 = _mm256_shuffle_epi8(SSstate2, PR);//Byte permutation within 128-bit


		//*** Modify 3*****

		//The fourth round

		KK6 = _mm256_shuffle_epi8(KK7, F);//the output after function P2

		tG = _mm256_permutevar8x32_epi32(tK1, G_Word);
		KK7 = _mm256_shuffle_epi8(tG, G_Nibble);//the output after function G

		dKK6 = _mm256_shuffle_epi8(dKK7, F);//the output after function P2

		dtG = _mm256_permutevar8x32_epi32(dtK1, G_Word);
		dKK7 = _mm256_shuffle_epi8(dtG, G_Nibble);//the output after function G
		//得到K0，K1=（0x0a,0x0a,...）后生成KK0,KK1=(0xaa,0xaa,...)




		//K6_L4 = _mm256_slli_epi16(K6, 4);



		//KK6 = _mm256_xor_si256(K6, K6_L4);//KK0=(0xaa,0xaa,...)

		//K7_L4 = _mm256_slli_epi16(K7, 4);
		//KK7 = _mm256_xor_si256(K7, K7_L4);//KK1=(0xaa,0xaa,...)



		state1 = _mm256_xor_si256(state1, KK6);
		state2 = _mm256_xor_si256(state2, KK7);
		state1_L4 = _mm256_and_si256(state1, con_L4);
		state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);
		state1_R4 = _mm256_and_si256(state1, con);
		state1_L4 = _mm256_shuffle_epi8(S_L4, state1_L4_R4);
		state1_R4 = _mm256_shuffle_epi8(S, state1_R4);
		state1 = _mm256_xor_si256(state1_L4, state1_R4);

		state2_L4 = _mm256_and_si256(state2, con_L4);
		state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);
		state2_R4 = _mm256_and_si256(state2, con);
		state2_L4 = _mm256_shuffle_epi8(S_L4, state2_L4_R4);
		state2_R4 = _mm256_shuffle_epi8(S, state2_R4);
		state2 = _mm256_xor_si256(state2_L4, state2_R4);

		//state2 = _mm256_shuffle_epi8(S, state2);

		state2 = _mm256_xor_si256(state2, state1);

		k_m = _mm256_shuffle_epi8(state2, A1);
		state1 = _mm256_xor_si256(state1, k_m);

		k_m = _mm256_shuffle_epi8(state1, A2);
		state2 = _mm256_xor_si256(state2, k_m);

		k_m = _mm256_shuffle_epi8(state2, A2);
		state1 = _mm256_xor_si256(state1, k_m);

		k_m = _mm256_shuffle_epi8(state1, A3);
		state2 = _mm256_xor_si256(state2, k_m);

		state1 = _mm256_xor_si256(state1, state2);

		/*
		The difference with uBlock round function
		*/
		state3 = _mm256_permute2x128_si256(state1, state1, 1);//Left and right change
		state1 = _mm256_shuffle_epi8(state3, PL);//Byte permutation within 128-bit

		state2 = _mm256_shuffle_epi8(state2, PR);//Byte permutation within 128-bit


		//B3 and B4
		SSstate1 = _mm256_xor_si256(SSstate1, dKK6);
		SSstate2 = _mm256_xor_si256(SSstate2, dKK7);
		SSstate1_L4 = _mm256_and_si256(SSstate1, con_L4);
		SSstate1_L4_R4 = _mm256_srli_epi16(SSstate1_L4, 4);
		SSstate1_R4 = _mm256_and_si256(SSstate1, con);
		SSstate1_L4 = _mm256_shuffle_epi8(S_L4, SSstate1_L4_R4);
		SSstate1_R4 = _mm256_shuffle_epi8(S, SSstate1_R4);
		SSstate1 = _mm256_xor_si256(SSstate1_L4, SSstate1_R4);

		SSstate2_L4 = _mm256_and_si256(SSstate2, con_L4);
		SSstate2_L4_R4 = _mm256_srli_epi16(SSstate2_L4, 4);
		SSstate2_R4 = _mm256_and_si256(SSstate2, con);
		SSstate2_L4 = _mm256_shuffle_epi8(S_L4, SSstate2_L4_R4);
		SSstate2_R4 = _mm256_shuffle_epi8(S, SSstate2_R4);
		SSstate2 = _mm256_xor_si256(SSstate2_L4, SSstate2_R4);

		//state2 = _mm256_shuffle_epi8(S, state2);

		SSstate2 = _mm256_xor_si256(SSstate2, SSstate1);

		k_m = _mm256_shuffle_epi8(SSstate2, A1);
		SSstate1 = _mm256_xor_si256(SSstate1, k_m);

		k_m = _mm256_shuffle_epi8(SSstate1, A2);
		SSstate2 = _mm256_xor_si256(SSstate2, k_m);

		k_m = _mm256_shuffle_epi8(SSstate2, A2);
		SSstate1 = _mm256_xor_si256(SSstate1, k_m);

		k_m = _mm256_shuffle_epi8(SSstate1, A3);
		SSstate2 = _mm256_xor_si256(SSstate2, k_m);

		SSstate1 = _mm256_xor_si256(SSstate1, SSstate2);

		/*
		The difference with uBlock round function
		*/
		SSstate3 = _mm256_permute2x128_si256(SSstate1, SSstate1, 1);//Left and right change
		SSstate1 = _mm256_shuffle_epi8(SSstate3, PL);//Byte permutation within 128-bit

		SSstate2 = _mm256_shuffle_epi8(SSstate2, PR);//Byte permutation within 128-bit




	}


	//*** Modify 1***
	// //Remove the XOR white key in the end of the round function
	//state1 = _mm256_xor_si256(state1, KK0);
	//state2 = _mm256_xor_si256(state2, KK1);

	//SSstate1 = _mm256_xor_si256(SSstate1, KK0);
	//SSstate2 = _mm256_xor_si256(SSstate2, KK1);





	/*
	* ==========================================================================
	* 反馈操作，完成后输出u1=（0xaa,...）,u2,u3=(0x0a,....)
	* ==========================================================================
	*/
	//添加反馈操作，放到前面减少指令
	state1 = _mm256_xor_si256(state1, *u1_x);
	state2 = _mm256_xor_si256(state2, *u1_y);

	SSstate1 = _mm256_xor_si256(SSstate1, *u1_x);
	SSstate2 = _mm256_xor_si256(SSstate2, *u1_y);




	state1_L4 = _mm256_and_si256(state1, con_L4);
	state1_L4_R4 = _mm256_srli_epi16(state1_L4, 4);

	state1_R4 = _mm256_and_si256(state1, con);
	state1_R4_L4 = _mm256_slli_epi16(state1_R4, 4);

	state2_L4 = _mm256_and_si256(state2, con_L4);
	state2_L4_R4 = _mm256_srli_epi16(state2_L4, 4);

	state2_R4 = _mm256_and_si256(state2, con);
	state2_R4_L4 = _mm256_slli_epi16(state2_R4, 4);




	*u1_x = _mm256_xor_si256(state1_R4, state1_R4_L4);//u1_x,u1_y=(0xaa,0xaa,...)
	*u1_y = _mm256_xor_si256(state2_R4, state2_R4_L4);

	*u2_x = state1_L4_R4;//u2_x,u2_y=(0x0a,0x0a,...)
	*u2_y = state2_L4_R4;



	SSstate1_L4 = _mm256_and_si256(SSstate1, con_L4);
	SSstate1_L4_R4 = _mm256_srli_epi16(SSstate1_L4, 4);

	SSstate1_R4 = _mm256_and_si256(SSstate1, con);
	//SSstate1_R4_L4 = _mm256_slli_epi16(SSstate1_R4, 4);

	SSstate2_L4 = _mm256_and_si256(SSstate2, con_L4);
	SSstate2_L4_R4 = _mm256_srli_epi16(SSstate2_L4, 4);

	SSstate2_R4 = _mm256_and_si256(SSstate2, con);
	//SSstate2_R4_L4 = _mm256_slli_epi16(SSstate2_R4, 4);




	*u3_x = SSstate1_R4;//_mm256_xor_si256(SSstate1_R4, SSstate1_R4_L4);//u1_x,u1_y=(0xaa,0xaa,...)
	*u3_y = SSstate2_R4;//_mm256_xor_si256(SSstate2_R4, SSstate2_R4_L4);

	*u4_x = SSstate1_L4_R4;//u2_x,u2_y=(0x0a,0x0a,...)
	*u4_y = SSstate2_L4_R4;



	return;
}









void Padding(const unsigned char* msg, unsigned char* M, unsigned long long msg_len_bits, int A)
{
	//msg[msg_len_bits]={0x12,0x34,0x56,0x78,0x9a};
	//***NOTE***
	//The correctness details of padding should be verified.
	unsigned long long msg_len_bytes, msg_len_bytes_col;//msg_len_bytes is xia qu zheng
	unsigned long long AN_bits = A * BlockSize * 8;
	unsigned long long row, col, actual_len_bytes, i, j;
	row = (int)(msg_len_bits / AN_bits) + 1;
	row = AN_bits * row;
	actual_len_bytes = (int)(row / 8);
	msg_len_bytes = (int)(msg_len_bits / 8);
	//col = msg_len_bits % AN_bits;
	msg_len_bytes_col = msg_len_bits % 8;
	//M = (unsigned char*)malloc(actual_len_bytes);
	//memset(M, 0, actual_len_bytes);
	for (i = 0; i < msg_len_bytes; i++)
		M[i] = msg[i];
	if (msg_len_bytes_col == 0)
		M[msg_len_bytes] = 0x80;
	else
		M[msg_len_bytes] = msg[msg_len_bytes] + (1 << (7 - msg_len_bytes_col));


	/*
	printf("The value after Paddint:::\n");
	for (int t = 0; t < actual_len_bytes; t++)
		printf(" % 02x ", M[t]);
	printf("\n");
	*/

	return;//The length of message after padding
}



int uHash_512(const unsigned char* msg, unsigned long long msg_len_bits, unsigned char* h)
{
	//共有in_len块M=m0||m1=(0x0a,....)
	__m256i IV512_1_x = _mm256_setr_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	__m256i IV512_1_y = _mm256_setr_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x22, 0, 0);

	__m256i IV512_2_x = _mm256_setr_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	__m256i IV512_2_y = _mm256_setr_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	__m256i con = _mm256_set1_epi8(0x0f);//取右4-bit
	__m256i c5 = _mm256_setr_epi8(0, 2, 4, 6, 8, 10, 12, 14, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
	__m256i c6 = _mm256_setr_epi8(1, 3, 5, 7, 9, 11, 13, 15, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
	__m256i c7 = _mm256_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0, 2, 4, 6, 8, 10, 12, 14, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
	__m256i c8 = _mm256_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 1, 3, 5, 7, 9, 11, 13, 15, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
	__m256i c55 = _mm256_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0, 2, 4, 6, 8, 10, 12, 14, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
	__m256i c66 = _mm256_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 1, 3, 5, 7, 9, 11, 13, 15, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
	__m256i c77 = _mm256_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0, 2, 4, 6, 8, 10, 12, 14);
	__m256i c88 = _mm256_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 1, 3, 5, 7, 9, 11, 13, 15);



	int i;
	int Num_Block;
	Num_Block = 3;
	unsigned long long row, col, l;
	unsigned char M[3 * BlockSize] = { 0 };//The message after padding
	unsigned long long actual_len_bytes, msg_len_bytes_col, msg_len_bytes_row, msg_len_bytes;
	unsigned long long AN_bits = Num_Block * BlockSize * 8;
	unsigned long long AN_byte = Num_Block * BlockSize;
	row = (int)(msg_len_bits / AN_bits) + 1;
	row = AN_bits * row;
	actual_len_bytes = (int)(row / 8);
	msg_len_bytes = (int)(msg_len_bits / 8);

	//Padding Optimal
	msg_len_bytes_col = msg_len_bits % 8;
	//msg_len_bytes_row = msg_len_bytes - msg_len_bytes_col;
	msg_len_bytes_row = msg_len_bytes - (msg_len_bytes % AN_byte);

	if (msg_len_bytes_col == 0)
		M[msg_len_bytes % AN_byte] = 0x80;
	else
	{
		M[msg_len_bytes % AN_byte] = msg[msg_len_bytes] + (1 << (7 - msg_len_bytes_col));
	}
	for (int tt = 0; tt < msg_len_bytes % AN_byte; tt++)
		M[tt] = msg[msg_len_bytes_row + tt];







	l = (int)(actual_len_bytes / (Num_Block * BlockSize));//The number of blocks of message M
	__m256i u1_x, u1_y, u2_x, u2_y;//u1=(0xaa,...),u2,u3=(0x0a,...)
	__m256i state3, state4, state1, state2, t1, t2, t3, t4, t5, t6, t7, t8;

	u1_x = IV512_1_x;
	u1_y = IV512_1_y;;
	u2_x = IV512_2_x;
	u2_y = IV512_2_y;

	for (i = 0; i < l - 1; i++)
	{
		CF_512(msg + Num_Block * 32 * i, &u1_x, &u1_y, &u2_x, &u2_y, ROUND);
	}
	CF_512_Final(M, &u1_x, &u1_y, &u2_x, &u2_y, ROUND);

	//整合输出
	state1 = _mm256_and_si256(u1_x, con);
	state2 = _mm256_and_si256(u1_y, con);
	state3 = _mm256_permute2x128_si256(state1, state1, 1);
	state4 = _mm256_permute2x128_si256(state2, state2, 1);
	t1 = _mm256_shuffle_epi8(state1, c5);
	t2 = _mm256_shuffle_epi8(state3, c7);
	t3 = _mm256_shuffle_epi8(state4, c55);
	t4 = _mm256_shuffle_epi8(state2, c77);
	t5 = _mm256_shuffle_epi8(state1, c6);
	t6 = _mm256_shuffle_epi8(state3, c8);
	t7 = _mm256_shuffle_epi8(state4, c66);
	t8 = _mm256_shuffle_epi8(state2, c88);

	state1 = _mm256_xor_si256(t1, t2);
	state3 = _mm256_xor_si256(t3, t4);
	state1 = _mm256_xor_si256(state1, state3);
	state2 = _mm256_xor_si256(t5, t6);
	state4 = _mm256_xor_si256(t7, t8);
	state2 = _mm256_xor_si256(state2, state4);

	state1 = _mm256_slli_epi16(state1, 4);
	state1 = _mm256_xor_si256(state1, state2);




	_mm256_storeu_si256((__m256i*)h, state1);




	state1 = u2_x;
	state2 = u2_y;
	state3 = _mm256_permute2x128_si256(state1, state1, 1);
	state4 = _mm256_permute2x128_si256(state2, state2, 1);
	t1 = _mm256_shuffle_epi8(state1, c5);
	t2 = _mm256_shuffle_epi8(state3, c7);
	t3 = _mm256_shuffle_epi8(state4, c55);
	t4 = _mm256_shuffle_epi8(state2, c77);
	t5 = _mm256_shuffle_epi8(state1, c6);
	t6 = _mm256_shuffle_epi8(state3, c8);
	t7 = _mm256_shuffle_epi8(state4, c66);
	t8 = _mm256_shuffle_epi8(state2, c88);

	state1 = _mm256_xor_si256(t1, t2);
	state3 = _mm256_xor_si256(t3, t4);
	state1 = _mm256_xor_si256(state1, state3);
	state2 = _mm256_xor_si256(t5, t6);
	state4 = _mm256_xor_si256(t7, t8);
	state2 = _mm256_xor_si256(state2, state4);

	state1 = _mm256_slli_epi16(state1, 4);
	state1 = _mm256_xor_si256(state1, state2);


	_mm256_storeu_si256((__m256i*)(h + 32), state1);

	return 0;



}



int uHash_768(const unsigned char* msg, unsigned long long msg_len_bits, unsigned char* h)
{
	//共有in_len块M=m0||m1=(0x0a,....)
	__m256i IV768_1_x = _mm256_setr_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	__m256i IV768_1_y = _mm256_setr_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x33, 0, 0);

	__m256i IV768_2_x = _mm256_setr_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	__m256i IV768_2_y = _mm256_setr_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

	__m256i IV768_3_x = _mm256_setr_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x3, 0, 0);
	__m256i IV768_3_y = _mm256_setr_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	__m256i con = _mm256_set1_epi8(0x0f);//取右4-bit
	__m256i c5 = _mm256_setr_epi8(0, 2, 4, 6, 8, 10, 12, 14, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
	__m256i c6 = _mm256_setr_epi8(1, 3, 5, 7, 9, 11, 13, 15, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
	__m256i c7 = _mm256_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0, 2, 4, 6, 8, 10, 12, 14, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
	__m256i c8 = _mm256_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 1, 3, 5, 7, 9, 11, 13, 15, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
	__m256i c55 = _mm256_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0, 2, 4, 6, 8, 10, 12, 14, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
	__m256i c66 = _mm256_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 1, 3, 5, 7, 9, 11, 13, 15, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
	__m256i c77 = _mm256_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0, 2, 4, 6, 8, 10, 12, 14);
	__m256i c88 = _mm256_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 1, 3, 5, 7, 9, 11, 13, 15);


	int i;
	int Num_Block;
	Num_Block = 2;
	unsigned long long row, col, l;
	unsigned char M[2 * BlockSize] = { 0 };//The message after padding
	unsigned long long actual_len_bytes, msg_len_bytes, msg_len_bytes_col, msg_len_bytes_row;
	unsigned long long AN_bits = Num_Block * BlockSize * 8;
	unsigned long long AN_byte = Num_Block * BlockSize;
	row = (int)(msg_len_bits / AN_bits) + 1;
	row = AN_bits * row;
	actual_len_bytes = (int)(row / 8);


	msg_len_bytes = (int)(msg_len_bits / 8);

	//Padding Optimal
	msg_len_bytes_col = msg_len_bits % 8;
	//msg_len_bytes_row = msg_len_bytes - msg_len_bytes_col;
	msg_len_bytes_row = msg_len_bytes - (msg_len_bytes % AN_byte);

	if (msg_len_bytes_col == 0)
		M[msg_len_bytes % AN_byte] = 0x80;
	else
	{
		M[msg_len_bytes % AN_byte] = msg[msg_len_bytes] + (1 << (7 - msg_len_bytes_col));
	}
	for (int tt = 0; tt < msg_len_bytes % AN_byte; tt++)
		M[tt] = msg[msg_len_bytes_row + tt];


	l = (int)(actual_len_bytes / (Num_Block * BlockSize));//The number of blocks of message M
	__m256i u1_x, u1_y, u2_x, u2_y, u3_x, u3_y;//u1=(0xaa,...),u2,u3=(0x0a,...)
	__m256i state3, state4, state1, state2, t1, t2, t3, t4, t5, t6, t7, t8, SSstate3, SSstate4, SSstate1, SSstate2;

	u1_x = IV768_1_x;
	u1_y = IV768_1_y;

	u2_x = IV768_2_x;
	u2_y = IV768_2_y;

	u3_x = IV768_3_x;
	u3_y = IV768_3_y;

	for (i = 0; i < l - 1; i++)
	{
		CF_768(msg + Num_Block * 32 * i, &u1_x, &u1_y, &u2_x, &u2_y, &u3_x, &u3_y, ROUND);
	}
	CF_768_Final(M, &u1_x, &u1_y, &u2_x, &u2_y, &u3_x, &u3_y, ROUND);

	//整合输出
	state1 = _mm256_and_si256(u1_x, con);
	state2 = _mm256_and_si256(u1_y, con);
	state3 = _mm256_permute2x128_si256(state1, state1, 1);
	state4 = _mm256_permute2x128_si256(state2, state2, 1);
	t1 = _mm256_shuffle_epi8(state1, c5);
	t2 = _mm256_shuffle_epi8(state3, c7);
	t3 = _mm256_shuffle_epi8(state4, c55);
	t4 = _mm256_shuffle_epi8(state2, c77);
	t5 = _mm256_shuffle_epi8(state1, c6);
	t6 = _mm256_shuffle_epi8(state3, c8);
	t7 = _mm256_shuffle_epi8(state4, c66);
	t8 = _mm256_shuffle_epi8(state2, c88);

	state1 = _mm256_xor_si256(t1, t2);
	state3 = _mm256_xor_si256(t3, t4);
	state1 = _mm256_xor_si256(state1, state3);
	state2 = _mm256_xor_si256(t5, t6);
	state4 = _mm256_xor_si256(t7, t8);
	state2 = _mm256_xor_si256(state2, state4);

	state1 = _mm256_slli_epi16(state1, 4);
	state1 = _mm256_xor_si256(state1, state2);




	_mm256_storeu_si256((__m256i*)h, state1);




	state1 = u2_x;
	state2 = u2_y;
	state3 = _mm256_permute2x128_si256(state1, state1, 1);
	state4 = _mm256_permute2x128_si256(state2, state2, 1);
	t1 = _mm256_shuffle_epi8(state1, c5);
	t2 = _mm256_shuffle_epi8(state3, c7);
	t3 = _mm256_shuffle_epi8(state4, c55);
	t4 = _mm256_shuffle_epi8(state2, c77);
	t5 = _mm256_shuffle_epi8(state1, c6);
	t6 = _mm256_shuffle_epi8(state3, c8);
	t7 = _mm256_shuffle_epi8(state4, c66);
	t8 = _mm256_shuffle_epi8(state2, c88);

	state1 = _mm256_xor_si256(t1, t2);
	state3 = _mm256_xor_si256(t3, t4);
	state1 = _mm256_xor_si256(state1, state3);
	state2 = _mm256_xor_si256(t5, t6);
	state4 = _mm256_xor_si256(t7, t8);
	state2 = _mm256_xor_si256(state2, state4);

	state1 = _mm256_slli_epi16(state1, 4);
	state1 = _mm256_xor_si256(state1, state2);


	_mm256_storeu_si256((__m256i*)(h + 32), state1);



	state1 = u3_x;
	state2 = u3_y;
	state3 = _mm256_permute2x128_si256(state1, state1, 1);
	state4 = _mm256_permute2x128_si256(state2, state2, 1);
	t1 = _mm256_shuffle_epi8(state1, c5);
	t2 = _mm256_shuffle_epi8(state3, c7);
	t3 = _mm256_shuffle_epi8(state4, c55);
	t4 = _mm256_shuffle_epi8(state2, c77);
	t5 = _mm256_shuffle_epi8(state1, c6);
	t6 = _mm256_shuffle_epi8(state3, c8);
	t7 = _mm256_shuffle_epi8(state4, c66);
	t8 = _mm256_shuffle_epi8(state2, c88);

	state1 = _mm256_xor_si256(t1, t2);
	state3 = _mm256_xor_si256(t3, t4);
	state1 = _mm256_xor_si256(state1, state3);
	state2 = _mm256_xor_si256(t5, t6);
	state4 = _mm256_xor_si256(t7, t8);
	state2 = _mm256_xor_si256(state2, state4);

	state1 = _mm256_slli_epi16(state1, 4);
	state1 = _mm256_xor_si256(state1, state2);


	_mm256_storeu_si256((__m256i*)(h + 64), state1);

	return 0;

}

int uHash_1024(const unsigned char* msg, unsigned long long msg_len_bits, unsigned char* h)
{
	//共有in_len块M=m0||m1=(0x0a,....)

	__m256i IV1024_1_x = _mm256_setr_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	__m256i IV1024_1_y = _mm256_setr_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x44, 0, 0);

	__m256i IV1024_2_x = _mm256_setr_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	__m256i IV1024_2_y = _mm256_setr_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

	__m256i IV1024_3_x = _mm256_setr_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x4, 0, 0);
	__m256i IV1024_3_y = _mm256_setr_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

	__m256i IV1024_4_x = _mm256_setr_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	__m256i IV1024_4_y = _mm256_setr_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	__m256i con = _mm256_set1_epi8(0x0f);//取右4-bit
	__m256i c5 = _mm256_setr_epi8(0, 2, 4, 6, 8, 10, 12, 14, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
	__m256i c6 = _mm256_setr_epi8(1, 3, 5, 7, 9, 11, 13, 15, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
	__m256i c7 = _mm256_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0, 2, 4, 6, 8, 10, 12, 14, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
	__m256i c8 = _mm256_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 1, 3, 5, 7, 9, 11, 13, 15, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
	__m256i c55 = _mm256_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0, 2, 4, 6, 8, 10, 12, 14, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
	__m256i c66 = _mm256_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 1, 3, 5, 7, 9, 11, 13, 15, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
	__m256i c77 = _mm256_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0, 2, 4, 6, 8, 10, 12, 14);
	__m256i c88 = _mm256_setr_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 1, 3, 5, 7, 9, 11, 13, 15);



	int i;
	int Num_Block;
	Num_Block = 1;
	unsigned long long row, col, l;
	//unsigned char* M;//The message after padding
	unsigned char M[BlockSize] = { 0 };
	unsigned long long actual_len_bytes, msg_len_bytes, msg_len_bytes_col, msg_len_bytes_row;
	unsigned long long AN_bits = Num_Block * BlockSize * 8;
	unsigned long long AN_byte = Num_Block * BlockSize;
	row = (int)(msg_len_bits / AN_bits) + 1;
	row = AN_bits * row;
	actual_len_bytes = (int)(row / 8);
	msg_len_bytes = (int)(msg_len_bits / 8);


	//Padding Optimal
	msg_len_bytes_col = msg_len_bits % 8;
	//msg_len_bytes_row = msg_len_bytes - msg_len_bytes_col;
	msg_len_bytes_row = msg_len_bytes - (msg_len_bytes % AN_byte);

	if (msg_len_bytes_col == 0)
		M[msg_len_bytes % AN_byte] = 0x80;
	else
	{
		M[msg_len_bytes % AN_byte] = msg[msg_len_bytes] + (1 << (7 - msg_len_bytes_col));
	}
	for (int tt = 0; tt < msg_len_bytes % AN_byte; tt++)
		M[tt] = msg[msg_len_bytes_row + tt];


	//Padding(msg, M, msg_len_bits, Num_Block);




	l = (int)(actual_len_bytes / (Num_Block * BlockSize));//The number of blocks of message M
	__m256i u1_x, u1_y, u2_x, u2_y, u3_x, u3_y, u4_x, u4_y;//u1=(0xaa,...),u2,u3=(0x0a,...)
	__m256i state3, state4, state1, state2, t1, t2, t3, t4, t5, t6, t7, t8;

	u1_x = IV1024_1_x;
	u1_y = IV1024_1_y;;
	u2_x = IV1024_2_x;
	u2_y = IV1024_2_y;
	u3_x = IV1024_3_x;
	u3_y = IV1024_3_y;;
	u4_x = IV1024_4_x;
	u4_y = IV1024_4_y;

	for (i = 0; i < l - 1; i++)
	{
		CF_1024(msg + Num_Block * 32 * i, &u1_x, &u1_y, &u2_x, &u2_y, &u3_x, &u3_y, &u4_x, &u4_y, ROUND);
	}
	CF_1024_Final(M, &u1_x, &u1_y, &u2_x, &u2_y, &u3_x, &u3_y, &u4_x, &u4_y, ROUND);

	//整合输出
	state1 = _mm256_and_si256(u1_x, con);
	state2 = _mm256_and_si256(u1_y, con);
	state3 = _mm256_permute2x128_si256(state1, state1, 1);
	state4 = _mm256_permute2x128_si256(state2, state2, 1);
	t1 = _mm256_shuffle_epi8(state1, c5);
	t2 = _mm256_shuffle_epi8(state3, c7);
	t3 = _mm256_shuffle_epi8(state4, c55);
	t4 = _mm256_shuffle_epi8(state2, c77);
	t5 = _mm256_shuffle_epi8(state1, c6);
	t6 = _mm256_shuffle_epi8(state3, c8);
	t7 = _mm256_shuffle_epi8(state4, c66);
	t8 = _mm256_shuffle_epi8(state2, c88);

	state1 = _mm256_xor_si256(t1, t2);
	state3 = _mm256_xor_si256(t3, t4);
	state1 = _mm256_xor_si256(state1, state3);
	state2 = _mm256_xor_si256(t5, t6);
	state4 = _mm256_xor_si256(t7, t8);
	state2 = _mm256_xor_si256(state2, state4);

	state1 = _mm256_slli_epi16(state1, 4);
	state1 = _mm256_xor_si256(state1, state2);




	_mm256_storeu_si256((__m256i*)h, state1);




	state1 = u2_x;
	state2 = u2_y;
	state3 = _mm256_permute2x128_si256(state1, state1, 1);
	state4 = _mm256_permute2x128_si256(state2, state2, 1);
	t1 = _mm256_shuffle_epi8(state1, c5);
	t2 = _mm256_shuffle_epi8(state3, c7);
	t3 = _mm256_shuffle_epi8(state4, c55);
	t4 = _mm256_shuffle_epi8(state2, c77);
	t5 = _mm256_shuffle_epi8(state1, c6);
	t6 = _mm256_shuffle_epi8(state3, c8);
	t7 = _mm256_shuffle_epi8(state4, c66);
	t8 = _mm256_shuffle_epi8(state2, c88);

	state1 = _mm256_xor_si256(t1, t2);
	state3 = _mm256_xor_si256(t3, t4);
	state1 = _mm256_xor_si256(state1, state3);
	state2 = _mm256_xor_si256(t5, t6);
	state4 = _mm256_xor_si256(t7, t8);
	state2 = _mm256_xor_si256(state2, state4);

	state1 = _mm256_slli_epi16(state1, 4);
	state1 = _mm256_xor_si256(state1, state2);


	_mm256_storeu_si256((__m256i*)(h + 32), state1);



	state1 = u3_x;
	state2 = u3_y;
	state3 = _mm256_permute2x128_si256(state1, state1, 1);
	state4 = _mm256_permute2x128_si256(state2, state2, 1);
	t1 = _mm256_shuffle_epi8(state1, c5);
	t2 = _mm256_shuffle_epi8(state3, c7);
	t3 = _mm256_shuffle_epi8(state4, c55);
	t4 = _mm256_shuffle_epi8(state2, c77);
	t5 = _mm256_shuffle_epi8(state1, c6);
	t6 = _mm256_shuffle_epi8(state3, c8);
	t7 = _mm256_shuffle_epi8(state4, c66);
	t8 = _mm256_shuffle_epi8(state2, c88);

	state1 = _mm256_xor_si256(t1, t2);
	state3 = _mm256_xor_si256(t3, t4);
	state1 = _mm256_xor_si256(state1, state3);
	state2 = _mm256_xor_si256(t5, t6);
	state4 = _mm256_xor_si256(t7, t8);
	state2 = _mm256_xor_si256(state2, state4);

	state1 = _mm256_slli_epi16(state1, 4);
	state1 = _mm256_xor_si256(state1, state2);


	_mm256_storeu_si256((__m256i*)(h + 64), state1);


	state1 = u4_x;
	state2 = u4_y;
	state3 = _mm256_permute2x128_si256(state1, state1, 1);
	state4 = _mm256_permute2x128_si256(state2, state2, 1);
	t1 = _mm256_shuffle_epi8(state1, c5);
	t2 = _mm256_shuffle_epi8(state3, c7);
	t3 = _mm256_shuffle_epi8(state4, c55);
	t4 = _mm256_shuffle_epi8(state2, c77);
	t5 = _mm256_shuffle_epi8(state1, c6);
	t6 = _mm256_shuffle_epi8(state3, c8);
	t7 = _mm256_shuffle_epi8(state4, c66);
	t8 = _mm256_shuffle_epi8(state2, c88);

	state1 = _mm256_xor_si256(t1, t2);
	state3 = _mm256_xor_si256(t3, t4);
	state1 = _mm256_xor_si256(state1, state3);
	state2 = _mm256_xor_si256(t5, t6);
	state4 = _mm256_xor_si256(t7, t8);
	state2 = _mm256_xor_si256(state2, state4);

	state1 = _mm256_slli_epi16(state1, 4);
	state1 = _mm256_xor_si256(state1, state2);


	_mm256_storeu_si256((__m256i*)(h + 96), state1);


	return 0;



}




int CryptHash(int digest_len_bits, const unsigned char* msg, unsigned long long msg_len_bits, unsigned char* digest)
{

	if (digest_len_bits == 512)
		uHash_512(msg, msg_len_bits, digest);
	if (digest_len_bits == 768)
		uHash_768(msg, msg_len_bits, digest);
	if (digest_len_bits == 1024)
		uHash_1024(msg, msg_len_bits, digest);
	return 0;
}


//////static volatile int force_no_opt = 1;
////
//
//int main()
//{
//	unsigned char msg[129] = { 0x30 };
//	unsigned long long msg_len_bits = 1024;
//	unsigned char digest[128] = { 0 };
//	unsigned long long dig_len = 1024;
//
//	//
//	// uhash_512(msg, msg_len_bits, digest);
//	CryptHash(dig_len, msg, msg_len_bits, digest);
//
//	//************check code********************
//	printf("plaintext:::\n");
//	for (int i = 0; i < (int)msg_len_bits / 8 + 1; i++)
//		printf("%02x ", msg[i]);
//	printf("\n");
//
//	printf("digest:::\n");
//	for (int i = 0; i < (int)dig_len / 8; i++)
//		printf("%02x ", digest[i]);
//	printf("\n");
//	//************check code********************
//
//
//
//
//
//
//	return 0;
//
//
//}
//


