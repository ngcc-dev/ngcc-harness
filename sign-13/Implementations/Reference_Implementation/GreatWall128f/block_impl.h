#ifndef BLOCK_IMPL_REFERENCE_H
#define BLOCK_IMPL_REFERENCE_H

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

typedef struct { uint64_t data[2]; } block128;
typedef struct { uint64_t data[4]; } block256;

typedef struct { block128 data[3]; } block384;
typedef struct { block256 data[2]; } block512;
typedef struct { block256 data[4]; } block1024;

#if defined(__GNUC__) || defined(__clang__)
#define REF_PARITY64(x) ((unsigned int)__builtin_parityll((uint64_t)(x)))
#else
#define REF_PARITY64(x) ref_parity64_fallback((uint64_t)(x))
static inline unsigned int ref_parity64_fallback(uint64_t x)
{
	x ^= x >> 32;
	x ^= x >> 16;
	x ^= x >> 8;
	x ^= x >> 4;
	x &= 0xf;
	return (0x6996u >> x) & 1u;
}
#endif

#define REF_REPEAT_BYTE(x) ((uint64_t)(uint8_t)(x) * UINT64_C(0x0101010101010101))
#define REF_CLMUL64(out_, a_, b_)                                                     \
	do {                                                                              \
		uint64_t ref_a_ = (a_);                                                       \
		uint64_t ref_b_ = (b_);                                                       \
		(out_)[0] = 0;                                                                \
		(out_)[1] = 0;                                                                \
		for (unsigned int ref_i_ = 0; ref_i_ < 64; ++ref_i_) {                        \
			const uint64_t ref_mask_ = UINT64_C(0) - ((ref_b_ >> ref_i_) & 1u);        \
			(out_)[0] ^= (ref_a_ << ref_i_) & ref_mask_;                              \
			if (ref_i_)                                                               \
				(out_)[1] ^= (ref_a_ >> (64 - ref_i_)) & ref_mask_;                   \
		}                                                                             \
	} while (0)

static inline void ref_shift_words_left(uint64_t* out, size_t out_words,
                                        const uint64_t* in, size_t in_words,
                                        size_t shift)
{
	const size_t word_shift = shift >> 6;
	const unsigned int bit_shift = shift & 63;
	for (size_t i = 0; i < in_words; ++i) {
		if (word_shift + i < out_words)
			out[word_shift + i] ^= in[i] << bit_shift;
		if (bit_shift && word_shift + i + 1 < out_words)
			out[word_shift + i + 1] ^= in[i] >> (64 - bit_shift);
	}
}

static inline bool ref_get_bit(const uint64_t* words, size_t bit)
{
	return (words[bit >> 6] >> (bit & 63)) & 1u;
}

static inline void ref_flip_bit(uint64_t* words, size_t bit)
{
	words[bit >> 6] ^= UINT64_C(1) << (bit & 63);
}

static inline void ref_reduce_poly(uint64_t* p, int high_bit, int degree,
                                   const int* taps, size_t num_taps)
{
	for (int bit = high_bit; bit >= degree; --bit) {
		if (!ref_get_bit(p, (size_t)bit))
			continue;
		ref_flip_bit(p, (size_t)bit);
		for (size_t i = 0; i < num_taps; ++i)
			ref_flip_bit(p, (size_t)(bit - degree + taps[i]));
	}
}

static inline void ref_gf_mul(uint64_t* out, size_t out_words,
                              const uint64_t* a, const uint64_t* b,
                              int degree, const int* taps, size_t num_taps)
{
	uint64_t product[18] = {0};
	const size_t words = (degree + 63u) / 64u;
	for (size_t word = 0; word < words; ++word) {
		unsigned int limit = 64;
		if (word == words - 1)
			limit = (unsigned int)(degree - (int)word * 64);
		for (unsigned int bit = 0; bit < limit; ++bit) {
			if ((b[word] >> bit) & 1u)
				ref_shift_words_left(product, 18, a, words, word * 64 + bit);
		}
	}
	ref_reduce_poly(product, 2 * degree - 2, degree, taps, num_taps);
	memcpy(out, product, out_words * sizeof(uint64_t));
	const unsigned int top_bits = (unsigned int)(degree & 63);
	if (top_bits)
		out[out_words - 1] &= (UINT64_C(1) << top_bits) - 1;
}

