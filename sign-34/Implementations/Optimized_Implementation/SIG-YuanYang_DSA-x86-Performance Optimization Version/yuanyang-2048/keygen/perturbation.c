/*
 * yuanyang-512 keygen floating perturbation decomposition.
 *
 * This is the keygen-only floating path.  It keeps Falcon's packed FFT layout,
 * but performs the precomputation arithmetic directly on double values.  The
 * values stored in yuanyang_expanded_sk are still fpr because codec/signing
 * consume that type; they are exact conversions from the rounded integer grids
 * that are serialized in the secret key.
 */

#include <limits.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "constants.h"
#include "fft.h"
#include "yuanyang_inner.h"

static int
round_to_scaled_i64(int64_t *dst, double x, unsigned bits)
{
	double y;

	y = ldexp(x, (int)bits);
	if (y < (double)LLONG_MIN || y > (double)LLONG_MAX) {
		return 0;
	}
	*dst = (int64_t)llrint(y);
	return 1;
}

static int
round_fft_poly_to_fpr_and_double(
	fpr dst_fpr[YUANYANG_D],
	double dst_double[YUANYANG_D],
	const double src_fft[YUANYANG_D],
	unsigned bits,
	int64_t min_z, int64_t max_z)
{
	double tmp[YUANYANG_D];

	yykg_poly_from_fft(tmp, src_fft);
	for (size_t u = 0; u < YUANYANG_D; u++) {
		int64_t z;

		if (!round_to_scaled_i64(&z, tmp[u], bits)
			|| z < min_z || z > max_z)
		{
			return 0;
		}
		dst_double[u] = ldexp((double)z, -(int)bits);
		dst_fpr[u] = fpr_from_scaled_i64(z, bits);
	}
	return 1;
}

static int
round_fft_poly_to_fpr(
	fpr dst_fpr[YUANYANG_D],
	const double src_fft[YUANYANG_D],
	unsigned bits,
	int64_t min_z, int64_t max_z)
{
	double tmp[YUANYANG_D];

	yykg_poly_from_fft(tmp, src_fft);
	for (size_t u = 0; u < YUANYANG_D; u++) {
		int64_t z;

		if (!round_to_scaled_i64(&z, tmp[u], bits)
			|| z < min_z || z > max_z)
		{
			return 0;
		}
		dst_fpr[u] = fpr_from_scaled_i64(z, bits);
	}
	return 1;
}

static void
sigma_p_set_slot(
	double dst[YUANYANG_MAT2_SIZE], size_t slot,
	yykg_cplx f, yykg_cplx g,
	yykg_cplx Fh, yykg_cplx Gh)
{
	double norm_b1, norm_hat_b2;
	yykg_cplx inner_sum, sigma01;

	norm_b1 = yykg_c_abs2(f) + yykg_c_abs2(g);
	norm_hat_b2 = yykg_c_abs2(Fh) + yykg_c_abs2(Gh);
	inner_sum = yykg_c_add(
		yykg_c_mul(yykg_c_conj(f), Fh),
		yykg_c_mul(yykg_c_conj(g), Gh));
	sigma01 = yykg_c_neg(inner_sum);
	yykg_fft_set(YUANYANG_MAT2_POLY(dst, YUANYANG_MAT2_00),
		slot, yykg_c_make(YYKG_SIGMA_SQ - norm_b1, 0.0));
	yykg_fft_set(YUANYANG_MAT2_POLY(dst, YUANYANG_MAT2_01),
		slot, sigma01);
	yykg_fft_set(YUANYANG_MAT2_POLY(dst, YUANYANG_MAT2_10),
		slot, yykg_c_conj(sigma01));
	yykg_fft_set(YUANYANG_MAT2_POLY(dst, YUANYANG_MAT2_11),
		slot, yykg_c_make(YYKG_SIGMA_SQ - norm_hat_b2, 0.0));
}

static yykg_cplx
c_sub_mul_conj(yykg_cplx sigma, yykg_cplx a, yykg_cplx b)
{
	return yykg_c_sub(sigma, yykg_c_mul(a, yykg_c_conj(b)));
}

