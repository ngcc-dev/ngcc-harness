/*
 * CTL key pair generation.
 *
 * =====================================================================
 * WARNING: This file was derived from the key pair generation in
 * Falcon. Falcon uses a different naming convention: in CTL, the
 * NTRU equation is:
 *    g*F - f*G = q
 * whereas the Falcon convention is:
 *    f*G - g*F = q
 * i.e. the Falcon convention exchanges the roles of f and g, and
 * F and G. Throughout most of this file, the Falcon convention is used.
 * The exchange is done in the public functions at the end of this file
 * (starting at ctl_keygen_make_fg()).
 * =====================================================================
 */

#include "inner.h"
#include "ntru_solver.h"
#include "ntru_utils.h"
#include <math.h>

#include <sys/time.h>
#include <stdio.h>

/* ==================================================================== */

#ifndef CTL_ENFORCE_EMBED_CHECK
#define CTL_ENFORCE_EMBED_CHECK   0
#endif

#ifndef CTL_USE_NTRUGEN_SOLVER
#define CTL_USE_NTRUGEN_SOLVER   0
#endif

#ifndef CTL_TRACK_SHRINK_BLOCK
#define CTL_TRACK_SHRINK_BLOCK   0
#endif


/*
 * Diagnostics counters for key generation rejection reasons.
 */
static ctl_keygen_diag_stats kg_diag_stats;
static uint32_t kg_last_inter_depth;
static uint32_t kg_last_inter_stage;
static uint32_t kg_last_inter_index;
static uint32_t kg_last_inter_slen;
static uint32_t kg_last_inter_fglen;
static uint32_t kg_last_modp_expected;
static uint32_t kg_last_modp_actual;
static uint32_t kg_last_modp_mismatch_count;
static uint32_t kg_last_modp_eq_first_count;
static uint32_t kg_last_modp_min;
static uint32_t kg_last_modp_max;
static uint32_t kg_last_modp_p;
static uint32_t kg_last_modp_slen;
static uint32_t kg_last_modp_llen;
static uint32_t kg_last_modp_chklen;
static uint32_t kg_last_modp_logn;
static uint64_t kg_modp_crosscheck_count;
static uint64_t kg_modp_ntt_false_positive_count;
static uint32_t kg_last_modp_direct_ok;
static uint32_t kg_last_smallf_max_abs;
static uint32_t kg_last_smallf_over_count;
static uint32_t kg_last_smallf_first_index;
static int32_t kg_last_smallf_first_value;
static uint32_t kg_last_smallg_max_abs;
static uint32_t kg_last_smallg_over_count;
static uint32_t kg_last_smallg_first_index;
static int32_t kg_last_smallg_first_value;
static ctl_tail_exit_diag kg_last_smallf_d7_tail_exit;
static ctl_tail_exit_diag kg_last_smallf_d8_tail_exit;
static ctl_tail_exit_diag kg_last_smallf_d9_tail_exit;
static uint32_t kg_s3_snap_valid;
static uint32_t kg_s3_snap_depth;
static uint32_t kg_s3_snap_logn;
static uint32_t kg_s3_snap_p;
static uint32_t kg_s3_snap_slen;
static uint32_t kg_s3_snap_llen;
static uint32_t kg_s3_snap_chklen;
static uint32_t kg_s3_snap_bad_index;
static uint32_t kg_s3_snap_expected;
static uint32_t kg_s3_snap_actual;
static uint32_t kg_s3_snap_mismatch_count;
static uint32_t kg_last_modp_term_fG;
static uint32_t kg_last_modp_term_gF;
static uint32_t kg_s3_snap_term_fG;
static uint32_t kg_s3_snap_term_gF;
static uint32_t kg_s3_snap_iter;
static uint32_t kg_s3_snap_fglen;
static uint32_t kg_s3_snap_scale_fg;
static uint32_t kg_s3_snap_scale_FG;
static uint32_t kg_s3_snap_scale_k;
static uint32_t kg_d8_post_valid;
static uint32_t kg_d8_post_iter;
static uint32_t kg_d8_post_fglen;
static uint32_t kg_d8_post_bad_index;
static uint32_t kg_d8_post_expected;
static uint32_t kg_d8_post_actual;
static uint32_t kg_d8_post_mismatch_count;
static uint32_t kg_d8_post_had_update;
static uint32_t kg_d8_post_scale_k;
static uint32_t kg_d8_post_k_nonzero_count;
static uint32_t kg_d8_post_k_max_abs;
static uint32_t kg_d8_post_k_first_nz_index;
static int32_t kg_d8_post_k_first_nz_value;
static int32_t kg_d8_post_k_bad_value;
static uint32_t kg_d8_post_rt2_lo;
static uint32_t kg_d8_post_rt2_hi;
static int32_t kg_d8_post_rt2_round;
static int32_t kg_d8_pre_k[8];
static uint64_t kg_d8_pre_rt2[8];
static uint64_t kg_d8_pre_rt1[8];
static uint64_t kg_d8_pre_rt3[8];
static uint64_t kg_d8_pre_rt4[8];
static uint64_t kg_d8_pre_rt5[8];
static uint32_t kg_d8_post_scale_x;
static uint32_t kg_d8_post_scale_t;
static uint32_t kg_d8_post_tlen;
static uint32_t kg_d8_post_toff;
static uint32_t kg_sk0_valid;
static uint32_t kg_sk0_depth;
static uint32_t kg_sk0_logn;
static uint32_t kg_sk0_iter;
static uint32_t kg_sk0_rlen;
static uint32_t kg_sk0_slen;
static uint32_t kg_sk0_fglen;
static uint32_t kg_sk0_scale_fg;
static uint32_t kg_sk0_scale_FG;
static uint32_t kg_sk0_scale_k;
static uint64_t kg_sk0_hist[12];
static uint32_t kg_s3_snap_ft_lo;
static uint32_t kg_s3_snap_ft_hi;
static uint32_t kg_s3_snap_gt_lo;
static uint32_t kg_s3_snap_gt_hi;
static uint32_t kg_s3_snap_Ft_lo;
static uint32_t kg_s3_snap_Ft_hi;
static uint32_t kg_s3_snap_Gt_lo;
static uint32_t kg_s3_snap_Gt_hi;
#define KG_STAGE3_DIAG_MAXN   8
static ctl_stage3_round_diag kg_s3_last_round;
static ctl_stage3_round_diag kg_s3_snap_round;
static uint32_t kg_obs_small_bits[12];
static uint32_t kg_obs_large_bits[12];
static uint64_t kg_d10_iter_count;
static uint64_t kg_d10_k_zero_count;
static uint64_t kg_d10_k_nonzero_count;
static uint64_t kg_d10_top_changed;
static uint64_t kg_d10_top_unchanged;
static ctl_tail_exit_diag kg_d7_tail_exit;
static uint64_t kg_d7_update_no_shrink_count;
static uint64_t kg_d7_update_and_shrink_count;
static uint64_t kg_d7_update_no_shrink_top_same_count;
static ctl_tail_exit_diag kg_d8_tail_exit;
static uint64_t kg_d8_update_no_shrink_count;
static uint64_t kg_d8_update_and_shrink_count;
static uint64_t kg_d8_update_no_shrink_top_same_count;
static uint64_t kg_d9_iter_count;
static uint64_t kg_d9_k_zero_count;
static uint64_t kg_d9_k_nonzero_count;
static uint64_t kg_d9_top_changed;
static uint64_t kg_d9_top_unchanged;
static uint64_t kg_d9_fglen_shrink_count;
static uint32_t kg_d9_fglen_min;
static ctl_tail_exit_diag kg_d9_tail_exit;
static uint64_t kg_d9_update_no_shrink_count;
static uint64_t kg_d9_update_and_shrink_count;
static uint64_t kg_d9_update_no_shrink_top_same_count;
static uint32_t kg_d9_bad_pre_valid;
static uint32_t kg_d9_bad_pre_iter;
static uint32_t kg_d9_bad_pre_bad_index;
static uint32_t kg_d9_bad_post_valid;
static uint32_t kg_d9_bad_post_iter;
static uint32_t kg_d9_bad_post_bad_index;
static ctl_stage3_round_diag kg_d9_bad_post_round;
static uint64_t kg_d6_iter_count;
static uint64_t kg_d6_k_zero_count;
static uint64_t kg_d6_k_nonzero_count;
static uint64_t kg_d6_top_changed;
static uint64_t kg_d6_top_unchanged;
static uint64_t kg_d6_fglen_shrink_count;
static uint32_t kg_d6_fglen_min;
static uint64_t kg_d6_tail_boost_count;
static uint64_t kg_d5_iter_count;
static uint64_t kg_d5_k_zero_count;
static uint64_t kg_d5_k_nonzero_count;
static uint64_t kg_d5_top_changed;
static uint64_t kg_d5_top_unchanged;
static uint64_t kg_d5_fglen_shrink_count;
static uint32_t kg_d5_fglen_min;
static uint64_t kg_d4_iter_count;
static uint64_t kg_d4_k_zero_count;
static uint64_t kg_d4_k_nonzero_count;
static uint64_t kg_d4_top_changed;
static uint64_t kg_d4_top_unchanged;
static uint64_t kg_d2_iter_count;
static uint64_t kg_d2_k_zero_count;
static uint64_t kg_d2_k_nonzero_count;
static uint64_t kg_d2_top_changed;
static uint64_t kg_d2_top_unchanged;
static uint64_t kg_d2_k_abs_sum;
static uint64_t kg_d2_k_abs_samples;
static uint32_t kg_d2_k_abs_max;
static uint32_t kg_d2_last_scale_fg;
static uint32_t kg_d2_last_scale_FG;
static uint32_t kg_d2_last_scale_k;
static uint32_t kg_d2_last_rlen;
static uint32_t kg_d2_last_slen;
static uint32_t kg_d2_last_fglen;
static uint32_t kg_d2_last_iter;
static uint64_t kg_d9_shrink_block_events;
static uint64_t kg_d9_shrink_block_total_bad;
static uint32_t kg_d9_shrink_block_last_bad;
static uint32_t kg_d9_shrink_block_index;
static uint32_t kg_d9_shrink_block_ft_hi;
static uint32_t kg_d9_shrink_block_ft_lo;
static uint32_t kg_d9_shrink_block_gt_hi;
static uint32_t kg_d9_shrink_block_gt_lo;
static uint64_t kg_d6_shrink_block_events;
static uint64_t kg_d6_shrink_block_total_bad;
static uint32_t kg_d6_shrink_block_last_bad;
static uint32_t kg_d6_shrink_block_index;
static uint32_t kg_d6_shrink_block_ft_hi;
static uint32_t kg_d6_shrink_block_ft_lo;
static uint32_t kg_d6_shrink_block_gt_hi;
static uint32_t kg_d6_shrink_block_gt_lo;
static uint64_t kg_d5_shrink_block_events;
static uint64_t kg_d5_shrink_block_total_bad;
static uint32_t kg_d5_shrink_block_last_bad;
static uint32_t kg_d5_shrink_block_index;
static uint32_t kg_d5_shrink_block_ft_hi;
static uint32_t kg_d5_shrink_block_ft_lo;
static uint32_t kg_d5_shrink_block_gt_hi;
static uint32_t kg_d5_shrink_block_gt_lo;
static uint64_t kg_d4_shrink_block_events;
static uint64_t kg_d4_shrink_block_total_bad;
static uint32_t kg_d4_shrink_block_last_bad;
static uint32_t kg_d4_shrink_block_index;
static uint32_t kg_d4_shrink_block_ft_hi;
static uint32_t kg_d4_shrink_block_ft_lo;
static uint32_t kg_d4_shrink_block_gt_hi;
static uint32_t kg_d4_shrink_block_gt_lo;
static uint64_t kg_d2_shrink_block_events;
static uint64_t kg_d2_shrink_block_total_bad;
static uint32_t kg_d2_shrink_block_last_bad;
static uint32_t kg_d2_shrink_block_index;
static uint32_t kg_d2_shrink_block_ft_hi;
static uint32_t kg_d2_shrink_block_ft_lo;
static uint32_t kg_d2_shrink_block_gt_hi;
static uint32_t kg_d2_shrink_block_gt_lo;
static uint32_t kg_cw_last_v1_abs;
static uint32_t kg_cw_last_lim1;
static uint32_t kg_cw_max_v1_abs;

#define KG_TOP_TRACK_MAXN   2048
static uint32_t kg_top_ft_before_words[KG_TOP_TRACK_MAXN];
static uint32_t kg_top_gt_before_words[KG_TOP_TRACK_MAXN];


static int step4_fail_count = 0;
static double step4_total_time_ms = 0.0;

static int step5_fail_count = 0;
static double step5_total_time_ms = 0.0;

static int step11_fail_count = 0;
static double step11_total_time_ms = 0.0;

void get_step4_stats(unsigned int *fails, double *time) { *fails = step4_fail_count; *time = step4_total_time_ms; }
void get_step5_stats(unsigned int *fails, double *time) { *fails = step5_fail_count; *time = step5_total_time_ms; }
void get_step11_stats(unsigned int *fails, double *time) { *fails = step11_fail_count; *time = step11_total_time_ms; }

//重置统计数据
// Reset profiling counters.
void reset_step4_stats(void)
{
	step4_fail_count = 0;
	step4_total_time_ms = 0.0;
}
void reset_step5_stats(void)
{
	step5_fail_count = 0;
	step5_total_time_ms = 0.0;
}
void reset_step11_stats(void)
{
	step11_fail_count = 0;
	step11_total_time_ms = 0.0;
}

// Record one failed attempt duration for step4.
void record_step4_failure(double time_ms)
{
	step4_fail_count++;
	step4_total_time_ms += time_ms;
}

void record_step5_failure(double time_ms)
{
	step5_fail_count++;
	step5_total_time_ms += time_ms;
}

void record_step11_failure(double time_ms)
{
	step11_fail_count++;
	step11_total_time_ms += time_ms;
}

// 鏉堟挸鍤張鈧紒鍫㈢埠鐠侊紕绮ㄩ弸婊愮礄娓氭稑顦婚柈銊︾ゴ鐠囨洖鍤遍弫鎷岀殶閻㈩煉绱?
void print_step4_stats(void)
{
	printf("\n");
	printf("========== Step4 缂佺喕顓哥紒鎾寸亯 ==========\n");
	printf("Step4 閹銇戠拹銉︻偧閺? %d\n", step4_fail_count);
	printf("Step4 婢惰精瑙﹂幀鏄忊偓妤佹: %.3f 濮ｎ偆顫梊n", step4_total_time_ms);
	printf("=====================================\n");
}

void print_step5_stats(void)
{
	printf("\n");
	printf("========== Step5 缂佺喕顓哥紒鎾寸亯 ==========\n");
	printf("Step5 閹銇戠拹銉︻偧閺? %d\n", step5_fail_count);
	printf("Step5 婢惰精瑙﹂幀鏄忊偓妤佹: %.3f 濮ｎ偆顫梊n", step5_total_time_ms);
	printf("=====================================\n");
}

void print_step11_stats(void)
{
	printf("\n");
	printf("========== Step11 缂佺喕顓哥紒鎾寸亯 ==========\n");
	printf("Step11 閹銇戠拹銉︻偧閺? %d\n", step11_fail_count);
	printf("Step11 婢惰精瑙﹂幀鏄忊偓妤佹: %.3f 濮ｎ偆顫梊n", step11_total_time_ms);
	printf("=====================================\n");
}

enum {
	KG_CW_FAIL_NONE = 0,
	KG_CW_FAIL_LIM1 = 1,
	KG_CW_FAIL_DIV = 2,
	KG_CW_FAIL_FNR_ABS = 3,
	KG_CW_FAIL_ROUND_RANGE = 4
};

static int kg_cw_last_fail;

//诊断数据重置
/* see inner.h */
void
ctl_keygen_diag_reset(void)
{
	memset(&kg_diag_stats, 0, sizeof kg_diag_stats);
	kg_last_inter_depth = 0;
	kg_last_inter_stage = 0;
	kg_last_inter_index = 0;
	kg_last_inter_slen = 0;
	kg_last_inter_fglen = 0;
	kg_last_modp_expected = 0;
	kg_last_modp_actual = 0;
	kg_last_modp_mismatch_count = 0;
	kg_last_modp_eq_first_count = 0;
	kg_last_modp_min = 0;
	kg_last_modp_max = 0;
	kg_last_modp_p = 0;
	kg_last_modp_slen = 0;
	kg_last_modp_llen = 0;
	kg_last_modp_chklen = 0;
	kg_last_modp_logn = 0;
	kg_modp_crosscheck_count = 0;
	kg_modp_ntt_false_positive_count = 0;
	kg_last_modp_direct_ok = 0;
	kg_last_smallf_max_abs = 0;
	kg_last_smallf_over_count = 0;
	kg_last_smallf_first_index = 0;
	kg_last_smallf_first_value = 0;
	kg_last_smallg_max_abs = 0;
	kg_last_smallg_over_count = 0;
	kg_last_smallg_first_index = 0;
	kg_last_smallg_first_value = 0;
	memset(&kg_last_smallf_d7_tail_exit, 0, sizeof kg_last_smallf_d7_tail_exit);
	memset(&kg_last_smallf_d8_tail_exit, 0, sizeof kg_last_smallf_d8_tail_exit);
	memset(&kg_last_smallf_d9_tail_exit, 0, sizeof kg_last_smallf_d9_tail_exit);
	kg_s3_snap_valid = 0;
	kg_s3_snap_depth = 0;
	kg_s3_snap_logn = 0;
	kg_s3_snap_p = 0;
	kg_s3_snap_slen = 0;
	kg_s3_snap_llen = 0;
	kg_s3_snap_chklen = 0;
	kg_s3_snap_bad_index = 0;
	kg_s3_snap_expected = 0;
	kg_s3_snap_actual = 0;
	kg_s3_snap_mismatch_count = 0;
	kg_last_modp_term_fG = 0;
	kg_last_modp_term_gF = 0;
	kg_s3_snap_term_fG = 0;
	kg_s3_snap_term_gF = 0;
	kg_s3_snap_iter = 0;
	kg_s3_snap_fglen = 0;
	kg_s3_snap_scale_fg = 0;
	kg_s3_snap_scale_FG = 0;
	kg_s3_snap_scale_k = 0;
	kg_d8_post_valid = 0;
	kg_d8_post_iter = 0;
	kg_d8_post_fglen = 0;
	kg_d8_post_bad_index = 0;
	kg_d8_post_expected = 0;
	kg_d8_post_actual = 0;
	kg_d8_post_mismatch_count = 0;
	kg_d8_post_had_update = 0;
	kg_d8_post_scale_k = 0;
	kg_d8_post_k_nonzero_count = 0;
	kg_d8_post_k_max_abs = 0;
	kg_d8_post_k_first_nz_index = 0;
	kg_d8_post_k_first_nz_value = 0;
	kg_d8_post_k_bad_value = 0;
	kg_d8_post_rt2_lo = 0;
	kg_d8_post_rt2_hi = 0;
	kg_d8_post_rt2_round = 0;
	memset(kg_d8_pre_k, 0, sizeof kg_d8_pre_k);
	memset(kg_d8_pre_rt2, 0, sizeof kg_d8_pre_rt2);
	memset(kg_d8_pre_rt1, 0, sizeof kg_d8_pre_rt1);
	memset(kg_d8_pre_rt3, 0, sizeof kg_d8_pre_rt3);
	memset(kg_d8_pre_rt4, 0, sizeof kg_d8_pre_rt4);
	memset(kg_d8_pre_rt5, 0, sizeof kg_d8_pre_rt5);
	kg_d8_post_scale_x = 0;
	kg_d8_post_scale_t = 0;
	kg_d8_post_tlen = 0;
	kg_d8_post_toff = 0;
	kg_sk0_valid = 0;
	kg_sk0_depth = 0;
	kg_sk0_logn = 0;
	kg_sk0_iter = 0;
	kg_sk0_rlen = 0;
	kg_sk0_slen = 0;
	kg_sk0_fglen = 0;
	kg_sk0_scale_fg = 0;
	kg_sk0_scale_FG = 0;
	kg_sk0_scale_k = 0;
	memset(kg_sk0_hist, 0, sizeof kg_sk0_hist);
	kg_s3_snap_ft_lo = 0;
	kg_s3_snap_ft_hi = 0;
	kg_s3_snap_gt_lo = 0;
	kg_s3_snap_gt_hi = 0;
	kg_s3_snap_Ft_lo = 0;
	kg_s3_snap_Ft_hi = 0;
	kg_s3_snap_Gt_lo = 0;
	kg_s3_snap_Gt_hi = 0;
	memset(&kg_s3_last_round, 0, sizeof kg_s3_last_round);
	memset(&kg_s3_snap_round, 0, sizeof kg_s3_snap_round);
	memset(kg_obs_small_bits, 0, sizeof kg_obs_small_bits);
	memset(kg_obs_large_bits, 0, sizeof kg_obs_large_bits);
	kg_d10_iter_count = 0;
	kg_d10_k_zero_count = 0;
	kg_d10_k_nonzero_count = 0;
	kg_d10_top_changed = 0;
	kg_d10_top_unchanged = 0;
	memset(&kg_d7_tail_exit, 0, sizeof kg_d7_tail_exit);
	kg_d7_update_no_shrink_count = 0;
	kg_d7_update_and_shrink_count = 0;
	kg_d7_update_no_shrink_top_same_count = 0;
	memset(&kg_d8_tail_exit, 0, sizeof kg_d8_tail_exit);
	kg_d8_update_no_shrink_count = 0;
	kg_d8_update_and_shrink_count = 0;
	kg_d8_update_no_shrink_top_same_count = 0;
	kg_d9_iter_count = 0;
	kg_d9_k_zero_count = 0;
	kg_d9_k_nonzero_count = 0;
	kg_d9_top_changed = 0;
	kg_d9_top_unchanged = 0;
	kg_d9_fglen_shrink_count = 0;
	kg_d9_fglen_min = 0;
	memset(&kg_d9_tail_exit, 0, sizeof kg_d9_tail_exit);
	kg_d9_update_no_shrink_count = 0;
	kg_d9_update_and_shrink_count = 0;
	kg_d9_update_no_shrink_top_same_count = 0;
	kg_d9_bad_pre_valid = 0;
	kg_d9_bad_pre_iter = 0;
	kg_d9_bad_pre_bad_index = 0;
	kg_d9_bad_post_valid = 0;
	kg_d9_bad_post_iter = 0;
	kg_d9_bad_post_bad_index = 0;
	memset(&kg_d9_bad_post_round, 0, sizeof kg_d9_bad_post_round);
	kg_d6_iter_count = 0;
	kg_d6_k_zero_count = 0;
	kg_d6_k_nonzero_count = 0;
	kg_d6_top_changed = 0;
	kg_d6_top_unchanged = 0;
	kg_d6_fglen_shrink_count = 0;
	kg_d6_fglen_min = 0;
	kg_d6_tail_boost_count = 0;
	kg_d5_iter_count = 0;
	kg_d5_k_zero_count = 0;
	kg_d5_k_nonzero_count = 0;
	kg_d5_top_changed = 0;
	kg_d5_top_unchanged = 0;
	kg_d5_fglen_shrink_count = 0;
	kg_d5_fglen_min = 0;
	kg_d4_iter_count = 0;
	kg_d4_k_zero_count = 0;
	kg_d4_k_nonzero_count = 0;
	kg_d4_top_changed = 0;
	kg_d4_top_unchanged = 0;
	kg_d2_iter_count = 0;
	kg_d2_k_zero_count = 0;
	kg_d2_k_nonzero_count = 0;
	kg_d2_top_changed = 0;
	kg_d2_top_unchanged = 0;
	kg_d2_k_abs_sum = 0;
	kg_d2_k_abs_samples = 0;
	kg_d2_k_abs_max = 0;
	kg_d2_last_scale_fg = 0;
	kg_d2_last_scale_FG = 0;
	kg_d2_last_scale_k = 0;
	kg_d2_last_rlen = 0;
	kg_d2_last_slen = 0;
	kg_d2_last_fglen = 0;
	kg_d2_last_iter = 0;
	kg_d9_shrink_block_events = 0;
	kg_d9_shrink_block_total_bad = 0;
	kg_d9_shrink_block_last_bad = 0;
	kg_d9_shrink_block_index = 0;
	kg_d9_shrink_block_ft_hi = 0;
	kg_d9_shrink_block_ft_lo = 0;
	kg_d9_shrink_block_gt_hi = 0;
	kg_d9_shrink_block_gt_lo = 0;
	kg_d6_shrink_block_events = 0;
	kg_d6_shrink_block_total_bad = 0;
	kg_d6_shrink_block_last_bad = 0;
	kg_d6_shrink_block_index = 0;
	kg_d6_shrink_block_ft_hi = 0;
	kg_d6_shrink_block_ft_lo = 0;
	kg_d6_shrink_block_gt_hi = 0;
	kg_d6_shrink_block_gt_lo = 0;
	kg_d5_shrink_block_events = 0;
	kg_d5_shrink_block_total_bad = 0;
	kg_d5_shrink_block_last_bad = 0;
	kg_d5_shrink_block_index = 0;
	kg_d5_shrink_block_ft_hi = 0;
	kg_d5_shrink_block_ft_lo = 0;
	kg_d5_shrink_block_gt_hi = 0;
	kg_d5_shrink_block_gt_lo = 0;
	kg_d4_shrink_block_events = 0;
	kg_d4_shrink_block_total_bad = 0;
	kg_d4_shrink_block_last_bad = 0;
	kg_d4_shrink_block_index = 0;
	kg_d4_shrink_block_ft_hi = 0;
	kg_d4_shrink_block_ft_lo = 0;
	kg_d4_shrink_block_gt_hi = 0;
	kg_d4_shrink_block_gt_lo = 0;
	kg_d2_shrink_block_events = 0;
	kg_d2_shrink_block_total_bad = 0;
	kg_d2_shrink_block_last_bad = 0;
	kg_d2_shrink_block_index = 0;
	kg_d2_shrink_block_ft_hi = 0;
	kg_d2_shrink_block_ft_lo = 0;
	kg_d2_shrink_block_gt_hi = 0;
	kg_d2_shrink_block_gt_lo = 0;
	kg_cw_last_v1_abs = 0;
	kg_cw_last_lim1 = 0;
	kg_cw_max_v1_abs = 0;
}

/* see inner.h */
void
ctl_keygen_diag_get(ctl_keygen_diag_stats *dst)
{
	if (dst != NULL) {
		*dst = kg_diag_stats;
	}
}

/* see inner.h */
void
ctl_debug_get_last_intermediate_fail(
	uint32_t *depth, uint32_t *stage, uint32_t *index)
{
	if (depth != NULL) {
		*depth = kg_last_inter_depth;
	}
	if (stage != NULL) {
		*stage = kg_last_inter_stage;
	}
	if (index != NULL) {
		*index = kg_last_inter_index;
	}
}

/* see inner.h */
void
ctl_debug_get_last_intermediate_sizes(uint32_t *slen, uint32_t *fglen)
{
	if (slen != NULL) {
		*slen = kg_last_inter_slen;
	}
	if (fglen != NULL) {
		*fglen = kg_last_inter_fglen;
	}
}

/* see inner.h */
void
ctl_debug_get_d10_iter_stats(
	uint64_t *iter_count, uint64_t *k_zero_count, uint64_t *k_nonzero_count)
{
	if (iter_count != NULL) {
		*iter_count = kg_d10_iter_count;
	}
	if (k_zero_count != NULL) {
		*k_zero_count = kg_d10_k_zero_count;
	}
	if (k_nonzero_count != NULL) {
		*k_nonzero_count = kg_d10_k_nonzero_count;
	}
}

/* see inner.h */
void
ctl_debug_get_d10_top_stats(uint64_t *top_changed, uint64_t *top_unchanged)
{
	if (top_changed != NULL) {
		*top_changed = kg_d10_top_changed;
	}
	if (top_unchanged != NULL) {
		*top_unchanged = kg_d10_top_unchanged;
	}
}

/* see inner.h */
void
ctl_debug_get_d9_iter_stats(
	uint64_t *iter_count, uint64_t *k_zero_count, uint64_t *k_nonzero_count)
{
	if (iter_count != NULL) {
		*iter_count = kg_d9_iter_count;
	}
	if (k_zero_count != NULL) {
		*k_zero_count = kg_d9_k_zero_count;
	}
	if (k_nonzero_count != NULL) {
		*k_nonzero_count = kg_d9_k_nonzero_count;
	}
}

/* see inner.h */
void
ctl_debug_get_d9_top_stats(uint64_t *top_changed, uint64_t *top_unchanged)
{
	if (top_changed != NULL) {
		*top_changed = kg_d9_top_changed;
	}
	if (top_unchanged != NULL) {
		*top_unchanged = kg_d9_top_unchanged;
	}
}

/* see inner.h */
void
ctl_debug_get_d9_fglen_stats(uint64_t *shrink_count, uint32_t *min_fglen)
{
	if (shrink_count != NULL) {
		*shrink_count = kg_d9_fglen_shrink_count;
	}
	if (min_fglen != NULL) {
		*min_fglen = kg_d9_fglen_min;
	}
}

/* see inner.h */
void
ctl_debug_get_d7_tail_exit(ctl_tail_exit_diag *dst)
{
	if (dst == NULL) {
		return;
	}
	*dst = kg_d7_tail_exit;
}

/* see inner.h */
void
ctl_debug_get_d7_update_stall_stats(
	uint64_t *update_no_shrink,
	uint64_t *update_and_shrink,
	uint64_t *update_no_shrink_top_same)
{
	if (update_no_shrink != NULL) {
		*update_no_shrink = kg_d7_update_no_shrink_count;
	}
	if (update_and_shrink != NULL) {
		*update_and_shrink = kg_d7_update_and_shrink_count;
	}
	if (update_no_shrink_top_same != NULL) {
		*update_no_shrink_top_same = kg_d7_update_no_shrink_top_same_count;
	}
}

/* see inner.h */
void
ctl_debug_get_d8_tail_exit(ctl_tail_exit_diag *dst)
{
	if (dst == NULL) {
		return;
	}
	*dst = kg_d8_tail_exit;
}

/* see inner.h */
void
ctl_debug_get_d9_tail_exit(ctl_tail_exit_diag *dst)
{
	if (dst == NULL) {
		return;
	}
	*dst = kg_d9_tail_exit;
}

/* see inner.h */
void
ctl_debug_get_d8_update_stall_stats(
	uint64_t *update_no_shrink,
	uint64_t *update_and_shrink,
	uint64_t *update_no_shrink_top_same)
{
	if (update_no_shrink != NULL) {
		*update_no_shrink = kg_d8_update_no_shrink_count;
	}
	if (update_and_shrink != NULL) {
		*update_and_shrink = kg_d8_update_and_shrink_count;
	}
	if (update_no_shrink_top_same != NULL) {
		*update_no_shrink_top_same = kg_d8_update_no_shrink_top_same_count;
	}
}

/* see inner.h */
void
ctl_debug_get_d9_update_stall_stats(
	uint64_t *update_no_shrink,
	uint64_t *update_and_shrink,
	uint64_t *update_no_shrink_top_same)
{
	if (update_no_shrink != NULL) {
		*update_no_shrink = kg_d9_update_no_shrink_count;
	}
	if (update_and_shrink != NULL) {
		*update_and_shrink = kg_d9_update_and_shrink_count;
	}
	if (update_no_shrink_top_same != NULL) {
		*update_no_shrink_top_same = kg_d9_update_no_shrink_top_same_count;
	}
}

/* see inner.h */
void
ctl_debug_get_first_d9_bad_pre(uint32_t *iter, uint32_t *bad_index)
{
	if (iter != NULL) {
		*iter = kg_d9_bad_pre_valid ? kg_d9_bad_pre_iter : 0;
	}
	if (bad_index != NULL) {
		*bad_index = kg_d9_bad_pre_valid ? kg_d9_bad_pre_bad_index : 0;
	}
}

/* see inner.h */
void
ctl_debug_get_first_d9_bad_post(
	uint32_t *iter, uint32_t *bad_index,
	ctl_stage3_round_diag *round)
{
	if (iter != NULL) {
		*iter = kg_d9_bad_post_valid ? kg_d9_bad_post_iter : 0;
	}
	if (bad_index != NULL) {
		*bad_index = kg_d9_bad_post_valid ? kg_d9_bad_post_bad_index : 0;
	}
	if (round != NULL) {
		if (kg_d9_bad_post_valid) {
			*round = kg_d9_bad_post_round;
		} else {
			memset(round, 0, sizeof *round);
		}
	}
}

/* see inner.h */
void
ctl_debug_get_d6_iter_stats(
	uint64_t *iter_count, uint64_t *k_zero_count, uint64_t *k_nonzero_count)
{
	if (iter_count != NULL) {
		*iter_count = kg_d6_iter_count;
	}
	if (k_zero_count != NULL) {
		*k_zero_count = kg_d6_k_zero_count;
	}
	if (k_nonzero_count != NULL) {
		*k_nonzero_count = kg_d6_k_nonzero_count;
	}
}

/* see inner.h */
void
ctl_debug_get_d6_top_stats(uint64_t *top_changed, uint64_t *top_unchanged)
{
	if (top_changed != NULL) {
		*top_changed = kg_d6_top_changed;
	}
	if (top_unchanged != NULL) {
		*top_unchanged = kg_d6_top_unchanged;
	}
}

/* see inner.h */
void
ctl_debug_get_d6_fglen_stats(
	uint64_t *shrink_count, uint32_t *min_fglen, uint64_t *tail_boost_count)
{
	if (shrink_count != NULL) {
		*shrink_count = kg_d6_fglen_shrink_count;
	}
	if (min_fglen != NULL) {
		*min_fglen = kg_d6_fglen_min;
	}
	if (tail_boost_count != NULL) {
		*tail_boost_count = kg_d6_tail_boost_count;
	}
}

/* see inner.h */
void
ctl_debug_get_d5_iter_stats(
	uint64_t *iter_count, uint64_t *k_zero_count, uint64_t *k_nonzero_count)
{
	if (iter_count != NULL) {
		*iter_count = kg_d5_iter_count;
	}
	if (k_zero_count != NULL) {
		*k_zero_count = kg_d5_k_zero_count;
	}
	if (k_nonzero_count != NULL) {
		*k_nonzero_count = kg_d5_k_nonzero_count;
	}
}

/* see inner.h */
void
ctl_debug_get_d5_top_stats(uint64_t *top_changed, uint64_t *top_unchanged)
{
	if (top_changed != NULL) {
		*top_changed = kg_d5_top_changed;
	}
	if (top_unchanged != NULL) {
		*top_unchanged = kg_d5_top_unchanged;
	}
}

/* see inner.h */
void
ctl_debug_get_d5_fglen_stats(uint64_t *shrink_count, uint32_t *min_fglen)
{
	if (shrink_count != NULL) {
		*shrink_count = kg_d5_fglen_shrink_count;
	}
	if (min_fglen != NULL) {
		*min_fglen = kg_d5_fglen_min;
	}
}

/* see inner.h */
void
ctl_debug_get_last_modp_mismatch(uint32_t *expected, uint32_t *actual)
{
	if (expected != NULL) {
		*expected = kg_last_modp_expected;
	}
	if (actual != NULL) {
		*actual = kg_last_modp_actual;
	}
}

/* see inner.h */
void
ctl_debug_get_last_modp_mismatch_count(uint32_t *count)
{
	if (count != NULL) {
		*count = kg_last_modp_mismatch_count;
	}
}

/* see inner.h */
void
ctl_debug_get_last_modp_profile(
	uint32_t *eq_first_count, uint32_t *vmin, uint32_t *vmax)
{
	if (eq_first_count != NULL) {
		*eq_first_count = kg_last_modp_eq_first_count;
	}
	if (vmin != NULL) {
		*vmin = kg_last_modp_min;
	}
	if (vmax != NULL) {
		*vmax = kg_last_modp_max;
	}
}

/* see inner.h */
void
ctl_debug_get_last_modp_context(
	uint32_t *p, uint32_t *slen, uint32_t *llen,
	uint32_t *chklen, uint32_t *logn)
{
	if (p != NULL) {
		*p = kg_last_modp_p;
	}
	if (slen != NULL) {
		*slen = kg_last_modp_slen;
	}
	if (llen != NULL) {
		*llen = kg_last_modp_llen;
	}
	if (chklen != NULL) {
		*chklen = kg_last_modp_chklen;
	}
	if (logn != NULL) {
		*logn = kg_last_modp_logn;
	}
}

/* see inner.h */
void
ctl_debug_get_modp_crosscheck_stats(
	uint64_t *crosscheck_count,
	uint64_t *ntt_false_positive_count,
	uint32_t *last_direct_ok)
{
	if (crosscheck_count != NULL) {
		*crosscheck_count = kg_modp_crosscheck_count;
	}
	if (ntt_false_positive_count != NULL) {
		*ntt_false_positive_count = kg_modp_ntt_false_positive_count;
	}
	if (last_direct_ok != NULL) {
		*last_direct_ok = kg_last_modp_direct_ok;
	}
}

/* see inner.h */
void
ctl_debug_get_last_poly_small_i16_f_stats(
	uint32_t *max_abs, uint32_t *over_count,
	uint32_t *first_index, int32_t *first_value)
{
	if (max_abs != NULL) {
		*max_abs = kg_last_smallf_max_abs;
	}
	if (over_count != NULL) {
		*over_count = kg_last_smallf_over_count;
	}
	if (first_index != NULL) {
		*first_index = kg_last_smallf_first_index;
	}
	if (first_value != NULL) {
		*first_value = kg_last_smallf_first_value;
	}
}

/* see inner.h */
void
ctl_debug_get_last_poly_small_i16_g_stats(
	uint32_t *max_abs, uint32_t *over_count,
	uint32_t *first_index, int32_t *first_value)
{
	if (max_abs != NULL) {
		*max_abs = kg_last_smallg_max_abs;
	}
	if (over_count != NULL) {
		*over_count = kg_last_smallg_over_count;
	}
	if (first_index != NULL) {
		*first_index = kg_last_smallg_first_index;
	}
	if (first_value != NULL) {
		*first_value = kg_last_smallg_first_value;
	}
}

/* see inner.h */
void
ctl_debug_get_last_poly_small_i16_f_tail_exits(
	ctl_tail_exit_diag *d7, ctl_tail_exit_diag *d8, ctl_tail_exit_diag *d9)
{
	if (d7 != NULL) {
		*d7 = kg_last_smallf_d7_tail_exit;
	}
	if (d8 != NULL) {
		*d8 = kg_last_smallf_d8_tail_exit;
	}
	if (d9 != NULL) {
		*d9 = kg_last_smallf_d9_tail_exit;
	}
}

/* see inner.h */
void
ctl_debug_get_first_stage3_fail_snapshot(
	uint32_t *depth, uint32_t *logn, uint32_t *p,
	uint32_t *slen, uint32_t *llen, uint32_t *chklen,
	uint32_t *bad_index, uint32_t *expected, uint32_t *actual,
	uint32_t *mismatch_count)
{
	if (depth != NULL) {
		*depth = kg_s3_snap_valid ? kg_s3_snap_depth : 0;
	}
	if (logn != NULL) {
		*logn = kg_s3_snap_valid ? kg_s3_snap_logn : 0;
	}
	if (p != NULL) {
		*p = kg_s3_snap_valid ? kg_s3_snap_p : 0;
	}
	if (slen != NULL) {
		*slen = kg_s3_snap_valid ? kg_s3_snap_slen : 0;
	}
	if (llen != NULL) {
		*llen = kg_s3_snap_valid ? kg_s3_snap_llen : 0;
	}
	if (chklen != NULL) {
		*chklen = kg_s3_snap_valid ? kg_s3_snap_chklen : 0;
	}
	if (bad_index != NULL) {
		*bad_index = kg_s3_snap_valid ? kg_s3_snap_bad_index : 0;
	}
	if (expected != NULL) {
		*expected = kg_s3_snap_valid ? kg_s3_snap_expected : 0;
	}
	if (actual != NULL) {
		*actual = kg_s3_snap_valid ? kg_s3_snap_actual : 0;
	}
	if (mismatch_count != NULL) {
		*mismatch_count = kg_s3_snap_valid ? kg_s3_snap_mismatch_count : 0;
	}
}

/* see inner.h */
void
ctl_debug_get_first_stage3_fail_terms(
	uint32_t *term_fG, uint32_t *term_gF)
{
	if (term_fG != NULL) {
		*term_fG = kg_s3_snap_valid ? kg_s3_snap_term_fG : 0;
	}
	if (term_gF != NULL) {
		*term_gF = kg_s3_snap_valid ? kg_s3_snap_term_gF : 0;
	}
}

/* see inner.h */
void
ctl_debug_get_first_stage3_fail_scales(
	uint32_t *iter, uint32_t *fglen,
	uint32_t *scale_fg, uint32_t *scale_FG, uint32_t *scale_k)
{
	if (iter != NULL) {
		*iter = kg_s3_snap_valid ? kg_s3_snap_iter : 0;
	}
	if (fglen != NULL) {
		*fglen = kg_s3_snap_valid ? kg_s3_snap_fglen : 0;
	}
	if (scale_fg != NULL) {
		*scale_fg = kg_s3_snap_valid ? kg_s3_snap_scale_fg : 0;
	}
	if (scale_FG != NULL) {
		*scale_FG = kg_s3_snap_valid ? kg_s3_snap_scale_FG : 0;
	}
	if (scale_k != NULL) {
		*scale_k = kg_s3_snap_valid ? kg_s3_snap_scale_k : 0;
	}
}

/* see inner.h */
void
ctl_debug_get_first_depth8_post_update_fail(
	uint32_t *iter, uint32_t *fglen, uint32_t *bad_index,
	uint32_t *expected, uint32_t *actual, uint32_t *mismatch_count,
	uint32_t *had_update, uint32_t *scale_k)
{
	if (iter != NULL) {
		*iter = kg_d8_post_valid ? kg_d8_post_iter : 0;
	}
	if (fglen != NULL) {
		*fglen = kg_d8_post_valid ? kg_d8_post_fglen : 0;
	}
	if (bad_index != NULL) {
		*bad_index = kg_d8_post_valid ? kg_d8_post_bad_index : 0;
	}
	if (expected != NULL) {
		*expected = kg_d8_post_valid ? kg_d8_post_expected : 0;
	}
	if (actual != NULL) {
		*actual = kg_d8_post_valid ? kg_d8_post_actual : 0;
	}
	if (mismatch_count != NULL) {
		*mismatch_count = kg_d8_post_valid ? kg_d8_post_mismatch_count : 0;
	}
	if (had_update != NULL) {
		*had_update = kg_d8_post_valid ? kg_d8_post_had_update : 0;
	}
	if (scale_k != NULL) {
		*scale_k = kg_d8_post_valid ? kg_d8_post_scale_k : 0;
	}
}

/* see inner.h */
void
ctl_debug_get_first_depth8_post_update_kstats(
	uint32_t *nonzero_count, uint32_t *max_abs,
	uint32_t *first_nz_index, int32_t *first_nz_value,
	int32_t *bad_k_value, uint32_t *rt2_lo, uint32_t *rt2_hi,
	int32_t *rt2_round)
{
	if (nonzero_count != NULL) {
		*nonzero_count = kg_d8_post_valid ? kg_d8_post_k_nonzero_count : 0;
	}
	if (max_abs != NULL) {
		*max_abs = kg_d8_post_valid ? kg_d8_post_k_max_abs : 0;
	}
	if (first_nz_index != NULL) {
		*first_nz_index = kg_d8_post_valid ? kg_d8_post_k_first_nz_index : 0;
	}
	if (first_nz_value != NULL) {
		*first_nz_value = kg_d8_post_valid ? kg_d8_post_k_first_nz_value : 0;
	}
	if (bad_k_value != NULL) {
		*bad_k_value = kg_d8_post_valid ? kg_d8_post_k_bad_value : 0;
	}
	if (rt2_lo != NULL) {
		*rt2_lo = kg_d8_post_valid ? kg_d8_post_rt2_lo : 0;
	}
	if (rt2_hi != NULL) {
		*rt2_hi = kg_d8_post_valid ? kg_d8_post_rt2_hi : 0;
	}
	if (rt2_round != NULL) {
		*rt2_round = kg_d8_post_valid ? kg_d8_post_rt2_round : 0;
	}
}

/* see inner.h */
void
ctl_debug_get_first_depth8_post_update_chain(
	uint32_t *scale_x, uint32_t *scale_t,
	uint32_t *tlen, uint32_t *toff,
	uint32_t *rt1_lo, uint32_t *rt1_hi,
	uint32_t *rt3_lo, uint32_t *rt3_hi,
	uint32_t *rt4_lo, uint32_t *rt4_hi,
	uint32_t *rt5_lo, uint32_t *rt5_hi)
{
	if (scale_x != NULL) {
		*scale_x = kg_d8_post_valid ? kg_d8_post_scale_x : 0;
	}
	if (scale_t != NULL) {
		*scale_t = kg_d8_post_valid ? kg_d8_post_scale_t : 0;
	}
	if (tlen != NULL) {
		*tlen = kg_d8_post_valid ? kg_d8_post_tlen : 0;
	}
	if (toff != NULL) {
		*toff = kg_d8_post_valid ? kg_d8_post_toff : 0;
	}
	if (rt1_lo != NULL) {
		*rt1_lo = kg_d8_post_valid ? (uint32_t)kg_d8_pre_rt1[0] : 0;
	}
	if (rt1_hi != NULL) {
		*rt1_hi = kg_d8_post_valid ? (uint32_t)(kg_d8_pre_rt1[0] >> 32) : 0;
	}
	if (rt3_lo != NULL) {
		*rt3_lo = kg_d8_post_valid ? (uint32_t)kg_d8_pre_rt3[0] : 0;
	}
	if (rt3_hi != NULL) {
		*rt3_hi = kg_d8_post_valid ? (uint32_t)(kg_d8_pre_rt3[0] >> 32) : 0;
	}
	if (rt4_lo != NULL) {
		*rt4_lo = kg_d8_post_valid ? (uint32_t)kg_d8_pre_rt4[0] : 0;
	}
	if (rt4_hi != NULL) {
		*rt4_hi = kg_d8_post_valid ? (uint32_t)(kg_d8_pre_rt4[0] >> 32) : 0;
	}
	if (rt5_lo != NULL) {
		*rt5_lo = kg_d8_post_valid ? (uint32_t)kg_d8_pre_rt5[0] : 0;
	}
	if (rt5_hi != NULL) {
		*rt5_hi = kg_d8_post_valid ? (uint32_t)(kg_d8_pre_rt5[0] >> 32) : 0;
	}
}

/* see inner.h */
void
ctl_debug_get_first_scale_k_zero_snapshot(
	uint32_t *depth, uint32_t *logn, uint32_t *iter,
	uint32_t *rlen, uint32_t *slen, uint32_t *fglen,
	uint32_t *scale_fg, uint32_t *scale_FG, uint32_t *scale_k)
{
	if (depth != NULL) {
		*depth = kg_sk0_valid ? kg_sk0_depth : 0;
	}
	if (logn != NULL) {
		*logn = kg_sk0_valid ? kg_sk0_logn : 0;
	}
	if (iter != NULL) {
		*iter = kg_sk0_valid ? kg_sk0_iter : 0;
	}
	if (rlen != NULL) {
		*rlen = kg_sk0_valid ? kg_sk0_rlen : 0;
	}
	if (slen != NULL) {
		*slen = kg_sk0_valid ? kg_sk0_slen : 0;
	}
	if (fglen != NULL) {
		*fglen = kg_sk0_valid ? kg_sk0_fglen : 0;
	}
	if (scale_fg != NULL) {
		*scale_fg = kg_sk0_valid ? kg_sk0_scale_fg : 0;
	}
	if (scale_FG != NULL) {
		*scale_FG = kg_sk0_valid ? kg_sk0_scale_FG : 0;
	}
	if (scale_k != NULL) {
		*scale_k = kg_sk0_valid ? kg_sk0_scale_k : 0;
	}
}

/* see inner.h */
void
ctl_debug_get_scale_k_zero_hist(uint64_t *dst, size_t dst_len)
{
	size_t n, u;

	if (dst == NULL || dst_len == 0) {
		return;
	}
	n = dst_len;
	if (n > (sizeof kg_sk0_hist / sizeof kg_sk0_hist[0])) {
		n = sizeof kg_sk0_hist / sizeof kg_sk0_hist[0];
	}
	for (u = 0; u < n; u ++) {
		dst[u] = kg_sk0_hist[u];
	}
	for (; u < dst_len; u ++) {
		dst[u] = 0;
	}
}

/* see inner.h */
void
ctl_debug_get_first_stage3_fail_limbs(
	uint32_t *ft_lo, uint32_t *ft_hi,
	uint32_t *gt_lo, uint32_t *gt_hi,
	uint32_t *Ft_lo, uint32_t *Ft_hi,
	uint32_t *Gt_lo, uint32_t *Gt_hi)
{
	if (ft_lo != NULL) {
		*ft_lo = kg_s3_snap_valid ? kg_s3_snap_ft_lo : 0;
	}
	if (ft_hi != NULL) {
		*ft_hi = kg_s3_snap_valid ? kg_s3_snap_ft_hi : 0;
	}
	if (gt_lo != NULL) {
		*gt_lo = kg_s3_snap_valid ? kg_s3_snap_gt_lo : 0;
	}
	if (gt_hi != NULL) {
		*gt_hi = kg_s3_snap_valid ? kg_s3_snap_gt_hi : 0;
	}
	if (Ft_lo != NULL) {
		*Ft_lo = kg_s3_snap_valid ? kg_s3_snap_Ft_lo : 0;
	}
	if (Ft_hi != NULL) {
		*Ft_hi = kg_s3_snap_valid ? kg_s3_snap_Ft_hi : 0;
	}
	if (Gt_lo != NULL) {
		*Gt_lo = kg_s3_snap_valid ? kg_s3_snap_Gt_lo : 0;
	}
	if (Gt_hi != NULL) {
		*Gt_hi = kg_s3_snap_valid ? kg_s3_snap_Gt_hi : 0;
	}
}

/* see inner.h */
void
ctl_debug_get_first_stage3_fail_round_diag(ctl_stage3_round_diag *dst)
{
	if (dst == NULL) {
		return;
	}
	if (kg_s3_snap_round.valid) {
		*dst = kg_s3_snap_round;
	} else {
		memset(dst, 0, sizeof *dst);
	}
}

/* see inner.h */
void
ctl_debug_get_observed_bitlengths(
	uint32_t *small_bits, uint32_t *large_bits, size_t len)
{
	size_t u, n;

	n = len;
	if (n > (sizeof kg_obs_small_bits / sizeof kg_obs_small_bits[0])) {
		n = sizeof kg_obs_small_bits / sizeof kg_obs_small_bits[0];
	}
	for (u = 0; u < n; u ++) {
		if (small_bits != NULL) {
			small_bits[u] = kg_obs_small_bits[u];
		}
		if (large_bits != NULL) {
			large_bits[u] = kg_obs_large_bits[u];
		}
	}
}

/* see inner.h */
void
ctl_debug_get_d4_iter_stats(
	uint64_t *iter_count, uint64_t *k_zero_count, uint64_t *k_nonzero_count)
{
	if (iter_count != NULL) {
		*iter_count = kg_d4_iter_count;
	}
	if (k_zero_count != NULL) {
		*k_zero_count = kg_d4_k_zero_count;
	}
	if (k_nonzero_count != NULL) {
		*k_nonzero_count = kg_d4_k_nonzero_count;
	}
}

/* see inner.h */
void
ctl_debug_get_d4_top_stats(uint64_t *top_changed, uint64_t *top_unchanged)
{
	if (top_changed != NULL) {
		*top_changed = kg_d4_top_changed;
	}
	if (top_unchanged != NULL) {
		*top_unchanged = kg_d4_top_unchanged;
	}
}

/* see inner.h */
void
ctl_debug_get_d2_iter_stats(
	uint64_t *iter_count, uint64_t *k_zero_count, uint64_t *k_nonzero_count)
{
	if (iter_count != NULL) {
		*iter_count = kg_d2_iter_count;
	}
	if (k_zero_count != NULL) {
		*k_zero_count = kg_d2_k_zero_count;
	}
	if (k_nonzero_count != NULL) {
		*k_nonzero_count = kg_d2_k_nonzero_count;
	}
}

/* see inner.h */
void
ctl_debug_get_d2_top_stats(uint64_t *top_changed, uint64_t *top_unchanged)
{
	if (top_changed != NULL) {
		*top_changed = kg_d2_top_changed;
	}
	if (top_unchanged != NULL) {
		*top_unchanged = kg_d2_top_unchanged;
	}
}

/* see inner.h */
void
ctl_debug_get_d2_kabs_stats(
	uint64_t *sum_abs, uint64_t *sample_count, uint32_t *max_abs)
{
	if (sum_abs != NULL) {
		*sum_abs = kg_d2_k_abs_sum;
	}
	if (sample_count != NULL) {
		*sample_count = kg_d2_k_abs_samples;
	}
	if (max_abs != NULL) {
		*max_abs = kg_d2_k_abs_max;
	}
}

/* see inner.h */
void
ctl_debug_get_d2_last_state(
	uint32_t *iter, uint32_t *rlen, uint32_t *slen, uint32_t *fglen,
	uint32_t *scale_fg, uint32_t *scale_FG, uint32_t *scale_k)
{
	if (iter != NULL) {
		*iter = kg_d2_last_iter;
	}
	if (rlen != NULL) {
		*rlen = kg_d2_last_rlen;
	}
	if (slen != NULL) {
		*slen = kg_d2_last_slen;
	}
	if (fglen != NULL) {
		*fglen = kg_d2_last_fglen;
	}
	if (scale_fg != NULL) {
		*scale_fg = kg_d2_last_scale_fg;
	}
	if (scale_FG != NULL) {
		*scale_FG = kg_d2_last_scale_FG;
	}
	if (scale_k != NULL) {
		*scale_k = kg_d2_last_scale_k;
	}
}

/* see inner.h */
void
ctl_debug_get_d9_shrink_block_stats(
	uint64_t *events, uint64_t *total_bad, uint32_t *last_bad)
{
	if (events != NULL) {
		*events = kg_d9_shrink_block_events;
	}
	if (total_bad != NULL) {
		*total_bad = kg_d9_shrink_block_total_bad;
	}
	if (last_bad != NULL) {
		*last_bad = kg_d9_shrink_block_last_bad;
	}
}

/* see inner.h */
void
ctl_debug_get_d9_shrink_block_sample(
	uint32_t *index,
	uint32_t *ft_hi, uint32_t *ft_lo,
	uint32_t *gt_hi, uint32_t *gt_lo)
{
	if (index != NULL) {
		*index = kg_d9_shrink_block_index;
	}
	if (ft_hi != NULL) {
		*ft_hi = kg_d9_shrink_block_ft_hi;
	}
	if (ft_lo != NULL) {
		*ft_lo = kg_d9_shrink_block_ft_lo;
	}
	if (gt_hi != NULL) {
		*gt_hi = kg_d9_shrink_block_gt_hi;
	}
	if (gt_lo != NULL) {
		*gt_lo = kg_d9_shrink_block_gt_lo;
	}
}

/* see inner.h */
void
ctl_debug_get_d6_shrink_block_stats(
	uint64_t *events, uint64_t *total_bad, uint32_t *last_bad)
{
	if (events != NULL) {
		*events = kg_d6_shrink_block_events;
	}
	if (total_bad != NULL) {
		*total_bad = kg_d6_shrink_block_total_bad;
	}
	if (last_bad != NULL) {
		*last_bad = kg_d6_shrink_block_last_bad;
	}
}

/* see inner.h */
void
ctl_debug_get_d6_shrink_block_sample(
	uint32_t *index,
	uint32_t *ft_hi, uint32_t *ft_lo,
	uint32_t *gt_hi, uint32_t *gt_lo)
{
	if (index != NULL) {
		*index = kg_d6_shrink_block_index;
	}
	if (ft_hi != NULL) {
		*ft_hi = kg_d6_shrink_block_ft_hi;
	}
	if (ft_lo != NULL) {
		*ft_lo = kg_d6_shrink_block_ft_lo;
	}
	if (gt_hi != NULL) {
		*gt_hi = kg_d6_shrink_block_gt_hi;
	}
	if (gt_lo != NULL) {
		*gt_lo = kg_d6_shrink_block_gt_lo;
	}
}

/* see inner.h */
void
ctl_debug_get_d5_shrink_block_stats(
	uint64_t *events, uint64_t *total_bad, uint32_t *last_bad)
{
	if (events != NULL) {
		*events = kg_d5_shrink_block_events;
	}
	if (total_bad != NULL) {
		*total_bad = kg_d5_shrink_block_total_bad;
	}
	if (last_bad != NULL) {
		*last_bad = kg_d5_shrink_block_last_bad;
	}
}

/* see inner.h */
void
ctl_debug_get_d5_shrink_block_sample(
	uint32_t *index,
	uint32_t *ft_hi, uint32_t *ft_lo,
	uint32_t *gt_hi, uint32_t *gt_lo)
{
	if (index != NULL) {
		*index = kg_d5_shrink_block_index;
	}
	if (ft_hi != NULL) {
		*ft_hi = kg_d5_shrink_block_ft_hi;
	}
	if (ft_lo != NULL) {
		*ft_lo = kg_d5_shrink_block_ft_lo;
	}
	if (gt_hi != NULL) {
		*gt_hi = kg_d5_shrink_block_gt_hi;
	}
	if (gt_lo != NULL) {
		*gt_lo = kg_d5_shrink_block_gt_lo;
	}
}

/* see inner.h */
void
ctl_debug_get_d4_shrink_block_stats(
	uint64_t *events, uint64_t *total_bad, uint32_t *last_bad)
{
	if (events != NULL) {
		*events = kg_d4_shrink_block_events;
	}
	if (total_bad != NULL) {
		*total_bad = kg_d4_shrink_block_total_bad;
	}
	if (last_bad != NULL) {
		*last_bad = kg_d4_shrink_block_last_bad;
	}
}

/* see inner.h */
void
ctl_debug_get_d4_shrink_block_sample(
	uint32_t *index,
	uint32_t *ft_hi, uint32_t *ft_lo,
	uint32_t *gt_hi, uint32_t *gt_lo)
{
	if (index != NULL) {
		*index = kg_d4_shrink_block_index;
	}
	if (ft_hi != NULL) {
		*ft_hi = kg_d4_shrink_block_ft_hi;
	}
	if (ft_lo != NULL) {
		*ft_lo = kg_d4_shrink_block_ft_lo;
	}
	if (gt_hi != NULL) {
		*gt_hi = kg_d4_shrink_block_gt_hi;
	}
	if (gt_lo != NULL) {
		*gt_lo = kg_d4_shrink_block_gt_lo;
	}
}

/* see inner.h */
void
ctl_debug_get_d2_shrink_block_stats(
	uint64_t *events, uint64_t *total_bad, uint32_t *last_bad)
{
	if (events != NULL) {
		*events = kg_d2_shrink_block_events;
	}
	if (total_bad != NULL) {
		*total_bad = kg_d2_shrink_block_total_bad;
	}
	if (last_bad != NULL) {
		*last_bad = kg_d2_shrink_block_last_bad;
	}
}

/* see inner.h */
void
ctl_debug_get_d2_shrink_block_sample(
	uint32_t *index,
	uint32_t *ft_hi, uint32_t *ft_lo,
	uint32_t *gt_hi, uint32_t *gt_lo)
{
	if (index != NULL) {
		*index = kg_d2_shrink_block_index;
	}
	if (ft_hi != NULL) {
		*ft_hi = kg_d2_shrink_block_ft_hi;
	}
	if (ft_lo != NULL) {
		*ft_lo = kg_d2_shrink_block_ft_lo;
	}
	if (gt_hi != NULL) {
		*gt_hi = kg_d2_shrink_block_gt_hi;
	}
	if (gt_lo != NULL) {
		*gt_lo = kg_d2_shrink_block_gt_lo;
	}
}

/* see inner.h */
void
ctl_debug_get_compute_w_lim1_stats(
	uint32_t *last_v1_abs, uint32_t *last_lim1, uint32_t *max_v1_abs)
{
	if (last_v1_abs != NULL) {
		*last_v1_abs = kg_cw_last_v1_abs;
	}
	if (last_lim1 != NULL) {
		*last_lim1 = kg_cw_last_lim1;
	}
	if (max_v1_abs != NULL) {
		*max_v1_abs = kg_cw_max_v1_abs;
	}
}

/* ==================================================================== */
/*
 * Modular arithmetics.
 *
 * We implement a few functions for computing modulo a small integer p.
 *
 * All functions require that 2^30 < p < 2^31. Moreover, operands must
 * be in the 0..p-1 range.
 *
 * Modular addition and subtraction work for all such p.
 *
 * Montgomery multiplication requires that p is odd, and must be provided
 * with an additional value p0i = -1/p mod 2^31. See below for some basics
 * on Montgomery multiplication.
 *
 * Division computes an inverse modulo p by an exponentiation (with
 * exponent p-2): this works only if p is prime. Multiplication
 * requirements also apply, i.e. p must be odd and p0i must be provided.
 *
 * The NTT and inverse NTT need all of the above, and also that
 * p = 1 mod 2048.
 *
 * -----------------------------------------------------------------------
 *
 * We use Montgomery representation with 31-bit values:
 *
 *   Let R = 2^31 mod p. When 2^30 < p < 2^31, R = 2^31 - p.
 *   Montgomery representation of an integer x modulo p is x*R mod p.
 *
 *   Montgomery multiplication computes (x*y)/R mod p for
 *   operands x and y. Therefore:
 *
 *    - if operands are x*R and y*R (Montgomery representations of x and
 *      y), then Montgomery multiplication computes (x*R*y*R)/R = (x*y)*R
 *      mod p, which is the Montgomery representation of the product x*y;
 *
 *    - if operands are x*R and y (or x and y*R), then Montgomery
 *      multiplication returns x*y mod p: mixed-representation
 *      multiplications yield results in normal representation.
 *
 * To convert to Montgomery representation, we multiply by R, which is done
 * by Montgomery-multiplying by R^2. Stand-alone conversion back from
 * Montgomery representation is Montgomery-multiplication by 1.
 */

/*
 * Precomputed small primes. Each element contains the following:
 *
 *  p   The prime itself.
 *
 *  g   A primitive root of phi = X^N+1 (in field Z_p).
 *
 *  s   The inverse of the product of all previous primes in the array,
 *      computed modulo p and in Montgomery representation.
 *
 * All primes are such that p = 1 mod 4096, and are lower than 2^31. They
 * are listed in decreasing order.
 */

typedef struct {
	uint32_t p;
	uint32_t g;
	uint32_t s;
} small_prime;

#if 0
static const small_prime PRIMES[] = {
	{ 2147473409,  383167813,      10239 },
	{ 2147389441,  211808905,  471403745 },
	{ 2147387393,   37672282, 1329335065 },
	{ 2147377153, 1977035326,  968223422 },
	{ 2147358721, 1067163706,  132460015 },
	{ 2147352577, 1606082042,  598693809 },
	{ 2147346433, 2033915641, 1056257184 },
	{ 2147338241, 1653770625,  421286710 },
	{ 2147309569,  631200819, 1111201074 },
	{ 2147297281, 2038364663, 1042003613 },
	{ 2147295233, 1962540515,   19440033 },
	{ 2147239937, 2100082663,  353296760 },
	{ 2147235841, 1991153006, 1703918027 },
	{ 2147217409,  516405114, 1258919613 },
	{ 2147205121,  409347988, 1089726929 },
	{ 2147196929,  927788991, 1946238668 },
	{ 2147178497, 1136922411, 1347028164 },
	{ 2147100673,  868626236,  701164723 },
	{ 2147082241, 1897279176,  617820870 },
	{ 2147074049, 1888819123,  158382189 },
	{ 2147051521,   25006327,  522758543 },
	{ 2147043329,  327546255,   37227845 },
	{ 2147039233,  766324424, 1133356428 },
	{ 2146988033, 1862817362,   73861329 },
	{ 2146963457,  404622040,  653019435 },
	{ 2146959361, 1936581214,  995143093 },
	{ 2146938881, 1559770096,  634921513 },
	{ 2146908161,  422623708, 1985060172 },
	{ 2146885633, 1751189170,  298238186 },
	{ 2146871297,  578919515,  291810829 },
	{ 2146846721, 1114060353,  915902322 },
	{ 2146834433, 2069565474,   47859524 },
	{ 2146818049, 1552824584,  646281055 },
	{ 2146775041, 1906267847, 1597832891 },
	{ 2146756609, 1847414714, 1228090888 },
	{ 2146744321, 1818792070, 1176377637 },
	{ 2146738177, 1118066398, 1054971214 },
	{ 2146736129,   52057278,  933422153 },
	{ 2146713601,  592259376, 1406621510 },
	{ 2146695169,  263161877, 1514178701 },
	{ 2146656257,  685363115,  384505091 },
	{ 2146650113,  927727032,  537575289 },
	{ 2146646017,   52575506, 1799464037 },
	{ 2146643969, 1276803876, 1348954416 },
	{ 2146603009,  814028633, 1521547704 },
	{ 2146572289, 1846678872, 1310832121 },
	{ 2146547713,  919368090, 1019041349 },
	{ 2146508801,  671847612,   38582496 },
	{ 2146492417,  283911680,  532424562 },
	{ 2146490369, 1780044827,  896447978 },
	{ 2146459649,  327980850, 1327906900 },
	{ 2146447361, 1310561493,  958645253 },
	{ 2146441217,  412148926,  287271128 },
	{ 2146437121,  293186449, 2009822534 },
	{ 2146430977,  179034356, 1359155584 },
	{ 2146418689, 1517345488, 1790248672 },
	{ 2146406401, 1615820390, 1584833571 },
	{ 2146404353,  826651445,  607120498 },
	{ 2146379777,    3816988, 1897049071 },
	{ 2146363393, 1221409784, 1986921567 },
	{ 2146355201, 1388081168,  849968120 },
	{ 2146336769, 1803473237, 1655544036 },
	{ 2146312193, 1023484977,  273671831 },
	{ 2146293761, 1074591448,  467406983 },
	{ 2146283521,  831604668, 1523950494 },
	{ 2146203649,  712865423, 1170834574 },
	{ 2146154497, 1764991362, 1064856763 },
	{ 2146142209,  627386213, 1406840151 },
	{ 2146127873, 1638674429, 2088393537 },
	{ 2146099201, 1516001018,  690673370 },
	{ 2146093057, 1294931393,  315136610 },
	{ 2146091009, 1942399533,  973539425 },
	{ 2146078721, 1843461814, 2132275436 },
	{ 2146060289, 1098740778,  360423481 },
	{ 2146048001, 1617213232, 1951981294 },
	{ 2146041857, 1805783169, 2075683489 },
	{ 2146019329,  272027909, 1753219918 },
	{ 2145986561, 1206530344, 2034028118 },
	{ 2145976321, 1243769360, 1173377644 },
	{ 2145964033,  887200839, 1281344586 },
	{ 2145906689, 1651026455,  906178216 },
	{ 2145875969, 1673238256, 1043521212 },
	{ 2145871873, 1226591210, 1399796492 },
	{ 2145841153, 1465353397, 1324527802 },
	{ 2145832961, 1150638905,  554084759 },
	{ 2145816577,  221601706,  427340863 },
	{ 2145785857,  608896761,  316590738 },
	{ 2145755137, 1712054942, 1684294304 },
	{ 2145742849, 1302302867,  724873116 },
	{ 2145728513,  516717693,  431671476 },
	{ 2145699841,  524575579, 1619722537 },
	{ 2145691649, 1925625239,  982974435 },
	{ 2145687553,  463795662, 1293154300 },
	{ 2145673217,  771716636,  881778029 },
	{ 2145630209, 1509556977,  837364988 },
	{ 2145595393,  229091856,  851648427 },
	{ 2145587201, 1796903241,  635342424 },
	{ 2145525761,  715310882, 1677228081 },
	{ 2145495041, 1040930522,  200685896 },
	{ 2145466369,  949804237, 1809146322 },
	{ 2145445889, 1673903706,   95316881 },
	{ 2145390593,  806941852, 1428671135 },
	{ 2145372161, 1402525292,  159350694 },
	{ 2145361921, 2124760298, 1589134749 },
	{ 2145359873, 1217503067, 1561543010 },
	{ 2145355777,  338341402,   83865711 },
	{ 2145343489, 1381532164,  641430002 },
	{ 2145325057, 1883895478, 1528469895 },
	{ 2145318913, 1335370424,   65809740 },
	{ 2145312769, 2000008042, 1919775760 },
	{ 2145300481,  961450962, 1229540578 },
	{ 2145282049,  910466767, 1964062701 },
	{ 2145232897,  816527501,  450152063 },
	{ 2145218561, 1435128058, 1794509700 },
	{ 2145187841,   33505311, 1272467582 },
	{ 2145181697,  269767433, 1380363849 },
	{ 2145175553,   56386299, 1316870546 },
	{ 2145079297, 2106880293, 1391797340 },
	{ 2145021953, 1347906152,  720510798 },
	{ 2145015809,  206769262, 1651459955 },
	{ 2145003521, 1885513236, 1393381284 },
	{ 2144960513, 1810381315,   31937275 },
	{ 2144944129, 1306487838, 2019419520 },
	{ 2144935937,   37304730, 1841489054 },
	{ 2144894977, 1601434616,  157985831 },
	{ 2144888833,   98749330, 2128592228 },
	{ 2144880641, 1772327002, 2076128344 },
	{ 2144864257, 1404514762, 2029969964 },
	{ 2144827393,  801236594,  406627220 },
	{ 2144806913,  349217443, 1501080290 },
	{ 2144796673, 1542656776, 2084736519 },
	{ 2144778241, 1210734884, 1746416203 },
	{ 2144759809, 1146598851,  716464489 },
	{ 2144757761,  286328400, 1823728177 },
	{ 2144729089, 1347555695, 1836644881 },
	{ 2144727041, 1795703790,  520296412 },
	{ 2144696321, 1302475157,  852964281 },
	{ 2144667649, 1075877614,  504992927 },
	{ 2144573441,  198765808, 1617144982 },
	{ 2144555009,  321528767,  155821259 },
	{ 2144550913,  814139516, 1819937644 },
	{ 2144536577,  571143206,  962942255 },
	{ 2144524289, 1746733766,    2471321 },
	{ 2144512001, 1821415077,  124190939 },
	{ 2144468993,  917871546, 1260072806 },
	{ 2144458753,  378417981, 1569240563 },
	{ 2144421889,  175229668, 1825620763 },
	{ 2144409601, 1699216963,  351648117 },
	{ 2144370689, 1071885991,  958186029 },
	{ 2144348161, 1763151227,  540353574 },
	{ 2144335873, 1060214804,  919598847 },
	{ 2144329729,  663515846, 1448552668 },
	{ 2144327681, 1057776305,  590222840 },
	{ 2144309249, 1705149168, 1459294624 },
	{ 2144296961,  325823721, 1649016934 },
	{ 2144290817,  738775789,  447427206 },
	{ 2144243713,  962347618,  893050215 },
	{ 2144237569, 1655257077,  900860862 },
	{ 2144161793,  242206694, 1567868672 },
	{ 2144155649,  769415308, 1247993134 },
	{ 2144137217,  320492023,  515841070 },
	{ 2144120833, 1639388522,  770877302 },
	{ 2144071681, 1761785233,  964296120 },
	{ 2144065537,  419817825,  204564472 },
	{ 2144028673,  666050597, 2091019760 },
	{ 2144010241, 1413657615, 1518702610 },
	{ 2143952897, 1238327946,  475672271 },
	{ 2143940609,  307063413, 1176750846 },
	{ 2143918081, 2062905559,  786785803 },
	{ 2143899649, 1338112849, 1562292083 },
	{ 2143891457,   68149545,   87166451 },
	{ 2143885313,  921750778,  394460854 },
	{ 2143854593,  719766593,  133877196 },
	{ 2143836161, 1149399850, 1861591875 },
	{ 2143762433, 1848739366, 1335934145 },
	{ 2143756289, 1326674710,  102999236 },
	{ 2143713281,  808061791, 1156900308 },
	{ 2143690753,  388399459, 1926468019 },
	{ 2143670273, 1427891374, 1756689401 },
	{ 2143666177, 1912173949,  986629565 },
	{ 2143645697, 2041160111,  371842865 },
	{ 2143641601, 1279906897, 2023974350 },
	{ 2143635457,  720473174, 1389027526 },
	{ 2143621121, 1298309455, 1732632006 },
	{ 2143598593, 1548762216, 1825417506 },
	{ 2143567873,  620475784, 1073787233 },
	{ 2143561729, 1932954575,  949167309 },
	{ 2143553537,  354315656, 1652037534 },
	{ 2143541249,  577424288, 1097027618 },
	{ 2143531009,  357862822,  478640055 },
	{ 2143522817, 2017706025, 1550531668 },
	{ 2143506433, 2078127419, 1824320165 },
	{ 2143488001,  613475285, 1604011510 },
	{ 2143469569, 1466594987,  502095196 },
	{ 2143426561, 1115430331, 1044637111 },
	{ 2143383553,    9778045, 1902463734 },
	{ 2143377409, 1557401276, 2056861771 },
	{ 2143363073,  652036455, 1965915971 },
	{ 2143260673, 1464581171, 1523257541 },
	{ 2143246337, 1876119649,  764541916 },
	{ 2143209473, 1614992673, 1920672844 },
	{ 2143203329,  981052047, 2049774209 },
	{ 2143160321, 1847355533,  728535665 },
	{ 2143129601,  965558457,  603052992 },
	{ 2143123457, 2140817191,    8348679 },
	{ 2143100929, 1547263683,  694209023 },
	{ 2143092737,  643459066, 1979934533 },
	{ 2143082497,  188603778, 2026175670 },
	{ 2143062017, 1657329695,  377451099 },
	{ 2143051777,  114967950,  979255473 },
	{ 2143025153, 1698431342, 1449196896 },
	{ 2143006721, 1862741675, 1739650365 },
	{ 2142996481,  756660457,  996160050 },
	{ 2142976001,  927864010, 1166847574 },
	{ 2142965761,  905070557,  661974566 },
	{ 2142916609,   40932754, 1787161127 },
	{ 2142892033, 1987985648,  675335382 },
	{ 2142885889,  797497211, 1323096997 },
	{ 2142871553, 2068025830, 1411877159 },
	{ 2142861313, 1217177090, 1438410687 },
	{ 2142830593,  409906375, 1767860634 },
	{ 2142803969, 1197788993,  359782919 },
	{ 2142785537,  643817365,  513932862 },
	{ 2142779393, 1717046338,  218943121 },
	{ 2142724097,   89336830,  416687049 },
	{ 2142707713,    5944581, 1356813523 },
	{ 2142658561,  887942135, 2074011722 },
	{ 2142638081,  151851972, 1647339939 },
	{ 2142564353, 1691505537, 1483107336 },
	{ 2142533633, 1989920200, 1135938817 },
	{ 2142529537,  959263126, 1531961857 },
	{ 2142527489,  453251129, 1725566162 },
	{ 2142502913, 1536028102,  182053257 },
	{ 2142498817,  570138730,  701443447 },
	{ 2142416897,  326965800,  411931819 },
	{ 2142363649, 1675665410, 1517191733 },
	{ 2142351361,  968529566, 1575712703 },
	{ 2142330881, 1384953238, 1769087884 },
	{ 2142314497, 1977173242, 1833745524 },
	{ 2142289921,   95082313, 1714775493 },
	{ 2142283777,  109377615, 1070584533 },
	{ 2142277633,   16960510,  702157145 },
	{ 2142263297,  553850819,  431364395 },
	{ 2142208001,  241466367, 2053967982 },
	{ 2142164993, 1795661326, 1031836848 },
	{ 2142097409, 1212530046,  712772031 },
	{ 2142087169, 1763869720,  822276067 },
	{ 2142078977,  644065713, 1765268066 },
	{ 2142074881,  112671944,  643204925 },
	{ 2142044161, 1387785471, 1297890174 },
	{ 2142025729,  783885537, 1000425730 },
	{ 2142011393,  905662232, 1679401033 },
	{ 2141974529,  799788433,  468119557 },
	{ 2141943809, 1932544124,  449305555 },
	{ 2141933569, 1527403256,  841867925 },
	{ 2141931521, 1247076451,  743823916 },
	{ 2141902849, 1199660531,  401687910 },
	{ 2141890561,  150132350, 1720336972 },
	{ 2141857793, 1287438162,  663880489 },
	{ 2141833217,  618017731, 1819208266 },
	{ 2141820929,  999578638, 1403090096 },
	{ 2141786113,   81834325, 1523542501 },
	{ 2141771777,  120001928,  463556492 },
	{ 2141759489,  122455485, 2124928282 },
	{ 2141749249,  141986041,  940339153 },
	{ 2141685761,  889088734,  477141499 },
	{ 2141673473,  324212681, 1122558298 },
	{ 2141669377, 1175806187, 1373818177 },
	{ 2141655041, 1113654822,  296887082 },
	{ 2141587457,  991103258, 1585913875 },
	{ 2141583361, 1401451409, 1802457360 },
	{ 2141575169, 1571977166,  712760980 },
	{ 2141546497, 1107849376, 1250270109 },
	{ 2141515777,  196544219,  356001130 },
	{ 2141495297, 1733571506, 1060744866 },
	{ 2141483009,  321552363, 1168297026 },
	{ 2141458433,  505818251,  733225819 },
	{ 2141360129, 1026840098,  948342276 },
	{ 2141325313,  945133744, 2129965998 },
	{ 2141317121, 1871100260, 1843844634 },
	{ 2141286401, 1790639498, 1750465696 },
	{ 2141267969, 1376858592,  186160720 },
	{ 2141255681, 2129698296, 1876677959 },
	{ 2141243393, 2138900688, 1340009628 },
	{ 2141214721, 1933049835, 1087819477 },
	{ 2141212673, 1898664939, 1786328049 },
	{ 2141202433,  990234828,  940682169 },
	{ 2141175809, 1406392421,  993089586 },
	{ 2141165569, 1263518371,  289019479 },
	{ 2141073409, 1485624211,  507864514 },
	{ 2141052929, 1885134788,  311252465 },
	{ 2141040641, 1285021247,  280941862 },
	{ 2141028353, 1527610374,  375035110 },
	{ 2141011969, 1400626168,  164696620 },
	{ 2140999681,  632959608,  966175067 },
	{ 2140997633, 2045628978, 1290889438 },
	{ 2140993537, 1412755491,  375366253 },
	{ 2140942337,  719477232,  785367828 },
	{ 2140925953,   45224252,  836552317 },
	{ 2140917761, 1157376588, 1001839569 },
	{ 2140887041,  278480752, 2098732796 },
	{ 2140837889, 1663139953,  924094810 },
	{ 2140788737,  802501511, 2045368990 },
	{ 2140766209, 1820083885, 1800295504 },
	{ 2140764161, 1169561905, 2106792035 },
	{ 2140696577,  127781498, 1885987531 },
	{ 2140684289,   16014477, 1098116827 },
	{ 2140653569,  665960598, 1796728247 },
	{ 2140594177, 1043085491,  377310938 },
	{ 2140579841, 1732838211, 1504505945 },
	{ 2140569601,  302071939,  358291016 },
	{ 2140567553,  192393733, 1909137143 },
	{ 2140557313,  406595731, 1175330270 },
	{ 2140549121, 1748850918,  525007007 },
	{ 2140477441,  499436566, 1031159814 },
	{ 2140469249, 1886004401, 1029951320 },
	{ 2140426241, 1483168100, 1676273461 },
	{ 2140420097, 1779917297,  846024476 },
	{ 2140413953,  522948893, 1816354149 },
	{ 2140383233, 1931364473, 1296921241 },
	{ 2140366849, 1917356555,  147196204 },
	{ 2140354561,   16466177, 1349052107 },
	{ 2140348417, 1875366972, 1860485634 },
	{ 2140323841,  456498717, 1790256483 },
	{ 2140321793, 1629493973,  150031888 },
	{ 2140315649, 1904063898,  395510935 },
	{ 2140280833, 1784104328,  831417909 },
	{ 2140250113,  256087139,  697349101 },
	{ 2140229633,  388553070,  243875754 },
	{ 2140223489,  747459608, 1396270850 },
	{ 2140200961,  507423743, 1895572209 },
	{ 2140162049,  580106016, 2045297469 },
	{ 2140149761,  712426444,  785217995 },
	{ 2140137473, 1441607584,  536866543 },
	{ 2140119041,  346538902, 1740434653 },
	{ 2140090369,  282642885,   21051094 },
	{ 2140076033, 1407456228,  319910029 },
	{ 2140047361, 1619330500, 1488632070 },
	{ 2140041217, 2089408064, 2012026134 },
	{ 2140008449, 1705524800, 1613440760 },
	{ 2139924481, 1846208233, 1280649481 },
	{ 2139906049,  989438755, 1185646076 },
	{ 2139867137, 1522314850,  372783595 },
	{ 2139842561, 1681587377,  216848235 },
	{ 2139826177, 2066284988, 1784999464 },
	{ 2139824129,  480888214, 1513323027 },
	{ 2139789313,  847937200,  858192859 },
	{ 2139783169, 1642000434, 1583261448 },
	{ 2139770881,  940699589,  179702100 },
	{ 2139768833,  315623242,  964612676 },
	{ 2139666433,  331649203,  764666914 },
	{ 2139641857, 2118730799, 1313764644 },
	{ 2139635713,  519149027,  519212449 },
	{ 2139598849, 1526413634, 1769667104 },
	{ 2139574273,  551148610,  820739925 },
	{ 2139568129, 1386800242,  472447405 },
	{ 2139549697,  813760130, 1412328531 },
	{ 2139537409, 1615286260, 1609362979 },
	{ 2139475969, 1352559299, 1696720421 },
	{ 2139455489, 1048691649, 1584935400 },
	{ 2139432961,  836025845,  950121150 },
	{ 2139424769, 1558281165, 1635486858 },
	{ 2139406337, 1728402143, 1674423301 },
	{ 2139396097, 1727715782, 1483470544 },
	{ 2139383809, 1092853491, 1741699084 },
	{ 2139369473,  690776899, 1242798709 },
	{ 2139351041, 1768782380, 2120712049 },
	{ 2139334657, 1739968247, 1427249225 },
	{ 2139332609, 1547189119,  623011170 },
	{ 2139310081, 1346827917, 1605466350 },
	{ 2139303937,  369317948,  828392831 },
	{ 2139301889, 1560417239, 1788073219 },
	{ 2139283457, 1303121623,  595079358 },
	{ 2139248641, 1354555286,  573424177 },
	{ 2139240449,   60974056,  885781403 },
	{ 2139222017,  355573421, 1221054839 },
	{ 2139215873,  566477826, 1724006500 },
	{ 2139150337,  871437673, 1609133294 },
	{ 2139144193, 1478130914, 1137491905 },
	{ 2139117569, 1854880922,  964728507 },
	{ 2139076609,  202405335,  756508944 },
	{ 2139062273, 1399715741,  884826059 },
	{ 2139045889, 1051045798, 1202295476 },
	{ 2139033601, 1707715206,  632234634 },
	{ 2139006977, 2035853139,  231626690 },
	{ 2138951681,  183867876,  838350879 },
	{ 2138945537, 1403254661,  404460202 },
	{ 2138920961,  310865011, 1282911681 },
	{ 2138910721, 1328496553,  103472415 },
	{ 2138904577,   78831681,  993513549 },
	{ 2138902529, 1319697451, 1055904361 },
	{ 2138816513,  384338872, 1706202469 },
	{ 2138810369, 1084868275,  405677177 },
	{ 2138787841,  401181788, 1964773901 },
	{ 2138775553, 1850532988, 1247087473 },
	{ 2138767361,  874261901, 1576073565 },
	{ 2138757121, 1187474742,  993541415 },
	{ 2138748929, 1782458888, 1043206483 },
	{ 2138744833, 1221500487,  800141243 },
	{ 2138738689,  413465368, 1450660558 },
	{ 2138695681,  739045140,  342611472 },
	{ 2138658817, 1355845756,  672674190 },
	{ 2138644481,  608379162, 1538874380 },
	{ 2138632193, 1444914034,  686911254 },
	{ 2138607617,  484707818, 1435142134 },
	{ 2138591233,  539460669, 1290458549 },
	{ 2138572801, 2093538990, 2011138646 },
	{ 2138552321, 1149786988, 1076414907 },
	{ 2138546177,  840688206, 2108985273 },
	{ 2138533889,  209669619,  198172413 },
	{ 2138523649, 1975879426, 1277003968 },
	{ 2138490881, 1351891144, 1976858109 },
	{ 2138460161, 1817321013, 1979278293 },
	{ 2138429441, 1950077177,  203441928 },
	{ 2138400769,  908970113,  628395069 },
	{ 2138398721,  219890864,  758486760 },
	{ 2138376193, 1306654379,  977554090 },
	{ 2138351617,  298822498, 2004708503 },
	{ 2138337281,  441457816, 1049002108 },
	{ 2138320897, 1517731724, 1442269609 },
	{ 2138290177, 1355911197, 1647139103 },
	{ 2138234881,  531313247, 1746591962 },
	{ 2138214401, 1899410930,  781416444 },
	{ 2138202113, 1813477173, 1622508515 },
	{ 2138191873, 1086458299, 1025408615 },
	{ 2138183681, 1998800427,  827063290 },
	{ 2138173441, 1921308898,  749670117 },
	{ 2138103809, 1620902804, 2126787647 },
	{ 2138099713,  828647069, 1892961817 },
	{ 2138085377,  179405355, 1525506535 },
	{ 2138060801,  615683235, 1259580138 },
	{ 2138044417, 2030277840, 1731266562 },
	{ 2138042369, 2087222316, 1627902259 },
	{ 2138032129,  126388712, 1108640984 },
	{ 2138011649,  715026550, 1017980050 },
	{ 2137993217, 1693714349, 1351778704 },
	{ 2137888769, 1289762259, 1053090405 },
	{ 2137853953,  199991890, 1254192789 },
	{ 2137833473,  941421685,  896995556 },
	{ 2137817089,  750416446, 1251031181 },
	{ 2137792513,  798075119,  368077456 },
	{ 2137786369,  878543495, 1035375025 },
	{ 2137767937,    9351178, 1156563902 },
	{ 2137755649, 1382297614, 1686559583 },
	{ 2137724929, 1345472850, 1681096331 },
	{ 2137704449,  834666929,  630551727 },
	{ 2137673729, 1646165729, 1892091571 },
	{ 2137620481,  778943821,   48456461 },
	{ 2137618433, 1730837875, 1713336725 },
	{ 2137581569,  805610339, 1378891359 },
	{ 2137538561,  204342388, 1950165220 },
	{ 2137526273, 1947629754, 1500789441 },
	{ 2137516033,  719902645, 1499525372 },
	{ 2137491457,  230451261,  556382829 },
	{ 2137440257,  979573541,  412760291 },
	{ 2137374721,  927841248, 1954137185 },
	{ 2137362433, 1243778559,  861024672 },
	{ 2137313281, 1341338501,  980638386 },
	{ 2137311233,  937415182, 1793212117 },
	{ 2137255937,  795331324, 1410253405 },
	{ 2137243649,  150756339, 1966999887 },
	{ 2137182209,  163346914, 1939301431 },
	{ 2137171969, 1952552395,  758913141 },
	{ 2137159681,  570788721,  218668666 },
	{ 2137147393, 1896656810, 2045670345 },
	{ 2137141249,  358493842,  518199643 },
	{ 2137139201, 1505023029,  674695848 },
	{ 2137133057,   27911103,  830956306 },
	{ 2137122817,  439771337, 1555268614 },
	{ 2137116673,  790988579, 1871449599 },
	{ 2137110529,  432109234,  811805080 },
	{ 2137102337, 1357900653, 1184997641 },
	{ 2137098241,  515119035, 1715693095 },
	{ 2137090049,  408575203, 2085660657 },
	{ 2137085953, 2097793407, 1349626963 },
	{ 2137055233, 1556739954, 1449960883 },
	{ 2137030657, 1545758650, 1369303716 },
	{ 2136987649,  332602570,  103875114 },
	{ 2136969217, 1499989506, 1662964115 },
	{ 2136924161,  857040753,    4738842 },
	{ 2136895489, 1948872712,  570436091 },
	{ 2136893441,   58969960, 1568349634 },
	{ 2136887297, 2127193379,  273612548 },
	{ 2136850433,  111208983, 1181257116 },
	{ 2136809473, 1627275942, 1680317971 },
	{ 2136764417, 1574888217,   14011331 },
	{ 2136741889,   14011055, 1129154251 },
	{ 2136727553,   35862563, 1838555253 },
	{ 2136721409,  310235666, 1363928244 },
	{ 2136698881, 1612429202, 1560383828 },
	{ 2136649729, 1138540131,  800014364 },
	{ 2136606721,  602323503, 1433096652 },
	{ 2136563713,  182209265, 1919611038 },
	{ 2136555521,  324156477,  165591039 },
	{ 2136549377,  195513113,  217165345 },
	{ 2136526849, 1050768046,  939647887 },
	{ 2136508417, 1886286237, 1619926572 },
	{ 2136477697,  609647664,   35065157 },
	{ 2136471553,  679352216, 1452259468 },
	{ 2136457217,  128630031,  824816521 },
	{ 2136422401,   19787464, 1526049830 },
	{ 2136420353,  698316836, 1530623527 },
	{ 2136371201, 1651862373, 1804812805 },
	{ 2136334337,  326596005,  336977082 },
	{ 2136322049,   63253370, 1904972151 },
	{ 2136297473,  312176076,  172182411 },
	{ 2136248321,  381261841,  369032670 },
	{ 2136242177,  358688773, 1640007994 },
	{ 2136229889,  512677188,   75585225 },
	{ 2136219649, 2095003250, 1970086149 },
	{ 2136207361, 1909650722,  537760675 },
	{ 2136176641, 1334616195, 1533487619 },
	{ 2136158209, 2096285632, 1793285210 },
	{ 2136143873, 1897347517,  293843959 },
	{ 2136133633,  923586222, 1022655978 },
	{ 2136096769, 1464868191, 1515074410 },
	{ 2136094721, 2020679520, 2061636104 },
	{ 2136076289,  290798503, 1814726809 },
	{ 2136041473,  156415894, 1250757633 },
	{ 2135996417,  297459940, 1132158924 },
	{ 2135955457,  538755304, 1688831340 },
	{ 0, 0, 0 }
};
#endif

#include "keygen_primes_4096.h"

/*
 * Reduce a small signed integer modulo a small prime. The source
 * value x MUST be such that -p < x < p.
 */
static inline uint32_t
modp_set(int32_t x, uint32_t p)
{
	uint32_t w;

	w = (uint32_t)x;
	w += p & -(w >> 31);
	return w;
}

/*
 * Normalize a modular integer around 0.
 */
static inline int32_t
modp_norm(uint32_t x, uint32_t p)
{
	return (int32_t)(x - (p & (((x - ((p + 1) >> 1)) >> 31) - 1)));
}

/*
 * Compute -1/p mod 2^31. This works for all odd integers p that fit
 * on 31 bits.
 */
static uint32_t
modp_ninv31(uint32_t p)
{
	uint32_t y;

	y = 2 - p;
	y *= 2 - p * y;
	y *= 2 - p * y;
	y *= 2 - p * y;
	y *= 2 - p * y;
	return (uint32_t)0x7FFFFFFF & -y;
}

/*
 * Compute R = 2^31 mod p.
 */
static inline uint32_t
modp_R(uint32_t p)
{
	/*
	 * Since 2^30 < p < 2^31, we know that 2^31 mod p is simply
	 * 2^31 - p.
	 */
	return ((uint32_t)1 << 31) - p;
}

/*
 * Addition modulo p.
 */
static inline uint32_t
modp_add(uint32_t a, uint32_t b, uint32_t p)
{
	uint32_t d;

	d = a + b - p;
	d += p & -(d >> 31);
	return d;
}

/*
 * Subtraction modulo p.
 */
static inline uint32_t
modp_sub(uint32_t a, uint32_t b, uint32_t p)
{
	uint32_t d;

	d = a - b;
	d += p & -(d >> 31);
	return d;
}

/*
 * Montgomery multiplication modulo p. The 'p0i' value is -1/p mod 2^31.
 * It is required that p is an odd integer.
 */
static inline uint32_t
modp_montymul(uint32_t a, uint32_t b, uint32_t p, uint32_t p0i)
{
	uint64_t z, w;
	uint32_t d;

	z = (uint64_t)a * (uint64_t)b;
	w = ((z * p0i) & (uint64_t)0x7FFFFFFF) * p;
	d = (uint32_t)((z + w) >> 31) - p;
	d += p & -(d >> 31);
	return d;
}

/*
 * Compute R2 = 2^62 mod p.
 */
static uint32_t
modp_R2(uint32_t p, uint32_t p0i)
{
	uint32_t z;

	/*
	 * Compute z = 2^31 mod p (this is the value 1 in Montgomery
	 * representation), then double it with an addition.
	 */
	z = modp_R(p);
	z = modp_add(z, z, p);

	/*
	 * Square it five times to obtain 2^32 in Montgomery representation
	 * (i.e. 2^63 mod p).
	 */
	z = modp_montymul(z, z, p, p0i);
	z = modp_montymul(z, z, p, p0i);
	z = modp_montymul(z, z, p, p0i);
	z = modp_montymul(z, z, p, p0i);
	z = modp_montymul(z, z, p, p0i);

	/*
	 * Halve the value mod p to get 2^62.
	 */
	z = (z + (p & -(z & 1))) >> 1;
	return z;
}

/*
 * Compute 2^(31*x) modulo p. This works for integers x up to 2^11.
 * p must be prime such that 2^30 < p < 2^31; p0i must be equal to
 * -1/p mod 2^31; R2 must be equal to 2^62 mod p.
 */
static inline uint32_t
modp_Rx(unsigned x, uint32_t p, uint32_t p0i, uint32_t R2)
{
	int i;
	uint32_t r, z;

	/*
	 * 2^(31*x) = (2^31)*(2^(31*(x-1))); i.e. we want the Montgomery
	 * representation of (2^31)^e mod p, where e = x-1.
	 * R2 is 2^31 in Montgomery representation.
	 */
	x --;
	r = R2;
	z = modp_R(p);
	for (i = 0; (1U << i) <= x; i ++) {
		if ((x & (1U << i)) != 0) {
			z = modp_montymul(z, r, p, p0i);
		}
		r = modp_montymul(r, r, p, p0i);
	}
	return z;
}

/*
 * Division modulo p. If the divisor (b) is 0, then 0 is returned.
 * This function computes proper results only when p is prime.
 * Parameters:
 *   a     dividend
 *   b     divisor
 *   p     odd prime modulus
 *   p0i   -1/p mod 2^31
 *   R     2^31 mod R
 */
static uint32_t
modp_div(uint32_t a, uint32_t b, uint32_t p, uint32_t p0i, uint32_t R)
{
	uint32_t z, e;
	int i;

	e = p - 2;
	z = R;
	for (i = 30; i >= 0; i --) {
		uint32_t z2;

		z = modp_montymul(z, z, p, p0i);
		z2 = modp_montymul(z, b, p, p0i);
		z ^= (z ^ z2) & -(uint32_t)((e >> i) & 1);
	}

	/*
	 * The loop above just assumed that b was in Montgomery
	 * representation, i.e. really contained b*R; under that
	 * assumption, it returns 1/b in Montgomery representation,
	 * which is R/b. But we gave it b in normal representation,
	 * so the loop really returned R/(b/R) = R^2/b.
	 *
	 * We want a/b, so we need one Montgomery multiplication with a,
	 * which also remove one of the R factors, and another such
	 * multiplication to remove the second R factor.
	 */
	z = modp_montymul(z, 1, p, p0i);
	return modp_montymul(a, z, p, p0i);
}

/*
 * Legacy 10-bit bit-reversal table kept only for reference.
 */
#if 0
static const uint16_t REV10[] = {
	   0,  512,  256,  768,  128,  640,  384,  896,   64,  576,  320,  832,
	 192,  704,  448,  960,   32,  544,  288,  800,  160,  672,  416,  928,
	  96,  608,  352,  864,  224,  736,  480,  992,   16,  528,  272,  784,
	 144,  656,  400,  912,   80,  592,  336,  848,  208,  720,  464,  976,
	  48,  560,  304,  816,  176,  688,  432,  944,  112,  624,  368,  880,
	 240,  752,  496, 1008,    8,  520,  264,  776,  136,  648,  392,  904,
	  72,  584,  328,  840,  200,  712,  456,  968,   40,  552,  296,  808,
	 168,  680,  424,  936,  104,  616,  360,  872,  232,  744,  488, 1000,
	  24,  536,  280,  792,  152,  664,  408,  920,   88,  600,  344,  856,
	 216,  728,  472,  984,   56,  568,  312,  824,  184,  696,  440,  952,
	 120,  632,  376,  888,  248,  760,  504, 1016,    4,  516,  260,  772,
	 132,  644,  388,  900,   68,  580,  324,  836,  196,  708,  452,  964,
	  36,  548,  292,  804,  164,  676,  420,  932,  100,  612,  356,  868,
	 228,  740,  484,  996,   20,  532,  276,  788,  148,  660,  404,  916,
	  84,  596,  340,  852,  212,  724,  468,  980,   52,  564,  308,  820,
	 180,  692,  436,  948,  116,  628,  372,  884,  244,  756,  500, 1012,
	  12,  524,  268,  780,  140,  652,  396,  908,   76,  588,  332,  844,
	 204,  716,  460,  972,   44,  556,  300,  812,  172,  684,  428,  940,
	 108,  620,  364,  876,  236,  748,  492, 1004,   28,  540,  284,  796,
	 156,  668,  412,  924,   92,  604,  348,  860,  220,  732,  476,  988,
	  60,  572,  316,  828,  188,  700,  444,  956,  124,  636,  380,  892,
	 252,  764,  508, 1020,    2,  514,  258,  770,  130,  642,  386,  898,
	  66,  578,  322,  834,  194,  706,  450,  962,   34,  546,  290,  802,
	 162,  674,  418,  930,   98,  610,  354,  866,  226,  738,  482,  994,
	  18,  530,  274,  786,  146,  658,  402,  914,   82,  594,  338,  850,
	 210,  722,  466,  978,   50,  562,  306,  818,  178,  690,  434,  946,
	 114,  626,  370,  882,  242,  754,  498, 1010,   10,  522,  266,  778,
	 138,  650,  394,  906,   74,  586,  330,  842,  202,  714,  458,  970,
	  42,  554,  298,  810,  170,  682,  426,  938,  106,  618,  362,  874,
	 234,  746,  490, 1002,   26,  538,  282,  794,  154,  666,  410,  922,
	  90,  602,  346,  858,  218,  730,  474,  986,   58,  570,  314,  826,
	 186,  698,  442,  954,  122,  634,  378,  890,  250,  762,  506, 1018,
	   6,  518,  262,  774,  134,  646,  390,  902,   70,  582,  326,  838,
	 198,  710,  454,  966,   38,  550,  294,  806,  166,  678,  422,  934,
	 102,  614,  358,  870,  230,  742,  486,  998,   22,  534,  278,  790,
	 150,  662,  406,  918,   86,  598,  342,  854,  214,  726,  470,  982,
	  54,  566,  310,  822,  182,  694,  438,  950,  118,  630,  374,  886,
	 246,  758,  502, 1014,   14,  526,  270,  782,  142,  654,  398,  910,
	  78,  590,  334,  846,  206,  718,  462,  974,   46,  558,  302,  814,
	 174,  686,  430,  942,  110,  622,  366,  878,  238,  750,  494, 1006,
	  30,  542,  286,  798,  158,  670,  414,  926,   94,  606,  350,  862,
	 222,  734,  478,  990,   62,  574,  318,  830,  190,  702,  446,  958,
	 126,  638,  382,  894,  254,  766,  510, 1022,    1,  513,  257,  769,
	 129,  641,  385,  897,   65,  577,  321,  833,  193,  705,  449,  961,
	  33,  545,  289,  801,  161,  673,  417,  929,   97,  609,  353,  865,
	 225,  737,  481,  993,   17,  529,  273,  785,  145,  657,  401,  913,
	  81,  593,  337,  849,  209,  721,  465,  977,   49,  561,  305,  817,
	 177,  689,  433,  945,  113,  625,  369,  881,  241,  753,  497, 1009,
	   9,  521,  265,  777,  137,  649,  393,  905,   73,  585,  329,  841,
	 201,  713,  457,  969,   41,  553,  297,  809,  169,  681,  425,  937,
	 105,  617,  361,  873,  233,  745,  489, 1001,   25,  537,  281,  793,
	 153,  665,  409,  921,   89,  601,  345,  857,  217,  729,  473,  985,
	  57,  569,  313,  825,  185,  697,  441,  953,  121,  633,  377,  889,
	 249,  761,  505, 1017,    5,  517,  261,  773,  133,  645,  389,  901,
	  69,  581,  325,  837,  197,  709,  453,  965,   37,  549,  293,  805,
	 165,  677,  421,  933,  101,  613,  357,  869,  229,  741,  485,  997,
	  21,  533,  277,  789,  149,  661,  405,  917,   85,  597,  341,  853,
	 213,  725,  469,  981,   53,  565,  309,  821,  181,  693,  437,  949,
	 117,  629,  373,  885,  245,  757,  501, 1013,   13,  525,  269,  781,
	 141,  653,  397,  909,   77,  589,  333,  845,  205,  717,  461,  973,
	  45,  557,  301,  813,  173,  685,  429,  941,  109,  621,  365,  877,
	 237,  749,  493, 1005,   29,  541,  285,  797,  157,  669,  413,  925,
	  93,  605,  349,  861,  221,  733,  477,  989,   61,  573,  317,  829,
	 189,  701,  445,  957,  125,  637,  381,  893,  253,  765,  509, 1021,
	   3,  515,  259,  771,  131,  643,  387,  899,   67,  579,  323,  835,
	 195,  707,  451,  963,   35,  547,  291,  803,  163,  675,  419,  931,
	  99,  611,  355,  867,  227,  739,  483,  995,   19,  531,  275,  787,
	 147,  659,  403,  915,   83,  595,  339,  851,  211,  723,  467,  979,
	  51,  563,  307,  819,  179,  691,  435,  947,  115,  627,  371,  883,
	 243,  755,  499, 1011,   11,  523,  267,  779,  139,  651,  395,  907,
	  75,  587,  331,  843,  203,  715,  459,  971,   43,  555,  299,  811,
	 171,  683,  427,  939,  107,  619,  363,  875,  235,  747,  491, 1003,
	  27,  539,  283,  795,  155,  667,  411,  923,   91,  603,  347,  859,
	 219,  731,  475,  987,   59,  571,  315,  827,  187,  699,  443,  955,
	 123,  635,  379,  891,  251,  763,  507, 1019,    7,  519,  263,  775,
	 135,  647,  391,  903,   71,  583,  327,  839,  199,  711,  455,  967,
	  39,  551,  295,  807,  167,  679,  423,  935,  103,  615,  359,  871,
	 231,  743,  487,  999,   23,  535,  279,  791,  151,  663,  407,  919,
	  87,  599,  343,  855,  215,  727,  471,  983,   55,  567,  311,  823,
	 183,  695,  439,  951,  119,  631,  375,  887,  247,  759,  503, 1015,
	  15,  527,  271,  783,  143,  655,  399,  911,   79,  591,  335,  847,
	 207,  719,  463,  975,   47,  559,  303,  815,  175,  687,  431,  943,
	 111,  623,  367,  879,  239,  751,  495, 1007,   31,  543,  287,  799,
	 159,  671,  415,  927,   95,  607,  351,  863,  223,  735,  479,  991,
	  63,  575,  319,  831,  191,  703,  447,  959,  127,  639,  383,  895,
	 255,  767,  511, 1023
};
#endif

static size_t
modp_bitrev(size_t x, unsigned logn)
{
	size_t r;
	unsigned u;

	r = 0;
	for (u = 0; u < logn; u ++) {
		r = (r << 1) | (x & 1);
		x >>= 1;
	}
	return r;
}

/*
 * Compute the roots for NTT and inverse NTT. Input parameter g is a
 * primitive 4096-th root of 1 modulo p (i.e. g^2048 = -1 mod p). This
 * fills gm[] and igm[] with powers of g and 1/g:
 *   gm[rev(i)] = g^i mod p
 *   igm[rev(i)] = (1/g)^i mod p
 * where rev() is the bit reversal function over logn bits. It fills
 * the arrays only up to N = 2^logn values.
 *
 * The values stored in gm[] and igm[] are in Montgomery representation.
 *
 * p must be a prime such that p = 1 mod 4096.
 */
static void
modp_mkgm(uint32_t *restrict gm, uint32_t *restrict igm, unsigned logn,
	uint32_t g, uint32_t p, uint32_t p0i)
{
	size_t u, n;
	unsigned k;
	uint32_t ig, x1, x2, R2;

	n = (size_t)1 << logn;

	/*
	 * We want g such that g^(2N) = 1 mod p, but the provided
	 * generator has order 4096. We must square it a few times.
	 */
	R2 = modp_R2(p, p0i);
	g = modp_montymul(g, R2, p, p0i);
	for (k = logn; k < 11; k ++) {
		g = modp_montymul(g, g, p, p0i);
	}

	ig = modp_div(R2, g, p, p0i, modp_R(p));
	x1 = x2 = modp_R(p);
	for (u = 0; u < n; u ++) {
		size_t v;

		v = modp_bitrev(u, logn);
		gm[v] = x1;
		igm[v] = x2;
		x1 = modp_montymul(x1, g, p, p0i);
		x2 = modp_montymul(x2, ig, p, p0i);
	}
}

/*
 * Compute the NTT over a polynomial. Polynomial elements are a[0],
 * a[stride], a[2 * stride]...
 */
static void
modp_NTT_ext(uint32_t *a, size_t stride, const uint32_t *gm, unsigned logn,
	uint32_t p, uint32_t p0i)
{
	size_t t, m, n;

	if (logn == 0) {
		return;
	}
	n = (size_t)1 << logn;
	t = n;
	for (m = 1; m < n; m <<= 1) {
		size_t ht, u, v1;

		ht = t >> 1;
		for (u = 0, v1 = 0; u < m; u ++, v1 += t) {
			uint32_t s;
			size_t v;
			uint32_t *r1, *r2;

			s = gm[m + u];
			r1 = a + v1 * stride;
			r2 = r1 + ht * stride;
			for (v = 0; v < ht; v ++, r1 += stride, r2 += stride) {
				uint32_t x, y;

				x = *r1;
				y = modp_montymul(*r2, s, p, p0i);
				*r1 = modp_add(x, y, p);
				*r2 = modp_sub(x, y, p);
			}
		}
		t = ht;
	}
}

/*
 * Compute the inverse NTT over a polynomial.
 */
static void
modp_iNTT_ext(uint32_t *a, size_t stride, const uint32_t *igm, unsigned logn,
	uint32_t p, uint32_t p0i)
{
	size_t t, m, n, k;
	uint32_t ni;
	uint32_t *r;

	if (logn == 0) {
		return;
	}
	n = (size_t)1 << logn;
	t = 1;
	for (m = n; m > 1; m >>= 1) {
		size_t hm, dt, u, v1;

		hm = m >> 1;
		dt = t << 1;
		for (u = 0, v1 = 0; u < hm; u ++, v1 += dt) {
			uint32_t s;
			size_t v;
			uint32_t *r1, *r2;

			s = igm[hm + u];
			r1 = a + v1 * stride;
			r2 = r1 + t * stride;
			for (v = 0; v < t; v ++, r1 += stride, r2 += stride) {
				uint32_t x, y;

				x = *r1;
				y = *r2;
				*r1 = modp_add(x, y, p);
				*r2 = modp_montymul(
					modp_sub(x, y, p), s, p, p0i);;
			}
		}
		t = dt;
	}

	/*
	 * We need 1/n in Montgomery representation, i.e. R/n. Since
	 * 1 <= logn <= 11, R/n is an integer; morever, R/n <= 2^30 < p,
	 * thus a simple shift will do.
	 */
	ni = (uint32_t)1 << (31 - logn);
	for (k = 0, r = a; k < n; k ++, r += stride) {
		*r = modp_montymul(*r, ni, p, p0i);
	}
}

/*
 * Simplified macros for NTT and iNTT when the elements are consecutive
 * in RAM.
 */
#define modp_NTT(a, gm, logn, p, p0i)    modp_NTT_ext(a, 1, gm, logn, p, p0i)
#define modp_iNTT(a, igm, logn, p, p0i)  modp_iNTT_ext(a, 1, igm, logn, p, p0i)

/*
 * Given polynomial f in NTT representation modulo p, compute f' of degree
 * less than N/2 such that f' = f0^2 - X*f1^2, where f0 and f1 are
 * polynomials of degree less than N/2 such that f = f0(X^2) + X*f1(X^2).
 *
 * The new polynomial is written "in place" over the first N/2 elements
 * of f.
 *
 * If applied logn times successively on a given polynomial, the resulting
 * degree-0 polynomial is the resultant of f and X^N+1 modulo p.
 */
static void
modp_poly_rec_res(uint32_t *f, unsigned logn,
	uint32_t p, uint32_t p0i, uint32_t R2)
{
	size_t hn, u;

	hn = (size_t)1 << (logn - 1);
	for (u = 0; u < hn; u ++) {
		uint32_t w0, w1;

		w0 = f[(u << 1) + 0];
		w1 = f[(u << 1) + 1];
		f[u] = modp_montymul(modp_montymul(w0, w1, p, p0i), R2, p, p0i);
	}
}

/*
 * Convert a polynomial in one-word normal representation (31 bits per word)
 * into RNS modulo the single prime p.
 */
static void
modp_poly_set(uint32_t *f, unsigned logn, uint32_t p)
{
	size_t n, u;

	n = (size_t)1 << logn;
	for (u = 0; u < n; u ++) {
		uint32_t x;

		x = f[u];
		x |= (x & 0x40000000) << 1;
		f[u] = (uint32_t)modp_set(*(int32_t *)&x, p);
	}
}

/*
 * Convert a polynomial in RNS (modulo a single prime p) into one-word
 * normal representation (31 bits per word, normalized).
 */
static void
modp_poly_norm(uint32_t *f, unsigned logn, uint32_t p)
{
	size_t n, u;

	n = (size_t)1 << logn;
	for (u = 0; u < n; u ++) {
		f[u] = (uint32_t)modp_norm(f[u], p) & 0x7FFFFFFF;
	}
}

/*
 * Convert a one-word 31-bit value into a signed integer.
 */
static inline int32_t
sign_ext(uint32_t x)
{
	x |= ((x & 0x40000000) << 1);
	return *(int32_t *)&x;
}

/* ==================================================================== */
/*
 * Custom bignum implementation.
 *
 * This is a very reduced set of functionalities. We need to do the
 * following operations:
 *
 *  - Rebuild the resultant and the polynomial coefficients from their
 *    values modulo small primes (of length 31 bits each).
 *
 *  - Compute an extended GCD between the two computed resultants.
 *
 *  - Extract top bits and add scaled values during the successive steps
 *    of Babai rounding.
 *
 * When rebuilding values using CRT, we must also recompute the product
 * of the small prime factors. We always do it one small factor at a
 * time, so the "complicated" operations can be done modulo the small
 * prime with the modp_* functions. CRT coefficients (inverses) are
 * precomputed.
 *
 * All values are positive until the last step: when the polynomial
 * coefficients have been rebuilt, we normalize them around 0. But then,
 * only additions and subtractions on the upper few bits are needed
 * afterwards.
 *
 * We keep big integers as arrays of 31-bit words (in uint32_t values);
 * the top bit of each uint32_t is kept equal to 0. Using 31-bit words
 * makes it easier to keep track of carries. When negative values are
 * used, two's complement is used.
 */

/*
 * Subtract integer b from integer a. Both integers are supposed to have
 * the same size. The carry (0 or 1) is returned. Source arrays a and b
 * MUST be distinct.
 *
 * The operation is performed as described above if ctl = 1. If
 * ctl = 0, the value a[] is unmodified, but all memory accesses are
 * still performed, and the carry is computed and returned.
 */
static uint32_t
zint_sub(uint32_t *restrict a, const uint32_t *restrict b, size_t len,
	uint32_t ctl)
{
	size_t u;
	uint32_t cc, m;

	cc = 0;
	m = -ctl;
	for (u = 0; u < len; u ++) {
		uint32_t aw, w;

		aw = a[u];
		w = aw - b[u] - cc;
		cc = w >> 31;
		aw ^= ((w & 0x7FFFFFFF) ^ aw) & m;
		a[u] = aw;
	}
	return cc;
}

/*
 * Mutiply the provided big integer m with a small value x.
 * This function assumes that x < 2^31. The carry word is returned.
 */
static uint32_t
zint_mul_small(uint32_t *m, size_t mlen, uint32_t x)
{
	size_t u;
	uint32_t cc;

	cc = 0;
	for (u = 0; u < mlen; u ++) {
		uint64_t z;

		z = (uint64_t)m[u] * (uint64_t)x + cc;
		m[u] = (uint32_t)z & 0x7FFFFFFF;
		cc = (uint32_t)(z >> 31);
	}
	return cc;
}

/*
 * Reduce a big integer d modulo a small integer p.
 * Rules:
 *  d is unsigned
 *  p is prime
 *  2^30 < p < 2^31
 *  p0i = -(1/p) mod 2^31
 *  R2 = 2^62 mod p
 */
static uint32_t
zint_mod_small_unsigned(const uint32_t *d, size_t dlen,
	uint32_t p, uint32_t p0i, uint32_t R2)
{
	uint32_t x;
	size_t u;

	/*
	 * Algorithm: we inject words one by one, starting with the high
	 * word. Each step is:
	 *  - multiply x by 2^31
	 *  - add new word
	 */
	x = 0;
	u = dlen;
	while (u -- > 0) {
		uint32_t w;

		x = modp_montymul(x, R2, p, p0i);
		w = d[u] - p;
		w += p & -(w >> 31);
		x = modp_add(x, w, p);
	}
	return x;
}

/*
 * Similar to zint_mod_small_unsigned(), except that d may be signed.
 * Extra parameter is Rx = 2^(31*dlen) mod p.
 */
static uint32_t
zint_mod_small_signed(const uint32_t *d, size_t dlen,
	uint32_t p, uint32_t p0i, uint32_t R2, uint32_t Rx)
{
	uint32_t z;

	if (dlen == 0) {
		return 0;
	}
	z = zint_mod_small_unsigned(d, dlen, p, p0i, R2);
	z = modp_sub(z, Rx & -(d[dlen - 1] >> 30), p);
	return z;
}

/*
 * Add y*s to x. x and y initially have length 'len' words; the new x
 * has length 'len+1' words. 's' must fit on 31 bits. x[] and y[] must
 * not overlap.
 */
static void
zint_add_mul_small(uint32_t *restrict x,
	const uint32_t *restrict y, size_t len, uint32_t s)
{
	size_t u;
	uint32_t cc;

	cc = 0;
	for (u = 0; u < len; u ++) {
		uint32_t xw, yw;
		uint64_t z;

		xw = x[u];
		yw = y[u];
		z = (uint64_t)yw * (uint64_t)s + (uint64_t)xw + (uint64_t)cc;
		x[u] = (uint32_t)z & 0x7FFFFFFF;
		cc = (uint32_t)(z >> 31);
	}
	x[len] = cc;
}

/*
 * Normalize a modular integer around 0: if x > p/2, then x is replaced
 * with x - p (signed encoding with two's complement); otherwise, x is
 * untouched. The two integers x and p are encoded over the same length.
 */
static void
zint_norm_zero(uint32_t *restrict x, const uint32_t *restrict p, size_t len)
{
	size_t u;
	uint32_t r, bb;

	/*
	 * Compare x with p/2. We use the shifted version of p, and p
	 * is odd, so we really compare with (p-1)/2; we want to perform
	 * the subtraction if and only if x > (p-1)/2.
	 */
	r = 0;
	bb = 0;
	u = len;
	while (u -- > 0) {
		uint32_t wx, wp, cc;

		/*
		 * Get the two words to compare in wx and wp (both over
		 * 31 bits exactly).
		 */
		wx = x[u];
		wp = (p[u] >> 1) | (bb << 30);
		bb = p[u] & 1;

		/*
		 * We set cc to -1, 0 or 1, depending on whether wp is
		 * lower than, equal to, or greater than wx.
		 */
		cc = wp - wx;
		cc = ((-cc) >> 31) | -(cc >> 31);

		/*
		 * If r != 0 then it is either 1 or -1, and we keep its
		 * value. Otherwise, if r = 0, then we replace it with cc.
		 */
		r |= cc & ((r & 1) - 1);
	}

	/*
	 * At this point, r = -1, 0 or 1, depending on whether (p-1)/2
	 * is lower than, equal to, or greater than x. We thus want to
	 * do the subtraction only if r = -1.
	 */
	zint_sub(x, p, len, r >> 31);
}

/*
 * Rebuild integers from their RNS representation. There are 'num'
 * integers, and each consists in 'xlen' words. 'xx' points at that
 * first word of the first integer; subsequent integers are accessed
 * by adding 'xstride' repeatedly.
 *
 * The words of an integer are the RNS representation of that integer,
 * using the provided 'primes' as moduli. This function replaces
 * each integer with its multi-word value (little-endian order).
 *
 * If "normalize_signed" is non-zero, then the returned value is
 * normalized to the -m/2..m/2 interval (where m is the product of all
 * small prime moduli); two's complement is used for negative values.
 */
static void
zint_rebuild_CRT(uint32_t *restrict xx, size_t xlen, size_t xstride,
	size_t num, const small_prime *primes, int normalize_signed,
	uint32_t *restrict tmp)
{
	size_t u;
	uint32_t *x;

	tmp[0] = primes[0].p;
	for (u = 1; u < xlen; u ++) {
		/*
		 * At the entry of each loop iteration:
		 *  - the first u words of each array have been
		 *    reassembled;
		 *  - the first u words of tmp[] contains the
		 * product of the prime moduli processed so far.
		 *
		 * We call 'q' the product of all previous primes.
		 */
		uint32_t p, p0i, s, R2;
		size_t v;

		p = primes[u].p;
		s = primes[u].s;
		p0i = modp_ninv31(p);
		R2 = modp_R2(p, p0i);

		for (v = 0, x = xx; v < num; v ++, x += xstride) {
			uint32_t xp, xq, xr;
			/*
			 * xp = the integer x modulo the prime p for this
			 *      iteration
			 * xq = (x mod q) mod p
			 */
			xp = x[u];
			xq = zint_mod_small_unsigned(x, u, p, p0i, R2);

			/*
			 * New value is (x mod q) + q * (s * (xp - xq) mod p)
			 */
			xr = modp_montymul(s, modp_sub(xp, xq, p), p, p0i);
			zint_add_mul_small(x, tmp, u, xr);
		}

		/*
		 * Update product of primes in tmp[].
		 */
		tmp[u] = zint_mul_small(tmp, u, p);
	}

	/*
	 * Normalize the reconstructed values around 0.
	 */
	if (normalize_signed) {
		for (u = 0, x = xx; u < num; u ++, x += xstride) {
			zint_norm_zero(x, tmp, xlen);
		}
	}
}

/*
 * Negate a big integer conditionally: value a is replaced with -a if
 * and only if ctl = 1. Control value ctl must be 0 or 1.
 */
static void
zint_negate(uint32_t *a, size_t len, uint32_t ctl)
{
	size_t u;
	uint32_t cc, m;

	/*
	 * If ctl = 1 then we flip the bits of a by XORing with
	 * 0x7FFFFFFF, and we add 1 to the value. If ctl = 0 then we XOR
	 * with 0 and add 0, which leaves the value unchanged.
	 */
	cc = ctl;
	m = -ctl >> 1;
	for (u = 0; u < len; u ++) {
		uint32_t aw;

		aw = a[u];
		aw = (aw ^ m) + cc;
		a[u] = aw & 0x7FFFFFFF;
		cc = aw >> 31;
	}
}

/*
 * Replace a with (a*xa+b*xb)/(2^31) and b with (a*ya+b*yb)/(2^31).
 * The low bits are dropped (the caller should compute the coefficients
 * such that these dropped bits are all zeros). If either or both
 * yields a negative value, then the value is negated.
 *
 * Returned value is:
 *  0  both values were positive
 *  1  new a had to be negated
 *  2  new b had to be negated
 *  3  both new a and new b had to be negated
 *
 * Coefficients xa, xb, ya and yb may use the full signed 32-bit range.
 */
static uint32_t
zint_co_reduce(uint32_t *a, uint32_t *b, size_t len,
	int64_t xa, int64_t xb, int64_t ya, int64_t yb)
{
	size_t u;
	int64_t cca, ccb;
	uint32_t nega, negb;

	cca = 0;
	ccb = 0;
	for (u = 0; u < len; u ++) {
		uint32_t wa, wb;
		uint64_t za, zb;

		wa = a[u];
		wb = b[u];
		za = wa * (uint64_t)xa + wb * (uint64_t)xb + (uint64_t)cca;
		zb = wa * (uint64_t)ya + wb * (uint64_t)yb + (uint64_t)ccb;
		if (u > 0) {
			a[u - 1] = (uint32_t)za & 0x7FFFFFFF;
			b[u - 1] = (uint32_t)zb & 0x7FFFFFFF;
		}
		cca = *(int64_t *)&za >> 31;
		ccb = *(int64_t *)&zb >> 31;
	}
	a[len - 1] = (uint32_t)cca;
	b[len - 1] = (uint32_t)ccb;

	nega = (uint32_t)((uint64_t)cca >> 63);
	negb = (uint32_t)((uint64_t)ccb >> 63);
	zint_negate(a, len, nega);
	zint_negate(b, len, negb);
	return nega | (negb << 1);
}

/*
 * Finish modular reduction. Rules on input parameters:
 *
 *   if neg = 1, then -m <= a < 0
 *   if neg = 0, then 0 <= a < 2*m
 *
 * If neg = 0, then the top word of a[] is allowed to use 32 bits.
 *
 * Modulus m must be odd.
 */
static void
zint_finish_mod(uint32_t *a, size_t len, const uint32_t *m, uint32_t neg)
{
	size_t u;
	uint32_t cc, xm, ym;

	/*
	 * First pass: compare a (assumed nonnegative) with m. Note that
	 * if the top word uses 32 bits, subtracting m must yield a
	 * value less than 2^31 since a < 2*m.
	 */
	cc = 0;
	for (u = 0; u < len; u ++) {
		cc = (a[u] - m[u] - cc) >> 31;
	}

	/*
	 * If neg = 1 then we must add m (regardless of cc)
	 * If neg = 0 and cc = 0 then we must subtract m
	 * If neg = 0 and cc = 1 then we must do nothing
	 *
	 * In the loop below, we conditionally subtract either m or -m
	 * from a. Word xm is a word of m (if neg = 0) or -m (if neg = 1);
	 * but if neg = 0 and cc = 1, then ym = 0 and it forces mw to 0.
	 */
	xm = -neg >> 1;
	ym = -(neg | (1 - cc));
	cc = neg;
	for (u = 0; u < len; u ++) {
		uint32_t aw, mw;

		aw = a[u];
		mw = (m[u] ^ xm) & ym;
		aw = aw - mw - cc;
		a[u] = aw & 0x7FFFFFFF;
		cc = aw >> 31;
	}
}

/*
 * Replace a with (a*xa+b*xb)/(2^31) mod m, and b with
 * (a*ya+b*yb)/(2^31) mod m. Modulus m must be odd; m0i = -1/m[0] mod 2^31.
 */
static void
zint_co_reduce_mod(uint32_t *a, uint32_t *b, const uint32_t *m, size_t len,
	uint32_t m0i, int64_t xa, int64_t xb, int64_t ya, int64_t yb)
{
	size_t u;
	int64_t cca, ccb;
	uint32_t fa, fb;

	/*
	 * These are actually four combined Montgomery multiplications.
	 */
	cca = 0;
	ccb = 0;
	fa = ((a[0] * (uint32_t)xa + b[0] * (uint32_t)xb) * m0i) & 0x7FFFFFFF;
	fb = ((a[0] * (uint32_t)ya + b[0] * (uint32_t)yb) * m0i) & 0x7FFFFFFF;
	for (u = 0; u < len; u ++) {
		uint32_t wa, wb;
		uint64_t za, zb;

		wa = a[u];
		wb = b[u];
		za = wa * (uint64_t)xa + wb * (uint64_t)xb
			+ m[u] * (uint64_t)fa + (uint64_t)cca;
		zb = wa * (uint64_t)ya + wb * (uint64_t)yb
			+ m[u] * (uint64_t)fb + (uint64_t)ccb;
		if (u > 0) {
			a[u - 1] = (uint32_t)za & 0x7FFFFFFF;
			b[u - 1] = (uint32_t)zb & 0x7FFFFFFF;
		}
		cca = *(int64_t *)&za >> 31;
		ccb = *(int64_t *)&zb >> 31;
	}
	a[len - 1] = (uint32_t)cca;
	b[len - 1] = (uint32_t)ccb;

	/*
	 * At this point:
	 *   -m <= a < 2*m
	 *   -m <= b < 2*m
	 * (this is a case of Montgomery reduction)
	 * The top words of 'a' and 'b' may have a 32-th bit set.
	 * We want to add or subtract the modulus, as required.
	 */
	zint_finish_mod(a, len, m, (uint32_t)((uint64_t)cca >> 63));
	zint_finish_mod(b, len, m, (uint32_t)((uint64_t)ccb >> 63));
}

/*
 * Compute a GCD between two positive big integers x and y. The two
 * integers must be odd. Returned value is 1 if the GCD is 1, 0
 * otherwise. When 1 is returned, arrays u and v are filled with values
 * such that:
 *   0 <= u <= y
 *   0 <= v <= x
 *   x*u - y*v = 1
 * x[] and y[] are unmodified. Both input values must have the same
 * encoded length. Temporary array must be large enough to accommodate 4
 * extra values of that length. Arrays u, v and tmp may not overlap with
 * each other, or with either x or y.
 */
static int
zint_bezout(uint32_t *restrict u, uint32_t *restrict v,
	const uint32_t *restrict x, const uint32_t *restrict y,
	size_t len, uint32_t *restrict tmp)
{
	/*
	 * Algorithm is an extended binary GCD. We maintain 6 values
	 * a, b, u0, u1, v0 and v1 with the following invariants:
	 *
	 *  a = x*u0 - y*v0
	 *  b = x*u1 - y*v1
	 *  0 <= a <= x
	 *  0 <= b <= y
	 *  0 <= u0 < y
	 *  0 <= v0 < x
	 *  0 <= u1 <= y
	 *  0 <= v1 < x
	 *
	 * Initial values are:
	 *
	 *  a = x   u0 = 1   v0 = 0
	 *  b = y   u1 = y   v1 = x-1
	 *
	 * Each iteration reduces either a or b, and maintains the
	 * invariants. Algorithm stops when a = b, at which point their
	 * common value is GCD(a,b) and (u0,v0) (or (u1,v1)) contains
	 * the values (u,v) we want to return.
	 *
	 * The formal definition of the algorithm is a sequence of steps:
	 *
	 *  - If a is even, then:
	 *        a <- a/2
	 *        u0 <- u0/2 mod y
	 *        v0 <- v0/2 mod x
	 *
	 *  - Otherwise, if b is even, then:
	 *        b <- b/2
	 *        u1 <- u1/2 mod y
	 *        v1 <- v1/2 mod x
	 *
	 *  - Otherwise, if a > b, then:
	 *        a <- (a-b)/2
	 *        u0 <- (u0-u1)/2 mod y
	 *        v0 <- (v0-v1)/2 mod x
	 *
	 *  - Otherwise:
	 *        b <- (b-a)/2
	 *        u1 <- (u1-u0)/2 mod y
	 *        v1 <- (v1-v0)/2 mod y
	 *
	 * We can show that the operations above preserve the invariants:
	 *
	 *  - If a is even, then u0 and v0 are either both even or both
	 *    odd (since a = x*u0 - y*v0, and x and y are both odd).
	 *    If u0 and v0 are both even, then (u0,v0) <- (u0/2,v0/2).
	 *    Otherwise, (u0,v0) <- ((u0+y)/2,(v0+x)/2). Either way,
	 *    the a = x*u0 - y*v0 invariant is preserved.
	 *
	 *  - The same holds for the case where b is even.
	 *
	 *  - If a and b are odd, and a > b, then:
	 *
	 *      a-b = x*(u0-u1) - y*(v0-v1)
	 *
	 *    In that situation, if u0 < u1, then x*(u0-u1) < 0, but
	 *    a-b > 0; therefore, it must be that v0 < v1, and the
	 *    first part of the update is: (u0,v0) <- (u0-u1+y,v0-v1+x),
	 *    which preserves the invariants. Otherwise, if u0 > u1,
	 *    then u0-u1 >= 1, thus x*(u0-u1) >= x. But a <= x and
	 *    b >= 0, hence a-b <= x. It follows that, in that case,
	 *    v0-v1 >= 0. The first part of the update is then:
	 *    (u0,v0) <- (u0-u1,v0-v1), which again preserves the
	 *    invariants.
	 *
	 *    Either way, once the subtraction is done, the new value of
	 *    a, which is the difference of two odd values, is even,
	 *    and the remaining of this step is a subcase of the
	 *    first algorithm case (i.e. when a is even).
	 *
	 *  - If a and b are odd, and b > a, then the a similar
	 *    argument holds.
	 *
	 * The values a and b start at x and y, respectively. Since x
	 * and y are odd, their GCD is odd, and it is easily seen that
	 * all steps conserve the GCD (GCD(a-b,b) = GCD(a, b);
	 * GCD(a/2,b) = GCD(a,b) if GCD(a,b) is odd). Moreover, either a
	 * or b is reduced by at least one bit at each iteration, so
	 * the algorithm necessarily converges on the case a = b, at
	 * which point the common value is the GCD.
	 *
	 * In the algorithm expressed above, when a = b, the fourth case
	 * applies, and sets b = 0. Since a contains the GCD of x and y,
	 * which are both odd, a must be odd, and subsequent iterations
	 * (if any) will simply divide b by 2 repeatedly, which has no
	 * consequence. Thus, the algorithm can run for more iterations
	 * than necessary; the final GCD will be in a, and the (u,v)
	 * coefficients will be (u0,v0).
	 *
	 *
	 * The algorithm cannot properly handle cases where x = 1 or
	 * y = 1. We report an error in those case; this is handled
	 * at the end.
	 *
	 *
	 * The presentation above is bit-by-bit. It can be sped up by
	 * noticing that all decisions are taken based on the low bits
	 * and high bits of a and b. We can extract the two top words
	 * and low word of each of a and b, and compute reduction
	 * parameters pa, pb, qa and qb such that the new values for
	 * a and b are:
	 *    a' = (a*pa + b*pb) / (2^31)
	 *    b' = (a*qa + b*qb) / (2^31)
	 * the two divisions being exact. The coefficients are obtained
	 * just from the extracted words, and may be slightly off, requiring
	 * an optional correction: if a' < 0, then we replace pa with -pa
	 * and pb with -pb. Each such step will reduce the total length
	 * (sum of lengths of a and b) by at least 30 bits at each
	 * iteration.
	 */
	uint32_t *u0, *u1, *v0, *v1, *a, *b;
	uint32_t x0i, y0i;
	uint32_t num, rc, rcx, rcy;
	size_t j;

	if (len == 0) {
		return 0;
	}

	/*
	 * u0 and v0 are the u and v result buffers; the four other
	 * values (u1, v1, a and b) are taken from tmp[].
	 */
	u0 = u;
	v0 = v;
	u1 = tmp;
	v1 = u1 + len;
	a = v1 + len;
	b = a + len;

	/*
	 * We'll need the Montgomery reduction coefficients.
	 */
	x0i = modp_ninv31(x[0]);
	y0i = modp_ninv31(y[0]);

	/*
	 * Initialize a, b, u0, u1, v0 and v1.
	 *  a = x   u0 = 1   v0 = 0
	 *  b = y   u1 = y   v1 = x-1
	 * Note that x is odd, so computing x-1 is easy.
	 */
	memcpy(a, x, len * sizeof *x);
	memcpy(b, y, len * sizeof *y);
	u0[0] = 1;
	memset(u0 + 1, 0, (len - 1) * sizeof *u0);
	memset(v0, 0, len * sizeof *v0);
	memcpy(u1, y, len * sizeof *u1);
	memcpy(v1, x, len * sizeof *v1);
	v1[0] --;

	/*
	 * Each input operand may be as large as 31*len bits, and we
	 * reduce the total length by at least 30 bits at each iteration.
	 */
	for (num = 62 * (uint32_t)len + 30; num >= 30; num -= 30) {
		uint32_t c0, c1;
		uint32_t a0, a1, b0, b1;
		uint64_t a_hi, b_hi;
		uint32_t a_lo, b_lo;
		int64_t pa, pb, qa, qb;
		int i;
		uint32_t r;

		/*
		 * Extract the top words of a and b. If j is the highest
		 * index >= 1 such that a[j] != 0 or b[j] != 0, then we
		 * want (a[j] << 31) + a[j-1] and (b[j] << 31) + b[j-1].
		 * If a and b are down to one word each, then we use
		 * a[0] and b[0].
		 */
		c0 = (uint32_t)-1;
		c1 = (uint32_t)-1;
		a0 = 0;
		a1 = 0;
		b0 = 0;
		b1 = 0;
		j = len;
		while (j -- > 0) {
			uint32_t aw, bw;

			aw = a[j];
			bw = b[j];
			a0 ^= (a0 ^ aw) & c0;
			a1 ^= (a1 ^ aw) & c1;
			b0 ^= (b0 ^ bw) & c0;
			b1 ^= (b1 ^ bw) & c1;
			c1 = c0;
			c0 &= (((aw | bw) + 0x7FFFFFFF) >> 31) - (uint32_t)1;
		}

		/*
		 * If c1 = 0, then we grabbed two words for a and b.
		 * If c1 != 0 but c0 = 0, then we grabbed one word. It
		 * is not possible that c1 != 0 and c0 != 0, because that
		 * would mean that both integers are zero.
		 */
		a1 |= a0 & c1;
		a0 &= ~c1;
		b1 |= b0 & c1;
		b0 &= ~c1;
		a_hi = ((uint64_t)a0 << 31) + a1;
		b_hi = ((uint64_t)b0 << 31) + b1;
		a_lo = a[0];
		b_lo = b[0];

		/*
		 * Compute reduction factors:
		 *
		 *   a' = a*pa + b*pb
		 *   b' = a*qa + b*qb
		 *
		 * such that a' and b' are both multiple of 2^31, but are
		 * only marginally larger than a and b.
		 */
		pa = 1;
		pb = 0;
		qa = 0;
		qb = 1;
		for (i = 0; i < 31; i ++) {
			/*
			 * At each iteration:
			 *
			 *   a <- (a-b)/2 if: a is odd, b is odd, a_hi > b_hi
			 *   b <- (b-a)/2 if: a is odd, b is odd, a_hi <= b_hi
			 *   a <- a/2 if: a is even
			 *   b <- b/2 if: a is odd, b is even
			 *
			 * We multiply a_lo and b_lo by 2 at each
			 * iteration, thus a division by 2 really is a
			 * non-multiplication by 2.
			 */
			uint32_t rt, oa, ob, cAB, cBA, cA;
			uint64_t rz;

			/*
			 * rt = 1 if a_hi > b_hi, 0 otherwise.
			 */
			rz = b_hi - a_hi;
			rt = (uint32_t)((rz ^ ((a_hi ^ b_hi)
				& (a_hi ^ rz))) >> 63);

			/*
			 * cAB = 1 if b must be subtracted from a
			 * cBA = 1 if a must be subtracted from b
			 * cA = 1 if a must be divided by 2
			 *
			 * Rules:
			 *
			 *   cAB and cBA cannot both be 1.
			 *   If a is not divided by 2, b is.
			 */
			oa = (a_lo >> i) & 1;
			ob = (b_lo >> i) & 1;
			cAB = oa & ob & rt;
			cBA = oa & ob & ~rt;
			cA = cAB | (oa ^ 1);

			/*
			 * Conditional subtractions.
			 */
			a_lo -= b_lo & -cAB;
			a_hi -= b_hi & -(uint64_t)cAB;
			pa -= qa & -(int64_t)cAB;
			pb -= qb & -(int64_t)cAB;
			b_lo -= a_lo & -cBA;
			b_hi -= a_hi & -(uint64_t)cBA;
			qa -= pa & -(int64_t)cBA;
			qb -= pb & -(int64_t)cBA;

			/*
			 * Shifting.
			 */
			a_lo += a_lo & (cA - 1);
			pa += pa & ((int64_t)cA - 1);
			pb += pb & ((int64_t)cA - 1);
			a_hi ^= (a_hi ^ (a_hi >> 1)) & -(uint64_t)cA;
			b_lo += b_lo & -cA;
			qa += qa & -(int64_t)cA;
			qb += qb & -(int64_t)cA;
			b_hi ^= (b_hi ^ (b_hi >> 1)) & ((uint64_t)cA - 1);
		}

		/*
		 * Apply the computed parameters to our values. We
		 * may have to correct pa and pb depending on the
		 * returned value of zint_co_reduce() (when a and/or b
		 * had to be negated).
		 */
		r = zint_co_reduce(a, b, len, pa, pb, qa, qb);
		pa -= (pa + pa) & -(int64_t)(r & 1);
		pb -= (pb + pb) & -(int64_t)(r & 1);
		qa -= (qa + qa) & -(int64_t)(r >> 1);
		qb -= (qb + qb) & -(int64_t)(r >> 1);
		zint_co_reduce_mod(u0, u1, y, len, y0i, pa, pb, qa, qb);
		zint_co_reduce_mod(v0, v1, x, len, x0i, pa, pb, qa, qb);
	}

	/*
	 * At that point, array a[] should contain the GCD, and the
	 * results (u,v) should already be set. We check that the GCD
	 * is indeed 1. We also check that the two operands x and y
	 * are odd, and that neither is equal to 1.
	 */

	/* rc == 0 if and only if a == 1 */
	rc = a[0] ^ 1;
	for (j = 1; j < len; j ++) {
		rc |= a[j];
	}

	/* rcx == 0 if and only if x == 1 */
	/* rcy == 0 if and only if y == 1 */
	rcx = x[0] ^ 1;
	rcy = y[0] ^ 1;
	for (j = 1; j < len; j ++) {
		rcx |= x[j];
		rcy |= y[j];
	}

	/*
	 * Result is correct if all of the following hold:
	 *    rc == 0            (GCD is 1)
	 *    rcx != 0           (x != 1)
	 *    rcy != 0           (y != 1)
	 *    (x[0] & 1) == 1    (x is odd)
	 *    (y[0] & 1) == 1    (y is odd)
	 */
	return (1 - ((uint32_t)(rc | -rc) >> 31))
		& ((uint32_t)(rcx | -rcx) >> 31)
		& ((uint32_t)(rcy | -rcy) >> 31)
		& (x[0] & 1) & (y[0] & 1);
}

/*
 * Add k*(2^sc)*y to x. The result is assumed to fit in the array of
 * size xlen (truncation is applied if necessary).
 * Scale factor sc is provided as sch and scl, such that:
 *    sch = sc / 31
 *    scl = sc % 31  (in the 0..30 range)
 * xlen MUST NOT be lower than ylen; however, it is allowed that
 * xlen is greater than ylen.
 *
 * x[] and y[] are both signed integers, using two's complement for
 * negative values.
 */
static void
zint_add_scaled_mul_small(uint32_t *restrict x, size_t xlen,
	const uint32_t *restrict y, size_t ylen, int32_t k,
	uint32_t sch, uint32_t scl)
{
	size_t u;
	uint32_t ysign, tw;
	int32_t cc;

	if (ylen == 0) {
		return;
	}

	ysign = -(y[ylen - 1] >> 30) >> 1;
	tw = 0;
	cc = 0;
	for (u = sch; u < xlen; u ++) {
		size_t v;
		uint32_t wy, wys, ccu;
		uint64_t z;

		/*
		 * Get the next word of (2^sc)*y.
		 */
		v = u - sch;
		wy = v < ylen ? y[v] : ysign;
		wys = ((wy << scl) & 0x7FFFFFFF) | tw;
		tw = wy >> (31 - scl);

		/*
		 * The expression below does not overflow.
		 */
		z = (uint64_t)((int64_t)wys * (int64_t)k + (int64_t)x[u] + cc);
		x[u] = (uint32_t)z & 0x7FFFFFFF;

		/*
		 * New carry word is a _signed_ right-shift of z.
		 */
		ccu = (uint32_t)(z >> 31);
		cc = *(int32_t *)&ccu;
	}
}

/*
 * Subtract y*2^sc from x. The result is assumed to fit in the array of
 * size xlen (truncation is applied if necessary).
 * Scale factor 'sc' is provided as sch and scl, such that:
 *   sch = sc / 31
 *   scl = sc % 31
 * xlen MUST NOT be lower than ylen.
 *
 * x[] and y[] are both signed integers, using two's complement for
 * negative values.
 */
static void
zint_sub_scaled(uint32_t *restrict x, size_t xlen,
	const uint32_t *restrict y, size_t ylen, uint32_t sch, uint32_t scl)
{
	size_t u;
	uint32_t ysign, tw;
	uint32_t cc;

	if (ylen == 0) {
		return;
	}

	ysign = -(y[ylen - 1] >> 30) >> 1;
	tw = 0;
	cc = 0;
	for (u = sch; u < xlen; u ++) {
		size_t v;
		uint32_t w, wy, wys;

		/*
		 * Get the next word of y (scaled).
		 */
		v = u - sch;
		wy = v < ylen ? y[v] : ysign;
		wys = ((wy << scl) & 0x7FFFFFFF) | tw;
		tw = wy >> (31 - scl);

		w = x[u] - wys - cc;
		x[u] = w & 0x7FFFFFFF;
		cc = w >> 31;
	}
}

static void
stage3_diag_poly_to_modp(uint32_t *d, const uint32_t *src,
	size_t len, size_t stride, size_t n,
	uint32_t p, uint32_t p0i, uint32_t R2)
{
	size_t u;
	uint32_t Rx;

	memset(d, 0, KG_STAGE3_DIAG_MAXN * sizeof *d);
	if (n > KG_STAGE3_DIAG_MAXN) {
		return;
	}
	Rx = modp_Rx((unsigned)len, p, p0i, R2);
	for (u = 0; u < n; u ++) {
		d[u] = zint_mod_small_signed(src + u * stride,
			len, p, p0i, R2, Rx);
	}
}

static void
stage3_diag_negacyclic_mul(uint32_t *d,
	const uint32_t *a, const uint32_t *b, size_t n, uint32_t p)
{
	size_t u, v;

	memset(d, 0, KG_STAGE3_DIAG_MAXN * sizeof *d);
	if (n > KG_STAGE3_DIAG_MAXN) {
		return;
	}
	for (u = 0; u < n; u ++) {
		for (v = 0; v < n; v ++) {
			uint32_t z;
			size_t w;

			z = (uint32_t)(((uint64_t)a[u] * (uint64_t)b[v]) % p);
			w = u + v;
			if (w < n) {
				d[w] = modp_add(d[w], z, p);
			} else {
				d[w - n] = modp_sub(d[w - n], z, p);
			}
		}
	}
}

static void
stage3_diag_capture_pre(unsigned logn, uint32_t p,
	const uint32_t *ft, const uint32_t *gt, size_t sstride,
	const uint32_t *Ft, const uint32_t *Gt, size_t llen,
	const int32_t *k, uint32_t iter)
{
	size_t n, u;
	uint32_t p0i, R2;
	ctl_stage3_round_diag *d;

	n = (size_t)1 << logn;
	d = &kg_s3_last_round;
	memset(d, 0, sizeof *d);
	if (n > KG_STAGE3_DIAG_MAXN) {
		return;
	}
	p0i = modp_ninv31(p);
	R2 = modp_R2(p, p0i);
	d->valid = 1;
	d->n = (uint32_t)n;
	d->p = p;
	d->iter = iter;
	stage3_diag_poly_to_modp(d->f, ft, sstride, sstride, n, p, p0i, R2);
	stage3_diag_poly_to_modp(d->g, gt, sstride, sstride, n, p, p0i, R2);
	stage3_diag_poly_to_modp(d->F_pre, Ft, llen, llen, n, p, p0i, R2);
	stage3_diag_poly_to_modp(d->G_pre, Gt, llen, llen, n, p, p0i, R2);
	for (u = 0; u < n; u ++) {
		d->k_raw[u] = k[u];
		d->k_modp[u] = modp_set(k[u], p);
	}
}

static void
stage3_diag_capture_post(unsigned logn, uint32_t p,
	const uint32_t *Ft, const uint32_t *Gt, size_t llen)
{
	size_t n;
	uint32_t p0i, R2;
	ctl_stage3_round_diag *d;

	n = (size_t)1 << logn;
	d = &kg_s3_last_round;
	if (!d->valid || d->n != n || d->p != p || n > KG_STAGE3_DIAG_MAXN) {
		return;
	}
	p0i = modp_ninv31(p);
	R2 = modp_R2(p, p0i);
	stage3_diag_poly_to_modp(d->F_post, Ft, llen, llen, n, p, p0i, R2);
	stage3_diag_poly_to_modp(d->G_post, Gt, llen, llen, n, p, p0i, R2);
	stage3_diag_negacyclic_mul(d->kf, d->k_modp, d->f, n, p);
	stage3_diag_negacyclic_mul(d->kg, d->k_modp, d->g, n, p);
	stage3_diag_negacyclic_mul(d->pre_gF, d->g, d->F_pre, n, p);
	stage3_diag_negacyclic_mul(d->pre_fG, d->f, d->G_pre, n, p);
	stage3_diag_negacyclic_mul(d->post_gF, d->g, d->F_post, n, p);
	stage3_diag_negacyclic_mul(d->post_fG, d->f, d->G_post, n, p);
	stage3_diag_negacyclic_mul(d->delta_gkf, d->g, d->kf, n, p);
	stage3_diag_negacyclic_mul(d->delta_fkg, d->f, d->kg, n, p);
}

static void
intermediate_mulacc_negacyclic(uint32_t *d,
	const uint32_t *a, const uint32_t *b, size_t n, uint32_t p, int subtract)
{
	size_t u, v;

	for (u = 0; u < n; u ++) {
		for (v = 0; v < n; v ++) {
			uint32_t z;
			size_t w;

			z = (uint32_t)(((uint64_t)a[u] * (uint64_t)b[v]) % p);
			w = u + v;
			if (!subtract) {
				if (w < n) {
					d[w] = modp_add(d[w], z, p);
				} else {
					d[w - n] = modp_sub(d[w - n], z, p);
				}
			} else {
				if (w < n) {
					d[w] = modp_sub(d[w], z, p);
				} else {
					d[w - n] = modp_add(d[w - n], z, p);
				}
			}
		}
	}
}

static int
intermediate_check_modp_direct(const uint32_t *ft, const uint32_t *gt,
	size_t slen, size_t sstride,
	const uint32_t *Ft, const uint32_t *Gt, size_t llen, size_t chklen,
	uint32_t q, unsigned logn, uint32_t *tmp, uint32_t *bad_index)
{
	size_t n, u;
	uint32_t *t1, *t2, *t3, *t4;
	uint32_t p, p0i, R2, Rx_s, Rx_fg;
	uint32_t rq;

	n = (size_t)1 << logn;
	t1 = tmp;
	t2 = t1 + n;
	t3 = t2 + n;
	t4 = t3 + n;
	p = PRIMES[0].p;
	p0i = modp_ninv31(p);
	R2 = modp_R2(p, p0i);
	Rx_s = modp_Rx((unsigned)slen, p, p0i, R2);
	Rx_fg = modp_Rx((unsigned)chklen, p, p0i, R2);
	kg_last_modp_p = p;
	kg_last_modp_slen = (uint32_t)slen;
	kg_last_modp_llen = (uint32_t)llen;
	kg_last_modp_chklen = (uint32_t)chklen;
	kg_last_modp_logn = logn;
	kg_last_modp_expected = 0;
	kg_last_modp_actual = 0;
	kg_last_modp_direct_ok = 1;

	for (u = 0; u < n; u ++) {
		t1[u] = zint_mod_small_signed(
			ft + u * sstride, slen, p, p0i, R2, Rx_s);
		t2[u] = zint_mod_small_signed(
			gt + u * sstride, slen, p, p0i, R2, Rx_s);
		t4[u] = zint_mod_small_signed(
			Gt + u * llen, chklen, p, p0i, R2, Rx_fg);
		t3[u] = 0;
	}
	intermediate_mulacc_negacyclic(t3, t1, t4, n, p, 0);
	for (u = 0; u < n; u ++) {
		t4[u] = zint_mod_small_signed(
			Ft + u * llen, chklen, p, p0i, R2, Rx_fg);
	}
	intermediate_mulacc_negacyclic(t3, t2, t4, n, p, 1);

	rq = modp_set((int32_t)q, p);
	for (u = 0; u < n; u ++) {
		uint32_t want;

		want = (u == 0) ? rq : 0;
		if (t3[u] != want) {
			kg_last_modp_expected = want;
			kg_last_modp_actual = t3[u];
			kg_last_modp_direct_ok = 0;
			if (bad_index != NULL) {
				*bad_index = (uint32_t)u;
			}
			return 0;
		}
	}
	if (bad_index != NULL) {
		*bad_index = 0;
	}
	return 1;
}

/*
 * Get the bit length of a 32-bit unsigned integer (constant-time). Returned
 * value is in the 0..32 range.
 */
static uint32_t
bitlength(uint32_t x)
{
	uint32_t k, nz;

	k = 0;
	nz = ((uint32_t)(x - 0x10000) >> 31) - 1;
	k -= (nz << 4);
	x ^= nz & (x ^ (x >> 16));
	nz = ((uint32_t)(x - 0x00100) >> 31) - 1;
	k -= (nz << 3);
	x ^= nz & (x ^ (x >> 8));
	nz = ((uint32_t)(x - 0x00010) >> 31) - 1;
	k -= (nz << 2);
	x ^= nz & (x ^ (x >> 4));
	nz = ((uint32_t)(x - 0x00004) >> 31) - 1;
	k -= (nz << 1);
	x ^= nz & (x ^ (x >> 2));
	nz = ((uint32_t)(x - 0x00002) >> 31) - 1;
	k -= nz;
	x ^= nz & (x ^ (x >> 1));
	k += x;
	return k;
}

/* ==================================================================== */

/*
 * Convert a polynomial to small integers. Source values are supposed
 * to be one-word integers, signed over 31 bits. Returned value is 0
 * if any of the coefficients exceeds the provided limit (in absolute
 * value), or 1 on success.
 *
 * This is not constant-time; this is not a problem here, because on
 * any failure, the NTRU-solving process will be deemed to have failed
 * and the (f,g) polynomials will be discarded.
 */
static int
poly_big_to_small(int8_t *d, const uint32_t *s, int lim, unsigned logn)
{
	size_t n, u;

	n = (size_t)1 << logn;
	for (u = 0; u < n; u ++) {
		int32_t z;

		z = sign_ext(s[u]);
		if (z < -lim || z > lim) {
			return 0;
		}
		d[u] = (int8_t)z;
	}
	return 1;
}

/*
 * Same as poly_big_to_small(), but destination slots are int16_t.
 */
static int
poly_big_to_small_i16(int16_t *d, const uint32_t *s, int lim, unsigned logn)
{
	size_t n, u;

	n = (size_t)1 << logn;
	for (u = 0; u < n; u ++) {
		int32_t z;

		z = sign_ext(s[u]);
		if (z < -lim || z > lim) {
			return 0;
		}
		d[u] = (int16_t)z;
	}
	return 1;
}

/*
 * Gather overflow diagnostics for i16 conversion limit checks.
 */
static void
poly_big_i16_overflow_stats(const uint32_t *s, int lim, unsigned logn,
	uint32_t *max_abs, uint32_t *over_count,
	uint32_t *first_index, int32_t *first_value)
{
	size_t n, u;
	uint32_t ma, oc, fi;
	int32_t fv;
	int have_first;

	n = (size_t)1 << logn;
	ma = 0;
	oc = 0;
	fi = 0;
	fv = 0;
	have_first = 0;
	for (u = 0; u < n; u ++) {
		int32_t z;
		uint32_t az;

		z = sign_ext(s[u]);
		az = (uint32_t)(z < 0 ? -z : z);
		if (az > ma) {
			ma = az;
		}
		if (z < -lim || z > lim) {
			oc ++;
			if (!have_first) {
				have_first = 1;
				fi = (uint32_t)u;
				fv = z;
			}
		}
	}
	if (max_abs != NULL) {
		*max_abs = ma;
	}
	if (over_count != NULL) {
		*over_count = oc;
	}
	if (first_index != NULL) {
		*first_index = fi;
	}
	if (first_value != NULL) {
		*first_value = fv;
	}
}

/*
 * Get the maximum bit length of all coefficients of a polynomial. Each
 * coefficient has size len words, and coefficient start offsets are
 * separated by fstride words.
 *
 * The bit length of a big integer is defined to be the length of the
 * minimal binary representation, using two's complement for negative
 * values, and excluding the sign bit. This definition implies that
 * if x = 2^k, then x has bit length k but -x has bit length k-1. For
 * non powers of two, x and -x have the same bit length.
 *
 * This function is constant-time with regard to coefficient values and
 * the returned bit length.
 */
static uint32_t
poly_max_bitlength(const uint32_t *f, size_t flen, size_t fstride,
	unsigned logn)
{
	size_t u, n;
	uint32_t t, tk, nz;

	if (flen == 0) {
		return 0;
	}

	n = (size_t)1 << logn;
	t = 0;
	tk = 0;
	for (u = 0; u < n; u ++, f += fstride) {
		uint32_t m, c, ck;
		size_t v;

		/* Extend sign bit into a 31-bit mask. */
		m = -(f[flen - 1] >> 30) & (uint32_t)0x7FFFFFFF;

		/* Get top non-zero sign-adjusted word, with index. */
		c = 0;
		ck = 0;
		for (v = 0; v < flen; v ++) {
			uint32_t w;

			w = f[v] ^ m;
			nz = ((w - 1) >> 31) - 1;
			c ^= nz & (c ^ w);
			ck ^= nz & (ck ^ (uint32_t)v);
		}

		/* If ck > tk, or tk == ck but c > t, then (c,ck) must
		   replace (t,tk) as current candidate for top word/index. */
		nz = -(((tk - ck) | (((tk ^ ck) - 1) & (t - c))) >> 31);
		t ^= nz & (t ^ c);
		tk ^= nz & (tk ^ ck);
	}

	/*
	 * Get bit length of the top word (which has been sign-adjusted)
	 * and return the result.
	 */
	return 31 * tk + bitlength(t);
}

/*
 * Compute q = x / 31 and r = x % 31 for an unsigned integer x. This
 * macro is constant-time and works for values x up to 63487 (inclusive).
 */
#define DIVREM31(q, r, x)  { \
		uint32_t divrem31_q, divrem31_x; \
		divrem31_x = (x); \
		divrem31_q = (uint32_t)(divrem31_x * (uint32_t)67651) >> 21; \
		(q) = divrem31_q; \
		(r) = divrem31_x - 31 * divrem31_q; \
	} while (0)

/*
 * Convert a polynomial to fixed-point approximations, with scaling.
 * Source coefficients of f have length flen words; start offsets
 * are separated by fstride words. For each coefficient x, the
 * computed approximation is x/2^sc.
 *
 * This function assumes that |x| < 2^(30+sc). The length of each
 * coefficient must be less than 2^24 words.
 *
 * This function is constant-time with regard to the coefficient values
 * and to the scaling factor.
 */
static void
poly_big_to_fixed(fnr *d, const uint32_t *f, size_t flen, size_t fstride,
	uint32_t sc, unsigned logn)
{
	size_t u, v, n;
	uint32_t sch, scl, z, t0, t1, t2;

	n = (size_t)1 << logn;

	if (flen == 0) {
		memset(d, 0, n * sizeof *d);
		return;
	}

	/*
	 * We split the bit length into sch and scl such that:
	 *   sc = 31*sch + scl
	 * We also want scl in the 1..31 range, not 0..30. It may happen
	 * that sch becomes -1, which will "wrap around" (harmlessly).
	 *
	 * For each coefficient, we need three words, each with a given
	 * left shift (negative for a right shift):
	 *    sch-1   1 - scl
	 *    sch     32 - scl
	 *    sch+1   63 - scl
	 */
	DIVREM31(sch, scl, sc);
	z = (scl - 1) >> 31;
	sch -= z;
	scl |= 31 & -z;

	t0 = (uint32_t)(sch - 1) & 0xFFFFFF;
	t1 = sch & 0xFFFFFF;
	t2 = (uint32_t)(sch + 1) & 0xFFFFFF;

	for (u = 0; u < n; u ++, f += fstride) {
		uint32_t w0, w1, w2, ws, xl, xh;

		w0 = 0;
		w1 = 0;
		w2 = 0;
		for (v = 0; v < flen; v ++) {
			uint32_t t, w;

			w = f[v];
			t = (uint32_t)v & 0xFFFFFF;
			w0 |= w & -((uint32_t)((t ^ t0) - 1) >> 31);
			w1 |= w & -((uint32_t)((t ^ t1) - 1) >> 31);
			w2 |= w & -((uint32_t)((t ^ t2) - 1) >> 31);
		}

		/*
		 * If there were not enough words for the requested
		 * scaling, then we must supply copies with the proper
		 * sign.
		 */
		ws = -(f[flen - 1] >> 30) >> 1;
		w0 |= ws & -((uint32_t)((uint32_t)flen - sch) >> 31);
		w1 |= ws & -((uint32_t)((uint32_t)flen - sch - 1) >> 31);
		w2 |= ws & -((uint32_t)((uint32_t)flen - sch - 2) >> 31);

		/*
		 * Assemble the 64-bit value with the shifts. We assume
		 * that shifts on 32-bit values are constant-time with
		 * regard to the shift count (this should be true on all
		 * modern architectures; the last notable arch on which
		 * shift timing depended on the count was the Pentium IV).
		 *
		 * Since the shift count (scl) is guaranteed to be in 1..31,
		 * we do not have special cases to handle.
		 *
		 * We must sign-extend w2 to ensure the sign bit is properly
		 * set in the fnr value.
		 */
		w2 |= (uint32_t)(w2 & 0x40000000) << 1;
		xl = (w0 >> (scl - 1)) | (w1 << (32 - scl));
		xh = (w1 >> scl) | (w2 << (31 - scl));
		d[u] = fnr_of_scaled32((uint64_t)xl | ((uint64_t)xh << 32));
	}
}

/*
 * Subtract k*f from F, where F, f and k are polynomials modulo X^N+1.
 * Coefficients of polynomial k are small integers (signed values in the
 * -2^31..+2^31 range) scaled by 2^sc.
 *
 * This function implements the basic quadratic multiplication algorithm,
 * which is efficient in space (no extra buffer needed) but slow at
 * high degree.
 */
static void
poly_sub_scaled(uint32_t *restrict F, size_t Flen, size_t Fstride,
	const uint32_t *restrict f, size_t flen, size_t fstride,
	const int32_t *restrict k, uint32_t sc, unsigned logn)
{
	size_t n, u;
	uint32_t sch, scl;

	n = (size_t)1 << logn;
	DIVREM31(sch, scl, sc);
	for (u = 0; u < n; u ++) {
		int32_t kf;
		size_t v;
		uint32_t *x;
		const uint32_t *y;

		kf = -k[u];
		x = F + u * Fstride;
		y = f;
		for (v = 0; v < n; v ++) {
			zint_add_scaled_mul_small(
				x, Flen, y, flen, kf, sch, scl);
			if (u + v == n - 1) {
				x = F;
				kf = -kf;
			} else {
				x += Fstride;
			}
			y += fstride;
		}
	}
}

/*
 * Subtract k*f from F. Coefficients of polynomial k are small integers
 * (signed values in the -2^31..+2^31 range) scaled by 2^sc. This function
 * assumes that the degree is large, and integers relatively small.
 */
static void
poly_sub_scaled_ntt(uint32_t *restrict F, size_t Flen, size_t Fstride,
	const uint32_t *restrict f, size_t flen, size_t fstride,
	const int32_t *restrict k, uint32_t sc, unsigned logn,
	uint32_t *restrict tmp)
{
	uint32_t *gm, *igm, *fk, *t1, *x;
	const uint32_t *y;
	size_t n, u, tlen;
	uint32_t sch, scl;

	n = (size_t)1 << logn;
	tlen = flen + 1;
	gm = tmp;
	igm = gm + n;
	fk = igm + n;
	t1 = fk + n * tlen;
	DIVREM31(sch, scl, sc);

	/*
	 * Compute k*f in fk[], in RNS notation.
	 */
	for (u = 0; u < tlen; u ++) {
		uint32_t p, p0i, R2, Rx;
		size_t v;

		p = PRIMES[u].p;
		p0i = modp_ninv31(p);
		R2 = modp_R2(p, p0i);
		Rx = modp_Rx((unsigned)flen, p, p0i, R2);
		modp_mkgm(gm, igm, logn, PRIMES[u].g, p, p0i);

		for (v = 0; v < n; v ++) {
			t1[v] = modp_set(k[v], p);
		}
		modp_NTT(t1, gm, logn, p, p0i);
		for (v = 0, y = f, x = fk + u;
			v < n; v ++, y += fstride, x += tlen)
		{
			*x = zint_mod_small_signed(y, flen, p, p0i, R2, Rx);
		}
		modp_NTT_ext(fk + u, tlen, gm, logn, p, p0i);
		for (v = 0, x = fk + u; v < n; v ++, x += tlen) {
			*x = modp_montymul(
				modp_montymul(t1[v], *x, p, p0i), R2, p, p0i);
		}
		modp_iNTT_ext(fk + u, tlen, igm, logn, p, p0i);
	}

	/*
	 * Rebuild k*f.
	 */
	zint_rebuild_CRT(fk, tlen, tlen, n, PRIMES, 1, t1);

	/*
	 * Subtract k*f, scaled, from F.
	 */
	for (u = 0, x = F, y = fk; u < n; u ++, x += Fstride, y += tlen) {
		zint_sub_scaled(x, Flen, y, tlen, sch, scl);
	}
}

/* ==================================================================== */

/*
 * The MAX_BL_SMALL_*[] and MAX_BL_LARGE_*[] contain the lengths, in 31-bit
 * words, of intermediate values in the computation:
 *
 *   MAX_BL_SMALL_*[depth]: length for the input f and g at that depth
 *   MAX_BL_LARGE_*[depth]: length for the unreduced F and G at that depth
 *
 * Rules:
 *
 *  - Within an array, values grow.
 *
 *  - The 'SMALL' array must have an entry for maximum depth, corresponding
 *    to the size of values used in the binary GCD. There is no such value
 *    for the 'LARGE' array (the binary GCD yields already reduced
 *    coefficients).
 *
 *  - MAX_BL_LARGE_*[depth] >= MAX_BL_SMALL_*[depth + 1].
 *
 *  - Values must be large enough to handle the common cases, with some
 *    margins.
 *
 *  - Values must not be "too large" either because we will convert some
 *    integers into floating-point values by considering the top 10 words,
 *    i.e. 310 bits; hence, for values of length more than 10 words, we
 *    should take care to have the length centered on the expected size.
 *
 * A further complication is that the lengths depend on the use integer q.
 * Therefore, there are three variants of each array, each for its value
 * of q (257, 769 or 3329).
 *
 * IMPORTANT: if these values are modified, then the temporary buffer
 * sizes (CTL_KEYGEN_TEMP_*, in inner.h) must be recomputed
 * accordingly.
 */

static const size_t MAX_BL_SMALL_257[] = {
	1, 1, 2, 2, 3, 5, 9, 16, 31, 60
};

static const size_t MAX_BL_LARGE_257[] = {
	2, 2, 2, 4, 7, 12, 23, 45, 87
};

static const size_t MAX_BL_SMALL_769[] = {
	1, 1, 2, 2, 3, 6, 11, 20, 38, 73, 142
};

static const size_t MAX_BL_LARGE_769[] = {
	2, 2, 3, 5, 8, 15, 28, 55, 107, 210
};

static const size_t MAX_BL_SMALL_3329[] = {
	1, 1, 2, 2, 3, 6, 13, 24, 46, 88, 173, 430
};

static const size_t MAX_BL_LARGE_3329[] = {
	2, 2, 3, 5, 9, 18, 36, 69, 132, 259, 510
};
/*
 * WORD_WIN[depth] is how many top words we look at (at most) when
 * extracting approximations of coefficients of polynomials; it is
 * indexed by depth. For the deepest recursion level, where Babai's
 * reduction is not used, the value from the previous level is used
 * to compute the bit lengths of the resultants.
 */
static const size_t WORD_WIN[] = {
	1, 1, 2, 2, 2, 3, 4, 6, 8, 12, 16
};

/*
 * Minimal recursion depth at which we rebuild intermediate values
 * when reconstructing f and g.
 */
#define DEPTH_INT_FG   4

/*
 * Assumed size reduction (in bits) at each iteration of Babai's nearest
 * plane.
 */
#define REDUCE_BITS   16

#define PROFILE_DEPTH_NONE   0xFFFFFFFFu

typedef struct {
	uint32_t q;
	unsigned max_logn;
	const size_t *max_bl_small;
	size_t max_bl_small_len;
	const size_t *max_bl_large;
	size_t max_bl_large_len;
	const size_t *word_win;
	size_t word_win_len;
	uint32_t reduce_bits;
	uint32_t force_full_window_from_depth;
	uint32_t tail_keep_reducing_from_depth;
	uint32_t reduce_bits_alt_from_depth;
	uint32_t reduce_bits_alt_value;
} ctl_solve_profile;

static const ctl_solve_profile CTL_SOLVE_PROFILE_257 = {
	257,
	9,
	MAX_BL_SMALL_257, sizeof MAX_BL_SMALL_257 / sizeof MAX_BL_SMALL_257[0],
	MAX_BL_LARGE_257, sizeof MAX_BL_LARGE_257 / sizeof MAX_BL_LARGE_257[0],
	WORD_WIN, sizeof WORD_WIN / sizeof WORD_WIN[0],
	REDUCE_BITS,
	PROFILE_DEPTH_NONE,
	PROFILE_DEPTH_NONE,
	PROFILE_DEPTH_NONE, REDUCE_BITS
};

static const ctl_solve_profile CTL_SOLVE_PROFILE_769 = {
	769,
	10,
	MAX_BL_SMALL_769, sizeof MAX_BL_SMALL_769 / sizeof MAX_BL_SMALL_769[0],
	MAX_BL_LARGE_769, sizeof MAX_BL_LARGE_769 / sizeof MAX_BL_LARGE_769[0],
	WORD_WIN, sizeof WORD_WIN / sizeof WORD_WIN[0],
	REDUCE_BITS,
	PROFILE_DEPTH_NONE,
	PROFILE_DEPTH_NONE,
	PROFILE_DEPTH_NONE, REDUCE_BITS
};

static const ctl_solve_profile CTL_SOLVE_PROFILE_3329 = {
	3329,
	11,
	MAX_BL_SMALL_3329, sizeof MAX_BL_SMALL_3329 / sizeof MAX_BL_SMALL_3329[0],
	MAX_BL_LARGE_3329, sizeof MAX_BL_LARGE_3329 / sizeof MAX_BL_LARGE_3329[0],
	WORD_WIN, sizeof WORD_WIN / sizeof WORD_WIN[0],
	REDUCE_BITS,
	8,
	7,
	7, 2
};

static const ctl_solve_profile *
ctl_get_solve_profile(uint32_t q)
{
	switch (q) {
	case 257:
		return &CTL_SOLVE_PROFILE_257;
	case 769:
		return &CTL_SOLVE_PROFILE_769;
	case 3329:
		return &CTL_SOLVE_PROFILE_3329;
	default:
		return NULL;
	}
}

/*
 * Compute squared norm of a short vector. Maximum possible returned value
 * is 16384*(2^logn), which will fit in a uint32_t as long as logn <= 17
 * (the rest of this implementation assumes that logn <= 11).
 */
static uint32_t
poly_small_sqnorm(const int8_t *f, unsigned logn)
{
	size_t n, u;
	uint32_t s;

	n = (size_t)1 << logn;
	s = 0;
	for (u = 0; u < n; u ++) {
		int32_t z;

		z = f[u];
		s += (uint32_t)(z * z);
	}
	return s;
}

/*
 * Align (upwards) the provided 'data' pointer with regards to 'base'
 * so that the offset is a multiple of the size of 'fnr'.
 */
static fnr *
align_fnr(void *base, void *data)
{
	uint8_t *cb, *cd;
	size_t k, km;

	cb = base;
	cd = data;
	k = (size_t)(cd - cb);
	km = k % sizeof(fnr);
	if (km) {
		k += (sizeof(fnr)) - km;
	}
	return (fnr *)(cb + k);
}

/*
 * Align (upwards) the provided 'data' pointer with regards to 'base'
 * so that the offset is a multiple of the size of 'uint32_t'.
 */
static uint32_t *
align_u32(void *base, void *data)
{
	uint8_t *cb, *cd;
	size_t k, km;

	cb = base;
	cd = data;
	k = (size_t)(cd - cb);
	km = k % sizeof(uint32_t);
	if (km) {
		k += (sizeof(uint32_t)) - km;
	}
	return (uint32_t *)(cb + k);
}

/*
 * Input: f,g of degree N = 2^logn; 'depth' is used only to get their
 * individual length.
 *
 * Output: f',g' of degree N/2, with the length for 'depth+1'.
 *
 * Values are in RNS; input and/or output may also be in NTT.
 *
 * Return value: 1 on success, 0 on error (bad value for q and logn).
 */
static int
make_fg_step(uint32_t *data, uint32_t q, unsigned logn, unsigned depth)
{
	size_t n, hn, u;
	size_t slen, tlen;
	uint32_t *fd, *gd, *fs, *gs, *gm, *igm, *t1;
	const ctl_solve_profile *prof;

	n = (size_t)1 << logn;
	hn = n >> 1;
	prof = ctl_get_solve_profile(q);
	if (prof == NULL || logn > prof->max_logn
		|| (size_t)(depth + 1) >= prof->max_bl_small_len)
	{
		return 0;
	}
	slen = prof->max_bl_small[depth];
	tlen = prof->max_bl_small[depth + 1];
	/*
	 * Prepare room for the result.
	 */
	fd = data;
	gd = fd + hn * tlen;
	fs = gd + hn * tlen;
	gs = fs + n * slen;
	gm = gs + n * slen;
	igm = gm + n;
	t1 = igm + n;
	memmove(fs, data, 2 * n * slen * sizeof *data);

	/*
	 * First slen words: we use the input values directly, and apply
	 * inverse NTT as we go.
	 */
	for (u = 0; u < slen; u ++) {
		uint32_t p, p0i, R2;
		size_t v;
		uint32_t *x, *y;

		p = PRIMES[u].p;
		p0i = modp_ninv31(p);
		R2 = modp_R2(p, p0i);
		modp_mkgm(gm, igm, logn, PRIMES[u].g, p, p0i);

		for (v = 0, x = fs + u, y = fd + u;
			v < hn; v ++, x += 2 * slen, y += tlen)
		{
			*y = modp_montymul(
				modp_montymul(x[0], x[slen], p, p0i),
				R2, p, p0i);
		}
		modp_iNTT_ext(fs + u, slen, igm, logn, p, p0i);

		for (v = 0, x = gs + u, y = gd + u;
			v < hn; v ++, x += 2 * slen, y += tlen)
		{
			*y = modp_montymul(
				modp_montymul(x[0], x[slen], p, p0i),
				R2, p, p0i);
		}
		modp_iNTT_ext(gs + u, slen, igm, logn, p, p0i);
	}

	/*
	 * Since the fs and gs words have been de-NTTized, we can use the
	 * CRT to rebuild the values.
	 */
	zint_rebuild_CRT(fs, slen, slen, n, PRIMES, 1, gm);
	zint_rebuild_CRT(gs, slen, slen, n, PRIMES, 1, gm);

	/*
	 * We make t1 an alias for igm.
	 */
	t1 = igm;

	/*
	 * Remaining words: use modular reductions to extract the values.
	 */
	for (u = slen; u < tlen; u ++) {
		uint32_t p, p0i, R2, Rx;
		size_t v;
		uint32_t *x;

		p = PRIMES[u].p;
		p0i = modp_ninv31(p);
		R2 = modp_R2(p, p0i);
		Rx = modp_Rx((unsigned)slen, p, p0i, R2);

		/*
		 * We won't use iNTT from now on, so we can put igm in
		 * a temporary.
		 */
		modp_mkgm(gm, t1, logn, PRIMES[u].g, p, p0i);
		for (v = 0, x = fs; v < n; v ++, x += slen) {
			t1[v] = zint_mod_small_signed(x, slen, p, p0i, R2, Rx);
		}
		modp_NTT(t1, gm, logn, p, p0i);
		for (v = 0, x = fd + u; v < hn; v ++, x += tlen) {
			uint32_t w0, w1;

			w0 = t1[(v << 1) + 0];
			w1 = t1[(v << 1) + 1];
			*x = modp_montymul(
				modp_montymul(w0, w1, p, p0i), R2, p, p0i);
		}
		for (v = 0, x = gs; v < n; v ++, x += slen) {
			t1[v] = zint_mod_small_signed(x, slen, p, p0i, R2, Rx);
		}
		modp_NTT(t1, gm, logn, p, p0i);
		for (v = 0, x = gd + u; v < hn; v ++, x += tlen) {
			uint32_t w0, w1;

			w0 = t1[(v << 1) + 0];
			w1 = t1[(v << 1) + 1];
			*x = modp_montymul(
				modp_montymul(w0, w1, p, p0i), R2, p, p0i);
		}
	}
	return 1;
}

/*
 * Compute f and g at a specific depth, in RNS+NTT notation.
 *
 * Returned values are stored in the data[] array, at slen words per integer.
 *
 * Conditions:
 *   0 <= depth <= logn
 *
 * Space use in data[]: enough room for any two successive values (f', g',
 * f and g).
 *
 * Return value: 1 on success, 0 on error (bad value for q and logn).
 */
static int
make_fg(uint32_t *data, const int8_t *f, const int8_t *g,
	uint32_t q, unsigned logn, unsigned depth)
{
	size_t n, u;
	uint32_t *ft, *gt, *gm, *igm, p, p0i;
	unsigned d;

	n = (size_t)1 << logn;
	ft = data;
	gt = ft + n;
	p = PRIMES[0].p;
	p0i = modp_ninv31(p);
	for (u = 0; u < n; u ++) {
		ft[u] = modp_set(f[u], p);
		gt[u] = modp_set(g[u], p);
	}
	gm = gt + n;
	igm = gm + n;
	modp_mkgm(gm, igm, logn, PRIMES[0].g, p, p0i);
	modp_NTT(ft, gm, logn, p, p0i);
	modp_NTT(gt, gm, logn, p, p0i);

	for (d = 0; d < depth; d ++) {
		if (!make_fg_step(data, q, logn - d, d)) {
			return 0;
		}
	}

	return 1;
}

/*
 * Solving the NTRU equation, deepest level: compute the resultants of
 * f and g with X^N+1, and use binary GCD. The F and G values are
 * returned in tmp[].
 *
 * Return value: 1 on success, 0 on error (bad value for q and logn).
 */
static int
solve_NTRU_deepest(unsigned logn_top,
	const int8_t *f, const int8_t *g, uint32_t q, uint32_t *tmp)
{
	size_t len;
	uint32_t *Fp, *Gp, *fp, *gp, *t1;
	const ctl_solve_profile *prof;

	prof = ctl_get_solve_profile(q);
	if (prof == NULL || logn_top > prof->max_logn
		|| (size_t)logn_top >= prof->max_bl_small_len)
	{
		kg_diag_stats.solve_fg_fail_intermediate_bad_param ++;
		return 0;
	}
	len = prof->max_bl_small[logn_top];

	Fp = tmp;
	Gp = Fp + len;
	fp = Gp + len;
	gp = fp + len;
	t1 = gp + len;

	/*
	 * make_fg() returns the two deepest values in RNS+NTT. However,
	 * at the deepest level, the NTT is a no-operation, so we can
	 * ignore it.
	 */
	if (!make_fg(tmp, f, g, q, logn_top, logn_top)) {
		kg_diag_stats.solve_fg_fail_deepest_make_fg ++;
		return 0;
	}
	memmove(fp, tmp, 2 * len * sizeof *fp);

	/*
	 * We use the CRT to rebuild the resultants as big integers.
	 * There are two such big integers. The resultants are always
	 * nonnegative.
	 */
	zint_rebuild_CRT(fp, len, len, 2, PRIMES, 0, t1);

	/*
	 * Apply the binary GCD. The zint_bezout() function works only
	 * if both inputs are odd.
	 *
	 * We can test on the result and return 0 because that would
	 * imply failure of the NTRU solving equation, and the (f,g)
	 * values will be abandoned in that case.
	 */
	if (!zint_bezout(Gp, Fp, fp, gp, len, t1)) {
		kg_diag_stats.solve_fg_fail_deepest_bezout ++;
		return 0;
	}

	/*
	 * Multiply the two values by the target value q. Values must
	 * fit in the destination arrays.
	 * We can again test on the returned words: a non-zero output
	 * of zint_mul_small() means that we exceeded our array
	 * capacity, and that implies failure and rejection of (f,g).
	 */
	if (zint_mul_small(Fp, len, q) != 0
		|| zint_mul_small(Gp, len, q) != 0)
	{
		kg_diag_stats.solve_fg_fail_deepest_mul_q ++;
		return 0;
	}

	return 1;
}

/* see inner.h */
int
ctl_debug_deepest_resultants(uint32_t q, unsigned logn,
	const int8_t *f, const int8_t *g, uint32_t *tmp,
	uint32_t *fp_lo, uint32_t *gp_lo, uint32_t *fp_hi, uint32_t *gp_hi)
{
	size_t len;
	uint32_t *Fp, *Gp, *fp, *gp, *t1;
	const ctl_solve_profile *prof;

	prof = ctl_get_solve_profile(q);
	if (prof == NULL || logn > prof->max_logn
		|| (size_t)logn >= prof->max_bl_small_len)
	{
		return 0;
	}
	len = prof->max_bl_small[logn];

	Fp = tmp;
	Gp = Fp + len;
	fp = Gp + len;
	gp = fp + len;
	t1 = gp + len;

	if (!make_fg(tmp, f, g, q, logn, logn)) {
		return 0;
	}
	memmove(fp, tmp, 2 * len * sizeof *fp);
	zint_rebuild_CRT(fp, len, len, 2, PRIMES, 0, t1);

	if (fp_lo != NULL) {
		*fp_lo = fp[0];
	}
	if (gp_lo != NULL) {
		*gp_lo = gp[0];
	}
	if (fp_hi != NULL) {
		*fp_hi = fp[len - 1];
	}
	if (gp_hi != NULL) {
		*gp_hi = gp[len - 1];
	}

	return zint_bezout(Gp, Fp, fp, gp, len, t1);
}

/* see inner.h */
int
ctl_debug_deepest_mulq(uint32_t q, unsigned logn,
	const int8_t *f, const int8_t *g, uint32_t *tmp,
	uint32_t *carry_f, uint32_t *carry_g,
	uint32_t *f_hi_before, uint32_t *g_hi_before,
	uint32_t *f_hi_after, uint32_t *g_hi_after)
{
	size_t len;
	uint32_t *Fp, *Gp, *fp, *gp, *t1;
	uint32_t cf, cg;
	const ctl_solve_profile *prof;

	prof = ctl_get_solve_profile(q);
	if (prof == NULL || logn > prof->max_logn
		|| (size_t)logn >= prof->max_bl_small_len)
	{
		return 0;
	}
	len = prof->max_bl_small[logn];

	Fp = tmp;
	Gp = Fp + len;
	fp = Gp + len;
	gp = fp + len;
	t1 = gp + len;

	if (!make_fg(tmp, f, g, q, logn, logn)) {
		return 0;
	}
	memmove(fp, tmp, 2 * len * sizeof *fp);
	zint_rebuild_CRT(fp, len, len, 2, PRIMES, 0, t1);
	if (!zint_bezout(Gp, Fp, fp, gp, len, t1)) {
		return 0;
	}

	if (f_hi_before != NULL) {
		*f_hi_before = Fp[len - 1];
	}
	if (g_hi_before != NULL) {
		*g_hi_before = Gp[len - 1];
	}

	cf = zint_mul_small(Fp, len, q);
	cg = zint_mul_small(Gp, len, q);

	if (carry_f != NULL) {
		*carry_f = cf;
	}
	if (carry_g != NULL) {
		*carry_g = cg;
	}
	if (f_hi_after != NULL) {
		*f_hi_after = Fp[len - 1];
	}
	if (g_hi_after != NULL) {
		*g_hi_after = Gp[len - 1];
	}
	return 1;
}

/*
 * Check f*G - g*F == q (mod X^n+1, mod p0) using one small prime.
 * Returns 1 on success, 0 on mismatch; first mismatch index is optional.
 */
static int
intermediate_check_modp(const uint32_t *ft, const uint32_t *gt, size_t slen,
	size_t sstride,
	const uint32_t *Ft, const uint32_t *Gt, size_t llen, size_t chklen,
	uint32_t q, unsigned logn, uint32_t *tmp, uint32_t *bad_index)
{
	size_t n, u;
	uint32_t *t1, *t2, *t3, *gm;
	uint32_t p, p0i, R2, Rx_s, Rx_fg, rq;

	n = (size_t)1 << logn;
	t1 = tmp;
	t2 = t1 + n;
	t3 = t2 + n;
	gm = t3 + n;
	p = PRIMES[0].p;
	p0i = modp_ninv31(p);
	modp_mkgm(gm, t3, logn, PRIMES[0].g, p, p0i);
	R2 = modp_R2(p, p0i);
	Rx_s = modp_Rx((unsigned)slen, p, p0i, R2);
	Rx_fg = modp_Rx((unsigned)chklen, p, p0i, R2);
	kg_last_modp_p = p;
	kg_last_modp_slen = (uint32_t)slen;
	kg_last_modp_llen = (uint32_t)llen;
	kg_last_modp_chklen = (uint32_t)chklen;
	kg_last_modp_logn = logn;
	kg_last_modp_direct_ok = 0;

	for (u = 0; u < n; u ++) {
		t1[u] = zint_mod_small_signed(
			ft + u * sstride, slen, p, p0i, R2, Rx_s);
		t2[u] = zint_mod_small_signed(
			Gt + u * llen, chklen, p, p0i, R2, Rx_fg);
	}
	modp_NTT(t1, gm, logn, p, p0i);
	modp_NTT(t2, gm, logn, p, p0i);
	for (u = 0; u < n; u ++) {
		t3[u] = modp_montymul(t1[u], t2[u], p, p0i);
	}

	for (u = 0; u < n; u ++) {
		t1[u] = zint_mod_small_signed(
			gt + u * sstride, slen, p, p0i, R2, Rx_s);
		t2[u] = zint_mod_small_signed(
			Ft + u * llen, chklen, p, p0i, R2, Rx_fg);
	}
	modp_NTT(t1, gm, logn, p, p0i);
	modp_NTT(t2, gm, logn, p, p0i);
	for (u = 0; u < n; u ++) {
		t3[u] = modp_sub(t3[u], modp_montymul(t1[u], t2[u], p, p0i), p);
	}

	rq = modp_montymul(1, q, p, p0i);
	for (u = 0; u < n; u ++) {
		if (t3[u] != rq) {
			size_t v;
			uint32_t c, ceq, vmin, vmax, first;
			uint32_t direct_bad_index;

			first = t3[u];
			c = 1;
			ceq = 1;
			vmin = first;
			vmax = first;
			for (v = u + 1; v < n; v ++) {
				uint32_t w;

				w = t3[v];
				c += (w != rq);
				ceq += (w == first);
				vmin ^= (vmin ^ w) & -((int32_t)((w - vmin) >> 31));
				vmax ^= (vmax ^ w) & -((int32_t)((vmax - w) >> 31));
			}
			kg_modp_crosscheck_count ++;
			if (intermediate_check_modp_direct(ft, gt, slen, sstride,
				Ft, Gt, llen, chklen, q, logn, tmp, &direct_bad_index))
			{
				kg_modp_ntt_false_positive_count ++;
				kg_last_modp_direct_ok = 1;
				if (bad_index != NULL) {
					*bad_index = 0;
				}
				return 1;
			}
			kg_last_modp_expected = rq;
			kg_last_modp_actual = first;
			kg_last_modp_mismatch_count = c;
			kg_last_modp_eq_first_count = ceq;
			kg_last_modp_min = vmin;
			kg_last_modp_max = vmax;
			kg_last_modp_term_gF = modp_montymul(t1[u], t2[u], p, p0i);
			kg_last_modp_term_fG =
				modp_add(first, kg_last_modp_term_gF, p);
			if (bad_index != NULL) {
				*bad_index = (uint32_t)u;
			}
			return 0;
		}
	}
	if (bad_index != NULL) {
		*bad_index = 0;
	}
	return 1;
}

/*
 * Return 1 if highest limb in current FGlen can be dropped for all
 * coefficients of both Ft and Gt (sign-extension limb), else 0.
 */
static int
intermediate_can_shrink_fglen(
	const uint32_t *Ft, const uint32_t *Gt, size_t llen, size_t FGlen, size_t n)
{
	size_t u;
	size_t hi, lo;

	if (FGlen <= 1) {
		return 0;
	}
	hi = FGlen - 1;
	lo = FGlen - 2;
	for (u = 0; u < n; u ++) {
		size_t off;
		uint32_t wf_hi, wf_lo, wg_hi, wg_lo;
		uint32_t sf, sg;

		off = u * llen;
		wf_hi = Ft[off + hi];
		wf_lo = Ft[off + lo];
		wg_hi = Gt[off + hi];
		wg_lo = Gt[off + lo];
		sf = (wf_lo >> 30) ? 0x7FFFFFFFu : 0u;
		sg = (wg_lo >> 30) ? 0x7FFFFFFFu : 0u;
		if (wf_hi != sf || wg_hi != sg) {
			return 0;
		}
	}
	return 1;
}

static size_t
intermediate_compact_len_pair(
	const uint32_t *Ft, const uint32_t *Gt, size_t stride, size_t len, size_t n)
{
	while (len > 1) {
		size_t u;
		size_t hi, lo;
		int ok;

		hi = len - 1;
		lo = len - 2;
		ok = 1;
		for (u = 0; u < n; u ++) {
			size_t off;
			uint32_t wf_hi, wf_lo, wg_hi, wg_lo;
			uint32_t sf, sg;

			off = u * stride;
			wf_hi = Ft[off + hi];
			wf_lo = Ft[off + lo];
			wg_hi = Gt[off + hi];
			wg_lo = Gt[off + lo];
			sf = (wf_lo >> 30) ? 0x7FFFFFFFu : 0u;
			sg = (wg_lo >> 30) ? 0x7FFFFFFFu : 0u;
			if (wf_hi != sf || wg_hi != sg) {
				ok = 0;
				break;
			}
		}
		if (!ok) {
			break;
		}
		len --;
	}
	return len;
}

/*
 * Count how many coefficient slots block sign-extension shrinking.
 */
static uint32_t
intermediate_count_shrink_blockers(
	const uint32_t *Ft, const uint32_t *Gt, size_t llen, size_t FGlen, size_t n)
{
	size_t u;
	size_t hi, lo;
	uint32_t bad;

	if (FGlen <= 1) {
		return 0;
	}
	hi = FGlen - 1;
	lo = FGlen - 2;
	bad = 0;
	for (u = 0; u < n; u ++) {
		size_t off;
		uint32_t wf_hi, wf_lo, wg_hi, wg_lo;
		uint32_t sf, sg;

		off = u * llen;
		wf_hi = Ft[off + hi];
		wf_lo = Ft[off + lo];
		wg_hi = Gt[off + hi];
		wg_lo = Gt[off + lo];
		sf = (wf_lo >> 30) ? 0x7FFFFFFFu : 0u;
		sg = (wg_lo >> 30) ? 0x7FFFFFFFu : 0u;
		if (wf_hi != sf || wg_hi != sg) {
			bad ++;
		}
	}
	return bad;
}

/*
 * Return 1 and fill outputs with the first blocking coefficient; return 0
 * if no blocker was found.
 */
static int
intermediate_get_first_shrink_blocker(
	const uint32_t *Ft, const uint32_t *Gt, size_t llen, size_t FGlen, size_t n,
	uint32_t *index,
	uint32_t *ft_hi, uint32_t *ft_lo,
	uint32_t *gt_hi, uint32_t *gt_lo)
{
	size_t u;
	size_t hi, lo;

	if (FGlen <= 1) {
		return 0;
	}
	hi = FGlen - 1;
	lo = FGlen - 2;
	for (u = 0; u < n; u ++) {
		size_t off;
		uint32_t wf_hi, wf_lo, wg_hi, wg_lo;
		uint32_t sf, sg;

		off = u * llen;
		wf_hi = Ft[off + hi];
		wf_lo = Ft[off + lo];
		wg_hi = Gt[off + hi];
		wg_lo = Gt[off + lo];
		sf = (wf_lo >> 30) ? 0x7FFFFFFFu : 0u;
		sg = (wg_lo >> 30) ? 0x7FFFFFFFu : 0u;
		if (wf_hi != sf || wg_hi != sg) {
			if (index != NULL) {
				*index = (uint32_t)u;
			}
			if (ft_hi != NULL) {
				*ft_hi = wf_hi;
			}
			if (ft_lo != NULL) {
				*ft_lo = wf_lo;
			}
			if (gt_hi != NULL) {
				*gt_hi = wg_hi;
			}
			if (gt_lo != NULL) {
				*gt_lo = wg_lo;
			}
			return 1;
		}
	}
	return 0;
}

static void
intermediate_fill_sign_extension(uint32_t *Ft, uint32_t *Gt, size_t llen,
	size_t cur_len, size_t target_len, size_t n)
{
	size_t u;

	if (cur_len == 0 || cur_len >= llen) {
		return;
	}
	if (target_len > llen) {
		target_len = llen;
	}
	if (cur_len >= target_len) {
		return;
	}
	for (u = 0; u < n; u ++) {
		size_t off, v;
		uint32_t sf, sg;

		off = u * llen;
		sf = (Ft[off + cur_len - 1] >> 30) ? 0x7FFFFFFFu : 0u;
		sg = (Gt[off + cur_len - 1] >> 30) ? 0x7FFFFFFFu : 0u;
		for (v = cur_len; v < target_len; v ++) {
			Ft[off + v] = sf;
			Gt[off + v] = sg;
		}
	}
}

static void
kg_capture_top_words(uint32_t *dst_f, uint32_t *dst_g,
	const uint32_t *Ft, const uint32_t *Gt, size_t llen, size_t FGlen, size_t n)
{
	size_t u, hi;

	if (FGlen == 0 || n > KG_TOP_TRACK_MAXN) {
		return;
	}
	hi = FGlen - 1;
	for (u = 0; u < n; u ++) {
		size_t off;

		off = u * llen;
		dst_f[u] = Ft[off + hi];
		dst_g[u] = Gt[off + hi];
	}
}

static uint32_t
kg_count_top_word_changes(const uint32_t *Ft, const uint32_t *Gt,
	size_t llen, size_t FGlen, size_t n,
	const uint32_t *pre_f, const uint32_t *pre_g,
	uint32_t *first_index,
	uint32_t *first_ft_pre, uint32_t *first_ft_post,
	uint32_t *first_gt_pre, uint32_t *first_gt_post)
{
	size_t u, hi;
	uint32_t count;

	if (FGlen == 0 || n > KG_TOP_TRACK_MAXN) {
		if (first_index != NULL) {
			*first_index = 0;
		}
		if (first_ft_pre != NULL) {
			*first_ft_pre = 0;
		}
		if (first_ft_post != NULL) {
			*first_ft_post = 0;
		}
		if (first_gt_pre != NULL) {
			*first_gt_pre = 0;
		}
		if (first_gt_post != NULL) {
			*first_gt_post = 0;
		}
		return 0;
	}
	hi = FGlen - 1;
	count = 0;
	for (u = 0; u < n; u ++) {
		size_t off;
		uint32_t ft_post, gt_post;

		off = u * llen;
		ft_post = Ft[off + hi];
		gt_post = Gt[off + hi];
		if (pre_f[u] != ft_post || pre_g[u] != gt_post) {
			if (count == 0) {
				if (first_index != NULL) {
					*first_index = (uint32_t)u;
				}
				if (first_ft_pre != NULL) {
					*first_ft_pre = pre_f[u];
				}
				if (first_ft_post != NULL) {
					*first_ft_post = ft_post;
				}
				if (first_gt_pre != NULL) {
					*first_gt_pre = pre_g[u];
				}
				if (first_gt_post != NULL) {
					*first_gt_post = gt_post;
				}
			}
			count ++;
		}
	}
	if (count == 0) {
		if (first_index != NULL) {
			*first_index = 0;
		}
		if (first_ft_pre != NULL) {
			*first_ft_pre = 0;
		}
		if (first_ft_post != NULL) {
			*first_ft_post = 0;
		}
		if (first_gt_pre != NULL) {
			*first_gt_pre = 0;
		}
		if (first_gt_post != NULL) {
			*first_gt_post = 0;
		}
	}
	return count;
}

#define KG_TAIL_REASON_SCALE_DONE       1
#define KG_TAIL_REASON_ITER_LIMIT       2
#define KG_TAIL_REASON_NOUPDATE_LIMIT   3
#define KG_TAIL_BLOCKER_WORDS_MAX     512

static void
kg_record_update_stall(unsigned depth, int had_update,
	uint32_t top_word_changed, size_t fglen_before, size_t fglen_after)
{
	if (!had_update) {
		return;
	}
	if (depth == 7) {
		if (fglen_after < fglen_before) {
			kg_d7_update_and_shrink_count ++;
		} else {
			kg_d7_update_no_shrink_count ++;
			if (!top_word_changed) {
				kg_d7_update_no_shrink_top_same_count ++;
			}
		}
	} else if (depth == 8) {
		if (fglen_after < fglen_before) {
			kg_d8_update_and_shrink_count ++;
		} else {
			kg_d8_update_no_shrink_count ++;
			if (!top_word_changed) {
				kg_d8_update_no_shrink_top_same_count ++;
			}
		}
	} else if (depth == 9) {
		if (fglen_after < fglen_before) {
			kg_d9_update_and_shrink_count ++;
		} else {
			kg_d9_update_no_shrink_count ++;
			if (!top_word_changed) {
				kg_d9_update_no_shrink_top_same_count ++;
			}
		}
	}
}

static void
kg_tail_compute_scaled_coeff_update(uint32_t *dst, size_t dstlen,
	const uint32_t *f, size_t flen, size_t fstride,
	const int32_t *k, uint32_t sc, unsigned logn, size_t coeff_index)
{
	size_t n, u;
	uint32_t sch, scl;

	memset(dst, 0, dstlen * sizeof *dst);
	n = (size_t)1 << logn;
	if (coeff_index >= n || dstlen == 0) {
		return;
	}
	DIVREM31(sch, scl, sc);
	for (u = 0; u < n; u ++) {
		int32_t ku;
		size_t v, xidx;
		const uint32_t *y;

		ku = -k[u];
		xidx = u;
		y = f;
		for (v = 0; v < n; v ++, y += fstride) {
			if (xidx == coeff_index) {
				zint_add_scaled_mul_small(dst, dstlen, y, flen, ku, sch, scl);
				break;
			}
			if (u + v == n - 1) {
				xidx = 0;
				ku = -ku;
			} else {
				xidx ++;
			}
		}
	}
}

static void
kg_tail_compute_scaled_coeff_update_one(uint32_t *dst, size_t dstlen,
	const uint32_t *f, size_t flen, size_t fstride,
	size_t k_index, int32_t k_value, uint32_t sc, unsigned logn,
	size_t coeff_index)
{
	size_t n, v, xidx;
	uint32_t sch, scl;
	int32_t ku;
	const uint32_t *y;

	memset(dst, 0, dstlen * sizeof *dst);
	n = (size_t)1 << logn;
	if (coeff_index >= n || k_index >= n || dstlen == 0) {
		return;
	}
	DIVREM31(sch, scl, sc);
	ku = -k_value;
	xidx = k_index;
	y = f;
	for (v = 0; v < n; v ++, y += fstride) {
		if (xidx == coeff_index) {
			zint_add_scaled_mul_small(dst, dstlen, y, flen, ku, sch, scl);
			break;
		}
		if (k_index + v == n - 1) {
			xidx = 0;
			ku = -ku;
		} else {
			xidx ++;
		}
	}
}

static inline uint32_t
kg_tail_signext_target(uint32_t lo)
{
	return (lo >> 30) ? 0x7FFFFFFFu : 0u;
}

static inline uint32_t
kg_tail_signext_gap(uint32_t hi, uint32_t lo)
{
	uint32_t target;

	target = kg_tail_signext_target(lo);
	return (target == 0u) ? hi : (0x7FFFFFFFu - hi);
}

static inline int
kg_tail_metrics_better(uint32_t trial_blockers, uint64_t trial_gap,
	uint64_t trial_excess, uint32_t ref_blockers, uint64_t ref_gap,
	uint64_t ref_excess)
{
	if (trial_blockers != ref_blockers) {
		return trial_blockers < ref_blockers;
	}
	if (trial_gap != ref_gap) {
		return trial_gap < ref_gap;
	}
	return trial_excess < ref_excess;
}

static inline int
kg_tail_metrics_progress(uint32_t trial_blockers, uint64_t trial_gap,
	uint32_t ref_blockers, uint64_t ref_gap)
{
	return trial_blockers < ref_blockers || trial_gap < ref_gap;
}

static inline int
kg_tail_k_saturated(const int32_t *k, size_t n)
{
	size_t u;

	for (u = 0; u < n; u ++) {
		if (k[u] == (int32_t)0x7FFFFFFF || k[u] == -(int32_t)0x7FFFFFFF) {
			return 1;
		}
	}
	return 0;
}

#define KG_TAIL_PROBE_MAX_N   16

static int
kg_tail_candidate_modp_ok(const uint32_t *Ft, const uint32_t *Gt,
	size_t llen, size_t FGlen,
	const uint32_t *ft, const uint32_t *gt,
	size_t flen, size_t fstride,
	const int32_t *k, uint32_t scale_k, uint32_t q, unsigned logn,
	uint32_t *bad_index_out, uint32_t *expected_out, uint32_t *actual_out)
{
	uint32_t Ft2[KG_TAIL_PROBE_MAX_N * KG_TAIL_BLOCKER_WORDS_MAX];
	uint32_t Gt2[KG_TAIL_PROBE_MAX_N * KG_TAIL_BLOCKER_WORDS_MAX];
	uint32_t modp_tmp[4 * KG_TAIL_PROBE_MAX_N];
	uint32_t bad_index;
	size_t n, u;

	n = (size_t)1 << logn;
	if (n == 0 || n > KG_TAIL_PROBE_MAX_N
		|| FGlen == 0 || FGlen > llen || llen > KG_TAIL_BLOCKER_WORDS_MAX)
	{
		if (bad_index_out != NULL) {
			*bad_index_out = 0;
		}
		if (expected_out != NULL) {
			*expected_out = 0;
		}
		if (actual_out != NULL) {
			*actual_out = 0;
		}
		return 0;
	}
	for (u = 0; u < n; u ++) {
		memcpy(Ft2 + u * llen, Ft + u * llen, llen * sizeof *Ft2);
		memcpy(Gt2 + u * llen, Gt + u * llen, llen * sizeof *Gt2);
	}
	poly_sub_scaled(Ft2, FGlen, llen, ft, flen, fstride, k, scale_k, logn);
	poly_sub_scaled(Gt2, FGlen, llen, gt, flen, fstride, k, scale_k, logn);
	intermediate_fill_sign_extension(Ft2, Gt2, llen, FGlen, llen, n);
	if (intermediate_check_modp_direct(ft, gt, fstride, fstride,
		Ft2, Gt2, llen, llen, q, logn, modp_tmp, &bad_index))
	{
		if (bad_index_out != NULL) {
			*bad_index_out = 0;
		}
		if (expected_out != NULL) {
			*expected_out = 0;
		}
		if (actual_out != NULL) {
			*actual_out = 0;
		}
		return 1;
	}
	if (bad_index_out != NULL) {
		*bad_index_out = bad_index;
	}
	if (expected_out != NULL) {
		*expected_out = kg_last_modp_expected;
	}
	if (actual_out != NULL) {
		*actual_out = kg_last_modp_actual;
	}
	return 0;
}

static void
kg_tail_capture_blocker_local(ctl_tail_exit_diag *dst,
	const uint32_t *Ft, const uint32_t *Gt, size_t llen, size_t FGlen,
	const uint32_t *ft, const uint32_t *gt, size_t flen, size_t fstride,
	const int32_t *k,
	const uint64_t *rt2_raw_snapshot, const int32_t *rt2_round_snapshot,
	size_t snap_n, uint32_t scale_k, unsigned logn)
{
	uint32_t f_delta[KG_TAIL_BLOCKER_WORDS_MAX];
	uint32_t g_delta[KG_TAIL_BLOCKER_WORDS_MAX];
	uint32_t f_pre[KG_TAIL_BLOCKER_WORDS_MAX];
	uint32_t g_pre[KG_TAIL_BLOCKER_WORDS_MAX];
	size_t idx, off, hi, lo, n;
	uint64_t rv;

	if (dst->blocker_count == 0 || FGlen < 2 || FGlen > KG_TAIL_BLOCKER_WORDS_MAX) {
		return;
	}
	idx = (size_t)dst->blocker_index;
	off = idx * llen;
	hi = FGlen - 1;
	lo = FGlen - 2;
	n = (size_t)1 << logn;

	kg_tail_compute_scaled_coeff_update(f_delta, FGlen,
		ft, flen, fstride, k, scale_k, logn, idx);
	kg_tail_compute_scaled_coeff_update(g_delta, FGlen,
		gt, flen, fstride, k, scale_k, logn, idx);

	memcpy(f_pre, Ft + off, FGlen * sizeof *f_pre);
	memcpy(g_pre, Gt + off, FGlen * sizeof *g_pre);
	zint_add_scaled_mul_small(f_pre, FGlen, f_delta, FGlen, -1, 0, 0);
	zint_add_scaled_mul_small(g_pre, FGlen, g_delta, FGlen, -1, 0, 0);

	dst->ft_pre_hi = f_pre[hi];
	dst->ft_pre_lo = f_pre[lo];
	dst->gt_pre_hi = g_pre[hi];
	dst->gt_pre_lo = g_pre[lo];
	dst->ft_delta_hi = f_delta[hi];
	dst->ft_delta_lo = f_delta[lo];
	dst->gt_delta_hi = g_delta[hi];
	dst->gt_delta_lo = g_delta[lo];
	dst->ft_target_hi_pre = kg_tail_signext_target(dst->ft_pre_lo);
	dst->ft_target_hi_post = kg_tail_signext_target(dst->ft_lo);
	dst->gt_target_hi_pre = kg_tail_signext_target(dst->gt_pre_lo);
	dst->gt_target_hi_post = kg_tail_signext_target(dst->gt_lo);
	dst->ft_gap_pre = kg_tail_signext_gap(dst->ft_pre_hi, dst->ft_pre_lo);
	dst->ft_gap_post = kg_tail_signext_gap(dst->ft_hi, dst->ft_lo);
	dst->gt_gap_pre = kg_tail_signext_gap(dst->gt_pre_hi, dst->gt_pre_lo);
	dst->gt_gap_post = kg_tail_signext_gap(dst->gt_hi, dst->gt_lo);
	dst->ft_gap_progress = (int32_t)dst->ft_gap_pre - (int32_t)dst->ft_gap_post;
	dst->gt_gap_progress = (int32_t)dst->gt_gap_pre - (int32_t)dst->gt_gap_post;
	dst->unit_snap_n = (uint32_t)((n < 8) ? n : 8);
	for (size_t u = 0; u < dst->unit_snap_n; u ++) {
		uint32_t uf_delta[KG_TAIL_BLOCKER_WORDS_MAX];
		uint32_t ug_delta[KG_TAIL_BLOCKER_WORDS_MAX];

		kg_tail_compute_scaled_coeff_update_one(uf_delta, FGlen,
			ft, flen, fstride, u, 1, scale_k, logn, idx);
		kg_tail_compute_scaled_coeff_update_one(ug_delta, FGlen,
			gt, flen, fstride, u, 1, scale_k, logn, idx);
		dst->blocker_unit_ft_hi[u] = sign_ext(uf_delta[hi]);
		dst->blocker_unit_ft_lo[u] = sign_ext(uf_delta[lo]);
		dst->blocker_unit_gt_hi[u] = sign_ext(ug_delta[hi]);
		dst->blocker_unit_gt_lo[u] = sign_ext(ug_delta[lo]);
	}
	dst->blocker_k_value = k[idx];
	if (rt2_raw_snapshot != NULL && rt2_round_snapshot != NULL && idx < snap_n) {
		rv = rt2_raw_snapshot[idx];
		dst->blocker_rt2_lo = (uint32_t)rv;
		dst->blocker_rt2_hi = (uint32_t)(rv >> 32);
		dst->blocker_rt2_round = rt2_round_snapshot[idx];
	}
}

static void
kg_tail_capture_blocker_other_window(ctl_tail_exit_diag *dst,
	const uint32_t *Ft, const uint32_t *Gt, size_t llen, size_t FGlen,
	const uint32_t *ft, const uint32_t *gt, size_t fstride,
	size_t current_flen, size_t other_flen, size_t rlen_cur,
	uint32_t scale_fg, uint32_t scale_FG_eff, const int32_t *k,
	unsigned logn)
{
	uint32_t f_delta_cur[KG_TAIL_BLOCKER_WORDS_MAX];
	uint32_t g_delta_cur[KG_TAIL_BLOCKER_WORDS_MAX];
	uint32_t f_delta[KG_TAIL_BLOCKER_WORDS_MAX];
	uint32_t g_delta[KG_TAIL_BLOCKER_WORDS_MAX];
	uint32_t f_pre[KG_TAIL_BLOCKER_WORDS_MAX];
	uint32_t g_pre[KG_TAIL_BLOCKER_WORDS_MAX];
	uint32_t f_post[KG_TAIL_BLOCKER_WORDS_MAX];
	uint32_t g_post[KG_TAIL_BLOCKER_WORDS_MAX];
	size_t idx, off, hi, lo;
	size_t rlen_base, other_rlen_cur;
	uint32_t current_scale_k, other_scale_fg, other_scale_k;

	if (dst->blocker_count == 0 || FGlen < 2 || FGlen > KG_TAIL_BLOCKER_WORDS_MAX) {
		return;
	}
	if (other_flen == 0 || other_flen == current_flen) {
		return;
	}
	idx = (size_t)dst->blocker_index;
	off = idx * llen;
	hi = FGlen - 1;
	lo = FGlen - 2;
	rlen_base = rlen_cur + (scale_fg / 31u);
	current_scale_k = scale_FG_eff - scale_fg;
	other_rlen_cur = other_flen;
	if (other_rlen_cur > rlen_base) {
		other_rlen_cur = rlen_base;
	}
	other_scale_fg = (uint32_t)(31u * (uint32_t)(other_flen - other_rlen_cur));
	other_scale_k = scale_FG_eff - other_scale_fg;

	kg_tail_compute_scaled_coeff_update(f_delta_cur, FGlen,
		ft, current_flen, fstride, k, current_scale_k, logn, idx);
	kg_tail_compute_scaled_coeff_update(g_delta_cur, FGlen,
		gt, current_flen, fstride, k, current_scale_k, logn, idx);
	kg_tail_compute_scaled_coeff_update(f_delta, FGlen,
		ft, other_flen, fstride, k, other_scale_k, logn, idx);
	kg_tail_compute_scaled_coeff_update(g_delta, FGlen,
		gt, other_flen, fstride, k, other_scale_k, logn, idx);

	memcpy(f_pre, Ft + off, FGlen * sizeof *f_pre);
	memcpy(g_pre, Gt + off, FGlen * sizeof *g_pre);
	zint_add_scaled_mul_small(f_pre, FGlen, f_delta_cur, FGlen, -1, 0, 0);
	zint_add_scaled_mul_small(g_pre, FGlen, g_delta_cur, FGlen, -1, 0, 0);

	memcpy(f_post, f_pre, FGlen * sizeof *f_post);
	memcpy(g_post, g_pre, FGlen * sizeof *g_post);
	zint_add_scaled_mul_small(f_post, FGlen, f_delta, FGlen, 1, 0, 0);
	zint_add_scaled_mul_small(g_post, FGlen, g_delta, FGlen, 1, 0, 0);

	dst->current_flen = (uint32_t)current_flen;
	dst->other_flen = (uint32_t)other_flen;
	dst->other_rlen_cur = (uint32_t)other_rlen_cur;
	dst->other_scale_fg = other_scale_fg;
	dst->other_scale_k = other_scale_k;
	dst->other_ft_delta_hi = f_delta[hi];
	dst->other_ft_delta_lo = f_delta[lo];
	dst->other_gt_delta_hi = g_delta[hi];
	dst->other_gt_delta_lo = g_delta[lo];
	dst->other_ft_hi = f_post[hi];
	dst->other_ft_lo = f_post[lo];
	dst->other_gt_hi = g_post[hi];
	dst->other_gt_lo = g_post[lo];
	dst->other_ft_gap_post = kg_tail_signext_gap(dst->other_ft_hi, dst->other_ft_lo);
	dst->other_gt_gap_post = kg_tail_signext_gap(dst->other_gt_hi, dst->other_gt_lo);
	dst->other_ft_gap_progress =
		(int32_t)dst->ft_gap_pre - (int32_t)dst->other_ft_gap_post;
	dst->other_gt_gap_progress =
		(int32_t)dst->gt_gap_pre - (int32_t)dst->other_gt_gap_post;
}

static void
kg_tail_capture_round_snapshot(ctl_tail_exit_diag *dst,
	const int32_t *k, const int32_t *rt2_round_snapshot, size_t n)
{
	size_t u, m;

	m = n;
	if (m > 8) {
		m = 8;
	}
	dst->snap_n = (uint32_t)m;
	for (u = 0; u < m; u ++) {
		dst->k_snapshot[u] = k[u];
		dst->rt2_round_snapshot[u] = rt2_round_snapshot[u];
	}
}

static int
kg_tail_compute_probe_k(int32_t *k_out,
	uint64_t *rt2_raw_snapshot, size_t rt2_snap_n,
	uint32_t *scale_fg_out, uint32_t *scale_x_out,
	uint32_t *scale_t_out, uint32_t *tlen_out, uint32_t *toff_out,
	uint64_t *rt3_raw_out, uint64_t *rt4_raw_out, uint64_t *rt5_raw_out,
	uint32_t *scale_k_out,
	uint32_t *k_nz_out, uint64_t *k_abs_sum_out, uint32_t *k_abs_max_out,
	const uint32_t *Ft, const uint32_t *Gt, size_t FGlen, size_t llen,
	const uint32_t *ft, const uint32_t *gt, size_t flen, size_t rlen_cur,
	size_t sstride, uint32_t scale_FG_eff, uint32_t scale_x_fg,
	int direct_scale_k, int use_local_scale_x_fg, unsigned logn)
{
	fnr prt1[KG_TAIL_PROBE_MAX_N];
	fnr prt2[KG_TAIL_PROBE_MAX_N];
	fnr prt3[KG_TAIL_PROBE_MAX_N];
	fnr prt4[KG_TAIL_PROBE_MAX_N];
	fnr prt5[KG_TAIL_PROBE_MAX_N];
	size_t n, u, tlen;
	uint32_t toff;
	uint32_t scale_fg, scale_xf, scale_xg, scale_x, scale_t, scale_k;
	uint32_t scale_x_fg_eff;
	uint32_t k_nz, k_abs_max;
	uint64_t k_abs_sum;

	n = (size_t)1 << logn;
	if (n == 0 || n > KG_TAIL_PROBE_MAX_N || rlen_cur == 0
		|| rlen_cur > flen)
	{
		return 0;
	}
	scale_fg = 31 * (uint32_t)(flen - rlen_cur);
	scale_xf = poly_max_bitlength(
		ft + flen - rlen_cur, rlen_cur, sstride, logn);
	scale_xg = poly_max_bitlength(
		gt + flen - rlen_cur, rlen_cur, sstride, logn);
	scale_x = scale_xf
		^ (-((scale_xf - scale_xg) >> 31) & (scale_xf ^ scale_xg));
	scale_t = 15 - logn;
	scale_t ^= -((uint32_t)(scale_x - scale_t) >> 31)
		& (scale_x ^ scale_t);
	scale_x_fg_eff = use_local_scale_x_fg ? scale_x : scale_x_fg;
	poly_big_to_fixed(prt3, ft + flen - rlen_cur, rlen_cur, sstride,
		scale_x - scale_t, logn);
	poly_big_to_fixed(prt4, gt + flen - rlen_cur, rlen_cur, sstride,
		scale_x - scale_t, logn);
	ctl_FFT(prt3, logn);
	ctl_FFT(prt4, logn);
	ctl_poly_invnorm_fft(prt5, prt3, prt4, scale_t << 1, logn);
	ctl_poly_adj_fft(prt3, logn);
	ctl_poly_adj_fft(prt4, logn);
	ctl_poly_div_2e(prt3, scale_t, logn);
	ctl_poly_div_2e(prt4, scale_t, logn);
	if (rt3_raw_out != NULL) {
		*rt3_raw_out = prt3[0].v;
	}
	if (rt4_raw_out != NULL) {
		*rt4_raw_out = prt4[0].v;
	}
	if (rt5_raw_out != NULL) {
		*rt5_raw_out = prt5[0].v;
	}

	DIVREM31(tlen, toff, scale_FG_eff);
	if (tlen > FGlen) {
		return 0;
	}
	poly_big_to_fixed(prt1, Ft + tlen, FGlen - tlen, llen,
		scale_x_fg_eff + toff, logn);
	poly_big_to_fixed(prt2, Gt + tlen, FGlen - tlen, llen,
		scale_x_fg_eff + toff, logn);
	ctl_FFT(prt1, logn);
	ctl_FFT(prt2, logn);
	ctl_poly_mul_fft(prt1, prt3, logn);
	ctl_poly_mul_fft(prt2, prt4, logn);
	ctl_poly_add(prt2, prt1, logn);
	ctl_poly_mul_autoadj_fft(prt2, prt5, logn);
	ctl_iFFT(prt2, logn);

	k_nz = 0;
	k_abs_sum = 0;
	k_abs_max = 0;
	if (direct_scale_k) {
		scale_k = scale_FG_eff - scale_fg;
	} else {
		scale_k = (scale_FG_eff + scale_x_fg_eff) - (scale_fg + scale_x);
	}
	for (u = 0; u < n; u ++) {
		uint32_t ak;

		k_out[u] = fnr_round(prt2[u]);
		ak = (uint32_t)(k_out[u] < 0 ? -k_out[u] : k_out[u]);
		k_nz += (uint32_t)(k_out[u] != 0);
		k_abs_sum += ak;
		if (ak > k_abs_max) {
			k_abs_max = ak;
		}
		if (u < rt2_snap_n) {
			rt2_raw_snapshot[u] = prt2[u].v;
		}
	}
	*scale_k_out = scale_k;
	*scale_fg_out = scale_fg;
	*scale_x_out = scale_x;
	*scale_t_out = scale_t;
	*tlen_out = (uint32_t)tlen;
	*toff_out = toff;
	*k_nz_out = k_nz;
	*k_abs_sum_out = k_abs_sum;
	*k_abs_max_out = k_abs_max;
	return 1;
}

static int
kg_tail_eval_blocker_gap_for_k(uint64_t *gap_out,
	const uint32_t *Ft, const uint32_t *Gt, size_t llen, size_t FGlen,
	uint32_t blocker_index,
	const uint32_t *ft, const uint32_t *gt, size_t flen, size_t fstride,
	const int32_t *k, uint32_t scale_k, unsigned logn)
{
	uint32_t f_delta[KG_TAIL_BLOCKER_WORDS_MAX];
	uint32_t g_delta[KG_TAIL_BLOCKER_WORDS_MAX];
	uint32_t f_post[KG_TAIL_BLOCKER_WORDS_MAX];
	uint32_t g_post[KG_TAIL_BLOCKER_WORDS_MAX];
	size_t idx, off, hi, lo, n;

	n = (size_t)1 << logn;
	if (FGlen < 2 || FGlen > KG_TAIL_BLOCKER_WORDS_MAX || blocker_index >= n) {
		return 0;
	}
	idx = (size_t)blocker_index;
	off = idx * llen;
	hi = FGlen - 1;
	lo = FGlen - 2;
	kg_tail_compute_scaled_coeff_update(f_delta, FGlen,
		ft, flen, fstride, k, scale_k, logn, idx);
	kg_tail_compute_scaled_coeff_update(g_delta, FGlen,
		gt, flen, fstride, k, scale_k, logn, idx);
	memcpy(f_post, Ft + off, FGlen * sizeof *f_post);
	memcpy(g_post, Gt + off, FGlen * sizeof *g_post);
	zint_add_scaled_mul_small(f_post, FGlen, f_delta, FGlen, 1, 0, 0);
	zint_add_scaled_mul_small(g_post, FGlen, g_delta, FGlen, 1, 0, 0);
	*gap_out = (uint64_t)kg_tail_signext_gap(f_post[hi], f_post[lo])
		+ (uint64_t)kg_tail_signext_gap(g_post[hi], g_post[lo]);
	return 1;
}

static inline int64_t kg_tail_pair_to_s64(uint32_t hi, uint32_t lo);
static inline uint64_t kg_tail_pair_excess(uint32_t hi, uint32_t lo);
static int32_t kg_tail_round_div64(int64_t num, int64_t den);
static int32_t kg_tail_round_div128(__int128 num, __int128 den);
static void kg_tail_add_candidate(int32_t *cand, size_t *num, int32_t v);
static void kg_tail_eval_single_k(uint32_t *blockers, uint64_t *excess,
	const uint32_t *Ft, const uint32_t *Gt, size_t llen, size_t FGlen,
	const uint32_t *ft, const uint32_t *gt, size_t flen, size_t fstride,
	size_t k_index, int32_t k_value, uint32_t scale_k, unsigned logn);
static void kg_tail_eval_sparse_k(uint32_t *blockers, uint64_t *excess,
	const uint32_t *Ft, const uint32_t *Gt, size_t llen, size_t FGlen,
	const uint32_t *ft, const uint32_t *gt, size_t flen, size_t fstride,
	const int32_t *k, uint32_t scale_k, unsigned logn);
static void kg_tail_eval_sparse_k_exact(uint32_t *blockers, uint64_t *excess,
	const uint32_t *Ft, const uint32_t *Gt, size_t llen, size_t FGlen,
	const uint32_t *ft, const uint32_t *gt, size_t flen, size_t fstride,
	const int32_t *k, uint32_t scale_k, unsigned logn);

static int
kg_tail_project_probe_exact(int32_t *k_out, uint64_t *gap_out,
	const uint32_t *Ft, const uint32_t *Gt, size_t llen, size_t FGlen,
	uint32_t blocker_index,
	const uint32_t *ft, const uint32_t *gt, size_t exact_flen,
	size_t fstride, uint32_t exact_scale_k,
	size_t probe_flen, uint32_t probe_scale_k,
	const int32_t *probe_k, unsigned logn)
{
	uint32_t pf_delta[KG_TAIL_BLOCKER_WORDS_MAX];
	uint32_t pg_delta[KG_TAIL_BLOCKER_WORDS_MAX];
	uint32_t ef_delta[KG_TAIL_BLOCKER_WORDS_MAX];
	uint32_t eg_delta[KG_TAIL_BLOCKER_WORDS_MAX];
	int32_t cand[8];
	int32_t k_tmp[KG_TAIL_PROBE_MAX_N];
	size_t n, hi, lo, u, cand_num, best_u;
	int64_t cur_f, cur_g, probe_df, probe_dg, exact_df, exact_dg;
	uint64_t best_gap, cand_gap;
	int found;

	n = (size_t)1 << logn;
	if (n == 0 || n > KG_TAIL_PROBE_MAX_N || FGlen < 2
		|| blocker_index >= n)
	{
		return 0;
	}
	hi = FGlen - 1;
	lo = FGlen - 2;
	cur_f = kg_tail_pair_to_s64(Ft[blocker_index * llen + hi],
		Ft[blocker_index * llen + lo]);
	cur_g = kg_tail_pair_to_s64(Gt[blocker_index * llen + hi],
		Gt[blocker_index * llen + lo]);
	kg_tail_compute_scaled_coeff_update(pf_delta, FGlen,
		ft, probe_flen, fstride, probe_k, probe_scale_k, logn, blocker_index);
	kg_tail_compute_scaled_coeff_update(pg_delta, FGlen,
		gt, probe_flen, fstride, probe_k, probe_scale_k, logn, blocker_index);
	kg_tail_compute_scaled_coeff_update(ef_delta, FGlen,
		ft, exact_flen, fstride, probe_k, exact_scale_k, logn, blocker_index);
	kg_tail_compute_scaled_coeff_update(eg_delta, FGlen,
		gt, exact_flen, fstride, probe_k, exact_scale_k, logn, blocker_index);
	probe_df = kg_tail_pair_to_s64(pf_delta[hi], pf_delta[lo]);
	probe_dg = kg_tail_pair_to_s64(pg_delta[hi], pg_delta[lo]);
	exact_df = kg_tail_pair_to_s64(ef_delta[hi], ef_delta[lo]);
	exact_dg = kg_tail_pair_to_s64(eg_delta[hi], eg_delta[lo]);
	if ((probe_df == 0 && probe_dg == 0) || (exact_df == 0 && exact_dg == 0)) {
		return 0;
	}
	cand_num = 0;
	kg_tail_add_candidate(cand, &cand_num, 1);
	kg_tail_add_candidate(cand, &cand_num, -1);
	if (exact_df != 0) {
		int32_t qf;

		qf = kg_tail_round_div64(probe_df, exact_df);
		kg_tail_add_candidate(cand, &cand_num, qf);
		kg_tail_add_candidate(cand, &cand_num, qf - 1);
		kg_tail_add_candidate(cand, &cand_num, qf + 1);
		qf = kg_tail_round_div64(-cur_f, exact_df);
		kg_tail_add_candidate(cand, &cand_num, qf);
	}
	if (exact_dg != 0) {
		int32_t qg;

		qg = kg_tail_round_div64(probe_dg, exact_dg);
		kg_tail_add_candidate(cand, &cand_num, qg);
		kg_tail_add_candidate(cand, &cand_num, qg - 1);
		kg_tail_add_candidate(cand, &cand_num, qg + 1);
		qg = kg_tail_round_div64(-cur_g, exact_dg);
		kg_tail_add_candidate(cand, &cand_num, qg);
	}

	best_gap = (uint64_t)-1;
	best_u = 0;
	found = 0;
	for (u = 0; u < cand_num; u ++) {
		size_t v;
		int32_t km;
		int valid;

		km = cand[u];
		if (km == 0) {
			continue;
		}
		valid = 0;
		for (v = 0; v < n; v ++) {
			int64_t z;

			z = (int64_t)probe_k[v] * (int64_t)km;
			if (z < -0x7FFFFFFFLL || z > 0x7FFFFFFFLL) {
				valid = 0;
				break;
			}
			k_tmp[v] = (int32_t)z;
			valid |= (k_tmp[v] != 0);
		}
		if (!valid) {
			continue;
		}
		if (!kg_tail_eval_blocker_gap_for_k(&cand_gap,
			Ft, Gt, llen, FGlen, blocker_index,
			ft, gt, exact_flen, fstride,
			k_tmp, exact_scale_k, logn))
		{
			continue;
		}
		if (!found || cand_gap < best_gap) {
			best_gap = cand_gap;
			best_u = u;
			found = 1;
		}
	}
	if (!found) {
		return 0;
	}
	for (u = 0; u < n; u ++) {
		k_out[u] = (int32_t)((int64_t)probe_k[u] * (int64_t)cand[best_u]);
	}
	if (gap_out != NULL) {
		*gap_out = best_gap;
	}
	return 1;
}

static int
kg_tail_fit_probe_pair_exact(int32_t *k_out, uint64_t *gap_out,
	const uint32_t *Ft, const uint32_t *Gt, size_t llen, size_t FGlen,
	uint32_t blocker_index,
	const uint32_t *ft, const uint32_t *gt, size_t exact_flen,
	size_t fstride, uint32_t exact_scale_k,
	size_t probe_flen, uint32_t probe_scale_k,
	const int32_t *probe_k, unsigned logn)
{
	uint32_t pf_delta[KG_TAIL_BLOCKER_WORDS_MAX];
	uint32_t pg_delta[KG_TAIL_BLOCKER_WORDS_MAX];
	uint32_t unit_f1[KG_TAIL_BLOCKER_WORDS_MAX];
	uint32_t unit_g1[KG_TAIL_BLOCKER_WORDS_MAX];
	uint32_t unit_f2[KG_TAIL_BLOCKER_WORDS_MAX];
	uint32_t unit_g2[KG_TAIL_BLOCKER_WORDS_MAX];
	uint32_t support[KG_TAIL_PROBE_MAX_N];
	int32_t k_tmp[KG_TAIL_PROBE_MAX_N];
	size_t n, hi, lo, j, support_num, a, b, u, v;
	int64_t target_f, target_g;
	uint64_t best_gap;
	int found;

	n = (size_t)1 << logn;
	if (n == 0 || n > KG_TAIL_PROBE_MAX_N || FGlen < 2
		|| blocker_index >= n)
	{
		return 0;
	}
	hi = FGlen - 1;
	lo = FGlen - 2;
	kg_tail_compute_scaled_coeff_update(pf_delta, FGlen,
		ft, probe_flen, fstride, probe_k, probe_scale_k, logn, blocker_index);
	kg_tail_compute_scaled_coeff_update(pg_delta, FGlen,
		gt, probe_flen, fstride, probe_k, probe_scale_k, logn, blocker_index);
	target_f = kg_tail_pair_to_s64(pf_delta[hi], pf_delta[lo]);
	target_g = kg_tail_pair_to_s64(pg_delta[hi], pg_delta[lo]);
	if (target_f == 0 && target_g == 0) {
		return 0;
	}
	support_num = 0;
	for (j = 0; j < n; j ++) {
		if (probe_k[j] != 0 && support_num < KG_TAIL_PROBE_MAX_N) {
			support[support_num ++] = (uint32_t)j;
		}
	}
	if (support_num < 2) {
		return 0;
	}
	best_gap = (uint64_t)-1;
	found = 0;
	memset(k_out, 0, n * sizeof *k_out);
	for (a = 0; a + 1 < support_num; a ++) {
		for (b = a + 1; b < support_num; b ++) {
			size_t j1, j2;
			int64_t d1_f, d1_g, d2_f, d2_g;
			__int128 det;
			int32_t base1, base2;
			int32_t cand1[4], cand2[4];
			size_t cand1_num, cand2_num;

			j1 = support[a];
			j2 = support[b];
			kg_tail_compute_scaled_coeff_update_one(unit_f1, FGlen,
				ft, exact_flen, fstride, j1, 1, exact_scale_k, logn,
				blocker_index);
			kg_tail_compute_scaled_coeff_update_one(unit_g1, FGlen,
				gt, exact_flen, fstride, j1, 1, exact_scale_k, logn,
				blocker_index);
			kg_tail_compute_scaled_coeff_update_one(unit_f2, FGlen,
				ft, exact_flen, fstride, j2, 1, exact_scale_k, logn,
				blocker_index);
			kg_tail_compute_scaled_coeff_update_one(unit_g2, FGlen,
				gt, exact_flen, fstride, j2, 1, exact_scale_k, logn,
				blocker_index);
			d1_f = kg_tail_pair_to_s64(unit_f1[hi], unit_f1[lo]);
			d1_g = kg_tail_pair_to_s64(unit_g1[hi], unit_g1[lo]);
			d2_f = kg_tail_pair_to_s64(unit_f2[hi], unit_f2[lo]);
			d2_g = kg_tail_pair_to_s64(unit_g2[hi], unit_g2[lo]);
			det = (__int128)d1_f * (__int128)d2_g
				- (__int128)d1_g * (__int128)d2_f;
			if (det == 0) {
				continue;
			}
			base1 = kg_tail_round_div128(
				(__int128)target_f * (__int128)d2_g
				- (__int128)target_g * (__int128)d2_f, det);
			base2 = kg_tail_round_div128(
				(__int128)d1_f * (__int128)target_g
				- (__int128)d1_g * (__int128)target_f, det);
			cand1_num = 0;
			cand2_num = 0;
			kg_tail_add_candidate(cand1, &cand1_num, base1);
			kg_tail_add_candidate(cand1, &cand1_num, base1 - 1);
			kg_tail_add_candidate(cand1, &cand1_num, base1 + 1);
			kg_tail_add_candidate(cand1, &cand1_num,
				(probe_k[j1] < 0) ? -1 : 1);
			kg_tail_add_candidate(cand2, &cand2_num, base2);
			kg_tail_add_candidate(cand2, &cand2_num, base2 - 1);
			kg_tail_add_candidate(cand2, &cand2_num, base2 + 1);
			kg_tail_add_candidate(cand2, &cand2_num,
				(probe_k[j2] < 0) ? -1 : 1);
			for (u = 0; u < cand1_num; u ++) {
				for (v = 0; v < cand2_num; v ++) {
					uint64_t cand_gap;

					memset(k_tmp, 0, n * sizeof *k_tmp);
					k_tmp[j1] = cand1[u];
					k_tmp[j2] = cand2[v];
					if (k_tmp[j1] == 0 && k_tmp[j2] == 0) {
						continue;
					}
					if (!kg_tail_eval_blocker_gap_for_k(&cand_gap,
						Ft, Gt, llen, FGlen, blocker_index,
						ft, gt, exact_flen, fstride,
						k_tmp, exact_scale_k, logn))
					{
						continue;
					}
					if (!found || cand_gap < best_gap) {
						best_gap = cand_gap;
						memcpy(k_out, k_tmp, n * sizeof *k_out);
						found = 1;
					}
				}
			}
		}
	}
	if (!found) {
		return 0;
	}
	if (gap_out != NULL) {
		*gap_out = best_gap;
	}
	return 1;
}

static int
kg_tail_fit_probe_support_exact(int32_t *k_out, uint64_t *gap_out,
	const uint32_t *Ft, const uint32_t *Gt, size_t llen, size_t FGlen,
	uint32_t blocker_index,
	const uint32_t *ft, const uint32_t *gt, size_t exact_flen,
	size_t fstride, uint32_t exact_scale_k,
	size_t probe_flen, uint32_t probe_scale_k,
	const int32_t *probe_k, unsigned logn)
{
	uint32_t pf_delta[KG_TAIL_BLOCKER_WORDS_MAX];
	uint32_t pg_delta[KG_TAIL_BLOCKER_WORDS_MAX];
	uint32_t unit_f[KG_TAIL_BLOCKER_WORDS_MAX];
	uint32_t unit_g[KG_TAIL_BLOCKER_WORDS_MAX];
	uint32_t support[KG_TAIL_PROBE_MAX_N];
	size_t n, hi, lo, j, support_num, iter;
	int64_t target_f, target_g, res_f, res_g;
	int nonzero;

	n = (size_t)1 << logn;
	if (n == 0 || n > KG_TAIL_PROBE_MAX_N || FGlen < 2
		|| blocker_index >= n)
	{
		return 0;
	}
	hi = FGlen - 1;
	lo = FGlen - 2;
	kg_tail_compute_scaled_coeff_update(pf_delta, FGlen,
		ft, probe_flen, fstride, probe_k, probe_scale_k, logn, blocker_index);
	kg_tail_compute_scaled_coeff_update(pg_delta, FGlen,
		gt, probe_flen, fstride, probe_k, probe_scale_k, logn, blocker_index);
	target_f = kg_tail_pair_to_s64(pf_delta[hi], pf_delta[lo]);
	target_g = kg_tail_pair_to_s64(pg_delta[hi], pg_delta[lo]);
	if (target_f == 0 && target_g == 0) {
		return 0;
	}
	support_num = 0;
	for (j = 0; j < n; j ++) {
		if (probe_k[j] != 0 && support_num < KG_TAIL_PROBE_MAX_N) {
			support[support_num ++] = (uint32_t)j;
		}
	}
	if (support_num == 0) {
		return 0;
	}
	memset(k_out, 0, n * sizeof *k_out);
	res_f = target_f;
	res_g = target_g;
	for (iter = 0; iter < support_num; iter ++) {
		size_t best_j;
		__int128 best_score;
		int64_t best_df, best_dg;
		int32_t best_q;
		int found;

		best_j = 0;
		best_score = 0;
		best_df = 0;
		best_dg = 0;
		best_q = 0;
		found = 0;
		for (j = 0; j < support_num; j ++) {
			size_t idx;
			int64_t df, dg;
			__int128 dot, norm;
			int32_t q;

			idx = support[j];
			kg_tail_compute_scaled_coeff_update_one(unit_f, FGlen,
				ft, exact_flen, fstride, idx, 1, exact_scale_k, logn,
				blocker_index);
			kg_tail_compute_scaled_coeff_update_one(unit_g, FGlen,
				gt, exact_flen, fstride, idx, 1, exact_scale_k, logn,
				blocker_index);
			df = kg_tail_pair_to_s64(unit_f[hi], unit_f[lo]);
			dg = kg_tail_pair_to_s64(unit_g[hi], unit_g[lo]);
			if (df == 0 && dg == 0) {
				continue;
			}
			dot = (__int128)res_f * (__int128)df
				+ (__int128)res_g * (__int128)dg;
			norm = (__int128)df * (__int128)df
				+ (__int128)dg * (__int128)dg;
			if (norm == 0) {
				continue;
			}
			q = kg_tail_round_div128(dot, norm);
			if (q == 0) {
				q = (probe_k[idx] < 0) ? -1 : 1;
			}
			if (!found || dot * dot > best_score) {
				best_score = dot * dot;
				best_j = idx;
				best_df = df;
				best_dg = dg;
				best_q = q;
				found = 1;
			}
		}
		if (!found || best_q == 0) {
			break;
		}
		k_out[best_j] += best_q;
		res_f -= (int64_t)((__int128)best_q * (__int128)best_df);
		res_g -= (int64_t)((__int128)best_q * (__int128)best_dg);
		if (res_f == 0 && res_g == 0) {
			break;
		}
	}
	nonzero = 0;
	for (j = 0; j < n; j ++) {
		nonzero |= (k_out[j] != 0);
	}
	if (!nonzero) {
		return 0;
	}
	if (gap_out != NULL
		&& !kg_tail_eval_blocker_gap_for_k(gap_out,
			Ft, Gt, llen, FGlen, blocker_index,
			ft, gt, exact_flen, fstride, k_out, exact_scale_k, logn))
	{
		*gap_out = 0;
	}
	return 1;
}

static int
kg_tail_try_escape_guided(int32_t *k_out, uint64_t *gap_out,
	const uint32_t *Ft, const uint32_t *Gt, size_t llen, size_t FGlen,
	const uint32_t *ft, const uint32_t *gt, size_t flen, size_t fstride,
	uint32_t scale_k, const int32_t *hint_k, unsigned logn)
{
	uint32_t f_delta[KG_TAIL_BLOCKER_WORDS_MAX];
	uint32_t g_delta[KG_TAIL_BLOCKER_WORDS_MAX];
	size_t n, hi, lo, idx, best_j, j;
	uint32_t best_blockers, cur_blockers;
	uint64_t best_excess, cur_excess;
	int32_t best_kv;
	int found;
	uint32_t ft_hi, ft_lo, gt_hi, gt_lo;
	int64_t cur_f, cur_g;

	n = (size_t)1 << logn;
	if (FGlen < 2 || FGlen > KG_TAIL_BLOCKER_WORDS_MAX || n == 0
		|| n > KG_TAIL_PROBE_MAX_N)
	{
		return 0;
	}
	if (!intermediate_get_first_shrink_blocker(Ft, Gt, llen, FGlen, n,
		(uint32_t *)&idx, &ft_hi, &ft_lo, &gt_hi, &gt_lo))
	{
		return 0;
	}
	hi = FGlen - 1;
	lo = FGlen - 2;
	cur_f = kg_tail_pair_to_s64(ft_hi, ft_lo);
	cur_g = kg_tail_pair_to_s64(gt_hi, gt_lo);
	cur_blockers = intermediate_count_shrink_blockers(Ft, Gt, llen, FGlen, n);
	cur_excess = 0;
	for (j = 0; j < n; j ++) {
		uint64_t ef, eg;
		size_t off;

		off = j * llen;
		ef = kg_tail_pair_excess(Ft[off + hi], Ft[off + lo]);
		eg = kg_tail_pair_excess(Gt[off + hi], Gt[off + lo]);
		cur_excess += ef + eg;
	}
	best_blockers = cur_blockers;
	best_excess = cur_excess;
	best_j = 0;
	best_kv = 0;
	found = 0;
	for (j = 0; j < n; j ++) {
		int32_t cand[8];
		int32_t k_tmp[KG_TAIL_PROBE_MAX_N];
		size_t cand_num, v;
		int64_t d_f, d_g;
		int32_t sign_hint;

		if (hint_k[j] == 0) {
			continue;
		}
		sign_hint = (hint_k[j] < 0) ? -1 : 1;
		cand_num = 0;
		kg_tail_compute_scaled_coeff_update_one(f_delta, FGlen,
			ft, flen, fstride, j, 1, scale_k, logn, idx);
		kg_tail_compute_scaled_coeff_update_one(g_delta, FGlen,
			gt, flen, fstride, j, 1, scale_k, logn, idx);
		d_f = kg_tail_pair_to_s64(f_delta[hi], f_delta[lo]);
		d_g = kg_tail_pair_to_s64(g_delta[hi], g_delta[lo]);
		kg_tail_add_candidate(cand, &cand_num, sign_hint);
		kg_tail_add_candidate(cand, &cand_num, -sign_hint);
		if (d_f != 0) {
			int32_t qf;

			qf = kg_tail_round_div64(-cur_f, d_f);
			kg_tail_add_candidate(cand, &cand_num, qf);
			kg_tail_add_candidate(cand, &cand_num, qf - sign_hint);
			kg_tail_add_candidate(cand, &cand_num, qf + sign_hint);
		}
		if (d_g != 0) {
			int32_t qg;

			qg = kg_tail_round_div64(-cur_g, d_g);
			kg_tail_add_candidate(cand, &cand_num, qg);
			kg_tail_add_candidate(cand, &cand_num, qg - sign_hint);
			kg_tail_add_candidate(cand, &cand_num, qg + sign_hint);
		}
		for (v = 0; v < cand_num; v ++) {
			uint32_t trial_blockers;
			uint64_t trial_excess;

			memset(k_tmp, 0, n * sizeof *k_tmp);
			k_tmp[j] = cand[v];
			kg_tail_eval_sparse_k_exact(&trial_blockers, &trial_excess,
				Ft, Gt, llen, FGlen,
				ft, gt, flen, fstride,
				k_tmp, scale_k, logn);
			if (trial_blockers < best_blockers
				|| (trial_blockers == best_blockers
					&& trial_excess < best_excess))
			{
				best_blockers = trial_blockers;
				best_excess = trial_excess;
				best_j = j;
				best_kv = cand[v];
				found = 1;
			}
		}
	}
	if (!found
		|| (best_blockers > cur_blockers)
		|| (best_blockers == cur_blockers && best_excess >= cur_excess))
	{
		return 0;
	}
	memset(k_out, 0, n * sizeof *k_out);
	k_out[best_j] = best_kv;
	if (gap_out != NULL
		&& !kg_tail_eval_blocker_gap_for_k(gap_out,
			Ft, Gt, llen, FGlen, (uint32_t)idx,
			ft, gt, flen, fstride, k_out, scale_k, logn))
	{
		*gap_out = 0;
	}
	return 1;
}

static int
kg_tail_try_escape_guided_pair(int32_t *k_out, uint64_t *gap_out,
	const uint32_t *Ft, const uint32_t *Gt, size_t llen, size_t FGlen,
	const uint32_t *ft, const uint32_t *gt, size_t flen, size_t fstride,
	uint32_t scale_k, const int32_t *hint_k, unsigned logn)
{
	uint32_t support[KG_TAIL_PROBE_MAX_N];
	int32_t k_tmp[KG_TAIL_PROBE_MAX_N];
	size_t n, hi, lo, idx, support_num, a, b, j;
	uint32_t cur_blockers, best_blockers;
	uint64_t cur_excess, best_excess;
	size_t best_j1, best_j2;
	int32_t best_k1, best_k2;
	int found;
	uint32_t ft_hi, ft_lo, gt_hi, gt_lo;
	int64_t cur_f, cur_g;

	n = (size_t)1 << logn;
	if (FGlen < 2 || FGlen > KG_TAIL_BLOCKER_WORDS_MAX || n == 0
		|| n > KG_TAIL_PROBE_MAX_N)
	{
		return 0;
	}
	if (!intermediate_get_first_shrink_blocker(Ft, Gt, llen, FGlen, n,
		(uint32_t *)&idx, &ft_hi, &ft_lo, &gt_hi, &gt_lo))
	{
		return 0;
	}
	hi = FGlen - 1;
	lo = FGlen - 2;
	cur_f = kg_tail_pair_to_s64(ft_hi, ft_lo);
	cur_g = kg_tail_pair_to_s64(gt_hi, gt_lo);
	support_num = 0;
	for (j = 0; j < n; j ++) {
		if (hint_k[j] != 0 && support_num < KG_TAIL_PROBE_MAX_N) {
			support[support_num ++] = (uint32_t)j;
		}
	}
	if (support_num < 2) {
		return 0;
	}
	cur_blockers = intermediate_count_shrink_blockers(Ft, Gt, llen, FGlen, n);
	cur_excess = 0;
	for (j = 0; j < n; j ++) {
		uint64_t ef, eg;
		size_t off;

		off = j * llen;
		ef = kg_tail_pair_excess(Ft[off + hi], Ft[off + lo]);
		eg = kg_tail_pair_excess(Gt[off + hi], Gt[off + lo]);
		cur_excess += ef + eg;
	}
	best_blockers = cur_blockers;
	best_excess = cur_excess;
	best_j1 = 0;
	best_j2 = 0;
	best_k1 = 0;
	best_k2 = 0;
	found = 0;
	for (a = 0; a + 1 < support_num; a ++) {
		for (b = a + 1; b < support_num; b ++) {
			uint32_t f1_delta[KG_TAIL_BLOCKER_WORDS_MAX];
			uint32_t g1_delta[KG_TAIL_BLOCKER_WORDS_MAX];
			uint32_t f2_delta[KG_TAIL_BLOCKER_WORDS_MAX];
			uint32_t g2_delta[KG_TAIL_BLOCKER_WORDS_MAX];
			int32_t cand1[8], cand2[8];
			size_t cand1_num, cand2_num, u, v;
			size_t j1, j2;
			int32_t sign1, sign2;
			int64_t d1_f, d1_g, d2_f, d2_g;
			__int128 det;

			j1 = support[a];
			j2 = support[b];
			sign1 = (hint_k[j1] < 0) ? -1 : 1;
			sign2 = (hint_k[j2] < 0) ? -1 : 1;
			cand1_num = 0;
			cand2_num = 0;
			kg_tail_compute_scaled_coeff_update_one(f1_delta, FGlen,
				ft, flen, fstride, j1, 1, scale_k, logn, idx);
			kg_tail_compute_scaled_coeff_update_one(g1_delta, FGlen,
				gt, flen, fstride, j1, 1, scale_k, logn, idx);
			kg_tail_compute_scaled_coeff_update_one(f2_delta, FGlen,
				ft, flen, fstride, j2, 1, scale_k, logn, idx);
			kg_tail_compute_scaled_coeff_update_one(g2_delta, FGlen,
				gt, flen, fstride, j2, 1, scale_k, logn, idx);
			d1_f = kg_tail_pair_to_s64(f1_delta[hi], f1_delta[lo]);
			d1_g = kg_tail_pair_to_s64(g1_delta[hi], g1_delta[lo]);
			d2_f = kg_tail_pair_to_s64(f2_delta[hi], f2_delta[lo]);
			d2_g = kg_tail_pair_to_s64(g2_delta[hi], g2_delta[lo]);
			kg_tail_add_candidate(cand1, &cand1_num, sign1);
			kg_tail_add_candidate(cand1, &cand1_num, -sign1);
			kg_tail_add_candidate(cand2, &cand2_num, sign2);
			kg_tail_add_candidate(cand2, &cand2_num, -sign2);
			det = (__int128)d1_f * (__int128)d2_g
				- (__int128)d1_g * (__int128)d2_f;
			if (det != 0) {
				int32_t q1, q2;

				q1 = kg_tail_round_div128(
					-((__int128)cur_f * (__int128)d2_g)
					+ ((__int128)cur_g * (__int128)d2_f), det);
				q2 = kg_tail_round_div128(
					((__int128)d1_g * (__int128)cur_f)
					- ((__int128)d1_f * (__int128)cur_g), det);
				kg_tail_add_candidate(cand1, &cand1_num, q1);
				kg_tail_add_candidate(cand1, &cand1_num, q1 - sign1);
				kg_tail_add_candidate(cand1, &cand1_num, q1 + sign1);
				kg_tail_add_candidate(cand2, &cand2_num, q2);
				kg_tail_add_candidate(cand2, &cand2_num, q2 - sign2);
				kg_tail_add_candidate(cand2, &cand2_num, q2 + sign2);
			}
			if (d1_f != 0) {
				kg_tail_add_candidate(cand1, &cand1_num,
					kg_tail_round_div64(-cur_f, d1_f));
			}
			if (d1_g != 0) {
				kg_tail_add_candidate(cand1, &cand1_num,
					kg_tail_round_div64(-cur_g, d1_g));
			}
			if (d2_f != 0) {
				kg_tail_add_candidate(cand2, &cand2_num,
					kg_tail_round_div64(-cur_f, d2_f));
			}
			if (d2_g != 0) {
				kg_tail_add_candidate(cand2, &cand2_num,
					kg_tail_round_div64(-cur_g, d2_g));
			}
			for (u = 0; u < cand1_num; u ++) {
				for (v = 0; v < cand2_num; v ++) {
					uint32_t trial_blockers;
					uint64_t trial_excess;

					if (cand1[u] == 0 && cand2[v] == 0) {
						continue;
					}
					memset(k_tmp, 0, n * sizeof *k_tmp);
					k_tmp[j1] = cand1[u];
					k_tmp[j2] = cand2[v];
					kg_tail_eval_sparse_k_exact(&trial_blockers, &trial_excess,
						Ft, Gt, llen, FGlen,
						ft, gt, flen, fstride,
						k_tmp, scale_k, logn);
					if (trial_blockers < best_blockers
						|| (trial_blockers == best_blockers
							&& trial_excess < best_excess))
					{
						best_blockers = trial_blockers;
						best_excess = trial_excess;
						best_j1 = j1;
						best_j2 = j2;
						best_k1 = cand1[u];
						best_k2 = cand2[v];
						found = 1;
					}
				}
			}
		}
	}
	if (!found
		|| (best_blockers > cur_blockers)
		|| (best_blockers == cur_blockers && best_excess >= cur_excess))
	{
		return 0;
	}
	memset(k_out, 0, n * sizeof *k_out);
	k_out[best_j1] = best_k1;
	k_out[best_j2] = best_k2;
	if (gap_out != NULL
		&& !kg_tail_eval_blocker_gap_for_k(gap_out,
			Ft, Gt, llen, FGlen, (uint32_t)idx,
			ft, gt, flen, fstride, k_out, scale_k, logn))
	{
		*gap_out = 0;
	}
	return 1;
}

static inline int64_t
kg_tail_pair_to_s64(uint32_t hi, uint32_t lo)
{
	return ((int64_t)sign_ext(hi) << 31) + (int64_t)lo;
}

static inline uint64_t
kg_tail_pair_excess(uint32_t hi, uint32_t lo)
{
	int64_t z;
	const int64_t zmin = -((int64_t)1 << 30);
	const int64_t zmax = ((int64_t)1 << 30) - 1;

	z = kg_tail_pair_to_s64(hi, lo);
	if (z < zmin) {
		return (uint64_t)(zmin - z);
	}
	if (z > zmax) {
		return (uint64_t)(z - zmax);
	}
	return 0;
}

static int32_t
kg_tail_round_div64(int64_t num, int64_t den)
{
	uint64_t a, b, q;
	int neg;

	if (den == 0) {
		return 0;
	}
	neg = ((num < 0) ^ (den < 0));
	a = (uint64_t)(num < 0 ? -num : num);
	b = (uint64_t)(den < 0 ? -den : den);
	q = (a + (b >> 1)) / b;
	if (q > 0x7FFFFFFFu) {
		q = 0x7FFFFFFFu;
	}
	return neg ? -(int32_t)q : (int32_t)q;
}

static int32_t
kg_tail_round_div128(__int128 num, __int128 den)
{
	__int128 q;

	if (den == 0) {
		return 0;
	}
	if ((num < 0) != (den < 0)) {
		num = -num;
		q = -(((-num) + ((-den) >> 1)) / (-den));
	} else {
		if (num < 0) {
			num = -num;
			den = -den;
		}
		q = (num + (den >> 1)) / den;
	}
	if (q > (__int128)0x7FFFFFFF) {
		q = (__int128)0x7FFFFFFF;
	} else if (q < -(__int128)0x7FFFFFFF) {
		q = -(__int128)0x7FFFFFFF;
	}
	return (int32_t)q;
}

static void
kg_tail_add_candidate(int32_t *cand, size_t *num, int32_t v)
{
	size_t u;

	if (v == 0) {
		return;
	}
	for (u = 0; u < *num; u ++) {
		if (cand[u] == v) {
			return;
		}
	}
	if (*num < 8) {
		cand[*num] = v;
		(*num) ++;
	}
}

static void
kg_tail_eval_sparse_k(uint32_t *blockers, uint64_t *excess,
	const uint32_t *Ft, const uint32_t *Gt, size_t llen, size_t FGlen,
	const uint32_t *ft, const uint32_t *gt, size_t flen, size_t fstride,
	const int32_t *k, uint32_t scale_k, unsigned logn)
{
	uint32_t f_delta[KG_TAIL_BLOCKER_WORDS_MAX];
	uint32_t g_delta[KG_TAIL_BLOCKER_WORDS_MAX];
	uint32_t f_tmp[KG_TAIL_BLOCKER_WORDS_MAX];
	uint32_t g_tmp[KG_TAIL_BLOCKER_WORDS_MAX];
	size_t n, u, hi, lo;
	uint32_t bad;
	uint64_t acc;

	n = (size_t)1 << logn;
	hi = FGlen - 1;
	lo = FGlen - 2;
	bad = 0;
	acc = 0;
	for (u = 0; u < n; u ++) {
		uint64_t ef, eg;

		kg_tail_compute_scaled_coeff_update(f_delta, FGlen,
			ft, flen, fstride, k, scale_k, logn, u);
		kg_tail_compute_scaled_coeff_update(g_delta, FGlen,
			gt, flen, fstride, k, scale_k, logn, u);
		memcpy(f_tmp, Ft + u * llen, FGlen * sizeof *f_tmp);
		memcpy(g_tmp, Gt + u * llen, FGlen * sizeof *g_tmp);
		zint_add_scaled_mul_small(f_tmp, FGlen, f_delta, FGlen, 1, 0, 0);
		zint_add_scaled_mul_small(g_tmp, FGlen, g_delta, FGlen, 1, 0, 0);
		ef = kg_tail_pair_excess(f_tmp[hi], f_tmp[lo]);
		eg = kg_tail_pair_excess(g_tmp[hi], g_tmp[lo]);
		bad += (uint32_t)(ef != 0 || eg != 0);
		acc += ef + eg;
	}
	*blockers = bad;
	*excess = acc;
}

static void
kg_tail_eval_sparse_k_exact(uint32_t *blockers, uint64_t *excess,
	const uint32_t *Ft, const uint32_t *Gt, size_t llen, size_t FGlen,
	const uint32_t *ft, const uint32_t *gt, size_t flen, size_t fstride,
	const int32_t *k, uint32_t scale_k, unsigned logn)
{
	uint32_t f_tmp[KG_TAIL_PROBE_MAX_N * KG_TAIL_BLOCKER_WORDS_MAX];
	uint32_t g_tmp[KG_TAIL_PROBE_MAX_N * KG_TAIL_BLOCKER_WORDS_MAX];
	size_t n, u, hi, lo;
	uint32_t bad;
	uint64_t acc;

	n = (size_t)1 << logn;
	if (n == 0 || n > KG_TAIL_PROBE_MAX_N
		|| FGlen == 0 || FGlen > KG_TAIL_BLOCKER_WORDS_MAX)
	{
		*blockers = 0xFFFFFFFFu;
		*excess = (uint64_t)-1;
		return;
	}
	for (u = 0; u < n; u ++) {
		memcpy(f_tmp + u * FGlen, Ft + u * llen, FGlen * sizeof *f_tmp);
		memcpy(g_tmp + u * FGlen, Gt + u * llen, FGlen * sizeof *g_tmp);
	}
	poly_sub_scaled(f_tmp, FGlen, FGlen, ft, flen, fstride, k, scale_k, logn);
	poly_sub_scaled(g_tmp, FGlen, FGlen, gt, flen, fstride, k, scale_k, logn);
	hi = FGlen - 1;
	lo = FGlen - 2;
	bad = 0;
	acc = 0;
	for (u = 0; u < n; u ++) {
		uint64_t ef, eg;
		size_t off;

		off = u * FGlen;
		ef = kg_tail_pair_excess(f_tmp[off + hi], f_tmp[off + lo]);
		eg = kg_tail_pair_excess(g_tmp[off + hi], g_tmp[off + lo]);
		bad += (uint32_t)(ef != 0 || eg != 0);
		acc += ef + eg;
	}
	*blockers = bad;
	*excess = acc;
}

static void
kg_tail_eval_single_k(uint32_t *blockers, uint64_t *excess,
	const uint32_t *Ft, const uint32_t *Gt, size_t llen, size_t FGlen,
	const uint32_t *ft, const uint32_t *gt, size_t flen, size_t fstride,
	size_t k_index, int32_t k_value, uint32_t scale_k, unsigned logn)
{
	int32_t k_tmp[KG_TAIL_PROBE_MAX_N];
	size_t n;

	n = (size_t)1 << logn;
	memset(k_tmp, 0, n * sizeof *k_tmp);
	k_tmp[k_index] = k_value;
	kg_tail_eval_sparse_k(blockers, excess,
		Ft, Gt, llen, FGlen,
		ft, gt, flen, fstride,
		k_tmp, scale_k, logn);
}

static int
kg_tail_try_escape(uint32_t *Ft, uint32_t *Gt, size_t llen, size_t FGlen,
	const uint32_t *ft, const uint32_t *gt, size_t flen, size_t fstride,
	int32_t *k, uint32_t scale_k, unsigned logn)
{
	uint32_t f_delta[KG_TAIL_BLOCKER_WORDS_MAX];
	uint32_t g_delta[KG_TAIL_BLOCKER_WORDS_MAX];
	size_t n, hi, lo, idx, best_j, j;
	uint32_t best_blockers, cur_blockers;
	uint64_t cur_excess;
	int32_t best_kv;
	int found;
	uint32_t ft_hi, ft_lo, gt_hi, gt_lo;
	int64_t cur_f, cur_g;

	n = (size_t)1 << logn;
	if (FGlen < 2 || FGlen > KG_TAIL_BLOCKER_WORDS_MAX || n == 0 || n > 16) {
		return 0;
	}
	if (!intermediate_get_first_shrink_blocker(Ft, Gt, llen, FGlen, n,
		(uint32_t *)&idx, &ft_hi, &ft_lo, &gt_hi, &gt_lo))
	{
		return 0;
	}
	hi = FGlen - 1;
	lo = FGlen - 2;
	cur_f = kg_tail_pair_to_s64(ft_hi, ft_lo);
	cur_g = kg_tail_pair_to_s64(gt_hi, gt_lo);
	cur_blockers = intermediate_count_shrink_blockers(Ft, Gt, llen, FGlen, n);
	cur_excess = 0;
	for (j = 0; j < n; j ++) {
		uint64_t ef, eg;
		size_t off;

		off = j * llen;
		ef = kg_tail_pair_excess(Ft[off + hi], Ft[off + lo]);
		eg = kg_tail_pair_excess(Gt[off + hi], Gt[off + lo]);
		cur_excess += ef + eg;
	}
	best_blockers = cur_blockers;
	best_j = 0;
	best_kv = 0;
	found = 0;
	for (j = 0; j < n; j ++) {
		int32_t cand[8];
		size_t cand_num, v;
		int64_t d_f, d_g;

		cand_num = 0;
		kg_tail_compute_scaled_coeff_update_one(f_delta, FGlen,
			ft, flen, fstride, j, 1, scale_k, logn, idx);
		kg_tail_compute_scaled_coeff_update_one(g_delta, FGlen,
			gt, flen, fstride, j, 1, scale_k, logn, idx);
		d_f = kg_tail_pair_to_s64(f_delta[hi], f_delta[lo]);
		d_g = kg_tail_pair_to_s64(g_delta[hi], g_delta[lo]);
		kg_tail_add_candidate(cand, &cand_num, 1);
		kg_tail_add_candidate(cand, &cand_num, -1);
		if (d_f != 0) {
			int32_t qf;

			qf = kg_tail_round_div64(-cur_f, d_f);
			kg_tail_add_candidate(cand, &cand_num, qf);
			kg_tail_add_candidate(cand, &cand_num, qf - 1);
			kg_tail_add_candidate(cand, &cand_num, qf + 1);
		}
		if (d_g != 0) {
			int32_t qg;

			qg = kg_tail_round_div64(-cur_g, d_g);
			kg_tail_add_candidate(cand, &cand_num, qg);
			kg_tail_add_candidate(cand, &cand_num, qg - 1);
			kg_tail_add_candidate(cand, &cand_num, qg + 1);
		}
		for (v = 0; v < cand_num; v ++) {
			uint32_t trial_blockers;
			uint64_t trial_excess;

			kg_tail_eval_single_k(&trial_blockers, &trial_excess,
				Ft, Gt, llen, FGlen,
				ft, gt, flen, fstride,
				j, cand[v], scale_k, logn);
			if (trial_blockers < best_blockers)
			{
				best_blockers = trial_blockers;
				best_j = j;
				best_kv = cand[v];
				found = 1;
			}
		}
	}
	if (!found || best_blockers >= cur_blockers) {
		return 0;
	}
	memset(k, 0, n * sizeof *k);
	k[best_j] = best_kv;
	poly_sub_scaled(Ft, FGlen, llen, ft, flen, fstride, k, scale_k, logn);
	poly_sub_scaled(Gt, FGlen, llen, gt, flen, fstride, k, scale_k, logn);
	return 1;
}

static long double
kg_fnr_to_ld(fnr x)
{
	int64_t z;

	memcpy(&z, &x.v, sizeof z);
	return (long double)z / 4294967296.0L;
}

static int
kg_tail_compute_direct_quotient_snapshot(
	int32_t *k_out, uint32_t *diff_count, uint32_t *max_diff,
	const int32_t *k_ref,
	const uint32_t *Ft, const uint32_t *Gt, size_t llen, size_t FGlen,
	const uint32_t *ft, const uint32_t *gt, size_t current_flen, size_t rlen_cur,
	size_t fstride, uint32_t scale_x, uint32_t scale_t,
	uint32_t tlen, uint32_t toff, unsigned logn)
{
	fnr ff[KG_TAIL_PROBE_MAX_N], gg[KG_TAIL_PROBE_MAX_N];
	fnr FF[KG_TAIL_PROBE_MAX_N], GG[KG_TAIL_PROBE_MAX_N];
	long double f[KG_TAIL_PROBE_MAX_N], g[KG_TAIL_PROBE_MAX_N];
	long double F[KG_TAIL_PROBE_MAX_N], G[KG_TAIL_PROBE_MAX_N];
	long double qre[KG_TAIL_PROBE_MAX_N], qim[KG_TAIL_PROBE_MAX_N];
	size_t n, u, v;

	n = (size_t)1 << logn;
	if (n == 0 || n > 8 || n > KG_TAIL_PROBE_MAX_N
		|| rlen_cur == 0 || rlen_cur > current_flen
		|| tlen > FGlen)
	{
		return 0;
	}

	poly_big_to_fixed(ff,
		ft + current_flen - rlen_cur, rlen_cur, fstride,
		scale_x - scale_t, logn);
	poly_big_to_fixed(gg,
		gt + current_flen - rlen_cur, rlen_cur, fstride,
		scale_x - scale_t, logn);
	poly_big_to_fixed(FF, Ft + tlen, FGlen - tlen, llen,
		scale_x + toff, logn);
	poly_big_to_fixed(GG, Gt + tlen, FGlen - tlen, llen,
		scale_x + toff, logn);
	for (u = 0; u < n; u ++) {
		f[u] = kg_fnr_to_ld(ff[u]);
		g[u] = kg_fnr_to_ld(gg[u]);
		F[u] = kg_fnr_to_ld(FF[u]);
		G[u] = kg_fnr_to_ld(GG[u]);
	}
	for (u = 0; u < n; u ++) {
		long double theta;
		long double wr, wi;
		long double zr, zi;
		long double fr, fi, gr, gi, Fr, Fi, Gr, Gi;
		long double den, nr, ni;

		theta = ((long double)(2 * u + 1) * acosl(-1.0L)) / (long double)n;
		wr = cosl(theta);
		wi = sinl(theta);
		zr = 1.0L;
		zi = 0.0L;
		fr = fi = gr = gi = Fr = Fi = Gr = Gi = 0.0L;
		for (v = 0; v < n; v ++) {
			fr += f[v] * zr;
			fi += f[v] * zi;
			gr += g[v] * zr;
			gi += g[v] * zi;
			Fr += F[v] * zr;
			Fi += F[v] * zi;
			Gr += G[v] * zr;
			Gi += G[v] * zi;
			{
				long double tr, ti;

				tr = zr * wr - zi * wi;
				ti = zr * wi + zi * wr;
				zr = tr;
				zi = ti;
			}
		}
		den = fr * fr + fi * fi + gr * gr + gi * gi;
		if (den < 1.0e-24L) {
			return 0;
		}
		nr = Fr * fr + Fi * fi + Gr * gr + Gi * gi;
		ni = Fi * fr - Fr * fi + Gi * gr - Gr * gi;
		qre[u] = nr / den;
		qim[u] = ni / den;
	}
	for (u = 0; u < n; u ++) {
		long double acc_re, acc_im;
		int64_t zk;

		acc_re = 0.0L;
		acc_im = 0.0L;
		for (v = 0; v < n; v ++) {
			size_t j;
			long double theta;
			long double wr, wi;
			long double zr, zi;

			theta = ((long double)(2 * v + 1) * acosl(-1.0L))
				/ (long double)n;
			wr = cosl(theta);
			wi = -sinl(theta);
			zr = 1.0L;
			zi = 0.0L;
			for (j = 0; j < u; j ++) {
				long double tr, ti;

				tr = zr * wr - zi * wi;
				ti = zr * wi + zi * wr;
				zr = tr;
				zi = ti;
			}
			acc_re += qre[v] * zr - qim[v] * zi;
			acc_im += qre[v] * zi + qim[v] * zr;
			(void)acc_im;
		}
		zk = (int64_t)llroundl(acc_re / (long double)n);
		if (zk > 0x7FFFFFFFLL) {
			zk = 0x7FFFFFFFLL;
		} else if (zk < -0x7FFFFFFFLL) {
			zk = -0x7FFFFFFFLL;
		}
		k_out[u] = (int32_t)zk;
	}
	if (diff_count != NULL || max_diff != NULL) {
		uint32_t dc, md;

		dc = 0;
		md = 0;
		for (u = 0; u < n; u ++) {
			uint32_t ad;
			int64_t dz;

			dz = (int64_t)k_out[u] - (int64_t)k_ref[u];
			ad = (uint32_t)(dz < 0 ? -dz : dz);
			dc += (uint32_t)(ad != 0);
			if (ad > md) {
				md = ad;
			}
		}
		if (diff_count != NULL) {
			*diff_count = dc;
		}
		if (max_diff != NULL) {
			*max_diff = md;
		}
	}
	return 1;
}

static void
kg_record_tail_exit(ctl_tail_exit_diag *dst, uint32_t reason, uint32_t iter,
	size_t slen, size_t sstride, size_t llen, size_t FGlen, size_t rlen_cur,
	uint32_t scale_fg, uint32_t scale_FG, uint32_t scale_FG_eff,
	uint32_t scale_k, uint32_t scale_x, uint32_t scale_t,
	uint32_t reduce_bits, uint32_t tlen, uint32_t toff,
	uint32_t k_nonzero_count, uint32_t top_word_changed,
	uint32_t top_word_change_count, uint32_t top_word_change_first_index,
	uint32_t top_word_change_ft_pre, uint32_t top_word_change_ft_post,
	uint32_t top_word_change_gt_pre, uint32_t top_word_change_gt_post,
	uint32_t probe_attempted, uint32_t probe_selected,
	uint32_t probe_flen, uint32_t probe_scale_fg,
	uint32_t probe_scale_k, uint32_t probe_scale_x, uint32_t probe_scale_t,
	uint32_t probe_tlen, uint32_t probe_toff, uint32_t probe_k_nonzero,
	uint64_t cur_rt3_raw, uint64_t cur_rt4_raw, uint64_t cur_rt5_raw,
	uint64_t probe_rt3_raw, uint64_t probe_rt4_raw, uint64_t probe_rt5_raw,
	size_t probe_snap_n, const int32_t *probe_k_snapshot,
	const uint64_t *probe_rt2_raw_snapshot,
	uint64_t probe_cur_gap, uint64_t probe_alt_gap,
	uint32_t other_strong_progress,
	uint32_t other_direct_modp_ok,
	uint32_t other_exact_ok, uint32_t other_exact_modp_ok,
	uint32_t cur_exact_ok, uint32_t cur_exact_modp_ok,
	uint32_t probe_exact_ok, uint32_t probe_exact_modp_ok,
	uint64_t other_exact_gap_post,
	uint64_t cur_exact_gap_post,
	uint64_t probe_exact_gap_post,
	uint32_t other_exact_bad_index, uint32_t other_exact_expected,
	uint32_t other_exact_actual,
	uint32_t cur_exact_bad_index, uint32_t cur_exact_expected,
	uint32_t cur_exact_actual,
	uint32_t probe_exact_bad_index, uint32_t probe_exact_expected,
	uint32_t probe_exact_actual,
	uint32_t tail_no_update,
	const uint32_t *Ft, const uint32_t *Gt, size_t n,
	const uint32_t *ft, const uint32_t *gt,
	size_t current_flen, size_t other_flen, size_t fstride,
	const int32_t *k,
	const uint64_t *rt2_raw_snapshot, const int32_t *rt2_round_snapshot,
	size_t snap_n, unsigned logn)
{
	uint32_t bad_index, ft_hi, ft_lo, gt_hi, gt_lo;

	memset(dst, 0, sizeof *dst);
	dst->valid = 1;
	dst->reason = reason;
	dst->iter = iter;
	dst->slen = (uint32_t)slen;
	dst->sstride = (uint32_t)sstride;
	dst->llen = (uint32_t)llen;
	dst->fglen = (uint32_t)FGlen;
	dst->rlen_cur = (uint32_t)rlen_cur;
	dst->scale_fg = scale_fg;
	dst->scale_FG = scale_FG;
	dst->scale_FG_eff = scale_FG_eff;
	dst->scale_k = scale_k;
	dst->scale_x = scale_x;
	dst->scale_t = scale_t;
	dst->reduce_bits = reduce_bits;
	dst->tlen = tlen;
	dst->toff = toff;
	dst->k_nonzero_count = k_nonzero_count;
	dst->top_word_changed = top_word_changed;
	dst->top_word_change_count = top_word_change_count;
	dst->top_word_change_first_index = top_word_change_first_index;
	dst->top_word_change_ft_pre = top_word_change_ft_pre;
	dst->top_word_change_ft_post = top_word_change_ft_post;
	dst->top_word_change_gt_pre = top_word_change_gt_pre;
	dst->top_word_change_gt_post = top_word_change_gt_post;
	dst->probe_attempted = probe_attempted;
	dst->probe_selected = probe_selected;
	dst->probe_flen = probe_flen;
	dst->probe_scale_fg = probe_scale_fg;
	dst->probe_scale_k = probe_scale_k;
	dst->probe_scale_x = probe_scale_x;
	dst->probe_scale_t = probe_scale_t;
	dst->probe_tlen = probe_tlen;
	dst->probe_toff = probe_toff;
	dst->cur_rt3_lo = (uint32_t)cur_rt3_raw;
	dst->cur_rt3_hi = (uint32_t)(cur_rt3_raw >> 32);
	dst->cur_rt4_lo = (uint32_t)cur_rt4_raw;
	dst->cur_rt4_hi = (uint32_t)(cur_rt4_raw >> 32);
	dst->cur_rt5_lo = (uint32_t)cur_rt5_raw;
	dst->cur_rt5_hi = (uint32_t)(cur_rt5_raw >> 32);
	dst->probe_rt3_lo = (uint32_t)probe_rt3_raw;
	dst->probe_rt3_hi = (uint32_t)(probe_rt3_raw >> 32);
	dst->probe_rt4_lo = (uint32_t)probe_rt4_raw;
	dst->probe_rt4_hi = (uint32_t)(probe_rt4_raw >> 32);
	dst->probe_rt5_lo = (uint32_t)probe_rt5_raw;
	dst->probe_rt5_hi = (uint32_t)(probe_rt5_raw >> 32);
	dst->probe_k_nonzero = probe_k_nonzero;
	dst->probe_cur_gap = probe_cur_gap;
	dst->probe_alt_gap = probe_alt_gap;
	dst->other_strong_progress = other_strong_progress;
	dst->other_direct_modp_ok = other_direct_modp_ok;
	dst->other_exact_ok = other_exact_ok;
	dst->other_exact_modp_ok = other_exact_modp_ok;
	dst->cur_exact_ok = cur_exact_ok;
	dst->cur_exact_modp_ok = cur_exact_modp_ok;
	dst->probe_exact_ok = probe_exact_ok;
	dst->probe_exact_modp_ok = probe_exact_modp_ok;
	dst->other_exact_gap_post = other_exact_gap_post;
	dst->cur_exact_gap_post = cur_exact_gap_post;
	dst->probe_exact_gap_post = probe_exact_gap_post;
	dst->other_exact_bad_index = other_exact_bad_index;
	dst->other_exact_expected = other_exact_expected;
	dst->other_exact_actual = other_exact_actual;
	dst->cur_exact_bad_index = cur_exact_bad_index;
	dst->cur_exact_expected = cur_exact_expected;
	dst->cur_exact_actual = cur_exact_actual;
	dst->probe_exact_bad_index = probe_exact_bad_index;
	dst->probe_exact_expected = probe_exact_expected;
	dst->probe_exact_actual = probe_exact_actual;
	dst->probe_snap_n = (uint32_t)((probe_snap_n > 8) ? 8 : probe_snap_n);
	for (bad_index = 0; bad_index < dst->probe_snap_n; bad_index ++) {
		dst->probe_k_snapshot[bad_index] =
			probe_k_snapshot != NULL ? probe_k_snapshot[bad_index] : 0;
		if (probe_rt2_raw_snapshot != NULL) {
			dst->probe_rt2_lo[bad_index] =
				(uint32_t)probe_rt2_raw_snapshot[bad_index];
			dst->probe_rt2_hi[bad_index] =
				(uint32_t)(probe_rt2_raw_snapshot[bad_index] >> 32);
		}
	}
	dst->tail_no_update = tail_no_update;
	kg_tail_capture_round_snapshot(dst, k, rt2_round_snapshot, snap_n);
	dst->direct_valid = kg_tail_compute_direct_quotient_snapshot(
		dst->direct_k_snapshot,
		&dst->direct_diff_count, &dst->direct_max_diff,
		k,
		Ft, Gt, llen, FGlen,
		ft, gt, current_flen, rlen_cur,
		fstride, scale_x, scale_t, tlen, toff, logn) ? 1u : 0u;
	dst->direct_snap_n = dst->direct_valid
		? (uint32_t)(((size_t)1 << logn) > 8 ? 8 : ((size_t)1 << logn))
		: 0u;
	dst->can_shrink = intermediate_can_shrink_fglen(Ft, Gt, llen, FGlen, n);
	dst->blocker_count = intermediate_count_shrink_blockers(Ft, Gt, llen, FGlen, n);
	bad_index = 0;
	ft_hi = 0;
	ft_lo = 0;
	gt_hi = 0;
	gt_lo = 0;
	if (intermediate_get_first_shrink_blocker(
		Ft, Gt, llen, FGlen, n, &bad_index, &ft_hi, &ft_lo, &gt_hi, &gt_lo))
	{
		dst->blocker_index = bad_index;
		dst->ft_hi = ft_hi;
		dst->ft_lo = ft_lo;
		dst->gt_hi = gt_hi;
		dst->gt_lo = gt_lo;
		kg_tail_capture_blocker_local(dst,
			Ft, Gt, llen, FGlen,
			ft, gt, current_flen, fstride, k,
			rt2_raw_snapshot, rt2_round_snapshot, snap_n, scale_k, logn);
		kg_tail_capture_blocker_other_window(dst,
			Ft, Gt, llen, FGlen,
			ft, gt, fstride,
			current_flen, other_flen, rlen_cur,
			scale_fg, scale_FG_eff, k, logn);
	}
}

/*
 * Solving the NTRU equation, intermediate level. Upon entry, the F and G
 * from the previous level should be in the tmp[] array.
 * This function MAY be invoked for the top-level (in which case depth = 0).
 *
 * Returned value: 1 on success, 0 on error.
 */
static int
solve_NTRU_intermediate(unsigned logn_top,
	const int8_t *f, const int8_t *g, uint32_t q,
	unsigned depth, uint32_t *tmp)
{
	/*
	 * In this function, 'logn' is the log2 of the degree for
	 * this step. If N = 2^logn, then:
	 *  - the F and G values already in fk->tmp (from the deeper
	 *    levels) have degree N/2;
	 *  - this function should return F and G of degree N.
	 */
	unsigned logn;
	size_t n, hn, slen, dlen, llen, rlen, FGlen, sstride, babai_slen, u;
	size_t rlen_base, rlen_cur, fg_tail_flen;
	uint32_t *Fd, *Gd, *Ft, *Gt, *ft, *gt, *gm, *igm, *t1;
	fnr *rt1, *rt2, *rt3, *rt4, *rt5;
	uint64_t rt2_raw_snapshot[8];
	int32_t rt2_round_snapshot[8];
	size_t rt2_snap_n;
	uint32_t p, p0i, R2, Rx;
	uint32_t scale_fg, scale_FG, scale_k;
	uint32_t current_scale_k;
	uint32_t scale_xf, scale_xg, scale_x, scale_t;
	uint32_t reduce_bits;
	uint32_t iter_count;
	int need_rebuild;
	uint32_t tail_no_update;
	uint32_t tail_no_topchange;
	uint32_t tail_resync_count;
	uint32_t tail_iter_limit;
	uint32_t tail_no_update_limit;
	uint32_t tail_no_topchange_limit;
	uint32_t *x, *y;
	int32_t *k;
	const ctl_solve_profile *prof;

	logn = logn_top - depth;
	n = (size_t)1 << logn;
	hn = n >> 1;
	iter_count = 0;
	need_rebuild = 1;
	current_scale_k = 0xFFFFFFFFu;
	tail_no_update = 0;
	tail_no_topchange = 0;
	tail_resync_count = 0;
	tail_iter_limit = 65536;
	tail_no_update_limit = 1024;
	tail_no_topchange_limit = 512;

	/*
	 * slen = size for our input f and g; also size of the reduced
	 *        F and G we return (degree N)
	 *
	 * dlen = size of the F and G obtained from the deeper level
	 *        (degree N/2)
	 *
	 * llen = size for intermediary F and G before reduction (degree N)
	 *
	 * We build our non-reduced F and G as two independent halves each,
	 * of degree N/2 (F = F0 + X*F1, G = G0 + X*G1).
	 */
	prof = ctl_get_solve_profile(q);
	if (prof == NULL || logn_top > prof->max_logn
		|| (size_t)(depth + 1) >= prof->max_bl_small_len
		|| (size_t)depth >= prof->max_bl_large_len)
	{
		return 0;
	}
	slen = prof->max_bl_small[depth];
	dlen = prof->max_bl_small[depth + 1];
	llen = prof->max_bl_large[depth];
	if (dlen > llen) {
		kg_diag_stats.solve_fg_fail_intermediate_bad_param ++;
		return 0;
	}
	/*
	 * Fd and Gd are the F and G from the deeper level.
	 */
	Fd = tmp;
	Gd = Fd + dlen * hn;

	/*
	 * Compute the input f and g for this level. Note that we get f
	 * and g in RNS + NTT representation.
	 */
	ft = Gd + dlen * hn;
	if (!make_fg(ft, f, g, q, logn_top, depth)) {
		kg_diag_stats.solve_fg_fail_intermediate_make_fg ++;
		return 0;
	}

	/*
	 * Move the newly computed f and g to make room for our candidate
	 * F and G (unreduced).
	 */
	Ft = tmp;
	Gt = Ft + n * llen;
	t1 = Gt + n * llen;
	memmove(t1, ft, 2 * n * slen * sizeof *ft);
	ft = t1;
	gt = ft + slen * n;
	t1 = gt + slen * n;

	/*
	 * Move Fd and Gd _after_ f and g.
	 */
	memmove(t1, Fd, 2 * hn * dlen * sizeof *Fd);
	Fd = t1;
	Gd = Fd + hn * dlen;

	/*
	 * We reduce Fd and Gd modulo all the small primes we will need,
	 * and store the values in Ft and Gt (only n/2 values in each).
	 */
	for (u = 0; u < llen; u ++) {
		size_t v;
		uint32_t *xs, *ys, *xd, *yd;

		p = PRIMES[u].p;
		p0i = modp_ninv31(p);
		R2 = modp_R2(p, p0i);
		Rx = modp_Rx((unsigned)dlen, p, p0i, R2);
		for (v = 0, xs = Fd, ys = Gd, xd = Ft + u, yd = Gt + u;
			v < hn;
			v ++, xs += dlen, ys += dlen, xd += llen, yd += llen)
		{
			*xd = zint_mod_small_signed(xs, dlen, p, p0i, R2, Rx);
			*yd = zint_mod_small_signed(ys, dlen, p, p0i, R2, Rx);
		}
	}

	/*
	 * We do not need Fd and Gd after that point.
	 */

	/*
	 * Compute our F and G modulo sufficiently many small primes.
	 */
	for (u = 0; u < llen; u ++) {
		uint32_t *fx, *gx, *Fp, *Gp;
		size_t v;

		/*
		 * All computations are done modulo p.
		 */
		p = PRIMES[u].p;
		p0i = modp_ninv31(p);
		R2 = modp_R2(p, p0i);

		/*
		 * If we processed slen words, then f and g have been
		 * de-NTTized, and are in RNS; we can rebuild them.
		 */
		if (u == slen) {
			zint_rebuild_CRT(ft, slen, slen, n, PRIMES, 1, t1);
			zint_rebuild_CRT(gt, slen, slen, n, PRIMES, 1, t1);
		}

		gm = t1;
		igm = gm + n;
		fx = igm + n;
		gx = fx + n;

		modp_mkgm(gm, igm, logn, PRIMES[u].g, p, p0i);

		if (u < slen) {
			for (v = 0, x = ft + u, y = gt + u;
				v < n; v ++, x += slen, y += slen)
			{
				fx[v] = *x;
				gx[v] = *y;
			}
			modp_iNTT_ext(ft + u, slen, igm, logn, p, p0i);
			modp_iNTT_ext(gt + u, slen, igm, logn, p, p0i);
		} else {
			Rx = modp_Rx((unsigned)slen, p, p0i, R2);
			for (v = 0, x = ft, y = gt;
				v < n; v ++, x += slen, y += slen)
			{
				fx[v] = zint_mod_small_signed(x, slen,
					p, p0i, R2, Rx);
				gx[v] = zint_mod_small_signed(y, slen,
					p, p0i, R2, Rx);
			}
			modp_NTT(fx, gm, logn, p, p0i);
			modp_NTT(gx, gm, logn, p, p0i);
		}

		/*
		 * Get F' and G' modulo p and in NTT representation
		 * (they have degree n/2). These values were computed in
		 * a previous step, and stored in Ft and Gt.
		 */
		Fp = gx + n;
		Gp = Fp + hn;
		for (v = 0, x = Ft + u, y = Gt + u;
			v < hn; v ++, x += llen, y += llen)
		{
			Fp[v] = *x;
			Gp[v] = *y;
		}
		modp_NTT(Fp, gm, logn - 1, p, p0i);
		modp_NTT(Gp, gm, logn - 1, p, p0i);

		/*
		 * Compute our F and G modulo p.
		 *
		 * General case:
		 *
		 *   we divide degree by d = 2
		 *   f'(x^d) = N(f)(x^d) = f * adj(f)
		 *   g'(x^d) = N(g)(x^d) = g * adj(g)
		 *   f'*G' - g'*F' = q
		 *   F = F'(x^d) * adj(g)
		 *   G = G'(x^d) * adj(f)
		 *
		 * We compute things in the NTT. We group roots of phi
		 * such that all roots x in a group share the same x^d.
		 * If the roots in a group are x_1, x_2... x_d, then:
		 *
		 *   N(f)(x_1^d) = f(x_1)*f(x_2)*...*f(x_d)
		 *
		 * Thus, we have:
		 *
		 *   G(x_1) = f(x_2)*f(x_3)*...*f(x_d)*G'(x_1^d)
		 *   G(x_2) = f(x_1)*f(x_3)*...*f(x_d)*G'(x_1^d)
		 *   ...
		 *   G(x_d) = f(x_1)*f(x_2)*...*f(x_{d-1})*G'(x_1^d)
		 *
		 * In all cases, we can thus compute F and G in NTT
		 * representation by a few simple multiplications.
		 * Moreover, in our chosen NTT representation, roots
		 * from the same group are consecutive in RAM.
		 */
		for (v = 0, x = Ft + u, y = Gt + u; v < hn;
			v ++, x += (llen << 1), y += (llen << 1))
		{
			uint32_t ftA, ftB, gtA, gtB;
			uint32_t mFp, mGp;

			ftA = fx[(v << 1) + 0];
			ftB = fx[(v << 1) + 1];
			gtA = gx[(v << 1) + 0];
			gtB = gx[(v << 1) + 1];
			mFp = modp_montymul(Fp[v], R2, p, p0i);
			mGp = modp_montymul(Gp[v], R2, p, p0i);
			x[0] = modp_montymul(gtB, mFp, p, p0i);
			x[llen] = modp_montymul(gtA, mFp, p, p0i);
			y[0] = modp_montymul(ftB, mGp, p, p0i);
			y[llen] = modp_montymul(ftA, mGp, p, p0i);
		}
		modp_iNTT_ext(Ft + u, llen, igm, logn, p, p0i);
		modp_iNTT_ext(Gt + u, llen, igm, logn, p, p0i);
	}

	/*
	 * If slen == llen, then f and g have not been converted back
	 * from RNS to CRT, so we do it now.
	 */
	if (slen == llen) {
		zint_rebuild_CRT(ft, slen, slen, n, PRIMES, 1, t1);
		zint_rebuild_CRT(gt, slen, slen, n, PRIMES, 1, t1);
	}
	sstride = slen;

	/*
	 * Rebuild F and G with the CRT, then trim the active lengths of
	 * (f,g) and (F,G) to their actual sign-extended sizes. The profile
	 * values are upper bounds; using them directly here would distort
	 * the Babai scaling chain when those bounds are loose.
	 */
	zint_rebuild_CRT(Ft, llen, llen, n, PRIMES, 1, t1);
	zint_rebuild_CRT(Gt, llen, llen, n, PRIMES, 1, t1);
	if (depth < (sizeof kg_obs_small_bits / sizeof kg_obs_small_bits[0])) {
		uint32_t sbf, sbg, lbf, lbg, sb, lb;

		sbf = poly_max_bitlength(ft, sstride, sstride, logn);
		sbg = poly_max_bitlength(gt, sstride, sstride, logn);
		sb = sbf ^ (-((sbf - sbg) >> 31) & (sbf ^ sbg));
		slen = (size_t)((sb + 30u) / 31u) + 2;
		if (slen == 0) {
			slen = 1;
		}
		if (slen > sstride) {
			slen = sstride;
		}
		if (sb > kg_obs_small_bits[depth]) {
			kg_obs_small_bits[depth] = sb;
		}

		lbf = poly_max_bitlength(Ft, llen, llen, logn);
		lbg = poly_max_bitlength(Gt, llen, llen, logn);
		lb = lbf ^ (-((lbf - lbg) >> 31) & (lbf ^ lbg));
		FGlen = (size_t)((lb + 30u) / 31u) + 4;
		if (FGlen == 0) {
			FGlen = 1;
		}
		if (FGlen > llen) {
			FGlen = llen;
		}
		if (FGlen < slen) {
			FGlen = slen;
		}
		if (lb > kg_obs_large_bits[depth]) {
			kg_obs_large_bits[depth] = lb;
		}
	} else {
		uint32_t sbf, sbg, lbf, lbg, sb, lb;

		sbf = poly_max_bitlength(ft, sstride, sstride, logn);
		sbg = poly_max_bitlength(gt, sstride, sstride, logn);
		sb = sbf ^ (-((sbf - sbg) >> 31) & (sbf ^ sbg));
		slen = (size_t)((sb + 30u) / 31u) + 2;
		if (slen == 0) {
			slen = 1;
		}
		if (slen > sstride) {
			slen = sstride;
		}

		lbf = poly_max_bitlength(Ft, llen, llen, logn);
		lbg = poly_max_bitlength(Gt, llen, llen, logn);
		lb = lbf ^ (-((lbf - lbg) >> 31) & (lbf ^ lbg));
		FGlen = (size_t)((lb + 30u) / 31u) + 4;
		if (FGlen == 0) {
			FGlen = 1;
		}
		if (FGlen > llen) {
			FGlen = llen;
		}
		if (FGlen < slen) {
			FGlen = slen;
		}
	}

	/*
	 * Sanity check right after lifting, before Babai reduction.
	 */
	{
		uint32_t bad_index;
		if (!intermediate_check_modp(ft, gt, sstride, sstride,
			Ft, Gt, llen, llen,
			q, logn, t1, &bad_index))
		{
			kg_diag_stats.solve_fg_fail_intermediate_check_pre ++;
			kg_diag_stats.solve_fg_fail_intermediate_check ++;
			kg_last_inter_depth = depth;
			kg_last_inter_stage = 1;
			kg_last_inter_index = bad_index;
			kg_last_inter_slen = (uint32_t)slen;
			kg_last_inter_fglen = (uint32_t)llen;
			return 0;
		}
	}

	/*
	 * At that point, Ft, Gt, ft and gt are consecutive in RAM (in that
	 * order).
	 */

	/*
	 * Apply Babai reduction to bring back F and G to size slen.
	 *
	 * We use the FFT to compute successive approximations of the
	 * reduction coefficient. We first isolate the top bits of the
	 * coefficients of f and g, and convert them to fixed point;
	 * with the FFT, we compute adj(f), adj(g), and
	 * 1/(f*adj(f)+g*adj(g)).
	 *
	 * Then, we repeatedly apply the following:
	 *
	 *   - Get the top bits of the coefficients of F and G into
	 *     floating point, and use the FFT to compute:
	 *        (F*adj(f)+G*adj(g))/(f*adj(f)+g*adj(g))
	 *
	 *   - Convert back that value into normal representation, and
	 *     round it to the nearest integers, yielding a polynomial k.
	 *     Proper scaling is applied to f, g, F and G so that the
	 *     coefficients fit on 32 bits (signed).
	 *
	 *   - Subtract k*f from F and k*g from G.
	 *
	 * Under normal conditions, this process reduces the size of F
	 * and G by some bits at each iteration. For constant-time
	 * operation, we do not want to measure the actual length of
	 * F and G; instead, we do the following:
	 *
	 *   - f and g are converted to fixed-point approximation; the
	 *     exact scaling factor is secret. In fact, the scaling
	 *     factor is scale_fg + scale_x, with scale_fg being public
	 *     (and fixed for a given recursion depth) and scale_x
	 *     being secret.
	 *
	 *   - For each iteration, we _assume_ a maximum size for F and G,
	 *     and use the values at that scaling factor. Specifically, the
	 *     scaling factor of F and G is scale_FG + scale_x, with scale_FG
	 *     being a fixed public value, and scale_x the same value as
	 *     for (f,g). In particular, the _difference_ between the
	 *     scaling factors of (F,G) and of (f,g) is public.
	 *
	 *   - If we overreach, then we get zeros, which is harmless: the'
	 *     resulting coefficients of k will be 0 and the value won't be
	 *     reduced.
	 *
	 *   - We conservatively assume that F and G will be reduced by
	 *     at least 12 bits at each iteration; that is, we decrease
	 *     scale_FG by 12. When scale_FG reaches scale_fg, (F,G) are
	 *     supposed to be fully reduced, since they should now have
	 *     about the same size as (f,g).
	 */

	/*
	 * Memory layout:
	 *  - We need to compute and keep adj(f), adj(g), and
	 *    1/(f*adj(f)+g*adj(g)) (sizes N, N and N/2 fp numbers,
	 *    respectively).
	 *  - At each iteration we need two extra fp buffer (N fp values),
	 *    and produce a k (N 32-bit words). k will be shared with one
	 *    of the fp buffers.
	 *  - To compute k*f and k*g efficiently (with the NTT), we need
	 *    some extra room; we reuse the space of the temporary buffers.
	 *
	 * Arrays of 'fnr' are obtained from the temporary array itself.
	 * We ensure that the base is at a properly aligned offset (the
	 * source array tmp[] is supposed to be already aligned).
	 */

	rt3 = align_fnr(tmp, t1);
	rt4 = rt3 + n;
	rt5 = rt4 + n;
	rt1 = rt5 + (n >> 1);
	k = (int32_t *)align_u32(tmp, rt1);
	rt2 = align_fnr(tmp, k + n);
	if (rt2 < (rt1 + n)) {
		rt2 = rt1 + n;
	}
	t1 = (uint32_t *)k + n;

	/*
	 * Scaling:
	 *   scale_fg   base scale value (in bits) for (f,g)
	 *   scale_FG   base scale value (in bits) for (F,G)
	 *   scale_x    secret scale (in bits) within the windows
	 * When computing fixed-point approximations of the polynomials,
	 * (f,g) use scale_fg+scale_sec, and (F,G) use scale_FG+scale_sec.
	 * The difference is scale_FG-scale_fg, which is public and
	 * independent of the values.
	 *
	 * We use a window of some words; how many words we use depends
	 * on the depth (we want to cover 12 times the measured standard
	 * deviation so that the window will be good enough most of the
	 * time).
	 */
	babai_slen = sstride;
	rlen = prof->word_win[
		depth < prof->word_win_len
		? depth
		: (prof->word_win_len - 1)
	];
	if (prof->force_full_window_from_depth != PROFILE_DEPTH_NONE
		&& depth >= prof->force_full_window_from_depth)
	{
		rlen = babai_slen;
	}
	if (rlen > babai_slen) {
		rlen = babai_slen;
	}
	rlen_base = rlen;
	scale_fg = 31 * (int)(babai_slen - rlen_base);
	fg_tail_flen = intermediate_compact_len_pair(
		ft, gt, sstride, babai_slen, n);
	reduce_bits = prof->reduce_bits;
	if (prof->reduce_bits_alt_from_depth != PROFILE_DEPTH_NONE
		&& depth >= prof->reduce_bits_alt_from_depth)
	{
		reduce_bits = prof->reduce_bits_alt_value;
	}

	scale_x = 0;
	scale_t = 0;

	/*
	 * Reduce F and G repeatedly.
	 *
	 * F and G coefficients have size llen words at this point; they
	 * will be reduced below. Since (f,g) fit in arrays of size slen,
	 * scale_fg + scale_x is necessarily lower than 31*slen:
	 *    0 <= scale_x < 31*slen - scale_fg
	 * Thus, since each iteration brings (F,G) to size about
	 * scale_FG + scale_x (we add 10 extra bits here as margin), we
	 * use the size limit:
	 *     scale_FG + scale_x < scale_FG - scale_fg + 31*slen
	 * We keep the value in FGlen, updated when scale_FG is updated.
	 */
	scale_FG = 31 * (int)FGlen;
	if (q == 3329 && depth == 9) {
		if (kg_d9_fglen_min == 0 || (uint32_t)FGlen < kg_d9_fglen_min) {
			kg_d9_fglen_min = (uint32_t)FGlen;
		}
	}

	for (;;) {
		uint32_t tlen, toff;
		int had_update;
		uint32_t k_nz_iter;
		uint64_t k_abs_sum_iter;
		uint32_t k_abs_max_iter;
		uint32_t scale_FG_eff;
		size_t FGlen_before_iter;
		size_t update_flen;
		size_t apply_flen;
		uint32_t apply_scale_k;
		uint32_t top_word_changed_iter;
		uint32_t top_word_change_count_iter;
		uint32_t top_word_change_first_index_iter;
		uint32_t top_word_change_ft_pre_iter;
		uint32_t top_word_change_ft_post_iter;
		uint32_t top_word_change_gt_pre_iter;
		uint32_t top_word_change_gt_post_iter;
		uint32_t blocker_count_pre_iter;
		uint32_t blocker_count_post_iter;
		uint64_t blocker_gap_pre_iter;
		uint64_t blocker_gap_post_iter;
		uint32_t blocker_progress_iter;
		uint32_t probe_attempted_iter;
		uint32_t probe_selected_iter;
		uint32_t probe_flen_iter;
		uint32_t probe_scale_fg_iter;
		uint32_t probe_scale_k_iter;
		uint32_t probe_scale_x_iter;
		uint32_t probe_scale_t_iter;
		uint32_t probe_tlen_iter;
		uint32_t probe_toff_iter;
		uint64_t cur_rt3_raw_iter;
		uint64_t cur_rt4_raw_iter;
		uint64_t cur_rt5_raw_iter;
		uint64_t probe_rt3_raw_iter;
		uint64_t probe_rt4_raw_iter;
		uint64_t probe_rt5_raw_iter;
		uint32_t probe_k_nonzero_iter;
		int32_t probe_k_snapshot_iter[8];
		uint64_t probe_rt2_raw_snapshot_iter[8];
		size_t probe_snap_n_iter;
		uint64_t probe_cur_gap_iter;
		uint64_t probe_alt_gap_iter;
		uint32_t other_strong_progress_iter;
		uint32_t other_direct_modp_ok_iter;
		uint32_t other_exact_ok_iter;
		uint32_t other_exact_modp_ok_iter;
		uint32_t cur_exact_ok_iter;
		uint32_t cur_exact_modp_ok_iter;
		uint32_t probe_exact_ok_iter;
		uint32_t probe_exact_modp_ok_iter;
		uint32_t other_exact_bad_index_iter;
		uint32_t other_exact_expected_iter;
		uint32_t other_exact_actual_iter;
		uint32_t cur_exact_bad_index_iter;
		uint32_t cur_exact_expected_iter;
		uint32_t cur_exact_actual_iter;
		uint32_t probe_exact_bad_index_iter;
		uint32_t probe_exact_expected_iter;
		uint32_t probe_exact_actual_iter;
		uint64_t other_exact_gap_iter;
		uint64_t cur_exact_gap_iter;
		uint64_t probe_exact_gap_iter;
		int stall_counted;
		int nominal_tail_iter;
		int effective_tail_iter;
		int pseudo_tail_iter;
		int tail_stage_iter;
		int can_shrink_iter;
		int effective_update_iter;
		int force_modp_guard_iter;
		int32_t k_escape[16];

		FGlen_before_iter = FGlen;
		top_word_changed_iter = 0;
		top_word_change_count_iter = 0;
		top_word_change_first_index_iter = 0;
		top_word_change_ft_pre_iter = 0;
		top_word_change_ft_post_iter = 0;
		top_word_change_gt_pre_iter = 0;
		top_word_change_gt_post_iter = 0;
		blocker_count_pre_iter = 0;
		blocker_count_post_iter = 0;
		blocker_gap_pre_iter = 0;
		blocker_gap_post_iter = 0;
		blocker_progress_iter = 0;
		probe_attempted_iter = 0;
		probe_selected_iter = 0;
		probe_flen_iter = 0;
		probe_scale_fg_iter = 0;
		probe_scale_k_iter = 0;
		probe_scale_x_iter = 0;
		probe_scale_t_iter = 0;
		probe_tlen_iter = 0;
		probe_toff_iter = 0;
		cur_rt3_raw_iter = 0;
		cur_rt4_raw_iter = 0;
		cur_rt5_raw_iter = 0;
		probe_rt3_raw_iter = 0;
		probe_rt4_raw_iter = 0;
		probe_rt5_raw_iter = 0;
		probe_k_nonzero_iter = 0;
		memset(probe_k_snapshot_iter, 0, sizeof probe_k_snapshot_iter);
		memset(probe_rt2_raw_snapshot_iter, 0, sizeof probe_rt2_raw_snapshot_iter);
		probe_snap_n_iter = 0;
		probe_cur_gap_iter = 0;
		probe_alt_gap_iter = 0;
		other_strong_progress_iter = 0;
		other_direct_modp_ok_iter = 0;
		other_exact_ok_iter = 0;
		other_exact_modp_ok_iter = 0;
		cur_exact_ok_iter = 0;
		cur_exact_modp_ok_iter = 0;
		probe_exact_ok_iter = 0;
		probe_exact_modp_ok_iter = 0;
		other_exact_bad_index_iter = 0;
		other_exact_expected_iter = 0;
		other_exact_actual_iter = 0;
		cur_exact_bad_index_iter = 0;
		cur_exact_expected_iter = 0;
		cur_exact_actual_iter = 0;
		probe_exact_bad_index_iter = 0;
		probe_exact_expected_iter = 0;
		probe_exact_actual_iter = 0;
		other_exact_gap_iter = 0;
		cur_exact_gap_iter = 0;
		probe_exact_gap_iter = 0;
		stall_counted = 0;
		nominal_tail_iter = 0;
		effective_tail_iter = 0;
		pseudo_tail_iter = 0;
		tail_stage_iter = 0;
		can_shrink_iter = 1;
		effective_update_iter = 0;
		force_modp_guard_iter = 0;
		update_flen = babai_slen;
		if (q == 3329 && depth >= 7 && current_scale_k == 0
			&& fg_tail_flen < update_flen)
		{
			update_flen = fg_tail_flen;
		}
		apply_flen = update_flen;
		if (need_rebuild) {
			rlen_cur = rlen_base;
			if (rlen_cur > update_flen) {
				rlen_cur = update_flen;
			}
			scale_fg = 31 * (int)(update_flen - rlen_cur);
			scale_xf = poly_max_bitlength(
				ft + update_flen - rlen_cur, rlen_cur, sstride, logn);
			scale_xg = poly_max_bitlength(
				gt + update_flen - rlen_cur, rlen_cur, sstride, logn);
			scale_x = scale_xf
				^ (-((scale_xf - scale_xg) >> 31) & (scale_xf ^ scale_xg));
			scale_t = 15 - logn;
			scale_t ^= -((uint32_t)(scale_x - scale_t) >> 31)
				& (scale_x ^ scale_t);
			poly_big_to_fixed(rt3, ft + update_flen - rlen_cur, rlen_cur, sstride,
				scale_x - scale_t, logn);
			poly_big_to_fixed(rt4, gt + update_flen - rlen_cur, rlen_cur, sstride,
				scale_x - scale_t, logn);
			ctl_FFT(rt3, logn);
			ctl_FFT(rt4, logn);
			ctl_poly_invnorm_fft(rt5, rt3, rt4, scale_t << 1, logn);
			ctl_poly_adj_fft(rt3, logn);
			ctl_poly_adj_fft(rt4, logn);
			ctl_poly_div_2e(rt3, scale_t, logn);
			ctl_poly_div_2e(rt4, scale_t, logn);
			need_rebuild = 0;
		}
		cur_rt3_raw_iter = rt3[0].v;
		cur_rt4_raw_iter = rt4[0].v;
		cur_rt5_raw_iter = rt5[0].v;

		/*
		 * Convert current F and G into fixed-point. We want to
		 * apply scaling scale_FG + scale_x. However, scale_FG
		 * is not necessarily a multiple of 31, so we have to
		 * account for it.
		 */
		scale_FG_eff = scale_FG;
		if (q == 3329 && depth >= 7 && FGlen > rlen_cur) {
			size_t floor_words;
			uint32_t scale_floor;

			floor_words = FGlen - rlen_cur;
			scale_floor = 31 * (uint32_t)floor_words;
			if (scale_FG > scale_fg) {
				uint32_t phase;

				if (scale_FG_eff < scale_floor) {
					phase = 0;
					if (reduce_bits != 0) {
						phase = (uint32_t)((31u
							- (((uint64_t)iter_count * (uint64_t)reduce_bits) % 31u))
							% 31u);
					}
					scale_FG_eff = scale_floor + phase;
				}
			} else if (scale_FG_eff < scale_floor) {
				uint32_t phase;

				/*
				 * In the final-scale tail, keep sliding the public window
				 * within the highest active limbs. A fixed word-aligned cut
				 * tends to drive rt2 to zero on 3329/2048 even while the top
				 * sign-extension limb is still not redundant.
				 */
				phase = 0;
				if (reduce_bits != 0) {
					phase = (uint32_t)((31u
						- (((uint64_t)iter_count * (uint64_t)reduce_bits) % 31u))
						% 31u);
				}
				scale_FG_eff = scale_floor + phase;
			}
		}
		nominal_tail_iter = (scale_FG <= scale_fg);
		effective_tail_iter = (scale_FG_eff <= scale_fg);
		pseudo_tail_iter = nominal_tail_iter && !effective_tail_iter;
		tail_stage_iter = (q == 3329 && depth >= 7)
			? (effective_tail_iter || pseudo_tail_iter)
			: nominal_tail_iter;
		DIVREM31(tlen, toff, scale_FG_eff);
		if (tlen > FGlen) {
			kg_diag_stats.solve_fg_fail_intermediate_check_post ++;
			kg_diag_stats.solve_fg_fail_intermediate_check_true ++;
			kg_diag_stats.solve_fg_fail_intermediate_check ++;
			kg_last_inter_depth = depth;
			kg_last_inter_stage = 5;
			kg_last_inter_index = iter_count;
			kg_last_inter_slen = (uint32_t)slen;
			kg_last_inter_fglen = (uint32_t)FGlen;
			return 0;
		}
		poly_big_to_fixed(rt1, Ft + tlen, FGlen - tlen, llen,
			scale_x + toff, logn);
		poly_big_to_fixed(rt2, Gt + tlen, FGlen - tlen, llen,
			scale_x + toff, logn);

		/*
		 * Compute (F*adj(f)+G*adj(g))/(f*adj(f)+g*adj(g)) in rt2.
		 */
		ctl_FFT(rt1, logn);
		ctl_FFT(rt2, logn);
		ctl_poly_mul_fft(rt1, rt3, logn);
		ctl_poly_mul_fft(rt2, rt4, logn);
		ctl_poly_add(rt2, rt1, logn);
		ctl_poly_mul_autoadj_fft(rt2, rt5, logn);
		ctl_iFFT(rt2, logn);

		/*
		 * Round k to integers.
		 */
		had_update = 0;
		k_nz_iter = 0;
		k_abs_sum_iter = 0;
		k_abs_max_iter = 0;
		for (u = 0; u < n; u ++) {
			uint32_t ak;

			k[u] = fnr_round(rt2[u]);
			had_update |= (k[u] != 0);
			k_nz_iter += (uint32_t)(k[u] != 0);
			ak = (uint32_t)(k[u] < 0 ? -k[u] : k[u]);
			k_abs_sum_iter += ak;
			if (ak > k_abs_max_iter) {
				k_abs_max_iter = ak;
			}
		}
		rt2_snap_n = n;
		if (rt2_snap_n > 8) {
			rt2_snap_n = 8;
		}
		for (u = 0; u < rt2_snap_n; u ++) {
			rt2_raw_snapshot[u] = rt2[u].v;
			rt2_round_snapshot[u] = k[u];
		}
		apply_scale_k = scale_FG_eff - scale_fg;
		if (q == 3329 && depth >= 7 && depth <= 9
			&& pseudo_tail_iter
			&& FGlen > babai_slen
			&& fg_tail_flen < update_flen && rlen_cur == fg_tail_flen
			&& had_update)
		{
			uint32_t bad_index;
			uint32_t ft_hi, ft_lo, gt_hi, gt_lo;

			bad_index = 0;
			ft_hi = 0;
			ft_lo = 0;
			gt_hi = 0;
			gt_lo = 0;
			if (intermediate_get_first_shrink_blocker(
				Ft, Gt, llen, FGlen, n,
				&bad_index, &ft_hi, &ft_lo, &gt_hi, &gt_lo))
			{
				uint64_t cur_gap_post;
				uint32_t cur_blockers_post, best_scaled_blockers;
				uint32_t other_blockers_post;
				uint32_t nominal_blockers_post;
				uint32_t exact_blockers_post;
				uint32_t other_exact_blockers_post;
				uint32_t cur_exact_blockers_post;
				uint32_t other_scale_fg, other_scale_k;
				uint64_t cur_excess_post, best_scaled_excess;
				uint64_t other_gap_post, other_excess_post;
				uint64_t nominal_excess_post;
				uint64_t exact_excess_post;
				uint64_t other_exact_gap_post, other_exact_excess_post;
				uint64_t cur_exact_gap_post, cur_exact_excess_post;
				uint64_t best_scaled_gap_post;
				uint32_t best_scaled_factor;
				int use_other_window;
				int other_strong_progress;
				int32_t k_scaled_best[KG_TAIL_PROBE_MAX_N];
				uint64_t nominal_gap_post;
				uint64_t exact_gap_post;
				int32_t k_nominal[KG_TAIL_PROBE_MAX_N];
				int32_t k_probe[KG_TAIL_PROBE_MAX_N];
				int32_t k_exact[KG_TAIL_PROBE_MAX_N];
				uint64_t nominal_rt2_raw[8];
				uint64_t probe_rt2_raw[8];
				uint32_t nominal_scale_fg, nominal_scale_x, nominal_scale_t;
				uint32_t nominal_tlen, nominal_toff;
				uint32_t nominal_scale_k;
				uint32_t probe_scale_fg, probe_scale_x, probe_scale_t;
				uint32_t probe_tlen, probe_toff;
				uint32_t probe_scale_k;
				uint32_t probe_direct_blockers_post;
				uint32_t exact_scale_k;
				uint32_t other_exact_scale_k;
				uint32_t cur_exact_scale_k;
				uint32_t nominal_k_nz, nominal_k_abs_max;
				uint32_t probe_k_nz, probe_k_abs_max;
				uint64_t nominal_k_abs_sum;
				uint64_t probe_k_abs_sum;
				uint64_t probe_direct_excess_post;
				uint64_t probe_direct_gap_post;
				int nominal_ok;
				int probe_ok;
				int probe_gap_ok;
				int exact_ok;
				int other_exact_ok;
				int cur_exact_ok;

				cur_gap_post = blocker_gap_pre_iter;
				if (!kg_tail_eval_blocker_gap_for_k(&cur_gap_post,
					Ft, Gt, llen, FGlen, bad_index,
					ft, gt, update_flen, sstride,
					k, apply_scale_k, logn))
				{
					cur_gap_post = blocker_gap_pre_iter;
				}
				kg_tail_eval_sparse_k_exact(&cur_blockers_post, &cur_excess_post,
					Ft, Gt, llen, FGlen,
					ft, gt, update_flen, sstride,
					k, apply_scale_k, logn);
				best_scaled_blockers = cur_blockers_post;
				best_scaled_excess = cur_excess_post;
				best_scaled_gap_post = cur_gap_post;
				best_scaled_factor = 0;
				use_other_window = 0;
				memset(k_scaled_best, 0, n * sizeof *k_scaled_best);
				other_scale_fg = 31u
					* (uint32_t)(fg_tail_flen - rlen_cur);
				other_scale_k = scale_FG_eff - other_scale_fg;
				other_gap_post = cur_gap_post;
				other_excess_post = cur_excess_post;
				kg_tail_eval_sparse_k_exact(&other_blockers_post, &other_excess_post,
					Ft, Gt, llen, FGlen,
					ft, gt, fg_tail_flen, sstride,
					k, other_scale_k, logn);
				if (!kg_tail_eval_blocker_gap_for_k(&other_gap_post,
					Ft, Gt, llen, FGlen, bad_index,
					ft, gt, fg_tail_flen, sstride,
					k, other_scale_k, logn))
				{
					other_gap_post = cur_gap_post;
				}
				if (kg_tail_metrics_better(other_blockers_post, other_gap_post,
					other_excess_post, best_scaled_blockers,
					best_scaled_gap_post, best_scaled_excess))
				{
					best_scaled_blockers = other_blockers_post;
					best_scaled_excess = other_excess_post;
					best_scaled_gap_post = other_gap_post;
					use_other_window = 1;
				}
				other_strong_progress =
					(other_blockers_post <= cur_blockers_post)
					&& (other_gap_post < cur_gap_post)
					&& (other_gap_post + (cur_gap_post >> 3) < cur_gap_post);
				other_strong_progress_iter = other_strong_progress ? 1u : 0u;
				for (u = 2; u <= 4; u ++) {
					int32_t k_scaled[KG_TAIL_PROBE_MAX_N];
					uint32_t trial_blockers;
					uint64_t trial_excess;
					uint64_t trial_gap;
					size_t v;
					int ok;

					ok = 1;
					for (v = 0; v < n; v ++) {
						int64_t sv;

						sv = (int64_t)k[v] * (int64_t)u;
						if (sv < -(int64_t)0x7FFFFFFF || sv > (int64_t)0x7FFFFFFF) {
							ok = 0;
							break;
						}
						k_scaled[v] = (int32_t)sv;
					}
					if (!ok) {
						continue;
					}
					kg_tail_eval_sparse_k_exact(&trial_blockers, &trial_excess,
						Ft, Gt, llen, FGlen,
						ft, gt, update_flen, sstride,
						k_scaled, apply_scale_k, logn);
					trial_gap = cur_gap_post;
					if (!kg_tail_eval_blocker_gap_for_k(&trial_gap,
						Ft, Gt, llen, FGlen, bad_index,
						ft, gt, update_flen, sstride,
						k_scaled, apply_scale_k, logn))
					{
						trial_gap = cur_gap_post;
					}
					if (kg_tail_metrics_better(trial_blockers, trial_gap,
						trial_excess, best_scaled_blockers,
						best_scaled_gap_post, best_scaled_excess))
					{
						best_scaled_blockers = trial_blockers;
						best_scaled_excess = trial_excess;
						best_scaled_gap_post = trial_gap;
						best_scaled_factor = (uint32_t)u;
						memcpy(k_scaled_best, k_scaled, n * sizeof *k_scaled_best);
					}
				}
				if (best_scaled_factor != 0
					&& !kg_tail_k_saturated(k_scaled_best, n)
					&& kg_tail_metrics_progress(
						best_scaled_blockers, best_scaled_gap_post,
						cur_blockers_post, cur_gap_post)
					&& kg_tail_candidate_modp_ok(
						Ft, Gt, llen, FGlen,
						ft, gt, update_flen, sstride,
						k_scaled_best, apply_scale_k, q, logn,
						NULL, NULL, NULL))
				{
						memcpy(k, k_scaled_best, n * sizeof *k);
						k_abs_sum_iter = 0;
						k_abs_max_iter = 0;
						for (u = 0; u < n; u ++) {
							uint32_t ak;

							ak = (uint32_t)(k[u] < 0 ? -k[u] : k[u]);
							k_abs_sum_iter += ak;
							if (ak > k_abs_max_iter) {
								k_abs_max_iter = ak;
							}
						}
						probe_attempted_iter = 3;
						probe_selected_iter = best_scaled_factor;
						probe_flen_iter = (uint32_t)update_flen;
						probe_scale_fg_iter = scale_fg;
						probe_scale_k_iter = apply_scale_k;
						probe_scale_x_iter = scale_x;
						probe_scale_t_iter = scale_t;
						probe_tlen_iter = (uint32_t)tlen;
						probe_toff_iter = (uint32_t)toff;
						probe_k_nonzero_iter = k_nz_iter;
					had_update = 1;
					force_modp_guard_iter = 1;
					probe_cur_gap_iter = cur_gap_post;
					probe_alt_gap_iter = best_scaled_gap_post;
					probe_snap_n_iter = rt2_snap_n;
					memcpy(probe_k_snapshot_iter, k, rt2_snap_n * sizeof *probe_k_snapshot_iter);
					memcpy(probe_rt2_raw_snapshot_iter, rt2_raw_snapshot,
						rt2_snap_n * sizeof *probe_rt2_raw_snapshot_iter);
					goto pseudo_tail_candidate_done;
				}
				if ((use_other_window || other_strong_progress)
					&& !kg_tail_k_saturated(k, n)
					&& kg_tail_metrics_progress(
						other_blockers_post, other_gap_post,
						cur_blockers_post, cur_gap_post))
				{
					other_direct_modp_ok_iter = kg_tail_candidate_modp_ok(
						Ft, Gt, llen, FGlen,
						ft, gt, fg_tail_flen, sstride,
						k, other_scale_k, q, logn,
						NULL, NULL, NULL);
					if (!other_direct_modp_ok_iter) {
						goto other_window_direct_done;
					}
					had_update = 1;
					apply_flen = fg_tail_flen;
					apply_scale_k = other_scale_k;
					force_modp_guard_iter = 1;
					probe_attempted_iter = 4;
					probe_selected_iter = 4;
					probe_flen_iter = (uint32_t)fg_tail_flen;
					probe_scale_fg_iter = other_scale_fg;
					probe_scale_k_iter = other_scale_k;
					probe_scale_x_iter = scale_x;
					probe_scale_t_iter = scale_t;
					probe_tlen_iter = (uint32_t)tlen;
					probe_toff_iter = (uint32_t)toff;
					probe_k_nonzero_iter = k_nz_iter;
					probe_cur_gap_iter = cur_gap_post;
					probe_alt_gap_iter = other_gap_post;
					probe_snap_n_iter = rt2_snap_n;
					memcpy(probe_k_snapshot_iter, k, rt2_snap_n * sizeof *probe_k_snapshot_iter);
					memcpy(probe_rt2_raw_snapshot_iter, rt2_raw_snapshot,
						rt2_snap_n * sizeof *probe_rt2_raw_snapshot_iter);
					need_rebuild = 1;
					goto pseudo_tail_candidate_done;
				}
other_window_direct_done:
				;
				other_exact_scale_k = scale_FG_eff - scale_fg;
				other_exact_ok = use_other_window
					&& kg_tail_fit_probe_pair_exact(k_exact,
						&other_exact_gap_post,
						Ft, Gt, llen, FGlen, bad_index,
						ft, gt, update_flen, sstride, other_exact_scale_k,
						fg_tail_flen, other_scale_k,
						k, logn);
				if (!other_exact_ok) {
					other_exact_ok = use_other_window
						&& kg_tail_fit_probe_support_exact(k_exact,
							&other_exact_gap_post,
							Ft, Gt, llen, FGlen, bad_index,
							ft, gt, update_flen, sstride, other_exact_scale_k,
							fg_tail_flen, other_scale_k,
							k, logn);
				}
				if (!other_exact_ok) {
					other_exact_ok = use_other_window
						&& kg_tail_try_escape_guided_pair(k_exact,
							&other_exact_gap_post,
							Ft, Gt, llen, FGlen,
							ft, gt, update_flen, sstride,
							other_exact_scale_k, k, logn);
				}
				if (!other_exact_ok) {
					other_exact_ok = use_other_window
						&& kg_tail_try_escape_guided(k_exact,
							&other_exact_gap_post,
							Ft, Gt, llen, FGlen,
							ft, gt, update_flen, sstride,
							other_exact_scale_k, k, logn);
				}
				if (!other_exact_ok) {
					other_exact_ok = use_other_window
						&& kg_tail_project_probe_exact(k_exact,
							&other_exact_gap_post,
							Ft, Gt, llen, FGlen, bad_index,
							ft, gt, update_flen, sstride, other_exact_scale_k,
							fg_tail_flen, other_scale_k,
							k, logn);
				}
				if (other_exact_ok) {
					other_exact_ok_iter = 1;
					other_exact_gap_iter = other_exact_gap_post;
					kg_tail_eval_sparse_k_exact(&other_exact_blockers_post,
						&other_exact_excess_post,
						Ft, Gt, llen, FGlen,
						ft, gt, update_flen, sstride,
						k_exact, other_exact_scale_k, logn);
				}
				if (other_exact_ok && !kg_tail_k_saturated(k_exact, n)
					&& kg_tail_metrics_progress(
						other_exact_blockers_post, other_exact_gap_post,
						cur_blockers_post, cur_gap_post)
					&& kg_tail_metrics_better(
					other_exact_blockers_post, other_exact_gap_post,
					other_exact_excess_post, cur_blockers_post,
					cur_gap_post, cur_excess_post))
				{
					other_exact_modp_ok_iter = kg_tail_candidate_modp_ok(
						Ft, Gt, llen, FGlen,
						ft, gt, update_flen, sstride,
						k_exact, other_exact_scale_k, q, logn,
						&other_exact_bad_index_iter,
						&other_exact_expected_iter,
						&other_exact_actual_iter);
					if (!other_exact_modp_ok_iter) {
						goto other_exact_done;
					}
					memcpy(k, k_exact, n * sizeof *k);
					had_update = 1;
					k_nz_iter = 0;
					k_abs_sum_iter = 0;
					k_abs_max_iter = 0;
					for (u = 0; u < n; u ++) {
						uint32_t ak;

						k_nz_iter += (uint32_t)(k[u] != 0);
						ak = (uint32_t)(k[u] < 0 ? -k[u] : k[u]);
						k_abs_sum_iter += ak;
						if (ak > k_abs_max_iter) {
							k_abs_max_iter = ak;
						}
					}
					apply_flen = update_flen;
					apply_scale_k = other_exact_scale_k;
					force_modp_guard_iter = 1;
					probe_attempted_iter = 4;
					probe_selected_iter = 4;
					probe_flen_iter = (uint32_t)fg_tail_flen;
					probe_scale_fg_iter = other_scale_fg;
					probe_scale_k_iter = other_exact_scale_k;
					probe_scale_x_iter = scale_x;
					probe_scale_t_iter = scale_t;
					probe_tlen_iter = (uint32_t)tlen;
					probe_toff_iter = (uint32_t)toff;
					probe_k_nonzero_iter = k_nz_iter;
					probe_cur_gap_iter = cur_gap_post;
					probe_alt_gap_iter = other_exact_gap_post;
					probe_snap_n_iter = rt2_snap_n;
					memcpy(probe_k_snapshot_iter, k, rt2_snap_n * sizeof *probe_k_snapshot_iter);
					memcpy(probe_rt2_raw_snapshot_iter, rt2_raw_snapshot,
						rt2_snap_n * sizeof *probe_rt2_raw_snapshot_iter);
					goto pseudo_tail_candidate_done;
				}
other_exact_done:
				;
				cur_exact_scale_k = apply_scale_k;
				cur_exact_ok = kg_tail_fit_probe_pair_exact(k_exact,
					&cur_exact_gap_post,
					Ft, Gt, llen, FGlen, bad_index,
					ft, gt, update_flen, sstride, cur_exact_scale_k,
					update_flen, cur_exact_scale_k,
					k, logn);
				if (!cur_exact_ok) {
					cur_exact_ok = kg_tail_fit_probe_support_exact(k_exact,
						&cur_exact_gap_post,
						Ft, Gt, llen, FGlen, bad_index,
						ft, gt, update_flen, sstride, cur_exact_scale_k,
						update_flen, cur_exact_scale_k,
						k, logn);
				}
				if (!cur_exact_ok) {
					cur_exact_ok = kg_tail_try_escape_guided_pair(k_exact,
						&cur_exact_gap_post,
						Ft, Gt, llen, FGlen,
						ft, gt, update_flen, sstride,
						cur_exact_scale_k, k, logn);
				}
				if (!cur_exact_ok) {
					cur_exact_ok = kg_tail_try_escape_guided(k_exact,
						&cur_exact_gap_post,
						Ft, Gt, llen, FGlen,
						ft, gt, update_flen, sstride,
						cur_exact_scale_k, k, logn);
				}
				if (!cur_exact_ok) {
					cur_exact_ok = kg_tail_project_probe_exact(k_exact,
						&cur_exact_gap_post,
						Ft, Gt, llen, FGlen, bad_index,
						ft, gt, update_flen, sstride, cur_exact_scale_k,
						update_flen, cur_exact_scale_k,
						k, logn);
				}
				if (cur_exact_ok) {
					cur_exact_ok_iter = 1;
					cur_exact_gap_iter = cur_exact_gap_post;
					kg_tail_eval_sparse_k_exact(&cur_exact_blockers_post,
						&cur_exact_excess_post,
						Ft, Gt, llen, FGlen,
						ft, gt, update_flen, sstride,
						k_exact, cur_exact_scale_k, logn);
				}
				if (cur_exact_ok && !kg_tail_k_saturated(k_exact, n)
					&& kg_tail_metrics_progress(
						cur_exact_blockers_post, cur_exact_gap_post,
						cur_blockers_post, cur_gap_post)
					&& kg_tail_metrics_better(
					cur_exact_blockers_post, cur_exact_gap_post,
					cur_exact_excess_post, cur_blockers_post, cur_gap_post,
					cur_excess_post))
				{
					cur_exact_modp_ok_iter = kg_tail_candidate_modp_ok(
						Ft, Gt, llen, FGlen,
						ft, gt, update_flen, sstride,
						k_exact, cur_exact_scale_k, q, logn,
						&cur_exact_bad_index_iter,
						&cur_exact_expected_iter,
						&cur_exact_actual_iter);
					if (!cur_exact_modp_ok_iter) {
						goto cur_exact_done;
					}
					memcpy(k, k_exact, n * sizeof *k);
					had_update = 1;
					k_nz_iter = 0;
					k_abs_sum_iter = 0;
					k_abs_max_iter = 0;
					for (u = 0; u < n; u ++) {
						uint32_t ak;

						k_nz_iter += (uint32_t)(k[u] != 0);
						ak = (uint32_t)(k[u] < 0 ? -k[u] : k[u]);
						k_abs_sum_iter += ak;
						if (ak > k_abs_max_iter) {
							k_abs_max_iter = ak;
						}
					}
					apply_flen = update_flen;
					apply_scale_k = cur_exact_scale_k;
					force_modp_guard_iter = 1;
					probe_attempted_iter = 5;
					probe_selected_iter = 5;
					probe_flen_iter = (uint32_t)update_flen;
					probe_scale_fg_iter = scale_fg;
					probe_scale_k_iter = cur_exact_scale_k;
					probe_scale_x_iter = scale_x;
					probe_scale_t_iter = scale_t;
					probe_tlen_iter = (uint32_t)tlen;
					probe_toff_iter = (uint32_t)toff;
					probe_k_nonzero_iter = k_nz_iter;
					probe_cur_gap_iter = cur_gap_post;
					probe_alt_gap_iter = cur_exact_gap_post;
					probe_snap_n_iter = rt2_snap_n;
					memcpy(probe_k_snapshot_iter, k, rt2_snap_n * sizeof *probe_k_snapshot_iter);
					memcpy(probe_rt2_raw_snapshot_iter, rt2_raw_snapshot,
						rt2_snap_n * sizeof *probe_rt2_raw_snapshot_iter);
					goto pseudo_tail_candidate_done;
				}
cur_exact_done:
				;
				memset(nominal_rt2_raw, 0, sizeof nominal_rt2_raw);
				nominal_ok = kg_tail_compute_probe_k(k_nominal,
					nominal_rt2_raw, rt2_snap_n,
					&nominal_scale_fg, &nominal_scale_x,
					&nominal_scale_t, &nominal_tlen, &nominal_toff,
					NULL, NULL, NULL,
					&nominal_scale_k,
					&nominal_k_nz, &nominal_k_abs_sum, &nominal_k_abs_max,
					Ft, Gt, FGlen, llen,
					ft, gt, update_flen, rlen_cur, sstride,
					scale_FG, scale_x, 0, 0, logn);
				if (nominal_ok && nominal_k_nz != 0
					&& kg_tail_eval_blocker_gap_for_k(&nominal_gap_post,
						Ft, Gt, llen, FGlen, bad_index,
						ft, gt, update_flen, sstride,
						k_nominal, nominal_scale_k, logn))
				{
					kg_tail_eval_sparse_k_exact(&nominal_blockers_post,
						&nominal_excess_post,
						Ft, Gt, llen, FGlen,
						ft, gt, update_flen, sstride,
						k_nominal, nominal_scale_k, logn);
				}
				if (nominal_ok && nominal_k_nz != 0
					&& !kg_tail_k_saturated(k_nominal, n)
					&& kg_tail_metrics_progress(
						nominal_blockers_post, nominal_gap_post,
						cur_blockers_post, cur_gap_post)
					&& kg_tail_metrics_better(
						nominal_blockers_post, nominal_gap_post,
						nominal_excess_post, cur_blockers_post,
						cur_gap_post, cur_excess_post)
					&& kg_tail_candidate_modp_ok(
						Ft, Gt, llen, FGlen,
						ft, gt, update_flen, sstride,
						k_nominal, nominal_scale_k, q, logn,
						NULL, NULL, NULL))
				{
					memcpy(k, k_nominal, n * sizeof *k);
					had_update = 1;
					k_nz_iter = nominal_k_nz;
					k_abs_sum_iter = nominal_k_abs_sum;
					k_abs_max_iter = nominal_k_abs_max;
					apply_flen = update_flen;
					apply_scale_k = nominal_scale_k;
					force_modp_guard_iter = 1;
					probe_attempted_iter = 2;
					probe_selected_iter = 2;
					probe_flen_iter = (uint32_t)update_flen;
					probe_scale_fg_iter = nominal_scale_fg;
					probe_scale_k_iter = nominal_scale_k;
					probe_scale_x_iter = nominal_scale_x;
					probe_scale_t_iter = nominal_scale_t;
					probe_tlen_iter = nominal_tlen;
					probe_toff_iter = nominal_toff;
					probe_k_nonzero_iter = nominal_k_nz;
					probe_cur_gap_iter = cur_gap_post;
					probe_alt_gap_iter = nominal_gap_post;
					probe_snap_n_iter = rt2_snap_n;
					memcpy(probe_k_snapshot_iter, k_nominal,
						rt2_snap_n * sizeof *probe_k_snapshot_iter);
					memcpy(probe_rt2_raw_snapshot_iter, nominal_rt2_raw,
						rt2_snap_n * sizeof *probe_rt2_raw_snapshot_iter);
					for (u = 0; u < rt2_snap_n; u ++) {
						rt2_raw_snapshot[u] = nominal_rt2_raw[u];
						rt2_round_snapshot[u] = k[u];
					}
					need_rebuild = 1;
					goto pseudo_tail_candidate_done;
				}
				memset(probe_rt2_raw, 0, sizeof probe_rt2_raw);
				probe_attempted_iter = 1;
				probe_ok = kg_tail_compute_probe_k(k_probe,
					probe_rt2_raw, rt2_snap_n,
					&probe_scale_fg, &probe_scale_x,
					&probe_scale_t, &probe_tlen, &probe_toff,
					&probe_rt3_raw_iter, &probe_rt4_raw_iter, &probe_rt5_raw_iter,
					&probe_scale_k,
					&probe_k_nz, &probe_k_abs_sum, &probe_k_abs_max,
					Ft, Gt, FGlen, llen,
					ft, gt, fg_tail_flen, rlen_cur, sstride,
					scale_FG_eff, scale_x, 0, 0, logn);
				if (probe_ok) {
					probe_flen_iter = (uint32_t)fg_tail_flen;
					probe_scale_fg_iter = probe_scale_fg;
					probe_scale_k_iter = probe_scale_k;
					probe_scale_x_iter = probe_scale_x;
					probe_scale_t_iter = probe_scale_t;
					probe_tlen_iter = probe_tlen;
					probe_toff_iter = probe_toff;
					probe_k_nonzero_iter = probe_k_nz;
					probe_snap_n_iter = rt2_snap_n;
					memcpy(probe_k_snapshot_iter, k_probe,
						rt2_snap_n * sizeof *probe_k_snapshot_iter);
					memcpy(probe_rt2_raw_snapshot_iter, probe_rt2_raw,
						rt2_snap_n * sizeof *probe_rt2_raw_snapshot_iter);
					probe_cur_gap_iter = cur_gap_post;
				}
				if (probe_ok && probe_k_nz != 0
					&& kg_tail_eval_blocker_gap_for_k(&probe_direct_gap_post,
						Ft, Gt, llen, FGlen, bad_index,
						ft, gt, update_flen, sstride,
						k_probe, probe_scale_k, logn))
				{
					kg_tail_eval_sparse_k_exact(&probe_direct_blockers_post,
						&probe_direct_excess_post,
						Ft, Gt, llen, FGlen,
						ft, gt, update_flen, sstride,
						k_probe, probe_scale_k, logn);
					if (!kg_tail_k_saturated(k_probe, n)
						&& kg_tail_metrics_progress(
							probe_direct_blockers_post, probe_direct_gap_post,
							cur_blockers_post, cur_gap_post)
						&& kg_tail_metrics_better(
							probe_direct_blockers_post, probe_direct_gap_post,
							probe_direct_excess_post, cur_blockers_post,
							cur_gap_post, cur_excess_post)
						&& kg_tail_candidate_modp_ok(
							Ft, Gt, llen, FGlen,
							ft, gt, update_flen, sstride,
							k_probe, probe_scale_k, q, logn,
							NULL, NULL, NULL))
					{
						memcpy(k, k_probe, n * sizeof *k);
						had_update = 1;
						k_nz_iter = probe_k_nz;
						k_abs_sum_iter = probe_k_abs_sum;
						k_abs_max_iter = probe_k_abs_max;
						apply_flen = update_flen;
						apply_scale_k = probe_scale_k;
						force_modp_guard_iter = 1;
						probe_selected_iter = 6;
						probe_alt_gap_iter = probe_direct_gap_post;
						for (u = 0; u < rt2_snap_n; u ++) {
							rt2_raw_snapshot[u] = probe_rt2_raw[u];
							rt2_round_snapshot[u] = k[u];
						}
						goto pseudo_tail_candidate_done;
					}
				}
				exact_scale_k = scale_FG_eff - scale_fg;
				exact_ok = probe_ok && probe_k_nz != 0
					&& kg_tail_fit_probe_pair_exact(k_exact, &exact_gap_post,
						Ft, Gt, llen, FGlen, bad_index,
						ft, gt, update_flen, sstride, exact_scale_k,
						fg_tail_flen, probe_scale_k,
						k_probe, logn);
				if (!exact_ok) {
					exact_ok = probe_ok && probe_k_nz != 0
					&& kg_tail_fit_probe_support_exact(k_exact, &exact_gap_post,
						Ft, Gt, llen, FGlen, bad_index,
						ft, gt, update_flen, sstride, exact_scale_k,
						fg_tail_flen, probe_scale_k,
						k_probe, logn);
				}
				if (!exact_ok) {
					exact_ok = probe_ok && probe_k_nz != 0
					&& kg_tail_try_escape_guided_pair(k_exact, &exact_gap_post,
						Ft, Gt, llen, FGlen,
						ft, gt, update_flen, sstride,
						exact_scale_k, k_probe, logn);
				}
				if (!exact_ok) {
					exact_ok = probe_ok && probe_k_nz != 0
					&& kg_tail_try_escape_guided(k_exact, &exact_gap_post,
						Ft, Gt, llen, FGlen,
						ft, gt, update_flen, sstride,
						exact_scale_k, k_probe, logn);
				}
				if (!exact_ok) {
					exact_ok = probe_ok && probe_k_nz != 0
						&& kg_tail_project_probe_exact(k_exact, &exact_gap_post,
							Ft, Gt, llen, FGlen, bad_index,
							ft, gt, update_flen, sstride, exact_scale_k,
							fg_tail_flen, probe_scale_k,
							k_probe, logn);
				}
				probe_gap_ok = exact_ok;
				if (probe_gap_ok) {
					probe_exact_ok_iter = 1;
					probe_exact_gap_iter = exact_gap_post;
					probe_alt_gap_iter = exact_gap_post;
					kg_tail_eval_sparse_k_exact(&exact_blockers_post,
						&exact_excess_post,
						Ft, Gt, llen, FGlen,
						ft, gt, update_flen, sstride,
						k_exact, exact_scale_k, logn);
				}
				if (probe_gap_ok && !kg_tail_k_saturated(k_exact, n)
					&& kg_tail_metrics_progress(
						exact_blockers_post, exact_gap_post,
						cur_blockers_post, cur_gap_post)
					&& kg_tail_metrics_better(
					exact_blockers_post, exact_gap_post,
					exact_excess_post, cur_blockers_post,
					cur_gap_post, cur_excess_post))
				{
					probe_exact_modp_ok_iter = kg_tail_candidate_modp_ok(
						Ft, Gt, llen, FGlen,
						ft, gt, update_flen, sstride,
						k_exact, exact_scale_k, q, logn,
						&probe_exact_bad_index_iter,
						&probe_exact_expected_iter,
						&probe_exact_actual_iter);
					if (!probe_exact_modp_ok_iter) {
						goto probe_exact_done;
					}
					memcpy(k, k_exact, n * sizeof *k);
					had_update = 1;
					k_nz_iter = 0;
					k_abs_sum_iter = 0;
					k_abs_max_iter = 0;
					for (u = 0; u < n; u ++) {
						uint32_t ak;

						k_nz_iter += (uint32_t)(k[u] != 0);
						ak = (uint32_t)(k[u] < 0 ? -k[u] : k[u]);
						k_abs_sum_iter += ak;
						if (ak > k_abs_max_iter) {
							k_abs_max_iter = ak;
						}
					}
					apply_flen = update_flen;
					apply_scale_k = exact_scale_k;
					force_modp_guard_iter = 1;
					probe_selected_iter = 1;
					for (u = 0; u < rt2_snap_n; u ++) {
						rt2_raw_snapshot[u] = probe_rt2_raw[u];
						rt2_round_snapshot[u] = k[u];
					}
					need_rebuild = 1;
				}
probe_exact_done:
				;
pseudo_tail_candidate_done:
				;
			}
		}
		if (q == 3329 && depth >= 7 && depth <= 9
			&& effective_tail_iter && FGlen > babai_slen
			&& fg_tail_flen < update_flen && rlen_cur == fg_tail_flen
			&& (tail_no_topchange >= 64 || tail_no_update >= 64))
		{
			uint32_t bad_index;
			uint32_t ft_hi, ft_lo, gt_hi, gt_lo;

			bad_index = 0;
			ft_hi = 0;
			ft_lo = 0;
			gt_hi = 0;
			gt_lo = 0;
			if (intermediate_get_first_shrink_blocker(
				Ft, Gt, llen, FGlen, n,
				&bad_index, &ft_hi, &ft_lo, &gt_hi, &gt_lo))
			{
				uint64_t cur_gap_post, probe_gap_post;
				int32_t k_probe[KG_TAIL_PROBE_MAX_N];
				uint64_t probe_rt2_raw[8];
				uint32_t probe_scale_fg, probe_scale_x, probe_scale_t;
				uint32_t probe_tlen, probe_toff;
				uint32_t probe_scale_k;
				uint32_t probe_k_nz, probe_k_abs_max;
				uint64_t probe_k_abs_sum;
				int probe_ok;
				int probe_gap_ok;

				cur_gap_post = blocker_gap_pre_iter;
				if (!kg_tail_eval_blocker_gap_for_k(&cur_gap_post,
					Ft, Gt, llen, FGlen, bad_index,
					ft, gt, update_flen, sstride,
					k, apply_scale_k, logn))
				{
					cur_gap_post = blocker_gap_pre_iter;
				}
				memset(probe_rt2_raw, 0, sizeof probe_rt2_raw);
				probe_attempted_iter = 1;
				probe_ok = kg_tail_compute_probe_k(k_probe,
					probe_rt2_raw, rt2_snap_n,
					&probe_scale_fg, &probe_scale_x,
					&probe_scale_t, &probe_tlen, &probe_toff,
					&probe_rt3_raw_iter, &probe_rt4_raw_iter, &probe_rt5_raw_iter,
					&probe_scale_k,
					&probe_k_nz, &probe_k_abs_sum, &probe_k_abs_max,
					Ft, Gt, FGlen, llen,
					ft, gt, fg_tail_flen, rlen_cur, sstride,
					scale_FG_eff, scale_x, 0, 0, logn);
				if (probe_ok) {
					probe_flen_iter = (uint32_t)fg_tail_flen;
					probe_scale_fg_iter = probe_scale_fg;
					probe_scale_k_iter = probe_scale_k;
					probe_scale_x_iter = probe_scale_x;
					probe_scale_t_iter = probe_scale_t;
					probe_tlen_iter = probe_tlen;
					probe_toff_iter = probe_toff;
					probe_k_nonzero_iter = probe_k_nz;
					probe_snap_n_iter = rt2_snap_n;
					memcpy(probe_k_snapshot_iter, k_probe,
						rt2_snap_n * sizeof *probe_k_snapshot_iter);
					memcpy(probe_rt2_raw_snapshot_iter, probe_rt2_raw,
						rt2_snap_n * sizeof *probe_rt2_raw_snapshot_iter);
					probe_cur_gap_iter = cur_gap_post;
				}
				probe_gap_ok = probe_ok && probe_k_nz != 0
					&& kg_tail_eval_blocker_gap_for_k(&probe_gap_post,
						Ft, Gt, llen, FGlen, bad_index,
						ft, gt, update_flen, sstride,
						k_probe, probe_scale_k, logn);
				if (probe_gap_ok) {
					probe_alt_gap_iter = probe_gap_post;
				}
				if (probe_gap_ok && probe_gap_post < cur_gap_post)
				{
					memcpy(k, k_probe, n * sizeof *k);
					had_update = 1;
					k_nz_iter = probe_k_nz;
					k_abs_sum_iter = probe_k_abs_sum;
					k_abs_max_iter = probe_k_abs_max;
					apply_flen = update_flen;
					apply_scale_k = probe_scale_k;
					for (u = 0; u < rt2_snap_n; u ++) {
						rt2_raw_snapshot[u] = probe_rt2_raw[u];
						rt2_round_snapshot[u] = k[u];
					}
					probe_selected_iter = 1;
					need_rebuild = 1;
				}
			}
		}
		if (q == 3329 && depth == 10) {
			kg_d10_iter_count ++;
			if (had_update) {
				kg_d10_k_nonzero_count ++;
			} else {
				kg_d10_k_zero_count ++;
			}
		}
		if (q == 3329 && depth == 9) {
			kg_d9_iter_count ++;
			if (had_update) {
				kg_d9_k_nonzero_count ++;
			} else {
				kg_d9_k_zero_count ++;
			}
		}
		if (q == 3329 && depth == 6) {
			kg_d6_iter_count ++;
			if (had_update) {
				kg_d6_k_nonzero_count ++;
			} else {
				kg_d6_k_zero_count ++;
			}
		}
		if (q == 3329 && depth == 5) {
			kg_d5_iter_count ++;
			if (had_update) {
				kg_d5_k_nonzero_count ++;
			} else {
				kg_d5_k_zero_count ++;
			}
		}
		if (q == 3329 && depth == 4) {
			kg_d4_iter_count ++;
			if (had_update) {
				kg_d4_k_nonzero_count ++;
			} else {
				kg_d4_k_zero_count ++;
			}
		}
		if (q == 3329 && depth == 2) {
			kg_d2_iter_count ++;
			if (had_update) {
				kg_d2_k_nonzero_count ++;
			} else {
				kg_d2_k_zero_count ++;
			}
			kg_d2_k_abs_sum += k_abs_sum_iter;
			kg_d2_k_abs_samples += n;
			if (k_abs_max_iter > kg_d2_k_abs_max) {
				kg_d2_k_abs_max = k_abs_max_iter;
			}
		}
		if (q == 3329 && depth == 8 && !kg_d8_post_valid) {
			uint32_t nzc, first_nz;
			int32_t first_val;

			nzc = 0;
			first_nz = 0;
			first_val = 0;
			for (u = 0; u < n; u ++) {
				if (k[u] != 0) {
					if (nzc == 0) {
						first_nz = (uint32_t)u;
						first_val = k[u];
					}
					nzc ++;
				}
			}
			kg_d8_post_k_nonzero_count = nzc;
			kg_d8_post_k_max_abs = k_abs_max_iter;
			kg_d8_post_k_first_nz_index = first_nz;
			kg_d8_post_k_first_nz_value = first_val;
			kg_d8_post_scale_x = scale_x;
			kg_d8_post_scale_t = scale_t;
			kg_d8_post_tlen = (uint32_t)tlen;
			kg_d8_post_toff = (uint32_t)toff;
			for (u = 0; u < n && u < 8; u ++) {
				kg_d8_pre_k[u] = k[u];
				kg_d8_pre_rt1[u] = rt1[u].v;
				kg_d8_pre_rt2[u] = rt2[u].v;
				kg_d8_pre_rt3[u] = rt3[u].v;
				kg_d8_pre_rt4[u] = rt4[u].v;
				kg_d8_pre_rt5[u] = rt5[u].v;
			}
		}

		/*
		 * (f,g) are scaled by scale_fg + scale_x.
		 * (F,G) are scaled by scale_FG + scale_x.
		 *
		 * Thus, k is scaled by scale_FG - scale_fg (this is a
		 * non-secret value, contrary to scale_x).
		 */
		current_scale_k = scale_FG_eff - scale_fg;
		scale_k = apply_scale_k;
		if (q == 3329 && depth < (sizeof kg_sk0_hist / sizeof kg_sk0_hist[0])
			&& current_scale_k == 0)
		{
			kg_sk0_hist[depth] ++;
			if (!kg_sk0_valid) {
				kg_sk0_valid = 1;
				kg_sk0_depth = depth;
				kg_sk0_logn = logn;
				kg_sk0_iter = iter_count;
				kg_sk0_rlen = (uint32_t)rlen_base;
				kg_sk0_slen = (uint32_t)slen;
				kg_sk0_fglen = (uint32_t)FGlen;
				kg_sk0_scale_fg = scale_fg;
				kg_sk0_scale_FG = scale_FG;
				kg_sk0_scale_k = current_scale_k;
			}
		}
		if (q == 3329 && depth == 2) {
			kg_d2_last_scale_fg = scale_fg;
			kg_d2_last_scale_FG = scale_FG;
			kg_d2_last_scale_k = current_scale_k;
			kg_d2_last_rlen = (uint32_t)rlen_base;
			kg_d2_last_slen = (uint32_t)slen;
			kg_d2_last_fglen = (uint32_t)FGlen;
			kg_d2_last_iter = iter_count;
		}
		if (q == 3329
			&& (depth == 2 || depth == 4 || depth == 5 || depth == 6
				|| depth == 7 || depth == 8 || depth == 9 || depth == 10))
		{
			kg_capture_top_words(kg_top_ft_before_words, kg_top_gt_before_words,
				Ft, Gt, llen, FGlen, n);
		}
		if (q == 3329) {
			intermediate_fill_sign_extension(Ft, Gt, llen, FGlen, llen, n);
			stage3_diag_capture_pre(logn, PRIMES[0].p,
				ft, gt, sstride, Ft, Gt, llen, k, iter_count);
			if (depth >= 7 && tail_stage_iter && FGlen > babai_slen) {
				uint32_t bad_index;
				uint32_t ft_hi, ft_lo, gt_hi, gt_lo;

				blocker_count_pre_iter = intermediate_count_shrink_blockers(
					Ft, Gt, llen, FGlen, n);
				bad_index = 0;
				ft_hi = 0;
				ft_lo = 0;
				gt_hi = 0;
				gt_lo = 0;
				if (intermediate_get_first_shrink_blocker(
					Ft, Gt, llen, FGlen, n,
					&bad_index, &ft_hi, &ft_lo, &gt_hi, &gt_lo))
				{
					(void)bad_index;
					blocker_gap_pre_iter =
						(uint64_t)kg_tail_signext_gap(ft_hi, ft_lo)
						+ (uint64_t)kg_tail_signext_gap(gt_hi, gt_lo);
				}
			}
			if (depth == 9 && !kg_d9_bad_pre_valid) {
				uint32_t bad_index;

				if (!intermediate_check_modp_direct(ft, gt, sstride, sstride,
					Ft, Gt, llen, llen, q, logn, t1, &bad_index))
				{
					kg_d9_bad_pre_valid = 1;
					kg_d9_bad_pre_iter = iter_count;
					kg_d9_bad_pre_bad_index = bad_index;
				}
			}
		}
		if (depth <= DEPTH_INT_FG) {
			poly_sub_scaled_ntt(Ft, FGlen, llen, ft, apply_flen, sstride,
				k, scale_k, logn, t1);
			poly_sub_scaled_ntt(Gt, FGlen, llen, gt, apply_flen, sstride,
				k, scale_k, logn, t1);
		} else {
			poly_sub_scaled(Ft, FGlen, llen, ft, apply_flen, sstride,
				k, scale_k, logn);
			poly_sub_scaled(Gt, FGlen, llen, gt, apply_flen, sstride,
				k, scale_k, logn);
		}
		if (q == 3329 && had_update
			&& (force_modp_guard_iter
				|| ((depth >= 7 && depth <= 8)
					|| (depth >= 9 && k_abs_max_iter <= 4 && k_nz_iter <= 8)))
			)
		{
			uint32_t bad_index;

			intermediate_fill_sign_extension(Ft, Gt, llen, FGlen, llen, n);
			if (!intermediate_check_modp(ft, gt, sstride, sstride,
				Ft, Gt, llen, llen, q, logn, t1, &bad_index))
			{
				for (u = 0; u < n; u ++) {
					k[u] = -k[u];
				}
				if (depth <= DEPTH_INT_FG) {
					poly_sub_scaled_ntt(Ft, FGlen, llen, ft, apply_flen, sstride,
						k, scale_k, logn, t1);
					poly_sub_scaled_ntt(Gt, FGlen, llen, gt, apply_flen, sstride,
						k, scale_k, logn, t1);
				} else {
					poly_sub_scaled(Ft, FGlen, llen, ft, apply_flen, sstride,
						k, scale_k, logn);
					poly_sub_scaled(Gt, FGlen, llen, gt, apply_flen, sstride,
						k, scale_k, logn);
				}
				for (u = 0; u < n; u ++) {
					k[u] = 0;
				}
				k_nz_iter = 0;
				k_abs_sum_iter = 0;
				k_abs_max_iter = 0;
				had_update = 0;
			}
		}
		if (q == 3329) {
			intermediate_fill_sign_extension(Ft, Gt, llen, FGlen, llen, n);
			stage3_diag_capture_post(logn, PRIMES[0].p, Ft, Gt, llen);
			if (depth == 9 && !kg_d9_bad_post_valid) {
				uint32_t bad_index;

				if (!intermediate_check_modp_direct(ft, gt, sstride, sstride,
					Ft, Gt, llen, llen, q, logn, t1, &bad_index))
				{
					kg_d9_bad_post_valid = 1;
					kg_d9_bad_post_iter = iter_count;
					kg_d9_bad_post_bad_index = bad_index;
					kg_d9_bad_post_round = kg_s3_last_round;
				}
			}
		}
		iter_count ++;
		if (q == 3329 && depth == 8 && !kg_d8_post_valid) {
			uint32_t bad_index;

			intermediate_fill_sign_extension(Ft, Gt, llen, FGlen, llen, n);
			if (!intermediate_check_modp(ft, gt, sstride, sstride,
				Ft, Gt, llen, llen,
				q, logn, t1, &bad_index))
			{
				kg_d8_post_valid = 1;
				kg_d8_post_iter = iter_count;
				kg_d8_post_fglen = (uint32_t)FGlen;
				kg_d8_post_bad_index = bad_index;
				kg_d8_post_expected = kg_last_modp_expected;
				kg_d8_post_actual = kg_last_modp_actual;
				kg_d8_post_mismatch_count = kg_last_modp_mismatch_count;
				kg_d8_post_had_update = had_update ? 1u : 0u;
				kg_d8_post_scale_k = scale_k;
				kg_d8_post_k_bad_value =
					(bad_index < 8) ? kg_d8_pre_k[bad_index] : 0;
				if (bad_index < 8) {
					kg_d8_post_rt2_lo = (uint32_t)kg_d8_pre_rt2[bad_index];
					kg_d8_post_rt2_hi =
						(uint32_t)(kg_d8_pre_rt2[bad_index] >> 32);
					kg_d8_post_rt2_round =
						fnr_round(fnr_of_scaled32(kg_d8_pre_rt2[bad_index]));
				} else {
					kg_d8_post_rt2_lo = 0;
					kg_d8_post_rt2_hi = 0;
					kg_d8_post_rt2_round = 0;
				}
			}
		}
		if (q == 3329 && depth == 10) {
			top_word_change_count_iter = kg_count_top_word_changes(
				Ft, Gt, llen, FGlen, n,
				kg_top_ft_before_words, kg_top_gt_before_words,
				&top_word_change_first_index_iter,
				&top_word_change_ft_pre_iter, &top_word_change_ft_post_iter,
				&top_word_change_gt_pre_iter, &top_word_change_gt_post_iter);
			top_word_changed_iter = (top_word_change_count_iter != 0);
			if (top_word_changed_iter) {
				kg_d10_top_changed ++;
			} else {
				kg_d10_top_unchanged ++;
			}
			uint32_t bad_index;
			intermediate_fill_sign_extension(Ft, Gt, llen, FGlen, llen, n);
			if (!intermediate_check_modp(ft, gt, sstride, sstride,
				Ft, Gt, llen, llen,
				q, logn, t1, &bad_index))
			{
				kg_diag_stats.solve_fg_fail_intermediate_check_post ++;
				kg_diag_stats.solve_fg_fail_intermediate_check_true ++;
				kg_diag_stats.solve_fg_fail_intermediate_check ++;
				kg_last_inter_depth = depth;
				kg_last_inter_stage = 4;
				kg_last_inter_index = iter_count;
				kg_last_inter_slen = (uint32_t)slen;
				kg_last_inter_fglen = (uint32_t)FGlen;
				(void)bad_index;
			}
		}
		if (q == 3329 && depth == 7) {
			top_word_change_count_iter = kg_count_top_word_changes(
				Ft, Gt, llen, FGlen, n,
				kg_top_ft_before_words, kg_top_gt_before_words,
				&top_word_change_first_index_iter,
				&top_word_change_ft_pre_iter, &top_word_change_ft_post_iter,
				&top_word_change_gt_pre_iter, &top_word_change_gt_post_iter);
			top_word_changed_iter = (top_word_change_count_iter != 0);
		}
		if (q == 3329 && depth == 8) {
			top_word_change_count_iter = kg_count_top_word_changes(
				Ft, Gt, llen, FGlen, n,
				kg_top_ft_before_words, kg_top_gt_before_words,
				&top_word_change_first_index_iter,
				&top_word_change_ft_pre_iter, &top_word_change_ft_post_iter,
				&top_word_change_gt_pre_iter, &top_word_change_gt_post_iter);
			top_word_changed_iter = (top_word_change_count_iter != 0);
		}
		if (q == 3329 && depth == 9) {
			top_word_change_count_iter = kg_count_top_word_changes(
				Ft, Gt, llen, FGlen, n,
				kg_top_ft_before_words, kg_top_gt_before_words,
				&top_word_change_first_index_iter,
				&top_word_change_ft_pre_iter, &top_word_change_ft_post_iter,
				&top_word_change_gt_pre_iter, &top_word_change_gt_post_iter);
			top_word_changed_iter = (top_word_change_count_iter != 0);
			if (top_word_changed_iter) {
				kg_d9_top_changed ++;
			} else {
				kg_d9_top_unchanged ++;
			}
		}
		if (q == 3329 && depth == 6) {
			top_word_change_count_iter = kg_count_top_word_changes(
				Ft, Gt, llen, FGlen, n,
				kg_top_ft_before_words, kg_top_gt_before_words,
				&top_word_change_first_index_iter,
				&top_word_change_ft_pre_iter, &top_word_change_ft_post_iter,
				&top_word_change_gt_pre_iter, &top_word_change_gt_post_iter);
			if (top_word_change_count_iter != 0) {
				kg_d6_top_changed ++;
			} else {
				kg_d6_top_unchanged ++;
			}
		}
		if (q == 3329 && depth == 5) {
			top_word_change_count_iter = kg_count_top_word_changes(
				Ft, Gt, llen, FGlen, n,
				kg_top_ft_before_words, kg_top_gt_before_words,
				&top_word_change_first_index_iter,
				&top_word_change_ft_pre_iter, &top_word_change_ft_post_iter,
				&top_word_change_gt_pre_iter, &top_word_change_gt_post_iter);
			if (top_word_change_count_iter != 0) {
				kg_d5_top_changed ++;
			} else {
				kg_d5_top_unchanged ++;
			}
		}
		if (q == 3329 && depth == 4) {
			top_word_change_count_iter = kg_count_top_word_changes(
				Ft, Gt, llen, FGlen, n,
				kg_top_ft_before_words, kg_top_gt_before_words,
				&top_word_change_first_index_iter,
				&top_word_change_ft_pre_iter, &top_word_change_ft_post_iter,
				&top_word_change_gt_pre_iter, &top_word_change_gt_post_iter);
			if (top_word_change_count_iter != 0) {
				kg_d4_top_changed ++;
			} else {
				kg_d4_top_unchanged ++;
			}
		}
		if (q == 3329 && depth == 2) {
			top_word_change_count_iter = kg_count_top_word_changes(
				Ft, Gt, llen, FGlen, n,
				kg_top_ft_before_words, kg_top_gt_before_words,
				&top_word_change_first_index_iter,
				&top_word_change_ft_pre_iter, &top_word_change_ft_post_iter,
				&top_word_change_gt_pre_iter, &top_word_change_gt_post_iter);
			if (top_word_change_count_iter != 0) {
				kg_d2_top_changed ++;
			} else {
				kg_d2_top_unchanged ++;
			}
		}
		can_shrink_iter = (FGlen <= babai_slen)
			? 1
			: intermediate_can_shrink_fglen(Ft, Gt, llen, FGlen, n);
		if (q == 3329 && depth >= 7 && tail_stage_iter) {
			if (FGlen > babai_slen) {
				uint32_t bad_index;
				uint32_t ft_hi, ft_lo, gt_hi, gt_lo;

				blocker_count_post_iter = intermediate_count_shrink_blockers(
					Ft, Gt, llen, FGlen, n);
				if (blocker_count_post_iter < blocker_count_pre_iter) {
					blocker_progress_iter = 1;
				}
				bad_index = 0;
				ft_hi = 0;
				ft_lo = 0;
				gt_hi = 0;
				gt_lo = 0;
				if (intermediate_get_first_shrink_blocker(
					Ft, Gt, llen, FGlen, n,
					&bad_index, &ft_hi, &ft_lo, &gt_hi, &gt_lo))
				{
					(void)bad_index;
					blocker_gap_post_iter =
						(uint64_t)kg_tail_signext_gap(ft_hi, ft_lo)
						+ (uint64_t)kg_tail_signext_gap(gt_hi, gt_lo);
					if (blocker_gap_post_iter < blocker_gap_pre_iter) {
						blocker_progress_iter = 1;
					}
				}
			}
			if (had_update && !blocker_progress_iter) {
				tail_no_topchange ++;
			} else {
				tail_no_topchange = 0;
			}
		} else if (q == 3329 && depth >= 7 && pseudo_tail_iter) {
			if (had_update && FGlen > babai_slen
				&& !top_word_changed_iter && !can_shrink_iter)
			{
				tail_no_topchange ++;
			} else {
				tail_no_topchange = 0;
			}
		}
		effective_update_iter = had_update;
		if (effective_update_iter
			&& q == 3329 && depth >= 7
			&& tail_stage_iter && FGlen_before_iter > babai_slen
			&& !blocker_progress_iter
			&& !top_word_changed_iter
			&& !can_shrink_iter)
		{
			effective_update_iter = 0;
		}
		/*
		 * We now assume that (F,G) has shrunk by REDUCE_BITS
		 * (but they have not gone lower than (f,g)). We also
		 * adjust FGlen accordingly.
		 */
		if (q == 3329 && depth >= 7 && depth <= 9
			&& FGlen > babai_slen
			&& !top_word_changed_iter
			&& (iter_count & 31u) == 0u
			&& !can_shrink_iter
			&& (current_scale_k == 0
				|| tail_no_topchange >= 64
				|| tail_no_update >= 64
				|| pseudo_tail_iter))
		{
			memset(k_escape, 0, sizeof k_escape);
			if (kg_tail_try_escape(Ft, Gt, llen, FGlen,
				ft, gt, apply_flen, sstride,
				k_escape, scale_k, logn))
			{
				uint32_t bad_index;

				intermediate_fill_sign_extension(Ft, Gt, llen, FGlen, llen, n);
				if (!intermediate_check_modp(ft, gt, sstride, sstride,
					Ft, Gt, llen, llen, q, logn, t1, &bad_index))
				{
					for (u = 0; u < n; u ++) {
						k_escape[u] = -k_escape[u];
					}
					poly_sub_scaled(Ft, FGlen, llen, ft, apply_flen, sstride,
						k_escape, scale_k, logn);
					poly_sub_scaled(Gt, FGlen, llen, gt, apply_flen, sstride,
						k_escape, scale_k, logn);
					intermediate_fill_sign_extension(Ft, Gt, llen, FGlen, llen, n);
				} else {
					tail_no_update = 0;
					tail_no_topchange = 0;
					continue;
				}
			}
		}
		if (scale_FG <= scale_fg) {
			if (prof->tail_keep_reducing_from_depth != PROFILE_DEPTH_NONE
				&& depth >= prof->tail_keep_reducing_from_depth
				&& FGlen > babai_slen)
			{
				if (q == 3329 && current_scale_k != 0
					&& fg_tail_flen < babai_slen
					&& tail_no_topchange >= tail_no_topchange_limit
					&& rlen_cur != fg_tail_flen)
				{
					rlen_base = fg_tail_flen;
					need_rebuild = 1;
					tail_no_topchange = 0;
					tail_no_update = 0;
					continue;
				}
				/*
				 * Depth 7 is still using a narrow public window in the
				 * initial Babai pass. Once we are in the final-scale tail,
				 * switch to the full output width so that the quotient is
				 * rebuilt against the same scale target as deeper levels.
				 */
				if (q == 3329 && current_scale_k == 0
					&& rlen_cur != update_flen)
				{
					rlen_base = update_flen;
					need_rebuild = 1;
					tail_no_update = 0;
					continue;
				}
				if (effective_update_iter) {
					tail_no_update = 0;
				} else {
					tail_no_update ++;
				}
				if (iter_count < tail_iter_limit
					&& tail_no_update < tail_no_update_limit
					&& !(q == 3329 && depth >= 7
						&& pseudo_tail_iter
						&& tail_no_topchange >= tail_no_topchange_limit))
				{
					/* keep reducing at final scale */
				} else {
					if (q == 3329 && depth >= 7
						&& pseudo_tail_iter
						&& scale_FG_eff > scale_fg
						&& tail_resync_count < 8)
					{
						scale_FG = scale_FG_eff;
						rlen_base = (fg_tail_flen < babai_slen)
							? fg_tail_flen : babai_slen;
						need_rebuild = 1;
						tail_no_update = 0;
						tail_no_topchange = 0;
						tail_resync_count ++;
						continue;
					}
					kg_record_update_stall(depth, had_update,
						top_word_changed_iter, FGlen_before_iter, FGlen);
					stall_counted = 1;
					if (q == 3329 && depth == 7) {
						kg_record_tail_exit(&kg_d7_tail_exit,
							(iter_count >= tail_iter_limit)
								? KG_TAIL_REASON_ITER_LIMIT
								: KG_TAIL_REASON_NOUPDATE_LIMIT,
							iter_count, slen, sstride, llen, FGlen, rlen_cur,
							scale_fg, scale_FG, scale_FG_eff, scale_k,
							scale_x, scale_t, reduce_bits,
							tlen, toff, k_nz_iter, top_word_changed_iter,
							top_word_change_count_iter,
							top_word_change_first_index_iter,
							top_word_change_ft_pre_iter,
							top_word_change_ft_post_iter,
							top_word_change_gt_pre_iter,
							top_word_change_gt_post_iter,
							probe_attempted_iter, probe_selected_iter,
							probe_flen_iter, probe_scale_fg_iter,
							probe_scale_k_iter, probe_scale_x_iter,
							probe_scale_t_iter, probe_tlen_iter, probe_toff_iter,
							probe_k_nonzero_iter,
							cur_rt3_raw_iter, cur_rt4_raw_iter, cur_rt5_raw_iter,
							probe_rt3_raw_iter, probe_rt4_raw_iter, probe_rt5_raw_iter,
							probe_snap_n_iter, probe_k_snapshot_iter,
							probe_rt2_raw_snapshot_iter,
							probe_cur_gap_iter, probe_alt_gap_iter,
							other_strong_progress_iter,
							other_direct_modp_ok_iter,
							other_exact_ok_iter, other_exact_modp_ok_iter,
							cur_exact_ok_iter, cur_exact_modp_ok_iter,
							probe_exact_ok_iter, probe_exact_modp_ok_iter,
							other_exact_gap_iter,
							cur_exact_gap_iter,
							probe_exact_gap_iter,
							other_exact_bad_index_iter,
							other_exact_expected_iter,
							other_exact_actual_iter,
							cur_exact_bad_index_iter,
							cur_exact_expected_iter,
							cur_exact_actual_iter,
							probe_exact_bad_index_iter,
							probe_exact_expected_iter,
							probe_exact_actual_iter,
							tail_no_update, Ft, Gt, n,
							ft, gt, apply_flen,
							(apply_flen != fg_tail_flen)
								? fg_tail_flen : babai_slen,
							sstride, k,
							rt2_raw_snapshot, rt2_round_snapshot, rt2_snap_n, logn);
					}
					if (q == 3329 && depth == 8) {
						kg_record_tail_exit(&kg_d8_tail_exit,
							(iter_count >= tail_iter_limit)
								? KG_TAIL_REASON_ITER_LIMIT
								: KG_TAIL_REASON_NOUPDATE_LIMIT,
							iter_count, slen, sstride, llen, FGlen, rlen_cur,
							scale_fg, scale_FG, scale_FG_eff, scale_k,
							scale_x, scale_t, reduce_bits,
							tlen, toff, k_nz_iter, top_word_changed_iter,
							top_word_change_count_iter,
							top_word_change_first_index_iter,
							top_word_change_ft_pre_iter,
							top_word_change_ft_post_iter,
							top_word_change_gt_pre_iter,
							top_word_change_gt_post_iter,
							probe_attempted_iter, probe_selected_iter,
							probe_flen_iter, probe_scale_fg_iter,
							probe_scale_k_iter, probe_scale_x_iter,
							probe_scale_t_iter, probe_tlen_iter, probe_toff_iter,
							probe_k_nonzero_iter,
							cur_rt3_raw_iter, cur_rt4_raw_iter, cur_rt5_raw_iter,
							probe_rt3_raw_iter, probe_rt4_raw_iter, probe_rt5_raw_iter,
							probe_snap_n_iter, probe_k_snapshot_iter,
							probe_rt2_raw_snapshot_iter,
							probe_cur_gap_iter, probe_alt_gap_iter,
							other_strong_progress_iter,
							other_direct_modp_ok_iter,
							other_exact_ok_iter, other_exact_modp_ok_iter,
							cur_exact_ok_iter, cur_exact_modp_ok_iter,
							probe_exact_ok_iter, probe_exact_modp_ok_iter,
							other_exact_gap_iter,
							cur_exact_gap_iter,
							probe_exact_gap_iter,
							other_exact_bad_index_iter,
							other_exact_expected_iter,
							other_exact_actual_iter,
							cur_exact_bad_index_iter,
							cur_exact_expected_iter,
							cur_exact_actual_iter,
							probe_exact_bad_index_iter,
							probe_exact_expected_iter,
							probe_exact_actual_iter,
							tail_no_update, Ft, Gt, n,
							ft, gt, apply_flen,
							(apply_flen != fg_tail_flen)
								? fg_tail_flen : babai_slen,
							sstride, k,
							rt2_raw_snapshot, rt2_round_snapshot, rt2_snap_n, logn);
					}
					if (q == 3329 && depth == 9) {
						kg_record_tail_exit(&kg_d9_tail_exit,
							(iter_count >= tail_iter_limit)
								? KG_TAIL_REASON_ITER_LIMIT
								: KG_TAIL_REASON_NOUPDATE_LIMIT,
							iter_count, slen, sstride, llen, FGlen, rlen_cur,
							scale_fg, scale_FG, scale_FG_eff, scale_k,
							scale_x, scale_t, reduce_bits,
							tlen, toff, k_nz_iter, top_word_changed_iter,
							top_word_change_count_iter,
							top_word_change_first_index_iter,
							top_word_change_ft_pre_iter,
							top_word_change_ft_post_iter,
							top_word_change_gt_pre_iter,
							top_word_change_gt_post_iter,
							probe_attempted_iter, probe_selected_iter,
							probe_flen_iter, probe_scale_fg_iter,
							probe_scale_k_iter, probe_scale_x_iter,
							probe_scale_t_iter, probe_tlen_iter, probe_toff_iter,
							probe_k_nonzero_iter,
							cur_rt3_raw_iter, cur_rt4_raw_iter, cur_rt5_raw_iter,
							probe_rt3_raw_iter, probe_rt4_raw_iter, probe_rt5_raw_iter,
							probe_snap_n_iter, probe_k_snapshot_iter,
							probe_rt2_raw_snapshot_iter,
							probe_cur_gap_iter, probe_alt_gap_iter,
							other_strong_progress_iter,
							other_direct_modp_ok_iter,
							other_exact_ok_iter, other_exact_modp_ok_iter,
							cur_exact_ok_iter, cur_exact_modp_ok_iter,
							probe_exact_ok_iter, probe_exact_modp_ok_iter,
							other_exact_gap_iter,
							cur_exact_gap_iter,
							probe_exact_gap_iter,
							other_exact_bad_index_iter,
							other_exact_expected_iter,
							other_exact_actual_iter,
							cur_exact_bad_index_iter,
							cur_exact_expected_iter,
							cur_exact_actual_iter,
							probe_exact_bad_index_iter,
							probe_exact_expected_iter,
							probe_exact_actual_iter,
							tail_no_update, Ft, Gt, n,
							ft, gt, apply_flen,
							(apply_flen != fg_tail_flen)
								? fg_tail_flen : babai_slen,
							sstride, k,
							rt2_raw_snapshot, rt2_round_snapshot, rt2_snap_n, logn);
					}
					break;
				}
			} else {
				kg_record_update_stall(depth, had_update,
					top_word_changed_iter, FGlen_before_iter, FGlen);
				stall_counted = 1;
				if (q == 3329 && depth == 7) {
					kg_record_tail_exit(&kg_d7_tail_exit,
						KG_TAIL_REASON_SCALE_DONE,
						iter_count, slen, sstride, llen, FGlen, rlen_cur,
						scale_fg, scale_FG, scale_FG_eff, scale_k,
						scale_x, scale_t, reduce_bits,
						tlen, toff, k_nz_iter, top_word_changed_iter,
						top_word_change_count_iter,
						top_word_change_first_index_iter,
						top_word_change_ft_pre_iter,
						top_word_change_ft_post_iter,
						top_word_change_gt_pre_iter,
						top_word_change_gt_post_iter,
						probe_attempted_iter, probe_selected_iter,
						probe_flen_iter, probe_scale_fg_iter,
						probe_scale_k_iter, probe_scale_x_iter,
						probe_scale_t_iter, probe_tlen_iter, probe_toff_iter,
						probe_k_nonzero_iter,
						cur_rt3_raw_iter, cur_rt4_raw_iter, cur_rt5_raw_iter,
						probe_rt3_raw_iter, probe_rt4_raw_iter, probe_rt5_raw_iter,
						probe_snap_n_iter, probe_k_snapshot_iter,
						probe_rt2_raw_snapshot_iter,
						probe_cur_gap_iter, probe_alt_gap_iter,
						other_strong_progress_iter,
						other_direct_modp_ok_iter,
						other_exact_ok_iter, other_exact_modp_ok_iter,
						cur_exact_ok_iter, cur_exact_modp_ok_iter,
						probe_exact_ok_iter, probe_exact_modp_ok_iter,
						other_exact_gap_iter,
						cur_exact_gap_iter,
						probe_exact_gap_iter,
						other_exact_bad_index_iter,
						other_exact_expected_iter,
						other_exact_actual_iter,
						cur_exact_bad_index_iter,
						cur_exact_expected_iter,
						cur_exact_actual_iter,
						probe_exact_bad_index_iter,
						probe_exact_expected_iter,
						probe_exact_actual_iter,
						tail_no_update, Ft, Gt, n,
						ft, gt, apply_flen,
						(apply_flen != fg_tail_flen)
							? fg_tail_flen : babai_slen,
						sstride, k,
						rt2_raw_snapshot, rt2_round_snapshot, rt2_snap_n, logn);
				}
				if (q == 3329 && depth == 8) {
					kg_record_tail_exit(&kg_d8_tail_exit,
						KG_TAIL_REASON_SCALE_DONE,
						iter_count, slen, sstride, llen, FGlen, rlen_cur,
						scale_fg, scale_FG, scale_FG_eff, scale_k,
						scale_x, scale_t, reduce_bits,
						tlen, toff, k_nz_iter, top_word_changed_iter,
						top_word_change_count_iter,
						top_word_change_first_index_iter,
						top_word_change_ft_pre_iter,
						top_word_change_ft_post_iter,
						top_word_change_gt_pre_iter,
						top_word_change_gt_post_iter,
						probe_attempted_iter, probe_selected_iter,
						probe_flen_iter, probe_scale_fg_iter,
						probe_scale_k_iter, probe_scale_x_iter,
						probe_scale_t_iter, probe_tlen_iter, probe_toff_iter,
						probe_k_nonzero_iter,
						cur_rt3_raw_iter, cur_rt4_raw_iter, cur_rt5_raw_iter,
						probe_rt3_raw_iter, probe_rt4_raw_iter, probe_rt5_raw_iter,
						probe_snap_n_iter, probe_k_snapshot_iter,
						probe_rt2_raw_snapshot_iter,
						probe_cur_gap_iter, probe_alt_gap_iter,
						other_strong_progress_iter,
						other_direct_modp_ok_iter,
						other_exact_ok_iter, other_exact_modp_ok_iter,
						cur_exact_ok_iter, cur_exact_modp_ok_iter,
						probe_exact_ok_iter, probe_exact_modp_ok_iter,
						other_exact_gap_iter,
						cur_exact_gap_iter,
						probe_exact_gap_iter,
						other_exact_bad_index_iter,
						other_exact_expected_iter,
						other_exact_actual_iter,
						cur_exact_bad_index_iter,
						cur_exact_expected_iter,
						cur_exact_actual_iter,
						probe_exact_bad_index_iter,
						probe_exact_expected_iter,
						probe_exact_actual_iter,
						tail_no_update, Ft, Gt, n,
						ft, gt, apply_flen,
						(apply_flen != fg_tail_flen)
							? fg_tail_flen : babai_slen,
						sstride, k,
						rt2_raw_snapshot, rt2_round_snapshot, rt2_snap_n, logn);
				}
				if (q == 3329 && depth == 9) {
					kg_record_tail_exit(&kg_d9_tail_exit,
						KG_TAIL_REASON_SCALE_DONE,
						iter_count, slen, sstride, llen, FGlen, rlen_cur,
						scale_fg, scale_FG, scale_FG_eff, scale_k,
						scale_x, scale_t, reduce_bits,
						tlen, toff, k_nz_iter, top_word_changed_iter,
						top_word_change_count_iter,
						top_word_change_first_index_iter,
						top_word_change_ft_pre_iter,
						top_word_change_ft_post_iter,
						top_word_change_gt_pre_iter,
						top_word_change_gt_post_iter,
						probe_attempted_iter, probe_selected_iter,
						probe_flen_iter, probe_scale_fg_iter,
						probe_scale_k_iter, probe_scale_x_iter,
						probe_scale_t_iter, probe_tlen_iter, probe_toff_iter,
						probe_k_nonzero_iter,
						cur_rt3_raw_iter, cur_rt4_raw_iter, cur_rt5_raw_iter,
						probe_rt3_raw_iter, probe_rt4_raw_iter, probe_rt5_raw_iter,
						probe_snap_n_iter, probe_k_snapshot_iter,
						probe_rt2_raw_snapshot_iter,
						probe_cur_gap_iter, probe_alt_gap_iter,
						other_strong_progress_iter,
						other_direct_modp_ok_iter,
						other_exact_ok_iter, other_exact_modp_ok_iter,
						cur_exact_ok_iter, cur_exact_modp_ok_iter,
						probe_exact_ok_iter, probe_exact_modp_ok_iter,
						other_exact_gap_iter,
						cur_exact_gap_iter,
						probe_exact_gap_iter,
						other_exact_bad_index_iter,
						other_exact_expected_iter,
						other_exact_actual_iter,
						cur_exact_bad_index_iter,
						cur_exact_expected_iter,
						cur_exact_actual_iter,
						probe_exact_bad_index_iter,
						probe_exact_expected_iter,
						probe_exact_actual_iter,
						tail_no_update, Ft, Gt, n,
						ft, gt, apply_flen,
						(apply_flen != fg_tail_flen)
							? fg_tail_flen : babai_slen,
						sstride, k,
						rt2_raw_snapshot, rt2_round_snapshot, rt2_snap_n, logn);
				}
				break;
			}
		}
		if (scale_FG <= (scale_fg + reduce_bits)) {
			scale_FG = scale_fg;
		} else {
			scale_FG -= reduce_bits;
		}
		while (FGlen > babai_slen
			&& 31 * (FGlen - babai_slen) > scale_FG - scale_fg + 30
			)
		{
			if (!intermediate_can_shrink_fglen(Ft, Gt, llen, FGlen, n)) {
				break;
			}
			if (CTL_TRACK_SHRINK_BLOCK && q == 3329 && depth == 9) {
				uint32_t bad;

				bad = intermediate_count_shrink_blockers(Ft, Gt, llen, FGlen, n);
				if (bad != 0) {
					kg_d9_shrink_block_events ++;
					kg_d9_shrink_block_total_bad += bad;
					kg_d9_shrink_block_last_bad = bad;
					(void)intermediate_get_first_shrink_blocker(
						Ft, Gt, llen, FGlen, n,
						&kg_d9_shrink_block_index,
						&kg_d9_shrink_block_ft_hi,
						&kg_d9_shrink_block_ft_lo,
						&kg_d9_shrink_block_gt_hi,
						&kg_d9_shrink_block_gt_lo);
				}
			}
			if (CTL_TRACK_SHRINK_BLOCK && q == 3329 && depth == 6) {
				uint32_t bad;

				bad = intermediate_count_shrink_blockers(Ft, Gt, llen, FGlen, n);
				if (bad != 0) {
					kg_d6_shrink_block_events ++;
					kg_d6_shrink_block_total_bad += bad;
					kg_d6_shrink_block_last_bad = bad;
					(void)intermediate_get_first_shrink_blocker(
						Ft, Gt, llen, FGlen, n,
						&kg_d6_shrink_block_index,
						&kg_d6_shrink_block_ft_hi,
						&kg_d6_shrink_block_ft_lo,
						&kg_d6_shrink_block_gt_hi,
						&kg_d6_shrink_block_gt_lo);
				}
			}
			if (CTL_TRACK_SHRINK_BLOCK && q == 3329 && depth == 5) {
				uint32_t bad;

				bad = intermediate_count_shrink_blockers(Ft, Gt, llen, FGlen, n);
				if (bad != 0) {
					kg_d5_shrink_block_events ++;
					kg_d5_shrink_block_total_bad += bad;
					kg_d5_shrink_block_last_bad = bad;
					(void)intermediate_get_first_shrink_blocker(
						Ft, Gt, llen, FGlen, n,
						&kg_d5_shrink_block_index,
						&kg_d5_shrink_block_ft_hi,
						&kg_d5_shrink_block_ft_lo,
						&kg_d5_shrink_block_gt_hi,
						&kg_d5_shrink_block_gt_lo);
				}
			}
			if (CTL_TRACK_SHRINK_BLOCK && q == 3329 && depth == 4) {
				uint32_t bad;

				bad = intermediate_count_shrink_blockers(Ft, Gt, llen, FGlen, n);
				if (bad != 0) {
					kg_d4_shrink_block_events ++;
					kg_d4_shrink_block_total_bad += bad;
					kg_d4_shrink_block_last_bad = bad;
					(void)intermediate_get_first_shrink_blocker(
						Ft, Gt, llen, FGlen, n,
						&kg_d4_shrink_block_index,
						&kg_d4_shrink_block_ft_hi,
						&kg_d4_shrink_block_ft_lo,
						&kg_d4_shrink_block_gt_hi,
						&kg_d4_shrink_block_gt_lo);
				}
			}
			if (CTL_TRACK_SHRINK_BLOCK && q == 3329 && depth == 2) {
				uint32_t bad;

				bad = intermediate_count_shrink_blockers(Ft, Gt, llen, FGlen, n);
				if (bad != 0) {
					kg_d2_shrink_block_events ++;
					kg_d2_shrink_block_total_bad += bad;
					kg_d2_shrink_block_last_bad = bad;
					(void)intermediate_get_first_shrink_blocker(
						Ft, Gt, llen, FGlen, n,
						&kg_d2_shrink_block_index,
						&kg_d2_shrink_block_ft_hi,
						&kg_d2_shrink_block_ft_lo,
						&kg_d2_shrink_block_gt_hi,
						&kg_d2_shrink_block_gt_lo);
				}
			}
			FGlen --;
			if (q == 3329 && depth >= 7) {
				tail_resync_count = 0;
			}
			if (q == 3329 && depth == 9) {
				kg_d9_fglen_shrink_count ++;
				if (kg_d9_fglen_min == 0 || (uint32_t)FGlen < kg_d9_fglen_min) {
					kg_d9_fglen_min = (uint32_t)FGlen;
				}
			}
			if (q == 3329 && depth == 6) {
				kg_d6_fglen_shrink_count ++;
				if (kg_d6_fglen_min == 0 || (uint32_t)FGlen < kg_d6_fglen_min) {
					kg_d6_fglen_min = (uint32_t)FGlen;
				}
			}
			if (q == 3329 && depth == 5) {
				kg_d5_fglen_shrink_count ++;
				if (kg_d5_fglen_min == 0 || (uint32_t)FGlen < kg_d5_fglen_min) {
					kg_d5_fglen_min = (uint32_t)FGlen;
				}
			}
		}
		if (!stall_counted) {
			kg_record_update_stall(depth, had_update,
				top_word_changed_iter, FGlen_before_iter, FGlen);
		}

	}

	/*
	 * If the active length is smaller than the profile output width, then
	 * explicitly fill the extra top words with sign extension before we
	 * test/output the compressed representation.
	 */
	if (FGlen < sstride) {
		intermediate_fill_sign_extension(Ft, Gt, llen, FGlen, sstride, n);
	}

	/*
	 * Early detect of failures after Babai reduction.
	 */
	{
		uint32_t bad_index;
		if (!intermediate_check_modp(ft, gt, sstride, sstride,
			Ft, Gt, llen, sstride,
			q, logn, t1, &bad_index))
		{
			int is_trunc;

			kg_diag_stats.solve_fg_fail_intermediate_check_post ++;
			intermediate_fill_sign_extension(Ft, Gt, llen, FGlen, llen, n);
			is_trunc = intermediate_check_modp(ft, gt, sstride, sstride,
				Ft, Gt, llen, llen,
				q, logn, t1, NULL)
				? 1 : 0;
			if (is_trunc) {
				kg_diag_stats.solve_fg_fail_intermediate_check_trunc ++;
				kg_last_inter_stage = 2;
			} else {
				kg_diag_stats.solve_fg_fail_intermediate_check_true ++;
				kg_last_inter_stage = 3;
				if (!kg_s3_snap_valid) {
					size_t of_s, of_l;

					kg_s3_snap_valid = 1;
					kg_s3_snap_depth = depth;
					kg_s3_snap_logn = logn;
					kg_s3_snap_p = kg_last_modp_p;
					kg_s3_snap_slen = (uint32_t)slen;
					kg_s3_snap_llen = (uint32_t)llen;
					kg_s3_snap_chklen = (uint32_t)sstride;
					kg_s3_snap_bad_index = bad_index;
					kg_s3_snap_expected = kg_last_modp_expected;
					kg_s3_snap_actual = kg_last_modp_actual;
					kg_s3_snap_mismatch_count = kg_last_modp_mismatch_count;
					kg_s3_snap_term_fG = kg_last_modp_term_fG;
					kg_s3_snap_term_gF = kg_last_modp_term_gF;
					kg_s3_snap_iter = iter_count;
					kg_s3_snap_fglen = (uint32_t)FGlen;
					kg_s3_snap_scale_fg = scale_fg;
					kg_s3_snap_scale_FG = scale_FG;
					kg_s3_snap_scale_k = current_scale_k;
					of_s = (size_t)bad_index * slen;
					of_l = (size_t)bad_index * llen;
					kg_s3_snap_ft_lo = ft[of_s];
					kg_s3_snap_ft_hi = ft[of_s + (slen - 1)];
					kg_s3_snap_gt_lo = gt[of_s];
					kg_s3_snap_gt_hi = gt[of_s + (slen - 1)];
					kg_s3_snap_Ft_lo = Ft[of_l];
					kg_s3_snap_Ft_hi = Ft[of_l + (llen - 1)];
					kg_s3_snap_Gt_lo = Gt[of_l];
					kg_s3_snap_Gt_hi = Gt[of_l + (llen - 1)];
					if (kg_s3_last_round.valid && !kg_s3_snap_round.valid) {
						kg_s3_snap_round = kg_s3_last_round;
					}
				}
			}
			kg_diag_stats.solve_fg_fail_intermediate_check ++;
			kg_last_inter_depth = depth;
			kg_last_inter_index = bad_index;
			kg_last_inter_slen = (uint32_t)slen;
			kg_last_inter_fglen = (uint32_t)FGlen;
			return 0;
		}
	}

	/*
	 * Compress encoding of all values back to the profile output width.
	 * The reduction itself may use a shorter active length internally,
	 * but callers still expect the original per-depth allocation.
	 */
	for (u = 0, x = tmp, y = tmp;
		u < (n << 1); u ++, x += sstride, y += llen)
	{
		memmove(x, y, sstride * sizeof *y);
	}

	return 1;
}

/*
 * Solving the NTRU equation, depth 1. Upon entry, the F and G
 * from the previous level should be in the tmp[] array.
 *
 * Returned value: 1 on success, 0 on error.
 */
static int
solve_NTRU_depth1(unsigned logn_top,
	const int8_t *f, const int8_t *g, uint32_t q, uint32_t *tmp)
{
	/*
	 * In this function, 'logn' is the log2 of the degree for
	 * this step. If N = 2^logn, then:
	 *  - the F and G values already in fk->tmp (from the deeper
	 *    levels) have degree N/2;
	 *  - this function should return F and G of degree N.
	 */
	unsigned logn;
	size_t n, hn, n_top, dlen, u, v;
	uint32_t *Fd, *Gd, *Ft, *Gt, *ft, *gt, *gm, *igm, *ht, *Ht, *t1, *t2;
	fnr *rt1, *rt2;
	uint32_t p, p0i, R2, R3, Rx;
	uint32_t extra[2];
	const ctl_solve_profile *prof;

	logn = logn_top - 1;
	n = (size_t)1 << logn;
	hn = n >> 1;
	n_top = (size_t)1 << logn_top;

	/*
	 * dlen = size of the F and G obtained from the deeper level
	 *        (degree N/2)
	 */
	prof = ctl_get_solve_profile(q);
	if (prof == NULL || logn_top > prof->max_logn || prof->max_bl_small_len <= 2) {
		return 0;
	}
	dlen = prof->max_bl_small[2];

	/*
	 * Fd and Gd are the F and G from the deeper level.
	 */
	Fd = tmp;
	Gd = Fd + dlen * hn;

	/*
	 * Compute the input f and g for this level. We can do it with
	 * the NTT and only one modulus because the result will easily
	 * fit (each coefficient is within +/-2^12 with very high
	 * probability).
	 */
	ft = Gd + dlen * hn;
	gt = ft + n_top;
	gm = gt + n_top;
	igm = gm + n_top;
	p = PRIMES[0].p;
	p0i = modp_ninv31(p);
	R2 = modp_R2(p, p0i);
	modp_mkgm(gm, igm, logn_top, PRIMES[0].g, p, p0i);
	for (u = 0; u < n_top; u ++) {
		ft[u] = modp_set(f[u], p);
		gt[u] = modp_set(g[u], p);
	}
	modp_NTT(ft, gm, logn_top, p, p0i);
	modp_NTT(gt, gm, logn_top, p, p0i);
	modp_poly_rec_res(ft, logn_top, p, p0i, R2);
	modp_poly_rec_res(gt, logn_top, p, p0i, R2);
	memmove(ft + n, gt, n * sizeof *gt);
	gt = ft + n;

	/*
	 * Move things to have room for our candidates F and G (unreduced,
	 * need two words per coefficient); Fd and Gd are moved _after_
	 * ft and gt.
	 *
	 *   Ft   2*n
	 *   Gt   2*n
	 *   ft   n
	 *   gt   n
	 *   Fd   (n/2)*dlen  (n or n/2)
	 *   Gd   (n/2)*dlen  (n or n/2)
	 */
	Ft = tmp;
	Gt = Ft + 2 * n;
	t1 = Gt + 2 * n;
	memmove(t1, ft, 2 * n * sizeof *ft);
	ft = t1;
	gt = t1 + n;
	t1 = gt + n;
	memmove(t1, Fd, 2 * hn * dlen * sizeof *Fd);
	Fd = t1;
	Gd = Fd + hn * dlen;

	/*
	 * We reduce Fd and Gd modulo the two prime moduli we will use,
	 * and store the values in Ft and Gt (only n/2 values in each).
	 */
	for (u = 0; u < 2; u ++) {
		p = PRIMES[u].p;
		p0i = modp_ninv31(p);
		R2 = modp_R2(p, p0i);
		Rx = modp_Rx((unsigned)dlen, p, p0i, R2);
		for (v = 0; v < hn; v ++) {
			Ft[(v << 1) + u] = zint_mod_small_signed(
				&Fd[v * dlen], dlen, p, p0i, R2, Rx);
			Gt[(v << 1) + u] = zint_mod_small_signed(
				&Gd[v * dlen], dlen, p, p0i, R2, Rx);
		}
	}

	/*
	 * We do not need Fd and Gd after that point.
	 *
	 *   Ft   2*n
	 *   Gt   2*n
	 *   ft   n
	 *   gt   n
	 *   gm   n
	 *   igm  n
	 */
	gm = t1;
	igm = gm + n;

	/*
	 * Compute our F and G modulo the two first prime moduli.
	 */
	for (u = 0; u < 2; u ++) {
		uint32_t *Fp, *Gp;

		/*
		 * All computations are done modulo p.
		 */
		p = PRIMES[u].p;
		p0i = modp_ninv31(p);
		R2 = modp_R2(p, p0i);

		modp_mkgm(gm, igm, logn, PRIMES[u].g, p, p0i);

		/*
		 * In the first iteration, ft and gt are already in NTT
		 * modulo the first prime. In the second iteration, they
		 * are in normal representation, and must be converted.
		 */
		if (u != 0) {
			modp_poly_set(ft, logn, p);
			modp_poly_set(gt, logn, p);
			modp_NTT(ft, gm, logn, p, p0i);
			modp_NTT(gt, gm, logn, p, p0i);
		}

		/*
		 * Get F' and G' modulo p and in NTT representation
		 * (they have degree n/2). These values were computed in
		 * a previous step, and stored in Ft and Gt.
		 */
		Fp = igm + n;
		Gp = Fp + hn;
		for (v = 0; v < hn; v ++) {
			Fp[v] = Ft[(v << 1) + u];
			Gp[v] = Gt[(v << 1) + u];
		}
		modp_NTT(Fp, gm, logn - 1, p, p0i);
		modp_NTT(Gp, gm, logn - 1, p, p0i);

		/*
		 * Compute our F and G modulo p.
		 *
		 * General case:
		 *
		 *   we divide degree by d = 2
		 *   f'(x^d) = N(f)(x^d) = f * adj(f)
		 *   g'(x^d) = N(g)(x^d) = g * adj(g)
		 *   f'*G' - g'*F' = q
		 *   F = F'(x^d) * adj(g)
		 *   G = G'(x^d) * adj(f)
		 *
		 * We compute things in the NTT. We group roots of phi
		 * such that all roots x in a group share the same x^d.
		 * If the roots in a group are x_1, x_2... x_d, then:
		 *
		 *   N(f)(x_1^d) = f(x_1)*f(x_2)*...*f(x_d)
		 *
		 * Thus, we have:
		 *
		 *   G(x_1) = f(x_2)*f(x_3)*...*f(x_d)*G'(x_1^d)
		 *   G(x_2) = f(x_1)*f(x_3)*...*f(x_d)*G'(x_1^d)
		 *   ...
		 *   G(x_d) = f(x_1)*f(x_2)*...*f(x_{d-1})*G'(x_1^d)
		 *
		 * In all cases, we can thus compute F and G in NTT
		 * representation by a few simple multiplications.
		 * Moreover, in our chosen NTT representation, roots
		 * from the same group are consecutive in RAM.
		 */
		for (v = 0; v < hn; v ++) {
			uint32_t ftA, ftB, gtA, gtB;
			uint32_t mFp, mGp;

			ftA = ft[(v << 1) + 0];
			ftB = ft[(v << 1) + 1];
			gtA = gt[(v << 1) + 0];
			gtB = gt[(v << 1) + 1];
			mFp = modp_montymul(Fp[v], R2, p, p0i);
			mGp = modp_montymul(Gp[v], R2, p, p0i);
			Ft[(v << 2) + u + 0] = modp_montymul(gtB, mFp, p, p0i);
			Ft[(v << 2) + u + 2] = modp_montymul(gtA, mFp, p, p0i);
			Gt[(v << 2) + u + 0] = modp_montymul(ftB, mGp, p, p0i);
			Gt[(v << 2) + u + 2] = modp_montymul(ftA, mGp, p, p0i);
		}

		/*
		 * Convert back ft and gt to normal representation.
		 */
		modp_iNTT(ft, igm, logn, p, p0i);
		modp_iNTT(gt, igm, logn, p, p0i);
		modp_poly_norm(ft, logn, p);
		modp_poly_norm(gt, logn, p);
	}

	/*
	 * At that point, Ft, Gt, ft and gt are consecutive in RAM (in that
	 * order).
	 *
	 *   Ft   2*n
	 *   Gt   2*n
	 *   ft   n
	 *   gt   n
	 */

	/*
	 * Apply Babai reduction on (F,G). This is specialized code that
	 * uses the fact that coefficients of (f,g) are small; notably,
	 * we can compute F*adj(f)+G*adj(g) and f*adj(f)+g*adj(g) with
	 * the NTT, over two moduli and one modulus, respectively.
	 *
	 * The NTT representation of adj(f) is obtained from the NTT
	 * representation of f by swapping the coefficients pairwise
	 * (if mm == n-1, then coefficient of index j is swapped with
	 * coefficient of index mm XOR j).
	 */

	/*
	 * Memory layout:
	 *       Ft   2*n words     F (unreduced) (NTT)
	 *       Gt   2*n words     G (unreduced) (NTT)
	 *       ft   n words       f
	 *       gt   n words       g
	 *  t1   igm  n words       iNTT table         (to be filled)
	 *       ht   n words       f*adj(f)+g*adj(g)  (to be filled)
	 *       gm   n words       NTT table          (to be filled)
	 *  rt1  Ht   2*n words     F*adj(f)+G*adj(g)  (to be filled)
	 */
	igm = t1;
	ht = igm + n;
	gm = ht + n;
	Ht = align_u32(tmp, align_fnr(tmp, gm + n));
	rt1 = align_fnr(tmp, Ht);
	rt2 = rt1 + n;

	/*
	 * Compute f*adj(f)+g*adj(g) into ht.
	 */
	p = PRIMES[0].p;
	p0i = modp_ninv31(p);
	R2 = modp_R2(p, p0i);
	modp_mkgm(gm, igm, logn, PRIMES[0].g, p, p0i);
	modp_poly_set(ft, logn, p);
	modp_poly_set(gt, logn, p);
	modp_NTT(ft, gm, logn, p, p0i);
	modp_NTT(gt, gm, logn, p, p0i);
	for (u = 0; u < n; u ++) {
		uint32_t sf, sfa, sg, sga;

		sf = modp_montymul(ft[u], R2, p, p0i);
		sfa = ft[u ^ (n - 1)];
		sg = modp_montymul(gt[u], R2, p, p0i);
		sga = gt[u ^ (n - 1)];
		ht[u] = modp_add(
			modp_montymul(sf, sfa, p, p0i),
			modp_montymul(sg, sga, p, p0i), p);
	}
	modp_iNTT(ht, igm, logn, p, p0i);
	modp_poly_norm(ht, logn, p);

	/*
	 * Compute F*adj(f)+G*adj(g) into Ht. At that point, ft and gt are
	 * in NTT for the first prime; Ft and Gt are in RNS+NTT.
	 */
	for (u = 0; u < n; u ++) {
		uint32_t sf, sfa, sg, sga;

		sf = modp_montymul(Ft[(u << 1) + 0], R2, p, p0i);
		sfa = ft[u ^ (n - 1)];
		sg = modp_montymul(Gt[(u << 1) + 0], R2, p, p0i);
		sga = gt[u ^ (n - 1)];
		Ht[(u << 1) + 0] = modp_add(
			modp_montymul(sf, sfa, p, p0i),
			modp_montymul(sg, sga, p, p0i), p);
	}
	modp_iNTT(ft, igm, logn, p, p0i);
	modp_iNTT(gt, igm, logn, p, p0i);
	modp_poly_norm(ft, logn, p);
	modp_poly_norm(gt, logn, p);
	modp_iNTT_ext(Ht + 0, 2, igm, logn, p, p0i);

	p = PRIMES[1].p;
	p0i = modp_ninv31(p);
	R2 = modp_R2(p, p0i);
	modp_mkgm(gm, igm, logn, PRIMES[1].g, p, p0i);
	modp_poly_set(ft, logn, p);
	modp_poly_set(gt, logn, p);
	modp_NTT(ft, gm, logn, p, p0i);
	modp_NTT(gt, gm, logn, p, p0i);
	for (u = 0; u < n; u ++) {
		uint32_t sf, sfa, sg, sga;

		sf = modp_montymul(Ft[(u << 1) + 1], R2, p, p0i);
		sfa = ft[u ^ (n - 1)];
		sg = modp_montymul(Gt[(u << 1) + 1], R2, p, p0i);
		sga = gt[u ^ (n - 1)];
		Ht[(u << 1) + 1] = modp_add(
			modp_montymul(sf, sfa, p, p0i),
			modp_montymul(sg, sga, p, p0i), p);
	}
	modp_iNTT_ext(Ht + 1, 2, igm, logn, p, p0i);
	zint_rebuild_CRT(Ht, 2, 2, n, PRIMES, 1, extra);

	/*
	 * From now on, we can compact Ft and Gt to a single word for
	 * each coefficient; we use the second word, i.e. the NTT
	 * representation modulo the second prime (which is the one in
	 * p, p0i, R2, gm, igm).
	 */
	for (u = 0; u < n; u ++) {
		Ft[u] = Ft[(u << 1) + 1];
	}
	for (u = 0; u < n; u ++) {
		Gt[u] = Gt[(u << 1) + 1];
	}

	/*
	 * Since we made some room, we can move data to reduce total
	 * memory usage.
	 *
	 *       Ft   n words       F (unreduced) (NTT)
	 *       Gt   n words       G (unreduced) (NTT)
	 *       ft   n words       f (NTT)
	 *       gt   n words       g (NTT)
	 *       igm  n words       iNTT table         (to be filled)
	 *  t2   ht   n words       f*adj(f)+g*adj(g)  (to be filled)
	 *       gm   n words       NTT table          (to be filled)
	 *  rt1  Ht   2*n words     F*adj(f)+G*adj(g)  (to be filled)
	 *       rt2  2*n words     temporary (fixed-point)
	 */
	memmove(Ft + n, Gt, n * sizeof *Gt);
	Gt = Ft + n;
	memmove(Gt + n, ft, 7 * n * sizeof *ft);
	ft = Gt + n;
	gt = ft + n;
	igm = gt + n;
	ht = igm + n;
	gm = ht + n;
	Ht = gm + n;
	rt1 = align_fnr(tmp, Ht);
	rt2 = rt1 + n;
	t2 = ht;

	/*
	 * At that point, ht and Ht are both in normal representation.
	 * ft and gt are still in NTT, modulo the second prime (which
	 * is in p and p0i, with tables gm and igm still filled).
	 *
	 * We convert Ht and ht into fixed point (into rt1 and rt2,
	 * respectively). Note that rt1 is an alias for Ht, so we
	 * must do the conversion of Ht into rt2 (to avoid aliasing
	 * issues) then move that value over rt1.
	 *
	 * For this round, we need to scale down values a bit when
	 * converting to fixed point, so that there is no overflow on
	 * fixed-point values (they can go up to 2^31 in absolute value)
	 * or on the coefficients of k (represented as 32-bit signed
	 * integers). However, we also need this round to return, with
	 * high probability, a polynomial with coefficients less than
	 * 2^30 in absolute value, so that we may reduce F and G to
	 * one-word representation.
	 *
	 * Thus, we scale down F*adj(f)+G*adj(g) by DOWNSCALE_LARGE bits,
	 * and f*adj(f)+g*adj(g) by DOWNSCALE_SMALL bits.
	 */

#define DOWNSCALE_LARGE   15
#define DOWNSCALE_SMALL   7

	for (u = 0; u < n; u ++) {
		uint64_t w;

		w = (uint64_t)Ht[(u << 1) + 0]
			| ((uint64_t)Ht[(u << 1) + 1] << 31);
		rt2[u] = fnr_of_scaled32(w << (32 - DOWNSCALE_LARGE));
	}
	memmove(rt1, rt2, n * sizeof *rt1);
	for (u = 0; u < n; u ++) {
		rt2[u] = fnr_div_2e(fnr_of(sign_ext(ht[u])), DOWNSCALE_SMALL);
	}

	/*
	 * Convert both values to FFT and compute the quotient into rt1.
	 */
	ctl_FFT(rt1, logn);
	ctl_FFT(rt2, logn);
	ctl_poly_div_autoadj_fft(rt1, rt2, logn);
	ctl_iFFT(rt1, logn);

	/*
	 * We now round the division result back to integers, into t2
	 * (this overwrites ht, which we do not need anymore).
	 */
	for (u = 0; u < n; u ++) {
		t2[u] = (uint32_t)fnr_round(rt1[u]) & 0x7FFFFFFF;
	}

	/*
	 * Now, subtract k*ft from Ft, and k*gt from Gt.
	 * At that point:
	 *   Ft and Gt are still in RNS+NTT
	 *   ft and gt are in NTT, using the second prime as modulus
	 *   p, p0i, R2, gm and igm are still set for the second prime
	 * Also, the result should fit in 30 bits (+sign), so we can
	 * do the whole computation modulo the second prime.
	 *
	 * Inside each loop, we need to multiply values from k (t2) by
	 * R2 (to cancel the extra effect of the Montgomery
	 * multiplication) but also by 2^t with t being the difference
	 * between DOWNSCALE_LARGE and DOWNSCALE_SMALL (scaling for k).
	 * These can be done conjointly in a single operation: we want
	 * to multiply by 2^(62+t) mod p. This value is the Montgomery
	 * representation of 2^(31+t), which we obtain by multiplying
	 * 2^31 with 2^t, both in Montgomery representation. R2 is 2^31
	 * in Montgomery representation, so we only need 2^t in
	 * Montgomery representation, which we get through another
	 * Montgomery multiplication.
	 */
	modp_poly_set(t2, logn, p);
	modp_NTT(t2, gm, logn, p, p0i);
	R3 = modp_montymul(
		1u << (DOWNSCALE_LARGE - DOWNSCALE_SMALL), R2, p, p0i);
	R3 = modp_montymul(R3, R2, p, p0i);
	for (u = 0; u < n; u ++) {
		uint32_t x;

		x = modp_montymul(t2[u], R3, p, p0i);
		Ft[u] = modp_sub(Ft[u],
			modp_montymul(x, ft[u], p, p0i), p);
		Gt[u] = modp_sub(Gt[u],
			modp_montymul(x, gt[u], p, p0i), p);
	}

	/*
	 * At that point:
	 *
	 *  - p, p0i, R2, gm, igm are set on the second prime.
	 *
	 *  - Ft and Gt contain the partially reduced F and G, in
	 *    NTT modulo the second prime.
	 *
	 *  - ft and gt are in NTT modulo the second prime.
	 *
	 *  - rt2 contains f*adj(f)+g*adj(g) in FFT (scaled DOWNSCALE_SMALL).
	 *
	 *  - rt1 and t2 are free.
	 *
	 * We compute F*adj(f)+G*adj(g) into t2 (NTT, then normal).
	 */
	for (u = 0; u < n; u ++) {
		uint32_t sf, sfa, sg, sga;

		sf = modp_montymul(Ft[u], R2, p, p0i);
		sfa = ft[u ^ (n - 1)];
		sg = modp_montymul(Gt[u], R2, p, p0i);
		sga = gt[u ^ (n - 1)];
		t2[u] = modp_add(
			modp_montymul(sf, sfa, p, p0i),
			modp_montymul(sg, sga, p, p0i), p);
	}
	modp_iNTT(t2, igm, logn, p, p0i);
	modp_poly_norm(t2, logn, p);

	/*
	 * Get F*adj(f)+G*adj(g) into fixed-point, then divide by
	 * f*adj(f)+g*adj(g) (which is still in rt2, in FFT) and
	 * round it back to integers (in t2) to get the reduction
	 * polynomial k.
	 *
	 * Since f*adj(f)+g*adj(g) is scaled down by 2^DOWNSCALE_SMALL,
	 * we must also scale down F*adj(f)+G*adj(j) by
	 * 2^DOWNSCALE_SMALL.
	 */
	for (u = 0; u < n; u ++) {
		rt1[u] = fnr_div_2e(fnr_of(sign_ext(t2[u])), DOWNSCALE_SMALL);
	}
	ctl_FFT(rt1, logn);
	ctl_poly_div_autoadj_fft(rt1, rt2, logn);
	ctl_iFFT(rt1, logn);
	for (u = 0; u < n; u ++) {
		t2[u] = (uint32_t)fnr_round(rt1[u]) & 0x7FFFFFFF;
	}

	/*
	 * Subtract k*f from F and k*g from G. F, G, f and g are already
	 * in NTT, but not k (in t2).
	 */
	modp_poly_set(t2, logn, p);
	modp_NTT(t2, gm, logn, p, p0i);
	for (u = 0; u < n; u ++) {
		uint32_t x;

		x = modp_montymul(t2[u], R2, p, p0i);
		Ft[u] = modp_sub(Ft[u],
			modp_montymul(x, ft[u], p, p0i), p);
		Gt[u] = modp_sub(Gt[u],
			modp_montymul(x, gt[u], p, p0i), p);
	}

	/*
	 * Get back F and G in normal representation.
	 */
	modp_iNTT(Ft, igm, logn, p, p0i);
	modp_iNTT(Gt, igm, logn, p, p0i);
	modp_poly_norm(Ft, logn, p);
	modp_poly_norm(Gt, logn, p);

	return 1;
}

/*
 * Solving the NTRU equation at depth 0. Upon entry, the F anf G
 * from the previous level should be in the tmp[] array.
 *
 * Returned value: 1 on success, 0 on error.
 */
static int
solve_NTRU_depth0(unsigned logn,
	const int8_t *f, const int8_t *g, uint32_t q, uint32_t *tmp)
{
	size_t n, hn, u, dlen;
	uint32_t p, p0i, R2;
	uint32_t *Fp, *Gp, *t1, *t2, *t3, *t4;
	uint32_t *gm, *igm, *ft, *gt;
	fnr *rt2, *rt3;
	const ctl_solve_profile *prof;

	n = (size_t)1 << logn;
	hn = n >> 1;

	/*
	 * The (F,G) from the upper level may use more than one word, but
	 * they should fit on a single word each at this point, so we
	 * compact them if necessary.
	 */
	prof = ctl_get_solve_profile(q);
	if (prof == NULL || prof->max_bl_small_len <= 1) {
		return 0;
	}
	dlen = prof->max_bl_small[1];
	if (dlen > 1) {
		size_t v;

		for (u = 1, v = dlen; u < n; u ++, v += dlen) {
			tmp[u] = tmp[v];
		}
	}

	/*
	 * Equations are:
	 *
	 *   f' = f0^2 - X^2*f1^2
	 *   g' = g0^2 - X^2*g1^2
	 *   F' and G' are a solution to f'G' - g'F' = q (from deeper levels)
	 *   F = F'*(g0 - X*g1)
	 *   G = G'*(f0 - X*f1)
	 *
	 * f0, f1, g0, g1, f', g', F' and G' are all "compressed" to
	 * degree N/2 (their odd-indexed coefficients are all zero).
	 *
	 * Everything should fit in 31-bit integers, hence we can just use
	 * the first small prime p = 2147473409.
	 */
	p = PRIMES[0].p;
	p0i = modp_ninv31(p);
	R2 = modp_R2(p, p0i);

	Fp = tmp;
	Gp = Fp + hn;
	ft = Gp + hn;
	gt = ft + n;
	gm = gt + n;
	igm = gm + n;

	/*
	 * Load f and g.
	 */
	modp_mkgm(gm, igm, logn, PRIMES[0].g, p, p0i);
	for (u = 0; u < n; u ++) {
		ft[u] = modp_set(f[u], p);
		gt[u] = modp_set(g[u], p);
	}
	modp_NTT(ft, gm, logn, p, p0i);
	modp_NTT(gt, gm, logn, p, p0i);

	/*
	 * Load F' and G' and convert them to NTT representation.
	 */
	modp_poly_set(Fp, logn - 1, p);
	modp_poly_set(Gp, logn - 1, p);
	modp_NTT(Fp, gm, logn - 1, p, p0i);
	modp_NTT(Gp, gm, logn - 1, p, p0i);

	/*
	 * Build the unreduced F,G in ft and gt.
	 */
	for (u = 0; u < n; u += 2) {
		uint32_t ftA, ftB, gtA, gtB;
		uint32_t mFp, mGp;

		ftA = ft[u + 0];
		ftB = ft[u + 1];
		gtA = gt[u + 0];
		gtB = gt[u + 1];
		mFp = modp_montymul(Fp[u >> 1], R2, p, p0i);
		mGp = modp_montymul(Gp[u >> 1], R2, p, p0i);
		ft[u + 0] = modp_montymul(gtB, mFp, p, p0i);
		ft[u + 1] = modp_montymul(gtA, mFp, p, p0i);
		gt[u + 0] = modp_montymul(ftB, mGp, p, p0i);
		gt[u + 1] = modp_montymul(ftA, mGp, p, p0i);
	}

	Gp = Fp + n;
	t1 = Gp + n;
	t2 = t1 + n;
	t3 = t2 + n;
	t4 = t3 + n;
	memmove(Fp, ft, 2 * n * sizeof *ft);

	/*
	 * At that point, memory contents are:
	 *     Fp         F, not reduced (NTT)
	 *     Gp         G, not reduced (NTT)
	 *     t1         free
	 *     t2   gm    NTT table
	 *     t3   igm   iNTT table
	 *     t4         free
	 * All arrays have size n.
	 */

	/*
	 * We now need to apply the Babai reduction.
	 *
	 * We can compute F*adj(f)+G*adj(g) and f*adj(f)+g*adj(g)
	 * modulo p, using the NTT. We still move memory around in
	 * order to save RAM.
	 */

	/*
	 * Load f in t4, in NTT representation.
	 */
	for (u = 0; u < n; u ++) {
		t4[u] = modp_set(f[u], p);
	}
	modp_NTT(t4, gm, logn, p, p0i);

	/*
	 * Compute F*adj(f) in t1, and f*adj(f) in t3
	 * (note: this overwrites igm, which was in t3).
	 */
	for (u = 0; u < n; u ++) {
		uint32_t w;

		w = modp_montymul(t4[u ^ (n - 1)], R2, p, p0i);
		t1[u] = modp_montymul(w, Fp[u], p, p0i);
		t3[u] = modp_montymul(w, t4[u], p, p0i);
	}

	/*
	 * Load g in t4, in NTT representation.
	 */
	for (u = 0; u < n; u ++) {
		t4[u] = modp_set(g[u], p);
	}
	modp_NTT(t4, gm, logn, p, p0i);

	/*
	 * Add G*adj(g) to t1, and g*adj(g) to t3.
	 */
	for (u = 0; u < n; u ++) {
		uint32_t w;

		w = modp_montymul(t4[u ^ (n - 1)], R2, p, p0i);
		t1[u] = modp_add(t1[u],
			modp_montymul(w, Gp[u], p, p0i), p);
		t3[u] = modp_add(t3[u],
			modp_montymul(w, t4[u], p, p0i), p);
	}

	/*
	 * Convert back t1 and t3 to normal representation (normalized
	 * around 0). Since we overwrote igm, we must recompute it (in t4).
	 */
	modp_mkgm(t2, t4, logn, PRIMES[0].g, p, p0i);
	modp_iNTT(t1, t4, logn, p, p0i);
	modp_iNTT(t3, t4, logn, p, p0i);
	modp_poly_norm(t1, logn, p);
	modp_poly_norm(t3, logn, p);

	/*
	 * Move things a bit to make some room.
	 */
	memmove(t2, t3, n * sizeof *t3);

	/*
	 * At that point, array contents are:
	 *
	 *   F (NTT representation) (Fp)
	 *   G (NTT representation) (Gp)
	 *   F*adj(f)+G*adj(g) (t1)
	 *   f*adj(f)+g*adj(g) (t2)
	 *
	 * We want to divide t1 by t2. The result is not integral; it
	 * must be rounded. We thus need to use the FFT.
	 */

	/*
	 * Get f*adj(f)+g*adj(g) in FFT representation. Since this
	 * polynomial is auto-adjoint, all its coordinates in FFT
	 * representation are actually real, so we can truncate off
	 * the imaginary parts.
	 */
	rt3 = align_fnr(tmp, t3);
	for (u = 0; u < n; u ++) {
		rt3[u] = fnr_of(sign_ext(t2[u]));
	}
	ctl_FFT(rt3, logn);
	rt2 = align_fnr(tmp, t2);
	memmove(rt2, rt3, hn * sizeof *rt3);
	rt3 = rt2 + hn;

	/*
	 * Memory layout at this point:
	 *
	 *    Fp     n words     F (NTT)
	 *    Gp     n words     G (NTT)
	 *    t1     n words     F*adj(f)+G*adj(g) (normal)
	 *    rt2    n/2 fnr     f*adj(f)+g*adj(g) (FFT, auto-adjoint)
	 *    rt3    n fnr       free
	 */

	/*
	 * Convert F*adj(f)+G*adj(g) in FFT representation.
	 */
	for (u = 0; u < n; u ++) {
		rt3[u] = fnr_of(sign_ext(t1[u]));
	}
	ctl_FFT(rt3, logn);

	/*
	 * Compute (F*adj(f)+G*adj(g))/(f*adj(f)+g*adj(g)), round it
	 * to integers, and convert it to RNS (i.e. modulo p).
	 */
	ctl_poly_div_autoadj_fft(rt3, rt2, logn);
	ctl_iFFT(rt3, logn);
	for (u = 0; u < n; u ++) {
		t1[u] = (uint32_t)fnr_round(rt3[u]) & 0x7FFFFFFF;
	}

	/*
	 * RAM contents are now:
	 *
	 *   Fp   n words   F (NTT)
	 *   Gp   n words   G (NTT)
	 *   t1   n words   k (normal)
	 *
	 * We want to compute F-k*f, and G-k*g.
	 *
	 * We allocate three n-word buffers after t1: t2, gm, igm
	 */
	t2 = t1 + n;
	gm = t2 + n;
	igm = gm + n;
	modp_mkgm(gm, igm, logn, PRIMES[0].g, p, p0i);

	/*
	 * Convert k to NTT and Montgomery representation.
	 */
	modp_poly_set(t1, logn, p);
	modp_NTT(t1, gm, logn, p, p0i);
	for (u = 0; u < n; u ++) {
		t1[u] = modp_montymul(t1[u], R2, p, p0i);
	}

	/*
	 * Subtract k*f from F and k*g from G.
	 */
	for (u = 0; u < n; u ++) {
		t2[u] = modp_set(f[u], p);
	}
	modp_NTT(t2, gm, logn, p, p0i);
	for (u = 0; u < n; u ++) {
		Fp[u] = modp_sub(Fp[u], modp_montymul(t1[u], t2[u], p, p0i), p);
	}
	for (u = 0; u < n; u ++) {
		t2[u] = modp_set(g[u], p);
	}
	modp_NTT(t2, gm, logn, p, p0i);
	for (u = 0; u < n; u ++) {
		Gp[u] = modp_sub(Gp[u], modp_montymul(t1[u], t2[u], p, p0i), p);
	}

	/*
	 * Convert back F and G into normal representation. We're done!
	 */
	modp_iNTT(Fp, igm, logn, p, p0i);
	modp_iNTT(Gp, igm, logn, p, p0i);
	modp_poly_norm(Fp, logn, p);
	modp_poly_norm(Gp, logn, p);

	return 1;
}

/*
 * Verify that the provided f, g, F and G fulfill the NTRU equation
 * (f*G - g*F = q).
 */
static int
verify_NTRU(
	const int8_t *f, const int8_t *g, const int8_t *F, const int8_t *G,
	uint32_t q, unsigned logn, uint32_t *tmp)
{
	/*
	 * Since all elements have short lengths, verifying modulo a
	 * small prime p works, and allows using the NTT. Indeed, with
	 * all coefficients within -128..+127, and degree n <= 1024, the
	 * maximum coefficient of f*G - g*F cannot exceed 2^24 in absolute
	 * value; thus, working modulo a 31-bit prime does not lose any
	 * information on the result.
	 */
	size_t u, n;
	uint32_t p, p0i, rq;
	uint32_t *gm, *t1, *t2, *t3;

	n = (size_t)1 << logn;
	gm = tmp;
	t1 = gm + n;
	t2 = t1 + n;
	t3 = t2 + n;

	/*
	 * We compute the NTT coefficients in gm[]. We will not need the
	 * inverse coefficients, so we put them in t1[] (scratch).
	 */
	p = PRIMES[0].p;
	p0i = modp_ninv31(p);
	modp_mkgm(gm, t1, logn, PRIMES[0].g, p, p0i);

	/*
	 * Compute (f*G)/R in t1 (the coefficients are obtained in
	 * anti-Montgomery representation).
	 */
	for (u = 0; u < n; u ++) {
		t1[u] = modp_set(f[u], p);
		t2[u] = modp_set(G[u], p);
	}
	modp_NTT(t1, gm, logn, p, p0i);
	modp_NTT(t2, gm, logn, p, p0i);
	for (u = 0; u < n; u ++) {
		t1[u] = modp_montymul(t1[u], t2[u], p, p0i);
	}

	/*
	 * Compute (g*F)/R in t2.
	 */
	for (u = 0; u < n; u ++) {
		t2[u] = modp_set(g[u], p);
		t3[u] = modp_set(F[u], p);
	}
	modp_NTT(t2, gm, logn, p, p0i);
	modp_NTT(t3, gm, logn, p, p0i);
	for (u = 0; u < n; u ++) {
		t2[u] = modp_montymul(t2[u], t3[u], p, p0i);
	}

	/*
	 * Verify that f*G - g*F = q. We have the NTT coefficients of f*G
	 * and g*F in t1[] and t2[], respectively (divided by R modulo p).
	 * The NTT coefficients of q are all equal to q.
	 */
	rq = modp_montymul(q, 1, p, p0i);
	for (u = 0; u < n; u ++) {
		if (modp_sub(t1[u], t2[u], p) != rq) {
			return 0;
		}
	}
	return 1;
}

/*
 * Verify NTRU equation with F/G provided as int16_t.
 * We use an exact negacyclic convolution over int64_t.
 */
static int
verify_NTRU_i16(
	const int8_t *f, const int8_t *g, const int16_t *F, const int16_t *G,
	uint32_t q, unsigned logn, uint32_t *tmp)
{
	size_t n, i, j;
	int64_t *t;

	n = (size_t)1 << logn;
	t = (int64_t *)tmp;
	for (i = 0; i < n; i ++) {
		t[i] = 0;
	}

	for (i = 0; i < n; i ++) {
		int64_t fi, gi;
		fi = f[i];
		gi = g[i];
		for (j = 0; j < n; j ++) {
			size_t k;
			int64_t z;

			k = i + j;
			z = fi * (int64_t)G[j] - gi * (int64_t)F[j];
			if (k >= n) {
				t[k - n] -= z;
			} else {
				t[k] += z;
			}
		}
	}

	if (t[0] != (int64_t)q) {
		return 0;
	}
	for (i = 1; i < n; i ++) {
		if (t[i] != 0) {
			return 0;
		}
	}
	return 1;
}

/*
 * Solve the NTRU equation. Returned value is 1 on success, 0 on error.
 * G can be NULL, in which case that value is computed but not returned.
 * If any of the coefficients of F and G exceeds lim (in absolute value),
 * then 0 is returned.
 */
static int
solve_NTRU_core(unsigned logn,
	const int8_t *f, const int8_t *g, uint32_t q, uint32_t *tmp)
{
	unsigned depth;

	if (!solve_NTRU_deepest(logn, f, g, q, tmp)) {
		kg_diag_stats.solve_fg_fail_deepest ++;
		return 0;
	}

	depth = logn;
	while (depth -- > 2) {
		if (!solve_NTRU_intermediate(logn, f, g, q, depth, tmp)) {
			kg_diag_stats.solve_fg_fail_intermediate ++;
			return 0;
		}
	}
	if (depth == 1) {
		if (!solve_NTRU_depth1(logn, f, g, q, tmp)) {
			kg_diag_stats.solve_fg_fail_depth1 ++;
			return 0;
		}
	}
	if (!solve_NTRU_depth0(logn, f, g, q, tmp)) {
		kg_diag_stats.solve_fg_fail_depth0 ++;
		return 0;
	}

	return 1;
}

static int
solve_NTRU(unsigned logn, int8_t *F, int8_t *G,
	const int8_t *f, const int8_t *g, uint32_t q, int lim, uint32_t *tmp)
{
	size_t n;

	n = (size_t)1 << logn;

	if (!solve_NTRU_core(logn, f, g, q, tmp)) {
		return 0;
	}

	/*
	 * If no buffer has been provided for G, use a temporary one.
	 * We have a special case for logn <= 2 because we want to keep
	 * tmp 64-bit aligned.
	 */
	if (G == NULL) {
		G = (int8_t *)tmp;
		if (logn <= 2) {
			tmp = (uint32_t *)(G + 8);
		} else {
			tmp = (uint32_t *)(G + n);
		}
	}

	/*
	 * Final F and G are in fk->tmp, one word per coefficient
	 * (signed value over 31 bits).
	 */
	if (!poly_big_to_small(F, tmp, lim, logn)) {
		kg_diag_stats.solve_fg_fail_poly_small_f ++;
		return 0;
	}
	if (!poly_big_to_small(G, tmp + n, lim, logn)) {
		kg_diag_stats.solve_fg_fail_poly_small_g ++;
		return 0;
	}

	/*
	 * Final verification that the solution is correct. If the
	 * reduction went awry at some point (because of the intrinsic
	 * limitations of the fixed-point approximation), then this
	 * usually shows up as out-of-range results, and thus will be
	 * rejected above (in poly_big_to_small()); however, in some
	 * rare cases, errors won't be trapped earlier and the final
	 * verification is needed.
	 */
	if (!verify_NTRU(f, g, F, G, q, logn, tmp)) {
		kg_diag_stats.solve_fg_fail_verify ++;
		return 0;
	}

	return 1;
}

/*
 * Same as solve_NTRU(), but exports F/G into int16_t slots.
 */
static int
solve_NTRU_i16(unsigned logn, int16_t *F, int16_t *G,
	const int8_t *f, const int8_t *g, uint32_t q, int lim, uint32_t *tmp)
{
	size_t n;

	n = (size_t)1 << logn;

	if (!solve_NTRU_core(logn, f, g, q, tmp)) {
		return 0;
	}

	if (G == NULL) {
		G = (int16_t *)tmp;
		if (logn <= 2) {
			tmp = (uint32_t *)(G + 8);
		} else {
			tmp = (uint32_t *)(G + n);
		}
	}
	kg_last_smallf_max_abs = 0;
	kg_last_smallf_over_count = 0;
	kg_last_smallf_first_index = 0;
	kg_last_smallf_first_value = 0;
	kg_last_smallg_max_abs = 0;
	kg_last_smallg_over_count = 0;
	kg_last_smallg_first_index = 0;
	kg_last_smallg_first_value = 0;

	if (!poly_big_to_small_i16(F, tmp, lim, logn)) {
		poly_big_i16_overflow_stats(tmp, lim, logn,
			&kg_last_smallf_max_abs, &kg_last_smallf_over_count,
			&kg_last_smallf_first_index, &kg_last_smallf_first_value);
		kg_last_smallf_d7_tail_exit = kg_d7_tail_exit;
		kg_last_smallf_d8_tail_exit = kg_d8_tail_exit;
		kg_last_smallf_d9_tail_exit = kg_d9_tail_exit;
		kg_diag_stats.solve_fg_fail_poly_small_f ++;
		return 0;
	}
	if (!poly_big_to_small_i16(G, tmp + n, lim, logn)) {
		poly_big_i16_overflow_stats(tmp + n, lim, logn,
			&kg_last_smallg_max_abs, &kg_last_smallg_over_count,
			&kg_last_smallg_first_index, &kg_last_smallg_first_value);
		kg_diag_stats.solve_fg_fail_poly_small_g ++;
		return 0;
	}

	if (!verify_NTRU_i16(f, g, F, G, q, logn, tmp)) {
		kg_diag_stats.solve_fg_fail_verify ++;
		return 0;
	}

	return 1;
}

/*
 * Convert a PRNG output word into a real number in [0,1).
 */
static inline double
prng_u64_to_f64(uint64_t x)
{
	return (double)x * 5.42101086242752217004e-20;
}

/*
 * Convert a double to fnr (rounded to nearest, ties away from zero).
 */
static inline fnr
fnr_of_double(double x)
{
	int64_t z;

	if (x >= 0) {
		z = (int64_t)(x * 4294967296.0 + 0.5);
	} else {
		z = (int64_t)(x * 4294967296.0 - 0.5);
	}
	return fnr_of_scaled32((uint64_t)z);
}

/*
 * Decode a real vector as an int8 vector with odd coefficient sum.
 * Return 1 on success, 0 on range error.
 */
static int
decode_odd_i8(int8_t *u, const fnr *utilde, unsigned logn, int lim)
{
	size_t n, i, worst_coeff;
	unsigned umod2;
	int32_t wi;
	fnr maxdiff;
	int have_diff;

	n = (size_t)1 << logn;
	umod2 = 0;
	wi = 0;
	worst_coeff = 0;
	maxdiff = fnr_zero;
	have_diff = 0;

	for (i = 0; i < n; i ++) {
		int32_t ui;
		fnr ur, diff;

		ui = fnr_round(utilde[i]);
		if (ui < -lim || ui > +lim) {
			return 0;
		}
		umod2 ^= (unsigned)ui & 1u;

		ur = fnr_of(ui);
		diff = fnr_abs(fnr_sub(utilde[i], ur));
		if (!have_diff || fnr_lt(maxdiff, diff)) {
			have_diff = 1;
			maxdiff = diff;
			worst_coeff = i;
			wi = fnr_lt(ur, utilde[i]) ? (ui + 1) : (ui - 1);
		}
		u[i] = (int8_t)ui;
	}

	if ((umod2 & 1u) == 0) {
		if (wi < -lim || wi > +lim) {
			return 0;
		}
		u[worst_coeff] = (int8_t)wi;
	}
	return 1;
}

#if CTL_ENFORCE_EMBED_CHECK
/*
 * Check that all embeddings satisfy:
 *   q/alpha^2 <= |phi_i(f)|^2 + |phi_i(g)|^2 <= alpha^2*q
 * after coefficient rounding.
 */
static int
check_embed_norm_range_i8(const int8_t *f, const int8_t *g,
	uint32_t q, unsigned logn, double alpha)
{
	size_t n, hn, u;
	fnr ff[2048], gg[2048];
	fnr lo, hi;
	double lov, hiv, slack;

	n = (size_t)1 << logn;
	hn = n >> 1;
	/*
	 * Rounding from real coefficients to integers induces a frequency-space
	 * perturctlion; use n/6 compensation (as in the framework bound) so that
	 * this post-rounding check remains practical.
	 */
	slack = (double)n / 6.0;
	lov = (double)q / (alpha * alpha) - slack;
	if (lov < 0) {
		lov = 0;
	}
	hiv = (double)q * (alpha * alpha) + slack;
	lo = fnr_of_double(lov);
	hi = fnr_of_double(hiv);

	for (u = 0; u < n; u ++) {
		ff[u] = fnr_of(f[u]);
		gg[u] = fnr_of(g[u]);
	}
	ctl_FFT(ff, logn);
	ctl_FFT(gg, logn);

	for (u = 0; u < hn; u ++) {
		fnr nf, ng, v;

		nf = fnr_add(fnr_sqr(ff[u]), fnr_sqr(ff[u + hn]));
		ng = fnr_add(fnr_sqr(gg[u]), fnr_sqr(gg[u + hn]));
		v = fnr_add(nf, ng);
		if (fnr_lt(v, lo) || fnr_lt(hi, v)) {
			return 0;
		}
	}
	return 1;
}
#endif

/*
 * Sample f and g jointly with the antrag keygen method:
 *  - generate in FFT domain from uniform random values
 *  - iFFT to time domain
 *  - decode to odd-sum integer vectors
 *  - enforce embedding norm bounds after rounding
 */
static int
poly_small_mkantrag_fg(prng_context *rng, int8_t *f, int8_t *g,
	uint32_t q, unsigned logn, double alpha)
{
	size_t n, hn, u;
	int lim;
	double r[2 * 2048];
	fnr ff[2048], gg[2048];
	const double pi = 3.14159265358979323846264338327950288;
	const double xi = 1.0 / 3.0;
	const double inv_alpha = 1.0 / alpha;
	const double alow = 0.5 * (alpha + inv_alpha)
		- 0.5 * xi * (alpha - inv_alpha);
	const double ahigh = 0.5 * (alpha + inv_alpha)
		+ 0.5 * xi * (alpha - inv_alpha);
	const double qlow = (double)q * alow * alow;
	const double qhigh = (double)q * ahigh * ahigh;

	if (logn == 0 || logn > 11 || alpha <= 1.0) {
		return 0;
	}
	n = (size_t)1 << logn;
	hn = n >> 1;
	lim = 1 << (ctl_max_fg_bits[logn] - 1);

	for (;;) {
		for (u = 0; u < (2 * n); u ++) {
			r[u] = prng_u64_to_f64(prng_get_u64(rng));
		}

		for (u = 0; u < hn; u ++) {
			double z, af, ag, tf, tg;

			z = sqrt(qlow + (qhigh - qlow) * r[u]);
			af = z * cos((pi * 0.5) * r[u + hn]);
			ag = z * sin((pi * 0.5) * r[u + hn]);
			tf = 2.0 * pi * r[u + (2 * hn)];
			tg = 2.0 * pi * r[u + (3 * hn)];

			ff[u] = fnr_of_double(af * cos(tf));
			ff[u + hn] = fnr_of_double(af * sin(tf));
			gg[u] = fnr_of_double(ag * cos(tg));
			gg[u + hn] = fnr_of_double(ag * sin(tg));
		}

		ctl_iFFT(ff, logn);
		ctl_iFFT(gg, logn);
		if (!decode_odd_i8(f, ff, logn, lim)
			|| !decode_odd_i8(g, gg, logn, lim))
		{
			continue;
		}
#if CTL_ENFORCE_EMBED_CHECK
		if (!check_embed_norm_range_i8(f, g, q, logn, alpha)) {
			continue;
		}
#endif

		return 1;
	}
}

/*
 * Compute the vector w:
 *   w = round(qp*(gamma2*G*adj(g)+F*adj(f))/(gamma2*g*adj(g)+f*adj(f)))
 *
 * Value qp is normally 64513, and gamma2 = (k^2-1)/3 = 1 or 5. All
 * coefficients of w are supposed to fit on signed 17-bit integers (all
 * values should be in -65535..+65535).
 *
 * Return value: 1 on success, 0 on error (an error is reported on overflow
 * of any coefficient of w).
 */
static int
compute_w(int32_t *w, const int8_t *f, const int8_t *g,
	const int8_t *F, const int8_t *G, unsigned qp, unsigned gamma2,
	unsigned logn, uint32_t *tmp)
{
	size_t n, u, hn;
	uint32_t p, p0i, R2, R2g2;
	uint32_t *gm, *igm, *ft, *gt, *Ft, *Gt, *t1, *t2;
	fnr *rt1, *rt2, rslim;
	int32_t lim1;
	unsigned scale_bits;

	n = (size_t)1 << logn;
	hn = n >> 1;
	kg_cw_last_fail = KG_CW_FAIL_NONE;

	/*
	 * Memory layout:
	 *    gm    n words    also t1 (receives numerator)
	 *    Ft    n words    also t2 (receives denominator)
	 *    Gt    n words
	 *    ft    n words
	 *    gt    n words
	 *    igm   n words
	 */
	gm = tmp;
	Ft = gm + n;
	Gt = Ft + n;
	ft = Gt + n;
	gt = ft + n;
	igm = gt + n;
	t1 = gm;
	t2 = Ft;

	/*
	 * We compute f*adj(f)+gamma2*g*adj(g) and F*adj(f)+gamma2*G*adj(g)
	 * with the NTT; results should fit in 19 bits in absolute value.
	 */
	p = PRIMES[0].p;
	p0i = modp_ninv31(p);
	modp_mkgm(gm, igm, logn, PRIMES[0].g, p, p0i);

	/*
	 * R2 = Montgomery representation of R
	 * R2g2 = Montgomery representation of R*gamma2
	 */
	R2 = modp_R2(p, p0i);
	R2g2 = modp_montymul(R2,
		modp_montymul(R2, modp_set(gamma2, p), p, p0i),
		p, p0i);

	/*
	 * Get f, g, F and G into NTT.
	 */
	for (u = 0; u < n; u ++) {
		ft[u] = modp_set(f[u], p);
		gt[u] = modp_set(g[u], p);
		Ft[u] = modp_set(F[u], p);
		Gt[u] = modp_set(G[u], p);
	}
	modp_NTT(ft, gm, logn, p, p0i);
	modp_NTT(gt, gm, logn, p, p0i);
	modp_NTT(Ft, gm, logn, p, p0i);
	modp_NTT(Gt, gm, logn, p, p0i);

	/*
	 * Compute F*adj(f)+gamma2*G*adj(g) into t1.
	 * This overwrites gm, which we don't need anymore.
	 */
	for (u = 0; u < n; u ++) {
		uint32_t sF, sG, sfa, sga;

		sF = Ft[u];
		sG = Gt[u];
		sfa = modp_montymul(R2, ft[u ^ (n - 1)], p, p0i);
		sga = modp_montymul(R2g2, gt[u ^ (n - 1)], p, p0i);
		t1[u] = modp_add(
			modp_montymul(sF, sfa, p, p0i),
			modp_montymul(sG, sga, p, p0i), p);
	}

	/*
	 * Compute f*adj(f)+gamma2*g*adj(g) into t2.
	 * This overwrites Ft, which we don't need anymore.
	 */
	for (u = 0; u < n; u ++) {
		uint32_t sf, sg, sfa, sga;

		sf = ft[u];
		sg = gt[u];
		sfa = modp_montymul(R2, ft[u ^ (n - 1)], p, p0i);
		sga = modp_montymul(R2g2, gt[u ^ (n - 1)], p, p0i);
		t2[u] = modp_add(
			modp_montymul(sf, sfa, p, p0i),
			modp_montymul(sg, sga, p, p0i), p);
	}

	/*
	 * Convert back numerator and denominator into RNS, then
	 * fixed-point; we also multiply the numerator by qp. To give
	 * us some margin, we scale down the numerator by 10 bits.
	 *
	 * We check that the values are such that the FFT won't overflow.
	 * We can do a conditional jump for that, since any failure here
	 * will imply that the private key is discarded.
	 */
	modp_iNTT(t1, igm, logn, p, p0i);
	modp_iNTT(t2, igm, logn, p, p0i);

	rt1 = align_fnr(tmp, t2 + n);
	rt2 = rt1 + n;
	scale_bits = 10;
	for (u = 0; u < n; u ++) {
		uint32_t av1;
		int32_t v1;

		v1 = modp_norm(t1[u], p);
		av1 = (uint32_t)(v1 < 0 ? -v1 : v1);
		if (av1 > kg_cw_max_v1_abs) {
			kg_cw_max_v1_abs = av1;
		}
	}
	for (;;) {
		lim1 = (int32_t)(((uint64_t)1 << (31 + scale_bits - logn)) / qp);
		if (kg_cw_max_v1_abs < (uint32_t)lim1 || scale_bits >= 16) {
			break;
		}
		scale_bits ++;
	}
	if (kg_cw_max_v1_abs >= (uint32_t)lim1) {
		kg_cw_last_v1_abs = kg_cw_max_v1_abs;
		kg_cw_last_lim1 = (uint32_t)lim1;
		kg_cw_last_fail = KG_CW_FAIL_LIM1;
		return 0;
	}
	for (u = 0; u < n; u ++) {
		int32_t v1, v2;

		v1 = modp_norm(t1[u], p);
		v2 = modp_norm(t2[u], p);
		if (v1 <= -lim1 || v1 >= +lim1) {
			kg_cw_last_v1_abs = (uint32_t)(v1 < 0 ? -v1 : v1);
			kg_cw_last_lim1 = (uint32_t)lim1;
			kg_cw_last_fail = KG_CW_FAIL_LIM1;
			return 0;
		}

		rt1[u] = fnr_of_scaled32(
			((uint64_t)qp * (uint64_t)v1) << (32 - scale_bits));
		rt2[u] = fnr_of_scaled32(
			(uint64_t)v2 << 32);
	}
	ctl_FFT(rt1, logn);
	ctl_FFT(rt2, logn);

	/*
	 * Compute the division. Note that the denominator is auto-adjoint,
	 * and thus its FFT representation contains only real numbers (and
	 * they are all positive). We use an explicit loop here because we
	 * want to be able to check that none of the divisions overflows.
	 */
	for (u = 0; u < hn; u ++) {
		fnr z1a, z1b, z2;

		z1a = rt1[u];
		z1b = rt1[u + hn];
		z2 = rt2[u];

		/*
		 * We want to make sure that |z1| < 2^(30-logn)*|z2|;
		 * this guarantees that the division will not overflow,
		 * AND that the inverse FFT won't overflow either.
		 */
		if (!fnr_lt(fnr_div_2e(fnr_abs(z1a), 30 - logn), z2)
			|| !fnr_lt(fnr_div_2e(fnr_abs(z1b), 30 - logn), z2))
		{
			kg_cw_last_fail = KG_CW_FAIL_DIV;
			return 0;
		}
		rt1[u] = fnr_div(z1a, z2);
		rt1[u + hn] = fnr_div(z1b, z2);
	}
	ctl_iFFT(rt1, logn);

	/*
	 * Now round all coefficients of w. Check that they all fit
	 * in the allowed range (-2^16..+2^16). The coefficients in rt1[]
	 * are currently scaled down (by 10 bits) so we must first scale
	 * them up.
	 */
	rslim = fnr_of(1 << (16 - scale_bits));
	for (u = 0; u < n; u ++) {
		int32_t z;

		if (!fnr_lt(fnr_abs(rt1[u]), rslim)) {
			kg_cw_last_fail = KG_CW_FAIL_FNR_ABS;
			return 0;
		}
		z = fnr_round(fnr_mul_2e(rt1[u], scale_bits));
		if (z < -((int32_t)1 << 16) || z > +((int32_t)1 << 16)) {
			kg_cw_last_fail = KG_CW_FAIL_ROUND_RANGE;
			return 0;
		}
		w[u] = (int32_t)z;
	}

	return 1;
}

/*
 * Variant of compute_w() with F/G provided as int16_t.
 */
static int
compute_w_i16(int32_t *w, const int8_t *f, const int8_t *g,
	const int16_t *F, const int16_t *G, unsigned qp, unsigned gamma2,
	unsigned logn, uint32_t *tmp)
{
	size_t n, u, hn;
	uint32_t p, p0i, R2, R2g2;
	uint32_t *gm, *igm, *ft, *gt, *Ft, *Gt, *t1, *t2;
	fnr *rt1, *rt2, rslim;
	int32_t lim1;
	unsigned scale_bits;

	n = (size_t)1 << logn;
	hn = n >> 1;
	kg_cw_last_fail = KG_CW_FAIL_NONE;
	gm = tmp;
	Ft = gm + n;
	Gt = Ft + n;
	ft = Gt + n;
	gt = ft + n;
	igm = gt + n;
	t1 = gm;
	t2 = Ft;

	p = PRIMES[0].p;
	p0i = modp_ninv31(p);
	modp_mkgm(gm, igm, logn, PRIMES[0].g, p, p0i);
	R2 = modp_R2(p, p0i);
	R2g2 = modp_montymul(R2,
		modp_montymul(R2, modp_set(gamma2, p), p, p0i),
		p, p0i);

	for (u = 0; u < n; u ++) {
		ft[u] = modp_set(f[u], p);
		gt[u] = modp_set(g[u], p);
		Ft[u] = modp_set(F[u], p);
		Gt[u] = modp_set(G[u], p);
	}
	modp_NTT(ft, gm, logn, p, p0i);
	modp_NTT(gt, gm, logn, p, p0i);
	modp_NTT(Ft, gm, logn, p, p0i);
	modp_NTT(Gt, gm, logn, p, p0i);

	for (u = 0; u < n; u ++) {
		uint32_t sF, sG, sfa, sga;

		sF = Ft[u];
		sG = Gt[u];
		sfa = modp_montymul(R2, ft[u ^ (n - 1)], p, p0i);
		sga = modp_montymul(R2g2, gt[u ^ (n - 1)], p, p0i);
		t1[u] = modp_add(
			modp_montymul(sF, sfa, p, p0i),
			modp_montymul(sG, sga, p, p0i), p);
	}

	for (u = 0; u < n; u ++) {
		uint32_t sf, sg, sfa, sga;

		sf = ft[u];
		sg = gt[u];
		sfa = modp_montymul(R2, ft[u ^ (n - 1)], p, p0i);
		sga = modp_montymul(R2g2, gt[u ^ (n - 1)], p, p0i);
		t2[u] = modp_add(
			modp_montymul(sf, sfa, p, p0i),
			modp_montymul(sg, sga, p, p0i), p);
	}

	modp_iNTT(t1, igm, logn, p, p0i);
	modp_iNTT(t2, igm, logn, p, p0i);

	rt1 = align_fnr(tmp, t2 + n);
	rt2 = rt1 + n;
	scale_bits = 10;
	for (u = 0; u < n; u ++) {
		uint32_t av1;
		int32_t v1;

		v1 = modp_norm(t1[u], p);
		av1 = (uint32_t)(v1 < 0 ? -v1 : v1);
		if (av1 > kg_cw_max_v1_abs) {
			kg_cw_max_v1_abs = av1;
		}
	}
	for (;;) {
		lim1 = (int32_t)(((uint64_t)1 << (31 + scale_bits - logn)) / qp);
		if (kg_cw_max_v1_abs < (uint32_t)lim1 || scale_bits >= 16) {
			break;
		}
		scale_bits ++;
	}
	if (kg_cw_max_v1_abs >= (uint32_t)lim1) {
		kg_cw_last_v1_abs = kg_cw_max_v1_abs;
		kg_cw_last_lim1 = (uint32_t)lim1;
		kg_cw_last_fail = KG_CW_FAIL_LIM1;
		return 0;
	}
	for (u = 0; u < n; u ++) {
		int32_t v1, v2;

		v1 = modp_norm(t1[u], p);
		v2 = modp_norm(t2[u], p);
		if (v1 <= -lim1 || v1 >= +lim1) {
			kg_cw_last_v1_abs = (uint32_t)(v1 < 0 ? -v1 : v1);
			kg_cw_last_lim1 = (uint32_t)lim1;
			kg_cw_last_fail = KG_CW_FAIL_LIM1;
			return 0;
		}
		rt1[u] = fnr_of_scaled32(
			((uint64_t)qp * (uint64_t)v1) << (32 - scale_bits));
		rt2[u] = fnr_of_scaled32(
			(uint64_t)v2 << 32);
	}
	ctl_FFT(rt1, logn);
	ctl_FFT(rt2, logn);

	for (u = 0; u < hn; u ++) {
		fnr z1a, z1b, z2;

		z1a = rt1[u];
		z1b = rt1[u + hn];
		z2 = rt2[u];
		if (!fnr_lt(fnr_div_2e(fnr_abs(z1a), 30 - logn), z2)
			|| !fnr_lt(fnr_div_2e(fnr_abs(z1b), 30 - logn), z2))
		{
			kg_cw_last_fail = KG_CW_FAIL_DIV;
			return 0;
		}
		rt1[u] = fnr_div(z1a, z2);
		rt1[u + hn] = fnr_div(z1b, z2);
	}
	ctl_iFFT(rt1, logn);

	rslim = fnr_of(1 << (16 - scale_bits));
	for (u = 0; u < n; u ++) {
		int32_t z;

		if (!fnr_lt(fnr_abs(rt1[u]), rslim)) {
			kg_cw_last_fail = KG_CW_FAIL_FNR_ABS;
			return 0;
		}
		z = fnr_round(fnr_mul_2e(rt1[u], scale_bits));
		if (z < -((int32_t)1 << 16) || z > +((int32_t)1 << 16)) {
			kg_cw_last_fail = KG_CW_FAIL_ROUND_RANGE;
			return 0;
		}
		w[u] = z;
	}

	return 1;
}

/*
 * Compute the squared norm of (Fd, gamma*Gd) with:
 *   Fd = qp*F - f*w
 *   Gd = qp*G - g*w
 */
static uint64_t
compute_dnorm(const int8_t *f, const int8_t *g,
	const int8_t *F, const int8_t *G, const int32_t *w,
	unsigned qp, unsigned gamma2, unsigned logn, uint32_t *tmp)
{
	size_t n, u;
	uint32_t *gm, *igm, *t1, *t2, *t3;
	uint32_t p, p0i, R2;
	uint64_t dnorm1, dnorm2;

	/*
	 * Max values for f and g is 6*(1024/n) (in absolute value);
	 * coefficients of F and G are less than 2^7, and coefficients
	 * of w are less than 2^16. Thus, maximum absolute value of a
	 * coefficient of Fd or Gd is n*6*(1024/n)*(2^16-1) +
	 * qp*(2^7-1). With qp = 64513, this is 410840191. Thus, we
	 * can compute Fd and Gd with the NTT and a single prime, as long
	 * as that prime is greater than 821680383.
	 */
	n = (size_t)1 << logn;
	gm = tmp;
	igm = gm + n;
	t1 = igm + n;
	t2 = t1 + n;
	t3 = t2 + n;

	p = PRIMES[0].p;
	p0i = modp_ninv31(p);
	R2 = modp_R2(p, p0i);
	modp_mkgm(gm, igm, logn, PRIMES[0].g, p, p0i);

	/*
	 * Load w in t1 and convert it to NTT. Also, convert all elements in
	 * Montgomery representation.
	 */
	for (u = 0; u < n; u ++) {
		t1[u] = modp_montymul(modp_set(w[u], p), R2, p, p0i);
	}
	modp_NTT(t1, gm, logn, p, p0i);

	/*
	 * Load f in t2 and qp*F in t3; convert both to NTT, compute Fd,
	 * and compute its squared norm.
	 */
	for (u = 0; u < n; u ++) {
		t2[u] = modp_set(f[u], p);
		t3[u] = modp_set((int32_t)qp * (int32_t)F[u], p);
	}
	modp_NTT(t2, gm, logn, p, p0i);
	modp_NTT(t3, gm, logn, p, p0i);
	for (u = 0; u < n; u ++) {
		t2[u] = modp_sub(t3[u],
			modp_montymul(t1[u], t2[u], p, p0i), p);
	}
	modp_iNTT(t2, igm, logn, p, p0i);
	dnorm1 = 0;
	for (u = 0; u < n; u ++) {
		int32_t x;

		x = modp_norm(t2[u], p);
		dnorm1 += (uint64_t)((int64_t)x * (int64_t)x);
	}

	/*
	 * Load g in t2 and qp*G in t3; convert both to NTT, compute Gd,
	 * and compute its squared norm.
	 */
	for (u = 0; u < n; u ++) {
		t2[u] = modp_set(g[u], p);
		t3[u] = modp_set((int32_t)qp * (int32_t)G[u], p);
	}
	modp_NTT(t2, gm, logn, p, p0i);
	modp_NTT(t3, gm, logn, p, p0i);
	for (u = 0; u < n; u ++) {
		t2[u] = modp_sub(t3[u],
			modp_montymul(t1[u], t2[u], p, p0i), p);
	}
	modp_iNTT(t2, igm, logn, p, p0i);
	dnorm2 = 0;
	for (u = 0; u < n; u ++) {
		int32_t x;

		x = modp_norm(t2[u], p);
		dnorm2 += (uint64_t)((int64_t)x * (int64_t)x);
	}

	/*
	 * Result is dnorm1 + (gamma^2)*dnorm2, since there is a factor
	 * gamma on Gd.
	 */
	return dnorm1 + (uint64_t)gamma2 * dnorm2;
}

/*
 * Variant of compute_dnorm() with F/G provided as int16_t.
 */
static uint64_t
compute_dnorm_i16(const int8_t *f, const int8_t *g,
	const int16_t *F, const int16_t *G, const int32_t *w,
	unsigned qp, unsigned gamma2, unsigned logn, uint32_t *tmp)
{
	size_t n, u;
	uint32_t *gm, *igm, *t1, *t2, *t3;
	uint32_t p, p0i, R2;
	uint64_t dnorm1, dnorm2;

	n = (size_t)1 << logn;
	gm = tmp;
	igm = gm + n;
	t1 = igm + n;
	t2 = t1 + n;
	t3 = t2 + n;

	p = PRIMES[0].p;
	p0i = modp_ninv31(p);
	R2 = modp_R2(p, p0i);
	modp_mkgm(gm, igm, logn, PRIMES[0].g, p, p0i);

	for (u = 0; u < n; u ++) {
		t1[u] = modp_montymul(modp_set(w[u], p), R2, p, p0i);
	}
	modp_NTT(t1, gm, logn, p, p0i);

	for (u = 0; u < n; u ++) {
		t2[u] = modp_set(f[u], p);
		t3[u] = modp_set((int32_t)qp * (int32_t)F[u], p);
	}
	modp_NTT(t2, gm, logn, p, p0i);
	modp_NTT(t3, gm, logn, p, p0i);
	for (u = 0; u < n; u ++) {
		t2[u] = modp_sub(t3[u],
			modp_montymul(t1[u], t2[u], p, p0i), p);
	}
	modp_iNTT(t2, igm, logn, p, p0i);
	dnorm1 = 0;
	for (u = 0; u < n; u ++) {
		int32_t x;
		x = modp_norm(t2[u], p);
		dnorm1 += (uint64_t)((int64_t)x * (int64_t)x);
	}

	for (u = 0; u < n; u ++) {
		t2[u] = modp_set(g[u], p);
		t3[u] = modp_set((int32_t)qp * (int32_t)G[u], p);
	}
	modp_NTT(t2, gm, logn, p, p0i);
	modp_NTT(t3, gm, logn, p, p0i);
	for (u = 0; u < n; u ++) {
		t2[u] = modp_sub(t3[u],
			modp_montymul(t1[u], t2[u], p, p0i), p);
	}
	modp_iNTT(t2, igm, logn, p, p0i);
	dnorm2 = 0;
	for (u = 0; u < n; u ++) {
		int32_t x;
		x = modp_norm(t2[u], p);
		dnorm2 += (uint64_t)((int64_t)x * (int64_t)x);
	}

	return dnorm1 + (uint64_t)gamma2 * dnorm2;
}

/*
 * Map parameter set to k, then derive gamma^2 = (k^2 - 1)/3.
 */
static int
keygen_get_k_gamma2(uint32_t q, unsigned logn, unsigned *k, unsigned *gamma2)
{
	unsigned kk;

	switch (q) {
	case 257:
		if (logn == 0 || logn > 9) {
			return 0;
		}
		kk = 2;
		break;
	case 769:
		if (logn == 0 || logn > 10) {
			return 0;
		}
		kk = 4;
		break;
	case 3329:
		if (logn != 11) {
			return 0;
		}
		kk = 8;
		break;
	default:
		return 0;
	}

	if (k != NULL) {
		*k = kk;
	}
	if (gamma2 != NULL) {
		*gamma2 = (kk * kk - 1u) / 3u;
	}
	return 1;
}

/*
 * Compute framework bounds from formulas (xi = 1/3):
 *   r = ((alpha + 2/alpha)/3)*sqrt(q)
 *   R = ((2*alpha + 1/alpha)/3)*sqrt(q)
 *   I(gamma,r,R) = ln(R^2/r^2)/(gamma*(R^2-r^2))
 *   ||(G',gamma*F')|| <= q*gamma*sqrt(I) +
 *      sqrt(n*(1+gamma^2)*((r^2+R^2)/2 + n/6)) / (2*q')
 */

/*
 * Compute I(gamma, r, R) = ln(R^2/r^2) / (gamma * (R^2 - r^2))
 */
static double
compute_I(double gamma, double r, double R)
{
	double r2 = r * r;
	double R2 = R * R;
	return log(R2 / r2) / (gamma * (R2 - r2));
}

static double
keygen_framework_bounds(uint32_t q, unsigned logn,
	unsigned gamma2, unsigned qp, double alpha)
{
	size_t n;
	double inv_alpha, sqrt_q;
	double r, R, r2, R2;
	double gamma, I, gpb;

	inv_alpha = 1.0 / alpha;
	n = (size_t)1 << logn;
	sqrt_q = sqrt((double)q);
	r = ((alpha + (2.0 * inv_alpha)) / 3.0) * sqrt_q;
	R = (((2.0 * alpha) + inv_alpha) / 3.0) * sqrt_q;
	r2 = r * r;
	R2 = R * R;
	gamma = sqrt((double)gamma2);
	I = compute_I(gamma, r, R);
	gpb = (double)q * gamma * sqrt(I)
		+ sqrt((double)n * (1.0 + (double)gamma2)
			* (0.5 * (r2 + R2) + (double)n / 6.0))
			/ (2.0 * (double)qp);

	return gpb;
}

/*
 * Solve alpha by bisection on:
 *   q*gamma*sqrt(I(alpha)) +
 *   sqrt(n*(1+gamma^2)*((r^2+R^2)/2+n/6))/(2*q') = alpha*sqrt(q)
 */
static double
keygen_solve_alpha(uint32_t q, unsigned logn, unsigned gamma2, unsigned qp)
{
	double alpha_out;
	
	(void)q;
	(void)gamma2;
	(void)qp;
	
	alpha_out = (logn <= 9) ? 1.18 : 1.23;

	return alpha_out;
}


/*
 * Compute only the Step-5 norm squared:
 *
 *   || q*gamma * (-gamma*adj(f)/D, adj(g)/D) ||^2
 *
 * where:
 *   D = gamma^2 * f*adj(f) + g*adj(g)
 *
 * Returned value:
 *   1 on success, 0 on error.
 *
 * Output:
 *   *gs2 receives the squared norm in fnr format.
 *
 * Constraints:
 *   - logn <= 11
 *   - requires ctl_FFT(), ctl_iFFT(), ctl_poly_adj_fft(),
 *     ctl_poly_mulconst(), ctl_poly_invnorm_fft(),
 *     ctl_poly_mul_autoadj_fft()
 */
static int
compute_step5_norm2(fnr *gs2,
    const int8_t *f, const int8_t *g,
    uint32_t q, unsigned gamma2, unsigned logn)
{
    size_t n, u;
    fnr ff[2048], gg[2048], t1[2048], t2[2048], dinv[2048];
    fnr acc_gs2;
    fnr qf, gamma, c1, c2;

    if (gs2 == NULL) {
        return 0;
    }
    if (logn == 0 || logn > 11) {
        return 0;
    }

    n = (size_t)1 << logn;

    qf = fnr_of((int32_t)q);
    if (gamma2 == 1) {
        gamma = fnr_of(1);
    } else {
        gamma = fnr_of_double(sqrt((double)gamma2));
    }

    /*
     * FFT(f), FFT(g)
     */
    for (u = 0; u < n; u ++) {
        ff[u] = fnr_of((int32_t)f[u]);
        gg[u] = fnr_of((int32_t)g[u]);
    }
    ctl_FFT(ff, logn);
    ctl_FFT(gg, logn);

    /*
     * dinv = 1 / (gamma^2*f*adj(f) + g*adj(g))
     * by passing a = gamma*f, b = g
     */
    for (u = 0; u < n; u ++) {
        t1[u] = ff[u];
    }
    ctl_poly_mulconst(t1, gamma, logn);
    ctl_poly_invnorm_fft(dinv, t1, gg, 0, logn);

    /*
     * u1 = -q*gamma^2 * adj(f) / D
     *   (-q*gamma) * gamma * adj(f) / D
     */
    for (u = 0; u < n; u ++) {
        t1[u] = ff[u];
    }
    ctl_poly_adj_fft(t1, logn);
    c1 = fnr_neg(fnr_mul(qf, gamma));
    ctl_poly_mulconst(t1, c1, logn);
    ctl_poly_mulconst(t1, gamma, logn);
    ctl_poly_mul_autoadj_fft(t1, dinv, logn);
    ctl_iFFT(t1, logn);

    /*
     * u2 = q*gamma * adj(g) / D
     */
    for (u = 0; u < n; u ++) {
        t2[u] = gg[u];
    }
    ctl_poly_adj_fft(t2, logn);
    c2 = fnr_mul(qf, gamma);
    ctl_poly_mulconst(t2, c2, logn);
    ctl_poly_mul_autoadj_fft(t2, dinv, logn);
    ctl_iFFT(t2, logn);

    /*
     * Squared norm.
     */
    acc_gs2 = fnr_zero;
    for (u = 0; u < n; u ++) {
        acc_gs2 = fnr_add(acc_gs2, fnr_sqr(t1[u]));
        acc_gs2 = fnr_add(acc_gs2, fnr_sqr(t2[u]));
    }

    *gs2 = acc_gs2;
    return 1;
}

static double
keygen_step5_main_bound(uint32_t q, unsigned logn,
    unsigned gamma2, double alpha)
{
    double inv_alpha, sqrt_q;
    double r, R;
    double gamma, I;

    (void)logn;

    inv_alpha = 1.0 / alpha;
    sqrt_q = sqrt((double)q);

    r = ((alpha + (2.0 * inv_alpha)) / 3.0) * sqrt_q;
    R = (((2.0 * alpha) + inv_alpha) / 3.0) * sqrt_q;

    gamma = sqrt((double)gamma2);
    I = compute_I(gamma, r, R);
	double eta = 1;
    return (double) eta * q * gamma * sqrt(I);
}

/* ===================================================================== */
/*
 * Functions below this line use the CTL naming conventions (g*F - f*G = q).
 */

/* see inner.h */
int
ctl_keygen_make_fg(int8_t *f, int8_t *g, uint16_t *h,
	uint32_t q, unsigned logn,
	const void *seed, size_t seed_len, uint32_t *tmp)
{
	size_t n;
	prng_context rng;
	uint32_t normf, normg;
	unsigned gamma2_t;
	double alpha, t_fg, t_lim;
	fnr gs2, lim2;

	n = (size_t)1 << logn;

	if (!keygen_get_k_gamma2(q, logn, NULL, &gamma2_t)) {
		return 0;
	}
	alpha = keygen_solve_alpha(q, logn, gamma2_t, 64513);

	/*
	 * The BLAKE2s-based PRNG is initialized with the provided seed;
	 * the 'label' contains the modulus (q, 16-bit) and the logarithm
	 * of the degree (logn, in bits 16..19).
	 */
	prng_init(&rng, seed, seed_len, q | (uint32_t)logn << 16);

	/*
	 * Generate f and g.
	 */
	if (!poly_small_mkantrag_fg(&rng, f, g, q, logn, alpha)) {
		return 0;
	}

	/*
	 * Ensure deepest-level Bezout precondition early:
	 * reject samples whose resultants are not suitable, instead of
	 * letting solve_FG fail later on zint_bezout().
	 */
	if (!ctl_debug_deepest_resultants(q, logn, f, g, tmp,
		NULL, NULL, NULL, NULL))
	{
		return 0;
	}

	/*
	 * New framework keygen check:
	 *   T = max(||(g,-f)||, bound(||(G',gamma*F')||))
	 * and reject if T exceeds alpha*sqrt(q).
	 */
	normf = poly_small_sqnorm(f, logn);
	normg = poly_small_sqnorm(g, logn);
	t_fg = sqrt((double)normf + (double)normg);

	// ================== 娣囶喗鏁奸弽绋跨妇闁槒绶敍?鐠佲剝妞傛稉搴°亼鐠愩儳绮虹拋?==================
	struct timeval step4_start, step4_end;
	struct timeval step5_start, step5_end;
	
	double fail_time_4_ms;
	double fail_time_5_ms;
	

	// 1. 瀵偓婵?Step4 閻楃懓鐣剧拋鈩冩
	gettimeofday(&step4_start, NULL);

	// 2. Step4 婢惰精瑙﹂崚銈嗘焽
	if (t_fg > alpha * sqrt((double)q))
	{
		// 鐠侊紕鐣婚張顒侇偧婢惰精瑙﹂懓妤佹
		gettimeofday(&step4_end, NULL);
		fail_time_4_ms = (step4_end.tv_sec - step4_start.tv_sec) * 1000.0 +
					   (step4_end.tv_usec - step4_start.tv_usec) / 1000.0;

		// 缁鳖垰濮炵紒鐔活吀閺佺増宓?
		record_step4_failure(fail_time_4_ms);

		return 0;
	}

	/*
 	* Step 5: reject if
 	*   || q*gamma * (-gamma*adj(f)/D, adj(g)/D) ||
	 * exceeds the framework bound.
	 */
	if (!compute_step5_norm2(&gs2, f, g, q, gamma2_t, logn)) {
    	return 0;
	}

	t_lim = keygen_step5_main_bound(q, logn, gamma2_t, alpha);
	if (t_lim <= 0) {
	    return 0;
	}

	//  瀵偓婵?Step5 閻楃懓鐣剧拋鈩冩
	gettimeofday(&step5_start, NULL);

	lim2 = fnr_of_double(t_lim * t_lim);
	if (fnr_lt(lim2, gs2))
	{
		gettimeofday(&step5_end, NULL);
		fail_time_5_ms = (step5_end.tv_sec - step5_start.tv_sec) * 1000.0 +
					   (step5_end.tv_usec - step5_start.tv_usec) / 1000.0;

		// 缁鳖垰濮炵紒鐔活吀閺佺増宓?
		record_step5_failure(fail_time_5_ms);
		return 0;
	}

	/*
	 * Compute public key h = g/f mod X^n+1 mod q. We must do it
	 * even if the caller did not ask for it, because we must report
	 * a failure if f is not invertible modulo X^n+1 modulo q.
	 */
	if (h == NULL) {
		h = (uint16_t *)tmp;
		tmp = (uint32_t *)(h + n);
	}
	switch (q) {
	case 257:
		if (!ctl_make_public_257(h, f, g, logn, tmp)) {
			return 0;
		}
		break;
	default:
		return 0;
	}

	return 1;
}

/* see inner.h */
int
ctl_keygen_solve_FG(int8_t *F, int8_t *G, const int8_t *f, const int8_t *g,
	uint32_t q, unsigned logn, uint32_t *tmp)
{
	int lim;
	int ok;

	kg_diag_stats.solve_fg_calls ++;

	/*
	 * Solve the NTRU equation to get F and G.
	 * (caution: solve_NTRU() expects values in Falcon order, hence
	 * the swap (f,g) <-> (g,f) and (F,G) <-> (G,F))
	 */
	switch (q) {
	case 257: if (logn == 0 || logn > 9) { kg_diag_stats.solve_fg_fail_bad_param ++; return 0; } break;
	case 769: if (logn == 0 || logn > 10) { kg_diag_stats.solve_fg_fail_bad_param ++; return 0; } break;
	case 3329: if (logn != 11) { kg_diag_stats.solve_fg_fail_bad_param ++; return 0; } break;
	default:
		kg_diag_stats.solve_fg_fail_bad_param ++;
		return 0;
	}
	lim = (1 << (ctl_max_FG_bits[logn] - 1)) - 1;
#if CTL_USE_NTRUGEN_SOLVER
	ok = (ctl_ntrugen_solve_FG(F, G, f, g, q, logn, tmp) == 0);
#else
	ok = solve_NTRU(logn, G, F, g, f, q, lim, (uint32_t *)tmp);
#endif
	if (ok) {
		kg_diag_stats.solve_fg_success ++;
	}
	return ok;
}

/* see inner.h */
int
ctl_keygen_solve_FG_3329(int16_t *F, int16_t *G,
	const int8_t *f, const int8_t *g, unsigned logn, uint32_t *tmp)
{
	int ok;
	int32_t F32[1 << 11];
	int32_t G32[1 << 11];
	int lim;
	size_t u, n;

	kg_diag_stats.solve_fg_calls ++;
	if (logn != 11) {
		kg_diag_stats.solve_fg_fail_bad_param ++;
		return 0;
	}
	n = (size_t)1 << logn;
	lim = (1 << (ctl_max_FG_bits[logn] - 1)) - 1;

	/*
	 * The clean solver uses the Falcon convention:
	 *   f*G - g*F = q
	 * CTL uses:
	 *   g*F - f*G = q
	 * Therefore, solve on swapped (g, f), then swap the outputs back
	 * into CTL order.
	 */
	ok = solve_ntru_3329_2048_clean(g, f, G32, F32, NULL, NULL);
	if (!ok) {
		return 0;
	}

	for (u = 0; u < n; u ++) {
		if (F32[u] < -lim || F32[u] > +lim) {
			kg_diag_stats.solve_fg_fail_poly_small_f ++;
			return 0;
		}
		if (G32[u] < -lim || G32[u] > +lim) {
			kg_diag_stats.solve_fg_fail_poly_small_g ++;
			return 0;
		}
		F[u] = (int16_t)F32[u];
		G[u] = (int16_t)G32[u];
	}
	if (!verify_NTRU_i16(g, f, G, F, 3329, logn, tmp)) {
		kg_diag_stats.solve_fg_fail_verify ++;
		return 0;
	}
	if (ok) {
		kg_diag_stats.solve_fg_success ++;
	}
	return ok;
}

/* see inner.h */
int
ctl_keygen_rebuild_G(int8_t *G,
	const int8_t *f, const int8_t *g, const int8_t *F,
	uint32_t q, unsigned logn, uint32_t *tmp)
{
	/*
	 * Since g*F - f*G = q, and f is invertible modulo q, we can
	 * recompute G modulo q with:
	 *   G = (g*F)/f  mod X^n+1 mod q
	 * The coefficients of G are supposed to be small (they must fit
	 * in -127..+127), hence G itself can be obtained from G modulo q.
	 */
	switch (q) {

	case 257:
		if (logn == 0 || logn > 9) {
			return 0;
		}
		return ctl_rebuild_G_257(G, f, g, F, logn, tmp);

	default:
		return 0;
	}
}

/* see inner.h */
int
ctl_keygen_verify_FG(
	const int8_t *f, const int8_t *g, const int8_t *F, const int8_t *G,
	uint32_t q, unsigned logn, uint32_t *tmp)
{
	switch (q) {
	case 257: if (logn == 0 || logn > 9) { return 0; } break;
	case 769: if (logn == 0 || logn > 10) { return 0; } break;
	case 3329: if (logn != 11) { return 0; } break;
	default:
		return 0;
	}
	return verify_NTRU(g, f, G, F, q, logn, tmp);
}

/* see inner.h */
int
ctl_keygen_verify_FG_3329_i16(
	const int8_t *f, const int8_t *g, const int16_t *F, const int16_t *G,
	unsigned logn, uint32_t *tmp)
{
	if (logn != 11) {
		return 0;
	}
	return verify_NTRU_i16(g, f, G, F, 3329, logn, tmp);
}

/* see inner.h */
int
ctl_keygen_compute_w(int32_t *w, const int8_t *f, const int8_t *g,
	const int8_t *F, const int8_t *G, uint32_t q, unsigned logn,
	uint32_t *tmp)
{
	unsigned gamma2;
	uint64_t dnorm, max_dnorm;
	double alpha, t_gprime, lim2;
	struct timeval step11_start, step11_end;
	double fail_time_11_ms;	

	kg_diag_stats.compute_w_calls ++;

	if (!keygen_get_k_gamma2(q, logn, NULL, &gamma2)) {
		kg_diag_stats.compute_w_fail_bad_param ++;
		return 0;
	}
	alpha = keygen_solve_alpha(q, logn, gamma2, 64513);


	t_gprime = keygen_framework_bounds(q, logn, gamma2, 64513, alpha);
	if (!isfinite(alpha) || !isfinite(t_gprime) || t_gprime <= 0.0) {
		kg_diag_stats.compute_w_fail_bounds ++;
		return 0;
	}

	lim2 = ((double)64513.0 * t_gprime);
	lim2 *= lim2;
	if (!isfinite(lim2) || lim2 < 0.0) {
		kg_diag_stats.compute_w_fail_bounds ++;
		return 0;
	}
	if (lim2 >= (double)UINT64_MAX) {
		max_dnorm = UINT64_MAX;
	} else {
		max_dnorm = (uint64_t)ceil(lim2);
	}

	if (!compute_w(w, g, f, G, F, 64513, gamma2, logn, tmp)) {
		switch (kg_cw_last_fail) {
		case KG_CW_FAIL_LIM1:
			kg_diag_stats.compute_w_fail_lim1 ++;
			break;
		case KG_CW_FAIL_DIV:
			kg_diag_stats.compute_w_fail_div ++;
			break;
		case KG_CW_FAIL_FNR_ABS:
			kg_diag_stats.compute_w_fail_fnr_abs ++;
			break;
		case KG_CW_FAIL_ROUND_RANGE:
			kg_diag_stats.compute_w_fail_round_range ++;
			break;
		default:
			kg_diag_stats.compute_w_fail_internal_other ++;
			break;
		}
		return 0;
	}


	/*
	 * Verify that (gamma*Fd, Gd) is small enough.
	 */
	dnorm = compute_dnorm(g, f, G, F, w, 64513, gamma2, logn, tmp);

	//  瀵偓婵?Step11 閻楃懓鐣剧拋鈩冩
	gettimeofday(&step11_start, NULL);

	if (dnorm > max_dnorm)
	{
		kg_diag_stats.compute_w_fail_dnorm ++;
		gettimeofday(&step11_end, NULL);
		fail_time_11_ms = (step11_end.tv_sec - step11_start.tv_sec) * 1000.0 +
						  (step11_end.tv_usec - step11_start.tv_usec) / 1000.0;

		// 缁鳖垰濮炵紒鐔活吀閺佺増宓?
		record_step11_failure(fail_time_11_ms);

		return 0;
	}
	
	kg_diag_stats.compute_w_success ++;
	return 1;
}

/* see inner.h */
int
ctl_keygen_compute_w_3329(int32_t *w, const int8_t *f, const int8_t *g,
	const int16_t *F, const int16_t *G, unsigned logn, uint32_t *tmp)
{
	unsigned gamma2;
	uint64_t dnorm, max_dnorm;
	double alpha, t_gprime, lim2;

	kg_diag_stats.compute_w_calls ++;
	if (logn != 11 || !keygen_get_k_gamma2(3329, logn, NULL, &gamma2)) {
		kg_diag_stats.compute_w_fail_bad_param ++;
		return 0;
	}
	alpha = keygen_solve_alpha(3329, logn, gamma2, 64513);
	t_gprime = keygen_framework_bounds(3329, logn, gamma2, 64513, alpha);
	if (!isfinite(alpha) || !isfinite(t_gprime) || t_gprime <= 0.0) {
		kg_diag_stats.compute_w_fail_bounds ++;
		return 0;
	}
	lim2 = ((double)64513.0 * t_gprime);
	lim2 *= lim2;
	if (!isfinite(lim2) || lim2 < 0.0) {
		kg_diag_stats.compute_w_fail_bounds ++;
		return 0;
	}
	if (lim2 >= (double)UINT64_MAX) {
		max_dnorm = UINT64_MAX;
	} else {
		max_dnorm = (uint64_t)ceil(lim2);
	}

	if (!compute_w_i16(w, g, f, G, F, 64513, gamma2, logn, tmp)) {
		switch (kg_cw_last_fail) {
		case KG_CW_FAIL_LIM1: kg_diag_stats.compute_w_fail_lim1 ++; break;
		case KG_CW_FAIL_DIV: kg_diag_stats.compute_w_fail_div ++; break;
		case KG_CW_FAIL_FNR_ABS: kg_diag_stats.compute_w_fail_fnr_abs ++; break;
		case KG_CW_FAIL_ROUND_RANGE: kg_diag_stats.compute_w_fail_round_range ++; break;
		default: kg_diag_stats.compute_w_fail_internal_other ++; break;
		}
		return 0;
	}

	dnorm = compute_dnorm_i16(g, f, G, F, w, 64513, gamma2, logn, tmp);
	if (dnorm > max_dnorm) {
		kg_diag_stats.compute_w_fail_dnorm ++;
		return 0;
	}
	kg_diag_stats.compute_w_success ++;
	return 1;
}



