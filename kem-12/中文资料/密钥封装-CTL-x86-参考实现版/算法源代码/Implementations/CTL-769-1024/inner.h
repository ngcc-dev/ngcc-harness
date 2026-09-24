
#ifndef CTL_INNER_H__
#define CTL_INNER_H__

/*
 * Internal functions for CTL.
 */

/* ====================================================================== */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "auxfunc.h"

/*
 * Disable warning on applying unary minus on an unsigned type.
 */
#if defined _MSC_VER && _MSC_VER
#pragma warning( disable : 4146 )
#pragma warning( disable : 4244 )
#pragma warning( disable : 4267 )
#pragma warning( disable : 4334 )
#endif

/*
 * Auto-detect 64-bit architectures.
 */
#ifndef CTL_64
#if defined __x86_64__ || defined _M_X64 \
	|| defined __ia64 || defined __itanium__ || defined _M_IA64 \
	|| defined __powerpc64__ || defined __ppc64__ || defined __PPC64__ \
	|| defined __64BIT__ || defined _LP64 || defined __LP64__ \
	|| defined __sparc64__ \
	|| defined __aarch64__ || defined _M_ARM64 \
	|| defined __mips64
#define CTL_64   1
#else
#define CTL_64   0
#endif
#endif

/*
 * Auto-detect endianness and support of unaligned accesses.
 */
#if defined __i386__ || defined _M_IX86 \
	|| defined __x86_64__ || defined _M_X64 \
	|| (defined _ARCH_PWR8 \
		&& (defined __LITTLE_ENDIAN || defined __LITTLE_ENDIAN__))

#ifndef CTL_LE
#define CTL_LE   1
#endif
#ifndef CTL_UNALIGNED
#define CTL_UNALIGNED   1
#endif

#elif (defined __LITTLE_ENDIAN && __LITTLE_ENDIAN__) \
	|| (defined __BYTE_ORDER__ && defined __ORDER_LITTLE_ENDIAN__ \
		&& __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)

#ifndef CTL_LE
#define CTL_LE   1
#endif
#ifndef CTL_UNALIGNED
#define CTL_UNALIGNED   0
#endif

#else

#ifndef CTL_LE
#define CTL_LE   0
#endif
#ifndef CTL_UNALIGNED
#define CTL_UNALIGNED   0
#endif

#endif

/*
 * For seed generation:
 *
 *  - On Linux (glibc-2.25+), FreeBSD 12+ and OpenBSD, use getentropy().
 *  - On other Unix-like systems, use /dev/urandom (also a fallback for
 *    failed getentropy() calls).
 *  - On Windows, use CryptGenRandom().
 */

#ifndef CTL_RAND_GETENTROPY
#if (defined __linux && defined __GLIBC__ \
	&& (__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 25))) \
	|| (defined __FreeBSD__ && __FreeBSD__ >= 12) \
	|| defined __OpenBSD__
#define CTL_RAND_GETENTROPY   1
#else
#define CTL_RAND_GETENTROPY   0
#endif
#endif

#ifndef CTL_RAND_URANDOM
#if defined _AIX \
	|| defined __ANDROID__ \
	|| defined __FreeBSD__ \
	|| defined __NetBSD__ \
	|| defined __OpenBSD__ \
	|| defined __DragonFly__ \
	|| defined __linux__ \
	|| (defined __sun && (defined __SVR4 || defined __svr4__)) \
	|| (defined __APPLE__ && defined __MACH__)
#define CTL_RAND_URANDOM   1
#else
#define CTL_RAND_URANDOM   0
#endif
#endif

#ifndef CTL_RAND_WIN32
#if defined _WIN32 || defined _WIN64
#define CTL_RAND_WIN32   1
#else
#define CTL_RAND_WIN32   0
#endif
#endif

/*
 * Ensure all macros are defined, to avoid warnings with -Wundef.
 */
/*
 * MSVC 2015 does not known the C99 keyword 'restrict'.
 */
#if defined _MSC_VER && _MSC_VER
#ifndef restrict
#define restrict   __restrict
#endif
#endif

/* ====================================================================== */
/*
 * Fixed-point numbers.
 *
 * For FFT and other computations with approximations, we use a fixed-point
 * format over 64 bits; the top 32 bits are the integral part, and the low
 * 32 bits are the fractional part.
 */

/*
 * We wrap the type into a struct in order to detect any attempt at using
 * arithmetic operators on values directly. Since all functions are inline,
 * the compiler will be able to remove the wrapper, which will then have
 * no runtime cost.
 */
typedef struct {
	uint64_t v;
} fnr;

static inline fnr
fnr_of(int32_t j)
{
	fnr x;

	x.v = (uint64_t)j << 32;
	return x;
}

static inline fnr
fnr_of_scaled32(uint64_t t)
{
	fnr x;

	x.v = t;
	return x;
}

static inline fnr
fnr_add(fnr x, fnr y)
{
	x.v += y.v;
	return x;
}

static inline fnr
fnr_sub(fnr x, fnr y)
{
	x.v -= y.v;
	return x;
}

static inline fnr
fnr_double(fnr x)
{
	x.v <<= 1;
	return x;
}

static inline fnr
fnr_neg(fnr x)
{
	x.v = (uint64_t)0 - x.v;
	return x;
}

static inline fnr
fnr_abs(fnr x)
{
	x.v -= (x.v << 1) & -(uint64_t)(x.v >> 63);
	return x;
}

static inline fnr
fnr_mul(fnr x, fnr y)
{
#if defined __GNUC__ && defined __x86_64__
	__int128 z;

	z = (__int128)*(int64_t *)&x.v * (__int128)*(int64_t *)&y.v;
	x.v = (uint64_t)(z >> 32);
	return x;
#else
	int32_t xh, yh;
	uint32_t xl, yl;
	uint64_t z0, z1, z2, z3;

	xl = (uint32_t)x.v;
	yl = (uint32_t)y.v;
	xh = (int32_t)(*(int64_t *)&x.v >> 32);
	yh = (int32_t)(*(int64_t *)&y.v >> 32);
	z0 = ((uint64_t)xl * (uint64_t)yl + 0x80000000ul) >> 32;
	z1 = (uint64_t)((int64_t)xl * (int64_t)yh);
	z2 = (uint64_t)((int64_t)yl * (int64_t)xh);
	z3 = (uint64_t)((int64_t)xh * (int64_t)yh) << 32;
	x.v = z0 + z1 + z2 + z3;
	return x;
#endif
}

static inline fnr
fnr_sqr(fnr x)
{
#if defined __GNUC__ && defined __x86_64__
	int64_t t;
	__int128 z;

	t = *(int64_t *)&x.v;
	z = (__int128)t * (__int128)t;
	x.v = (uint64_t)(z >> 32);
	return x;
#else
	int32_t xh;
	uint32_t xl;
	uint64_t z0, z1, z3;

	xl = (uint32_t)x.v;
	xh = (int32_t)(*(int64_t *)&x.v >> 32);
	z0 = ((uint64_t)xl * (uint64_t)xl + 0x80000000ul) >> 32;
	z1 = (uint64_t)((int64_t)xl * (int64_t)xh);
	z3 = (uint64_t)((int64_t)xh * (int64_t)xh) << 32;
	x.v = z0 + (z1 << 1) + z3;
	return x;
#endif
}

static inline int32_t
fnr_round(fnr x)
{
	x.v += 0x80000000ul;
	return (int32_t)(*(int64_t *)&x.v >> 32);
}

