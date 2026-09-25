#include "bavc.h"
#include "random_oracle.h"
#include "compat.h"
#include "prg.h"
#include "params.h"
#include "auxfunc.h"
#ifdef SM3_ACCE
#include "auxfunc_acce.h"
#endif

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#ifdef SIG_TIMING
#include <stdint.h>
#include <stdio.h>
#include <time.h>

static uint64_t bavc_now_ns(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * UINT64_C(1000000000) + (uint64_t)ts.tv_nsec;
}

static void bavc_print_timing(const char* label, uint64_t dt_ns) {
  printf("[bavc] %s: %.3f ms\n", label, (double)dt_ns / 1000000.0);
}
#endif

#ifdef SM3_ACCE
static uint8_t* g_h1_batch_scratch = NULL;
static size_t g_h1_batch_scratch_cap = 0;

static uint8_t* bavc_h1_batch_scratch_reserve(size_t need) {
  if (need == 0) {
    return NULL;
  }
  if (g_h1_batch_scratch_cap >= need) {
    return g_h1_batch_scratch;
  }
  uint8_t* p = realloc(g_h1_batch_scratch, need);
  assert(p);
  g_h1_batch_scratch = p;
  g_h1_batch_scratch_cap = need;
  return p;
}
#endif

#define NODE(nodes, node, lambda_bytes) (&nodes[(node) * (lambda_bytes)])

static void em_leaf_commit_columns_from_nodes(const params_t* params, const uint8_t* iv,
                                              const uint8_t* leaf_nodes, uint8_t* sd_out,
                                              uint8_t* com_out) {
  const unsigned int tau = params->tau;
  const unsigned int tau1 = params->tau1;
  const unsigned int k = params->k;
  const unsigned int L = params->L;
  const unsigned int lambda = params->lambda;
  const unsigned int lambda_bytes = params->lambda_bytes;
  const unsigned int com_size = lambda_bytes * params->n_leaf;
  const unsigned int half_leaf_cnt = 1u << (k - 1);
  const size_t tau_stride = (size_t)tau * lambda_bytes;
  const size_t tau1_stride = (size_t)tau1 * lambda_bytes;
  const uint8_t* second_leaf_nodes = leaf_nodes + (size_t)tau * half_leaf_cnt * lambda_bytes;
  for (unsigned int i = 0; i < tau; ++i) {
    const uint32_t tweak = i + L - 1;
    const size_t dst_idx = (i < tau1) ? ((size_t)(2u * i) * half_leaf_cnt)
                                      : ((size_t)(i + tau1) * half_leaf_cnt);
    const uint8_t* key_col = leaf_nodes + (size_t)i * lambda_bytes;
    prg_2_lambda_fixed_tweak_independent_batch_with_sd(
        key_col, tau_stride, iv, tweak, sd_out + dst_idx * lambda_bytes, lambda_bytes,
        com_out + dst_idx * com_size, com_size, lambda, half_leaf_cnt);
  }

  for (unsigned int i = 0; i < tau1; ++i) {
    const uint32_t tweak = i + L - 1;
    const size_t dst_idx = ((size_t)(2u * i + 1u)) * half_leaf_cnt;
    const uint8_t* key_col = second_leaf_nodes + (size_t)i * lambda_bytes;
    prg_2_lambda_fixed_tweak_independent_batch_with_sd(
        key_col, tau1_stride, iv, tweak, sd_out + dst_idx * lambda_bytes, lambda_bytes,
        com_out + dst_idx * com_size, com_size, lambda, half_leaf_cnt);
  }
}

