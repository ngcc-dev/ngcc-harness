#include "sm4th_sm4_128.h"
#include "fields.h"
#include "macros.h"
#include "vole.h"
#include "utils.h"
#include "universal_hashing.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#define N_BLOCK 128
#define SM4TH_LAMBDA 128

#define bfSSS_t bf128_t
#define bfSSS_load bf128_load
#define bfSSS_from_bit bf128_from_bit
#define bfSSS_store bf128_store
#define bfSSS_zero bf128_zero
#define bfSSS_one bf128_one
#define bfSSS_add bf128_add
#define bfSSS_mul bf128_mul
#define bfSSS_sqr(x) bfSSS_mul((x), (x))
#define bfSSS_mul_bit bf128_mul_bit
#define bfSSS_byte_combine bf128_byte_combine_sm4
#define bfSSS_byte_combine_bits bf128_byte_combine_bits_sm4
#define bfSSS_byte_combine_sq bf128_byte_combine_sq_sm4
#define bfSSS_byte_combine_bits_sq bf128_byte_combine_bits_sq_sm4
#define bfSSS_sq_bit_inplace bf128_sq_bit_inplace_sm4
#define bfSSS_sum_poly bf128_sum_poly
#define bfSSS_sum_poly_bits bf128_sum_poly_bits
#define BFSSS_NUM_BYTES BF128_NUM_BYTES
#define BFSSS_ALIGN BF128_ALIGN
#define PAD_TO(s, a) (((s) + (a) - 1) & ~((a) - 1))
#define BFSSS_ALLOC(s) aligned_alloc(BFSSS_ALIGN, PAD_TO((s) * sizeof(bfSSS_t), BFSSS_ALIGN))

#define zk_hash_SSS_3_ctx zk_hash_128_2_ctx
#define zk_hash_SSS_3_finalize zk_hash_128_2_finalize
#define zk_hash_SSS_3_init zk_hash_128_2_init
#define zk_hash_SSS_3_update zk_hash_128_2_update
#define zk_hash_SSS_ctx zk_hash_128_ctx
#define zk_hash_SSS_finalize zk_hash_128_finalize
#define zk_hash_SSS_init zk_hash_128_init
#define zk_hash_SSS_update zk_hash_128_update



/* ------------------------------------------------------------------ */
/* Forward declarations                                                  */
/* ------------------------------------------------------------------ */
static void column_to_row_major_and_shrink_V_SSS(bfSSS_t* new_v, uint8_t** v, unsigned int ell);
static void constant_to_vole_SSS_prover(bfSSS_t* tag, unsigned int n);
static void constant_to_vole_SSS_verifier(bfSSS_t* key, const uint8_t* val, bfSSS_t delta,
                                          unsigned int n);

/* Prover-side tag for any public constant is always zero.
 * Use this shared zero array instead of calling constant_to_vole_SSS_prover every round. */
static const bfSSS_t ZERO_TAG_8[8];  /* zero-initialised by C */
static bfSSS_t g_sm4_byte_combine_bits_lut[256];
static bfSSS_t g_sm4_byte_combine_bits_sq_lut[256];
static int g_sm4_byte_combine_bits_lut_ready = 0;
static void sm4_SSS_constraints_prover(const params_t* params, zk_hash_SSS_3_ctx* hasher,
                                       const uint8_t* w, const bfSSS_t* w_tag,
                                       const uint8_t* owf_in, const uint8_t* owf_out);
static void sm4_SSS_expkey_constraints_prover(const params_t* params, zk_hash_SSS_3_ctx* hasher,
                                              const uint8_t* w, const bfSSS_t* w_tag);
static void sm4_SSS_enc_constraints_prover(const params_t* params, zk_hash_SSS_3_ctx* hasher,
                                           const uint8_t* in, const bfSSS_t* in_tag,
                                           const uint8_t* out, const bfSSS_t* out_tag,
                                           const uint8_t* w, const bfSSS_t* w_tag,
                                           const uint8_t* k, const bfSSS_t* k_tag,
                                           int k_tag_is_zero);
static void sm4_SSS_T_prime_affine_prover(const params_t* params,
                                          const uint8_t* w_in, const bfSSS_t* w_in_tag,
                                          const uint8_t* x_out, const bfSSS_t* x_out_tag,
                                          const uint32_t ck_value);
static void sm4_SSS_T_prime_inv_affine_prover(const params_t* params,
                                              const uint8_t* w_in, const bfSSS_t* w_in_tag,
                                              const uint8_t* x_out, const bfSSS_t* x_out_tag);
static void sm4_SSS_T_affine_prover(const params_t* params,
                                    const uint8_t* w_in, const bfSSS_t* w_in_tag,
                                    const uint8_t* k_in, const bfSSS_t* k_in_tag,
                                    const uint8_t* x_out, const bfSSS_t* x_out_tag);
static void sm4_SSS_T_affine_prover_keytag_zero(const params_t* params,
                                                const uint8_t* w_in, const bfSSS_t* w_in_tag,
                                                const uint8_t* k_in, const uint8_t* x_out,
                                                const bfSSS_t* x_out_tag);
static void sm4_SSS_T_inv_affine_prover(const params_t* params,
                                        const uint8_t* w_in, const bfSSS_t* w_in_tag,
                                        const uint8_t* y_out, const bfSSS_t* y_out_tag);
static void sm4_SSS_constraints_verifier(const params_t* params, zk_hash_SSS_ctx* hasher,
                                         const bfSSS_t* w_key, const uint8_t* owf_in,
                                         const uint8_t* owf_out, bfSSS_t delta);
static void sm4_SSS_expkey_constraints_verifier(const params_t* params, zk_hash_SSS_ctx* hasher,
                                                const bfSSS_t* w_key, const bfSSS_t delta);
static void sm4_SSS_enc_constraints_verifier(const params_t* params, zk_hash_SSS_ctx* hasher,
                                             const bfSSS_t* in_key, const bfSSS_t* out_key,
                                             const bfSSS_t* w_key, const bfSSS_t* k_key,
                                             const bfSSS_t delta);
static void sm4_SSS_T_prime_affine_verifier(const bfSSS_t* w_in_key, bfSSS_t* x_out_key,
                                            bfSSS_t delta, uint32_t ck_value,
                                            const bfSSS_t* affine_const_key);
static void sm4_SSS_T_prime_inv_affine_verifier(const bfSSS_t* w_in_key,
                                                bfSSS_t* y_out_key, bfSSS_t delta,
                                                const bfSSS_t* inv_affine_const_key);
static void sm4_SSS_T_affine_verifier(const bfSSS_t* w_in_key, const bfSSS_t* k_in_key,
                                      bfSSS_t* x_out_key, bfSSS_t delta,
                                      const bfSSS_t* affine_const_key);
