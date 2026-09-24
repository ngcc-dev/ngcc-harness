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
#include <stdbool.h>

#define SM4_OWF_N_BLOCK SM4_OWF_BLOCK_BITS
#define N_BLOCK PRG_BLOCK_BITS
#define SM4TH_LAMBDA 160
#define SM4TH_SSS_BF160 1

#define bfSSS_t bf160_t
#define bfSSS_load bf160_load
#define bfSSS_from_bit bf160_from_bit
#define bfSSS_store bf160_store
#define bfSSS_zero bf160_zero
#define bfSSS_one bf160_one
#define bfSSS_add bf160_add
#define bfSSS_mul bf160_mul
#define bfSSS_sqr bf160_sqr
#define bfSSS_mul_bit bf160_mul_bit
#define bfSSS_byte_combine bf160_byte_combine_sm4
#define bfSSS_byte_combine_bits bf160_byte_combine_bits_sm4
#define bfSSS_byte_combine_sq bf160_byte_combine_sq_sm4
#define bfSSS_byte_combine_bits_sq bf160_byte_combine_bits_sq_sm4
#define bfSSS_sq_bit_inplace bf160_sq_bit_inplace_sm4
#define bfSSS_sum_poly bf160_sum_poly
#define bfSSS_sum_poly_bits bf160_sum_poly_bits
#define BFSSS_NUM_BYTES BF160_NUM_BYTES
#define BFSSS_ALIGN BF160_ALIGN
#define PAD_TO(s, a) (((s) + (a) - 1) & ~((a) - 1))
#define BFSSS_ALLOC(s) aligned_alloc(BFSSS_ALIGN, PAD_TO((s) * sizeof(bfSSS_t), BFSSS_ALIGN))

#define zk_hash_SSS_3_ctx zk_hash_160_3_ctx
#define zk_hash_SSS_3_finalize zk_hash_160_3_finalize
#define zk_hash_SSS_3_init zk_hash_160_3_init
#define zk_hash_SSS_3_update zk_hash_160_3_update
#define zk_hash_SSS_ctx zk_hash_160_ctx
#define zk_hash_SSS_finalize zk_hash_160_finalize
#define zk_hash_SSS_init zk_hash_160_init
#define zk_hash_SSS_update zk_hash_160_update

/* ------------------------------------------------------------------ */
/* Forward declarations                                                  */
/* ------------------------------------------------------------------ */
static void column_to_row_major_and_shrink_V_SSS(bfSSS_t* new_v, uint8_t** v, unsigned int ell);
static void constant_to_vole_SSS_prover(bfSSS_t* tag, unsigned int n);
static void constant_to_vole_SSS_verifier(bfSSS_t* key, const uint8_t* val, bfSSS_t delta,
                                          unsigned int n);

static bfSSS_t g_sm4_byte_combine_bits_lut[256];
static bfSSS_t g_sm4_byte_combine_bits_sq_lut[256];
static uint8_t g_sm4_bf8_square_lut[256][8];
static bfSSS_t g_sm4_coeff_conj_lut[256][8];
static int g_sm4_byte_combine_bits_lut_ready = 0;
static bfSSS_t g_sm4_current_delta;
static bfSSS_t g_sm4_current_delta_sq;
static int g_sm4_norm_verifier_mode = 0;
static void sm4_SSS_constraints_prover(const params_t* params, zk_hash_SSS_3_ctx* hasher,
                                       const uint8_t* w, const bfSSS_t* w_tag,
                                       const uint8_t* owf_in, const uint8_t* owf_out);
static void sm4_SSS_constraints_verifier(const params_t* params, zk_hash_SSS_ctx* hasher,
                                         const bfSSS_t* w_key, const uint8_t* owf_in,
                                         const uint8_t* owf_out, bfSSS_t delta);

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
    uint8_t sq = (uint8_t)i;
    for (unsigned int s = 0u; s != 8u; ++s) {
      g_sm4_bf8_square_lut[i][s] = sq;
      g_sm4_coeff_conj_lut[i][s] = bfSSS_byte_combine_bits(sq);
      sq = bits_sq_sm4(sq);
    }
  }
  g_sm4_byte_combine_bits_lut_ready = 1;
}

static inline bfSSS_t sm4_byte_combine_bits_lut(uint8_t x) {
  return g_sm4_byte_combine_bits_lut[x];
}

static inline bfSSS_t sm4_byte_combine_bits_sq_lut(uint8_t x) {
  return g_sm4_byte_combine_bits_sq_lut[x];
}

typedef struct {
  uint8_t value;
  unsigned int degree;
  bfSSS_t conj[8];
  bfSSS_t lin[8];
  bfSSS_t quad[8];
} sm4_norm_byte_expr_t;

typedef struct {
  sm4_norm_byte_expr_t byte[4];
} sm4_norm_word_expr_t;

static void sm4_SSS_enc_constraints_prover(const params_t* params, zk_hash_SSS_3_ctx* hasher,
                                           const uint8_t* in, const bfSSS_t* in_tag,
                                           const uint8_t* out, const bfSSS_t* out_tag,
                                           const uint8_t* w, const bfSSS_t* w_tag,
                                           const uint8_t* k, const bfSSS_t* k_tag,
                                           int k_tag_is_zero);
static void sm4_SSS_enc_constraints_verifier(const params_t* params, zk_hash_SSS_ctx* hasher,
                                             const bfSSS_t* in_key, const bfSSS_t* out_key,
                                             const bfSSS_t* w_key, const bfSSS_t* k_key,
                                             const bfSSS_t delta);
static void sm4_norm_build_key_words_prover(const uint8_t* w, const bfSSS_t* w_tag,
                                            sm4_norm_word_expr_t K[36],
                                            zk_hash_SSS_3_ctx* hasher);
static void sm4_norm_build_key_words_verifier(const bfSSS_t* w_key,
                                              sm4_norm_word_expr_t K[36],
                                              zk_hash_SSS_ctx* hasher,
                                              bfSSS_t delta,
                                              const bfSSS_t delta_conj[8]);
static void sm4_norm_build_enc_words_prover(const uint8_t* in, const bfSSS_t* in_tag,
                                            const uint8_t* out, const bfSSS_t* out_tag,
                                            const uint8_t* w, const bfSSS_t* w_tag,
                                            const sm4_norm_word_expr_t K[36],
                                            zk_hash_SSS_3_ctx* hasher);
static void sm4_norm_build_enc_words_verifier(const bfSSS_t* in_key,
                                              const bfSSS_t* out_key,
                                              const bfSSS_t* w_key,
                                              const sm4_norm_word_expr_t K[36],
                                              zk_hash_SSS_ctx* hasher,
                                              bfSSS_t delta,
                                              const bfSSS_t delta_conj[8]);

static const uint8_t SM4_NORM_BETA4_SQ[5] = {0x0c, 0x50, 0x0d, 0x51, 0x0c};
static const uint8_t SM4_NORM_BETA4_CUBE[4] = {0x2a, 0x7a, 0x77, 0x26};
static const uint8_t SM4_AFFINE_LIN_COEFFS[8] = {0x18, 0x06, 0x91, 0xe9,
                                                 0xf4, 0xc8, 0xe6, 0x77};
static const uint8_t SM4_INV_AFFINE_LIN_COEFFS[8] = {0x19, 0x9e, 0x7c, 0x64,
                                                     0xf4, 0x31, 0x96, 0x49};

