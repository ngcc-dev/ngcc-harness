#include "utils_ublock/ublockith_ublock_256.h"

#include "fields.h"
#include "macros.h"
#include "utils_ublock/ublock_constraints.h"
#include "universal_hashing.h"
#include "utils.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define UBLOCKITH_LAMBDA 256u

#define bfSSS_t bf256_t
#define bfSSS_load bf256_load
#define bfSSS_store bf256_store
#define bfSSS_add bf256_add
#define bfSSS_mul bf256_mul
#define bfSSS_mul_bit bf256_mul_bit
#define bfSSS_zero bf256_zero
#define bfSSS_sum_poly bf256_sum_poly
#define bfSSS_sum_poly_bits bf256_sum_poly_bits
#define BFSSS_ALIGN BF256_ALIGN

#define zk_hash_SSS_3_ctx zk_hash_256_3_ctx
#define zk_hash_SSS_3_init zk_hash_256_3_init
#define zk_hash_SSS_3_update zk_hash_256_3_update
#define zk_hash_SSS_3_finalize zk_hash_256_3_finalize
#define zk_hash_SSS_ctx zk_hash_256_ctx
#define zk_hash_SSS_init zk_hash_256_init
#define zk_hash_SSS_update zk_hash_256_update
#define zk_hash_SSS_finalize zk_hash_256_finalize

#define PAD_TO(s, a) (((s) + (a) - 1) & ~((a) - 1))
#define BFSSS_ALLOC(s) aligned_alloc(BFSSS_ALIGN, PAD_TO((s) * sizeof(bfSSS_t), BFSSS_ALIGN))

enum {
  UBLOCK_BLOCK_BITS = UBLOCK_BLOCK * 8u,
  UBLOCK_KBAR_BITS = (UBLOCK_ROUNDS + 1u) * UBLOCK_BLOCK_BITS,
  UBLOCK_KEYEXP_W_BITS = 256u + 64u * UBLOCK_ROUNDS,
  UBLOCK_KEYEXP_O_BITS = UBLOCK_ROUNDS * 64u,
  UBLOCK_ENC_W_BITS = ((UBLOCK_ROUNDS / 2u) - 1u) * UBLOCK_BLOCK_BITS,
  UBLOCK_ENC_O_BITS = (UBLOCK_ROUNDS / 2u) * UBLOCK_BLOCK_BITS,
};

static bfSSS_t* column_to_row_major_and_shrink_V_SSS(uint8_t** v, unsigned int ell);
static void constant_to_vole_SSS_prover(bfSSS_t* tag, unsigned int n);
static void constant_to_vole_SSS_verifier(bfSSS_t* key, const uint8_t* val, bfSSS_t delta,
                                          unsigned int n);
static void ublock_build_round_keys_bytes(uint8_t* k_bar, const uint8_t* key);
static inline void ublock_hash_SSS_3_update_inline(zk_hash_SSS_3_ctx* ctx, bfSSS_t v0, bfSSS_t v1,
                                                   bfSSS_t v2);
static inline void ublock_hash_SSS_update_inline(zk_hash_SSS_ctx* ctx, bfSSS_t v);
static void ublock_hash_deg3_constraints(zk_hash_SSS_3_ctx* hasher, const bfSSS_t* deg0,
                                         const bfSSS_t* deg1, const bfSSS_t* deg2,
                                         unsigned int n_bits);
static void ublock_hash_deg1_constraints(zk_hash_SSS_ctx* hasher, const bfSSS_t* key,
                                         unsigned int n_bits);
static void ublock_SSS_constraints_prover(const params_t* params, zk_hash_SSS_3_ctx* hasher,
                                          const uint8_t* w, const bfSSS_t* w_tag,
                                          const uint8_t* owf_in, const uint8_t* owf_out);
static void ublock_SSS_constraints_verifier(const params_t* params, zk_hash_SSS_ctx* hasher,
                                            const bfSSS_t* w_key, const uint8_t* owf_in,
                                            const uint8_t* owf_out, bfSSS_t delta);

