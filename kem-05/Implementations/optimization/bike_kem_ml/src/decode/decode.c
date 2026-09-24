/* Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0"
 *
 * Written by Nir Drucker, Shay Gueron and Dusan Kostic,
 * AWS Cryptographic Algorithms Group.
 *
 * [1] The optimizations are based on the description developed in the paper:
 *     Drucker, Nir, and Shay Gueron. 2019. “A Toolbox for Software Optimization
 *     of QC-MDPC Code-Based Cryptosystems.” Journal of Cryptographic Engineering,
 *     January, 1–17. https://doi.org/10.1007/s13389-018-00200-4.
 *
 * [2] The decoder algorithm is the Black-Gray decoder in
 *     the early submission of CAKE (due to N. Sandrier and R Misoczki).
 *
 * [3] The analysis for the constant time implementation is given in
 *     Drucker, Nir, Shay Gueron, and Dusan Kostic. 2019.
 *     “On Constant-Time QC-MDPC Decoding with Negligible Failure Rate.”
 *     Cryptology EPrint Archive, 2019. https://eprint.iacr.org/2019/1289.
 *
 * [4] it was adapted to BGF in:
 *     Drucker, Nir, Shay Gueron, and Dusan Kostic. 2019.
 *     “QC-MDPC decoders with several shades of gray.”
 *     Cryptology EPrint Archive, 2019. To be published.
 *
 * [5] Chou, T.: QcBits: Constant-Time Small-Key Code-Based Cryptography.
 *     In: Gier-lichs, B., Poschmann, A.Y. (eds.) Cryptographic Hardware
 *     and Embedded Systems– CHES 2016. pp. 280–300. Springer Berlin Heidelberg,
 *     Berlin, Heidelberg (2016)
 *
 * [6] The rotate512_small funciton is a derivative of the code described in:
 *     Guimarães, Antonio, Diego F Aranha, and Edson Borin. 2019.
 *     “Optimized Implementation of QC-MDPC Code-Based Cryptography.”
 *     Concurrency and Computation: Practice and Experience 31 (18):
 *     e5089. https://doi.org/10.1002/cpe.5089.
 */

#include "decode.h"
#include "cleanup.h"
#include "decode_internal.h"
#include "gf2x.h"
#include "mlthre_model.h"
#include "mlthre_sampling.h"
#include "utilities.h"

#include <math.h>

#if !defined(BIKE_MLTHRE_ENABLED)
#  define BIKE_MLTHRE_ENABLED 1
#endif

#if !defined(BIKE_MLTHRE_DELTA_ENABLED)
#  define BIKE_MLTHRE_DELTA_ENABLED 1
#endif

// Decoding (bit-flipping) parameter
#if defined(BG_DECODER)
#  if(LEVEL == 1)
#    define MAX_IT 3
#  elif(LEVEL == 3)
#    define MAX_IT 4
#  else
#    error "Level can only be 1/3"
#  endif
#elif defined(BGF_DECODER)
#  define MAX_IT 5
#endif

#if defined(BIKE_MAX_IT_OVERRIDE) && (BIKE_MAX_IT_OVERRIDE > 0)
#  undef MAX_IT
#  define MAX_IT BIKE_MAX_IT_OVERRIDE
#endif

void compute_syndrome(OUT syndrome_t *syndrome,
                      IN const pad_r_t *c0,
                      IN const pad_r_t *h0,
                      IN const decode_ctx *ctx)
{
  DEFER_CLEANUP(pad_r_t pad_s, pad_r_cleanup);

  gf2x_mod_mul(&pad_s, c0, h0);

  bike_memcpy((uint8_t *)syndrome->qw, pad_s.val.raw, R_BYTES);
  ctx->dup(syndrome);
}

_INLINE_ void recompute_syndrome(OUT syndrome_t *syndrome,
                                 IN const pad_r_t *c0,
                                 IN const pad_r_t *h0,
                                 IN const pad_r_t *pk,
                                 IN const e_t *e,
                                 IN const decode_ctx *ctx)
{
  DEFER_CLEANUP(pad_r_t tmp_c0, pad_r_cleanup);
  DEFER_CLEANUP(pad_r_t e0 = {0}, pad_r_cleanup);
  DEFER_CLEANUP(pad_r_t e1 = {0}, pad_r_cleanup);

  e0.val = e->val[0];
  e1.val = e->val[1];

  // tmp_c0 = pk * e1 + c0 + e0
  gf2x_mod_mul(&tmp_c0, &e1, pk);
  gf2x_mod_add(&tmp_c0, &tmp_c0, c0);
  gf2x_mod_add(&tmp_c0, &tmp_c0, &e0);

  // Recompute the syndrome using the updated ciphertext
  compute_syndrome(syndrome, &tmp_c0, h0, ctx);
}

#define MUL64HIGH(c, a, b)                           \
  do {                                               \
    uint64_t a_lo, a_hi, b_lo, b_hi;                 \
    a_lo = a & 0xffffffff;                           \
    b_lo = b & 0xffffffff;                           \
    a_hi = a >> 32;                                  \
    b_hi = b >> 32;                                  \
    c = a_hi*b_hi + ((a_hi*b_lo + a_lo*b_hi) >> 32); \
  } while(0)

#define MLTHRE_THRESHOLD_SCALE MLTHRE_MODEL_FEATURE_SCALE
#define MLTHRE_COEFF_SHIFT    20

#if(LEVEL == 1)
#  define MLTHRE_VAR_A_Q  2842U
#  define MLTHRE_VAR_B_Q  1679028U
#  define MLTHRE_MARGIN_Q (3U * MLTHRE_THRESHOLD_SCALE)
#elif(LEVEL == 3)
#  define MLTHRE_VAR_A_Q  3400U
#  define MLTHRE_VAR_B_Q  1217055U
#  define MLTHRE_MARGIN_Q (5U * MLTHRE_THRESHOLD_SCALE)
#else
#  define MLTHRE_VAR_A_Q  3950U
#  define MLTHRE_VAR_B_Q  968615U
#  define MLTHRE_MARGIN_Q (6U * MLTHRE_THRESHOLD_SCALE)
#endif

_INLINE_ uint32_t ct_max_u32(IN const uint32_t a, IN const uint32_t b)
{
  const uint32_t mask = secure_l32_mask(a, b);
  return (u32_barrier(mask) & a) | (u32_barrier(~mask) & b);
}

_INLINE_ uint32_t ct_min_u32(IN const uint32_t a, IN const uint32_t b)
{
  const uint32_t mask = secure_l32_mask(a, b);
  return (u32_barrier(~mask) & a) | (u32_barrier(mask) & b);
}

_INLINE_ uint32_t ceil_q_to_u32(IN const uint32_t x)
{
  return (x + (MLTHRE_THRESHOLD_SCALE - 1U)) / MLTHRE_THRESHOLD_SCALE;
}

