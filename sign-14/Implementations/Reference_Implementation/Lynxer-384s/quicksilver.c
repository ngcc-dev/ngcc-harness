/*
 *  SPDX-License-Identifier: MIT
 */

#include "quicksilver.h"
#include "lynx_matrices.h"
#include "lynx_frob_consts.h"

#include "fields.h"
#include "universal_hashing.h"
#include "utils.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define LYNX_128_CSP 128
#define LYNX_160_CSP 160
#define LYNX_192_CSP 192
#define LYNX_256_CSP 256
#define LYNX_384_CSP 384
#define LYNX_512_CSP 512

static void* lynx_aligned_alloc(size_t alignment, size_t size) {
  if ((alignment & (alignment - 1)) != 0) {
    return NULL;
  }
  if (alignment < sizeof(void*)) {
    alignment = sizeof(void*);
  }

  void* raw = malloc(size + alignment - 1 + sizeof(void*));
  if (!raw) {
    return NULL;
  }

  uintptr_t start   = (uintptr_t)raw + sizeof(void*);
  uintptr_t aligned = (start + alignment - 1) & ~(uintptr_t)(alignment - 1);
  ((void**)aligned)[-1] = raw;
  return (void*)aligned;
}

static void lynx_aligned_free(void* ptr) {
  if (ptr) {
    free(((void**)ptr)[-1]);
  }
}

#define PAD_TO(s, a) (((s) + (a) - 1) & ~((a) - 1))
#define BF128_ALLOC(s)                                                                            \
  lynx_aligned_alloc(BF128_ALIGN, PAD_TO((s) * sizeof(bf128_t), BF128_ALIGN))
#define BF160_ALLOC(s)                                                                            \
  lynx_aligned_alloc(BF160_ALIGN, PAD_TO((s) * sizeof(bf160_t), BF160_ALIGN))
#define BF192_ALLOC(s)                                                                            \
  lynx_aligned_alloc(BF192_ALIGN, PAD_TO((s) * sizeof(bf192_t), BF192_ALIGN))
#define BF256_ALLOC(s)                                                                            \
  lynx_aligned_alloc(BF256_ALIGN, PAD_TO((s) * sizeof(bf256_t), BF256_ALIGN))
#define BF384_ALLOC(s)                                                                            \
  lynx_aligned_alloc(BF384_ALIGN, PAD_TO((s) * sizeof(bf384_t), BF384_ALIGN))
#define BF512_ALLOC(s)                                                                            \
  lynx_aligned_alloc(BF512_ALIGN, PAD_TO((s) * sizeof(bf512_t), BF512_ALIGN))

#define NGCC_NUM_CONSTRAINTS 3

static bf128_t* column_to_row_major_and_shrink_V_128(uint8_t** v, unsigned int ell) {
  bf128_t* new_v = BF128_ALLOC(ell + LYNX_128_CSP * 2);
  assert(new_v);
  for (unsigned int row = 0; row != ell + LYNX_128_CSP * 2; ++row) {
    uint8_t new_row[BF128_NUM_BYTES] = {0};
    for (unsigned int column = 0; column != LYNX_128_CSP; ++column) {
      ptr_set_bit(new_row, column, ptr_get_bit(v[column], row));
    }
    new_v[row] = bf128_load(new_row);
  }
  return new_v;
}

static void lynx_128_linear_map_with_auth(const uint64_t M[LYNX_128_CSP][2], const uint8_t* x_bits,
                                          const bf128_t* x_tag_bits, bf128_t* y, bf128_t* mac_y) {
  bf128_t y_tag_bits[LYNX_128_CSP];
  *y = bf128_zero();

  for (unsigned int row = 0; row < LYNX_128_CSP; ++row) {
    bf128_t row_tag = bf128_zero();
    uint8_t bit     = 0;
    for (unsigned int col = 0; col < LYNX_128_CSP; ++col) {
      const uint64_t mask_word = M[row][col / 64];
      const uint8_t mask_bit   = (mask_word >> (col % 64)) & 1u;
      if (mask_bit) {
        row_tag = bf128_add(row_tag, x_tag_bits[col]);
        bit ^= ptr_get_bit(x_bits, col);
      }
    }
    y_tag_bits[row] = row_tag;
    BF_VALUE(*y, row / 64) ^= ((uint64_t)bit << (row % 64));
  }

  *mac_y = bf128_sum_poly(y_tag_bits);
}

/*
 * First Lynx proof backend: full VOLE/hash plumbing with a no-op constraints core.
 * This intentionally only establishes the Lynx-128 prover/verifier integration path.
 */
static void lynx_128_constraints_prover(zk_hash_128_2_ctx* hasher, const uint8_t* w,
                                        const bf128_t* w_tag, const uint8_t* owf_in,
                                        const uint8_t* owf_out, const sig_paramset_t* params) {
  lynx_t mats;
  if (lynx_init(&mats, 128, owf_in, params->owf_input_size) != 0) {
    return;
  }
  const uint64_t (*L0)[2] = (const uint64_t (*)[2])mats.L0;
  const uint64_t (*L1)[2] = (const uint64_t (*)[2])mats.L1;
  const uint64_t (*L2)[2] = (const uint64_t (*)[2])mats.L2;
  const uint64_t (*L3)[2] = (const uint64_t (*)[2])mats.L3;
  (void)owf_out;
  bf128_t k = bf128_load(w);
  bf128_t mac_k = bf128_sum_poly(w_tag);
  bf128_t v1 = bf128_load(w + 16);
  bf128_t mac_v1 = bf128_sum_poly(w_tag + LYNX_128_CSP);
  bf128_t v2 = bf128_load(w + 32);
  bf128_t mac_v2 = bf128_sum_poly(w_tag + LYNX_128_CSP * 2);
  bf128_t c0 = bf128_load(mats.C0);
  bf128_t c1 = bf128_load(mats.C1);

  bf128_t a0, a1;


  bf128_t L0k, L3k, L3v1, L3v2, L1v1, L2v2;
  bf128_t mac_L0k, mac_L3k, mac_L3v1, mac_L3v2, mac_L1v1, mac_L2v2;
  lynx_128_linear_map_with_auth(L0, w, w_tag, &L0k, &mac_L0k);
  lynx_128_linear_map_with_auth(L3, w, w_tag, &L3k, &mac_L3k);
  lynx_128_linear_map_with_auth(L3, w + 16, w_tag + LYNX_128_CSP, &L3v1, &mac_L3v1);
  lynx_128_linear_map_with_auth(L3, w + 32, w_tag + LYNX_128_CSP * 2, &L3v2, &mac_L3v2);
  lynx_128_linear_map_with_auth(L1, w + 16, w_tag + LYNX_128_CSP, &L1v1, &mac_L1v1);
  lynx_128_linear_map_with_auth(L2, w + 32, w_tag + LYNX_128_CSP * 2, &L2v2, &mac_L2v2);

  // Constraint 0: v1 * (k + c0) = 1
  {
    bf128_t a1_c = bf128_add(bf128_mul(v1, mac_k), bf128_mul(k, mac_v1));
    a1 = bf128_add(a1_c, bf128_mul(c0, mac_v1));
    a0 = bf128_mul(mac_v1, mac_k);
    zk_hash_128_2_update(hasher, a0, a1);
  }

  // Constraint 1: v2 * (L0(k) + c1) = 1
  {
    bf128_t a1_c = bf128_add(bf128_mul(v2, mac_L0k), bf128_mul(L0k, mac_v2));
    a1 = bf128_add(a1_c, bf128_mul(c1, mac_v2));
    a0 = bf128_mul(mac_v2, mac_L0k);
    zk_hash_128_2_update(hasher, a0, a1);
  }

  // Constraint 2: L1(v1) * L2(v2) + L3(k) + L3(v1) + L3(v2) = y
  {
    a1 = bf128_add(bf128_mul(L1v1, mac_L2v2), bf128_mul(L2v2, mac_L1v1));
    a1 = bf128_add(a1, bf128_add(mac_L3k, bf128_add(mac_L3v1, mac_L3v2)));
    a0 = bf128_mul(mac_L1v1, mac_L2v2);
    zk_hash_128_2_update(hasher, a0, a1);
  }
}

static void lynx_128_constraints_verifier(zk_hash_128_ctx* hasher, const bf128_t* w_key,
                                          const uint8_t* owf_in, const uint8_t* owf_out,
                                          bf128_t delta,
                                          const sig_paramset_t* params) {
  lynx_t mats;
  if (lynx_init(&mats, 128, owf_in, params->owf_input_size) != 0) {
    return;
  }
  const uint64_t (*L0)[2] = (const uint64_t (*)[2])mats.L0;
  const uint64_t (*L1)[2] = (const uint64_t (*)[2])mats.L1;
  const uint64_t (*L2)[2] = (const uint64_t (*)[2])mats.L2;
  const uint64_t (*L3)[2] = (const uint64_t (*)[2])mats.L3;
  bf128_t c0 = bf128_load(mats.C0);
  bf128_t c1 = bf128_load(mats.C1);
  bf128_t y = bf128_load(owf_out);
  bf128_t delta_sq = bf128_mul(delta, delta);
  bf128_t key_k = bf128_sum_poly(w_key);
  bf128_t key_v1 = bf128_sum_poly(w_key + LYNX_128_CSP);
  bf128_t key_v2 = bf128_sum_poly(w_key + LYNX_128_CSP * 2);
  bf128_t zero = bf128_zero();
  bf128_t temp = bf128_zero();
  bf128_t b;


  bf128_t key_L0k, key_L1v1, key_L2v2, key_L3k, key_L3v1, key_L3v2;
  lynx_128_linear_map_with_auth(L0, (const uint8_t*) &zero, w_key, &temp, &key_L0k);
  lynx_128_linear_map_with_auth(L1, (const uint8_t*) &zero, w_key + LYNX_128_CSP, &temp, &key_L1v1);
  lynx_128_linear_map_with_auth(L2, (const uint8_t*) &zero, w_key + LYNX_128_CSP * 2, &temp, &key_L2v2);
  lynx_128_linear_map_with_auth(L3, (const uint8_t*) &zero, w_key, &temp, &key_L3k);
  lynx_128_linear_map_with_auth(L3, (const uint8_t*) &zero, w_key + LYNX_128_CSP, &temp, &key_L3v1);
  lynx_128_linear_map_with_auth(L3, (const uint8_t*) &zero, w_key + LYNX_128_CSP * 2, &temp, &key_L3v2);

  // Constraint 0: v1 * (k + c0) = 1
  {
    bf128_t b_c = bf128_mul(key_k, key_v1);
    b_c = bf128_add(b_c, bf128_mul(delta, bf128_mul(c0, key_v1)));
    b = bf128_add(b_c, delta_sq);
    zk_hash_128_update(hasher, b);
  }

  // Constraint 1: v2 * (L0(k) + c1) = 1
  {
    bf128_t b_c = bf128_mul(key_L0k, key_v2);
    b_c = bf128_add(b_c, bf128_mul(delta, bf128_mul(c1, key_v2)));
    b = bf128_add(b_c, delta_sq);
    zk_hash_128_update(hasher, b);
  }

  // Constraint 2: L1(v1) * L2(v2) + L3(k) + L3(v1) + L3(v2) = y
  {
    b = bf128_mul(key_L1v1, key_L2v2);
    b = bf128_add(b, bf128_mul(delta, bf128_add(key_L3k, bf128_add(key_L3v1, key_L3v2))));
    b = bf128_add(b, bf128_mul(delta_sq, y));
    zk_hash_128_update(hasher, b);
  }
}