static inline void ublock_hash_keyspace_reduction_prover(zk_hash_SSS_3_ctx* hasher,
                                                         const uint8_t* w,
                                                         const bfSSS_t* w_tag) {
  zk_hash_SSS_3_update(
      hasher, bfSSS_mul(w_tag[0], w_tag[1]),
      bfSSS_add(bfSSS_mul_bit(w_tag[0], ptr_get_bit(w, 1)),
                bfSSS_mul_bit(w_tag[1], ptr_get_bit(w, 0))),
      bfSSS_zero());
}

static inline void ublock_hash_keyspace_reduction_verifier(zk_hash_SSS_ctx* hasher,
                                                           const bfSSS_t* w_key) {
  zk_hash_SSS_update(hasher, bfSSS_mul(w_key[0], w_key[1]));
}

static inline uint64_t bit_transpose8x8_u64(uint64_t x) {
  uint64_t t;
  t = (x ^ (x >> 7)) & UINT64_C(0x00AA00AA00AA00AA);
  x ^= t ^ (t << 7);
  t = (x ^ (x >> 14)) & UINT64_C(0x0000CCCC0000CCCC);
  x ^= t ^ (t << 14);
  t = (x ^ (x >> 28)) & UINT64_C(0x00000000F0F0F0F0);
  x ^= t ^ (t << 28);
  return x;
}

void ublock_256_prover(const params_t* params, uint8_t* a0_tilde, uint8_t* a1_tilde,
                       uint8_t* a2_tilde, const uint8_t* w, const uint8_t* u, uint8_t** V,
                       const uint8_t* owf_in, const uint8_t* owf_out, const uint8_t* chall_2) {
  const unsigned int ell = params->ell;

  bfSSS_t* w_tag = column_to_row_major_and_shrink_V_SSS(V, ell);

  const bfSSS_t u_star_0 = bfSSS_sum_poly_bits(u);
  const bfSSS_t u_star_1 = bfSSS_sum_poly_bits(u + (UBLOCKITH_LAMBDA / 8u));
  const bfSSS_t v_star_0 = bfSSS_sum_poly(w_tag + ell);
  const bfSSS_t v_star_1 = bfSSS_sum_poly(w_tag + ell + UBLOCKITH_LAMBDA);

  zk_hash_SSS_3_ctx hasher;
  zk_hash_SSS_3_init(&hasher, chall_2);
  ublock_SSS_constraints_prover(params, &hasher, w, w_tag, owf_in, owf_out);
  zk_hash_SSS_3_finalize(a0_tilde, a1_tilde, a2_tilde, &hasher, v_star_0,
                         bfSSS_add(v_star_1, u_star_0), u_star_1);

  aligned_free(w_tag);
}

void ublock_256_verifier(const params_t* params, uint8_t* a0_tilde, const uint8_t* d,
                         uint8_t** Q, const uint8_t* owf_in, const uint8_t* owf_out,
                         const uint8_t* chall_2, const uint8_t* chall_3,
                         const uint8_t* a1_tilde, const uint8_t* a2_tilde) {
  const unsigned int ell = params->ell;
  const bfSSS_t bf_delta = bfSSS_load(chall_3);

  bfSSS_t* q_key = column_to_row_major_and_shrink_V_SSS(Q, ell);

  const bfSSS_t q_star_0 = bfSSS_sum_poly(q_key + ell);
  const bfSSS_t q_star_1 = bfSSS_sum_poly(q_key + ell + UBLOCKITH_LAMBDA);
  const bfSSS_t q_star = bfSSS_add(q_star_0, bfSSS_mul(q_star_1, bf_delta));

  for (unsigned int i = 0; i < ell; ++i) {
    q_key[i] = bfSSS_add(q_key[i], bfSSS_mul_bit(bf_delta, ptr_get_bit(d, i)));
  }

  zk_hash_SSS_ctx hasher;
  zk_hash_SSS_init(&hasher, chall_2);
  ublock_SSS_constraints_verifier(params, &hasher, q_key, owf_in, owf_out, bf_delta);
  aligned_free(q_key);

  uint8_t q_tilde[UBLOCKITH_LAMBDA / 8u];
  zk_hash_SSS_finalize(q_tilde, &hasher, q_star);

  const bfSSS_t bf_delta_sq = bfSSS_mul(bf_delta, bf_delta);
  bfSSS_t a0 = bfSSS_load(q_tilde);
  a0 = bfSSS_add(a0, bfSSS_mul(bfSSS_load(a1_tilde), bf_delta));
  a0 = bfSSS_add(a0, bfSSS_mul(bfSSS_load(a2_tilde), bf_delta_sq));
  bfSSS_store(a0_tilde, a0);
}

