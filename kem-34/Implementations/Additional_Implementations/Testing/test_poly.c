#include "cbd.h"
#include "drng.h"
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

// The decompression mapping function
//
static int
decompress (uint32_t y)
{
	//  q / 2^d
	double factor = (double)KEM_Q / (double)(1 << LOG2T);
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
	double factor = (double)(1 << LOG2T) / (double)KEM_Q;
	factor *= (double)x;
	uint32_t comp_x = ((int)(factor + 0.5)) % (1 << LOG2T);

	return comp_x;
}

static void
unpack_poly (poly *p, uint8_t comp_p[KEM_POLYCOMPRESSEDBYTES])
{
	size_t j = 0;
	for (int i = 0; i < KEM_N;)
		{
			size_t add_i = 0;
#if LOG2T == 6
			p->coeffs[i + 0] = (comp_p[j + 0] >> 0);
			p->coeffs[i + 1] = (comp_p[j + 0] >> 6) | (comp_p[j + 1] << 2);
			p->coeffs[i + 2] = (comp_p[j + 1] >> 4) | (comp_p[j + 2] << 4);
			p->coeffs[i + 3] = (comp_p[j + 2] >> 2);
			p->coeffs[i + 4] = (comp_p[j + 3] >> 0);
			p->coeffs[i + 5] = (comp_p[j + 3] >> 6) | (comp_p[j + 4] << 2);
			p->coeffs[i + 6] = (comp_p[j + 4] >> 4) | (comp_p[j + 5] << 4);
			p->coeffs[i + 7] = (comp_p[j + 5] >> 2);
			add_i += 8;
			j += 6;
#elif LOG2T == 7
			p->coeffs[i + 0] = (comp_p[j + 0] >> 0);
			p->coeffs[i + 1] = (comp_p[j + 0] >> 7) | ((uint16_t)comp_p[j + 1] << 1);
			p->coeffs[i + 2] = (comp_p[j + 1] >> 6) | ((uint16_t)comp_p[j + 2] << 2);
			p->coeffs[i + 3] = (comp_p[j + 2] >> 5) | ((uint16_t)comp_p[j + 3] << 3);
			p->coeffs[i + 4] = (comp_p[j + 3] >> 4) | ((uint16_t)comp_p[j + 4] << 4);
			p->coeffs[i + 5] = (comp_p[j + 4] >> 3) | ((uint16_t)comp_p[j + 5] << 5);
			p->coeffs[i + 6] = (comp_p[j + 5] >> 2) | ((uint16_t)comp_p[j + 6] << 6);
			p->coeffs[i + 7] = (comp_p[j + 6] >> 1);
			add_i += 8;
			j += 7;
#elif LOG2T == 9
			p->coeffs[i + 0] = (comp_p[j + 0] >> 0) | ((uint16_t)comp_p[j + 1] << 8);
			p->coeffs[i + 1] = (comp_p[j + 1] >> 1) | ((uint16_t)comp_p[j + 2] << 7);
			p->coeffs[i + 2] = (comp_p[j + 2] >> 2) | ((uint16_t)comp_p[j + 3] << 6);
			p->coeffs[i + 3] = (comp_p[j + 3] >> 3) | ((uint16_t)comp_p[j + 4] << 5);
			p->coeffs[i + 4] = (comp_p[j + 4] >> 4) | ((uint16_t)comp_p[j + 5] << 4);
			p->coeffs[i + 5] = (comp_p[j + 5] >> 5) | ((uint16_t)comp_p[j + 6] << 3);
			p->coeffs[i + 6] = (comp_p[j + 6] >> 6) | ((uint16_t)comp_p[j + 7] << 2);
			p->coeffs[i + 7] = (comp_p[j + 7] >> 7) | ((uint16_t)comp_p[j + 8] << 1);
			add_i += 8;
			j += 9;
#endif
			for (; add_i > 0; add_i--)
				{
					p->coeffs[i] = p->coeffs[i] & LOG2T_BITMASK;
					i++;
				}
		}
}

