/*
 *  SPDX-License-Identifier: MIT
 */

#include "lynx_matrices.h"

#include "fields.h"
#include "xof.h"
#include "lynx_owf_consts.h"
#include "utils.h"

#include <stdlib.h>
#include <string.h>

#define LYNX_MAT_DOMAIN "LYNX-SAMPLEMATVEC-v1"
typedef struct lynx_rng_t {
  hash_context ctx;
} lynx_rng_t;

static unsigned int seclvl_from_lambda(unsigned int lambda) {
  if (lambda == 128 || lambda == 160 || lambda == 192 || lambda == 256 || lambda == 384 || lambda == 512) {
    return lambda;
  }
  return 0;
}

static void rng_init(lynx_rng_t* rng, unsigned int lambda, const uint8_t* seed, size_t seed_len) {
  const uint8_t domain[] = LYNX_MAT_DOMAIN;
  const uint8_t lambda_le[2] = {(uint8_t)(lambda & 0xff), (uint8_t)((lambda >> 8) & 0xff)};
  const uint8_t seed_len_le[2] = {(uint8_t)(seed_len & 0xff),
                                  (uint8_t)((seed_len >> 8) & 0xff)};

  hash_init(&rng->ctx, seclvl_from_lambda(lambda));
  hash_update(&rng->ctx, domain, sizeof(domain) - 1);
  hash_update(&rng->ctx, lambda_le, sizeof(lambda_le));
  hash_update(&rng->ctx, seed_len_le, sizeof(seed_len_le));
  hash_update(&rng->ctx, seed, seed_len);
  hash_final(&rng->ctx);
}

static void rng_clear(lynx_rng_t* rng) {
  hash_clear(&rng->ctx);
}

#if 0
static inline uint8_t gf2_row_bit(const uint64_t row[LYNX_MAX_WORDS], unsigned int col) {
  return (uint8_t)((row[col / 64] >> (col % 64)) & 1u);
}

static inline void gf2_row_swap(uint64_t lhs[LYNX_MAX_WORDS], uint64_t rhs[LYNX_MAX_WORDS],
                                unsigned int words) {
  for (unsigned int i = 0; i < words; ++i) {
    const uint64_t t = lhs[i];
    lhs[i] = rhs[i];
    rhs[i] = t;
  }
}
#endif

typedef struct lynx_bitstream_t {
  lynx_rng_t* rng;
  uint8_t buf[256];
  size_t byte_pos;
  size_t byte_len;
} lynx_bitstream_t;

static void bitstream_init(lynx_bitstream_t* bs, lynx_rng_t* rng) {
  bs->rng = rng;
  bs->byte_pos = 0;
  bs->byte_len = 0;
}

static inline uint64_t load_u64_le(const uint8_t in[8]) {
  return ((uint64_t)in[0]) | ((uint64_t)in[1] << 8) | ((uint64_t)in[2] << 16) |
         ((uint64_t)in[3] << 24) | ((uint64_t)in[4] << 32) | ((uint64_t)in[5] << 40) |
         ((uint64_t)in[6] << 48) | ((uint64_t)in[7] << 56);
}

static uint64_t bitstream_get_u64(lynx_bitstream_t* bs) {
  uint8_t tmp[8];
  size_t copied = 0;

  while (copied < sizeof(tmp)) {
    if (bs->byte_pos >= bs->byte_len) {
      hash_squeeze(&bs->rng->ctx, bs->buf, sizeof(bs->buf));
      bs->byte_pos = 0;
      bs->byte_len = sizeof(bs->buf);
    }
    const size_t avail = bs->byte_len - bs->byte_pos;
    const size_t need = sizeof(tmp) - copied;
    const size_t take = avail < need ? avail : need;
    memcpy(tmp + copied, bs->buf + bs->byte_pos, take);
    bs->byte_pos += take;
    copied += take;
  }

  return load_u64_le(tmp);
}

static inline void gf2_row_xor(uint64_t dst[LYNX_MAX_WORDS], const uint64_t src[LYNX_MAX_WORDS],
                               unsigned int words) {
  for (unsigned int i = 0; i < words; ++i) {
    dst[i] ^= src[i];
  }
}

static inline uint64_t lower_mask(unsigned int bit) {
  if (bit == 0) {
    return 0;
  }
  return (UINT64_C(1) << bit) - 1;
}

static inline uint64_t upper_mask(unsigned int bit) {
  if (bit == 63) {
    return 0;
  }
  return ~((UINT64_C(1) << (bit + 1)) - 1);
}