static const uint8_t SM4_WORDMAP_LINV[4][4][8] = {
    {{0x19,0xbe,0x34,0xee,0xe4,0xe0,0xf5,0x99},
     {0x18,0xf3,0x7c,0x30,0xb8,0x6e,0x6e,0x4e},
     {0x7b,0xfa,0x4e,0x30,0x2b,0x69,0x90,0x68},
     {0x07,0xb8,0x88,0xdc,0x23,0x6e,0xd7,0x30}},
    {{0x07,0xb8,0x88,0xdc,0x23,0x6e,0xd7,0x30},
     {0x19,0xbe,0x34,0xee,0xe4,0xe0,0xf5,0x99},
     {0x18,0xf3,0x7c,0x30,0xb8,0x6e,0x6e,0x4e},
     {0x7b,0xfa,0x4e,0x30,0x2b,0x69,0x90,0x68}},
    {{0x7b,0xfa,0x4e,0x30,0x2b,0x69,0x90,0x68},
     {0x07,0xb8,0x88,0xdc,0x23,0x6e,0xd7,0x30},
     {0x19,0xbe,0x34,0xee,0xe4,0xe0,0xf5,0x99},
     {0x18,0xf3,0x7c,0x30,0xb8,0x6e,0x6e,0x4e}},
    {{0x18,0xf3,0x7c,0x30,0xb8,0x6e,0x6e,0x4e},
     {0x7b,0xfa,0x4e,0x30,0x2b,0x69,0x90,0x68},
     {0x07,0xb8,0x88,0xdc,0x23,0x6e,0xd7,0x30},
     {0x19,0xbe,0x34,0xee,0xe4,0xe0,0xf5,0x99}}};

static const uint8_t SM4_WORDMAP_LPINV[4][4][8] = {
    {{0x7a,0x37,0xa2,0xcd,0x92,0xae,0x58,0x53},
     {0x00,0x3a,0x5b,0x08,0xd9,0x43,0x27,0x8d},
     {0x7b,0xf8,0xe6,0x9d,0xc4,0xf2,0xff,0xf3},
     {0x01,0x6d,0xf2,0xd5,0x8f,0xe6,0xf0,0x13}},
    {{0x01,0x6d,0xf2,0xd5,0x8f,0xe6,0xf0,0x13},
     {0x7a,0x37,0xa2,0xcd,0x92,0xae,0x58,0x53},
     {0x00,0x3a,0x5b,0x08,0xd9,0x43,0x27,0x8d},
     {0x7b,0xf8,0xe6,0x9d,0xc4,0xf2,0xff,0xf3}},
    {{0x7b,0xf8,0xe6,0x9d,0xc4,0xf2,0xff,0xf3},
     {0x01,0x6d,0xf2,0xd5,0x8f,0xe6,0xf0,0x13},
     {0x7a,0x37,0xa2,0xcd,0x92,0xae,0x58,0x53},
     {0x00,0x3a,0x5b,0x08,0xd9,0x43,0x27,0x8d}},
    {{0x00,0x3a,0x5b,0x08,0xd9,0x43,0x27,0x8d},
     {0x7b,0xf8,0xe6,0x9d,0xc4,0xf2,0xff,0xf3},
     {0x01,0x6d,0xf2,0xd5,0x8f,0xe6,0xf0,0x13},
     {0x7a,0x37,0xa2,0xcd,0x92,0xae,0x58,0x53}}};

static const uint8_t SM4_WORDMAP_L_AFTER_AFFINE_CONST[4] = {0x4f, 0x4f, 0x4f, 0x4f};

static const uint8_t SM4_WORDMAP_L_AFTER_AFFINE[4][4][8] = {
    {{0x19,0x7f,0x4b,0xc7,0x40,0x9a,0x35,0xe2},
     {0x7a,0x02,0xe2,0x97,0xec,0xfb,0x2f,0x1a},
     {0x7a,0x02,0xe2,0x97,0xec,0xfb,0x2f,0x1a},
     {0x63,0x7d,0xa9,0x50,0xac,0x61,0x1a,0xf8}},
    {{0x63,0x7d,0xa9,0x50,0xac,0x61,0x1a,0xf8},
     {0x19,0x7f,0x4b,0xc7,0x40,0x9a,0x35,0xe2},
     {0x7a,0x02,0xe2,0x97,0xec,0xfb,0x2f,0x1a},
     {0x7a,0x02,0xe2,0x97,0xec,0xfb,0x2f,0x1a}},
    {{0x7a,0x02,0xe2,0x97,0xec,0xfb,0x2f,0x1a},
     {0x63,0x7d,0xa9,0x50,0xac,0x61,0x1a,0xf8},
     {0x19,0x7f,0x4b,0xc7,0x40,0x9a,0x35,0xe2},
     {0x7a,0x02,0xe2,0x97,0xec,0xfb,0x2f,0x1a}},
    {{0x7a,0x02,0xe2,0x97,0xec,0xfb,0x2f,0x1a},
     {0x7a,0x02,0xe2,0x97,0xec,0xfb,0x2f,0x1a},
     {0x63,0x7d,0xa9,0x50,0xac,0x61,0x1a,0xf8},
     {0x19,0x7f,0x4b,0xc7,0x40,0x9a,0x35,0xe2}}};

static const uint8_t SM4_WORDMAP_LP_AFTER_AFFINE_CONST[4] = {0x40, 0x40, 0x40, 0x40};

static const uint8_t SM4_WORDMAP_LP_AFTER_AFFINE[4][4][8] = {
    {{0x18,0x06,0x91,0xe9,0xf4,0xc8,0xe6,0x77},
     {0xe1,0xb4,0xa9,0x0d,0x41,0xf3,0xb3,0x90},
     {0x8d,0x47,0x3e,0x75,0x9e,0x9e,0xc3,0xdb},
     {0x6c,0x7a,0x56,0xc3,0xdd,0x26,0x7e,0x63}},
    {{0x6c,0x7a,0x56,0xc3,0xdd,0x26,0x7e,0x63},
     {0x18,0x06,0x91,0xe9,0xf4,0xc8,0xe6,0x77},
     {0xe1,0xb4,0xa9,0x0d,0x41,0xf3,0xb3,0x90},
     {0x8d,0x47,0x3e,0x75,0x9e,0x9e,0xc3,0xdb}},
    {{0x8d,0x47,0x3e,0x75,0x9e,0x9e,0xc3,0xdb},
     {0x6c,0x7a,0x56,0xc3,0xdd,0x26,0x7e,0x63},
     {0x18,0x06,0x91,0xe9,0xf4,0xc8,0xe6,0x77},
     {0xe1,0xb4,0xa9,0x0d,0x41,0xf3,0xb3,0x90}},
    {{0xe1,0xb4,0xa9,0x0d,0x41,0xf3,0xb3,0x90},
     {0x8d,0x47,0x3e,0x75,0x9e,0x9e,0xc3,0xdb},
     {0x6c,0x7a,0x56,0xc3,0xdd,0x26,0x7e,0x63},
     {0x18,0x06,0x91,0xe9,0xf4,0xc8,0xe6,0x77}}};

static uint8_t sm4_bf8_square_n(uint8_t x, unsigned int n) {
  return g_sm4_bf8_square_lut[x][n & 7u];
}

static const bfSSS_t* sm4_norm_byte_coeffs(const sm4_norm_byte_expr_t* in,
                                           unsigned int degree) {
  switch (degree) {
  case 0: return in->conj;
  case 1: return in->lin;
  default: return in->quad;
  }
}

static bfSSS_t* sm4_norm_byte_coeffs_mut(sm4_norm_byte_expr_t* in, unsigned int degree) {
  switch (degree) {
  case 0: return in->conj;
  case 1: return in->lin;
  default: return in->quad;
  }
}

static bfSSS_t sm4_norm_delta_pow(unsigned int shift) {
  if (shift == 0u) {
    return bfSSS_one();
  }
  if (shift == 1u) {
    return g_sm4_current_delta;
  }
  return g_sm4_current_delta_sq;
}

static bfSSS_t sm4_coeff_mul_conj(uint8_t coeff, unsigned int square_count, bfSSS_t x) {
  const unsigned int s = square_count & 7u;
  const uint8_t c = g_sm4_bf8_square_lut[coeff][s];
  if (c == 0) {
    return bfSSS_zero();
  }
  if (c == 1) {
    return x;
  }
  return bfSSS_mul(g_sm4_coeff_conj_lut[coeff][s], x);
}

