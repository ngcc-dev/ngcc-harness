/**
 * @file sm3_avx2.c
 * @brief x86_64 AVX2 SM3 and pseudo-XOF implementation for Optimized builds.
 *
 * This file is selected only by Optimized SM3 AVX2 builds.  It exposes the
 * same `sm3_backend.h` byte interface as the portable reference implementation;
 * vectorization changes throughput only, not serialized output.
 */

/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.
*/

#include <string.h>
#include <stdlib.h>
#include <immintrin.h>
#include <stdint.h>
#include "sm3_backend.h"

#define L1 512
#define L2 256
#define HASH_SUCCESS 0
#define XOF_SUCCESS 0
#define MEMORY_ALLOCATION_FAILED -2
#define HASH_FAILED -3
#define INVALID_DIGESTBITLEN -4

#define FF1(x, y, z) ((x) ^ (y) ^ (z))
#define FF2(x, y, z) (((x) & (y)) | ((x) & (z)) | ((y) & (z)))
#define GG1(x, y, z) ((x) ^ (y) ^ (z))
#define GG2(x, y, z) ((((y) ^ (z)) & (x)) ^ (z))
static unsigned int rotl32(unsigned int a, unsigned int n)
{
	n &= 31U;
	return (a << n) | (a >> ((32U - n) & 31U));
}

#define L_SHIFT(a, n) rotl32((unsigned int)(a), (unsigned int)(n))
#define P0(x) ((x) ^ L_SHIFT((x), 9) ^ L_SHIFT((x), 17))
#define P1(x) ((x) ^ L_SHIFT((x), 15) ^ L_SHIFT((x), 23))
#define PUT32(a, b) ((a)[0] = (unsigned char)((b) >> 24), \
					 (a)[1] = (unsigned char)((b) >> 16), \
					 (a)[2] = (unsigned char)((b) >> 8),  \
					 (a)[3] = (unsigned char)(b))

// GB/T 32905-2016
static void sm3_bit_init(unsigned int *init_digest)
{
	init_digest[0] = 0x7380166F;
	init_digest[1] = 0x4914B2B9;
	init_digest[2] = 0x172442D7;
	init_digest[3] = 0xDA8A0600;
	init_digest[4] = 0xA96F30BC;
	init_digest[5] = 0x163138AA;
	init_digest[6] = 0xE38DEE4D;
	init_digest[7] = 0xB0FB0E4E;
}

static void sm3_bit_compress(unsigned int dgst[8], const unsigned char *msg, unsigned long long blocks)
{
	unsigned int A, B, C, D, E, F, G, H;
	unsigned int W[68];
	unsigned int W_prime[64];
	unsigned int SS1, SS2, TT1, TT2;
	int i;
	while (blocks--)
	{
		/***********************Round function*************************/
		for (i = 0; i < 16; i++)
		{
			W[i] = ((unsigned int)(msg + i * 4)[0] << 24 | (unsigned int)(msg + i * 4)[1] << 16 | (unsigned int)(msg + i * 4)[2] << 8 | (unsigned int)(msg + i * 4)[3]);
		}
		for (; i < 68; i++)
		{
			W[i] = P1(W[i - 16] ^ W[i - 9] ^ L_SHIFT(W[i - 3], 15)) ^ L_SHIFT(W[i - 13], 7) ^ W[i - 6];
		}
		for (i = 0; i < 64; i++)
		{
			W_prime[i] = W[i] ^ W[i + 4];
		}
		A = dgst[0];
		B = dgst[1];
		C = dgst[2];
		D = dgst[3];
		E = dgst[4];
		F = dgst[5];
		G = dgst[6];
		H = dgst[7];
		for (i = 0; i < 64; i++)
		{
			if (i < 16)
			{
				SS1 = L_SHIFT(L_SHIFT(A, 12) + E + L_SHIFT(0x79cc4519U, i & 0x1F), 7);
			}
			else
			{
				SS1 = L_SHIFT(L_SHIFT(A, 12) + E + L_SHIFT(0x7a879d8aU, i & 0x1F), 7);
			}
			SS2 = SS1 ^ L_SHIFT(A, 12);
			if (i < 16)
			{
				TT1 = FF1(A, B, C) + D + SS2 + W_prime[i];
				TT2 = GG1(E, F, G) + H + SS1 + W[i];
			}
			else
			{
				TT1 = FF2(A, B, C) + D + SS2 + W_prime[i];
				TT2 = GG2(E, F, G) + H + SS1 + W[i];
			}
			D = C;
			C = L_SHIFT(B, 9);
			B = A;
			A = TT1;
			H = G;
			G = L_SHIFT(F, 19);
			F = E;
			E = P0(TT2);
		}
		dgst[0] ^= A;
		dgst[1] ^= B;
		dgst[2] ^= C;
		dgst[3] ^= D;
		dgst[4] ^= E;
		dgst[5] ^= F;
		dgst[6] ^= G;
		dgst[7] ^= H;
		msg += 64;
	}
}

static void sm3_bit(const unsigned char *msg, unsigned long long msg_bitlen, unsigned char *dgst)
{
	unsigned long long block_num;
	unsigned long long remain;
	block_num = msg_bitlen / 512;
	remain = msg_bitlen & 0x1FF;
	/*********************Initializing value********************/
	unsigned int digest[8];
	sm3_bit_init(digest);
	/******************Round function**************************/
	if (block_num != 0)
	{
		sm3_bit_compress(digest, msg, block_num);
	}
	unsigned char block[64];
	memset(block, 0, 64);
	memcpy(block, msg + block_num * 64, (remain + 7) >> 3);
	block[remain >> 3] &= ((0xFF00 >> (remain & 0x7)) & 0xFF);
	block[remain >> 3] |= (1 << (7 - (remain & 0x7)));
	/*************************Padding**************************/
	if (remain <= 512 - 65)
	{
		memset(block + (remain >> 3) + 1, 0, (512 - remain - 65) >> 3);
	}
	else
	{
		memset(block + (remain >> 3) + 1, 0, (512 - remain - 1) >> 3);
		sm3_bit_compress(digest, block, 1);
		memset(block, 0, 64 - 8);
	}
	PUT32(block + 56, block_num >> 32 << 9);
	PUT32(block + 60, (block_num << 9) + (remain));
	/**********************Round function************************/
	sm3_bit_compress(digest, block, 1);
	/*********************Output hash value**********************/
	for (int i = 0; i < 8; i++)
	{
		PUT32(dgst + i * 4, digest[i]);
	}
}