static void ublock_SSS_constraints_prover(const params_t* params, zk_hash_SSS_3_ctx* hasher,
                                          const uint8_t* w, const bfSSS_t* w_tag,
                                          const uint8_t* owf_in, const uint8_t* owf_out) {
  const bool use_em = is_em(params);
  const unsigned int lke_bits = params->lke;
  const unsigned int lke_bytes = lke_bits / 8u;

  assert(params->ell >= lke_bits + UBLOCK_ENC_W_BITS);
  ublock_hash_keyspace_reduction_prover(hasher, w, w_tag);

  if (use_em) {
    assert(lke_bits == 0);
  } else {
    assert(lke_bits >= UBLOCK_KEYEXP_W_BITS);
  }

  uint8_t* k_bar = malloc(UBLOCK_KBAR_BITS / 8u);
  bfSSS_t* k_bar_tag = BFSSS_ALLOC(UBLOCK_KBAR_BITS);
  uint8_t* o_enc = malloc(UBLOCK_ENC_O_BITS / 8u);
  bfSSS_t* o_enc_deg0 = BFSSS_ALLOC(UBLOCK_ENC_O_BITS);
  bfSSS_t* o_enc_deg1 = BFSSS_ALLOC(UBLOCK_ENC_O_BITS);
  bfSSS_t* o_enc_deg2 = BFSSS_ALLOC(UBLOCK_ENC_O_BITS);
  assert(k_bar);
  assert(k_bar_tag);
  assert(o_enc);
  assert(o_enc_deg0);
  assert(o_enc_deg1);
  assert(o_enc_deg2);

  if (use_em) {
    uint8_t in[UBLOCK_BLOCK];
    uint8_t out[UBLOCK_BLOCK];
    bfSSS_t in_tag[UBLOCK_BLOCK_BITS];
    bfSSS_t out_tag[UBLOCK_BLOCK_BITS];

    memcpy(in, w, UBLOCK_BLOCK);
    memcpy(in_tag, w_tag, sizeof(in_tag));
    xor_u8_array(in, owf_out, out, UBLOCK_BLOCK);
    memcpy(out_tag, in_tag, sizeof(out_tag));

    ublock_build_round_keys_bytes(k_bar, owf_in);
    constant_to_vole_SSS_prover(k_bar_tag, UBLOCK_KBAR_BITS);

    ublock_SSS_enc_constraints_prover(o_enc, o_enc_deg0, o_enc_deg1, o_enc_deg2, in, in_tag, out,
                                      out_tag, w + lke_bytes, w_tag + lke_bits, k_bar, k_bar_tag);
    ublock_hash_deg3_constraints(hasher, o_enc_deg0, o_enc_deg1, o_enc_deg2, UBLOCK_ENC_O_BITS);
  } else {
    uint8_t in[UBLOCK_BLOCK];
    uint8_t out[UBLOCK_BLOCK];
    bfSSS_t in_tag[UBLOCK_BLOCK_BITS];
    bfSSS_t out_tag[UBLOCK_BLOCK_BITS];
    uint8_t o_ke[UBLOCK_KEYEXP_O_BITS / 8u];
    bfSSS_t* o_ke_deg0 = BFSSS_ALLOC(UBLOCK_KEYEXP_O_BITS);
    bfSSS_t* o_ke_deg1 = BFSSS_ALLOC(UBLOCK_KEYEXP_O_BITS);
    bfSSS_t* o_ke_deg2 = BFSSS_ALLOC(UBLOCK_KEYEXP_O_BITS);
    assert(o_ke_deg0);
    assert(o_ke_deg1);
    assert(o_ke_deg2);

    memcpy(in, owf_in, UBLOCK_BLOCK);
    memcpy(out, owf_out, UBLOCK_BLOCK);
    constant_to_vole_SSS_prover(in_tag, UBLOCK_BLOCK_BITS);
    constant_to_vole_SSS_prover(out_tag, UBLOCK_BLOCK_BITS);

    ublock_SSS_expkey_constraints_prover(k_bar, k_bar_tag, o_ke, o_ke_deg0, o_ke_deg1, o_ke_deg2,
                                         w, w_tag);
    ublock_hash_deg3_constraints(hasher, o_ke_deg0, o_ke_deg1, o_ke_deg2, UBLOCK_KEYEXP_O_BITS);
    ublock_SSS_enc_constraints_prover(o_enc, o_enc_deg0, o_enc_deg1, o_enc_deg2, in, in_tag, out,
                                      out_tag, w + lke_bytes, w_tag + lke_bits, k_bar, k_bar_tag);
    ublock_hash_deg3_constraints(hasher, o_enc_deg0, o_enc_deg1, o_enc_deg2, UBLOCK_ENC_O_BITS);

    aligned_free(o_ke_deg2);
    aligned_free(o_ke_deg1);
    aligned_free(o_ke_deg0);
  }

  aligned_free(o_enc_deg2);
  aligned_free(o_enc_deg1);
  aligned_free(o_enc_deg0);
  free(o_enc);
  aligned_free(k_bar_tag);
  free(k_bar);
}