_INLINE_ uint8_t clamp_threshold(IN const int32_t threshold)
{
  const uint32_t min_threshold = (D + 1U) / 2U;
  const uint32_t lower_clamped = ct_max_u32((uint32_t)threshold, min_threshold);
  return (uint8_t)ct_min_u32(lower_clamped, D);
}

_INLINE_ uint32_t mlthre_var_threshold_q(IN const uint32_t syndrome_weight)
{
  const uint64_t product = (uint64_t)MLTHRE_VAR_B_Q * syndrome_weight;
  return MLTHRE_VAR_A_Q + (uint32_t)(product >> MLTHRE_COEFF_SHIFT);
}

#if(LEVEL == 7) && defined(BIKE_L7_DYNAMIC_THRESHOLD)
#  if !defined(BIKE_L7_EMPIRICAL_THRESHOLD) || (BIKE_L7_EMPIRICAL_THRESHOLD == 0)
static double l7_lnbino(IN const size_t n, IN const size_t t)
{
  if((t == 0U) || (n == t)) {
    return 0.0;
  }

  return lgamma((double)n + 1.0) - lgamma((double)t + 1.0) -
         lgamma((double)(n - t) + 1.0);
}

static double l7_xlny(IN const double x, IN const double y)
{
  if(x == 0.0) {
    return 0.0;
  }

  return x * log(y);
}

static double l7_lnbinomialpmf(IN const size_t n,
                               IN const size_t k,
                               IN const double p,
                               IN const double q)
{
  return l7_lnbino(n, k) + l7_xlny((double)k, p) +
         l7_xlny((double)(n - k), q);
}

static double l7_Euh_log(IN const size_t n,
                         IN const size_t w,
                         IN const size_t t,
                         IN const size_t i)
{
  return l7_lnbino(w, i) + l7_lnbino(n - w, t - i) - l7_lnbino(n, t);
}

static double l7_iks(IN const size_t r,
                     IN const size_t n,
                     IN const size_t w,
                     IN const size_t t)
{
  (void)r;

  double x = 0.0;
  double denom = 0.0;

  for(size_t i = 1U; (i < 10U) && (i < t); i += 2U) {
    const double e = exp(l7_Euh_log(n, w, t, i));
    x += (double)(i - 1U) * e;
    denom += e;
  }

  if(denom == 0.0) {
    return 0.0;
  }

  return x / denom;
}

static size_t l7_compute_threshold(IN const size_t r,
                                   IN const size_t n,
                                   IN const size_t d,
                                   IN const size_t w,
                                   IN const size_t S,
                                   IN const size_t t)
{
  const double x = l7_iks(r, n, w, t) * (double)S;
  const double p =
    (((double)(w - 1U) * (double)S) - x) / (double)(n - t) / (double)d;
  const double q = ((double)S + x) / (double)t / (double)d;
  size_t threshold;

  if((p >= 1.0) || (p > q)) {
    threshold = d;
  } else if(q >= 1.0) {
    double diff = 0.0;
    threshold = d + 1U;
    do {
      threshold--;
      diff = -exp(l7_lnbinomialpmf(d, threshold, p, 1.0 - p)) *
               (double)(n - t) +
             1.0;
    } while((diff >= 0.0) && (threshold > ((d + 1U) / 2U)));
    threshold = (threshold < d) ? threshold + 1U : d;
  } else {
    double diff = 0.0;
    threshold = d + 1U;
    do {
      threshold--;
      diff = -exp(l7_lnbinomialpmf(d, threshold, p, 1.0 - p)) *
               (double)(n - t) +
             exp(l7_lnbinomialpmf(d, threshold, q, 1.0 - q)) * (double)t;
    } while((diff >= 0.0) && (threshold > ((d + 1U) / 2U)));
    threshold = (threshold < d) ? threshold + 1U : d;
  }

  if(threshold < 1U) {
    return 1U;
  }
  if(threshold > d) {
    return d;
  }
  return threshold;
}
#  endif

_INLINE_ uint32_t l7_dynamic_threshold(IN const uint32_t syndrome_weight)
{
#  if defined(BIKE_L7_EMPIRICAL_THRESHOLD) && (BIKE_L7_EMPIRICAL_THRESHOLD != 0)
#    if !defined(BIKE_L7_EMP_TH_DEN)
#      define BIKE_L7_EMP_TH_DEN 1000000000ULL
#    endif
#    if !defined(BIKE_L7_EMP_TH_A_NUM)
#      define BIKE_L7_EMP_TH_A_NUM 136560997000ULL
#    endif
#    if !defined(BIKE_L7_EMP_TH_B_NUM)
#      define BIKE_L7_EMP_TH_B_NUM 173435ULL
#    endif
#    if !defined(BIKE_L7_EMP_TH_MIN)
#      define BIKE_L7_EMP_TH_MIN 138U
#    endif

  const uint64_t scaled = BIKE_L7_EMP_TH_A_NUM +
                          (BIKE_L7_EMP_TH_B_NUM * (uint64_t)syndrome_weight);
  uint32_t threshold =
    (uint32_t)((scaled + (BIKE_L7_EMP_TH_DEN / 2ULL)) / BIKE_L7_EMP_TH_DEN);
  threshold = ct_max_u32(threshold, BIKE_L7_EMP_TH_MIN);
  return ct_min_u32(threshold, D);
#  else
  return clamp_threshold((int32_t)l7_compute_threshold(R_BITS,
                                                       N_BITS,
                                                       D,
                                                       2U * D,
                                                       syndrome_weight,
                                                       T));
#  endif
}
#endif

_INLINE_ uint32_t mlthre_lower_bound_q(IN const uint32_t initial_weight,
                                       IN const uint32_t iteration)
{
  const uint32_t t0_q        = mlthre_var_threshold_q(initial_weight);
  const uint32_t min_q       = ((D + 1U) * MLTHRE_THRESHOLD_SCALE) / 2U;
  const uint32_t transition_q = (t0_q - min_q) / 3U;

  if(iteration == 1U) {
    return t0_q + MLTHRE_MARGIN_Q;
  }
  if(iteration == 2U) {
    return t0_q + MLTHRE_MARGIN_Q - transition_q;
  }
  if(iteration == 3U) {
    return t0_q + MLTHRE_MARGIN_Q - (2U * transition_q);
  }
  return min_q + MLTHRE_MARGIN_Q;
}