static void seed_prg_em_pipeline_commit(const params_t* params, uint8_t* nodes, const uint8_t* iv,
                                        uint8_t* sd_out, uint8_t* com_out
#ifdef SIG_TIMING
                                        ,
                                        uint64_t* seed_prg_ns, uint64_t* leaf_commit_ns
#endif
                                        ) {
  const unsigned int L = params->L;
  const unsigned int lambda_bytes = params->lambda_bytes;
  const size_t out_stride = 2u * (size_t)lambda_bytes;

  if (L > 1) {
    unsigned int level_start = 0;
    unsigned int level_count = 1;
    while (level_start < (L - 1)) {
      if (level_start + level_count > (L - 1)) {
        level_count = (L - 1) - level_start;
      }
#ifdef SIG_TIMING
      const uint64_t t = bavc_now_ns();
#endif
      prg_2_lambda_independent_batch(
          NODE(nodes, level_start, lambda_bytes), lambda_bytes, iv, level_start,
          NODE(nodes, 2 * level_start + 1, lambda_bytes), out_stride, params->lambda,
          level_count);
#ifdef SIG_TIMING
      if (seed_prg_ns) {
        *seed_prg_ns += bavc_now_ns() - t;
      }
#endif
      level_start += level_count;
      level_count <<= 1;
    }
  }

#ifdef SIG_TIMING
  const uint64_t t_leaf = bavc_now_ns();
#endif
  em_leaf_commit_columns_from_nodes(params, iv, NODE(nodes, L - 1, lambda_bytes), sd_out, com_out);
#ifdef SIG_TIMING
  if (leaf_commit_ns) {
    *leaf_commit_ns += bavc_now_ns() - t_leaf;
  }
#endif
}

// static uint8_t* generate_seeds(const params_t* params, const uint8_t* root_seed,
//                                const uint8_t* iv, bool em_optimized) {
//   unsigned int lambda_bytes = params->lambda_bytes;
//   uint8_t* nodes            = calloc(2 * params->L - 1, lambda_bytes);
//   assert(nodes);

//   memcpy(NODE(nodes, 0, lambda_bytes), root_seed, lambda_bytes);
//   if (em_optimized) {
//   } else {
//   }

//   return nodes;
// }