static inline fnr
fnr_div_2e(fnr x, unsigned n)
{
	int64_t v;

	v = *(int64_t *)&x.v;
	x.v = (uint64_t)((v + (((int64_t)1 << n) >> 1)) >> n);
	return x;
}

static inline fnr
fnr_mul_2e(fnr x, unsigned n)
{
	x.v <<= n;
	return x;
}

uint64_t ctl_fnr_div(uint64_t x, uint64_t y);

static inline fnr
fnr_inv(fnr x)
{
	x.v = ctl_fnr_div((uint64_t)1 << 32, x.v);
	return x;
}

static inline fnr
fnr_div(fnr x, fnr y)
{
	x.v = ctl_fnr_div(x.v, y.v);
	return x;
}

static inline int
fnr_lt(fnr x, fnr y)
{
	return *(int64_t *)&x.v < *(int64_t *)&y.v;
}

static const fnr fnr_zero = { 0 };
static const fnr fnr_sqrt2 = { 6074001000ull };

/* ====================================================================== */
/*
 * Apply FFT on a vector.
 */
void ctl_FFT(fnr *f, unsigned logn);

/*
 * Apply inverse FFT on a vector.
 */
void ctl_iFFT(fnr *f, unsigned logn);

/*
 * Add polynomial b to polynomial a (works in FFT and non-FFT). The two
 * polynomial arrays must be distinct.
 */
void ctl_poly_add(fnr *restrict a, const fnr *restrict b, unsigned logn);

/*
 * Subtract polynomial b from polynomial a (works in FFT and non-FFT). The two
 * polynomial arrays must be distinct.
 */
void ctl_poly_sub(fnr *restrict a, const fnr *restrict b, unsigned logn);

/*
 * Negate polynomial a (works in FFT and non-FFT).
 */
void ctl_poly_neg(fnr *a, unsigned logn);

/*
 * Multiply polynomial a by constant c.
 */
void ctl_poly_mulconst(fnr *a, fnr c, unsigned logn);

/*
 * Multiply polynomial a by polynomial b (FFT representation only). The two
 * polynomial arrays must be distinct.
 */
void ctl_poly_mul_fft(fnr *restrict a, const fnr *restrict b, unsigned logn);

/*
 * Compute the adjoint of a polynomial in FFT representation.
 */
void ctl_poly_adj_fft(fnr *a, unsigned logn);

/*
 * Scale a polynomial down by a factor 2^e.
 */
void ctl_poly_div_2e(fnr *a, unsigned e, unsigned logn);

/*
 * Multiply polynomial a by polynomial b (FFT representation only). The two
 * polynomial arrays must be distinct. The polynomial b must be auto-adjoint,
 * i.e. all its coefficients in FFT representation are real numbers (the
 * polynomial has half-length; the imaginary values of the coefficients,
 * assumed to be zero and located in the second half, are not accessed).
 */
void ctl_poly_mul_autoadj_fft(fnr *restrict a,
	const fnr *restrict b, unsigned logn);

/*
 * Divide polynomial a by polynomial b (FFT representation only). The two
 * polynomial arrays must be distinct. The polynomial b must be auto-adjoint,
 * i.e. all its coefficients in FFT representation are real numbers (the
 * polynomial has half-length; the imaginary values of the coefficients,
 * assumed to be zero and located in the second half, are not accessed).
 */
void ctl_poly_div_autoadj_fft(fnr *restrict a,
	const fnr *restrict b, unsigned logn);

/*
 * Compute (2^e)/(a*adj(a)+b*adj(b)) into d[]. Polynomials are in FFT
 * representation. d[] is a half-size polynomial because all FFT
 * coefficients are zero (they are not set by this function). Parameter e
 * can be 0.
 */
void ctl_poly_invnorm_fft(fnr *restrict d,
	const fnr *restrict a, const fnr *restrict b,
	unsigned e, unsigned logn);

/* ====================================================================== */

/*
 * Max size in bits for elements of (f,g), indexed by log(N). Size includes
 * the sign bit.
 */
extern const uint8_t ctl_max_fg_bits[];

/*
 * Max size in bits for elements of (F,G), indexed by log(N). Size includes
 * the sign bit.
 */
extern const uint8_t ctl_max_FG_bits[];

/*
 * Max size in bits for elements of w, indexed by log(N). Size includes
 * the sign bit.
 */
extern const uint8_t ctl_max_w_bits[];

/*
 * Key generation diagnostics: counters for solve_FG / compute_w rejections.
 * These counters are intended for profiling and parameter tuning.
 */
typedef struct {
	uint64_t solve_fg_calls;
	uint64_t solve_fg_success;
	uint64_t solve_fg_fail_bad_param;
	uint64_t solve_fg_fail_deepest;
	uint64_t solve_fg_fail_deepest_make_fg;
	uint64_t solve_fg_fail_deepest_bezout;
	uint64_t solve_fg_fail_deepest_mul_q;
	uint64_t solve_fg_fail_intermediate;
	uint64_t solve_fg_fail_intermediate_bad_param;
	uint64_t solve_fg_fail_intermediate_make_fg;
	uint64_t solve_fg_fail_intermediate_check_pre;
	uint64_t solve_fg_fail_intermediate_check;
	uint64_t solve_fg_fail_intermediate_check_trunc;
	uint64_t solve_fg_fail_intermediate_check_true;
	uint64_t solve_fg_fail_intermediate_check_post;
	uint64_t solve_fg_fail_depth1;
	uint64_t solve_fg_fail_depth0;
	uint64_t solve_fg_fail_poly_small_f;
	uint64_t solve_fg_fail_poly_small_g;
	uint64_t solve_fg_fail_verify;
	uint64_t compute_w_calls;
	uint64_t compute_w_success;
	uint64_t compute_w_fail_bad_param;
	uint64_t compute_w_fail_bounds;
	uint64_t compute_w_fail_lim1;
	uint64_t compute_w_fail_div;
	uint64_t compute_w_fail_fnr_abs;
	uint64_t compute_w_fail_round_range;
	uint64_t compute_w_fail_internal_other;
	uint64_t compute_w_fail_dnorm;
} ctl_keygen_diag_stats;

typedef struct {
	uint32_t valid;
	uint32_t n;
	uint32_t p;
	uint32_t iter;
	int32_t k_raw[8];
	uint32_t f[8];
	uint32_t g[8];
	uint32_t F_pre[8];
	uint32_t G_pre[8];
	uint32_t F_post[8];
	uint32_t G_post[8];
	uint32_t k_modp[8];
	uint32_t kf[8];
	uint32_t kg[8];
	uint32_t pre_gF[8];
	uint32_t pre_fG[8];
	uint32_t post_gF[8];
	uint32_t post_fG[8];
	uint32_t delta_gkf[8];
	uint32_t delta_fkg[8];
} ctl_stage3_round_diag;

