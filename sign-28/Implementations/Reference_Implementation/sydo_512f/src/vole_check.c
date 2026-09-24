/*
 *  SPDX-License-Identifier: MIT
 */

#if defined(HAVE_CONFIG_H)
#include <config.h>
#endif

#include "vole_check.h"

#include "compat.h"
#include "field.h"
#include "internal.h"
#include "uhash.h"
#include "vole.h"

#include <stdlib.h>
#include <string.h>

size_t sydo_ref_vole_check_proof_size(const sydo_ref_paramset_t* params) {
  return sydo_ref_vole_check_hash_bytes(params);
}

size_t sydo_ref_vole_check_challenge_size(const sydo_ref_paramset_t* params) {
  return sydo_ref_vole_check_challenge_bytes(params);
}

size_t sydo_ref_vole_check_transcript_size(const sydo_ref_paramset_t* params) {
  return ((size_t)params->secpar_bits + 1u) * sydo_ref_vole_check_hash_bytes(params);
}

static void load_challenge(const sydo_ref_paramset_t* params, const uint8_t* challenge,
                           const uint8_t** matrix, const uint8_t** key_secpar,
                           uint64_t* key64) {
  const size_t lambda_bytes = sydo_ref_secpar_bytes(params);
  for (size_t i = 0; i != 4u; ++i) {
    matrix[i] = challenge + i * lambda_bytes;
  }
  *key_secpar = challenge + 4u * lambda_bytes;
  *key64 = sydo_ref_load64_le(challenge + 5u * lambda_bytes);
}

static bool hash_column(const sydo_ref_paramset_t* params, const uint8_t* to_hash,
                        const uint8_t* key_secpar, uint64_t key64, uint8_t* out_secpar,
                        uint8_t out64[8]) {
  const size_t lambda_bytes = sydo_ref_secpar_bytes(params);
  const size_t lambda_bits = params->secpar_bits;
  const size_t qs_rows = sydo_ref_quicksilver_rows(params);
  const size_t padded_rows = sydo_ref_ceil_div_size(qs_rows, lambda_bits) * lambda_bits;
  const size_t gf64_chunks_per_secpar = sydo_ref_ceil_div_size(lambda_bits, 64u);
  const size_t padded_gf64_coefficients =
      (padded_rows / lambda_bits) * gf64_chunks_per_secpar;
  sydo_ref_uhash_secpar_key_t key_s;
  sydo_ref_uhash64_key_t key_64;
  sydo_ref_uhash_secpar_state_t state_s;
  sydo_ref_uhash64_state_t state_64;
  uint8_t chunk[64];

  sydo_ref_uhash_secpar_key_init(&key_s, key_secpar, params);
  sydo_ref_uhash64_key_init(&key_64, key64);
  sydo_ref_uhash_secpar_state_init(&state_s, padded_rows / lambda_bits);
  sydo_ref_uhash64_state_init(&state_64, padded_gf64_coefficients);

  for (size_t bit_off = 0; bit_off != padded_rows; bit_off += lambda_bits) {
    const size_t remaining_bits = qs_rows > bit_off ? qs_rows - bit_off : 0u;
    const size_t take_bits = remaining_bits < lambda_bits ? remaining_bits : lambda_bits;
    const size_t take_bytes = (take_bits + 7u) / 8u;
    memset(chunk, 0, sizeof(chunk));
    if (take_bytes != 0u) {
      memcpy(chunk, to_hash + bit_off / 8u, take_bytes);
    }
    sydo_ref_uhash_secpar_update(&state_s, &key_s, chunk, params);
    for (size_t j = 0; j < lambda_bits; j += 64u) {
      uint8_t tmp[8] = {0};
      const size_t byte_off = j / 8u;
      const size_t bytes_left = lambda_bytes > byte_off ? lambda_bytes - byte_off : 0u;
      const size_t bytes_to_copy = bytes_left < sizeof(tmp) ? bytes_left : sizeof(tmp);
      if (bytes_to_copy != 0u) {
        memcpy(tmp, chunk + byte_off, bytes_to_copy);
      }
      sydo_ref_uhash64_update(&state_64, &key_64, sydo_ref_load64_le(tmp));
    }
  }
  sydo_ref_uhash_secpar_finalize(&state_s, out_secpar, params);
  sydo_ref_store64_le(out64, sydo_ref_uhash64_finalize(&state_64));
  return true;
}

static bool map_hashes(const sydo_ref_paramset_t* params, const uint8_t** matrix,
                       const uint8_t* hash_secpar, const uint8_t hash64[8],
                       uint8_t* mapped_two_blocks) {
  const size_t lambda_bytes = sydo_ref_secpar_bytes(params);

  for (size_t j = 0; j != 2u; ++j) {
    uint8_t hash64_field[64];
    memset(mapped_two_blocks + j * lambda_bytes, 0, lambda_bytes);
    memset(hash64_field, 0, sizeof(hash64_field));
    memcpy(hash64_field, hash64, 8u);
    sydo_ref_field_muladd(mapped_two_blocks + j * lambda_bytes, matrix[2u * j], hash_secpar,
                          params);
    sydo_ref_field_muladd(mapped_two_blocks + j * lambda_bytes, matrix[2u * j + 1u],
                          hash64_field, params);
  }
  return true;
}

