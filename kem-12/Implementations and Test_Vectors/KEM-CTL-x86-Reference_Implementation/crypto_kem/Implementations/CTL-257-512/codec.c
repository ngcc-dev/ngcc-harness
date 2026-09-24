#include "inner.h"

/* see inner.h */
const uint8_t ctl_max_fg_bits[] = {
	0, /* unused */
	6,
	6,
	6,
	6,
	6,
	5,
	5,
	4,
	4,
	4,
	4
};

/* see inner.h */
const uint8_t ctl_max_FG_bits[] = {
	0, /* unused */
	6,
	6,
	6,
	6,
	6,
	6,
	6,
	6,
	6,
	6,
	12
};

/* see inner.h */
const uint8_t ctl_max_w_bits[] = {
	0, /* unused */
	17,
	17,
	17,
	17,
	17,
	17,
	17,
	17,
	17,
	17,
	17
};

/* see inner.h */
size_t
ctl_trim_i32_encode(
	void *out, size_t max_out_len,
	const int32_t *x, unsigned logn, unsigned bits)
{
	size_t n, u, out_len;
	uint8_t *buf;
	uint32_t acc, mask;
	unsigned acc_len;

	n = (size_t)1 << logn;
	out_len = ((n * bits) + 7) >> 3;
	if (out == NULL) {
		return out_len;
	}
	if (out_len > max_out_len) {
		return 0;
	}
	buf = out;
	acc = 0;
	acc_len = 0;
	mask = ((uint32_t)1 << bits) - 1;
	for (u = 0; u < n; u ++) {
		acc = (acc << bits) | ((uint32_t)x[u] & mask);
		acc_len += bits;
		while (acc_len >= 8) {
			acc_len -= 8;
			*buf ++ = (uint8_t)(acc >> acc_len);
		}
	}
	if (acc_len > 0) {
		*buf ++ = (uint8_t)(acc << (8 - acc_len));
	}
	return out_len;
}

/* see inner.h */
size_t
ctl_trim_i32_decode(
	int32_t *x, unsigned logn, unsigned bits,
	const void *in, size_t max_in_len)
{
	size_t n, in_len;
	const uint8_t *buf;
	size_t u;
	uint32_t acc, mask1, mask2;
	unsigned acc_len;
	uint32_t r;

	n = (size_t)1 << logn;
	in_len = ((n * bits) + 7) >> 3;
	if (in_len > max_in_len) {
		return 0;
	}
	buf = in;
	u = 0;
	acc = 0;
	acc_len = 0;
	mask1 = ((uint32_t)1 << bits) - 1;
	mask2 = (uint32_t)1 << (bits - 1);
	r = (uint32_t)-1;
	while (u < n) {
		acc = (acc << 8) | *buf ++;
		acc_len += 8;
		while (acc_len >= bits && u < n) {
			uint32_t w, q;

			acc_len -= bits;
			w = (acc >> acc_len) & mask1;
			w |= -(w & mask2);
			x[u ++] = *(int32_t *)&w;

			/* Value w == -mask2 is forbidden. */
			q = w + mask2;
			r &= q | -q;
		}
	}

	/* Extra bits in the last byte must be zero. */
	acc &= (((uint32_t)1 << acc_len) - 1);
	r &= ~(acc | -acc);

	return in_len & -(size_t)(r >> 31);
}

/* see inner.h */
size_t
ctl_trim_i8_encode(
	void *out, size_t max_out_len,
	const int8_t *x, unsigned logn, unsigned bits)
{
	size_t n, u, out_len;
	uint8_t *buf;
	uint32_t acc, mask;
	unsigned acc_len;

	n = (size_t)1 << logn;
	out_len = ((n * bits) + 7) >> 3;
	if (out == NULL) {
		return out_len;
	}
	if (out_len > max_out_len) {
		return 0;
	}
	buf = out;
	acc = 0;
	acc_len = 0;
	mask = ((uint32_t)1 << bits) - 1;
	for (u = 0; u < n; u ++) {
		acc = (acc << bits) | ((uint8_t)x[u] & mask);
		acc_len += bits;
		while (acc_len >= 8) {
			acc_len -= 8;
			*buf ++ = (uint8_t)(acc >> acc_len);
		}
	}
	if (acc_len > 0) {
		*buf ++ = (uint8_t)(acc << (8 - acc_len));
	}
	return out_len;
}

