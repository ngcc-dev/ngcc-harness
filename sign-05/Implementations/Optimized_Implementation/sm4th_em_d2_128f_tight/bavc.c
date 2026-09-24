#include "bavc.h"
#include "random_oracle.h"
#include "compat.h"
#include "prg.h"
#include "params.h"
#include "universal_hashing.h"
#include "auxfunc.h"
#include "sig_timing.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef SM4TH_COMPONENT_TIMING_VERBOSE
#define SM4TH_COMPONENT_TIMING_VERBOSE 0
#endif

#if SM4TH_COMPONENT_TIMING && SM4TH_COMPONENT_TIMING_VERBOSE
static void bavc_detail_timing_print(const char* label, double elapsed_ms) {
  fprintf(stderr, "[sm4th timing] %-56s %9.6f ms\n", label, elapsed_ms);
}
#else
static inline void bavc_detail_timing_print(const char* label, double elapsed_ms) {
  (void)label;
  (void)elapsed_ms;
}
#endif

#define NODE(nodes, node, lambda_bytes) (&nodes[(node) * (lambda_bytes)])
#define VOLE_TWEAK_OFFSET UINT32_C(0x80000000)

/*
 * Portable H1 batch finalize helper.
 * It preserves the 4-lane scheduling structure while using auxfunc.c hash
 * entry points (sm3hash/pseudohash/pseudoXOF).
 */
static int h1_finalize_batch4_same_len_portable(unsigned int out_bytes_len,
                                                const uint8_t* const msg[4], size_t msg_len,
                                                uint8_t* const digest[4], size_t lane_count,
                                                uint8_t* scratch, size_t scratch_stride) {
  if (!msg || !digest || lane_count == 0u || lane_count > 4u) {
    return -1;
  }
  for (size_t lane = 0; lane < lane_count; ++lane) {
    if (!msg[lane] || !digest[lane]) {
      return -1;
    }
  }

  if (msg_len > SIZE_MAX - 1u) {
    return -1;
  }
  const size_t msg_with_domain_len = msg_len + 1u;
  if (!scratch || scratch_stride < msg_with_domain_len || scratch_stride > SIZE_MAX / 4u) {
    return -1;
  }

  const int output_bits = (int)(out_bytes_len * 8u);
  const unsigned long long bit_len = (unsigned long long)msg_with_domain_len * 8ULL;

  for (size_t lane = 0; lane < lane_count; ++lane) {
    uint8_t* msg_lane = scratch + lane * scratch_stride;
    if (msg_len) {
      memcpy(msg_lane, msg[lane], msg_len);
    }
    msg_lane[msg_len] = 1u; /* H1 domain_sep */

    int rc = 0;
    if (output_bits == 256) {
      rc = sm3hash(output_bits, msg_lane, bit_len, digest[lane]);
    } else if (output_bits == 512 || output_bits == 768 || output_bits == 1024) {
      rc = pseudohash(output_bits, msg_lane, bit_len, digest[lane]);
    } else {
      rc = pseudoXOF((unsigned long long)output_bits, msg_lane, bit_len, digest[lane]);
    }
    if (rc != 0) {
      return rc;
    }
  }

  return 0;
}

static inline int h1_finalize_batch4_same_len_dispatch(unsigned int out_bytes_len,
                                                       const uint8_t* const msg[4], size_t msg_len,
                                                       uint8_t* const digest[4], size_t lane_count,
                                                       uint8_t* scratch, size_t scratch_stride) {
  return h1_finalize_batch4_same_len_portable(out_bytes_len, msg, msg_len, digest, lane_count,
                                              scratch, scratch_stride);
}