static void sm4_SSS_T_inv_affine_verifier(const bfSSS_t* w_in_key,
                                          bfSSS_t* y_out_key, bfSSS_t delta,
                                          const bfSSS_t* inv_affine_const_key);

static void sm4_byte_combine_bits_lut_ensure(void);
static inline bfSSS_t sm4_byte_combine_bits_lut(uint8_t x);
static inline bfSSS_t sm4_byte_combine_bits_sq_lut(uint8_t x);

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

static void sm4_byte_combine_bits_lut_ensure(void) {
  if (g_sm4_byte_combine_bits_lut_ready) {
    return;
  }
  for (unsigned int i = 0; i < 256u; ++i) {
    g_sm4_byte_combine_bits_lut[i] = bfSSS_byte_combine_bits((uint8_t)i);
    g_sm4_byte_combine_bits_sq_lut[i] = bfSSS_byte_combine_bits_sq((uint8_t)i);
  }
  g_sm4_byte_combine_bits_lut_ready = 1;
}

static inline bfSSS_t sm4_byte_combine_bits_lut(uint8_t x) {
  return g_sm4_byte_combine_bits_lut[x];
}

static inline bfSSS_t sm4_byte_combine_bits_sq_lut(uint8_t x) {
  return g_sm4_byte_combine_bits_sq_lut[x];
}

// OWF PROVER
// clang-format off
void sm4_128_prover(const params_t* params, uint8_t* a0_tilde, uint8_t* a1_tilde,
                    const uint8_t* w, const uint8_t* u, uint8_t** V,
                    const uint8_t* owf_in, const uint8_t* owf_out,
                    const uint8_t* chall_2) {
  // clang-format on
  // const unsigned int ell = 1280;
  const unsigned int ell = params->ell;

  // ::1-5
  // V becomes the w_tag: ell + 2 * lambda field elements
  // w_tag: v1||v2, of lengths ell and 2 * lambda bits, respectively.
  // The former is used for witness masks, and the latter is used for zk mask.
  bfSSS_t w_tag[ell + 2 * SM4TH_LAMBDA];
  column_to_row_major_and_shrink_V_SSS(w_tag, V, ell); // This is the tag for w

  // ::6-7 embed VOLE masks
  // The input of u is of length 3 * lambda + B bits, but we only need 2 * lambda bits by using the bfSSS_sum_poly_bits function.
  // bfSSS_sum_poly_bits: ToField u's **first** lambda bits to a field element.
  bfSSS_t bf_u_star_0 = bfSSS_sum_poly_bits(u);
  // ToField u's **next** lambda bits to a field element.

  // ::8-9
  // bfSSS_sum_poly: ToField v's lambda ZK-mask bits to a field element.
  bfSSS_t bf_v_star_0 = bfSSS_sum_poly(w_tag + ell);
  // ToField v's next lambda ZK-mask bits to a field element.
  bfSSS_t bf_v_star_1 = bfSSS_sum_poly(w_tag + ell + SM4TH_LAMBDA);

  // Step: 13-18
  zk_hash_SSS_3_ctx hasher;
  zk_hash_SSS_3_init(&hasher, chall_2);
  sm4_byte_combine_bits_lut_ensure();

  sm4_SSS_constraints_prover(params, &hasher, w, w_tag, owf_in, owf_out);

  // TAG: update to a0_tilde, a1_tilde, but it should still work as before due to the compatility of Deg2To3.
  zk_hash_SSS_3_finalize(a0_tilde, a1_tilde, &hasher, bf_v_star_0,
                         bfSSS_add(bf_v_star_1, bf_u_star_0));

}

// The explict input is w, w_tag, pk = (owf_in, owf_out), and params
// w length is ell / 8 bytes, in SM4th, ell = (36 + 28) * 32 = 2048 bits;
// so w[0..3] = k[0], w[4..7] = k[1], w[8..11] = k[2], w[12..15] = k[3], and w[16..36 * 4 - 1 = 143] are the witness for the key expansion
// w_tag length is ell bits, so w_tag[0..32] are the tags for k[0]
// The explict output is z and z_tag which should be zero
// for non-em, w[0..(36+28) *4 - 1 = 255], owf_in is X[0..3], owf_out is X[35..32]
// for em, w[0..31*4 - 1 = 127]; owf_input is sk_input, is SM4 key
// owf_key is sk_key, is SM4 input
// owf_output is SM4 output xor SM4 input
static void sm4_SSS_constraints_prover(const params_t* params, zk_hash_SSS_3_ctx* hasher,
                                       const uint8_t* w, const bfSSS_t* w_tag,
                                       const uint8_t* owf_in, const uint8_t* owf_out) {
  // const unsigned int lambda    = params->lambda;
  const unsigned int Lke       = params->lke;
  const unsigned int blocksize = SM4_BLOCK_SIZE * 8; // in bits
  const bool use_em     = is_em(params);

  // Secret-witness keyspace reduction: w[0] * w[1] = 0.
  zk_hash_SSS_3_update(hasher, bfSSS_mul(w_tag[0], w_tag[1]),
                       bfSSS_add(bfSSS_mul_bit(w_tag[0], ptr_get_bit(w, 1)),
                                 bfSSS_mul_bit(w_tag[1], ptr_get_bit(w, 0))));

  // ::6
  uint8_t in[SM4_BLOCK_SIZE];
  bfSSS_t in_tag[SM4_BLOCK_SIZE * 8];
  uint8_t out[SM4_BLOCK_SIZE];
  bfSSS_t out_tag[SM4_BLOCK_SIZE * 8];
  
  if (use_em){
    uint8_t expand_keys[144];

    // ::8
    sm4_init_round_keys(expand_keys, owf_in);
    
    // ::10
    // copy w[0..3], w[4..7], w[8..11], w[12..15] and their tags to in[] and in_tag[]
    // in is SM4 input
    memcpy(in, w, 16);
    memcpy(in_tag, w_tag, 16 * 8 * sizeof(bfSSS_t));

    // ::11
    // obtain out and out_tag from owf_out and in
    // owf_out = SM4 output xor SM4 input = X[35..32] xor X[0..3]
    // so, SM4 output = owf_out xor in
    // as owf_out is constant, so tags of X[35..32] should be the same as w_tag[0..127]
    xor_u8_array(in, owf_out, out, 16);
    memcpy(out_tag, w_tag, 16 * 8 * sizeof(bfSSS_t));

    // ::18-19
    // only use enc witness of w
    sm4_SSS_enc_constraints_prover(params, hasher, 
                      // X[0..3], X[35], X[34], X[33], X[32]
                      in, in_tag, out, out_tag, 
                      // X[3],...,X[32], 1152 / 8 = 144
                      w + 16, w_tag + 16 * 8, 
                      // K[0] = w[0..3], so K[4] = w[16, 19], K[4..35]
                      expand_keys + 16, NULL, 1);

  } else {
    // jump to ::12 for SM4
    // constant_to_vole_SSS_prover: transform a constant to an VOLE relation, i.e.,
    // ::13
    memcpy(in, owf_in, blocksize / 8);
    constant_to_vole_SSS_prover(in_tag, blocksize);

    // ::14
    memcpy(out, owf_out, blocksize / 8);
    constant_to_vole_SSS_prover(out_tag, blocksize);

    // ::15-17
    // obtain constraints in the key expansion, and add them to the hasher
    sm4_SSS_expkey_constraints_prover(params, hasher, w, w_tag);

    // ::18-19
    // only use enc witness of w
    sm4_SSS_enc_constraints_prover(params, hasher, 
                      // X[0..3], X[35], X[34], X[33], X[32]
                      in, in_tag, out, out_tag, 
                      // X[3],...,X[32], 1152 / 8 = 144
                      w + Lke / 8, w_tag + Lke, 
                      // K[0] = w[0..3], so K[4] = w[16, 19], K[4..35]
                      w + 16, w_tag + 16 * 8, 0);
  }

}