static void xor_mask_inplace(uint8_t* out, const uint8_t* mask, size_t len) {
  for (size_t i = 0; i != len; ++i) {
    out[i] ^= mask[i];
  }
}

static void xor_byte_mask_inplace(uint8_t* out, const uint8_t* in, size_t len, uint8_t mask) {
  for (size_t i = 0; i != len; ++i) {
    out[i] ^= (uint8_t)(in[i] & mask);
  }
}

static bool vole_check_both(const sydo_ref_paramset_t* params, bool verifier, const uint8_t* u,
                            const uint8_t* vq, const uint8_t* delta_bytes,
                            const uint8_t* challenge, uint8_t* proof, uint8_t* transcript,
                            size_t transcript_len) {
  const size_t lambda_bytes = sydo_ref_secpar_bytes(params);
  const size_t col_blocks = sydo_ref_vole_col_blocks(params);
  const size_t col_bytes = col_blocks * SYDO_REF_VOLE_BLOCK_BYTES;
  const size_t qs_bytes = sydo_ref_quicksilver_rows(params) / 8u;
  const size_t hash_bytes = sydo_ref_vole_check_hash_bytes(params);
  const size_t transcript_bytes = sydo_ref_vole_check_transcript_size(params);
  const uint8_t* matrix[4];
  const uint8_t* key_secpar = NULL;
  uint64_t key64 = 0;
  uint8_t* u_hash = (uint8_t*)calloc(2u, lambda_bytes);
  uint8_t* hash_secpar = (uint8_t*)malloc(lambda_bytes);
  uint8_t hash64[8];
  uint8_t* mapped = (uint8_t*)malloc(2u * lambda_bytes);
  size_t transcript_off = 0;
  bool ok = false;

  if (transcript_len < transcript_bytes || !u_hash || !hash_secpar || !mapped) {
    goto cleanup;
  }
  load_challenge(params, challenge, matrix, &key_secpar, &key64);
  if (verifier) {
    memcpy(u_hash, proof, hash_bytes);
    memcpy(transcript, u_hash, hash_bytes);
    transcript_off += hash_bytes;
  }

  for (int col = verifier ? 0 : -1; col < (int)sydo_ref_delta_bits(params); ++col) {
    const uint8_t* to_hash = col == -1 ? u : vq + (size_t)col * col_bytes;
    uint8_t* hash_output = col == -1 ? u_hash : mapped;

    memset(mapped, 0, 2u * lambda_bytes);
    if (!hash_column(params, to_hash, key_secpar, key64, hash_secpar, hash64)) {
      goto cleanup;
    }
    if (!map_hashes(params, matrix, hash_secpar, hash64, hash_output)) {
      goto cleanup;
    }
    xor_mask_inplace(hash_output, to_hash + qs_bytes, hash_bytes);
    if (verifier) {
      xor_byte_mask_inplace(hash_output, u_hash, hash_bytes, delta_bytes[col]);
    }
    memcpy(transcript + transcript_off, hash_output, hash_bytes);
    transcript_off += hash_bytes;
  }

  for (size_t col = sydo_ref_delta_bits(params); col != params->secpar_bits; ++col) {
    const uint8_t* to_hash = vq + col * col_bytes;
    memcpy(transcript + transcript_off, to_hash + qs_bytes, hash_bytes);
    transcript_off += hash_bytes;
  }

  if (!verifier) {
    memcpy(proof, u_hash, hash_bytes);
  }
  ok = transcript_off == transcript_bytes;

cleanup:
  if (u_hash) {
    sydo_explicit_bzero(u_hash, 2u * lambda_bytes);
    free(u_hash);
  }
  if (hash_secpar) {
    sydo_explicit_bzero(hash_secpar, lambda_bytes);
    free(hash_secpar);
  }
  if (mapped) {
    sydo_explicit_bzero(mapped, 2u * lambda_bytes);
    free(mapped);
  }
  sydo_explicit_bzero(hash64, sizeof(hash64));
  return ok;
}

bool sydo_ref_vole_check_sender(const sydo_ref_paramset_t* params, const uint8_t* u,
                                const uint8_t* v, const uint8_t* challenge, uint8_t* proof,
                                uint8_t* transcript, size_t transcript_len) {
  return vole_check_both(params, false, u, v, NULL, challenge, proof, transcript, transcript_len);
}

bool sydo_ref_vole_check_receiver(const sydo_ref_paramset_t* params, const uint8_t* q,
                                  const uint8_t* delta_bytes, const uint8_t* challenge,
                                  const uint8_t* proof, uint8_t* transcript,
                                  size_t transcript_len) {
  uint8_t* proof_copy = (uint8_t*)malloc(sydo_ref_vole_check_hash_bytes(params));
  bool ok = false;

  if (!proof_copy) {
    return false;
  }
  memcpy(proof_copy, proof, sydo_ref_vole_check_hash_bytes(params));
  ok = vole_check_both(params, true, NULL, q, delta_bytes, challenge, proof_copy, transcript,
                       transcript_len);
  sydo_explicit_bzero(proof_copy, sydo_ref_vole_check_hash_bytes(params));
  free(proof_copy);
  return ok;
}
