#include "poly.h"
#include "minal.h"
#include "ntt.h"
#include "params.h"
#include "reduce.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>

/*************************************************
 * Name:        poly_compress
 *
 * Description: Compression and subsequent serialization of a polynomial
 *
 * Arguments:   - uint8_t *r: pointer to output byte array
 *                            (of length KEM_POLYCOMPRESSEDBYTES)
 *              - const poly *a: pointer to input polynomial
 **************************************************/
void
poly_compress (uint8_t r[KEM_POLYCOMPRESSEDBYTES], const poly *a)
{
	unsigned int i, j;
	uint16_t t[8], u;
	uint32_t d0;
	for (i = 0; i < KEM_N / 8; i++)
		{
			for (j = 0; j < 8; j++)
				{
					// map to positive standard representatives

					// Get the coefficient
					u = freeze (a->coeffs[8 * i + j]);
					assert (u < KEM_Q);

					// NOTE: Don't need this because the coefs are positive
					// If it is negative add modular Q to rotate
					// u += (u >> 15) & KEM_Q;

					// This is compressing u -> round((2^d/q) * u) mod 2^d
					// where:
					// - d = LOG2T
					// - q = KEM_Q
					// We implement:
					//   round((2^d/q) * u)
					// As:
					//   floor((2^d/q) * u + 1/2)
					// = ((2^d/q) * u + 1/2) & (2^d - 1)
					// = ((2^d/q) * u + q/2q) & (2^d - 1)
					// = (2^(d+1)u/2q + q/2q) & (2^d - 1)
					// = (2^(d+1)u/2 + q/2)/q & (2^d - 1)
					// = (2^(d)u + q/2)/q & (2^d - 1)
					// = ((u << d) + q/2)/q & (2^d - 1)
					// t[j] = ((((uint32_t)u << 5) + KEM_Q/2)/KEM_Q) & 31;
					d0 = u << LOG2T; // LOG2T
					// d0 += 3840; //KEM_Q/2;
					// d0 *= 17474;//(2^27/KEM_Q)+1;
					//
					d0 += KEM_Q_2; // KEM_Q/2
					//
					// Up-shift the denominator for increased precision
					// 2^(32-LOG2T)/KEM_Q, chosen 2^(t-d) scaling because we only care
					// about LOG2T bits anyway so any overflow doesn't matter
					d0 *= KEM_Q_DENOM; // 2^(32-LOG2T)/q
					// d0 += KEM_32_LOG2T;
					//  Down-shift back to get the proper result
					d0 >>= 26; // 32-LOG2T
					// Take only the bottom 5 bits
					t[j] = d0 & LOG2T_BITMASK;
				}

			// Pack tightly into bytes
			r[0] = (t[0] >> 0) | (t[1] << 6);
			r[1] = (t[1] >> 2) | (t[2] << 4);
			r[2] = (t[2] >> 4) | (t[3] << 2);
			r[3] = (t[4] >> 0) | (t[5] << 6);
			r[4] = (t[5] >> 2) | (t[6] << 4);
			r[5] = (t[6] >> 4) | (t[7] << 2);
			r += 6;
		}
}

/*************************************************
 * Name:        poly_decompress
 *
 * Description: De-serialization and subsequent decompression of a polynomial;
 *              approximate inverse of poly_compress
 *
 * Arguments:   - poly *r: pointer to output polynomial
 *              - const uint8_t *a: pointer to input byte array
 *                                  (of length KEM_POLYCOMPRESSEDBYTES bytes)
 **************************************************/
void
poly_decompress (poly *r, const uint8_t a[KEM_POLYCOMPRESSEDBYTES])
{
	unsigned int i, j;
	uint8_t t[8];
	for (i = 0; i < KEM_N / 8; i++)
		{
			t[0] = (a[0] >> 0);
			t[1] = (a[0] >> 6) | (a[1] << 2);
			t[2] = (a[1] >> 4) | (a[2] << 4);
			t[3] = (a[2] >> 2);
			t[4] = (a[3] >> 0);
			t[5] = (a[3] >> 6) | (a[4] << 2);
			t[6] = (a[4] >> 4) | (a[5] << 4);
			t[7] = (a[5] >> 2);
			a += 6;

			for (j = 0; j < 8; j++)
				// c = round((q/2^d) * t)
				//   = floor((q/2^d) * t + 1/2)
				//   = floor((t * q)/2^d + 1/2)
				//   = floor(((t * q) + 2^(d-1))/2^d)
				r->coeffs[8 * i + j]
						= ((uint32_t)(t[j] & LOG2T_BITMASK) * KEM_Q + 32) >> LOG2T;
		}
}

