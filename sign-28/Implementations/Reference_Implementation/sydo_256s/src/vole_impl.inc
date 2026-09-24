/*
 *  SPDX-License-Identifier: MIT
 */

#if defined(HAVE_CONFIG_H)
#include <config.h>
#endif

#include "vole.h"

#include "aes.h"
#include "compat.h"
#include "lib/blake2/ref/blake2.h"
#include "internal.h"
#include "primitives.h"

#include <stdlib.h>
#include <string.h>

static unsigned int ctz_size(size_t x) {
  unsigned int out = 0;
  while (((x >> out) & 1u) == 0u && out < 8u * sizeof(x)) {
    ++out;
  }
  return out;
}

static void block128_xor_inplace(uint8_t* out, const uint8_t* in) {
  for (size_t i = 0; i != SYDO_REF_VOLE_BLOCK_BYTES; ++i) {
    out[i] ^= in[i];
  }
}

static void block128_and_byte(uint8_t* out, const uint8_t* in, uint8_t mask) {
  for (size_t i = 0; i != SYDO_REF_VOLE_BLOCK_BYTES; ++i) {
    out[i] = (uint8_t)(in[i] & mask);
  }
}

static size_t prg_output_parts(const sydo_ref_paramset_t* params) {
  const size_t lambda_bytes = sydo_ref_secpar_bytes(params);
  if (lambda_bytes > SYDO_REF_VOLE_BLOCK_BYTES &&
      (lambda_bytes % SYDO_REF_VOLE_BLOCK_BYTES) == 0u) {
    return lambda_bytes / SYDO_REF_VOLE_BLOCK_BYTES;
  }
  return 1u;
}

static void xor_reduce_8(uint8_t blocks[8][SYDO_REF_VOLE_BLOCK_BYTES]) {
  for (size_t i = 0; i != 3u; ++i) {
    const size_t stride = (size_t)1u << i;
    for (size_t j = 0; j != 8u; j += 2u * stride) {
      for (size_t d = 0; d <= i; ++d) {
        block128_xor_inplace(blocks[j + d], blocks[j + d + stride]);
      }
      memcpy(blocks[j + i + 1u], blocks[j + stride], SYDO_REF_VOLE_BLOCK_BYTES);
    }
  }
}

static void make_vole_prg_input(uint8_t* out, const uint8_t* iv, size_t len, uint32_t tweak,
                                uint32_t counter) {
  uint8_t add[64];
  sydo_ref_block_set_low_high32(add, len, counter, tweak);
  sydo_ref_block_add32(out, iv, add, len);
  sydo_explicit_bzero(add, sizeof(add));
}

static void init_vole_round_keys(const sydo_ref_paramset_t* params, aes_round_keys_t* round_keys,
                                 const uint8_t* keys, size_t key_count) {
  const size_t lambda_bytes = sydo_ref_secpar_bytes(params);
  for (size_t i = 0; i != key_count; ++i) {
    const uint8_t* key = keys + i * lambda_bytes;
    if (params->secpar_bits == 160) {
      uint8_t padded_key[24] = {0};
      memcpy(padded_key, key, 20u);
      rijndael192_init_round_keys(&round_keys[i], padded_key);
      sydo_explicit_bzero(padded_key, sizeof(padded_key));
    } else {
      rijndael256_init_round_keys(&round_keys[i], key);
    }
  }
}

static void eval_vole_round_key(const sydo_ref_paramset_t* params, uint8_t* out,
                                const aes_round_keys_t* round_key, const uint8_t* iv,
                                uint32_t tweak, uint32_t counter) {
  uint8_t input[64];
  const size_t lambda_bytes = sydo_ref_secpar_bytes(params);

  make_vole_prg_input(input, iv, lambda_bytes, tweak, counter);
  if (params->secpar_bits == 160) {
    uint8_t padded_input[24] = {0};
    uint8_t padded_output[24] = {0};
    memcpy(padded_input, input, 20u);
    rijndael192_encrypt_block(round_key, padded_input, padded_output);
    memcpy(out, padded_output, 20u);
    sydo_explicit_bzero(padded_input, sizeof(padded_input));
    sydo_explicit_bzero(padded_output, sizeof(padded_output));
  } else {
    rijndael256_encrypt_block(round_key, input, out);
  }
  sydo_explicit_bzero(input, sizeof(input));
}

