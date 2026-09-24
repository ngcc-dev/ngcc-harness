/*
 *  SPDX-License-Identifier: MIT
 */

#if defined(HAVE_CONFIG_H)
#include <config.h>
#endif

#include "primitives.h"

#include "aes.h"
#include "compat.h"
#include "lib/blake2/ref/blake2.h"
#include "internal.h"

#include <string.h>

enum { SYDO_REF_PRG_CACHE_SIZE = 128 };

typedef struct sydo_ref_prg_cache_entry_t {
  uint8_t valid;
  uint16_t secpar_bits;
  uint32_t age;
  uint8_t key[32];
  aes_round_keys_t round_keys;
} sydo_ref_prg_cache_entry_t;

static sydo_ref_prg_cache_entry_t prg_cache[SYDO_REF_PRG_CACHE_SIZE];
static uint32_t prg_cache_age = 1;

static uint32_t load32_le(const uint8_t* src) {
  return (uint32_t)src[0] | ((uint32_t)src[1] << 8) | ((uint32_t)src[2] << 16) |
         ((uint32_t)src[3] << 24);
}

static void store32_le(uint8_t* dst, uint32_t v) {
  dst[0] = (uint8_t)(v & 0xffu);
  dst[1] = (uint8_t)((v >> 8) & 0xffu);
  dst[2] = (uint8_t)((v >> 16) & 0xffu);
  dst[3] = (uint8_t)((v >> 24) & 0xffu);
}

void sydo_ref_block_zero(uint8_t* out, size_t len) {
  memset(out, 0, len);
}

void sydo_ref_block_xor(uint8_t* out, const uint8_t* a, const uint8_t* b, size_t len) {
  for (size_t i = 0; i != len; ++i) {
    out[i] = (uint8_t)(a[i] ^ b[i]);
  }
}

void sydo_ref_block_and(uint8_t* out, const uint8_t* a, const uint8_t* b, size_t len) {
  for (size_t i = 0; i != len; ++i) {
    out[i] = (uint8_t)(a[i] & b[i]);
  }
}

void sydo_ref_block_set_all_8(uint8_t* out, size_t len, uint8_t byte) {
  memset(out, byte, len);
}

void sydo_ref_block_add32(uint8_t* out, const uint8_t* a, const uint8_t* b, size_t len) {
  for (size_t i = 0; i != len; i += 4) {
    store32_le(out + i, load32_le(a + i) + load32_le(b + i));
  }
}

void sydo_ref_block_set_low_high32(uint8_t* out, size_t len, uint32_t low, uint32_t high) {
  memset(out, 0, len);
  store32_le(out, low);
  store32_le(out + len - 4u, high);
}

static void make_ctr_input(uint8_t* out, const uint8_t* iv, size_t len, uint32_t tweak,
                           uint32_t counter) {
  uint8_t add[64];
  sydo_ref_block_set_low_high32(add, len, counter, tweak);
  sydo_ref_block_add32(out, iv, add, len);
  sydo_explicit_bzero(add, sizeof(add));
}

static bool rijndael192_trunc160_eval(uint8_t* out, const uint8_t* key, const uint8_t* input) {
  uint8_t padded_key[24] = {0};
  uint8_t padded_input[24] = {0};
  uint8_t padded_output[24] = {0};
  aes_round_keys_t* round_keys;

  memcpy(padded_key, key, 20);
  memcpy(padded_input, input, 20);
  round_keys = NULL;
  for (size_t i = 0; i != SYDO_REF_PRG_CACHE_SIZE; ++i) {
    if (prg_cache[i].valid && prg_cache[i].secpar_bits == 160u &&
        memcmp(prg_cache[i].key, key, 20u) == 0) {
      round_keys = &prg_cache[i].round_keys;
      prg_cache[i].age = ++prg_cache_age;
      break;
    }
  }
  if (!round_keys) {
    size_t victim = 0;
    for (size_t i = 1; i != SYDO_REF_PRG_CACHE_SIZE; ++i) {
      if (!prg_cache[i].valid || prg_cache[i].age < prg_cache[victim].age) {
        victim = i;
      }
    }
    memset(&prg_cache[victim], 0, sizeof(prg_cache[victim]));
    prg_cache[victim].valid = 1;
    prg_cache[victim].secpar_bits = 160u;
    prg_cache[victim].age = ++prg_cache_age;
    memcpy(prg_cache[victim].key, key, 20u);
    rijndael192_init_round_keys(&prg_cache[victim].round_keys, padded_key);
    round_keys = &prg_cache[victim].round_keys;
  }
  rijndael192_encrypt_block(round_keys, padded_input, padded_output);
  memcpy(out, padded_output, 20);

  sydo_explicit_bzero(padded_key, sizeof(padded_key));
  sydo_explicit_bzero(padded_input, sizeof(padded_input));
  sydo_explicit_bzero(padded_output, sizeof(padded_output));
  return true;
}

