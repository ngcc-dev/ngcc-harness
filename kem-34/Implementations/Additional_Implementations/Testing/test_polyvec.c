#include "api.h"
#include "cbd.h"
#include "drng.h"
#include "indcpa.h"
#include "ntt.h"
#include "polyvec.h"
#include "symmetric.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define NTESTS 100
#define SEED_LEN_BYTES 64
DRNG_ctx drng_algorithm;

// Generate a random polynomial
static void
rand_poly (poly *res)
{
	uint16_t val;
	unsigned char rand[2];
	// Generate polynomial with coefficients mod Q
	for (int i = 0; i < KEM_N; i++)
		{
			get_random_number (&drng_algorithm, rand, 2 * 8);
			memcpy (&val, rand, 2);
			val = val % KEM_Q;
			res->coeffs[i] = val;
		}
}

// Generate a random polynomial vector
static void
rand_polyvec (polyvec *res)
{
	for (int i = 0; i < KEM_L; i++)
		{
			rand_poly (&res->vec[i]);
		}
}

#if LOG2P != LOG2Q
// The decompression mapping function
//
static int
decompress (uint32_t y)
{
	//  q / 2^d
	double factor = (double)KEM_Q / (double)(1 << LOG2P);
	factor *= (double)y;
	// Round
	uint32_t comp_y = ((int)(factor + 0.5));

	return comp_y;
}

// The compression mapping function
//
static int
compress (uint32_t x)
{
	// 2^d / q
	double factor = (double)(1 << LOG2P) / (double)KEM_Q;
	factor *= (double)x;
	uint32_t comp_x = ((int)(factor + 0.5)) % (1 << LOG2P);

	return comp_x;
}
#endif

static void
unpack_polyvec (polyvec *pv, uint8_t comp_p[KEM_POLYVECCOMPRESSEDBYTES])
{
	size_t j = 0;
	for (int p_i = 0; p_i < KEM_L; p_i++)
		{
			poly *p = &(pv->vec[p_i]);
			for (int i = 0; i < KEM_N;)
				{
					size_t add_i = 0;
#if LOG2T == 6
					p->coeffs[i + 0]
							= (comp_p[j + 0] >> 0) | ((uint16_t)comp_p[j + 1] << 8);
					p->coeffs[i + 1]
							= (comp_p[j + 1] >> 4) | ((uint16_t)comp_p[j + 2] << 4);
					add_i += 2;
					j += 3;
#elif LOG2T == 7
					p->coeffs[i + 0] = (comp_p[j + 0] & (0xff))
														 | (((uint16_t)comp_p[j + 1] & 0x1f) << 8);
					p->coeffs[i + 1] = (comp_p[j + 1] >> 5 & (0x07))
														 | (((uint16_t)comp_p[j + 2] & 0xff) << 3)
														 | (((uint16_t)comp_p[j + 3] & 0x03) << 11);
					p->coeffs[i + 2] = (comp_p[j + 3] >> 2 & (0x3f))
														 | (((uint16_t)comp_p[j + 4] & 0x7f) << 6);
					p->coeffs[i + 3] = (comp_p[j + 4] >> 7 & (0x01))
														 | (((uint16_t)comp_p[j + 5] & 0xff) << 1)
														 | (((uint16_t)comp_p[j + 6] & 0x0f) << 9);
					p->coeffs[i + 4] = (comp_p[j + 6] >> 4 & (0x0f))
														 | (((uint16_t)comp_p[j + 7] & 0xff) << 4)
														 | (((uint16_t)comp_p[j + 8] & 0x01) << 12);
					p->coeffs[i + 5] = (comp_p[j + 8] >> 1 & (0x7f))
														 | (((uint16_t)comp_p[j + 9] & 0x3f) << 7);
					p->coeffs[i + 6] = (comp_p[j + 9] >> 6 & (0x03))
														 | (((uint16_t)comp_p[j + 10] & 0xff) << 2)
														 | (((uint16_t)comp_p[j + 11] & 0x07) << 10);
					p->coeffs[i + 7] = (comp_p[j + 11] >> 3 & (0x1f))
														 | (((uint16_t)comp_p[j + 12] & 0xff) << 5);
					add_i += 8;
					j += 13;
#elif LOG2T == 9
					p->coeffs[i + 0]
							= (comp_p[j + 0] >> 0) | ((uint16_t)comp_p[j + 1] << 8);
					p->coeffs[i + 1]
							= (comp_p[j + 1] >> 3) | ((uint16_t)comp_p[j + 2] << 5);
					p->coeffs[i + 2] = (comp_p[j + 2] >> 6)
														 | ((uint16_t)comp_p[j + 3] << 2)
														 | ((uint16_t)comp_p[j + 4] << 10);
					p->coeffs[i + 3]
							= (comp_p[j + 4] >> 1) | ((uint16_t)comp_p[j + 5] << 7);
					p->coeffs[i + 4]
							= (comp_p[j + 5] >> 4) | ((uint16_t)comp_p[j + 6] << 4);
					p->coeffs[i + 5] = (comp_p[j + 6] >> 7)
														 | ((uint16_t)comp_p[j + 7] << 1)
														 | ((uint16_t)comp_p[j + 8] << 9);
					p->coeffs[i + 6]
							= (comp_p[j + 8] >> 2) | ((uint16_t)comp_p[j + 9] << 6);
					p->coeffs[i + 7]
							= (comp_p[j + 9] >> 5) | ((uint16_t)comp_p[j + 10] << 3);
					add_i += 8;
					j += 11;
#endif
					for (; add_i > 0; add_i--)
						{
							p->coeffs[i] = p->coeffs[i] & LOG2P_BITMASK;
							i++;
						}
				}
		}
}

