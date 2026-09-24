/**
 * @file aes_aesni.c
 * @brief x86_64 AES-NI AES-128 CTR implementation for Optimized AES builds.
 *
 * This backend accelerates public-matrix byte generation with AES-NI.  It
 * shares the `aes.h` interface with the portable and AArch64 backends.
 */

#include <stdint.h>
#include <string.h>
#include <wmmintrin.h>
#include "aes.h"

static __m128i key_expand(__m128i key, __m128i keygened)
{
	key = _mm_xor_si128(key, _mm_slli_si128(key, 4));
	key = _mm_xor_si128(key, _mm_slli_si128(key, 4));
	key = _mm_xor_si128(key, _mm_slli_si128(key, 4));
	keygened = _mm_shuffle_epi32(keygened, _MM_SHUFFLE(3, 3, 3, 3));
	return _mm_xor_si128(key, keygened);
}


#define key_exp(k, rcon) key_expand(k, _mm_aeskeygenassist_si128(k, rcon))

void AES128_load_schedule(const uint8_t *key, uint8_t *_schedule)
{
	__m128i *schedule = (__m128i *)_schedule;
	schedule[0] = _mm_loadu_si128((const __m128i *)key);
	schedule[1] = key_exp(schedule[0], 0x01);
	schedule[2] = key_exp(schedule[1], 0x02);
	schedule[3] = key_exp(schedule[2], 0x04);
	schedule[4] = key_exp(schedule[3], 0x08);
	schedule[5] = key_exp(schedule[4], 0x10);
	schedule[6] = key_exp(schedule[5], 0x20);
	schedule[7] = key_exp(schedule[6], 0x40);
	schedule[8] = key_exp(schedule[7], 0x80);
	schedule[9] = key_exp(schedule[8], 0x1b);
	schedule[10] = key_exp(schedule[9], 0x36);
}

static inline __m128i aes128_enc_m128(__m128i m, const uint8_t *_schedule)
{
	__m128i *schedule = (__m128i *)_schedule;

	m = _mm_xor_si128(m, schedule[0]);
	for (size_t i = 1; i < 10; i++)
	{
		m = _mm_aesenc_si128(m, schedule[i]);
	}
	return _mm_aesenclast_si128(m, schedule[10]);
}

void AES128_CTR_zero_sch(uint32_t counter_start, size_t block_count,
						 const uint8_t *schedule, uint8_t *ciphertext)
{
	while (block_count >= 4U)
	{
		__m128i block0 = _mm_cvtsi32_si128((int)(counter_start + 0U));
		__m128i block1 = _mm_cvtsi32_si128((int)(counter_start + 1U));
		__m128i block2 = _mm_cvtsi32_si128((int)(counter_start + 2U));
		__m128i block3 = _mm_cvtsi32_si128((int)(counter_start + 3U));

		block0 = aes128_enc_m128(block0, schedule);
		block1 = aes128_enc_m128(block1, schedule);
		block2 = aes128_enc_m128(block2, schedule);
		block3 = aes128_enc_m128(block3, schedule);

		_mm_storeu_si128((__m128i *)(ciphertext + 0U * 16U), block0);
		_mm_storeu_si128((__m128i *)(ciphertext + 1U * 16U), block1);
		_mm_storeu_si128((__m128i *)(ciphertext + 2U * 16U), block2);
		_mm_storeu_si128((__m128i *)(ciphertext + 3U * 16U), block3);

		counter_start += 4U;
		ciphertext += 4U * 16U;
		block_count -= 4U;
	}
	while (block_count > 0U)
	{
		const __m128i block =
			aes128_enc_m128(_mm_cvtsi32_si128((int)counter_start), schedule);

		_mm_storeu_si128((__m128i *)ciphertext, block);
		counter_start++;
		ciphertext += 16U;
		block_count--;
	}
}

void AES128_free_schedule(uint8_t *schedule) { memset(schedule, 0, 16 * 11); }
