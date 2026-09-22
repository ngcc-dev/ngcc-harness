/*
 * Local benchmark for the signing offline/online split.
 *
 * This is not part of the submission API.  It measures the ordinary signing
 * path against a path where the expanded key, signing NTT precomputation, and
 * message-independent presamples are prepared before the online signature.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#if defined __i386__ || defined _M_IX86 || defined __x86_64__ || defined _M_X64
#include <immintrin.h>
#endif

#include "drng.h"
#include "prng.h"
#include "SIG_AlgorithmInstance.h"
#include "sign_ntt.h"

extern DRNG_ctx drng_algorithm;

#if defined __i386__ || defined _M_IX86 || defined __x86_64__ || defined _M_X64
#define YUANYANG_BENCH_HAVE_CYCLES 1
#else
#define YUANYANG_BENCH_HAVE_CYCLES 0
#endif

#define YUANYANG_BATCH_PRESAMPLE_LOW_WATER 128u

typedef struct {
	uint64_t runs;
	uint64_t sign_standalone_cpu_ns;
	uint64_t sign_standalone_cycles;
	uint64_t sign_batch_cpu_ns;
	uint64_t sign_batch_cycles;
	uint64_t presample_offline_cpu_ns;
	uint64_t presample_offline_cycles;
	uint64_t presample_offline_refills;
	uint64_t presamples_generated;
} aggregate_batch_signature_bench;

typedef struct {
	uint64_t cpu_ns;
	uint64_t cycles;
} bench_elapsed;

static int
drng_randombytes_bench(void *ctx, unsigned char *buf,
	unsigned long long len_bytes)
{
	return get_random_number((DRNG_ctx *)ctx, buf, 8u * len_bytes);
}

static uint64_t
cpu_now_ns(void)
{
	clock_t c;

	c = clock();
	if (c == (clock_t)-1) {
		return 0;
	}
	return ((uint64_t)c * 1000000000ull) / (uint64_t)CLOCKS_PER_SEC;
}

static uint64_t
cycles_now(void)
{
#if YUANYANG_BENCH_HAVE_CYCLES
#if defined __GNUC__ && !defined __clang__
	uint32_t hi, lo;

	_mm_lfence();
	__asm__ __volatile__ ("rdtsc" : "=d" (hi), "=a" (lo) : : );
	return ((uint64_t)hi << 32) | (uint64_t)lo;
#else
	_mm_lfence();
	return __rdtsc();
#endif
#else
	return 0;
#endif
}

static void
format_time_cell(char *dst, size_t dst_len, uint64_t ns)
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
format_cycles_cell(char *dst, size_t dst_len, uint64_t cycles)
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
print_elapsed(const char *label, bench_elapsed x)
{
	char cpu[16], cycles[16];

	format_time_cell(cpu, sizeof cpu, x.cpu_ns);
	format_cycles_cell(cycles, sizeof cycles, x.cycles);
	printf("    %-28s : %s cpu", label, cpu);
	if (YUANYANG_BENCH_HAVE_CYCLES) {
		printf(", %s", cycles);
	}
	printf("\n");
}

static void
print_average(const char *label, uint64_t cpu_ns, uint64_t cycles,
	uint64_t runs)
{
	bench_elapsed x;

	x.cpu_ns = cpu_ns / runs;
	x.cycles = cycles / runs;
	print_elapsed(label, x);
}

static double
throughput(uint64_t cpu_ns)
{
	return cpu_ns == 0 ? 0.0 : 1000000000.0 / (double)cpu_ns;
}

static void
print_elapsed_operation(const char *label, bench_elapsed x)
{
	char cpu[16], cycles[16];

	format_time_cell(cpu, sizeof cpu, x.cpu_ns);
	format_cycles_cell(cycles, sizeof cycles, x.cycles);
	printf("    %-28s : %s cpu", label, cpu);
	if (YUANYANG_BENCH_HAVE_CYCLES) {
		printf(", %s", cycles);
	}
	printf(", %12.3f ops/s\n", throughput(x.cpu_ns));
}

static void
print_average_operation(const char *label,
	uint64_t cpu_ns, uint64_t cycles, uint64_t runs)
{
	bench_elapsed x;

	x.cpu_ns = cpu_ns / runs;
	x.cycles = cycles / runs;
	print_elapsed_operation(label, x);
}

static bench_elapsed
elapsed_add(bench_elapsed a, bench_elapsed b)
{
	bench_elapsed out;

	out.cpu_ns = a.cpu_ns + b.cpu_ns;
	out.cycles = a.cycles + b.cycles;
	return out;
}

static bench_elapsed
elapsed_average(bench_elapsed total, uint64_t runs)
{
	bench_elapsed out;

	out.cpu_ns = total.cpu_ns / runs;
	out.cycles = total.cycles / runs;
	return out;
}

static double
speedup(uint64_t baseline, uint64_t candidate)
{
	return candidate == 0 ? 0.0 : (double)baseline / (double)candidate;
}

static void
print_speedup(const char *label,
	uint64_t baseline_cpu, uint64_t baseline_cycles,
	uint64_t candidate_cpu, uint64_t candidate_cycles, uint64_t runs)
{
	uint64_t candidate_avg_cpu;

	candidate_avg_cpu = candidate_cpu / runs;
	printf("    %-28s : %.2fx cpu",
		label,
		speedup(baseline_cpu, candidate_cpu));
	if (YUANYANG_BENCH_HAVE_CYCLES) {
		printf(", %.2fx cycles",
			speedup(baseline_cycles, candidate_cycles));
	}
	printf(", %12.3f ops/s online\n", throughput(candidate_avg_cpu));
}

static int
seed_drng(unsigned base, unsigned run)
{
	unsigned char seed[64];

	for (size_t u = 0; u < sizeof seed; u++) {
		seed[u] = (unsigned char)(base + u + run);
	}
	return init_random_number(&drng_algorithm, seed, sizeof seed);
}

static int
refill_presamples_if_low(
	aggregate_batch_signature_bench *agg,
	yuanyang_sign_presample_buffer *presample_buffer,
	const yuanyang_sign_ntt_precomp *ntt_precomp,
	size_t presample_depth)
{
	prng offline_rng;
	uint64_t offline_cpu_start, offline_cpu_end;
	uint64_t offline_cycle_start, offline_cycle_end;
	size_t before_count;
	size_t low_water;
	int rc;

	/*
	 * Keep enough key-only presamples queued that the online path mostly
	 * measures challenge-dependent signing work.
	 */
	low_water = presample_depth < YUANYANG_BATCH_PRESAMPLE_LOW_WATER
		? presample_depth
		: YUANYANG_BATCH_PRESAMPLE_LOW_WATER;
	if (presample_buffer->count >= low_water) {
		return 0;
	}

	if (seed_drng(0x30u, (unsigned)agg->presample_offline_refills) != 0) {
		fprintf(stderr, "DRNG init failed before offline refill %llu\n",
			(unsigned long long)agg->presample_offline_refills);
		return 1;
	}
	prng_init(&offline_rng, drng_randombytes_bench, &drng_algorithm);
	if (prng_status(&offline_rng) != YUANYANG_SUCCESS) {
		fprintf(stderr, "offline PRNG init failed\n");
		return 1;
	}

	before_count = presample_buffer->count;
	offline_cpu_start = cpu_now_ns();
	offline_cycle_start = cycles_now();
	rc = yuanyang_sign_presample_buffer_fill_ntt(
		presample_buffer, ntt_precomp, &offline_rng, presample_depth);
	offline_cpu_end = cpu_now_ns();
	offline_cycle_end = cycles_now();
	if (rc != YUANYANG_SUCCESS) {
		fprintf(stderr, "offline presample refill failed: %d\n", rc);
		return 1;
	}

	agg->presample_offline_cpu_ns += offline_cpu_end - offline_cpu_start;
	agg->presample_offline_cycles += offline_cycle_end - offline_cycle_start;
	agg->presample_offline_refills++;
	agg->presamples_generated += presample_buffer->count - before_count;
	return 0;
}