void lynx_128_prover(uint8_t* a0_tilde, uint8_t* a1_tilde, const uint8_t* w,
                     const uint8_t* u, uint8_t** V, const uint8_t* owf_in, const uint8_t* owf_out,
                     const uint8_t* chall_2, const sig_paramset_t* params) {
  const unsigned int ell = params->lenwit;

  bf128_t* w_tag = column_to_row_major_and_shrink_V_128(V, ell);

  bf128_t bf_u_star_0 = bf128_sum_poly_bits(u);
  bf128_t bf_v_star_0 = bf128_sum_poly(w_tag + ell);

  zk_hash_128_2_ctx hasher;
  zk_hash_128_2_init(&hasher, chall_2);
  lynx_128_constraints_prover(&hasher, w, w_tag, owf_in, owf_out, params);
  zk_hash_128_2_finalize(a0_tilde, a1_tilde, &hasher, bf_v_star_0, bf_u_star_0);
  lynx_aligned_free(w_tag);
}

void lynx_128_verifier(uint8_t* a0_tilde, const uint8_t* d, uint8_t** Q, const uint8_t* owf_in,
                       const uint8_t* owf_out, const uint8_t* chall_2, const uint8_t* chall_3,
                       const uint8_t* a1_tilde,
                       const sig_paramset_t* params) {
  const unsigned int ell = params->lenwit;

  bf128_t bf_delta = bf128_load(chall_3);

  bf128_t* q_key = column_to_row_major_and_shrink_V_128(Q, ell);
  bf128_t q_star = bf128_sum_poly(q_key + ell);

  for (unsigned int i = 0; i < ell; i++) {
    q_key[i] = bf128_add(q_key[i], bf128_mul_bit(bf_delta, ptr_get_bit(d, i)));
  }

  zk_hash_128_ctx b_ctx;
  zk_hash_128_init(&b_ctx, chall_2);
  lynx_128_constraints_verifier(&b_ctx, q_key, owf_in, owf_out, bf_delta, params);
  lynx_aligned_free(q_key);

  uint8_t q_tilde[LYNX_128_CSP / 8];
  zk_hash_128_finalize(q_tilde, &b_ctx, q_star);

  bf128_t tmp1 = bf128_mul(bf128_load(a1_tilde), bf_delta);
  bf128_t ret  = bf128_add(bf128_load(q_tilde), tmp1);

  bf128_store(a0_tilde, ret);
}

static bf160_t* column_to_row_major_and_shrink_V_160(uint8_t** v, unsigned int ell) {
  bf160_t* new_v = BF160_ALLOC(ell + LYNX_160_CSP * 2);
  assert(new_v);
  for (unsigned int row = 0; row != ell + LYNX_160_CSP * 2; ++row) {
    uint8_t new_row[BF160_NUM_BYTES] = {0};
    for (unsigned int column = 0; column != LYNX_160_CSP; ++column) {
      ptr_set_bit(new_row, column, ptr_get_bit(v[column], row));
    }
    new_v[row] = bf160_load(new_row);
  }
  return new_v;
}

static void lynx_160_linear_map_with_auth(const uint64_t M[LYNX_160_CSP][3], const uint8_t* x_bits,
                                          const bf160_t* x_tag_bits, bf160_t* y, bf160_t* mac_y) {
  bf160_t y_tag_bits[LYNX_160_CSP];
  *y = bf160_zero();

  for (unsigned int row = 0; row < LYNX_160_CSP; ++row) {
    bf160_t row_tag = bf160_zero();
    uint8_t bit     = 0;
    for (unsigned int col = 0; col < LYNX_160_CSP; ++col) {
      const uint64_t mask_word = M[row][col / 64];
      const uint8_t mask_bit   = (mask_word >> (col % 64)) & 1u;
      if (mask_bit) {
        row_tag = bf160_add(row_tag, x_tag_bits[col]);
        bit ^= ptr_get_bit(x_bits, col);
      }
    }
    y_tag_bits[row] = row_tag;
    BF_VALUE(*y, row / 64) ^= ((uint64_t)bit << (row % 64));
  }

  *mac_y = bf160_sum_poly(y_tag_bits);
}

static void lynx_160_linear_map_tag_bits(const uint64_t M[LYNX_160_CSP][3],
                                         const bf160_t* x_tag_bits, bf160_t* y_tag_bits) {
  for (unsigned int row = 0; row < LYNX_160_CSP; ++row) {
    bf160_t row_tag = bf160_zero();
    for (unsigned int col = 0; col < LYNX_160_CSP; ++col) {
      const uint64_t mask_word = M[row][col / 64];
      const uint8_t mask_bit   = (mask_word >> (col % 64)) & 1u;
      if (mask_bit) {
        row_tag = bf160_add(row_tag, x_tag_bits[col]);
      }
    }
    y_tag_bits[row] = row_tag;
  }
}

static void lynx_160_constraints_prover(zk_hash_160_2_ctx* hasher, const uint8_t* w,
                                        const bf160_t* w_tag, const uint8_t* owf_in,
                                        const uint8_t* owf_out, const sig_paramset_t* params) {
  lynx_t mats;
  if (lynx_init(&mats, 160, owf_in, params->owf_input_size) != 0) {
    return;
  }
  const uint64_t (*L0)[3] = (const uint64_t (*)[3])mats.L0;
  const uint64_t (*L1)[3] = (const uint64_t (*)[3])mats.L1;
  const uint64_t (*L2)[3] = (const uint64_t (*)[3])mats.L2;
  const uint64_t (*L3)[3] = (const uint64_t (*)[3])mats.L3;
  bf160_t k = bf160_load(w);
  bf160_t mac_k = bf160_sum_poly(w_tag);
  bf160_t v1 = bf160_load(w + 20);
  bf160_t mac_v1 = bf160_sum_poly(w_tag + LYNX_160_CSP);
  bf160_t v2 = bf160_load(w + 40);
  bf160_t mac_v2 = bf160_sum_poly(w_tag + LYNX_160_CSP * 2);
  bf160_t c0 = bf160_load(mats.C0);
  bf160_t y = bf160_load(owf_out);

  bf160_t a0, a1;


  bf160_t k_plus_c0 = bf160_add(k, c0);

  bf160_t L0k, L3k, L3v1, L3v2, L1v1, L2v2;
  bf160_t mac_L0k, mac_L3k, mac_L3v1, mac_L3v2, mac_L1v1, mac_L2v2;
  bf160_t L0k_tag_bits[LYNX_160_CSP];
  lynx_160_linear_map_with_auth(L0, w, w_tag, &L0k, &mac_L0k);
  lynx_160_linear_map_tag_bits(L0, w_tag, L0k_tag_bits);
  lynx_160_linear_map_with_auth(L3, w, w_tag, &L3k, &mac_L3k);
  lynx_160_linear_map_with_auth(L3, w + 20, w_tag + LYNX_160_CSP, &L3v1, &mac_L3v1);
  lynx_160_linear_map_with_auth(L3, w + 40, w_tag + LYNX_160_CSP * 2, &L3v2, &mac_L3v2);
  lynx_160_linear_map_with_auth(L1, w + 20, w_tag + LYNX_160_CSP, &L1v1, &mac_L1v1);
  lynx_160_linear_map_with_auth(L2, w + 40, w_tag + LYNX_160_CSP * 2, &L2v2, &mac_L2v2);

  bf160_t L0k_plus_c0 = bf160_add(L0k, c0);
  bf160_t L0k_plus_c0_frob67, mac_L0k_plus_c0_frob67;
  lynx_160_linear_map_with_auth(frob160_67, (const uint8_t*) &L0k_plus_c0, L0k_tag_bits,
                                &L0k_plus_c0_frob67, &mac_L0k_plus_c0_frob67);

  // Constraint 0: v1 * (k + c0) = 1
  {
    a1 = bf160_add(bf160_mul(k_plus_c0, mac_v1), bf160_mul(v1, mac_k));
    a0 = bf160_mul(mac_k, mac_v1);
    zk_hash_160_2_update(hasher, a0, a1);
  }

  // Constraint 1: (L0(k) + c0)^(2^67) = (L0(k) + c0) * v2
  {
    bf160_t a1_c = bf160_add(bf160_mul(L0k_plus_c0, mac_v2), bf160_mul(v2, mac_L0k));
    a1 = bf160_add(a1_c, mac_L0k_plus_c0_frob67);
    a0 = bf160_mul(mac_L0k, mac_v2);
    zk_hash_160_2_update(hasher, a0, a1);
  }

  // Constraint 2: L1(v1) * L2(v2) + L3(k) + L3(v1) + L3(v2) = y
  {
    a1 = bf160_add(bf160_mul(L1v1, mac_L2v2), bf160_mul(L2v2, mac_L1v1));
    a1 = bf160_add(a1, bf160_add(mac_L3k, bf160_add(mac_L3v1, mac_L3v2)));
    a0 = bf160_mul(mac_L1v1, mac_L2v2);
    zk_hash_160_2_update(hasher, a0, a1);
  }
  (void)y;
}

