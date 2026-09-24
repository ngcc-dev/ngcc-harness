#include "aes.h"

#include <assert.h>

#define KEYGEN_WIDTH 4

typedef struct
{
	block128 key_slices[SECURITY_PARAM / 32];

	block128 next_sbox;
} aes_keygen_state;

ALWAYS_INLINE void cumulative_xor(block128* x, size_t n)
{
	#ifdef __GNUC__
	_Pragma(STRINGIZE(GCC unroll (2*KEYGEN_WIDTH)))
	#endif
	for (size_t i = 0; i + 1 < n; ++i)
		x[i + 1] = block128_xor(x[i], x[i + 1]);
}

ALWAYS_INLINE void shift2_mod6(block128* x)
{
	block128 output[6];
	memcpy(output + 2, x, 4 * sizeof(block128));
	memcpy(output, x + 4, 2 * sizeof(block128));
	memcpy(x, output, 6 * sizeof(block128));
}

ALWAYS_INLINE void shift4_mod8(block128* x)
{
	block128 output[8];
	memcpy(output + 4, x, 4 * sizeof(block128));
	memcpy(output, x + 4, 4 * sizeof(block128));
	memcpy(x, output, 8 * sizeof(block128));
}

ALWAYS_INLINE void aes_keygen_init(
	aes_keygen_state* keygen_states, aes_round_keys* aeses,
	const block_secpar* keys_in, size_t num_keys)
{
	#ifdef __GNUC__
	_Pragma(STRINGIZE(GCC unroll (2*AES_PREFERRED_WIDTH / KEYGEN_WIDTH)))
	#endif
	for (size_t i = 0; i < num_keys;
	     i += KEYGEN_WIDTH, keys_in += KEYGEN_WIDTH, aeses += KEYGEN_WIDTH, ++keygen_states)
	{
		size_t chunk_size = num_keys - i < KEYGEN_WIDTH ? num_keys - i : KEYGEN_WIDTH;
		block_secpar keys[KEYGEN_WIDTH];

		#ifdef __GNUC__
		_Pragma(STRINGIZE(GCC unroll (KEYGEN_WIDTH)))
		#endif
		for (size_t j = 0; j < chunk_size; ++j)
		{
			keys[j] = keys_in[j];

			memcpy(&aeses[j].keys[0], &keys[j], sizeof(block_secpar));
		}
		for (size_t j = chunk_size; j < KEYGEN_WIDTH; ++j)
			keys[j] = block_secpar_set_zero();

#if SECURITY_PARAM == 128
		transpose4x4_32(&keygen_states->key_slices[0], &keys[0]);

#elif SECURITY_PARAM == 192
		block128 low128s[KEYGEN_WIDTH];
		for (size_t j = 0; j < KEYGEN_WIDTH; ++j)
			memcpy(&low128s[j], &keys[j], sizeof(block128));
		block128 hi64_01 = _mm_set_epi64x(keys[1].data[2], keys[0].data[2]);
		block128 hi64_23 = _mm_set_epi64x(keys[3].data[2], keys[2].data[2]);

		transpose4x4_32(&keygen_states->key_slices[0], &low128s[0]);
		transpose4x2_32(&keygen_states->key_slices[4], hi64_01, hi64_23);

#elif SECURITY_PARAM == 256
		block128 low128s[KEYGEN_WIDTH];
		block128 hi128s[KEYGEN_WIDTH];
		for (size_t j = 0; j < KEYGEN_WIDTH; ++j)
		{
			memcpy(&low128s[j], &keys[j], sizeof(block128));
			memcpy(&hi128s[j], ((const char*) &keys[j]) + sizeof(block128), sizeof(block128));
		}

		transpose4x4_32(&keygen_states->key_slices[0], &low128s[0]);
		transpose4x4_32(&keygen_states->key_slices[4], &hi128s[0]);
#endif

		keygen_states->next_sbox = keygen_states->key_slices[SECURITY_PARAM / 32 - 1];

		if (SECURITY_PARAM == 128)
			cumulative_xor(&keygen_states->key_slices[0], 4);
		else if (SECURITY_PARAM == 192)
		{
			shift2_mod6(&keygen_states->key_slices[0]);
			cumulative_xor(&keygen_states->key_slices[2], 4);
		}
		else
		{
			cumulative_xor(&keygen_states->key_slices[0], 4);
			cumulative_xor(&keygen_states->key_slices[4], 4);
		}
	}
}

