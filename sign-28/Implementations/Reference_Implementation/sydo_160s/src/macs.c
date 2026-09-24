/*
 *  SPDX-License-Identifier: MIT
 */

#if defined(HAVE_CONFIG_H)
#include <config.h>
#endif

#include "macs.h"

#include "compat.h"
#include "internal.h"
#include "vole.h"

#include <stdlib.h>
#include <string.h>

static void set_bit(uint8_t* x, size_t bit, unsigned int v) {
  const uint8_t mask = (uint8_t)(1u << (bit & 7u));
  if (v) {
    x[bit / 8u] |= mask;
  } else {
    x[bit / 8u] &= (uint8_t)~mask;
  }
}

size_t sydo_ref_witness_blocks(const sydo_ref_paramset_t* params) {
  return sydo_ref_ceil_div_size(sydo_ref_witness_bits(params), SYDO_REF_VOLE_BLOCK_BITS);
}

size_t sydo_ref_quicksilver_rows_padded_extended(const sydo_ref_paramset_t* params) {
  const size_t rows = sydo_ref_witness_extended_bits(params) +
                      (SYDO_REF_RSD_QS_DEGREE - 1u) * (size_t)params->secpar_bits;
  return sydo_ref_ceil_div_size(rows, SYDO_REF_TRANSPOSE_BITS_ROWS) *
         SYDO_REF_TRANSPOSE_BITS_ROWS;
}

void sydo_ref_compute_macs(const sydo_ref_paramset_t* params, const uint8_t* vole_cols,
                           uint8_t* mac_rows, size_t rows_padded) {
  const size_t lambda_bytes = sydo_ref_secpar_bytes(params);
  const size_t col_blocks = sydo_ref_vole_col_blocks(params);
  const size_t col_bytes = col_blocks * SYDO_REF_VOLE_BLOCK_BYTES;

  memset(mac_rows, 0, rows_padded * lambda_bytes);
  for (size_t row = 0; row != rows_padded; ++row) {
    uint8_t* out = mac_rows + row * lambda_bytes;
    const size_t row_block = row / SYDO_REF_VOLE_BLOCK_BITS;
    const size_t row_in_block = row % SYDO_REF_VOLE_BLOCK_BITS;
    const size_t row_byte = row_block * SYDO_REF_VOLE_BLOCK_BYTES + row_in_block / 8u;
    const size_t row_bit = row_in_block & 7u;
    for (size_t col = 0; col != params->secpar_bits; ++col) {
      const uint8_t* col_ptr = vole_cols + col * col_bytes;
      const unsigned int bit = (unsigned int)((col_ptr[row_byte] >> row_bit) & 1u);
      set_bit(out, col, bit);
    }
  }
}

static void extend_common(const sydo_ref_paramset_t* params, const uint8_t* macs,
                          uint8_t* macs_extended) {
  const size_t lambda_bytes = sydo_ref_secpar_bytes(params);
  const size_t ext_bits = sydo_ref_witness_extended_bits(params);
  const size_t ext_aligned_bits = sydo_ref_witness_extended_bytes(params) * 8u;
  const size_t witness_bits = sydo_ref_witness_bits(params);
  const size_t tail_bits = (SYDO_REF_RSD_QS_DEGREE - 1u) * (size_t)params->secpar_bits;
  const size_t rows_padded_ext = sydo_ref_quicksilver_rows_padded_extended(params);

  memset(macs_extended, 0, rows_padded_ext * lambda_bytes);
  for (size_t i = 0; i != ext_bits; ++i) {
    memcpy(macs_extended + i * lambda_bytes, macs + i * lambda_bytes, lambda_bytes);
  }
  for (size_t i = 0; i != tail_bits; ++i) {
    memcpy(macs_extended + (ext_aligned_bits + i) * lambda_bytes,
           macs + (witness_bits + i) * lambda_bytes, lambda_bytes);
  }
}

bool sydo_ref_compute_macs_extended_prover(const sydo_ref_paramset_t* params, const uint8_t* v,
                                           uint8_t* macs_extended) {
  const size_t lambda_bytes = sydo_ref_secpar_bytes(params);
  const size_t rows_padded = sydo_ref_quicksilver_rows_padded(params);
  uint8_t* macs = (uint8_t*)malloc(rows_padded * lambda_bytes);
  if (!macs) {
    return false;
  }
  sydo_ref_compute_macs(params, v, macs, rows_padded);
  extend_common(params, macs, macs_extended);
  sydo_explicit_bzero(macs, rows_padded * lambda_bytes);
  free(macs);
  return true;
}

bool sydo_ref_compute_macs_extended_verifier(const sydo_ref_paramset_t* params, uint8_t* q,
                                             const uint8_t* correction_padded,
                                             const uint8_t* delta_bytes,
                                             uint8_t* macs_extended) {
  const size_t lambda_bytes = sydo_ref_secpar_bytes(params);
  const size_t rows_padded = sydo_ref_quicksilver_rows_padded(params);
  uint8_t* macs = (uint8_t*)malloc(rows_padded * lambda_bytes);
  if (!macs) {
    return false;
  }
  sydo_ref_vole_apply_correction(params, sydo_ref_witness_blocks(params),
                                 sydo_ref_delta_bits(params), correction_padded, q, delta_bytes);
  sydo_ref_compute_macs(params, q, macs, rows_padded);
  extend_common(params, macs, macs_extended);
  sydo_explicit_bzero(macs, rows_padded * lambda_bytes);
  free(macs);
  return true;
}