// LeafCommit
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
  if (!bavc_mul_size(node_count, params->lambda_bytes, &keys_need) ||
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
  const unsigned int lambda_bytes = params->lambda_bytes;
  const unsigned int com_size     = lambda_bytes * params->n_leaf; // size of com_ij

#ifdef SIG_TIMING
  const uint64_t t_total = bavc_now_ns();
  uint64_t h1_update_ns = 0;
  uint64_t h1_nonupdate_ns = 0;
  uint64_t em_leaf_ns = 0;
  uint64_t prep_ns = 0;
#endif

#ifdef SIG_TIMING
  uint64_t t_stage = bavc_now_ns();
#endif
  H1_context_t h1_com_ctx;
  H1_init(&h1_com_ctx);
#ifdef SIG_TIMING
  h1_nonupdate_ns += bavc_now_ns() - t_stage;
#endif

  // Initialzing stuff
#ifdef SIG_TIMING
  uint64_t init_ns = bavc_now_ns();
#endif
  uint8_t* nodes = calloc(2 * L - 1, lambda_bytes);
  assert(nodes);
  bavc->h   = malloc(lambda_bytes * 2);
  bavc->com = malloc(L * com_size);
  bavc->sd  = malloc(L * lambda_bytes);
  assert(bavc->h && bavc->com && bavc->sd);
#ifdef SIG_TIMING
  init_ns = bavc_now_ns() - init_ns;
#endif

  // Generating the tree and leaf commitments in one pipeline.
#ifdef SIG_TIMING
  uint64_t dt_seed = 0;
  uint64_t seed_prg_ns = 0;
  const uint64_t t_seed_alloc = bavc_now_ns();
#endif
  memcpy(NODE(nodes, 0, lambda_bytes), rootKey, lambda_bytes);
  bavc->k = NODE(nodes, 0, lambda_bytes);
  assert(bavc->k);
  // do seed_prg and leaf commit together to preserve parallelism and save some memory.
  seed_prg_em_pipeline_commit(params, nodes, iv, bavc->sd, bavc->com
#ifdef SIG_TIMING
                              ,
                              &seed_prg_ns, &em_leaf_ns
#endif
                              );
#ifdef SIG_TIMING
  {
    const uint64_t dt_seed_total = bavc_now_ns() - t_seed_alloc;
    if (dt_seed_total >= em_leaf_ns) {
      dt_seed = dt_seed_total - em_leaf_ns;
    } else {
      dt_seed = seed_prg_ns;
    }
  }
  if (dt_seed < seed_prg_ns) {
    dt_seed = seed_prg_ns;
  }
#endif

  // Step: 1..3
#ifdef SIG_TIMING
  t_stage = bavc_now_ns();
#endif
  // Step: 4..5 + Step 10..12 over per-instance commitments.
#ifdef SIG_TIMING
  prep_ns += bavc_now_ns() - t_stage;
#endif
  size_t com_off = 0;
  H1_context_t h1_ctx = {0};
  bool h1_ctx_inited = false;
#ifdef SM3_ACCE
  size_t h1_batch_scratch_stride = 0;
  uint8_t* h1_batch_scratch = NULL;
  bool use_h1_batch4 = false;

  if (2u * lambda_bytes == 32u) {
    // SM3(256-bit) fastpath: hash (msg || domain_sep) directly without scratch copy.
    // This keeps per-lane prep tiny, so batch mode mainly pays the hash work
    // itself instead of extra repack overhead.
    use_h1_batch4 = true;
  } else {
    const size_t max_h1_msg_len = (size_t)bavc_max_node_index(0, tau1, k) * com_size;

    /*
     * H1 batch helper may need up to:
     *   64 (K1) + 2 (0x0200) + msg_len + 1 (domain sep)
     * bytes per lane for lambda=256 fastpath.
     */
    if (max_h1_msg_len < SIZE_MAX - 67u) {
      h1_batch_scratch_stride = max_h1_msg_len + 67u;
      if (h1_batch_scratch_stride != 0 && h1_batch_scratch_stride <= SIZE_MAX / 4) {
        h1_batch_scratch = bavc_h1_batch_scratch_reserve(4u * h1_batch_scratch_stride);
      }
    }
    use_h1_batch4 = (h1_batch_scratch != NULL);
  }

  if (use_h1_batch4) {
    uint8_t hi4[4][2 * lambda_bytes];
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

#ifdef SIG_TIMING
        t_stage = bavc_now_ns();
#endif
        const int h1_rc = h1_finalize_batch4_same_len_acce(
            lambda_bytes, msg, msg_len_region, digest, lane_count, h1_batch_scratch,
            h1_batch_scratch_stride);
        assert(h1_rc == 0);
        (void)h1_rc;
#ifdef SIG_TIMING
        h1_nonupdate_ns += bavc_now_ns() - t_stage;
#endif

#ifdef SIG_TIMING
        t_stage = bavc_now_ns();
#endif
        H1_update(&h1_com_ctx, hi4[0], lane_count * 2u * lambda_bytes);
#ifdef SIG_TIMING
        h1_update_ns += bavc_now_ns() - t_stage;
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

#ifdef SIG_TIMING
          t_stage = bavc_now_ns();
#endif
          const int h1_rc = h1_finalize_batch4_same_len_acce(
              lambda_bytes, msg, msg_len_region, digest, lane_count, h1_batch_scratch,
              h1_batch_scratch_stride);
          assert(h1_rc == 0);
          (void)h1_rc;
#ifdef SIG_TIMING
          h1_nonupdate_ns += bavc_now_ns() - t_stage;
#endif

#ifdef SIG_TIMING
          t_stage = bavc_now_ns();
#endif
          H1_update(&h1_com_ctx, hi4[0], lane_count * 2u * lambda_bytes);
#ifdef SIG_TIMING
          h1_update_ns += bavc_now_ns() - t_stage;
#endif
          i += (unsigned int)lane_count;
        }
      }
    }

    (void)h1_batch_scratch;
  } else
