/*
 * yuanyang-512 reference byte encodings.
 *
 *   PK = q || encode(h),
 *   SK = expanded fast-signing key
 *        (encode(h) || f || g || F || G || u_hat || A_hat_lower
 *         || Sigma_delta),
 *   Sn = salt || compressed s1. 
 *
 * Floating precomputations are encoded as scaled signed integers.  u_hat uses
 * scale 2^11 on int16, A_hat uses scale 2^22 on int32, and Sigma_delta uses
 * scale 2^22 on packed signed 22-bit two's-complement coefficients.  The
 * Cholesky factor A_hat is lower-triangular, so its A01 block is reconstructed
 * as zero instead of serialized. T is derived from Sigma_delta during signing.
 */

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <limits.h>


/*
 * Compress the public key by encoding each coefficient of h mod b.
 * The output length in bytes is ceil(ceil(n/k)*ceil(log(b^k))/8)
 * The YuanYang public key uses b=q and k=4, matching the KEM packing.
 */
#if defined(__GNUC__) || defined(__clang__)
#define ASSUME(cond) do { if (!(cond)) __builtin_unreachable(); } while (0)
typedef uint64_t v4u64 __attribute__((vector_size(32)));
#else
#define ASSUME(cond) ((void)0)
#endif


void
yuanyang_encode_uniform(
	const uint16_t *tab, unsigned int n, unsigned int b, unsigned int k, uint8_t *output)
{
	ASSUME(k<=4 && n%512==0);
	uint64_t word = 0;
	unsigned int nb = 0, nb_res_bits = 0;
	uint_fast8_t res_bits = 0;

	for(uint_fast16_t i = 0; i < n; i++) {
		word = word*b + tab[i];
		nb++;
		if (nb == k || i == n-1) {
			*output = res_bits | ((word << nb_res_bits) % 256);
			output++;
			int nb_bits = ceil(log(b)*k/log(2));
			word >>= 8-nb_res_bits;
			nb_bits -= 8-nb_res_bits;
			while(nb_bits >= 8){
				*output = word%256;
				output++;
				word >>= 8;
				nb_bits -= 8;
			}
			res_bits = word;
			nb_res_bits = nb_bits;
			nb = 0;
			word = 0;
		}
	}
}

void
yuanyang_decode_uniform(
	uint16_t *tab, unsigned int n, unsigned int b, unsigned int k, const uint8_t *input)
{
	ASSUME(k<=4);
	uint64_t word = 0;
	int nb_res_bits = 0;
	uint_fast8_t res_bits = 0, nb_bits = ceil(log(b)*k/log(2));

	for(uint_fast16_t i = 0; i < (n+k-1)/k; i++){
		word = res_bits;
		while(nb_res_bits < nb_bits){
			word |= (0ull+(*input)) << nb_res_bits;
			nb_res_bits += 8;
			input++;
		}
		res_bits = word >> nb_bits;
		nb_res_bits -= nb_bits;
		word = word % (1ull << nb_bits);
		for(int_fast8_t j = k-1; j >= 0; j--){
			if(i*k+j < n)
				tab[i*k+j] = word % b;
			word /= b;
		}
	}

}

