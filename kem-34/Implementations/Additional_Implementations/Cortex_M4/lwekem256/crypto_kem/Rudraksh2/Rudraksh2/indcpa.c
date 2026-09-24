#include "cbd.h"
#include "drng.h"
#include "indcpa.h"
#include "ntt.h"
#include "params.h"
#include "poly.h"
#include "polyvec.h"
#include "symmetric.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <stdio.h>

extern DRNG_ctx drng_algorithm;

/*************************************************
 * Name:        pack_sk
 *
 * Description: Serialize the secret key
 *
 * Arguments:   - uint8_t *r: pointer to output serialized secret key
 *              - polyvec *sk: pointer to input vector of polynomials (secret
 * key)
 **************************************************/
void
pack_sk (uint8_t r[KEM_INDCPA_SECRETKEYBYTES], polyvec *sk)
{
	polyvec_tobytes (r, sk);
}


/*************************************************
 * Name:        rej_uniform
 *
 * Description: Run rejection sampling on uniform random bytes to generate
 *              uniform random integers mod q
 *
 * Arguments:   - int16_t *r: pointer to output buffer
 *              - unsigned int len: requested number of 16-bit integers
 * (uniform mod q)
 *              - const uint8_t *buf: pointer to input buffer (assumed to be
 * uniformly random bytes)
 *              - unsigned int buflen: length of input buffer in bytes
 *
 * Returns number of sampled 16-bit integers (at most len)
 **************************************************/

static unsigned int
rej_uniform (int16_t *r, unsigned int len, const uint8_t *buf,
						 unsigned int buflen)
{
	unsigned int ctr, pos;
	uint16_t val0, val1;

	ctr = pos = 0;
	while (ctr < len && pos + 3 <= buflen)
		{
			val0 = ((buf[pos + 0] >> 0) | ((uint16_t)buf[pos + 1] << 8)) & 0xFFF;
			val1 = ((buf[pos + 1] >> 4) | ((uint16_t)buf[pos + 2] << 4)) & 0xFFF;
			pos += 3;

			if (val0 < KEM_Q)
				r[ctr++] = val0;
			if (ctr < len && val1 < KEM_Q)
				r[ctr++] = val1;
		}

	return ctr;
}

/*************************************************
 * Name:        gen_matrix
 *
 * Description: Deterministically generate matrix A (or the transpose of A)
 *              from a seed. Entries of the matrix are polynomials that look
 *              uniformly random. Performs rejection sampling on output of
 *              a XOF
 *
 * Arguments:   - polyvec *a: pointer to ouptput matrix A
 *              - const uint8_t *seed: pointer to input seed
 *              - int transposed: boolean deciding whether A or A^T is
 * generated
 **************************************************/

void
gen_matrix_mult_acc (poly *r, polyvec *b, int i, int j, const uint8_t seed[KEM_SYMBYTES], int transposed)
{
	poly ap;
	unsigned int ctr; // j; // .i
	unsigned int buflen = LOG2Q * KEM_N;
	uint8_t buf[buflen], seed1[KEM_SYMBYTES + 3];
	uint8_t *seed_counter = &(seed1[KEM_SYMBYTES + 2]);

	uint8_t buf1[buflen];

	// for (i = 0; i < KEM_L; i++)
		// {
		//	for (j = 0; j < KEM_L; j++)
		//		{
					// ascon_inithash (&state);
					memcpy (seed1, seed, KEM_SYMBYTES);
					if (transposed)
						{
							seed1[KEM_SYMBYTES] = i;
							seed1[KEM_SYMBYTES + 1] = j;
						}
					else
						{
							seed1[KEM_SYMBYTES] = j;
							seed1[KEM_SYMBYTES + 1] = i;
						}
					// ascon_absorb (&state, seed1, KEM_SYMBYTES + 2);
					*seed_counter = (uint8_t)0;

					// ascon_squeeze (&state, buf1, buflen);
					pseudoXOF ((buflen) * 8, seed1, (KEM_SYMBYTES + 3) * 8, buf1);
					for (size_t i1 = 0; i1 < buflen; i1 += 8)
						{
							for (size_t j1 = 0; j1 < 8; j1++)
								{
									buf[i1 + 7 - j1] = buf1[i1 + j1];
								}
						}

					// ctr = rej_uniform (a[i].vec[j].coeffs, KEM_N, buf, buflen);
					ctr = rej_uniform (ap.coeffs, KEM_N, buf, buflen);
	

					while (ctr < KEM_N)
						{
							if (*seed_counter == 256)
								// rejection sampling failed
								abort ();
						

							(*seed_counter)++;
							pseudoXOF ((buflen) * 8, seed1, (KEM_SYMBYTES + 3) * 8, buf1);

							for (size_t i1 = 0; i1 < buflen; i1 += 8)
								{
									for (size_t j1 = 0; j1 < 8; j1++)
										{
											buf[i1 + 7 - j1] = buf1[i1 + j1];
										}
								}

							// ctr += rej_uniform (a[i].vec[j].coeffs + ctr, KEM_N - ctr, buf,
																	// buflen);
							ctr += rej_uniform (ap.coeffs + ctr, KEM_N - ctr, buf,
																	buflen);
						}
						


					// poly_basemul_montgomery_acc (&r, &ap, &b->vec[j]);

					uint16_t t;
					for (int j1 = 0; j1 < KEM_N; j1++)
						{
							t = montgomery_reduce (MONT_2
														 * (uint32_t)b->vec[j].coeffs[j1]); // 1674 = 2^{2*18} % q
							r->coeffs[j1] += montgomery_reduce (ap.coeffs[j1] * t);
							r->coeffs[j1] = barrett_reduce (r->coeffs[j1]);
						}

				
				
					}
		// }
