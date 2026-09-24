#include "vistrutith_vistrutah_512.h"

#include "../fields.h"
#include "../macros.h"
#include "../universal_hashing.h"
#include "../utils.h"
#include "vistrutah.h"
#include "vistrutith_constraints.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define VISTRUTITH_LAMBDA 512u
#define VISTRUTITH_BLOCK_BITS (VISTRUTAH_512_BLOCK_SIZE * 8u)

#define bfSSS_t bf512_t
#define bfSSS_load bf512_load
#define bfSSS_store bf512_store
#define bfSSS_zero bf512_zero
#define bfSSS_add bf512_add
#define bfSSS_mul bf512_mul
#define bfSSS_mul_bit bf512_mul_bit
#define bfSSS_sum_poly bf512_sum_poly
#define bfSSS_sum_poly_bits bf512_sum_poly_bits

#define zk_hash_SSS_3_ctx zk_hash_512_3_ctx
#define zk_hash_SSS_3_init zk_hash_512_3_init
#define zk_hash_SSS_3_update zk_hash_512_3_update
#define zk_hash_SSS_3_finalize zk_hash_512_3_finalize
#define zk_hash_SSS_ctx zk_hash_512_ctx
#define zk_hash_SSS_init zk_hash_512_init
#define zk_hash_SSS_update zk_hash_512_update
#define zk_hash_SSS_finalize zk_hash_512_finalize

static bfSSS_t* column_to_row_major_and_shrink_V_SSS(uint8_t** v, unsigned int ell);
static void constant_to_vole_SSS_prover(bfSSS_t* tag, unsigned int n);
static void constant_to_vole_SSS_verifier(bfSSS_t* key, const uint8_t* val, bfSSS_t delta,
                                          unsigned int n);
static void vistrutith_SSS_constraints_prover(const params_t* params, zk_hash_SSS_3_ctx* hasher,
                                              const uint8_t* w, const bfSSS_t* w_tag,
                                              const uint8_t* owf_in, const uint8_t* owf_out);
static void vistrutith_SSS_constraints_verifier(const params_t* params, zk_hash_SSS_ctx* hasher,
                                                const bfSSS_t* w_key, const uint8_t* owf_in,
                                                const uint8_t* owf_out, bfSSS_t delta);

static inline void vistrutith_hash_keyspace_reduction_prover(zk_hash_SSS_3_ctx* hasher,
                                                             const uint8_t* w,
                                                             const bfSSS_t* w_tag) {
  zk_hash_SSS_3_update(
      hasher, bfSSS_mul(w_tag[0], w_tag[1]),
      bfSSS_add(bfSSS_mul_bit(w_tag[0], ptr_get_bit(w, 1)),
                bfSSS_mul_bit(w_tag[1], ptr_get_bit(w, 0))),
      bfSSS_zero());
}

static inline void vistrutith_hash_keyspace_reduction_verifier(zk_hash_SSS_ctx* hasher,
                                                               const bfSSS_t* w_key) {
  zk_hash_SSS_update(hasher, bfSSS_mul(w_key[0], w_key[1]));
}

_Static_assert(VISTRUTITH_ENC_CSTRNTS_NORM_LEN == VISTRUTITH_ENC_CSTRNTS_IO_LEN,
               "enc constraints triplets must have equal length");

void vistrutith_512_prover(const params_t* params, uint8_t* a0_tilde, uint8_t* a1_tilde,
                           uint8_t* a2_tilde, const uint8_t* w, const uint8_t* u, uint8_t** V,
                           const uint8_t* owf_in, const uint8_t* owf_out,
                           const uint8_t* chall_2) {
  const unsigned int ell = params->ell;

  bfSSS_t* w_tag = column_to_row_major_and_shrink_V_SSS(V, ell);
  assert(w_tag);

  const bfSSS_t u_star_0 = bfSSS_sum_poly_bits(u);
  const bfSSS_t u_star_1 = bfSSS_sum_poly_bits(u + (VISTRUTITH_LAMBDA / 8u));
  const bfSSS_t v_star_0 = bfSSS_sum_poly(w_tag + ell);
  const bfSSS_t v_star_1 = bfSSS_sum_poly(w_tag + ell + VISTRUTITH_LAMBDA);

  zk_hash_SSS_3_ctx hasher;
  zk_hash_SSS_3_init(&hasher, chall_2);
  vistrutith_SSS_constraints_prover(params, &hasher, w, w_tag, owf_in, owf_out);
  zk_hash_SSS_3_finalize(a0_tilde, a1_tilde, a2_tilde, &hasher, v_star_0,
                         bfSSS_add(v_star_1, u_star_0), u_star_1);

  free(w_tag);
}

