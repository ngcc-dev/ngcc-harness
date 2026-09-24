#include "auxfunc.h"
#include "cpucycles.h"
#include "cbd.h"
#include "drng.h"
#include "indcpa.h"
#include "params.h"
#include "poly.h"
#include "polyvec.h"
#include "speed_print.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <x86intrin.h>
#include <time.h>

#define NTESTS 1000
#define SEED_LEN_BYTES 64
#define CACHE_FLUSH_SIZE (8 * 1024 * 1024)  /* 8 MB to flush L3 cache */

DRNG_ctx drng_algorithm;
static uint8_t cache_flush_buffer[CACHE_FLUSH_SIZE];

struct BenchmarkResults
{
	uint64_t pseudoXOF_cycles;
	uint64_t ntt_cycles;
    uint64_t invntt_cycles;
	uint64_t poly_add_cycles;
	uint64_t poly_basemul_cycles;
	uint64_t polyvec_basemul_acc_cycles;
	uint64_t poly_cbd_cycles;
	uint64_t gen_matrix_cycles;
};

static void
flush_cache(void)
{
	/* Flush L3 cache by accessing a large buffer beyond cache size */
	volatile uint8_t *flush_ptr = (volatile uint8_t *)cache_flush_buffer;
	for (size_t i = 0; i < CACHE_FLUSH_SIZE; i++)
		{
			flush_ptr[i];
		}
	/* Add memory fence to ensure all operations complete */
	_mm_mfence();
}

static void
fill_random_poly(poly *res)
{
	unsigned char randbuf[2];
	for (int i = 0; i < KEM_N; i++)
		{
			get_random_number(&drng_algorithm, randbuf, sizeof(randbuf) * 8);
			uint16_t val;
			memcpy(&val, randbuf, sizeof(val));
			res->coeffs[i] = (uint16_t)(val % KEM_Q);
		}
}

static void
fill_random_polyvec(polyvec *res)
{
	for (int i = 0; i < KEM_L; i++)
		{
			fill_random_poly(&res->vec[i]);
		}
}

static int
cmp_uint64(const void *a, const void *b)
{
	const uint64_t left = *(const uint64_t *)a;
	const uint64_t right = *(const uint64_t *)b;
	return (left > right) - (left < right);
}

static uint64_t
median(uint64_t *values, size_t len)
{
	qsort(values, len, sizeof(uint64_t), cmp_uint64);
	if (len % 2 == 1)
		{
			return values[len / 2];
		}
	return (values[len / 2 - 1] + values[len / 2]) / 2;
}

static uint64_t
average(uint64_t *values, size_t len)
{
	uint64_t acc = 0;
	for (size_t i = 0; i < len; i++)
		{
			acc += values[i];
		}
	return acc / len;
}

static void
print_duration_results(const char *label, uint64_t *values, size_t len)
{
	printf("%s\n", label);
	printf("median: %llu cycles/ticks\n", (unsigned long long)median(values, len));
	printf("average: %llu cycles/ticks\n\n", (unsigned long long)average(values, len));
}

