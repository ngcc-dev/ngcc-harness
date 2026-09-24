/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "sm3.h"

#define L1 512
#define L2 256
#define HASH_SUCCESS 0
#define XOF_SUCCESS 0
#define MEMORY_ALLOCATION_FAILED -2
#define HASH_FAILED -3
#define INVALID_DIGESTBITLEN -4

#define FF0(x, y, z) ((x) ^ (y) ^ (z))
#define GG0(x, y, z) ((x) ^ (y) ^ (z))
#define FF16(x, y, z) (((x) & (y)) | ((z) & ((x) | (y))))
#define GG16(x, y, z) ((z) ^ ((x) & ((y) ^ (z))))
#define PUT32(a, b) ((a)[0] = (unsigned char)((b) >> 24), \
					 (a)[1] = (unsigned char)((b) >> 16), \
					 (a)[2] = (unsigned char)((b) >> 8),  \
					 (a)[3] = (unsigned char)(b))

static inline uint32_t rol32(uint32_t x, uint32_t n)
{
	n &= 31u;
	return (x << n) | (x >> ((32u - n) & 31u));
}

#define P0(x) ((x) ^ rol32((x), 9) ^ rol32((x), 17))
#define P1(x) ((x) ^ rol32((x), 15) ^ rol32((x), 23))

void sm3_store8_be_m4(unsigned char *out, const uint32_t in[8]);

static inline uint32_t load_be32(const unsigned char *p)
{
	if (((uintptr_t)p & 3u) == 0u)
	{
		return __builtin_bswap32(*(const uint32_t *)p);
	}

	return ((uint32_t)p[0] << 24) |
		   ((uint32_t)p[1] << 16) |
		   ((uint32_t)p[2] << 8) |
		   ((uint32_t)p[3]);
}

static const uint32_t T_sm3[64] = {
	0x79cc4519u, 0xf3988a32u, 0xe7311465u, 0xce6228cbu,
	0x9cc45197u, 0x3988a32fu, 0x7311465eu, 0xe6228cbcu,
	0xcc451979u, 0x988a32f3u, 0x311465e7u, 0x6228cbceu,
	0xc451979cu, 0x88a32f39u, 0x11465e73u, 0x228cbce6u,
	0x9d8a7a87u, 0x3b14f50fu, 0x7629ea1eu, 0xec53d43cu,
	0xd8a7a879u, 0xb14f50f3u, 0x629ea1e7u, 0xc53d43ceu,
	0x8a7a879du, 0x14f50f3bu, 0x29ea1e76u, 0x53d43cecu,
	0xa7a879d8u, 0x4f50f3b1u, 0x9ea1e762u, 0x3d43cec5u,
	0x7a879d8au, 0xf50f3b14u, 0xea1e7629u, 0xd43cec53u,
	0xa879d8a7u, 0x50f3b14fu, 0xa1e7629eu, 0x43cec53du,
	0x879d8a7au, 0x0f3b14f5u, 0x1e7629eau, 0x3cec53d4u,
	0x79d8a7a8u, 0xf3b14f50u, 0xe7629ea1u, 0xcec53d43u,
	0x9d8a7a87u, 0x3b14f50fu, 0x7629ea1eu, 0xec53d43cu,
	0xd8a7a879u, 0xb14f50f3u, 0x629ea1e7u, 0xc53d43ceu,
	0x8a7a879du, 0x14f50f3bu, 0x29ea1e76u, 0x53d43cecu,
	0xa7a879d8u, 0x4f50f3b1u, 0x9ea1e762u, 0x3d43cec5u,
};

#define SM3_R0(a, b, c, d, e, f, g, h, w, w1, tj) do {       \
	uint32_t a12 = rol32((a), 12);                            \
	uint32_t ss1 = rol32(a12 + (e) + (tj), 7);                \
	uint32_t ss2 = ss1 ^ a12;                                 \
	uint32_t tt1 = FF0((a), (b), (c)) + (d) + ss2 + (w1);     \
	uint32_t tt2 = GG0((e), (f), (g)) + (h) + ss1 + (w);      \
	(b) = rol32((b), 9);                                      \
	(f) = rol32((f), 19);                                     \
	(d) = tt1;                                                \
	(h) = P0(tt2);                                            \
} while (0)

#define SM3_R1(a, b, c, d, e, f, g, h, w, w1, tj) do {       \
	uint32_t a12 = rol32((a), 12);                            \
	uint32_t ss1 = rol32(a12 + (e) + (tj), 7);                \
	uint32_t ss2 = ss1 ^ a12;                                 \
	uint32_t tt1 = FF16((a), (b), (c)) + (d) + ss2 + (w1);    \
	uint32_t tt2 = GG16((e), (f), (g)) + (h) + ss1 + (w);     \
	(b) = rol32((b), 9);                                      \
	(f) = rol32((f), 19);                                     \
	(d) = tt1;                                                \
	(h) = P0(tt2);                                            \
} while (0)