#define SM3_AVX2_BATCH 8U
#define SM3_AVX2_MAX_BATCH_MSG_BYTES 128U
#define SM3_AVX2_MAX_BATCH_BLOCKS \
	((SM3_AVX2_MAX_BATCH_MSG_BYTES + 1U + 8U + 63U) / 64U)
#define SM3_AVX2_BATCH_BLOCK_BYTES (SM3_AVX2_MAX_BATCH_BLOCKS * 64U)

static inline __m256i sm3_avx2_rotl32(__m256i x, int n)
{
	if (n == 0)
	{
		return x;
	}
	return _mm256_or_si256(_mm256_slli_epi32(x, n),
						   _mm256_srli_epi32(x, 32 - n));
}

static inline uint32_t sm3_scalar_rotl32(uint32_t x, int n)
{
	if (n == 0)
	{
		return x;
	}
	return (x << n) | (x >> (32 - n));
}

static inline __m256i sm3_avx2_p0(__m256i x)
{
	return _mm256_xor_si256(
		_mm256_xor_si256(x, sm3_avx2_rotl32(x, 9)),
		sm3_avx2_rotl32(x, 17));
}

static inline __m256i sm3_avx2_p1(__m256i x)
{
	return _mm256_xor_si256(
		_mm256_xor_si256(x, sm3_avx2_rotl32(x, 15)),
		sm3_avx2_rotl32(x, 23));
}

static inline __m256i sm3_avx2_ff1(__m256i x, __m256i y, __m256i z)
{
	return _mm256_xor_si256(_mm256_xor_si256(x, y), z);
}

static inline __m256i sm3_avx2_ff2(__m256i x, __m256i y, __m256i z)
{
	return _mm256_or_si256(_mm256_or_si256(_mm256_and_si256(x, y),
										   _mm256_and_si256(x, z)),
						   _mm256_and_si256(y, z));
}

static inline __m256i sm3_avx2_gg1(__m256i x, __m256i y, __m256i z)
{
	return _mm256_xor_si256(_mm256_xor_si256(x, y), z);
}

static inline __m256i sm3_avx2_gg2(__m256i x, __m256i y, __m256i z)
{
	return _mm256_xor_si256(_mm256_and_si256(_mm256_xor_si256(y, z), x), z);
}

static uint32_t sm3_load_be32_avx2(const unsigned char *in)
{
	return ((uint32_t)in[0] << 24) |
		   ((uint32_t)in[1] << 16) |
		   ((uint32_t)in[2] << 8) |
		   (uint32_t)in[3];
}

static void sm3_store_be32_avx2(unsigned char *out, uint32_t v)
{
	out[0] = (unsigned char)(v >> 24);
	out[1] = (unsigned char)(v >> 16);
	out[2] = (unsigned char)(v >> 8);
	out[3] = (unsigned char)v;
}

static void sm3_store_be64_avx2(unsigned char *out, uint64_t v)
{
	out[0] = (unsigned char)(v >> 56);
	out[1] = (unsigned char)(v >> 48);
	out[2] = (unsigned char)(v >> 40);
	out[3] = (unsigned char)(v >> 32);
	out[4] = (unsigned char)(v >> 24);
	out[5] = (unsigned char)(v >> 16);
	out[6] = (unsigned char)(v >> 8);
	out[7] = (unsigned char)v;
}

static __m256i sm3_gather_be32_8_avx2(const unsigned char *blocks,
									   size_t offset)
{
	return _mm256_set_epi32(
		(int)sm3_load_be32_avx2(blocks + 7U * SM3_AVX2_BATCH_BLOCK_BYTES + offset),
		(int)sm3_load_be32_avx2(blocks + 6U * SM3_AVX2_BATCH_BLOCK_BYTES + offset),
		(int)sm3_load_be32_avx2(blocks + 5U * SM3_AVX2_BATCH_BLOCK_BYTES + offset),
		(int)sm3_load_be32_avx2(blocks + 4U * SM3_AVX2_BATCH_BLOCK_BYTES + offset),
		(int)sm3_load_be32_avx2(blocks + 3U * SM3_AVX2_BATCH_BLOCK_BYTES + offset),
		(int)sm3_load_be32_avx2(blocks + 2U * SM3_AVX2_BATCH_BLOCK_BYTES + offset),
		(int)sm3_load_be32_avx2(blocks + 1U * SM3_AVX2_BATCH_BLOCK_BYTES + offset),
		(int)sm3_load_be32_avx2(blocks + 0U * SM3_AVX2_BATCH_BLOCK_BYTES + offset));
}

