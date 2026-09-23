#include "gabidulin.h"

#include "loong_status.h"
#include "qpoly.h"
#include "rbc_vec.h"

#include <stdlib.h>

int gabidulin_code_init(gabidulin_code *code, const rbc_elt *g,
                        unsigned int k, unsigned int n)
{
	if (code == 0 || g == 0) {
		return LOONG_ERR_NULL;
	}
	if (k == 0 || n == 0 || k > n || n > RBC_FIELD_M) {
		return LOONG_ERR_BAD_LENGTH;
	}
	code->g = g;
	code->k = k;
	code->n = n;
	return LOONG_SUCCESS;
}

int gabidulin_code_encode(rbc_elt *c, unsigned int c_size,
                          const gabidulin_code *code, const rbc_elt *m,
                          unsigned int m_size)
{
	unsigned int j;
	unsigned int i;

	if (c == 0 || code == 0 || code->g == 0 || m == 0) {
		return LOONG_ERR_NULL;
	}
	if (c_size < code->n || m_size < code->k) {
		return LOONG_ERR_BAD_LENGTH;
	}

	rbc_vec_set_zero(c, c_size);
	for (j = 0; j < code->n; j++) {
		rbc_elt g_qi;
		rbc_elt term;

		rbc_elt_set(&g_qi, &code->g[j]);
		for (i = 0; i < code->k; i++) {
			if (i != 0) {
				rbc_elt_sqr(&g_qi, &g_qi);
			}
			rbc_elt_mul(&term, &m[i], &g_qi);
			rbc_elt_add(&c[j], &c[j], &term);
		}
	}
	return LOONG_SUCCESS;
}

int gabidulin_code_decode_no_error(rbc_elt *m, unsigned int m_size,
                                   const gabidulin_code *code,
                                   const rbc_elt *y, unsigned int y_size)
{
	qpoly annihilator;
	qpoly interpolant;
	unsigned int i;
	int status;

	if (m == 0 || code == 0 || code->g == 0 || y == 0) {
		return LOONG_ERR_NULL;
	}
	if (m_size < code->k || y_size < code->n || code->k == 0) {
		return LOONG_ERR_BAD_LENGTH;
	}

	status = qpoly_init(&annihilator, code->k);
	if (status != LOONG_SUCCESS) {
		return status;
	}
	status = qpoly_init(&interpolant, code->k - 1U);
	if (status != LOONG_SUCCESS) {
		qpoly_clear(&annihilator);
		return status;
	}

	status = qpoly_set_interpolate_vect_and_zero(&annihilator, &interpolant,
	                                            code->g, y, code->k);
	if (status == LOONG_SUCCESS) {
		for (i = 0; i < code->k; i++) {
			qpoly_get_coefficient(&m[i], &interpolant, i);
		}
	}

	qpoly_clear(&annihilator);
	qpoly_clear(&interpolant);
	return status;
}

