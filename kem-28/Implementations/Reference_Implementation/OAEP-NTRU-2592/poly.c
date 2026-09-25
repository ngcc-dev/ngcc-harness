#include <stdint.h>
#include <stdio.h>
#include "params.h"
#include "poly.h"
#include "ntt.h"
#include "reduce.h"
#include "symmetric.h"


/*************************************************
* Name:        load32_littleendian
*
* Description: load 4 bytes into a 32-bit integer
*              in little-endian order
*
* Arguments:   - const uint8_t *x: pointer to input byte array
*
* Returns 32-bit unsigned integer loaded from x
**************************************************/
static uint32_t load32_littleendian(const uint8_t x[4])
{
	uint32_t r;
	r  = (uint32_t)x[0];
	r |= (uint32_t)x[1] << 8;
	r |= (uint32_t)x[2] << 16;
	r |= (uint32_t)x[3] << 24;
	return r;
}

/*************************************************
* Name:        crepmod3
*
* Description: Compute modulus 3 operation
*
* Arguments: - poly *a: pointer to intput integer to be reduced
*
* Returns:     integer in {-1,0,1} congruent to a modulo 3.
**************************************************/
static int16_t crepmod3(int16_t a)
{

	a += (a >> 15) & NTRUOAEP_Q;
	a -= (NTRUOAEP_Q-1)/2;
	a += (a >> 15) & NTRUOAEP_Q;
	a -= (NTRUOAEP_Q+1)/2;

	a  = (a >> 8) + (a & 255);
	a  = (a >> 4) + (a & 15);
	a  = (a >> 2) + (a & 3);
	a  = (a >> 2) + (a & 3);
	a -= 3;
	a += ((a + 1) >> 15) & 3;

	return a;
}

/*************************************************
* Name:        poly_tobytes
*
* Description: Serialization of a polynomial
*
* Arguments:   - uint8_t *r: pointer to output byte array
*                            (needs space for NTRUOAEP_POLYBYTES bytes)
*              - poly *a:    pointer to input polynomial
**************************************************/
void poly_tobytes(uint8_t r[NTRUOAEP_POLYBYTES], const poly *a)
{
    int16_t t[8];

	for (int i = 0; i < NTRUOAEP_N / 8; i++) {
		for (int j = 0; j < 8; j++) {
			t[j] = a->coeffs[8 * i + j];
			t[j] += (t[j] >> 15) & NTRUOAEP_Q;
		}

		r[15 * i]      = (uint8_t)t[0];
		r[15 * i + 1]  = (uint8_t)((t[0] >> 8)  | (t[1] << 7));
		r[15 * i + 2]  = (uint8_t)(t[1] >> 1);
		r[15 * i + 3]  = (uint8_t)((t[1] >> 9)  | (t[2] << 6));
		r[15 * i + 4]  = (uint8_t)(t[2] >> 2);
		r[15 * i + 5]  = (uint8_t)((t[2] >> 10) | (t[3] << 5));
		r[15 * i + 6]  = (uint8_t)(t[3] >> 3);
		r[15 * i + 7]  = (uint8_t)((t[3] >> 11) | (t[4] << 4));
		r[15 * i + 8]  = (uint8_t)(t[4] >> 4);
		r[15 * i + 9]  = (uint8_t)((t[4] >> 12) | (t[5] << 3));
		r[15 * i + 10] = (uint8_t)(t[5] >> 5);
		r[15 * i + 11] = (uint8_t)((t[5] >> 13) | (t[6] << 2));
		r[15 * i + 12] = (uint8_t)(t[6] >> 6);
		r[15 * i + 13] = (uint8_t)((t[6] >> 14) | (t[7] << 1));
		r[15 * i + 14] = (uint8_t)(t[7] >> 7);
	}
}