static void em_leaf_commit_columns_from_nodes(const params_t* params, const uint8_t* iv,
                                              const uint8_t* leaf_nodes, uint8_t* sd_out,
                                              uint8_t* com_out, uint8_t* vole_out_170) {
  const unsigned int tau = params->tau;
  const unsigned int tau1 = params->tau1;
  const unsigned int k = params->k;
  const unsigned int L = params->L;
  const unsigned int seed_bytes = params->lambda_prg_bytes;
  const unsigned int prg_lambda = params->lambda_prg;
  const unsigned int ell_hat_bytes = params->ell_hat_bytes;
  const unsigned int com_size = seed_bytes * params->n_leaf;
  const unsigned int half_leaf_cnt = 1u << (k - 1);
  const size_t tau_stride = (size_t)tau * seed_bytes;
  const size_t tau1_stride = (size_t)tau1 * seed_bytes;
  const uint8_t* second_leaf_nodes = leaf_nodes + (size_t)tau * half_leaf_cnt * seed_bytes;
  assert(params->lambda == 128u);
  assert(prg_lambda == 256u);
  assert(ell_hat_bytes == 170u);
  assert(vole_out_170 != NULL);

  // First layer: keep fixed tweak and large count to preserve the fast batch path.
  for (unsigned int i = 0; i < tau; ++i) {
    const uint32_t tweak = i + L - 1;
    const uint32_t vole_tweak = i ^ VOLE_TWEAK_OFFSET;
    const size_t dst_idx = (i < tau1) ? ((size_t)(2u * i) * half_leaf_cnt)
                                      : ((size_t)(i + tau1) * half_leaf_cnt);
    const uint8_t* key_col = leaf_nodes + (size_t)i * seed_bytes;
    prg_2_lambda_and_170_fixed_tweak_independent_batch_with_sd(
        key_col, tau_stride, iv, tweak, vole_tweak, sd_out + dst_idx * seed_bytes,
        seed_bytes, com_out + dst_idx * com_size, com_size,
        vole_out_170 + dst_idx * ell_hat_bytes, ell_hat_bytes, prg_lambda, half_leaf_cnt);
  }

  // Optional second layer for i < tau1.
  for (unsigned int i = 0; i < tau1; ++i) {
    const uint32_t tweak = i + L - 1;
    const uint32_t vole_tweak = i ^ VOLE_TWEAK_OFFSET;
    const size_t dst_idx = ((size_t)(2u * i + 1u)) * half_leaf_cnt;
    const uint8_t* key_col = second_leaf_nodes + (size_t)i * seed_bytes;
    prg_2_lambda_and_170_fixed_tweak_independent_batch_with_sd(
        key_col, tau1_stride, iv, tweak, vole_tweak, sd_out + dst_idx * seed_bytes,
        seed_bytes, com_out + dst_idx * com_size, com_size,
        vole_out_170 + dst_idx * ell_hat_bytes, ell_hat_bytes, prg_lambda, half_leaf_cnt);
  }
}

static void seed_prg_em_pipeline_commit(const params_t* params, uint8_t* nodes, const uint8_t* iv,
                                        uint8_t* sd_out, uint8_t* com_out,
                                        uint8_t* vole_out_170) {
  const unsigned int L = params->L;
  const unsigned int seed_bytes = params->lambda_prg_bytes;
  const unsigned int prg_lambda = params->lambda_prg;
  const size_t out_stride = 2u * (size_t)seed_bytes;

  if (L > 1) {
    unsigned int level_start = 0;
    unsigned int level_count = 1;
    while (level_start < (L - 1)) {
      if (level_start + level_count > (L - 1)) {
        level_count = (L - 1) - level_start;
      }
      {
        prg_2_lambda_independent_batch(
            NODE(nodes, level_start, seed_bytes), seed_bytes, iv, level_start,
            NODE(nodes, 2 * level_start + 1, seed_bytes), out_stride, prg_lambda,
            level_count);
      }
      level_start += level_count;
      level_count <<= 1;
    }
  }

  em_leaf_commit_columns_from_nodes(params, iv, NODE(nodes, L - 1, seed_bytes), sd_out,
                                    com_out, vole_out_170);
}

// BAVC.PosInTree
static inline unsigned int pos_in_tree(const params_t* params, unsigned int i,
                                       unsigned int j) {
  const unsigned int tmp = 1u << (params->k - 1);
  if (j < tmp) {
    return params->L - 1 + params->tau * j + i;
  }
  // mod 2^(k-1) is the same as & 2^(k-1)-1
  const unsigned int mask = tmp - 1;
  return params->L - 1 + params->tau * tmp + params->tau1 * (j & mask) + i;
}

static bool bavc_mark_open_path(const params_t* params, const uint16_t* i_delta,
                                uint8_t* on_path, unsigned int* path_nodes) {
  const unsigned int tau = params->tau;
  unsigned int marked = 0;
  memset(on_path, 0, (size_t)(2 * params->L - 1));

  for (unsigned int i = 0; i < tau; ++i) {
    unsigned int alpha = pos_in_tree(params, i, i_delta[i]);
    for (;;) {
      if (on_path[alpha]) {
        break;
      }
      on_path[alpha] = 1;
      ++marked;
      if (alpha == 0) {
        break;
      }
      alpha = (alpha - 1) >> 1;
    }
  }

  if (path_nodes) {
    *path_nodes = marked;
  }
  return true;
}

typedef struct {
  uint8_t* on_path;
  size_t on_path_cap;
  uint8_t* em_keys;
  size_t em_keys_cap;
  uint8_t* em_com_batch;
  size_t em_com_batch_cap;
} bavc_scratch_cache_t;

static bavc_scratch_cache_t g_bavc_scratch = {0};

static bool bavc_mul_size(size_t a, size_t b, size_t* out) {
  if (a != 0 && b > SIZE_MAX / a) {
    return false;
  }
  *out = a * b;
  return true;
}

static bool bavc_cache_reserve(uint8_t** buf, size_t* cap, size_t need) {
  if (*cap >= need) {
    return true;
  }
  uint8_t* p = realloc(*buf, need);
  if (!p) {
    return false;
  }
  *buf = p;
  *cap = need;
  return true;
}

static bool bavc_on_path_scratch_get(size_t need, uint8_t** out) {
  if (!bavc_cache_reserve(&g_bavc_scratch.on_path, &g_bavc_scratch.on_path_cap, need)) {
    return false;
  }
  *out = g_bavc_scratch.on_path;
  return true;
}