static void ublock_SSS_constraints_verifier(const params_t* params, zk_hash_SSS_ctx* hasher,
                                            const bfSSS_t* w_key, const uint8_t* owf_in,
                                            const uint8_t* owf_out, bfSSS_t delta) {
  const bool use_em = is_em(params);
  const unsigned int lke_bits = params->lke;

  assert(params->ell >= lke_bits + UBLOCK_ENC_W_BITS);
  ublock_hash_keyspace_reduction_verifier(hasher, w_key);

  if (use_em) {
    assert(lke_bits == 0);
  } else {
    assert(lke_bits >= UBLOCK_KEYEXP_W_BITS);
  }

  bfSSS_t* o_enc_key = BFSSS_ALLOC(UBLOCK_ENC_O_BITS);
  assert(o_enc_key);

  if (use_em) {
    bfSSS_t in_key[UBLOCK_BLOCK_BITS];
    bfSSS_t out_key[UBLOCK_BLOCK_BITS];
    bfSSS_t* k_bar_key = BFSSS_ALLOC(UBLOCK_KBAR_BITS);
    uint8_t* k_bar = malloc(UBLOCK_KBAR_BITS / 8u);
    assert(k_bar_key);
    assert(k_bar);

    memcpy(in_key, w_key, sizeof(in_key));
    for (unsigned int i = 0; i < UBLOCK_BLOCK_BITS; ++i) {
      out_key[i] = bfSSS_add(w_key[i], bfSSS_mul_bit(delta, ptr_get_bit(owf_out, i)));
    }

    ublock_build_round_keys_bytes(k_bar, owf_in);
    constant_to_vole_SSS_verifier(k_bar_key, k_bar, delta, UBLOCK_KBAR_BITS);

    ublock_SSS_enc_constraints_verifier(o_enc_key, in_key, out_key, w_key + lke_bits,
                                        k_bar_key, delta);
    ublock_hash_deg1_constraints(hasher, o_enc_key, UBLOCK_ENC_O_BITS);

    free(k_bar);
    aligned_free(k_bar_key);
  } else {
    bfSSS_t in_key[UBLOCK_BLOCK_BITS];
    bfSSS_t out_key[UBLOCK_BLOCK_BITS];
    bfSSS_t* k_bar_key = BFSSS_ALLOC(UBLOCK_KBAR_BITS);
    bfSSS_t* o_ke_key = BFSSS_ALLOC(UBLOCK_KEYEXP_O_BITS);
    assert(k_bar_key);
    assert(o_ke_key);

    constant_to_vole_SSS_verifier(in_key, owf_in, delta, UBLOCK_BLOCK_BITS);
    constant_to_vole_SSS_verifier(out_key, owf_out, delta, UBLOCK_BLOCK_BITS);

    ublock_SSS_expkey_constraints_verifier(k_bar_key, o_ke_key, w_key, delta);
    ublock_hash_deg1_constraints(hasher, o_ke_key, UBLOCK_KEYEXP_O_BITS);
    ublock_SSS_enc_constraints_verifier(o_enc_key, in_key, out_key, w_key + lke_bits, k_bar_key,
                                        delta);
    ublock_hash_deg1_constraints(hasher, o_enc_key, UBLOCK_ENC_O_BITS);

    aligned_free(o_ke_key);
    aligned_free(k_bar_key);
  }

  aligned_free(o_enc_key);
}