/*************************************************
* Name:        poly_frombytes
*
* Description: De-serialization of a polynomial;
*              inverse of poly_tobytes
*
* Arguments:   - poly *r:          pointer to output polynomial
*              - const uint8_t *a: pointer to input byte array
*                                  (of NTRUOAEP_POLYBYTES bytes)
**************************************************/
void poly_frombytes(poly *r, const uint8_t a[NTRUOAEP_POLYBYTES])
{
    uint8_t t[15];

	for (int i = 0; i < NTRUOAEP_N / 8; i++) {
		for (int j = 0; j < 15; j++) {
			t[j] = a[15 * i + j];
		}

		r->coeffs[8 * i]     =                          (int16_t)t[0]         | (((int16_t)t[1] & 0x7f) << 8);
		r->coeffs[8 * i + 1] = (int16_t)(t[1] >> 7)  | ((int16_t)t[2] << 1)   | (((int16_t)t[3] & 0x3f) << 9);
		r->coeffs[8 * i + 2] = (int16_t)(t[3] >> 6)  | ((int16_t)t[4] << 2)   | (((int16_t)t[5] & 0x1f) << 10);
		r->coeffs[8 * i + 3] = (int16_t)(t[5] >> 5)  | ((int16_t)t[6] << 3)   | (((int16_t)t[7] & 0x0f) << 11);
		r->coeffs[8 * i + 4] = (int16_t)(t[7] >> 4)  | ((int16_t)t[8] << 4)   | (((int16_t)t[9] & 0x07) << 12);
		r->coeffs[8 * i + 5] = (int16_t)(t[9] >> 3)  | ((int16_t)t[10] << 5)  | (((int16_t)t[11] & 0x03) << 13);
		r->coeffs[8 * i + 6] = (int16_t)(t[11] >> 2) | ((int16_t)t[12] << 6)  | (((int16_t)t[13] & 0x01) << 14);
		r->coeffs[8 * i + 7] = (int16_t)(t[13] >> 1) | ((int16_t)t[14] << 7);
	}
}


/*************************************************
* Name:        poly_cbd1
*
* Description: Sample a polynomial deterministically from a random,
*              with output polynomial close to centered binomial distribution
*
* Arguments:   - poly *r: pointer to output polynomial
*              - const uint8_t *buf: pointer to input random
*                                     (of length NTRUOAEP_N/4 bytes)
**************************************************/
void poly_cbd1(poly *r, const uint8_t buf[NTRUOAEP_N/4])
{
	uint32_t t1, t2;

	for(int i = 0; i < 10; i++)
	{
		for(int j = 0; j < 8; j++)
		{
			t1 = load32_littleendian(buf + 32*i + 4*j);
			t2 = load32_littleendian(buf + 32*i + 4*j + 324);

			for (int k = 0; k < 2; k++)
			{
				for(int l = 0; l < 16; l++)
				{
					r->coeffs[256*i + 16*l + 2*j + k] = (t1 & 0x1) - (t2 & 0x1);

					t1 >>= 1;
					t2 >>= 1;
				}
			}

		}
	}


	t1 = load32_littleendian(buf + 320);
	t2 = load32_littleendian(buf + 644);

	for (int k = 0; k < 2; k++)
	{
		for(int l = 0; l < 16; l++)
		{
			r->coeffs[2560 + 2*l + k] = (t1 & 0x1) - (t2 & 0x1);

			t1 >>= 1;
			t2 >>= 1;
		}
	}

}



/*************************************************
* Name:        poly_cbd1_inv
*
* Description: Recover the randomness w when given a polynomial a in {-1,0,1}^n and another randomness x in {0,1}^n s.t. a = x - r
*
* Arguments:   - uint8_t *w: pointer to output randomness array
* 			   - const poly *a: pointer to input polynomial
*              - const uint8_t *buf: pointer to input randomness
*                                     (of length NTRUOAEP_N/8 bytes)
**************************************************/
int poly_cbd1_inv(uint8_t *w, const poly *a, const uint8_t buf[NTRUOAEP_N/8]) {
	uint32_t t2, t3, t4;
	uint32_t r = 0;

	for(int i = 0; i < 10; i++)
	{
		for(int j = 0; j < 8; j++)
		{
			t2 = load32_littleendian(buf + 32*i + 4*j);
			t3 = 0;

			for (int k = 0; k < 2; k++)
			{
				for(int l = 0; l < 16; l++)
				{
					t4 = t2 & 0x1;
					t4 = t4 - a->coeffs[256*i + 16*l + 2*j + k];
					r |= t4;
					t4 = t4 & 0x1;
					t3 ^= t4 << (l+16*k);
					t2 >>= 1;
				}
			}

			w[32*i + 4*j   ] = t3;
			w[32*i + 4*j + 1] = t3 >> 8;
			w[32*i + 4*j + 2] = t3 >> 16;
			w[32*i + 4*j + 3] = t3 >> 24;
		}
	}


		t2 = load32_littleendian(buf  + 320);
		t3 = 0;

		for (int k = 0; k < 2; k++)
		{
			for(int l = 0; l < 16; l++)
			{
				t4 = t2 & 0x1;
				t4 = t4 - a->coeffs[2560 + 2*l + k];
				r |= t4;
				t4 = t4 & 0x1;
				t3 ^= t4 << (l+16*k);
				t2 >>= 1;
			}
		}

		w[320     ] = t3;
		w[320  + 1] = t3 >> 8;
		w[320  + 2] = t3 >> 16;
		w[320  + 3] = t3 >> 24;



	r = r >> 1;
	r = (-(uint64_t)r) >> 63;

	return r;
}



