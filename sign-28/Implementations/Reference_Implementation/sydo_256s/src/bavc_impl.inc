/*
 *  SPDX-License-Identifier: MIT
 */

#if defined(HAVE_CONFIG_H)
#include <config.h>
#endif

#include "bavc.h"

#include "compat.h"
#include "internal.h"
#include "primitives.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

typedef struct sydo_ref_bavc_dims_t {
  size_t lambda_bytes;
  size_t delta_bits;
  size_t min_k;
  size_t max_k;
  size_t num_max_k;
  size_t num_min_k;
  size_t short_len;
  size_t long_len;
  size_t commit_leaves;
  size_t commit_nodes;
  size_t hash_len;
  size_t opening_size;
} sydo_ref_bavc_dims_t;

static bool trace_enabled(void) {
  return getenv("SYDO_REF_TRACE") != NULL;
}

static clock_t trace_clock(void) {
  return clock();
}

static void trace_stage(const sydo_ref_paramset_t* params, const char* stage, clock_t start) {
  if (trace_enabled()) {
    const double ms = 1000.0 * (double)(clock() - start) / (double)CLOCKS_PER_SEC;
    fprintf(stderr, "%s bavc %s %.2f ms\n", params->name, stage, ms);
  }
}

static sydo_ref_bavc_dims_t bavc_dims(const sydo_ref_paramset_t* params) {
  sydo_ref_bavc_dims_t d;
  d.lambda_bytes = sydo_ref_secpar_bytes(params);
  d.delta_bits = sydo_ref_delta_bits(params);
  d.min_k = d.delta_bits / params->tau;
  d.max_k = sydo_ref_ceil_div_size(d.delta_bits, params->tau);
  d.num_max_k = d.delta_bits % params->tau;
  d.num_min_k = params->tau - d.num_max_k;
  d.short_len = (size_t)1u << d.min_k;
  d.long_len = (size_t)1u << d.max_k;
  d.commit_leaves = d.num_min_k * d.short_len + d.num_max_k * d.long_len;
  d.commit_nodes = 2u * d.commit_leaves - 1u;
  d.hash_len = 2u * d.lambda_bytes;
  d.opening_size = sydo_ref_bavc_open_bytes(params);
  return d;
}

static bool get_expanded_delta_bit(const uint8_t* delta, size_t index) {
  return delta[index] != 0;
}

static size_t vole_permute_key_index_inv(size_t i, size_t width_shift, size_t max_k) {
  const size_t width = (size_t)1u << width_shift;
  size_t j = i;
  for (unsigned int shift = 1; shift < max_k - width_shift; shift <<= 1) {
    j ^= j >> shift;
  }
  return (j & ~(width - 1u)) + (i % width);
}

static size_t vole_permute_inv_increment(size_t i, size_t offset, size_t width_shift,
                                         size_t max_k) {
  const size_t width = (size_t)1u << width_shift;
  if (max_k < 16u) {
    const size_t diff_in = i ^ (i + offset);
    const size_t diff_out_even = diff_in & (0x5555u | (width - 1u));
    const size_t diff_out_odd = diff_in & (0xaaaau | (width - 1u));
    return diff_out_odd > diff_out_even ? diff_out_odd : diff_out_even;
  }
  return vole_permute_key_index_inv(i, width_shift, max_k) ^
         vole_permute_key_index_inv(i + offset, width_shift, max_k);
}

static size_t vole_width_shift(const sydo_ref_paramset_t* params) {
  (void)params;
  return 3u;
}

static size_t batch_vec_len(const sydo_ref_bavc_dims_t* dims, size_t vec_index) {
  return vec_index < dims->num_max_k ? dims->long_len : dims->short_len;
}

static size_t batch_vec_pos_in_tree(const sydo_ref_paramset_t* params,
                                    const sydo_ref_bavc_dims_t* dims, size_t vec_index,
                                    size_t leaf_index) {
  const size_t start_of_leaves = dims->commit_leaves - 1u;
  (void)params;
  if (leaf_index < dims->short_len) {
    return start_of_leaves + params->tau * leaf_index + vec_index;
  }
  return start_of_leaves + dims->num_min_k * dims->short_len +
         dims->num_max_k * leaf_index + vec_index;
}