static bfSSS_t* column_to_row_major_and_shrink_V_SSS(uint8_t** v, unsigned int ell) {
  const unsigned int rows = ell + 2u * UBLOCKITH_LAMBDA;

  bfSSS_t* new_v = BFSSS_ALLOC(rows);
  assert(new_v);

  unsigned int row = 0;
  for (; row + 8u <= rows; row += 8u) {
    const unsigned int row_byte = row >> 3;
    uint8_t packed_rows[8][UBLOCKITH_LAMBDA / 8u];

    for (unsigned int byte_col = 0; byte_col < (UBLOCKITH_LAMBDA / 8u); ++byte_col) {
      const unsigned int c = byte_col * 8u;
      uint64_t block = (uint64_t)v[c + 0u][row_byte];
      block |= (uint64_t)v[c + 1u][row_byte] << 8;
      block |= (uint64_t)v[c + 2u][row_byte] << 16;
      block |= (uint64_t)v[c + 3u][row_byte] << 24;
      block |= (uint64_t)v[c + 4u][row_byte] << 32;
      block |= (uint64_t)v[c + 5u][row_byte] << 40;
      block |= (uint64_t)v[c + 6u][row_byte] << 48;
      block |= (uint64_t)v[c + 7u][row_byte] << 56;

      const uint64_t t = bit_transpose8x8_u64(block);
      packed_rows[0][byte_col] = (uint8_t)(t);
      packed_rows[1][byte_col] = (uint8_t)(t >> 8);
      packed_rows[2][byte_col] = (uint8_t)(t >> 16);
      packed_rows[3][byte_col] = (uint8_t)(t >> 24);
      packed_rows[4][byte_col] = (uint8_t)(t >> 32);
      packed_rows[5][byte_col] = (uint8_t)(t >> 40);
      packed_rows[6][byte_col] = (uint8_t)(t >> 48);
      packed_rows[7][byte_col] = (uint8_t)(t >> 56);
    }

    new_v[row + 0u] = bfSSS_load(packed_rows[0]);
    new_v[row + 1u] = bfSSS_load(packed_rows[1]);
    new_v[row + 2u] = bfSSS_load(packed_rows[2]);
    new_v[row + 3u] = bfSSS_load(packed_rows[3]);
    new_v[row + 4u] = bfSSS_load(packed_rows[4]);
    new_v[row + 5u] = bfSSS_load(packed_rows[5]);
    new_v[row + 6u] = bfSSS_load(packed_rows[6]);
    new_v[row + 7u] = bfSSS_load(packed_rows[7]);
  }

  for (; row != rows; ++row) {
    uint8_t new_row[UBLOCKITH_LAMBDA / 8u] = {0};
    const unsigned int row_byte = row >> 3;
    const uint8_t mask = (uint8_t)(1u << (row & 7u));

    for (unsigned int out_b = 0; out_b < (UBLOCKITH_LAMBDA / 8u); ++out_b) {
      const unsigned int col0 = out_b * 8u;
      uint8_t b = 0;
      b |= (uint8_t)((v[col0 + 0u][row_byte] & mask) ? (1u << 0) : 0u);
      b |= (uint8_t)((v[col0 + 1u][row_byte] & mask) ? (1u << 1) : 0u);
      b |= (uint8_t)((v[col0 + 2u][row_byte] & mask) ? (1u << 2) : 0u);
      b |= (uint8_t)((v[col0 + 3u][row_byte] & mask) ? (1u << 3) : 0u);
      b |= (uint8_t)((v[col0 + 4u][row_byte] & mask) ? (1u << 4) : 0u);
      b |= (uint8_t)((v[col0 + 5u][row_byte] & mask) ? (1u << 5) : 0u);
      b |= (uint8_t)((v[col0 + 6u][row_byte] & mask) ? (1u << 6) : 0u);
      b |= (uint8_t)((v[col0 + 7u][row_byte] & mask) ? (1u << 7) : 0u);
      new_row[out_b] = b;
    }
    new_v[row] = bfSSS_load(new_row);
  }
  return new_v;
}