static void lynx_160_constraints_verifier(zk_hash_160_ctx* hasher, const bf160_t* w_key,
                                          const uint8_t* owf_in, const uint8_t* owf_out,
                                          bf160_t delta,
                                          const sig_paramset_t* params) {
  lynx_t mats;
  if (lynx_init(&mats, 160, owf_in, params->owf_input_size) != 0) {
    return;
  }
  const uint64_t (*L0)[3] = (const uint64_t (*)[3])mats.L0;
  const uint64_t (*L1)[3] = (const uint64_t (*)[3])mats.L1;
  const uint64_t (*L2)[3] = (const uint64_t (*)[3])mats.L2;
  const uint64_t (*L3)[3] = (const uint64_t (*)[3])mats.L3;
  bf160_t c0 = bf160_load(mats.C0);
  bf160_t y = bf160_load(owf_out);
  bf160_t delta_sq = bf160_mul(delta, delta);
  bf160_t key_k = bf160_sum_poly(w_key);
  bf160_t key_v1 = bf160_sum_poly(w_key + LYNX_160_CSP);
  bf160_t key_v2 = bf160_sum_poly(w_key + LYNX_160_CSP * 2);
  bf160_t zero = bf160_zero();
  bf160_t temp = bf160_zero();
  bf160_t b;


  bf160_t key_L0k, key_L0k_frob67;
  bf160_t key_L1v1, key_L2v2, key_L3k, key_L3v1, key_L3v2;
  bf160_t L0k_tag_keys[LYNX_160_CSP];
  lynx_160_linear_map_with_auth(L0, (const uint8_t*) &zero, w_key, &temp, &key_L0k);
  lynx_160_linear_map_tag_bits(L0, w_key, L0k_tag_keys);
  lynx_160_linear_map_with_auth(frob160_67, (const uint8_t*) &zero, L0k_tag_keys, &temp,
                                &key_L0k_frob67);
  lynx_160_linear_map_with_auth(L1, (const uint8_t*) &zero, w_key + LYNX_160_CSP, &temp, &key_L1v1);
  lynx_160_linear_map_with_auth(L2, (const uint8_t*) &zero, w_key + LYNX_160_CSP * 2, &temp, &key_L2v2);
  lynx_160_linear_map_with_auth(L3, (const uint8_t*) &zero, w_key, &temp, &key_L3k);
  lynx_160_linear_map_with_auth(L3, (const uint8_t*) &zero, w_key + LYNX_160_CSP, &temp, &key_L3v1);
  lynx_160_linear_map_with_auth(L3, (const uint8_t*) &zero, w_key + LYNX_160_CSP * 2, &temp, &key_L3v2);

  // Constraint 0: v1 * (k + c0) = 1
  {
    bf160_t b_c = bf160_mul(key_k, key_v1);
    b_c = bf160_add(b_c, bf160_mul(delta, bf160_mul(c0, key_v1)));
    b = bf160_add(b_c, delta_sq);
    zk_hash_160_update(hasher, b);
  }

  // Constraint 1: (L0(k) + c0)^(2^67) = (L0(k) + c0) * v2
  {
    bf160_t c0_frob67 = c0;
    for (unsigned int i = 0; i < 67; ++i) {
      c0_frob67 = bf160_mul(c0_frob67, c0_frob67);
    }
    bf160_t b_c = bf160_mul(key_v2, bf160_add(key_L0k, bf160_mul(delta, c0)));
    b_c = bf160_add(b_c, bf160_mul(delta, key_L0k_frob67));
    b = bf160_add(b_c, bf160_mul(delta_sq, c0_frob67));
    zk_hash_160_update(hasher, b);
  }

  // Constraint 2: L1(v1) * L2(v2) + L3(k) + L3(v1) + L3(v2) = y
  {
    b = bf160_mul(key_L1v1, key_L2v2);
    b = bf160_add(b, bf160_mul(delta, bf160_add(key_L3k, bf160_add(key_L3v1, key_L3v2))));
    b = bf160_add(b, bf160_mul(delta_sq, y));
    zk_hash_160_update(hasher, b);
  }
}

void lynx_160_prover(uint8_t* a0_tilde, uint8_t* a1_tilde, const uint8_t* w,
                     const uint8_t* u, uint8_t** V, const uint8_t* owf_in, const uint8_t* owf_out,
                     const uint8_t* chall_2, const sig_paramset_t* params) {
  const unsigned int ell = params->lenwit;

  bf160_t* w_tag = column_to_row_major_and_shrink_V_160(V, ell);

  bf160_t bf_u_star_0 = bf160_sum_poly_bits(u);
  bf160_t bf_v_star_0 = bf160_sum_poly(w_tag + ell);

  zk_hash_160_2_ctx hasher;
  zk_hash_160_2_init(&hasher, chall_2);
  lynx_160_constraints_prover(&hasher, w, w_tag, owf_in, owf_out, params);
  zk_hash_160_2_finalize(a0_tilde, a1_tilde, &hasher, bf_v_star_0, bf_u_star_0);
  lynx_aligned_free(w_tag);
}

void lynx_160_verifier(uint8_t* a0_tilde, const uint8_t* d, uint8_t** Q, const uint8_t* owf_in,
                       const uint8_t* owf_out, const uint8_t* chall_2, const uint8_t* chall_3,
                       const uint8_t* a1_tilde,
                       const sig_paramset_t* params) {
  const unsigned int ell = params->lenwit;

  bf160_t bf_delta = bf160_load(chall_3);

  bf160_t* q_key = column_to_row_major_and_shrink_V_160(Q, ell);
  bf160_t q_star = bf160_sum_poly(q_key + ell);

  for (unsigned int i = 0; i < ell; i++) {
    q_key[i] = bf160_add(q_key[i], bf160_mul_bit(bf_delta, ptr_get_bit(d, i)));
  }

  zk_hash_160_ctx b_ctx;
  zk_hash_160_init(&b_ctx, chall_2);
  lynx_160_constraints_verifier(&b_ctx, q_key, owf_in, owf_out, bf_delta, params);
  lynx_aligned_free(q_key);

  uint8_t q_tilde[LYNX_160_CSP / 8];
  zk_hash_160_finalize(q_tilde, &b_ctx, q_star);

  bf160_t tmp1 = bf160_mul(bf160_load(a1_tilde), bf_delta);
  bf160_t ret  = bf160_add(bf160_load(q_tilde), tmp1);
  bf160_store(a0_tilde, ret);
}

static bf192_t* column_to_row_major_and_shrink_V_192(uint8_t** v, unsigned int ell) {
  bf192_t* new_v = BF192_ALLOC(ell + LYNX_192_CSP * 2);
  assert(new_v);
  for (unsigned int row = 0; row != ell + LYNX_192_CSP * 2; ++row) {
    uint8_t new_row[BF192_NUM_BYTES] = {0};
    for (unsigned int column = 0; column != LYNX_192_CSP; ++column) {
      ptr_set_bit(new_row, column, ptr_get_bit(v[column], row));
    }
    new_v[row] = bf192_load(new_row);
  }
  return new_v;
}

static void lynx_192_linear_map_with_auth(const uint64_t M[LYNX_192_CSP][3], const uint8_t* x_bits,
                                          const bf192_t* x_tag_bits, bf192_t* y, bf192_t* mac_y) {
  bf192_t y_tag_bits[LYNX_192_CSP];
  *y = bf192_zero();

  for (unsigned int row = 0; row < LYNX_192_CSP; ++row) {
    bf192_t row_tag = bf192_zero();
    uint8_t bit     = 0;
    for (unsigned int col = 0; col < LYNX_192_CSP; ++col) {
      const uint64_t mask_word = M[row][col / 64];
      const uint8_t mask_bit   = (mask_word >> (col % 64)) & 1u;
      if (mask_bit) {
        row_tag = bf192_add(row_tag, x_tag_bits[col]);
        bit ^= ptr_get_bit(x_bits, col);
      }
    }
    y_tag_bits[row] = row_tag;
    BF_VALUE(*y, row / 64) ^= ((uint64_t)bit << (row % 64));
  }

  *mac_y = bf192_sum_poly(y_tag_bits);
}

static void lynx_192_linear_map_tag_bits(const uint64_t M[LYNX_192_CSP][3],
                                         const bf192_t* x_tag_bits, bf192_t* y_tag_bits) {
  for (unsigned int row = 0; row < LYNX_192_CSP; ++row) {
    bf192_t row_tag = bf192_zero();
    for (unsigned int col = 0; col < LYNX_192_CSP; ++col) {
      const uint64_t mask_word = M[row][col / 64];
      const uint8_t mask_bit   = (mask_word >> (col % 64)) & 1u;
      if (mask_bit) {
        row_tag = bf192_add(row_tag, x_tag_bits[col]);
      }
    }
    y_tag_bits[row] = row_tag;
  }
}

static void lynx_192_constraints_prover(zk_hash_192_2_ctx* hasher, const uint8_t* w,
                                        const bf192_t* w_tag, const uint8_t* owf_in,
                                        const uint8_t* owf_out, const sig_paramset_t* params) {
  lynx_t mats;
  if (lynx_init(&mats, 192, owf_in, params->owf_input_size) != 0) {
    return;
  }
  const uint64_t (*L0)[3] = (const uint64_t (*)[3])mats.L0;
  const uint64_t (*L1)[3] = (const uint64_t (*)[3])mats.L1;
  const uint64_t (*L2)[3] = (const uint64_t (*)[3])mats.L2;
  const uint64_t (*L3)[3] = (const uint64_t (*)[3])mats.L3;
  bf192_t k = bf192_load(w);
  bf192_t mac_k = bf192_sum_poly(w_tag);
  bf192_t v1 = bf192_load(w + 24);
  bf192_t mac_v1 = bf192_sum_poly(w_tag + LYNX_192_CSP);
  bf192_t v2 = bf192_load(w + 48);
  bf192_t mac_v2 = bf192_sum_poly(w_tag + LYNX_192_CSP * 2);
  bf192_t c0 = bf192_load(mats.C0);
  bf192_t c1 = bf192_load(mats.C1);
  bf192_t y = bf192_load(owf_out);

  bf192_t a0, a1;


  bf192_t k_plus_c0 = bf192_add(k, c0);
  bf192_t v1_frob13, mac_v1_frob13;
  lynx_192_linear_map_with_auth(frob192_13, (const uint8_t*) &v1, w_tag + LYNX_192_CSP,
                                &v1_frob13, &mac_v1_frob13);

  bf192_t L0k, L3k, L3v1, L3v2, L1v1, L2v2;
  bf192_t mac_L0k, mac_L3k, mac_L3v1, mac_L3v2, mac_L1v1, mac_L2v2;
  bf192_t L0k_tag_bits[LYNX_192_CSP];
  lynx_192_linear_map_with_auth(L0, w, w_tag, &L0k, &mac_L0k);
  lynx_192_linear_map_tag_bits(L0, w_tag, L0k_tag_bits);
  lynx_192_linear_map_with_auth(L3, w, w_tag, &L3k, &mac_L3k);
  lynx_192_linear_map_with_auth(L3, w + 24, w_tag + LYNX_192_CSP, &L3v1, &mac_L3v1);
  lynx_192_linear_map_with_auth(L3, w + 48, w_tag + LYNX_192_CSP * 2, &L3v2, &mac_L3v2);
  lynx_192_linear_map_with_auth(L1, w + 24, w_tag + LYNX_192_CSP, &L1v1, &mac_L1v1);
  lynx_192_linear_map_with_auth(L2, w + 48, w_tag + LYNX_192_CSP * 2, &L2v2, &mac_L2v2);

  bf192_t L0k_plus_c1 = bf192_add(L0k, c1);
  bf192_t L0k_plus_c1_frob59, mac_L0k_plus_c1_frob59;
  lynx_192_linear_map_with_auth(frob192_59, (const uint8_t*) &L0k_plus_c1, L0k_tag_bits,
                                &L0k_plus_c1_frob59, &mac_L0k_plus_c1_frob59);

  // Constraint 0: v1 * (k + c0) = v1^(2^13)
  {
    a1 = bf192_add(bf192_mul(k_plus_c0, mac_v1), bf192_mul(v1, mac_k));
    a1 = bf192_add(a1, mac_v1_frob13);
    a0 = bf192_mul(mac_k, mac_v1);
    zk_hash_192_2_update(hasher, a0, a1);
  }

  // Constraint 1: (L0(k) + c1) * v2 = (L0(k) + c1)^(2^59)
  {
    a1 = bf192_add(bf192_mul(L0k_plus_c1, mac_v2), bf192_mul(v2, mac_L0k));
    a1 = bf192_add(a1, mac_L0k_plus_c1_frob59);
    a0 = bf192_mul(mac_L0k, mac_v2);
    zk_hash_192_2_update(hasher, a0, a1);
  }

  // Constraint 2: L1(v1) * L2(v2) + L3(k) + L3(v1) + L3(v2) = y
  {
    a1 = bf192_add(bf192_mul(L1v1, mac_L2v2), bf192_mul(L2v2, mac_L1v1));
    a1 = bf192_add(a1, bf192_add(mac_L3k, bf192_add(mac_L3v1, mac_L3v2)));
    a0 = bf192_mul(mac_L1v1, mac_L2v2);
    zk_hash_192_2_update(hasher, a0, a1);
  }
  (void)y;
}

