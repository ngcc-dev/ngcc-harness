#ifndef VOLE_COMMIT_H
#define VOLE_COMMIT_H

#include "block.h"

#define VOLE_COMMIT_SIZE ((VOLE_ROWS / 8) * (BITS_PER_WITNESS - 1))
#define VOLE_COMMIT_CHECK_SIZE (2 * SECURITY_PARAM / 8)

#if USE_IMPROVED_VECTOR_COMMITMENTS == 0
	#define FOREST_SIZE PPRF_VECTOR_COM_NODES
	#define VECTOR_COM_OPEN_SIZE PPRF_VECTOR_COM_OPEN_SIZE
#else
	#define FOREST_SIZE BATCH_VECTOR_COMMIT_NODES
	#define VECTOR_COM_OPEN_SIZE BATCH_VECTOR_OPENING_SIZE
#endif

void vole_commit(
	block_secpar seed, block128 iv, block_secpar* restrict forest, block_2secpar* hashed_leaves,
	vole_block* restrict u, vole_block* restrict v,
	uint8_t* restrict commitment, uint8_t* restrict check);

bool vole_reconstruct(
	block128 iv, vole_block* restrict q, const uint8_t* delta_bytes,
	const uint8_t* restrict commitment, const uint8_t* restrict opening, uint8_t* restrict check);

#endif