static void sm3_compress8_avx2(
	__m256i digest[8],
	const unsigned char *blocks,
	size_t block_offset)
{
	__m256i W[68];
	__m256i Wp[64];

	for (int i = 0; i < 16; i++)
	{
		W[i] = sm3_gather_be32_8_avx2(blocks, block_offset + (size_t)i * 4U);
	}
	for (int i = 16; i < 68; i++)
	{
		W[i] = sm3_avx2_p1(_mm256_xor_si256(
				   _mm256_xor_si256(W[i - 16], W[i - 9]),
				   sm3_avx2_rotl32(W[i - 3], 15)));
		W[i] = _mm256_xor_si256(W[i],
					_mm256_xor_si256(sm3_avx2_rotl32(W[i - 13], 7),
									  W[i - 6]));
	}
	for (int i = 0; i < 64; i++)
	{
		Wp[i] = _mm256_xor_si256(W[i], W[i + 4]);
	}

	__m256i A = digest[0];
	__m256i B = digest[1];
	__m256i C = digest[2];
	__m256i D = digest[3];
	__m256i E = digest[4];
	__m256i F = digest[5];
	__m256i G = digest[6];
	__m256i H = digest[7];

	for (int i = 0; i < 16; i++)
	{
		const __m256i tj =
			_mm256_set1_epi32((int)sm3_scalar_rotl32(0x79cc4519U, i & 0x1F));
		const __m256i ss1 = sm3_avx2_rotl32(
			_mm256_add_epi32(_mm256_add_epi32(sm3_avx2_rotl32(A, 12), E), tj),
			7);
		const __m256i ss2 = _mm256_xor_si256(ss1, sm3_avx2_rotl32(A, 12));
		const __m256i tt1 = _mm256_add_epi32(
			_mm256_add_epi32(_mm256_add_epi32(sm3_avx2_ff1(A, B, C), D), ss2),
			Wp[i]);
		const __m256i tt2 = _mm256_add_epi32(
			_mm256_add_epi32(_mm256_add_epi32(sm3_avx2_gg1(E, F, G), H), ss1),
			W[i]);

		D = C;
		C = sm3_avx2_rotl32(B, 9);
		B = A;
		A = tt1;
		H = G;
		G = sm3_avx2_rotl32(F, 19);
		F = E;
		E = sm3_avx2_p0(tt2);
	}
	for (int i = 16; i < 64; i++)
	{
		const __m256i tj =
			_mm256_set1_epi32((int)sm3_scalar_rotl32(0x7a879d8aU, i & 0x1F));
		const __m256i ss1 = sm3_avx2_rotl32(
			_mm256_add_epi32(_mm256_add_epi32(sm3_avx2_rotl32(A, 12), E), tj),
			7);
		const __m256i ss2 = _mm256_xor_si256(ss1, sm3_avx2_rotl32(A, 12));
		const __m256i tt1 = _mm256_add_epi32(
			_mm256_add_epi32(_mm256_add_epi32(sm3_avx2_ff2(A, B, C), D), ss2),
			Wp[i]);
		const __m256i tt2 = _mm256_add_epi32(
			_mm256_add_epi32(_mm256_add_epi32(sm3_avx2_gg2(E, F, G), H), ss1),
			W[i]);

		D = C;
		C = sm3_avx2_rotl32(B, 9);
		B = A;
		A = tt1;
		H = G;
		G = sm3_avx2_rotl32(F, 19);
		F = E;
		E = sm3_avx2_p0(tt2);
	}

	digest[0] = _mm256_xor_si256(digest[0], A);
	digest[1] = _mm256_xor_si256(digest[1], B);
	digest[2] = _mm256_xor_si256(digest[2], C);
	digest[3] = _mm256_xor_si256(digest[3], D);
	digest[4] = _mm256_xor_si256(digest[4], E);
	digest[5] = _mm256_xor_si256(digest[5], F);
	digest[6] = _mm256_xor_si256(digest[6], G);
	digest[7] = _mm256_xor_si256(digest[7], H);
}

static void sm3_hash8_short_avx2(const unsigned char *msg[SM3_AVX2_BATCH],
								 size_t msg_len,
								 unsigned char digest[SM3_AVX2_BATCH][32])
{
	if (msg_len > SM3_AVX2_MAX_BATCH_MSG_BYTES)
	{
		for (size_t lane = 0; lane < SM3_AVX2_BATCH; lane++)
		{
			sm3_bit(msg[lane], (unsigned long long)msg_len * 8ULL, digest[lane]);
		}
		return;
	}

	const size_t total_blocks = (msg_len + 1U + 8U + 63U) / 64U;
	unsigned char blocks[SM3_AVX2_BATCH][SM3_AVX2_BATCH_BLOCK_BYTES];
	memset(blocks, 0, sizeof(blocks));

	for (size_t lane = 0; lane < SM3_AVX2_BATCH; lane++)
	{
		memcpy(blocks[lane], msg[lane], msg_len);
		blocks[lane][msg_len] = 0x80U;
		const uint64_t bit_len = (uint64_t)msg_len * 8ULL;
		unsigned char *lenp = blocks[lane] + total_blocks * 64U - 8U;
		for (int i = 0; i < 8; i++)
		{
			lenp[i] = (unsigned char)(bit_len >> (56 - 8 * i));
		}
	}

	__m256i V[8] = {
		_mm256_set1_epi32(0x7380166F),
		_mm256_set1_epi32(0x4914B2B9),
		_mm256_set1_epi32(0x172442D7),
		_mm256_set1_epi32(0xDA8A0600),
		_mm256_set1_epi32(0xA96F30BC),
		_mm256_set1_epi32(0x163138AA),
		_mm256_set1_epi32(0xE38DEE4D),
		_mm256_set1_epi32(0xB0FB0E4E)};

	for (size_t block = 0; block < total_blocks; block++)
	{
		sm3_compress8_avx2(V, blocks[0], block * 64U);
	}

	uint32_t lanes[SM3_AVX2_BATCH];
	for (int word = 0; word < 8; word++)
	{
		_mm256_storeu_si256((__m256i *)lanes, V[word]);
		for (size_t lane = 0; lane < SM3_AVX2_BATCH; lane++)
		{
			sm3_store_be32_avx2(digest[lane] + (size_t)word * 4U, lanes[lane]);
		}
	}
	memset(blocks, 0, sizeof(blocks));
}