// }

/*************************************************
* Name:        indcpa_keypair
*
* Description: Generates public and private key for the CPA-secure
*              public-key encryption scheme underlying KEM
*
* Arguments:   - uint8_t *pk: pointer to output public key
*                             (of length KEM_INDCPA_PUBLICKEYBYTES bytes)
*              - uint8_t *sk: pointer to output private key
															(of length KEM_INDCPA_SECRETKEYBYTES bytes)
**************************************************/
void
indcpa_keypair (uint8_t pk[KEM_INDCPA_PUBLICKEYBYTES],
								uint8_t sk[KEM_INDCPA_SECRETKEYBYTES])
{
	unsigned int i, j;
	uint8_t buf[2 * KEM_SYMBYTES];
	const uint8_t *publicseed = buf;
	const uint8_t *noiseseed = buf + KEM_SYMBYTES;

	uint8_t noiseseed1[KEM_SYMBYTES + 1];

	polyvec skpv;
	poly pkp;
	 poly e;

	get_random_number (&drng_algorithm, buf, KEM_SYMBYTES * 8);

	// buf[0] = 0xa2; buf[1] = 0x59; buf[2] = 0x42; buf[3] = 0xa3; buf[4] = 0x85;
	// buf[5] = 0xb7; buf[6] = 0x60; buf[7] = 0xde; buf[8] = 0x58; buf[9] = 0xe2;
	// buf[10] = 0x01; buf[11] = 0x7f; buf[12] = 0x08; buf[13] = 0xc9; buf[14] =
	// 0x50; buf[15] = 0x83;

	hash_g (buf, buf, KEM_SYMBYTES);
	uint8_t buf1[KEM_ETA * KEM_N / 4];
	int buf1len;
	buf1len = KEM_ETA * KEM_N / 4;

	for (i = 0; i < KEM_SYMBYTES; i++)
		noiseseed1[i] = noiseseed[i];
	noiseseed1[KEM_SYMBYTES] = 0;
	for (i = 0; i < KEM_L; i++)
		{
			pseudoXOF (buf1len * 8, noiseseed1, (KEM_SYMBYTES + 1) * 8, buf1);
			noiseseed1[KEM_SYMBYTES] += 1;
			// poly_cbd_eta(&skpv.vec[i], buf1+(i*KEM_ETA*KEM_N/4));
			poly_cbd_eta (&skpv.vec[i], buf1);
		}
	polyvec_ntt (&skpv);
	// matrix-vector multiplication
	for (i = 0; i < KEM_L; i++)
		{
			 poly_zeroize (&pkp);
			for (j = 0; j < KEM_L; j++)
				{
					 gen_matrix_mult_acc(&pkp, &skpv, i, j, publicseed, 1);
					
				}
			
			poly_addnoise_ntt (&pkp, &e, buf1len, noiseseed1, buf1);
			
			poly_tobytes (pk + i*KEM_POLYBYTES, &pkp);

		}
	 pack_sk (sk, &skpv);
	for(i=0;i<KEM_SYMBYTES;i++)
		{
			pk[i + KEM_POLYVECBYTES] = publicseed[i];	
		}

}

