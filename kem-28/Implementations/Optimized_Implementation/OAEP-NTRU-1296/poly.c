#include <stdint.h>
#include <stdio.h>
#include "params.h"
#include "poly.h"
#include "ntt.h"
#include "reduce.h"
#include "symmetric.h"
#include "avx2.h"


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
	poly_tobytes_avx2(r, a->coeffs);
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
	poly_frombytes_avx2(r->coeffs, a);
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
	poly_cbd1_avx2(r->coeffs, buf);
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
	return poly_cbd1_inv_avx2(w, a->coeffs, buf);
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
	// 代码是不是没有改成新版本里面的，带减法的方案？
	// 改了，减法在调用的函数里
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
	return poly_sotp_inv_avx2(msg, a->coeffs, buf);
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

	for(int i = 0; i < NTRUOAEP_N/2; ++i)
	{
		if (a->coeffs[2*i+0]==0 && a->coeffs[2*i+1]==0) 
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
	return poly_baseinv_avx2(r->coeffs, a->coeffs);
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
	poly_basemul_avx2(r->coeffs, a->coeffs, b->coeffs);
}

/*************************************************
* Name:        poly_baseadd
*
* Description: Addition of two polynomials in NTT-domain base representation
*
* Arguments:   - poly *r:       pointer to output polynomial
*              - const poly *a: pointer to first input polynomial
*              - const poly *c: pointer to second input polynomial
**************************************************/
void poly_baseadd(poly *r, const poly *a, const poly *c)
{
	poly_baseadd_avx2(r->coeffs, a->coeffs, c->coeffs);
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
	poly_sub_avx2(r->coeffs, a->coeffs, b->coeffs);
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
	poly_triple_avx2(r->coeffs, a->coeffs);
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
	poly_crepmod3_avx2(r->coeffs, a->coeffs);
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
	short_poly_to_bytes_avx2(buf, a->coeffs);
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
	poly_tomontgomery_avx2(r->coeffs, a->coeffs);
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
	poly_frommontgomery_avx2(r->coeffs, a->coeffs);
}