static void sm3_digest8_to_bytes_avx2(__m256i V[8],
									  unsigned char digest[SM3_AVX2_BATCH][32])
{
	uint32_t lanes[SM3_AVX2_BATCH];

	for (int word = 0; word < 8; word++)
	{
		_mm256_storeu_si256((__m256i *)lanes, V[word]);
		for (size_t lane = 0; lane < SM3_AVX2_BATCH; lane++)
		{
			sm3_store_be32_avx2(digest[lane] + (size_t)word * 4U, lanes[lane]);
		}
	}
}

static void sm3_hash8_common_msg_ct_avx2(const unsigned char *msg,
										 size_t msg_len, unsigned int ct_start,
										 unsigned char digest[SM3_AVX2_BATCH][32])
{
	unsigned int prefix[8];
	const size_t prefix_blocks = msg_len / 64U;
	const size_t tail_len = msg_len - prefix_blocks * 64U;
	const size_t final_input_len = tail_len + 4U;
	const size_t final_blocks = (final_input_len + 1U + 8U + 63U) / 64U;
	unsigned char blocks[SM3_AVX2_BATCH][SM3_AVX2_BATCH_BLOCK_BYTES];
	__m256i V[8];

	sm3_bit_init(prefix);
	if (prefix_blocks != 0U)
	{
		sm3_bit_compress(prefix, msg, prefix_blocks);
	}

	memset(blocks, 0, sizeof(blocks));
	for (size_t lane = 0; lane < SM3_AVX2_BATCH; lane++)
	{
		const unsigned int ct = ct_start + (unsigned int)lane;
		memcpy(blocks[lane], msg + prefix_blocks * 64U, tail_len);
		blocks[lane][tail_len + 0U] = (unsigned char)(ct >> 24);
		blocks[lane][tail_len + 1U] = (unsigned char)(ct >> 16);
		blocks[lane][tail_len + 2U] = (unsigned char)(ct >> 8);
		blocks[lane][tail_len + 3U] = (unsigned char)ct;
		blocks[lane][final_input_len] = 0x80U;
		sm3_store_be64_avx2(blocks[lane] + final_blocks * 64U - 8U,
							 (uint64_t)(msg_len + 4U) * 8ULL);
	}

	for (int word = 0; word < 8; word++)
	{
		V[word] = _mm256_set1_epi32((int)prefix[word]);
	}
	for (size_t block = 0; block < final_blocks; block++)
	{
		sm3_compress8_avx2(V, blocks[0], block * 64U);
	}
	sm3_digest8_to_bytes_avx2(V, digest);
	memset(blocks, 0, sizeof(blocks));
}

int pseudoXOF_precompute_byte_aligned(const unsigned char *msg,
									  unsigned long long msg_len_bits,
									  unsigned int base_digest[8],
									  unsigned char tail[64],
									  size_t *tail_len,
									  unsigned long long *total_bitlen)
{
	if ((msg_len_bits & 7ULL) != 0ULL)
	{
		return INVALID_DIGESTBITLEN;
	}

	const size_t msg_len = (size_t)(msg_len_bits / 8ULL);
	const size_t full_blocks = msg_len / 64U;
	const size_t rem = msg_len & 63U;

	sm3_bit_init(base_digest);
	if (full_blocks != 0U)
	{
		sm3_bit_compress(base_digest, msg, (unsigned long long)full_blocks);
	}
	memset(tail, 0, 64U);
	if (rem != 0U)
	{
		memcpy(tail, msg + full_blocks * 64U, rem);
	}
	*tail_len = rem;
	*total_bitlen = (unsigned long long)(msg_len + 4U) * 8ULL;

	return XOF_SUCCESS;
}

static void sm3_hash8_precomputed_msg_ct_avx2(
	const unsigned int base_digest[8], const unsigned char tail[64],
	size_t tail_len, unsigned long long total_bitlen, unsigned int ct_start,
	unsigned char digest[SM3_AVX2_BATCH][32])
{
	const size_t final_input_len = tail_len + 4U;
	const size_t final_blocks = (final_input_len + 1U + 8U + 63U) / 64U;
	unsigned char blocks[SM3_AVX2_BATCH][SM3_AVX2_BATCH_BLOCK_BYTES];
	__m256i V[8];

	memset(blocks, 0, sizeof(blocks));
	for (size_t lane = 0; lane < SM3_AVX2_BATCH; lane++)
	{
		const unsigned int ct = ct_start + (unsigned int)lane;
		memcpy(blocks[lane], tail, tail_len);
		blocks[lane][tail_len + 0U] = (unsigned char)(ct >> 24);
		blocks[lane][tail_len + 1U] = (unsigned char)(ct >> 16);
		blocks[lane][tail_len + 2U] = (unsigned char)(ct >> 8);
		blocks[lane][tail_len + 3U] = (unsigned char)ct;
		blocks[lane][final_input_len] = 0x80U;
		sm3_store_be64_avx2(blocks[lane] + final_blocks * 64U - 8U,
							 (uint64_t)total_bitlen);
	}

	for (int word = 0; word < 8; word++)
	{
		V[word] = _mm256_set1_epi32((int)base_digest[word]);
	}
	for (size_t block = 0; block < final_blocks; block++)
	{
		sm3_compress8_avx2(V, blocks[0], block * 64U);
	}
	sm3_digest8_to_bytes_avx2(V, digest);
	memset(blocks, 0, sizeof(blocks));
}