static bfSSS_t sm4_coeff_linear_comb_degree(const uint8_t coeffs[8],
                                            unsigned int square_count,
                                            const sm4_norm_byte_expr_t* in,
                                            unsigned int degree) {
  const unsigned int s = square_count & 7u;
  const bfSSS_t* in_coeff = sm4_norm_byte_coeffs(in, degree);
  bfSSS_t acc = bfSSS_zero();
  for (unsigned int k = 0u; k != 8u; ++k) {
    acc = bfSSS_add(acc, sm4_coeff_mul_conj(coeffs[k], s, in_coeff[(k + s) & 7u]));
  }
  return acc;
}

static bfSSS_t sm4_word_coeff_linear_comb_degree(const uint8_t coeffs[4][8],
                                                 unsigned int square_count,
                                                 const sm4_norm_word_expr_t* in,
                                                 unsigned int degree) {
  const unsigned int s = square_count & 7u;
  bfSSS_t acc = bfSSS_zero();
  for (unsigned int ib = 0u; ib != 4u; ++ib) {
    acc = bfSSS_add(acc, sm4_coeff_linear_comb_degree(coeffs[ib], s, &in->byte[ib], degree));
  }
  return acc;
}

static void sm4_word_coeff_linear_comb_all(const uint8_t coeffs[4][8],
                                           unsigned int square_count,
                                           const sm4_norm_word_expr_t* in,
                                           unsigned int max_degree,
                                           bfSSS_t out[3]) {
  if (max_degree > 2u) {
    max_degree = 2u;
  }

  for (unsigned int d = 0u; d != 3u; ++d) {
    out[d] = d <= max_degree ? sm4_word_coeff_linear_comb_degree(coeffs, square_count, in, d)
                             : bfSSS_zero();
  }
}

static uint8_t sm4_unpack_norm_nibble(const uint8_t* packed, unsigned int idx) {
  const uint8_t v = packed[idx >> 1u];
  return (idx & 1u) ? (uint8_t)(v >> 4) : (uint8_t)(v & 0x0fu);
}

static uint8_t sm4_norm_nibble_conjugate_byte(uint8_t nibble, unsigned int conj) {
  const unsigned int j = conj & 3u;
  uint8_t h = 0u;
  if (nibble & 0x1u) {
    h ^= 0x01u;
  }
  if (nibble & 0x2u) {
    h ^= SM4_NORM_BETA4_SQ[j];
  }
  if (nibble & 0x4u) {
    h ^= SM4_NORM_BETA4_SQ[j + 1u];
  }
  if (nibble & 0x8u) {
    h ^= SM4_NORM_BETA4_CUBE[j];
  }
  return h;
}

static void sm4_norm_byte_from_bits(sm4_norm_byte_expr_t* out, uint8_t value,
                                    const bfSSS_t* bit_expr) {
  bfSSS_t tmp[8];
  out->value = value;
  out->degree = 1u;
  memcpy(tmp, bit_expr, sizeof(tmp));
  for (unsigned int i = 0; i != 8u; ++i) {
    out->conj[i] = bfSSS_byte_combine(tmp);
    out->lin[i] = sm4_byte_combine_bits_lut(sm4_bf8_square_n(value, i));
    out->quad[i] = bfSSS_zero();
    bfSSS_sq_bit_inplace(tmp);
  }
}

static void sm4_norm_byte_public_prover(sm4_norm_byte_expr_t* out, uint8_t value) {
  out->value = value;
  out->degree = 1u;
  for (unsigned int i = 0; i != 8u; ++i) {
    out->conj[i] = bfSSS_zero();
    out->lin[i] = sm4_byte_combine_bits_lut(sm4_bf8_square_n(value, i));
    out->quad[i] = bfSSS_zero();
  }
}

static void sm4_norm_byte_public_verifier(sm4_norm_byte_expr_t* out, uint8_t value,
                                          bfSSS_t delta) {
  bfSSS_t bits[8];
  for (unsigned int i = 0; i != 8u; ++i) {
    bits[i] = bfSSS_mul_bit(delta, get_bit(value, i));
  }
  sm4_norm_byte_from_bits(out, value, bits);
  for (unsigned int i = 0; i != 8u; ++i) {
    out->lin[i] = bfSSS_zero();
  }
}

static void sm4_norm_byte_add(sm4_norm_byte_expr_t* out, const sm4_norm_byte_expr_t* a,
                              const sm4_norm_byte_expr_t* b) {
  const unsigned int degree = a->degree > b->degree ? a->degree : b->degree;
  out->value = (uint8_t)(a->value ^ b->value);
  out->degree = degree;
  if (g_sm4_norm_verifier_mode) {
    const unsigned int ashift = degree - a->degree;
    const unsigned int bshift = degree - b->degree;
    const bfSSS_t apow = ashift == 0u ? bfSSS_zero() : sm4_norm_delta_pow(ashift);
    const bfSSS_t bpow = bshift == 0u ? bfSSS_zero() : sm4_norm_delta_pow(bshift);
    const bfSSS_t zero = bfSSS_zero();
    for (unsigned int i = 0; i != 8u; ++i) {
      const bfSSS_t aconj = ashift == 0u ? a->conj[i] : bfSSS_mul(apow, a->conj[i]);
      const bfSSS_t bconj = bshift == 0u ? b->conj[i] : bfSSS_mul(bpow, b->conj[i]);
      out->conj[i] = bfSSS_add(aconj, bconj);
      out->lin[i] = zero;
      out->quad[i] = zero;
    }
    return;
  }

  const unsigned int ashift = degree - a->degree;
  const unsigned int bshift = degree - b->degree;
  for (unsigned int i = 0; i != 8u; ++i) {
    bfSSS_t c0 = bfSSS_zero();
    bfSSS_t c1 = bfSSS_zero();
    bfSSS_t c2 = bfSSS_zero();

    if (ashift == 0u) {
      c0 = bfSSS_add(c0, a->conj[i]);
      if (a->degree >= 1u) {
        c1 = bfSSS_add(c1, a->lin[i]);
      }
      if (a->degree >= 2u) {
        c2 = bfSSS_add(c2, a->quad[i]);
      }
    } else if (ashift == 1u) {
      c1 = bfSSS_add(c1, a->conj[i]);
      if (a->degree >= 1u) {
        c2 = bfSSS_add(c2, a->lin[i]);
      }
    } else {
      c2 = bfSSS_add(c2, a->conj[i]);
    }

    if (bshift == 0u) {
      c0 = bfSSS_add(c0, b->conj[i]);
      if (b->degree >= 1u) {
        c1 = bfSSS_add(c1, b->lin[i]);
      }
      if (b->degree >= 2u) {
        c2 = bfSSS_add(c2, b->quad[i]);
      }
    } else if (bshift == 1u) {
      c1 = bfSSS_add(c1, b->conj[i]);
      if (b->degree >= 1u) {
        c2 = bfSSS_add(c2, b->lin[i]);
      }
    } else {
      c2 = bfSSS_add(c2, b->conj[i]);
    }

    out->conj[i] = c0;
    out->lin[i] = c1;
    out->quad[i] = c2;
  }
}

static void sm4_norm_word_add(sm4_norm_word_expr_t* out, const sm4_norm_word_expr_t* a,
                              const sm4_norm_word_expr_t* b) {
  for (unsigned int i = 0; i != 4u; ++i) {
    sm4_norm_byte_add(&out->byte[i], &a->byte[i], &b->byte[i]);
  }
}

static void sm4_norm_word_from_bits(sm4_norm_word_expr_t* out, const uint8_t* value,
                                    const bfSSS_t* bit_expr) {
  for (unsigned int i = 0; i != 4u; ++i) {
    sm4_norm_byte_from_bits(&out->byte[i], value[i], bit_expr + i * 8u);
  }
}

