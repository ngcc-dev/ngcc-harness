#include "cbd.h"
#include "minal.h"
#include "ntt.h"
#include "params.h"
#include "poly.h"
#include "reduce.h"
#include "symmetric.h"
#include <assert.h>
#include <stdint.h>


void poly_zeroize(poly *r)
{
  int i;
  for(i=0;i<KEM_N;i++)
    r->coeffs[i] = 0;
}



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
	uint64_t d0;
	for (i = 0; i < (KEM_N / 8); i++)
		{
			for (j = 0; j < 8; j++)
				{
					u = freeze (a->coeffs[8 * i + j]);
					assert ( u < KEM_Q );
					d0 = u << LOG2T; // LOG2T
					d0 += KEM_Q_2; // KEM_Q/2
					d0 *= KEM_Q_DENOM;
					d0 >>= 29; // from find_factor.py script
					t[j] = d0 & LOG2T_BITMASK;
				}

			// Pack tightly into bytes
			r[0] = (t[0] >> 0);
			r[1] = (t[0] >> 8) | (t[1] << 1);
			r[2] = (t[1] >> 7) | (t[2] << 2);
			r[3] = (t[2] >> 6) | (t[3] << 3);
			r[4] = (t[3] >> 5) | (t[4] << 4);
			r[5] = (t[4] >> 4) | (t[5] << 5);
			r[6] = (t[5] >> 3) | (t[6] << 6);
			r[7] = (t[6] >> 2) | (t[7] << 7);
			r[8] = (t[7] >> 1);
			r += 9;
		}
}



/*************************************************
* Name:        cmp_poly_compress
*
* Description: Serializes and consequently compares polynomial to a serialized polynomial
*
* Arguments:   - const unsigned char *r:    pointer to serialized polynomial to compare with
*              - poly *a:                   pointer to input polynomial to serialize and compare
* Returns:                                  boolean indicating whether the polynomials are equal
**************************************************/

int
cmp_poly_compress (const unsigned char *r, const poly *a)
{
	unsigned char rc = 0;
	unsigned int i, j, k=0;
	uint16_t t[8], u;
	uint64_t d0;
	for (i = 0; i < (KEM_N / 8); i++)
		{
			for (j = 0; j < 8; j++)
				{
					u = freeze (a->coeffs[8 * i + j]);
					assert ( u < KEM_Q );
					d0 = u << LOG2T; // LOG2T
					d0 += KEM_Q_2; // KEM_Q/2
					d0 *= KEM_Q_DENOM;
					d0 >>= 29; // from find_factor.py script
					t[j] = d0 & LOG2T_BITMASK;
				}

			// Pack tightly into bytes
			rc |= r[k] ^ (t[0] >> 0);
			rc |= r[k + 1] ^ ((t[0] >> 8) | (t[1] << 1));
			rc |= r[k + 2] ^ ((t[1] >> 7) | (t[2] << 2));
			rc |= r[k + 3] ^ ((t[2] >> 6) | (t[3] << 3));
			rc |= r[k + 4] ^ ((t[3] >> 5) | (t[4] << 4));
			rc |= r[k + 5] ^ ((t[4] >> 4) | (t[5] << 5));
			rc |= r[k + 6] ^ ((t[5] >> 3) | (t[6] << 6));
			rc |= r[k + 7] ^ ((t[6] >> 2) | (t[7] << 7));
			rc |= r[k + 8] ^ ((t[7] >> 1));
			k += 9;
		}

		return rc;
	}


// packing a polynomial with polyvec compression for vectorized operations


void
poly_packcompress_forvec (uint8_t *r, poly *a, int i)
{
	unsigned int j, k; // i
	uint64_t d0;
	uint16_t t[8];
	// for (i = 0; i < KEM_L; i++)
	// {
		for (j = 0; j < KEM_N/8; j++)
		{
			for (k = 0; k < 8; k++)
			{
			
				t[k] = a->coeffs[8 * j + k];
				d0 = t[k];
				d0 <<= LOG2P;
				d0 += KEM_Q_2;
				d0 *= KEM_Q_DENOM;
				d0 >>= 29;
				t[k] = d0 & LOG2P_BITMASK;
			}
			r[176*i + 11*j + 0] = (t[0] >> 0); // as KEM_POLYVECCOMPRESSEDBYTES/KEM_L = KEM_N * LOG2P / 8 = 128 * 11 / 8 = 176
			r[176*i + 11*j + 1] = (t[0] >> 8) | (t[1] << 3);
			r[176*i + 11*j + 2] = (t[1] >> 5) | (t[2] << 6);
			r[176*i + 11*j + 3] = (t[2] >> 2);
			r[176*i + 11*j + 4] = (t[2] >> 10) | (t[3] << 1);
			r[176*i + 11*j + 5] = (t[3] >> 7) | (t[4] << 4);
			r[176*i + 11*j + 6] = (t[4] >> 4) | (t[5] << 7);
			r[176*i + 11*j + 7] = (t[5] >> 1);
			r[176*i + 11*j + 8] = (t[5] >> 9) | (t[6] << 2);
			r[176*i + 11*j + 9] = (t[6] >> 6) | (t[7] << 5);
			r[176*i + 11*j + 10] = (t[7] >> 3);
			// r += 11;
		}
	}