// // KEY EXP CSTRNTS
static void sm4_SSS_expkey_constraints_prover(const params_t* params, zk_hash_SSS_3_ctx* hasher, 
                                              const uint8_t* w, const bfSSS_t* w_tag) {
  const unsigned int N_word_bytes = 4;
  const unsigned int N_word = N_word_bytes * 8;

  /* Per-round: compute x[4] and y[4] (4 S-box inputs/outputs), then immediately
   * combine and hash — avoids allocating Ske-sized arrays and a second full pass */
  uint8_t  x_word[4], y_word[4];
  bfSSS_t  x_word_tag[32], y_word_tag[32];

  for (unsigned int i = 0; i < SM4_ROUNDS; i++) {
    sm4_SSS_T_prime_affine_prover(params, w + i * N_word_bytes, w_tag + i * N_word,
                                  x_word, x_word_tag, CK[i]);
    sm4_SSS_T_prime_inv_affine_prover(params, w + i * N_word_bytes, w_tag + i * N_word,
                                      y_word, y_word_tag);

    for (unsigned int j = 0; j < 4; j++) {
      bfSSS_t x_hat    = sm4_byte_combine_bits_lut(x_word[j]);
      bfSSS_t x_hat_sq = sm4_byte_combine_bits_sq_lut(x_word[j]);
      bfSSS_t y_hat    = sm4_byte_combine_bits_lut(y_word[j]);
      bfSSS_t y_hat_sq = sm4_byte_combine_bits_sq_lut(y_word[j]);

      bfSSS_t x_hat_tag    = bfSSS_byte_combine(x_word_tag + j * 8);
      bfSSS_t x_hat_tag_sq = bfSSS_byte_combine_sq(x_word_tag + j * 8);
      bfSSS_t y_hat_tag    = bfSSS_byte_combine(y_word_tag + j * 8);
      bfSSS_t y_hat_tag_sq = bfSSS_byte_combine_sq(y_word_tag + j * 8);

      zk_hash_SSS_3_update(hasher, bfSSS_mul(x_hat_tag_sq, y_hat_tag),
                           bfSSS_add(bfSSS_add(bfSSS_mul(x_hat_sq, y_hat_tag),
                                               bfSSS_mul(x_hat_tag_sq, y_hat)),
                                     x_hat_tag));
      zk_hash_SSS_3_update(hasher, bfSSS_mul(x_hat_tag, y_hat_tag_sq),
                           bfSSS_add(bfSSS_add(bfSSS_mul(x_hat, y_hat_tag_sq),
                                               bfSSS_mul(x_hat_tag, y_hat_sq)),
                                     y_hat_tag));
    }
  }
}

// // Enc CSTRNTS
static void sm4_SSS_enc_constraints_prover(const params_t* params, zk_hash_SSS_3_ctx* hasher, 
                                              const uint8_t* in, const bfSSS_t* in_tag,
                                              const uint8_t* out, const bfSSS_t* out_tag,
                                              const uint8_t* w, const bfSSS_t* w_tag,
                                              const uint8_t* k, const bfSSS_t* k_tag,
                                              int k_tag_is_zero) {
  // k starts from k4; w: X[4..32]
  const unsigned int N_word_bytes = 4;
  const unsigned int N_word = N_word_bytes * 8;

  // Build w_flat = in || w[4..32] || out (reversed), and w_flat_tag similarly.
  // out is placed inversely: out[0..3]=X[35], out[4..7]=X[34], ..., out[12..15]=X[32]
  uint8_t  w_flat[(SM4_ROUNDS + 4) * 4];
  bfSSS_t w_flat_tag[(SM4_ROUNDS + 4) * 32];

  memcpy(w_flat, in, 4 * N_word_bytes);
  memcpy(w_flat_tag, in_tag, 4 * N_word * sizeof(bfSSS_t));
  for (unsigned int i = 0; i < 4; i++) {
    memcpy(w_flat     + (SM4_ROUNDS + i) * N_word_bytes, out     + (3 - i) * N_word_bytes, N_word_bytes);
    memcpy(w_flat_tag + (SM4_ROUNDS + i) * N_word,       out_tag + (3 - i) * N_word,       N_word * sizeof(bfSSS_t));
  }
  memcpy(w_flat     + 4 * N_word_bytes, w,     (SM4_ROUNDS - 4) * N_word_bytes);
  memcpy(w_flat_tag + 4 * N_word,       w_tag, (SM4_ROUNDS - 4) * N_word * sizeof(bfSSS_t));

  /* Per-round: compute x[4] and y[4], then immediately combine and hash */
  uint8_t  x_word[4], y_word[4];
  bfSSS_t  x_word_tag[32], y_word_tag[32];

  for (unsigned int i = 0; i < SM4_ROUNDS; i++) {
    if (k_tag_is_zero) {
      sm4_SSS_T_affine_prover_keytag_zero(params,
                                          w_flat + i * N_word_bytes, w_flat_tag + i * N_word,
                                          k + i * N_word_bytes, x_word, x_word_tag);
    } else {
      sm4_SSS_T_affine_prover(params,
                              w_flat + i * N_word_bytes, w_flat_tag + i * N_word,
                              k + i * N_word_bytes, k_tag + i * N_word,
                              x_word, x_word_tag);
    }
    sm4_SSS_T_inv_affine_prover(params,
                                w_flat + i * N_word_bytes, w_flat_tag + i * N_word,
                                y_word, y_word_tag);

    for (unsigned int j = 0; j < 4; j++) {
      bfSSS_t x_hat    = sm4_byte_combine_bits_lut(x_word[j]);
      bfSSS_t x_hat_sq = sm4_byte_combine_bits_sq_lut(x_word[j]);
      bfSSS_t y_hat    = sm4_byte_combine_bits_lut(y_word[j]);
      bfSSS_t y_hat_sq = sm4_byte_combine_bits_sq_lut(y_word[j]);

      bfSSS_t x_hat_tag    = bfSSS_byte_combine(x_word_tag + j * 8);
      bfSSS_t x_hat_tag_sq = bfSSS_byte_combine_sq(x_word_tag + j * 8);
      bfSSS_t y_hat_tag    = bfSSS_byte_combine(y_word_tag + j * 8);
      bfSSS_t y_hat_tag_sq = bfSSS_byte_combine_sq(y_word_tag + j * 8);

      zk_hash_SSS_3_update(hasher, bfSSS_mul(x_hat_tag_sq, y_hat_tag),
                           bfSSS_add(bfSSS_add(bfSSS_mul(x_hat_sq, y_hat_tag),
                                               bfSSS_mul(x_hat_tag_sq, y_hat)),
                                     x_hat_tag));
      zk_hash_SSS_3_update(hasher, bfSSS_mul(x_hat_tag, y_hat_tag_sq),
                           bfSSS_add(bfSSS_add(bfSSS_mul(x_hat, y_hat_tag_sq),
                                               bfSSS_mul(x_hat_tag, y_hat_sq)),
                                     y_hat_tag));
    }
  }

}