static void sample_lower_triangular(unsigned int lambda, unsigned int words, lynx_bitstream_t* bs,
                                    uint64_t L[LYNX_MAX_CSP][LYNX_MAX_WORDS]) {
  for (unsigned int r = 0; r < lambda; ++r) {
    const unsigned int diag_word = r / 64;
    const unsigned int diag_bit = r % 64;
    for (unsigned int w = 0; w < diag_word; ++w) {
      L[r][w] = bitstream_get_u64(bs);
    }
    L[r][diag_word] = (bitstream_get_u64(bs) & lower_mask(diag_bit)) | (UINT64_C(1) << diag_bit);
    for (unsigned int w = diag_word + 1; w < words; ++w) {
      L[r][w] = 0;
    }
  }
}

/* Legacy matrix validation/filtering code retained for reference only. */
#if 0
static int gf2_matrix_inverse(unsigned int lambda, unsigned int words,
                              const uint64_t A[LYNX_MAX_CSP][LYNX_MAX_WORDS],
                              uint64_t inv[LYNX_MAX_CSP][LYNX_MAX_WORDS]) {
  uint64_t work[LYNX_MAX_CSP][LYNX_MAX_WORDS];
  memset(work, 0, sizeof(work));
  memset(inv, 0, sizeof(uint64_t) * LYNX_MAX_CSP * LYNX_MAX_WORDS);

  for (unsigned int r = 0; r < lambda; ++r) {
    memcpy(work[r], A[r], sizeof(uint64_t) * words);
    inv[r][r / 64] = UINT64_C(1) << (r % 64);
  }

  for (unsigned int col = 0; col < lambda; ++col) {
    unsigned int pivot = col;
    while (pivot < lambda && gf2_row_bit(work[pivot], col) == 0) {
      ++pivot;
    }
    if (pivot == lambda) {
      return -1;
    }

    if (pivot != col) {
      gf2_row_swap(work[col], work[pivot], words);
      gf2_row_swap(inv[col], inv[pivot], words);
    }

    for (unsigned int row = 0; row < lambda; ++row) {
      if (row != col && gf2_row_bit(work[row], col)) {
        gf2_row_xor(work[row], work[col], words);
        gf2_row_xor(inv[row], inv[col], words);
      }
    }
  }

  return 0;
}