static int gabidulin_reconstruct(qpoly *out_n1, qpoly *out_v1,
                                 const gabidulin_code *code,
                                 const rbc_elt *y)
{
	unsigned int i;
	qpoly a;
	qpoly interp;
	qpoly n0;
	qpoly n1;
	qpoly v0;
	qpoly v1;
	qpoly qtmp1;
	qpoly qtmp2;
	qpoly qtmp3;
	qpoly qtmp4;
	rbc_elt *u0 = 0;
	rbc_elt *u1 = 0;
	int status = LOONG_SUCCESS;

	status = qpoly_init(&a, RBC_FIELD_M - 1U);
	if (status != LOONG_SUCCESS) {
		return status;
	}
	status = qpoly_init(&interp, RBC_FIELD_M - 1U);
	if (status != LOONG_SUCCESS) {
		qpoly_clear(&a);
		return status;
	}
	status = qpoly_init(&n0, RBC_FIELD_M - 1U);
	if (status != LOONG_SUCCESS) {
		qpoly_clear(&a);
		qpoly_clear(&interp);
		return status;
	}
	status = qpoly_init(&n1, RBC_FIELD_M - 1U);
	if (status != LOONG_SUCCESS) {
		qpoly_clear(&a);
		qpoly_clear(&interp);
		qpoly_clear(&n0);
		return status;
	}
	status = qpoly_init(&v0, RBC_FIELD_M - 1U);
	if (status != LOONG_SUCCESS) {
		qpoly_clear(&a);
		qpoly_clear(&interp);
		qpoly_clear(&n0);
		qpoly_clear(&n1);
		return status;
	}
	status = qpoly_init(&v1, RBC_FIELD_M - 1U);
	if (status != LOONG_SUCCESS) {
		qpoly_clear(&a);
		qpoly_clear(&interp);
		qpoly_clear(&n0);
		qpoly_clear(&n1);
		qpoly_clear(&v0);
		return status;
	}
	status = qpoly_init(&qtmp1, RBC_FIELD_M - 1U);
	if (status != LOONG_SUCCESS) {
		goto cleanup_polys;
	}
	status = qpoly_init(&qtmp2, RBC_FIELD_M - 1U);
	if (status != LOONG_SUCCESS) {
		qpoly_clear(&qtmp1);
		goto cleanup_polys;
	}
	status = qpoly_init(&qtmp3, RBC_FIELD_M - 1U);
	if (status != LOONG_SUCCESS) {
		qpoly_clear(&qtmp1);
		qpoly_clear(&qtmp2);
		goto cleanup_polys;
	}
	status = qpoly_init(&qtmp4, RBC_FIELD_M - 1U);
	if (status != LOONG_SUCCESS) {
		qpoly_clear(&qtmp1);
		qpoly_clear(&qtmp2);
		qpoly_clear(&qtmp3);
		goto cleanup_polys;
	}

	u0 = (rbc_elt *)calloc((size_t)code->n, sizeof(*u0));
	u1 = (rbc_elt *)calloc((size_t)code->n, sizeof(*u1));
	if (u0 == 0 || u1 == 0) {
		status = LOONG_ERR_ALLOC;
		goto cleanup_all;
	}

	status = qpoly_set_interpolate_vect_and_zero(&a, &interp, code->g, y,
	                                            code->k);
	if (status != LOONG_SUCCESS) {
		goto cleanup_all;
	}
	qpoly_set_one(&n0);
	qpoly_set_zero(&n1);
	qpoly_set_zero(&v0);
	qpoly_set_one(&v1);

	for (i = 0; i < code->n; i++) {
		rbc_elt left;
		rbc_elt right;

		qpoly_evaluate(&left, &a, &code->g[i]);
		qpoly_evaluate(&right, &v0, &y[i]);
		rbc_elt_add(&u0[i], &left, &right);

		qpoly_evaluate(&left, &interp, &code->g[i]);
		qpoly_evaluate(&right, &v1, &y[i]);
		rbc_elt_add(&u1[i], &left, &right);
	}

	for (i = code->k; i < code->n; i++) {
		unsigned int j = i;
		unsigned int k;
		int update_type = 0;
		rbc_elt e1;
		rbc_elt e2;
		rbc_elt inv;

		while (j < code->n && !rbc_elt_is_zero(&u0[j]) &&
		       rbc_elt_is_zero(&u1[j])) {
			j++;
		}
		if (j == code->n) {
			break;
		}
		if (i != j) {
			rbc_elt tmp;
			rbc_elt_set(&tmp, &u0[i]);
			rbc_elt_set(&u0[i], &u0[j]);
			rbc_elt_set(&u0[j], &tmp);
			rbc_elt_set(&tmp, &u1[i]);
			rbc_elt_set(&u1[i], &u1[j]);
			rbc_elt_set(&u1[j], &tmp);
		}

		if (!rbc_elt_is_zero(&u1[i])) {
			update_type = 1;
			if (rbc_elt_inv(&inv, &u1[i]) != 0) {
				status = LOONG_ERR_CRYPTO_REJECT;
				goto cleanup_all;
			}
			rbc_elt_sqr(&e1, &u1[i]);
			rbc_elt_mul(&e1, &e1, &inv);
			rbc_elt_mul(&e2, &u0[i], &inv);

			qpoly_scalar_mul(&qtmp1, &n1, &e1);
			qpoly_qexp(&qtmp2, &n1);
			qpoly_scalar_mul(&qtmp3, &v1, &e1);
			qpoly_qexp(&qtmp4, &v1);

			qpoly_scalar_mul(&n1, &n1, &e2);
			qpoly_add(&n1, &n0, &n1);
			qpoly_scalar_mul(&v1, &v1, &e2);
			qpoly_add(&v1, &v0, &v1);
			qpoly_add(&n0, &qtmp1, &qtmp2);
			qpoly_add(&v0, &qtmp3, &qtmp4);
		}

		if (rbc_elt_is_zero(&u0[i]) && rbc_elt_is_zero(&u1[i])) {
			update_type = 2;
			qpoly_qexp(&qtmp1, &n1);
			qpoly_qexp(&qtmp2, &v1);
			qpoly_set(&n1, &n0);
			qpoly_set(&v1, &v0);
			qpoly_set(&n0, &qtmp1);
			qpoly_set(&v0, &qtmp2);
		}

		for (k = i + 1U; k < code->n; k++) {
			if (update_type == 1) {
				rbc_elt new_u0;
				rbc_elt new_u1;
				rbc_elt tmp;

				rbc_elt_mul(&tmp, &e1, &u1[k]);
				rbc_elt_sqr(&new_u0, &u1[k]);
				rbc_elt_add(&new_u0, &new_u0, &tmp);

				rbc_elt_mul(&tmp, &e2, &u1[k]);
				rbc_elt_add(&new_u1, &u0[k], &tmp);
				rbc_elt_set(&u0[k], &new_u0);
				rbc_elt_set(&u1[k], &new_u1);
			} else if (update_type == 2) {
				rbc_elt_sqr(&u1[k], &u1[k]);
			}
		}
	}

	qpoly_set(out_n1, &n1);
	qpoly_set(out_v1, &v1);

cleanup_all:
	free(u0);
	free(u1);
	qpoly_clear(&qtmp1);
	qpoly_clear(&qtmp2);
	qpoly_clear(&qtmp3);
	qpoly_clear(&qtmp4);
cleanup_polys:
	qpoly_clear(&a);
	qpoly_clear(&interp);
	qpoly_clear(&n0);
	qpoly_clear(&n1);
	qpoly_clear(&v0);
	qpoly_clear(&v1);
	return status;
}

