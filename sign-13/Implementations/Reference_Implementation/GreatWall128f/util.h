#ifndef UTIL_H
#define UTIL_H

#include <assert.h>
#include <stddef.h>
#include <inttypes.h>
#include <string.h>

#ifdef _MSC_VER
#include "intrin.h"
#endif

#if defined(__GNUC__)
#define ALWAYS_INLINE inline __attribute__ ((__always_inline__))
#elif defined(_MSC_VER)
#define ALWAYS_INLINE __forceinline
#else
#define ALWAYS_INLINE inline
#endif

#define STRINGIZE_NO_EXPAND(x) #x
#define STRINGIZE(x) STRINGIZE_NO_EXPAND(x)


inline unsigned int count_trailing_zeros(uint64_t x)
{
#if defined(__GNUC__) || defined(__clang__)
	return __builtin_ctzll(x);
#elif defined(_MSC_VER)
	unsigned long result;
	_BitScanForward64(&result, x);
	return result;
#endif

	for (unsigned int i = 0; i < 64; ++i, x >>= 1)
		if (x & 1)
			return i;
	return 64;
}

inline uint8_t expand_bit_to_byte(unsigned long x, unsigned int i)
{
	return -((x >> i) & 1);
}

inline void expand_bits_to_bytes(uint8_t* output, size_t num_bits, size_t x)
{
    assert(num_bits <= sizeof(x) * 8);
    for (size_t i = 0; i < num_bits; ++i)
        output[i] = expand_bit_to_byte(x, i);
}

ALWAYS_INLINE size_t rotate_left(size_t x, unsigned int shift, unsigned int n_bits)
{
	shift %= n_bits;
	size_t mask = ((size_t) 1 << n_bits) - 1;
	return ((x << shift) & mask) + ((x & mask) >> (n_bits - shift));
}

ALWAYS_INLINE size_t rotate_right(size_t x, unsigned int shift, unsigned int n_bits)
{
	shift %= n_bits;
	size_t mask = ((size_t) 1 << n_bits) - 1;
	return ((x << (n_bits - shift)) & mask) + ((x & mask) >> shift);
}

void printHex(const char* s, const uint8_t* data, size_t len);

#endif