/*************************************************
* Name:        poly_sotp
*
* Description: Encode a message deterministically using SOTP and a random,
			   with output polynomial close to centered binomial distribution
*
* Arguments:   - poly *r: pointer to output polynomial
*              - const uint8_t *msg: pointer to input message
*              - const uint8_t *buf: pointer to input random
**************************************************/
void poly_sotp(poly *r, const uint8_t *msg, const uint8_t *buf)
{
    uint8_t tmp[NTRUOAEP_N/4];

    for(int i = 0; i < NTRUOAEP_N/8; i++)
    {
         tmp[i] = buf[i]^msg[i];
    }

    for(int i = NTRUOAEP_N/8; i < NTRUOAEP_N/4; i++)
    {
         tmp[i] = buf[i];
    }
	poly_cbd1(r, tmp);
}


/*************************************************
* Name:        poly_sotp_inv
*
* Description: Decode a message deterministically using SOTP_INV and a random
*
* Arguments:   - uint8_t *msg: pointer to output message
*              - const poly *a: pointer to iput polynomial
*              - const uint8_t *buf: pointer to input random
*
* Returns 0 (success) or 1 (failure)
**************************************************/
int poly_sotp_inv(uint8_t *msg, const poly *a, const uint8_t *buf)
{
	uint32_t t1, t2, t3, t4;
	uint32_t r = 0;

	for(int i = 0; i < 10; i++)
	{
		for(int j = 0; j < 8; j++)
		{
			t1 = load32_littleendian(buf + 32*i + 4*j);
			t2 = load32_littleendian(buf + 32*i + 4*j + NTRUOAEP_N/8);
			t3 = 0;

			for (int k = 0; k < 2; k++)
			{
				for(int l = 0; l < 16; l++)
				{
					t4 = t2 & 0x1;
					t4 = a->coeffs[256*i + 16*l + 2*j + k] + t4;
					r |= t4;
					t4 = (t4^t1) & 0x1;
					t3 ^= t4 << (l+16*k);

					t1 >>= 1;
					t2 >>= 1;
				}
			}

			msg[32*i + 4*j   ] = t3;
			msg[32*i + 4*j + 1] = t3 >> 8;
			msg[32*i + 4*j + 2] = t3 >> 16;
			msg[32*i + 4*j + 3] = t3 >> 24;
		}
	}

	t1 = load32_littleendian(buf  + 320);
	t2 = load32_littleendian(buf + 320 + NTRUOAEP_N/8);
	t3 = 0;

	for (int k = 0; k < 2; k++)
	{
		for(int l = 0; l < 16; l++)
		{
    		t4 = t2 & 0x1;
    		t4 = a->coeffs[2560 + 2*l + k] + t4;
    		r |= t4;
    		t4 = (t4^t1) & 0x1;
    		t3 ^= t4 << (l+16*k);

    		t1 >>= 1;
    		t2 >>= 1;
		}
	}

	msg[320     ] = t3;
	msg[320  + 1] = t3 >> 8;
	msg[320  + 2] = t3 >> 16;
	msg[320  + 3] = t3 >> 24;

	r = r >> 1;
	r = (-(uint64_t)r) >> 63;

	return r;
}




/*************************************************
* Name:        poly_ntt
*
* Description: Computes number-theoretic transform (NTT)
*
* Arguments:   - poly *r: pointer to output polynomial
*              - poly *a: pointer to input polynomial
**************************************************/
void poly_ntt(poly *r, const poly *a)
{
	ntt(r->coeffs, a->coeffs);
}