static bool rijndael256_eval(uint8_t* out, const uint8_t* key, const uint8_t* input) {
  aes_round_keys_t* round_keys = NULL;
  for (size_t i = 0; i != SYDO_REF_PRG_CACHE_SIZE; ++i) {
    if (prg_cache[i].valid && prg_cache[i].secpar_bits == 256u &&
        memcmp(prg_cache[i].key, key, 32u) == 0) {
      round_keys = &prg_cache[i].round_keys;
      prg_cache[i].age = ++prg_cache_age;
      break;
    }
  }
  if (!round_keys) {
    size_t victim = 0;
    for (size_t i = 1; i != SYDO_REF_PRG_CACHE_SIZE; ++i) {
      if (!prg_cache[i].valid || prg_cache[i].age < prg_cache[victim].age) {
        victim = i;
      }
    }
    memset(&prg_cache[victim], 0, sizeof(prg_cache[victim]));
    prg_cache[victim].valid = 1;
    prg_cache[victim].secpar_bits = 256u;
    prg_cache[victim].age = ++prg_cache_age;
    memcpy(prg_cache[victim].key, key, 32u);
    rijndael256_init_round_keys(&prg_cache[victim].round_keys, key);
    round_keys = &prg_cache[victim].round_keys;
  }
  rijndael256_encrypt_block(round_keys, input, out);
  return true;
}

static uint32_t blake2s_bc_rotr32(uint32_t x, unsigned int n) {
  return (x >> n) | (x << (32u - n));
}

static void blake2s_bc_g(uint32_t* a, uint32_t* b, uint32_t* c, uint32_t* d, uint32_t x,
                         uint32_t y) {
  *a = *a + *b + x;
  *d = blake2s_bc_rotr32(*d ^ *a, 16u);
  *c = *c + *d;
  *b = blake2s_bc_rotr32(*b ^ *c, 12u);
  *a = *a + *b + y;
  *d = blake2s_bc_rotr32(*d ^ *a, 8u);
  *c = *c + *d;
  *b = blake2s_bc_rotr32(*b ^ *c, 7u);
}

static void blake2s_bc_round(uint32_t v[16], const uint32_t m[16], const uint8_t sigma[16]) {
  blake2s_bc_g(&v[0], &v[4], &v[8], &v[12], m[sigma[0]], m[sigma[1]]);
  blake2s_bc_g(&v[1], &v[5], &v[9], &v[13], m[sigma[2]], m[sigma[3]]);
  blake2s_bc_g(&v[2], &v[6], &v[10], &v[14], m[sigma[4]], m[sigma[5]]);
  blake2s_bc_g(&v[3], &v[7], &v[11], &v[15], m[sigma[6]], m[sigma[7]]);
  blake2s_bc_g(&v[0], &v[5], &v[10], &v[15], m[sigma[8]], m[sigma[9]]);
  blake2s_bc_g(&v[1], &v[6], &v[11], &v[12], m[sigma[10]], m[sigma[11]]);
  blake2s_bc_g(&v[2], &v[7], &v[8], &v[13], m[sigma[12]], m[sigma[13]]);
  blake2s_bc_g(&v[3], &v[4], &v[9], &v[14], m[sigma[14]], m[sigma[15]]);
}

bool sydo_ref_blake2s_512_bc_eval(uint8_t* out, const uint8_t* key, const uint8_t* input) {
  static const uint8_t sigma[10][16] = {
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
      {14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3},
      {11, 8, 12, 0, 5, 2, 15, 13, 10, 14, 3, 6, 7, 1, 9, 4},
      {7, 9, 3, 1, 13, 12, 11, 14, 2, 6, 5, 10, 4, 0, 15, 8},
      {9, 0, 5, 7, 2, 4, 10, 15, 14, 1, 11, 12, 6, 8, 3, 13},
      {2, 12, 6, 10, 0, 11, 8, 3, 4, 13, 7, 5, 15, 14, 1, 9},
      {12, 5, 1, 15, 14, 13, 4, 10, 0, 7, 6, 3, 9, 2, 8, 11},
      {13, 11, 7, 14, 12, 1, 3, 9, 5, 0, 15, 4, 8, 6, 2, 10},
      {6, 15, 14, 9, 11, 3, 0, 8, 12, 2, 13, 7, 1, 4, 10, 5},
      {10, 2, 8, 4, 7, 6, 1, 5, 15, 11, 9, 14, 3, 12, 13, 0},
  };
  uint32_t m[16];
  uint32_t v[16];

  for (size_t i = 0; i != 16u; ++i) {
    m[i] = load32_le(key + 4u * i);
    v[i] = load32_le(input + 4u * i);
  }
  for (size_t r = 0; r != 10u; ++r) {
    blake2s_bc_round(v, m, sigma[r]);
  }
  for (size_t i = 0; i != 16u; ++i) {
    store32_le(out + 4u * i, v[i]);
  }

  sydo_explicit_bzero(m, sizeof(m));
  sydo_explicit_bzero(v, sizeof(v));
  return true;
}