typedef struct {
	uint32_t valid;
	uint32_t reason;
	uint32_t iter;
	uint32_t slen;
	uint32_t sstride;
	uint32_t llen;
	uint32_t fglen;
	uint32_t rlen_cur;
	uint32_t scale_fg;
	uint32_t scale_FG;
	uint32_t scale_FG_eff;
	uint32_t scale_k;
	uint32_t scale_x;
	uint32_t scale_t;
	uint32_t reduce_bits;
	uint32_t tlen;
	uint32_t toff;
	uint32_t k_nonzero_count;
	uint32_t top_word_changed;
	uint32_t top_word_change_count;
	uint32_t top_word_change_first_index;
	uint32_t top_word_change_ft_pre;
	uint32_t top_word_change_ft_post;
	uint32_t top_word_change_gt_pre;
	uint32_t top_word_change_gt_post;
	uint32_t can_shrink;
	uint32_t blocker_count;
	uint32_t blocker_index;
	uint32_t ft_hi;
	uint32_t ft_lo;
	uint32_t gt_hi;
	uint32_t gt_lo;
	uint32_t ft_pre_hi;
	uint32_t ft_pre_lo;
	uint32_t gt_pre_hi;
	uint32_t gt_pre_lo;
	uint32_t ft_delta_hi;
	uint32_t ft_delta_lo;
	uint32_t gt_delta_hi;
	uint32_t gt_delta_lo;
	uint32_t ft_target_hi_pre;
	uint32_t ft_target_hi_post;
	uint32_t gt_target_hi_pre;
	uint32_t gt_target_hi_post;
	uint32_t ft_gap_pre;
	uint32_t ft_gap_post;
	uint32_t gt_gap_pre;
	uint32_t gt_gap_post;
	int32_t ft_gap_progress;
	int32_t gt_gap_progress;
	uint32_t blocker_rt2_lo;
	uint32_t blocker_rt2_hi;
	int32_t blocker_rt2_round;
	int32_t blocker_k_value;
	uint32_t snap_n;
	int32_t k_snapshot[8];
	int32_t rt2_round_snapshot[8];
	uint32_t unit_snap_n;
	int32_t blocker_unit_ft_hi[8];
	int32_t blocker_unit_ft_lo[8];
	int32_t blocker_unit_gt_hi[8];
	int32_t blocker_unit_gt_lo[8];
	uint32_t current_flen;
	uint32_t other_flen;
	uint32_t other_rlen_cur;
	uint32_t other_scale_fg;
	uint32_t other_scale_k;
	uint32_t other_ft_hi;
	uint32_t other_ft_lo;
	uint32_t other_gt_hi;
	uint32_t other_gt_lo;
	uint32_t other_ft_delta_hi;
	uint32_t other_ft_delta_lo;
	uint32_t other_gt_delta_hi;
	uint32_t other_gt_delta_lo;
	uint32_t other_ft_gap_post;
	uint32_t other_gt_gap_post;
	int32_t other_ft_gap_progress;
	int32_t other_gt_gap_progress;
	uint32_t other_strong_progress;
	uint32_t other_direct_modp_ok;
	uint32_t other_exact_ok;
	uint32_t other_exact_modp_ok;
	uint32_t cur_exact_ok;
	uint32_t cur_exact_modp_ok;
	uint32_t probe_exact_ok;
	uint32_t probe_exact_modp_ok;
	uint64_t other_exact_gap_post;
	uint64_t cur_exact_gap_post;
	uint64_t probe_exact_gap_post;
	uint32_t other_exact_bad_index;
	uint32_t other_exact_expected;
	uint32_t other_exact_actual;
	uint32_t cur_exact_bad_index;
	uint32_t cur_exact_expected;
	uint32_t cur_exact_actual;
	uint32_t probe_exact_bad_index;
	uint32_t probe_exact_expected;
	uint32_t probe_exact_actual;
	uint32_t direct_valid;
	uint32_t direct_snap_n;
	uint32_t direct_diff_count;
	uint32_t direct_max_diff;
	int32_t direct_k_snapshot[8];
	uint32_t probe_attempted;
	uint32_t probe_selected;
	uint32_t probe_flen;
	uint32_t probe_scale_fg;
	uint32_t probe_scale_k;
	uint32_t probe_scale_x;
	uint32_t probe_scale_t;
	uint32_t probe_tlen;
	uint32_t probe_toff;
	uint32_t cur_rt3_lo;
	uint32_t cur_rt3_hi;
	uint32_t cur_rt4_lo;
	uint32_t cur_rt4_hi;
	uint32_t cur_rt5_lo;
	uint32_t cur_rt5_hi;
	uint32_t probe_rt3_lo;
	uint32_t probe_rt3_hi;
	uint32_t probe_rt4_lo;
	uint32_t probe_rt4_hi;
	uint32_t probe_rt5_lo;
	uint32_t probe_rt5_hi;
	uint32_t probe_k_nonzero;
	uint32_t probe_snap_n;
	int32_t probe_k_snapshot[8];
	uint32_t probe_rt2_lo[8];
	uint32_t probe_rt2_hi[8];
	uint64_t probe_cur_gap;
	uint64_t probe_alt_gap;
	uint32_t tail_no_update;
} ctl_tail_exit_diag;

void ctl_keygen_diag_reset(void);
void ctl_keygen_diag_get(ctl_keygen_diag_stats *dst);
void ctl_debug_get_last_intermediate_fail(
	uint32_t *depth, uint32_t *stage, uint32_t *index);
void ctl_debug_get_last_intermediate_sizes(
	uint32_t *slen, uint32_t *fglen);
void ctl_debug_get_last_modp_mismatch(
	uint32_t *expected, uint32_t *actual);
void ctl_debug_get_last_modp_mismatch_count(uint32_t *count);
void ctl_debug_get_last_modp_profile(
	uint32_t *eq_first_count, uint32_t *vmin, uint32_t *vmax);
void ctl_debug_get_last_modp_context(
	uint32_t *p, uint32_t *slen, uint32_t *llen,
	uint32_t *chklen, uint32_t *logn);
void ctl_debug_get_modp_crosscheck_stats(
	uint64_t *crosscheck_count,
	uint64_t *ntt_false_positive_count,
	uint32_t *last_direct_ok);
void ctl_debug_get_last_poly_small_i16_f_stats(
	uint32_t *max_abs, uint32_t *over_count,
	uint32_t *first_index, int32_t *first_value);
void ctl_debug_get_last_poly_small_i16_g_stats(
	uint32_t *max_abs, uint32_t *over_count,
	uint32_t *first_index, int32_t *first_value);
void ctl_debug_get_last_poly_small_i16_f_tail_exits(
	ctl_tail_exit_diag *d7, ctl_tail_exit_diag *d8, ctl_tail_exit_diag *d9);
void ctl_debug_get_first_stage3_fail_snapshot(
	uint32_t *depth, uint32_t *logn, uint32_t *p,
	uint32_t *slen, uint32_t *llen, uint32_t *chklen,
	uint32_t *bad_index, uint32_t *expected, uint32_t *actual,
	uint32_t *mismatch_count);
void ctl_debug_get_first_stage3_fail_terms(
	uint32_t *term_fG, uint32_t *term_gF);
void ctl_debug_get_first_stage3_fail_scales(
	uint32_t *iter, uint32_t *fglen,
	uint32_t *scale_fg, uint32_t *scale_FG, uint32_t *scale_k);
void ctl_debug_get_first_depth8_post_update_fail(
	uint32_t *iter, uint32_t *fglen, uint32_t *bad_index,
	uint32_t *expected, uint32_t *actual, uint32_t *mismatch_count,
	uint32_t *had_update, uint32_t *scale_k);
void ctl_debug_get_first_depth8_post_update_kstats(
	uint32_t *nonzero_count, uint32_t *max_abs,
	uint32_t *first_nz_index, int32_t *first_nz_value,
	int32_t *bad_k_value, uint32_t *rt2_lo, uint32_t *rt2_hi,
	int32_t *rt2_round);
void ctl_debug_get_first_depth8_post_update_chain(
	uint32_t *scale_x, uint32_t *scale_t,
	uint32_t *tlen, uint32_t *toff,
	uint32_t *rt1_lo, uint32_t *rt1_hi,
	uint32_t *rt3_lo, uint32_t *rt3_hi,
	uint32_t *rt4_lo, uint32_t *rt4_hi,
	uint32_t *rt5_lo, uint32_t *rt5_hi);