/*************************************************
* Name:        poly_invntt
*
* Description: Computes inverse of number-theoretic transform (NTT)
*
* Arguments:   - poly *r: pointer to output polynomial
*              - poly *a: pointer to input polynomial
**************************************************/
void poly_invntt(poly *r, const poly *a)
{
	invntt(r->coeffs, a->coeffs);
}

/*************************************************
* Name:        poly_invntt_normalized
*
* Description: Computes inverse of number-theoretic transform (NTT)
*
* Arguments:   - poly *r: pointer to output polynomial
*              - poly *a: pointer to input polynomial
**************************************************/
void poly_invntt_normalized(poly *r, const poly *a)
{
	invntt_normalized(r->coeffs, a->coeffs);
}


/*************************************************
* Name:        poly_is_invertible
*
* Description: Check if a NTT-Domain Polynomial is invertible.
*	           Return 1 if the input is not invertible
*
* Arguments:   - const poly *a: pointer to the input polynomial
**************************************************/
int poly_is_invertible(const poly *a)
{

	for(int i = 0; i < NTRUOAEP_N/3; ++i)
	{
		if (a->coeffs[3*i+0]==0 && a->coeffs[3*i+1]==0 && a->coeffs[3*i+2]==0) 
			return 1;
	}

	return 0;
}

/*************************************************
* Name:        poly_baseinv
*
* Description: Inversion of polynomial in NTT domain
*
* Arguments:   - poly *r:       pointer to output polynomial
*              - const poly *a: pointer to input polynomial
*
* Returns:     integer
**************************************************/
int poly_baseinv(poly *r, const poly *a)
{
	int result = 0;

	for(int i = 0; i < NTRUOAEP_N/6; ++i)
	{
		result = baseinv(r->coeffs + 6*i, a->coeffs + 6*i, zetas[432 + i]);
		if(result) return 1;
		result = baseinv(r->coeffs + 6*i + 3, a->coeffs + 6*i + 3, -zetas[432 + i]);
		if(result) return 1;
	 }

	return 0;
}

/*************************************************
* Name:        poly_basemul
*
* Description: Multiplication of two polynomials in NTT domain
*              Do not support inplace operation
*
* Arguments:   - poly *r:       pointer to output polynomial
*              - const poly *a: pointer to first input polynomial
*              - const poly *b: pointer to second input polynomial
**************************************************/
void poly_basemul(poly *r, const poly *a, const poly *b)
{
	for(int i = 0; i < NTRUOAEP_N/6; ++i)
	{
		basemul(r->coeffs + 6*i, a->coeffs + 6*i, b->coeffs + 6*i, zetas[432 + i]);
		basemul(r->coeffs + 6*i + 3, a->coeffs + 6*i + 3, b->coeffs + 6*i + 3, -zetas[432 + i]);
	}
}

/*************************************************
* Name:        poly_basemul_add
*
* Description: Multiplication then addition of three polynomials in NTT domain
*
* Arguments:   - poly *r:       pointer to output polynomial
*              - const poly *a: pointer to first input polynomial
*              - const poly *b: pointer to second input polynomial
*              - const poly *c: pointer to third input polynomial
**************************************************/
void poly_basemul_add(poly *r, const poly *a, const poly *b, const poly *c)
{
	for(int i = 0; i < NTRUOAEP_N/6; ++i)
	{
		basemul_add(r->coeffs + 6*i, a->coeffs + 6*i, b->coeffs + 6*i, c->coeffs + 6*i, zetas[432 + i]);
		basemul_add(r->coeffs + 6*i + 3, a->coeffs + 6*i + 3, b->coeffs + 6*i + 3, c->coeffs + 6*i + 3, -zetas[432 + i]);
	}
}

/*************************************************
* Name:        poly_sub
*
* Description: Subtract two polynomials; 
*
* Arguments: - poly *r: pointer to output polynomial
*            - const poly *a: pointer to first input polynomial
*            - const poly *b: pointer to second input polynomial
**************************************************/
void poly_sub(poly *r, const poly *a, const poly *b)
{
	for(int i = 0; i < NTRUOAEP_N; ++i)
		r->coeffs[i] = onetime_reduce( a->coeffs[i] - b->coeffs[i] );
}