ALWAYS_INLINE void aes_keygen_round(
	aes_keygen_state* keygen_states, aes_round_keys* aeses, size_t num_keys, int round)
{
	if (round < SECURITY_PARAM / 128) return;

	#ifdef __GNUC__
	_Pragma(STRINGIZE(GCC unroll (2*AES_PREFERRED_WIDTH / KEYGEN_WIDTH)))
	#endif
	for (size_t i = 0; i < num_keys; i += KEYGEN_WIDTH, aeses += KEYGEN_WIDTH, ++keygen_states)
	{
		size_t chunk_size = num_keys - i < KEYGEN_WIDTH ? num_keys - i : KEYGEN_WIDTH;

		if (SECURITY_PARAM != 192 || round % 3 < 2)
		{
			block128 inv_shift_rows =
				_mm_setr_epi8( 0, 13, 10,  7,  4,  1, 14, 11,  8,  5,  2, 15, 12,  9,  6,  3);
			block128 rot_word_then_inv_shift_rows =
				_mm_setr_epi8( 1, 14, 11,  4,  5,  2, 15,  8,  9,  6,  3, 12, 13, 10,  7,  0);

			block128 perm, round_constant;
			if (SECURITY_PARAM == 256 && round % 2 == 1)
			{
				perm = inv_shift_rows;
				round_constant = _mm_setzero_si128();
			}
			else
			{
				perm = rot_word_then_inv_shift_rows;
				int idx = (2 * round + 1) / (SECURITY_PARAM / 64);
				round_constant = _mm_set1_epi32(aes_round_constants[idx - 1]);
			}

			block128 sbox_out = _mm_aesenclast_si128(
				_mm_shuffle_epi8(keygen_states->next_sbox, perm), round_constant);

			size_t range_start = (SECURITY_PARAM == 192 && round % 3 == 1) ? 2 : 0;
			size_t range_end = (SECURITY_PARAM == 192) ? 6 : 4;
			for (size_t j = range_start; j < range_end; ++j)
				keygen_states->key_slices[j] = block128_xor(keygen_states->key_slices[j], sbox_out);
		}

		if (SECURITY_PARAM != 192 || round % 3 == 2)
			keygen_states->next_sbox = keygen_states->key_slices[3];
		else if (round % 3 == 0)
			keygen_states->next_sbox = keygen_states->key_slices[5];

		block128 round_keys[KEYGEN_WIDTH];
		transpose4x4_32(round_keys, &keygen_states->key_slices[0]);
		#ifdef __GNUC__
		_Pragma(STRINGIZE(GCC unroll (KEYGEN_WIDTH)))
		#endif
		for (size_t j = 0; j < chunk_size; ++j)
			aeses[j].keys[round] = round_keys[j];

		if (SECURITY_PARAM == 192)
		{
			shift2_mod6(&keygen_states->key_slices[0]);
			size_t next_sbox_idx = (round % 3 + 1) * 2;
			cumulative_xor(&keygen_states->key_slices[1], next_sbox_idx - 1);
			cumulative_xor(&keygen_states->key_slices[next_sbox_idx], 6 - next_sbox_idx);
		}
		else
		{
			cumulative_xor(&keygen_states->key_slices[0], 4);
			if (SECURITY_PARAM == 256)
				shift4_mod8(&keygen_states->key_slices[0]);
		}
	}
}