void ctl_debug_get_first_scale_k_zero_snapshot(
	uint32_t *depth, uint32_t *logn, uint32_t *iter,
	uint32_t *rlen, uint32_t *slen, uint32_t *fglen,
	uint32_t *scale_fg, uint32_t *scale_FG, uint32_t *scale_k);
void ctl_debug_get_scale_k_zero_hist(uint64_t *dst, size_t dst_len);
void ctl_debug_get_first_stage3_fail_limbs(
	uint32_t *ft_lo, uint32_t *ft_hi,
	uint32_t *gt_lo, uint32_t *gt_hi,
	uint32_t *Ft_lo, uint32_t *Ft_hi,
	uint32_t *Gt_lo, uint32_t *Gt_hi);
void ctl_debug_get_first_stage3_fail_round_diag(
	ctl_stage3_round_diag *dst);
void ctl_debug_get_observed_bitlengths(
	uint32_t *small_bits, uint32_t *large_bits, size_t len);
void ctl_debug_get_d10_iter_stats(
	uint64_t *iter_count, uint64_t *k_zero_count, uint64_t *k_nonzero_count);
void ctl_debug_get_d10_top_stats(
	uint64_t *top_changed, uint64_t *top_unchanged);
void ctl_debug_get_d9_iter_stats(
	uint64_t *iter_count, uint64_t *k_zero_count, uint64_t *k_nonzero_count);
void ctl_debug_get_d9_top_stats(
	uint64_t *top_changed, uint64_t *top_unchanged);
void ctl_debug_get_d9_fglen_stats(
	uint64_t *shrink_count, uint32_t *min_fglen);
void ctl_debug_get_d7_tail_exit(ctl_tail_exit_diag *dst);
void ctl_debug_get_d7_update_stall_stats(
	uint64_t *update_no_shrink,
	uint64_t *update_and_shrink,
	uint64_t *update_no_shrink_top_same);
void ctl_debug_get_d8_tail_exit(ctl_tail_exit_diag *dst);
void ctl_debug_get_d9_tail_exit(ctl_tail_exit_diag *dst);
void ctl_debug_get_d8_update_stall_stats(
	uint64_t *update_no_shrink,
	uint64_t *update_and_shrink,
	uint64_t *update_no_shrink_top_same);
void ctl_debug_get_d9_update_stall_stats(
	uint64_t *update_no_shrink,
	uint64_t *update_and_shrink,
	uint64_t *update_no_shrink_top_same);
void ctl_debug_get_first_d9_bad_pre(
	uint32_t *iter, uint32_t *bad_index);
void ctl_debug_get_first_d9_bad_post(
	uint32_t *iter, uint32_t *bad_index,
	ctl_stage3_round_diag *round);
void ctl_debug_get_d6_iter_stats(
	uint64_t *iter_count, uint64_t *k_zero_count, uint64_t *k_nonzero_count);
void ctl_debug_get_d6_top_stats(
	uint64_t *top_changed, uint64_t *top_unchanged);
void ctl_debug_get_d6_fglen_stats(
	uint64_t *shrink_count, uint32_t *min_fglen, uint64_t *tail_boost_count);
void ctl_debug_get_d5_iter_stats(
	uint64_t *iter_count, uint64_t *k_zero_count, uint64_t *k_nonzero_count);
void ctl_debug_get_d5_top_stats(
	uint64_t *top_changed, uint64_t *top_unchanged);
void ctl_debug_get_d5_fglen_stats(
	uint64_t *shrink_count, uint32_t *min_fglen);
void ctl_debug_get_d4_iter_stats(
	uint64_t *iter_count, uint64_t *k_zero_count, uint64_t *k_nonzero_count);
void ctl_debug_get_d4_top_stats(
	uint64_t *top_changed, uint64_t *top_unchanged);
void ctl_debug_get_d2_iter_stats(
	uint64_t *iter_count, uint64_t *k_zero_count, uint64_t *k_nonzero_count);
void ctl_debug_get_d2_top_stats(
	uint64_t *top_changed, uint64_t *top_unchanged);
void ctl_debug_get_d2_kabs_stats(
	uint64_t *sum_abs, uint64_t *sample_count, uint32_t *max_abs);
void ctl_debug_get_d2_last_state(
	uint32_t *iter, uint32_t *rlen, uint32_t *slen, uint32_t *fglen,
	uint32_t *scale_fg, uint32_t *scale_FG, uint32_t *scale_k);
void ctl_debug_get_d9_shrink_block_stats(
	uint64_t *events, uint64_t *total_bad, uint32_t *last_bad);
void ctl_debug_get_d9_shrink_block_sample(
	uint32_t *index,
	uint32_t *ft_hi, uint32_t *ft_lo,
	uint32_t *gt_hi, uint32_t *gt_lo);
void ctl_debug_get_d6_shrink_block_stats(
	uint64_t *events, uint64_t *total_bad, uint32_t *last_bad);
void ctl_debug_get_d6_shrink_block_sample(
	uint32_t *index,
	uint32_t *ft_hi, uint32_t *ft_lo,
	uint32_t *gt_hi, uint32_t *gt_lo);
void ctl_debug_get_d5_shrink_block_stats(
	uint64_t *events, uint64_t *total_bad, uint32_t *last_bad);
void ctl_debug_get_d5_shrink_block_sample(
	uint32_t *index,
	uint32_t *ft_hi, uint32_t *ft_lo,
	uint32_t *gt_hi, uint32_t *gt_lo);
void ctl_debug_get_d4_shrink_block_stats(
	uint64_t *events, uint64_t *total_bad, uint32_t *last_bad);
void ctl_debug_get_d4_shrink_block_sample(
	uint32_t *index,
	uint32_t *ft_hi, uint32_t *ft_lo,
	uint32_t *gt_hi, uint32_t *gt_lo);
void ctl_debug_get_d2_shrink_block_stats(
	uint64_t *events, uint64_t *total_bad, uint32_t *last_bad);
void ctl_debug_get_d2_shrink_block_sample(
	uint32_t *index,
	uint32_t *ft_hi, uint32_t *ft_lo,
	uint32_t *gt_hi, uint32_t *gt_lo);
void ctl_debug_get_compute_w_lim1_stats(
	uint32_t *last_v1_abs, uint32_t *last_lim1, uint32_t *max_v1_abs);
int ctl_debug_deepest_resultants(uint32_t q, unsigned logn,
	const int8_t *f, const int8_t *g, uint32_t *tmp,
	uint32_t *fp_lo, uint32_t *gp_lo, uint32_t *fp_hi, uint32_t *gp_hi);
int ctl_debug_deepest_mulq(uint32_t q, unsigned logn,
	const int8_t *f, const int8_t *g, uint32_t *tmp,
	uint32_t *carry_f, uint32_t *carry_g,
	uint32_t *f_hi_before, uint32_t *g_hi_before,
	uint32_t *f_hi_after, uint32_t *g_hi_after);

/* ====================================================================== */

/*
 * Key pair generation, first step: given a seed, candidate polynomials
 * f and g are generated. The following properties are checked:
 *  - All coefficients of f and g are within the expected bounds.
 *  - Res(f, x^n+1) == 1 mod 2.
 *  - Res(g, x^n+1) == 1 mod 2.
 *  - The (f,g) vector has an acceptable norm, both in normal and in
 *    orthogonalized representations.
 *  - f is invertible modulo x^n+1 modulo q.
 * If any of these properties is not met, then a failure is reported
 * (returned value is 0) and the contents of f[] and g[] are indeterminate.
 * Otherwise, success (1) is returned.
 *
 * If h != NULL, then the public key h = g/f mod x^n+1 mod q is returned
 * in that array. Note that h is always internally computed, regardless
 * of whether h == NULL or not.
 *
 * Size of tmp[]: 6*n elements (24*n bytes).
 * tmp[] MUST be 64-bit aligned.
 *
 * The seed length MUST NOT exceed 48 bytes.
 */