static void sm4_norm_word_from_key(sm4_norm_word_expr_t* out, const bfSSS_t* bit_key) {
  static const uint8_t zero_word[4];
  sm4_norm_word_from_bits(out, zero_word, bit_key);
}

static void sm4_norm_word_public_prover(sm4_norm_word_expr_t* out, const uint8_t* value) {
  for (unsigned int i = 0; i != 4u; ++i) {
    sm4_norm_byte_public_prover(&out->byte[i], value[i]);
  }
}

static void sm4_norm_word_public_verifier(sm4_norm_word_expr_t* out, const uint8_t* value,
                                          bfSSS_t delta) {
  for (unsigned int i = 0; i != 4u; ++i) {
    sm4_norm_byte_public_verifier(&out->byte[i], value[i], delta);
  }
}

static void sm4_norm_delta_conjugates(bfSSS_t out[8], bfSSS_t delta) {
  out[0] = delta;
  for (unsigned int i = 1u; i != 8u; ++i) {
    out[i] = bfSSS_sqr(out[i - 1u]);
  }
}

static void sm4_norm_byte_linearized(sm4_norm_byte_expr_t* out,
                                     const sm4_norm_byte_expr_t* in,
                                     const uint8_t coeffs[8], uint8_t constant,
                                     uint8_t (*byte_map)(uint8_t),
                                     const bfSSS_t* delta_conj) {
  out->value = byte_map ? byte_map(in->value) : 0u;
  out->degree = in->degree;
  for (unsigned int s = 0; s != 8u; ++s) {
    bfSSS_t acc = sm4_coeff_linear_comb_degree(coeffs, s, in, 0u);
    if (delta_conj) {
      acc = bfSSS_add(acc, bfSSS_mul(sm4_norm_delta_pow(out->degree),
                                     sm4_byte_combine_bits_lut(sm4_bf8_square_n(constant, s))));
      out->conj[s] = acc;
      out->lin[s] = bfSSS_zero();
      out->quad[s] = bfSSS_zero();
      continue;
    }
    out->conj[s] = acc;
    out->lin[s] = in->degree >= 1u ? sm4_coeff_linear_comb_degree(coeffs, s, in, 1u)
                                   : bfSSS_zero();
    out->quad[s] = in->degree >= 2u ? sm4_coeff_linear_comb_degree(coeffs, s, in, 2u)
                                    : bfSSS_zero();
    bfSSS_t* target = sm4_norm_byte_coeffs_mut(out, out->degree);
    target[s] = bfSSS_add(target[s],
                          sm4_byte_combine_bits_lut(sm4_bf8_square_n(constant, s)));
  }
}

static void sm4_norm_byte_linearized_first2(sm4_norm_byte_expr_t* out,
                                            const sm4_norm_byte_expr_t* in,
                                            const uint8_t coeffs[8],
                                            uint8_t constant,
                                            uint8_t (*byte_map)(uint8_t),
                                            const bfSSS_t* delta_conj) {
  out->value = byte_map ? byte_map(in->value) : 0u;
  out->degree = in->degree;
  for (unsigned int s = 0; s != 2u; ++s) {
    bfSSS_t acc = sm4_coeff_linear_comb_degree(coeffs, s, in, 0u);
    if (delta_conj) {
      acc = bfSSS_add(acc, bfSSS_mul(sm4_norm_delta_pow(out->degree),
                                     sm4_byte_combine_bits_lut(sm4_bf8_square_n(constant, s))));
      out->conj[s] = acc;
      out->lin[s] = bfSSS_zero();
      out->quad[s] = bfSSS_zero();
      continue;
    }
    out->conj[s] = acc;
    out->lin[s] = in->degree >= 1u ? sm4_coeff_linear_comb_degree(coeffs, s, in, 1u)
                                   : bfSSS_zero();
    out->quad[s] = in->degree >= 2u ? sm4_coeff_linear_comb_degree(coeffs, s, in, 2u)
                                    : bfSSS_zero();
    bfSSS_t* target = sm4_norm_byte_coeffs_mut(out, out->degree);
    target[s] = bfSSS_add(target[s],
                          sm4_byte_combine_bits_lut(sm4_bf8_square_n(constant, s)));
  }
}

static void sm4_norm_byte_affine_prover(sm4_norm_byte_expr_t* out,
                                        const sm4_norm_byte_expr_t* in) {
  sm4_norm_byte_linearized(out, in, SM4_AFFINE_LIN_COEFFS, SM4_AFFINE_CONST,
                           sm4_affine_byte, NULL);
}

static void sm4_norm_byte_affine_verifier(sm4_norm_byte_expr_t* out,
                                          const sm4_norm_byte_expr_t* in,
                                          const bfSSS_t delta_conj[8]) {
  sm4_norm_byte_linearized(out, in, SM4_AFFINE_LIN_COEFFS, SM4_AFFINE_CONST,
                           NULL, delta_conj);
}

static void sm4_norm_byte_affine2_prover(sm4_norm_byte_expr_t* out,
                                         const sm4_norm_byte_expr_t* in) {
  sm4_norm_byte_linearized_first2(out, in, SM4_AFFINE_LIN_COEFFS, SM4_AFFINE_CONST,
                                  sm4_affine_byte, NULL);
}

static void sm4_norm_byte_affine2_verifier(sm4_norm_byte_expr_t* out,
                                           const sm4_norm_byte_expr_t* in,
                                           const bfSSS_t delta_conj[8]) {
  sm4_norm_byte_linearized_first2(out, in, SM4_AFFINE_LIN_COEFFS, SM4_AFFINE_CONST,
                                  NULL, delta_conj);
}

static void sm4_norm_byte_inv_affine2_prover(sm4_norm_byte_expr_t* out,
                                             const sm4_norm_byte_expr_t* in) {
  sm4_norm_byte_linearized_first2(out, in, SM4_INV_AFFINE_LIN_COEFFS, SM4_INV_AFFINE_CONST,
                                  sm4_inv_affine_byte, NULL);
}

static void sm4_norm_byte_inv_affine2_verifier(sm4_norm_byte_expr_t* out,
                                               const sm4_norm_byte_expr_t* in,
                                               const bfSSS_t delta_conj[8]) {
  sm4_norm_byte_linearized_first2(out, in, SM4_INV_AFFINE_LIN_COEFFS, SM4_INV_AFFINE_CONST,
                                  NULL, delta_conj);
}

static void sm4_norm_nibble_conjugates_key(bfSSS_t out[4], const bfSSS_t* nibble_key) {
  for (unsigned int j = 0; j != 4u; ++j) {
    bfSSS_t acc = nibble_key[0];
    acc = bfSSS_add(acc, bfSSS_mul(g_sm4_coeff_conj_lut[SM4_NORM_BETA4_SQ[j]][0],
                                   nibble_key[1]));
    acc = bfSSS_add(acc, bfSSS_mul(g_sm4_coeff_conj_lut[SM4_NORM_BETA4_SQ[j + 1u]][0],
                                   nibble_key[2]));
    acc = bfSSS_add(acc, bfSSS_mul(g_sm4_coeff_conj_lut[SM4_NORM_BETA4_CUBE[j]][0],
                                   nibble_key[3]));
    out[j] = acc;
  }
}

