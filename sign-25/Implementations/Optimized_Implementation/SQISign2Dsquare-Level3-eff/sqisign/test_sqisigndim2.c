#include <drng.h>
#include <stdio.h>
#include <ec.h>
#include <inttypes.h>
#include <locale.h>
#include <time.h>

#include "bench.h"
#include "test_sqisigndim2.h"
#include <tools.h>
#include <isog.h>
#include "theta_structure.h"

#include "SIG_AlgorithmInstance.h"
#include "internal.h"
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#ifndef BENCH_RUNS
#define BENCH_RUNS 1000
#endif

#if defined(_WIN32)
#include <Windows.h>
#elif defined(__linux__) || defined(__unix__) || defined(__APPLE__)
#include <sys/utsname.h>
#include <unistd.h>
#endif

DRNG_ctx drng_algorithm;

static void
print_environment(FILE *fp)
{
	time_t t = time(NULL);

	fprintf(fp, "\n===========================================================\n");
	fprintf(fp, "Environment Information\n");

#if defined(_WIN32)
	SYSTEM_INFO si;
	MEMORYSTATUSEX mem_status;
	GetNativeSystemInfo(&si);

	fprintf(fp, "OS: Windows\n");
	fprintf(fp, "CPU cores: %u\n", si.dwNumberOfProcessors);

	mem_status.dwLength = sizeof(mem_status);
	if (GlobalMemoryStatusEx(&mem_status))
	{
		fprintf(fp, "Total RAM: %" PRIu64 " GB\n",
				(uint64_t)(mem_status.ullTotalPhys / (1024ULL * 1024ULL * 1024ULL)));
	}

#if defined(_MSC_VER)
	fprintf(fp, "Compiler: MSVC %d\n", _MSC_VER);
#else
	fprintf(fp, "Compiler: non-MSVC on Windows\n");
#endif
#elif defined(__linux__) || defined(__unix__) || defined(__APPLE__)
	struct utsname u;
	if (uname(&u) == 0)
	{
		fprintf(fp, "OS: %s %s (%s)\n", u.sysname, u.release, u.machine);
	}
	else
	{
		fprintf(fp, "OS: Unix-like\n");
	}

	{
		long cores = sysconf(_SC_NPROCESSORS_ONLN);
		long pages = sysconf(_SC_PHYS_PAGES);
		long page_size = sysconf(_SC_PAGE_SIZE);
		if (cores > 0)
		{
			fprintf(fp, "CPU cores: %ld\n", cores);
		}
		if (pages > 0 && page_size > 0)
		{
			double gb = ((double)pages * (double)page_size) / (1024.0 * 1024.0 * 1024.0);
			fprintf(fp, "Total RAM: %.2f GB\n", gb);
		}
	}

#if defined(__clang__)
	fprintf(fp, "Compiler: clang %d.%d.%d\n", __clang_major__, __clang_minor__, __clang_patchlevel__);
#elif defined(__GNUC__)
	fprintf(fp, "Compiler: gcc %d.%d.%d\n", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
#else
	fprintf(fp, "Compiler: unknown\n");
#endif
#else
	fprintf(fp, "OS: unknown\n");
	fprintf(fp, "Compiler: unknown\n");
#endif

#if defined(NDEBUG)
	fprintf(fp, "Build: Release\n");
#else
	fprintf(fp, "Build: Debug\n");
#endif

	fprintf(fp, "Arch: %s\n", (sizeof(void *) == 8) ? "x64" : "x86");
	fprintf(fp, "Timestamp: %s", ctime(&t));
}

static uint64_t
rdtsc(void)
{
	return (uint64_t)cpucycles();
}

static void fp_print(const char *name, const fp_t *a)
{
	printf("%s = [", name);
	for (int i = 0; i < NWORDS_FIELD; i++)
	{
		printf("0x%016llx%s",
			   (unsigned long long)(*a)[i],
			   (i == (NWORDS_FIELD - 1) ? "" : ", "));
	}
	printf("]\n");
}

static int fp_equal(const fp_t *a, const fp_t *b)
{
	return memcmp(*a, *b, sizeof(fp_t)) == 0;
}

/* Simple 64-bit random number generator. */
static uint64_t rand_u64(void)
{
	uint64_t x = 0;
	for (int i = 0; i < 5; i++)
	{
		x = (x << 15) ^ (uint64_t)(rand() & 0x7fff);
	}
	return x;
}

static void fp_random(fp_t *a)
{
	for (int i = 0; i < (NWORDS_FIELD - 1); i++)
	{
		(*a)[i] = rand_u64();
	}
	(*a)[NWORDS_FIELD - 1] = rand_u64() % (1801416877654);
}

static void fp2_random(fp2_t *a)
{
	for (int i = 0; i < (NWORDS_FIELD - 1); i++)
	{
		(a->re)[i] = rand_u64();
	}
	(a->re)[NWORDS_FIELD - 1] = rand_u64() % (1801416877654);

	for (int i = 0; i < (NWORDS_FIELD - 1); i++)
	{
		(a->im)[i] = rand_u64();
	}
	(a->im)[NWORDS_FIELD - 1] = rand_u64() % (1801416877654);
}

void init_random_array(unsigned char arr[32])
{
	for (int i = 0; i < 32; i++)
	{
		arr[i] = (unsigned char)(rand() % 256);
	}
}

static double timer_now_ms(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}

static inline uint64_t rdtsc_begin()
{
	unsigned int dummy;
	_mm_mfence();
	_mm_lfence();
	uint64_t t = __rdtsc();
	_mm_lfence();
	return t;
}

static inline uint64_t rdtsc_end()
{
	unsigned int dummy;
	uint64_t t = __rdtscp(&dummy);
	_mm_lfence();
	_mm_mfence();
	return t;
}

static uint64_t measure_overhead(int rounds)
{
	uint64_t total = 0;
	for (int i = 0; i < rounds; i++)
	{
		uint64_t t0 = rdtsc_begin();
		uint64_t t1 = rdtsc_end();
		total += (t1 - t0);
	}
	return total / (uint64_t)rounds;
}

int test_sqisign(int repeat, uint64_t bench)
{
	int res = 1;

	unsigned char state_sk[SECRETKEY_BYTES], state_pk[PUBLICKEY_BYTES], state_sig[SIGNATURE_BYTES];

	unsigned char msg[32] = {0};

	// srand((unsigned int)time(NULL));
	srand(2026);
	init_random_array(msg);

	// setlocale(LC_NUMERIC, "");
	uint64_t t0, t1;
	clock_t t;

	uint64_t total_keygen = 0;
	uint64_t total_sign = 0;
	uint64_t total_verif = 0;
	uint64_t overhead = measure_overhead(10000);

	printf("\n\nTesting signatures\n");
	for (int i = 0; i < repeat; ++i)
	{
		printf("#%d \n", i);
		t = tic();
		protocols_keygen(state_pk, state_sk);
		TOC(t, "Keygen");
		t = tic();
		protocols_sign(state_sig, state_sk, msg, 32);
		TOC(t, "Signing");

		t = tic();
		int check = protocols_verif(state_sig, state_pk, msg, 32);
		if (!check)
		{
			printf("verif failed !\n");
		}
		TOC(t, "Verification");

		printf("[full signature was: %s\n\n",
			   check ? "valid]" : "invalid]");

		res = res & check;
	}

	print_public_key(state_pk);
	print_secret_key(state_sk);
	print_message(msg, 32);
	print_signature(state_sig);

	if (bench)
	{
		float ms;

		printf("\n\nBenchmarking signatures\n");

		t = tic();
		for (int i = 0; i < bench; ++i)
		{
			uint64_t t0 = rdtsc_begin();
			protocols_keygen(state_pk, state_sk);
			uint64_t t1 = rdtsc_end();

			uint64_t diff = t1 - t0;
			total_keygen += (diff > overhead) ? (diff - overhead) : diff;
		}
		ms = (1000. * (float)(clock() - t) / CLOCKS_PER_SEC);
		printf("Average keygen time [%.2f ms]\n", (float)(ms / bench));

		t = tic();
		for (int i = 0; i < bench; ++i)
		{
			uint64_t t0 = rdtsc_begin();
			protocols_sign(state_sig, state_sk, msg, 32);
			uint64_t t1 = rdtsc_end();

			uint64_t diff = t1 - t0;
			total_sign += (diff > overhead) ? (diff - overhead) : diff;
		}
		ms = (1000. * (float)(clock() - t) / CLOCKS_PER_SEC);
		printf("Average signature time [%.2f ms]\n", (float)(ms / bench));

		t = tic();
		for (int i = 0; i < bench; ++i)
		{
			uint64_t t0 = rdtsc_begin();
			protocols_verif(state_sig, state_pk, msg, 32);
			uint64_t t1 = rdtsc_end();

			uint64_t diff = t1 - t0;
			total_verif += (diff > overhead) ? (diff - overhead) : diff;
		}
		ms = (1000. * (float)(clock() - t) / CLOCKS_PER_SEC);
		printf("Average verification time [%.2f ms]\n", (float)(ms / bench));

		printf("Average keygen cycles      : %.2f Mcycles\n", (double)total_keygen / (double)(bench * 1000000));
		printf("Average signing cycles     : %.2f Mcycles\n", (double)total_sign / (double)(bench * 1000000));
		printf("Average verification cycles: %.2f Mcycles\n", (double)total_verif / (double)(bench * 1000000));
		printf("Measurement overhead       : %llu cycles\n",
			   (unsigned long long)overhead);
	}

	return res;
}


int main(/*int argc, char **argv*/)
{
	int res = 1;

	unsigned char personalization_string[48] = "SQISign2Dsquare";
	// init drng_algorithm using seed
	init_random_number(&drng_algorithm, personalization_string, 48);

	printf("\nRunning reduction test\n \n");
	printf("Secret Key Size:%d, Public Key Size:%d, Signature Size:%d.\n", SECRETKEY_BYTES, PUBLICKEY_BYTES, SIGNATURE_BYTES);

	int runs = BENCH_RUNS;
	res &= test_sqisign(1, runs);

	if (!res)
	{
		printf("\nSome tests failed!\n");
	}
	else
	{
		printf("All tests passed!\n");
	}

	return (!res);

}
