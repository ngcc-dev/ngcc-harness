#include "qpoly.h"

#include "loong_status.h"

#include <stdlib.h>

static unsigned int qpoly_max_u(unsigned int a, unsigned int b)
{
	return a > b ? a : b;
}

int qpoly_init(qpoly *p, unsigned int max_degree)
{
	if (p == 0) {
		return LOONG_ERR_NULL;
	}
	if (max_degree >= RBC_FIELD_M) {
		return LOONG_ERR_BAD_LENGTH;
	}
	p->coeffs = (rbc_elt *)calloc((size_t)max_degree + 1U, sizeof(*p->coeffs));
	if (p->coeffs == 0) {
		p->max_degree = 0;
		p->degree = -1;
		return LOONG_ERR_ALLOC;
	}
	p->max_degree = max_degree;
	p->degree = -1;
	return LOONG_SUCCESS;
}

void qpoly_clear(qpoly *p)
{
	if (p == 0) {
		return;
	}
	free(p->coeffs);
	p->coeffs = 0;
	p->max_degree = 0;
	p->degree = -1;
}

void qpoly_update_degree(qpoly *p, unsigned int position)
{
	int i;

	if (p == 0 || p->coeffs == 0) {
		return;
	}
	if (position > p->max_degree) {
		position = p->max_degree;
	}
	for (i = (int)position; i >= 0; i--) {
		if (!rbc_elt_is_zero(&p->coeffs[i])) {
			p->degree = i;
			return;
		}
	}
	p->degree = -1;
}

int qpoly_set_zero(qpoly *p)
{
	unsigned int i;

	if (p == 0 || p->coeffs == 0) {
		return LOONG_ERR_NULL;
	}
	for (i = 0; i <= p->max_degree; i++) {
		rbc_elt_set_zero(&p->coeffs[i]);
	}
	p->degree = -1;
	return LOONG_SUCCESS;
}

int qpoly_set_one(qpoly *p)
{
	int status;

	status = qpoly_set_zero(p);
	if (status != LOONG_SUCCESS) {
		return status;
	}
	rbc_elt_set_one(&p->coeffs[0]);
	p->degree = 0;
	return LOONG_SUCCESS;
}

int qpoly_is_zero(const qpoly *p)
{
	return p == 0 || p->coeffs == 0 || p->degree < 0;
}

int qpoly_set(qpoly *out, const qpoly *in)
{
	unsigned int i;

	if (out == 0 || in == 0 || out->coeffs == 0 || in->coeffs == 0) {
		return LOONG_ERR_NULL;
	}
	if (in->degree > (int)out->max_degree) {
		return LOONG_ERR_BAD_LENGTH;
	}
	for (i = 0; i <= out->max_degree; i++) {
		if (i <= in->max_degree) {
			rbc_elt_set(&out->coeffs[i], &in->coeffs[i]);
		} else {
			rbc_elt_set_zero(&out->coeffs[i]);
		}
	}
	out->degree = in->degree;
	qpoly_update_degree(out, out->max_degree);
	return LOONG_SUCCESS;
}

int qpoly_set_coefficient(qpoly *p, unsigned int position, const rbc_elt *e)
{
	if (p == 0 || e == 0 || p->coeffs == 0) {
		return LOONG_ERR_NULL;
	}
	if (position > p->max_degree) {
		return LOONG_ERR_BAD_LENGTH;
	}
	rbc_elt_set(&p->coeffs[position], e);
	if (rbc_elt_is_zero(e)) {
		if (p->degree == (int)position) {
			qpoly_update_degree(p, position);
		}
	} else if ((int)position > p->degree) {
		p->degree = (int)position;
	}
	return LOONG_SUCCESS;
}

int qpoly_get_coefficient(rbc_elt *e, const qpoly *p, unsigned int position)
{
	if (e == 0) {
		return LOONG_ERR_NULL;
	}
	if (p == 0 || p->coeffs == 0 || position > p->max_degree) {
		rbc_elt_set_zero(e);
		return LOONG_SUCCESS;
	}
	rbc_elt_set(e, &p->coeffs[position]);
	return LOONG_SUCCESS;
}