static size_t batch_vec_hash_pos_in_output(const sydo_ref_bavc_dims_t* dims, size_t vec_index,
                                           size_t leaf_index) {
  if (vec_index < dims->num_max_k) {
    return vec_index * dims->long_len + leaf_index;
  }
  return dims->num_max_k * dims->long_len +
         (vec_index - dims->num_max_k) * dims->short_len + leaf_index;
}

static size_t batch_vec_leaf_pos_in_output(const sydo_ref_bavc_dims_t* dims, size_t vec_index,
                                           size_t local_leaf_index) {
  if (vec_index < dims->num_max_k) {
    return vec_index * dims->long_len + local_leaf_index;
  }
  return dims->num_max_k * dims->long_len + (vec_index - dims->num_max_k) * dims->short_len +
         local_leaf_index;
}

static uint8_t* tree_node(uint8_t* tree, size_t node, size_t lambda_bytes) {
  return tree + node * lambda_bytes;
}

static const uint8_t* const_tree_node(const uint8_t* tree, size_t node, size_t lambda_bytes) {
  return tree + node * lambda_bytes;
}

static bool expand_one_node(const sydo_ref_paramset_t* params, uint8_t* tree, const uint8_t* iv,
                            size_t node) {
  uint8_t out[128];
  const size_t lambda_bytes = sydo_ref_secpar_bytes(params);
  if (!sydo_ref_tree_prg_expand_2(params, out, const_tree_node(tree, node, lambda_bytes), iv,
                                  (uint32_t)node)) {
    return false;
  }
  memcpy(tree_node(tree, 2u * node + 1u, lambda_bytes), out, lambda_bytes);
  memcpy(tree_node(tree, 2u * node + 2u, lambda_bytes), out + lambda_bytes, lambda_bytes);
  sydo_explicit_bzero(out, sizeof(out));
  return true;
}

static bool expand_tree_all(const sydo_ref_paramset_t* params, uint8_t* tree, const uint8_t* iv,
                            size_t commit_leaves) {
  for (size_t node = 0; node != commit_leaves - 1u; ++node) {
    if (!expand_one_node(params, tree, iv, node)) {
      return false;
    }
  }
  return true;
}

static bool write_leaf(const sydo_ref_paramset_t* params, const sydo_ref_bavc_dims_t* dims,
                       const uint8_t* iv, size_t vec_index, const uint8_t* key,
                       uint8_t* leaf_out, uint8_t* hash_out) {
  return sydo_ref_leaf_hash_prg(params, leaf_out, hash_out, key, iv,
                                (uint32_t)(vec_index + dims->commit_leaves - 1u));
}

void sydo_ref_bavc_init(sydo_ref_bavc_t* bavc) {
  if (bavc) {
    memset(bavc, 0, sizeof(*bavc));
  }
}

void sydo_ref_bavc_clear(sydo_ref_bavc_t* bavc) {
  if (!bavc) {
    return;
  }
  if (bavc->tree) {
    sydo_explicit_bzero(bavc->tree, bavc->commit_nodes * bavc->hash_len / 2u);
    free(bavc->tree);
  }
  if (bavc->leaves) {
    sydo_explicit_bzero(bavc->leaves, bavc->commit_leaves * bavc->hash_len / 2u);
    free(bavc->leaves);
  }
  if (bavc->hashed_leaves) {
    sydo_explicit_bzero(bavc->hashed_leaves, bavc->commit_leaves * bavc->hash_len);
    free(bavc->hashed_leaves);
  }
  sydo_ref_bavc_init(bavc);
}