static void lynx_192_constraints_verifier(zk_hash_192_ctx* hasher, const bf192_t* w_key,
                                          const uint8_t* owf_in, const uint8_t* owf_out,
                                          bf192_t delta,
                                          const sig_paramset_t* params) {
  lynx_t mats;
  if (lynx_init(&mats, 192, owf_in, params->owf_input_size) != 0) {
    return;
  }
  const uint64_t (*L0)[3] = (const uint64_t (*)[3])mats.L0;
  const uint64_t (*L1)[3] = (const uint64_t (*)[3])mats.L1;
  const uint64_t (*L2)[3] = (const uint64_t (*)[3])mats.L2;
  const uint64_t (*L3)[3] = (const uint64_t (*)[3])mats.L3;
  bf192_t c0 = bf192_load(mats.C0);
  bf192_t c1 = bf192_load(mats.C1);
  bf192_t y = bf192_load(owf_out);
  bf192_t delta_sq = bf192_mul(delta, delta);
  bf192_t key_k = bf192_sum_poly(w_key);
  bf192_t key_v1 = bf192_sum_poly(w_key + LYNX_192_CSP);
  bf192_t key_v2 = bf192_sum_poly(w_key + LYNX_192_CSP * 2);
  bf192_t zero = bf192_zero();
  bf192_t temp = bf192_zero();
  bf192_t b;


  bf192_t key_v1_frob13, key_L0k, key_L0k_frob59;
  bf192_t key_L1v1, key_L2v2, key_L3k, key_L3v1, key_L3v2;
  bf192_t c1_frob59 = c1;
  bf192_t L0k_tag_keys[LYNX_192_CSP];
  for (unsigned int i = 0; i < 59; ++i) {
    c1_frob59 = bf192_mul(c1_frob59, c1_frob59);
  }
  lynx_192_linear_map_with_auth(frob192_13, (const uint8_t*) &zero, w_key + LYNX_192_CSP, &temp,
                                &key_v1_frob13);
  lynx_192_linear_map_with_auth(L0, (const uint8_t*) &zero, w_key, &temp, &key_L0k);
  lynx_192_linear_map_tag_bits(L0, w_key, L0k_tag_keys);
  lynx_192_linear_map_with_auth(frob192_59, (const uint8_t*) &zero, L0k_tag_keys, &temp,
                                &key_L0k_frob59);
  lynx_192_linear_map_with_auth(L1, (const uint8_t*) &zero, w_key + LYNX_192_CSP, &temp, &key_L1v1);
  lynx_192_linear_map_with_auth(L2, (const uint8_t*) &zero, w_key + LYNX_192_CSP * 2, &temp, &key_L2v2);
  lynx_192_linear_map_with_auth(L3, (const uint8_t*) &zero, w_key, &temp, &key_L3k);
  lynx_192_linear_map_with_auth(L3, (const uint8_t*) &zero, w_key + LYNX_192_CSP, &temp, &key_L3v1);
  lynx_192_linear_map_with_auth(L3, (const uint8_t*) &zero, w_key + LYNX_192_CSP * 2, &temp, &key_L3v2);

  // Constraint 0: v1 * (k + c0) = v1^(2^13)
  {
    bf192_t b_c = bf192_mul(bf192_add(key_k, bf192_mul(delta, c0)), key_v1);
    b = bf192_add(b_c, bf192_mul(delta, key_v1_frob13));
    zk_hash_192_update(hasher, b);
  }

  // Constraint 1: (L0(k) + c1) * v2 = (L0(k) + c1)^(2^59)
  {
    bf192_t b_c = bf192_mul(key_v2, bf192_add(key_L0k, bf192_mul(delta, c1)));
    b_c = bf192_add(b_c, bf192_mul(delta, key_L0k_frob59));
    b = bf192_add(b_c, bf192_mul(delta_sq, c1_frob59));
    zk_hash_192_update(hasher, b);
  }

  // Constraint 2: L1(v1) * L2(v2) + L3(k) + L3(v1) + L3(v2) = y
  {
    b = bf192_mul(key_L1v1, key_L2v2);
    b = bf192_add(b, bf192_mul(delta, bf192_add(key_L3k, bf192_add(key_L3v1, key_L3v2))));
    b = bf192_add(b, bf192_mul(delta_sq, y));
    zk_hash_192_update(hasher, b);
  }
}

void lynx_192_prover(uint8_t* a0_tilde, uint8_t* a1_tilde, const uint8_t* w,
                     const uint8_t* u, uint8_t** V, const uint8_t* owf_in, const uint8_t* owf_out,
                     const uint8_t* chall_2, const sig_paramset_t* params) {
  const unsigned int ell = params->lenwit;

  bf192_t* w_tag = column_to_row_major_and_shrink_V_192(V, ell);

  bf192_t bf_u_star_0 = bf192_sum_poly_bits(u);
  bf192_t bf_v_star_0 = bf192_sum_poly(w_tag + ell);

  zk_hash_192_2_ctx hasher;
  zk_hash_192_2_init(&hasher, chall_2);
  lynx_192_constraints_prover(&hasher, w, w_tag, owf_in, owf_out, params);
  zk_hash_192_2_finalize(a0_tilde, a1_tilde, &hasher, bf_v_star_0, bf_u_star_0);
  lynx_aligned_free(w_tag);
}

void lynx_192_verifier(uint8_t* a0_tilde, const uint8_t* d, uint8_t** Q, const uint8_t* owf_in,
                       const uint8_t* owf_out, const uint8_t* chall_2, const uint8_t* chall_3,
                       const uint8_t* a1_tilde,
                       const sig_paramset_t* params) {
  const unsigned int ell = params->lenwit;

  bf192_t bf_delta = bf192_load(chall_3);

  bf192_t* q_key = column_to_row_major_and_shrink_V_192(Q, ell);
  bf192_t q_star = bf192_sum_poly(q_key + ell);

  for (unsigned int i = 0; i < ell; i++) {
    q_key[i] = bf192_add(q_key[i], bf192_mul_bit(bf_delta, ptr_get_bit(d, i)));
  }

  zk_hash_192_ctx b_ctx;
  zk_hash_192_init(&b_ctx, chall_2);
  lynx_192_constraints_verifier(&b_ctx, q_key, owf_in, owf_out, bf_delta, params);
  lynx_aligned_free(q_key);

  uint8_t q_tilde[LYNX_192_CSP / 8];
  zk_hash_192_finalize(q_tilde, &b_ctx, q_star);

  bf192_t tmp1 = bf192_mul(bf192_load(a1_tilde), bf_delta);
  bf192_t ret  = bf192_add(bf192_load(q_tilde), tmp1);
  bf192_store(a0_tilde, ret);
}

static bf256_t* column_to_row_major_and_shrink_V_256(uint8_t** v, unsigned int ell) {
  bf256_t* new_v = BF256_ALLOC(ell + LYNX_256_CSP * 2);
  assert(new_v);
  for (unsigned int row = 0; row != ell + LYNX_256_CSP * 2; ++row) {
    uint8_t new_row[BF256_NUM_BYTES] = {0};
    for (unsigned int column = 0; column != LYNX_256_CSP; ++column) {
      ptr_set_bit(new_row, column, ptr_get_bit(v[column], row));
    }
    new_v[row] = bf256_load(new_row);
  }
  return new_v;
}

static void lynx_256_linear_map_with_auth(const uint64_t M[LYNX_256_CSP][4], const uint8_t* x_bits,
                                          const bf256_t* x_tag_bits, bf256_t* y, bf256_t* mac_y) {
  bf256_t y_tag_bits[LYNX_256_CSP];
  *y = bf256_zero();

  for (unsigned int row = 0; row < LYNX_256_CSP; ++row) {
    bf256_t row_tag = bf256_zero();
    uint8_t bit     = 0;
    for (unsigned int col = 0; col < LYNX_256_CSP; ++col) {
      const uint64_t mask_word = M[row][col / 64];
      const uint8_t mask_bit   = (mask_word >> (col % 64)) & 1u;
      if (mask_bit) {
        row_tag = bf256_add(row_tag, x_tag_bits[col]);
        bit ^= ptr_get_bit(x_bits, col);
      }
    }
    y_tag_bits[row] = row_tag;
    BF_VALUE(*y, row / 64) ^= ((uint64_t)bit << (row % 64));
  }

  *mac_y = bf256_sum_poly(y_tag_bits);
}

static void lynx_256_linear_map_tag_bits(const uint64_t M[LYNX_256_CSP][4],
                                         const bf256_t* x_tag_bits, bf256_t* y_tag_bits) {
  for (unsigned int row = 0; row < LYNX_256_CSP; ++row) {
    bf256_t row_tag = bf256_zero();
    for (unsigned int col = 0; col < LYNX_256_CSP; ++col) {
      const uint64_t mask_word = M[row][col / 64];
      const uint8_t mask_bit   = (mask_word >> (col % 64)) & 1u;
      if (mask_bit) {
        row_tag = bf256_add(row_tag, x_tag_bits[col]);
      }
    }
    y_tag_bits[row] = row_tag;
  }
}