inline block128 block128_xor(block128 x, block128 y)
{
	block128 out = {{x.data[0] ^ y.data[0], x.data[1] ^ y.data[1]}};
	return out;
}
inline block256 block256_xor(block256 x, block256 y)
{
	block256 out;
	for (size_t i = 0; i < 4; ++i)
		out.data[i] = x.data[i] ^ y.data[i];
	return out;
}
inline block128 block128_and(block128 x, block128 y)
{
	block128 out = {{x.data[0] & y.data[0], x.data[1] & y.data[1]}};
	return out;
}
inline block256 block256_and(block256 x, block256 y)
{
	block256 out;
	for (size_t i = 0; i < 4; ++i)
		out.data[i] = x.data[i] & y.data[i];
	return out;
}
inline block128 block128_set_zero() { block128 out = {{0, 0}}; return out; }
inline block256 block256_set_zero() { block256 out = {{0, 0, 0, 0}}; return out; }
inline block128 block128_set_all_8(uint8_t x)
{
	const uint64_t y = REF_REPEAT_BYTE(x);
	block128 out = {{y, y}};
	return out;
}
inline block256 block256_set_all_8(uint8_t x)
{
	const uint64_t y = REF_REPEAT_BYTE(x);
	block256 out = {{y, y, y, y}};
	return out;
}
inline block128 block128_set_low32(uint32_t x) { block128 out = {{x, 0}}; return out; }
inline block256 block256_set_low32(uint32_t x) { block256 out = {{x, 0, 0, 0}}; return out; }
inline block128 block128_set_low64(uint64_t x) { block128 out = {{x, 0}}; return out; }
inline block256 block256_set_low64(uint64_t x) { block256 out = {{x, 0, 0, 0}}; return out; }
inline block256 block256_set_128(block128 x0, block128 x1)
{
	block256 out = {{x0.data[0], x0.data[1], x1.data[0], x1.data[1]}};
	return out;
}
inline block256 block256_set_low128(block128 x)
{
	block256 out = {{x.data[0], x.data[1], 0, 0}};
	return out;
}

inline block384 block384_xor(block384 x, block384 y)
{
	block384 out;
	for (size_t i = 0; i < 3; ++i)
		out.data[i] = block128_xor(x.data[i], y.data[i]);
	return out;
}
inline block512 block512_xor(block512 x, block512 y)
{
	block512 out;
	for (size_t i = 0; i < 2; ++i)
		out.data[i] = block256_xor(x.data[i], y.data[i]);
	return out;
}
inline block1024 block1024_xor(block1024 x, block1024 y)
{
	block1024 out;
	for (size_t i = 0; i < 4; ++i)
		out.data[i] = block256_xor(x.data[i], y.data[i]);
	return out;
}
inline block384 block384_and(block384 x, block384 y)
{
	block384 out;
	for (size_t i = 0; i < 3; ++i)
		out.data[i] = block128_and(x.data[i], y.data[i]);
	return out;
}
inline block512 block512_and(block512 x, block512 y)
{
	block512 out;
	for (size_t i = 0; i < 2; ++i)
		out.data[i] = block256_and(x.data[i], y.data[i]);
	return out;
}
inline block1024 block1024_and(block1024 x, block1024 y)
{
	block1024 out;
	for (size_t i = 0; i < 4; ++i)
		out.data[i] = block256_and(x.data[i], y.data[i]);
	return out;
}
inline block384 block384_set_zero() { block384 out = {{{{0, 0}}, {{0, 0}}, {{0, 0}}}}; return out; }
inline block512 block512_set_zero() { block512 out = {{{{0, 0, 0, 0}}, {{0, 0, 0, 0}}}}; return out; }
inline block1024 block1024_set_zero() { block1024 out = {{{{0, 0, 0, 0}}, {{0, 0, 0, 0}}, {{0, 0, 0, 0}}, {{0, 0, 0, 0}}}}; return out; }
inline block384 block384_set_all_8(uint8_t x)
{
	block384 out;
	for (size_t i = 0; i < 3; ++i)
		out.data[i] = block128_set_all_8(x);
	return out;
}
inline block512 block512_set_all_8(uint8_t x)
{
	block512 out;
	for (size_t i = 0; i < 2; ++i)
		out.data[i] = block256_set_all_8(x);
	return out;
}
inline block1024 block1024_set_all_8(uint8_t x)
{
	block1024 out;
	for (size_t i = 0; i < 4; ++i)
		out.data[i] = block256_set_all_8(x);
	return out;
}
inline block384 block384_set_low32(uint32_t x) { block384 out = block384_set_zero(); out.data[0] = block128_set_low32(x); return out; }
inline block512 block512_set_low32(uint32_t x) { block512 out = block512_set_zero(); out.data[0] = block256_set_low32(x); return out; }
inline block1024 block1024_set_low32(uint32_t x) { block1024 out = block1024_set_zero(); out.data[0] = block256_set_low32(x); return out; }
inline block384 block384_set_low64(uint64_t x) { block384 out = block384_set_zero(); out.data[0] = block128_set_low64(x); return out; }
inline block512 block512_set_low64(uint64_t x) { block512 out = block512_set_zero(); out.data[0] = block256_set_low64(x); return out; }
inline block1024 block1024_set_low64(uint64_t x) { block1024 out = block1024_set_zero(); out.data[0] = block256_set_low64(x); return out; }

