/*
 * Fixed-point division.
 */

#include "inner.h"

/* see inner.h */
uint64_t
ctl_fnr_div(uint64_t x, uint64_t y)
{
	uint64_t sx, sy, q, b, num;
	int i;

	/*
	 * Get absolute values and signs. From now on, we can suppose
	 * that x and y fit on 63 bits (we ignore edge conditions).
	 */
	sx = x >> 63;
	x = (x ^ -sx) + sx;
	sy = y >> 63;
	y = (y ^ -sy) + sy;

	/*
	 * Do a bit by bit division, assuming that the quotient fits.
	 * The numerator starts at x*2^31, and is shifted one bit a time.
	 */
	q = 0;
	num = x >> 31;
	for (i = 63; i >= 0; i --) {
		b = 1 - ((num - y) >> 63);
		q |= b << i;
		num -= y & -b;
		num <<= 1;
		if (i >= 33) {
			num |= (x >> (i - 33)) & 1;
		}
	}

	/*
	 * Rounding: if the remainder is at least y/2 (scaled), we add
	 * 2^(-32) to the quotient.
	 */
	b = 1 - ((num - y) >> 63);
	q += b;

	/*
	 * Sign management: if the original x and y had different signs,
	 * then we must negate the quotient.
	 */
	sx ^= sy;
	q = (q ^ -sx) + sx;

	return q;
}
