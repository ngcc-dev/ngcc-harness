/*
 * YuanYang-512 keygen SecurityLoss check.
 *
 * This file intentionally uses native double arithmetic.  It implements the
 * basis-specific loss check from the specification through the keygen-local
 * double FFT path, without sharing the production fpr/fixed-point FFT path used
 * by signing.
 */

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#if defined(YUANYANG_SECURITY_LOSS_TRACE)
#include <stdio.h>
#endif

#include "constants.h"
#include "yuanyang_inner.h"
#include "fft.h"
#include "security_loss.h"

#define YUANYANG_SL_VECTOR_LEN       (2u * YUANYANG_D)

#define YUANYANG_SL_LOSS_BOUND        128.0
#define YUANYANG_SL_LAMBDA            513.0
#define YUANYANG_SL_SIGMA_SIG         YYKG_SIGMA_SIG
#define YUANYANG_SL_ALPHA             YYKG_QUALITY_ALPHA
#define YUANYANG_SL_LOG2              0.693147180559945309417232121458176568
#define YUANYANG_SL_PI                3.141592653589793238462643383279502884

/*
 * Compute G_0 = ff^* + gg^* with FFT.
 */
static int
g0_from_pair_fft(double g0_fft[YUANYANG_D],
	const int8_t f[YUANYANG_D],
	const int8_t g[YUANYANG_D])
{
	double f_fft[YUANYANG_D];
	double g_fft[YUANYANG_D];
	size_t hn;

	yykg_poly_to_fft_i8(f_fft, f);
	yykg_poly_to_fft_i8(g_fft, g);

	/*
	 * The specification writes G0 = ff^* + gg^*.  In the FFT embedding,
	 * the adjoint is complex conjugation.  In Falcon's packed layout this means
	 * each stored complex slot becomes
	 *   f_re^2 + f_im^2 + g_re^2 + g_im^2,
	 * with zero imaginary part.
	 */
	hn = YUANYANG_D >> 1;
	for (size_t u = 0; u < hn; u++) {
		double z;

		z = f_fft[u] * f_fft[u]
			+ f_fft[u + hn] * f_fft[u + hn]
			+ g_fft[u] * g_fft[u]
			+ g_fft[u + hn] * g_fft[u + hn];
		g0_fft[u] = z;
		g0_fft[u + hn] = 0.0;
	}
	return 1;
}

static void
ldl_d11_fft(double d11[YUANYANG_D],
	const double g0[YUANYANG_D],
	const double g1[YUANYANG_D],
	unsigned logn)
{
	size_t hn;

	hn = (size_t)1u << (logn - 1u);
	for (size_t u = 0; u < hn; u++) {
		double g0_re, g0_im, g1_re, g1_im;
		double den, inv_re, inv_im, mu_re, mu_im, xi_re, xi_im;

		g0_re = g0[u];
		g0_im = g0[u + hn];
		g1_re = g1[u];
		g1_im = g1[u + hn];

		/*
		 * This is the scalar form of Falcon poly_LDLmv_fft().
		 *
		 * In Falcon's packed FFT convention, the implicit conjugate half of an
		 * auto-adjoint polynomial changes the signs used by poly_LDLmv_fft().
		 * The variable named mu below uses Falcon's stored sign convention: its
		 * imaginary component is the negative of the ordinary complex quotient
		 * g1/g0.  With that convention, Falcon computes the Schur complement as
		 * the real update below and stores the imaginary part as
		 *   d11_im = g0_im + Im(mu*g1).
		 * This is algebraically the same LDL step as
		 * d11 = g0 - (g1/g0)*adj(g1), but written in the packed layout.
		 */
		den = g0_re * g0_re + g0_im * g0_im;
		inv_re = g0_re / den;
		inv_im = g0_im / den;
		mu_re = g1_re * inv_re + g1_im * inv_im;
		mu_im = g1_re * inv_im - g1_im * inv_re;
		xi_re = mu_re * g1_re - mu_im * g1_im;
		xi_im = mu_im * g1_re + mu_re * g1_im;
		d11[u] = g0_re - xi_re;
		d11[u + hn] = g0_im + xi_im;
	}
}

