#include "vole_check.h"

#include <stdbool.h>
#include "hash.h"
#include "universal_hash.h"
#include "vole_params.h"

typedef struct
{
	poly_secpar_vec matrix[4];

	poly_secpar_vec key_secpar;
	poly64_vec key_64;

	hasher_gfsecpar_key hasher_key_secpar;
	hasher_gf64_key hasher_key_64;
} vole_check_challenge;

static vole_check_challenge load_challenge(const uint8_t* in)
{
	vole_check_challenge out;
	for (size_t i = 0; i < 4; ++i, in += SECURITY_PARAM / 8)
		out.matrix[i] = poly_secpar_load_dup(in);

	out.key_secpar = poly_secpar_load_dup(in);
	out.key_64 = poly64_load_dup(in + SECURITY_PARAM / 8);

	hasher_gfsecpar_init_key(&out.hasher_key_secpar, poly_secpar_exp(out.key_secpar, POLY_VEC_LEN));
	hasher_gf64_init_key(&out.hasher_key_64, poly64_exp(out.key_64, POLY_VEC_LEN));
	return out;
}

static void vole_check_both(
	bool verifier, const vole_block* restrict u, const vole_block* restrict vq,
	const uint8_t* restrict delta_bytes,
	const uint8_t* restrict challenge, uint8_t* restrict proof, uint8_t* restrict check)
{

	vole_check_challenge chal = load_challenge(challenge);

	block_secpar u_hash[2] = {block_secpar_set_zero(), block_secpar_set_zero()};
	if (verifier)
		memcpy(&u_hash[0], proof, VOLE_CHECK_HASH_BYTES);

	hash_state hasher;
	hash_init(&hasher);

	for (int col = -!verifier; col < SECURITY_PARAM; ++col)
	{
		const uint8_t* to_hash = (const uint8_t*) (col == -1 ? u : vq + VOLE_COL_BLOCKS * col);

		size_t hasher_chunk_size = POLY_VEC_LEN * SECURITY_PARAM;
		size_t padded_rows =
			((QUICKSILVER_ROWS + hasher_chunk_size - 1) / hasher_chunk_size) * hasher_chunk_size;

		hasher_gfsecpar_state state_secpar;
		hasher_gf64_state state_64;
		hasher_gfsecpar_init_state(&state_secpar, padded_rows / hasher_chunk_size);
		hasher_gf64_init_state(&state_64, padded_rows / (POLY_VEC_LEN * 64));

		size_t i = 0;

		if (padded_rows - QUICKSILVER_ROWS >= SECURITY_PARAM)
		{
			size_t extra_blocks = (padded_rows - QUICKSILVER_ROWS) / SECURITY_PARAM;
			uint8_t first_chunk[POLY_VEC_LEN * sizeof(block_secpar)];
			memset(first_chunk, 0, extra_blocks * sizeof(block_secpar));
			memcpy(first_chunk + extra_blocks * sizeof(block_secpar), to_hash,
			       (POLY_VEC_LEN - extra_blocks) * sizeof(block_secpar));

			hasher_gfsecpar_update(&chal.hasher_key_secpar, &state_secpar, poly_secpar_load(first_chunk));
			for (size_t j = 0; j < hasher_chunk_size; j += POLY_VEC_LEN * 64)
				hasher_gf64_update(&chal.hasher_key_64, &state_64, poly64_load(&first_chunk[j / 8]));

			i = extra_blocks * SECURITY_PARAM;
		}

		for (; i + hasher_chunk_size <= QUICKSILVER_ROWS; i += hasher_chunk_size)
		{
			hasher_gfsecpar_update(&chal.hasher_key_secpar, &state_secpar, poly_secpar_load(to_hash + i / 8));
			for (size_t j = 0; j < hasher_chunk_size; j += POLY_VEC_LEN * 64)
				hasher_gf64_update(&chal.hasher_key_64, &state_64, poly64_load(to_hash + (i + j) / 8));
		}

		assert(i == QUICKSILVER_ROWS - (QUICKSILVER_ROWS % SECURITY_PARAM));
		i = QUICKSILVER_ROWS - (QUICKSILVER_ROWS % SECURITY_PARAM);

		if (QUICKSILVER_ROWS % SECURITY_PARAM)
		{
			uint8_t last_chunk[POLY_VEC_LEN * sizeof(block_secpar)];
			memcpy(last_chunk, to_hash + i / 8, (QUICKSILVER_ROWS - i) / 8);
			memset(last_chunk + (QUICKSILVER_ROWS - i) / 8, 0, sizeof(last_chunk) - (QUICKSILVER_ROWS - i) / 8);

			hasher_gfsecpar_update(&chal.hasher_key_secpar, &state_secpar, poly_secpar_load(last_chunk));
			for (size_t j = 0; j < hasher_chunk_size; j += POLY_VEC_LEN * 64)
				hasher_gf64_update(&chal.hasher_key_64, &state_64, poly64_load(&last_chunk[j / 8]));
		}

		poly_secpar_vec poly_hashes[2];
		poly_hashes[0] =
			gfsecpar_combine_hashes(chal.key_secpar, hasher_gfsecpar_final(&state_secpar));
		poly_hashes[1] =
			poly_secpar_from_64(gf64_combine_hashes(chal.key_64, hasher_gf64_final(&state_64)));

		poly_secpar_vec mapped_hashes[2];
		for (size_t j = 0; j < 2; ++j)
			mapped_hashes[j] =
				poly_2secpar_reduce_secpar(poly_2secpar_add(
					poly_secpar_mul(chal.matrix[2 * j + 0], poly_hashes[0]),
					poly_secpar_mul(chal.matrix[2 * j + 1], poly_hashes[1])));

		block_secpar hash_output_storage[2];
		block_secpar* hash_output = col == -1 ? u_hash : hash_output_storage;
		for (size_t j = 0; j < 2; ++j)
			poly_secpar_store1(&hash_output[j], mapped_hashes[j]);

		block_secpar mask[2] = { block_secpar_set_zero(), block_secpar_set_zero() };
		memcpy(mask, to_hash + QUICKSILVER_ROWS / 8, VOLE_CHECK_HASH_BYTES);
		for (size_t j = 0; j < 2; ++j)
			hash_output[j] = block_secpar_xor(hash_output[j], mask[j]);

		if (verifier)
			for (size_t j = 0; j < 2; ++j)
				hash_output[j] = block_secpar_xor(hash_output[j],
					block_secpar_and(u_hash[j], block_secpar_set_all_8(delta_bytes[col])));

		if (col >= 0)
			hash_update(&hasher, &hash_output[0], VOLE_CHECK_HASH_BYTES);
	}

	if (!verifier)
		memcpy(proof, &u_hash[0], VOLE_CHECK_HASH_BYTES);

	hash_update_byte(&hasher, 1);
	hash_final(&hasher, check, VOLE_CHECK_CHECK_BYTES);
}

void vole_check_sender(
	const vole_block* restrict u, const vole_block* restrict v,
	const uint8_t* restrict challenge, uint8_t* restrict proof, uint8_t* restrict check)
{
	vole_check_both(false, u, v, NULL, challenge, proof, check);
}

void vole_check_receiver(
	const vole_block* restrict q, const uint8_t* restrict delta_bytes,
	const uint8_t* restrict challenge, const uint8_t* restrict proof, uint8_t* restrict check)
{
	vole_check_both(true, NULL, q, delta_bytes, challenge, (uint8_t*) proof, check);
}