// GB/T 32905-2016
static void sm3_bit_init(uint32_t *init_digest)
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

static void sm3_bit_compress(uint32_t dgst[8], const unsigned char *msg, unsigned long long blocks)
{
	uint32_t A, B, C, D, E, F, G, H;
	uint32_t W[68] __attribute__((aligned(4)));
	unsigned int i;
	while (blocks--)
	{
		/***********************Round function*************************/
		for (i = 0; i < 16; i++)
		{
			W[i] = load_be32(msg + i * 4u);
		}
		for (; i < 68; i++)
		{
			W[i] = P1(W[i - 16] ^ W[i - 9] ^ rol32(W[i - 3], 15)) ^ rol32(W[i - 13], 7) ^ W[i - 6];
		}
		A = dgst[0];
		B = dgst[1];
		C = dgst[2];
		D = dgst[3];
		E = dgst[4];
		F = dgst[5];
		G = dgst[6];
		H = dgst[7];

#define R4_0(j) do { \
	SM3_R0(A, B, C, D, E, F, G, H, W[(j) + 0], W[(j) + 0] ^ W[(j) + 4], T_sm3[(j) + 0]); \
	SM3_R0(D, A, B, C, H, E, F, G, W[(j) + 1], W[(j) + 1] ^ W[(j) + 5], T_sm3[(j) + 1]); \
	SM3_R0(C, D, A, B, G, H, E, F, W[(j) + 2], W[(j) + 2] ^ W[(j) + 6], T_sm3[(j) + 2]); \
	SM3_R0(B, C, D, A, F, G, H, E, W[(j) + 3], W[(j) + 3] ^ W[(j) + 7], T_sm3[(j) + 3]); \
} while (0)

#define R4_1(j) do { \
	SM3_R1(A, B, C, D, E, F, G, H, W[(j) + 0], W[(j) + 0] ^ W[(j) + 4], T_sm3[(j) + 0]); \
	SM3_R1(D, A, B, C, H, E, F, G, W[(j) + 1], W[(j) + 1] ^ W[(j) + 5], T_sm3[(j) + 1]); \
	SM3_R1(C, D, A, B, G, H, E, F, W[(j) + 2], W[(j) + 2] ^ W[(j) + 6], T_sm3[(j) + 2]); \
	SM3_R1(B, C, D, A, F, G, H, E, W[(j) + 3], W[(j) + 3] ^ W[(j) + 7], T_sm3[(j) + 3]); \
} while (0)

		R4_0(0);  R4_0(4);  R4_0(8);  R4_0(12);
		R4_1(16); R4_1(20); R4_1(24); R4_1(28);
		R4_1(32); R4_1(36); R4_1(40); R4_1(44);
		R4_1(48); R4_1(52); R4_1(56); R4_1(60);

#undef R4_0
#undef R4_1

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

static inline void __attribute__((always_inline)) sm3_compress_rounds(uint32_t dgst[8], const uint32_t W[68])
{
	uint32_t A, B, C, D, E, F, G, H;

	A = dgst[0];
	B = dgst[1];
	C = dgst[2];
	D = dgst[3];
	E = dgst[4];
	F = dgst[5];
	G = dgst[6];
	H = dgst[7];

#define R4_0(j) do { \
	SM3_R0(A, B, C, D, E, F, G, H, W[(j) + 0], W[(j) + 0] ^ W[(j) + 4], T_sm3[(j) + 0]); \
	SM3_R0(D, A, B, C, H, E, F, G, W[(j) + 1], W[(j) + 1] ^ W[(j) + 5], T_sm3[(j) + 1]); \
	SM3_R0(C, D, A, B, G, H, E, F, W[(j) + 2], W[(j) + 2] ^ W[(j) + 6], T_sm3[(j) + 2]); \
	SM3_R0(B, C, D, A, F, G, H, E, W[(j) + 3], W[(j) + 3] ^ W[(j) + 7], T_sm3[(j) + 3]); \
} while (0)

#define R4_1(j) do { \
	SM3_R1(A, B, C, D, E, F, G, H, W[(j) + 0], W[(j) + 0] ^ W[(j) + 4], T_sm3[(j) + 0]); \
	SM3_R1(D, A, B, C, H, E, F, G, W[(j) + 1], W[(j) + 1] ^ W[(j) + 5], T_sm3[(j) + 1]); \
	SM3_R1(C, D, A, B, G, H, E, F, W[(j) + 2], W[(j) + 2] ^ W[(j) + 6], T_sm3[(j) + 2]); \
	SM3_R1(B, C, D, A, F, G, H, E, W[(j) + 3], W[(j) + 3] ^ W[(j) + 7], T_sm3[(j) + 3]); \
} while (0)

	R4_0(0);  R4_0(4);  R4_0(8);  R4_0(12);
	R4_1(16); R4_1(20); R4_1(24); R4_1(28);
	R4_1(32); R4_1(36); R4_1(40); R4_1(44);
	R4_1(48); R4_1(52); R4_1(56); R4_1(60);