int pseudoXOF_from_precomputed_counter(unsigned long long output_len_bits,
									   const unsigned int base_digest[8],
									   const unsigned char tail[64],
									   size_t tail_len,
									   unsigned long long total_bitlen,
									   unsigned int start_ct,
									   unsigned char *output)
{
	if ((output_len_bits & 7ULL) != 0ULL || tail_len > 63U)
	{
		return INVALID_DIGESTBITLEN;
	}

	const size_t outlen = (size_t)(output_len_bits / 8ULL);
	unsigned char digest[SM3_AVX2_BATCH][32];
	size_t produced = 0U;
	unsigned int ct = start_ct;

	while (produced < outlen)
	{
		sm3_hash8_precomputed_msg_ct_avx2(base_digest, tail, tail_len,
										  total_bitlen, ct, digest);
		for (size_t lane = 0; lane < SM3_AVX2_BATCH && produced < outlen; lane++)
		{
			const size_t take =
				(outlen - produced < 32U) ? (outlen - produced) : 32U;
			memcpy(output + produced, digest[lane], take);
			produced += take;
		}
		ct += (unsigned int)SM3_AVX2_BATCH;
	}

	memset(digest, 0, sizeof(digest));
	return XOF_SUCCESS;
}

int pseudoXOF8_equal_inputlen_avx2(unsigned long long output_len_bits,
								   const unsigned char *msg[SM3_AVX2_BATCH],
								   unsigned long long msg_len_bits,
								   unsigned char *output[SM3_AVX2_BATCH])
{
	if ((output_len_bits & 7ULL) != 0ULL || (msg_len_bits & 7ULL) != 0ULL)
	{
		for (size_t lane = 0; lane < SM3_AVX2_BATCH; lane++)
		{
			const int rc = pseudoXOF(output_len_bits, msg[lane], msg_len_bits,
									 output[lane]);
			if (rc != 0)
			{
				return rc;
			}
		}
		return XOF_SUCCESS;
	}

	const size_t outlen = (size_t)(output_len_bits / 8ULL);
	const size_t msg_len = (size_t)(msg_len_bits / 8ULL);
	if (msg_len + 4U > SM3_AVX2_MAX_BATCH_MSG_BYTES)
	{
		for (size_t lane = 0; lane < SM3_AVX2_BATCH; lane++)
		{
			const int rc = pseudoXOF(output_len_bits, msg[lane], msg_len_bits,
									 output[lane]);
			if (rc != 0)
			{
				return rc;
			}
		}
		return XOF_SUCCESS;
	}

	unsigned char input[SM3_AVX2_BATCH][SM3_AVX2_MAX_BATCH_MSG_BYTES];
	const unsigned char *input_ptrs[SM3_AVX2_BATCH];
	unsigned char digest[SM3_AVX2_BATCH][32];
	size_t produced = 0U;
	unsigned int ct = 1U;

	for (size_t lane = 0; lane < SM3_AVX2_BATCH; lane++)
	{
		input_ptrs[lane] = input[lane];
	}

	while (produced < outlen)
	{
		for (size_t lane = 0; lane < SM3_AVX2_BATCH; lane++)
		{
			memcpy(input[lane], msg[lane], msg_len);
			input[lane][msg_len + 0U] = (unsigned char)(ct >> 24);
			input[lane][msg_len + 1U] = (unsigned char)(ct >> 16);
			input[lane][msg_len + 2U] = (unsigned char)(ct >> 8);
			input[lane][msg_len + 3U] = (unsigned char)ct;
		}
		sm3_hash8_short_avx2(input_ptrs, msg_len + 4U, digest);

		const size_t take = (outlen - produced < 32U) ? (outlen - produced) : 32U;
		for (size_t lane = 0; lane < SM3_AVX2_BATCH; lane++)
		{
			memcpy(output[lane] + produced, digest[lane], take);
		}
		produced += take;
		ct++;
	}

	memset(input, 0, sizeof(input));
	memset(digest, 0, sizeof(digest));
	return XOF_SUCCESS;
}

// key = SM3(UTF-8("3.14159265358979")) || SM3(UTF-8("2.71828182845904"))
static const unsigned char key[] = {
	0x53, 0x07, 0xf6, 0xd5, 0xeb, 0x6a, 0x3c, 0xed,
	0x3d, 0x24, 0xc5, 0x3c, 0xc9, 0xc8, 0x2c, 0xce,
	0x2f, 0x89, 0x36, 0x39, 0x70, 0x23, 0xf0, 0x69,
	0x5c, 0x26, 0xc8, 0x0c, 0x1a, 0xb1, 0x82, 0xa7,
	0x1d, 0xb0, 0x2b, 0xa9, 0x2f, 0x54, 0x40, 0x18,
	0x11, 0x5a, 0x96, 0xe7, 0x19, 0x66, 0x2c, 0xa3,
	0x2b, 0x7c, 0x7e, 0xfc, 0x0a, 0x6d, 0x24, 0x82,
	0x15, 0x07, 0x66, 0xba, 0x6f, 0x65, 0x5b, 0x8e};

static unsigned long long key_len_bits = (sizeof(key) / sizeof(key[0])) * 8;

static void normalize(unsigned char *input, unsigned long long total_bits)
{
	unsigned long long full_bytes = total_bits / 8;
	unsigned long long remaining_bits = total_bits % 8;
	if (remaining_bits > 0)
	{
		input[full_bytes] = input[full_bytes] & ~((1 << (8 - remaining_bits)) - 1);
	}
}