/* ================================================================ */
/*  Helper: Constant to VOLE (for constants like CK)                */
/* ================================================================ */
static void constant_to_vole_SSS_prover(bfSSS_t* tag, unsigned int n) {
  // For constant values, the tag is zero (no VOLE component)
  memset(tag, 0, sizeof(bfSSS_t) * n);
}

static void sm4_linear_byte_SSS(bfSSS_t* out, const bfSSS_t* in, const bfSSS_t* constant,
                                const uint8_t row_masks[8]) {
  bfSSS_t tmp[8];

  for (unsigned int row = 0; row != 8u; ++row) {
    bfSSS_t acc = constant ? constant[row] : bfSSS_zero();
    const uint8_t mask = row_masks[row];
    for (unsigned int bit = 0; bit != 8u; ++bit) {
      if ((mask >> bit) & 1u) {
        acc = bfSSS_add(acc, in[bit]);
      }
    }
    tmp[row] = acc;
  }

  memcpy(out, tmp, sizeof(tmp));
}

static void sm4_affine_byte_SSS(bfSSS_t* out, const bfSSS_t* in, const bfSSS_t* constant) {
  static const uint8_t row_masks[8] = {
      0xA7u, 0x4Fu, 0x9Eu, 0x3Du, 0x7Au, 0xF4u, 0xE9u, 0xD3u};
  sm4_linear_byte_SSS(out, in, constant, row_masks);
}

static void sm4_inv_affine_byte_SSS(bfSSS_t* out, const bfSSS_t* in,
                                    const bfSSS_t* constant) {
  static const uint8_t row_masks[8] = {
      0x43u, 0x86u, 0x0Du, 0x1Au, 0x34u, 0x68u, 0xD0u, 0xA1u};
  sm4_linear_byte_SSS(out, in, constant, row_masks);
}

static void sm4_L_inv_word_SSS(bfSSS_t* out, const bfSSS_t* in, const uint8_t* shifts,
                               unsigned int shift_count) {
  bfSSS_t tmp[32];

  for (unsigned int out_bit = 0; out_bit != 32u; ++out_bit) {
    unsigned int src_bit = (out_bit + 32u - shifts[0]) & 31u;
    bfSSS_t acc = in[sm4_word_tag_index(src_bit)];
    for (unsigned int i = 1u; i != shift_count; ++i) {
      src_bit = (out_bit + 32u - shifts[i]) & 31u;
      acc = bfSSS_add(acc, in[sm4_word_tag_index(src_bit)]);
    }
    tmp[out_bit] = acc;
  }

  for (unsigned int out_bit = 0; out_bit != 32u; ++out_bit) {
    out[sm4_word_tag_index(out_bit)] = tmp[out_bit];
  }
}

static void SM4_L_prime_inv_bytes_SSS(bfSSS_t* out, const bfSSS_t* in) {
  static const uint8_t shifts[13] = {0u, 2u, 4u, 8u, 11u, 12u, 14u,
                                     17u, 22u, 23u, 24u, 30u, 31u};
  sm4_L_inv_word_SSS(out, in, shifts, 13u);
}

static void SM4_L_inv_bytes_SSS(bfSSS_t* out, const bfSSS_t* in) {
  static const uint8_t shifts[11] = {0u, 2u, 4u, 8u, 12u, 14u, 16u, 18u, 22u, 24u, 30u};
  sm4_L_inv_word_SSS(out, in, shifts, 11u);
}

/* ================================================================ */
/*  SM4th.KeyExpCstrnts - T'.Affine and T'.InvAffine Implementation */
/* ================================================================ */

/**
 * SM4th.KeyExpCstrnts - T'.Affine Prover
**/
static void sm4_SSS_T_prime_affine_prover(const params_t* params, 
                                          const uint8_t* w_in, const bfSSS_t* w_in_tag,
                                          const uint8_t* x_out, const bfSSS_t* x_out_tag,
                                          const uint32_t ck_value) {
  (void)params;

  // derive k1, k2, k3 as const pointers into w_in / w_in_tag
  const uint8_t*  k1     = w_in     +  4;   // w_in[4..7]
  const uint8_t*  k2     = w_in     +  8;   // w_in[8..11]
  const uint8_t*  k3     = w_in     + 12;   // w_in[12..15]
  const bfSSS_t*  k1_tag = w_in_tag + 32;
  const bfSSS_t*  k2_tag = w_in_tag + 64;
  const bfSSS_t*  k3_tag = w_in_tag + 96;

  // public constant CK (big-endian bytes)
  uint8_t ck[4];
  ck[0] = (uint8_t)(ck_value >> 24);
  ck[1] = (uint8_t)(ck_value >> 16);
  ck[2] = (uint8_t)(ck_value >>  8);
  ck[3] = (uint8_t)(ck_value);

  for (unsigned int j = 0; j < 4; j++) {
    ((uint8_t*)x_out)[j] = sm4_affine_byte(k1[j] ^ k2[j] ^ k3[j] ^ ck[j]);
  }

  // xor_sum_tag for each byte j, then affine byte tag — compute inline
  for (unsigned int j = 0; j < 4; j++) {
    bfSSS_t xor_sum_tag[8];
    for (unsigned int i = 0; i < 8; ++i) {
      xor_sum_tag[i] = bfSSS_add(k1_tag[j * 8 + i],
                       bfSSS_add(k2_tag[j * 8 + i], k3_tag[j * 8 + i]));
    }
    // ck_tag == 0 (public constant) — pass ZERO_TAG_8
    sm4_affine_byte_SSS((bfSSS_t*)x_out_tag + j * 8, xor_sum_tag, ZERO_TAG_8);
  }
}

