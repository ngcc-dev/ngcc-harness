#include "api.h"
#include "cbd.h"
#include "drng.h"
#include "indcpa.h"
#include "params.h"
#include "polyvec.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define NTESTS 1
#define SEED_LEN_BYTES 64
DRNG_ctx drng_algorithm;

#define poly_cbd_eta cbd
// Generate a random polynomial
static int
rand_poly (poly *res)
{
	uint16_t val;
	unsigned char rand[2];
	// Generate polynomial with coefficients mod Q
	for (int i = 0; i < KEM_N; i++)
		{
			get_random_number (&drng_algorithm, rand, 2 * 8);
			memcpy (&val, rand, 2);
			val = val;
			res->coeffs[i] = val;
		}
}

static void
bytes_to_bits (uint8_t *out_arr, uint8_t *arr, size_t l)
{
	for (int i = 0; i < l; i++)
		{
			for (int j = 0; j < 8; j++)
				{
					out_arr[8 * i + j] = arr[i] & 1;
					arr[i] >>= 1;
				}
		}
}
static uint32_t load32_littleendian(const uint8_t x[4])
{
  uint32_t r;
  r  = (uint32_t)x[0];
  r |= (uint32_t)x[1] << 8;
  r |= (uint32_t)x[2] << 16;
  r |= (uint32_t)x[3] << 24;
  return r;
}
void basic_cbd(
    poly *r, 
    const uint8_t buf[SCABBARD_CBD_POLYBYTES])
{
  unsigned int i,j;
  uint32_t t,d;
  int16_t a,b;

  for(i=0;i<SCABBARD_N/8;i++) {
    t  = load32_littleendian(buf+4*i);
    d  = t & 0x55555555;
    d += (t>>1) & 0x55555555;

    for(j=0;j<8;j++) {
      a = (d >> (4*j+0)) & 0x3;
      b = (d >> (4*j+2)) & 0x3;
      r->coeffs[8*i+j] = a - b;
    }
  }
}

int
test_cbd ()
{
	poly p, true_poly;
	uint8_t buf[KEM_ETA * KEM_N];

	get_random_number (&drng_algorithm, buf, (KEM_ETA * KEM_N) * 8);

	poly_cbd_eta (&p, buf);

	basic_cbd (&true_poly, buf);

	if (memcmp (p.coeffs, true_poly.coeffs, KEM_N * sizeof (uint16_t)) != 0)
		{
			printf ("ERROR: test_cbd fails to generate according to spec\n");
			for (int i = 0; i < KEM_N; i++)
				{
					if (p.coeffs[i] != true_poly.coeffs[i])
						{
							printf (
									"\tcoefficient %d doesn't match: given %d, spec value %d\n",
									i, p.coeffs[i], true_poly.coeffs[i]);
						}
				}
			return 1;
		}
	return 0;
}

int
main (void)
{
	unsigned char pk[KEM_PUBLICKEYBYTES];
	unsigned long long pk_len;
	unsigned char sk[KEM_SECRETKEYBYTES];
	unsigned long long sk_len;
	unsigned char ct[KEM_CIPHERTEXTBYTES];
	unsigned long long ct_len;
	unsigned char ss[KEM_SSBYTES];
	unsigned long long ss_len;
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
			ret |= test_cbd ();
		}

	printf ("KEM_SECRETKEYBYTES:  %d\n", KEM_SECRETKEYBYTES);
	printf ("KEM_PUBLICKEYBYTES:  %d\n", KEM_PUBLICKEYBYTES);
	printf ("KEM_CIPHERTEXTBYTES: %d\n", KEM_CIPHERTEXTBYTES);

	return ret;
}
