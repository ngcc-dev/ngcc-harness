#define _POSIX_C_SOURCE 200809L

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#if defined(__linux__) || defined(__APPLE__) || defined(__unix__)
#include <sys/resource.h>
#include <unistd.h>
#endif
#if defined(__i386__) || defined(__x86_64__)
#include <immintrin.h>
#endif

#include "drng.h"
#include "cpu_features.h"
#include "prng.h"
#include "SIG_AlgorithmInstance.h"
#include "yuanyang_inner.h"
#include "yuanyang_opt_params.h"

extern DRNG_ctx drng_algorithm;

#if defined(__i386__) || defined(__x86_64__)
#define YUANYANG_BENCH_HAVE_CYCLES 1
#else
#define YUANYANG_BENCH_HAVE_CYCLES 0
#endif

#ifndef YUANYANG_BENCH_BUILD_FLAGS
#define YUANYANG_BENCH_BUILD_FLAGS "not recorded"
#endif
#ifndef YUANYANG_BENCH_BUILD_TOOL
#define YUANYANG_BENCH_BUILD_TOOL "not recorded"
#endif

#define BENCH_DEFAULT_RUNS 100u
#define BENCH_MIN_RECOMMENDED_RUNS 100u
#define BENCH_MAX_RUNS 1000000ul
#define BENCH_MESSAGE_BYTES 64u

#if defined(__clang__)
#define YUANYANG_BENCH_COMPILER "clang " __clang_version__
#elif defined(__GNUC__)
#define YUANYANG_BENCH_COMPILER "gcc " __VERSION__
#else
#define YUANYANG_BENCH_COMPILER __VERSION__
#endif

typedef struct {
	uint64_t wall_ns;
	uint64_t cpu_ns;
	uint64_t cycles;
} bench_sample;

typedef struct {
	uint64_t wall_ns;
	uint64_t cpu_ns;
	uint64_t cycles;
} bench_total;