static void lynx_256_constraints_prover(zk_hash_256_2_ctx* hasher, const uint8_t* w,
                                        const bf256_t* w_tag, const uint8_t* owf_in,
                                        const uint8_t* owf_out, const sig_paramset_t* params) {
  lynx_t mats;
  if (lynx_init(&mats, 256, owf_in, params->owf_input_size) != 0) {
    return;
  }
  const uint64_t (*L0)[4] = (const uint64_t (*)[4])mats.L0;
  const uint64_t (*L1)[4] = (const uint64_t (*)[4])mats.L1;
  const uint64_t (*L2)[4] = (const uint64_t (*)[4])mats.L2;
  const uint64_t (*L3)[4] = (const uint64_t (*)[4])mats.L3;
  bf256_t k = bf256_load(w);
  bf256_t mac_k = bf256_sum_poly(w_tag);
  bf256_t v1 = bf256_load(w + 32);
  bf256_t mac_v1 = bf256_sum_poly(w_tag + LYNX_256_CSP);
  bf256_t v2 = bf256_load(w + 64);
  bf256_t mac_v2 = bf256_sum_poly(w_tag + LYNX_256_CSP * 2);
  bf256_t c0 = bf256_load(mats.C0);
  bf256_t c1 = bf256_load(mats.C1);
  bf256_t y = bf256_load(owf_out);

  bf256_t a0, a1;


  bf256_t z = bf256_add(k, c0);
  bf256_t v1_frob5, mac_v1_frob5;
  lynx_256_linear_map_with_auth(frob256_5, (const uint8_t*) &v1, w_tag + LYNX_256_CSP,
                                &v1_frob5, &mac_v1_frob5);

  bf256_t L0k, L3k, L3v1, L3v2, L1v1, L2v2;
  bf256_t mac_L0k, mac_L3k, mac_L3v1, mac_L3v2, mac_L1v1, mac_L2v2;
  bf256_t L0k_tag_bits[LYNX_256_CSP];
  lynx_256_linear_map_with_auth(L0, w, w_tag, &L0k, &mac_L0k);
  lynx_256_linear_map_tag_bits(L0, w_tag, L0k_tag_bits);
  lynx_256_linear_map_with_auth(L3, w, w_tag, &L3k, &mac_L3k);
  lynx_256_linear_map_with_auth(L3, w + 32, w_tag + LYNX_256_CSP, &L3v1, &mac_L3v1);
  lynx_256_linear_map_with_auth(L3, w + 64, w_tag + LYNX_256_CSP * 2, &L3v2, &mac_L3v2);
  lynx_256_linear_map_with_auth(L1, w + 32, w_tag + LYNX_256_CSP, &L1v1, &mac_L1v1);
  lynx_256_linear_map_with_auth(L2, w + 64, w_tag + LYNX_256_CSP * 2, &L2v2, &mac_L2v2);

  bf256_t L0k_plus_c1 = bf256_add(L0k, c1);
  bf256_t L0k_plus_c1_frob53, mac_L0k_plus_c1_frob53, L0k_plus_c1_sq, mac_L0k_plus_c1_sq;
  lynx_256_linear_map_with_auth(frob256_53, (const uint8_t*) &L0k_plus_c1, L0k_tag_bits,
                                &L0k_plus_c1_frob53, &mac_L0k_plus_c1_frob53);
  lynx_256_linear_map_with_auth(frob256_1, (const uint8_t*) &L0k_plus_c1, L0k_tag_bits,
                                &L0k_plus_c1_sq, &mac_L0k_plus_c1_sq);

  // Constraint 0: v1 * (k+c0) = v1^(2^5)
  {
    a1 = bf256_add(bf256_mul(z, mac_v1), bf256_mul(v1, mac_k));
    a1 = bf256_add(a1, mac_v1_frob5);
    a0 = bf256_mul(mac_k, mac_v1);
    zk_hash_256_2_update(hasher, a0, a1);
  }

  // Constraint 1: (L0(k)+c1)^(2^53)*(L0(k)+c1) = (L0(k)+c1)^2*v2
  {
    a1 = bf256_add(bf256_mul(L0k_plus_c1_frob53, mac_L0k), bf256_mul(L0k_plus_c1, mac_L0k_plus_c1_frob53));
    a1 = bf256_add(a1, bf256_add(bf256_mul(L0k_plus_c1_sq, mac_v2), bf256_mul(v2, mac_L0k_plus_c1_sq)));
    a0 = bf256_add(bf256_mul(mac_L0k_plus_c1_frob53, mac_L0k), bf256_mul(mac_L0k_plus_c1_sq, mac_v2));
    zk_hash_256_2_update(hasher, a0, a1);
  }

  // Constraint 2: L1(v1)*L2(v2) = (L3(k+v1+v2)+y)*(k+c2)
  {
    bf256_t c2 = bf256_load(mats.C2);
    bf256_t kc2 = bf256_add(k, c2);
    bf256_t L3sum = bf256_add(L3k, bf256_add(L3v1, L3v2));
    bf256_t C = bf256_add(L3sum, y);
    bf256_t mac_C = bf256_add(mac_L3k, bf256_add(mac_L3v1, mac_L3v2));
    a1 = bf256_add(bf256_mul(L1v1, mac_L2v2), bf256_mul(L2v2, mac_L1v1));
    a1 = bf256_add(a1, bf256_add(bf256_mul(C, mac_k), bf256_mul(kc2, mac_C)));
    a0 = bf256_add(bf256_mul(mac_L1v1, mac_L2v2), bf256_mul(mac_C, mac_k));
    zk_hash_256_2_update(hasher, a0, a1);
  }
}

static void lynx_256_constraints_verifier(zk_hash_256_ctx* hasher, const bf256_t* w_key,
                                          const uint8_t* owf_in, const uint8_t* owf_out,
                                          bf256_t delta,
                                          const sig_paramset_t* params) {
  lynx_t mats;
  if (lynx_init(&mats, 256, owf_in, params->owf_input_size) != 0) {
    return;
  }
  const uint64_t (*L0)[4] = (const uint64_t (*)[4])mats.L0;
  const uint64_t (*L1)[4] = (const uint64_t (*)[4])mats.L1;
  const uint64_t (*L2)[4] = (const uint64_t (*)[4])mats.L2;
  const uint64_t (*L3)[4] = (const uint64_t (*)[4])mats.L3;
  bf256_t c0 = bf256_load(mats.C0);
  bf256_t c1 = bf256_load(mats.C1);
  bf256_t c2 = bf256_load(mats.C2);
  bf256_t y = bf256_load(owf_out);
  bf256_t key_k = bf256_sum_poly(w_key);
  bf256_t key_v1 = bf256_sum_poly(w_key + LYNX_256_CSP);
  bf256_t key_v2 = bf256_sum_poly(w_key + LYNX_256_CSP * 2);
  bf256_t zero = bf256_zero();
  bf256_t temp = bf256_zero();
  bf256_t b;


  bf256_t key_v1_frob5, key_L0k, key_L0k_frob53, key_L0k_sq;
  bf256_t key_L1v1, key_L2v2, key_L3k, key_L3v1, key_L3v2;
  bf256_t c1_frob53 = c1;
  bf256_t c1_sq = bf256_mul(c1, c1);
  bf256_t L0k_tag_keys[LYNX_256_CSP];
  for (unsigned int i = 0; i < 53; ++i) {
    c1_frob53 = bf256_mul(c1_frob53, c1_frob53);
  }

  lynx_256_linear_map_with_auth(frob256_5, (const uint8_t*) &zero, w_key + LYNX_256_CSP, &temp,
                                &key_v1_frob5);
  lynx_256_linear_map_with_auth(L0, (const uint8_t*) &zero, w_key, &temp, &key_L0k);
  lynx_256_linear_map_tag_bits(L0, w_key, L0k_tag_keys);
  lynx_256_linear_map_with_auth(frob256_53, (const uint8_t*) &zero, L0k_tag_keys, &temp,
                                &key_L0k_frob53);
  lynx_256_linear_map_with_auth(frob256_1, (const uint8_t*) &zero, L0k_tag_keys, &temp,
                                &key_L0k_sq);
  lynx_256_linear_map_with_auth(L1, (const uint8_t*) &zero, w_key + LYNX_256_CSP,
                                &temp, &key_L1v1);
  lynx_256_linear_map_with_auth(L2, (const uint8_t*) &zero, w_key + LYNX_256_CSP * 2,
                                &temp, &key_L2v2);
  lynx_256_linear_map_with_auth(L3, (const uint8_t*) &zero, w_key, &temp, &key_L3k);
  lynx_256_linear_map_with_auth(L3, (const uint8_t*) &zero, w_key + LYNX_256_CSP,
                                &temp, &key_L3v1);
  lynx_256_linear_map_with_auth(L3, (const uint8_t*) &zero, w_key + LYNX_256_CSP * 2,
                                &temp, &key_L3v2);

  // Constraint 0: v1 * (k+c0) = v1^(2^5)
  {
    bf256_t key_z = bf256_add(key_k, bf256_mul(delta, c0));
    b = bf256_mul(key_z, key_v1);
    b = bf256_add(b, bf256_mul(delta, key_v1_frob5));
    zk_hash_256_update(hasher, b);
  }

  // Constraint 1: (L0(k)+c1)^(2^53)*(L0(k)+c1) = (L0(k)+c1)^2*v2
  {
    bf256_t key_x = bf256_add(key_L0k, bf256_mul(delta, c1));
    bf256_t key_x_frob53 = bf256_add(key_L0k_frob53, bf256_mul(delta, c1_frob53));
    bf256_t key_x_sq = bf256_add(key_L0k_sq, bf256_mul(delta, c1_sq));
    b = bf256_add(bf256_mul(key_x_frob53, key_x), bf256_mul(key_x_sq, key_v2));
    zk_hash_256_update(hasher, b);
  }

  // Constraint 2: L1(v1)*L2(v2) = (L3(k+v1+v2)+y)*(k+c2)
  {
    bf256_t key_kc2 = bf256_add(key_k, bf256_mul(delta, c2));
    bf256_t key_L3sum = bf256_add(key_L3k, bf256_add(key_L3v1, key_L3v2));
    b = bf256_add(bf256_mul(key_L1v1, key_L2v2),
                  bf256_mul(bf256_add(key_L3sum, bf256_mul(delta, y)), key_kc2));
    zk_hash_256_update(hasher, b);
  }
}

void lynx_256_prover(uint8_t* a0_tilde, uint8_t* a1_tilde, const uint8_t* w,
                     const uint8_t* u, uint8_t** V, const uint8_t* owf_in, const uint8_t* owf_out,
                     const uint8_t* chall_2, const sig_paramset_t* params) {
  const unsigned int ell = params->lenwit;

  bf256_t* w_tag = column_to_row_major_and_shrink_V_256(V, ell);

  bf256_t bf_u_star_0 = bf256_sum_poly_bits(u);
  bf256_t bf_v_star_0 = bf256_sum_poly(w_tag + ell);

  zk_hash_256_2_ctx hasher;
  zk_hash_256_2_init(&hasher, chall_2);
  lynx_256_constraints_prover(&hasher, w, w_tag, owf_in, owf_out, params);
  zk_hash_256_2_finalize(a0_tilde, a1_tilde, &hasher, bf_v_star_0, bf_u_star_0);
  lynx_aligned_free(w_tag);
}