_INLINE_ uint8_t get_spec_threshold(IN const uint32_t syndrome_weight)
{
  // The threshold coefficients are defined in the spec as floating point values.
  // Since we want to avoid floating point operations for constant-timeness,
  // we use integer arithmetic to compute the threshold.
  // For example, in the case of Level-1 parameters, instead of having:
  //   T0 = 13.530 and T1 = 0.0069722,
  // we multipy the values by 10^8 and work with integers:
  //   T0' = 1353000000 and T1' = 697220.
  // Then, instead of computing the threshold by:
  //   T0 + T1*S,
  // we compute:
  //   (T0' + T1'*S)/10^8,
  // where S is the syndrome weight. Additionally, instead of dividing by 10^8,
  // we compute the result by a multiplication and a right shift (both
  // constant-time operations), as described in:
  //   https://dl.acm.org/doi/pdf/10.1145/178243.178249
  uint64_t thr  = THRESHOLD_COEFF0 + (THRESHOLD_COEFF1 * syndrome_weight);
  MUL64HIGH(thr, thr, THRESHOLD_MUL_CONST);
  thr >>= THRESHOLD_SHR_CONST;

  const uint32_t mask = secure_l32_mask((uint32_t)thr, THRESHOLD_MIN);
  thr = (u32_barrier(mask) & thr) | (u32_barrier(~mask) & THRESHOLD_MIN);

  DMSG("    Threshold: %lu\n", thr);
  return thr;
}

_INLINE_ uint32_t mlthre_base_threshold(IN const uint32_t current_weight,
                                        IN const uint32_t initial_weight,
                                        IN const uint32_t iteration)
{
#if(LEVEL == 7) && defined(BIKE_L7_DYNAMIC_THRESHOLD)
  (void)initial_weight;
  (void)iteration;
  return l7_dynamic_threshold(current_weight);
#else
  const uint32_t var_threshold_q = mlthre_var_threshold_q(current_weight);
  const uint32_t lower_bound_q   = mlthre_lower_bound_q(initial_weight, iteration);
  return clamp_threshold((int32_t)ceil_q_to_u32(
    ct_max_u32(var_threshold_q, lower_bound_q)));
#endif
}

_INLINE_ void mlthre_z_counts(OUT uint32_t *z0,
                              OUT uint32_t *z1,
                              IN const syndrome_t *current,
                              IN const syndrome_t *previous)
{
  const r_t *cur  = (const r_t *)current->qw;
  const r_t *prev = (const r_t *)previous->qw;

  *z0 = 0;
  *z1 = 0;

  for(size_t i = 0; i < (R_BYTES - 1); i++) {
    *z1 += __builtin_popcount(cur->raw[i] & prev->raw[i]);
    *z0 += __builtin_popcount(cur->raw[i] & (uint8_t)~prev->raw[i]);
  }

  const uint8_t cur_last  = cur->raw[R_BYTES - 1] & LAST_R_BYTE_MASK;
  const uint8_t prev_last = prev->raw[R_BYTES - 1] & LAST_R_BYTE_MASK;
  *z1 += __builtin_popcount(cur_last & prev_last);
  *z0 += __builtin_popcount(cur_last & (uint8_t)~prev_last);
}

_INLINE_ uint8_t get_threshold(IN const syndrome_t *s,
                               IN const syndrome_t *prev_s,
                               IN const uint32_t    initial_weight,
                               IN const uint32_t    prev_weight,
                               IN const uint32_t    iteration)
{
  bike_static_assert(sizeof(*s) >= sizeof(r_t), syndrome_is_large_enough);

  const uint32_t current_weight = (uint32_t)r_bits_vector_weight((const r_t *)s->qw);

#if(LEVEL == 7) && defined(BIKE_L7_THRESHOLD_SCHEDULE)
  (void)prev_s;
  (void)initial_weight;
  (void)prev_weight;
  if(iteration == 1U) {
    return BIKE_L7_TH1;
  }
  if(iteration == 2U) {
    return BIKE_L7_TH2;
  }
  if(iteration == 3U) {
    return BIKE_L7_TH3;
  }
  if(iteration == 4U) {
    return BIKE_L7_TH4;
  }
  return BIKE_L7_TH5;
#endif

#if !defined(BIKE_MLTHRE_ENABLED) || (BIKE_MLTHRE_ENABLED == 0)
  (void)prev_s;
  (void)initial_weight;
  (void)prev_weight;
  (void)iteration;
  return get_spec_threshold(current_weight);
#else
  const uint32_t base_threshold =
    mlthre_base_threshold(current_weight, initial_weight, iteration);
  int32_t delta = 0;

#if defined(BIKE_MLTHRE_DELTA_ENABLED) && (BIKE_MLTHRE_DELTA_ENABLED != 0)
  if(iteration <= 2U) {
    uint32_t z0 = 0;
    uint32_t z1 = 0;
    mlthre_z_counts(&z0, &z1, s, prev_s);

    const uint32_t effective_prev_weight =
      (iteration == 1U) ? current_weight : prev_weight;
    const uint32_t delta_mask =
      ~secure_l32_mask(current_weight, effective_prev_weight);
    const uint32_t syndrome_delta =
      (effective_prev_weight - current_weight) & u32_barrier(delta_mask);
#if(LEVEL == 7) && defined(BIKE_L7_DYNAMIC_THRESHOLD)
    const uint32_t var_threshold = l7_dynamic_threshold(current_weight);
    const uint32_t lower_bound   = l7_dynamic_threshold(initial_weight);
#else
    const uint32_t var_threshold =
      ceil_q_to_u32(mlthre_var_threshold_q(current_weight));
    const uint32_t lower_bound =
      ceil_q_to_u32(mlthre_lower_bound_q(initial_weight, iteration));
#endif
    const int32_t features[MLTHRE_MODEL_INPUT_DIM] = {
      (int32_t)(LEVEL * MLTHRE_MODEL_FEATURE_SCALE),
      (int32_t)(iteration * MLTHRE_MODEL_FEATURE_SCALE),
      (int32_t)(initial_weight * MLTHRE_MODEL_FEATURE_SCALE),
      (int32_t)(effective_prev_weight * MLTHRE_MODEL_FEATURE_SCALE),
      (int32_t)(current_weight * MLTHRE_MODEL_FEATURE_SCALE),
      (int32_t)(syndrome_delta * MLTHRE_MODEL_FEATURE_SCALE),
      (int32_t)(z0 * MLTHRE_MODEL_FEATURE_SCALE),
      (int32_t)(z1 * MLTHRE_MODEL_FEATURE_SCALE),
      (int32_t)(var_threshold * MLTHRE_MODEL_FEATURE_SCALE),
      (int32_t)(lower_bound * MLTHRE_MODEL_FEATURE_SCALE),
      (int32_t)(base_threshold * MLTHRE_MODEL_FEATURE_SCALE)};

    delta = mlthre_model_predict_delta(features);
  }
#else
  (void)prev_s;
  (void)prev_weight;
#endif

  const uint8_t threshold = clamp_threshold((int32_t)base_threshold + delta);
  DMSG("    ML threshold: base=%u delta=%d applied=%u\n",
       base_threshold,
       delta,
       threshold);
  return threshold;
#endif
}