#endif
  {
    uint8_t hi[2 * lambda_bytes];
    H1_init(&h1_ctx);
    h1_ctx_inited = true;
    for (unsigned int i = 0; i < tau; ++i) {
      const unsigned int N_i = bavc_max_node_index(i, tau1, k);
      const uint8_t* com_i_start = bavc->com + com_off * com_size;
#ifdef SIG_TIMING
      t_stage = bavc_now_ns();
#endif
      H1_reset(&h1_ctx);
#ifdef SIG_TIMING
      h1_nonupdate_ns += bavc_now_ns() - t_stage;
#endif

#ifdef SIG_TIMING
      t_stage = bavc_now_ns();
#endif
      H1_update(&h1_ctx, com_i_start, (size_t)N_i * com_size);
#ifdef SIG_TIMING
      h1_update_ns += bavc_now_ns() - t_stage;
#endif

      // Step 11
#ifdef SIG_TIMING
      t_stage = bavc_now_ns();
#endif
      H1_final_keep(params, &h1_ctx, hi);
#ifdef SIG_TIMING
      h1_nonupdate_ns += bavc_now_ns() - t_stage;
      t_stage = bavc_now_ns();
#endif
      // Step 12
      H1_update(&h1_com_ctx, hi, lambda_bytes * 2);
#ifdef SIG_TIMING
      h1_update_ns += bavc_now_ns() - t_stage;
#endif
      com_off += N_i;
    }
  }

  // Step 12
#ifdef SIG_TIMING
  t_stage = bavc_now_ns();
#endif
  if (h1_ctx_inited) {
    H1_clear(&h1_ctx);
  }
#ifdef SIG_TIMING
  h1_nonupdate_ns += bavc_now_ns() - t_stage;
  t_stage = bavc_now_ns();
#endif
  H1_final(params, &h1_com_ctx, bavc->h);
#ifdef SIG_TIMING
  h1_nonupdate_ns += bavc_now_ns() - t_stage;

  uint64_t actual_total = bavc_now_ns() - t_total;
  uint64_t accounted_ns = dt_seed + init_ns + prep_ns + em_leaf_ns +
                          h1_update_ns + h1_nonupdate_ns;
  uint64_t other_ns = actual_total - accounted_ns;

  bavc_print_timing("detail.sign.bavc_commit_em.seed.total", dt_seed);
  bavc_print_timing("detail.sign.bavc_commit_em.prep (set_k)", prep_ns);
  bavc_print_timing("detail.sign.bavc_commit_em.leaf.em_leaf_commit", em_leaf_ns);
  bavc_print_timing("detail.sign.bavc_commit_em.h1_nonupdate", h1_nonupdate_ns);
  bavc_print_timing("detail.sign.bavc_commit_em.h1_update", h1_update_ns);
  bavc_print_timing("detail.sign.bavc_commit_em.other (init_alloc/unaccounted)", init_ns + other_ns);
  bavc_print_timing("detail.sign.bavc_commit_em.total", actual_total);
#endif
}

void bavc_commit(const params_t* params, bavc_t* bavc, const uint8_t* root_key,
                 const uint8_t* iv) {
  bavc_commit_em(params, bavc, root_key, iv);
}