int cmp_poly_packcompress_forvec(const unsigned char *r, poly *a, int i)
{
	unsigned char rc = 0;
	unsigned int j, k; // i
	uint64_t d0;
	uint16_t t[8];
	// for (i = 0; i < KEM_L; i++)
	// {
		for (j = 0; j < KEM_N/8; j++)
		{
			for (k = 0; k < 8; k++)
			{
				t[k] = a->coeffs[8 * j + k];
				d0 = t[k];
				d0 <<= LOG2P;
				d0 += KEM_Q_2;
				d0 *= KEM_Q_DENOM;
				d0 >>= 29;
				t[k] = d0 & LOG2P_BITMASK;
			}
			rc |= r[176*i + 11*j + 0] ^ (t[0] >> 0); // as KEM_POLYVECCOMPRESSEDBYTES/KEM_L = KEM_N * LOG2P / 8 = 128 * 11 / 8 = 176
			rc |= r[176*i + 11*j + 1] ^ ((t[0] >> 8) | (t[1] << 3));
			rc |= r[176*i + 11*j + 2] ^ ((t[1] >> 5) | (t[2] << 6));
			rc |= r[176*i + 11*j + 3] ^ (t[2] >> 2);
			rc |= r[176*i + 11*j + 4] ^ ((t[2] >> 10) | (t[3] << 1));
			rc |= r[176*i + 11*j + 5] ^ ((t[3] >> 7) | (t[4] << 4));
			rc |= r[176*i + 11*j + 6] ^ ((t[4] >> 4) | (t[5] << 7));
			rc |= r[176*i + 11*j + 7] ^ (t[5] >> 1);
			rc |= r[176*i + 11*j + 8] ^ ((t[5] >> 9) | (t[6] << 2));
			rc |= r[176*i + 11*j + 9] ^ ((t[6] >> 6) | (t[7] << 5));
			rc |= r[176*i + 11*j + 10] ^ (t[7] >> 3);
			// r += 11;
		}

		return rc;

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
	uint16_t t[8];
	for (i = 0; i < KEM_N / 8; i++)
		{
			t[0] = (a[0] >> 0) | ((uint16_t)a[1] << 8);
			t[1] = (a[1] >> 1) | ((uint16_t)a[2] << 7);
			t[2] = (a[2] >> 2) | ((uint16_t)a[3] << 6);
			t[3] = (a[3] >> 3) | ((uint16_t)a[4] << 5);
			t[4] = (a[4] >> 4) | ((uint16_t)a[5] << 4);
			t[5] = (a[5] >> 5) | ((uint16_t)a[6] << 3);
			t[6] = (a[6] >> 6) | ((uint16_t)a[7] << 2);
			t[7] = (a[7] >> 7) | ((uint16_t)a[8] << 1);
			a += 9;

			for (j = 0; j < 8; j++)
				// 256 = 2^(t-1)
				r->coeffs[8 * i + j]
						= ((uint32_t)(t[j] & LOG2T_BITMASK) * KEM_Q + 256) >> LOG2T;
		}
}



