/* sign-17-4: when the signer's message-sized allocation fails, the reference
 * sig_sign returns success with signature length 0.
 *
 * Built with the unmodified sources of one OPS tree; no allocator
 * interposition. The driver signs a short message (control), then a 64 MiB
 * message twice: once normally, and once after lowering RLIMIT_AS to the
 * current address-space size plus 32 MiB, so that the signer's copy of the
 * message cannot be allocated. The signature buffer is filled with 0xA5
 * beforehand to show whether anything was written.
 * Output: RESULT <alg> short=<rc>/<len>/<verify> big=<rc>/<len>/<verify>
 *         limited=<rc>/<len>/<untouched>/<verify>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>

#include "SIG_AlgorithmInstance.h"
#include "drng.h"
#include "params.h"

#ifndef OPS_LABEL
#define OPS_LABEL "OPSsig"
#endif

DRNG_ctx drng_algorithm;

#define BIG (64u << 20)

static long vm_size_bytes(void)
{
	char line[256];
	long kb = -1;
	FILE *f = fopen("/proc/self/status", "r");
	if (!f)
		return -1;
	while (fgets(line, sizeof line, f))
		if (sscanf(line, "VmSize: %ld kB", &kb) == 1)
			break;
	fclose(f);
	return kb < 0 ? -1 : kb * 1024;
}

int main(void)
{
	static unsigned char pk[CRYPTO_PUBLICKEYBYTES], sk[CRYPTO_SECRETKEYBYTES], sn[CRYPTO_BYTES];
	unsigned char seed[48], small[64];
	unsigned long long pkl = 0, skl = 0, snl;
	int rc, vr;

	for (int i = 0; i < 48; i++)
		seed[i] = (unsigned char)(5 * i + 3);
	for (int i = 0; i < 64; i++)
		small[i] = (unsigned char)i;
	init_random_number(&drng_algorithm, seed, sizeof seed);
	if (sig_keygen(pk, &pkl, sk, &skl)) {
		puts("ERROR keygen");
		return 2;
	}
	snl = 1;
	rc = sig_sign(sk, skl, small, sizeof small, sn, &snl);
	vr = sig_verify(pk, pkl, sn, snl, small, sizeof small);
	printf("short message: sign rc=%d len=%llu verify=%d\n", rc, snl, vr);
	int s_rc = rc, s_vr = vr;
	unsigned long long s_len = snl;

	unsigned char *big = malloc(BIG);
	if (!big) {
		puts("ERROR message allocation");
		return 2;
	}
	for (size_t i = 0; i < BIG; i++)
		big[i] = (unsigned char)(i * 131u + 7u);
	snl = 1;
	rc = sig_sign(sk, skl, big, BIG, sn, &snl);
	vr = sig_verify(pk, pkl, sn, snl, big, BIG);
	printf("64 MiB message: sign rc=%d len=%llu verify=%d\n", rc, snl, vr);
	int b_rc = rc, b_vr = vr;
	unsigned long long b_len = snl;

	struct rlimit old, cap;
	long vm = vm_size_bytes();
	if (vm < 0 || getrlimit(RLIMIT_AS, &old)) {
		puts("ERROR cannot read the address-space size");
		return 2;
	}
	cap = old;
	cap.rlim_cur = (rlim_t)vm + (BIG / 2);
	memset(sn, 0xA5, sizeof sn);
	snl = 1;
	fflush(stdout);
	if (setrlimit(RLIMIT_AS, &cap)) {
		puts("ERROR setrlimit");
		return 2;
	}
	rc = sig_sign(sk, skl, big, BIG, sn, &snl);
	setrlimit(RLIMIT_AS, &old);
	int untouched = 1;
	for (size_t i = 0; i < sizeof sn; i++)
		untouched &= sn[i] == 0xA5;
	vr = sig_verify(pk, pkl, sn, snl, big, BIG);
	printf("64 MiB message, address space capped at +32 MiB: sign rc=%d len=%llu buffer untouched=%d verify=%d\n",
	       rc, snl, untouched, vr);
	printf("RESULT %s short=%d/%llu/%d big=%d/%llu/%d limited=%d/%llu/%d/%d\n", OPS_LABEL,
	       s_rc, s_len, s_vr, b_rc, b_len, b_vr, rc, snl, untouched, vr);
	free(big);
	return 0;
}