static bool process_output_parts_round_keys(const sydo_ref_paramset_t* params, bool ignore_0,
                                            size_t row_block, size_t num_parts,
                                            unsigned int output_col, uint8_t* accum, uint8_t* vq,
                                            const aes_round_keys_t* round_keys,
                                            const uint8_t* iv, uint32_t tweak,
                                            uint32_t counter) {
  uint8_t raw[8][64];
  uint8_t prg_output[8][SYDO_REF_VOLE_BLOCK_BYTES];
  const size_t lambda_bytes = sydo_ref_secpar_bytes(params);
  const size_t col_blocks = sydo_ref_vole_col_blocks(params);
  const size_t first = ignore_0 ? 1u : 0u;

  if (ignore_0) {
    memset(raw[0], 0, lambda_bytes);
  }
  for (size_t i = first; i != 8u; ++i) {
    const size_t key_index = ignore_0 ? i - 1u : i;
    eval_vole_round_key(params, raw[i], &round_keys[key_index], iv, tweak, counter);
  }

  for (size_t part = 0; part != num_parts; ++part) {
    const size_t dst_row = row_block + part;
    for (size_t i = 0; i != 8u; ++i) {
      memcpy(prg_output[i], raw[i] + part * SYDO_REF_VOLE_BLOCK_BYTES,
             SYDO_REF_VOLE_BLOCK_BYTES);
    }

    xor_reduce_8(prg_output);

    if (!ignore_0) {
      block128_xor_inplace(accum + dst_row * SYDO_REF_VOLE_BLOCK_BYTES, prg_output[0]);
    }
    for (size_t col = 0; col != 3u; ++col) {
      block128_xor_inplace(vq + (col * col_blocks + dst_row) * SYDO_REF_VOLE_BLOCK_BYTES,
                           prg_output[col + 1u]);
    }
    if (!ignore_0) {
      block128_xor_inplace(vq + (output_col * col_blocks + dst_row) * SYDO_REF_VOLE_BLOCK_BYTES,
                           accum + dst_row * SYDO_REF_VOLE_BLOCK_BYTES);
    }
  }
  sydo_explicit_bzero(raw, sizeof(raw));
  return true;
}

static void init_vole_blake_keys(uint8_t expanded_keys[8][64], const uint8_t* keys,
                                 size_t key_count) {
  for (size_t i = 0; i != key_count; ++i) {
    memcpy(expanded_keys[i], keys + i * 64u, 64u);
  }
}

static void eval_vole_blake_key(uint8_t* out, const uint8_t key[64], const uint8_t* iv,
                                uint32_t tweak, uint32_t counter) {
  uint8_t input[64];
  make_vole_prg_input(input, iv, sizeof(input), tweak, counter);
  (void)sydo_ref_blake2s_512_bc_eval(out, key, input);
}

static bool process_output_parts_blake_keys(bool ignore_0, size_t row_block, size_t num_parts,
                                            unsigned int output_col, uint8_t* accum,
                                            uint8_t* vq, const uint8_t keys[8][64],
                                            const uint8_t* iv, uint32_t tweak,
                                            uint32_t counter, size_t col_blocks) {
  uint8_t raw[8][64];
  uint8_t prg_output[8][SYDO_REF_VOLE_BLOCK_BYTES];
  const size_t first = ignore_0 ? 1u : 0u;

  if (ignore_0) {
    memset(raw[0], 0, sizeof(raw[0]));
  }
  for (size_t i = first; i != 8u; ++i) {
    const size_t key_index = ignore_0 ? i - 1u : i;
    eval_vole_blake_key(raw[i], keys[key_index], iv, tweak, counter);
  }

  for (size_t part = 0; part != num_parts; ++part) {
    const size_t dst_row = row_block + part;
    for (size_t i = 0; i != 8u; ++i) {
      memcpy(prg_output[i], raw[i] + part * SYDO_REF_VOLE_BLOCK_BYTES,
             SYDO_REF_VOLE_BLOCK_BYTES);
    }
    xor_reduce_8(prg_output);
    if (!ignore_0) {
      block128_xor_inplace(accum + dst_row * SYDO_REF_VOLE_BLOCK_BYTES, prg_output[0]);
    }
    for (size_t col = 0; col != 3u; ++col) {
      block128_xor_inplace(vq + (col * col_blocks + dst_row) * SYDO_REF_VOLE_BLOCK_BYTES,
                           prg_output[col + 1u]);
    }
    if (!ignore_0) {
      block128_xor_inplace(vq + (output_col * col_blocks + dst_row) * SYDO_REF_VOLE_BLOCK_BYTES,
                           accum + dst_row * SYDO_REF_VOLE_BLOCK_BYTES);
    }
  }
  sydo_explicit_bzero(raw, sizeof(raw));
  return true;
}

