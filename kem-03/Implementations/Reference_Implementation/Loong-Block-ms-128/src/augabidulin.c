#include "augabidulin.h"

#include "gabidulin.h"
#include "loong_status.h"
#include "qpoly.h"
#include "rbc_vec.h"

#include <stdlib.h>

static int recover_support_basis(rbc_elt *basis, unsigned int *rank,
                                 unsigned int max_rank, const rbc_elt *v,
                                 unsigned int size)
{
	unsigned int basis_size = 0;
	unsigned int i;

	if (rank == 0 || (max_rank != 0 && basis == 0) ||
	    (size != 0 && v == 0)) {
		return LOONG_ERR_NULL;
	}
	for (i = 0; i < size; i++) {
		rbc_elt tmp[128];
		unsigned int before;
		unsigned int after;
		unsigned int j;

		if (rbc_elt_is_zero(&v[i])) {
			continue;
		}
		if (basis_size >= max_rank || basis_size + 1U > 128U) {
			return LOONG_ERR_BAD_LENGTH;
		}
		for (j = 0; j < basis_size; j++) {
			rbc_elt_set(&tmp[j], &basis[j]);
		}
		rbc_vec_get_rank(&before, tmp, basis_size);
		rbc_elt_set(&tmp[basis_size], &v[i]);
		rbc_vec_get_rank(&after, tmp, basis_size + 1U);
		if (after == before + 1U) {
			rbc_elt_set(&basis[basis_size], &v[i]);
			basis_size++;
		}
	}
	*rank = basis_size;
	return LOONG_SUCCESS;
}

int augabidulin_code_init(augabidulin_code *code, const rbc_elt *g,
                          unsigned int k, unsigned int n,
                          unsigned int n_prime)
{
	if (code == 0 || g == 0) {
		return LOONG_ERR_NULL;
	}
	if (k == 0 || n == 0 || n_prime == 0 || k > n_prime ||
	    n_prime > n || n_prime > RBC_FIELD_M) {
		return LOONG_ERR_BAD_LENGTH;
	}
	code->g = g;
	code->k = k;
	code->n = n;
	code->n_prime = n_prime;
	return LOONG_SUCCESS;
}

int augabidulin_code_encode(rbc_elt *c, unsigned int c_size,
                            const augabidulin_code *code, const rbc_elt *m,
                            unsigned int m_size)
{
	gabidulin_code base;
	int status;

	if (c == 0 || code == 0 || code->g == 0 || m == 0) {
		return LOONG_ERR_NULL;
	}
	if (c_size < code->n || m_size < code->k) {
		return LOONG_ERR_BAD_LENGTH;
	}
	rbc_vec_set_zero(c, c_size);
	status = gabidulin_code_init(&base, code->g, code->k, code->n_prime);
	if (status != LOONG_SUCCESS) {
		return status;
	}
	return gabidulin_code_encode(c, code->n_prime, &base, m, m_size);
}

int augabidulin_code_decode_no_error(rbc_elt *m, unsigned int m_size,
                                     const augabidulin_code *code,
                                     const rbc_elt *y, unsigned int y_size)
{
	gabidulin_code base;
	int status;

	if (m == 0 || code == 0 || code->g == 0 || y == 0) {
		return LOONG_ERR_NULL;
	}
	if (m_size < code->k || y_size < code->n) {
		return LOONG_ERR_BAD_LENGTH;
	}
	status = gabidulin_code_init(&base, code->g, code->k, code->n_prime);
	if (status != LOONG_SUCCESS) {
		return status;
	}
	return gabidulin_code_decode_no_error(m, m_size, &base, y, code->n_prime);
}

int augabidulin_code_decode(rbc_elt *m, unsigned int m_size,
                            const augabidulin_code *code, const rbc_elt *y,
                            unsigned int y_size, unsigned int epsilon)
{
	rbc_elt *basis = 0;
	rbc_elt *z = 0;
	qpoly v2;
	gabidulin_code base;
	unsigned int tail_size;
	unsigned int tail_rank = 0;
	unsigned int i;
	int status;

	if (m == 0 || code == 0 || code->g == 0 || y == 0) {
		return LOONG_ERR_NULL;
	}
	if (m_size < code->k || y_size < code->n) {
		return LOONG_ERR_BAD_LENGTH;
	}
	if (code->n < code->n_prime || code->k + epsilon > code->n_prime) {
		return LOONG_ERR_BAD_LENGTH;
	}

	tail_size = code->n - code->n_prime;
	basis = (rbc_elt *)calloc((size_t)(epsilon == 0 ? 1U : epsilon),
	                         sizeof(*basis));
	z = (rbc_elt *)calloc((size_t)code->n_prime, sizeof(*z));
	if (basis == 0 || z == 0) {
		free(basis);
		free(z);
		return LOONG_ERR_ALLOC;
	}

	status = recover_support_basis(basis, &tail_rank, epsilon,
	                               y + code->n_prime, tail_size);
	if (status != LOONG_SUCCESS) {
		goto cleanup;
	}
	if (tail_rank > epsilon || code->k + tail_rank > code->n_prime) {
		status = LOONG_ERR_CRYPTO_REJECT;
		goto cleanup;
	}

	status = qpoly_init(&v2, tail_rank == 0 ? 0U : tail_rank);
	if (status != LOONG_SUCCESS) {
		goto cleanup;
	}
	if (tail_rank == 0) {
		qpoly_set_one(&v2);
	} else {
		status = qpoly_set_interpolate_zero(&v2, basis, tail_rank);
		if (status != LOONG_SUCCESS) {
			qpoly_clear(&v2);
			goto cleanup;
		}
	}
	for (i = 0; i < code->n_prime; i++) {
		qpoly_evaluate(&z[i], &v2, &y[i]);
	}

	status = gabidulin_code_init(&base, code->g, code->k + tail_rank,
	                             code->n_prime);
	if (status == LOONG_SUCCESS) {
		status = gabidulin_code_decode_with_annihilator(m, m_size, &base, z,
		                                                code->n_prime, &v2);
	}
	qpoly_clear(&v2);

cleanup:
	free(basis);
	free(z);
	return status;
}