static int
ffldl_inner_leaves(double *ell,
	const double g0[YUANYANG_D],
	const double g1[YUANYANG_D],
	unsigned logn)
{
	double d11[YUANYANG_D];
	double left0[YUANYANG_D];
	double left1[YUANYANG_D];
	double right0[YUANYANG_D];
	double right1[YUANYANG_D];
	size_t n, hn;

	/*
	 * This is Falcon ffLDL_inner(), specialized to output scalar leaves instead
	 * of the sampler tree.  The inputs are the first row of the current
	 * auto-adjoint quasicyclic 2x2 matrix in FFT representation.
	 */
	if (logn == 0u) {
		ell[0] = g0[0];
		return 1;
	}

	n = (size_t)1u << logn;
	hn = n >> 1;
	ldl_d11_fft(d11, g0, g1, logn);

	/*
	 * As in Falcon, split d00 (= g0) and d11.  Each split gives the first row
	 * of one child matrix.  Recursing on both children of this retained root
	 * subtree yields ell_0..ell_{d-1}.
	 */
	yykg_poly_split_fft(left0, left1, g0, logn);
	yykg_poly_split_fft(right0, right1, d11, logn);

	return ffldl_inner_leaves(ell, left0, left1, logn - 1u)
		&& ffldl_inner_leaves(ell + hn, right0, right1, logn - 1u);
}

static int
ffldl_root_left_leaves(double ell[YUANYANG_D],
	const double g0_fft[YUANYANG_D])
{
	double g0[YUANYANG_D];
	double g1[YUANYANG_D];
	double q2;
	size_t hn;
	int ok;

	/*
	 * Falcon's symplectic tree optimization keeps the left root subtree and
	 * obtains the other half by symplecticity.  For the SecurityLoss profile,
	 * G0 = ff^* + gg^* is the d00 polynomial at the ffLDL root; the retained
	 * subtree starts with SplitFFT(G0).
	 *
	 * This mirrors Falcon ffLDL_fft_root(): we do not need the root L
	 * coefficient or the right root subtree, so the only root operation kept is
	 * the split of d00 followed by ffLDL_inner on that left subtree.
	 */
	yykg_poly_split_fft(g0, g1, g0_fft, YUANYANG_LOGD);
	ok = ffldl_inner_leaves(ell, g0, g1, YUANYANG_LOGD - 1u);
	if (!ok) {
		return 0;
	}

	/*
	 * ffLDL_inner(logd-1) produced d/2 scalar leaves.  The fixed-point Falcon
	 * code reconstructs the sibling root subtree by symplecticity; for squared
	 * GSO lengths the mirror relation is simply ell' = q^2/ell.
	 */
	q2 = (double)YUANYANG_Q * (double)YUANYANG_Q;
	hn = YUANYANG_D >> 1;
	for (size_t u = 0; u < hn; u++) {
		ell[hn + u] = q2 / ell[u];
	}
	return 1;
}

static int
complete_loss_vector(double ellhat[YUANYANG_SL_VECTOR_LEN],
	const double ell[YUANYANG_D])
{
	double q2;

	q2 = (double)YUANYANG_Q * (double)YUANYANG_Q;
	for (size_t u = 0; u < YUANYANG_D; u++) {
		ellhat[u] = ell[u];
		ellhat[YUANYANG_D + u] = q2 / ell[u];
	}
	return 1;
}