bool sydo_ref_bavc_commit(const sydo_ref_paramset_t* params, sydo_ref_bavc_t* bavc,
                          const uint8_t* seed, const uint8_t* iv) {
  const sydo_ref_bavc_dims_t dims = bavc_dims(params);
  const size_t lambda_bytes = dims.lambda_bytes;
  const size_t width_shift = vole_width_shift(params);

  sydo_ref_bavc_clear(bavc);
  bavc->commit_leaves = dims.commit_leaves;
  bavc->commit_nodes = dims.commit_nodes;
  bavc->hash_len = dims.hash_len;
  bavc->tree = (uint8_t*)calloc(dims.commit_nodes, lambda_bytes);
  bavc->leaves = (uint8_t*)calloc(dims.commit_leaves, lambda_bytes);
  bavc->hashed_leaves = (uint8_t*)calloc(dims.commit_leaves * dims.hash_len + 1u, 1u);
  if (!bavc->tree || !bavc->leaves || !bavc->hashed_leaves) {
    sydo_ref_bavc_clear(bavc);
    return false;
  }

  memcpy(tree_node(bavc->tree, 0, lambda_bytes), seed, lambda_bytes);
  clock_t stage_start = trace_clock();
  if (!expand_tree_all(params, bavc->tree, iv, dims.commit_leaves)) {
    sydo_ref_bavc_clear(bavc);
    return false;
  }
  trace_stage(params, "commit expand_tree", stage_start);

  stage_start = trace_clock();
  for (size_t vec_index = 0; vec_index != params->tau; ++vec_index) {
    size_t permuted_leaf_idx =
        vole_permute_key_index_inv(0, width_shift, dims.max_k);
    const size_t len = batch_vec_len(&dims, vec_index);
    for (size_t leaf_index = 0; leaf_index != len; ++leaf_index) {
      const size_t tree_pos = batch_vec_pos_in_tree(params, &dims, vec_index, leaf_index);
      const size_t hash_pos = batch_vec_hash_pos_in_output(&dims, vec_index, leaf_index);
      if (!write_leaf(params, &dims, iv, vec_index,
                      const_tree_node(bavc->tree, tree_pos, lambda_bytes),
                      bavc->leaves +
                          batch_vec_leaf_pos_in_output(&dims, vec_index, permuted_leaf_idx) *
                              lambda_bytes,
                      bavc->hashed_leaves + hash_pos * dims.hash_len)) {
        sydo_ref_bavc_clear(bavc);
        return false;
      }
      permuted_leaf_idx ^=
          vole_permute_inv_increment(leaf_index, 1, width_shift, dims.max_k);
    }
  }
  trace_stage(params, "commit leaf_hash", stage_start);
  return true;
}

bool sydo_ref_bavc_open(const sydo_ref_paramset_t* params, const sydo_ref_bavc_t* bavc,
                        const uint8_t* delta_bytes, uint8_t* opening) {
  const sydo_ref_bavc_dims_t dims = bavc_dims(params);
  uint8_t* dont_reveal = NULL;
  size_t* delta_parsed = NULL;
  size_t opening_bytes_written = 0;
  size_t number_of_hidden_nodes = 0;
  size_t delta_bit = 0;
  bool ok = false;

  if (!params || !bavc || !opening || !delta_bytes || !bavc->tree || !bavc->hashed_leaves) {
    return false;
  }

  dont_reveal = (uint8_t*)calloc(dims.commit_nodes, 1);
  delta_parsed = (size_t*)calloc(params->tau, sizeof(*delta_parsed));
  if (!dont_reveal || !delta_parsed) {
    goto cleanup;
  }

  for (size_t vec_index = 0; vec_index != params->tau; ++vec_index) {
    const size_t depth = vec_index < dims.num_max_k ? dims.max_k : dims.min_k;
    size_t delta_i = 0;
    for (size_t d = 0; d != depth; ++d) {
      if (get_expanded_delta_bit(delta_bytes, delta_bit + d)) {
        delta_i |= (size_t)1u << d;
      }
    }
    delta_bit += depth;
    delta_parsed[vec_index] = delta_i;

    size_t pos = batch_vec_pos_in_tree(params, &dims, vec_index, delta_parsed[vec_index]);
    while (true) {
      if (!dont_reveal[pos]) {
        dont_reveal[pos] = 1;
        ++number_of_hidden_nodes;
      }
      if (pos == 0) {
        break;
      }
      pos = (pos - 1u) / 2u;
    }
  }

  if (number_of_hidden_nodes - 2u * params->tau + 1u >
      params->bavc_opening_threshold) {
    goto cleanup;
  }

  for (size_t vec_index = 0; vec_index != params->tau; ++vec_index) {
    const size_t hash_pos =
        batch_vec_hash_pos_in_output(&dims, vec_index, delta_parsed[vec_index]);
    memcpy(opening + opening_bytes_written, bavc->hashed_leaves + hash_pos * dims.hash_len,
           dims.hash_len);
    opening_bytes_written += dims.hash_len;
  }

  for (size_t i = dims.commit_leaves - 1u; i != 0; --i) {
    const size_t node = i - 1u;
    const bool left = dont_reveal[2u * node + 1u] != 0;
    const bool right = dont_reveal[2u * node + 2u] != 0;
    dont_reveal[node] = (uint8_t)(left || right);
    if (left != right) {
      const size_t reveal = left ? (2u * node + 2u) : (2u * node + 1u);
      memcpy(opening + opening_bytes_written,
             const_tree_node(bavc->tree, reveal, dims.lambda_bytes), dims.lambda_bytes);
      opening_bytes_written += dims.lambda_bytes;
    }
  }

  memset(opening + opening_bytes_written, 0, dims.opening_size - opening_bytes_written);
  ok = true;

cleanup:
  if (dont_reveal) {
    sydo_explicit_bzero(dont_reveal, dims.commit_nodes);
    free(dont_reveal);
  }
  if (delta_parsed) {
    sydo_explicit_bzero(delta_parsed, params->tau * sizeof(*delta_parsed));
    free(delta_parsed);
  }
  return ok;
}

