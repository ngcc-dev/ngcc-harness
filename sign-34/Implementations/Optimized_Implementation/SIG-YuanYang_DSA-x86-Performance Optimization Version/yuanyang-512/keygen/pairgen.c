/*
 * yuanyang-512 PairGen bring-up.
 *
 * This ports the Python prototype's Pairgen.py structure into the C reference
 * implementation.  Values are sampled directly in the imported Falcon FFT
 * representation:
 *   Re(f(w_j)) is stored in slot j, Im(f(w_j)) in slot j + d/2.
 */

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "constants.h"
#include "fft.h"
#include "yuanyang_inner.h"
#include "prng.h"


static inline void
fill_rand_doubles(prng * rng, double *buff, uint8_t *tmp_buff, size_t len)
{
	const double ptwo_m8 = 1.0/256;
	prng_get_bytes(rng, (uint8_t *) tmp_buff, len);
	for (size_t i = 0; i < len; i++)
	{
		buff[i] = ((double) tmp_buff[i]) * ptwo_m8;
	}
}

static void
sample_unifcrown(
	const double *rand_buff, double *x, double *y, size_t index)
{
	double u_rho, u_theta;
	double rho, theta;

	/*
	 * U53 sampling is kept deliberately. A full-U64 variant could work, but
	 * would change deterministic PK/SK stream, so changing it would be a future
	 * KAT/spec decision rather than a local cleanup.
	 */
	u_rho = rand_buff[index];
	u_theta = rand_buff[index + YUANYANG_D/2];
	rho = sqrt(YYKG_LOWER_RADIUS_SQ
		+ u_rho * YYKG_RADIUS_SQ_SPAN);
	theta = YYKG_HALF_PI * u_theta;
	*x = rho * cos(theta);
	*y = rho * sin(theta);
}

static int
round_coefficients(
	int8_t out[YUANYANG_D], const double values[YUANYANG_D])
{
	for (size_t u = 0; u < YUANYANG_D; u++) {
		int64_t x;

		x = llrint(values[u]);
		if (x < -127 || x > 127) {
			return 0;
		}
		out[u] = (int8_t)x;
	}
	return 1;
}

static void
load_coefficients(double values[YUANYANG_D], const int8_t in[YUANYANG_D])
{
	for (size_t u = 0; u < YUANYANG_D; u++) {
		values[u] = (double)in[u];
	}
}

static int
eval_at_one_is_even_mod2(const int8_t src[YUANYANG_D])
{
	unsigned parity;

	parity = 0;
	for (size_t u = 0; u < YUANYANG_D; u++) {
		parity ^= (unsigned)src[u] & 1u;
	}
	return parity == 0;
}

int
yuanyang_pairgen(
	int8_t f[YUANYANG_D],
	int8_t g[YUANYANG_D], prng *rng)
{
	double f_fft[YUANYANG_D], g_fft[YUANYANG_D];
	size_t hn;
	double rand_buff[4*YUANYANG_D/2] = {0.0};
	uint8_t tmp_buff[4*YUANYANG_D/2] = {0};

	if (f == 0 || g == 0 || rng == 0) {
		return YUANYANG_BADARG;
	}
	hn = YUANYANG_D >> 1;
	for (;;) {
		int ok = 1;

		/*
		 * For PairGen we need 4 random element for each coordinate:
		 *    1. u_rho in UnifCrown
		 *    2. u_theta in UnifCrown
		 *    3. theta_x in PairGen
		 *    4. theta_y in PairGen
		 *
		 * Since each coordinate has ~2bits of entropy, we can randomly draw 8
		 * bits foreach element and still have a confortable margin. Each byte
		 * is then converted to double with the appropriate scaling.
		 *
		 * We batch-generate the entire buffer for the trial, and pick in the
		 * buffer at YUANYANG_D/2 stride for each element.
		*/
		fill_rand_doubles(rng, rand_buff, tmp_buff, 4*YUANYANG_D/2);

		for (size_t u = 0; u < hn; u++) {
			double theta_x, theta_y;
			double x, y, tx, ty;

			sample_unifcrown(rand_buff, &x, &y, u);

			theta_x = rand_buff[u + 2*YUANYANG_D/2];
			theta_y = rand_buff[u + 3*YUANYANG_D/2];
			tx = YYKG_TWO_PI * theta_x;
			ty = YYKG_TWO_PI * theta_y;
			f_fft[u] = x * cos(tx);
			f_fft[u + hn] = x * sin(tx);
			g_fft[u] = y * cos(ty);
			g_fft[u + hn] = y * sin(ty);
		}

		yykg_ifft(f_fft, YUANYANG_LOGD);
		yykg_ifft(g_fft, YUANYANG_LOGD);

		/*
		 * We include a sanity check on arithmetic overflow of coefficients:
		 * they must fit in 1 byte each. Experimentally, we never had f and g
		 * not fit in one byte here, so there is a negligible probability to
		 * restart here.
		 */
		if (!round_coefficients(f, f_fft)
			|| !round_coefficients(g, g_fft))
		{
			continue;
		}

		/*
		 * If f(1), we fail ntrusolve because it expects
		 * N(f) and N(g) at the bottom of the recursion to get f and g coprime.
		 * By doing this early abort check, we save some computations that would
		 * lead to a fail later in ntrusolve.
		 */
		if (eval_at_one_is_even_mod2(f)) {
			continue;
		}

		load_coefficients(f_fft, f);
		load_coefficients(g_fft, g);
		yykg_fft(f_fft, YUANYANG_LOGD);
		yykg_fft(g_fft, YUANYANG_LOGD);

		uint32_t norm_sq = 0;
		for (size_t u = 0; u < hn; u++) {
			double norm_sq_fft;

			norm_sq_fft =
				f_fft[u] * f_fft[u]
				+ f_fft[u + hn] * f_fft[u + hn]
				+ g_fft[u] * g_fft[u]
				+ g_fft[u + hn] * g_fft[u + hn];
			if (norm_sq_fft < YYKG_PAIRGEN_NORM_MIN
				|| YYKG_PAIRGEN_NORM_MAX < norm_sq_fft)
			{
				ok = 0;
				break;
			}
			norm_sq += f[u]*f[u] + g[u]*g[u] + f[u+hn]*f[u+hn] + g[u+hn]*g[u+hn];
		}
		if (ok && norm_sq < YYKG_NORM_FG_MIN) {
			ok = 0;
		}
		if (ok) {
			return YUANYANG_SUCCESS;
		}
	}
}