/*************************************************
* Name:        poly_triple
*
* Description: Multiply polynomial by 3;
*              This function will only perform on ternary polynomials.
*              So no modular reduction is performed.
*
*
* Arguments: - poly *r: pointer to output polynomial
*            - const poly *a: pointer to input polynomial
**************************************************/
void poly_triple(poly *r, const poly *a)
{
	for(int i = 0; i < NTRUOAEP_N; ++i)
		r->coeffs[i] = 3*a->coeffs[i];
}

/*************************************************
* Name:        poly_crepmod3
*
* Description: Compute modulus 3 operation to polynomial
*
* Arguments: - poly *r: pointer to output polynomial
*            - const poly *a: pointer to input polynomial
**************************************************/
void poly_crepmod3(poly *r, const poly *a)
{
  for(int i = 0; i < NTRUOAEP_N; i++)
    r->coeffs[i] = crepmod3(a->coeffs[i]);
}



/*************************************************
* Name:        short_poly_to_bytes
*
* Description: pack short polynomials
*
* Arguments: - poly *a: pointer to input polynomial with coefficients {-1,0,1}
*            - char *buf: pointer to output byte array
**************************************************/
void short_poly_to_bytes(uint8_t *buf, const poly *a)
{
	uint8_t c;

	for (int i = 0; i < NTRUOAEP_N / 8; i++) {
		c = a->coeffs[8*i] + 1;
		c += (a->coeffs[8*i + 1] + 1) << 2;
		c += (a->coeffs[8*i + 2] + 1) << 4;
		c += (a->coeffs[8*i + 3] + 1) << 6;
		buf[2*i] = c;

		c = a->coeffs[8*i + 4] + 1;
		c += (a->coeffs[8*i + 5] + 1) << 2;
		c += (a->coeffs[8*i + 6] + 1) << 4;
		c += (a->coeffs[8*i + 7] + 1) << 6;
		buf[2*i + 1] = c;
	}
}



/*************************************************
* Name:        poly_from_byte
*
* Description: sample a
*
* Arguments: - poly *s, *t: pointer to output polynomial
*            - const uint8_t *r, *m: pointer to input randomness and message with len
**************************************************/



/*************************************************
* Name:        poly_oaep
*
* Description: compute two polynomial s and t from randomness r and message m
*
* Arguments: - poly *s, *t: pointer to output polynomial
*            - const uint8_t *r, *m: pointer to input randomness and message with len
**************************************************/
void poly_oaep(poly *s, poly *t, const uint8_t *r, const uint8_t *m)
{
	uint8_t buff[NTRUOAEP_N / 4];
	uint8_t t_byte[NTRUOAEP_N / 4];

	poly_cbd1(t, r);

	short_poly_to_bytes(t_byte, t);

	hash_g(buff, t_byte);

	poly_sotp(s, m, buff);
}


/*************************************************
* Name:        poly_oaep_inv
*
* Description: compute the randomness and mesasge from two polynomial s and t
*
* Arguments: - poly *s, *t: pointer to input polynomial
*            - const uint8_t *r, *m: pointer to output randomness and message with len
**************************************************/
int poly_oaep_inv(const poly *s, const poly *t, uint8_t *r, uint8_t *m)
{
	int fail = 0;
	uint8_t buff[NTRUOAEP_N / 4];

	short_poly_to_bytes(r, t);

	hash_g(buff, r);

	fail |= poly_sotp_inv(m, s, buff);

	return fail;
}







/*************************************************
* Name:        poly_tomontgomery
*
* Description: multiply the polynomial by constant R=2^16
*
* Arguments: - poly *r: pointer to output polynomial
*            - poly *a: pointer to input polynomial
**************************************************/
void poly_tomontgomery(poly *r, const poly *a)
{
	for (int i = 0; i < NTRUOAEP_N; i++)
	{
		r->coeffs[i] = montgomery_reduce(a->coeffs[i]*-2920);
	}
}


/*************************************************
* Name:        poly_frommontgomery
*
* Description: multiply the polynomial by the inverse of constant R=2^16
*
* Arguments: - poly *r: pointer to output polynomial
*            - poly *a: pointer to input polynomial
**************************************************/
void poly_frommontgomery(poly *r, const poly *a)
{
	for (int i = 0; i < NTRUOAEP_N; i++)
	{
		r->coeffs[i] = montgomery_reduce(a->coeffs[i]);
	}
}
