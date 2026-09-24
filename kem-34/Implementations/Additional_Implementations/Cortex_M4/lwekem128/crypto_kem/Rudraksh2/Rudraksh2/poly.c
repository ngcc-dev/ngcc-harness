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
	uint32_t d0;
	for (i = 0; i < KEM_N / 8; i++)
		{
			for (j = 0; j < 8; j++)
				{
					u = freeze (a->coeffs[8 * i + j]);
					assert ( u < KEM_Q );
					d0 = u << LOG2T;
					d0 += KEM_Q_2; // KEM_Q/2
					d0 *= KEM_Q_DENOM; // 2^(32-LOG2T)/q
					d0 >>= 26; // 32-LOG2T
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
* Name:        cmp_poly_compress
*
* Description: Serializes and consequently compares polynomial to a serialized polynomial
*
* Arguments:   - const unsigned char *r:    pointer to serialized polynomial to compare with
*              - poly *a:                   pointer to input polynomial to serialize and compare
* Returns:                                  boolean indicating whether the polynomials are equal
**************************************************/
int cmp_poly_compress(const unsigned char *r, poly *a) {
    unsigned char rc = 0;
	unsigned int i, j, k=0;
	uint16_t t[8], u;
	uint32_t d0;
	for (i = 0; i < KEM_N / 8; i++)
		{
			for (j = 0; j < 8; j++)
				{
					u = freeze (a->coeffs[8 * i + j]);
					assert ( u < KEM_Q );
					d0 = u << LOG2T; // LOG2T
					d0 += KEM_Q_2; // KEM_Q/2
					d0 *= KEM_Q_DENOM; // 2^(32-LOG2T)/q
					d0 >>= 26; // 32-LOG2T
					t[j] = d0 & LOG2T_BITMASK;
				}

			// Pack tightly into bytes
			rc |= r[k] ^ ((t[0] >> 0) | (t[1] << 6));
			rc |= r[k+1] ^ ((t[1] >> 2) | (t[2] << 4));
			rc |= r[k+2] ^ ((t[2] >> 4) | (t[3] << 2));
			rc |= r[k+3] ^ ((t[4] >> 0) | (t[5] << 6));
			rc |= r[k+4] ^ ((t[5] >> 2) | (t[6] << 4));
			rc |= r[k+5] ^ ((t[6] >> 4) | (t[7] << 2));
			k += 6;
		}

		return rc;
	}



// packing a polynomial with polyvec compression for vectorized operations.


void
poly_packcompress_forvec (uint8_t *r, poly *a, int i)
{
	unsigned int j, k; // i
	uint64_t d0;
	uint16_t t[2];
	// for (i = 0; i < KEM_L; i++)
	// 	{
#if LOG2P != LOG2Q
			for (j = 0; j < KEM_N / 2; j++)
				{
					for (k = 0; k < 2; k++)
						{
							// For explanation of this section, see poly_compress
							t[k] = a->coeffs[2 * j + k];
							//// t[k] += ((int16_t)t[k] >> 15) & KEM_Q;
							////  t[k]  = ((((uint32_t)t[k] << 10) + KEM_Q/2)/ KEM_Q) & 0x3ff;
							d0 = t[k];
							d0 <<= LOG2P;
							// d0 += 3840;		// KEM_Q/2;
							// d0 *= 559168; //(2^32/KEM_Q)+1;
							d0 += KEM_Q_2;
							d0 *= KEM_Q_232;
							d0 >>= 32;
							t[k] = d0 & LOG2P_BITMASK;
						}

					r[96*i + 3*j + 0] = (t[0] >> 0);
					r[96*i + 3*j + 1] = (t[0] >> 8) | (t[1] << 4);
					r[96*i + 3*j + 2] = (t[1] >> 4);
					}
#else
		for (j = 0; j < KEM_N/2; j++) {
					
					r[96*i + 3*j + 0] = (a->coeffs[2*j] >> 0); // as KEM_POLYVECCOMPRESSEDBYTES/KEM_L = KEM_N * LOG2P / 8 = 64 * 12 / 8
					r[96*i + 3*j + 1] = (a->coeffs[2*j] >> 8) | (a->coeffs[2*j+1] << 4);
					r[96*i + 3*j + 2] = (a->coeffs[2*j+1] >> 4);
					// j += 2;
#endif
					// r += 3;
				}
		//}
// }
}


// Description: Serializes and consequently compares polynomial with polyvec compression to a serialized polynomial for vectorized operations.


int cmp_poly_packcompress_forvec(const unsigned char *r, poly *a, int i)
{
	unsigned char rc = 0;
	unsigned int j, k; // i
	uint64_t d0;
	uint16_t t[2];
	// for (i = 0; i < KEM_L; i++)
	// 	{
#if LOG2P != LOG2Q
			for (j = 0; j < KEM_N / 2; j++)
				{
					for (k = 0; k < 2; k++)
						{
							t[k] = a->coeffs[2 * j + k];
							d0 = t[k];
							d0 <<= LOG2P;
							d0 += KEM_Q_2;
							d0 *= KEM_Q_232;
							d0 >>= 32;
							t[k] = d0 & LOG2P_BITMASK;
						}

					
					rc |= r[96*i + 3*j + 0] ^ (t[0] >> 0);
					rc |= r[96*i + 3*j + 1] ^ ((t[0] >> 8) | (t[1] << 4));
					rc |= r[96*i + 3*j + 2] ^ (t[1] >> 4); 
				}
#else
		for (j = 0; j < KEM_N/2; j++) {
					
					rc |= r[96*i + 3*j + 0] ^ (a->coeffs[2*j] >> 0); // as KEM_POLYVECCOMPRESSEDBYTES/KEM_L = KEM_N * LOG2P / 8 = 64 * 12 / 8
					rc |= r[96*i + 3*j + 1] ^ ((a->coeffs[2*j] >> 8) | (a->coeffs[2*j+1] << 4));
					rc |= r[96*i + 3*j + 2] ^ (a->coeffs[2*j+1] >> 4);
					// j += 2;
#endif
					// r += 3;
				}
		//}
// }

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
				r->coeffs[8 * i + j]
						= ((uint32_t)(t[j] & LOG2T_BITMASK) * KEM_Q + 32) >> LOG2T;
		}
}



void
poly_unpackdecompress_forvec (poly *r, const uint8_t *a, int i)
{
	unsigned int j, k; // i
	uint16_t t[2];
	// for (i = 0; i < KEM_L; i++)
	// 	{
#if LOG2P != LOG2Q
			for (j = 0; j < KEM_N / 2; j++)
				{
					t[0] = (a[96*i + 3*j + 0] >> 0) | ((uint16_t)a[96*i + 3*j + 1] << 8);
					t[1] = (a[96*i + 3*j + 1] >> 4) | ((uint16_t)a[96*i + 3*j + 2] << 4);
					// a += 3;

					for (k = 0; k < 2; k++)
						// 2048: P/2
						r->coeffs[2 * j + k]
								= ((uint32_t)(t[k] & LOG2P_BITMASK) * KEM_Q + 2048) >> LOG2P;
				}
#else
			for (j = 0; j < KEM_N/2; j++) {
					r->coeffs[2*j] = ((a[96*i + 3*j + 0] >> 0) | ((uint16_t)a[96*i + 3*j + 1] << 8)) & LOG2P_BITMASK;
					r->coeffs[2*j+1] = ((a[96*i + 3*j + 1] >> 4) | ((uint16_t)a[96*i + 3*j + 2] << 4)) & LOG2P_BITMASK;
					// a += 3;
			}
#endif
		}
//}




















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