int gabidulin_code_decode_with_annihilator(rbc_elt *m, unsigned int m_size,
                                           const gabidulin_code *code,
                                           const rbc_elt *y,
                                           unsigned int y_size,
                                           const qpoly *annihilator)
{
	qpoly a;
	qpoly interp;
	qpoly n1;
	qpoly v1;
	qpoly product;
	qpoly quotient;
	qpoly remainder;
	qpoly numerator;
	unsigned int i;
	int status;

	if (m == 0 || code == 0 || code->g == 0 || y == 0 ||
	    annihilator == 0 || annihilator->coeffs == 0) {
		return LOONG_ERR_NULL;
	}
	if (m_size == 0 || m_size > code->k || y_size < code->n ||
	    code->k == 0 || code->n > RBC_FIELD_M) {
		return LOONG_ERR_BAD_LENGTH;
	}

	status = qpoly_init(&a, RBC_FIELD_M - 1U);
	if (status != LOONG_SUCCESS) {
		return status;
	}
	status = qpoly_init(&interp, RBC_FIELD_M - 1U);
	if (status != LOONG_SUCCESS) {
		qpoly_clear(&a);
		return status;
	}
	status = qpoly_init(&n1, RBC_FIELD_M - 1U);
	if (status != LOONG_SUCCESS) {
		qpoly_clear(&a);
		qpoly_clear(&interp);
		return status;
	}
	status = qpoly_init(&v1, RBC_FIELD_M - 1U);
	if (status != LOONG_SUCCESS) {
		qpoly_clear(&a);
		qpoly_clear(&interp);
		qpoly_clear(&n1);
		return status;
	}
	status = qpoly_init(&product, RBC_FIELD_M - 1U);
	if (status != LOONG_SUCCESS) {
		qpoly_clear(&a);
		qpoly_clear(&interp);
		qpoly_clear(&n1);
		qpoly_clear(&v1);
		return status;
	}
	status = qpoly_init(&quotient, RBC_FIELD_M - 1U);
	if (status != LOONG_SUCCESS) {
		qpoly_clear(&a);
		qpoly_clear(&interp);
		qpoly_clear(&n1);
		qpoly_clear(&v1);
		qpoly_clear(&product);
		return status;
	}
	status = qpoly_init(&remainder, RBC_FIELD_M - 1U);
	if (status != LOONG_SUCCESS) {
		qpoly_clear(&a);
		qpoly_clear(&interp);
		qpoly_clear(&n1);
		qpoly_clear(&v1);
		qpoly_clear(&product);
		qpoly_clear(&quotient);
		return status;
	}
	status = qpoly_init(&numerator, RBC_FIELD_M - 1U);
	if (status != LOONG_SUCCESS) {
		qpoly_clear(&a);
		qpoly_clear(&interp);
		qpoly_clear(&n1);
		qpoly_clear(&v1);
		qpoly_clear(&product);
		qpoly_clear(&quotient);
		qpoly_clear(&remainder);
		return status;
	}

	status = qpoly_set_interpolate_vect_and_zero(&a, &interp, code->g, y,
	                                            code->k);
	if (status != LOONG_SUCCESS) {
		goto cleanup;
	}
	status = gabidulin_reconstruct(&n1, &v1, code, y);
	if (status != LOONG_SUCCESS) {
		goto cleanup;
	}
	status = qpoly_mul(&product, &n1, &a);
	if (status != LOONG_SUCCESS) {
		goto cleanup;
	}
	status = qpoly_left_div(&quotient, &remainder, &product, &v1);
	if (status != LOONG_SUCCESS) {
		goto cleanup;
	}
	if (!qpoly_is_zero(&remainder)) {
		status = LOONG_ERR_CRYPTO_REJECT;
		goto cleanup;
	}
	status = qpoly_add(&numerator, &quotient, &interp);
	if (status != LOONG_SUCCESS) {
		goto cleanup;
	}
	status = qpoly_left_div(&quotient, &remainder, &numerator, annihilator);
	if (status != LOONG_SUCCESS) {
		goto cleanup;
	}
	if (!qpoly_is_zero(&remainder)) {
		status = LOONG_ERR_CRYPTO_REJECT;
		goto cleanup;
	}
	for (i = 0; i < m_size; i++) {
		qpoly_get_coefficient(&m[i], &quotient, i);
	}

cleanup:
	qpoly_clear(&a);
	qpoly_clear(&interp);
	qpoly_clear(&n1);
	qpoly_clear(&v1);
	qpoly_clear(&product);
	qpoly_clear(&quotient);
	qpoly_clear(&remainder);
	qpoly_clear(&numerator);
	return status;
}

int gabidulin_code_decode(rbc_elt *m, unsigned int m_size,
                          const gabidulin_code *code, const rbc_elt *y,
                          unsigned int y_size)
{
	qpoly one;
	int status;

	status = qpoly_init(&one, 0);
	if (status != LOONG_SUCCESS) {
		return status;
	}
	qpoly_set_one(&one);
	status = gabidulin_code_decode_with_annihilator(m, m_size, code, y,
	                                               y_size, &one);
	qpoly_clear(&one);
	return status;
}