// Calculate the Unsatisfied Parity Checks (UPCs) and update the errors
// vector (e) accordingly. In addition, update the black and gray errors vector
// with the relevant values.
_INLINE_ void find_err1(OUT e_t *e,
                        OUT e_t *black_e,
                        OUT e_t *gray_e,
                        IN const syndrome_t *          syndrome,
                        IN const compressed_idx_d_ar_t wlist,
                        IN const uint8_t               threshold,
                        IN const decode_ctx *ctx)
{
  // This function uses the bit-slice-adder methodology of [5]:
  DEFER_CLEANUP(syndrome_t rotated_syndrome = {0}, syndrome_cleanup);
  DEFER_CLEANUP(upc_t upc, upc_cleanup);

  for(uint32_t i = 0; i < N0; i++) {
    // UPC must start from zero at every iteration
    bike_memset(&upc, 0, sizeof(upc));

    // 1) Right-rotate the syndrome for every secret key set bit index
    //    Then slice-add it to the UPC array.
    for(size_t j = 0; j < D; j++) {
      ctx->rotate_right(&rotated_syndrome, syndrome, wlist[i].val[j]);
      ctx->bit_sliced_adder(&upc, &rotated_syndrome, LOG2_MSB(j + 1));
    }

    // 2) Subtract the threshold from the UPC counters
    ctx->bit_slice_full_subtract(&upc, threshold);

    // 3) Update the errors and the black errors vectors.
    //    The last slice of the UPC array holds the MSB of the accumulated values
    //    minus the threshold. Every zero bit indicates a potential error bit.
    //    The errors values are stored in the black array and xored with the
    //    errors Of the previous iteration.
    const r_t *last_slice = &(upc.slice[SLICES - 1].u.r.val);
    for(size_t j = 0; j < R_BYTES; j++) {
      const uint8_t sum_msb  = (~last_slice->raw[j]);
      black_e->val[i].raw[j] = sum_msb;
      e->val[i].raw[j] ^= sum_msb;
    }

    // Ensure that the padding bits (upper bits of the last byte) are zero so
    // they will not be included in the multiplication and in the hash function.
    e->val[i].raw[R_BYTES - 1] &= LAST_R_BYTE_MASK;

    // 4) Calculate the gray error array by adding "DELTA" to the UPC array.
    //    For that we reuse the rotated_syndrome variable setting it to all "1".
    const uint32_t gray_delta = (uint32_t)DELTA;
    for(uint32_t l = 0; l < gray_delta; l++) {
      bike_memset((uint8_t *)rotated_syndrome.qw, 0xff, R_BYTES);
      ctx->bit_sliced_adder(&upc, &rotated_syndrome, SLICES);
    }

    // 5) Update the gray list with the relevant bits that are not
    //    set in the black list.
    for(size_t j = 0; j < R_BYTES; j++) {
      const uint8_t sum_msb = (~last_slice->raw[j]);
      gray_e->val[i].raw[j] = (~(black_e->val[i].raw[j])) & sum_msb;
    }
  }
}

// Recalculate the UPCs and update the errors vector (e) according to it
// and to the black/gray vectors.
_INLINE_ void find_err2(OUT e_t *e,
                        IN e_t * pos_e,
                        IN const syndrome_t *          syndrome,
                        IN const compressed_idx_d_ar_t wlist,
                        IN const uint8_t               threshold,
                        IN const decode_ctx *ctx)
{
  DEFER_CLEANUP(syndrome_t rotated_syndrome = {0}, syndrome_cleanup);
  DEFER_CLEANUP(upc_t upc, upc_cleanup);

  for(uint32_t i = 0; i < N0; i++) {
    // UPC must start from zero at every iteration
    bike_memset(&upc, 0, sizeof(upc));

    // 1) Right-rotate the syndrome, for every index of a set bit in the secret
    // key. Then slice-add it to the UPC array.
    for(size_t j = 0; j < D; j++) {
      ctx->rotate_right(&rotated_syndrome, syndrome, wlist[i].val[j]);
      ctx->bit_sliced_adder(&upc, &rotated_syndrome, LOG2_MSB(j + 1));
    }

    // 2) Subtract the threshold from the UPC counters
    ctx->bit_slice_full_subtract(&upc, threshold);

    // 3) Update the errors vector.
    //    The last slice of the UPC array holds the MSB of the accumulated values
    //    minus the threshold. Every zero bit indicates a potential error bit.
    const r_t *last_slice = &(upc.slice[SLICES - 1].u.r.val);
    for(size_t j = 0; j < R_BYTES; j++) {
      const uint8_t sum_msb = (~last_slice->raw[j]);
      e->val[i].raw[j] ^= (pos_e->val[i].raw[j] & sum_msb);
    }

    // Ensure that the padding bits (upper bits of the last byte) are zero, so
    // they are not included in the multiplication, and in the hash function.
    e->val[i].raw[R_BYTES - 1] &= LAST_R_BYTE_MASK;
  }
}

#if(LEVEL == 7) && defined(BIKE_L7_REFERENCE_DECODER) && \
  (BIKE_L7_REFERENCE_DECODER != 0)

_INLINE_ uint32_t l7_ref_weight(IN const uint8_t *v, IN const uint32_t len)
{
  uint32_t count = 0;
  for(uint32_t i = 0; i < len; i++) {
    count += v[i];
  }
  return count;
}

_INLINE_ void l7_ref_syndrome_from_bytes(OUT uint8_t s[R_BITS],
                                         IN const uint8_t raw[R_BYTES])
{
  bike_memset(s, 0, R_BITS);
  s[0] = raw[0] & 1U;
  for(uint32_t i = 1; i < R_BITS; i++) {
    const uint32_t row_pos = R_BITS - i;
    s[i] = (raw[row_pos >> 3] >> (row_pos & 7U)) & 1U;
  }
}

_INLINE_ void l7_ref_compute_syndrome(OUT uint8_t s[R_BITS],
                                      IN const pad_r_t *c0,
                                      IN const pad_r_t *h0)
{
  DEFER_CLEANUP(pad_r_t tmp, pad_r_cleanup);
  gf2x_mod_mul(&tmp, c0, h0);
  l7_ref_syndrome_from_bytes(s, tmp.val.raw);
}

_INLINE_ void l7_ref_get_col(OUT idx_t col[D], IN const idx_t row[D])
{
  if(row[0] == 0U) {
    col[0] = 0U;
    for(uint32_t i = 1; i < D; i++) {
      col[i] = R_BITS - row[D - i];
    }
  } else {
    for(uint32_t i = 0; i < D; i++) {
      col[i] = R_BITS - row[D - 1U - i];
    }
  }
}

_INLINE_ uint32_t l7_ref_ctr(IN const idx_t col[D],
                             IN const uint32_t position,
                             IN const uint8_t s[R_BITS])
{
  uint32_t count = 0;
  for(uint32_t i = 0; i < D; i++) {
    count += s[(col[i] + position) % R_BITS];
  }
  return count;
}