void
poly_unpackdecompress_forvec (poly *r, const uint8_t *a, int i)
{
	unsigned int j, k; // i
	uint16_t t[8];
	// for (i = 0; i < KEM_L; i++)
	// {
		for (j = 0; j < (KEM_N / 8); j++)
		{

			t[0] = (a[176*i + 11*j + 0] >> 0) | ((uint16_t)a[176*i + 11*j + 1] << 8);
			t[1] = (a[176*i + 11*j + 1] >> 3) | ((uint16_t)a[176*i + 11*j + 2] << 5);
			t[2] = (a[176*i + 11*j + 2] >> 6) | ((uint16_t)a[176*i + 11*j + 3] << 2) | ((uint16_t)a[176*i + 11*j + 4] << 10);
			t[3] = (a[176*i + 11*j + 4] >> 1) | ((uint16_t)a[176*i + 11*j + 5] << 7);
			t[4] = (a[176*i + 11*j + 5] >> 4) | ((uint16_t)a[176*i + 11*j + 6] << 4);
			t[5] = (a[176*i + 11*j + 6] >> 7) | ((uint16_t)a[176*i + 11*j + 7] << 1) | ((uint16_t)a[176*i + 11*j + 8] << 9);
			t[6] = (a[176*i + 11*j + 8] >> 2) | ((uint16_t)a[176*i + 11*j + 9] << 6);
			t[7] = (a[176*i + 11*j + 9] >> 5) | ((uint16_t)a[176*i + 11*j + 10] << 3);
			// a += 11;

			for (k = 0; k < 8; k++)
				// 1024: P/2
				r->coeffs[8 * j + k]
						= ((uint32_t)(t[k] & LOG2P_BITMASK) * KEM_Q + 1024) >> LOG2P;
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
	// freeze?
	for (int i = 0; i < KEM_N; i++)
	{
		r->coeffs[i] = freeze (r->coeffs[i]);
	}
}

/*************************************************
 * Name:        poly_basemul_montgomery
 *
 * Description: Multiplication of two polynomials in NTT domain
 *
 * Input Domain: NTT+Montgomery
 *
 * Output Domain: NTT+Montgomery
 *
 * Arguments:   - poly *r: pointer to output polynomial
 *              - const poly *a: pointer to first input polynomial
 *              - const poly *b: pointer to second input polynomial
 **************************************************/

void
poly_basemul_montgomery (poly *r, const poly *a, const poly *b)
{
	int j, t;
	for (j = 0; j < KEM_N; j++)
		{
			t = montgomery_reduce (MONT_2 * (uint32_t)b->coeffs[j]); // 1674 = 2^{2*18} % q
			r->coeffs[j] = montgomery_reduce (a->coeffs[j] * t);
			r->coeffs[j] = barrett_reduce (r->coeffs[j]);
		}
}

//Multiplication of two polynomials in NTT domain, accumulated into r.

void
poly_basemul_montgomery_acc (poly *r, poly *a, poly *b)
{
	int j;
	uint16_t t;
	for (j = 0; j < KEM_N; j++)
		{
			t = montgomery_reduce (MONT_2
														 * (uint32_t)b->coeffs[j]); // 1674 = 2^{2*18} % q
			r->coeffs[j] += montgomery_reduce (a->coeffs[j] * t);
			r->coeffs[j] = barrett_reduce (r->coeffs[j]);
		}
	}




/*************************************************
 * Name:        poly_reduce
 *
 * Description: Applies Barrett reduction to all coefficients of a polynomial
 *              for details of the Barrett reduction see comments in reduce.c
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
			// if (r->coeffs[i] > KEM_Q)
			//	r->coeffs[i] = (r->coeffs[i] - KEM_Q);
		}
	// r->coeffs[i] = barrett_reduce(r->coeffs[i]);
}

void
poly_freeze (poly *r)
{
	unsigned int i;
	for (i = 0; i < KEM_N; i++)
		{
			r->coeffs[i] = freeze (r->coeffs[i]);
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
		r->coeffs[i] = barrett_reduce(a->coeffs[i] + b->coeffs[i]);
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
		r->coeffs[i] = barrett_reduce(a->coeffs[i] + 3*KEM_Q - b->coeffs[i]);
	}
}

/*************************************************
 * Name:        poly_tomont// r->coeffs[i] = barrett_reduce (r->coeffs[i]);
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



// generating and adding the noise polynomial

void poly_addnoise_ntt (poly *r, poly *e, int buf1len, uint8_t *noiseseed1, uint8_t *buf1)
{	
	pseudoXOF (buf1len * 8, noiseseed1, (KEM_SYMBYTES + 1) * 8, buf1);
	noiseseed1[KEM_SYMBYTES] += 1;
	// poly_cbd_eta(&skpv.vec[i], buf1+(i*KEM_ETA*KEM_N/4));
	poly_cbd_eta (e, buf1);

	poly_ntt (e);
	poly_add (r, r, e);

}

void poly_addnoise_nontt (poly *r, poly *e, int buf1len, uint8_t *noiseseed1, uint8_t *buf1)
{	
	pseudoXOF (buf1len * 8, noiseseed1, (KEM_SYMBYTES + 1) * 8, buf1);
	noiseseed1[KEM_SYMBYTES] += 1;
	// poly_cbd_eta(&skpv.vec[i], buf1+(i*KEM_ETA*KEM_N/4));
	poly_cbd_eta (e, buf1);

	// poly_ntt (e);
	poly_add (r, r, e);

}