int ctl_keygen_make_fg(int8_t *f, int8_t *g, uint16_t *h,
	uint32_t q, unsigned logn,
	const void *seed, size_t seed_len, uint32_t *tmp);

/*
 * Given polynomials f and g, solve the NTRU equation for F and G. This
 * may fail if there is no solution, or if some intermediate value exceeds
 * an internal heuristic threshold. Returned value is 1 on success, 0
 * on failure. On failure, contents of F and G are indeterminate.
 *
 * Size of tmp[]: 6*n elements (24*n bytes).
 * tmp[] MUST be 64-bit aligned.
 */
int ctl_keygen_solve_FG(int8_t *F, int8_t *G,
	const int8_t *f, const int8_t *g,
	uint32_t q, unsigned logn, uint32_t *tmp);

/*
 * Given polynomials f, g and F, rebuild the polynomial G that completes
 * the NTRU equation g*F - f*G = q. Returned value is 1 on success, 0 on
 * failure. A failure is reported if the rebuilt solution has
 * coefficients outside of the expected maximum range, or f is not
 * invertible modulo x^n+1 modulo q. This function does NOT fully verify
 * that f, g, F, G is a solution to the NTRU equation.
 *
 * Size of tmp[]: n elements (4*n bytes).
 */
int ctl_keygen_rebuild_G(int8_t *G,
	const int8_t *f, const int8_t *g, const int8_t *F,
	uint32_t q, unsigned logn, uint32_t *tmp);

/*
 * Verify that the given f, g, F, G fulfill the NTRU equation g*F - f*G = q.
 * Returned value is 1 on success, 0 on error.
 *
 * This function may be called when decoding a private key of unsure
 * provenance. It is implicitly called by ctl_keygen_solve_FG().
 *
 * Size of tmp[]: 4*n elements (16*n bytes).
 */
int ctl_keygen_verify_FG(
	const int8_t *f, const int8_t *g, const int8_t *F, const int8_t *G,
	uint32_t q, unsigned logn, uint32_t *tmp);

/*
 * Compute the w vector. Returned value is 1 on success, 0 on error. An
 * error is reported if the w vector has coefficients that do not fit
 * in signed 16-bit integer, or if the norm of (gamma*F_d, G_d) exceeds
 * the prescribed limit.
 *
 * Size of tmp[]: 6*n elements (24*n bytes).
 * tmp[] MUST be 64-bit aligned.
 */
int ctl_keygen_compute_w(int32_t *w,
	const int8_t *f, const int8_t *g, const int8_t *F, const int8_t *G,
	uint32_t q, unsigned logn, uint32_t *tmp);

/*
 * 3329-specific variants that accept F and G on int16_t.
 */
int ctl_keygen_solve_FG_3329(int16_t *F, int16_t *G,
	const int8_t *f, const int8_t *g, unsigned logn, uint32_t *tmp);
int ctl_keygen_rebuild_G_3329_i16(int16_t *G,
	const int8_t *f, const int8_t *g, const int16_t *F,
	unsigned logn, uint32_t *tmp);
int ctl_keygen_verify_FG_3329_i16(
	const int8_t *f, const int8_t *g, const int16_t *F, const int16_t *G,
	unsigned logn, uint32_t *tmp);
int ctl_keygen_compute_w_3329(int32_t *w,
	const int8_t *f, const int8_t *g, const int16_t *F, const int16_t *G,
	unsigned logn, uint32_t *tmp);

/*
 * Compute the public key h = g/f. Returned value is 1 on success, 0 on
 * error. An error is reported if f is not invertible modulo X^n+1.
 * This function is for q = 257 and 1 <= logn <= 9.
 *
 * Size of tmp[]: n elements (4*n bytes).
 */
int ctl_make_public_257(uint16_t *h, const int8_t *f, const int8_t *g,
	unsigned logn, uint32_t *tmp);

/*
 * Compute the public key h = g/f. Returned value is 1 on success, 0 on
 * error. An error is reported if f is not invertible modulo X^n+1.
 * This function is for q = 769 and 1 <= logn <= 10.
 *
 * Size of tmp[]: n elements (4*n bytes).
 */
int ctl_make_public_769(uint16_t *h, const int8_t *f, const int8_t *g,
	unsigned logn, uint32_t *tmp);

/*
 * Compute the public key h = g/f. Returned value is 1 on success, 0 on
 * error. An error is reported if f is not invertible modulo X^n+1.
 * This function is for q = 3329 and 1 <= logn <= 11.
 *
 * Size of tmp[]: n elements (4*n bytes).
 */
int ctl_make_public_3329(uint16_t *h, const int8_t *f, const int8_t *g,
	unsigned logn, uint32_t *tmp);

/*
 * Given f, g and F, rebuild G, for the case q = 257. This function
 * reports a failure if (q,logn) are not supported parameters, if f is
 * not invertible modulo x^n+1 and modulo q, or if the rebuilt value G
 * has coefficients that exceed the expected maximum size.
 *
 * This function does NOT check that the returned G matches the NTRU
 * equation.
 *
 * Size of tmp[]: n elements (4*n bytes).
 */
int ctl_rebuild_G_257(int8_t *G,
	const int8_t *f, const int8_t *g, const int8_t *F,
	unsigned logn, uint32_t *tmp);

/*
 * Given f, g and F, rebuild G, for the case q = 769. This function
 * reports a failure if (q,logn) are not supported parameters, if f is
 * not invertible modulo x^n+1 and modulo q, or if the rebuilt value G
 * has coefficients that exceed the expected maximum size.
 *
 * This function does NOT check that the returned G matches the NTRU
 * equation.
 *
 * Size of tmp[]: n elements (4*n bytes).
 */
int ctl_rebuild_G_769(int8_t *G,
	const int8_t *f, const int8_t *g, const int8_t *F,
	unsigned logn, uint32_t *tmp);

/*
 * Given f, g and F, rebuild G, for the case q = 3329. This function
 * reports a failure if (q,logn) are not supported parameters, if f is
 * not invertible modulo x^n+1 and modulo q, or if the rebuilt value G
 * has coefficients that exceed the expected maximum size.
 *
 * This function does NOT check that the returned G matches the NTRU
 * equation.
 *
 * Size of tmp[]: n elements (4*n bytes).
 */
int ctl_rebuild_G_3329_i16(int16_t *G,
	const int8_t *f, const int8_t *g, const int16_t *F,
	unsigned logn, uint32_t *tmp);

/* ====================================================================== */

/*
 * Get the length of sbuf, for a given degree n, with n = 2^logn.
 * The logn parameter must be between 1 and 11, inclusive. Returned length
 * is in bytes, between 1 and 256, inclusive.
 */
#define SBUF_LEN(logn)   (((1 << (logn)) + 7) >> 3)

/*
 * Encrypt: given public key (in h) and secret polynomial s (in sbuf[]),
 * produce ciphertext c1 (in c).
 *
 * This function is for q = 257, with logn = 1 to 9. Ciphertext elements
 * are in the -64..+64 range. The function cannot fail, hence it always
 * returns 1.
 *
 * Size of tmp[]: n elements (4*n bytes).
 */
uint32_t ctl_encrypt_257(int8_t *c, const uint8_t *sbuf,
	const uint16_t *h, unsigned logn, uint32_t *tmp);