int
yuanyang_expand_private_key(
	yuanyang_expanded_sk *expanded,
	const yuanyang_compact_sk *compact)
{
	double f_fft[YUANYANG_D], g_fft[YUANYANG_D];
	double F_fft[YUANYANG_D], G_fft[YUANYANG_D];
	double u_fft[YUANYANG_D], u_hat_poly[YUANYANG_D];
	double u_hat_fft[YUANYANG_D];
	double F_hat_fft[YUANYANG_D], G_hat_fft[YUANYANG_D];
	double sigma_p_fft[YUANYANG_MAT2_SIZE];
	double a_fft[YUANYANG_MAT2_SIZE];
	double A_hat_poly[YUANYANG_MAT2_SIZE];
	double A_hat_fft[YUANYANG_MAT2_SIZE];
	double sigma_delta_fft[YUANYANG_MAT2_SIZE];
	size_t hn;

	if (expanded == NULL || compact == NULL) {
		return YUANYANG_BADARG;
	}
	memset(expanded, 0, sizeof *expanded);
	expanded->compact = *compact;
	memset(a_fft, 0, sizeof a_fft);
	memset(A_hat_poly, 0, sizeof A_hat_poly);
	memset(A_hat_fft, 0, sizeof A_hat_fft);
	hn = YUANYANG_D >> 1;

	yykg_poly_to_fft_i8(f_fft, compact->f);
	yykg_poly_to_fft_i8(g_fft, compact->g);
	yykg_poly_to_fft_i8(F_fft, compact->F);
	yykg_poly_to_fft_i8(G_fft, compact->G);

	/* Compute u = -(adj(f)*F + adj(g)*G)/(adj(f)*f + adj(g)*g). */
	for (size_t u = 0; u < hn; u++) {
		yykg_cplx f, g, F, G, inner, uu;
		double norm;

		f = yykg_fft_get(f_fft, u);
		g = yykg_fft_get(g_fft, u);
		F = yykg_fft_get(F_fft, u);
		G = yykg_fft_get(G_fft, u);
		norm = yykg_c_abs2(f) + yykg_c_abs2(g);
		inner = yykg_c_add(yykg_c_mul(yykg_c_conj(f), F),
			yykg_c_mul(yykg_c_conj(g), G));
		uu = yykg_c_neg(yykg_c_div_real(inner, norm));
		yykg_fft_set(u_fft, u, uu);
	}

	if (!round_fft_poly_to_fpr_and_double(expanded->u_hat, u_hat_poly,
		u_fft, YUANYANG_U_HAT_PRECISION_BITS, INT16_MIN, INT16_MAX))
	{
		return YUANYANG_KEYGEN_FAILED;
	}

	/*
	 * The rounded u_hat is the serialized value.  Re-enter FFT form before
	 * deriving the adjusted second basis vector.
	 */
	yykg_poly_to_fft_double(u_hat_fft, u_hat_poly);
	for (size_t u = 0; u < hn; u++) {
		yykg_cplx f, g, F, G, uh, Fh, Gh;

		f = yykg_fft_get(f_fft, u);
		g = yykg_fft_get(g_fft, u);
		F = yykg_fft_get(F_fft, u);
		G = yykg_fft_get(G_fft, u);
		uh = yykg_fft_get(u_hat_fft, u);
		Fh = yykg_c_add(F, yykg_c_mul(uh, f));
		Gh = yykg_c_add(G, yykg_c_mul(uh, g));
		yykg_fft_set(F_hat_fft, u, Fh);
		yykg_fft_set(G_hat_fft, u, Gh);
		sigma_p_set_slot(sigma_p_fft, u, f, g, Fh, Gh);
	}

	/* Per-FFT-slot Cholesky square root of Sigma_p - I. */
	for (size_t u = 0; u < hn; u++) {
		yykg_cplx s00, s01, s11, l10;
		double a, l00, residual, l11;

		s00 = yykg_fft_get(
			YUANYANG_MAT2_POLY(sigma_p_fft, YUANYANG_MAT2_00), u);
		s01 = yykg_fft_get(
			YUANYANG_MAT2_POLY(sigma_p_fft, YUANYANG_MAT2_01), u);
		s11 = yykg_fft_get(
			YUANYANG_MAT2_POLY(sigma_p_fft, YUANYANG_MAT2_11), u);
		a = s00.re - 1.0;
		if (a <= 0.0) {
			return YUANYANG_KEYGEN_FAILED;
		}
		l00 = sqrt(a);
		l10 = yykg_c_div_real(yykg_c_conj(s01), l00);
		residual = s11.re - 1.0 - yykg_c_abs2(l10);
		if (residual < 0.0) {
			return YUANYANG_KEYGEN_FAILED;
		}
		l11 = sqrt(residual);
		yykg_fft_set(YUANYANG_MAT2_POLY(a_fft, YUANYANG_MAT2_00),
			u, yykg_c_make(l00, 0.0));
		yykg_fft_set(YUANYANG_MAT2_POLY(a_fft, YUANYANG_MAT2_10),
			u, l10);
		yykg_fft_set(YUANYANG_MAT2_POLY(a_fft, YUANYANG_MAT2_11),
			u, yykg_c_make(l11, 0.0));
	}

	if (!round_fft_poly_to_fpr_and_double(
			YUANYANG_MAT2_POLY(expanded->A_hat, YUANYANG_MAT2_00),
			YUANYANG_MAT2_POLY(A_hat_poly, YUANYANG_MAT2_00),
			YUANYANG_MAT2_POLY(a_fft, YUANYANG_MAT2_00),
			YUANYANG_A_HAT_PRECISION_BITS, INT32_MIN, INT32_MAX)
		|| !round_fft_poly_to_fpr_and_double(
			YUANYANG_MAT2_POLY(expanded->A_hat, YUANYANG_MAT2_10),
			YUANYANG_MAT2_POLY(A_hat_poly, YUANYANG_MAT2_10),
			YUANYANG_MAT2_POLY(a_fft, YUANYANG_MAT2_10),
			YUANYANG_A_HAT_PRECISION_BITS, INT32_MIN, INT32_MAX)
		|| !round_fft_poly_to_fpr_and_double(
			YUANYANG_MAT2_POLY(expanded->A_hat, YUANYANG_MAT2_11),
			YUANYANG_MAT2_POLY(A_hat_poly, YUANYANG_MAT2_11),
			YUANYANG_MAT2_POLY(a_fft, YUANYANG_MAT2_11),
			YUANYANG_A_HAT_PRECISION_BITS, INT32_MIN, INT32_MAX))
	{
		return YUANYANG_KEYGEN_FAILED;
	}

	for (size_t i = 0; i < YUANYANG_MAT2_POLYS; i++) {
		yykg_poly_to_fft_double(YUANYANG_MAT2_POLY(A_hat_fft, i),
			YUANYANG_MAT2_POLY(A_hat_poly, i));
	}

	/*
	 * Sigma_delta is derived from the rounded A_hat that is serialized in the
	 * expanded secret key.
	 */
	for (size_t u = 0; u < hn; u++) {
		yykg_cplx A00, A10, A11;
		yykg_cplx sigma00, sigma01, sigma10, sigma11;
		yykg_cplx delta00, delta01, delta10, delta11;

		A00 = yykg_fft_get(
			YUANYANG_MAT2_POLY(A_hat_fft, YUANYANG_MAT2_00), u);
		A10 = yykg_fft_get(
			YUANYANG_MAT2_POLY(A_hat_fft, YUANYANG_MAT2_10), u);
		A11 = yykg_fft_get(
			YUANYANG_MAT2_POLY(A_hat_fft, YUANYANG_MAT2_11), u);
		sigma00 = yykg_fft_get(
			YUANYANG_MAT2_POLY(sigma_p_fft, YUANYANG_MAT2_00), u);
		sigma01 = yykg_fft_get(
			YUANYANG_MAT2_POLY(sigma_p_fft, YUANYANG_MAT2_01), u);
		sigma10 = yykg_fft_get(
			YUANYANG_MAT2_POLY(sigma_p_fft, YUANYANG_MAT2_10), u);
		sigma11 = yykg_fft_get(
			YUANYANG_MAT2_POLY(sigma_p_fft, YUANYANG_MAT2_11), u);

		sigma00.re -= 1.0;
		sigma11.re -= 1.0;
		delta00 = c_sub_mul_conj(sigma00, A00, A00);
		delta01 = c_sub_mul_conj(sigma01, A00, A10);
		delta10 = c_sub_mul_conj(sigma10, A10, A00);
		delta11 = c_sub_mul_conj(
			c_sub_mul_conj(sigma11, A10, A10), A11, A11);
		yykg_fft_set(
			YUANYANG_MAT2_POLY(sigma_delta_fft, YUANYANG_MAT2_00),
			u, delta00);
		yykg_fft_set(
			YUANYANG_MAT2_POLY(sigma_delta_fft, YUANYANG_MAT2_01),
			u, delta01);
		yykg_fft_set(
			YUANYANG_MAT2_POLY(sigma_delta_fft, YUANYANG_MAT2_10),
			u, delta10);
		yykg_fft_set(
			YUANYANG_MAT2_POLY(sigma_delta_fft, YUANYANG_MAT2_11),
			u, delta11);
	}

	for (size_t i = 0; i < YUANYANG_MAT2_POLYS; i++) {
		if (!round_fft_poly_to_fpr(
			YUANYANG_MAT2_POLY(expanded->sigma_delta, i),
			YUANYANG_MAT2_POLY(sigma_delta_fft, i),
			YUANYANG_SIGMA_DELTA_BITS, INT32_MIN, INT32_MAX))
		{
			return YUANYANG_KEYGEN_FAILED;
		}
	}

	return YUANYANG_SUCCESS;
}
