/*
 * YuanYang.DSA Delta2 witness driver and read-only instrumentation (sign-34-4).
 *
 * Linked into an isolated copy of a reference tree by
 * sign-34/reproduce_delta2_selection.sh, which inserts one hook at the end of
 * delta2_probability_ntt() (sign.c).  The hook only reads values; no sampling decision, random draw or output is changed (the shell
 * script checks the first signature against the prebuilt library).
 *
 * For every Delta2 evaluation the hook evaluates, in double precision and
 * independently of the code's fixed-point path, the specification's quadratic
 * form q = x^T G x with G = Sigma_delta/sigma^2 from the signer's precomputation
 * and x = (err0, err1) the candidate's error vector, and the same form q' under
 * the G' of an unrelated key (control).  It accumulates the implemented
 * acceptance probability split by sign(q) and by sign(q').
 *
 * usage: yyd2 N seed   (exit 0 when the secret-dependent selection is shown)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include "yuanyang_inner.h"
#include "sign_ntt.h"
#include "SIG_AlgorithmInstance.h"
#include "drng.h"

#define Q43 8796093022208.0          /* 2^43: fpr fraction scale */
#define Q62 4611686018427387904.0    /* 2^62: fpr_q62 scale */

DRNG_ctx drng_algorithm;

static int phase = 0;
static int64_t Gctl[YUANYANG_MAT2_SIZE];
static int have_ctl = 0;
static double last_q = 0, last_qc = 0;
static long n_eval = 0, n_neg = 0, n_negc = 0, n_big = 0, n_ratio = 0, n_clamp_agree = 0;
static double pa[2][2], s_pspec = 0, s_pcode = 0, s_ratio = 0, maxdev = 0;
static long pn[2][2];

static void negacyc(double *out, const int64_t *a, const ERR_T *x)
{
	for (int i = 0; i < YUANYANG_D; i++)
		out[i] = 0;
	for (int i = 0; i < YUANYANG_D; i++) {
		if (!a[i])
			continue;
		double ai = (double)a[i];
		for (int j = 0; j < YUANYANG_D; j++) {
			int k = i + j;
			double v = ai * (double)x[j];
			if (k >= YUANYANG_D)
				out[k - YUANYANG_D] -= v;
			else
				out[k] += v;
		}
	}
}

static double qform(const int64_t *G, const ERR_T *x0, const ERR_T *x1)
{
	static double t0[YUANYANG_D], t1[YUANYANG_D], u[YUANYANG_D];
	double q = 0;
	negacyc(t0, YUANYANG_MAT2_POLY(G, 0), x0);
	negacyc(u, YUANYANG_MAT2_POLY(G, 1), x1);
	for (int i = 0; i < YUANYANG_D; i++)
		t0[i] += u[i];
	negacyc(t1, YUANYANG_MAT2_POLY(G, 2), x0);
	negacyc(u, YUANYANG_MAT2_POLY(G, 3), x1);
	for (int i = 0; i < YUANYANG_D; i++)
		t1[i] += u[i];
	for (int i = 0; i < YUANYANG_D; i++)
		q += (double)x0[i] * t0[i] + (double)x1[i] * t1[i];
	return q / Q43;
}

static double spec_norm(const int64_t *G)
{
	double mx = 0;
	for (int j = 0; j < YUANYANG_D / 2; j++) {
		double th = M_PI * (2 * j + 1) / YUANYANG_D, Gr[4], Gi[4];
		for (int a = 0; a < 4; a++) {
			double ar = 0, ai = 0;
			const int64_t *g = YUANYANG_MAT2_POLY(G, a);
			for (int k = 0; k < YUANYANG_D; k++) {
				ar += (double)g[k] * cos(th * k);
				ai += (double)g[k] * sin(th * k);
			}
			Gr[a] = ar / Q43;
			Gi[a] = ai / Q43;
		}
		double A = Gr[0], C = Gr[3], B2 = Gr[1] * Gr[1] + Gi[1] * Gi[1];
		double r = sqrt((A - C) * (A - C) / 4 + B2);
		double e1 = fabs((A + C) / 2 + r), e2 = fabs((A + C) / 2 - r);
		if (e1 > mx) mx = e1;
		if (e2 > mx) mx = e2;
	}
	return mx;
}

void yy_instr(const yuanyang_sign_ntt_precomp *p, const ERR_T *x0, const ERR_T *x1,
	int64_t exp_raw, int64_t prob_raw, double sigsig, double invC)
{
	if (phase == 0) {
		if (!have_ctl) {
			memcpy(Gctl, p->delta_g_qD, sizeof Gctl);
			have_ctl = 1;
		}
		return;
	}
	static int once = 0;
	if (!once) {
		once = 1;
		printf("  spectral norm of G = %.4g (signing key), G' = %.4g (control key)\n",
			spec_norm(p->delta_g_qD), spec_norm(Gctl));
	}
	double q = qform(p->delta_g_qD, x0, x1), qc = qform(Gctl, x0, x1);
	double spec = q / (2.0 * sigsig * sigsig), code = (double)exp_raw / Q43;
	double pr = (double)prob_raw / Q62;
	last_q = q;
	last_qc = qc;
	n_eval++;
	n_neg += q < 0;
	n_negc += qc < 0;
	n_big += code > 1;
	n_clamp_agree += (exp_raw == 0) == (q <= 0);
	if (q > 0 && code < 1) {
		s_ratio += code / spec;
		n_ratio++;
	}
	pa[0][q < 0] += pr;
	pn[0][q < 0]++;
	pa[1][qc < 0] += pr;
	pn[1][qc < 0]++;
	if (code < 0.5) {
		double dev = fabs(pr - exp(-code) * invC);
		if (dev > maxdev)
			maxdev = dev;
	}
	s_pcode += pr;
	s_pspec += exp(spec) * invC;
}