void lynx_256_verifier(uint8_t* a0_tilde, const uint8_t* d, uint8_t** Q, const uint8_t* owf_in,
                       const uint8_t* owf_out, const uint8_t* chall_2, const uint8_t* chall_3,
                       const uint8_t* a1_tilde,
                       const sig_paramset_t* params) {
  const unsigned int ell = params->lenwit;

  bf256_t bf_delta = bf256_load(chall_3);

  bf256_t* q_key = column_to_row_major_and_shrink_V_256(Q, ell);
  bf256_t q_star = bf256_sum_poly(q_key + ell);

  for (unsigned int i = 0; i < ell; i++) {
    q_key[i] = bf256_add(q_key[i], bf256_mul_bit(bf_delta, ptr_get_bit(d, i)));
  }

  zk_hash_256_ctx b_ctx;
  zk_hash_256_init(&b_ctx, chall_2);
  lynx_256_constraints_verifier(&b_ctx, q_key, owf_in, owf_out, bf_delta, params);
  lynx_aligned_free(q_key);

  uint8_t q_tilde[LYNX_256_CSP / 8];
  zk_hash_256_finalize(q_tilde, &b_ctx, q_star);

  bf256_t tmp1 = bf256_mul(bf256_load(a1_tilde), bf_delta);
  bf256_t ret  = bf256_add(bf256_load(q_tilde), tmp1);
  bf256_store(a0_tilde, ret);
}

static bf384_t* column_to_row_major_and_shrink_V_384(uint8_t** v, unsigned int ell) {
  bf384_t* new_v = BF384_ALLOC(ell + LYNX_384_CSP * 2);
  assert(new_v);
  for (unsigned int row = 0; row != ell + LYNX_384_CSP * 2; ++row) {
    uint8_t new_row[BF384_NUM_BYTES] = {0};
    for (unsigned int column = 0; column != LYNX_384_CSP; ++column) {
      ptr_set_bit(new_row, column, ptr_get_bit(v[column], row));
    }
    new_v[row] = bf384_load(new_row);
  }
  return new_v;
}

static void lynx_384_linear_map_with_auth(const uint64_t M[LYNX_384_CSP][6], const uint8_t* x_bits,
                                          const bf384_t* x_tag_bits, bf384_t* y, bf384_t* mac_y) {
  bf384_t y_tag_bits[LYNX_384_CSP];
  uint8_t y_bytes[BF384_NUM_BYTES] = {0};

  for (unsigned int row = 0; row < LYNX_384_CSP; ++row) {
    bf384_t row_tag = bf384_zero();
    uint8_t bit     = 0;
    for (unsigned int col = 0; col < LYNX_384_CSP; ++col) {
      const uint64_t mask_word = M[row][col / 64];
      const uint8_t mask_bit   = (mask_word >> (col % 64)) & 1u;
      if (mask_bit) {
        row_tag = bf384_add(row_tag, x_tag_bits[col]);
        bit ^= ptr_get_bit(x_bits, col);
      }
    }
    y_tag_bits[row] = row_tag;
    ptr_set_bit(y_bytes, row, bit);
  }

  *y = bf384_load(y_bytes);
  *mac_y = bf384_sum_poly(y_tag_bits);
}

static void lynx_384_linear_map_tag_bits(const uint64_t M[LYNX_384_CSP][6],
                                         const bf384_t* x_tag_bits, bf384_t* y_tag_bits) {
  for (unsigned int row = 0; row < LYNX_384_CSP; ++row) {
    bf384_t row_tag = bf384_zero();
    for (unsigned int col = 0; col < LYNX_384_CSP; ++col) {
      const uint64_t mask_word = M[row][col / 64];
      const uint8_t mask_bit   = (mask_word >> (col % 64)) & 1u;
      if (mask_bit) {
        row_tag = bf384_add(row_tag, x_tag_bits[col]);
      }
    }
    y_tag_bits[row] = row_tag;
  }
}

static void lynx_384_constraints_prover(zk_hash_384_2_ctx* hasher, const uint8_t* w,
                                        const bf384_t* w_tag, const uint8_t* owf_in,
                                        const uint8_t* owf_out, const sig_paramset_t* params) {
  lynx_t mats;
  if (lynx_init(&mats, 384, owf_in, params->owf_input_size) != 0) {
    return;
  }
  const uint64_t (*L0)[6] = (const uint64_t (*)[6])mats.L0;
  const uint64_t (*L1)[6] = (const uint64_t (*)[6])mats.L1;
  const uint64_t (*L2)[6] = (const uint64_t (*)[6])mats.L2;
  const uint64_t (*L3)[6] = (const uint64_t (*)[6])mats.L3;
  bf384_t k = bf384_load(w);
  bf384_t mac_k = bf384_sum_poly(w_tag);
  bf384_t v1 = bf384_load(w + 48);
  bf384_t mac_v1 = bf384_sum_poly(w_tag + LYNX_384_CSP);
  bf384_t v2 = bf384_load(w + 96);
  bf384_t mac_v2 = bf384_sum_poly(w_tag + LYNX_384_CSP * 2);
  bf384_t c0 = bf384_load(mats.C0);
  bf384_t c1 = bf384_load(mats.C1);
  bf384_t c2 = bf384_load(mats.C2);
  bf384_t y = bf384_load(owf_out);

  bf384_t a0, a1;


  bf384_t z = bf384_add(k, c0);
  bf384_t v1_frob13, mac_v1_frob13;
  lynx_384_linear_map_with_auth(frob384_13, (const uint8_t*) &v1, w_tag + LYNX_384_CSP,
                                &v1_frob13, &mac_v1_frob13);

  bf384_t L0k, L3k, L3v1, L1v1, L2v2, L3v2;
  bf384_t mac_L0k, mac_L3k, mac_L3v1, mac_L1v1, mac_L2v2, mac_L3v2;
  bf384_t L0k_tag_bits[LYNX_384_CSP];
  lynx_384_linear_map_with_auth(L0, w, w_tag, &L0k, &mac_L0k);
  lynx_384_linear_map_tag_bits(L0, w_tag, L0k_tag_bits);
  lynx_384_linear_map_with_auth(L3, w, w_tag, &L3k, &mac_L3k);
  lynx_384_linear_map_with_auth(L3, w + 48, w_tag + LYNX_384_CSP, &L3v1, &mac_L3v1);
  lynx_384_linear_map_with_auth(L1, w + 48, w_tag + LYNX_384_CSP, &L1v1, &mac_L1v1);
  lynx_384_linear_map_with_auth(L2, w + 96, w_tag + LYNX_384_CSP * 2, &L2v2, &mac_L2v2);
  lynx_384_linear_map_with_auth(L3, w + 96, w_tag + LYNX_384_CSP * 2, &L3v2, &mac_L3v2);

  bf384_t L0k_plus_c1 = bf384_add(L0k, c1);
  bf384_t L0k_plus_c1_frob60, mac_L0k_plus_c1_frob60, L0k_plus_c1_frob30, mac_L0k_plus_c1_frob30;
  lynx_384_linear_map_with_auth(frob384_60, (const uint8_t*) &L0k_plus_c1, L0k_tag_bits,
                                &L0k_plus_c1_frob60, &mac_L0k_plus_c1_frob60);
  lynx_384_linear_map_with_auth(frob384_30, (const uint8_t*) &L0k_plus_c1, L0k_tag_bits,
                                &L0k_plus_c1_frob30, &mac_L0k_plus_c1_frob30);

  // Constraint 0: v1 * (k+c0) = v1^(2^13)
  {
    a1 = bf384_add(bf384_mul(z, mac_v1), bf384_mul(v1, mac_k));
    a1 = bf384_add(a1, mac_v1_frob13);
    a0 = bf384_mul(mac_k, mac_v1);
    zk_hash_384_2_update(hasher, a0, a1);
  }

  // Constraint 1: (L0k+c1)^(2^60)*(L0k+c1) = (L0k+c1)^(2^30)*v2
  {
    a1 = bf384_add(bf384_mul(L0k_plus_c1_frob60, mac_L0k), bf384_mul(L0k_plus_c1, mac_L0k_plus_c1_frob60));
    a1 = bf384_add(a1, bf384_add(bf384_mul(L0k_plus_c1_frob30, mac_v2), bf384_mul(v2, mac_L0k_plus_c1_frob30)));
    a0 = bf384_add(bf384_mul(mac_L0k_plus_c1_frob60, mac_L0k), bf384_mul(mac_L0k_plus_c1_frob30, mac_v2));
    zk_hash_384_2_update(hasher, a0, a1);
  }

  // Constraint 2: L1(v1)*L2(v2) = (L3(k+v1+v2)+y)*(k+c2)
  {
    bf384_t kc2 = bf384_add(k, c2);
    bf384_t L3sum = bf384_add(L3k, bf384_add(L3v1, L3v2));
    bf384_t C = bf384_add(L3sum, y);
    bf384_t mac_C = bf384_add(mac_L3k, bf384_add(mac_L3v1, mac_L3v2));
    a1 = bf384_add(bf384_mul(L1v1, mac_L2v2), bf384_mul(L2v2, mac_L1v1));
    a1 = bf384_add(a1, bf384_add(bf384_mul(C, mac_k), bf384_mul(kc2, mac_C)));
    a0 = bf384_add(bf384_mul(mac_L1v1, mac_L2v2), bf384_mul(mac_C, mac_k));
    zk_hash_384_2_update(hasher, a0, a1);
  }
}