_INLINE_ void l7_ref_recompute_syndrome(IN OUT uint8_t s[R_BITS],
                                        IN const uint32_t pos,
                                        IN const idx_t h0[D],
                                        IN const idx_t h1[D])
{
  const idx_t *h = (pos < R_BITS) ? h0 : h1;
  const uint32_t local_pos = (pos < R_BITS) ? pos : (pos - R_BITS);

  for(uint32_t j = 0; j < D; j++) {
    if(h[j] <= local_pos) {
      s[local_pos - h[j]] ^= 1U;
    } else {
      s[R_BITS - h[j] + local_pos] ^= 1U;
    }
  }
}

_INLINE_ uint32_t l7_ref_adjusted_error_position(IN const uint32_t position)
{
  if((position == 0U) || (position == R_BITS)) {
    return position;
  }

  if(position > R_BITS) {
    return (N_BITS - position) + R_BITS;
  }
  return R_BITS - position;
}

_INLINE_ void l7_ref_flip_error(IN OUT uint8_t e[N_BITS],
                                IN const uint32_t position)
{
  e[l7_ref_adjusted_error_position(position)] ^= 1U;
}

_INLINE_ void l7_ref_z_counts(OUT uint32_t *z0,
                              OUT uint32_t *z1,
                              IN const uint8_t current[R_BITS],
                              IN const uint8_t previous[R_BITS])
{
  *z0 = 0;
  *z1 = 0;
  for(uint32_t i = 0; i < R_BITS; i++) {
    if(current[i]) {
      if(previous[i]) {
        (*z1)++;
      } else {
        (*z0)++;
      }
    }
  }
}

_INLINE_ uint8_t l7_ref_threshold(IN const uint8_t s[R_BITS],
                                  IN const uint8_t prev_s[R_BITS],
                                  IN const uint32_t initial_weight,
                                  IN const uint32_t prev_weight,
                                  IN const uint32_t iteration)
{
  const uint32_t current_weight = l7_ref_weight(s, R_BITS);
  uint32_t base_threshold = 0;
  int32_t delta = 0;
  (void)iteration;

#  if defined(BIKE_L7_THRESHOLD_SCHEDULE)
  if(iteration == 1U) {
    base_threshold = BIKE_L7_TH1;
  } else if(iteration == 2U) {
    base_threshold = BIKE_L7_TH2;
  } else if(iteration == 3U) {
    base_threshold = BIKE_L7_TH3;
  } else if(iteration == 4U) {
    base_threshold = BIKE_L7_TH4;
  } else {
    base_threshold = BIKE_L7_TH5;
  }
#  elif defined(BIKE_L7_DYNAMIC_THRESHOLD)
  base_threshold = l7_dynamic_threshold(current_weight);
#  else
  base_threshold = get_spec_threshold(current_weight);
#  endif

#  if defined(BIKE_MLTHRE_ENABLED) && (BIKE_MLTHRE_ENABLED != 0) && \
    defined(BIKE_MLTHRE_DELTA_ENABLED) && (BIKE_MLTHRE_DELTA_ENABLED != 0)
  if(current_weight != 0U) {
    uint32_t z0 = 0;
    uint32_t z1 = 0;
    l7_ref_z_counts(&z0, &z1, s, prev_s);

    const uint32_t effective_prev_weight =
      (iteration == 1U) ? current_weight : prev_weight;
    const uint32_t syndrome_delta =
      (effective_prev_weight > current_weight)
        ? (effective_prev_weight - current_weight)
        : 0U;
    const uint32_t var_threshold = l7_dynamic_threshold(current_weight);
    const uint32_t lower_bound   = l7_dynamic_threshold(initial_weight);
    const int32_t features[MLTHRE_MODEL_INPUT_DIM] = {
      (int32_t)(LEVEL * MLTHRE_MODEL_FEATURE_SCALE),
      (int32_t)(iteration * MLTHRE_MODEL_FEATURE_SCALE),
      (int32_t)(initial_weight * MLTHRE_MODEL_FEATURE_SCALE),
      (int32_t)(effective_prev_weight * MLTHRE_MODEL_FEATURE_SCALE),
      (int32_t)(current_weight * MLTHRE_MODEL_FEATURE_SCALE),
      (int32_t)(syndrome_delta * MLTHRE_MODEL_FEATURE_SCALE),
      (int32_t)(z0 * MLTHRE_MODEL_FEATURE_SCALE),
      (int32_t)(z1 * MLTHRE_MODEL_FEATURE_SCALE),
      (int32_t)(var_threshold * MLTHRE_MODEL_FEATURE_SCALE),
      (int32_t)(lower_bound * MLTHRE_MODEL_FEATURE_SCALE),
      (int32_t)(base_threshold * MLTHRE_MODEL_FEATURE_SCALE)};

    delta = mlthre_model_predict_delta(features);
  }
#  else
  (void)prev_s;
  (void)initial_weight;
  (void)prev_weight;
#  endif

  const uint8_t threshold = clamp_threshold((int32_t)base_threshold + delta);
  DMSG("    L7 ref threshold: base=%u delta=%d applied=%u\n",
       base_threshold,
       delta,
       threshold);
  return threshold;
}

_INLINE_ void l7_ref_bf_masked_iter(IN OUT uint8_t e[N_BITS],
                                    IN OUT uint8_t s[R_BITS],
                                    IN const uint8_t mask[N_BITS],
                                    IN const uint32_t threshold,
                                    IN const idx_t h0[D],
                                    IN const idx_t h1[D],
                                    IN const idx_t h0_col[D],
                                    IN const idx_t h1_col[D])
{
  uint8_t pos[N_BITS] = {0};

  for(uint32_t j = 0; j < R_BITS; j++) {
    if((l7_ref_ctr(h0_col, j, s) >= threshold) && mask[j]) {
      l7_ref_flip_error(e, j);
      pos[j] = 1U;
    }
  }

  for(uint32_t j = 0; j < R_BITS; j++) {
    const uint32_t p = R_BITS + j;
    if((l7_ref_ctr(h1_col, j, s) >= threshold) && mask[p]) {
      l7_ref_flip_error(e, p);
      pos[p] = 1U;
    }
  }

  for(uint32_t j = 0; j < N_BITS; j++) {
    if(pos[j] == 1U) {
      l7_ref_recompute_syndrome(s, j, h0, h1);
    }
  }
}

