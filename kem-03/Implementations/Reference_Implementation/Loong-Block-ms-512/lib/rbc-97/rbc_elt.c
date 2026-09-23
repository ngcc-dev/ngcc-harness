#include "rbc_elt.h"

#include <stddef.h>
#include <string.h>

#define RBC_PRODUCT_WORDS (((2U * RBC_FIELD_M - 1U) + 63U) / 64U)

static const unsigned int rbc_field_poly_terms[RBC_FIELD_POLY_TERMS_COUNT] =
	RBC_FIELD_POLY_TERMS;

static int raw_get_bit(const uint64_t *a, unsigned int position)
{
	return (int)((a[position / 64U] >> (position % 64U)) & 1ULL);
}

static void raw_toggle_bit(uint64_t *a, unsigned int position)
{
	a[position / 64U] ^= 1ULL << (position % 64U);
}

static void xor_shifted(uint64_t product[RBC_PRODUCT_WORDS],
                        const rbc_elt *e, unsigned int shift)
{
	unsigned int i;
	unsigned int word_shift = shift / 64U;
	unsigned int bit_shift = shift % 64U;

	for (i = 0; i < RBC_ELT_WORDS; i++) {
		uint64_t limb = e->v[i];
		unsigned int out_word = i + word_shift;

		if (limb == 0 || out_word >= RBC_PRODUCT_WORDS) {
			continue;
		}

		product[out_word] ^= limb << bit_shift;
		if (bit_shift != 0 && out_word + 1U < RBC_PRODUCT_WORDS) {
			product[out_word + 1U] ^= limb >> (64U - bit_shift);
		}
	}
}

void rbc_elt_normalize(rbc_elt *o)
{
	unsigned int rem;

	if (o == NULL) {
		return;
	}

	rem = RBC_FIELD_M % 64U;
	if (rem != 0) {
		o->v[RBC_ELT_WORDS - 1U] &= (1ULL << rem) - 1ULL;
	}
}

void rbc_elt_set_zero(rbc_elt *o)
{
	if (o != NULL) {
		memset(o, 0, sizeof(*o));
	}
}

void rbc_elt_set_one(rbc_elt *o)
{
	rbc_elt_set_zero(o);
	if (o != NULL) {
		o->v[0] = 1ULL;
	}
}

void rbc_elt_set(rbc_elt *o, const rbc_elt *e)
{
	if (o == NULL || e == NULL) {
		return;
	}
	memcpy(o, e, sizeof(*o));
	rbc_elt_normalize(o);
}

int rbc_elt_is_zero(const rbc_elt *e)
{
	unsigned int i;
	rbc_elt t;

	if (e == NULL) {
		return 1;
	}

	rbc_elt_set(&t, e);
	for (i = 0; i < RBC_ELT_WORDS; i++) {
		if (t.v[i] != 0) {
			return 0;
		}
	}
	return 1;
}

int rbc_elt_equal(const rbc_elt *a, const rbc_elt *b)
{
	unsigned int i;
	rbc_elt ta;
	rbc_elt tb;

	if (a == NULL || b == NULL) {
		return 0;
	}

	rbc_elt_set(&ta, a);
	rbc_elt_set(&tb, b);
	for (i = 0; i < RBC_ELT_WORDS; i++) {
		if (ta.v[i] != tb.v[i]) {
			return 0;
		}
	}
	return 1;
}

int rbc_elt_get_degree(const rbc_elt *e)
{
	int i;

	if (e == NULL) {
		return -1;
	}

	for (i = (int)RBC_FIELD_M - 1; i >= 0; i--) {
		if (rbc_elt_get_coefficient(e, (unsigned int)i) != 0) {
			return i;
		}
	}
	return -1;
}

int rbc_elt_get_coefficient(const rbc_elt *e, unsigned int position)
{
	if (e == NULL || position >= RBC_FIELD_M) {
		return 0;
	}
	return raw_get_bit(e->v, position);
}

int rbc_elt_set_coefficient(rbc_elt *e, unsigned int position, unsigned int bit)
{
	uint64_t mask;

	if (e == NULL || position >= RBC_FIELD_M) {
		return -1;
	}

	mask = 1ULL << (position % 64U);
	if ((bit & 1U) != 0) {
		e->v[position / 64U] |= mask;
	} else {
		e->v[position / 64U] &= ~mask;
	}
	return 0;
}

void rbc_elt_add(rbc_elt *o, const rbc_elt *a, const rbc_elt *b)
{
	unsigned int i;

	if (o == NULL || a == NULL || b == NULL) {
		return;
	}

	for (i = 0; i < RBC_ELT_WORDS; i++) {
		o->v[i] = a->v[i] ^ b->v[i];
	}
	rbc_elt_normalize(o);
}

void rbc_elt_mul(rbc_elt *o, const rbc_elt *a, const rbc_elt *b)
{
	uint64_t product[RBC_PRODUCT_WORDS];
	int d;
	unsigned int i;

	if (o == NULL || a == NULL || b == NULL) {
		return;
	}

	memset(product, 0, sizeof(product));
	for (i = 0; i < RBC_FIELD_M; i++) {
		if (rbc_elt_get_coefficient(a, i) != 0) {
			xor_shifted(product, b, i);
		}
	}

	for (d = (int)(2U * RBC_FIELD_M - 2U); d >= (int)RBC_FIELD_M; d--) {
		if (raw_get_bit(product, (unsigned int)d) != 0) {
			unsigned int j;
			raw_toggle_bit(product, (unsigned int)d);
			for (j = 0; j < RBC_FIELD_POLY_TERMS_COUNT; j++) {
				raw_toggle_bit(product,
				               (unsigned int)d - RBC_FIELD_M + rbc_field_poly_terms[j]);
			}
		}
	}

	for (i = 0; i < RBC_ELT_WORDS; i++) {
		o->v[i] = product[i];
	}
	rbc_elt_normalize(o);
}

void rbc_elt_sqr(rbc_elt *o, const rbc_elt *a)
{
	rbc_elt_mul(o, a, a);
}

void rbc_elt_qpow_2exp(rbc_elt *o, const rbc_elt *a, unsigned int exponent)
{
	unsigned int i;
	rbc_elt t;

	if (o == NULL || a == NULL) {
		return;
	}

	rbc_elt_set(&t, a);
	for (i = 0; i < exponent; i++) {
		rbc_elt_sqr(&t, &t);
	}
	rbc_elt_set(o, &t);
}

void rbc_elt_nth_root(rbc_elt *o, const rbc_elt *a, unsigned int n)
{
	unsigned int exp;

	if (o == NULL || a == NULL) {
		return;
	}

	exp = (RBC_FIELD_M - (n % RBC_FIELD_M)) % RBC_FIELD_M;
	rbc_elt_qpow_2exp(o, a, exp);
}

int rbc_elt_inv(rbc_elt *o, const rbc_elt *a)
{
	int i;
	rbc_elt result;

	if (o == NULL || a == NULL) {
		return -1;
	}
	if (rbc_elt_is_zero(a)) {
		rbc_elt_set_zero(o);
		return -1;
	}

	rbc_elt_set_one(&result);
	for (i = (int)RBC_FIELD_M - 1; i >= 0; i--) {
		rbc_elt_sqr(&result, &result);
		if (i >= 1) {
			rbc_elt_mul(&result, &result, a);
		}
	}

	rbc_elt_set(o, &result);
	return 0;
}