/* see inner.h */
size_t
ctl_trim_i8_decode(
	int8_t *x, unsigned logn, unsigned bits,
	const void *in, size_t max_in_len)
{
	size_t n, in_len;
	const uint8_t *buf;
	size_t u;
	uint32_t acc, mask1, mask2;
	unsigned acc_len;
	uint32_t r;

	n = (size_t)1 << logn;
	in_len = ((n * bits) + 7) >> 3;
	if (in_len > max_in_len) {
		return 0;
	}
	buf = in;
	u = 0;
	acc = 0;
	acc_len = 0;
	mask1 = ((uint32_t)1 << bits) - 1;
	mask2 = (uint32_t)1 << (bits - 1);
	r = (uint32_t)-1;
	while (u < n) {
		acc = (acc << 8) | *buf ++;
		acc_len += 8;
		while (acc_len >= bits && u < n) {
			uint32_t w, q;

			acc_len -= bits;
			w = (acc >> acc_len) & mask1;
			w |= -(w & mask2);
			x[u ++] = (int8_t)*(int32_t *)&w;

			/* Value w == -mask2 is forbidden. */
			q = w + mask2;
			r &= q | -q;
		}
	}

	/* Extra bits in the last byte must be zero. */
	acc &= (((uint32_t)1 << acc_len) - 1);
	r &= ~(acc | -acc);

	return in_len & -(size_t)(r >> 31);
}

/* see inner.h */
size_t
ctl_trim_i16_encode(
	void *out, size_t max_out_len,
	const int16_t *x, unsigned logn, unsigned bits)
{
	size_t n, u, out_len;
	uint8_t *buf;
	uint32_t acc, mask;
	unsigned acc_len;

	n = (size_t)1 << logn;
	out_len = ((n * bits) + 7) >> 3;
	if (out == NULL) {
		return out_len;
	}
	if (out_len > max_out_len) {
		return 0;
	}
	buf = out;
	acc = 0;
	acc_len = 0;
	mask = ((uint32_t)1 << bits) - 1;
	for (u = 0; u < n; u ++) {
		acc = (acc << bits) | ((uint16_t)x[u] & mask);
		acc_len += bits;
		while (acc_len >= 8) {
			acc_len -= 8;
			*buf ++ = (uint8_t)(acc >> acc_len);
		}
	}
	if (acc_len > 0) {
		*buf ++ = (uint8_t)(acc << (8 - acc_len));
	}
	return out_len;
}

/* see inner.h */
size_t
ctl_trim_i16_decode(
	int16_t *x, unsigned logn, unsigned bits,
	const void *in, size_t max_in_len)
{
	size_t n, in_len;
	const uint8_t *buf;
	size_t u;
	uint32_t acc, mask1, mask2;
	unsigned acc_len;
	uint32_t r;

	n = (size_t)1 << logn;
	in_len = ((n * bits) + 7) >> 3;
	if (in_len > max_in_len) {
		return 0;
	}
	buf = in;
	u = 0;
	acc = 0;
	acc_len = 0;
	mask1 = ((uint32_t)1 << bits) - 1;
	mask2 = (uint32_t)1 << (bits - 1);
	r = (uint32_t)-1;
	while (u < n) {
		acc = (acc << 8) | *buf ++;
		acc_len += 8;
		while (acc_len >= bits && u < n) {
			uint32_t w, q;

			acc_len -= bits;
			w = (acc >> acc_len) & mask1;
			w |= -(w & mask2);
			x[u ++] = (int16_t)*(int32_t *)&w;

			/* Value w == -mask2 is forbidden. */
			q = w + mask2;
			r &= q | -q;
		}
	}

	/* Extra bits in the last byte must be zero. */
	acc &= (((uint32_t)1 << acc_len) - 1);
	r &= ~(acc | -acc);

	return in_len & -(size_t)(r >> 31);
}