inline bool block128_any_zeros(block128 x)
{
	const uint8_t* p = (const uint8_t*)&x;
	for (size_t i = 0; i < sizeof(x); ++i)
		if (p[i] == 0)
			return true;
	return false;
}
inline bool block192_any_zeros(block192 x)
{
	const uint8_t* p = (const uint8_t*)&x;
	for (size_t i = 0; i < sizeof(x); ++i)
		if (p[i] == 0)
			return true;
	return false;
}
inline bool block256_any_zeros(block256 x)
{
	const uint8_t* p = (const uint8_t*)&x;
	for (size_t i = 0; i < sizeof(x); ++i)
		if (p[i] == 0)
			return true;
	return false;
}
inline bool block512_any_zeros(block512 x)
{
	const uint8_t* p = (const uint8_t*)&x;
	for (size_t i = 0; i < sizeof(x); ++i)
		if (p[i] == 0)
			return true;
	return false;
}
inline block128 block128_byte_reverse(block128 x)
{
	block128 out;
	const uint8_t* in = (const uint8_t*)&x;
	uint8_t* dst = (uint8_t*)&out;
	for (size_t i = 0; i < sizeof(out); ++i)
		dst[i] = in[sizeof(out) - 1 - i];
	return out;
}
inline block256 block256_from_2_block128(block128 x, block128 y)
{
	return block256_set_128(x, y);
}

#define VOLE_BLOCK_SHIFT 0
typedef block128 vole_block;
inline vole_block vole_block_set_zero() { return block128_set_zero(); }
inline vole_block vole_block_xor(vole_block x, vole_block y) { return block128_xor(x, y); }
inline vole_block vole_block_and(vole_block x, vole_block y) { return block128_and(x, y); }
inline vole_block vole_block_set_all_8(uint8_t x) { return block128_set_all_8(x); }
inline vole_block vole_block_set_low32(uint32_t x) { return block128_set_low32(x); }
inline vole_block vole_block_set_low64(uint64_t x) { return block128_set_low64(x); }

#define POLY_VEC_LEN_SHIFT 0
typedef block128 clmul_block;
inline clmul_block clmul_block_xor(clmul_block x, clmul_block y) { return block128_xor(x, y); }
inline clmul_block clmul_block_and(clmul_block x, clmul_block y) { return block128_and(x, y); }
inline clmul_block clmul_block_set_all_8(uint8_t x) { return block128_set_all_8(x); }
inline clmul_block clmul_block_set_zero() { return block128_set_zero(); }
inline clmul_block clmul_block_clmul_ll(clmul_block x, clmul_block y)
{
	clmul_block out;
	REF_CLMUL64(out.data, x.data[0], y.data[0]);
	return out;
}
inline clmul_block clmul_block_clmul_lh(clmul_block x, clmul_block y)
{
	clmul_block out;
	REF_CLMUL64(out.data, x.data[0], y.data[1]);
	return out;
}
inline clmul_block clmul_block_clmul_hl(clmul_block x, clmul_block y)
{
	clmul_block out;
	REF_CLMUL64(out.data, x.data[1], y.data[0]);
	return out;
}
inline clmul_block clmul_block_clmul_hh(clmul_block x, clmul_block y)
{
	clmul_block out;
	REF_CLMUL64(out.data, x.data[1], y.data[1]);
	return out;
}
inline clmul_block clmul_block_shift_left_64(clmul_block x)
{
	clmul_block out = {{0, x.data[0]}};
	return out;
}
inline clmul_block clmul_block_shift_right_64(clmul_block x)
{
	clmul_block out = {{x.data[1], 0}};
	return out;
}
inline clmul_block clmul_block_mix_64(clmul_block x, clmul_block y)
{
	clmul_block out = {{y.data[1], x.data[0]}};
	return out;
}
inline clmul_block clmul_block_broadcast_low64(clmul_block x)
{
	clmul_block out = {{x.data[0], x.data[0]}};
	return out;
}

