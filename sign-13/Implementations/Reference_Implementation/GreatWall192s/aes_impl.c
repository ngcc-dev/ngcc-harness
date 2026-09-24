#include "aes.h"

#include <assert.h>
#include <string.h>

static const uint8_t aes_sbox[256] = {
	0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
	0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
	0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
	0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
	0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
	0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
	0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
	0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
	0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
	0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
	0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
	0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
	0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
	0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
	0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
	0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16,
};

static uint8_t xtime(uint8_t x)
{
	return (uint8_t)((x << 1) ^ ((x >> 7) * 0x1b));
}

static void add_round_key_nb(uint8_t* state, const uint8_t* round_key, size_t nb)
{
	for (size_t i = 0; i < 4 * nb; ++i)
		state[i] ^= round_key[i];
}

static void sub_bytes_nb(uint8_t* state, size_t nb)
{
	for (size_t i = 0; i < 4 * nb; ++i)
		state[i] = aes_sbox[state[i]];
}

static void shift_rows_nb(uint8_t* state, size_t nb)
{
	uint8_t tmp[32];
	assert(nb <= 8);
	for (size_t c = 0; c < nb; ++c)
		for (size_t r = 0; r < 4; ++r)
			tmp[4 * c + r] = state[4 * ((c + r) % nb) + r];
	memcpy(state, tmp, 4 * nb);
}

static void mix_columns_nb(uint8_t* state, size_t nb)
{
	for (size_t c = 0; c < nb; ++c) {
		uint8_t* col = &state[4 * c];
		const uint8_t a0 = col[0];
		const uint8_t a1 = col[1];
		const uint8_t a2 = col[2];
		const uint8_t a3 = col[3];
		const uint8_t t = a0 ^ a1 ^ a2 ^ a3;
		const uint8_t u = a0;
		col[0] ^= t ^ xtime(a0 ^ a1);
		col[1] ^= t ^ xtime(a1 ^ a2);
		col[2] ^= t ^ xtime(a2 ^ a3);
		col[3] ^= t ^ xtime(a3 ^ u);
	}
}

static void rijndael_key_expand(uint8_t* expanded, const uint8_t* key, size_t nb, size_t nk, size_t nr)
{
	memcpy(expanded, key, 4 * nk);

	uint8_t rcon = 1;
	const size_t words = nb * (nr + 1);
	for (size_t i = nk; i < words; ++i) {
		uint8_t temp[4];
		memcpy(temp, expanded + 4 * (i - 1), sizeof(temp));

		if (i % nk == 0) {
			const uint8_t first = temp[0];
			temp[0] = (uint8_t)(aes_sbox[temp[1]] ^ rcon);
			temp[1] = aes_sbox[temp[2]];
			temp[2] = aes_sbox[temp[3]];
			temp[3] = aes_sbox[first];
			rcon = xtime(rcon);
		} else if (nk > 6 && i % nk == 4) {
			for (size_t j = 0; j < 4; ++j)
				temp[j] = aes_sbox[temp[j]];
		}

		for (size_t j = 0; j < 4; ++j)
			expanded[4 * i + j] = expanded[4 * (i - nk) + j] ^ temp[j];
	}
}

static void rijndael_encrypt_bytes(
	const uint8_t* round_keys, size_t nb, size_t nr, uint8_t* state)
{
	add_round_key_nb(state, round_keys, nb);
	for (size_t round = 1; round < nr; ++round) {
		sub_bytes_nb(state, nb);
		shift_rows_nb(state, nb);
		mix_columns_nb(state, nb);
		add_round_key_nb(state, round_keys + 4 * nb * round, nb);
	}
	sub_bytes_nb(state, nb);
	shift_rows_nb(state, nb);
	add_round_key_nb(state, round_keys + 4 * nb * nr, nb);
}

static void rijndael_round_bytes(
	const uint8_t* round_key, size_t nb, size_t nr, uint8_t* state,
	uint8_t* after_sbox, int round)
{
	sub_bytes_nb(state, nb);
	memcpy(after_sbox, state, 4 * nb);
	shift_rows_nb(state, nb);
	if ((size_t)round < nr)
		mix_columns_nb(state, nb);
	add_round_key_nb(state, round_key, nb);
}