static void sm4_norm_inv_from_norm_prover(sm4_norm_byte_expr_t* inv, bfSSS_t z_norm[3],
                                          const sm4_norm_byte_expr_t* sbox_input,
                                          uint8_t norm_nibble,
                                          const bfSSS_t* norm_bits) {
  sm4_norm_byte_expr_t x;
  bfSSS_t h_val[4];
  bfSSS_t h[4];

  sm4_norm_byte_affine_prover(&x, sbox_input);
  sm4_norm_nibble_conjugates_key(h, norm_bits);
  for (unsigned int j = 0u; j != 4u; ++j) {
    h_val[j] = sm4_byte_combine_bits_lut(sm4_norm_nibble_conjugate_byte(norm_nibble, j));
  }

  const bfSSS_t x16_val = x.lin[4];
  const bfSSS_t x2_val = x.lin[1];

  const bfSSS_t hx16_tag = bfSSS_mul(h[0], x.conj[4]);
  const bfSSS_t hx16_val0 = bfSSS_mul(h_val[0], x.conj[4]);
  const bfSSS_t hx16_val1 = bfSSS_mul(h[0], x16_val);
  const bfSSS_t hx16_val = bfSSS_add(hx16_val0, hx16_val1);
  const bfSSS_t hx16_valval = bfSSS_mul(h_val[0], x16_val);

  z_norm[0] = bfSSS_mul(hx16_tag, x.conj[1]);
  z_norm[1] = bfSSS_add(bfSSS_mul(hx16_val, x.conj[1]), bfSSS_mul(hx16_tag, x2_val));
  z_norm[2] = bfSSS_add(bfSSS_add(bfSSS_mul(hx16_valval, x.conj[1]),
                                  bfSSS_mul(hx16_val, x2_val)),
                        x.conj[0]);

  inv->value = bf8_inv_sm4(x.value);
  inv->degree = 2u;
  for (unsigned int j = 0; j != 8u; ++j) {
    const unsigned int xs = (j + 4u) & 7u;
    const unsigned int hs = j & 3u;
    inv->conj[j] = bfSSS_mul(x.conj[xs], h[hs]);
    inv->lin[j] = bfSSS_add(bfSSS_mul(x.lin[xs], h[hs]),
                            bfSSS_mul(x.conj[xs], h_val[hs]));
    inv->quad[j] = bfSSS_mul(x.lin[xs], h_val[hs]);
  }
}

static void sm4_norm_inv_from_norm_verifier(sm4_norm_byte_expr_t* inv, bfSSS_t* z_norm,
                                            const sm4_norm_byte_expr_t* sbox_input,
                                            const bfSSS_t* norm_key,
                                            bfSSS_t delta_sq,
                                            const bfSSS_t delta_conj[8]) {
  sm4_norm_byte_expr_t x;
  bfSSS_t h[4];

  sm4_norm_byte_affine_verifier(&x, sbox_input, delta_conj);
  sm4_norm_nibble_conjugates_key(h, norm_key);

  *z_norm = bfSSS_add(bfSSS_mul(bfSSS_mul(h[0], x.conj[4]), x.conj[1]),
                      bfSSS_mul(delta_sq, x.conj[0]));

  inv->value = 0u;
  inv->degree = 2u;
  for (unsigned int j = 0; j != 8u; ++j) {
    inv->conj[j] = bfSSS_mul(x.conj[(j + 4u) & 7u], h[j & 3u]);
    inv->lin[j] = bfSSS_zero();
    inv->quad[j] = bfSSS_zero();
  }
}

static void sm4_norm_word_map(sm4_norm_word_expr_t* out, const sm4_norm_word_expr_t* in,
                              const uint8_t coeffs[4][4][8], uint32_t (*word_map)(uint32_t)) {
  const uint32_t mapped = word_map(sm4_load_be_u32((const uint8_t[4]){
      in->byte[0].value, in->byte[1].value, in->byte[2].value, in->byte[3].value}));
  uint8_t mapped_bytes[4];
  sm4_store_be_u32(mapped, mapped_bytes);
	  for (unsigned int ob = 0; ob != 4u; ++ob) {
	    out->byte[ob].value = mapped_bytes[ob];
	    out->byte[ob].degree = in->byte[0].degree;
	    for (unsigned int s = 0; s != 8u; ++s) {
        if (g_sm4_norm_verifier_mode) {
          out->byte[ob].conj[s] = sm4_word_coeff_linear_comb_degree(coeffs[ob], s, in, 0u);
          out->byte[ob].lin[s] = bfSSS_zero();
          out->byte[ob].quad[s] = bfSSS_zero();
          continue;
        }
        bfSSS_t row[3];
        if (in->byte[0].degree >= 2u) {
          sm4_word_coeff_linear_comb_all(coeffs[ob], s, in, in->byte[0].degree, row);
        } else {
          row[0] = sm4_word_coeff_linear_comb_degree(coeffs[ob], s, in, 0u);
          row[1] = in->byte[0].degree >= 1u
                       ? sm4_word_coeff_linear_comb_degree(coeffs[ob], s, in, 1u)
                       : bfSSS_zero();
          row[2] = bfSSS_zero();
        }
	      out->byte[ob].conj[s] = row[0];
	      out->byte[ob].lin[s] = row[1];
	      out->byte[ob].quad[s] = row[2];
	    }
	  }
	}

static void sm4_norm_word_map_after_affine(sm4_norm_word_expr_t* out,
                                           const sm4_norm_word_expr_t* in,
                                           const uint8_t coeffs[4][4][8],
                                           const uint8_t constants[4],
                                           uint32_t (*word_map)(uint32_t)) {
  const uint32_t mapped = word_map(sm4_load_be_u32((const uint8_t[4]){
      sm4_affine_byte(in->byte[0].value), sm4_affine_byte(in->byte[1].value),
      sm4_affine_byte(in->byte[2].value), sm4_affine_byte(in->byte[3].value)}));
  uint8_t mapped_bytes[4];
  sm4_store_be_u32(mapped, mapped_bytes);

  for (unsigned int ob = 0; ob != 4u; ++ob) {
    out->byte[ob].value = mapped_bytes[ob];
    out->byte[ob].degree = in->byte[0].degree;
    for (unsigned int s = 0; s != 8u; ++s) {
      if (g_sm4_norm_verifier_mode) {
        bfSSS_t acc = sm4_word_coeff_linear_comb_degree(coeffs[ob], s, in, 0u);
        acc = bfSSS_add(acc,
                        bfSSS_mul(sm4_norm_delta_pow(out->byte[ob].degree),
                                  sm4_byte_combine_bits_lut(
                                      sm4_bf8_square_n(constants[ob], s))));
        out->byte[ob].conj[s] = acc;
        out->byte[ob].lin[s] = bfSSS_zero();
        out->byte[ob].quad[s] = bfSSS_zero();
        continue;
      }
      bfSSS_t row[3];
      if (in->byte[0].degree >= 2u) {
        sm4_word_coeff_linear_comb_all(coeffs[ob], s, in, in->byte[0].degree, row);
      } else {
        row[0] = sm4_word_coeff_linear_comb_degree(coeffs[ob], s, in, 0u);
        row[1] = in->byte[0].degree >= 1u
                     ? sm4_word_coeff_linear_comb_degree(coeffs[ob], s, in, 1u)
                     : bfSSS_zero();
        row[2] = bfSSS_zero();
      }
      bfSSS_t acc = row[0];
      out->byte[ob].conj[s] = acc;
      out->byte[ob].lin[s] = row[1];
      out->byte[ob].quad[s] = row[2];
      bfSSS_t* target = sm4_norm_byte_coeffs_mut(&out->byte[ob], out->byte[ob].degree);
      target[s] = bfSSS_add(target[s],
                            sm4_byte_combine_bits_lut(
                                sm4_bf8_square_n(constants[ob], s)));
    }
  }
}

static uint32_t sm4_word_map_l(uint32_t x) {
  return SM4_L(x);
}

static uint32_t sm4_word_map_lprime(uint32_t x) {
  return SM4_L_prime(x);
}

static uint32_t sm4_word_map_linv(uint32_t x) {
  return SM4_L_inv(x);
}

static uint32_t sm4_word_map_lprime_inv(uint32_t x) {
  return SM4_L_prime_inv(x);
}

static void sm4_norm_word_Linv(sm4_norm_word_expr_t* out, const sm4_norm_word_expr_t* in) {
  sm4_norm_word_map(out, in, SM4_WORDMAP_LINV, sm4_word_map_linv);
}