int
main(int argc, char **argv)
{
	unsigned runs;
	size_t presample_depth;
	unsigned long long pk_len, sk_len;
	unsigned long long pk_cap, sk_cap, sn_cap;
	aggregate_batch_signature_bench agg;
	yuanyang_expanded_sk expanded;
	yuanyang_sign_ntt_precomp ntt_precomp;
	yuanyang_sign_presample *presample_slots;
	yuanyang_sign_presample_buffer presample_buffer;
	bench_elapsed keygen_time;
	bench_elapsed decode_time;
	bench_elapsed precomp_time;
	int rc;

	runs = 100;
	if (argc >= 2) {
		unsigned long x = strtoul(argv[1], NULL, 10);

		if (x > 0 && x <= 1000000ul) {
			runs = (unsigned)x;
		}
	}
	presample_depth = 256;
	if (argc >= 3) {
		unsigned long x = strtoul(argv[2], NULL, 10);

		if (x > 0 && x <= 1000000ul) {
			presample_depth = (size_t)x;
		}
	}
	memset(&agg, 0, sizeof agg);
	pk_cap = sig_get_pk_len_bytes();
	sk_cap = sig_get_sk_len_bytes();
	sn_cap = sig_get_sn_len_bytes();
	presample_slots = malloc(presample_depth * sizeof *presample_slots);
	if (presample_slots == NULL) {
		fprintf(stderr, "failed to allocate %zu presample slots\n",
			presample_depth);
		return 1;
	}
	yuanyang_sign_presample_buffer_init(&presample_buffer,
		presample_slots, presample_depth);

	{
		unsigned char pk[pk_cap];
		unsigned char sk[sk_cap];
		uint64_t cpu_start, cpu_end;
		uint64_t cycle_start, cycle_end;

		if (seed_drng(1u, 0u) != 0) {
			fprintf(stderr, "DRNG init failed for setup keygen\n");
			return 1;
		}
		pk_len = pk_cap;
		sk_len = sk_cap;
		cpu_start = cpu_now_ns();
		cycle_start = cycles_now();
		rc = sig_keygen(pk, &pk_len, sk, &sk_len);
		cpu_end = cpu_now_ns();
		cycle_end = cycles_now();
		if (rc != 0) {
			fprintf(stderr, "setup keygen failed: %d\n", rc);
			return 1;
		}
		keygen_time.cpu_ns = cpu_end - cpu_start;
		keygen_time.cycles = cycle_end - cycle_start;

		cpu_start = cpu_now_ns();
		cycle_start = cycles_now();
		rc = yuanyang_decode_private_key(&expanded, sk, sk_len);
		cpu_end = cpu_now_ns();
		cycle_end = cycles_now();
		if (rc != YUANYANG_SUCCESS) {
			fprintf(stderr, "decode private key failed: %d\n", rc);
			return 1;
		}
		decode_time.cpu_ns = cpu_end - cpu_start;
		decode_time.cycles = cycle_end - cycle_start;

		cpu_start = cpu_now_ns();
		cycle_start = cycles_now();
		yuanyang_sign_ntt_precompute(&ntt_precomp, &expanded);
		cpu_end = cpu_now_ns();
		cycle_end = cycles_now();
		precomp_time.cpu_ns = cpu_end - cpu_start;
		precomp_time.cycles = cycle_end - cycle_start;

		for (unsigned i = 0; i < runs; i++) {
			unsigned char sn_standalone[sn_cap];
			unsigned char sn_batch[sn_cap];
			unsigned long long sn_standalone_len;
			unsigned long long sn_batch_len;
			unsigned char message[64];
			uint64_t sign_cpu_start, sign_cpu_end;
			uint64_t sign_cycle_start, sign_cycle_end;
			yuanyang_sign_stats standalone_stats;
			yuanyang_sign_stats batch_stats;
			int message_len;

			message_len = snprintf((char *)message, sizeof message,
				"%s batch signature bench #%u", ALGORITHM_INSTANCE, i);
			if (message_len <= 0 || (size_t)message_len >= sizeof message) {
				fprintf(stderr, "message formatting failed on run %u\n", i);
				return 1;
			}

			if (seed_drng(0x80u, i) != 0) {
				fprintf(stderr, "DRNG init failed before standalone run %u\n", i);
				return 1;
			}
			sn_standalone_len = sn_cap;
			sign_cpu_start = cpu_now_ns();
			sign_cycle_start = cycles_now();
			rc = yuanyang_sign_core_with_stats(sk, sk_len, message,
				(unsigned long long)message_len, sn_standalone,
				&sn_standalone_len, &standalone_stats);
			sign_cpu_end = cpu_now_ns();
			sign_cycle_end = cycles_now();
			if (rc != 0) {
				fprintf(stderr, "standalone sign failed on run %u: %d\n", i, rc);
				return 1;
			}

			if (refill_presamples_if_low(&agg, &presample_buffer,
					&ntt_precomp, presample_depth) != 0)
			{
				return 1;
			}

			if (seed_drng(0x80u, i) != 0) {
				fprintf(stderr, "DRNG init failed before batch run %u\n", i);
				return 1;
			}
			sn_batch_len = sn_cap;
			{
				uint64_t batch_cpu_start, batch_cpu_end;
				uint64_t batch_cycle_start, batch_cycle_end;

				batch_cpu_start = cpu_now_ns();
				batch_cycle_start = cycles_now();
				rc = yuanyang_sign_core_precomputed_ntt_buffered(
					&expanded, &ntt_precomp, &presample_buffer, message,
					(unsigned long long)message_len, sn_batch,
					&sn_batch_len, &batch_stats);
				batch_cpu_end = cpu_now_ns();
				batch_cycle_end = cycles_now();
				if (rc != 0) {
					fprintf(stderr, "batch sign failed on run %u: %d\n", i, rc);
					return 1;
				}
				agg.sign_batch_cpu_ns += batch_cpu_end - batch_cpu_start;
				agg.sign_batch_cycles += batch_cycle_end - batch_cycle_start;
			}

			rc = sig_verify(pk, pk_len, sn_standalone, sn_standalone_len,
				message, (unsigned long long)message_len);
			if (rc != 0) {
				fprintf(stderr, "standalone verify failed on run %u: %d\n", i, rc);
				return 1;
			}

			agg.sign_standalone_cpu_ns += sign_cpu_end - sign_cpu_start;
			agg.sign_standalone_cycles += sign_cycle_end - sign_cycle_start;

			rc = sig_verify(pk, pk_len, sn_batch, sn_batch_len,
				message, (unsigned long long)message_len);
			if (rc != 0) {
				fprintf(stderr, "batch verify failed on run %u: %d\n", i, rc);
				return 1;
			}

			agg.runs++;
		}
	}

	printf("%s full vs batch/offline signing benchmark over %llu runs\n",
		ALGORITHM_INSTANCE,
		(unsigned long long)agg.runs);
	printf("=================================================================\n");
	printf("One-time setup\n");
	print_elapsed("setup keygen", keygen_time);
	printf("\nOffline batch work\n");
	print_elapsed("private key decode (1/key)", decode_time);
	print_elapsed("NTT precompute (1/key)", precomp_time);
	printf("    %-28s : %zu slots\n", "presample buffer depth",
		presample_depth);
	print_elapsed("offline presample total",
		(bench_elapsed){ agg.presample_offline_cpu_ns,
			agg.presample_offline_cycles });
	if (agg.presamples_generated != 0) {
		print_average("avg offline presample/slot",
			agg.presample_offline_cpu_ns,
			agg.presample_offline_cycles,
			agg.presamples_generated);
	}
	printf("    %-28s : %llu refills, %llu generated, %llu consumed\n",
		"offline presamples",
		(unsigned long long)agg.presample_offline_refills,
		(unsigned long long)agg.presamples_generated,
		(unsigned long long)(agg.presamples_generated
			- presample_buffer.count));
	printf("\nSignature paths\n");
	print_average_operation("avg full sign", agg.sign_standalone_cpu_ns,
		agg.sign_standalone_cycles, agg.runs);
	print_average_operation("avg online sign (batch)", agg.sign_batch_cpu_ns,
		agg.sign_batch_cycles, agg.runs);
	{
		bench_elapsed offline_per_run;
		bench_elapsed online_per_run;
		bench_elapsed online_with_presample;

		offline_per_run = elapsed_average(
			(bench_elapsed){ agg.presample_offline_cpu_ns,
				agg.presample_offline_cycles },
			agg.runs);
		online_per_run = elapsed_average(
			(bench_elapsed){ agg.sign_batch_cpu_ns,
				agg.sign_batch_cycles },
			agg.runs);
		online_with_presample = elapsed_add(online_per_run,
			offline_per_run);
		print_elapsed_operation("avg batch+presample/run",
			online_with_presample);
	}
	print_speedup("full/online speedup",
		agg.sign_standalone_cpu_ns,
		agg.sign_standalone_cycles,
		agg.sign_batch_cpu_ns,
		agg.sign_batch_cycles,
		agg.runs);

	free(presample_slots);
	return 0;
}
