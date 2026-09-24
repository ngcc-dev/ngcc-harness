/*
 *  SPDX-License-Identifier: MIT
 */

#ifndef SYDO_REF_INTERNAL_H
#define SYDO_REF_INTERNAL_H

#include "sydo.h"

#include <stddef.h>
#include <stdint.h>

enum {
  SYDO_REF_RSD_NUM_VAR_BLOCKS = 2,
  SYDO_REF_RSD_BLOCK0_SIZE = 2,
  SYDO_REF_RSD_BLOCK0_WEIGHT = 1,
  SYDO_REF_RSD_BLOCK1_SIZE = 8,
  SYDO_REF_RSD_BLOCK1_WEIGHT = 3,
  SYDO_REF_RSD_INPUT_REDUCED_BITS = 10,
  SYDO_REF_RSD_EXTENDED_SUBVECTOR_BITS = 10,
  SYDO_REF_RSD_QS_DEGREE = 4,
  SYDO_REF_RSD_Y_VECTOR_LEN = 3,
  SYDO_REF_TRANSPOSE_BITS_ROWS = 256,
  SYDO_REF_VOLE_BLOCK_BITS = 128
};

typedef struct sydo_ref_signature_layout_t {
  size_t vole_commit_offset;
  size_t vole_commit_size;
  size_t vole_check_offset;
  size_t vole_check_size;
  size_t correction_offset;
  size_t correction_size;
  size_t qs_proof_offset;
  size_t qs_proof_size;
  size_t bavc_open_offset;
  size_t bavc_open_size;
  size_t delta_offset;
  size_t delta_size;
  size_t iv_offset;
  size_t iv_size;
  size_t grinding_counter_offset;
  size_t grinding_counter_size;
  size_t total_size;
} sydo_ref_signature_layout_t;

static inline size_t sydo_ref_ceil_div_size(size_t x, size_t y) {
  return (x + y - 1u) / y;
}

static inline size_t sydo_ref_secpar_bytes(const sydo_ref_paramset_t* params) {
  return params->secpar_bits / 8u;
}

static inline size_t sydo_ref_ceil_log2_size(size_t x) {
  size_t bits = 0;
  size_t v = x <= 1u ? 0u : x - 1u;
  while (v != 0u) {
    ++bits;
    v >>= 1;
  }
  return bits;
}

static inline size_t sydo_ref_delta_bits(const sydo_ref_paramset_t* params) {
  return params->secpar_bits - params->zero_bits_in_delta +
         sydo_ref_ceil_log2_size(SYDO_REF_RSD_QS_DEGREE);
}

static inline size_t sydo_ref_unused_delta_bits(const sydo_ref_paramset_t* params) {
  return params->secpar_bits - sydo_ref_delta_bits(params);
}

static inline size_t sydo_ref_witness_bits(const sydo_ref_paramset_t* params) {
  return sydo_ref_ceil_div_size((size_t)params->rsd_w * SYDO_REF_RSD_INPUT_REDUCED_BITS, 8u) *
         8u;
}

static inline size_t sydo_ref_witness_extended_bits(const sydo_ref_paramset_t* params) {
  return (size_t)params->rsd_w * SYDO_REF_RSD_EXTENDED_SUBVECTOR_BITS;
}

static inline size_t sydo_ref_witness_extended_bytes(const sydo_ref_paramset_t* params) {
  return sydo_ref_ceil_div_size(sydo_ref_witness_extended_bits(params), 8u);
}

static inline size_t sydo_ref_qs_challenge_bytes(const sydo_ref_paramset_t* params) {
  return (3u * params->secpar_bits + 64u +
          SYDO_REF_RSD_Y_VECTOR_LEN * (size_t)params->secpar_bits) /
         8u;
}

static inline size_t sydo_ref_qs_proof_bytes(const sydo_ref_paramset_t* params) {
  return (SYDO_REF_RSD_QS_DEGREE - 1u) * sydo_ref_secpar_bytes(params);
}

static inline size_t sydo_ref_vole_check_hash_bytes(const sydo_ref_paramset_t* params) {
  return sydo_ref_secpar_bytes(params) + 2u;
}

static inline size_t sydo_ref_vole_check_challenge_bytes(const sydo_ref_paramset_t* params) {
  return (5u * params->secpar_bits + 64u) / 8u;
}

static inline size_t sydo_ref_quicksilver_rows(const sydo_ref_paramset_t* params) {
  return sydo_ref_witness_bits(params) +
         (SYDO_REF_RSD_QS_DEGREE - 1u) * (size_t)params->secpar_bits;
}

static inline size_t sydo_ref_quicksilver_rows_padded(const sydo_ref_paramset_t* params) {
  return sydo_ref_ceil_div_size(sydo_ref_quicksilver_rows(params),
                                SYDO_REF_TRANSPOSE_BITS_ROWS) *
         SYDO_REF_TRANSPOSE_BITS_ROWS;
}

static inline size_t sydo_ref_vole_rows(const sydo_ref_paramset_t* params) {
  return sydo_ref_quicksilver_rows(params) + 8u * sydo_ref_vole_check_hash_bytes(params);
}

static inline size_t sydo_ref_vole_col_blocks(const sydo_ref_paramset_t* params) {
  return sydo_ref_ceil_div_size(sydo_ref_vole_rows(params), SYDO_REF_VOLE_BLOCK_BITS);
}

static inline size_t sydo_ref_vole_commit_bytes(const sydo_ref_paramset_t* params) {
  return (sydo_ref_vole_rows(params) / 8u) * ((size_t)params->tau - 1u);
}

static inline size_t sydo_ref_bavc_open_bytes(const sydo_ref_paramset_t* params) {
  return (size_t)params->tau * 2u * sydo_ref_secpar_bytes(params) +
         (size_t)params->bavc_opening_threshold * sydo_ref_secpar_bytes(params);
}

static inline size_t sydo_ref_grinding_counter_bytes(const sydo_ref_paramset_t* params) {
  (void)params;
  return 4u;
}

static inline size_t sydo_ref_iv_bytes(const sydo_ref_paramset_t* params) {
  return sydo_ref_secpar_bytes(params);
}

static inline sydo_ref_signature_layout_t
sydo_ref_signature_layout(const sydo_ref_paramset_t* params) {
  sydo_ref_signature_layout_t out;
  size_t off = 0;
  out.vole_commit_offset = off;
  out.vole_commit_size = sydo_ref_vole_commit_bytes(params);
  off += out.vole_commit_size;
  out.vole_check_offset = off;
  out.vole_check_size = sydo_ref_vole_check_hash_bytes(params);
  off += out.vole_check_size;
  out.correction_offset = off;
  out.correction_size = sydo_ref_witness_bits(params) / 8u;
  off += out.correction_size;
  out.qs_proof_offset = off;
  out.qs_proof_size = sydo_ref_qs_proof_bytes(params);
  off += out.qs_proof_size;
  out.bavc_open_offset = off;
  out.bavc_open_size = sydo_ref_bavc_open_bytes(params);
  off += out.bavc_open_size;
  out.delta_offset = off;
  out.delta_size = sydo_ref_secpar_bytes(params);
  off += out.delta_size;
  out.iv_offset = off;
  out.iv_size = sydo_ref_iv_bytes(params);
  off += out.iv_size;
  out.grinding_counter_offset = off;
  out.grinding_counter_size = sydo_ref_grinding_counter_bytes(params);
  off += out.grinding_counter_size;
  out.total_size = off;
  return out;
}

#endif