static int
security_loss_from_vector(double *loss, const double ellhat[YUANYANG_SL_VECTOR_LEN])
{
	double ell_max, epsilon_bar, log_base, t;
	double eps_prime, k, a, x, term;
	double c_s;

	/*
	 * Updated SecurityLoss/ProfileLoss:
	 *
	 *   epsilon_bar is computed from the current GSO profile:
	 *     sigma_sig = max_i sqrt(ell_i)
	 *       * sqrt(log(4*d*(1 + 1/epsilon_bar))/2) / pi
	 *
	 *   gamma_i = 2*max_j ell_j/ell_i
	 *   T = sum_i 2*(epsilon_bar/(4*d*(1+epsilon_bar)))^gamma_i
	 *   epsilon' = T/(1-T)
	 *   loss = min_{a>1} lambda/a + C_s*log2(1 + a*epsilon'/2)
	 */
	ell_max = ellhat[0];
	for (size_t u = 1; u < YUANYANG_SL_VECTOR_LEN; u++) {
		if (ellhat[u] > ell_max) {
			ell_max = ellhat[u];
		}
	}

	{
		double den, xexp;

		xexp = exp(2.0 * YUANYANG_SL_SIGMA_SIG * YUANYANG_SL_SIGMA_SIG
			* YUANYANG_SL_PI * YUANYANG_SL_PI / ell_max);
		den = xexp / (4.0 * (double)YUANYANG_D) - 1.0;
		epsilon_bar = 1.0 / den;
	}
	log_base = log(epsilon_bar
		/ (4.0 * (double)YUANYANG_D * (1.0 + epsilon_bar)));

	t = 0.0;
	for (size_t u = 0; u < YUANYANG_SL_VECTOR_LEN; u++) {
		double gamma_i;

		gamma_i = (2.0 * ell_max) / ellhat[u];
		t += 2.0 * exp(gamma_i * log_base);
	}
	if (t >= 1.0) {
		return 0;
	}

	eps_prime = t / (1.0 - t);
	k = 0.5 * eps_prime;
	if (k == 0.0) {
		*loss = 0.0;
		return 1;
	}

	c_s = ldexp(1.0, 80) + ldexp(1.0, 77);
	term = c_s * k / YUANYANG_SL_LOG2;

	/*
	 * Closed-form minimizer of lambda/a + C_s*log2(1 + a*k), with
	 * k = epsilon'/2.  If the stationary point lies below the admissible range
	 * a>1, the infimum is reached at the boundary a=1.
	 */
	a = (YUANYANG_SL_LAMBDA * k
		+ sqrt(YUANYANG_SL_LAMBDA * YUANYANG_SL_LAMBDA * k * k
			+ 4.0 * YUANYANG_SL_LAMBDA * term))
		/ (2.0 * term);
	if (a < 1.0) {
		a = 1.0;
	}

	x = a * k;
	*loss = YUANYANG_SL_LAMBDA / a
		+ c_s * (log1p(x) / YUANYANG_SL_LOG2);
#if defined(YUANYANG_SECURITY_LOSS_TRACE)
	fprintf(stderr,
		"SLTRACE ellmax=%.17g epsbar=%.17g logepsbar=%.17g t=%.17g epsp=%.17g a=%.17g loss=%.17g\n",
		ell_max, epsilon_bar, -log(epsilon_bar) / YUANYANG_SL_LOG2,
		t, eps_prime, a, *loss);
#endif
	return 1;
}

int
yuanyang_security_loss_accept(
	const int8_t f[YUANYANG_D],
	const int8_t g[YUANYANG_D])
{
	double g0_fft[YUANYANG_D];
	double ell[YUANYANG_D];
	double ellhat[YUANYANG_SL_VECTOR_LEN];
	double loss;


	if (!g0_from_pair_fft(g0_fft, f, g)
		|| !ffldl_root_left_leaves(ell, g0_fft)
		|| !complete_loss_vector(ellhat, ell)
		|| !security_loss_from_vector(&loss, ellhat))
	{
		return 0;
	}

	return loss <= YUANYANG_SL_LOSS_BOUND;
}
