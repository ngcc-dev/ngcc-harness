/* TRIKE (kem-36): multi-ciphertext message search against the unsalted deterministic FO (scaled).
   Links the submitted reference code UNMODIFIED. We supply only get_random_number(): run_all.sh compiles
   the submitted ICCS/drng.c with -Dget_random_number=iccs_get_random_number, and the shim below forwards
   every call to it, except the single Encaps read of the global drng_algorithm (the l-bit message m),
   which it replaces by a chosen message whose low B bits only are nonzero (scale model of l).
   Local DRNGs seeded from sigma or (m, r2) are the scheme's own XOF and are always forwarded.
   The shim aborts unless every Encaps reads the global RNG exactly once, with width PARAM_M.
   Victim: T Encaps under one static pk, m uniform in the B-bit space.
   Attacker: pk + the T ciphertexts only; enumerates m from a random start, re-runs the submitted
   kem_enc with m forced, and looks u up in a hash table of the T targets.
   SCORING only: full-ct equality, ss_attacker == ss_Encaps == kem_dec(sk, ct).
   usage: mtv B logT trials seed */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <time.h>
#include "KEM_AlgorithmInstance.h"
#include "trike_types.h"
#include "drng.h"

DRNG_ctx drng_algorithm;
int iccs_get_random_number(DRNG_ctx *drng, unsigned char *out, unsigned long long bits);

static int force_mode = 0;
static uint64_t forced_m = 0;
static unsigned long long rng_calls_forced = 0;

int get_random_number(DRNG_ctx *drng, unsigned char *out, unsigned long long bits)
{
	/* local DRNGs seeded from sigma / (m,r2) are the scheme's XOF: always forward */
	if (!force_mode || drng != &drng_algorithm) return iccs_get_random_number(drng, out, bits);
	rng_calls_forced++;
	if (rng_calls_forced > 1 || bits != PARAM_M) { fprintf(stderr, "unexpected RNG width %llu in Encaps\n", bits); exit(2); }
	memset(out, 0, (bits + 7) / 8);
	memcpy(out, &forced_m, sizeof forced_m); /* little-endian low B bits */
	return 0;
}

static uint64_t sm_state;
static uint64_t splitmix(void)
{
	uint64_t z = (sm_state += 0x9E3779B97F4A7C15ULL);
	z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
	z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
	return z ^ (z >> 31);
}

static void enc_with(unsigned char *pk, uint64_t m, ciphertext_t *ct, unsigned char *ss)
{
	unsigned long long a, b;
	force_mode = 1; forced_m = m; rng_calls_forced = 0;
	kem_enc(pk, sizeof(public_key_t), ss, &a, (unsigned char *)ct, &b);
	force_mode = 0;
	if (rng_calls_forced != 1) { fprintf(stderr, "Encaps read the global RNG %llu times\n", rng_calls_forced); exit(2); }
}

/* open-addressing table on the first 8 bytes of u */
typedef struct { uint64_t key; int32_t idx; } slot_t;

int main(int argc, char **argv)
{
	int B = argc > 1 ? atoi(argv[1]) : 14;      /* scaled message width */
	int logT = argc > 2 ? atoi(argv[2]) : 4;    /* number of target ciphertexts */
	int trials = argc > 3 ? atoi(argv[3]) : 10;
	uint64_t seed = argc > 4 ? strtoull(argv[4], 0, 10) : 1;
	uint64_t T = 1ULL << logT, space = 1ULL << B, mask = space - 1;
	sm_state = seed;

	unsigned char kseed[48];
	for (int i = 0; i < 48; i++) kseed[i] = (unsigned char)splitmix();
	init_random_number(&drng_algorithm, kseed, 48);
	unsigned char *pk = malloc(sizeof(public_key_t)), *sk = malloc(sizeof(secret_key_t));
	unsigned long long a, b;
	kem_keygen(pk, &a, sk, &b);

	ciphertext_t *cts = malloc(T * sizeof(ciphertext_t));
	unsigned char (*sss)[64] = malloc(T * 64);
	uint64_t tsz = 1; while (tsz < 4 * T) tsz <<= 1;
	slot_t *tab = malloc(tsz * sizeof(slot_t));
	size_t ssl = sizeof(shared_secret_t);

	double sum_ratio = 0, sum_work = 0; int ok_all = 1;
	printf("TRIKE r=%d l=%d | scale B=%d T=2^%d trials=%d | expected work 2^B/(T+1)=%.1f\n",
	       PARAM_R, PARAM_M, B, logT, trials, (double)space / (T + 1));
	clock_t t0 = clock(); unsigned long long encs = 0;
	for (int tr = 0; tr < trials; tr++) {
		memset(tab, 0xff, tsz * sizeof(slot_t));
		for (uint64_t i = 0; i < T; i++) {          /* victim side */
			uint64_t m = splitmix() & mask;
			enc_with(pk, m, &cts[i], sss[i]); encs++;
			uint64_t k; memcpy(&k, cts[i].u, 8);
			uint64_t h = (k * 0x9E3779B97F4A7C15ULL) & (tsz - 1);
			while (tab[h].idx != -1) h = (h + 1) & (tsz - 1);
			tab[h].key = k; tab[h].idx = (int32_t)i;
		}
		uint64_t start = splitmix() & mask, work = 0; int hit = -1;
		unsigned char ss[64]; ciphertext_t ct;
		for (uint64_t j = 0; j < space && hit < 0; j++) {   /* attacker side */
			uint64_t m = (start + j) & mask;
			enc_with(pk, m, &ct, ss); encs++; work++;
			uint64_t k; memcpy(&k, ct.u, 8);
			uint64_t h = (k * 0x9E3779B97F4A7C15ULL) & (tsz - 1);
			for (; tab[h].idx != -1; h = (h + 1) & (tsz - 1))
				if (tab[h].key == k && memcmp(&ct, &cts[tab[h].idx], sizeof ct) == 0) { hit = tab[h].idx; break; }
		}
		if (hit < 0) { printf("trial %d: no hit\n", tr); ok_all = 0; continue; }
		unsigned char ssd[64]; unsigned long long sl;
		kem_dec(sk, sizeof(secret_key_t), (unsigned char *)&cts[hit], sizeof(ciphertext_t), ssd, &sl);
		int eq_enc = memcmp(ss, sss[hit], ssl) == 0, eq_dec = memcmp(ss, ssd, ssl) == 0;
		ok_all &= eq_enc && eq_dec;
		double expw = (double)space / (T + 1);
		sum_ratio += work / expw; sum_work += work;
		printf("trial %2d: hit target #%d after %llu re-encs (2^%.2f)  ss==Encaps:%s ss==Decaps(sk):%s\n",
		       tr, hit, (unsigned long long)work, log2((double)work), eq_enc ? "YES" : "NO", eq_dec ? "YES" : "NO");
	}
	double secs = (double)(clock() - t0) / CLOCKS_PER_SEC;
	printf("SUMMARY B=%d logT=%d: mean work=2^%.2f  mean(work/expected)=%.3f  all-ss-match=%s  %.2f ms/enc\n",
	       B, logT, log2(sum_work / trials), sum_ratio / trials, ok_all ? "YES" : "NO", 1e3 * secs / encs);
	if (!ok_all) return 1;
	puts("MULTI-CIPHERTEXT PROPERTY kem-36-7 CONFIRMED: scaled recovery matched encapsulation and decapsulation keys");
	return 0;
}