static bool bavc_reconstruct_em_scratch_get(const params_t* params, unsigned int max_instances,
                                            unsigned int com_size, uint8_t** keys,
                                            uint8_t** com_batch, size_t* com_lane_stride) {
  if (params->L == 0) {
    return false;
  }

  const size_t node_count = (size_t)2u * params->L - 1u;
  size_t keys_need = 0;
  if (!bavc_mul_size(node_count, params->lambda_prg_bytes, &keys_need) ||
      !bavc_cache_reserve(&g_bavc_scratch.em_keys, &g_bavc_scratch.em_keys_cap, keys_need)) {
    return false;
  }
  memset(g_bavc_scratch.em_keys, 0, keys_need);
  *keys = g_bavc_scratch.em_keys;

  if (!bavc_mul_size((size_t)max_instances, com_size, com_lane_stride)) {
    return false;
  }
  size_t com_batch_need = 0;
  if (!bavc_mul_size((size_t)params->L, com_size, &com_batch_need) ||
      !bavc_cache_reserve(&g_bavc_scratch.em_com_batch, &g_bavc_scratch.em_com_batch_cap,
                          com_batch_need)) {
    return false;
  }
  *com_batch = g_bavc_scratch.em_com_batch;

  return true;
}

// BAVC.Commit (EM)
static void bavc_commit_em(const params_t* params,
                                bavc_t* bavc, const uint8_t* rootKey, const uint8_t* iv
                                ) {
  const unsigned int tau          = params->tau;
  const unsigned int tau1         = params->tau1;
  const unsigned int k            = params->k;
  const unsigned int L            = params->L;
  const unsigned int seed_bytes   = params->lambda_prg_bytes;
  const unsigned int com_size     = seed_bytes * params->n_leaf; // size of com_ij
  const unsigned int h1_bytes     = params->prg_block_bytes * params->n_leaf;
  const unsigned int ell_hat_bytes = params->ell_hat_bytes;

  SIG_TSTART(t_bavc_commit_em);
#if SM4TH_COMPONENT_TIMING && SM4TH_COMPONENT_TIMING_VERBOSE
  double bavc_detail_h1_nonupdate = 0.0;
  double bavc_detail_h1_update = 0.0;
  double bavc_detail_seed_leaf_prg = 0.0;
  double bavc_detail_other = 0.0;
  double bavc_detail_start = 0.0;
#endif

#if SM4TH_COMPONENT_TIMING && SM4TH_COMPONENT_TIMING_VERBOSE
  bavc_detail_start = sm4th_timing_now_ms();
#endif
  H1_context_t h1_com_ctx;
  H1_init(&h1_com_ctx);
#if SM4TH_COMPONENT_TIMING && SM4TH_COMPONENT_TIMING_VERBOSE
  bavc_detail_h1_nonupdate += sm4th_timing_now_ms() - bavc_detail_start;
#endif

  // Initialzing stuff
#if SM4TH_COMPONENT_TIMING && SM4TH_COMPONENT_TIMING_VERBOSE
  bavc_detail_start = sm4th_timing_now_ms();
#endif
  uint8_t* nodes = calloc(2 * L - 1, seed_bytes);
  assert(nodes);
  const bool precompute_sd_prg170 = (params->lambda_prg == 256u) && (ell_hat_bytes == 170u);
  assert(precompute_sd_prg170);
  bavc->h   = malloc(h1_bytes);
  bavc->com = malloc(L * com_size);
  bavc->sd  = malloc(L * seed_bytes);
  bavc->sd_prg170 = precompute_sd_prg170 ? malloc((size_t)L * ell_hat_bytes) : NULL;
  assert(bavc->h && bavc->com && bavc->sd);
  assert(!precompute_sd_prg170 || bavc->sd_prg170);
#if SM4TH_COMPONENT_TIMING && SM4TH_COMPONENT_TIMING_VERBOSE
  bavc_detail_other += sm4th_timing_now_ms() - bavc_detail_start;
#endif

  // Generating the tree and leaf commitments in one pipeline.
  memcpy(NODE(nodes, 0, seed_bytes), rootKey, seed_bytes);
  bavc->k = NODE(nodes, 0, seed_bytes);
  assert(bavc->k);
  // do seed_prg and leaf commit together to preserve parallelism and save some memory.
#if SM4TH_COMPONENT_TIMING && SM4TH_COMPONENT_TIMING_VERBOSE
  bavc_detail_start = sm4th_timing_now_ms();
#endif
  seed_prg_em_pipeline_commit(params, nodes, iv, bavc->sd, bavc->com, bavc->sd_prg170);
#if SM4TH_COMPONENT_TIMING && SM4TH_COMPONENT_TIMING_VERBOSE
  bavc_detail_seed_leaf_prg += sm4th_timing_now_ms() - bavc_detail_start;
#endif

  // Step: 1..3
  // Step: 4..5 + Step 10..12 over per-instance commitments.
  size_t com_off = 0;
  H1_context_t h1_ctx = {0};
  bool h1_ctx_inited = false;
  size_t h1_batch_scratch_stride = 0;
  uint8_t* h1_batch_scratch = NULL;
  bool use_h1_batch4 = false;

  {
    const size_t max_h1_msg_len = (size_t)bavc_max_node_index(0, tau1, k) * com_size;
    if (max_h1_msg_len < SIZE_MAX - 1u) {
      h1_batch_scratch_stride = max_h1_msg_len + 1u; /* msg || domain_sep */
      if (h1_batch_scratch_stride != 0u && h1_batch_scratch_stride <= SIZE_MAX / 4u) {
        h1_batch_scratch = malloc(4u * h1_batch_scratch_stride);
      }
    }
    use_h1_batch4 = (h1_batch_scratch != NULL);
  }

  if (use_h1_batch4) {
    uint8_t hi4[4][h1_bytes];
    const uint8_t* com_base = bavc->com;

    if (tau1 == 0u) {
      const unsigned int N_region = 1u << (k - 1u);
      const size_t msg_len_region = (size_t)N_region * com_size;
      for (unsigned int i = 0; i < tau;) {
        const size_t lane_count = (size_t)((tau - i >= 4u) ? 4u : (tau - i));
        const uint8_t* msg[4] = {NULL, NULL, NULL, NULL};
        uint8_t* digest[4] = {hi4[0], hi4[1], hi4[2], hi4[3]};

        const uint8_t* com_ptr = com_base + com_off * com_size;
        msg[0] = com_ptr;
        if (lane_count >= 2u) {
          msg[1] = com_ptr + msg_len_region;
        }
        if (lane_count >= 3u) {
          msg[2] = com_ptr + 2u * msg_len_region;
        }
        if (lane_count == 4u) {
          msg[3] = com_ptr + 3u * msg_len_region;
        }
        com_off += lane_count * (size_t)N_region;

#if SM4TH_COMPONENT_TIMING && SM4TH_COMPONENT_TIMING_VERBOSE
        bavc_detail_start = sm4th_timing_now_ms();
#endif
        const int h1_rc = h1_finalize_batch4_same_len_dispatch(
            h1_bytes, msg, msg_len_region, digest, lane_count, h1_batch_scratch,
            h1_batch_scratch_stride);
        assert(h1_rc == 0);
        (void)h1_rc;
#if SM4TH_COMPONENT_TIMING && SM4TH_COMPONENT_TIMING_VERBOSE
        bavc_detail_h1_nonupdate += sm4th_timing_now_ms() - bavc_detail_start;
        bavc_detail_start = sm4th_timing_now_ms();
#endif

        H1_update(&h1_com_ctx, hi4[0], lane_count * h1_bytes);
#if SM4TH_COMPONENT_TIMING && SM4TH_COMPONENT_TIMING_VERBOSE
        bavc_detail_h1_update += sm4th_timing_now_ms() - bavc_detail_start;
#endif
        i += (unsigned int)lane_count;
      }
    } else {
      /*
       * Split into two homogeneous regions:
       * - i in [0, tau1): N_i = 2^k
       * - i in [tau1, tau): N_i = 2^(k-1)
       * so each region has fixed msg_len and avoids mixed-lane boundary handling.
       */
      for (unsigned int region = 0; region < 2; ++region) {
        const unsigned int i_begin = (region == 0) ? 0u : tau1;
        const unsigned int i_end = (region == 0) ? tau1 : tau;
        if (i_begin >= i_end) {
          continue;
        }

        const unsigned int N_region = bavc_max_node_index(i_begin, tau1, k);
        const size_t msg_len_region = (size_t)N_region * com_size;

        for (unsigned int i = i_begin; i < i_end;) {
          const size_t lane_count = (size_t)((i_end - i >= 4u) ? 4u : (i_end - i));
          const uint8_t* msg[4] = {NULL, NULL, NULL, NULL};
          uint8_t* digest[4] = {hi4[0], hi4[1], hi4[2], hi4[3]};

          const uint8_t* com_ptr = com_base + com_off * com_size;
          if (lane_count == 4u) {
            msg[0] = com_ptr;
            msg[1] = com_ptr + msg_len_region;
            msg[2] = com_ptr + 2u * msg_len_region;
            msg[3] = com_ptr + 3u * msg_len_region;
          } else if (lane_count == 3u) {
            msg[0] = com_ptr;
            msg[1] = com_ptr + msg_len_region;
            msg[2] = com_ptr + 2u * msg_len_region;
          } else if (lane_count == 2u) {
            msg[0] = com_ptr;
            msg[1] = com_ptr + msg_len_region;
          } else {
            msg[0] = com_ptr;
          }
          com_off += lane_count * (size_t)N_region;

#if SM4TH_COMPONENT_TIMING && SM4TH_COMPONENT_TIMING_VERBOSE
          bavc_detail_start = sm4th_timing_now_ms();
#endif
          const int h1_rc = h1_finalize_batch4_same_len_dispatch(
              h1_bytes, msg, msg_len_region, digest, lane_count, h1_batch_scratch,
              h1_batch_scratch_stride);
          assert(h1_rc == 0);
          (void)h1_rc;
#if SM4TH_COMPONENT_TIMING && SM4TH_COMPONENT_TIMING_VERBOSE
          bavc_detail_h1_nonupdate += sm4th_timing_now_ms() - bavc_detail_start;
          bavc_detail_start = sm4th_timing_now_ms();
#endif

          H1_update(&h1_com_ctx, hi4[0], lane_count * h1_bytes);
#if SM4TH_COMPONENT_TIMING && SM4TH_COMPONENT_TIMING_VERBOSE
          bavc_detail_h1_update += sm4th_timing_now_ms() - bavc_detail_start;
#endif
          i += (unsigned int)lane_count;
        }
      }
    }

    if (h1_batch_scratch) {
      free(h1_batch_scratch);
    }
  } else {
    uint8_t hi[h1_bytes];
#if SM4TH_COMPONENT_TIMING && SM4TH_COMPONENT_TIMING_VERBOSE
    bavc_detail_start = sm4th_timing_now_ms();
#endif
    H1_init(&h1_ctx);
#if SM4TH_COMPONENT_TIMING && SM4TH_COMPONENT_TIMING_VERBOSE
    bavc_detail_h1_nonupdate += sm4th_timing_now_ms() - bavc_detail_start;
#endif
    h1_ctx_inited = true;
    for (unsigned int i = 0; i < tau; ++i) {
      const unsigned int N_i = bavc_max_node_index(i, tau1, k);
      const uint8_t* com_i_start = bavc->com + com_off * com_size;
#if SM4TH_COMPONENT_TIMING && SM4TH_COMPONENT_TIMING_VERBOSE
      bavc_detail_start = sm4th_timing_now_ms();
#endif
      H1_reset(&h1_ctx);
#if SM4TH_COMPONENT_TIMING && SM4TH_COMPONENT_TIMING_VERBOSE
      bavc_detail_h1_nonupdate += sm4th_timing_now_ms() - bavc_detail_start;
      bavc_detail_start = sm4th_timing_now_ms();
#endif

      H1_update(&h1_ctx, com_i_start, (size_t)N_i * com_size);
#if SM4TH_COMPONENT_TIMING && SM4TH_COMPONENT_TIMING_VERBOSE
      bavc_detail_h1_update += sm4th_timing_now_ms() - bavc_detail_start;
      bavc_detail_start = sm4th_timing_now_ms();
#endif

      // Step 11
      H1_final_keep(params, &h1_ctx, hi);
#if SM4TH_COMPONENT_TIMING && SM4TH_COMPONENT_TIMING_VERBOSE
      bavc_detail_h1_nonupdate += sm4th_timing_now_ms() - bavc_detail_start;
      bavc_detail_start = sm4th_timing_now_ms();
#endif
      // Step 12
      H1_update(&h1_com_ctx, hi, h1_bytes);
#if SM4TH_COMPONENT_TIMING && SM4TH_COMPONENT_TIMING_VERBOSE
      bavc_detail_h1_update += sm4th_timing_now_ms() - bavc_detail_start;
#endif
      com_off += N_i;
    }
  }

  // Step 12
  if (h1_ctx_inited) {
#if SM4TH_COMPONENT_TIMING && SM4TH_COMPONENT_TIMING_VERBOSE
    bavc_detail_start = sm4th_timing_now_ms();
#endif
    H1_clear(&h1_ctx);
#if SM4TH_COMPONENT_TIMING && SM4TH_COMPONENT_TIMING_VERBOSE
    bavc_detail_h1_nonupdate += sm4th_timing_now_ms() - bavc_detail_start;
#endif
  }
#if SM4TH_COMPONENT_TIMING && SM4TH_COMPONENT_TIMING_VERBOSE
  bavc_detail_start = sm4th_timing_now_ms();
#endif
  H1_final(params, &h1_com_ctx, bavc->h);
#if SM4TH_COMPONENT_TIMING && SM4TH_COMPONENT_TIMING_VERBOSE
  bavc_detail_h1_nonupdate += sm4th_timing_now_ms() - bavc_detail_start;
  bavc_detail_timing_print("detail.sign.bavc_commit_em.seed_leaf_prg.total",
                           bavc_detail_seed_leaf_prg);
  bavc_detail_timing_print("detail.sign.bavc_commit_em.h1_nonupdate",
                           bavc_detail_h1_nonupdate);
  bavc_detail_timing_print("detail.sign.bavc_commit_em.h1_update", bavc_detail_h1_update);
  bavc_detail_timing_print("detail.sign.bavc_commit_em.other", bavc_detail_other);
#endif
  SIG_TEND("bavc.commit.em", t_bavc_commit_em);
}