// GB/T 15852.2-2024 Section 7
static int hmac(const unsigned char *msg, unsigned long long msg_len_bits, const unsigned char *key, unsigned long long key_len_bits, unsigned char *mac)
{
	// key expansion
	unsigned char K[L1 / 8] = {0};
	memcpy(K, key, key_len_bits / 8);
	unsigned char K1[L1 / 8], K2[L1 / 8];
	unsigned char IPAD[L1 / 8], OPAD[L1 / 8];
	memset(IPAD, 0x36, sizeof(IPAD));
	memset(OPAD, 0x5C, sizeof(OPAD));
	for (unsigned long long i = 0; i < L1 / 8; i++)
	{
		K1[i] = K[i] ^ IPAD[i];
		K2[i] = K[i] ^ OPAD[i];
	}
	// hash operation
	unsigned char H1[L2 / 8], H2[L2 / 8];
	const size_t temp1_len = (size_t)(L1 / 8) + (size_t)((msg_len_bits + 7) / 8);
	unsigned char temp1_stack[256];
	unsigned char *temp1 = temp1_stack;
	if (temp1_len > sizeof(temp1_stack))
	{
		temp1 = (unsigned char *)malloc(temp1_len);
	}
	if (temp1 == NULL)
	{
		return MEMORY_ALLOCATION_FAILED;
	}
	memcpy(temp1, K1, L1 / 8);
	memcpy(temp1 + L1 / 8, msg, (msg_len_bits + 7) / 8);
	sm3_bit(temp1, L1 + msg_len_bits, H1);
	// output transformation
	unsigned char temp2[L1 / 8 + L2 / 8];
	memcpy(temp2, K2, L1 / 8);
	memcpy(temp2 + L1 / 8, H1, L2 / 8);
	sm3_bit(temp2, L1 + L2, H2);
	// truncation operation
	memcpy(mac, H2, L2 / 8);
	if (temp1 != temp1_stack)
	{
		free(temp1);
	}
	return HASH_SUCCESS;
}

static int pseudohash_512(const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *hash512)
{
	// k1
	unsigned char k1[32];
	unsigned char *cascade_0200_msg;
	cascade_0200_msg = (unsigned char *)malloc((msg_len_bits + 7) / 8 + 2);
	if (cascade_0200_msg == NULL)
	{
		return MEMORY_ALLOCATION_FAILED;
	}
	cascade_0200_msg[0] = 0x02;
	cascade_0200_msg[1] = 0x00;
	memcpy(cascade_0200_msg + 2, msg, (msg_len_bits + 7) / 8);
	if (hmac(cascade_0200_msg, 16 + msg_len_bits, key, key_len_bits, k1) != 0)
	{
		free(cascade_0200_msg);
		return HASH_FAILED;
	}
	// h1
	unsigned char h1[32];
	unsigned char *cascade_msg_0200;
	cascade_msg_0200 = (unsigned char *)malloc((msg_len_bits + 7) / 8 + 2);
	if (cascade_msg_0200 == NULL)
	{
		free(cascade_0200_msg);
		return MEMORY_ALLOCATION_FAILED;
	}
	memcpy(cascade_msg_0200, msg, (msg_len_bits + 7) / 8);
	if (msg_len_bits % 8 == 0)
	{
		cascade_msg_0200[(msg_len_bits + 7) / 8] = 0x02;
		cascade_msg_0200[(msg_len_bits + 7) / 8 + 1] = 0x00;
	}
	else
	{
		normalize(cascade_msg_0200, msg_len_bits);
		cascade_msg_0200[(msg_len_bits + 7) / 8 - 1] = cascade_msg_0200[(msg_len_bits + 7) / 8 - 1] ^ (0x02 >> (msg_len_bits % 8));
		cascade_msg_0200[(msg_len_bits + 7) / 8] = (0x02 << (8 - msg_len_bits % 8)) & 0xff;
		cascade_msg_0200[(msg_len_bits + 7) / 8 + 1] = 0x00;
	}
	sm3_bit(cascade_msg_0200, 16 + msg_len_bits, h1);
	// h2
	unsigned char h2[32];
	unsigned char k1_cascade_h1[64];
	memcpy(k1_cascade_h1, k1, 32);
	memcpy(k1_cascade_h1 + 32, h1, 32);
	sm3_bit(k1_cascade_h1, (sizeof(k1_cascade_h1) / sizeof(k1_cascade_h1)[0]) * 8, h2);
	// final result
	memcpy(hash512, h1, 32);
	memcpy(hash512 + 32, h2, 32);
	// free
	free(cascade_0200_msg);
	free(cascade_msg_0200);
	return HASH_SUCCESS;
}