static void sm4_norm_word_Lprime_inv(sm4_norm_word_expr_t* out,
                                     const sm4_norm_word_expr_t* in) {
  sm4_norm_word_map(out, in, SM4_WORDMAP_LPINV, sm4_word_map_lprime_inv);
}

// OWF PROVER
// clang-format off
void sm4_128_prover(const params_t* params, uint8_t* a0_tilde, uint8_t* a1_tilde,
                    uint8_t* a2_tilde, const uint8_t* w, const uint8_t* u, uint8_t** V,
                    const uint8_t* owf_in, const uint8_t* owf_out,
                    const uint8_t* chall_2) {
  // clang-format on
  // const unsigned int ell = 1280;
  const unsigned int ell = params->ell;
  g_sm4_norm_verifier_mode = 0;

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
  bfSSS_t bf_u_star_1 = bfSSS_sum_poly_bits(u + (SM4TH_LAMBDA / 8u));

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
  zk_hash_SSS_3_finalize(a0_tilde, a1_tilde, a2_tilde, &hasher, bf_v_star_0,
                         bfSSS_add(bf_v_star_1, bf_u_star_0), bf_u_star_1);

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

  // ::4-5
  // constraint: w[0] * w[1] = 0; 
  // zk_hash input: (z0 = v0 * v1, z1 = w[0] * v1 + w[1] * v0, z2 = 0)
  zk_hash_SSS_3_update(hasher, bfSSS_mul(w_tag[0], w_tag[1]),
                       bfSSS_add(bfSSS_mul_bit(w_tag[0], ptr_get_bit(w, 1)),
                                 bfSSS_mul_bit(w_tag[1], ptr_get_bit(w, 0))),
                       bfSSS_zero());

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
    // Round-key expressions are public in EM mode; only encryption constraints
    // are appended to the hasher here.
    sm4_SSS_enc_constraints_prover(params, hasher, 
                      // X[0..3], X[35], X[34], X[33], X[32]
                      in, in_tag, out, out_tag, 
                      // X[3],...,X[32], 1152 / 8 = 144
                      w + 16, w_tag + 16 * 8, 
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

    /*
     * Build the virtual round-key words once.  The norm layout omits anchor
     * K-words from the witness, so encryption needs the reconstructed K[4..35]
     * expressions too.  Reusing the KeyExp result avoids a second full key
     * schedule pass in the fast parameter set.
     */
    sm4_norm_word_expr_t K[36];
    sm4_norm_build_key_words_prover(w, w_tag, K, hasher);
    sm4_norm_build_enc_words_prover(in, in_tag, out, out_tag, w + Lke / 8,
                                    w_tag + Lke, K, hasher);
  }

}

static bool sm4_norm_is_key_anchor(unsigned int word) {
  return word >= 4u && word <= 32u && ((word & 3u) == 0u);
}

static bool sm4_norm_is_enc_anchor(unsigned int word) {
  return word >= 5u && word <= 29u && ((word & 3u) == 1u);
}

static unsigned int sm4_norm_key_full_byte_offset(unsigned int word) {
  if (word < 4u) {
    return word * 4u;
  }

  const unsigned int prior_words = word - 4u;
  const unsigned int prior_anchors = (word - 1u) >> 2u;
  const unsigned int full_words = prior_words - prior_anchors;
  return 32u + full_words * 4u;
}

static unsigned int sm4_norm_enc_full_byte_offset(unsigned int word) {
  const unsigned int prior_words = word - 4u;
  const unsigned int prior_anchors = word <= 5u ? 0u : ((word - 6u) >> 2u) + 1u;
  const unsigned int full_words = prior_words - prior_anchors;
  return 14u + full_words * 4u;
}

static void sm4_norm_public_u32_prover(sm4_norm_word_expr_t* out, uint32_t value) {
  uint8_t bytes[4];
  sm4_store_be_u32(value, bytes);
  sm4_norm_word_public_prover(out, bytes);
}

static void sm4_norm_public_u32_verifier(sm4_norm_word_expr_t* out, uint32_t value,
                                         bfSSS_t delta) {
  uint8_t bytes[4];
  sm4_store_be_u32(value, bytes);
  sm4_norm_word_public_verifier(out, bytes, delta);
}

static void sm4_norm_word_add4(sm4_norm_word_expr_t* out, const sm4_norm_word_expr_t* a,
                               const sm4_norm_word_expr_t* b,
                               const sm4_norm_word_expr_t* c,
                               const sm4_norm_word_expr_t* d) {
  for (unsigned int i = 0u; i != 4u; ++i) {
    sm4_norm_byte_expr_t ab;
    sm4_norm_byte_expr_t cd;
    sm4_norm_byte_add(&ab, &a->byte[i], &b->byte[i]);
    sm4_norm_byte_add(&cd, &c->byte[i], &d->byte[i]);
    sm4_norm_byte_add(&out->byte[i], &ab, &cd);
  }
}

static void sm4_norm_append_io_prover(zk_hash_SSS_3_ctx* hasher,
                                      const sm4_norm_byte_expr_t* x,
                                      const sm4_norm_byte_expr_t* y) {
  bfSSS_t row0[3] = {bfSSS_zero(), bfSSS_zero(), bfSSS_zero()};
  bfSSS_t row1[3] = {bfSSS_zero(), bfSSS_zero(), bfSSS_zero()};

  for (unsigned int dx = 0u; dx <= x->degree; ++dx) {
    for (unsigned int dy = 0u; dy <= y->degree; ++dy) {
      const unsigned int d = dx + dy;
      if (d <= 2u) {
        row0[d] = bfSSS_add(row0[d],
                            bfSSS_mul(sm4_norm_byte_coeffs(x, dx)[1],
                                      sm4_norm_byte_coeffs(y, dy)[0]));
        row1[d] = bfSSS_add(row1[d],
                            bfSSS_mul(sm4_norm_byte_coeffs(x, dx)[0],
                                      sm4_norm_byte_coeffs(y, dy)[1]));
      }
    }
  }

  for (unsigned int dx = 0u; dx <= x->degree; ++dx) {
    const unsigned int d = dx + y->degree;
    if (d <= 2u) {
      row0[d] = bfSSS_add(row0[d], sm4_norm_byte_coeffs(x, dx)[0]);
    }
  }
  for (unsigned int dy = 0u; dy <= y->degree; ++dy) {
    const unsigned int d = dy + x->degree;
    if (d <= 2u) {
      row1[d] = bfSSS_add(row1[d], sm4_norm_byte_coeffs(y, dy)[0]);
    }
  }

  zk_hash_SSS_3_update(hasher, row0[0], row0[1], row0[2]);
  zk_hash_SSS_3_update(hasher, row1[0], row1[1], row1[2]);
}

static void sm4_norm_append_io_verifier(zk_hash_SSS_ctx* hasher,
                                        const sm4_norm_byte_expr_t* x,
                                        const sm4_norm_byte_expr_t* y,
                                        bfSSS_t delta, bfSSS_t delta_sq) {
  (void)delta;
  (void)delta_sq;
  const bfSSS_t row0 =
      bfSSS_add(bfSSS_mul(sm4_norm_delta_pow(y->degree), x->conj[0]),
                bfSSS_mul(x->conj[1], y->conj[0]));
  const bfSSS_t row1 =
      bfSSS_add(bfSSS_mul(sm4_norm_delta_pow(x->degree), y->conj[0]),
                bfSSS_mul(x->conj[0], y->conj[1]));
  zk_hash_SSS_update(hasher, row0);
  zk_hash_SSS_update(hasher, row1);
}