_INLINE_ void l7_ref_bf_iter(IN OUT uint8_t e[N_BITS],
                             OUT uint8_t black[N_BITS],
                             OUT uint8_t gray[N_BITS],
                             IN OUT uint8_t s[R_BITS],
                             IN const uint32_t threshold,
                             IN const idx_t h0[D],
                             IN const idx_t h1[D],
                             IN const idx_t h0_col[D],
                             IN const idx_t h1_col[D])
{
  uint8_t pos[N_BITS] = {0};
  const uint32_t gray_threshold =
    (threshold > DELTA) ? (threshold - DELTA) : 0U;

  for(uint32_t j = 0; j < R_BITS; j++) {
    const uint32_t counter = l7_ref_ctr(h0_col, j, s);
    if(counter >= threshold) {
      l7_ref_flip_error(e, j);
      pos[j] = 1U;
      black[j] = 1U;
    } else if(counter >= gray_threshold) {
      gray[j] = 1U;
    }
  }

  for(uint32_t j = 0; j < R_BITS; j++) {
    const uint32_t p = R_BITS + j;
    const uint32_t counter = l7_ref_ctr(h1_col, j, s);
    if(counter >= threshold) {
      l7_ref_flip_error(e, p);
      pos[p] = 1U;
      black[p] = 1U;
    } else if(counter >= gray_threshold) {
      gray[p] = 1U;
    }
  }

  for(uint32_t j = 0; j < N_BITS; j++) {
    if(pos[j] == 1U) {
      l7_ref_recompute_syndrome(s, j, h0, h1);
    }
  }
}

_INLINE_ void l7_ref_pack_error(OUT e_t *out, IN const uint8_t e_bits[N_BITS])
{
  bike_memset(out, 0, sizeof(*out));
  for(uint32_t i = 0; i < R_BITS; i++) {
    if(e_bits[i]) {
      out->val[0].raw[i >> 3] |= (uint8_t)(1U << (i & 7U));
    }
    if(e_bits[R_BITS + i]) {
      out->val[1].raw[i >> 3] |= (uint8_t)(1U << (i & 7U));
    }
  }

  out->val[0].raw[R_BYTES - 1] &= LAST_R_BYTE_MASK;
  out->val[1].raw[R_BYTES - 1] &= LAST_R_BYTE_MASK;
}

#  if defined(BIKE_MLTHRE_SAMPLING) && (BIKE_MLTHRE_SAMPLING != 0)

typedef struct l7_ref_candidate_outcome_s {
  uint32_t threshold;
  uint32_t residual;
  uint32_t syndrome_weight;
  uint32_t guess_weight;
} l7_ref_candidate_outcome_t;

_INLINE_ uint32_t l7_ref_abs_diff(IN const uint32_t a, IN const uint32_t b)
{
  return (a > b) ? (a - b) : (b - a);
}

_INLINE_ int l7_ref_candidate_is_better(
  IN const l7_ref_candidate_outcome_t *candidate,
  IN const l7_ref_candidate_outcome_t *best,
  IN const uint32_t applied_threshold)
{
  if(candidate->residual != best->residual) {
    return candidate->residual < best->residual;
  }
  if(candidate->syndrome_weight != best->syndrome_weight) {
    return candidate->syndrome_weight < best->syndrome_weight;
  }
  return l7_ref_abs_diff(candidate->threshold, applied_threshold) <
         l7_ref_abs_diff(best->threshold, applied_threshold);
}

_INLINE_ void l7_ref_compute_counters(OUT uint16_t counters[N_BITS],
                                      IN const uint8_t s[R_BITS],
                                      IN const idx_t h0_col[D],
                                      IN const idx_t h1_col[D])
{
  for(uint32_t j = 0; j < R_BITS; j++) {
    counters[j] = (uint16_t)l7_ref_ctr(h0_col, j, s);
    counters[R_BITS + j] = (uint16_t)l7_ref_ctr(h1_col, j, s);
  }
}

_INLINE_ void l7_ref_apply_threshold_from_counters(
  IN OUT uint8_t e_bits[N_BITS],
  IN OUT uint8_t s[R_BITS],
  IN const uint16_t counters[N_BITS],
  IN const uint32_t threshold,
  IN const idx_t h0[D],
  IN const idx_t h1[D])
{
  uint8_t pos[N_BITS] = {0};

  for(uint32_t j = 0; j < N_BITS; j++) {
    if(counters[j] >= threshold) {
      l7_ref_flip_error(e_bits, j);
      pos[j] = 1U;
    }
  }

  for(uint32_t j = 0; j < N_BITS; j++) {
    if(pos[j] == 1U) {
      l7_ref_recompute_syndrome(s, j, h0, h1);
    }
  }
}

static void l7_ref_sampling_record_threshold_candidates(
  IN const uint8_t e_bits[N_BITS],
  IN const uint8_t s[R_BITS],
  IN const uint8_t prev_s[R_BITS],
  IN const idx_t h0[D],
  IN const idx_t h1[D],
  IN const idx_t h0_col[D],
  IN const idx_t h1_col[D],
  IN const uint32_t initial_weight,
  IN const uint32_t prev_weight,
  IN const uint32_t iteration,
  IN const uint32_t applied_threshold)
{
  if(!mlthre_sampling_enabled()) {
    return;
  }

  enum {
    L7_REF_MIN_THRESHOLD = ((D + 1U) / 2U),
    L7_REF_MAX_CANDIDATES = (D - L7_REF_MIN_THRESHOLD + 1U)
  };

  l7_ref_candidate_outcome_t outcomes[L7_REF_MAX_CANDIDATES];
  l7_ref_candidate_outcome_t best = {0};
  uint32_t count = 0;
  uint32_t z0 = 0;
  uint32_t z1 = 0;
  uint16_t counters[N_BITS] = {0};

  const uint32_t current_weight = l7_ref_weight(s, R_BITS);
  const uint32_t effective_prev_weight =
    (iteration == 1U) ? current_weight : prev_weight;
  const uint32_t syndrome_delta =
    (effective_prev_weight > current_weight)
      ? (effective_prev_weight - current_weight)
      : 0U;
  const uint32_t var_threshold = l7_dynamic_threshold(current_weight);
  const uint32_t lower_bound   = l7_dynamic_threshold(initial_weight);
  l7_ref_z_counts(&z0, &z1, s, prev_s);
  l7_ref_compute_counters(counters, s, h0_col, h1_col);

  for(uint32_t threshold = L7_REF_MIN_THRESHOLD; threshold <= D; threshold++) {
    uint8_t candidate_e_bits[N_BITS] = {0};
    uint8_t candidate_s[R_BITS] = {0};
    e_t candidate_e = {0};

    bike_memcpy(candidate_e_bits, e_bits, sizeof(candidate_e_bits));
    bike_memcpy(candidate_s, s, sizeof(candidate_s));

    l7_ref_apply_threshold_from_counters(candidate_e_bits,
                                         candidate_s,
                                         counters,
                                         threshold,
                                         h0,
                                         h1);
    l7_ref_pack_error(&candidate_e, candidate_e_bits);

    outcomes[count].threshold = threshold;
    outcomes[count].residual = mlthre_sampling_residual(&candidate_e);
    outcomes[count].syndrome_weight = l7_ref_weight(candidate_s, R_BITS);
    outcomes[count].guess_weight = l7_ref_weight(candidate_e_bits, N_BITS);

    if((count == 0U) ||
       l7_ref_candidate_is_better(
         &outcomes[count], &best, applied_threshold)) {
      best = outcomes[count];
    }
    count++;
  }

  for(uint32_t i = 0; i < count; i++) {
    const int32_t delta_from_applied =
      (int32_t)outcomes[i].threshold - (int32_t)applied_threshold;
    const int32_t best_delta =
      (int32_t)best.threshold - (int32_t)applied_threshold;
    const uint32_t is_best_threshold =
      (outcomes[i].threshold == best.threshold) ? 1U : 0U;

    mlthre_sampling_record_candidate(mlthre_sampling_current_sample(),
                                     LEVEL,
                                     iteration,
                                     initial_weight,
                                     effective_prev_weight,
                                     current_weight,
                                     syndrome_delta,
                                     z0,
                                     z1,
                                     var_threshold,
                                     lower_bound,
                                     applied_threshold,
                                     outcomes[i].threshold,
                                     delta_from_applied,
                                     outcomes[i].residual,
                                     outcomes[i].syndrome_weight,
                                     outcomes[i].guess_weight,
                                     best.threshold,
                                     best_delta,
                                     best.residual,
                                     best.syndrome_weight,
                                     best.guess_weight,
                                     is_best_threshold);
  }
}