static void constant_to_vole_SSS_prover(bfSSS_t* tag, unsigned int n) {
  memset(tag, 0, n * sizeof(bfSSS_t));
}

static void constant_to_vole_SSS_verifier(bfSSS_t* key, const uint8_t* val, bfSSS_t delta,
                                          unsigned int n) {
  for (unsigned int i = 0; i < n; ++i) {
    key[i] = bfSSS_mul_bit(delta, ptr_get_bit(val, i));
  }
}

static void ublock_build_round_keys_bytes(uint8_t* k_bar, const uint8_t* key) {
  ublock256_key_t ks;
  const int ret = ublock256_set_key(key, &ks);
  assert(ret == 0);
  (void)ret;

  for (unsigned int r = 0; r <= UBLOCK_ROUNDS; ++r) {
    for (unsigned int w = 0; w < 8; ++w) {
      store_u32_be(ks.rk[r][w], k_bar + 32u * r + 4u * w);
    }
  }
}

static inline void ublock_hash_SSS_3_update_inline(zk_hash_SSS_3_ctx* ctx, bfSSS_t v0, bfSSS_t v1,
                                                   bfSSS_t v2) {
  ctx->h0[0] = bfSSS_add(bfSSS_mul(ctx->h0[0], ctx->s), v0);
  ctx->h1[0] = bfSSS_add(bf256_mul_64(ctx->h1[0], ctx->t), v0);
  ctx->h0[1] = bfSSS_add(bfSSS_mul(ctx->h0[1], ctx->s), v1);
  ctx->h1[1] = bfSSS_add(bf256_mul_64(ctx->h1[1], ctx->t), v1);
  ctx->h0[2] = bfSSS_add(bfSSS_mul(ctx->h0[2], ctx->s), v2);
  ctx->h1[2] = bfSSS_add(bf256_mul_64(ctx->h1[2], ctx->t), v2);
}

static inline void ublock_hash_SSS_update_inline(zk_hash_SSS_ctx* ctx, bfSSS_t v) {
  ctx->h0 = bfSSS_add(bfSSS_mul(ctx->h0, ctx->s), v);
  ctx->h1 = bfSSS_add(bf256_mul_64(ctx->h1, ctx->t), v);
}

static void ublock_hash_deg3_constraints(zk_hash_SSS_3_ctx* hasher, const bfSSS_t* deg0,
                                         const bfSSS_t* deg1, const bfSSS_t* deg2,
                                         unsigned int n_bits) {
  for (unsigned int i = 0; i < n_bits; ++i) {
    ublock_hash_SSS_3_update_inline(hasher, deg0[i], deg1[i], deg2[i]);
  }
}

static void ublock_hash_deg1_constraints(zk_hash_SSS_ctx* hasher, const bfSSS_t* key,
                                         unsigned int n_bits) {
  for (unsigned int i = 0; i < n_bits; ++i) {
    ublock_hash_SSS_update_inline(hasher, key[i]);
  }
}
