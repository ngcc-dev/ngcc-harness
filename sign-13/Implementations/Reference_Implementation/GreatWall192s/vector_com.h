#ifndef VECTOR_COM_H
#define VECTOR_COM_H

#include "config.h"
#include "aes.h"
#include "block.h"
#include "vole_params.h"

#define BATCH_VECTOR_COMMIT_LEAVES ((VOLES_MIN_K << VOLE_MIN_K) + (VOLES_MAX_K << VOLE_MAX_K))
#define BATCH_VECTOR_COMMIT_NODES (2*BATCH_VECTOR_COMMIT_LEAVES - 1)
#define BATCH_VECTOR_OPENING_SIZE ((2 * BITS_PER_WITNESS + BATCH_VECTOR_OPENING_SEEDS_TRESHOLD) * (SECURITY_PARAM / 8))

#define BATCH_VEC_POS_IN_TREE( vec_index , leaf_index ) \
		(BATCH_VECTOR_COMMIT_LEAVES - 1 + ((leaf_index < BATCH_VEC_SHORT_LEN) ? BITS_PER_WITNESS*leaf_index + vec_index : VOLES_MIN_K*BATCH_VEC_SHORT_LEN + VOLES_MAX_K*leaf_index + vec_index ))


#define BATCH_VEC_LONG_LEN (1<<VOLE_MAX_K)
#define BATCH_VEC_SHORT_LEN (1<<VOLE_MIN_K)
#define BATCH_VEC_LEN( vec_index ) ( vec_index < VOLES_MAX_K ? BATCH_VEC_LONG_LEN : BATCH_VEC_SHORT_LEN )

#define BATCH_VEC_POS_IN_OUTPUT_LONG( vec_index , leaf_index ) ( vec_index*BATCH_VEC_LONG_LEN + leaf_index )
#define BATCH_VEC_POS_IN_OUTPUT_SHORT( vec_index , leaf_index ) ( VOLES_MAX_K*BATCH_VEC_SHORT_LEN + vec_index*BATCH_VEC_SHORT_LEN + leaf_index  )

#define BATCH_VEC_HASH_POS_IN_OUTPUT( vec_index , leaf_index ) \
		( vec_index < VOLES_MAX_K ?  BATCH_VEC_POS_IN_OUTPUT_LONG( vec_index , leaf_index ) :  BATCH_VEC_POS_IN_OUTPUT_SHORT( vec_index , leaf_index ) )


#define BATCH_VEC_LEAF_POS_IN_OUTPUT( vec_index , leaf_index )  BATCH_VEC_HASH_POS_IN_OUTPUT( vec_index , vole_permute_key_index_inv(leaf_index) )




#define VECTOR_COMMIT_LEAVES ((VOLES_MIN_K << VOLE_MIN_K) + (VOLES_MAX_K << VOLE_MAX_K))
#define PPRF_VECTOR_COM_NODES (2 * VECTOR_COMMIT_LEAVES - BITS_PER_WITNESS)
#define PPRF_VECTOR_OPEN_BITS (NONZERO_BITS_IN_CHALLENGE_3 * SECURITY_PARAM + 2 * SECURITY_PARAM * BITS_PER_WITNESS)
#define PPRF_VECTOR_COM_OPEN_SIZE (PPRF_VECTOR_OPEN_BITS / 8)

void vector_commit(
	const block_secpar seed, block128 iv,
	block_secpar* restrict forest, block_secpar* restrict leaves,
	block_2secpar* restrict hashed_leaves);

void vector_commit_from_roots(
    block_secpar* roots, block128 iv, block_secpar* restrict forest,
    block_secpar* restrict leaves, block_2secpar* restrict hashed_leaves,
    const prg_tree_fixed_key* fixed_key_tree, const prg_leaf_fixed_key* fixed_key_leaf);

void vector_open(
	const block_secpar* restrict forest, const block_2secpar* restrict hashed_leaves,
	const uint8_t* restrict delta, unsigned char* restrict opening);

bool vector_verify(
	block128 iv, const unsigned char* restrict opening, const uint8_t* restrict delta,
	block_secpar* restrict leaves, block_2secpar* restrict hashed_leaves);


void batch_vector_commit(
	const block_secpar seed, block128 iv,
	block_secpar* restrict tree, block_secpar* restrict leaves,
	block_2secpar* restrict hashed_leaves);

bool batch_vector_open(
	const block_secpar* restrict tree, const block_2secpar* restrict hashed_leaves,
	const uint8_t* restrict delta, unsigned char* restrict opening);

bool batch_vector_verify(
	block128 iv, const unsigned char* restrict opening, const uint8_t* restrict delta,
	block_secpar* restrict leaves, block_2secpar* restrict hashed_leaves);

bool force_vector_open(const block_secpar* restrict forest, const block_2secpar* restrict hashed_leaves,
	uint8_t* restrict delta_out, unsigned char* restrict opening, const unsigned char *message, size_t m_len, uint32_t *counter);

#endif