bool bavc_open(const params_t* params, uint8_t* decom_i, const bavc_t* vc,
               const uint16_t* i_delta) {
  const unsigned int lambda       = params->lambda;
  const unsigned int L            = params->L;
  const unsigned int lambda_bytes = lambda / 8;
  const unsigned int k            = params->k;
  const unsigned int tau          = params->tau;
  const unsigned int tau_1        = params->tau1;
  const unsigned int com_size     = params->n_leaf * lambda_bytes;

  uint8_t* decom_i_end = decom_i + com_size * tau + params->t_open * lambda_bytes;

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
        decom_i = mempcpy(decom_i, NODE(vc->k, alpha, lambda_bytes), lambda_bytes);
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
#ifdef SIG_TIMING
                             , uint64_t* prg_ns
#endif
                             ) {
  const unsigned int lambda       = params->lambda;
  const unsigned int L            = params->L;
  const unsigned int lambda_bytes = lambda / 8;
  const unsigned int tau          = params->tau;

  uint8_t* on_path = NULL;
  if (!bavc_on_path_scratch_get((size_t)(2 * L - 1), &on_path)) {
    return false;
  }
  if (!bavc_mark_open_path(params, i_delta, on_path, NULL)) {
    return false;
  }

  const uint8_t* nodes = decom_i + params->n_leaf * tau * lambda_bytes;
  const uint8_t* end   = nodes + params->t_open * lambda_bytes;

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
        memcpy(keys + (size_t)alpha * lambda_bytes, nodes, lambda_bytes);
        nodes += lambda_bytes;
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
#ifdef SIG_TIMING
      uint64_t t = bavc_now_ns();
#endif
      prg_2_lambda_independent_batch(
          keys + (size_t)run_start * lambda_bytes, lambda_bytes, iv, run_start,
          keys + (size_t)(2 * run_start + 1) * lambda_bytes, 2 * lambda_bytes, lambda, run_len);
#ifdef SIG_TIMING
      if (prg_ns) {
        *prg_ns += bavc_now_ns() - t;
      }
#endif
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
  const unsigned int lambda_bytes = params->lambda_bytes;
  const unsigned int k            = params->k;
  const unsigned int tau          = params->tau;
  const unsigned int tau_1        = params->tau1;
  const unsigned int com_size     = lambda_bytes * params->n_leaf; // size of com_ij
  const unsigned int half_leaf_cnt = 1u << (k - 1);
  const unsigned int second_layer_base = L - 1 + tau * half_leaf_cnt;
  const size_t first_layer_key_stride = (size_t)tau * lambda_bytes;
  const size_t second_layer_key_stride = (size_t)tau_1 * lambda_bytes;
  const unsigned int max_instances = bavc_max_node_index(0, tau_1, k);
  uint8_t* keys = NULL;
  uint8_t* com_batch = NULL;
  size_t com_lane_stride = 0;
  if (!bavc_reconstruct_em_scratch_get(params, max_instances, com_size, &keys, &com_batch,
                                       &com_lane_stride)) {
    return false;
  }

  // Step 7..10
  if (!reconstruct_keys(params, keys, decom_i, i_delta, iv
#ifdef SIG_TIMING
                        , NULL
#endif
                        )) {
    return false;
  }

  H1_context_t h1_com_ctx;
  H1_init(&h1_com_ctx);
  H1_context_t h1_ctx = {0};
  bool h1_ctx_inited = false;
  uint8_t hi[2 * lambda_bytes];

  const uint8_t* decom_com = decom_i;

#ifdef SM3_ACCE
  size_t h1_batch_scratch_stride = 0;
  uint8_t* h1_batch_scratch = NULL;
  bool use_h1_batch4 = false;

  if (2u * lambda_bytes == 32u) {
    use_h1_batch4 = true;
  } else {
    const size_t max_h1_msg_len = (size_t)bavc_max_node_index(0, tau_1, k) * com_size;
    if (max_h1_msg_len < SIZE_MAX - 67u) {
      h1_batch_scratch_stride = max_h1_msg_len + 67u;
      if (h1_batch_scratch_stride != 0 && h1_batch_scratch_stride <= SIZE_MAX / 4u) {
        h1_batch_scratch = bavc_h1_batch_scratch_reserve(4u * h1_batch_scratch_stride);
      }
    }
    use_h1_batch4 = (h1_batch_scratch != NULL);
  }
#else
  const bool use_h1_batch4 = false;
#endif

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
      const unsigned int opened = i_delta[i];
      const uint8_t* key_base_first = keys + (size_t)(L - 1 + i) * lambda_bytes;
      uint8_t* out_first = com_batch + com_base_off[i] * com_size;
      uint8_t* sd_i_out = bavc_rec->s + sd_base_off[i] * lambda_bytes;
      const uint8_t* decom_com_i = decom_i + (size_t)i * com_size;

      memset(sd_i_out, 0, lambda_bytes);
      prg_2_lambda_fixed_tweak_independent_batch_with_sd_xor_map(
          key_base_first, first_layer_key_stride, iv, tweak, sd_i_out, lambda_bytes, 0u, opened,
          out_first, com_size, params->lambda, half_leaf_cnt);
      memcpy(out_first + (size_t)opened * com_size, decom_com_i, com_size);
    }
  } else {
    for (int ii = 0; ii < (int)tau; ++ii) {
      const unsigned int i = (unsigned int)ii;
      const uint32_t tweak = i + L - 1;
      const unsigned int opened = i_delta[i];
      const uint8_t* key_base_first = keys + (size_t)(L - 1 + i) * lambda_bytes;
      uint8_t* com_i = com_batch + com_base_off[i] * com_size;
      uint8_t* out_first = com_i;
      uint8_t* sd_i_out = bavc_rec->s + sd_base_off[i] * lambda_bytes;
      const uint8_t* decom_com_i = decom_i + (size_t)i * com_size;

      memset(sd_i_out, 0, lambda_bytes);
      prg_2_lambda_fixed_tweak_independent_batch_with_sd_xor_map(
          key_base_first, first_layer_key_stride, iv, tweak, sd_i_out, lambda_bytes, 0u, opened,
          out_first, com_size, params->lambda, half_leaf_cnt);
      memcpy(out_first + (size_t)opened * com_size, decom_com_i, com_size);

      if (i < tau_1) {
        const uint8_t* key_base_second = keys + (size_t)(second_layer_base + i) * lambda_bytes;
        uint8_t* out_second = com_i + (size_t)half_leaf_cnt * com_size;
        const unsigned int opened_second =
            opened >= half_leaf_cnt ? (opened - half_leaf_cnt) : half_leaf_cnt;

        prg_2_lambda_fixed_tweak_independent_batch_with_sd_xor_map(
            key_base_second, second_layer_key_stride, iv, tweak, sd_i_out, lambda_bytes,
            half_leaf_cnt, opened, out_second, com_size, params->lambda, half_leaf_cnt);
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
#ifdef SM3_ACCE
    uint8_t hi4[4][2 * lambda_bytes];
    uint8_t* digest[4] = {hi4[0], hi4[1], hi4[2], hi4[3]};
#endif

    for (size_t lane = 0; lane < lane_count; ++lane) {
      const unsigned int idx = i + (unsigned int)lane;
      msg[lane] = com_batch + com_base_off[idx] * com_size;
      msg_len[lane] = msg_len_arr[idx];
    }

#ifdef SM3_ACCE
    if (use_h1_batch4) {
      const int h1_rc = h1_finalize_batch4_same_len_acce(
          lambda_bytes, msg, msg_len[0], digest, lane_count, h1_batch_scratch,
          h1_batch_scratch_stride);
      assert(h1_rc == 0);
      (void)h1_rc;
      H1_update(&h1_com_ctx, hi4[0], lane_count * 2u * lambda_bytes);
    } else
#endif
    {
      if (!h1_ctx_inited) {
        H1_init(&h1_ctx);
        h1_ctx_inited = true;
      }
      for (size_t lane = 0; lane < lane_count; ++lane) {
        H1_reset(&h1_ctx);
        H1_update(&h1_ctx, msg[lane], msg_len[lane]);
        H1_final_keep(params, &h1_ctx, hi);
        H1_update(&h1_com_ctx, hi, lambda_bytes * 2);
      }
    }

    i += (unsigned int)lane_count;
  }

  if (h1_ctx_inited) {
    H1_clear(&h1_ctx);
  }
#ifdef SM3_ACCE
  (void)h1_batch_scratch;
#endif
  H1_final(params, &h1_com_ctx, bavc_rec->h);

  return true;
}
bool bavc_reconstruct(const params_t* params, bavc_rec_t* bavc_rec, const uint8_t* decom_i,
                      const uint16_t* i_delta, const uint8_t* iv) {
  return bavc_reconstruct_em(params, bavc_rec, decom_i, i_delta, iv);
}

void bavc_clear(bavc_t* com) {
  free(com->sd);
  free(com->com);
  free(com->h);
  free(com->k);
}