static void vole_both(const sydo_ref_paramset_t* params, bool receiver, unsigned int k,
                      const uint8_t* keys, const uint8_t* iv, uint32_t tweak,
                      const uint8_t* u_or_c_in, uint8_t* vq, uint8_t* c_out,
                      const uint8_t* delta) {
  const size_t col_blocks = sydo_ref_vole_col_blocks(params);
  const size_t col_bytes = col_blocks * SYDO_REF_VOLE_BLOCK_BYTES;
  const size_t lambda_bytes = sydo_ref_secpar_bytes(params);
  const size_t parts = prg_output_parts(params);
  uint8_t* accum = (uint8_t*)calloc(col_blocks, SYDO_REF_VOLE_BLOCK_BYTES);
  if (!accum) {
    return;
  }

  if (receiver && u_or_c_in) {
    for (unsigned int col = 0; col != k; ++col) {
      for (size_t j = 0; j != col_bytes; ++j) {
        vq[col * col_bytes + j] = (uint8_t)(u_or_c_in[j] & delta[col]);
      }
    }
  } else {
    memset(vq, 0, (size_t)k * col_bytes);
  }

  size_t i = 0;
  if (receiver) {
    if (params->secpar_bits != 512) {
      aes_round_keys_t round_keys[7];
      init_vole_round_keys(params, round_keys, keys + lambda_bytes, 7u);
      for (size_t row = 0; row < col_blocks; row += parts) {
        const uint32_t counter = (uint32_t)(row / parts);
        const size_t row_parts = row + parts <= col_blocks ? parts : col_blocks - row;
        if (!process_output_parts_round_keys(params, true, row, row_parts, 0, accum, vq,
                                             round_keys, iv, tweak, counter)) {
          sydo_explicit_bzero(round_keys, sizeof(round_keys));
          goto cleanup;
        }
      }
      sydo_explicit_bzero(round_keys, sizeof(round_keys));
    } else {
      uint8_t states[8][64];
      init_vole_blake_keys(states, keys + lambda_bytes, 7u);
      for (size_t row = 0; row < col_blocks; row += parts) {
        const uint32_t counter = (uint32_t)(row / parts);
        const size_t row_parts = row + parts <= col_blocks ? parts : col_blocks - row;
        if (!process_output_parts_blake_keys(true, row, row_parts, 0, accum, vq, states, iv,
                                               tweak, counter, col_blocks)) {
          sydo_explicit_bzero(states, sizeof(states));
          goto cleanup;
        }
      }
      sydo_explicit_bzero(states, sizeof(states));
    }
    i = 8u;
  }

  for (; i < ((size_t)1u << k); i += 8u) {
    const unsigned int output_col = ctz_size((i + 8u) | ((size_t)1u << (k - 1u)));
    if (params->secpar_bits != 512) {
      aes_round_keys_t round_keys[8];
      init_vole_round_keys(params, round_keys, keys + i * lambda_bytes, 8u);
      for (size_t row = 0; row < col_blocks; row += parts) {
        const uint32_t counter = (uint32_t)(row / parts);
        const size_t row_parts = row + parts <= col_blocks ? parts : col_blocks - row;
        if (!process_output_parts_round_keys(params, false, row, row_parts, output_col, accum, vq,
                                             round_keys, iv, tweak, counter)) {
          sydo_explicit_bzero(round_keys, sizeof(round_keys));
          goto cleanup;
        }
      }
      sydo_explicit_bzero(round_keys, sizeof(round_keys));
    } else {
      uint8_t states[8][64];
      init_vole_blake_keys(states, keys + i * lambda_bytes, 8u);
      for (size_t row = 0; row < col_blocks; row += parts) {
        const uint32_t counter = (uint32_t)(row / parts);
        const size_t row_parts = row + parts <= col_blocks ? parts : col_blocks - row;
        if (!process_output_parts_blake_keys(false, row, row_parts, output_col, accum, vq,
                                               states, iv, tweak, counter, col_blocks)) {
          sydo_explicit_bzero(states, sizeof(states));
          goto cleanup;
        }
      }
      sydo_explicit_bzero(states, sizeof(states));
    }
  }

  if (!receiver) {
    if (u_or_c_in) {
      for (size_t j = 0; j != col_bytes; ++j) {
        c_out[j] = (uint8_t)(u_or_c_in[j] ^ accum[j]);
      }
    } else {
      memcpy(c_out, accum, col_bytes);
    }
  }

cleanup:
  sydo_explicit_bzero(accum, col_bytes);
  free(accum);
}

void sydo_ref_vole_sender(const sydo_ref_paramset_t* params, unsigned int k, const uint8_t* keys,
                          const uint8_t* iv, uint32_t tweak, const uint8_t* u, uint8_t* v,
                          uint8_t* c) {
  vole_both(params, false, k, keys, iv, tweak, u, v, c, NULL);
}

void sydo_ref_vole_receiver(const sydo_ref_paramset_t* params, unsigned int k,
                            const uint8_t* keys, const uint8_t* iv, uint32_t tweak,
                            const uint8_t* c, uint8_t* q, const uint8_t* delta) {
  vole_both(params, true, k, keys, iv, tweak, c, q, NULL, delta);
}

void sydo_ref_vole_apply_correction(const sydo_ref_paramset_t* params, size_t row_blocks,
                                    size_t cols, const uint8_t* c, uint8_t* q,
                                    const uint8_t* delta) {
  const size_t col_blocks = sydo_ref_vole_col_blocks(params);
  for (size_t col = 0; col != cols; ++col) {
    for (size_t block = 0; block != row_blocks; ++block) {
      uint8_t masked[SYDO_REF_VOLE_BLOCK_BYTES];
      block128_and_byte(masked, c + block * SYDO_REF_VOLE_BLOCK_BYTES, delta[col]);
      block128_xor_inplace(q + (col * col_blocks + block) * SYDO_REF_VOLE_BLOCK_BYTES, masked);
    }
  }
}