void bavc_commit(const params_t* params, bavc_t* bavc, const uint8_t* root_key,
                 const uint8_t* iv) {
  bavc_commit_em(params, bavc, root_key, iv);
}

bool bavc_open(const params_t* params, uint8_t* decom_i, const bavc_t* vc,
               const uint16_t* i_delta) {
  const unsigned int L            = params->L;
  const unsigned int seed_bytes   = params->lambda_prg_bytes;
  const unsigned int k            = params->k;
  const unsigned int tau          = params->tau;
  const unsigned int tau_1        = params->tau1;
  const unsigned int com_size     = params->n_leaf * seed_bytes;

  uint8_t* decom_i_end = decom_i + com_size * tau + params->t_open * seed_bytes;

  uint8_t* on_path = calloc(2 * L - 1, 1);
  if (!on_path) {
    return false;
  }

  unsigned int nh = 0;
  if (!bavc_mark_open_path(params, i_delta, on_path, &nh)) {
    free(on_path);
    return false;
  }

  // Step 16..17
  if (nh - 2 * tau + 1 > params->t_open) {
    free(on_path);
    return false;
  }

  // Step 3
  const uint8_t* com = vc->com;
  for (unsigned int i = 0; i < tau; ++i) {
    decom_i = mempcpy(decom_i, com + i_delta[i] * com_size, com_size);
    com += bavc_max_node_index(i, tau_1, k) * com_size;
  }

  // Step 19..25: emit opening seeds depth-by-depth.
  unsigned int level_start = 0;
  unsigned int level_count = 1;
  while (level_start < (L - 1)) {
    if (level_start + level_count > (L - 1)) {
      level_count = (L - 1) - level_start;
    }

    const unsigned int level_end = level_start + level_count;
    for (unsigned int node = level_start; node < level_end; ++node) {
      const uint8_t left = on_path[2 * node + 1];
      const uint8_t right = on_path[2 * node + 2];
      if ((left ^ right) == 1) {
        const unsigned int alpha = 2 * node + 1 + left;
        decom_i = mempcpy(decom_i, NODE(vc->k, alpha, seed_bytes), seed_bytes);
      }
    }
    level_start += level_count;
    level_count <<= 1;
  }

  memset(decom_i, 0, decom_i_end - decom_i);

  free(on_path);
  return true;
}