void vistrutith_512_verifier(const params_t* params, uint8_t* a0_tilde, const uint8_t* d,
                             uint8_t** Q, const uint8_t* owf_in, const uint8_t* owf_out,
                             const uint8_t* chall_2, const uint8_t* chall_3,
                             const uint8_t* a1_tilde, const uint8_t* a2_tilde) {
  const unsigned int ell = params->ell;
  const bfSSS_t bf_delta = bfSSS_load(chall_3);

  bfSSS_t* q_key = column_to_row_major_and_shrink_V_SSS(Q, ell);
  assert(q_key);

  const bfSSS_t q_star_0 = bfSSS_sum_poly(q_key + ell);
  const bfSSS_t q_star_1 = bfSSS_sum_poly(q_key + ell + VISTRUTITH_LAMBDA);
  const bfSSS_t q_star = bfSSS_add(q_star_0, bfSSS_mul(q_star_1, bf_delta));

  for (unsigned int i = 0; i < ell; ++i) {
    q_key[i] = bfSSS_add(q_key[i], bfSSS_mul_bit(bf_delta, ptr_get_bit(d, i)));
  }

  zk_hash_SSS_ctx hasher;
  zk_hash_SSS_init(&hasher, chall_2);
  vistrutith_SSS_constraints_verifier(params, &hasher, q_key, owf_in, owf_out, bf_delta);
  free(q_key);

  uint8_t q_tilde[VISTRUTITH_LAMBDA / 8u];
  zk_hash_SSS_finalize(q_tilde, &hasher, q_star);

  const bfSSS_t bf_delta_sq = bfSSS_mul(bf_delta, bf_delta);
  bfSSS_t a0 = bfSSS_load(q_tilde);
  a0 = bfSSS_add(a0, bfSSS_mul(bfSSS_load(a1_tilde), bf_delta));
  a0 = bfSSS_add(a0, bfSSS_mul(bfSSS_load(a2_tilde), bf_delta_sq));
  bfSSS_store(a0_tilde, a0);
}

static void vistrutith_SSS_constraints_prover(const params_t* params, zk_hash_SSS_3_ctx* hasher,
                                              const uint8_t* w, const bfSSS_t* w_tag,
                                              const uint8_t* owf_in, const uint8_t* owf_out) {
  (void)params;

  bfSSS_t in_tag[VISTRUTITH_BLOCK_BITS];
  bfSSS_t out_tag[VISTRUTITH_BLOCK_BITS];
  bfSSS_t z_norm_val[VISTRUTITH_ENC_CSTRNTS_NORM_LEN];
  bfSSS_t z_norm_tag[VISTRUTITH_ENC_CSTRNTS_NORM_LEN];
  bfSSS_t z_io0_val[VISTRUTITH_ENC_CSTRNTS_IO_LEN];
  bfSSS_t z_io0_tag[VISTRUTITH_ENC_CSTRNTS_IO_LEN];
  bfSSS_t z_io1_val[VISTRUTITH_ENC_CSTRNTS_IO_LEN];
  bfSSS_t z_io1_tag[VISTRUTITH_ENC_CSTRNTS_IO_LEN];

  constant_to_vole_SSS_prover(in_tag, VISTRUTITH_BLOCK_BITS);
  constant_to_vole_SSS_prover(out_tag, VISTRUTITH_BLOCK_BITS);
  vistrutith_hash_keyspace_reduction_prover(hasher, w, w_tag);

  vistrutith_enc_constraints_prover(
      z_norm_val, z_norm_tag, z_io0_val, z_io0_tag, z_io1_val, z_io1_tag, owf_in, in_tag,
      owf_out, out_tag, w, w_tag);

  for (unsigned int i = 0; i < VISTRUTITH_ENC_CSTRNTS_NORM_LEN; ++i) {
    zk_hash_SSS_3_update(hasher, z_norm_tag[i], z_io0_tag[i], z_io1_tag[i]);
  }
}

