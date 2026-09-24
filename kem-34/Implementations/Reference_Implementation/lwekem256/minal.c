#include "cbd.h"
#include "minal.h"
#include "ntt.h"
#include "params.h"
#include "poly.h"
#include "reduce.h"
#include "symmetric.h"
#include <stdint.h>

#define B2_MINAL_Q KEM_Q
#define B2_MINAL_HALF_Q (B2_MINAL_Q / 2 + 1)

#define MINAL_Q (B2_MINAL_Q / 2)
#define MINAL_ALPHA (MINAL_Q / 2)
#define MINAL_BETA 220
#define MINAL_DIM 2

#define N_CODE_VALS 4

static int16_t CODE_VALS[4] = {0, MINAL_BETA, MINAL_ALPHA, MINAL_BETA + MINAL_ALPHA - MINAL_Q};

static int8_t CODEWORDS[4][2] = {
   {0, 0},
   {1, 2},
   {2, 1},
   {3, 3},
};

uint32_t
get_distance_sqr_to_codeword (uint16_t idx, uint32_t distsqr_matrix[][N_CODE_VALS])
{

	uint64_t dist_sqr = 0;
	for (int i = 0; i < MINAL_DIM; i++)
		{
			dist_sqr += distsqr_matrix[i][CODEWORDS[idx][i]];
		}

	// Returns `distance_sqr | codeword_index`
	return (dist_sqr) << 8 | idx;
}



// Returns 0xffffffff if (v1 < v2) and 0x00000000 otherwise
static __inline__ uint32_t
lower_than_mask (const uint32_t v1, const uint32_t v2)
{
	return -((v1 - v2) >> 31);
}

// Returns `abs(centered_mod(value, KYBER_Q)` assuming `-KYBER_Q <= value <=
// KYBER_Q`
static __inline__ int32_t
abs_center_mod_of_2q_centered_value (int32_t value, int q)
{
	// value = abs(value):
	uint32_t mask_sign = value >> 31;
	value ^= mask_sign;
	value += mask_sign & 1;
	value -= q & lower_than_mask (q / 2, value);
	return value;
}

// Computes min(v1, v2) in constant time
static __inline__ uint32_t
secure_min (int32_t v1, int32_t v2)
{
	uint32_t mask_min_v1 = lower_than_mask (v1, v2);
	return (mask_min_v1 & v1) | (~mask_min_v1 & v2);
}

static inline int16_t
centered_mod_i16 (int16_t a, int16_t q)
{
	// Assumes a is in [0, q-1]

	uint32_t mask = lower_than_mask(q/2, a);
	a = (mask & (a - q)) | (~mask & a);

	return a;
}

void
minal_b2_code_encode (int16_t codeword[], uint8_t msg_bits[])
{
	uint8_t external_bits[2] = { msg_bits[0], msg_bits[1] };
	uint8_t internal_bits[2] = { msg_bits[2], msg_bits[3] };

	codeword[0] = internal_bits[0] * MINAL_ALPHA + internal_bits[1] * MINAL_BETA;
	codeword[1] = internal_bits[1] * MINAL_ALPHA + internal_bits[0] * MINAL_BETA;

	codeword[0] += B2_MINAL_HALF_Q * external_bits[0];
	codeword[1] += B2_MINAL_HALF_Q * external_bits[1];
}



static __inline__ uint16_t
internal_minal_code_decode (int16_t target[])
{
	// Build matrix with square distances to target coordinates
	uint32_t distsqr_matrix[2][N_CODE_VALS];
	for (int i = 0; i < MINAL_DIM; i++)
		{
			for (int j = 0; j < N_CODE_VALS; j++)
				{
					distsqr_matrix[i][j] = abs_center_mod_of_2q_centered_value (
							target[i] - CODE_VALS[j], MINAL_Q);
					distsqr_matrix[i][j] *= distsqr_matrix[i][j];
				}
		}

	uint32_t min_dist_codeword = get_distance_sqr_to_codeword (0, distsqr_matrix);
	for (int i = 1; i < (1 << 2); i++)
		{
			min_dist_codeword = secure_min (
					get_distance_sqr_to_codeword (i, distsqr_matrix),
					min_dist_codeword);
		}
	// Retrieves `codeword_index` from `distance_sqr | codeword_index`

	// return min_dist_codeword & 0xFF;
	return min_dist_codeword & 0xFF;
}

uint16_t
minal_b2_code_decode (int16_t target[])
{
	// This function assumes that:
	//    * 0 <= target[i] < q/2

	int16_t internal_target[2] = { target[0] - B2_MINAL_HALF_Q,
											 					 target[1] - B2_MINAL_HALF_Q };
	int16_t internal_decode
			= internal_minal_code_decode (internal_target);
	uint8_t internal_decode_bits[2]
			= { (internal_decode >> 1) & 1, internal_decode & 1 };
	int16_t internal_cw[2] = { 0, 0 };

	internal_cw[0] = internal_decode_bits[0] * MINAL_ALPHA + internal_decode_bits[1] * MINAL_BETA;
	internal_cw[1] = internal_decode_bits[1] * MINAL_ALPHA + internal_decode_bits[0] * MINAL_BETA;

	int16_t dx = centered_mod_i16 (target[0] - internal_cw[0], B2_MINAL_Q);
	int16_t dy = centered_mod_i16 (target[1] - internal_cw[1], B2_MINAL_Q);

	uint16_t decoded = internal_decode;
	decoded |= (abs (dx) > B2_MINAL_HALF_Q / 2) << 3;
	decoded |= (abs (dy) > B2_MINAL_HALF_Q / 2) << 2;

	return decoded;
}

// static __inline__ int decode_msg_pair(int32_t x, int32_t y) {
//   uint32_t reflect_mask = lower_than_mask(x, y);

//   int32_t x_prime = (x & ~reflect_mask) | (y & reflect_mask);
//   int32_t y_prime = (y & ~reflect_mask) | (x & reflect_mask);

//   uint8_t above_l1 = ABOVE_L1(x_prime, y_prime);
//   uint8_t above_l2 = ABOVE_L2(x_prime, y_prime);
//   uint8_t above_l3 = ABOVE_L3(x_prime, y_prime);
//   uint8_t above_l4 = ABOVE_L4(x_prime, y_prime);
//   uint8_t above_l5 = ABOVE_L5(x_prime, y_prime);

//   // Not needed, but conceptually: uint8_t c00 = (~above_l3 & above_l2 &
//   above_l4); uint8_t c01 = ~above_l2 & ~above_l5 & ~above_l3; uint8_t c10 =
//   above_l2 & above_l3 & ~above_l1; uint8_t c11 = above_l1 | (above_l3 &
//   ~above_l2) | (above_l5 & ~above_l4);

//   c01 &= (1 ^ reflect_mask);
//   c10 &= (2 ^ reflect_mask);

//   uint8_t bits = c01 | c10 | c11;

//   return bits & 3;
// }