int qpoly_evaluate(rbc_elt *out, const qpoly *p, const rbc_elt *x)
{
	int i;
	rbc_elt x_qi;
	rbc_elt term;

	if (out == 0 || p == 0 || x == 0 || p->coeffs == 0) {
		return LOONG_ERR_NULL;
	}
	rbc_elt_set_zero(out);
	if (qpoly_is_zero(p)) {
		return LOONG_SUCCESS;
	}

	rbc_elt_set(&x_qi, x);
	for (i = 0; i <= p->degree; i++) {
		if (i != 0) {
			rbc_elt_sqr(&x_qi, &x_qi);
		}
		rbc_elt_mul(&term, &p->coeffs[i], &x_qi);
		rbc_elt_add(out, out, &term);
	}
	return LOONG_SUCCESS;
}

int qpoly_scalar_mul(qpoly *out, const qpoly *p, const rbc_elt *e)
{
	qpoly tmp;
	qpoly *target = out;
	unsigned int i;
	int status;

	if (out == 0 || p == 0 || e == 0 || out->coeffs == 0 || p->coeffs == 0) {
		return LOONG_ERR_NULL;
	}
	if (p->degree > (int)out->max_degree) {
		return LOONG_ERR_BAD_LENGTH;
	}
	if (out == p) {
		status = qpoly_init(&tmp, out->max_degree);
		if (status != LOONG_SUCCESS) {
			return status;
		}
		target = &tmp;
	}
	qpoly_set_zero(target);
	if (!qpoly_is_zero(p) && !rbc_elt_is_zero(e)) {
		for (i = 0; i <= (unsigned int)p->degree; i++) {
			rbc_elt_mul(&target->coeffs[i], &p->coeffs[i], e);
		}
		target->degree = p->degree;
		qpoly_update_degree(target, (unsigned int)p->degree);
	}
	if (target == &tmp) {
		status = qpoly_set(out, &tmp);
		qpoly_clear(&tmp);
		return status;
	}
	return LOONG_SUCCESS;
}

int qpoly_qexp(qpoly *out, const qpoly *p)
{
	qpoly tmp;
	qpoly *target = out;
	unsigned int i;
	int status;

	if (out == 0 || p == 0 || out->coeffs == 0 || p->coeffs == 0) {
		return LOONG_ERR_NULL;
	}
	if (!qpoly_is_zero(p) && p->degree + 1 > (int)out->max_degree) {
		return LOONG_ERR_BAD_LENGTH;
	}
	if (out == p) {
		status = qpoly_init(&tmp, out->max_degree);
		if (status != LOONG_SUCCESS) {
			return status;
		}
		target = &tmp;
	}
	qpoly_set_zero(target);
	if (!qpoly_is_zero(p)) {
		for (i = 0; i <= (unsigned int)p->degree; i++) {
			unsigned int pos = (i + 1U) % RBC_FIELD_M;
			if (pos > target->max_degree) {
				if (target == &tmp) {
					qpoly_clear(&tmp);
				}
				return LOONG_ERR_BAD_LENGTH;
			}
			rbc_elt_sqr(&target->coeffs[pos], &p->coeffs[i]);
		}
		qpoly_update_degree(target, target->max_degree);
	}
	if (target == &tmp) {
		status = qpoly_set(out, &tmp);
		qpoly_clear(&tmp);
		return status;
	}
	return LOONG_SUCCESS;
}

int qpoly_add(qpoly *out, const qpoly *a, const qpoly *b)
{
	qpoly tmp;
	qpoly *target = out;
	unsigned int degree;
	unsigned int i;
	int status;

	if (out == 0 || a == 0 || b == 0 || out->coeffs == 0 ||
	    a->coeffs == 0 || b->coeffs == 0) {
		return LOONG_ERR_NULL;
	}
	degree = qpoly_max_u(a->degree < 0 ? 0U : (unsigned int)a->degree,
	                    b->degree < 0 ? 0U : (unsigned int)b->degree);
	if ((a->degree >= 0 || b->degree >= 0) && degree > out->max_degree) {
		return LOONG_ERR_BAD_LENGTH;
	}
	if (out == a || out == b) {
		status = qpoly_init(&tmp, out->max_degree);
		if (status != LOONG_SUCCESS) {
			return status;
		}
		target = &tmp;
	}
	qpoly_set_zero(target);
	if (a->degree >= 0 || b->degree >= 0) {
		for (i = 0; i <= degree; i++) {
			rbc_elt av;
			rbc_elt bv;
			qpoly_get_coefficient(&av, a, i);
			qpoly_get_coefficient(&bv, b, i);
			rbc_elt_add(&target->coeffs[i], &av, &bv);
		}
		qpoly_update_degree(target, degree);
	}
	if (target == &tmp) {
		status = qpoly_set(out, &tmp);
		qpoly_clear(&tmp);
		return status;
	}
	return LOONG_SUCCESS;
}