static uint64_t
clock_now_ns(clockid_t clock_id)
{
	struct timespec ts;

	if (clock_gettime(clock_id, &ts) != 0) {
		return 0;
	}
	return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

static uint64_t
cycles_now(void)
{
#if YUANYANG_BENCH_HAVE_CYCLES
	uint32_t hi, lo;

	_mm_lfence();
	__asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
	_mm_lfence();
	return ((uint64_t)hi << 32) | (uint64_t)lo;
#else
	return 0;
#endif
}

static void
sample_begin(bench_sample *sample)
{
	sample->wall_ns = clock_now_ns(CLOCK_MONOTONIC);
	sample->cpu_ns = clock_now_ns(CLOCK_PROCESS_CPUTIME_ID);
	sample->cycles = cycles_now();
}

static void
sample_end(bench_sample *sample)
{
	uint64_t wall_end, cpu_end, cycle_end;

	cycle_end = cycles_now();
	cpu_end = clock_now_ns(CLOCK_PROCESS_CPUTIME_ID);
	wall_end = clock_now_ns(CLOCK_MONOTONIC);
	sample->wall_ns = wall_end - sample->wall_ns;
	sample->cpu_ns = cpu_end - sample->cpu_ns;
	sample->cycles = cycle_end - sample->cycles;
}

static uint64_t
current_rss_bytes(void)
{
#if defined(__linux__)
	FILE *fp;
	unsigned long total_pages, resident_pages;
	long page_size;

	fp = fopen("/proc/self/statm", "r");
	if (fp == NULL) {
		return 0;
	}
	if (fscanf(fp, "%lu %lu", &total_pages, &resident_pages) != 2) {
		fclose(fp);
		return 0;
	}
	fclose(fp);
	(void)total_pages;
	page_size = sysconf(_SC_PAGESIZE);
	return page_size > 0 ? (uint64_t)resident_pages * (uint64_t)page_size : 0;
#else
	return 0;
#endif
}

static uint64_t
peak_rss_bytes(void)
{
#if defined(__linux__) || defined(__APPLE__) || defined(__unix__)
	struct rusage usage;

	if (getrusage(RUSAGE_SELF, &usage) != 0) {
		return 0;
	}
#if defined(__APPLE__)
	return (uint64_t)usage.ru_maxrss;
#else
	return (uint64_t)usage.ru_maxrss * 1024ull;
#endif
#else
	return 0;
#endif
}

/* Fixed, documented seed derivation makes every software-DRNG run replayable. */
static int
seed_drng(unsigned run, unsigned domain)
{
	unsigned char seed[64];
	size_t u;

	for (u = 0; u < sizeof seed; u++) {
		seed[u] = (unsigned char)(0xA5u + 29u * (unsigned)u);
	}
	seed[0] ^= (unsigned char)domain;
	seed[1] ^= (unsigned char)run;
	seed[2] ^= (unsigned char)(run >> 8);
	seed[3] ^= (unsigned char)(run >> 16);
	seed[4] ^= (unsigned char)(run >> 24);
	return init_random_number(&drng_algorithm, seed, sizeof seed);
}

static void
add_sample(bench_total *total, const bench_sample *sample)
{
	total->wall_ns += sample->wall_ns;
	total->cpu_ns += sample->cpu_ns;
	total->cycles += sample->cycles;
}

static void
write_csv_row(FILE *csv, unsigned run, const char *operation,
	const bench_sample *sample, unsigned long long pk_len,
	unsigned long long sk_len, unsigned long long sig_len)
{
	if (csv != NULL) {
		fprintf(csv, "%u,%s,%u,%llu,%llu,%llu,%llu,%llu,%llu\n",
			run, operation, BENCH_MESSAGE_BYTES,
			(unsigned long long)sample->wall_ns,
			(unsigned long long)sample->cpu_ns,
			(unsigned long long)sample->cycles,
			pk_len, sk_len, sig_len);
	}
}

static void
format_time(char *dst, size_t dst_len, uint64_t ns)
{
	double value;
	const char *unit;

	if (ns >= 1000000000ull) {
		value = (double)ns / 1000000000.0;
		unit = "s";
	} else if (ns >= 1000000ull) {
		value = (double)ns / 1000000.0;
		unit = "ms";
	} else if (ns >= 1000ull) {
		value = (double)ns / 1000.0;
		unit = "us";
	} else {
		value = (double)ns;
		unit = "ns";
	}
	snprintf(dst, dst_len, "%9.3f %-2s", value, unit);
}

static void
format_cycles(char *dst, size_t dst_len, uint64_t cycles)
{
	double value;
	const char *unit;

	if (!YUANYANG_BENCH_HAVE_CYCLES) {
		snprintf(dst, dst_len, "%12s", "n/a");
		return;
	}
	if (cycles >= 1000000000ull) {
		value = (double)cycles / 1000000000.0;
		unit = "Gcy";
	} else if (cycles >= 1000000ull) {
		value = (double)cycles / 1000000.0;
		unit = "Mcy";
	} else if (cycles >= 1000ull) {
		value = (double)cycles / 1000.0;
		unit = "Kcy";
	} else {
		value = (double)cycles;
		unit = "cy";
	}
	snprintf(dst, dst_len, "%9.3f %-3s", value, unit);
}

static void
print_operation(const char *name, const bench_total *total, unsigned runs)
{
	char cpu[16], cycles[16];
	uint64_t avg_cpu, avg_cycles;
	double throughput;

	avg_cpu = total->cpu_ns / runs;
	avg_cycles = total->cycles / runs;
	throughput = total->wall_ns == 0 ? 0.0
		: (double)runs * 1000000000.0 / (double)total->wall_ns;
	format_time(cpu, sizeof cpu, avg_cpu);
	format_cycles(cycles, sizeof cycles, avg_cycles);
	printf("    %-30s : %s cpu", name, cpu);
	if (YUANYANG_BENCH_HAVE_CYCLES) {
		printf(", %s", cycles);
	}
	printf(", %12.3f ops/s\n", throughput);
}

int
main(int argc, char **argv)
{
	unsigned runs;
	unsigned long parsed_runs;
	unsigned long long pk_cap, sk_cap, sig_cap;
	unsigned long long pk_bytes, sk_bytes, sig_bytes;
	unsigned char message[BENCH_MESSAGE_BYTES];
	bench_total keygen_total, sign_total, verify_total;
	uint64_t baseline_memory, peak_memory;
	FILE *csv;
	const yuanyang_opt_param_entry *params;
	unsigned i;
	size_t u;

	runs = BENCH_DEFAULT_RUNS;
	if (argc >= 2) {
		char *end;

		parsed_runs = strtoul(argv[1], &end, 10);
		if (*argv[1] == '\0' || *end != '\0' || parsed_runs == 0
			|| parsed_runs > BENCH_MAX_RUNS)
		{
			fprintf(stderr, "usage: %s [runs] [raw.csv]\n", argv[0]);
			return 2;
		}
		runs = (unsigned)parsed_runs;
	}
	if (argc > 3) {
		fprintf(stderr, "usage: %s [runs] [raw.csv]\n", argv[0]);
		return 2;
	}
	if (runs < BENCH_MIN_RECOMMENDED_RUNS) {
		fprintf(stderr,
			"warning: the x86 self-assessment guideline recommends at least %u measurements\n",
			BENCH_MIN_RECOMMENDED_RUNS);
	}
	params = yuanyang_opt_param_for_logd(YUANYANG_LOGD);
	if (params == NULL || params->d != YUANYANG_D || params->q != YUANYANG_Q) {
		fprintf(stderr, "parameter table mismatch for logd=%u\n",
			(unsigned)YUANYANG_LOGD);
		return 1;
	}

	csv = NULL;
	if (argc == 3) {
		csv = fopen(argv[2], "w");
		if (csv == NULL) {
			perror(argv[2]);
			return 1;
		}
		fprintf(csv, "run,operation,message_bytes,wall_ns,cpu_ns,cycles,public_key_bytes,private_key_bytes,signature_bytes\n");
		fflush(csv);
	}

	for (u = 0; u < sizeof message; u++) {
		message[u] = (unsigned char)u;
	}
	memset(&keygen_total, 0, sizeof keygen_total);
	memset(&sign_total, 0, sizeof sign_total);
	memset(&verify_total, 0, sizeof verify_total);
	pk_cap = sig_get_pk_len_bytes();
	sk_cap = sig_get_sk_len_bytes();
	sig_cap = sig_get_sn_len_bytes();
	pk_bytes = 0;
	sk_bytes = 0;
	sig_bytes = 0;

	printf("%s benchmark over %u runs\n", ALGORITHM_INSTANCE, runs);
	printf("=================================================================\n");
	printf("Configuration\n");
	printf("    %-30s : %u\n", "measurements_per_operation", runs);
	printf("    %-30s : %u\n", "message_bytes", BENCH_MESSAGE_BYTES);
	printf("    %-30s : %s\n", "compiler_version", YUANYANG_BENCH_COMPILER);
	printf("    %-30s : %s\n", "build_flags", YUANYANG_BENCH_BUILD_FLAGS);
	fflush(stdout);
	baseline_memory = current_rss_bytes();

	for (i = 0; i < runs; i++) {
		unsigned char pk[pk_cap], sk[sk_cap], sig[sig_cap];
		unsigned long long pk_len, sk_len, sig_len;
		bench_sample sample;
		int rc;

		if (seed_drng(i, 0x4Bu) != 0) {
			fprintf(stderr, "DRNG init failed before keygen run %u\n", i);
			goto fail;
		}
		pk_len = pk_cap;
		sk_len = sk_cap;
		sample_begin(&sample);
		rc = sig_keygen(pk, &pk_len, sk, &sk_len);
		sample_end(&sample);
		if (rc != 0) {
			fprintf(stderr, "keygen failed on run %u: %d\n", i, rc);
			goto fail;
		}
		add_sample(&keygen_total, &sample);
		write_csv_row(csv, i, "keygen", &sample, pk_len, sk_len, 0);

		if (seed_drng(i, 0x53u) != 0) {
			fprintf(stderr, "DRNG init failed before sign run %u\n", i);
			goto fail;
		}
		sig_len = sig_cap;
		sample_begin(&sample);
		rc = sig_sign(sk, sk_len, message, BENCH_MESSAGE_BYTES, sig, &sig_len);
		sample_end(&sample);
		if (rc != 0) {
			fprintf(stderr, "sign failed on run %u: %d\n", i, rc);
			goto fail;
		}
		add_sample(&sign_total, &sample);
		write_csv_row(csv, i, "sign", &sample, pk_len, sk_len, sig_len);

		sample_begin(&sample);
		rc = sig_verify(pk, pk_len, sig, sig_len, message,
			BENCH_MESSAGE_BYTES);
		sample_end(&sample);
		if (rc != 0) {
			fprintf(stderr, "verify failed on run %u: %d\n", i, rc);
			goto fail;
		}
		add_sample(&verify_total, &sample);
		write_csv_row(csv, i, "verify", &sample, pk_len, sk_len, sig_len);

		pk_bytes += pk_len;
		sk_bytes += sk_len;
		sig_bytes += sig_len;
	}

	if (csv != NULL && fclose(csv) != 0) {
		perror("closing raw CSV");
		return 1;
	}
	peak_memory = peak_rss_bytes();
	if (peak_memory < baseline_memory) {
		peak_memory = baseline_memory;
	}

	printf("\nSize\n");
	printf("    %-30s : %llu\n", "public_key_bytes", pk_bytes / runs);
	printf("    %-30s : %llu\n", "private_key_bytes", sk_bytes / runs);
	printf("    %-30s : %llu\n", "signature_bytes", sig_bytes / runs);
	printf("    %-30s : %llu (%.3f KiB)\n", "static_memory_bytes",
		(unsigned long long)baseline_memory,
		(double)baseline_memory / 1024.0);
	printf("    %-30s : %llu (%.3f KiB)\n", "peak_memory_bytes",
		(unsigned long long)peak_memory,
		(double)peak_memory / 1024.0);
	printf("\nPerformance\n");
	print_operation("KeyGen", &keygen_total, runs);
	print_operation("Signature", &sign_total, runs);
	print_operation("Verification", &verify_total, runs);
	return 0;

fail:
	if (csv != NULL) {
		fclose(csv);
	}
	return 1;
}