/**
 * SM4th.KeyExpCstrnts - T'.InvAffine Prover
**/
static void sm4_SSS_T_prime_inv_affine_prover(const params_t* params, 
                                          const uint8_t* w_in, const bfSSS_t* w_in_tag,
                                          const uint8_t* x_out, const bfSSS_t* x_out_tag) {
  (void)params;

  const uint8_t*  k0     = w_in;
  const uint8_t*  k4     = w_in     + 16;
  const bfSSS_t*  k0_tag = w_in_tag +  0;
  const bfSSS_t*  k4_tag = w_in_tag + 128;

  // XOR k0 ^ k4 directly into output; apply L'_inv in-place
  uint8_t  tmp[4];
  bfSSS_t  tmp_tag[32];

  for (unsigned int j = 0; j < 4; j++) {
    tmp[j] = k0[j] ^ k4[j];
    tmp_tag[j*8+0] = bfSSS_add(k0_tag[j*8+0], k4_tag[j*8+0]);
    tmp_tag[j*8+1] = bfSSS_add(k0_tag[j*8+1], k4_tag[j*8+1]);
    tmp_tag[j*8+2] = bfSSS_add(k0_tag[j*8+2], k4_tag[j*8+2]);
    tmp_tag[j*8+3] = bfSSS_add(k0_tag[j*8+3], k4_tag[j*8+3]);
    tmp_tag[j*8+4] = bfSSS_add(k0_tag[j*8+4], k4_tag[j*8+4]);
    tmp_tag[j*8+5] = bfSSS_add(k0_tag[j*8+5], k4_tag[j*8+5]);
    tmp_tag[j*8+6] = bfSSS_add(k0_tag[j*8+6], k4_tag[j*8+6]);
    tmp_tag[j*8+7] = bfSSS_add(k0_tag[j*8+7], k4_tag[j*8+7]);
  }

  SM4_L_prime_inv_bytes(tmp, tmp);
  SM4_L_prime_inv_bytes_SSS(tmp_tag, tmp_tag);

  for (unsigned int j = 0; j < 4; j++) {
    ((uint8_t*)x_out)[j] = sm4_inv_affine_byte(tmp[j]);
    sm4_inv_affine_byte_SSS((bfSSS_t*)x_out_tag + j * 8, tmp_tag + j * 8, ZERO_TAG_8);
  }
}

/**
 * SM4th.EncCstrnts - T.Affine Prover
 * input: w_in starts from X0
 * output: k_in starts from k4
**/
static void sm4_SSS_T_affine_prover(const params_t* params, 
                                          const uint8_t* w_in, const bfSSS_t* w_in_tag,
                                          const uint8_t* k_in, const bfSSS_t* k_in_tag,
                                          const uint8_t* x_out, const bfSSS_t* x_out_tag) {
  (void)params;

  const uint8_t*  X1     = w_in     +  4;
  const uint8_t*  X2     = w_in     +  8;
  const uint8_t*  X3     = w_in     + 12;
  const uint8_t*  k4     = k_in;
  const bfSSS_t*  X1_tag = w_in_tag + 32;
  const bfSSS_t*  X2_tag = w_in_tag + 64;
  const bfSSS_t*  X3_tag = w_in_tag + 96;
  const bfSSS_t*  k4_tag = k_in_tag;

  for (unsigned int j = 0; j < 4; j++) {
    ((uint8_t*)x_out)[j] = sm4_affine_byte(X1[j] ^ X2[j] ^ X3[j] ^ k4[j]);
  }

  for (unsigned int j = 0; j < 4; j++) {
    bfSSS_t xor_sum_tag[8];
    for (unsigned int i = 0; i < 8; ++i) {
      xor_sum_tag[i] = bfSSS_add(bfSSS_add(X1_tag[j * 8 + i], X2_tag[j * 8 + i]),
                                 bfSSS_add(X3_tag[j * 8 + i], k4_tag[j * 8 + i]));
    }
    sm4_affine_byte_SSS((bfSSS_t*)x_out_tag + j * 8, xor_sum_tag, ZERO_TAG_8);
  }
}

/**
 * EM prover fastpath: k tags are all zero (public round keys),
 * so skip adding k4_tag in xor_sum_tag.
**/
static void sm4_SSS_T_affine_prover_keytag_zero(const params_t* params,
                                                const uint8_t* w_in, const bfSSS_t* w_in_tag,
                                                const uint8_t* k_in, const uint8_t* x_out,
                                                const bfSSS_t* x_out_tag) {
  (void)params;

  const uint8_t*  X1     = w_in     +  4;
  const uint8_t*  X2     = w_in     +  8;
  const uint8_t*  X3     = w_in     + 12;
  const uint8_t*  k4     = k_in;
  const bfSSS_t*  X1_tag = w_in_tag + 32;
  const bfSSS_t*  X2_tag = w_in_tag + 64;
  const bfSSS_t*  X3_tag = w_in_tag + 96;

  for (unsigned int j = 0; j < 4; j++) {
    ((uint8_t*)x_out)[j] = sm4_affine_byte(X1[j] ^ X2[j] ^ X3[j] ^ k4[j]);
  }

  for (unsigned int j = 0; j < 4; j++) {
    bfSSS_t xor_sum_tag[8];
    for (unsigned int i = 0; i < 8; ++i) {
      xor_sum_tag[i] = bfSSS_add(bfSSS_add(X1_tag[j * 8 + i], X2_tag[j * 8 + i]),
                                 X3_tag[j * 8 + i]);
    }
    sm4_affine_byte_SSS((bfSSS_t*)x_out_tag + j * 8, xor_sum_tag, ZERO_TAG_8);
  }
}

/**
 * SM4th.EncCstrnts - T.InvAffine Prover
 * input: w_in starts from X0
**/
static void sm4_SSS_T_inv_affine_prover(const params_t* params, 
                                          const uint8_t* w_in, const bfSSS_t* w_in_tag,
                                          const uint8_t* y_out, const bfSSS_t* y_out_tag) {
  (void)params;

  const uint8_t*  X0     = w_in;
  const uint8_t*  X4     = w_in     + 16;
  const bfSSS_t*  X0_tag = w_in_tag +  0;
  const bfSSS_t*  X4_tag = w_in_tag + 128;

  uint8_t  tmp[4];
  bfSSS_t  tmp_tag[32];

  for (unsigned int j = 0; j < 4; j++) {
    tmp[j] = X0[j] ^ X4[j];
    tmp_tag[j*8+0] = bfSSS_add(X0_tag[j*8+0], X4_tag[j*8+0]);
    tmp_tag[j*8+1] = bfSSS_add(X0_tag[j*8+1], X4_tag[j*8+1]);
    tmp_tag[j*8+2] = bfSSS_add(X0_tag[j*8+2], X4_tag[j*8+2]);
    tmp_tag[j*8+3] = bfSSS_add(X0_tag[j*8+3], X4_tag[j*8+3]);
    tmp_tag[j*8+4] = bfSSS_add(X0_tag[j*8+4], X4_tag[j*8+4]);
    tmp_tag[j*8+5] = bfSSS_add(X0_tag[j*8+5], X4_tag[j*8+5]);
    tmp_tag[j*8+6] = bfSSS_add(X0_tag[j*8+6], X4_tag[j*8+6]);
    tmp_tag[j*8+7] = bfSSS_add(X0_tag[j*8+7], X4_tag[j*8+7]);
  }

  SM4_L_inv_bytes(tmp, tmp);
  SM4_L_inv_bytes_SSS(tmp_tag, tmp_tag);

  for (unsigned int j = 0; j < 4; j++) {
    ((uint8_t*)y_out)[j] = sm4_inv_affine_byte(tmp[j]);
    sm4_inv_affine_byte_SSS((bfSSS_t*)y_out_tag + j * 8, tmp_tag + j * 8, ZERO_TAG_8);
  }
}