static int pseudohash_768(const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *hash768)
{
	// k1
	unsigned char k1[32];
	unsigned char *cascade_0300_msg;
	cascade_0300_msg = (unsigned char *)malloc((msg_len_bits + 7) / 8 + 2);
	if (cascade_0300_msg == NULL)
	{
		return MEMORY_ALLOCATION_FAILED;
	}
	cascade_0300_msg[0] = 0x03;
	cascade_0300_msg[1] = 0x00;
	memcpy(cascade_0300_msg + 2, msg, (msg_len_bits + 7) / 8);
	if (hmac(cascade_0300_msg, 16 + msg_len_bits, key, key_len_bits, k1) != 0)
	{
		free(cascade_0300_msg);
		return HASH_FAILED;
	}
	// h1
	unsigned char h1[32];
	unsigned char *cascade_msg_0300;
	cascade_msg_0300 = (unsigned char *)malloc((msg_len_bits + 7) / 8 + 2);
	if (cascade_msg_0300 == NULL)
	{
		free(cascade_0300_msg);
		return MEMORY_ALLOCATION_FAILED;
	}
	memcpy(cascade_msg_0300, msg, (msg_len_bits + 7) / 8);

	if (msg_len_bits % 8 == 0)
	{
		cascade_msg_0300[(msg_len_bits + 7) / 8] = 0x03;
		cascade_msg_0300[(msg_len_bits + 7) / 8 + 1] = 0x00;
	}
	else
	{
		normalize(cascade_msg_0300, msg_len_bits);
		cascade_msg_0300[(msg_len_bits + 7) / 8 - 1] = cascade_msg_0300[(msg_len_bits + 7) / 8 - 1] ^ (0x03 >> (msg_len_bits % 8));
		cascade_msg_0300[(msg_len_bits + 7) / 8] = (0x03 << (8 - msg_len_bits % 8)) & 0xff;
		cascade_msg_0300[(msg_len_bits + 7) / 8 + 1] = 0x00;
	}
	sm3_bit(cascade_msg_0300, 16 + msg_len_bits, h1);
	// k2
	unsigned char k2[32];
	if (hmac(h1, (sizeof(h1) / sizeof(h1[0])) * 8, key, key_len_bits, k2) != 0)
	{
		free(cascade_0300_msg);
		free(cascade_msg_0300);
		return HASH_FAILED;
	}
	// h2
	unsigned char h2[32];
	unsigned char k1_cascade_h1[64];
	memcpy(k1_cascade_h1, k1, 32);
	memcpy(k1_cascade_h1 + 32, h1, 32);
	sm3_bit(k1_cascade_h1, (sizeof(k1_cascade_h1) / sizeof(k1_cascade_h1[0])) * 8, h2);
	// h3
	unsigned char h3[32];
	unsigned char h2_cascade_k2[64];
	memcpy(h2_cascade_k2, h2, 32);
	memcpy(h2_cascade_k2 + 32, k2, 32);
	sm3_bit(h2_cascade_k2, (sizeof(h2_cascade_k2) / sizeof(h2_cascade_k2[0])) * 8, h3);
	// final result
	memcpy(hash768, h1, 32);
	memcpy(hash768 + 32, h2, 32);
	memcpy(hash768 + 64, h3, 32);
	// free
	free(cascade_0300_msg);
	free(cascade_msg_0300);
	return HASH_SUCCESS;
}

static int pseudohash_1024(const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *hash1024)
{
	// k1
	unsigned char k1[32];
	unsigned char *cascade_0400_msg;
	cascade_0400_msg = (unsigned char *)malloc((msg_len_bits + 7) / 8 + 2);
	if (cascade_0400_msg == NULL)
	{
		return MEMORY_ALLOCATION_FAILED;
	}
	cascade_0400_msg[0] = 0x04;
	cascade_0400_msg[1] = 0x00;
	memcpy(cascade_0400_msg + 2, msg, (msg_len_bits + 7) / 8);
	if (hmac(cascade_0400_msg, 16 + msg_len_bits, key, key_len_bits, k1) != 0)
	{
		free(cascade_0400_msg);
		return HASH_FAILED;
	}
	// h1
	unsigned char h1[32];
	unsigned char *cascade_msg_0400;
	cascade_msg_0400 = (unsigned char *)malloc((msg_len_bits + 7) / 8 + 2);
	if (cascade_msg_0400 == NULL)
	{
		free(cascade_0400_msg);
		return MEMORY_ALLOCATION_FAILED;
	}
	memcpy(cascade_msg_0400, msg, (msg_len_bits + 7) / 8);
	normalize(cascade_msg_0400, msg_len_bits);
	if (msg_len_bits % 8 == 0)
	{
		cascade_msg_0400[(msg_len_bits + 7) / 8] = 0x04;
		cascade_msg_0400[(msg_len_bits + 7) / 8 + 1] = 0x00;
	}
	else
	{
		cascade_msg_0400[(msg_len_bits + 7) / 8 - 1] = cascade_msg_0400[(msg_len_bits + 7) / 8 - 1] ^ (0x04 >> (msg_len_bits % 8));
		cascade_msg_0400[(msg_len_bits + 7) / 8] = (0x04 << (8 - msg_len_bits % 8)) & 0xff;
		cascade_msg_0400[(msg_len_bits + 7) / 8 + 1] = 0x00;
	}
	sm3_bit(cascade_msg_0400, 16 + msg_len_bits, h1);
	// k2
	unsigned char k2[32];
	if (hmac(h1, (sizeof(h1) / sizeof(h1[0])) * 8, key, key_len_bits, k2) != 0)
	{
		free(cascade_0400_msg);
		free(cascade_msg_0400);
		return HASH_FAILED;
	}
	// h2
	unsigned char h2[32];
	unsigned char k1_cascade_h1[64];
	memcpy(k1_cascade_h1, k1, 32);
	memcpy(k1_cascade_h1 + 32, h1, 32);
	sm3_bit(k1_cascade_h1, (sizeof(k1_cascade_h1) / sizeof(k1_cascade_h1[0])) * 8, h2);
	// k3
	unsigned char k3[32];
	if (hmac(h2, (sizeof(h2) / sizeof(h2[0])) * 8, key, key_len_bits, k3) != 0)
	{
		free(cascade_0400_msg);
		free(cascade_msg_0400);
		return HASH_FAILED;
	}
	// h3
	unsigned char h3[32];
	unsigned char h2_cascade_k2[64];
	memcpy(h2_cascade_k2, h2, 32);
	memcpy(h2_cascade_k2 + 32, k2, 32);
	sm3_bit(h2_cascade_k2, (sizeof(h2_cascade_k2) / sizeof(h2_cascade_k2[0])) * 8, h3);
	// k4
	unsigned char k4[32];
	if (hmac(h3, (sizeof(h3) / sizeof(h3[0])) * 8, key, key_len_bits, k4) != 0)
	{
		free(cascade_0400_msg);
		free(cascade_msg_0400);
		return HASH_FAILED;
	}
	// h4
	unsigned char h4[32];
	unsigned char k3_cascade_h3_k4[96];
	memcpy(k3_cascade_h3_k4, k3, 32);
	memcpy(k3_cascade_h3_k4 + 32, h3, 32);
	memcpy(k3_cascade_h3_k4 + 64, k4, 32);
	sm3_bit(k3_cascade_h3_k4, (sizeof(k3_cascade_h3_k4) / sizeof(k3_cascade_h3_k4[0])) * 8, h4);
	// final result
	memcpy(hash1024, h1, 32);
	memcpy(hash1024 + 32, h2, 32);
	memcpy(hash1024 + 64, h3, 32);
	memcpy(hash1024 + 96, h4, 32);
	// free
	free(cascade_0400_msg);
	free(cascade_msg_0400);
	return HASH_SUCCESS;
}