static inline void block128_sqr(block128* out, const block128* in)
{
	static const int taps[] = {0, 1, 2, 7};
	ref_gf_mul(out->data, 2, in->data, in->data, 128, taps, 4);
}
static inline void block128_mul(block128* out, const block128* a, const block128* b)
{
	static const int taps[] = {0, 1, 2, 7};
	ref_gf_mul(out->data, 2, a->data, b->data, 128, taps, 4);
}
static inline void block128_pow_size_t(block128* block, size_t exponent)
{
	block128 base = *block;
	block128 result = block128_set_low32(1);
	while (exponent) {
		if (exponent & 1)
			block128_mul(&result, &result, &base);
		block128_sqr(&base, &base);
		exponent >>= 1;
	}
	*block = result;
}
inline void block128_inverse(block128* block) { (void)block; assert(0 && "block128_inverse unused in GreatWall128f reference"); }
inline void block128_mersenne_inverse(block128* block) { (void)block; assert(0 && "block128_mersenne_inverse unused in GreatWall128f reference"); }
inline void block128_multiply_with_GF2_matrix(block128* block, const uint64_t* matrix)
{
	block128 out = block128_set_zero();
	for (size_t row = 0; row < 128; ++row) {
		const uint64_t bit =
			(REF_PARITY64(block->data[0] & matrix[row * 2]) ^
			 REF_PARITY64(block->data[1] & matrix[row * 2 + 1])) & 1u;
		out.data[row >> 6] |= bit << (row & 63);
	}
	*block = out;
}
inline void block128_multiply_with_transposed_GF2_matrix(block128* block, const uint64_t* matrix)
{
	block128_multiply_with_GF2_matrix(block, matrix);
}

static inline block137 block137_set_one(void)
{
	block137 r = {{1, 0, 0}};
	return r;
}
static inline void block137_mask(block137* x) { x->data[2] &= 0x1ffULL; }
static inline void block137_sqr(block137* out, const block137* in)
{
	static const int taps[] = {0, 21};
	ref_gf_mul(out->data, 3, in->data, in->data, 137, taps, 2);
	block137_mask(out);
}
static inline void block137_mul(block137* out, const block137* a, const block137* b)
{
	static const int taps[] = {0, 21};
	ref_gf_mul(out->data, 3, a->data, b->data, 137, taps, 2);
	block137_mask(out);
}
static inline void block137_pow_bin(block137* block, const char* exp_bits)
{
	block137 base = *block;
	block137 result = block137_set_one();
	for (const char* p = exp_bits; *p; ++p) {
		if (*p != '0' && *p != '1')
			continue;
		block137_sqr(&result, &result);
		if (*p == '1')
			block137_mul(&result, &result, &base);
	}
	*block = result;
}
static inline void block137_mersenne_inverse1(block137* block)
{
	static const char exp[] =
		"11011011011011011011011011011011011011011011011011011011011011011011101101101101101101101101101101101101101101101101101101101101101101101";
	block137_pow_bin(block, exp);
}
static inline void block137_mersenne_inverse2(block137* block)
{
	static const char exp[] =
		"11011011011101101101101110110110110111011011011011101101101101110110110111011011011011101101101101110110110110111011011011011101101101101";
	block137_pow_bin(block, exp);
}
static inline void block137_multiply_with_GF2_matrix(block137* block, const uint64_t matrix[137][3])
{
	block137 out = {{0, 0, 0}};
	for (size_t row = 0; row < 137; ++row) {
		const uint64_t bit =
			(REF_PARITY64(block->data[0] & matrix[row][0]) ^
			 REF_PARITY64(block->data[1] & matrix[row][1]) ^
			 REF_PARITY64((block->data[2] & 0x1ffULL) & matrix[row][2])) & 1u;
		out.data[row >> 6] |= bit << (row & 63);
	}
	block137_mask(&out);
	*block = out;
}