/*************************************************
 * Name:        indcpa_enc
 *
 * Description: Encryption function of the CPA-secure
 *              public-key encryption scheme underlying KEM.
 *
 * Arguments:   - uint8_t *c: pointer to output ciphertext
 *                            (of length KEM_INDCPA_BYTES bytes)
 *              - const uint8_t *m: pointer to input message
 *                                  (of length KEM_INDCPA_MSGBYTES bytes)
 *              - const uint8_t *pk: pointer to input public key
 *                                   (of length KEM_INDCPA_PUBLICKEYBYTES)
 *              - const uint8_t *coins: pointer to input random coins used as
 * seed (of length KEM_SYMBYTES) to deterministically generate all randomness
 **************************************************/
void
indcpa_enc (uint8_t c[KEM_INDCPA_BYTES], const uint8_t m[KEM_INDCPA_MSGBYTES],
						const uint8_t pk[KEM_INDCPA_PUBLICKEYBYTES],
						const uint8_t coins[KEM_SYMBYTES])
{
	int i, j;
	polyvec sp;
	poly v, epp;

	const uint8_t *seed = pk + KEM_POLYVECBYTES;

	uint8_t buf[KEM_ETA * KEM_N / 4];
	uint8_t coins1[KEM_SYMBYTES + 1];
	int buflen;
	buflen = KEM_ETA * KEM_N / 4;

	for (i = 0; i < KEM_SYMBYTES; i++)
		coins1[i] = coins[i];
	coins1[KEM_SYMBYTES] = 0;

	for (i = 0; i < KEM_L; i++)
		{
			pseudoXOF (buflen * 8, coins1, (KEM_SYMBYTES + 1) * 8, buf);
			coins1[KEM_SYMBYTES] += 1;
			poly_cbd_eta (sp.vec + i, buf);
		}
	
	polyvec_ntt (&sp);
	// matrix-vector multiplication
	for (i = 0; i < KEM_L; i++)
		{
			poly_zeroize (&v);
			for (j = 0; j < KEM_L; j++)
				{
					gen_matrix_mult_acc (&v, &sp, i, j, seed, 0);
				}
			
			poly_invntt_tomont (&v);
			poly_addnoise_nontt (&v, &epp, buflen, coins1, buf);
			poly_packcompress_forvec(c, &v, i);
			
		}	
	
		poly_frombytes (&epp, pk);
		poly_basemul_montgomery (&v, &epp, &sp.vec[0]);
		for (i = 1; i < KEM_L; i++)
			{
				poly_frombytes (&epp, pk + i * KEM_POLYBYTES);
				poly_basemul_montgomery_acc (&v, &epp, &sp.vec[i]);
			}
		
	
		poly_invntt_tomont (&v);
		
		pseudoXOF (buflen * 8, coins1, (KEM_SYMBYTES + 1) * 8, buf);
		 poly_cbd_eta (&epp, buf);

		poly_add (&v, &v, &epp);
		
		poly_frommsg (&epp, m);
		poly_add (&v, &v, &epp);

		poly_compress (c + KEM_POLYVECCOMPRESSEDBYTES, &v);

}


