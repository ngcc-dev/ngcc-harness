/* sign-25-6: sig_verify ignores the declared signature length and decodes
 * SIGNATURE_BYTES bytes from the caller's buffer.
 *
 * Built with the unmodified sources of one SQIsign2D2 tree and one value of
 * COMPRESSED. An honest signature is copied so that its declared bytes end
 * exactly at a page boundary; the following page is made PROT_NONE. Each
 * case runs in a child process so that a fault can be reported:
 *   full    all SIGNATURE_BYTES bytes, declared at full length (control)
 *   minus1  the first SIGNATURE_BYTES-1 bytes, declared at that length
 *   one     the first byte only, declared as one byte
 * Output: RESULT <set> sig=<bytes> full=<status> minus1=<status> one=<status>
 * where <status> is "acc" or "rej" (verify returned 0 or nonzero) or "sigN".
 */
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

#include "drng.h"

#ifndef SQI_LABEL
#define SQI_LABEL "SQIsign2D2"
#endif

DRNG_ctx drng_algorithm;
unsigned long long sig_get_pk_len_bytes(void), sig_get_sk_len_bytes(void), sig_get_sn_len_bytes(void);
int sig_keygen(unsigned char *, unsigned long long *, unsigned char *, unsigned long long *);
int sig_sign(unsigned char *, unsigned long long, unsigned char *, unsigned long long,
             unsigned char *, unsigned long long *);
int sig_verify(unsigned char *, unsigned long long, unsigned char *, unsigned long long,
               unsigned char *, unsigned long long);

static void run_case(unsigned char *pk, unsigned long long pkl, const unsigned char *sn,
                     unsigned long long declared, unsigned char *m, unsigned long long ml,
                     char *status, size_t status_len)
{
	long pg = sysconf(_SC_PAGESIZE);
	size_t span = ((declared + pg - 1) / pg) * pg;
	fflush(stdout);
	pid_t pid = fork();
	if (pid == 0) {
		unsigned char *map = mmap(NULL, span + pg, PROT_READ | PROT_WRITE,
		                          MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		if (map == MAP_FAILED)
			_exit(4);
		unsigned char *p = map + span - declared; /* declared bytes end at the page boundary */
		memcpy(p, sn, declared);
		if (mprotect(map + span, pg, PROT_NONE))
			_exit(5);
		_exit(sig_verify(pk, pkl, p, declared, m, ml) == 0 ? 0 : 1);
	}
	int st = 0;
	waitpid(pid, &st, 0);
	if (WIFSIGNALED(st))
		snprintf(status, status_len, "sig%d", WTERMSIG(st));
	else
		snprintf(status, status_len, WEXITSTATUS(st) == 0 ? "acc" : WEXITSTATUS(st) == 1 ? "rej" : "rc%d",
		         WEXITSTATUS(st));
}

int main(void)
{
	unsigned long long pkl = sig_get_pk_len_bytes(), skl = sig_get_sk_len_bytes(), snl = sig_get_sn_len_bytes(), a, b;
	unsigned char *pk = malloc(pkl), *sk = malloc(skl), *sn = malloc(snl), m[32], seed[48];
	char st_full[16], st_m1[16], st_one[16];

	if (!pk || !sk || !sn)
		return 2;
	for (int i = 0; i < 48; i++)
		seed[i] = (unsigned char)(9 * i + 4);
	for (int i = 0; i < 32; i++)
		m[i] = (unsigned char)(i + 100);
	init_random_number(&drng_algorithm, seed, sizeof seed);
	if (sig_keygen(pk, &a, sk, &b) || sig_sign(sk, skl, m, sizeof m, sn, &a) || a != snl)
		return 3;
	run_case(pk, pkl, sn, snl, m, sizeof m, st_full, sizeof st_full);
	run_case(pk, pkl, sn, snl - 1, m, sizeof m, st_m1, sizeof st_m1);
	run_case(pk, pkl, sn, 1, m, sizeof m, st_one, sizeof st_one);
	printf("RESULT %s sig=%llu full=%s minus1=%s one=%s\n", SQI_LABEL, snl, st_full, st_m1, st_one);
	return 0;
}
