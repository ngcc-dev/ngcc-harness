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
 * Name:        pack_pk
 *
 * Description: Serialize the public key as concatenation of the
 *              serialized vector of polynomials pk
 *              and the public seed used to generate the matrix A.
 *
 * Arguments:   uint8_t *r: pointer to the output serialized public key
 *              polyvec *pk: pointer to the input public-key polyvec
 *              const uint8_t *seed: pointer to the input public seed
 **************************************************/
void
pack_pk (uint8_t r[KEM_INDCPA_PUBLICKEYBYTES], polyvec *pk,
				 const uint8_t seed[KEM_SYMBYTES])
{
	size_t i;
	polyvec_tobytes (r, pk);
	for (i = 0; i < KEM_SYMBYTES; i++)
		r[i + KEM_POLYVECBYTES] = seed[i];
}

/*************************************************
 * Name:        unpack_pk
 *
 * Description: De-serialize public key from a byte array;
 *              approximate inverse of pack_pk
 *
 * Arguments:   - polyvec *pk: pointer to output public-key polynomial vector
 *              - uint8_t *seed: pointer to output seed to generate matrix A
 *              - const uint8_t *packedpk: pointer to input serialized public
 * key
 **************************************************/
void
unpack_pk (polyvec *pk, uint8_t seed[KEM_SYMBYTES],
					 const uint8_t packedpk[KEM_INDCPA_PUBLICKEYBYTES])
{
	size_t i;
	polyvec_frombytes (pk, packedpk);
	for (i = 0; i < KEM_SYMBYTES; i++)
		seed[i] = packedpk[i + KEM_POLYVECBYTES];
}

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
 * Name:        unpack_sk
 *
 * Description: De-serialize the secret key; inverse of pack_sk
 *
 * Arguments:   - polyvec *sk: pointer to output vector of polynomials (secret
 * key)
 *              - const uint8_t *packedsk: pointer to input serialized secret
 * key
 **************************************************/
void
unpack_sk (polyvec *sk, const uint8_t packedsk[KEM_INDCPA_SECRETKEYBYTES])
{
	polyvec_frombytes (sk, packedsk);
}

/*************************************************
 * Name:        pack_ciphertext
 *
 * Description: Serialize the ciphertext as concatenation of the
 *              compressed and serialized vector of polynomials b
 *              and the compressed and serialized polynomial v
 *
 * Arguments:   uint8_t *r: pointer to the output serialized ciphertext
 *              poly *pk: pointer to the input vector of polynomials b
 *              poly *v: pointer to the input polynomial v
 **************************************************/
void
pack_ciphertext (uint8_t r[KEM_INDCPA_BYTES], polyvec *b, poly *v)
{
	polyvec_compress (r, b);
	poly_compress (r + KEM_POLYVECCOMPRESSEDBYTES, v);
}

/*************************************************
 * Name:        unpack_ciphertext
 *
 * Description: De-serialize and decompress ciphertext from a byte array;
 *              approximate inverse of pack_ciphertext
 *
 * Arguments:   - polyvec *b: pointer to the output vector of polynomials b
 *              - poly *v: pointer to the output polynomial v
 *              - const uint8_t *c: pointer to the input serialized ciphertext
 **************************************************/