static inline block197 block197_set_one(void)
{
	block197 r = {{1, 0, 0, 0}};
	return r;
}
static inline void block197_mask(block197* x) { x->data[3] &= 0x1fULL; }
static inline void block197_sqr(block197* out, const block197* in)
{
	static const int taps[] = {0, 1, 2, 21};
	ref_gf_mul(out->data, 4, in->data, in->data, 197, taps, 4);
	block197_mask(out);
}
static inline void block197_mul(block197* out, const block197* a, const block197* b)
{
	static const int taps[] = {0, 1, 2, 21};
	ref_gf_mul(out->data, 4, a->data, b->data, 197, taps, 4);
	block197_mask(out);
}
static inline void block197_pow_bin(block197* block, const char* exp_bits)
{
	block197 base = *block;
	block197 result = block197_set_one();
	for (const char* p = exp_bits; *p; ++p) {
		if (*p != '0' && *p != '1')
			continue;
		block197_sqr(&result, &result);
		if (*p == '1')
			block197_mul(&result, &result, &base);
	}
	*block = result;
}
static inline void block197_mersenne_inverse1(block197* block)
{
	static const char exp[] =
		"11101110111101110111101110111101110111101110111101110111101110111101110111101110111101110111101110111011110111011110111011110111011110111011110111011110111011110111011110111011110111011110111011101";
	block197_pow_bin(block, exp);
}
static inline void block197_mersenne_inverse2(block197* block)
{
	static const char exp[] =
		"11111111111101111111111110111111111111011111111111101111111111110111111111111011111111111101111111111111011111111111101111111111110111111111111011111111111101111111111110111111111111011111111111101";
	block197_pow_bin(block, exp);
}
static inline void block197_multiply_with_GF2_matrix(block197* block, const uint64_t matrix[197][4])
{
	block197 out = {{0, 0, 0, 0}};
	for (size_t row = 0; row < 197; ++row) {
		const uint64_t bit =
			(REF_PARITY64(block->data[0] & matrix[row][0]) ^
			 REF_PARITY64(block->data[1] & matrix[row][1]) ^
			 REF_PARITY64(block->data[2] & matrix[row][2]) ^
			 REF_PARITY64((block->data[3] & 0x1fULL) & matrix[row][3])) & 1u;
		out.data[row >> 6] |= bit << (row & 63);
	}
	block197_mask(&out);
	*block = out;
}

static inline block263 block263_set_one(void)
{
	block263 r = {{1, 0, 0, 0, 0}};
	return r;
}
static inline void block263_mask(block263* x) { x->data[4] &= 0x7fULL; }
static inline void block263_sqr(block263* out, const block263* in)
{
	static const int taps[] = {0, 93};
	ref_gf_mul(out->data, 5, in->data, in->data, 263, taps, 2);
	block263_mask(out);
}
static inline void block263_mul(block263* out, const block263* a, const block263* b)
{
	static const int taps[] = {0, 93};
	ref_gf_mul(out->data, 5, a->data, b->data, 263, taps, 2);
	block263_mask(out);
}
static inline void block263_pow_bin(block263* block, const char* exp_bits)
{
	block263 base = *block;
	block263 result = block263_set_one();
	for (const char* p = exp_bits; *p; ++p) {
		if (*p != '0' && *p != '1')
			continue;
		block263_sqr(&result, &result);
		if (*p == '1')
			block263_mul(&result, &result, &base);
	}
	*block = result;
}
static inline void block263_mersenne_inverse1(block263* block)
{
	static const char exp[] =
		"11101111011110111101111011110111101111011110111101111011110111101111011110111101111011110111101111011110111101111011110111101111011101111011110111101111011110111101111011110111101111011110111101111011110111101111011110111101111011110111101111011110111101111011101";
	block263_pow_bin(block, exp);
}
static inline void block263_mersenne_inverse2(block263* block)
{
	static const char exp[] =
		"11111111011111111011111111011111111011111111011111111011111111011111111011111111011111111011111111011111111011111111011111111011111111101111111101111111101111111101111111101111111101111111101111111101111111101111111101111111101111111101111111101111111101111111101";
	block263_pow_bin(block, exp);
}
static inline void block263_multiply_with_GF2_matrix(block263* block, const uint64_t matrix[263][5])
{
	block263 out = {{0, 0, 0, 0, 0}};
	for (size_t row = 0; row < 263; ++row) {
		const uint64_t bit =
			(REF_PARITY64(block->data[0] & matrix[row][0]) ^
			 REF_PARITY64(block->data[1] & matrix[row][1]) ^
			 REF_PARITY64(block->data[2] & matrix[row][2]) ^
			 REF_PARITY64(block->data[3] & matrix[row][3]) ^
			 REF_PARITY64((block->data[4] & 0x7fULL) & matrix[row][4])) & 1u;
		out.data[row >> 6] |= bit << (row & 63);
	}
	block263_mask(&out);
	*block = out;
}