static int
check_pv_compression (polyvec *pv, uint8_t comp_p[KEM_POLYVECCOMPRESSEDBYTES])
{
	polyvec unpacked_comp_pv;
	unpack_polyvec (&unpacked_comp_pv, comp_p);
	for (int p_i = 0; p_i < KEM_L; p_i++)
		{
			poly p = pv->vec[p_i];
			poly unpacked_comp_p = unpacked_comp_pv.vec[p_i];
			for (int i = 0; i < KEM_N; i++)
				{
					uint16_t x = p.coeffs[i];
#if LOG2P != LOG2Q
					uint16_t compressed_x = compress (x);
#else
					uint16_t compressed_x = x;
#endif
					uint16_t unpacked_x = unpacked_comp_p.coeffs[i];
					if (compressed_x != unpacked_x)
						{
							printf ("ERROR poly compression incorrect. Original "
											"coefficient: %d, expected compressed value: %d, got "
											"compressed value: %d.\n",
											x, compressed_x, unpacked_x);
							return -1;
						}
				}
		}
	return 0;
}

static int
check_pv_decompression (polyvec *pv,
												uint8_t comp_p[KEM_POLYVECCOMPRESSEDBYTES])
{
	polyvec unpacked_comp_pv;
	unpack_polyvec (&unpacked_comp_pv, comp_p);
	for (int p_i = 0; p_i < KEM_L; p_i++)
		{
			poly p = pv->vec[p_i];
			poly unpacked_comp_p = unpacked_comp_pv.vec[p_i];
			for (int i = 0; i < KEM_N; i++)
				{
					uint16_t x = p.coeffs[i];
					uint16_t unpacked_x = unpacked_comp_p.coeffs[i];
#if LOG2P != LOG2Q
					uint16_t decompressed_x = decompress (unpacked_x);
#else
					uint16_t decompressed_x = unpacked_x;
#endif
					if (decompressed_x != x)
						{
							printf ("ERROR poly decompression incorrect. Original "
											"compressed coefficient %d: %d, expected decompressed "
											"value: %d, got decompressed value: %d.\n",
											i, unpacked_x, decompressed_x, x);
							return -1;
						}
				}
		}
	return 0;
}

// static int
// test_msg ()
//{
//	poly p;
//	uint8_t msg[KEM_INDCPA_MSGBYTES], res_msg[KEM_INDCPA_MSGBYTES];
//
//	// Random msg
//	get_random_number(&drng_algorithm, msg, KEM_INDCPA_MSGBYTES * 8);
//
//	// Convert msg to polynomial
//	poly_frommsg (&p, msg);
//
//	// Unpack the bytes to a polynomial
//	poly_tomsg (res_msg, &p);
//
//	poly p1;
//	// Unpack the bytes to a polynomial
//	poly_frommsg (&p1, res_msg);
//
//	// Check
//	if (memcmp (&msg, &res_msg, KEM_INDCPA_MSGBYTES) != 0)
//		{
//			printf ("ERROR poly_(to|from)msg fails\n");
//			return 1;
//		}
//
//	if (memcmp (&p, &p1, sizeof (poly)) != 0)
//		{
//			printf ("ERROR poly_(to|from)msg fails, msg -> poly -> msg -> poly
// doesn't match original poly\n"); 			return 1;
//		}
//
//	return 0;
// }
//
// static int
// test_bytes ()
//{
//	poly p, res_poly;
//	uint8_t bytes[KEM_POLYBYTES];
//
//	rand_poly (&p);
//
//	// Unpack the bytes to a polynomial
//	poly_tobytes (bytes, &p);
//
//	// Re-pack the public key
//	poly_frombytes (&res_poly, bytes);
//
//	// Check
//	if (memcmp (&p, &res_poly, sizeof (poly)) != 0)
//		{
//			printf ("ERROR poly_(to|from)bytes fails\n");
//			return 1;
//		}
//
//	return 0;
// }

static int
test_compress ()
{
	// Test polyvec
	int ret = 0;
	polyvec pv, res_pv;
	uint8_t pv_comp[KEM_POLYVECCOMPRESSEDBYTES];

	rand_polyvec (&pv);

	polyvec_compress (pv_comp, &pv);

	ret |= check_pv_compression (&pv, pv_comp);

	polyvec_decompress (&res_pv, pv_comp);

	ret |= check_pv_decompression (&res_pv, pv_comp);

	return ret;
}

int
main (void)
{
	int ret = 0;

	// Randomness
	// For generating the seed, we assume for testing purposes
	// memory is sufficiently random.
	DRNG_ctx drng_seed;
	unsigned char seed[SEED_LEN_BYTES];

	// Initialise the "randomly", using the drng_seed memory as the random ctx.
	get_random_number (&drng_seed, seed, SEED_LEN_BYTES * 8);

	// Init drng_rand
	init_random_number (&drng_algorithm, seed, SEED_LEN_BYTES);

	// Check packing and compression
	for (int i = 0; i < NTESTS; i++)
		{
			ret |= test_compress ();
		}

	printf ("KEM_SECRETKEYBYTES:  %d\n", KEM_SECRETKEYBYTES);
	printf ("KEM_PUBLICKEYBYTES:  %d\n", KEM_PUBLICKEYBYTES);
	printf ("KEM_CIPHERTEXTBYTES: %d\n", KEM_CIPHERTEXTBYTES);

	return ret;
}