/*************************************************
 * Name:        poly_tobytes
 *
 * Description: Serialization of a polynomial
 *
 * Arguments:   - uint8_t *r: pointer to output byte array
 *                            (needs space for KEM_POLYBYTES bytes)
 *              - const poly *a: pointer to input polynomial
 **************************************************/
void
poly_tobytes (uint8_t r[KEM_POLYBYTES], const poly *a)
{
	unsigned int i, j;
	uint16_t t[2];

	for (i = 0; i < KEM_N / 2; i++)
		{
			// map to positive standard representatives
			for (j = 0; j < 2; j++)
				{
					t[j] = a->coeffs[(2 * i) + j];
					// Don't need this because unsigned positive
					// t[j] += (t[j] >> 15) & KEM_Q;
				}
			uint8_t off = 3 * i;
			r[off + 0] = (uint8_t)(t[0]);
			r[off + 1] = (uint8_t)(((t[0] >> 8) & 0xf) | (t[1] << 4));
			r[off + 2] = (uint8_t)(t[1] >> 4);
		}
}

/*************************************************
 * Name:        poly_frombytes
 *
 * Description: De-serialization of a polynomial;
 *              inverse of poly_tobytes
 *
 * Arguments:   - poly *r: pointer to output polynomial
 *              - const uint8_t *a: pointer to input byte array
 *                                  (of KEM_POLYBYTES bytes)
 **************************************************/
void
poly_frombytes (poly *r, const uint8_t a[KEM_POLYBYTES])
{
	unsigned int i;
	for (i = 0; i < KEM_N / 2; i++)
		{
			uint8_t off1 = (2 * i);
			uint8_t off2 = (3 * i);
			r->coeffs[off1 + 0]
					= (a[off2 + 0] & (0xff)) | (((uint16_t)a[off2 + 1] & 0xf) << 8);
			r->coeffs[off1 + 1]
					= (a[off2 + 1] >> 4 & (0xf)) | (((uint16_t)a[off2 + 2] & 0xff) << 4);
		}
}

/*************************************************
 * Name:        poly_frommsg
 *
 * Description: Convert 32-byte message to polynomial
 *
 * Arguments:   - poly *r: pointer to output polynomial
 *              - const uint8_t *msg: pointer to input message
 **************************************************/
// Minal Decoding Implementation
void
poly_frommsg (poly *r, const uint8_t msg[KEM_INDCPA_MSGBYTES])
{
	unsigned int i, j;
	int16_t codeword[2];
	uint8_t
			msg_bits[4]; // 4--> 2 bits for internal code, 2 bits for external code

	size_t base = 0;
	for (i = 0; i < KEM_N / 4; i++)
		{
			for (j = 0; j < 8; j += 4)
				{
					msg_bits[0] = (msg[i] >> (j + 3)) & 1;
					msg_bits[1] = (msg[i] >> (j + 2)) & 1;
					msg_bits[2] = (msg[i] >> (j + 1)) & 1;
					msg_bits[3] = (msg[i] >> (j + 0)) & 1;

					minal_b2_code_encode (codeword, msg_bits);

					r->coeffs[base] = codeword[0];
					r->coeffs[base + KEM_N / 2] = codeword[1];

					base++;
				}
		}
}

/*************************************************
 * Name:        poly_tomsg
 *
 * Description: Convert polynomial to 32-byte message
 *
 * Arguments:   - uint8_t *msg: pointer to output message
 *              - const poly *a: pointer to input polynomial
 **************************************************/
// Minal Encoding Implementation
void
poly_tomsg (uint8_t msg[KEM_INDCPA_MSGBYTES], const poly *a)
{
	unsigned int i, j;
	int16_t codeword[2];
	uint32_t base = 0;

	for (i = 0; i < KEM_N / 4; i++)
		{
			msg[i] = 0;
			for (j = 0; j < 8; j += 4)
				{
					codeword[0] = (int16_t)a->coeffs[base];
					codeword[1] = (int16_t)a->coeffs[base + KEM_N / 2];
					msg[i] |= (minal_b2_code_decode (codeword) << j);

					base++;
				}
		}
}