bool sydo_ref_tree_prg_eval(const sydo_ref_paramset_t* params, uint8_t* out,
                            const uint8_t* key, const uint8_t* iv, uint32_t tweak,
                            uint32_t counter) {
  uint8_t input[64];
  const size_t lambda_bytes = sydo_ref_secpar_bytes(params);
  bool ok = false;

  if (lambda_bytes > sizeof(input)) {
    return false;
  }

  make_ctr_input(input, iv, lambda_bytes, tweak, counter);
  if (params->secpar_bits == 160) {
    ok = rijndael192_trunc160_eval(out, key, input);
  } else if (params->secpar_bits == 256) {
    ok = rijndael256_eval(out, key, input);
  } else if (params->secpar_bits == 512) {
    ok = sydo_ref_blake2s_512_bc_eval(out, key, input);
  }

  sydo_explicit_bzero(input, sizeof(input));
  return ok;
}

bool sydo_ref_tree_prg_expand_2(const sydo_ref_paramset_t* params, uint8_t* out,
                                const uint8_t* key, const uint8_t* iv, uint32_t tweak) {
  const size_t lambda_bytes = sydo_ref_secpar_bytes(params);
  uint8_t input0[64];
  uint8_t input1[64];
  if (lambda_bytes > sizeof(input0)) {
    return false;
  }

  make_ctr_input(input0, iv, lambda_bytes, tweak, 0);
  make_ctr_input(input1, iv, lambda_bytes, tweak, 1);
  if (params->secpar_bits == 160) {
    uint8_t padded_key[24] = {0};
    uint8_t padded_input0[24] = {0};
    uint8_t padded_input1[24] = {0};
    uint8_t padded_output0[24] = {0};
    uint8_t padded_output1[24] = {0};
    memcpy(padded_key, key, 20u);
    memcpy(padded_input0, input0, 20u);
    memcpy(padded_input1, input1, 20u);
    rijndael192_encrypt_2_blocks_from_key(padded_key, padded_input0, padded_input1,
                                          padded_output0, padded_output1);
    memcpy(out, padded_output0, 20u);
    memcpy(out + lambda_bytes, padded_output1, 20u);
    sydo_explicit_bzero(padded_key, sizeof(padded_key));
    sydo_explicit_bzero(padded_input0, sizeof(padded_input0));
    sydo_explicit_bzero(padded_input1, sizeof(padded_input1));
    sydo_explicit_bzero(padded_output0, sizeof(padded_output0));
    sydo_explicit_bzero(padded_output1, sizeof(padded_output1));
  } else if (params->secpar_bits == 256) {
    rijndael256_encrypt_2_blocks_from_key(key, input0, input1, out, out + lambda_bytes);
  } else if (params->secpar_bits == 512) {
    sydo_ref_blake2s_512_bc_eval(out, key, input0);
    sydo_ref_blake2s_512_bc_eval(out + lambda_bytes, key, input1);
  } else {
    if (!sydo_ref_tree_prg_eval(params, out, key, iv, tweak, 0)) {
      return false;
    }
    if (!sydo_ref_tree_prg_eval(params, out + lambda_bytes, key, iv, tweak, 1)) {
      return false;
    }
  }
  sydo_explicit_bzero(input0, sizeof(input0));
  sydo_explicit_bzero(input1, sizeof(input1));
  return true;
}

bool sydo_ref_leaf_hash_prg(const sydo_ref_paramset_t* params, uint8_t* leaf_seed,
                            uint8_t* leaf_hash, const uint8_t* key, const uint8_t* iv,
                            uint32_t tweak) {
  uint8_t out[128];
  const size_t lambda_bytes = sydo_ref_secpar_bytes(params);
  const size_t hash_bytes = 2u * lambda_bytes;

  if (2u * lambda_bytes > sizeof(out)) {
    return false;
  }
  if (!sydo_ref_tree_prg_expand_2(params, out, key, iv, tweak)) {
    return false;
  }
  memcpy(leaf_seed, key, lambda_bytes);
  memcpy(leaf_hash, out, hash_bytes);
  sydo_explicit_bzero(out, sizeof(out));
  return true;
}

void sydo_ref_prg_cache_clear(void) {
  sydo_explicit_bzero(prg_cache, sizeof(prg_cache));
  prg_cache_age = 1;
}
