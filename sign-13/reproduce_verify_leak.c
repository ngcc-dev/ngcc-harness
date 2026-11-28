/* sign-13-6: GreatWall verification leaks the commitment tree when a
 * signature sets one of the challenge bits that must be zero.
 *
 * Built with the unmodified reference sources of one parameter set. The
 * driver signs a message, then calls sig_verify N times (argument, default 3)
 * on each of five byte strings of signature length and reports the heap
 * growth per call measured with mallinfo2():
 *   valid    the honest signature (accepted)
 *   flip0    first byte flipped (rejected later in verification)
 *   grind    top bit of the last challenge (delta) byte flipped; with
 *            grinding bits this sets a bit that must be zero
 *   ones     all 0xFF bytes, no valid signature at all
 *   zeros    all zero bytes
 * The delta offset is computed with the same macros as faest_verify
 * (faest.c:548-553). Output: one line per case and one summary line
 *   RESULT <set> zero_bits=<Z> tree_bytes=<T> grind=<g> ones=<o> others=<x>
 * with the growth in bytes per call.
 */
#define _GNU_SOURCE
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SIG_AlgorithmInstance.h"
#include "api.h"
#include "drng.h"
#include "faest.h"
#include "owf_proof.h"
#include "quicksilver.h"
#include "vector_com.h"
#include "vole_check.h"
#include "vole_commit.h"
#include "vole_params.h"

DRNG_ctx drng_algorithm;


static size_t heap_in_use(void)
{
	struct mallinfo2 mi = mallinfo2();
	return mi.uordblks + mi.hblkhd;
}

int main(int argc, char **argv)
{
	const int n_calls = argc > 1 ? atoi(argv[1]) : 3;
	static unsigned char pk[CRYPTO_PUBLICKEYBYTES], sk[CRYPTO_SECRETKEYBYTES];
	static unsigned char sn[CRYPTO_BYTES], probe[CRYPTO_BYTES];
	unsigned char seed[48], msg[] = "sign-13-6 verify leak";
	unsigned long long pkl = 0, skl = 0, snl = 0;
	const size_t delta_off = VOLE_COMMIT_SIZE + VOLE_CHECK_PROOF_BYTES + WITNESS_BITS / 8 +
	                         QUICKSILVER_PROOF_BYTES + VECTOR_COM_OPEN_SIZE;
	const size_t delta_last = delta_off + SECURITY_PARAM / 8 - 1;
	const char *names[] = {"valid", "flip0", "grind", "ones", "zeros"};
	double growth[5];

	for (int i = 0; i < 48; i++)
		seed[i] = (unsigned char)(3 * i + 1);
	init_random_number(&drng_algorithm, seed, sizeof seed);
	if (sig_keygen(pk, &pkl, sk, &skl) || sig_sign(sk, skl, msg, sizeof msg, sn, &snl) ||
	    snl != CRYPTO_BYTES || sig_verify(pk, pkl, sn, snl, msg, sizeof msg) != 0) {
		puts("ERROR honest keygen/sign/verify failed");
		return 2;
	}
	for (int c = 0; c < 5; c++) {
		memcpy(probe, sn, sizeof probe);
		if (c == 1)
			probe[0] ^= 1;
		if (c == 2)
			probe[delta_last] ^= 0x80; /* forced to zero in honest signatures when there are grinding bits */
		if (c == 3)
			memset(probe, 0xFF, sizeof probe);
		if (c == 4)
			memset(probe, 0x00, sizeof probe);
		int rc = sig_verify(pk, pkl, probe, snl, msg, sizeof msg); /* warm-up call */
		size_t before = heap_in_use();
		for (int i = 0; i < n_calls; i++)
			rc = sig_verify(pk, pkl, probe, snl, msg, sizeof msg);
		growth[c] = (double)(heap_in_use() - before) / n_calls;
		printf("%-6s verify rc=%d heap growth per call %.0f bytes\n", names[c], rc, growth[c]);
		if ((c == 0) != (rc == 0)) {
			puts("ERROR unexpected verification result");
			return 2;
		}
	}
	printf("RESULT %s zero_bits=%d tree_bytes=%zu grind=%.0f ones=%.0f others=%.0f\n",
	       CRYPTO_ALGNAME, (int)ZERO_BITS_IN_CHALLENGE_3,
	       (size_t)BATCH_VECTOR_COMMIT_NODES * sizeof(block_secpar), growth[2], growth[3],
	       growth[0] + growth[1] + growth[4]);
	return 0;
}