ALWAYS_INLINE void aes_keygen_impl(
	aes_round_keys* aeses, const block_secpar* keys,
	size_t num_keys, uint32_t num_blocks, block128* output)
{
	aes_keygen_state keygen_states[AES_PREFERRED_WIDTH / KEYGEN_WIDTH];
	aes_keygen_init(keygen_states, aeses, keys, num_keys);

#if SECURITY_PARAM == 128
	int round_start = 1;
	int round_end = AES_ROUNDS - 1;
	int unroll_rounds = 1;
#elif SECURITY_PARAM == 192
	int round_start = 1;
	int round_end = AES_ROUNDS - 1;
	int unroll_rounds = 3;
#elif SECURITY_PARAM == 256
	int round_start = 2;
	int round_end = AES_ROUNDS - 2;
	int unroll_rounds = 2;
#endif

	aes_round(aeses, output, num_keys, num_blocks, 0);
	if (round_start > 1)
		aes_round(aeses, output, num_keys, num_blocks, 1);

	for (int round = round_start; round <= round_end; round += unroll_rounds)
	{

		aes_keygen_round(keygen_states, aeses, num_keys, round);
		aes_round(aeses, output, num_keys, num_blocks, round);
		if (unroll_rounds > 1)
		{
			if (round_end < round + 1)
				break;
			aes_keygen_round(keygen_states, aeses, num_keys, round + 1);
			aes_round(aeses, output, num_keys, num_blocks, round + 1);
		}
		if (unroll_rounds > 2)
		{
			if (round_end < round + 2)
				break;
			aes_keygen_round(keygen_states, aeses, num_keys, round + 2);
			aes_round(aeses, output, num_keys, num_blocks, round + 2);
		}
	}

	aes_keygen_round(keygen_states, aeses, num_keys, round_end + 1);
	aes_round(aeses, output, num_keys, num_blocks, round_end + 1);
	if (round_end + 2 <= AES_ROUNDS)
	{
		aes_keygen_round(keygen_states, aeses, num_keys, round_end + 2);
		aes_round(aeses, output, num_keys, num_blocks, round_end + 2);
	}
}

#define DEF_AES_KEYGEN_IMPL_KB(num_keys,num_blocks) \
	void aes_keygen_impl_##num_keys##_##num_blocks( \
		aes_round_keys* restrict aeses, const block_secpar* restrict keys, \
		block128* restrict output) \
	{ \
		assert(num_keys <= AES_PREFERRED_WIDTH && num_blocks <= 3);\
 \
		if (num_keys <= AES_PREFERRED_WIDTH && num_blocks <= 3) \
			aes_keygen_impl(aeses, keys, num_keys, num_blocks, output); \
	}
#define DEF_AES_KEYGEN_IMPL_K(num_keys) \
	DEF_AES_KEYGEN_IMPL_KB(num_keys, 1) \
	DEF_AES_KEYGEN_IMPL_KB(num_keys, 2) \
	DEF_AES_KEYGEN_IMPL_KB(num_keys, 3)

static_assert(AES_PREFERRED_WIDTH <= 16, "");
DEF_AES_KEYGEN_IMPL_K(1)
DEF_AES_KEYGEN_IMPL_K(2)
DEF_AES_KEYGEN_IMPL_K(3)
DEF_AES_KEYGEN_IMPL_K(4)
DEF_AES_KEYGEN_IMPL_K(5)
DEF_AES_KEYGEN_IMPL_K(6)
DEF_AES_KEYGEN_IMPL_K(7)
DEF_AES_KEYGEN_IMPL_K(8)
DEF_AES_KEYGEN_IMPL_K(9)
DEF_AES_KEYGEN_IMPL_K(10)
DEF_AES_KEYGEN_IMPL_K(11)
DEF_AES_KEYGEN_IMPL_K(12)
DEF_AES_KEYGEN_IMPL_K(13)
DEF_AES_KEYGEN_IMPL_K(14)
DEF_AES_KEYGEN_IMPL_K(15)
DEF_AES_KEYGEN_IMPL_K(16)

void aes_keygen(aes_round_keys* round_keys, block_secpar key)
{
	block128 empty_output;
	aes_keygen_impl(round_keys, &key, 1, 0, &empty_output);
}

static inline block128 load_high_128(const block256* block)
{
	block128 out;
	memcpy(&out, ((unsigned char*) block) + sizeof(block128), sizeof(out));
	return out;
}

static inline block128 load_high_64(const block192* block)
{
	return _mm_cvtsi64_si128(block->data[2]);
}