int sm3hash(int digest_len_bits, const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *digest)
{
	if (digest_len_bits == 256)
	{
		sm3_bit(msg, msg_len_bits, digest);
		return HASH_SUCCESS;
	}
	else
	{
		return INVALID_DIGESTBITLEN;
	}
}

int pseudohash(int digest_len_bits, const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *digest)
{
	if (digest_len_bits == 512)
	{
		return pseudohash_512(msg, msg_len_bits, digest);
	}
	else if (digest_len_bits == 768)
	{
		return pseudohash_768(msg, msg_len_bits, digest);
	}
	else if (digest_len_bits == 1024)
	{
		return pseudohash_1024(msg, msg_len_bits, digest);
	}
	else
	{
		return INVALID_DIGESTBITLEN;
	}
}

static int pseudoXOF_scalar_from_counter(unsigned long long output_len_bits,
										 const unsigned char *msg,
										 unsigned long long msg_len_bits,
										 unsigned int start_ct,
										 unsigned char *output)
{
	unsigned int ct = start_ct;
	const unsigned long long outlen_bytes = (output_len_bits + 7) / 8;
	unsigned long long produced = 0;
	unsigned char *cascade_msg_ct;
	cascade_msg_ct = (unsigned char *)malloc((msg_len_bits + 7) / 8 + 4);
	if (cascade_msg_ct == NULL)
	{
		return MEMORY_ALLOCATION_FAILED;
	}

	for (unsigned int i = 0; i < (output_len_bits + 255) / 256; i++)
	{
		unsigned char block[32];
		unsigned long long take;

		memcpy(cascade_msg_ct, msg, (msg_len_bits + 7) / 8);
		if (msg_len_bits % 8 == 0)
		{
			cascade_msg_ct[(msg_len_bits + 7) / 8] = ct >> 24;
			cascade_msg_ct[(msg_len_bits + 7) / 8 + 1] = (ct & 0xffffff) >> 16;
			cascade_msg_ct[(msg_len_bits + 7) / 8 + 2] = (ct & 0xffff) >> 8;
			cascade_msg_ct[(msg_len_bits + 7) / 8 + 3] = ct & 0xff;
		}
		else
		{
			normalize(cascade_msg_ct, msg_len_bits);
			cascade_msg_ct[(msg_len_bits + 7) / 8 - 1] = cascade_msg_ct[(msg_len_bits + 7) / 8 - 1] ^ (ct >> (32 - (8 - (msg_len_bits % 8))));
			cascade_msg_ct[(msg_len_bits + 7) / 8] = ((ct << (8 - (msg_len_bits % 8))) & 0xffffffff) >> 24;
			cascade_msg_ct[(msg_len_bits + 7) / 8 + 1] = ((ct << (16 - (msg_len_bits % 8))) & 0xffffffff) >> 24;
			cascade_msg_ct[(msg_len_bits + 7) / 8 + 2] = ((ct << (24 - (msg_len_bits % 8))) & 0xffffffff) >> 24;
			cascade_msg_ct[(msg_len_bits + 7) / 8 + 3] = ((ct << (32 - (msg_len_bits % 8))) & 0xffffffff) >> 24;
		}
		sm3_bit(cascade_msg_ct, msg_len_bits + 32, block);
		take = outlen_bytes - produced;
		if (take > sizeof(block))
		{
			take = sizeof(block);
		}
		memcpy(output + produced, block, (size_t)take);
		memset(block, 0, sizeof(block));
		produced += take;
		ct++;
	}
	free(cascade_msg_ct);
	if (output_len_bits % 256 != 0)
	{
		normalize(output, output_len_bits);
	}
	return XOF_SUCCESS;
}

int pseudoXOF_from_counter_avx2(unsigned long long output_len_bits,
								const unsigned char *msg,
								unsigned long long msg_len_bits,
								unsigned int start_ct,
								unsigned char *output)
{
	if ((output_len_bits & 7ULL) != 0ULL || (msg_len_bits & 7ULL) != 0ULL)
	{
		return pseudoXOF_scalar_from_counter(output_len_bits, msg, msg_len_bits,
											start_ct, output);
	}

	const size_t outlen = (size_t)(output_len_bits / 8ULL);
	const size_t msg_len = (size_t)(msg_len_bits / 8ULL);
	unsigned char digest[SM3_AVX2_BATCH][32];
	size_t produced = 0U;
	unsigned int ct = start_ct;

	while (produced < outlen)
	{
		sm3_hash8_common_msg_ct_avx2(msg, msg_len, ct, digest);
		for (size_t lane = 0; lane < SM3_AVX2_BATCH && produced < outlen; lane++)
		{
			const size_t take = (outlen - produced < 32U) ? (outlen - produced) : 32U;
			memcpy(output + produced, digest[lane], take);
			produced += take;
		}
		ct += (unsigned int)SM3_AVX2_BATCH;
	}

	memset(digest, 0, sizeof(digest));
	return XOF_SUCCESS;
}

// GB/T 32918.4-2016 Section 5.4.3
int pseudoXOF(unsigned long long output_len_bits, const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *output)
{
	return pseudoXOF_from_counter_avx2(output_len_bits, msg, msg_len_bits, 1U,
									   output);
}
