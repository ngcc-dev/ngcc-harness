#include "cbd.h"
#include "params.h"
#include "reduce.h"
#include <stdint.h>
#include <stdio.h>

/*************************************************
 * Name:        load32_littleendian
 *
 * Description: load bytes into a 32-bit integer
 *              in little-endian order
 *
 * Arguments:   - const unsigned char *x: pointer to input byte array
 *
 * Returns 32-bit unsigned integer loaded from x
 **************************************************/
static uint32_t
load32_littleendian (const unsigned char *x)
{
	uint32_t r;
	r = (uint32_t)x[0];
	r |= (uint32_t)x[1] << 8;
	r |= (uint32_t)x[2] << 16;
	r |= (uint32_t)x[3] << 24;
	return r;
}

/*************************************************
 * Name:        cbd
 *
 * Description: Given an array of uniformly random bytes, compute
 *              polynomial with coefficients distributed according to
 *              a centered binomial distribution with parameter KEM_ETA
 *
 * Arguments:   - poly *r:                  pointer to output polynomial
 *              - const unsigned char *buf: pointer to input byte array
 **************************************************/
static void
cbd (poly *r, const unsigned char *buf)
{
#if KEM_ETA == 2
	unsigned int i, j;
	uint32_t t, d;
	int16_t a, b;

	for (i = 0; i < KEM_N / 8; i++)
		{
			t = load32_littleendian (buf + 4 * i);
			d = t & 0x55555555;
			d += (t >> 1) & 0x55555555;

			for (j = 0; j < 8; j++)
				{
					a = (d >> (4 * j + 0)) & 0x3;
					b = (d >> (4 * j + 2)) & 0x3;
					// 0xfff for Q bitmask (i.e. 12 bits)
					r->coeffs[8 * i + j] = (a - b + KEM_Q) & (0xfff);
				}
		}
		// uint32_t d, t;
		// uint16_t a, b;
		// uint16_t r1[KEM_N];
		// int i, j;

		// for (i = 0; i < KEM_N / 8; i++)
		//	{
		//		t = load32_littleendian (buf + 4 * i);
		//		d = t & 0x55555555;
		//		d += (t >> 1) & 0x55555555;

		//		for (j = 0; j < 8; j++)
		//			{
		//				a = (d >> 4 * j) & 0x3;
		//				b = (d >> (4 * j + 2)) & 0x3;
		//				r1[8 * i + j] = (a + KEM_Q - b) & 0x1fff;
		//			}
		//	}
		// for (i = 0; i < KEM_N / 2; i++)
		//	{
		//		r->coeffs[i] = r1[2 * i];
		//		r->coeffs[32 + i] = r1[2 * i + 1];
		//	}

#else
	// uint16_t Qmod_minus1 = KEM_Q - 1;
	uint16_t d, a[4], b[4];
	int i;

	for (i = 0; i < KEM_N / 4; i++)
		{
			d = buf[i];
			a[0] = d & 0x1;
			b[0] = (d >> 1) & 0x1;
			a[1] = (d >> 2) & 0x1;
			b[1] = (d >> 3) & 0x1;
			a[2] = (d >> 4) & 0x1;
			b[2] = (d >> 5) & 0x1;
			a[3] = (d >> 6) & 0x1;
			b[3] = (d >> 7) & 0x1;

			r->coeffs[4 * i + 0] = (uint16_t)(a[0] - b[0] + KEM_Q) & (0xfff);
			r->coeffs[4 * i + 1] = (uint16_t)(a[1] - b[1] + KEM_Q) & (0xfff);
			r->coeffs[4 * i + 2] = (uint16_t)(a[2] - b[2] + KEM_Q) & (0xfff);
			r->coeffs[4 * i + 3] = (uint16_t)(a[3] - b[3] + KEM_Q) & (0xfff);
		}
#endif
}

void
poly_cbd_eta (poly *r, const uint8_t buf[KEM_ETA * KEM_N / 4])
{
	cbd (r, buf);
}