int qpoly_mul(qpoly *out, const qpoly *a, const qpoly *b)
{
	qpoly tmp;
	qpoly *target = out;
	int i;
	int j;
	int status;

	if (out == 0 || a == 0 || b == 0 || out->coeffs == 0 ||
	    a->coeffs == 0 || b->coeffs == 0) {
		return LOONG_ERR_NULL;
	}
	if (qpoly_is_zero(a) || qpoly_is_zero(b)) {
		return qpoly_set_zero(out);
	}
	if (a->degree + b->degree > (int)out->max_degree) {
		return LOONG_ERR_BAD_LENGTH;
	}
	if (out == a || out == b) {
		status = qpoly_init(&tmp, out->max_degree);
		if (status != LOONG_SUCCESS) {
			return status;
		}
		target = &tmp;
	}
	qpoly_set_zero(target);
	for (j = 0; j <= b->degree; j++) {
		rbc_elt q_coeff_power;
		rbc_elt term;
		rbc_elt accum;

		rbc_elt_set(&q_coeff_power, &b->coeffs[j]);
		for (i = 0; i <= a->degree; i++) {
			unsigned int pos = (unsigned int)(i + j) % RBC_FIELD_M;
			if (pos > target->max_degree) {
				if (target == &tmp) {
					qpoly_clear(&tmp);
				}
				return LOONG_ERR_BAD_LENGTH;
			}
			if (i != 0) {
				rbc_elt_sqr(&q_coeff_power, &q_coeff_power);
			}
			rbc_elt_mul(&term, &a->coeffs[i], &q_coeff_power);
			rbc_elt_add(&accum, &target->coeffs[pos], &term);
			rbc_elt_set(&target->coeffs[pos], &accum);
		}
	}
	qpoly_update_degree(target, target->max_degree);
	if (target == &tmp) {
		status = qpoly_set(out, &tmp);
		qpoly_clear(&tmp);
		return status;
	}
	return LOONG_SUCCESS;
}

int qpoly_left_div(qpoly *quotient, qpoly *remainder, const qpoly *a,
                   const qpoly *b)
{
	qpoly rtmp;
	qpoly monomial;
	qpoly product;
	rbc_elt b_inv;
	unsigned int guard;
	int status;

	if (quotient == 0 || remainder == 0 || a == 0 || b == 0 ||
	    quotient->coeffs == 0 || remainder->coeffs == 0 ||
	    a->coeffs == 0 || b->coeffs == 0) {
		return LOONG_ERR_NULL;
	}
	if (qpoly_is_zero(b)) {
		return LOONG_ERR_BAD_LENGTH;
	}
	if (qpoly_is_zero(a) || a->degree < b->degree) {
		qpoly_set_zero(quotient);
		return qpoly_set(remainder, a);
	}
	if ((unsigned int)(a->degree - b->degree) > quotient->max_degree ||
	    (b->degree > 0 && (unsigned int)(b->degree - 1) > remainder->max_degree)) {
		return LOONG_ERR_BAD_LENGTH;
	}
	status = qpoly_init(&rtmp, a->max_degree);
	if (status != LOONG_SUCCESS) {
		return status;
	}
	status = qpoly_init(&monomial, (unsigned int)(a->degree - b->degree));
	if (status != LOONG_SUCCESS) {
		qpoly_clear(&rtmp);
		return status;
	}
	status = qpoly_init(&product, a->max_degree);
	if (status != LOONG_SUCCESS) {
		qpoly_clear(&rtmp);
		qpoly_clear(&monomial);
		return status;
	}

	qpoly_set_zero(quotient);
	qpoly_set(&rtmp, a);
	rbc_elt_inv(&b_inv, &b->coeffs[b->degree]);

	guard = 0;
	while (!qpoly_is_zero(&rtmp) && rtmp.degree >= b->degree &&
	       guard <= a->max_degree + 1U) {
		unsigned int shift = (unsigned int)(rtmp.degree - b->degree);
		rbc_elt lead;

		rbc_elt_mul(&lead, &rtmp.coeffs[rtmp.degree], &b_inv);
		rbc_elt_nth_root(&lead, &lead, (unsigned int)b->degree);

		qpoly_set_zero(&monomial);
		qpoly_set_coefficient(&monomial, shift, &lead);
		qpoly_add(quotient, quotient, &monomial);
		qpoly_mul(&product, b, &monomial);
		qpoly_add(&rtmp, &rtmp, &product);
		guard++;
	}
	if (guard > a->max_degree + 1U) {
		status = LOONG_ERR_SAMPLING;
	} else {
		status = qpoly_set(remainder, &rtmp);
	}

	qpoly_clear(&rtmp);
	qpoly_clear(&monomial);
	qpoly_clear(&product);
	return status;
}