static void rijndael192_keygen_helper(
	const block192* round_key_in, block128 kga, block192* round_key_out)
{
	block128 t1, t2, t4;
	uint64_t t3;

	memcpy(&t1, &round_key_in->data[0], sizeof(t1));
	t2 = kga;
	t3 = round_key_in->data[2];

	t2 = _mm_shuffle_epi32(t2, 0x55);
	t4 = _mm_slli_si128(t1, 0x4);
	t1 = _mm_xor_si128(t1, t4);
	t4 = _mm_slli_si128(t4, 0x4);
	t1 = _mm_xor_si128(t1, t4);
	t4 = _mm_slli_si128(t4, 0x4);
	t1 = _mm_xor_si128(t1, t4);
	t1 = _mm_xor_si128(t1, t2);
	t3 ^= (uint32_t) _mm_extract_epi32(t1, 3);
	t3 ^= t3 << 32;

	memcpy(&round_key_out->data[0], &t1, sizeof(t1));
	round_key_out->data[2] = t3;
}

static void rijndael256_keygen_helper(
	const block256* round_key_in, block128 kga, block256* round_key_out)
{
	block128 t1, t2, t3, t4;

	memcpy(&t1, round_key_in, sizeof(t1));
	t3 = load_high_128(round_key_in);
	t2 = kga;

	t2 = _mm_shuffle_epi32(t2, 0xff);
	t4 = _mm_slli_si128(t1, 0x4);
	t1 = _mm_xor_si128(t1, t4);
	t4 = _mm_slli_si128(t4, 0x4);
	t1 = _mm_xor_si128(t1, t4);
	t4 = _mm_slli_si128(t4, 0x4);
	t1 = _mm_xor_si128(t1, t4);
	t1 = _mm_xor_si128(t1, t2);

	memcpy(round_key_out, &t1, sizeof(t1));

	t4 = _mm_aeskeygenassist_si128(t1, 0x00);
	t2 = _mm_shuffle_epi32(t4, 0xaa);
	t4 = _mm_slli_si128(t3, 0x4);
	t3 = _mm_xor_si128(t3, t4);
	t4 = _mm_slli_si128(t4, 0x4);
	t3 = _mm_xor_si128(t3, t4);
	t4 = _mm_slli_si128(t4, 0x4);
	t3 = _mm_xor_si128(t3, t4);
	t3 = _mm_xor_si128(t3, t2);

	memcpy(((unsigned char*) round_key_out) + sizeof(t1), &t3, sizeof(t3));
}

void rijndael192_keygen(rijndael192_round_keys* round_keys, block192 key)
{
	round_keys->keys[0] = key;

	block128 kga;
	kga = _mm_aeskeygenassist_si128(load_high_64(&round_keys->keys[0]), 0x01);
	rijndael192_keygen_helper(&round_keys->keys[0], kga, &round_keys->keys[1]);
	kga = _mm_aeskeygenassist_si128(load_high_64(&round_keys->keys[1]), 0x02);
	rijndael192_keygen_helper(&round_keys->keys[1], kga, &round_keys->keys[2]);
	kga = _mm_aeskeygenassist_si128(load_high_64(&round_keys->keys[2]), 0x04);
	rijndael192_keygen_helper(&round_keys->keys[2], kga, &round_keys->keys[3]);
	kga = _mm_aeskeygenassist_si128(load_high_64(&round_keys->keys[3]), 0x08);
	rijndael192_keygen_helper(&round_keys->keys[3], kga, &round_keys->keys[4]);
	kga = _mm_aeskeygenassist_si128(load_high_64(&round_keys->keys[4]), 0x10);
	rijndael192_keygen_helper(&round_keys->keys[4], kga, &round_keys->keys[5]);
	kga = _mm_aeskeygenassist_si128(load_high_64(&round_keys->keys[5]), 0x20);
	rijndael192_keygen_helper(&round_keys->keys[5], kga, &round_keys->keys[6]);
	kga = _mm_aeskeygenassist_si128(load_high_64(&round_keys->keys[6]), 0x40);
	rijndael192_keygen_helper(&round_keys->keys[6], kga, &round_keys->keys[7]);
	kga = _mm_aeskeygenassist_si128(load_high_64(&round_keys->keys[7]), 0x80);
	rijndael192_keygen_helper(&round_keys->keys[7], kga, &round_keys->keys[8]);
	kga = _mm_aeskeygenassist_si128(load_high_64(&round_keys->keys[8]), 0x1B);
	rijndael192_keygen_helper(&round_keys->keys[8], kga, &round_keys->keys[9]);
	kga = _mm_aeskeygenassist_si128(load_high_64(&round_keys->keys[9]), 0x36);
	rijndael192_keygen_helper(&round_keys->keys[9], kga, &round_keys->keys[10]);
	kga = _mm_aeskeygenassist_si128(load_high_64(&round_keys->keys[10]), 0x6C);
	rijndael192_keygen_helper(&round_keys->keys[10], kga, &round_keys->keys[11]);
	kga = _mm_aeskeygenassist_si128(load_high_64(&round_keys->keys[11]), 0xD8);
	rijndael192_keygen_helper(&round_keys->keys[11], kga, &round_keys->keys[12]);
}