static void sm4_norm_anchor_t_word_prover(sm4_norm_word_expr_t* out,
                                          zk_hash_SSS_3_ctx* hasher,
                                          const sm4_norm_word_expr_t* in,
                                          const uint8_t* norm_packed,
                                          const bfSSS_t* norm_tag,
                                          unsigned int anchor,
                                          const uint8_t coeffs[4][4][8],
                                          const uint8_t constants[4],
                                          uint32_t (*word_map)(uint32_t)) {
  sm4_norm_word_expr_t inv_word;
  for (unsigned int j = 0u; j != 4u; ++j) {
    bfSSS_t z_norm[3];
    const unsigned int nibble_idx = anchor * 4u + j;
    const uint8_t nibble = sm4_unpack_norm_nibble(norm_packed, nibble_idx);
    sm4_norm_inv_from_norm_prover(&inv_word.byte[j], z_norm, &in->byte[j], nibble,
                                  norm_tag + nibble_idx * 4u);
    if (hasher) {
      zk_hash_SSS_3_update(hasher, z_norm[0], z_norm[1], z_norm[2]);
    }
  }
  sm4_norm_word_map_after_affine(out, &inv_word, coeffs, constants, word_map);
}

static void sm4_norm_anchor_t_word_verifier(sm4_norm_word_expr_t* out,
                                            zk_hash_SSS_ctx* hasher,
                                            const sm4_norm_word_expr_t* in,
                                            const bfSSS_t* norm_key,
                                            unsigned int anchor,
                                            bfSSS_t delta_sq,
                                            const bfSSS_t delta_conj[8],
                                            const uint8_t coeffs[4][4][8],
                                            const uint8_t constants[4],
                                            uint32_t (*word_map)(uint32_t)) {
  sm4_norm_word_expr_t inv_word;
  for (unsigned int j = 0u; j != 4u; ++j) {
    bfSSS_t z_norm;
    const unsigned int nibble_idx = anchor * 4u + j;
    sm4_norm_inv_from_norm_verifier(&inv_word.byte[j], &z_norm, &in->byte[j],
                                    norm_key + nibble_idx * 4u, delta_sq, delta_conj);
    if (hasher) {
      zk_hash_SSS_update(hasher, z_norm);
    }
  }
  sm4_norm_word_map_after_affine(out, &inv_word, coeffs, constants, word_map);
}

static void sm4_norm_build_key_words_prover(const uint8_t* w, const bfSSS_t* w_tag,
                                            sm4_norm_word_expr_t K[36],
                                            zk_hash_SSS_3_ctx* hasher) {
  for (unsigned int word = 0u; word != 4u; ++word) {
    const unsigned int off = sm4_norm_key_full_byte_offset(word);
    sm4_norm_word_from_bits(&K[word], w + off, w_tag + off * 8u);
  }

  for (unsigned int round = 0u; round != SM4_ROUNDS; ++round) {
    const unsigned int word = round + 4u;
    sm4_norm_word_expr_t ck;
    sm4_norm_word_expr_t sbox_in;
    sm4_norm_word_expr_t t_word;

    sm4_norm_public_u32_prover(&ck, CK[round]);
    sm4_norm_word_add4(&sbox_in, &K[round + 1u], &K[round + 2u],
                       &K[round + 3u], &ck);

    if (sm4_norm_is_key_anchor(word)) {
      const unsigned int anchor = (word - 4u) >> 2u;
      sm4_norm_anchor_t_word_prover(&t_word, hasher, &sbox_in, w + 16u,
                                    w_tag + 16u * 8u, anchor,
                                    SM4_WORDMAP_LP_AFTER_AFFINE,
                                    SM4_WORDMAP_LP_AFTER_AFFINE_CONST,
                                    sm4_word_map_lprime);
      sm4_norm_word_add(&K[word], &K[round], &t_word);
    } else {
      const unsigned int off = sm4_norm_key_full_byte_offset(word);
      sm4_norm_word_expr_t diff;
      sm4_norm_word_expr_t y_pre;
      sm4_norm_word_from_bits(&K[word], w + off, w_tag + off * 8u);
      if (hasher) {
        sm4_norm_word_add(&diff, &K[round], &K[word]);
        sm4_norm_word_Lprime_inv(&y_pre, &diff);
        for (unsigned int j = 0u; j != 4u; ++j) {
          sm4_norm_byte_expr_t x;
          sm4_norm_byte_expr_t y;
          sm4_norm_byte_affine2_prover(&x, &sbox_in.byte[j]);
          sm4_norm_byte_inv_affine2_prover(&y, &y_pre.byte[j]);
          sm4_norm_append_io_prover(hasher, &x, &y);
        }
      }
    }
  }
}

static void sm4_norm_build_key_words_verifier(const bfSSS_t* w_key,
                                              sm4_norm_word_expr_t K[36],
                                              zk_hash_SSS_ctx* hasher,
                                              bfSSS_t delta,
                                              const bfSSS_t delta_conj[8]) {
  const bfSSS_t delta_sq = bfSSS_sqr(delta);

  for (unsigned int word = 0u; word != 4u; ++word) {
    const unsigned int off = sm4_norm_key_full_byte_offset(word);
    sm4_norm_word_from_key(&K[word], w_key + off * 8u);
  }

  for (unsigned int round = 0u; round != SM4_ROUNDS; ++round) {
    const unsigned int word = round + 4u;
    sm4_norm_word_expr_t ck;
    sm4_norm_word_expr_t sbox_in;
    sm4_norm_word_expr_t t_word;

    sm4_norm_public_u32_verifier(&ck, CK[round], delta);
    sm4_norm_word_add4(&sbox_in, &K[round + 1u], &K[round + 2u],
                       &K[round + 3u], &ck);

    if (sm4_norm_is_key_anchor(word)) {
      const unsigned int anchor = (word - 4u) >> 2u;
      sm4_norm_anchor_t_word_verifier(&t_word, hasher, &sbox_in,
                                      w_key + 16u * 8u, anchor, delta_sq, delta_conj,
                                      SM4_WORDMAP_LP_AFTER_AFFINE,
                                      SM4_WORDMAP_LP_AFTER_AFFINE_CONST,
                                      sm4_word_map_lprime);
      sm4_norm_word_add(&K[word], &K[round], &t_word);
    } else {
      const unsigned int off = sm4_norm_key_full_byte_offset(word);
      sm4_norm_word_expr_t diff;
      sm4_norm_word_expr_t y_pre;
      sm4_norm_word_from_key(&K[word], w_key + off * 8u);
      if (hasher) {
        sm4_norm_word_add(&diff, &K[round], &K[word]);
        sm4_norm_word_Lprime_inv(&y_pre, &diff);
        for (unsigned int j = 0u; j != 4u; ++j) {
          sm4_norm_byte_expr_t x;
          sm4_norm_byte_expr_t y;
          sm4_norm_byte_affine2_verifier(&x, &sbox_in.byte[j], delta_conj);
          sm4_norm_byte_inv_affine2_verifier(&y, &y_pre.byte[j], delta_conj);
          sm4_norm_append_io_verifier(hasher, &x, &y, delta, delta_sq);
        }
      }
    }
  }
}

