/*
 *  SPDX-License-Identifier: MIT
 */

#if defined(HAVE_CONFIG_H)
#include <config.h>
#endif

#include "vole_commit.h"

#include "compat.h"
#include "hash.h"
#include "internal.h"
#include "vole.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static bool sydo_ref_vole_trace_enabled(void) {
  return getenv("SYDO_REF_TRACE") != NULL;
}

static clock_t sydo_ref_vole_trace_clock(void) {
  return clock();
}

static void sydo_ref_vole_trace_stage(const sydo_ref_paramset_t* params, const char* stage,
                                      clock_t start) {
  if (sydo_ref_vole_trace_enabled()) {
    const double ms = 1000.0 * (double)(clock() - start) / (double)CLOCKS_PER_SEC;
    fprintf(stderr, "%s vole_commit %s %.2f ms\n", params->name, stage, ms);
  }
}

static size_t min_k(const sydo_ref_paramset_t* params) {
  return sydo_ref_delta_bits(params) / params->tau;
}

static size_t max_k(const sydo_ref_paramset_t* params) {
  return sydo_ref_ceil_div_size(sydo_ref_delta_bits(params), params->tau);
}

static size_t num_max_k(const sydo_ref_paramset_t* params) {
  return sydo_ref_delta_bits(params) % params->tau;
}

static size_t vec_k(const sydo_ref_paramset_t* params, size_t i) {
  return i < num_max_k(params) ? max_k(params) : min_k(params);
}

static bool hash_segment(uint8_t* out, uint8_t* data, size_t data_len, size_t out_len) {
  return sydo_ref_xof_with_suffix(out, out_len, data, data_len, 1u);
}

static bool hash_hashed_leaves(const sydo_ref_paramset_t* params, uint8_t* hashed_leaves,
                               uint8_t* check) {
  const size_t lambda_bytes = sydo_ref_secpar_bytes(params);
  const size_t hash_len = 2u * lambda_bytes;
  const size_t min = min_k(params);
  const size_t max = max_k(params);
  const size_t nmax = num_max_k(params);
  const size_t nmin = params->tau - nmax;
  const size_t long_bytes = hash_len << max;
  const size_t short_bytes = hash_len << min;
  const size_t digest_bytes = 2u * lambda_bytes;
  uint8_t* digests = NULL;
  size_t digest_count = 0;
  size_t off = 0;
  bool ok = false;

  digests = (uint8_t*)malloc(params->tau * digest_bytes + 1u);
  if (!digests) {
    return false;
  }
  for (size_t i = 0; i != nmax; ++i) {
    if (!hash_segment(digests + digest_count * digest_bytes, hashed_leaves + off, long_bytes,
                      digest_bytes)) {
      goto cleanup;
    }
    off += long_bytes;
    ++digest_count;
  }
  for (size_t i = 0; i != nmin; ++i) {
    if (!hash_segment(digests + digest_count * digest_bytes, hashed_leaves + off, short_bytes,
                      digest_bytes)) {
      goto cleanup;
    }
    off += short_bytes;
    ++digest_count;
  }
  digests[digest_count * digest_bytes] = 1;
  ok = sydo_ref_xof(check, 2u * lambda_bytes, digests, digest_count * digest_bytes + 1u);

cleanup:
  sydo_explicit_bzero(digests, params->tau * digest_bytes + 1u);
  free(digests);
  return ok;
}

bool sydo_ref_vole_commit(const sydo_ref_paramset_t* params, const uint8_t* seed,
                          const uint8_t* iv, sydo_ref_bavc_t* bavc, uint8_t* u, uint8_t* v,
                          uint8_t* commitment, uint8_t* check) {
  const size_t lambda_bytes = sydo_ref_secpar_bytes(params);
  const size_t col_blocks = sydo_ref_vole_col_blocks(params);
  const size_t col_bytes = col_blocks * SYDO_REF_VOLE_BLOCK_BYTES;
  const size_t rows_bytes = sydo_ref_vole_rows(params) / 8u;
  uint8_t* correction = NULL;
  uint8_t* leaves_iter;
  clock_t stage_start = sydo_ref_vole_trace_clock();

  if (!sydo_ref_bavc_commit(params, bavc, seed, iv)) {
    return false;
  }
  sydo_ref_vole_trace_stage(params, "bavc_commit_total", stage_start);
  stage_start = sydo_ref_vole_trace_clock();
  if (!hash_hashed_leaves(params, bavc->hashed_leaves, check)) {
    return false;
  }
  sydo_ref_vole_trace_stage(params, "hash_hashed_leaves", stage_start);

  correction = (uint8_t*)malloc(col_bytes);
  if (!correction) {
    return false;
  }
  leaves_iter = bavc->leaves;
  stage_start = sydo_ref_vole_trace_clock();
  for (size_t i = 0; i != params->tau; ++i) {
    const unsigned int k = (unsigned int)vec_k(params, i);
    const uint32_t tweak = UINT32_C(0x80000000) + (uint32_t)i;
    if (i == 0) {
      sydo_ref_vole_sender(params, k, leaves_iter, iv, tweak, NULL, v, u);
    } else {
      sydo_ref_vole_sender(params, k, leaves_iter, iv, tweak, u, v, correction);
      memcpy(commitment, correction, rows_bytes);
      commitment += rows_bytes;
    }
    leaves_iter += ((size_t)1u << k) * lambda_bytes;
    v += col_blocks * k * SYDO_REF_VOLE_BLOCK_BYTES;
  }
  sydo_ref_vole_trace_stage(params, "sender_loop", stage_start);
  if (sydo_ref_unused_delta_bits(params) != 0) {
    memset(v, 0, col_blocks * sydo_ref_unused_delta_bits(params) * SYDO_REF_VOLE_BLOCK_BYTES);
  }
  sydo_explicit_bzero(correction, col_bytes);
  free(correction);
  return true;
}