#  endif

static void decode_l7_reference(OUT e_t *e, IN const ct_t *ct, IN const sk_t *sk)
{
  DEFER_CLEANUP(pad_r_t c0 = {0}, pad_r_cleanup);
  DEFER_CLEANUP(pad_r_t h0 = {0}, pad_r_cleanup);
  c0.val = ct->c0;
  h0.val = sk->bin[0];

  uint8_t s[R_BITS] = {0};
  uint8_t prev_s[R_BITS] = {0};
  uint8_t e_bits[N_BITS] = {0};
  uint8_t black[N_BITS] = {0};
  uint8_t gray[N_BITS] = {0};
  idx_t h0_col[D] = {0};
  idx_t h1_col[D] = {0};

  l7_ref_compute_syndrome(s, &c0, &h0);
  bike_memcpy(prev_s, s, sizeof(prev_s));
  l7_ref_get_col(h0_col, sk->wlist[0].val);
  l7_ref_get_col(h1_col, sk->wlist[1].val);

  const uint32_t initial_weight = l7_ref_weight(s, R_BITS);
  uint32_t prev_weight = initial_weight;

  for(uint32_t iter = 1U; iter <= MAX_IT; iter++) {
    bike_memset(black, 0, sizeof(black));
    bike_memset(gray, 0, sizeof(gray));

    const uint8_t threshold =
      l7_ref_threshold(s, prev_s, initial_weight, prev_weight, iter);
    DMSG("    L7 reference iteration: %u\n", iter);
    DMSG("    Weight of e: %u\n", l7_ref_weight(e_bits, N_BITS));
    DMSG("    Weight of syndrome: %u\n", l7_ref_weight(s, R_BITS));
    DMSG("    Threshold: %u\n", threshold);

#  if defined(BIKE_MLTHRE_SAMPLING) && (BIKE_MLTHRE_SAMPLING != 0)
    l7_ref_sampling_record_threshold_candidates(e_bits,
                                                s,
                                                prev_s,
                                                sk->wlist[0].val,
                                                sk->wlist[1].val,
                                                h0_col,
                                                h1_col,
                                                initial_weight,
                                                prev_weight,
                                                iter,
                                                threshold);
#  endif

    l7_ref_bf_iter(e_bits,
                   black,
                   gray,
                   s,
                   threshold,
                   sk->wlist[0].val,
                   sk->wlist[1].val,
                   h0_col,
                   h1_col);

    if(iter == 1U) {
      const uint32_t masked_threshold = ((D + 1U) / 2U) + 1U;
      l7_ref_bf_masked_iter(e_bits,
                            s,
                            black,
                            masked_threshold,
                            sk->wlist[0].val,
                            sk->wlist[1].val,
                            h0_col,
                            h1_col);
      l7_ref_bf_masked_iter(e_bits,
                            s,
                            gray,
                            masked_threshold,
                            sk->wlist[0].val,
                            sk->wlist[1].val,
                            h0_col,
                            h1_col);
    }

    bike_memcpy(prev_s, s, sizeof(prev_s));
    prev_weight = l7_ref_weight(s, R_BITS);
  }

  DMSG("    Final syndrome weight: %u\n", l7_ref_weight(s, R_BITS));
  l7_ref_pack_error(e, e_bits);
}

#endif

#if defined(BIKE_MLTHRE_SAMPLING) && (BIKE_MLTHRE_SAMPLING != 0)

#  define MLTHRE_SAMPLING_MIN_THRESHOLD ((D + 1U) / 2U)
#  define MLTHRE_SAMPLING_MAX_CANDIDATES \
    (D - MLTHRE_SAMPLING_MIN_THRESHOLD + 1U)

typedef struct mlthre_candidate_outcome_s {
  uint8_t  threshold;
  uint32_t residual;
  uint32_t syndrome_weight;
  uint32_t guess_weight;
} mlthre_candidate_outcome_t;

_INLINE_ uint32_t mlthre_abs_diff_u32(IN const uint32_t a, IN const uint32_t b)
{
  return (a > b) ? (a - b) : (b - a);
}

_INLINE_ uint32_t mlthre_error_weight(IN const e_t *e)
{
  return (uint32_t)(r_bits_vector_weight(&e->val[0]) +
                    r_bits_vector_weight(&e->val[1]));
}

_INLINE_ int mlthre_candidate_is_better(
  IN const mlthre_candidate_outcome_t *candidate,
  IN const mlthre_candidate_outcome_t *best,
  IN const uint32_t                    applied_threshold)
{
  if(candidate->residual != best->residual) {
    return candidate->residual < best->residual;
  }
  if(candidate->syndrome_weight != best->syndrome_weight) {
    return candidate->syndrome_weight < best->syndrome_weight;
  }
  return mlthre_abs_diff_u32(candidate->threshold, applied_threshold) <
         mlthre_abs_diff_u32(best->threshold, applied_threshold);
}

