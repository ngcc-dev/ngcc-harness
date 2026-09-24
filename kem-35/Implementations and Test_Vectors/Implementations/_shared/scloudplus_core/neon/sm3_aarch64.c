/**
 * @file sm3_aarch64.c
 * @brief AArch64 SM3 and pseudo-XOF implementation for Optimized builds.
 *
 * This file is selected only by Optimized SM3 NEON/AArch64 builds.  It uses
 * Arm SM3 crypto instructions when available and falls back to scalar
 * compression otherwise; both paths expose the same `sm3_backend.h` byte interface.
 */

/*
 * The SM3 compression schedule below follows the Arm ACLE SM3 instruction
 * semantics.  The four-round grouping and state word order are adapted from
 * Botan's AArch64 SM3 backend (Simplified BSD License); the surrounding
 * pseudo-XOF, HMAC-SM3, and Scloud+ byte interfaces are kept identical to the
 * portable SM3 pseudo-XOF semantics used by the optimized SM3 backends.
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

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "sm3_backend.h"

#if defined(__ARM_FEATURE_SM3)
#include <arm_neon.h>
#endif

#define L1 512
#define L2 256
#define HASH_SUCCESS 0
#define XOF_SUCCESS 0
#define MEMORY_ALLOCATION_FAILED -2
#define HASH_FAILED -3
#define INVALID_DIGESTBITLEN -4

#if !defined(__ARM_FEATURE_SM3)
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
#endif
#define PUT32(a, b) ((a)[0] = (unsigned char)((b) >> 24), \
					 (a)[1] = (unsigned char)((b) >> 16), \
					 (a)[2] = (unsigned char)((b) >> 8),  \
					 (a)[3] = (unsigned char)(b))
#define SM3_DIGEST_BYTES 32U

static void sm3_store_digest_words(unsigned char *out, const unsigned int dgst[8])
{
	for (int i = 0; i < 8; i++)
	{
		PUT32(out + (size_t)i * 4U, dgst[i]);
	}
}

static void sm3_copy_digest_prefix(unsigned char *out, size_t take, const unsigned int dgst[8])
{
	if (take == SM3_DIGEST_BYTES)
	{
		sm3_store_digest_words(out, dgst);
	}
	else
	{
		unsigned char digest[SM3_DIGEST_BYTES];
		sm3_store_digest_words(digest, dgst);
		memcpy(out, digest, take);
		memset(digest, 0, sizeof(digest));
	}
}

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

#if defined(__ARM_FEATURE_SM3)
static const uint32_t SM3_TJ[64] = {
	0x79CC4519U, 0xF3988A32U, 0xE7311465U, 0xCE6228CBU,
	0x9CC45197U, 0x3988A32FU, 0x7311465EU, 0xE6228CBCU,
	0xCC451979U, 0x988A32F3U, 0x311465E7U, 0x6228CBCEU,
	0xC451979CU, 0x88A32F39U, 0x11465E73U, 0x228CBCE6U,
	0x9D8A7A87U, 0x3B14F50FU, 0x7629EA1EU, 0xEC53D43CU,
	0xD8A7A879U, 0xB14F50F3U, 0x629EA1E7U, 0xC53D43CEU,
	0x8A7A879DU, 0x14F50F3BU, 0x29EA1E76U, 0x53D43CECU,
	0xA7A879D8U, 0x4F50F3B1U, 0x9EA1E762U, 0x3D43CEC5U,
	0x7A879D8AU, 0xF50F3B14U, 0xEA1E7629U, 0xD43CEC53U,
	0xA879D8A7U, 0x50F3B14FU, 0xA1E7629EU, 0x43CEC53DU,
	0x879D8A7AU, 0x0F3B14F5U, 0x1E7629EAU, 0x3CEC53D4U,
	0x79D8A7A8U, 0xF3B14F50U, 0xE7629EA1U, 0xCEC53D43U,
	0x9D8A7A87U, 0x3B14F50FU, 0x7629EA1EU, 0xEC53D43CU,
	0xD8A7A879U, 0xB14F50F3U, 0x629EA1E7U, 0xC53D43CEU,
	0x8A7A879DU, 0x14F50F3BU, 0x29EA1E76U, 0x53D43CECU,
	0xA7A879D8U, 0x4F50F3B1U, 0x9EA1E762U, 0x3D43CEC5U
};

static inline uint32x4_t sm3_reverse_words(uint32x4_t v)
{
	v = vrev64q_u32(v);
	return vextq_u32(v, v, 2);
}

static inline uint32x4_t sm3_tj_vec(size_t round)
{
	return vsetq_lane_u32(SM3_TJ[round], vdupq_n_u32(0), 3);
}

static inline void sm3_round4_a(uint32x4_t *s0, uint32x4_t *s1,
								uint32x4_t w, uint32x4_t wp, size_t round)
{
	uint32x4_t t = vsm3ss1q_u32(*s0, *s1, sm3_tj_vec(round));
	*s0 = vsm3tt1aq_u32(*s0, t, wp, 0);
	*s1 = vsm3tt2aq_u32(*s1, t, w, 0);
	t = vsm3ss1q_u32(*s0, *s1, sm3_tj_vec(round + 1U));
	*s0 = vsm3tt1aq_u32(*s0, t, wp, 1);
	*s1 = vsm3tt2aq_u32(*s1, t, w, 1);
	t = vsm3ss1q_u32(*s0, *s1, sm3_tj_vec(round + 2U));
	*s0 = vsm3tt1aq_u32(*s0, t, wp, 2);
	*s1 = vsm3tt2aq_u32(*s1, t, w, 2);
	t = vsm3ss1q_u32(*s0, *s1, sm3_tj_vec(round + 3U));
	*s0 = vsm3tt1aq_u32(*s0, t, wp, 3);
	*s1 = vsm3tt2aq_u32(*s1, t, w, 3);
}

static inline void sm3_round4_b(uint32x4_t *s0, uint32x4_t *s1,
								uint32x4_t w, uint32x4_t wp, size_t round)
{
	uint32x4_t t = vsm3ss1q_u32(*s0, *s1, sm3_tj_vec(round));
	*s0 = vsm3tt1bq_u32(*s0, t, wp, 0);
	*s1 = vsm3tt2bq_u32(*s1, t, w, 0);
	t = vsm3ss1q_u32(*s0, *s1, sm3_tj_vec(round + 1U));
	*s0 = vsm3tt1bq_u32(*s0, t, wp, 1);
	*s1 = vsm3tt2bq_u32(*s1, t, w, 1);
	t = vsm3ss1q_u32(*s0, *s1, sm3_tj_vec(round + 2U));
	*s0 = vsm3tt1bq_u32(*s0, t, wp, 2);
	*s1 = vsm3tt2bq_u32(*s1, t, w, 2);
	t = vsm3ss1q_u32(*s0, *s1, sm3_tj_vec(round + 3U));
	*s0 = vsm3tt1bq_u32(*s0, t, wp, 3);
	*s1 = vsm3tt2bq_u32(*s1, t, w, 3);
}

static inline void sm3_expand4(uint32x4_t *w0, uint32x4_t w1,
							   uint32x4_t w2, uint32x4_t w3)
{
	const uint32x4_t w7_10 = vextq_u32(w1, w2, 3);
	const uint32x4_t w3_6 = vextq_u32(*w0, w1, 3);
	const uint32x4_t w10_13 = vextq_u32(w2, w3, 2);
	uint32x4_t t = vsm3partw1q_u32(*w0, w7_10, w3);
	*w0 = vsm3partw2q_u32(t, w10_13, w3_6);
}

static void sm3_bit_compress(unsigned int dgst[8], const unsigned char *msg, unsigned long long blocks)
{
	uint32x4_t S0 = sm3_reverse_words(vld1q_u32(dgst));
	uint32x4_t S1 = sm3_reverse_words(vld1q_u32(dgst + 4));

	while (blocks--)
	{
		const uint32x4_t S0_save = S0;
		const uint32x4_t S1_save = S1;
		uint32x4_t W0 = vreinterpretq_u32_u8(vrev32q_u8(vld1q_u8(msg + 0U)));
		uint32x4_t W1 = vreinterpretq_u32_u8(vrev32q_u8(vld1q_u8(msg + 16U)));
		uint32x4_t W2 = vreinterpretq_u32_u8(vrev32q_u8(vld1q_u8(msg + 32U)));
		uint32x4_t W3 = vreinterpretq_u32_u8(vrev32q_u8(vld1q_u8(msg + 48U)));

		msg += 64U;
		sm3_round4_a(&S0, &S1, W0, veorq_u32(W0, W1), 0U);   sm3_expand4(&W0, W1, W2, W3);
		sm3_round4_a(&S0, &S1, W1, veorq_u32(W1, W2), 4U);   sm3_expand4(&W1, W2, W3, W0);
		sm3_round4_a(&S0, &S1, W2, veorq_u32(W2, W3), 8U);   sm3_expand4(&W2, W3, W0, W1);
		sm3_round4_a(&S0, &S1, W3, veorq_u32(W3, W0), 12U);  sm3_expand4(&W3, W0, W1, W2);
		sm3_round4_b(&S0, &S1, W0, veorq_u32(W0, W1), 16U);  sm3_expand4(&W0, W1, W2, W3);
		sm3_round4_b(&S0, &S1, W1, veorq_u32(W1, W2), 20U);  sm3_expand4(&W1, W2, W3, W0);
		sm3_round4_b(&S0, &S1, W2, veorq_u32(W2, W3), 24U);  sm3_expand4(&W2, W3, W0, W1);
		sm3_round4_b(&S0, &S1, W3, veorq_u32(W3, W0), 28U);  sm3_expand4(&W3, W0, W1, W2);
		sm3_round4_b(&S0, &S1, W0, veorq_u32(W0, W1), 32U);  sm3_expand4(&W0, W1, W2, W3);
		sm3_round4_b(&S0, &S1, W1, veorq_u32(W1, W2), 36U);  sm3_expand4(&W1, W2, W3, W0);
		sm3_round4_b(&S0, &S1, W2, veorq_u32(W2, W3), 40U);  sm3_expand4(&W2, W3, W0, W1);
		sm3_round4_b(&S0, &S1, W3, veorq_u32(W3, W0), 44U);  sm3_expand4(&W3, W0, W1, W2);
		sm3_round4_b(&S0, &S1, W0, veorq_u32(W0, W1), 48U);  sm3_expand4(&W0, W1, W2, W3);
		sm3_round4_b(&S0, &S1, W1, veorq_u32(W1, W2), 52U);
		sm3_round4_b(&S0, &S1, W2, veorq_u32(W2, W3), 56U);
		sm3_round4_b(&S0, &S1, W3, veorq_u32(W3, W0), 60U);
		S0 = veorq_u32(S0, S0_save);
		S1 = veorq_u32(S1, S1_save);
	}

	vst1q_u32(dgst, sm3_reverse_words(S0));
	vst1q_u32(dgst + 4, sm3_reverse_words(S1));
}
#else
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
#endif

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

static int pseudoXOF_slow_from_counter(unsigned long long output_len_bits,
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

int pseudoXOF_from_counter(unsigned long long output_len_bits,
						   const unsigned char *msg,
						   unsigned long long msg_len_bits,
						   unsigned int start_ct,
						   unsigned char *output)
{
	if ((output_len_bits & 7ULL) != 0ULL || (msg_len_bits & 7ULL) != 0ULL)
	{
		return pseudoXOF_slow_from_counter(output_len_bits, msg, msg_len_bits,
										   start_ct, output);
	}

	const size_t outlen = (size_t)(output_len_bits / 8ULL);
	const size_t msg_len = (size_t)(msg_len_bits / 8ULL);
	size_t produced = 0U;
	unsigned int ct = start_ct;

	if (msg_len + 4U <= 55U)
	{
		unsigned char block[64];
		memset(block, 0, sizeof(block));
		if (msg_len != 0U)
		{
			memcpy(block, msg, msg_len);
		}
		block[msg_len + 4U] = 0x80U;
		PUT32(block + 56, 0U);
		PUT32(block + 60, (unsigned int)((msg_len + 4U) * 8U));

		while (produced < outlen)
		{
			const size_t take = (outlen - produced < SM3_DIGEST_BYTES) ? outlen - produced : SM3_DIGEST_BYTES;
			unsigned int block_digest[8];

			block[msg_len] = (unsigned char)(ct >> 24);
			block[msg_len + 1U] = (unsigned char)(ct >> 16);
			block[msg_len + 2U] = (unsigned char)(ct >> 8);
			block[msg_len + 3U] = (unsigned char)ct;
			sm3_bit_init(block_digest);
			sm3_bit_compress(block_digest, block, 1U);
			sm3_copy_digest_prefix(output + produced, take, block_digest);
			produced += take;
			ct++;
		}

		memset(block, 0, sizeof(block));
		return XOF_SUCCESS;
	}

	unsigned int base_digest[8];
	const size_t full_blocks = msg_len / 64U;
	const size_t tail_len = msg_len & 63U;
	const unsigned char *tail = msg + full_blocks * 64U;
	const unsigned long long total_bitlen =
		(unsigned long long)(msg_len + 4U) * 8ULL;

	sm3_bit_init(base_digest);
	if (full_blocks != 0U)
	{
		sm3_bit_compress(base_digest, msg, (unsigned long long)full_blocks);
	}

	while (produced < outlen)
	{
		const size_t take = (outlen - produced < SM3_DIGEST_BYTES) ? outlen - produced : SM3_DIGEST_BYTES;
		unsigned int block_digest[8];
		unsigned char final_blocks[128];
		const size_t msg_tail_with_counter = tail_len + 4U;

		memcpy(block_digest, base_digest, sizeof(block_digest));
		memset(final_blocks, 0, sizeof(final_blocks));
		if (tail_len != 0U)
		{
			memcpy(final_blocks, tail, tail_len);
		}
		final_blocks[tail_len + 0U] = (unsigned char)(ct >> 24);
		final_blocks[tail_len + 1U] = (unsigned char)(ct >> 16);
		final_blocks[tail_len + 2U] = (unsigned char)(ct >> 8);
		final_blocks[tail_len + 3U] = (unsigned char)ct;
		final_blocks[msg_tail_with_counter] = 0x80U;

		if (msg_tail_with_counter <= 55U)
		{
			PUT32(final_blocks + 56U, (unsigned int)(total_bitlen >> 32U));
			PUT32(final_blocks + 60U, (unsigned int)total_bitlen);
			sm3_bit_compress(block_digest, final_blocks, 1U);
		}
		else
		{
			PUT32(final_blocks + 64U + 56U,
				  (unsigned int)(total_bitlen >> 32U));
			PUT32(final_blocks + 64U + 60U, (unsigned int)total_bitlen);
			sm3_bit_compress(block_digest, final_blocks, 2U);
		}
		sm3_copy_digest_prefix(output + produced, take, block_digest);
		produced += take;
		ct++;
	}

	memset(base_digest, 0, sizeof(base_digest));

	return XOF_SUCCESS;
}

// GB/T 32918.4-2016 Section 5.4.3
int pseudoXOF(unsigned long long output_len_bits, const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *output)
{
	return pseudoXOF_from_counter(output_len_bits, msg, msg_len_bits, 1U,
								  output);
}