/* SM4th functions verifier */

/**
 * SM4th.KeyExpCstrnts - T'.Affine Verifier
 * input:  w_in_key starts from k[i]  (k[i+1], k[i+2], k[i+3] at offsets 32, 64, 96)
 * input:  ck_value  = CK[i] (public round constant, big-endian uint32)
 * output: x_out_key (32 elements = one 4-byte word in key space)
 **/
static void sm4_SSS_T_prime_affine_verifier(const bfSSS_t* w_in_key, bfSSS_t* x_out_key,
                                            bfSSS_t delta, uint32_t ck_value,
                                            const bfSSS_t* affine_const_key) {

  // derive k1_key, k2_key, k3_key as const pointers into w_in_key
  const bfSSS_t* k1_key = w_in_key + 32;   // k[i+1], 32 field elements
  const bfSSS_t* k2_key = w_in_key + 64;   // k[i+2]
  const bfSSS_t* k3_key = w_in_key + 96;   // k[i+3]

  // public constant CK in big-endian bytes; its VOLE key = delta * bit
  uint8_t ck[4];
  ck[0] = (uint8_t)(ck_value >> 24);
  ck[1] = (uint8_t)(ck_value >> 16);
  ck[2] = (uint8_t)(ck_value >>  8);
  ck[3] = (uint8_t)(ck_value);

  bfSSS_t tmp_key[32];

  for (unsigned int j = 0; j < 4; j++) {
    bfSSS_t xor_sum_key[8];
    for (unsigned int b = 0; b < 8; b++) {
      bfSSS_t ck_key_b = bfSSS_mul_bit(delta, (ck[j] >> b) & 1);
      xor_sum_key[b] = bfSSS_add(bfSSS_add(k1_key[j * 8 + b], k2_key[j * 8 + b]),
                                 bfSSS_add(k3_key[j * 8 + b], ck_key_b));
    }
    sm4_affine_byte_SSS(tmp_key + j * 8, xor_sum_key, affine_const_key);
  }

  memcpy(x_out_key, tmp_key, 32 * sizeof(bfSSS_t));
}

/**
 * SM4th.KeyExpCstrnts - T'.InvAffine Verifier
 * input:  w_in_key starts from k[i]  (k[i+4] at offset 128)
 * output: y_out_key (32 elements = one 4-byte word in key space)
 **/
static void sm4_SSS_T_prime_inv_affine_verifier(const bfSSS_t* w_in_key,
                                                bfSSS_t* y_out_key, bfSSS_t delta,
                                                const bfSSS_t* inv_affine_const_key) {
  // delta is unused in the body (affine const key is precomputed by the caller)
  (void)delta;
  // derive k0_key, k4_key as const pointers into w_in_key
  const bfSSS_t* k0_key = w_in_key;          // k[i],   32 field elements
  const bfSSS_t* k4_key = w_in_key + 128;    // k[i+4], 32 field elements

  // temporary mutable buffer (L'_inv operates in-place)
  bfSSS_t tmp_key[32];

  for (unsigned int b = 0; b < 32; b++) {
    tmp_key[b] = bfSSS_add(k0_key[b], k4_key[b]);
  }

  SM4_L_prime_inv_bytes_SSS(tmp_key, tmp_key);

  for (unsigned int j = 0; j < 4; j++) {
    sm4_inv_affine_byte_SSS(tmp_key + j * 8, tmp_key + j * 8, inv_affine_const_key);
  }

  memcpy(y_out_key, tmp_key, 32 * sizeof(bfSSS_t));
}

/**
 * SM4th.EncCstrnts - T.Affine Verifier
 * input:  w_in_key starts from X[i]  (w_in_tag layout: word0=X[i], word1=X[i+1], word2=X[i+2], word3=X[i+3])
 * input:  k_in_key starts from K[4+i]
 * output: x_out_key (32 elements = one 4-byte word in key space)
 **/
static void sm4_SSS_T_affine_verifier(const bfSSS_t* w_in_key, const bfSSS_t* k_in_key,
                                      bfSSS_t* x_out_key, bfSSS_t delta,
                                      const bfSSS_t* affine_const_key) {
  // delta is unused in the body (affine const key is precomputed by the caller)
  (void)delta;
  // derive X1_key, X2_key, X3_key from w_in_key
  const bfSSS_t* X1_key = w_in_key + 32;
  const bfSSS_t* X2_key = w_in_key + 64;
  const bfSSS_t* X3_key = w_in_key + 96;

  // derive k4_key from k_in_key
  const bfSSS_t* k4_key = k_in_key;        // K[4+i], 32 field elements

  // define temporary output
  bfSSS_t tmp_key[32];

  for (unsigned int j = 0; j < 4; j++) {
    // compute XOR sum key: X1_key[j] + X2_key[j] + X3_key[j] + k4_key[j]  (byte j)
    bfSSS_t xor_sum_key[8];
    for (unsigned int i = 0; i < 8; ++i) {
      xor_sum_key[i] = bfSSS_add(bfSSS_add(X1_key[j * 8 + i], X2_key[j * 8 + i]),
                                 bfSSS_add(X3_key[j * 8 + i], k4_key[j * 8 + i]));
    }
    // apply SM4 affine matrix key transform with precomputed constant key
    sm4_affine_byte_SSS(tmp_key + j * 8, xor_sum_key, affine_const_key);
  }

  memcpy(x_out_key, tmp_key, 32 * sizeof(bfSSS_t));
}

/**
 * SM4th.EncCstrnts - T.InvAffine Verifier
 * input:  w_in_key starts from X[i]  (must also span X[i+4]: w_in_key[128..159])
 * output: y_out_key (32 elements = one 4-byte word in key space)
 **/