static void sm4_norm_build_enc_words_prover(const uint8_t* in, const bfSSS_t* in_tag,
                                            const uint8_t* out, const bfSSS_t* out_tag,
                                            const uint8_t* w, const bfSSS_t* w_tag,
                                            const sm4_norm_word_expr_t K[36],
                                            zk_hash_SSS_3_ctx* hasher) {
  sm4_norm_word_expr_t X[36];

  // read K0-K3 
  for (unsigned int word = 0u; word != 4u; ++word) {
    sm4_norm_word_from_bits(&X[word], in + word * 4u, in_tag + word * 32u);
  }

  for (unsigned int word = 32u; word != 36u; ++word) {
    const unsigned int out_word = 35u - word;
    sm4_norm_word_from_bits(&X[word], out + out_word * 4u, out_tag + out_word * 32u);
  }

  for (unsigned int round = 0u; round != SM4_ROUNDS; ++round) {
    const unsigned int word = round + 4u;
    sm4_norm_word_expr_t sbox_in;
    sm4_norm_word_expr_t t_word;

    sm4_norm_word_add4(&sbox_in, &X[round + 1u], &X[round + 2u],
                       &X[round + 3u], &K[round + 4u]);

    if (sm4_norm_is_enc_anchor(word)) {
      const unsigned int anchor = (word - 5u) >> 2u;
      sm4_norm_anchor_t_word_prover(&t_word, hasher, &sbox_in, w, w_tag, anchor,
                                    SM4_WORDMAP_L_AFTER_AFFINE,
                                    SM4_WORDMAP_L_AFTER_AFFINE_CONST,
                                    sm4_word_map_l);
      sm4_norm_word_add(&X[word], &X[round], &t_word);
    } else {
      sm4_norm_word_expr_t diff;
      sm4_norm_word_expr_t y_pre;
      if (word < 32u) {
        const unsigned int off = sm4_norm_enc_full_byte_offset(word);
        sm4_norm_word_from_bits(&X[word], w + off, w_tag + off * 8u);
      }
      sm4_norm_word_add(&diff, &X[round], &X[word]);
      sm4_norm_word_Linv(&y_pre, &diff);
      for (unsigned int j = 0u; j != 4u; ++j) {
        sm4_norm_byte_expr_t x;
        sm4_norm_byte_expr_t y;
        sm4_norm_byte_affine2_prover(&x, &sbox_in.byte[j]);
        sm4_norm_byte_inv_affine2_prover(&y, &y_pre.byte[j]);
        sm4_norm_append_io_prover(hasher, &x, &y);
      }
    }
  }
}

static void sm4_norm_build_enc_words_verifier(const bfSSS_t* in_key,
                                              const bfSSS_t* out_key,
                                              const bfSSS_t* w_key,
                                              const sm4_norm_word_expr_t K[36],
                                              zk_hash_SSS_ctx* hasher,
                                              bfSSS_t delta,
                                              const bfSSS_t delta_conj[8]) {
  const bfSSS_t delta_sq = bfSSS_sqr(delta);
  sm4_norm_word_expr_t X[36];

  for (unsigned int word = 0u; word != 4u; ++word) {
    sm4_norm_word_from_key(&X[word], in_key + word * 32u);
  }
  for (unsigned int word = 32u; word != 36u; ++word) {
    const unsigned int out_word = 35u - word;
    sm4_norm_word_from_key(&X[word], out_key + out_word * 32u);
  }

  for (unsigned int round = 0u; round != SM4_ROUNDS; ++round) {
    const unsigned int word = round + 4u;
    sm4_norm_word_expr_t sbox_in;
    sm4_norm_word_expr_t t_word;

    sm4_norm_word_add4(&sbox_in, &X[round + 1u], &X[round + 2u],
                       &X[round + 3u], &K[round + 4u]);

    if (sm4_norm_is_enc_anchor(word)) {
      const unsigned int anchor = (word - 5u) >> 2u;
      sm4_norm_anchor_t_word_verifier(&t_word, hasher, &sbox_in, w_key,
                                      anchor, delta_sq, delta_conj,
                                      SM4_WORDMAP_L_AFTER_AFFINE,
                                      SM4_WORDMAP_L_AFTER_AFFINE_CONST,
                                      sm4_word_map_l);
      sm4_norm_word_add(&X[word], &X[round], &t_word);
    } else {
      sm4_norm_word_expr_t diff;
      sm4_norm_word_expr_t y_pre;
      if (word < 32u) {
        const unsigned int off = sm4_norm_enc_full_byte_offset(word);
        sm4_norm_word_from_key(&X[word], w_key + off * 8u);
      }
      sm4_norm_word_add(&diff, &X[round], &X[word]);
      sm4_norm_word_Linv(&y_pre, &diff);
      for (unsigned int j = 0u; j != 4u; ++j) {
        sm4_norm_byte_expr_t x;
        sm4_norm_byte_expr_t y;
        sm4_norm_byte_affine2_verifier(&x, &sbox_in.byte[j], delta_conj);
        sm4_norm_byte_inv_affine2_verifier(&y, &y_pre.byte[j], delta_conj);
        sm4_norm_append_io_verifier(hasher, &x, &y, delta, delta_sq);
      }
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
  (void)params;
  (void)k_tag_is_zero;
  assert(!k_tag_is_zero);

  sm4_norm_word_expr_t K[36];
  sm4_norm_build_key_words_prover(k - 16u, k_tag - 16u * 8u, K, NULL);
  sm4_norm_build_enc_words_prover(in, in_tag, out, out_tag, w, w_tag, K, hasher);
}

/* ================================================================ */
/*  Helper: Constant to VOLE (for constants like CK)                */
/* ================================================================ */
static void constant_to_vole_SSS_prover(bfSSS_t* tag, unsigned int n) {
  // For constant values, the tag is zero (no VOLE component)
  memset(tag, 0, sizeof(bfSSS_t) * n);
}

// OWF VERIFIER
// clang-format off
void sm4_128_verifier(const params_t* params, uint8_t* a0_tilde, const uint8_t* d,
                      uint8_t** Q, const uint8_t* owf_in, const uint8_t* owf_out,
                      const uint8_t* chall_2, const uint8_t* chall_3,
                      const uint8_t* a1_tilde, const uint8_t* a2_tilde) {
                        
  const unsigned int ell = params->ell;

  // ::1
  bfSSS_t bf_delta    = bfSSS_load(chall_3);
  g_sm4_norm_verifier_mode = 1;
  g_sm4_current_delta = bf_delta;
  g_sm4_current_delta_sq = bfSSS_sqr(bf_delta);

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
  sm4_byte_combine_bits_lut_ensure();

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
  bfSSS_t tmp2 = bfSSS_mul(bfSSS_load(a2_tilde), g_sm4_current_delta_sq);
  bfSSS_t ret  = bfSSS_add(bfSSS_add(bfSSS_load(q_tilde), tmp1), tmp2);

  bfSSS_store(a0_tilde, ret);
}

// OWF CONSTRAINTS VERIFIER

static void sm4_SSS_constraints_verifier(const params_t* params, zk_hash_SSS_ctx* hasher,
                                         const bfSSS_t* w_key, const uint8_t* owf_in,
                                         const uint8_t* owf_out, bfSSS_t delta) {

  const unsigned int Lke      = params->lke;
  const unsigned int blocksize = SM4_BLOCK_SIZE * 8; // in bits
  const bool use_em     = is_em(params);

  // ::4-5
  // constraint: w[0] * w[1] = 0
  // verifier (Deg2): q0 * q1
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

    /*
     * Same reuse as the prover side: KeyExp reconstructs all virtual round-key
     * expressions and appends its constraints, then Enc consumes those
     * expressions directly.
     */
    bfSSS_t delta_conj[8];
    sm4_norm_word_expr_t K[36];
    sm4_norm_delta_conjugates(delta_conj, delta);
    sm4_norm_build_key_words_verifier(w_key, K, hasher, delta, delta_conj);
    sm4_norm_build_enc_words_verifier(in_key, out_key, w_key + Lke, K, hasher,
                                      delta, delta_conj);
  }

}

	// // Enc CSTRNTS VERIFIER
	static void sm4_SSS_enc_constraints_verifier(const params_t* params, zk_hash_SSS_ctx* hasher,
                                             const bfSSS_t* in_key, const bfSSS_t* out_key,
                                             const bfSSS_t* w_key, const bfSSS_t* k_key,
                                             const bfSSS_t delta) {
  (void)params;
  bfSSS_t delta_conj[8];
  sm4_norm_word_expr_t K[36];
  sm4_norm_delta_conjugates(delta_conj, delta);
  sm4_norm_build_key_words_verifier(k_key - 16u * 8u, K, NULL, delta, delta_conj);
  sm4_norm_build_enc_words_verifier(in_key, out_key, w_key, K, hasher, delta, delta_conj);
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