static void
benchmark_iteration(struct BenchmarkResults *res)
{
	unsigned char msg[64];
	unsigned char out[32];
	poly a, b, r;
	polyvec av, bv;
	unsigned char seed[KEM_SYMBYTES];
	uint8_t cbd_buf[KEM_ETA * KEM_N / 4];
	polyvec matrix[KEM_L];
	volatile uint16_t sink = 0;
    srand(time(NULL));
    int index = 0;

	/* Prepare seed and buffers */
	for (int i = 0; i < KEM_SYMBYTES; i++)
		{
			seed[i] = (unsigned char)(i % 256);
		}
	for (int i = 0; i < 64; i++)
		{
			msg[i] = (unsigned char)(i % 256);
		}
	get_random_number(&drng_algorithm, cbd_buf, sizeof(cbd_buf) * 8);

	/* Benchmark pseudoXOF */
	memset(out, 0, sizeof(out));
	uint64_t start = cpucycles();
	pseudoXOF(256, msg, (unsigned long long)sizeof(msg) * 8, out);
	res->pseudoXOF_cycles = cpucycles() - start;
	index = rand() % 32;
    sink += out[index];
	flush_cache();

	/* Benchmark poly_ntt */
	fill_random_poly(&a);
	start = cpucycles();
	poly_ntt(&a);
	res->ntt_cycles = cpucycles() - start;
	sink += a.coeffs[rand() % KEM_N];
	flush_cache();

    /* Benchmark poly_invntt */
    fill_random_poly(&a);
	start = cpucycles();
	poly_invntt_tomont(&a);
	res->invntt_cycles = cpucycles() - start;
	sink += a.coeffs[rand() % KEM_N];
	flush_cache();

	/* Benchmark poly_add */
	fill_random_poly(&a);
	fill_random_poly(&b);
	start = cpucycles();
	poly_add(&r, &a, &b);
	res->poly_add_cycles = cpucycles() - start;
    sink += r.coeffs[rand() % KEM_N];
	flush_cache();

	/* Benchmark poly_basemul_montgomery */
	fill_random_poly(&a);
	fill_random_poly(&b);
	poly_ntt(&a);
	poly_ntt(&b);
	start = cpucycles();
	poly_basemul_montgomery(&r, &a, &b);
	res->poly_basemul_cycles = cpucycles() - start;
    sink += r.coeffs[rand() % KEM_N];
	flush_cache();

	/* Benchmark polyvec_basemul_acc_montgomery */
	fill_random_polyvec(&av);
	fill_random_polyvec(&bv);
	polyvec_ntt(&av);
	polyvec_ntt(&bv);
	start = cpucycles();
	polyvec_basemul_acc_montgomery(&r, &av, &bv);
	res->polyvec_basemul_acc_cycles = cpucycles() - start;
    sink += r.coeffs[rand() % KEM_N];
	flush_cache();

	/* Benchmark poly_cbd_eta */
	start = cpucycles();
	poly_cbd_eta(&a, cbd_buf);
	res->poly_cbd_cycles = cpucycles() - start;
    sink += a.coeffs[rand() % KEM_N];
	flush_cache();

	/* Benchmark gen_matrix */
	start = cpucycles();
	gen_matrix(matrix, seed, 0);
	res->gen_matrix_cycles = cpucycles() - start;
    int i, j;
    i = rand() % KEM_L;
    j = rand() % KEM_L;
    index = rand() % KEM_N;
    sink += matrix[i].vec[j].coeffs[index];
	flush_cache();
}

static void
analyze_results(struct BenchmarkResults results[NTESTS])
{
	uint64_t pseudoXOF_times[NTESTS];
	uint64_t ntt_times[NTESTS];
    uint64_t invntt_times[NTESTS];
	uint64_t poly_add_times[NTESTS];
	uint64_t poly_basemul_times[NTESTS];
	uint64_t polyvec_basemul_acc_times[NTESTS];
	uint64_t poly_cbd_times[NTESTS];
	uint64_t gen_matrix_times[NTESTS];

	for (int i = 0; i < NTESTS; i++)
		{
			pseudoXOF_times[i] = results[i].pseudoXOF_cycles;
			ntt_times[i] = results[i].ntt_cycles;
			invntt_times[i] = results[i].invntt_cycles;
			poly_add_times[i] = results[i].poly_add_cycles;
			poly_basemul_times[i] = results[i].poly_basemul_cycles;
			polyvec_basemul_acc_times[i] = results[i].polyvec_basemul_acc_cycles;
			poly_cbd_times[i] = results[i].poly_cbd_cycles;
			gen_matrix_times[i] = results[i].gen_matrix_cycles;
		}

	print_duration_results("pseudoXOF:", pseudoXOF_times, NTESTS);
	print_duration_results("poly_ntt:", ntt_times, NTESTS);
	print_duration_results("poly_invntt:", invntt_times, NTESTS);
	print_duration_results("poly_add:", poly_add_times, NTESTS);
	print_duration_results("poly_basemul_montgomery:", poly_basemul_times, NTESTS);
	print_duration_results("polyvec_basemul_acc_montgomery:", polyvec_basemul_acc_times, NTESTS);
	print_duration_results("poly_cbd_eta:", poly_cbd_times, NTESTS);
	print_duration_results("gen_matrix:", gen_matrix_times, NTESTS);
}

int
main(void)
{
	struct BenchmarkResults results[NTESTS];
	DRNG_ctx drng_seed;
	unsigned char seed[SEED_LEN_BYTES];

	get_random_number(&drng_seed, seed, SEED_LEN_BYTES * 8);
	init_random_number(&drng_algorithm, seed, SEED_LEN_BYTES);

	for (int i = 0; i < NTESTS; i++)
		{
			benchmark_iteration(&results[i]);
		}

	analyze_results(results);

	return 0;
}
