#include "vole_commit.h"

#include <stdalign.h>
#include <stdlib.h>
#include "small_vole.h"
#include "vector_com.h"
#include "hash.h"
#include <stdlib.h>

static void hash_hashed_leaves_all_same_size(
	hash_state* hasher, block_2secpar* hashed_leaves, size_t num_trees, size_t num_leaves)
{
	for (size_t i = 0; (i + 4) <= num_trees; i += 4)
	{
		const void* to_hash[4];
		for (size_t j = 0; j < 4; ++j, hashed_leaves += num_leaves)
			to_hash[j] = hashed_leaves;

		block_2secpar leaves_hashes[4];
		hash_state_x4 leaves_hasher;
		hash_init_x4(&leaves_hasher);
		hash_update_x4(&leaves_hasher, to_hash, num_leaves * sizeof(block_2secpar));
		hash_update_x4_1_byte(&leaves_hasher, 1);
		hash_final_x4_4(&leaves_hasher, &leaves_hashes[0], &leaves_hashes[1],
		                &leaves_hashes[2], &leaves_hashes[3], sizeof(leaves_hashes[0]));

		hash_update(hasher, &leaves_hashes[0], sizeof(leaves_hashes));
	}

	for (size_t i = num_trees - (num_trees % 4); i < num_trees; ++i, hashed_leaves += num_leaves)
	{
		block_2secpar leaves_hash;
		hash_state leaves_hasher;
		hash_init(&leaves_hasher);
		hash_update(&leaves_hasher, hashed_leaves, num_leaves * sizeof(block_2secpar));
		hash_update_byte(&leaves_hasher, 1);
		hash_final(&leaves_hasher, &leaves_hash, sizeof(leaves_hash));

		hash_update(hasher, &leaves_hash, sizeof(leaves_hash));
	}
}

static void hash_hashed_leaves(block_2secpar* hashed_leaves, uint8_t* restrict hash_of_hashes)
{
	hash_state hasher;
	hash_init(&hasher);
	hash_hashed_leaves_all_same_size(
		&hasher, hashed_leaves, VOLES_MAX_K, (size_t) 1 << VOLE_MAX_K);
	hash_hashed_leaves_all_same_size(
		&hasher, hashed_leaves + ((size_t) VOLES_MAX_K << VOLE_MAX_K),
		VOLES_MIN_K, (size_t) 1 << VOLE_MIN_K);
	hash_update_byte(&hasher, 1);
	hash_final(&hasher, hash_of_hashes, VOLE_COMMIT_CHECK_SIZE);
}

void vole_commit(
	block_secpar seed, block128 iv, block_secpar* restrict forest, block_2secpar* hashed_leaves,
	vole_block* restrict u, vole_block* restrict v,
	uint8_t* restrict commitment, uint8_t* restrict check)
{
	block_secpar* leaves =
		aligned_alloc(alignof(block_secpar), VECTOR_COMMIT_LEAVES * sizeof(block_secpar));

#if USE_IMPROVED_VECTOR_COMMITMENTS == 0
	vector_commit(seed, iv, forest, leaves, hashed_leaves);
#else
	batch_vector_commit(seed, iv, forest, leaves, hashed_leaves);
#endif

	hash_hashed_leaves(hashed_leaves, check);

	block_secpar fixed_key_iv = block_secpar_set_zero();
	memcpy(&fixed_key_iv, &iv, sizeof(iv));
	prg_vole_fixed_key fixed_key;
	vole_fixed_key_init(&fixed_key, fixed_key_iv);

	vole_block correction[VOLE_COL_BLOCKS];
	block_secpar* leaves_iter = leaves;
	for (size_t i = 0; i < BITS_PER_WITNESS; ++i)
	{
		unsigned int k = i < VOLES_MAX_K ? VOLE_MAX_K : VOLE_MIN_K;
		if (!i)
			vole_sender(k, leaves_iter, iv, &fixed_key, NULL, v, u);
		else
		{
			vole_sender(k, leaves_iter, iv, &fixed_key, u, v, correction);
			memcpy(commitment, correction, VOLE_ROWS / 8);
			commitment += VOLE_ROWS / 8;
		}

		leaves_iter += (size_t) 1 << k;
		v += VOLE_COL_BLOCKS * k;
	}

	memset(v, 0, VOLE_COL_BLOCKS * ZERO_BITS_IN_CHALLENGE_3 * sizeof(*v));

	free(leaves);
}

bool vole_reconstruct(
	block128 iv, vole_block* restrict q, const uint8_t* delta_bytes,
	const uint8_t* restrict commitment, const uint8_t* restrict opening, uint8_t* restrict check)
{
	bool success = 1;
	block_secpar* leaves =
		aligned_alloc(alignof(block_secpar), VECTOR_COMMIT_LEAVES * sizeof(block_secpar));
	block_2secpar* hashed_leaves =
		aligned_alloc(alignof(block_2secpar), VECTOR_COMMIT_LEAVES * sizeof(block_2secpar));

#if USE_IMPROVED_VECTOR_COMMITMENTS == 0
	bool vector_verify_status = vector_verify(iv, opening, delta_bytes, leaves, hashed_leaves);
#else
	bool vector_verify_status = batch_vector_verify(iv, opening, delta_bytes, leaves, hashed_leaves);
#endif
	if (vector_verify_status == 0) {
		success = 0;
		goto end;
	}

	hash_hashed_leaves(hashed_leaves, check);

	block_secpar fixed_key_iv = block_secpar_set_zero();
	memcpy(&fixed_key_iv, &iv, sizeof(iv));
	prg_vole_fixed_key fixed_key;
	vole_fixed_key_init(&fixed_key, fixed_key_iv);

	vole_block correction[VOLE_COL_BLOCKS];
	if (VOLE_COL_BLOCKS * sizeof(vole_block) != VOLE_ROWS / 8)
		correction[VOLE_COL_BLOCKS - 1] = vole_block_set_zero();

	block_secpar* leaves_iter = leaves;
	for (size_t i = 0; i < BITS_PER_WITNESS; ++i)
	{
		unsigned int k = i < VOLES_MAX_K ? VOLE_MAX_K : VOLE_MIN_K;
		if (!i)
			vole_receiver(k, leaves_iter, iv, &fixed_key, NULL, q, delta_bytes);
		else
		{
			memcpy(correction, commitment, VOLE_ROWS / 8);
			commitment += VOLE_ROWS / 8;
			vole_receiver(k, leaves_iter, iv, &fixed_key, correction, q, delta_bytes);
		}

		leaves_iter += (size_t) 1 << k;
		q += VOLE_COL_BLOCKS * k;
		delta_bytes += k;
	}

	memset(q, 0, VOLE_COL_BLOCKS * ZERO_BITS_IN_CHALLENGE_3 * sizeof(*q));

end:
	free(hashed_leaves);
	free(leaves);
	return success;
}