static void mlthre_sampling_record_threshold_candidates(
  IN const e_t *             e,
  IN const syndrome_t *      s,
  IN const syndrome_t *      prev_s,
  IN const pad_r_t *         c0,
  IN const pad_r_t *         h0,
  IN const pad_r_t *         pk,
  IN const sk_t *            sk,
  IN const decode_ctx *      ctx,
  IN const uint32_t          initial_weight,
  IN const uint32_t          prev_weight,
  IN const uint32_t          iteration,
  IN const uint32_t          applied_threshold)
{
  mlthre_candidate_outcome_t outcomes[MLTHRE_SAMPLING_MAX_CANDIDATES];
  mlthre_candidate_outcome_t best = {0};
  uint32_t                   z0   = 0;
  uint32_t                   z1   = 0;
  uint32_t                   count = 0;

  if(!mlthre_sampling_enabled()) {
    return;
  }

  const uint32_t current_weight =
    (uint32_t)r_bits_vector_weight((const r_t *)s->qw);
  const uint32_t effective_prev_weight =
    (iteration == 1U) ? current_weight : prev_weight;
  const uint32_t syndrome_delta =
    (effective_prev_weight > current_weight)
      ? (effective_prev_weight - current_weight)
      : 0U;
  const uint32_t var_threshold =
    ceil_q_to_u32(mlthre_var_threshold_q(current_weight));
  const uint32_t lower_bound =
    ceil_q_to_u32(mlthre_lower_bound_q(initial_weight, iteration));
  mlthre_z_counts(&z0, &z1, s, prev_s);

  for(uint32_t threshold = MLTHRE_SAMPLING_MIN_THRESHOLD; threshold <= D;
      threshold++) {
    e_t        candidate_e     = *e;
    e_t        candidate_black = {0};
    e_t        candidate_gray  = {0};
    syndrome_t candidate_s     = *s;

    find_err1(&candidate_e,
              &candidate_black,
              &candidate_gray,
              &candidate_s,
              sk->wlist,
              (uint8_t)threshold,
              ctx);
    recompute_syndrome(&candidate_s, c0, h0, pk, &candidate_e, ctx);

    outcomes[count].threshold       = (uint8_t)threshold;
    outcomes[count].residual        = mlthre_sampling_residual(&candidate_e);
    outcomes[count].syndrome_weight =
      (uint32_t)r_bits_vector_weight((const r_t *)candidate_s.qw);
    outcomes[count].guess_weight = mlthre_error_weight(&candidate_e);

    if((count == 0U) ||
       mlthre_candidate_is_better(&outcomes[count], &best, applied_threshold)) {
      best = outcomes[count];
    }
    count++;
  }

  for(uint32_t i = 0; i < count; i++) {
    const int32_t delta_from_applied =
      (int32_t)outcomes[i].threshold - (int32_t)applied_threshold;
    const int32_t best_delta =
      (int32_t)best.threshold - (int32_t)applied_threshold;
    const uint32_t is_best_threshold =
      (outcomes[i].threshold == best.threshold) ? 1U : 0U;

    mlthre_sampling_record_candidate(mlthre_sampling_current_sample(),
                                     LEVEL,
                                     iteration,
                                     initial_weight,
                                     effective_prev_weight,
                                     current_weight,
                                     syndrome_delta,
                                     z0,
                                     z1,
                                     var_threshold,
                                     lower_bound,
                                     applied_threshold,
                                     outcomes[i].threshold,
                                     delta_from_applied,
                                     outcomes[i].residual,
                                     outcomes[i].syndrome_weight,
                                     outcomes[i].guess_weight,
                                     best.threshold,
                                     best_delta,
                                     best.residual,
                                     best.syndrome_weight,
                                     best.guess_weight,
                                     is_best_threshold);
  }
}

#endif

void decode(OUT e_t *e, IN const ct_t *ct, IN const sk_t *sk)
{
#if(LEVEL == 7) && defined(BIKE_L7_REFERENCE_DECODER) && \
  (BIKE_L7_REFERENCE_DECODER != 0)
  decode_l7_reference(e, ct, sk);
  return;
#endif

  // Initialize the decode methods struct
  decode_ctx ctx;
  decode_ctx_init(&ctx);

  DEFER_CLEANUP(e_t black_e = {0}, e_cleanup);
  DEFER_CLEANUP(e_t gray_e = {0}, e_cleanup);

  DEFER_CLEANUP(pad_r_t c0 = {0}, pad_r_cleanup);
  DEFER_CLEANUP(pad_r_t h0 = {0}, pad_r_cleanup);
  pad_r_t pk = {0};

  // Pad ciphertext (c0), secret key (h0), and public key (h)
  c0.val = ct->c0;
  h0.val = sk->bin[0];
  pk.val = sk->pk;

  DEFER_CLEANUP(syndrome_t s = {0}, syndrome_cleanup);
  DMSG("  Computing s.\n");
  compute_syndrome(&s, &c0, &h0, &ctx);
  ctx.dup(&s);
  DEFER_CLEANUP(syndrome_t prev_s = {0}, syndrome_cleanup);
  prev_s = s;
  const uint32_t initial_weight = (uint32_t)r_bits_vector_weight((r_t *)s.qw);
  uint32_t       prev_weight    = initial_weight;

  // Reset (init) the error because it is xored in the find_err functions.
  bike_memset(e, 0, sizeof(*e));

  for(uint32_t iter = 0; iter < MAX_IT; iter++) {
    const uint8_t threshold =
      get_threshold(&s, &prev_s, initial_weight, prev_weight, iter + 1U);

    DMSG("    Iteration: %d\n", iter);
    DMSG("    Weight of e: %lu\n",
         r_bits_vector_weight(&e->val[0]) + r_bits_vector_weight(&e->val[1]));
    DMSG("    Weight of syndrome: %lu\n", r_bits_vector_weight((r_t *)s.qw));

#if defined(BIKE_MLTHRE_SAMPLING) && (BIKE_MLTHRE_SAMPLING != 0)
    mlthre_sampling_record_threshold_candidates(e,
                                                &s,
                                                &prev_s,
                                                &c0,
                                                &h0,
                                                &pk,
                                                sk,
                                                &ctx,
                                                initial_weight,
                                                prev_weight,
                                                iter + 1U,
                                                threshold);
#endif

    find_err1(e, &black_e, &gray_e, &s, sk->wlist, threshold, &ctx);
    recompute_syndrome(&s, &c0, &h0, &pk, e, &ctx);
#if defined(BGF_DECODER)
    if(iter >= 1) {
      prev_s      = s;
      prev_weight = (uint32_t)r_bits_vector_weight((r_t *)s.qw);
      continue;
    }
#endif
    DMSG("    Weight of e: %lu\n",
         r_bits_vector_weight(&e->val[0]) + r_bits_vector_weight(&e->val[1]));
    DMSG("    Weight of syndrome: %lu\n", r_bits_vector_weight((r_t *)s.qw));

    find_err2(e, &black_e, &s, sk->wlist, ((D + 1) / 2) + 1, &ctx);
    recompute_syndrome(&s, &c0, &h0, &pk, e, &ctx);

    DMSG("    Weight of e: %lu\n",
         r_bits_vector_weight(&e->val[0]) + r_bits_vector_weight(&e->val[1]));
    DMSG("    Weight of syndrome: %lu\n", r_bits_vector_weight((r_t *)s.qw));

    find_err2(e, &gray_e, &s, sk->wlist, ((D + 1) / 2) + 1, &ctx);
    recompute_syndrome(&s, &c0, &h0, &pk, e, &ctx);

    prev_s      = s;
    prev_weight = (uint32_t)r_bits_vector_weight((r_t *)s.qw);
  }
}
