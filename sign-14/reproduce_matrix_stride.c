/* sign-14-6: Lynx reads L0, L1 and L2 with a row stride of ceil(lambda/64)
 * words although lynx_init stores them with LYNX_MAX_WORDS = 8 words per row.
 *
 * Built with the unmodified sources of one Lynxer tree (default XOF). For
 * three OWF inputs (the public matrix seed) the driver
 *   - runs lynx_init and computes the GF(2) rank of each matrix as stored and
 *     as read through the cast that owf.c uses, uint64_t (*)[ceil(lambda/64)];
 *   - computes the kernel of L0 as read, and for up to 16 random kernel
 *     vectors d calls the exported lynx_extend_witness on a random key k and
 *     on k ^ d. The witness is (k, v1, v2) with v2 = S2(L0 k + c). If L0 as
 *     read is what the OWF uses, v2 is unchanged and v1 changes.
 * Output: RESULT lambda=<l> rank_stored=<r>,<r>,<r> rank_read=<a>,<b>,<c>
 *         kernel=<dim> v2_equal=<n>/<m> v1_differs=<n>/<m>   (one per input)
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "drng.h"
#include "instances.h"
#include "lynx_matrices.h"
#include "owf.h"

#ifndef LYNX_PARAM
#define LYNX_PARAM LYNXER_256F
#endif

DRNG_ctx drng_algorithm;

typedef uint64_t row_t[LYNX_MAX_WORDS];

/* Row-reduce in place (rows r_i: output bit i = parity(r_i & x)); returns the
 * rank and records the pivot column of each pivot row. */
static unsigned rref(row_t *m, unsigned n, unsigned words, int *pivcol)
{
	unsigned rank = 0;
	for (unsigned col = 0; col < n && rank < n; col++) {
		unsigned p = rank;
		while (p < n && !((m[p][col / 64] >> (col % 64)) & 1))
			p++;
		if (p == n)
			continue;
		row_t t;
		memcpy(t, m[p], sizeof t);
		memcpy(m[p], m[rank], sizeof t);
		memcpy(m[rank], t, sizeof t);
		for (unsigned r = 0; r < n; r++)
			if (r != rank && ((m[r][col / 64] >> (col % 64)) & 1))
				for (unsigned w = 0; w < words; w++)
					m[r][w] ^= m[rank][w];
		pivcol[rank++] = (int)col;
	}
	return rank;
}

static row_t tmp[LYNX_MAX_CSP];
static lynx_t mats;

static void as_read(const uint64_t M[LYNX_MAX_CSP][LYNX_MAX_WORDS], unsigned lambda, unsigned words)
{
	const uint64_t *flat = (const uint64_t *)M; /* the cast in owf.c */
	memset(tmp, 0, sizeof tmp);
	for (unsigned r = 0; r < lambda; r++)
		for (unsigned w = 0; w < words; w++) {
			uint64_t v = flat[r * words + w];
			unsigned rem = lambda - w * 64;
			tmp[r][w] = rem < 64 ? v & ((UINT64_C(1) << rem) - 1) : v;
		}
}

static unsigned rank_of(const uint64_t M[LYNX_MAX_CSP][LYNX_MAX_WORDS], unsigned lambda, unsigned words, int read)
{
	int piv[LYNX_MAX_CSP];
	if (read)
		as_read(M, lambda, words);
	else {
		memset(tmp, 0, sizeof tmp);
		for (unsigned r = 0; r < lambda; r++)
			memcpy(tmp[r], M[r], words * 8);
	}
	return rref(tmp, lambda, words, piv);
}

static uint64_t rng = 0x9E3779B97F4A7C15u;
static uint64_t next64(void)
{
	rng ^= rng << 13;
	rng ^= rng >> 7;
	rng ^= rng << 17;
	return rng;
}

int main(void)
{
	const sig_paramset_t *params = sig_get_paramset(LYNX_PARAM);
	const unsigned lambda = params->csp, words = (lambda + 63) / 64, nb = lambda / 8;
	uint8_t input[64], key[64], key2[64], w1[3 * 64], w2[3 * 64];

	printf("RESULT lambda=%u", lambda);
	for (unsigned s = 0; s < 3; s++) {
		for (unsigned j = 0; j < sizeof input; j++)
			input[j] = (uint8_t)(17 * j + 101 * s + lambda);
		if (lynx_init(&mats, lambda, input, params->owf_input_size)) {
			puts(" ERROR lynx_init");
			return 2;
		}
		unsigned rs[3], rr[3];
		const uint64_t(*L[3])[LYNX_MAX_WORDS] = {mats.L0, mats.L1, mats.L2};
		for (int i = 0; i < 3; i++) {
			rs[i] = rank_of(L[i], lambda, words, 0);
			rr[i] = rank_of(L[i], lambda, words, 1);
		}
		/* kernel of L0 as read: free columns of its RREF */
		int piv[LYNX_MAX_CSP], ispiv[LYNX_MAX_CSP] = {0};
		as_read(mats.L0, lambda, words);
		unsigned rank = rref(tmp, lambda, words, piv);
		for (unsigned i = 0; i < rank; i++)
			ispiv[piv[i]] = 1;
		unsigned kdim = lambda - rank, eq = 0, diff = 0, trials = kdim ? 16 : 0;
		for (unsigned t = 0; t < trials; t++) {
			uint64_t d[LYNX_MAX_WORDS] = {0};
			int nz = 0;
			for (unsigned f = 0; f < lambda; f++) {
				if (ispiv[f] || !(next64() & 1))
					continue;
				nz = 1;
				d[f / 64] ^= UINT64_C(1) << (f % 64);
				for (unsigned i = 0; i < rank; i++)
					if ((tmp[i][f / 64] >> (f % 64)) & 1)
						d[piv[i] / 64] ^= UINT64_C(1) << (piv[i] % 64);
			}
			if (!nz) {
				t--;
				continue;
			}
			for (unsigned j = 0; j < nb; j++)
				key[j] = (uint8_t)next64();
			memcpy(key2, key, nb);
			for (unsigned j = 0; j < nb; j++)
				key2[j] ^= (uint8_t)(d[j / 8] >> (8 * (j % 8)));
			lynx_extend_witness(w1, key, input, params);
			lynx_extend_witness(w2, key2, input, params);
			eq += memcmp(w1 + 2 * nb, w2 + 2 * nb, nb) == 0;
			diff += memcmp(w1 + nb, w2 + nb, nb) != 0;
		}
		printf(" | rank_stored=%u,%u,%u rank_read=%u,%u,%u kernel=%u v2_equal=%u/%u v1_differs=%u/%u",
		       rs[0], rs[1], rs[2], rr[0], rr[1], rr[2], kdim, eq, trials, diff, trials);
	}
	putchar('\n');
	return 0;
}