static bool reconstruct_keys(const params_t* params, uint8_t* keys,
                             const uint8_t* decom_i, const uint16_t* i_delta,
                             const uint8_t* iv
                             ) {
  const unsigned int L            = params->L;
  const unsigned int seed_bytes   = params->lambda_prg_bytes;
  const unsigned int prg_lambda   = params->lambda_prg;
  const unsigned int tau          = params->tau;

  uint8_t* on_path = NULL;
  if (!bavc_on_path_scratch_get((size_t)(2 * L - 1), &on_path)) {
    return false;
  }
  if (!bavc_mark_open_path(params, i_delta, on_path, NULL)) {
    return false;
  }

  const uint8_t* nodes = decom_i + params->n_leaf * tau * seed_bytes;
  const uint8_t* end   = nodes + params->t_open * seed_bytes;

  // Read opening seeds in depth-major order.
  unsigned int level_start = 0;
  unsigned int level_count = 1;
  while (level_start < (L - 1)) {
    if (level_start + level_count > (L - 1)) {
      level_count = (L - 1) - level_start;
    }

    const unsigned int level_end = level_start + level_count;
    for (unsigned int node = level_start; node < level_end; ++node) {
      const uint8_t left = on_path[2 * node + 1];
      const uint8_t right = on_path[2 * node + 2];
      if ((left ^ right) == 1) {
        if (nodes == end) {
          return false;
        }
        const unsigned int alpha = 2 * node + 1 + left;
        memcpy(keys + (size_t)alpha * seed_bytes, nodes, seed_bytes);
        nodes += seed_bytes;
      }
    }

    level_start += level_count;
    level_count <<= 1;
  }

  for (; nodes != end; ++nodes) {
    if (*nodes) {
      return false;
    }
  }

  // Expand hidden subtrees level-by-level in contiguous runs.
  level_start = 0;
  level_count = 1;
  while (level_start < (L - 1)) {
    if (level_start + level_count > (L - 1)) {
      level_count = (L - 1) - level_start;
    }

    const unsigned int level_end = level_start + level_count;
    unsigned int node = level_start;
    while (node < level_end) {
      while (node < level_end && on_path[node]) {
        ++node;
      }
      const unsigned int run_start = node;
      while (node < level_end && !on_path[node]) {
        ++node;
      }
      if (run_start == node) {
        continue;
      }

      const size_t run_len = (size_t)(node - run_start);
      prg_2_lambda_independent_batch(
          keys + (size_t)run_start * seed_bytes, seed_bytes, iv, run_start,
          keys + (size_t)(2 * run_start + 1) * seed_bytes, 2 * seed_bytes, prg_lambda, run_len);
    }

    level_start += level_count;
    level_count <<= 1;
  }

  return true;
}