void rijndael256_keygen(rijndael256_round_keys* round_keys, block256 key)
{
	round_keys->keys[0] = key;

	block128 kga;
	kga = _mm_aeskeygenassist_si128(load_high_128(&round_keys->keys[0]), 0x01);
	rijndael256_keygen_helper(&round_keys->keys[0], kga, &round_keys->keys[1]);
	kga = _mm_aeskeygenassist_si128(load_high_128(&round_keys->keys[1]), 0x02);
	rijndael256_keygen_helper(&round_keys->keys[1], kga, &round_keys->keys[2]);
	kga = _mm_aeskeygenassist_si128(load_high_128(&round_keys->keys[2]), 0x04);
	rijndael256_keygen_helper(&round_keys->keys[2], kga, &round_keys->keys[3]);
	kga = _mm_aeskeygenassist_si128(load_high_128(&round_keys->keys[3]), 0x08);
	rijndael256_keygen_helper(&round_keys->keys[3], kga, &round_keys->keys[4]);
	kga = _mm_aeskeygenassist_si128(load_high_128(&round_keys->keys[4]), 0x10);
	rijndael256_keygen_helper(&round_keys->keys[4], kga, &round_keys->keys[5]);
	kga = _mm_aeskeygenassist_si128(load_high_128(&round_keys->keys[5]), 0x20);
	rijndael256_keygen_helper(&round_keys->keys[5], kga, &round_keys->keys[6]);
	kga = _mm_aeskeygenassist_si128(load_high_128(&round_keys->keys[6]), 0x40);
	rijndael256_keygen_helper(&round_keys->keys[6], kga, &round_keys->keys[7]);
	kga = _mm_aeskeygenassist_si128(load_high_128(&round_keys->keys[7]), 0x80);
	rijndael256_keygen_helper(&round_keys->keys[7], kga, &round_keys->keys[8]);
	kga = _mm_aeskeygenassist_si128(load_high_128(&round_keys->keys[8]), 0x1B);
	rijndael256_keygen_helper(&round_keys->keys[8], kga, &round_keys->keys[9]);
	kga = _mm_aeskeygenassist_si128(load_high_128(&round_keys->keys[9]), 0x36);
	rijndael256_keygen_helper(&round_keys->keys[9], kga, &round_keys->keys[10]);
	kga = _mm_aeskeygenassist_si128(load_high_128(&round_keys->keys[10]), 0x6C);
	rijndael256_keygen_helper(&round_keys->keys[10], kga, &round_keys->keys[11]);
	kga = _mm_aeskeygenassist_si128(load_high_128(&round_keys->keys[11]), 0xD8);
	rijndael256_keygen_helper(&round_keys->keys[11], kga, &round_keys->keys[12]);
	kga = _mm_aeskeygenassist_si128(load_high_128(&round_keys->keys[12]), 0xAB);
	rijndael256_keygen_helper(&round_keys->keys[12], kga, &round_keys->keys[13]);
	kga = _mm_aeskeygenassist_si128(load_high_128(&round_keys->keys[13]), 0x4D);
	rijndael256_keygen_helper(&round_keys->keys[13], kga, &round_keys->keys[14]);
}

static inline void cvt192_to_2x128(block128* out, const block192* in)
{
	memcpy(&out[0], &in->data[0], sizeof(out[0]));
	out[1] = _mm_set1_epi64x(in->data[2]);
}