static void sm4_SSS_T_inv_affine_verifier(const bfSSS_t* w_in_key,
                                          bfSSS_t* y_out_key, bfSSS_t delta,
                                          const bfSSS_t* inv_affine_const_key) {
  // delta is unused in the body (affine const key is precomputed by the caller)
  (void)delta;
  // derive X0_key, X4_key from w_in_key
  const bfSSS_t* X0_key = w_in_key;          // X[i],   32 field elements
  const bfSSS_t* X4_key = w_in_key + 128;    // X[i+4], 32 field elements

  // define temporary output
  bfSSS_t tmp_key[32];

  for (unsigned int j = 0; j < 4; j++) {
    for (unsigned int i = 0; i < 8; ++i) {
      tmp_key[j * 8 + i] = bfSSS_add(X0_key[j * 8 + i], X4_key[j * 8 + i]);
    }
  }

  // apply L_inv linear transform on keys
  SM4_L_inv_bytes_SSS(tmp_key, tmp_key);

  for (unsigned int j = 0; j < 4; j++) {
    sm4_inv_affine_byte_SSS(tmp_key + j * 8, tmp_key + j * 8, inv_affine_const_key);
  }

  memcpy(y_out_key, tmp_key, 32 * sizeof(bfSSS_t));
}

// OWF VERIFIER
// clang-format off
void sm4_128_verifier(const params_t* params, uint8_t* a0_tilde, const uint8_t* d,
                      uint8_t** Q, const uint8_t* owf_in, const uint8_t* owf_out,
                      const uint8_t* chall_2, const uint8_t* chall_3,
                      const uint8_t* a1_tilde) {
                        
  const unsigned int ell = params->ell;

  // ::1
  bfSSS_t bf_delta    = bfSSS_load(chall_3);

  // ::2-6
  bfSSS_t q_key[ell + 2 * SM4TH_LAMBDA];
  column_to_row_major_and_shrink_V_SSS(q_key, Q, ell);

  // ::7-9
  // q_star = q_star_0 + delta * q_star_1.
  bfSSS_t q_star_0 = bfSSS_sum_poly(q_key + ell);
  bfSSS_t q_star_1 = bfSSS_sum_poly(q_key + ell + SM4TH_LAMBDA);
  bfSSS_t q_star   = bfSSS_add(q_star_0, bfSSS_mul(q_star_1, bf_delta));

  // ::13-14
  zk_hash_SSS_ctx b_ctx;
  zk_hash_SSS_init(&b_ctx, chall_2);

  for (unsigned int i = 0; i < ell; i++) {
    q_key[i] = bfSSS_add(q_key[i], bfSSS_mul_bit(bf_delta, ptr_get_bit(d, i)));
  }

  // ::11-12
  sm4_SSS_constraints_verifier(params, &b_ctx, q_key, owf_in, owf_out, bf_delta);
  // ::13-14
  uint8_t q_tilde[SM4TH_LAMBDA / 8];
  zk_hash_SSS_finalize(q_tilde, &b_ctx, q_star);

  // ::16
  bfSSS_t tmp1 = bfSSS_mul(bfSSS_load(a1_tilde), bf_delta);
  bfSSS_t ret  = bfSSS_add(bfSSS_load(q_tilde), tmp1);

  bfSSS_store(a0_tilde, ret);
}

// OWF CONSTRAINTS VERIFIER

static void sm4_SSS_constraints_verifier(const params_t* params, zk_hash_SSS_ctx* hasher,
                                         const bfSSS_t* w_key, const uint8_t* owf_in,
                                         const uint8_t* owf_out, bfSSS_t delta) {

  const unsigned int Lke      = params->lke;
  const unsigned int blocksize = SM4_BLOCK_SIZE * 8; // in bits
  const bool use_em     = is_em(params);

  // Secret-witness keyspace reduction: w[0] * w[1] = 0.
  zk_hash_SSS_update(hasher, bfSSS_mul(w_key[0], w_key[1]));

  // ::6
  bfSSS_t in_key[SM4_BLOCK_SIZE * 8];
  bfSSS_t out_key[SM4_BLOCK_SIZE * 8];

  if (use_em) {
    // EM mode: round keys are public (derived from owf_in), witness is SM4 input
    uint8_t expand_keys[144];
    bfSSS_t expand_keys_key[144 * 8];

    // ::8 compute public round keys
    sm4_init_round_keys(expand_keys, owf_in);
    // ::9 convert public constants to VOLE keys: key[i] = delta * bit_i(val)
    constant_to_vole_SSS_verifier(expand_keys_key, expand_keys, delta, 144 * 8);

    // ::10 in_key = w_key (SM4 input = witness)
    memcpy(in_key, w_key, blocksize * sizeof(bfSSS_t));

    // ::11 out_key = w_key XOR owf_out (owf_out is public constant)
    // owf_out = SM4(owf_in, w) XOR w, so SM4 output = owf_out XOR in
    for (unsigned int i = 0; i < blocksize; i++) {
      out_key[i] = bfSSS_add(w_key[i], bfSSS_mul_bit(delta, ptr_get_bit(owf_out, i)));
    }

    // ::18-19 enc constraints only (no key expansion constraints in EM mode)
    sm4_SSS_enc_constraints_verifier(params, hasher,
                                     in_key, out_key,
                                     w_key + blocksize,               // X[4..31] keys
                                     expand_keys_key + blocksize,     // K[4..35] keys (public)
                                     delta);

  } else {
    // Standard SM4: owf_in/owf_out are public constants
    // ::13
    constant_to_vole_SSS_verifier(in_key, owf_in, delta, blocksize);
    // ::14
    constant_to_vole_SSS_verifier(out_key, owf_out, delta, blocksize);

    // ::15-17 key expansion constraints
    sm4_SSS_expkey_constraints_verifier(params, hasher, w_key, delta);

    // ::18-19 enc constraints
    // w_key layout: [k0..k3 | k4..k35 | X4..X31 | X35..X32]
    // k0..k3 at w_key[0..127], k4..k35 at w_key[128..1151], X4..X31 at w_key[Lke..]
    sm4_SSS_enc_constraints_verifier(params, hasher,
                                     in_key, out_key,
                                     w_key + Lke,        // X[4..31] keys
                                     w_key + blocksize,  // K[4..35] keys = w_key[128..]
                                     delta);
  }

}