static void lynx_384_constraints_verifier(zk_hash_384_ctx* hasher, const bf384_t* w_key,
                                          const uint8_t* owf_in, const uint8_t* owf_out,
                                          bf384_t delta,
                                          const sig_paramset_t* params) {
  lynx_t mats;
  if (lynx_init(&mats, 384, owf_in, params->owf_input_size) != 0) {
    return;
  }
  const uint64_t (*L0)[6] = (const uint64_t (*)[6])mats.L0;
  const uint64_t (*L1)[6] = (const uint64_t (*)[6])mats.L1;
  const uint64_t (*L2)[6] = (const uint64_t (*)[6])mats.L2;
  const uint64_t (*L3)[6] = (const uint64_t (*)[6])mats.L3;
  bf384_t c0 = bf384_load(mats.C0);
  bf384_t c1 = bf384_load(mats.C1);
  bf384_t c2 = bf384_load(mats.C2);
  bf384_t y = bf384_load(owf_out);
  bf384_t key_k = bf384_sum_poly(w_key);
  bf384_t key_v1 = bf384_sum_poly(w_key + LYNX_384_CSP);
  bf384_t key_v2 = bf384_sum_poly(w_key + LYNX_384_CSP * 2);
  bf384_t zero = bf384_zero();
  bf384_t temp = bf384_zero();
  bf384_t b;


  bf384_t key_v1_frob13, key_L0k, key_L0k_frob60, key_L0k_frob30;
  bf384_t key_L1v1, key_L3k, key_L3v1, key_L2v2, key_L3v2;
  bf384_t c1_frob60 = c1;
  bf384_t c1_frob30 = c1;
  bf384_t L0k_tag_keys[LYNX_384_CSP];
  for (unsigned int i = 0; i < 60; ++i) {
    c1_frob60 = bf384_mul(c1_frob60, c1_frob60);
  }
  for (unsigned int i = 0; i < 30; ++i) {
    c1_frob30 = bf384_mul(c1_frob30, c1_frob30);
  }

  lynx_384_linear_map_with_auth(frob384_13, (const uint8_t*) &zero, w_key + LYNX_384_CSP,
                                &temp, &key_v1_frob13);
  lynx_384_linear_map_with_auth(L0, (const uint8_t*) &zero, w_key, &temp, &key_L0k);
  lynx_384_linear_map_tag_bits(L0, w_key, L0k_tag_keys);
  lynx_384_linear_map_with_auth(frob384_60, (const uint8_t*) &zero, L0k_tag_keys, &temp,
                                &key_L0k_frob60);
  lynx_384_linear_map_with_auth(frob384_30, (const uint8_t*) &zero, L0k_tag_keys, &temp,
                                &key_L0k_frob30);
  lynx_384_linear_map_with_auth(L1, (const uint8_t*) &zero, w_key + LYNX_384_CSP,
                                &temp, &key_L1v1);
  lynx_384_linear_map_with_auth(L3, (const uint8_t*) &zero, w_key, &temp, &key_L3k);
  lynx_384_linear_map_with_auth(L3, (const uint8_t*) &zero, w_key + LYNX_384_CSP,
                                &temp, &key_L3v1);
  lynx_384_linear_map_with_auth(L2, (const uint8_t*) &zero, w_key + LYNX_384_CSP * 2,
                                &temp, &key_L2v2);
  lynx_384_linear_map_with_auth(L3, (const uint8_t*) &zero, w_key + LYNX_384_CSP * 2,
                                &temp, &key_L3v2);

  // Constraint 0: v1 * (k+c0) = v1^(2^13)
  {
    bf384_t key_z = bf384_add(key_k, bf384_mul(delta, c0));
    b = bf384_mul(key_z, key_v1);
    b = bf384_add(b, bf384_mul(delta, key_v1_frob13));
    zk_hash_384_update(hasher, b);
  }

  // Constraint 1: (L0k+c1)^(2^60)*(L0k+c1) = (L0k+c1)^(2^30)*v2
  {
    bf384_t key_x = bf384_add(key_L0k, bf384_mul(delta, c1));
    bf384_t key_x_frob60 = bf384_add(key_L0k_frob60, bf384_mul(delta, c1_frob60));
    bf384_t key_x_frob30 = bf384_add(key_L0k_frob30, bf384_mul(delta, c1_frob30));
    b = bf384_add(bf384_mul(key_x_frob60, key_x), bf384_mul(key_x_frob30, key_v2));
    zk_hash_384_update(hasher, b);
  }

  // Constraint 2: L1(v1)*L2(v2) = (L3(k+v1+v2)+y)*(k+c2)
  {
    bf384_t key_kc2 = bf384_add(key_k, bf384_mul(delta, c2));
    bf384_t key_L3sum = bf384_add(key_L3k, bf384_add(key_L3v1, key_L3v2));
    b = bf384_add(bf384_mul(key_L1v1, key_L2v2),
                  bf384_mul(bf384_add(key_L3sum, bf384_mul(delta, y)), key_kc2));
    zk_hash_384_update(hasher, b);
  }
}

void lynx_384_prover(uint8_t* a0_tilde, uint8_t* a1_tilde, const uint8_t* w,
                     const uint8_t* u, uint8_t** V, const uint8_t* owf_in, const uint8_t* owf_out,
                     const uint8_t* chall_2, const sig_paramset_t* params) {
  const unsigned int ell = params->lenwit;

  bf384_t* w_tag = column_to_row_major_and_shrink_V_384(V, ell);

  bf384_t bf_u_star_0 = bf384_sum_poly_bits(u);
  bf384_t bf_v_star_0 = bf384_sum_poly(w_tag + ell);

  zk_hash_384_2_ctx hasher;
  zk_hash_384_2_init(&hasher, chall_2);
  lynx_384_constraints_prover(&hasher, w, w_tag, owf_in, owf_out, params);
  zk_hash_384_2_finalize(a0_tilde, a1_tilde, &hasher, bf_v_star_0, bf_u_star_0);
  lynx_aligned_free(w_tag);
}

void lynx_384_verifier(uint8_t* a0_tilde, const uint8_t* d, uint8_t** Q, const uint8_t* owf_in,
                       const uint8_t* owf_out, const uint8_t* chall_2, const uint8_t* chall_3,
                       const uint8_t* a1_tilde,
                       const sig_paramset_t* params) {
  const unsigned int ell = params->lenwit;

  bf384_t bf_delta = bf384_load(chall_3);

  bf384_t* q_key = column_to_row_major_and_shrink_V_384(Q, ell);
  bf384_t q_star = bf384_sum_poly(q_key + ell);

  for (unsigned int i = 0; i < ell; i++) {
    q_key[i] = bf384_add(q_key[i], bf384_mul_bit(bf_delta, ptr_get_bit(d, i)));
  }

  zk_hash_384_ctx b_ctx;
  zk_hash_384_init(&b_ctx, chall_2);
  lynx_384_constraints_verifier(&b_ctx, q_key, owf_in, owf_out, bf_delta, params);
  lynx_aligned_free(q_key);

  uint8_t q_tilde[LYNX_384_CSP / 8];
  zk_hash_384_finalize(q_tilde, &b_ctx, q_star);

  bf384_t tmp1 = bf384_mul(bf384_load(a1_tilde), bf_delta);
  bf384_t ret  = bf384_add(bf384_load(q_tilde), tmp1);
  bf384_store(a0_tilde, ret);
}

static bf512_t* column_to_row_major_and_shrink_V_512(uint8_t** v, unsigned int ell) {
  bf512_t* new_v = BF512_ALLOC(ell + LYNX_512_CSP * 2);
  assert(new_v);
  for (unsigned int row = 0; row != ell + LYNX_512_CSP * 2; ++row) {
    uint8_t new_row[BF512_NUM_BYTES] = {0};
    for (unsigned int column = 0; column != LYNX_512_CSP; ++column) {
      ptr_set_bit(new_row, column, ptr_get_bit(v[column], row));
    }
    new_v[row] = bf512_load(new_row);
  }
  return new_v;
}

static void lynx_512_linear_map_with_auth(const uint64_t M[LYNX_512_CSP][8], const uint8_t* x_bits,
                                          const bf512_t* x_tag_bits, bf512_t* y, bf512_t* mac_y) {
  bf512_t y_tag_bits[LYNX_512_CSP];
  *y = bf512_zero();

  for (unsigned int row = 0; row < LYNX_512_CSP; ++row) {
    bf512_t row_tag = bf512_zero();
    uint8_t bit     = 0;
    for (unsigned int col = 0; col < LYNX_512_CSP; ++col) {
      const uint64_t mask_word = M[row][col / 64];
      const uint8_t mask_bit   = (mask_word >> (col % 64)) & 1u;
      if (mask_bit) {
        row_tag = bf512_add(row_tag, x_tag_bits[col]);
        bit ^= ptr_get_bit(x_bits, col);
      }
    }
    y_tag_bits[row] = row_tag;
    BF_VALUE(*y, row / 64) ^= ((uint64_t)bit << (row % 64));
  }

  *mac_y = bf512_sum_poly(y_tag_bits);
}

static void lynx_512_linear_map_tag_bits(const uint64_t M[LYNX_512_CSP][8],
                                         const bf512_t* x_tag_bits, bf512_t* y_tag_bits) {
  for (unsigned int row = 0; row < LYNX_512_CSP; ++row) {
    bf512_t row_tag = bf512_zero();
    for (unsigned int col = 0; col < LYNX_512_CSP; ++col) {
      const uint64_t mask_word = M[row][col / 64];
      const uint8_t mask_bit   = (mask_word >> (col % 64)) & 1u;
      if (mask_bit) {
        row_tag = bf512_add(row_tag, x_tag_bits[col]);
      }
    }
    y_tag_bits[row] = row_tag;
  }
}