static inline block521 block521_set_one(void)
{
	block521 r = {{1, 0, 0, 0, 0, 0, 0, 0, 0}};
	return r;
}
static inline void block521_sqr(block521* out, const block521* in)
{
	static const int taps[] = {0, 32};
	ref_gf_mul(out->data, BLOCK521_WORDS, in->data, in->data, 521, taps, 2);
	block521_canonicalize(out);
}
static inline void block521_mul(block521* out, const block521* a, const block521* b)
{
	static const int taps[] = {0, 32};
	ref_gf_mul(out->data, BLOCK521_WORDS, a->data, b->data, 521, taps, 2);
	block521_canonicalize(out);
}
static inline bool block521_exp_bit(const uint64_t exponent[9], int bit)
{
	return (exponent[bit >> 6] >> (bit & 63)) & 1u;
}
static inline void block521_pow_words(block521* block, const uint64_t exponent[9])
{
	block521 base = *block;
	block521 result = block521_set_one();
	for (int bit = 520; bit >= 0; --bit) {
		block521_sqr(&result, &result);
		if (block521_exp_bit(exponent, bit))
			block521_mul(&result, &result, &base);
	}
	*block = result;
}
static inline void block521_mersenne_inverse1(block521* block)
{
	static const uint64_t exponent[9] = {
		UINT64_C(0xaaa9555554aaaaa9), UINT64_C(0x4aaaaaa5555552aa),
		UINT64_C(0x55552aaaaa955555), UINT64_C(0xa9555554aaaaaa55),
		UINT64_C(0xaaaaa9555554aaaa), UINT64_C(0x554aaaaaa5555552),
		UINT64_C(0x5555552aaaaa9555), UINT64_C(0xaaa9555554aaaaaa),
		UINT64_C(0xaa)};
	block521_pow_words(block, exponent);
}
static inline void block521_mersenne_inverse2(block521* block)
{
	static const uint64_t exponent[9] = {
		UINT64_C(0x5556aaaaab555555), UINT64_C(0xb555555aaaaaad55),
		UINT64_C(0xaaaad555556aaaaa), UINT64_C(0x56aaaaab555555aa),
		UINT64_C(0x555556aaaaab5555), UINT64_C(0xaab555555aaaaaad),
		UINT64_C(0xaaaaaad555556aaa), UINT64_C(0x5556aaaaab555555),
		UINT64_C(0x155)};
	block521_pow_words(block, exponent);
}
static inline void block521_multiply_with_GF2_matrix(block521* block, const uint64_t matrix[521][BLOCK521_WORDS])
{
	block521 out = {{0}};
	for (size_t row = 0; row < 521; ++row) {
		unsigned int bit = 0;
		for (size_t word = 0; word < BLOCK521_WORDS; ++word)
			bit ^= REF_PARITY64(block->data[word] & matrix[row][word]);
		out.data[row >> 6] |= (uint64_t)(bit & 1u) << (row & 63);
	}
	block521_canonicalize(&out);
	*block = out;
}

inline void block192_inverse(block192* block) { (void)block; assert(0 && "block192_inverse unused in GreatWall128f reference"); }
inline void block192_mersenne_inverse(block192* block) { (void)block; assert(0 && "block192_mersenne_inverse unused in GreatWall128f reference"); }
inline void block192_multiply_with_GF2_matrix(block192* block, const uint64_t* matrix) { (void)block; (void)matrix; assert(0 && "block192 matrix unused in GreatWall128f reference"); }
inline void block192_multiply_with_transposed_GF2_matrix(block192* block, const uint64_t* matrix) { (void)block; (void)matrix; assert(0 && "block192 transposed matrix unused in GreatWall128f reference"); }

inline void block256_inverse(block256* block) { (void)block; assert(0 && "block256_inverse unused in GreatWall128f reference"); }
inline void block256_mersenne_inverse(block256* block) { (void)block; assert(0 && "block256_mersenne_inverse unused in GreatWall128f reference"); }
inline void block256_multiply_with_GF2_matrix(block256* block, const uint64_t* matrix) { (void)block; (void)matrix; assert(0 && "block256 matrix unused in GreatWall128f reference"); }
inline void block256_multiply_with_transposed_GF2_matrix(block256* block, const uint64_t* matrix) { (void)block; (void)matrix; assert(0 && "block256 transposed matrix unused in GreatWall128f reference"); }

#endif
