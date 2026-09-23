#include "rbc_vec.h"

#include <limits.h>
#include <string.h>

unsigned long long rbc_vec_get_encoded_size(unsigned long long size)
{
	if (size > ULLONG_MAX / RBC_FIELD_M) {
		return 0;
	}
	return (size * RBC_FIELD_M + 7ULL) / 8ULL;
}

void rbc_vec_set_zero(rbc_elt *v, unsigned int size)
{
	unsigned int i;

	if (size != 0 && v == NULL) {
		return;
	}

	for (i = 0; i < size; i++) {
		rbc_elt_set_zero(&v[i]);
	}
}

void rbc_vec_set(rbc_elt *o, const rbc_elt *v, unsigned int size)
{
	unsigned int i;

	if (size != 0 && (o == NULL || v == NULL)) {
		return;
	}

	for (i = 0; i < size; i++) {
		rbc_elt_set(&o[i], &v[i]);
	}
}

void rbc_vec_add(rbc_elt *o, const rbc_elt *a, const rbc_elt *b,
                 unsigned int size)
{
	unsigned int i;

	if (size != 0 && (o == NULL || a == NULL || b == NULL)) {
		return;
	}

	for (i = 0; i < size; i++) {
		rbc_elt_add(&o[i], &a[i], &b[i]);
	}
}

int rbc_vec_equal(const rbc_elt *a, const rbc_elt *b, unsigned int size)
{
	unsigned int i;

	if (size != 0 && (a == NULL || b == NULL)) {
		return 0;
	}

	for (i = 0; i < size; i++) {
		if (!rbc_elt_equal(&a[i], &b[i])) {
			return 0;
		}
	}
	return 1;
}

int rbc_vec_get_coefficient(rbc_elt *o, const rbc_elt *v, unsigned int position,
                            unsigned int size)
{
	if (o == NULL || position >= size || v == NULL) {
		return -1;
	}
	rbc_elt_set(o, &v[position]);
	return 0;
}

int rbc_vec_set_coefficient(rbc_elt *v, const rbc_elt *e,
                            unsigned int position, unsigned int size)
{
	if (e == NULL || position >= size || v == NULL) {
		return -1;
	}
	rbc_elt_set(&v[position], e);
	return 0;
}

int rbc_vec_get_rank(unsigned int *rank, const rbc_elt *v, unsigned int size)
{
	unsigned int i;
	unsigned int r = 0;
	rbc_elt basis[RBC_FIELD_M];

	if (rank == NULL || (size != 0 && v == NULL)) {
		return -1;
	}

	rbc_vec_set_zero(basis, RBC_FIELD_M);

	for (i = 0; i < size; i++) {
		rbc_elt x;
		int degree;

		rbc_elt_set(&x, &v[i]);
		degree = rbc_elt_get_degree(&x);
		while (degree >= 0) {
			if (rbc_elt_is_zero(&basis[(unsigned int)degree])) {
				rbc_elt_set(&basis[(unsigned int)degree], &x);
				r++;
				break;
			}
			rbc_elt_add(&x, &x, &basis[(unsigned int)degree]);
			degree = rbc_elt_get_degree(&x);
		}
	}

	*rank = r;
	return 0;
}

int rbc_vec_is_full_rank(const rbc_elt *v, unsigned int size,
                         unsigned int expected_rank)
{
	unsigned int rank;

	if (rbc_vec_get_rank(&rank, v, size) != 0) {
		return 0;
	}
	return rank == expected_rank;
}

int rbc_vec_encode(unsigned char *out, unsigned long long out_len_bytes,
                   const rbc_elt *v, unsigned int size)
{
	unsigned int i;
	unsigned int j;
	unsigned long long needed = rbc_vec_get_encoded_size(size);
	unsigned long long bit_pos = 0;

	if (needed == 0 && size != 0) {
		return -1;
	}
	if (out_len_bytes != needed) {
		return -1;
	}
	if (needed != 0 && (out == NULL || v == NULL)) {
		return -1;
	}

	memset(out, 0, (size_t)needed);
	for (i = 0; i < size; i++) {
		for (j = 0; j < RBC_FIELD_M; j++) {
			if (rbc_elt_get_coefficient(&v[i], j) != 0) {
				out[bit_pos / 8ULL] |=
					(unsigned char)(1U << (7U - (unsigned int)(bit_pos % 8ULL)));
			}
			bit_pos++;
		}
	}

	return 0;
}

int rbc_vec_decode(rbc_elt *v, unsigned int size, const unsigned char *in,
                   unsigned long long in_len_bytes)
{
	unsigned int i;
	unsigned int j;
	unsigned long long needed = rbc_vec_get_encoded_size(size);
	unsigned long long total_bits;
	unsigned int used_in_last;
	unsigned char padding_mask;
	unsigned long long bit_pos = 0;

	if (needed == 0 && size != 0) {
		return -1;
	}
	if (in_len_bytes != needed) {
		return -1;
	}
	if (needed != 0 && (v == NULL || in == NULL)) {
		return -1;
	}

	total_bits = (unsigned long long)size * RBC_FIELD_M;
	used_in_last = (unsigned int)(total_bits % 8ULL);
	if (used_in_last != 0) {
		padding_mask = (unsigned char)((1U << (8U - used_in_last)) - 1U);
		if ((in[needed - 1ULL] & padding_mask) != 0) {
			return -1;
		}
	}

	rbc_vec_set_zero(v, size);
	for (i = 0; i < size; i++) {
		for (j = 0; j < RBC_FIELD_M; j++) {
			unsigned char bit = (unsigned char)
				((in[bit_pos / 8ULL] >> (7U - (unsigned int)(bit_pos % 8ULL))) & 1U);
			if (rbc_elt_set_coefficient(&v[i], j, bit) != 0) {
				return -1;
			}
			bit_pos++;
		}
	}

	return 0;
}