int main(int argc, char **argv)
{
	int N = argc > 1 ? atoi(argv[1]) : 50;
	unsigned char seed[48] = { (unsigned char)(argc > 2 ? atoi(argv[2]) : 21), 0x5a };
	unsigned long long pl, sl, nl;
	unsigned char *pk = malloc(sig_get_pk_len_bytes());
	unsigned char *sk = malloc(sig_get_sk_len_bytes());
	unsigned char *sn = malloc(sig_get_sn_len_bytes());
	unsigned char m[16] = "control";
	int bad = 0, mut_acc = 0;
	long ret_neg = 0, ret_negc = 0;

	init_random_number(&drng_algorithm, seed, 48);
	/* phase 0: an unrelated key whose G' is the control form */
	if (sig_keygen(pk, &pl, sk, &sl) || sig_sign(sk, sl, m, 16, sn, &nl)) {
		puts("control key failed");
		return 2;
	}
	phase = 1;
	if (sig_keygen(pk, &pl, sk, &sl)) {
		puts("keygen failed");
		return 2;
	}
	for (int i = 0; i < N; i++) {
		memset(m, 0, sizeof m);
		snprintf((char *)m, sizeof m, "msg%d", i);
		if (sig_sign(sk, sl, m, 16, sn, &nl)) {
			puts("sign failed");
			return 2;
		}
		if (sig_verify(pk, pl, sn, nl, m, 16))
			bad++;
		if (i < 10) {
			sn[(37 * i + 50) % nl] ^= 1;
			mut_acc += sig_verify(pk, pl, sn, nl, m, 16) == 0;
			sn[(37 * i + 50) % nl] ^= 1;
			m[0] ^= 1;
			mut_acc += sig_verify(pk, pl, sn, nl, m, 16) == 0;
		}
		if (i == 0) {
			unsigned h = 2166136261u;
			for (unsigned long long k = 0; k < nl; k++)
				h = (h ^ sn[k]) * 16777619u;
			printf("FIRSTSIG fnv1a=%08x len=%llu\n", h, nl);
		}
		ret_neg += last_q < 0;
		ret_negc += last_qc < 0;
	}

	double a_pos = pa[0][0] / pn[0][0], a_neg = pa[0][1] / pn[0][1];
	double c_pos = pa[1][0] / pn[1][0], c_neg = pa[1][1] / pn[1][1];
	double cand = (double)n_neg / n_eval, candc = (double)n_negc / n_eval;
	double ret = (double)ret_neg / N, retc = (double)ret_negc / N;
	double z = (ret - cand) / sqrt(cand * (1 - cand) / N);
	double zc = (retc - candc) / sqrt(candc * (1 - candc) / N);
	printf("  signed %d, honest verify failures %d, mutated/changed-message accepted %d/20\n", N, bad, mut_acc);
	printf("  Delta2 evaluations %ld; exponent clamped to 0 exactly when q<=0: %ld/%ld; code exponent > 1 in %.1f%%",
		n_eval, n_clamp_agree, n_eval, n_eval ? 100.0 * n_big / n_eval : 0.0);
	if (n_ratio)
		printf("; mean implemented/specified exponent for q>0 = %.4g", s_ratio / n_ratio);
	printf("\n  implemented Delta2 = exp(-w)/C: max |prob - exp(-w)/C| = %.2e (w < 0.5)\n", maxdev);
	printf("  implemented P[accept | q>0] = %.4f (n=%ld)   P[accept | q<0] = %.4f (n=%ld)\n", a_pos, pn[0][0], a_neg, pn[0][1]);
	printf("  control     P[accept | q'>0] = %.4f (n=%ld)  P[accept | q'<0] = %.4f (n=%ld)\n", c_pos, pn[1][0], c_neg, pn[1][1]);
	printf("  mean acceptance: implemented %.4f, specification exp(+q/2sigma^2)/C %.4f\n", s_pcode / n_eval, s_pspec / n_eval);
	printf("  share with q<0: candidates %.3f -> returned %.3f (z=%.1f); control q'<0: %.3f -> %.3f (z=%.1f)\n",
		cand, ret, z, candc, retc, zc);
	int leak = (a_neg - a_pos) > 0.15 && z > 3.0;
	int controls = bad == 0 && mut_acc == 0 && fabs(c_neg - c_pos) < 0.08 && fabs(zc) < 3.0;
	printf("SELECTION %s controls=%s\n", leak ? "SECRET-DEPENDENT" : "NONE", controls ? "clean" : "FAILED");
	return leak && controls ? 0 : 1;
}