static void lynx_512_constraints_prover(zk_hash_512_2_ctx* hasher, const uint8_t* w,
                                        const bf512_t* w_tag, const uint8_t* owf_in,
                                        const uint8_t* owf_out, const sig_paramset_t* params) {
  lynx_t mats;
  if (lynx_init(&mats, 512, owf_in, params->owf_input_size) != 0) {
    return;
  }
  const uint64_t (*L0)[8] = (const uint64_t (*)[8])mats.L0;
  const uint64_t (*L1)[8] = (const uint64_t (*)[8])mats.L1;
  const uint64_t (*L2)[8] = (const uint64_t (*)[8])mats.L2;
  const uint64_t (*L3)[8] = (const uint64_t (*)[8])mats.L3;
  bf512_t k = bf512_load(w);
  bf512_t mac_k = bf512_sum_poly(w_tag);
  bf512_t v1 = bf512_load(w + 64);
  bf512_t mac_v1 = bf512_sum_poly(w_tag + LYNX_512_CSP);
  bf512_t v2 = bf512_load(w + 128);
  bf512_t mac_v2 = bf512_sum_poly(w_tag + LYNX_512_CSP * 2);
  bf512_t c0 = bf512_load(mats.C0);
  bf512_t c1 = bf512_load(mats.C1);
  bf512_t y = bf512_load(owf_out);

  bf512_t a0, a1;


  bf512_t z = bf512_add(k, c0);
  bf512_t v1_frob6, mac_v1_frob6, v1_frob12, mac_v1_frob12;
  lynx_512_linear_map_with_auth(frob512_6, (const uint8_t*) &v1, w_tag + LYNX_512_CSP,
                                &v1_frob6, &mac_v1_frob6);
  lynx_512_linear_map_with_auth(frob512_12, (const uint8_t*) &v1, w_tag + LYNX_512_CSP,
                                &v1_frob12, &mac_v1_frob12);

  bf512_t L0k, L3k, L3v1, L3v2, L1v1, L2v2;
  bf512_t mac_L0k, mac_L3k, mac_L3v1, mac_L3v2, mac_L1v1, mac_L2v2;
  bf512_t L0k_tag_bits[LYNX_512_CSP];
  lynx_512_linear_map_with_auth(L0, w, w_tag, &L0k, &mac_L0k);
  lynx_512_linear_map_tag_bits(L0, w_tag, L0k_tag_bits);
  lynx_512_linear_map_with_auth(L3, w, w_tag, &L3k, &mac_L3k);
  lynx_512_linear_map_with_auth(L3, w + 64, w_tag + LYNX_512_CSP, &L3v1, &mac_L3v1);
  lynx_512_linear_map_with_auth(L3, w + 128, w_tag + LYNX_512_CSP * 2, &L3v2, &mac_L3v2);
  lynx_512_linear_map_with_auth(L1, w + 64, w_tag + LYNX_512_CSP, &L1v1, &mac_L1v1);
  lynx_512_linear_map_with_auth(L2, w + 128, w_tag + LYNX_512_CSP * 2, &L2v2, &mac_L2v2);

  bf512_t L0k_plus_c1 = bf512_add(L0k, c1);
  bf512_t L0k_plus_c1_frob180, mac_L0k_plus_c1_frob180, L0k_plus_c1_frob90, mac_L0k_plus_c1_frob90;
  lynx_512_linear_map_with_auth(frob512_180, (const uint8_t*) &L0k_plus_c1, L0k_tag_bits,
                                &L0k_plus_c1_frob180, &mac_L0k_plus_c1_frob180);
  lynx_512_linear_map_with_auth(frob512_90, (const uint8_t*) &L0k_plus_c1, L0k_tag_bits,
                                &L0k_plus_c1_frob90, &mac_L0k_plus_c1_frob90);

  // Constraint 0: v1^(2^6)*(k+c0) = v1^(2^12)*v1
  {
    a1 = bf512_add(bf512_mul(v1_frob6, mac_k), bf512_mul(z, mac_v1_frob6));
    a1 = bf512_add(a1, bf512_add(bf512_mul(v1_frob12, mac_v1), bf512_mul(v1, mac_v1_frob12)));
    a0 = bf512_add(bf512_mul(mac_v1_frob6, mac_k), bf512_mul(mac_v1_frob12, mac_v1));
    zk_hash_512_2_update(hasher, a0, a1);
  }

  // Constraint 1: (L0k+c1)^(2^180)*(L0k+c1) = (L0k+c1)^(2^90)*v2
  {
    a1 = bf512_add(bf512_mul(L0k_plus_c1_frob180, mac_L0k), bf512_mul(L0k_plus_c1, mac_L0k_plus_c1_frob180));
    a1 = bf512_add(a1, bf512_add(bf512_mul(L0k_plus_c1_frob90, mac_v2), bf512_mul(v2, mac_L0k_plus_c1_frob90)));
    a0 = bf512_add(bf512_mul(mac_L0k_plus_c1_frob180, mac_L0k), bf512_mul(mac_L0k_plus_c1_frob90, mac_v2));
    zk_hash_512_2_update(hasher, a0, a1);
  }

  // Constraint 2: L1(v1)*L2(v2) = (L3(k+v1+v2)+y)*(k+c2)
  {
    bf512_t c2 = bf512_load(mats.C2);
    bf512_t kc2 = bf512_add(k, c2);
    bf512_t L3_sum = bf512_add(L3k, bf512_add(L3v1, L3v2));
    bf512_t C = bf512_add(L3_sum, y);
    bf512_t mac_C = bf512_add(mac_L3k, bf512_add(mac_L3v1, mac_L3v2));
    a1 = bf512_add(bf512_mul(L1v1, mac_L2v2), bf512_mul(L2v2, mac_L1v1));
    a1 = bf512_add(a1, bf512_add(bf512_mul(C, mac_k), bf512_mul(kc2, mac_C)));
    a0 = bf512_add(bf512_mul(mac_L1v1, mac_L2v2), bf512_mul(mac_C, mac_k));
    zk_hash_512_2_update(hasher, a0, a1);
  }
}

static void lynx_512_constraints_verifier(zk_hash_512_ctx* hasher, const bf512_t* w_key,
                                          const uint8_t* owf_in, const uint8_t* owf_out,
                                          bf512_t delta,
                                          const sig_paramset_t* params) {
  lynx_t mats;
  if (lynx_init(&mats, 512, owf_in, params->owf_input_size) != 0) {
    return;
  }
  const uint64_t (*L0)[8] = (const uint64_t (*)[8])mats.L0;
  const uint64_t (*L1)[8] = (const uint64_t (*)[8])mats.L1;
  const uint64_t (*L2)[8] = (const uint64_t (*)[8])mats.L2;
  const uint64_t (*L3)[8] = (const uint64_t (*)[8])mats.L3;
  bf512_t c0 = bf512_load(mats.C0);
  bf512_t c1 = bf512_load(mats.C1);
  bf512_t y = bf512_load(owf_out);
  bf512_t key_k = bf512_sum_poly(w_key);
  bf512_t key_v1 = bf512_sum_poly(w_key + LYNX_512_CSP);
  bf512_t key_v2 = bf512_sum_poly(w_key + LYNX_512_CSP * 2);
  bf512_t zero = bf512_zero();
  bf512_t temp = bf512_zero();
  bf512_t b;


  bf512_t key_v1_frob6, key_v1_frob12, key_L0k, key_L0k_frob180, key_L0k_frob90;
  bf512_t key_L1v1, key_L2v2, key_L3k, key_L3v1, key_L3v2;
  bf512_t c1_frob180 = c1;
  bf512_t c1_frob90 = c1;
  bf512_t L0k_tag_keys[LYNX_512_CSP];
  for (unsigned int i = 0; i < 180; ++i) {
    c1_frob180 = bf512_mul(c1_frob180, c1_frob180);
  }
  for (unsigned int i = 0; i < 90; ++i) {
    c1_frob90 = bf512_mul(c1_frob90, c1_frob90);
  }

  lynx_512_linear_map_with_auth(frob512_6, (const uint8_t*) &zero, w_key + LYNX_512_CSP,
                                &temp, &key_v1_frob6);
  lynx_512_linear_map_with_auth(frob512_12, (const uint8_t*) &zero, w_key + LYNX_512_CSP,
                                &temp, &key_v1_frob12);
  lynx_512_linear_map_with_auth(L0, (const uint8_t*) &zero, w_key, &temp, &key_L0k);
  lynx_512_linear_map_tag_bits(L0, w_key, L0k_tag_keys);
  lynx_512_linear_map_with_auth(frob512_180, (const uint8_t*) &zero, L0k_tag_keys, &temp,
                                &key_L0k_frob180);
  lynx_512_linear_map_with_auth(frob512_90, (const uint8_t*) &zero, L0k_tag_keys, &temp,
                                &key_L0k_frob90);
  lynx_512_linear_map_with_auth(L1, (const uint8_t*) &zero, w_key + LYNX_512_CSP,
                                &temp, &key_L1v1);
  lynx_512_linear_map_with_auth(L2, (const uint8_t*) &zero, w_key + LYNX_512_CSP * 2,
                                &temp, &key_L2v2);
  lynx_512_linear_map_with_auth(L3, (const uint8_t*) &zero, w_key, &temp, &key_L3k);
  lynx_512_linear_map_with_auth(L3, (const uint8_t*) &zero, w_key + LYNX_512_CSP,
                                &temp, &key_L3v1);
  lynx_512_linear_map_with_auth(L3, (const uint8_t*) &zero, w_key + LYNX_512_CSP * 2,
                                &temp, &key_L3v2);

  // Constraint 0: v1^(2^6)*(k+c0) = v1^(2^12)*v1
  {
    bf512_t key_z = bf512_add(key_k, bf512_mul(delta, c0));
    b = bf512_add(bf512_mul(key_v1_frob6, key_z), bf512_mul(key_v1_frob12, key_v1));
    zk_hash_512_update(hasher, b);
  }

  // Constraint 1: (L0k+c1)^(2^180)*(L0k+c1) = (L0k+c1)^(2^90)*v2
  {
    bf512_t key_x = bf512_add(key_L0k, bf512_mul(delta, c1));
    bf512_t key_x_frob180 = bf512_add(key_L0k_frob180, bf512_mul(delta, c1_frob180));
    bf512_t key_x_frob90 = bf512_add(key_L0k_frob90, bf512_mul(delta, c1_frob90));
    b = bf512_add(bf512_mul(key_x_frob180, key_x), bf512_mul(key_x_frob90, key_v2));
    zk_hash_512_update(hasher, b);
  }

  // Constraint 2: L1(v1)*L2(v2) = (L3(k+v1+v2)+y)*(k+c2)
  {
    bf512_t c2 = bf512_load(mats.C2);
    bf512_t key_kc2 = bf512_add(key_k, bf512_mul(delta, c2));
    bf512_t key_L3sum = bf512_add(key_L3k, bf512_add(key_L3v1, key_L3v2));
    b = bf512_add(bf512_mul(key_L1v1, key_L2v2),
                  bf512_mul(bf512_add(key_L3sum, bf512_mul(delta, y)), key_kc2));
    zk_hash_512_update(hasher, b);
  }
}

void lynx_512_prover(uint8_t* a0_tilde, uint8_t* a1_tilde, const uint8_t* w,
                     const uint8_t* u, uint8_t** V, const uint8_t* owf_in, const uint8_t* owf_out,
                     const uint8_t* chall_2, const sig_paramset_t* params) {
  const unsigned int ell = params->lenwit;

  bf512_t* w_tag = column_to_row_major_and_shrink_V_512(V, ell);

  bf512_t bf_u_star_0 = bf512_sum_poly_bits(u);
  bf512_t bf_v_star_0 = bf512_sum_poly(w_tag + ell);

  zk_hash_512_2_ctx hasher;
  zk_hash_512_2_init(&hasher, chall_2);
  lynx_512_constraints_prover(&hasher, w, w_tag, owf_in, owf_out, params);
  zk_hash_512_2_finalize(a0_tilde, a1_tilde, &hasher, bf_v_star_0, bf_u_star_0);
  lynx_aligned_free(w_tag);
}

void lynx_512_verifier(uint8_t* a0_tilde, const uint8_t* d, uint8_t** Q, const uint8_t* owf_in,
                       const uint8_t* owf_out, const uint8_t* chall_2, const uint8_t* chall_3,
                       const uint8_t* a1_tilde,
                       const sig_paramset_t* params) {
  const unsigned int ell = params->lenwit;

  bf512_t bf_delta = bf512_load(chall_3);

  bf512_t* q_key = column_to_row_major_and_shrink_V_512(Q, ell);
  bf512_t q_star = bf512_sum_poly(q_key + ell);

  for (unsigned int i = 0; i < ell; i++) {
    q_key[i] = bf512_add(q_key[i], bf512_mul_bit(bf_delta, ptr_get_bit(d, i)));
  }

  zk_hash_512_ctx b_ctx;
  zk_hash_512_init(&b_ctx, chall_2);
  lynx_512_constraints_verifier(&b_ctx, q_key, owf_in, owf_out, bf_delta, params);
  lynx_aligned_free(q_key);

  uint8_t q_tilde[LYNX_512_CSP / 8];
  zk_hash_512_finalize(q_tilde, &b_ctx, q_star);

  bf512_t tmp1 = bf512_mul(bf512_load(a1_tilde), bf_delta);
  bf512_t ret  = bf512_add(bf512_load(q_tilde), tmp1);
  bf512_store(a0_tilde, ret);
}