/*
 * Encrypt: given public key (in h) and secret polynomial s (in sbuf[]),
 * produce ciphertext c1 (in c).
 *
 * This function is for q = 769, with logn = 1 to 10. Ciphertext elements
 * are in the -96..+96 range.
 *
 * The function may fail, if the norm of the result is too high, in which
 * case the caller should start again with a new seed (this is uncommon).
 * On failure, this function returns 0; on success, it returns 1.
 *
 * Size of tmp[]: 3*n/4 elements (3*n bytes).
 */
uint32_t ctl_encrypt_769(int8_t *c, const uint8_t *sbuf,
	const uint16_t *h, unsigned logn, uint32_t *tmp);

/*
 * Encrypt: given public key (in h) and secret polynomial s (in sbuf[]),
 * produce ciphertext c1 (in c).
 *
 * This function is for q = 3329, with logn = 11. Ciphertext elements
 * are expected to fit within the -208..+208 range, so int16_t is used.
 *
 * The function may fail, if the norm of the result is too high, in which
 * case the caller should start again with a new seed (this is uncommon).
 * On failure, this function returns 0; on success, it returns 1.
 *
 * Size of tmp[]: n elements (4*n bytes).
 */
uint32_t ctl_encrypt_3329(int16_t *c, const uint8_t *sbuf,
	const uint16_t *h, unsigned logn, uint32_t *tmp);

/*
 * Decrypt: given private key (f,g,F,G,w) and ciphertext c1, extract
 * secret s. The polynomial s has length n bits (with n = 2^logn); it
 * is returned in sbuf[] (ceil(n/8) bytes; for toy versions with logn <
 * 3, the upper bits of the incomplete byte are set to zero).
 *
 * This function is for q = 257. Ciphertext elements are in the -64..+64
 * range.
 *
 * Size of tmp[]: 2*n elements (8*n bytes).
 *
 * This function never fails; for proper security, the caller must obtain
 * the message m (using the second ciphertext element c2) and check that
 * encryption of m would indeed yield exactly ciphertext c1.
 */
void ctl_decrypt_257(uint8_t *sbuf, const int8_t *c,
	const int8_t *f, const int8_t *g, const int8_t *F, const int8_t *G,
	const int32_t *w, unsigned logn, uint32_t *tmp);

/*
 * Decrypt: given private key (f,g,F,G,w) and ciphertext c1, extract
 * secret s. The polynomial s has length n bits (with n = 2^logn); it
 * is returned in sbuf[] (ceil(n/8) bytes; for toy versions with logn <
 * 3, the upper bits of the incomplete byte are set to zero).
 *
 * This function is for q = 769. Ciphertext elements are in the -96..+96
 * range.
 *
 * Size of tmp[]: 2*n elements (8*n bytes).
 *
 * This function never fails; for proper security, the caller must obtain
 * the message m (using the second ciphertext element c2) and check that
 * encryption of m would indeed yield exactly ciphertext c1.
 */
void ctl_decrypt_769(uint8_t *sbuf, const int8_t *c,
	const int8_t *f, const int8_t *g, const int8_t *F, const int8_t *G,
	const int32_t *w, unsigned logn, uint32_t *tmp);

/*
 * Decrypt: given private key (f,g,F,G,w) and ciphertext c1, extract
 * secret s. The polynomial s has length n bits (with n = 2^logn); it
 * is returned in sbuf[] (ceil(n/8) bytes; for toy versions with logn <
 * 3, the upper bits of the incomplete byte are set to zero).
 *
 * This function is for q = 3329. Ciphertext elements exceed the int8_t
 * range, so int16_t is used.
 *
 * Size of tmp[]: 2*n elements (8*n bytes).
 *
 * This function never fails; for proper security, the caller must obtain
 * the message m (using the second ciphertext element c2) and check that
 * encryption of m would indeed yield exactly ciphertext c1.
 */
void ctl_decrypt_3329(uint8_t *sbuf, const int16_t *c,
	const int8_t *f, const int8_t *g, const int16_t *F, const int16_t *G,
	const int32_t *w, unsigned logn, uint32_t *tmp);

/*
 * Second phase of decapsulation, performed modulo 769.
 * Given c', c'', f, F and w, this function computes:
 *    Fd = q'*F - f*w
 *    q*q'*Q*s' = Fd*c' - f*c''
 *
 * On input, cp[] and cs[] must contain c' and c'', respectively, in
 * Montgomery representation modulo 769. On output, polynomial q*q'*Q*s'
 * is returned in cp[], in Montgomery representation modulo 769 (since
 * coefficients of s' can have only a few specific values, this is enough
 * to recover s'). cs[] is consumed. tmp[] must have room for 4*n bytes
 * (n 32-bit elements).
 *
 * Size of tmp[]: n elements (4*n bytes).
 */
void ctl_finish_decapsulate_769(uint16_t *cp, uint16_t *cs,
	const int8_t *f, const int8_t *F, const int32_t *w, unsigned logn,
	uint32_t *tmp);

/*
 * Second phase of decapsulation, performed modulo 257.
 * Given c', c'', f, F and w, this function computes:
 *    Fd = q'*F - f*w
 *    q*q'*Q*s' = Fd*c' - f*c''
 *
 * On input, cp[] and cs[] must contain c' and c'', respectively, in
 * Montgomery representation modulo 257. On output, polynomial q*q'*Q*s'
 * is returned in cp[], in Montgomery representation modulo 257 (since
 * coefficients of s' can have only a few specific values, this is enough
 * to recover s'). cs[] is consumed. tmp[] must have room for 4*n bytes
 * (n 32-bit elements).
 *
 * Size of tmp[]: n elements (4*n bytes).
 */
void ctl_finish_decapsulate_257(uint16_t *cp, uint16_t *cs,
	const int8_t *f, const int8_t *F, const int32_t *w, unsigned logn,
	uint32_t *tmp);
void ctl_finish_decapsulate_257_i16(uint16_t *cp, uint16_t *cs,
	const int8_t *f, const int16_t *F, const int32_t *w, unsigned logn,
	uint32_t *tmp);

/*
 * Second phase of decapsulation, performed modulo 3329.
 * Given c', c'', f, F and w, this function computes:
 *    Fd = q'*F - f*w
 *    q*q'*Q*s' = Fd*c' - f*c''
 *
 * On input, cp[] and cs[] must contain c' and c'', respectively, in
 * Montgomery representation modulo 3329. On output, polynomial q*q'*Q*s'
 * is returned in cp[], in Montgomery representation modulo 3329 (since
 * coefficients of s' can have only a few specific values, this is enough
 * to recover s'). cs[] is consumed. tmp[] must have room for 4*n bytes
 * (n 32-bit elements).
 *
 * Size of tmp[]: n elements (4*n bytes).
 */
void ctl_finish_decapsulate_3329(uint16_t *cp, uint16_t *cs,
	const int8_t *f, const int16_t *F, const int32_t *w, unsigned logn,
	uint32_t *tmp);

/*
 * Explicit reduction and conversion to Montgomery representation modulo
 * 257. This works for inputs x in range 0..4278190336.
 */
static inline uint32_t
m257_tomonty(uint32_t x)
{
	x *= 16711935;
	x = (x >> 16) * 257;
	return (x >> 16) + 1;
}

/*
 * Explicit reduction and conversion to Montgomery representation modulo
 * 769. This works for inputs x in range 0..4244636416.
 */
