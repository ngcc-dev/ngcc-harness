/* kem-04-5: kem_dec ignores the declared ciphertext length and reads
 * CCAKEM_CT_SIZE bytes from the caller's buffer.
 *
 * Built with the unmodified sources of one BAG-Piglet reference tree. An
 * honest ciphertext is copied so that its declared bytes end exactly at a
 * page boundary; the following page is made PROT_NONE. Each case runs in a
 * child process so that a fault can be reported:
 *   full    all CCAKEM_CT_SIZE bytes, declared at full length (control)
 *   minus1  the first CCAKEM_CT_SIZE-1 bytes, declared at that length
 *   one     the first byte only, declared as one byte
 * Output: RESULT <set> ct=<bytes> full=<status> minus1=<status> one=<status>
 * where <status> is "ok" (returned, shared secret correct for full),
 * "sigN" (killed by signal N) or "rcN".
 */
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

#include "KEM_AlgorithmInstance.h"
#include "drng.h"

#ifndef PIGLET_LABEL
#define PIGLET_LABEL "BAG-Piglet"
#endif

DRNG_ctx drng_algorithm;

static int run_case(unsigned char *sk, unsigned long long skl, const unsigned char *ct,
                    unsigned long long ctl, unsigned long long declared, const unsigned char *ss_ref,
                    unsigned long long ssl, char *status, size_t status_len)
{
	long pg = sysconf(_SC_PAGESIZE);
	size_t span = ((declared + pg - 1) / pg) * pg;
	fflush(stdout);
	pid_t pid = fork();
	if (pid < 0)
		return -1;
	if (pid == 0) {
		unsigned char *map = mmap(NULL, span + pg, PROT_READ | PROT_WRITE,
		                          MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		if (map == MAP_FAILED)
			_exit(4);
		unsigned char *p = map + span - declared; /* declared bytes end at the page boundary */
		memcpy(p, ct, declared);
		if (mprotect(map + span, pg, PROT_NONE))
			_exit(5);
		unsigned char ss[64];
		unsigned long long got = 0;
		int rc = kem_dec(sk, skl, p, declared, ss, &got);
		if (rc != 0)
			_exit(10 + (rc & 0x7f));
		if (declared == ctl && (got != ssl || memcmp(ss, ss_ref, ssl)))
			_exit(6);
		_exit(0);
	}
	int st = 0;
	waitpid(pid, &st, 0);
	if (WIFSIGNALED(st))
		snprintf(status, status_len, "sig%d", WTERMSIG(st));
	else if (WEXITSTATUS(st) == 0)
		snprintf(status, status_len, "ok");
	else
		snprintf(status, status_len, "rc%d", WEXITSTATUS(st));
	return 0;
}

int main(void)
{
	unsigned long long pkl = kem_get_pk_len_bytes(), skl = kem_get_sk_len_bytes();
	unsigned long long ctl = kem_get_ct_len_bytes(), ssl = kem_get_ss_len_bytes(), a, b;
	unsigned char *pk = malloc(pkl), *sk = malloc(skl), *ct = malloc(ctl), ss[64], seed[48];
	char st_full[16], st_m1[16], st_one[16];

	if (!pk || !sk || !ct || ssl > sizeof ss)
		return 2;
	for (int i = 0; i < 48; i++)
		seed[i] = (unsigned char)(7 * i + 2);
	init_random_number(&drng_algorithm, seed, sizeof seed);
	if (kem_keygen(pk, &a, sk, &b) || kem_enc(pk, pkl, ss, &a, ct, &b) || b != ctl)
		return 3;
	if (run_case(sk, skl, ct, ctl, ctl, ss, ssl, st_full, sizeof st_full) ||
	    run_case(sk, skl, ct, ctl, ctl - 1, ss, ssl, st_m1, sizeof st_m1) ||
	    run_case(sk, skl, ct, ctl, 1, ss, ssl, st_one, sizeof st_one))
		return 4;
	printf("RESULT %s ct=%llu full=%s minus1=%s one=%s\n", PIGLET_LABEL, ctl, st_full, st_m1, st_one);
	return 0;
}
