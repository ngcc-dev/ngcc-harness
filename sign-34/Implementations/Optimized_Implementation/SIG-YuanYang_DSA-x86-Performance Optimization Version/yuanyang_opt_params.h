#ifndef YUANYANG_OPT_PARAMS_H
#define YUANYANG_OPT_PARAMS_H

#include <stddef.h>
#include <stdint.h>

/*
 * Optimized build feature switches.  This mirrors ntrugen's policy: AVX2 is
 * enabled when the compiler target exposes it, unless the build overrides the
 * macro explicitly.  The top-level Makefile defaults to -march=native, so this
 * is automatic on AVX2-capable build machines and stays portable with
 * YUANYANG_OPT_NATIVE=0.
 */
#ifndef YUANYANG_AVX2
#if defined __AVX2__ && __AVX2__
#define YUANYANG_AVX2   1
#else
#define YUANYANG_AVX2   0
#endif
#endif

#ifndef YUANYANG_FMA
#if YUANYANG_AVX2 && defined __FMA__ && __FMA__
#define YUANYANG_FMA   1
#else
#define YUANYANG_FMA   0
#endif
#endif

#if YUANYANG_AVX2
#include <immintrin.h>
#if defined __GNUC__ || defined __clang__
#include <x86intrin.h>
#endif
#if defined __GNUC__
#if YUANYANG_FMA
#define YUANYANG_TARGET_AVX2   __attribute__((target("avx2,fma")))
#else
#define YUANYANG_TARGET_AVX2   __attribute__((target("avx2")))
#endif
#else
#define YUANYANG_TARGET_AVX2
#endif
#if YUANYANG_FMA
#define YUANYANG_FMADD(a, b, c)   _mm256_fmadd_pd(a, b, c)
#define YUANYANG_FMSUB(a, b, c)   _mm256_fmsub_pd(a, b, c)
#else
#define YUANYANG_FMADD(a, b, c)   _mm256_add_pd(_mm256_mul_pd(a, b), c)
#define YUANYANG_FMSUB(a, b, c)   _mm256_sub_pd(_mm256_mul_pd(a, b), c)
#endif
#endif

#ifndef YUANYANG_TARGET_AVX2
#define YUANYANG_TARGET_AVX2
#endif

/*
 * Shared optimized-build parameter table.  Entries are indexed by logd; the
 * unused lower entries are zero-filled deliberately so code can index directly
 * by YUANYANG_LOGD without a parameter-set switch.
 */
typedef struct {
	uint16_t d;
	uint16_t q;
	uint16_t salt_bytes;
	uint16_t comp_sig_bytes;
	uint8_t sig_low_bits;
	uint16_t rejection_bound;
} yuanyang_opt_param_entry;

static const yuanyang_opt_param_entry yuanyang_opt_params[12] = {
	[9] = {
		512u,
		2689u,
		26u,
		540u,
		5u,
		2337u
	},
	[10] = {
		1024u,
		4481u,
		42u,
		1120u,
		6u,
		4317u
	},
	[11] = {
		2048u,
		7681u,
		72u,
		2300u,
		6u,
		7604u
	}
};

static inline const yuanyang_opt_param_entry *
yuanyang_opt_param_for_logd(unsigned logd)
{
	if (logd >= sizeof yuanyang_opt_params / sizeof yuanyang_opt_params[0]
		|| yuanyang_opt_params[logd].d == 0)
	{
		return NULL;
	}
	return &yuanyang_opt_params[logd];
}

#endif
