#include "loong_support.h"

#include "loong_status.h"
#include "rbc_vec.h"

#include <stdlib.h>

#define LOONG_SUPPORT_MAX_ATTEMPTS 4096U

static int support_rank(unsigned int *rank, const rbc_elt *v, unsigned int size)
{
	return rbc_vec_get_rank(rank, v, size);
}

static int random_elt_from_reader(rbc_elt *e, loong_xof_reader *reader)
{
	unsigned char bytes[16];
	unsigned int i;

	if (e == 0 || reader == 0) {
		return LOONG_ERR_NULL;
	}
	if (RBC_ELT_BYTES > sizeof(bytes)) {
		return LOONG_ERR_OVERFLOW;
	}
	if (loong_xof_reader_read(bytes, RBC_ELT_BYTES, reader) != LOONG_SUCCESS) {
		return LOONG_ERR_BAD_LENGTH;
	}

	rbc_elt_set_zero(e);
	for (i = 0; i < RBC_FIELD_M; i++) {
		unsigned char bit = (unsigned char)((bytes[i / 8U] >> (i % 8U)) & 1U);
		if (rbc_elt_set_coefficient(e, i, bit) != 0) {
			return LOONG_ERR_BAD_LENGTH;
		}
	}
	rbc_elt_normalize(e);
	return LOONG_SUCCESS;
}

static int append_independent(rbc_elt *pool, unsigned int *pool_size,
                              unsigned int target_size, const rbc_elt *candidate)
{
	unsigned int before_rank;
	unsigned int after_rank;

	if (pool == 0 || pool_size == 0 || candidate == 0 ||
	    *pool_size >= target_size) {
		return LOONG_ERR_NULL;
	}
	if (support_rank(&before_rank, pool, *pool_size) != 0) {
		return LOONG_ERR_BAD_LENGTH;
	}
	rbc_elt_set(&pool[*pool_size], candidate);
	if (support_rank(&after_rank, pool, *pool_size + 1U) != 0) {
		return LOONG_ERR_BAD_LENGTH;
	}
	if (after_rank == before_rank + 1U) {
		*pool_size += 1U;
		return LOONG_SUCCESS;
	}
	rbc_elt_set_zero(&pool[*pool_size]);
	return LOONG_ERR_CRYPTO_REJECT;
}

int loong_support_span_contains_one(const rbc_elt *basis, unsigned int size)
{
	rbc_elt *tmp;
	rbc_elt one;
	unsigned int rank_before;
	unsigned int rank_after;
	unsigned int i;
	int result;

	if (size != 0 && basis == 0) {
		return 0;
	}
	tmp = (rbc_elt *)calloc((size_t)size + 1U, sizeof(*tmp));
	if (tmp == 0) {
		return 0;
	}
	for (i = 0; i < size; i++) {
		rbc_elt_set(&tmp[i], &basis[i]);
	}
	rbc_elt_set_one(&one);
	rbc_elt_set(&tmp[size], &one);

	result = 0;
	if (support_rank(&rank_before, tmp, size) == 0 &&
	    support_rank(&rank_after, tmp, size + 1U) == 0) {
		result = (rank_before == rank_after);
	}
	free(tmp);
	return result;
}

int loong_support_sample_pair(rbc_elt *v1, unsigned int v1_rank,
                              rbc_elt *v2, unsigned int v2_rank,
                              unsigned int intersection_dim,
                              int require_one_in_v2,
                              loong_xof_reader *reader)
{
	rbc_elt *pool;
	unsigned int pool_size = 0;
	unsigned int union_rank;
	unsigned int attempts = 0;
	unsigned int i;

	if ((v1_rank != 0 && v1 == 0) || (v2_rank != 0 && v2 == 0) ||
	    reader == 0) {
		return LOONG_ERR_NULL;
	}
	if (intersection_dim > v1_rank || intersection_dim > v2_rank) {
		return LOONG_ERR_BAD_LENGTH;
	}
	if (v1_rank > RBC_FIELD_M || v2_rank > RBC_FIELD_M) {
		return LOONG_ERR_BAD_LENGTH;
	}
	union_rank = v1_rank + v2_rank - intersection_dim;
	if (union_rank > RBC_FIELD_M) {
		return LOONG_ERR_BAD_LENGTH;
	}

	pool = (rbc_elt *)calloc((size_t)union_rank, sizeof(*pool));
	if (pool == 0 && union_rank != 0) {
		return LOONG_ERR_ALLOC;
	}

	if (require_one_in_v2 && union_rank != 0) {
		rbc_elt one;
		rbc_elt_set_one(&one);
		if (append_independent(pool, &pool_size, union_rank, &one) !=
		    LOONG_SUCCESS) {
			free(pool);
			return LOONG_ERR_CRYPTO_REJECT;
		}
	}

	while (pool_size < union_rank && attempts < LOONG_SUPPORT_MAX_ATTEMPTS) {
		rbc_elt candidate;
		int status;

		attempts++;
		status = random_elt_from_reader(&candidate, reader);
		if (status != LOONG_SUCCESS) {
			free(pool);
			return status;
		}
		if (rbc_elt_is_zero(&candidate)) {
			continue;
		}
		(void)append_independent(pool, &pool_size, union_rank, &candidate);
	}
	if (pool_size != union_rank) {
		free(pool);
		return LOONG_ERR_CRYPTO_REJECT;
	}

	for (i = 0; i < v1_rank; i++) {
		rbc_elt_set(&v1[i], &pool[i]);
	}
	for (i = 0; i < intersection_dim; i++) {
		rbc_elt_set(&v2[i], &pool[i]);
	}
	for (i = intersection_dim; i < v2_rank; i++) {
		unsigned int pool_index = v1_rank + (i - intersection_dim);
		rbc_elt_set(&v2[i], &pool[pool_index]);
	}

	free(pool);
	return LOONG_SUCCESS;
}