static inline uint32_t
m769_tomonty(uint32_t x)
{
	x *= 452395775;
	x = (x >> 16) * 769;
	x = (x >> 16) + 1;
	x *= 2016233021;
	x = (x >> 16) * 769;
	return (x >> 16) + 1;
}

/*
 * Explicit reduction and conversion to Montgomery representation modulo
 * 3329. 1353 is 2^32 mod 3329. Since 3329 is small, we can safely compute 
 * it using standard modulo arithmetic without overflow. Modern compilers
 * optimize this `% 3329` perfectly.
 */
static inline uint32_t
m3329_tomonty(uint32_t x)
{
	uint32_t r = x % 3329;
	return (r * 1353) % 3329;
}

/* ====================================================================== */
/*
 * Computations on polynomials modulo q' = 64513.
 */

/*
 * Compute d = -a*b mod X^n+1 mod q'
 * Coefficients of source values are plain integers (for value b, they must
 * be in the -503109..+503109 range). Coefficients of output values are
 * normalized in -32256..+32256.
 *
 * Array d[] may overlap, partially or totally, with a[]; however, it
 * MUST NOT overlap with b[].
 *
 * Size of tmp[]: n/2 elements (2*n bytes).
 */
void ctl_polyqp_mulneg(int16_t *d, const int16_t *a, const int32_t *b,
	unsigned logn, uint32_t *tmp);

/* ====================================================================== */
/*
 * Encoding/decoding functions.
 */

#if CTL_LE && CTL_UNALIGNED

static inline unsigned
dec16le(const void *src)
{
	return *(const uint16_t *)src;
}

static inline void
enc16le(void *dst, unsigned x)
{
	*(uint16_t *)dst = x;
}

static inline uint32_t
dec32le(const void *src)
{
	return *(const uint32_t *)src;
}

static inline void
enc32le(void *dst, uint32_t x)
{
	*(uint32_t *)dst = x;
}

static inline uint64_t
dec64le(const void *src)
{
	return *(const uint64_t *)src;
}

static inline void
enc64le(void *dst, uint64_t x)
{
	*(uint64_t *)dst = x;
}

#else

static inline unsigned
dec16le(const void *src)
{
	const uint8_t *buf;

	buf = src;
	return (unsigned)buf[0]
		| ((unsigned)buf[1] << 8);
}

static inline void
enc16le(void *dst, unsigned x)
{
	uint8_t *buf;

	buf = dst;
	buf[0] = (uint8_t)x;
	buf[1] = (uint8_t)(x >> 8);
}

static inline uint32_t
dec32le(const void *src)
{
	const uint8_t *buf;

	buf = src;
	return (uint32_t)buf[0]
		| ((uint32_t)buf[1] << 8)
		| ((uint32_t)buf[2] << 16)
		| ((uint32_t)buf[3] << 24);
}

static inline void
enc32le(void *dst, uint32_t x)
{
	uint8_t *buf;

	buf = dst;
	buf[0] = (uint8_t)x;
	buf[1] = (uint8_t)(x >> 8);
	buf[2] = (uint8_t)(x >> 16);
	buf[3] = (uint8_t)(x >> 24);
}

static inline uint64_t
dec64le(const void *src)
{
	const uint8_t *buf;

	buf = src;
	return (uint64_t)buf[0]
		| ((uint64_t)buf[1] << 8)
		| ((uint64_t)buf[2] << 16)
		| ((uint64_t)buf[3] << 24)
		| ((uint64_t)buf[4] << 32)
		| ((uint64_t)buf[5] << 40)
		| ((uint64_t)buf[6] << 48)
		| ((uint64_t)buf[7] << 56);
}

static inline void
enc64le(void *dst, uint64_t x)
{
	uint8_t *buf;

	buf = dst;
	buf[0] = (uint64_t)x;
	buf[1] = (uint64_t)(x >> 8);
	buf[2] = (uint64_t)(x >> 16);
	buf[3] = (uint64_t)(x >> 24);
	buf[4] = (uint64_t)(x >> 32);
	buf[5] = (uint64_t)(x >> 40);
	buf[6] = (uint64_t)(x >> 48);
	buf[7] = (uint64_t)(x >> 56);
}

#endif

static inline uint32_t
dec24le(const void *src)
{
	const uint8_t *buf;

	buf = src;
	return (uint32_t)buf[0]
		| ((uint32_t)buf[1] << 8)
		| ((uint32_t)buf[2] << 16);
}

static inline void
enc24le(void *dst, uint32_t x)
{
	uint8_t *buf;

	buf = dst;
	buf[0] = (uint8_t)x;
	buf[1] = (uint8_t)(x >> 8);
	buf[2] = (uint8_t)(x >> 16);
}

/*
 * ctl_trim_i32_encode() and ctl_trim_i32_decode() encode and decode
 * polynomials with signed coefficients (int32_t), using the specified
 * number of bits for each coefficient. The number of bits includes the
 * sign bit. Each coefficient x must be such that |x| < 2^(bits-1) (the
 * value -2^(bits-1), though conceptually encodable with two's
 * complement representation, is forbidden).
 *
 * ctl_trim_i8_encode() and ctl_trim_i8_decode() do the same work for
 * polynomials whose coefficients are held in slots of type int8_t.
 *
 * Encoding API:
 *
 *   Output buffer (out[]) has max length max_out_len (in bytes). If
 *   that length is not large enough, then no encoding occurs and the
 *   function returns 0; otherwise, the function returns the number of
 *   bytes which have been written into out[]. If out == NULL, then
 *   max_out_len is ignored, and no output is produced, but the function
 *   returns how many bytes it would produce.
 *
 *   Encoding functions assume that the input is valid (all values in
 *   the encodable range).
 *
 * Decoding API:
 *
 *   Input buffer (in[]) has maximum length max_in_len (in bytes). If
 *   the input length is not enough for the expected polynomial, then
 *   no decoding occurs and the function returns 0. Otherwise, the values
 *   are decoded and the number of processed input bytes is returned.
 *
 *   If the input is invalid in some way (a decoded coefficient has
 *   value -2^(bits-1), or some of the ignored bits in the last byte
 *   are non-zero), then the function fails and returns 0; the contents
 *   of the output array are then indeterminate.
 *
 * Both encoding and decoding are constant-time with regards to the
 * values and bits.
 */

size_t ctl_trim_i32_encode(void *out, size_t max_out_len,
	const int32_t *x, unsigned logn, unsigned bits);
size_t ctl_trim_i32_decode(int32_t *x, unsigned logn, unsigned bits,
	const void *in, size_t max_in_len);
size_t ctl_trim_i8_encode(void *out, size_t max_out_len,
	const int8_t *x, unsigned logn, unsigned bits);
size_t ctl_trim_i8_decode(int8_t *x, unsigned logn, unsigned bits,
	const void *in, size_t max_in_len);
size_t ctl_trim_i16_encode(void *out, size_t max_out_len,
	const int16_t *x, unsigned logn, unsigned bits);
size_t ctl_trim_i16_decode(int16_t *x, unsigned logn, unsigned bits,
	const void *in, size_t max_in_len);

/*
 * Encode a polynomial with coefficients modulo 257. This is used for
 * public keys with q = 257.
 *
 * If out == NULL, then max_out_len is ignored and the function returns
 * the size of the output it could produce (in bytes).
 * If out != NULL, then max_out_len is compared with the expected output
 * size. If max_out_len is lower, then no output is produced, and the
 * function returns 0; otherwise, the output is produced and its length
 * (in bytes) is returned.
 */
size_t ctl_encode_257(void *out, size_t max_out_len,
	const uint16_t *x, unsigned logn);