ALWAYS_INLINE void rijndael192_rotate_rows_undo_128(block128* s)
{
	block128 mask = _mm_setr_epi8(
		0, -1, -1,  0,
		0,  0, -1, -1,
		0,  0,  0, -1,
		0,  0,  0,  0);
	block128 b0_blended = _mm_blendv_epi8(s[0], s[1], mask);
	block128 b1_blended = _mm_blendv_epi8(s[1], s[0], mask);

	block128 shuffle_b0 = _mm_setr_epi8(
		 0,  1,  2, 11,
		 4,  5,  6,  7,
		 8,  9, 10,  3,
		12, 13, 14, 15);
	block128 shuffle_b1 = _mm_setr_epi8(
		 0,  1,  2, 11,
		 4,  5,  6,  7,
		 0,  1,  2, 11,
		 4,  5,  6,  7);
	s[0] = _mm_shuffle_epi8(b0_blended, shuffle_b0);
	s[1] = _mm_shuffle_epi8(b1_blended, shuffle_b1);
}

void rijndael192_encrypt_block(
	const rijndael192_round_keys* restrict fixed_key, block192* restrict block)
{
	block192 xored_block = block192_xor(*block, fixed_key->keys[0]);
	block128 state[2], round_key[2];
	cvt192_to_2x128(&state[0], &xored_block);

	for (int round = 1; round < RIJNDAEL192_ROUNDS; ++round)
	{
		cvt192_to_2x128(&round_key[0], &fixed_key->keys[round]);
		rijndael192_rotate_rows_undo_128(&state[0]);
		state[0] = _mm_aesenc_si128(state[0], round_key[0]);
		state[1] = _mm_aesenc_si128(state[1], round_key[1]);
	}

	rijndael192_rotate_rows_undo_128(&state[0]);
	cvt192_to_2x128(&round_key[0], &fixed_key->keys[RIJNDAEL192_ROUNDS]);
	state[0] = _mm_aesenclast_si128(state[0], round_key[0]);
	state[1] = _mm_aesenclast_si128(state[1], round_key[1]);

	memcpy(block, &state[0], sizeof(*block));
}

void rijndael192_round_function(
	const rijndael192_round_keys* restrict round_keys, block192* restrict block,
	block192* restrict after_sbox, int round)
{
	block128 state[2], state_after_sbox[2], round_key[2];
	cvt192_to_2x128(&state[0], block);
	cvt192_to_2x128(&round_key[0], &round_keys->keys[round]);

	rijndael192_rotate_rows_undo_128(&state[0]);
	state_after_sbox[0] = _mm_aesenclast_si128(state[0], block128_set_zero());
	state_after_sbox[1] = _mm_aesenclast_si128(state[1], block128_set_zero());

	if (round < AES_ROUNDS)
	{
		state[0] = _mm_aesenc_si128(state[0], round_key[0]);
		state[1] = _mm_aesenc_si128(state[1], round_key[1]);
	}
	else
	{
		state[0] = block128_xor(state_after_sbox[0], round_key[0]);
		state[1] = block128_xor(state_after_sbox[1], round_key[1]);
	}

	memcpy(after_sbox, &state_after_sbox[0], sizeof(*after_sbox));
	memcpy(block, &state[0], sizeof(*block));
}

void rijndael256_round_function(
	const rijndael256_round_keys* restrict round_keys, block256* restrict block,
	block256* restrict after_sbox, int round)
{
	block128 state[2], state_after_sbox[2], round_key[2];
	memcpy(&state[0], block, sizeof(*block));
	memcpy(&round_key[0], &round_keys->keys[round], sizeof(round_key));

	rijndael256_rotate_rows_undo_128(&state[0]);
	state_after_sbox[0] = _mm_aesenclast_si128(state[0], block128_set_zero());
	state_after_sbox[1] = _mm_aesenclast_si128(state[1], block128_set_zero());

	if (round < AES_ROUNDS)
	{
		state[0] = _mm_aesenc_si128(state[0], round_key[0]);
		state[1] = _mm_aesenc_si128(state[1], round_key[1]);
	}
	else
	{
		state[0] = block128_xor(state_after_sbox[0], round_key[0]);
		state[1] = block128_xor(state_after_sbox[1], round_key[1]);
	}

	memcpy(after_sbox, &state_after_sbox[0], sizeof(*after_sbox));
	memcpy(block, &state[0], sizeof(*block));
}