static void vistrutith_SSS_constraints_verifier(const params_t* params, zk_hash_SSS_ctx* hasher,
                                                const bfSSS_t* w_key, const uint8_t* owf_in,
                                                const uint8_t* owf_out, bfSSS_t delta) {
  (void)params;

  bfSSS_t in_key[VISTRUTITH_BLOCK_BITS];
  bfSSS_t out_key[VISTRUTITH_BLOCK_BITS];
  bfSSS_t z_norm_key[VISTRUTITH_ENC_CSTRNTS_NORM_LEN];
  bfSSS_t z_io0_key[VISTRUTITH_ENC_CSTRNTS_IO_LEN];
  bfSSS_t z_io1_key[VISTRUTITH_ENC_CSTRNTS_IO_LEN];

  constant_to_vole_SSS_verifier(in_key, owf_in, delta, VISTRUTITH_BLOCK_BITS);
  constant_to_vole_SSS_verifier(out_key, owf_out, delta, VISTRUTITH_BLOCK_BITS);
  vistrutith_hash_keyspace_reduction_verifier(hasher, w_key);

  vistrutith_enc_constraints_verifier(z_norm_key, z_io0_key, z_io1_key, in_key, out_key, w_key);

  const bfSSS_t delta_sq = bfSSS_mul(delta, delta);
  for (unsigned int i = 0; i < VISTRUTITH_ENC_CSTRNTS_NORM_LEN; ++i) {
    const bfSSS_t acc =
        bfSSS_add(z_norm_key[i], bfSSS_add(bfSSS_mul(delta, z_io0_key[i]),
                                           bfSSS_mul(delta_sq, z_io1_key[i])));
    zk_hash_SSS_update(hasher, acc);
  }
}

static bfSSS_t* column_to_row_major_and_shrink_V_SSS(uint8_t** v, unsigned int ell) {
  bfSSS_t* new_v = malloc((size_t)(ell + 2u * VISTRUTITH_LAMBDA) * sizeof(*new_v));
  assert(new_v);

  for (unsigned int row = 0; row < ell + 2u * VISTRUTITH_LAMBDA; ++row) {
    uint8_t new_row[VISTRUTITH_LAMBDA / 8u];
    const unsigned int byte_idx = row >> 3;
    const uint8_t mask = (uint8_t)(1u << (row & 7u));

    for (unsigned int out_b = 0; out_b < (VISTRUTITH_LAMBDA / 8u); ++out_b) {
      const unsigned int col0 = out_b * 8u;
      uint8_t b = 0;
      b |= (uint8_t)((v[col0 + 0u][byte_idx] & mask) ? (1u << 0) : 0u);
      b |= (uint8_t)((v[col0 + 1u][byte_idx] & mask) ? (1u << 1) : 0u);
      b |= (uint8_t)((v[col0 + 2u][byte_idx] & mask) ? (1u << 2) : 0u);
      b |= (uint8_t)((v[col0 + 3u][byte_idx] & mask) ? (1u << 3) : 0u);
      b |= (uint8_t)((v[col0 + 4u][byte_idx] & mask) ? (1u << 4) : 0u);
      b |= (uint8_t)((v[col0 + 5u][byte_idx] & mask) ? (1u << 5) : 0u);
      b |= (uint8_t)((v[col0 + 6u][byte_idx] & mask) ? (1u << 6) : 0u);
      b |= (uint8_t)((v[col0 + 7u][byte_idx] & mask) ? (1u << 7) : 0u);
      new_row[out_b] = b;
    }
    new_v[row] = bfSSS_load(new_row);
  }
  return new_v;
}

static void constant_to_vole_SSS_prover(bfSSS_t* tag, unsigned int n) {
  memset(tag, 0, n * sizeof(*tag));
}

static void constant_to_vole_SSS_verifier(bfSSS_t* key, const uint8_t* val, bfSSS_t delta,
                                          unsigned int n) {
  for (unsigned int i = 0; i < n; ++i) {
    key[i] = bfSSS_mul_bit(delta, ptr_get_bit(val, i));
  }
}
