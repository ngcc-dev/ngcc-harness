#include <string.h>
#include <stdint.h>
#include "avx2_neon.h"
#include "params.h"
/*************************************************
* Name:        verify
* 
* Description: Compare two arrays for equality in constant time.
*
* Arguments:   const uint8_t *a: pointer to first byte array
*              const uint8_t *b: pointer to second byte array
*              size_t len:             length of the byte arrays
*
* Returns 0 if the byte arrays are equal, 1 otherwise
**************************************************/
int verify(const uint8_t *a, const uint8_t *b, size_t len)
{
  uint64_t r;
  size_t i;
  r = 0;
  
  for(i=0;i<len;i++)
    r |= a[i] ^ b[i];

  r = (0-r) >> 63;
  return r;
}

int verify32(const uint8_t *a, const uint8_t *b, size_t len)
{
	
	int i;
	__m256i ta, tb, r;
	
	ta = _mm256_load_si256((__m256i*)a);
	tb = _mm256_load_si256((__m256i*)b);
	r = _mm256_xor_si256(ta, tb);
	for (i = 1; i < len/32; i++)
	{ 
		ta = _mm256_load_si256((__m256i*)&a[32 * i]);
		tb = _mm256_load_si256((__m256i*)&b[32 * i]);
		ta = _mm256_xor_si256(ta,tb);
		r = _mm256_or_si256(r, ta);
		
	}
	return 1 - _mm256_testz_si256(r,r);
}

/*************************************************
* Name:        cmov
* 
* Description: Copy len bytes from x to r if b is 1;
*              don't modify r if b is 0. Requires b to be in {0,1};
*              assumes two's complement representation of negative integers.
*              Runs in constant time.
*
* Arguments:   uint8_t *r:       pointer to output byte array
*              const uint8_t *x: pointer to input byte array
*              size_t len:             Amount of bytes to be copied
*              uint8_t b:        Condition bit; has to be in {0,1}
**************************************************/
void cmov(uint8_t *r, const uint8_t *x, size_t len, uint8_t b)
{
  size_t i;

  b = -b;
  for(i=0;i<len;i++)
    r[i] ^= b & (x[i] ^ r[i]);
}

void cmov32(uint8_t *r, const uint8_t *x,uint8_t b)
{
#if MSG_BYTES != 16 && MSG_BYTES != 32 && MSG_BYTES != 64
#error "this function only supports conditional move 128, 256, 512 bits"
#endif
	int i;

#if MSG_BYTES == 16
	__m128i t0, t1;
	__m128i mask = _mm_set1_epi8(-b);

	t0 = _mm_load_si128((__m128i*)x);
	t1 = _mm_load_si128((__m128i*)r);
	t0 = _mm_xor_si128(t0, t1);
	t0 = _mm_and_si128(t0, mask);
	t1 = _mm_xor_si128(t0, t1);
	_mm_storeu_si128((__m128i*)r, t1);
#elif MSG_BYTES == 32
	__m256i t0, t1;
	__m256i mask = _mm256_set1_epi8(-b);

	t0 = _mm256_load_si256((__m256i*)x);
	t1 = _mm256_load_si256((__m256i*)r);
	t0 = _mm256_xor_si256(t0, t1);
	t0 = _mm256_and_si256(t0, mask);
	t1 = _mm256_xor_si256(t0, t1);
	_mm256_storeu_si256((__m256i *)r, t1);
#elif MSG_BYTES == 64
	__m256i t0, t1;
	__m256i mask = _mm256_set1_epi8(-b);

	t0 = _mm256_load_si256((__m256i*)x);
	t1 = _mm256_load_si256((__m256i*)r);
	t0 = _mm256_xor_si256(t0, t1);
	t0 = _mm256_and_si256(t0, mask);
	t1 = _mm256_xor_si256(t0, t1);
	_mm256_storeu_si256((__m256i*)r, t1);

	t0 = _mm256_load_si256((__m256i*) & x[32]);
	t1 = _mm256_load_si256((__m256i*)&r[32]);
	t0 = _mm256_xor_si256(t0, t1);
	t0 = _mm256_and_si256(t0, mask);
	t1 = _mm256_xor_si256(t0, t1);
	_mm256_storeu_si256((__m256i*)&r[32], t1);
#endif
}