#undef R4_0
#undef R4_1

	dgst[0] ^= A;
	dgst[1] ^= B;
	dgst[2] ^= C;
	dgst[3] ^= D;
	dgst[4] ^= E;
	dgst[5] ^= F;
	dgst[6] ^= G;
	dgst[7] ^= H;
}

static void sm3_compress_8w_pad768_words(uint32_t dgst[8], const uint32_t msg8[8])
{
	uint32_t W[68] __attribute__((aligned(4)));
	unsigned int i;

	for (i = 0; i < 8; i++)
	{
		W[i] = msg8[i];
	}
	W[8] = 0x80000000u;
	for (i = 9; i < 15; i++)
	{
		W[i] = 0;
	}
	W[15] = 0x00000300u;
	for (i = 16; i < 68; i++)
	{
		W[i] = P1(W[i - 16] ^ W[i - 9] ^ rol32(W[i - 3], 15)) ^ rol32(W[i - 13], 7) ^ W[i - 6];
	}
	sm3_compress_rounds(dgst, W);
}

static void sm3_compress_8w_8w_words(uint32_t dgst[8], const uint32_t a[8], const uint32_t b[8])
{
	uint32_t W[68] __attribute__((aligned(4)));
	unsigned int i;

	for (i = 0; i < 8; i++)
	{
		W[i] = a[i];
		W[8u + i] = b[i];
	}
	for (i = 16; i < 68; i++)
	{
		W[i] = P1(W[i - 16] ^ W[i - 9] ^ rol32(W[i - 3], 15)) ^ rol32(W[i - 13], 7) ^ W[i - 6];
	}
	sm3_compress_rounds(dgst, W);
}