int qpoly_set_interpolate_zero(qpoly *annihilator, const rbc_elt *points,
                               unsigned int size)
{
	qpoly qexp;
	qpoly scaled;
	unsigned int i;
	int status;

	if (annihilator == 0 || (size != 0 && points == 0) ||
	    annihilator->coeffs == 0) {
		return LOONG_ERR_NULL;
	}
	if (size > annihilator->max_degree) {
		return LOONG_ERR_BAD_LENGTH;
	}
	status = qpoly_init(&qexp, annihilator->max_degree);
	if (status != LOONG_SUCCESS) {
		return status;
	}
	status = qpoly_init(&scaled, annihilator->max_degree);
	if (status != LOONG_SUCCESS) {
		qpoly_clear(&qexp);
		return status;
	}

	qpoly_set_one(annihilator);
	for (i = 0; i < size; i++) {
		rbc_elt value;
		status = qpoly_evaluate(&value, annihilator, &points[i]);
		if (status != LOONG_SUCCESS) {
			break;
		}
		if (rbc_elt_is_zero(&value)) {
			status = LOONG_ERR_CRYPTO_REJECT;
			break;
		}
		qpoly_qexp(&qexp, annihilator);
		qpoly_scalar_mul(&scaled, annihilator, &value);
		status = qpoly_add(annihilator, &scaled, &qexp);
		if (status != LOONG_SUCCESS) {
			break;
		}
	}

	qpoly_clear(&qexp);
	qpoly_clear(&scaled);
	return status;
}

int qpoly_set_interpolate_vect_and_zero(qpoly *annihilator, qpoly *interpolant,
                                        const rbc_elt *points,
                                        const rbc_elt *values,
                                        unsigned int size)
{
	qpoly qexp;
	qpoly scaled_ann;
	qpoly scaled_interp;
	unsigned int i;
	int status;

	if (annihilator == 0 || interpolant == 0 ||
	    (size != 0 && (points == 0 || values == 0)) ||
	    annihilator->coeffs == 0 || interpolant->coeffs == 0) {
		return LOONG_ERR_NULL;
	}
	if (size > annihilator->max_degree ||
	    (size != 0 && size - 1U > interpolant->max_degree)) {
		return LOONG_ERR_BAD_LENGTH;
	}
	status = qpoly_init(&qexp, annihilator->max_degree);
	if (status != LOONG_SUCCESS) {
		return status;
	}
	status = qpoly_init(&scaled_ann, annihilator->max_degree);
	if (status != LOONG_SUCCESS) {
		qpoly_clear(&qexp);
		return status;
	}
	status = qpoly_init(&scaled_interp, interpolant->max_degree);
	if (status != LOONG_SUCCESS) {
		qpoly_clear(&qexp);
		qpoly_clear(&scaled_ann);
		return status;
	}

	qpoly_set_one(annihilator);
	qpoly_set_zero(interpolant);
	for (i = 0; i < size; i++) {
		rbc_elt ann_value;
		rbc_elt interp_value;
		rbc_elt scale;
		rbc_elt inv_ann_value;

		qpoly_evaluate(&ann_value, annihilator, &points[i]);
		if (rbc_elt_is_zero(&ann_value)) {
			status = LOONG_ERR_CRYPTO_REJECT;
			break;
		}
		qpoly_evaluate(&interp_value, interpolant, &points[i]);
		rbc_elt_add(&scale, &interp_value, &values[i]);
		rbc_elt_inv(&inv_ann_value, &ann_value);
		rbc_elt_mul(&scale, &scale, &inv_ann_value);

		qpoly_scalar_mul(&scaled_interp, annihilator, &scale);
		qpoly_add(interpolant, interpolant, &scaled_interp);
		qpoly_qexp(&qexp, annihilator);
		qpoly_scalar_mul(&scaled_ann, annihilator, &ann_value);
		status = qpoly_add(annihilator, &scaled_ann, &qexp);
		if (status != LOONG_SUCCESS) {
			break;
		}
	}

	qpoly_clear(&qexp);
	qpoly_clear(&scaled_ann);
	qpoly_clear(&scaled_interp);
	return status;
}