/*************************************************
* Name:        indcpa_enc_cmp
*
* Description: Re-encryption function.
*              Compares the re-encypted ciphertext with the original ciphertext byte per byte.
*              The comparison is performed in a constant time manner.
*
*
* Arguments:   - unsigned char *ct:         pointer to input ciphertext to compare the new ciphertext with (of length KYBER_INDCPA_BYTES bytes)
*              - const unsigned char *m:    pointer to input message (of length KYBER_INDCPA_MSGBYTES bytes)
*              - const unsigned char *pk:   pointer to input public key (of length KYBER_INDCPA_PUBLICKEYBYTES bytes)
*              - const unsigned char *coin: pointer to input random coins used as seed (of length KYBER_SYMBYTES bytes)
*                                           to deterministically generate all randomness
* Returns:     - boolean byte indicating that re-encrypted ciphertext is NOT equal to the original ciphertext
**************************************************/
unsigned char
indcpa_enc_cmp (uint8_t c[KEM_INDCPA_BYTES], const uint8_t m[KEM_INDCPA_MSGBYTES],
						const uint8_t pk[KEM_INDCPA_PUBLICKEYBYTES],
						const uint8_t coins[KEM_SYMBYTES])
{
	uint64_t rc = 0;
	int i, j;
	polyvec sp;
	poly v, epp;

	const uint8_t *seed = pk + KEM_POLYVECBYTES;
	
	uint8_t buf[KEM_ETA * KEM_N / 4];
	uint8_t coins1[KEM_SYMBYTES + 1];
	int buflen;
	buflen = KEM_ETA * KEM_N / 4;

	for (i = 0; i < KEM_SYMBYTES; i++)
		coins1[i] = coins[i];
	coins1[KEM_SYMBYTES] = 0;

	for (i = 0; i < KEM_L; i++)
		{
			pseudoXOF (buflen * 8, coins1, (KEM_SYMBYTES + 1) * 8, buf);
			coins1[KEM_SYMBYTES] += 1;
			poly_cbd_eta (sp.vec + i, buf);
		}
	polyvec_ntt (&sp);
	// matrix-vector multiplication
	for (i = 0; i < KEM_L; i++)
		{
			poly_zeroize (&v);
			for (j = 0; j < KEM_L; j++)
				{
					gen_matrix_mult_acc (&v, &sp, i, j, seed, 0);
				}
			
			poly_invntt_tomont (&v);
			poly_addnoise_nontt (&v, &epp, buflen, coins1, buf);
			
			rc |= cmp_poly_packcompress_forvec(c, &v, i);
			
		}	
	
		poly_frombytes (&epp, pk);
		poly_basemul_montgomery (&v, &epp, &sp.vec[0]);
		for (i = 1; i < KEM_L; i++)
			{
				poly_frombytes (&epp, pk + i * KEM_POLYBYTES);
				poly_basemul_montgomery_acc (&v, &epp, &sp.vec[i]);
			}
		
	
		poly_invntt_tomont (&v);
		
		pseudoXOF (buflen * 8, coins1, (KEM_SYMBYTES + 1) * 8, buf);
		 poly_cbd_eta (&epp, buf);

		poly_add (&v, &v, &epp);
		
		poly_frommsg (&epp, m);
		poly_add (&v, &v, &epp);

	rc |= cmp_poly_compress(c+KEM_POLYVECCOMPRESSEDBYTES, &v); // compress v

  	rc = ~rc + 1;
  	rc >>= 63;
  	return (unsigned char)rc;
	


}


/*************************************************
 * Name:        indcpa_dec
 *
 * Description: Decryption function of the CPA-secure
 *              public-key encryption scheme underlying KEM.
 *
 * Arguments:   - uint8_t *m: pointer to output decrypted message
 *                            (of length KEM_INDCPA_MSGBYTES)
 *              - const uint8_t *c: pointer to input ciphertext
 *                                  (of length KEM_INDCPA_BYTES)
 *              - const uint8_t *sk: pointer to input secret key
 *                                   (of length KEM_INDCPA_SECRETKEYBYTES)
 **************************************************/
void
indcpa_dec (uint8_t m[KEM_INDCPA_MSGBYTES], const uint8_t c[KEM_INDCPA_BYTES],
						const uint8_t sk[KEM_INDCPA_SECRETKEYBYTES])
{
	int i;
	poly skp;
	poly v, mp;
	
	poly_unpackdecompress_forvec(&mp, c, 0);
	poly_ntt(&mp);
	poly_frombytes(&skp, sk);
	poly_basemul_montgomery(&mp, &mp, &skp); 
	for (i = 1; i < KEM_L; i++)
		{
			poly_unpackdecompress_forvec(&v, c, i);
			poly_ntt(&v);
			poly_frombytes(&skp, sk + i * KEM_POLYBYTES);
			poly_basemul_montgomery_acc(&mp, &v, &skp);
		}
	
	poly_invntt_tomont (&mp);
	
	poly_decompress (&v, c + KEM_POLYVECCOMPRESSEDBYTES);
	poly_sub (&mp, &v, &mp);
	poly_tomsg (m, &mp);

}