#define DECL_FIELD_HELPERS(PFX, TYPE, N, BYTES, LOAD, STORE, ZERO, ONE, ADD, MUL)               \
  static inline TYPE PFX##_from_basis_bit(unsigned int bit_idx) {                                 \
    uint8_t bytes[BYTES] = {0};                                                                    \
    ptr_set_bit(bytes, bit_idx, 1);                                                                \
    return LOAD(bytes);                                                                            \
  }                                                                                                \
                                                                                                   \
  static inline int PFX##_is_zero(TYPE x) {                                                        \
    uint8_t bytes[BYTES];                                                                          \
    STORE(bytes, x);                                                                               \
    for (unsigned int i = 0; i < BYTES; ++i) {                                                     \
      if (bytes[i] != 0) {                                                                         \
        return 0;                                                                                  \
      }                                                                                            \
    }                                                                                              \
    return 1;                                                                                      \
  }                                                                                                \
                                                                                                   \
  static inline TYPE PFX##_inv(TYPE x) {                                                           \
    TYPE result = ONE();                                                                           \
    for (int i = (N)-1; i >= 0; --i) {                                                             \
      result = MUL(result, result);                                                                \
      if (i != 0) {                                                                                \
        result = MUL(result, x);                                                                   \
      }                                                                                            \
    }                                                                                              \
    return result;                                                                                 \
  }                                                                                                \
                                                                                                   \
  static int PFX##_mat_inverse(const TYPE* A_in, TYPE* A_inv) {                                    \
    TYPE* A = malloc(sizeof(TYPE) * (N) * (N));                                                    \
    if (!A) {                                                                                      \
      return -1;                                                                                   \
    }                                                                                              \
    memcpy(A, A_in, sizeof(TYPE) * (N) * (N));                                                     \
    for (unsigned int r = 0; r < (N); ++r) {                                                       \
      for (unsigned int c = 0; c < (N); ++c) {                                                     \
        A_inv[r * (N) + c] = (r == c) ? ONE() : ZERO();                                            \
      }                                                                                            \
    }                                                                                              \
    for (unsigned int col = 0; col < (N); ++col) {                                                 \
      unsigned int pivot = col;                                                                    \
      while (pivot < (N) && PFX##_is_zero(A[pivot * (N) + col])) {                                \
        ++pivot;                                                                                   \
      }                                                                                            \
      if (pivot == (N)) {                                                                          \
        free(A);                                                                                   \
        return -1;                                                                                 \
      }                                                                                            \
      if (pivot != col) {                                                                          \
        for (unsigned int c = 0; c < (N); ++c) {                                                   \
          TYPE t1 = A[col * (N) + c];                                                              \
          A[col * (N) + c] = A[pivot * (N) + c];                                                   \
          A[pivot * (N) + c] = t1;                                                                 \
          TYPE t2 = A_inv[col * (N) + c];                                                          \
          A_inv[col * (N) + c] = A_inv[pivot * (N) + c];                                           \
          A_inv[pivot * (N) + c] = t2;                                                             \
        }                                                                                          \
      }                                                                                            \
      TYPE pivot_inv = PFX##_inv(A[col * (N) + col]);                                              \
      for (unsigned int c = 0; c < (N); ++c) {                                                     \
        A[col * (N) + c] = MUL(A[col * (N) + c], pivot_inv);                                       \
        A_inv[col * (N) + c] = MUL(A_inv[col * (N) + c], pivot_inv);                               \
      }                                                                                            \
      for (unsigned int r = 0; r < (N); ++r) {                                                     \
        if (r == col) {                                                                            \
          continue;                                                                                \
        }                                                                                          \
        TYPE factor = A[r * (N) + col];                                                            \
        if (PFX##_is_zero(factor)) {                                                               \
          continue;                                                                                \
        }                                                                                          \
        for (unsigned int c = 0; c < (N); ++c) {                                                   \
          A[r * (N) + c] = ADD(A[r * (N) + c], MUL(factor, A[col * (N) + c]));                    \
          A_inv[r * (N) + c] = ADD(A_inv[r * (N) + c], MUL(factor, A_inv[col * (N) + c]));        \
        }                                                                                          \
      }                                                                                            \
    }                                                                                              \
    free(A);                                                                                       \
    return 0;                                                                                      \
  }                                                                                                \
                                                                                                   \
  static const TYPE* PFX##_pinv(void) {                                                            \
    static TYPE* pinv = NULL;                                                                      \
    if (pinv) {                                                                                    \
      return pinv;                                                                                 \
    }                                                                                              \
    TYPE* P = malloc(sizeof(TYPE) * (N) * (N));                                                    \
    TYPE* I = malloc(sizeof(TYPE) * (N) * (N));                                                    \
    if (!P || !I) {                                                                                \
      free(P);                                                                                     \
      free(I);                                                                                     \
      return NULL;                                                                                 \
    }                                                                                              \
    for (unsigned int col = 0; col < (N); ++col) {                                                 \
      TYPE v = PFX##_from_basis_bit(col);                                                          \
      for (unsigned int row = 0; row < (N); ++row) {                                               \
        P[row * (N) + col] = v;                                                                    \
        v = MUL(v, v);                                                                             \
      }                                                                                            \
    }                                                                                              \
    if (PFX##_mat_inverse(P, I) != 0) {                                                            \
      free(P);                                                                                     \
      free(I);                                                                                     \
      return NULL;                                                                                 \
    }                                                                                              \
    free(P);                                                                                       \
    pinv = I;                                                                                      \
    return pinv;                                                                                   \
  }                                                                                                \
                                                                                                   \
  static int PFX##_coeffs_nonzero(const uint64_t M[LYNX_MAX_CSP][LYNX_MAX_WORDS],              \
                                  const uint64_t Minv[LYNX_MAX_CSP][LYNX_MAX_WORDS]) {         \
    const TYPE* pinv = PFX##_pinv();                                                               \
    TYPE* y = NULL;                                                                                \
    TYPE* y_inv = NULL;                                                                            \
    if (!pinv) {                                                                                   \
      return 0;                                                                                    \
    }                                                                                              \
    y = malloc(sizeof(TYPE) * (N));                                                                \
    y_inv = malloc(sizeof(TYPE) * (N));                                                            \
    if (!y || !y_inv) {                                                                            \
      free(y);                                                                                     \
      free(y_inv);                                                                                 \
      return 0;                                                                                    \
    }                                                                                              \
    for (unsigned int col = 0; col < (N); ++col) {                                                 \
      uint8_t bytes[BYTES] = {0};                                                                  \
      uint8_t bytes_inv[BYTES] = {0};                                                              \
      for (unsigned int row = 0; row < (N); ++row) {                                               \
        ptr_set_bit(bytes, row, (M[row][col / 64] >> (col % 64)) & 1u);                           \
        ptr_set_bit(bytes_inv, row, (Minv[row][col / 64] >> (col % 64)) & 1u);                    \
      }                                                                                            \
      y[col] = LOAD(bytes);                                                                        \
      y_inv[col] = LOAD(bytes_inv);                                                                \
    }                                                                                              \
    for (unsigned int i = 0; i < (N); ++i) {                                                       \
      TYPE c = ZERO();                                                                             \
      TYPE c_inv = ZERO();                                                                         \
      for (unsigned int j = 0; j < (N); ++j) {                                                     \
        c = ADD(c, MUL(y[j], pinv[j * (N) + i]));                                                  \
        c_inv = ADD(c_inv, MUL(y_inv[j], pinv[j * (N) + i]));                                      \
      }                                                                                            \
      if (PFX##_is_zero(c) || PFX##_is_zero(c_inv)) {                                              \
        free(y);                                                                                   \
        free(y_inv);                                                                               \
        return 0;                                                                                  \
      }                                                                                            \
    }                                                                                              \
    free(y);                                                                                       \
    free(y_inv);                                                                                   \
    return 1;                                                                                      \
  }