static int
check_compression (poly *p, uint8_t comp_p[KEM_POLYCOMPRESSEDBYTES])
{
	poly unpacked_comp_p;
	unpack_poly (&unpacked_comp_p, comp_p);
	for (int i = 0; i < KEM_N; i++)
		{
			uint16_t x = p->coeffs[i];
			uint16_t compressed_x = compress (x);
			uint16_t unpacked_x = unpacked_comp_p.coeffs[i];
			if (compressed_x != unpacked_x)
				{
					printf (
							"ERROR poly compression incorrect. Original coefficient: %d, "
							"expected compressed value: %d, got compressed value: %d.\n",
							x, compressed_x, unpacked_x);
					return -1;
				}
		}
	return 0;
}

static int
check_decompression (poly *p, uint8_t comp_p[KEM_POLYCOMPRESSEDBYTES])
{
	poly unpacked_comp_p;
	unpack_poly (&unpacked_comp_p, comp_p);
	for (int i = 0; i < KEM_N; i++)
		{
			uint16_t x = p->coeffs[i];
			uint16_t unpacked_x = unpacked_comp_p.coeffs[i];
			uint16_t decompressed_x = decompress (unpacked_x);
			if (decompressed_x != x)
				{
					printf ("ERROR poly decompression incorrect. Original compressed "
									"coefficient %d: %d, expected decompressed value: %d, got "
									"decompressed value: %d.\n",
									i, unpacked_x, decompressed_x, x);
					return -1;
				}
		}
	return 0;
}

static int
test_msg ()
{
	poly p;
	uint8_t msg[KEM_INDCPA_MSGBYTES], res_msg[KEM_INDCPA_MSGBYTES];

	// Random msg
	get_random_number (&drng_algorithm, msg, KEM_INDCPA_MSGBYTES * 8);

	// Convert msg to polynomial
	poly_frommsg (&p, msg);

	// Unpack the bytes to a polynomial
	poly_tomsg (res_msg, &p);

	poly p1;
	// Unpack the bytes to a polynomial
	poly_frommsg (&p1, res_msg);

	// Check
	if (memcmp (&msg, &res_msg, KEM_INDCPA_MSGBYTES) != 0)
		{
			printf ("ERROR poly_(to|from)msg fails\n");
			return 1;
		}

	if (memcmp (&p, &p1, sizeof (poly)) != 0)
		{
			printf ("ERROR poly_(to|from)msg fails, msg -> poly -> msg -> poly "
							"doesn't match original poly\n");
			return 1;
		}

	return 0;
}

static int
test_bytes ()
{
	poly p, res_poly;
	uint8_t bytes[KEM_POLYBYTES];

	rand_poly (&p);

	// Unpack the bytes to a polynomial
	poly_tobytes (bytes, &p);

	// Re-pack the public key
	poly_frombytes (&res_poly, bytes);

	// Check
	if (memcmp (&p, &res_poly, sizeof (poly)) != 0)
		{
			printf ("ERROR poly_(to|from)bytes fails\n");
			return 1;
		}

	return 0;
}

static int
test_compress ()
{
	// Test polys
	poly p, res_poly;
	uint8_t poly_comp[KEM_POLYCOMPRESSEDBYTES];
	int ret = 0;

	rand_poly (&p);

	poly_compress (poly_comp, &p);

	ret |= check_compression (&p, poly_comp);

	poly_decompress (&res_poly, poly_comp);

	ret |= check_decompression (&res_poly, poly_comp);

	return ret;
}

int
test_cbd ()
{
	poly p;
	uint8_t buf[KEM_ETA * KEM_N / 4];

	get_random_number (&drng_algorithm, buf, (KEM_ETA * KEM_N / 4) * 8);

	poly_cbd_eta (&p, buf);

	// Test all under Q
	// for (int i = 0; i < KEM_N; i++) {
	//	if (p.coeffs[i] >= KEM_Q) {
	//		printf ("ERROR: poly_cbd_eta generates out of field coeff at i: %d,
	// coef: %d\n", i, p.coeffs[i]); 		return 1;
	//	}
	//}

	return 0;
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
			ret |= test_bytes ();
			ret |= test_msg ();
			ret |= test_cbd ();
			ret |= test_compress ();
		}

	printf ("KEM_SECRETKEYBYTES:  %d\n", KEM_SECRETKEYBYTES);
	printf ("KEM_PUBLICKEYBYTES:  %d\n", KEM_PUBLICKEYBYTES);
	printf ("KEM_CIPHERTEXTBYTES: %d\n", KEM_CIPHERTEXTBYTES);

	return ret;
}