bool sydo_ref_vole_reconstruct(const sydo_ref_paramset_t* params, const uint8_t* iv, uint8_t* q,
                               const uint8_t* delta_bytes, const uint8_t* commitment,
                               const uint8_t* opening, uint8_t* check) {
  const size_t lambda_bytes = sydo_ref_secpar_bytes(params);
  const size_t col_blocks = sydo_ref_vole_col_blocks(params);
  const size_t col_bytes = col_blocks * SYDO_REF_VOLE_BLOCK_BYTES;
  const size_t rows_bytes = sydo_ref_vole_rows(params) / 8u;
  uint8_t* correction = NULL;
  uint8_t* leaves_iter;
  const uint8_t* delta_iter = delta_bytes;
  bool ok = false;
  clock_t stage_start = sydo_ref_vole_trace_clock();

  const size_t commit_leaves =
      (params->tau - num_max_k(params)) * ((size_t)1u << min_k(params)) +
      num_max_k(params) * ((size_t)1u << max_k(params));
  uint8_t* leaves = (uint8_t*)malloc(commit_leaves * lambda_bytes);
  uint8_t* hashed_leaves = (uint8_t*)malloc(commit_leaves * 2u * lambda_bytes + 1u);
  correction = (uint8_t*)calloc(col_blocks, SYDO_REF_VOLE_BLOCK_BYTES);
  if (!leaves || !hashed_leaves || !correction) {
    free(leaves);
    free(hashed_leaves);
    free(correction);
    return false;
  }
  if (!sydo_ref_bavc_verify(params, opening, delta_bytes, iv, leaves, hashed_leaves)) {
    goto cleanup;
  }
  sydo_ref_vole_trace_stage(params, "bavc_verify_total", stage_start);
  stage_start = sydo_ref_vole_trace_clock();
  if (!hash_hashed_leaves(params, hashed_leaves, check)) {
    goto cleanup;
  }
  sydo_ref_vole_trace_stage(params, "hash_hashed_leaves", stage_start);

  leaves_iter = leaves;
  stage_start = sydo_ref_vole_trace_clock();
  for (size_t i = 0; i != params->tau; ++i) {
    const unsigned int k = (unsigned int)vec_k(params, i);
    const uint32_t tweak = UINT32_C(0x80000000) + (uint32_t)i;
    if (i == 0) {
      sydo_ref_vole_receiver(params, k, leaves_iter, iv, tweak, NULL, q, delta_iter);
    } else {
      memset(correction, 0, col_bytes);
      memcpy(correction, commitment, rows_bytes);
      commitment += rows_bytes;
      sydo_ref_vole_receiver(params, k, leaves_iter, iv, tweak, correction, q, delta_iter);
    }
    leaves_iter += ((size_t)1u << k) * lambda_bytes;
    q += col_blocks * k * SYDO_REF_VOLE_BLOCK_BYTES;
    delta_iter += k;
  }
  sydo_ref_vole_trace_stage(params, "receiver_loop", stage_start);
  if (sydo_ref_unused_delta_bits(params) != 0) {
    memset(q, 0, col_blocks * sydo_ref_unused_delta_bits(params) * SYDO_REF_VOLE_BLOCK_BYTES);
  }
  ok = true;

cleanup:
  if (leaves) {
    sydo_explicit_bzero(leaves, commit_leaves * lambda_bytes);
    free(leaves);
  }
  if (hashed_leaves) {
    sydo_explicit_bzero(hashed_leaves, commit_leaves * 2u * lambda_bytes);
    free(hashed_leaves);
  }
  if (correction) {
    sydo_explicit_bzero(correction, col_bytes);
    free(correction);
  }
  return ok;
}