static void sm3_bit(const unsigned char *msg, unsigned long long msg_bitlen, unsigned char *dgst)
{
	unsigned long long block_num;
	unsigned long long remain;
	block_num = msg_bitlen / 512;
	remain = msg_bitlen & 0x1FF;
	/*********************Initializing value********************/
	uint32_t digest[8];
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

typedef struct
{
	uint32_t digest[8];
	uint64_t bitlen;
	unsigned char block[64];
	uint32_t used;
} sm3_ctx;

static int byte_len_u32(unsigned long long bitlen, uint32_t *byte_len)
{
	unsigned long long bytes;

	if ((bitlen & 7ULL) != 0ULL)
	{
		return 0;
	}

	bytes = bitlen >> 3;
	if (bytes > UINT32_MAX)
	{
		return 0;
	}

	*byte_len = (uint32_t)bytes;
	return 1;
}

static int ceil_byte_len_u32(unsigned long long bitlen, uint32_t *byte_len)
{
	unsigned long long bytes = (bitlen + 7ULL) >> 3;

	if (bytes > UINT32_MAX)
	{
		return 0;
	}

	*byte_len = (uint32_t)bytes;
	return 1;
}

static void sm3_ctx_init(sm3_ctx *ctx)
{
	sm3_bit_init(ctx->digest);
	ctx->bitlen = 0;
	ctx->used = 0;
}

static void sm3_ctx_init_from_digest(sm3_ctx *ctx, const uint32_t digest[8], uint64_t bitlen)
{
	memcpy(ctx->digest, digest, sizeof(ctx->digest));
	ctx->bitlen = bitlen;
	ctx->used = 0;
}

static void sm3_update_bytes(sm3_ctx *ctx, const unsigned char *msg, uint32_t len)
{
	ctx->bitlen += (uint64_t)len * 8ULL;
	if (len == 0u)
	{
		return;
	}

	if (ctx->used != 0u)
	{
		uint32_t take = 64u - ctx->used;
		if (take > len)
		{
			take = len;
		}
		memcpy(ctx->block + ctx->used, msg, take);
		ctx->used += take;
		msg += take;
		len -= take;

		if (ctx->used == 64u)
		{
			sm3_bit_compress(ctx->digest, ctx->block, 1);
			ctx->used = 0;
		}
	}

	if (len >= 64u)
	{
		uint32_t blocks = len >> 6;
		uint32_t consumed = blocks << 6;
		sm3_bit_compress(ctx->digest, msg, blocks);
		msg += consumed;
		len -= consumed;
	}

	if (len != 0u)
	{
		memcpy(ctx->block, msg, len);
		ctx->used = len;
	}
}

static void put_digest_words(unsigned char *out, const uint32_t words[8])
{
	for (int i = 0; i < 8; i++)
	{
		PUT32(out + i * 4, words[i]);
	}
}

static void sm3_final_words(sm3_ctx *ctx, uint32_t out[8])
{
	uint64_t bitlen = ctx->bitlen;

	ctx->block[ctx->used++] = 0x80;
	if (ctx->used > 56u)
	{
		memset(ctx->block + ctx->used, 0, 64u - ctx->used);
		sm3_bit_compress(ctx->digest, ctx->block, 1);
		ctx->used = 0;
	}

	memset(ctx->block + ctx->used, 0, 56u - ctx->used);
	PUT32(ctx->block + 56, (uint32_t)(bitlen >> 32));
	PUT32(ctx->block + 60, (uint32_t)bitlen);
	sm3_bit_compress(ctx->digest, ctx->block, 1);
	memcpy(out, ctx->digest, sizeof(ctx->digest));
}

static void sm3_final_bytes(sm3_ctx *ctx, unsigned char *dgst)
{
	uint64_t bitlen = ctx->bitlen;

	ctx->block[ctx->used++] = 0x80;
	if (ctx->used > 56u)
	{
		memset(ctx->block + ctx->used, 0, 64u - ctx->used);
		sm3_bit_compress(ctx->digest, ctx->block, 1);
		ctx->used = 0;
	}

	memset(ctx->block + ctx->used, 0, 56u - ctx->used);
	PUT32(ctx->block + 56, (uint32_t)(bitlen >> 32));
	PUT32(ctx->block + 60, (uint32_t)bitlen);
	sm3_bit_compress(ctx->digest, ctx->block, 1);

	for (int i = 0; i < 8; i++)
	{
		PUT32(dgst + i * 4, ctx->digest[i]);
	}
}

static void sm3_hash_bytes(const unsigned char *msg, uint32_t msg_len, unsigned char *dgst)
{
	sm3_ctx ctx;

	sm3_ctx_init(&ctx);
	sm3_update_bytes(&ctx, msg, msg_len);
	sm3_final_bytes(&ctx, dgst);
}

static void sm3_hash_bytes2_words(const unsigned char *a, uint32_t alen,
								  const unsigned char *b, uint32_t blen,
								  uint32_t out[8])
{
	sm3_ctx ctx;

	sm3_ctx_init(&ctx);
	sm3_update_bytes(&ctx, a, alen);
	sm3_update_bytes(&ctx, b, blen);
	sm3_final_words(&ctx, out);
}

static const uint32_t sm3_pad512_expanded_words[68] = {
	0x80000000u, 0x00000000u, 0x00000000u, 0x00000000u,
	0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u,
	0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u,
	0x00000000u, 0x00000000u, 0x00000000u, 0x00000200u,
	0x80404000u, 0x00000000u, 0x01008080u, 0x10005000u,
	0x00000000u, 0x002002a0u, 0xac545c04u, 0x00000000u,
	0x09582a39u, 0xa0003000u, 0x00000000u, 0x00200280u,
	0xa4515804u, 0x20200040u, 0x51609838u, 0x30005701u,
	0xa0002000u, 0x008200aau, 0x6ad525d0u, 0x0a0e0216u,
	0xb0f52042u, 0xfa7073b0u, 0x20000000u, 0x008200a8u,
	0x7a542590u, 0x22a20044u, 0xd5d6ebd2u, 0x82005771u,
	0x8a202240u, 0xb42826aau, 0xeaf84e59u, 0x4898eaf9u,
	0x8207283du, 0xee6775fau, 0xa3e0e0a0u, 0x8828488au,
	0x23b45a5du, 0x628a22c4u, 0x8d6d0615u, 0x38300a7eu,
	0xe96260e5u, 0x2b60c020u, 0x502ed531u, 0x9e878cb9u,
	0x218c38f8u, 0xdcae3cb7u, 0x2a3e0e0au, 0xe9e0c461u,
	0x8c3e3831u, 0x44aaa228u, 0xdc60a38bu, 0x518300f7u
};

static void sm3_hash_8w_8w_words(const uint32_t a[8], const uint32_t b[8], uint32_t out[8])
{
	sm3_bit_init(out);
	sm3_compress_8w_8w_words(out, a, b);
	sm3_compress_rounds(out, sm3_pad512_expanded_words);
}

static void sm3_hash_8w_8w_8w_words(const uint32_t a[8],
									const uint32_t b[8],
									const uint32_t c[8],
									uint32_t out[8])
{
	sm3_bit_init(out);
	sm3_compress_8w_8w_words(out, a, b);
	sm3_compress_8w_pad768_words(out, c);
}

// key = SM3(UTF-8("3.14159265358979")) || SM3(UTF-8("2.71828182845904"))
static const unsigned char sm3_hmac_default_key[] = {
	0x53, 0x07, 0xf6, 0xd5, 0xeb, 0x6a, 0x3c, 0xed,
	0x3d, 0x24, 0xc5, 0x3c, 0xc9, 0xc8, 0x2c, 0xce,
	0x2f, 0x89, 0x36, 0x39, 0x70, 0x23, 0xf0, 0x69,
	0x5c, 0x26, 0xc8, 0x0c, 0x1a, 0xb1, 0x82, 0xa7,
	0x1d, 0xb0, 0x2b, 0xa9, 0x2f, 0x54, 0x40, 0x18,
	0x11, 0x5a, 0x96, 0xe7, 0x19, 0x66, 0x2c, 0xa3,
	0x2b, 0x7c, 0x7e, 0xfc, 0x0a, 0x6d, 0x24, 0x82,
	0x15, 0x07, 0x66, 0xba, 0x6f, 0x65, 0x5b, 0x8e};

static unsigned long long sm3_hmac_default_key_len_bits =
	(sizeof(sm3_hmac_default_key) / sizeof(sm3_hmac_default_key[0])) * 8;

static uint32_t hmac_default_ipad_cv[8];
static uint32_t hmac_default_opad_cv[8];
static int hmac_default_precomputed;

static void hmac_default_precompute(void)
{
	unsigned char block[64];

	if (hmac_default_precomputed != 0)
	{
		return;
	}

	sm3_bit_init(hmac_default_ipad_cv);
	for (unsigned int i = 0; i < sizeof(block); i++)
	{
		block[i] = sm3_hmac_default_key[i] ^ 0x36u;
	}
	sm3_bit_compress(hmac_default_ipad_cv, block, 1);

	sm3_bit_init(hmac_default_opad_cv);
	for (unsigned int i = 0; i < sizeof(block); i++)
	{
		block[i] = sm3_hmac_default_key[i] ^ 0x5cu;
	}
	sm3_bit_compress(hmac_default_opad_cv, block, 1);

	hmac_default_precomputed = 1;
}

static void hmac_default_outer_32words(const uint32_t inner[8], uint32_t mac[8])
{
	hmac_default_precompute();
	memcpy(mac, hmac_default_opad_cv, sizeof(hmac_default_opad_cv));
	sm3_compress_8w_pad768_words(mac, inner);
}

static void hmac_default_bytes2_words(const unsigned char *a, uint32_t alen,
									  const unsigned char *b, uint32_t blen,
									  uint32_t mac[8])
{
	uint32_t inner[8];
	sm3_ctx ctx;

	hmac_default_precompute();
	sm3_ctx_init_from_digest(&ctx, hmac_default_ipad_cv, L1);
	sm3_update_bytes(&ctx, a, alen);
	sm3_update_bytes(&ctx, b, blen);
	sm3_final_words(&ctx, inner);
	hmac_default_outer_32words(inner, mac);
}

static void hmac_default_32words_words(const uint32_t msg[8], uint32_t mac[8])
{
	uint32_t inner[8];

	hmac_default_precompute();
	memcpy(inner, hmac_default_ipad_cv, sizeof(hmac_default_ipad_cv));
	sm3_compress_8w_pad768_words(inner, msg);
	hmac_default_outer_32words(inner, mac);
}

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
	unsigned char *temp1;
	temp1 = (unsigned char *)malloc(L1 / 8 + (msg_len_bits + 7) / 8);
	if (temp1 == NULL)
	{
		fprintf(stderr, "ERROR: Memory allocation failed at %s, line %d. \n", __FILE__, __LINE__);
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
	free(temp1);
	return HASH_SUCCESS;
}

static int pseudohash_512_bytes(const unsigned char *msg, uint32_t msg_len, unsigned char *hash512)
{
	static const unsigned char domain[2] = {0x02, 0x00};
	uint32_t k1[8];
	uint32_t h1[8];
	uint32_t h2[8];

	hmac_default_bytes2_words(domain, sizeof(domain), msg, msg_len, k1);
	sm3_hash_bytes2_words(msg, msg_len, domain, sizeof(domain), h1);
	sm3_hash_8w_8w_words(k1, h1, h2);

	put_digest_words(hash512, h1);
	put_digest_words(hash512 + 32, h2);
	return HASH_SUCCESS;
}

static int pseudohash_768_bytes(const unsigned char *msg, uint32_t msg_len, unsigned char *hash768)
{
	static const unsigned char domain[2] = {0x03, 0x00};
	uint32_t k1[8];
	uint32_t k2[8];
	uint32_t h1[8];
	uint32_t h2[8];
	uint32_t h3[8];

	hmac_default_bytes2_words(domain, sizeof(domain), msg, msg_len, k1);
	sm3_hash_bytes2_words(msg, msg_len, domain, sizeof(domain), h1);
	hmac_default_32words_words(h1, k2);
	sm3_hash_8w_8w_words(k1, h1, h2);
	sm3_hash_8w_8w_words(h2, k2, h3);

	put_digest_words(hash768, h1);
	put_digest_words(hash768 + 32, h2);
	put_digest_words(hash768 + 64, h3);
	return HASH_SUCCESS;
}

static int pseudohash_1024_bytes(const unsigned char *msg, uint32_t msg_len, unsigned char *hash1024)
{
	static const unsigned char domain[2] = {0x04, 0x00};
	uint32_t k1[8];
	uint32_t k2[8];
	uint32_t k3[8];
	uint32_t k4[8];
	uint32_t h1[8];
	uint32_t h2[8];
	uint32_t h3[8];
	uint32_t h4[8];

	hmac_default_bytes2_words(domain, sizeof(domain), msg, msg_len, k1);
	sm3_hash_bytes2_words(msg, msg_len, domain, sizeof(domain), h1);
	hmac_default_32words_words(h1, k2);
	sm3_hash_8w_8w_words(k1, h1, h2);
	hmac_default_32words_words(h2, k3);
	sm3_hash_8w_8w_words(h2, k2, h3);
	hmac_default_32words_words(h3, k4);
	sm3_hash_8w_8w_8w_words(k3, h3, k4, h4);

	put_digest_words(hash1024, h1);
	put_digest_words(hash1024 + 32, h2);
	put_digest_words(hash1024 + 64, h3);
	put_digest_words(hash1024 + 96, h4);
	return HASH_SUCCESS;
}

static int pseudohash_512(const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *hash512)
{
	uint32_t msg_len;

	if (byte_len_u32(msg_len_bits, &msg_len))
	{
		return pseudohash_512_bytes(msg, msg_len, hash512);
	}

	// k1
	unsigned char k1[32];
	unsigned char *cascade_0200_msg;
	cascade_0200_msg = (unsigned char *)malloc((msg_len_bits + 7) / 8 + 2);
	if (cascade_0200_msg == NULL)
	{
		fprintf(stderr, "ERROR: Memory allocation failed at %s, line %d. \n", __FILE__, __LINE__);
		return MEMORY_ALLOCATION_FAILED;
	}
	cascade_0200_msg[0] = 0x02;
	cascade_0200_msg[1] = 0x00;
	memcpy(cascade_0200_msg + 2, msg, (msg_len_bits + 7) / 8);
	if (hmac(cascade_0200_msg, 16 + msg_len_bits, sm3_hmac_default_key, sm3_hmac_default_key_len_bits, k1) != 0)
	{
		fprintf(stderr, "ERROR: HMAC failed at %s, line %d. \n", __FILE__, __LINE__);
		return HASH_FAILED;
	}
	// h1
	unsigned char h1[32];
	unsigned char *cascade_msg_0200;
	cascade_msg_0200 = (unsigned char *)malloc((msg_len_bits + 7) / 8 + 2);
	if (cascade_msg_0200 == NULL)
	{
		fprintf(stderr, "ERROR: Memory allocation failed at %s, line %d. \n", __FILE__, __LINE__);
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
	uint32_t msg_len;

	if (byte_len_u32(msg_len_bits, &msg_len))
	{
		return pseudohash_768_bytes(msg, msg_len, hash768);
	}

	// k1
	unsigned char k1[32];
	unsigned char *cascade_0300_msg;
	cascade_0300_msg = (unsigned char *)malloc((msg_len_bits + 7) / 8 + 2);
	if (cascade_0300_msg == NULL)
	{
		fprintf(stderr, "ERROR: Memory allocation failed at %s, line %d. \n", __FILE__, __LINE__);
		return MEMORY_ALLOCATION_FAILED;
	}
	cascade_0300_msg[0] = 0x03;
	cascade_0300_msg[1] = 0x00;
	memcpy(cascade_0300_msg + 2, msg, (msg_len_bits + 7) / 8);
	if (hmac(cascade_0300_msg, 16 + msg_len_bits, sm3_hmac_default_key, sm3_hmac_default_key_len_bits, k1) != 0)
	{
		fprintf(stderr, "ERROR: HMAC failed at %s, line %d. \n", __FILE__, __LINE__);
		return HASH_FAILED;
	}
	// h1
	unsigned char h1[32];
	unsigned char *cascade_msg_0300;
	cascade_msg_0300 = (unsigned char *)malloc((msg_len_bits + 7) / 8 + 2);
	if (cascade_msg_0300 == NULL)
	{
		fprintf(stderr, "ERROR: Memory allocation failed at %s, line %d. \n", __FILE__, __LINE__);
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
	if (hmac(h1, (sizeof(h1) / sizeof(h1[0])) * 8, sm3_hmac_default_key, sm3_hmac_default_key_len_bits, k2) != 0)
	{
		fprintf(stderr, "ERROR: HMAC failed at %s, line %d. \n", __FILE__, __LINE__);
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
	uint32_t msg_len;

	if (byte_len_u32(msg_len_bits, &msg_len))
	{
		return pseudohash_1024_bytes(msg, msg_len, hash1024);
	}

	// k1
	unsigned char k1[32];
	unsigned char *cascade_0400_msg;
	cascade_0400_msg = (unsigned char *)malloc((msg_len_bits + 7) / 8 + 2);
	if (cascade_0400_msg == NULL)
	{
		fprintf(stderr, "ERROR: Memory allocation failed at %s, line %d. \n", __FILE__, __LINE__);
		return MEMORY_ALLOCATION_FAILED;
	}
	cascade_0400_msg[0] = 0x04;
	cascade_0400_msg[1] = 0x00;
	memcpy(cascade_0400_msg + 2, msg, (msg_len_bits + 7) / 8);
	if (hmac(cascade_0400_msg, 16 + msg_len_bits, sm3_hmac_default_key, sm3_hmac_default_key_len_bits, k1) != 0)
	{
		fprintf(stderr, "ERROR: HMAC failed at %s, line %d. \n", __FILE__, __LINE__);
		return HASH_FAILED;
	}
	// h1
	unsigned char h1[32];
	unsigned char *cascade_msg_0400;
	cascade_msg_0400 = (unsigned char *)malloc((msg_len_bits + 7) / 8 + 2);
	if (cascade_msg_0400 == NULL)
	{
		fprintf(stderr, "ERROR: Memory allocation failed at %s, line %d. \n", __FILE__, __LINE__);
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
	if (hmac(h1, (sizeof(h1) / sizeof(h1[0])) * 8, sm3_hmac_default_key, sm3_hmac_default_key_len_bits, k2) != 0)
	{
		fprintf(stderr, "ERROR: HMAC failed at %s, line %d. \n", __FILE__, __LINE__);
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
	if (hmac(h2, (sizeof(h2) / sizeof(h2[0])) * 8, sm3_hmac_default_key, sm3_hmac_default_key_len_bits, k3) != 0)
	{
		fprintf(stderr, "ERROR: HMAC failed at %s, line %d. \n", __FILE__, __LINE__);
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
	if (hmac(h3, (sizeof(h3) / sizeof(h3[0])) * 8, sm3_hmac_default_key, sm3_hmac_default_key_len_bits, k4) != 0)
	{
		fprintf(stderr, "ERROR: HMAC failed at %s, line %d. \n", __FILE__, __LINE__);
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
		uint32_t msg_len;
		if (byte_len_u32(msg_len_bits, &msg_len))
		{
			sm3_hash_bytes(msg, msg_len, digest);
		}
		else
		{
			sm3_bit(msg, msg_len_bits, digest);
		}
		return HASH_SUCCESS;
	}
	else
	{
		fprintf(stderr, "ERROR: Digestbitlen not in the limitation. \n");
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
		fprintf(stderr, "ERROR: Digestbitlen not in the limitation. \n");
		return INVALID_DIGESTBITLEN;
	}
}

static inline void sm3_update_byte_no_count(sm3_ctx *ctx, unsigned char byte)
{
	ctx->block[ctx->used++] = byte;
	if (ctx->used == 64u)
	{
		sm3_bit_compress(ctx->digest, ctx->block, 1);
		ctx->used = 0;
	}
}

static void sm3_update_be32_counter(sm3_ctx *ctx, uint32_t ct)
{
	ctx->bitlen += 32u;
	sm3_update_byte_no_count(ctx, (unsigned char)(ct >> 24));
	sm3_update_byte_no_count(ctx, (unsigned char)(ct >> 16));
	sm3_update_byte_no_count(ctx, (unsigned char)(ct >> 8));
	sm3_update_byte_no_count(ctx, (unsigned char)ct);
}

static void put_digest_words_partial(unsigned char *out, const uint32_t words[8], uint32_t bytes)
{
	if (bytes == 32u && ((uintptr_t)out & 3u) == 0u)
	{
		sm3_store8_be_m4(out, words);
		return;
	}

	for (unsigned int i = 0; bytes != 0u; i++)
	{
		uint32_t word = words[i];

		if (bytes >= 4u)
		{
			PUT32(out, word);
			out += 4;
			bytes -= 4u;
		}
		else
		{
			*out++ = (unsigned char)(word >> 24);
			if (bytes > 1u)
			{
				*out++ = (unsigned char)(word >> 16);
			}
			if (bytes > 2u)
			{
				*out = (unsigned char)(word >> 8);
			}
			break;
		}
	}
}

static int pseudoXOF_bytes(unsigned long long output_len_bits,
						   const unsigned char *msg,
						   uint32_t msg_len,
						   unsigned char *output)
{
	uint32_t output_bytes;
	uint32_t blocks;
	uint32_t offset = 0;
	sm3_ctx base;

	if (!ceil_byte_len_u32(output_len_bits, &output_bytes))
	{
		return 0;
	}

	blocks = (uint32_t)((output_len_bits + 255ULL) >> 8);
	sm3_ctx_init(&base);
	sm3_update_bytes(&base, msg, msg_len);

	/* Fast counter-mode path: when the 4-byte counter, the 0x80 pad and the
	 * 64-bit length all fit in the same block as the buffered prefix, every
	 * output block is one SM3 compression whose 64-byte input differs only in
	 * the 4 counter bytes. Build the 16 message words once and rewrite only
	 * the 1-2 words holding the counter per block, skipping the per-block ctx
	 * copy, byte-wise counter append and padding setup. */
	if (base.used <= 51u)
	{
		unsigned char blk[64] __attribute__((aligned(4)));
		uint32_t Wt[16];
		uint64_t total_bitlen = base.bitlen + 32u;
		uint32_t cpos = base.used;          /* counter byte offset */
		uint32_t w0 = cpos >> 2;            /* first word holding the counter */
		uint32_t w1 = (cpos + 3u) >> 2;     /* last word holding the counter */
		unsigned int i;

		memset(blk, 0, sizeof(blk));
		memcpy(blk, base.block, base.used);
		blk[cpos + 4u] = 0x80u;
		PUT32(blk + 56, (uint32_t)(total_bitlen >> 32));
		PUT32(blk + 60, (uint32_t)total_bitlen);
		for (i = 0; i < 16u; i++)
		{
			Wt[i] = load_be32(blk + i * 4u);
		}

		for (uint32_t b = 0, ct = 1; b < blocks; b++, ct++, offset += 32u)
		{
			uint32_t dg[8];
			uint32_t todo = output_bytes - offset;

			if (todo > 32u)
			{
				todo = 32u;
			}

			blk[cpos + 0u] = (unsigned char)(ct >> 24);
			blk[cpos + 1u] = (unsigned char)(ct >> 16);
			blk[cpos + 2u] = (unsigned char)(ct >> 8);
			blk[cpos + 3u] = (unsigned char)ct;
			Wt[w0] = load_be32(blk + w0 * 4u);
			if (w1 != w0)
			{
				Wt[w1] = load_be32(blk + w1 * 4u);
			}

			memcpy(dg, base.digest, sizeof(dg));
			sm3_compress_8w_8w_words(dg, &Wt[0], &Wt[8]);
			put_digest_words_partial(output + offset, dg, todo);
		}
	}
	else
	{
		for (uint32_t i = 0, ct = 1; i < blocks; i++, ct++, offset += 32u)
		{
			uint32_t words[8];
			uint32_t todo = output_bytes - offset;
			sm3_ctx ctx = base;

			if (todo > 32u)
			{
				todo = 32u;
			}

			sm3_update_be32_counter(&ctx, ct);
			sm3_final_words(&ctx, words);
			put_digest_words_partial(output + offset, words, todo);
		}
	}

	if ((output_len_bits & 7ULL) != 0ULL)
	{
		normalize(output, output_len_bits);
	}

	return 1;
}

// GB/T 32918.4-2016 Section 5.4.3
int pseudoXOF(unsigned long long output_len_bits, const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *output)
{
	uint32_t msg_len;

	if (byte_len_u32(msg_len_bits, &msg_len) &&
		pseudoXOF_bytes(output_len_bits, msg, msg_len, output))
	{
		return XOF_SUCCESS;
	}

	unsigned int ct = 1;
	unsigned char *cascade_msg_ct;
	cascade_msg_ct = (unsigned char *)malloc((msg_len_bits + 7) / 8 + 4);
	if (cascade_msg_ct == NULL)
	{
		fprintf(stderr, "ERROR: Memory allocation failed at %s, line %d. \n", __FILE__, __LINE__);
		return MEMORY_ALLOCATION_FAILED;
	}
	unsigned char *K;
	K = (unsigned char *)malloc((output_len_bits + 255) / 8);
	if (K == NULL)
	{
		fprintf(stderr, "ERROR: Memory allocation failed at %s, line %d. \n", __FILE__, __LINE__);
		return MEMORY_ALLOCATION_FAILED;
	}

	for (unsigned int i = 0; i < (output_len_bits + 255) / 256; i++)
	{
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
		sm3_bit(cascade_msg_ct, msg_len_bits + 32, K + i * 32);
		ct++;
	}
	memcpy(output, K, (output_len_bits + 7) / 8);
	free(cascade_msg_ct);
	free(K);
	if (output_len_bits % 256 != 0)
	{
		normalize(output, output_len_bits);
	}
	return XOF_SUCCESS;
}