static bool bavc_reconstruct_em(const params_t* params, bavc_rec_t* bavc_rec, 
                                      const uint8_t* decom_i, const uint16_t* i_delta,
                                      const uint8_t* iv) {
  // Initializing
  const unsigned int L            = params->L;
  const unsigned int seed_bytes   = params->lambda_prg_bytes;
  const unsigned int prg_lambda   = params->lambda_prg;
  const unsigned int k            = params->k;
  const unsigned int tau          = params->tau;
  const unsigned int tau_1        = params->tau1;
  const unsigned int com_size     = seed_bytes * params->n_leaf; // size of com_ij
  const unsigned int h1_bytes     = params->prg_block_bytes * params->n_leaf;
  const unsigned int ell_hat_bytes = params->ell_hat_bytes;
  const unsigned int half_leaf_cnt = 1u << (k - 1);
  const unsigned int second_layer_base = L - 1 + tau * half_leaf_cnt;
  const size_t first_layer_key_stride = (size_t)tau * seed_bytes;
  const size_t second_layer_key_stride = (size_t)tau_1 * seed_bytes;
  assert(params->lambda == 128u);
  assert(prg_lambda == 256u);
  assert(ell_hat_bytes == 170u);
  assert(bavc_rec->s_prg170 != NULL);
  const unsigned int max_instances = bavc_max_node_index(0, tau_1, k);
  uint8_t* keys = NULL;
  uint8_t* com_batch = NULL;
  size_t com_lane_stride = 0;
  SIG_TSTART(t_bavc_reconstruct_em);
  if (!bavc_reconstruct_em_scratch_get(params, max_instances, com_size, &keys, &com_batch,
                                       &com_lane_stride)) {
    SIG_TEND("bavc.reconstruct.em", t_bavc_reconstruct_em);
    return false;
  }

  // Step 7..10
  if (!reconstruct_keys(params, keys, decom_i, i_delta, iv
                        )) {
    SIG_TEND("bavc.reconstruct.em", t_bavc_reconstruct_em);
    return false;
  }

  H1_context_t h1_com_ctx;
  H1_init(&h1_com_ctx);
  H1_context_t h1_ctx = {0};
  bool h1_ctx_inited = false;
  uint8_t hi[h1_bytes];

  const uint8_t* decom_com = decom_i;

  size_t h1_batch_scratch_stride = 0;
  uint8_t* h1_batch_scratch = NULL;
  bool use_h1_batch4 = false;

  {
    const size_t max_h1_msg_len = (size_t)bavc_max_node_index(0, tau_1, k) * com_size;
    if (max_h1_msg_len < SIZE_MAX - 1u) {
      h1_batch_scratch_stride = max_h1_msg_len + 1u; /* msg || domain_sep */
      if (h1_batch_scratch_stride != 0u && h1_batch_scratch_stride <= SIZE_MAX / 4u) {
        h1_batch_scratch = malloc(4u * h1_batch_scratch_stride);
      }
    }
    use_h1_batch4 = (h1_batch_scratch != NULL);
  }

  (void)decom_com;
  (void)com_lane_stride;

  assert(tau <= 128u);
  size_t sd_base_off[128];
  size_t com_base_off[128];
  size_t msg_len_arr[128];
  size_t sd_off_prefix = 0;
  size_t com_off_prefix = 0;
  for (unsigned int i = 0; i < tau; ++i) {
    const unsigned int total_instances = bavc_max_node_index(i, tau_1, k);
    sd_base_off[i] = sd_off_prefix;
    com_base_off[i] = com_off_prefix;
    msg_len_arr[i] = (size_t)total_instances * com_size;
    sd_off_prefix += total_instances;
    com_off_prefix += total_instances;
  }

  // Phase A: expand/mapping are independent per i, so parallelize across instances.
  if (tau_1 == 0u) {
    for (int ii = 0; ii < (int)tau; ++ii) {
      const unsigned int i = (unsigned int)ii;
      const uint32_t tweak = i + L - 1;
      const uint32_t vole_tweak = i ^ VOLE_TWEAK_OFFSET;
      const unsigned int opened = i_delta[i];
      const uint8_t* key_base_first = keys + (size_t)(L - 1 + i) * seed_bytes;
      uint8_t* out_first = com_batch + com_base_off[i] * com_size;
      uint8_t* sd_i_out = bavc_rec->s + sd_base_off[i] * seed_bytes;
      uint8_t* sd_i_prg170_out = bavc_rec->s_prg170 + sd_base_off[i] * ell_hat_bytes;
      const uint8_t* decom_com_i = decom_i + (size_t)i * com_size;

      memset(sd_i_out, 0, seed_bytes);
      memset(sd_i_prg170_out, 0, ell_hat_bytes);

      prg_2_lambda_and_170_fixed_tweak_independent_batch_with_sd_xor_map(
          key_base_first, first_layer_key_stride, iv, tweak, vole_tweak, sd_i_out, seed_bytes,
          0u, opened, out_first, com_size, sd_i_prg170_out, ell_hat_bytes, prg_lambda,
          half_leaf_cnt);
      memcpy(out_first + (size_t)opened * com_size, decom_com_i, com_size);
    }
  } else {
    for (int ii = 0; ii < (int)tau; ++ii) {
      const unsigned int i = (unsigned int)ii;
      const uint32_t tweak = i + L - 1;
      const uint32_t vole_tweak = i ^ VOLE_TWEAK_OFFSET;
      const unsigned int opened = i_delta[i];
      const uint8_t* key_base_first = keys + (size_t)(L - 1 + i) * seed_bytes;
      uint8_t* com_i = com_batch + com_base_off[i] * com_size;
      uint8_t* out_first = com_i;
      uint8_t* sd_i_out = bavc_rec->s + sd_base_off[i] * seed_bytes;
      uint8_t* sd_i_prg170_out = bavc_rec->s_prg170 + sd_base_off[i] * ell_hat_bytes;
      const uint8_t* decom_com_i = decom_i + (size_t)i * com_size;

      memset(sd_i_out, 0, seed_bytes);
      memset(sd_i_prg170_out, 0, ell_hat_bytes);

      prg_2_lambda_and_170_fixed_tweak_independent_batch_with_sd_xor_map(
          key_base_first, first_layer_key_stride, iv, tweak, vole_tweak, sd_i_out, seed_bytes,
          0u, opened, out_first, com_size, sd_i_prg170_out, ell_hat_bytes, prg_lambda,
          half_leaf_cnt);
      memcpy(out_first + (size_t)opened * com_size, decom_com_i, com_size);

      if (i < tau_1) {
        const uint8_t* key_base_second = keys + (size_t)(second_layer_base + i) * seed_bytes;
        uint8_t* out_second = com_i + (size_t)half_leaf_cnt * com_size;
        const unsigned int opened_second =
            opened >= half_leaf_cnt ? (opened - half_leaf_cnt) : half_leaf_cnt;

        prg_2_lambda_and_170_fixed_tweak_independent_batch_with_sd_xor_map(
            key_base_second, second_layer_key_stride, iv, tweak, vole_tweak, sd_i_out,
            seed_bytes, half_leaf_cnt, opened, out_second, com_size, sd_i_prg170_out,
            ell_hat_bytes, prg_lambda, half_leaf_cnt);
        if (opened_second < half_leaf_cnt) {
          memcpy(out_second + (size_t)opened_second * com_size, decom_com_i, com_size);
        }
      }
    }
  }

  // Phase B: hash per-instance commitments and fold into global H1 context.
  for (unsigned int i = 0; i < tau;) {
    const unsigned int region_end = (tau_1 == 0u) ? tau : ((i < tau_1) ? tau_1 : tau);
    const size_t lane_count =
        use_h1_batch4 ? ((region_end - i >= 4u) ? 4u : (size_t)(region_end - i)) : 1u;
    const uint8_t* msg[4] = {NULL, NULL, NULL, NULL};
    size_t msg_len[4] = {0, 0, 0, 0};
    uint8_t hi4[4][h1_bytes];
    uint8_t* digest[4] = {hi4[0], hi4[1], hi4[2], hi4[3]};

    for (size_t lane = 0; lane < lane_count; ++lane) {
      const unsigned int idx = i + (unsigned int)lane;
      msg[lane] = com_batch + com_base_off[idx] * com_size;
      msg_len[lane] = msg_len_arr[idx];
    }

    if (use_h1_batch4) {
      const int h1_rc = h1_finalize_batch4_same_len_dispatch(
          h1_bytes, msg, msg_len[0], digest, lane_count, h1_batch_scratch,
          h1_batch_scratch_stride);
      assert(h1_rc == 0);
      (void)h1_rc;
      H1_update(&h1_com_ctx, hi4[0], lane_count * h1_bytes);
    } else {
      if (!h1_ctx_inited) {
        H1_init(&h1_ctx);
        h1_ctx_inited = true;
      }
      for (size_t lane = 0; lane < lane_count; ++lane) {
        H1_reset(&h1_ctx);
        H1_update(&h1_ctx, msg[lane], msg_len[lane]);
        H1_final_keep(params, &h1_ctx, hi);
        H1_update(&h1_com_ctx, hi, h1_bytes);
      }
    }

    i += (unsigned int)lane_count;
  }

  if (h1_ctx_inited) {
    H1_clear(&h1_ctx);
  }
  if (h1_batch_scratch) {
    free(h1_batch_scratch);
  }
  H1_final(params, &h1_com_ctx, bavc_rec->h);

  SIG_TEND("bavc.reconstruct.em", t_bavc_reconstruct_em);
  return true;
}
bool bavc_reconstruct(const params_t* params, bavc_rec_t* bavc_rec, const uint8_t* decom_i,
                      const uint16_t* i_delta, const uint8_t* iv) {
  return bavc_reconstruct_em(params, bavc_rec, decom_i, i_delta, iv);
}

void bavc_clear(bavc_t* com) {
  free(com->sd_prg170);
  free(com->sd);
  free(com->com);
  free(com->h);
  free(com->k);
}