void
unpack_ciphertext (polyvec *b, poly *v, const uint8_t c[KEM_INDCPA_BYTES])
{
	polyvec_decompress (b, c);
	poly_decompress (v, c + KEM_POLYVECCOMPRESSEDBYTES);
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
// static unsigned int
// rej_uniform (uint16_t *r, unsigned int len, const uint8_t *buf,
//						 unsigned int buflen)
//{
//	unsigned int ctr, pos;
//	uint16_t val[8];
//
//	ctr = pos = 0;
//	while (ctr < len && pos + 13 <= buflen)
//		{
//			val[0]
//					= ((buf[pos + 0] & (0xff)) | ((buf[pos + 1] & 0x1f) << 8)) &
// 0x1fff; 			val[1] = ((buf[pos + 1] >> 5 & (0x07)) | ((buf[pos + 2] & 0xff)
// << 3) 								| ((buf[pos + 3] & 0x03) << 11)) 							 &
// 0x1fff; 			val[2] = ((buf[pos + 3] >>
// 2 & (0x3f)) | ((buf[pos + 4] & 0x7f) << 6)) 							 & 0x1fff;
// val[3] = ((buf[pos + 4]
//>> 7 & (0x01)) | ((buf[pos + 5] & 0xff) << 1) 								| ((buf[pos +
//6] & 0x0f) << 9)) 							 & 0x1fff; 			val[4] = ((buf[pos + 6] >> 4 & (0x0f)) |
//((buf[pos + 7] & 0xff) << 4) 								| ((buf[pos + 8] & 0x01) << 12)) 							 & 0x1fff;
//			val[5] = ((buf[pos + 8] >> 1 & (0x7f)) | ((buf[pos + 9] & 0x3f) << 7))
//							 & 0x1fff;
//			val[6] = ((buf[pos + 9] >> 6 & (0x03)) | ((buf[pos + 10] & 0xff) << 2)
//								| ((buf[pos + 11] & 0x07) << 10))
//							 & 0x1fff;
//			val[7] = ((buf[pos + 11] >> 3 & (0x1f)) | ((buf[pos + 12] & 0xff) <<
// 5)) 							 & 0x1fff;
//
//			// val[0]= (( buf[ pos + 12 ] & (0xff)) | ((buf[pos + 11] & 0x1f)<<8))
//&
//			// 0x1fff; val[1]= (( buf[ pos + 11 ]>>5 & (0x07)) | ((buf[pos + 10] &
//			// 0xff)<<3) | ((buf[pos + 9] & 0x03)<<11)) & 0x1fff; val[2]= (( buf[
// pos
//			// + 9 ]>>2 & (0x3f)) | ((buf[pos + 8] & 0x7f)<<6)) & 0x1fff; val[3]=
//((
//			// buf[ pos + 8 ]>>7 & (0x01)) | ((buf[pos + 7] & 0xff)<<1) | ((buf[pos
//+
//			// 6] & 0x0f)<<9)) & 0x1fff; val[4]= (( buf[ pos + 6 ]>>4 & (0x0f)) |
//			// ((buf[pos + 5] & 0xff)<<4) | ((buf[pos + 4] & 0x01)<<12)) & 0x1fff;
//			// val[5]= (( buf[ pos + 4]>>1 & (0x7f)) | ((buf[pos + 3] & 0x3f)<<7))
//&
//			// 0x1fff; val[6]= (( buf[ pos + 3]>>6 & (0x03)) | ((buf[pos + 2] &
//			// 0xff)<<2) | ((buf[pos + 1] & 0x07)<<10)) & 0x1fff; val[7]= (( buf[
// pos
//			// + 1]>>3 & (0x1f)) | ((buf[pos + 0] & 0xff)<<5)) & 0x1fff;
//			pos += 13;
//
//			if (val[0] < KEM_Q)
//				r[ctr++] = val[0];
//			if (ctr < len && val[1] < KEM_Q)
//				r[ctr++] = val[1];
//			if (ctr < len && val[2] < KEM_Q)
//				r[ctr++] = val[2];
//			if (ctr < len && val[3] < KEM_Q)
//				r[ctr++] = val[3];
//			if (ctr < len && val[4] < KEM_Q)
//				r[ctr++] = val[4];
//			if (ctr < len && val[5] < KEM_Q)
//				r[ctr++] = val[5];
//			if (ctr < len && val[6] < KEM_Q)
//				r[ctr++] = val[6];
//			if (ctr < len && val[7] < KEM_Q)
//				r[ctr++] = val[7];
//		}
//
//	return ctr;
// }
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

// #define gen_a(A,B)  gen_matrix(A,B,0)
// #define gen_at(A,B) gen_matrix(A,B,1)
#define gen_a(A, B) gen_matrix (A, B, 1)
#define gen_at(A, B) gen_matrix (A, B, 0)

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

// #define GEN_MATRIX_NBLOCKS ((13*KEM_N/8*(1 << 13)/KEM_Q +
// ASCON_XOF_BLOCKBYTES)/ASCON_XOF_BLOCKBYTES)
void
gen_matrix (polyvec *a, const uint8_t seed[KEM_SYMBYTES], int transposed)
{
	unsigned int ctr, i, j;
	unsigned int buflen = LOG2Q * KEM_N;
	uint8_t buf[buflen], seed1[KEM_SYMBYTES + 3];
	uint8_t *seed_counter = &(seed1[KEM_SYMBYTES + 2]);
	// ascon_state_t state;

	uint8_t buf1[buflen];

	for (i = 0; i < KEM_L; i++)
		{
			for (j = 0; j < KEM_L; j++)
				{
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

					ctr = rej_uniform (a[i].vec[j].coeffs, KEM_N, buf, buflen);

					// buflen = 32;

					while (ctr < KEM_N)
						{
							if (*seed_counter == 256)
								// rejection sampling failed
								abort ();
							// buflen = 13 * 8;

							// ascon_squeeze (&state, buf1, buflen);
							(*seed_counter)++;
							pseudoXOF ((buflen) * 8, seed1, (KEM_SYMBYTES + 3) * 8, buf1);

							for (size_t i1 = 0; i1 < buflen; i1 += 8)
								{
									for (size_t j1 = 0; j1 < 8; j1++)
										{
											buf[i1 + 7 - j1] = buf1[i1 + j1];
										}
								}

							ctr += rej_uniform (a[i].vec[j].coeffs + ctr, KEM_N - ctr, buf,
																	buflen);
						}
				}
		}
}

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
	unsigned int i;
	uint8_t buf[2 * KEM_SYMBYTES];
	const uint8_t *publicseed = buf;
	const uint8_t *noiseseed = buf + KEM_SYMBYTES;

	uint8_t noiseseed1[KEM_SYMBYTES + 1];

	polyvec a[KEM_L], e, pkpv, skpv;

	get_random_number (&drng_algorithm, buf, KEM_SYMBYTES * 8);

	// buf[0] = 0xa2; buf[1] = 0x59; buf[2] = 0x42; buf[3] = 0xa3; buf[4] = 0x85;
	// buf[5] = 0xb7; buf[6] = 0x60; buf[7] = 0xde; buf[8] = 0x58; buf[9] = 0xe2;
	// buf[10] = 0x01; buf[11] = 0x7f; buf[12] = 0x08; buf[13] = 0xc9; buf[14] =
	// 0x50; buf[15] = 0x83;

	hash_g (buf, buf, KEM_SYMBYTES);
	gen_a (a, publicseed);
	// uint8_t buf1[2*KEM_L*KEM_ETA*KEM_N/4];
	uint8_t buf1[KEM_ETA * KEM_N / 4];
	int buf1len;
	// buf1len = 2*KEM_L*KEM_ETA*KEM_N/4;
	// ascon_xof(buf1, buf1len, noiseseed, KEM_SYMBYTES);
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
	for (i = 0; i < KEM_L; i++)
		{
			pseudoXOF (buf1len * 8, noiseseed1, (KEM_SYMBYTES + 1) * 8, buf1);
			noiseseed1[KEM_SYMBYTES] += 1;
			// poly_cbd_eta(&e.vec[i],
			// buf1+((KEM_L*KEM_ETA*KEM_N/4)+(i*KEM_ETA*KEM_N/4)));
			poly_cbd_eta (&e.vec[i], buf1);
		}
	polyvec_ntt (&skpv);
	// matrix-vector multiplication
	for (i = 0; i < KEM_L; i++)
		{
			polyvec_basemul_acc_montgomery (&pkpv.vec[i], &a[i], &skpv);
			// poly_tomont (&pkpv.vec[i]);
		}
	// polyvec_invntt_tomont(&pkpv);
	polyvec_ntt (&e);
	polyvec_add (&pkpv, &pkpv, &e);
	pack_sk (sk, &skpv);
	pack_pk (pk, &pkpv, publicseed);
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
	int i;
	uint8_t seed[KEM_SYMBYTES];
	polyvec sp, pkpv, ep, at[KEM_L], b;
	poly v, k, epp;

	unpack_pk (&pkpv, seed, pk);
	// polyvec_ntt(&pkpv);
	poly_frommsg (&k, m);
	gen_at (at, seed);
	// uint8_t buf[(2*KEM_L*KEM_ETA*KEM_N/4) + (KEM_ETA*KEM_N/4)];
	// int buflen;
	// buflen = (2*KEM_L*KEM_ETA*KEM_N/4) + (KEM_ETA*KEM_N/4);
	// ascon_xof(buf, buflen, coins, KEM_SYMBYTES);

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
			// poly_cbd_eta(sp.vec+i, buf+(i*KEM_ETA*KEM_N/4));
		}
	for (i = 0; i < KEM_L; i++)
		{
			pseudoXOF (buflen * 8, coins1, (KEM_SYMBYTES + 1) * 8, buf);
			coins1[KEM_SYMBYTES] += 1;
			poly_cbd_eta (ep.vec + i, buf);
			// poly_cbd_eta(ep.vec+i,
			// buf+((KEM_L*KEM_ETA*KEM_N/4)+(i*KEM_ETA*KEM_N/4)));
		}
	pseudoXOF (buflen * 8, coins1, (KEM_SYMBYTES + 1) * 8, buf);
	poly_cbd_eta (&epp, buf);
	// poly_cbd_eta(&epp, buf+((2*KEM_L*KEM_ETA*KEM_N/4)));
	polyvec_ntt (&sp);
	// matrix-vector multiplication
	for (i = 0; i < KEM_L; i++)
		polyvec_basemul_acc_montgomery (&b.vec[i], &at[i], &sp);
	polyvec_basemul_acc_montgomery (&v, &pkpv, &sp);
	polyvec_invntt_tomont (&b);
	poly_invntt_tomont (&v);
	polyvec_add (&b, &b, &ep);
	poly_add (&v, &v, &epp);
	poly_add (&v, &v, &k);
	pack_ciphertext (c, &b, &v);
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
	polyvec b, skpv;
	poly v, mp;

	unpack_ciphertext (&b, &v, c);
	unpack_sk (&skpv, sk);
	polyvec_ntt (&b);
	polyvec_basemul_acc_montgomery (&mp, &skpv, &b);
	poly_invntt_tomont (&mp);
	poly_sub (&mp, &v, &mp);
	poly_tomsg (m, &mp);
}