bool sydo_ref_bavc_verify(const sydo_ref_paramset_t* params, const uint8_t* opening,
                          const uint8_t* delta_bytes, const uint8_t* iv, uint8_t* leaves,
                          uint8_t* hashed_leaves) {
  const sydo_ref_bavc_dims_t dims = bavc_dims(params);
  uint8_t* tree = NULL;
  uint8_t* dont_reveal = NULL;
  uint8_t* seed_present = NULL;
  size_t* delta_parsed = NULL;
  size_t* delta_adjusted = NULL;
  size_t opening_pos = params->tau * dims.hash_len;
  size_t delta_bit = 0;
  bool ok = false;
  const size_t width_shift = vole_width_shift(params);

  tree = (uint8_t*)calloc(dims.commit_nodes, dims.lambda_bytes);
  dont_reveal = (uint8_t*)calloc(dims.commit_nodes, 1);
  seed_present = (uint8_t*)calloc(dims.commit_nodes, 1);
  delta_parsed = (size_t*)calloc(params->tau, sizeof(*delta_parsed));
  delta_adjusted = (size_t*)calloc(params->tau, sizeof(*delta_adjusted));
  if (!tree || !dont_reveal || !seed_present || !delta_parsed || !delta_adjusted) {
    goto cleanup;
  }

  for (size_t vec_index = 0; vec_index != params->tau; ++vec_index) {
    const size_t depth = vec_index < dims.num_max_k ? dims.max_k : dims.min_k;
    size_t delta_i = 0;
    for (size_t d = 0; d != depth; ++d) {
      if (get_expanded_delta_bit(delta_bytes, delta_bit + d)) {
        delta_i |= (size_t)1u << d;
      }
    }
    delta_bit += depth;
    delta_adjusted[vec_index] = vole_permute_key_index_inv(delta_i, width_shift, dims.max_k);
    delta_parsed[vec_index] = delta_i;
    dont_reveal[batch_vec_pos_in_tree(params, &dims, vec_index, delta_parsed[vec_index])] = 1;
  }

  clock_t stage_start = trace_clock();
  for (size_t i = dims.commit_leaves - 1u; i != 0; --i) {
    const size_t node = i - 1u;
    const bool left = dont_reveal[2u * node + 1u] != 0;
    const bool right = dont_reveal[2u * node + 2u] != 0;
    dont_reveal[node] = (uint8_t)(left || right);
    if (left != right) {
      const size_t reveal = left ? (2u * node + 2u) : (2u * node + 1u);
      if (opening_pos + dims.lambda_bytes > dims.opening_size) {
        goto cleanup;
      }
      memcpy(tree_node(tree, reveal, dims.lambda_bytes), opening + opening_pos,
             dims.lambda_bytes);
      seed_present[reveal] = 1;
      opening_pos += dims.lambda_bytes;
    }
  }
  trace_stage(params, "verify read_opening", stage_start);

  stage_start = trace_clock();
  for (size_t node = 0; node != dims.commit_leaves - 1u; ++node) {
    if (!seed_present[node]) {
      continue;
    }
    uint8_t out[128];
    if (!sydo_ref_tree_prg_expand_2(params, out, const_tree_node(tree, node, dims.lambda_bytes),
                                    iv, (uint32_t)node)) {
      sydo_explicit_bzero(out, sizeof(out));
      goto cleanup;
    }
    const size_t left = 2u * node + 1u;
    const size_t right = 2u * node + 2u;
    if (!dont_reveal[left]) {
      memcpy(tree_node(tree, left, dims.lambda_bytes), out, dims.lambda_bytes);
      seed_present[left] = 1;
    }
    if (!dont_reveal[right]) {
      memcpy(tree_node(tree, right, dims.lambda_bytes), out + dims.lambda_bytes,
             dims.lambda_bytes);
      seed_present[right] = 1;
    }
    sydo_explicit_bzero(out, sizeof(out));
  }
  trace_stage(params, "verify expand_revealed", stage_start);

  stage_start = trace_clock();
  memset(leaves, 0, dims.commit_leaves * dims.lambda_bytes);
  memset(hashed_leaves, 0, dims.commit_leaves * dims.hash_len);
  for (size_t vec_index = 0; vec_index != params->tau; ++vec_index) {
    const size_t len = batch_vec_len(&dims, vec_index);
    size_t permuted_leaf_idx =
        vole_permute_key_index_inv(0, width_shift, dims.max_k);
    for (size_t leaf_index = 0; leaf_index != len; ++leaf_index) {
      const size_t tree_pos = batch_vec_pos_in_tree(params, &dims, vec_index, leaf_index);
      const size_t hash_pos = batch_vec_hash_pos_in_output(&dims, vec_index, leaf_index);
      if (leaf_index == delta_parsed[vec_index]) {
        memcpy(hashed_leaves + hash_pos * dims.hash_len, opening + vec_index * dims.hash_len,
               dims.hash_len);
      } else {
        if (!seed_present[tree_pos]) {
          goto cleanup;
        }
        const size_t receiver_leaf_idx = permuted_leaf_idx ^ delta_adjusted[vec_index];
        if (!write_leaf(params, &dims, iv, vec_index,
                        const_tree_node(tree, tree_pos, dims.lambda_bytes),
                        leaves +
                            batch_vec_leaf_pos_in_output(&dims, vec_index, receiver_leaf_idx) *
                                dims.lambda_bytes,
                        hashed_leaves + hash_pos * dims.hash_len)) {
          goto cleanup;
        }
      }
      permuted_leaf_idx ^=
          vole_permute_inv_increment(leaf_index, 1, width_shift, dims.max_k);
    }
  }
  trace_stage(params, "verify leaf_hash", stage_start);
  ok = true;

cleanup:
  if (tree) {
    sydo_explicit_bzero(tree, dims.commit_nodes * dims.lambda_bytes);
    free(tree);
  }
  if (dont_reveal) {
    sydo_explicit_bzero(dont_reveal, dims.commit_nodes);
    free(dont_reveal);
  }
  if (seed_present) {
    sydo_explicit_bzero(seed_present, dims.commit_nodes);
    free(seed_present);
  }
  if (delta_parsed) {
    sydo_explicit_bzero(delta_parsed, params->tau * sizeof(*delta_parsed));
    free(delta_parsed);
  }
  if (delta_adjusted) {
    sydo_explicit_bzero(delta_adjusted, params->tau * sizeof(*delta_adjusted));
    free(delta_adjusted);
  }
  return ok;
}