// KEY EXPANSION CONSTRAINTS VERIFIER
static void sm4_SSS_expkey_constraints_verifier(const params_t* params, zk_hash_SSS_ctx* hasher,
                                                const bfSSS_t* w_key, const bfSSS_t delta) {
  (void)params;
  const unsigned int N_word = 4 * 8; // 32 bits per 4-byte word

  // Precompute affine constant keys once for all 32 rounds
  bfSSS_t affine_const_key[8];
  bfSSS_t inv_affine_const_key[8];
  constant_to_vole_SSS_verifier(affine_const_key,     (uint8_t[]){SM4_AFFINE_CONST},     delta, 8);
  constant_to_vole_SSS_verifier(inv_affine_const_key, (uint8_t[]){SM4_INV_AFFINE_CONST}, delta, 8);

  /* Per-round: compute x_word_key and y_word_key (4 S-box in/out), then immediately
   * combine and hash — avoids allocating Ske-sized arrays and a second full pass */
  bfSSS_t x_word_key[32], y_word_key[32];

  for (unsigned int i = 0; i < SM4_ROUNDS; i++) {
    const bfSSS_t* w_word_key = w_key + i * N_word;

    sm4_SSS_T_prime_affine_verifier(w_word_key, x_word_key, delta, CK[i], affine_const_key);
    sm4_SSS_T_prime_inv_affine_verifier(w_word_key, y_word_key, delta, inv_affine_const_key);

    for (unsigned int j = 0; j < 4; j++) {
      bfSSS_t x_hat_key    = bfSSS_byte_combine(x_word_key + j * 8);
      bfSSS_t x_hat_key_sq = bfSSS_byte_combine_sq(x_word_key + j * 8);
      bfSSS_t y_hat_key    = bfSSS_byte_combine(y_word_key + j * 8);
      bfSSS_t y_hat_key_sq = bfSSS_byte_combine_sq(y_word_key + j * 8);

      zk_hash_SSS_update(hasher,
          bfSSS_add(bfSSS_mul(x_hat_key_sq, y_hat_key),
                    bfSSS_mul(delta, x_hat_key)));
      zk_hash_SSS_update(hasher,
          bfSSS_add(bfSSS_mul(x_hat_key, y_hat_key_sq),
                    bfSSS_mul(delta, y_hat_key)));
    }
  }
}

// // Enc CSTRNTS VERIFIER
static void sm4_SSS_enc_constraints_verifier(const params_t* params, zk_hash_SSS_ctx* hasher,
                                             const bfSSS_t* in_key, const bfSSS_t* out_key,
                                             const bfSSS_t* w_key, const bfSSS_t* k_key,
                                             const bfSSS_t delta) {
  (void)params;
  const unsigned int N_word = 4 * 8; // 32 bits per word

  // Build w_flat_key = in_key || w_key[X4..X31] || out_key (reversed)
  bfSSS_t w_flat_key[(SM4_ROUNDS + 4) * N_word];

  memcpy(w_flat_key, in_key, 4 * N_word * sizeof(bfSSS_t));
  for (unsigned int i = 0; i < 4; i++)
    memcpy(w_flat_key + (SM4_ROUNDS + i) * N_word, out_key + (3 - i) * N_word, N_word * sizeof(bfSSS_t));
  memcpy(w_flat_key + 4 * N_word, w_key, (SM4_ROUNDS - 4) * N_word * sizeof(bfSSS_t));

  // Precompute affine constant keys once for all 32 rounds
  bfSSS_t affine_const_key[8];
  bfSSS_t inv_affine_const_key[8];
  constant_to_vole_SSS_verifier(affine_const_key,     (uint8_t[]){SM4_AFFINE_CONST},     delta, 8);
  constant_to_vole_SSS_verifier(inv_affine_const_key, (uint8_t[]){SM4_INV_AFFINE_CONST}, delta, 8);

  /* Per-round: compute x_word_key and y_word_key, then immediately combine and hash */
  bfSSS_t x_word_key[32], y_word_key[32];

  for (unsigned int i = 0; i < SM4_ROUNDS; i++) {
    const bfSSS_t* w_word_key = w_flat_key + i * N_word;
    const bfSSS_t* k_word_key = k_key + i * N_word;

    sm4_SSS_T_affine_verifier(w_word_key, k_word_key, x_word_key, delta, affine_const_key);
    sm4_SSS_T_inv_affine_verifier(w_word_key, y_word_key, delta, inv_affine_const_key);

    for (unsigned int j = 0; j < 4; j++) {
      bfSSS_t x_hat_key    = bfSSS_byte_combine(x_word_key + j * 8);
      bfSSS_t x_hat_key_sq = bfSSS_byte_combine_sq(x_word_key + j * 8);
      bfSSS_t y_hat_key    = bfSSS_byte_combine(y_word_key + j * 8);
      bfSSS_t y_hat_key_sq = bfSSS_byte_combine_sq(y_word_key + j * 8);

      zk_hash_SSS_update(hasher,
          bfSSS_add(bfSSS_mul(x_hat_key_sq, y_hat_key),
                    bfSSS_mul(delta, x_hat_key)));
      zk_hash_SSS_update(hasher,
          bfSSS_add(bfSSS_mul(x_hat_key, y_hat_key_sq),
                    bfSSS_mul(delta, y_hat_key)));
    }
  }

}


// COLOUM TO ROW MAJOR
static void column_to_row_major_and_shrink_V_SSS(bfSSS_t* new_v, uint8_t** v, unsigned int ell) {
  // V is \hat \ell times \lambda matrix over F_2
  // v has \hat \ell rows, \lambda columns, storing in column-major order, new_v has \ell +
  // 2 * \lambda rows and \lambda columns storing in row-major order
  const unsigned int rows = ell + 2 * SM4TH_LAMBDA;
  unsigned int row = 0;

  for (; row + 8u <= rows; row += 8u) {
    const unsigned int row_byte = row >> 3;
    uint8_t packed_rows[8][BFSSS_NUM_BYTES];

    for (unsigned int byte_col = 0; byte_col != BFSSS_NUM_BYTES; ++byte_col) {
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
    uint8_t new_row[BFSSS_NUM_BYTES] = {0};
    const unsigned int row_byte = row >> 3;
    const uint8_t row_mask      = (uint8_t)(1u << (row & 7u));
    for (unsigned int byte_col = 0; byte_col != BFSSS_NUM_BYTES; ++byte_col) {
      const unsigned int c = byte_col * 8u;
      uint8_t b = 0;
      b |= (uint8_t)(((v[c + 0u][row_byte] & row_mask) != 0u) << 0);
      b |= (uint8_t)(((v[c + 1u][row_byte] & row_mask) != 0u) << 1);
      b |= (uint8_t)(((v[c + 2u][row_byte] & row_mask) != 0u) << 2);
      b |= (uint8_t)(((v[c + 3u][row_byte] & row_mask) != 0u) << 3);
      b |= (uint8_t)(((v[c + 4u][row_byte] & row_mask) != 0u) << 4);
      b |= (uint8_t)(((v[c + 5u][row_byte] & row_mask) != 0u) << 5);
      b |= (uint8_t)(((v[c + 6u][row_byte] & row_mask) != 0u) << 6);
      b |= (uint8_t)(((v[c + 7u][row_byte] & row_mask) != 0u) << 7);
      new_row[byte_col] = b;
    }
    new_v[row] = bfSSS_load(new_row);
  }
}

static void constant_to_vole_SSS_verifier(bfSSS_t* key, const uint8_t* val, bfSSS_t delta,
                                          unsigned int n) {
  for (unsigned int i = 0; i < n; i++) {
    key[i] = bfSSS_mul_bit(delta, ptr_get_bit(val, i));
  }
}