/*************************************************
 * Name:        poly_ntt
 *
 * Description: Computes negacyclic number-theoretic transform (NTT) of
 *              a polynomial in place;
 *              inputs assumed to be in normal order, output in bitreversed
 * order
 *
 * Arguments:   - uint16_t *r: pointer to in/output polynomial
 **************************************************/
void
poly_ntt (poly *r)
{
	ntt (r->coeffs);
}

/*************************************************
 * Name:        poly_invntt_tomont
 *
 * Description: Computes inverse of negacyclic number-theoretic transform (NTT)
 *              of a polynomial in place;
 *              inputs assumed to be in bitreversed order, output in normal
 * order
 *
 * Input Domain: NTT
 *
 * Output Domain: Standard
 *
 * Arguments:   - uint16_t *a: pointer to in/output polynomial
 **************************************************/
void
poly_invntt_tomont (poly *r)
{
	invntt (r->coeffs);
}

/*************************************************
 * Name:        poly_basemul_montgomery
 *
 * Description: Multiplication of two polynomials in NTT domain
 *
 * Arguments:   - uint16_t *r: pointer to output polynomial
 *              - const uint16_t *a: pointer to first input polynomial
 *              - const uint16_t *b: pointer to second input polynomial
 **************************************************/
void
poly_basemul_montgomery (poly *r, const poly *a, const poly *b)
{
	int j;
	uint16_t t;
	for (j = 0; j < KEM_N; j++)
		{
			t = montgomery_reduce (MONT_2
														 * (uint32_t)b->coeffs[j]); // 1674 = 2^{2*18} % q
			r->coeffs[j] = montgomery_reduce (a->coeffs[j] * t);
			r->coeffs[j] = barrett_reduce (r->coeffs[j]);
		}
}

/*************************************************
 * Name:        poly_reduce
 *
 * Description: Applies Barrett reduction to all coefficients of a polynomial
 *              for details of the Barrett reduction see comments in reduce.c.
 * 				The output will be coefficients in 0..2 * Q.
 *
 * Arguments:   - poly *r: pointer to input/output polynomial
 **************************************************/
void
poly_reduce (poly *r)
{
	unsigned int i;
	for (i = 0; i < KEM_N; i++)
		{
			r->coeffs[i] = barrett_reduce (r->coeffs[i]);
		}
}

/*************************************************
 * Name:        poly_add
 *
 * Description: Add two polynomials; no modular reduction is performed
 *
 * Arguments: - poly *r: pointer to output polynomial
 *            - const poly *a: pointer to first input polynomial
 *            - const poly *b: pointer to second input polynomial
 **************************************************/
void
poly_add (poly *r, const poly *a, const poly *b)
{
	unsigned int i;
	for (i = 0; i < KEM_N; i++)
		{
			r->coeffs[i] = barrett_reduce (a->coeffs[i] + b->coeffs[i]);
		}
}

/*************************************************
 * Name:        poly_sub
 *
 * Description: Subtract two polynomials; no modular reduction is performed
 *
 * Arguments: - poly *r:       pointer to output polynomial
 *            - const poly *a: pointer to first input polynomial
 *            - const poly *b: pointer to second input polynomial
 **************************************************/
void
poly_sub (poly *r, const poly *a, const poly *b)
{
	unsigned int i;
	for (i = 0; i < KEM_N; i++)
		{
			r->coeffs[i] = barrett_reduce (a->coeffs[i] + 3 * KEM_Q - b->coeffs[i]);
		}
}

/*************************************************
 * Name:        poly_tomont
 *
 * Description: Inplace conversion of all coefficients of a polynomial
 *              from normal domain to Montgomery domain
 *
 * Arguments:   - poly *r: pointer to input/output polynomial
 **************************************************/
void
poly_tomont (poly *r)
{
	unsigned int i;
	// const uint32_t f = (1ULL << 2 * RLOG) % KEM_Q;
	for (i = 0; i < KEM_N; i++)
		{
			r->coeffs[i] = montgomery_reduce ((uint32_t)r->coeffs[i] * MONT_2);
			r->coeffs[i] = barrett_reduce (r->coeffs[i]);
		}
}

/*************************************************
 * Name:        poly_freeze
 *
 * Description: Run freeze on all coefficients of
 * 							the polynomial to reduce to mod Q
 *
 * Arguments:   - poly *r: pointer to input/output polynomial
 **************************************************/
void
poly_freeze (poly *r)
{
	for (int i = 0; i < KEM_N; i++)
		{
			r->coeffs[i] = freeze (r->coeffs[i]);
		}
}
