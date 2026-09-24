#include "polyvec.h"
#include "params.h"
#include "poly.h"
#include "reduce.h"
#include <stdint.h>
#include <stdio.h>

/*************************************************
 * Name:        polyvec_compress
 *
 * Description: Compress and serialize vector of polynomials
 *
 * Arguments:   - uint8_t *r: pointer to output byte array
 *                            (needs space for KEM_POLYVECCOMPRESSEDBYTES)
 *              - const polyvec *a: pointer to input vector of polynomials
 **************************************************/
void
polyvec_compress (uint8_t r[KEM_POLYVECCOMPRESSEDBYTES], const polyvec *a)
{
	unsigned int i, j;
#if LOG2P != LOG2Q
	unsigned int k;
	uint64_t d0;
	uint16_t t[8];
#endif
	for (i = 0; i < KEM_L; i++)
		{
#if LOG2P != LOG2Q
			for (j = 0; j < KEM_N / 8; j++)
				{
					for (k = 0; k < 8; k++)
						{
							// For explanation of this section, see poly_compress
							t[k] = a->vec[i].coeffs[8 * j + k];
							d0 = t[k];
							d0 <<= LOG2P;
							d0 += KEM_Q_2;
							d0 *= KEM_Q_232;
							d0 >>= 32;
							t[k] = d0 & LOG2P_BITMASK;
						}
#else
			for (j = 0; j < KEM_N;)
				{
					r[0] = (uint8_t)(a->vec[i].coeffs[j]);
					r[1] = (uint8_t)(((a->vec[i].coeffs[j] >> 8) & 0x1f)
													 | (a->vec[i].coeffs[j + 1] << 5));
					r[2] = (uint8_t)(a->vec[i].coeffs[j + 1] >> 3);
					r[3] = (uint8_t)(((a->vec[i].coeffs[j + 1] >> 11) & 0x03)
													 | (a->vec[i].coeffs[j + 2] << 2));
					r[4] = (uint8_t)(((a->vec[i].coeffs[j + 2] >> 6) & 0x7f)
													 | (a->vec[i].coeffs[j + 3] << 7));
					r[5] = (uint8_t)(a->vec[i].coeffs[j + 3] >> 1);
					r[6] = (uint8_t)(((a->vec[i].coeffs[j + 3] >> 9) & 0x0f)
													 | (a->vec[i].coeffs[j + 4] << 4));
					r[7] = (uint8_t)(a->vec[i].coeffs[j + 4] >> 4);
					r[8] = (uint8_t)(((a->vec[i].coeffs[j + 4] >> 12) & 0x01)
													 | (a->vec[i].coeffs[j + 5] << 1));
					r[9] = (uint8_t)(((a->vec[i].coeffs[j + 5] >> 7) & 0x3f)
													 | (a->vec[i].coeffs[j + 6] << 6));
					r[10] = (uint8_t)(a->vec[i].coeffs[j + 6] >> 2);
					r[11] = (uint8_t)(((a->vec[i].coeffs[j + 6] >> 10) & 0x07)
														| (a->vec[i].coeffs[j + 7] << 3));
					r[12] = (uint8_t)(a->vec[i].coeffs[j + 7] >> 5);
					j += 8;
#endif
					r += 13;
				}
		}
}

/*************************************************
 * Name:        polyvec_decompress
 *
 * Description: De-serialize and decompress vector of polynomials;
 *              approximate inverse of polyvec_compress
 *
 * Arguments:   - polyvec *r:       pointer to output vector of polynomials
 *              - const uint8_t *a: pointer to input byte array
 *                                  (of length KEM_POLYVECCOMPRESSEDBYTES)
 **************************************************/
void
polyvec_decompress (polyvec *r, const uint8_t a[KEM_POLYVECCOMPRESSEDBYTES])
{
	unsigned int i, j;
#if LOG2P != LOG2Q
	unsigned int k;
	uint16_t t[8];
#endif
	for (i = 0; i < KEM_L; i++)
		{
#if LOG2P != LOG2Q
			for (j = 0; j < (KEM_N / 8); j++)
				{
					t[0] = (a[0] & (0xff)) | (((uint16_t)a[1] & 0x1f) << 8);
					t[1] = (a[1] >> 5 & (0x07)) | (((uint16_t)a[2] & 0xff) << 3)
								 | (((uint16_t)a[3] & 0x03) << 11);
					t[2] = (a[3] >> 2 & (0x3f)) | (((uint16_t)a[4] & 0x7f) << 6);
					t[3] = (a[4] >> 7 & (0x01)) | (((uint16_t)a[5] & 0xff) << 1)
								 | (((uint16_t)a[6] & 0x0f) << 9);
					t[4] = (a[6] >> 4 & (0x0f)) | (((uint16_t)a[7] & 0xff) << 4)
								 | (((uint16_t)a[8] & 0x01) << 12);
					t[5] = (a[8] >> 1 & (0x7f)) | (((uint16_t)a[9] & 0x3f) << 7);
					t[6] = (a[9] >> 6 & (0x03)) | (((uint16_t)a[10] & 0xff) << 2)
								 | (((uint16_t)a[11] & 0x07) << 10);
					t[7] = (a[11] >> 3 & (0x1f)) | (((uint16_t)a[12] & 0xff) << 5);
					a += 13;

					for (k = 0; k < 8; k++)
						// 4096: P/2
						r->vec[i].coeffs[8 * j + k]
								= ((uint32_t)(t[k] & LOG2P_BITMASK) * KEM_Q + 4096) >> LOG2P;
#else
			for (j = 0; j < KEM_N;)
				{
					r->vec[i].coeffs[j]
							= (a[0] & (0xff)) | (((uint16_t)a[1] & 0x1f) << 8);
					r->vec[i].coeffs[j + 1] = (a[1] >> 5 & (0x07))
																		| (((uint16_t)a[2] & 0xff) << 3)
																		| (((uint16_t)a[3] & 0x03) << 11);
					r->vec[i].coeffs[j + 2]
							= (a[3] >> 2 & (0x3f)) | (((uint16_t)a[4] & 0x7f) << 6);
					r->vec[i].coeffs[j + 3] = (a[4] >> 7 & (0x01))
																		| (((uint16_t)a[5] & 0xff) << 1)
																		| (((uint16_t)a[6] & 0x0f) << 9);
					r->vec[i].coeffs[j + 4] = (a[6] >> 4 & (0x0f))
																		| (((uint16_t)a[7] & 0xff) << 4)
																		| (((uint16_t)a[8] & 0x01) << 12);
					r->vec[i].coeffs[j + 5]
							= (a[8] >> 1 & (0x7f)) | (((uint16_t)a[9] & 0x3f) << 7);
					r->vec[i].coeffs[j + 6] = (a[9] >> 6 & (0x03))
																		| (((uint16_t)a[10] & 0xff) << 2)
																		| (((uint16_t)a[11] & 0x07) << 10);
					r->vec[i].coeffs[j + 7]
							= (a[11] >> 3 & (0x1f)) | (((uint16_t)a[12] & 0xff) << 5);
					j += 8;
					a += 13;
#endif
				}
		}
}

/*************************************************
 * Name:        polyvec_tobytes
 *
 * Description: Serialize vector of polynomials
 *
 * Arguments:   - uint8_t *r: pointer to output byte array
 *                            (needs space for KEM_POLYVECBYTES)
 *              - const polyvec *a: pointer to input vector of polynomials
 **************************************************/
void
polyvec_tobytes (uint8_t r[KEM_POLYVECBYTES], const polyvec *a)
{
	unsigned int i;
	for (i = 0; i < KEM_L; i++)
		poly_tobytes (r + i * KEM_POLYBYTES, &a->vec[i]);
}

/*************************************************
 * Name:        polyvec_frombytes
 *
 * Description: De-serialize vector of polynomials;
 *              inverse of polyvec_tobytes
 *
 * Arguments:   - uint8_t *r:       pointer to output byte array
 *              - const polyvec *a: pointer to input vector of polynomials
 *                                  (of length KEM_POLYVECBYTES)
 **************************************************/
void
polyvec_frombytes (polyvec *r, const uint8_t a[KEM_POLYVECBYTES])
{
	unsigned int i;
	for (i = 0; i < KEM_L; i++)
		poly_frombytes (&r->vec[i], a + i * KEM_POLYBYTES);
}

/*************************************************
 * Name:        polyvec_ntt
 *
 * Description: Apply forward NTT to all elements of a vector of polynomials
 *
 * Arguments:   - polyvec *r: pointer to in/output vector of polynomials
 **************************************************/
void
polyvec_ntt (polyvec *r)
{
	unsigned int i;
	for (i = 0; i < KEM_L; i++)
		{
			// Convert to mont
			// poly_tomont (&r->vec[i]);
			poly_ntt (&r->vec[i]);
		}
}

/*************************************************
 * Name:        polyvec_invntt_tomont
 *
 * Description: Apply inverse NTT to all elements of a vector of polynomials
 *              and multiply by Montgomery factor 2^16
 *
 * Arguments:   - polyvec *r: pointer to in/output vector of polynomials
 **************************************************/
void
polyvec_invntt_tomont (polyvec *r)
{
	unsigned int i;
	for (i = 0; i < KEM_L; i++)
		poly_invntt_tomont (&r->vec[i]);
}

/*************************************************
 * Name:        polyvec_basemul_acc_montgomery
 *
 * Description: Multiply elements of a and b in NTT domain, accumulate into r,
 *              and multiply by 2^-16.
 *
 * Input Domain: NTT+Montgomery
 *
 * Output Domain: NTT+Montgomery
 *
 * Arguments: - poly *r: pointer to output polynomial
 *            - const polyvec *a: pointer to first input vector of polynomials
 *            - const polyvec *b: pointer to second input vector of polynomials
 **************************************************/
void
polyvec_basemul_acc_montgomery (poly *r, const polyvec *a, const polyvec *b)
{
	int i, j;
	uint16_t t;
	for (j = 0; j < KEM_N; j++)
		{
			t = montgomery_reduce ((uint32_t)b->vec[0].coeffs[j] * MONT_2);
			r->coeffs[j] = montgomery_reduce ((uint32_t)a->vec[0].coeffs[j] * t);
			for (i = 1; i < KEM_L; i++)
				{
					t = montgomery_reduce ((uint32_t)b->vec[i].coeffs[j] * MONT_2);
					r->coeffs[j]
							+= montgomery_reduce ((uint32_t)a->vec[i].coeffs[j] * t);
					r->coeffs[j] = barrett_reduce (r->coeffs[j]);
				}
		}
}

/*************************************************
 * Name:        polyvec_reduce
 *
 * Description: Applies Barrett reduction to each coefficient
 *              of each element of a vector of polynomials;
 *              for details of the Barrett reduction see comments in reduce.c
 *
 * Arguments:   - polyvec *r: pointer to input/output polynomial
 **************************************************/
void
polyvec_reduce (polyvec *r)
{
	unsigned int i;
	for (i = 0; i < KEM_L; i++)
		poly_reduce (&r->vec[i]);
}

/*************************************************
 * Name:        polyvec_add
 *
 * Description: Add vectors of polynomials
 *
 * Arguments: - polyvec *r: pointer to output vector of polynomials
 *            - const polyvec *a: pointer to first input vector of polynomials
 *            - const polyvec *b: pointer to second input vector of polynomials
 **************************************************/
void
polyvec_add (polyvec *r, const polyvec *a, const polyvec *b)
{
	unsigned int i;
	for (i = 0; i < KEM_L; i++)
		poly_add (&r->vec[i], &a->vec[i], &b->vec[i]);
}