block128 aes_add_counter_to_iv_portable(block128 iv, uint32_t ctr)
{
	uint8_t bytes[16];
	memcpy(bytes, &iv, sizeof(bytes));

	uint32_t carry = ctr;
	for (int i = 15; i >= 0 && carry; --i) {
		const uint32_t sum = (uint32_t)bytes[i] + (carry & 0xffu);
		bytes[i] = (uint8_t)sum;
		carry = (carry >> 8) + (sum >> 8);
	}

	block128 out;
	memcpy(&out, bytes, sizeof(out));
	return out;
}

void aes_keygen(aes_round_keys* round_keys, block_secpar key)
{
	uint8_t expanded[16 * (AES_ROUNDS + 1)];
	rijndael_key_expand(expanded, (const uint8_t*)&key, 4, SECURITY_PARAM / 32, AES_ROUNDS);
	for (size_t round = 0; round <= AES_ROUNDS; ++round)
		memcpy(&round_keys->keys[round], expanded + 16 * round, 16);
}

void aes_encrypt_block_portable(
	const aes_round_keys* restrict round_keys, block128 in, block128* restrict out)
{
	uint8_t state[16];
	memcpy(state, &in, sizeof(state));
	rijndael_encrypt_bytes((const uint8_t*)round_keys->keys, 4, AES_ROUNDS, state);
	memcpy(out, state, sizeof(state));
}

void aes_round_function_portable(
	const aes_round_keys* restrict round_keys, block128* restrict block,
	block128* restrict after_sbox, int round)
{
	uint8_t state[16];
	memcpy(state, block, sizeof(state));
	rijndael_round_bytes(
		(const uint8_t*)&round_keys->keys[round], 4, AES_ROUNDS, state,
		(uint8_t*)after_sbox, round);
	memcpy(block, state, sizeof(state));
}

void rijndael192_keygen(rijndael192_round_keys* round_keys, block192 key)
{
	uint8_t expanded[24 * (RIJNDAEL192_ROUNDS + 1)];
	rijndael_key_expand(expanded, (const uint8_t*)&key, 6, 6, RIJNDAEL192_ROUNDS);
	for (size_t round = 0; round <= RIJNDAEL192_ROUNDS; ++round)
		memcpy(&round_keys->keys[round], expanded + 24 * round, 24);
}

void rijndael256_keygen(rijndael256_round_keys* round_keys, block256 key)
{
	uint8_t expanded[32 * (RIJNDAEL256_ROUNDS + 1)];
	rijndael_key_expand(expanded, (const uint8_t*)&key, 8, 8, RIJNDAEL256_ROUNDS);
	for (size_t round = 0; round <= RIJNDAEL256_ROUNDS; ++round)
		memcpy(&round_keys->keys[round], expanded + 32 * round, 32);
}

void rijndael192_encrypt_block(
	const rijndael192_round_keys* restrict fixed_key, block192* restrict block)
{
	rijndael_encrypt_bytes((const uint8_t*)fixed_key->keys, 6, RIJNDAEL192_ROUNDS, (uint8_t*)block);
}

void rijndael256_encrypt_block(
	const rijndael256_round_keys* restrict fixed_key, block256* restrict block)
{
	rijndael_encrypt_bytes((const uint8_t*)fixed_key->keys, 8, RIJNDAEL256_ROUNDS, (uint8_t*)block);
}

void rijndael192_round_function(
	const rijndael192_round_keys* restrict round_keys, block192* restrict block,
	block192* restrict after_sbox, int round)
{
	rijndael_round_bytes(
		(const uint8_t*)&round_keys->keys[round], 6, RIJNDAEL192_ROUNDS,
		(uint8_t*)block, (uint8_t*)after_sbox, round);
}

void rijndael256_round_function(
	const rijndael256_round_keys* restrict round_keys, block256* restrict block,
	block256* restrict after_sbox, int round)
{
	rijndael_round_bytes(
		(const uint8_t*)&round_keys->keys[round], 8, RIJNDAEL256_ROUNDS,
		(uint8_t*)block, (uint8_t*)after_sbox, round);
}