/*
 * Decode a polynomial with coefficients modulo 257. This is used for
 * public keys with q = 257.
 *
 * Input buffer (in[]) has maximum length max_in_len (in bytes). If
 * the input length is not enough for the expected polynomial, then
 * no decoding occurs and the function returns 0. Otherwise, the values
 * are decoded and the number of processed input bytes is returned.
 *
 * If the input is invalid in some way (a decoded coefficient is out of
 * the expected range, or some ignored bit is non-zero), then this the
 * function fails and returns 0; the contents of the output array are
 * then indeterminate.
 *
 * Decoding is constant-time as long as no failure occurs.
 */
size_t ctl_decode_257(uint16_t *x, unsigned logn,
	const void *in, size_t max_in_len);

/*
 * Encode a ciphertext polynomial, for q = 257; coefficients are in -64..+64.
 *
 * If out == NULL, then max_out_len is ignored and the function returns
 * the size of the output it could produce (in bytes).
 * If out != NULL, then max_out_len is compared with the expected output
 * size. If max_out_len is lower, then no output is produced, and the
 * function returns 0; otherwise, the output is produced and its length
 * (in bytes) is returned.
 */
size_t ctl_encode_ciphertext_257(void *out, size_t max_out_len,
	const int8_t *c, unsigned logn);
/*
 * Decode a ciphertext polynomial, for q = 257; coefficients are in -64..+64.
 *
 * Input buffer (in[]) has maximum length max_in_len (in bytes). If
 * the input length is not enough for the expected polynomial, then
 * no decoding occurs and the function returns 0. Otherwise, the values
 * are decoded and the number of processed input bytes is returned.
 *
 * If the input is invalid in some way (a decoded coefficient is out of
 * the expected range, or some ignored bit is non-zero), then this the
 * function fails and returns 0; the contents of the output array are
 * then indeterminate.
 *
 * Decoding is constant-time with regard to the coefficient values.
 */
size_t ctl_decode_ciphertext_257(int8_t *c, unsigned logn,
	const void *in, size_t max_in_len);

/*
 * Encode a polynomial with coefficients modulo 769. This is used for
 * public keys with q = 769.
 *
 * If out == NULL, then max_out_len is ignored and the function returns
 * the size of the output it could produce (in bytes).
 * If out != NULL, then max_out_len is compared with the expected output
 * size. If max_out_len is lower, then no output is produced, and the
 * function returns 0; otherwise, the output is produced and its length
 * (in bytes) is returned.
 */
size_t ctl_encode_769(void *out, size_t max_out_len,
	const uint16_t *x, unsigned logn);

/*
 * Decode a polynomial with coefficients modulo 769. This is used for
 * public keys with q = 769.
 *
 * Input buffer (in[]) has maximum length max_in_len (in bytes). If
 * the input length is not enough for the expected polynomial, then
 * no decoding occurs and the function returns 0. Otherwise, the values
 * are decoded and the number of processed input bytes is returned.
 *
 * If the input is invalid in some way (a decoded coefficient is out of
 * the expected range, or some ignored bit is non-zero), then this the
 * function fails and returns 0; the contents of the output array are
 * then indeterminate.
 *
 * Decoding is constant-time with regard to the coefficient values.
 */
size_t ctl_decode_769(uint16_t *x, unsigned logn,
	const void *in, size_t max_in_len);

/*
 * Encode a ciphertext polynomial, for q = 769; coefficients are in -96..+96.
 *
 * If out == NULL, then max_out_len is ignored and the function returns
 * the size of the output it could produce (in bytes).
 * If out != NULL, then max_out_len is compared with the expected output
 * size. If max_out_len is lower, then no output is produced, and the
 * function returns 0; otherwise, the output is produced and its length
 * (in bytes) is returned.
 */
size_t ctl_encode_ciphertext_769(void *out, size_t max_out_len,
	const int8_t *c, unsigned logn);
/*
 * Decode a ciphertext polynomial, for q = 769; coefficients are in -96..+96.
 *
 * Input buffer (in[]) has maximum length max_in_len (in bytes). If
 * the input length is not enough for the expected polynomial, then
 * no decoding occurs and the function returns 0. Otherwise, the values
 * are decoded and the number of processed input bytes is returned.
 *
 * If the input is invalid in some way (a decoded coefficient is out of
 * the expected range, or some ignored bit is non-zero), then this the
 * function fails and returns 0; the contents of the output array are
 * then indeterminate.
 *
 * Decoding is constant-time with regard to the coefficient values.
 */
size_t ctl_decode_ciphertext_769(int8_t *c, unsigned logn,
	const void *in, size_t max_in_len);

/*
 * Encode a polynomial with coefficients modulo 3329. This is used for
 * public keys with q = 3329.
 */
size_t ctl_encode_3329(void *out, size_t max_out_len,
	const uint16_t *x, unsigned logn);

/*
 * Decode a polynomial with coefficients modulo 3329. This is used for
 * public keys with q = 3329.
 */
size_t ctl_decode_3329(uint16_t *x, unsigned logn,
	const void *in, size_t max_in_len);

/*
 * Encode a ciphertext polynomial, for q = 3329; 
 * coefficients require int16_t due to bounds.
 */
size_t ctl_encode_ciphertext_3329(void *out, size_t max_out_len,
	const int16_t *c, unsigned logn);

/*
 * Decode a ciphertext polynomial, for q = 3329. Values outside the
 * supported -208..+208 range are rejected.
 * coefficients require int16_t due to bounds.
 */
size_t ctl_decode_ciphertext_3329(int16_t *c, unsigned logn,
	const void *in, size_t max_in_len);

/* ====================================================================== */

/* ====================================================================== */

/*
 * Obtain a random seed from the system RNG. Maximum allowed seed length
 * is 2048 bits (256 bytes).
 *
 * Returned value is 1 on success, 0 on error.
 */
int ctl_get_seed(void *seed, size_t len);

/*
 * Custom PRNG that outputs 64-bit integers. It is based on SM3 and pseudoXOF.
 */
typedef struct {
	uint8_t buf[128];
	uint8_t key[32];
	uint64_t ctr;
	size_t ptr;
} prng_context;

/*
 * Initialize the PRNG from the provided seed and an extra 64-bit integer.
 * The seed length MUST NOT exceed 48 bytes.
 */
static inline void
prng_init(prng_context *p, const void *seed, size_t seed_len, uint64_t label)
{
	uint8_t tmp[16 + 48];
	enc64le(tmp, label);
	memcpy(tmp + 8, seed, seed_len);
	pseudoXOF(256, tmp, (8 + seed_len) * 8, p->key);
	p->ctr = 0;
	p->ptr = sizeof p->buf;
}

/*
 * Get a 64-bit integer out of a PRNG.
 */
static inline uint64_t
prng_get_u64(prng_context *p)
{
	uint64_t x;

	if (p->ptr == sizeof p->buf) {
		uint8_t tmp[16 + 32];
		enc64le(tmp, p->ctr);
		memcpy(tmp + 8, p->key, 32);
		pseudoXOF(1024, tmp, 40 * 8, p->buf);
		p->ctr++;
		p->ptr = 0;
	}
	x = dec64le(p->buf + p->ptr);
	p->ptr += 8;
	return x;
}

/*
 * Get arbitrary bytes out of a PRNG.
 */
static inline void
prng_get_bytes(prng_context *p, void *dst, size_t len)
{
	uint8_t tmp[16 + 32];
	enc64le(tmp, p->ctr);
	memcpy(tmp + 8, p->key, 32);
	pseudoXOF(len * 8, tmp, 40 * 8, dst);
	p->ctr++;
}

/* ====================================================================== */

#endif