DECL_FIELD_HELPERS(f128, bf128_t, 128, BF128_NUM_BYTES, bf128_load, bf128_store, bf128_zero,
                   bf128_one, bf128_add, bf128_mul)
DECL_FIELD_HELPERS(f192, bf192_t, 192, BF192_NUM_BYTES, bf192_load, bf192_store, bf192_zero,
                   bf192_one, bf192_add, bf192_mul)
DECL_FIELD_HELPERS(f256, bf256_t, 256, BF256_NUM_BYTES, bf256_load, bf256_store, bf256_zero,
                   bf256_one, bf256_add, bf256_mul)
DECL_FIELD_HELPERS(f384, bf384_t, 384, BF384_NUM_BYTES, bf384_load, bf384_store, bf384_zero,
                   bf384_one, bf384_add, bf384_mul)
DECL_FIELD_HELPERS(f512, bf512_t, 512, BF512_NUM_BYTES, bf512_load, bf512_store, bf512_zero,
                   bf512_one, bf512_add, bf512_mul)
#endif /* 0 */

static void clear_matrix_rows(unsigned int lambda, unsigned int words,
                              uint64_t M[LYNX_MAX_CSP][LYNX_MAX_WORDS]) {
  for (unsigned int r = 0; r < lambda; ++r) {
    memset(M[r], 0, sizeof(uint64_t) * words);
  }
}

static unsigned int ctz_u64_nonzero(uint64_t x) {
  static const uint8_t index[64] = {
      0,  1,  48, 2,  57, 49, 28, 3,  61, 58, 50, 42, 38, 29, 17, 4,
      62, 55, 59, 36, 53, 51, 43, 22, 45, 39, 33, 30, 24, 18, 12, 5,
      63, 47, 56, 27, 60, 41, 37, 16, 54, 35, 52, 21, 44, 32, 23, 11,
      46, 26, 40, 15, 34, 20, 31, 10, 25, 14, 19, 9,  13, 8,  7,  6,
  };
  const uint64_t isolated = x & (~x + 1);
  return index[(isolated * UINT64_C(0x03f79d71b4cb0a89)) >> 58];
}

static void accumulate_upper_word_product(uint64_t dst[LYNX_MAX_WORDS], unsigned int lambda,
                                          unsigned int words,
                                          uint64_t lower[LYNX_MAX_CSP][LYNX_MAX_WORDS],
                                          unsigned int base_col, uint64_t upper_word) {
  (void)lambda;
  while (upper_word) {
    const unsigned int bit = ctz_u64_nonzero(upper_word);
    gf2_row_xor(dst, lower[base_col + bit], words);
    upper_word &= upper_word - 1;
  }
}

static void sample_matrix(lynx_rng_t* rng, unsigned int lambda, unsigned int words,
                          uint64_t M[LYNX_MAX_CSP][LYNX_MAX_WORDS]) {
  uint64_t lower[LYNX_MAX_CSP][LYNX_MAX_WORDS];
  lynx_bitstream_t bs;
  bitstream_init(&bs, rng);
  sample_lower_triangular(lambda, words, &bs, lower);
  clear_matrix_rows(lambda, words, M);

  for (unsigned int r = 0; r < lambda; ++r) {
    const unsigned int diag_word = r / 64;
    const unsigned int diag_bit = r % 64;
    const unsigned int valid_bits = lambda - diag_word * 64;
    const uint64_t word_mask = valid_bits >= 64 ? UINT64_MAX : ((UINT64_C(1) << valid_bits) - 1);
    const uint64_t diag =
        (bitstream_get_u64(&bs) & upper_mask(diag_bit)) | (UINT64_C(1) << diag_bit);

    accumulate_upper_word_product(M[r], lambda, words, lower, diag_word * 64, diag & word_mask);
    for (unsigned int w = diag_word + 1; w < words; ++w) {
      const unsigned int remaining_bits = lambda - w * 64;
      const uint64_t upper_mask_word =
          remaining_bits >= 64 ? UINT64_MAX : ((UINT64_C(1) << remaining_bits) - 1);
      const uint64_t upper_word = bitstream_get_u64(&bs) & upper_mask_word;
      accumulate_upper_word_product(M[r], lambda, words, lower, w * 64, upper_word);
    }
  }
}

static void load_fixed_l3(lynx_t* mats) {
  switch (mats->lambda) {
  case 128:
    memcpy(mats->L3, LYNX_L3_128, sizeof(LYNX_L3_128));
    break;
  case 160:
    memcpy(mats->L3, LYNX_L3_160, sizeof(LYNX_L3_160));
    break;
  case 192:
    memcpy(mats->L3, LYNX_L3_192, sizeof(LYNX_L3_192));
    break;
  case 256:
    memcpy(mats->L3, LYNX_L3_256, sizeof(LYNX_L3_256));
    break;
  case 384:
    memcpy(mats->L3, LYNX_L3_384, sizeof(LYNX_L3_384));
    break;
  case 512:
    memcpy(mats->L3, LYNX_L3_512, sizeof(LYNX_L3_512));
    break;
  default:
    break;
  }
}

#if 0
static int coeff_filter_pass(unsigned int lambda, const uint64_t M[LYNX_MAX_CSP][LYNX_MAX_WORDS],
                             const uint64_t Minv[LYNX_MAX_CSP][LYNX_MAX_WORDS]) {
  switch (lambda) {
  case 128:
    return f128_coeffs_nonzero(M, Minv);
  case 192:
    return f192_coeffs_nonzero(M, Minv);
  case 256:
    return f256_coeffs_nonzero(M, Minv);
  case 384:
    return f384_coeffs_nonzero(M, Minv);
  case 512:
    return f512_coeffs_nonzero(M, Minv);
  default:
    return 0;
  }
}

static int matrix_passes_invertibility_and_coeff_filter(
    unsigned int lambda, unsigned int words,
    const uint64_t M[LYNX_MAX_CSP][LYNX_MAX_WORDS]) {
  uint64_t inv_tmp[LYNX_MAX_CSP][LYNX_MAX_WORDS];
  return gf2_matrix_inverse(lambda, words, M, inv_tmp) == 0 && coeff_filter_pass(lambda, M, inv_tmp);
}
#endif

int lynx_is_valid(const lynx_t* mats) {
  (void)mats;
  return 1;
}

int lynx_seed_is_valid(unsigned int lambda, const uint8_t* seed, size_t seed_len) {
  (void)lambda;
  (void)seed;
  (void)seed_len;
  return 1;
}

int lynx_init(lynx_t* mats, unsigned int lambda, const uint8_t* seed,
              size_t seed_len) {
  lynx_rng_t rng;

  if (!mats || !seed || seed_len == 0) {
    return -1;
  }
  if (lambda != 128 && lambda != 160 && lambda != 192 && lambda != 256 && lambda != 384 && lambda != 512) {
    return -1;
  }

  memset(mats, 0, sizeof(*mats));
  mats->lambda = lambda;
  mats->words = (lambda + 63) / 64;

  rng_init(&rng, lambda, seed, seed_len);
  sample_matrix(&rng, lambda, mats->words, mats->L0);
  sample_matrix(&rng, lambda, mats->words, mats->L1);
  sample_matrix(&rng, lambda, mats->words, mats->L2);
  load_fixed_l3(mats);

  unsigned int bytes = lambda / 8;
  hash_squeeze(&rng.ctx, mats->C0, bytes);
  hash_squeeze(&rng.ctx, mats->C1, bytes);
  if (lambda == 256 || lambda == 384 || lambda == 512) {
    hash_squeeze(&rng.ctx, mats->C2, bytes);
  }
  rng_clear(&rng);

  return 0;
}
