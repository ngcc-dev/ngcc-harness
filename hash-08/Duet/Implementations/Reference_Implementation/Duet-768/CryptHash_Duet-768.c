/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.
*/

#include "CryptHash_Duet-768.h"

#include <stdint.h>
#include <string.h>

enum {
  r = 1088,
  c = 832,

  WORD_BITS = 64,

  WORDS_r = r / WORD_BITS,
  WORDS_c = c / WORD_BITS,
  WORDS_perm = WORDS_r + WORDS_c,

  R_BYTES = WORDS_r * 8,
  C_BYTES = WORDS_c * 8,
  PERM_BYTES = WORDS_perm * 8,

  ROUNDS = 18
};

enum {
  STATE1920_COLS = 6,
  STATE1920_ROWS = 5,
  STATE1920_WORDS = STATE1920_COLS * STATE1920_ROWS
};

typedef uint64_t state1920_t[6][5];

static const uint64_t E[2 * ROUNDS] = {
    0xb7e151628aed2a6a, 0xbf7158809cf4f3c7, 0x62e7160f38b4da56,
    0xa784d9045190cfef, 0x324e7738926cfbe5, 0xf4bf8d8d8c31d763,
    0xda06c80abb1185eb, 0x4f7c7b5757f59584, 0x90cfd47d7c19bb42,
    0x158d9554f7b46bce, 0xd55c4d79fd5f24d6, 0x613c31c3839a2ddf,
    0x8a9a276bcfbfa1c8, 0x77c56284dab79cd4, 0xc2b3293d20e9e5ea,
    0xf02ac60acc93ed87, 0x4422a52ecb238fee, 0xe5ab6add835fd1a0,
    0x753d0a8f78e537d2, 0xb95bb79d8dcaec64, 0x2c1e9f23b829b5c2,
    0x780bf38737df8bb3, 0x00d01334a0d0bd86, 0x45cbfa73a6160ffe,
    0x393c48cbbbca060f, 0x0ff8ec6d31beb5cc, 0xeed7f2f0bb088017,
    0x163bc60df45a0ecb, 0x1bcd289b06cbbfea, 0x21ad08e1847f3f73,
    0x78d56ced94640d6e, 0xf0d3d37be67008e1, 0x86d1bf275b9b241d,
    0xeb64749a47dfdfb9, 0x6632c3eb061b6472, 0xbbf84c26144e49c2,
};

#define STATE1920_INDEX(x, y) ((x) * STATE1920_ROWS + (y))

static inline uint64_t rotl64(uint64_t x, unsigned int n) {
  n &= 63U;
  return (x << n) | (x >> ((64U - n) & 63U));
}

static inline void permutation1920_sb(state1920_t state) {
  /* Apply the 5-bit S-box to each column in bitsliced form. */
  for (unsigned int col = 0; col < STATE1920_COLS; ++col) {
    uint64_t x0 = state[col][0];
    uint64_t x1 = state[col][1];
    uint64_t x2 = state[col][2];
    uint64_t x3 = state[col][3];
    uint64_t x4 = state[col][4];

    uint64_t t0 = x3 & x4;
    uint64_t t1 = x2 & x4;
    uint64_t t2 = x2 & x3;
    uint64_t t3 = x1 & x4;
    uint64_t t4 = x1 & x2;
    uint64_t t5 = x0 & x1;
    uint64_t t6 = x4 ^ x2;
    uint64_t t7 = x0 ^ t0;
    uint64_t t8 = t1 ^ t2;
    uint64_t t9 = t3 ^ t4;
    uint64_t t10 = t5 ^ t6;
    uint64_t t11 = t7 ^ t8;
    uint64_t t12 = t9 ^ t10;
    uint64_t t13 = t11 ^ t12;
    uint64_t t14 = ~t13;

    uint64_t t15 = x1 & x3;
    uint64_t t16 = x0 & x2;
    uint64_t t17 = x3 ^ x2;
    uint64_t t18 = x0 ^ t1;
    uint64_t t19 = t2 ^ t15;
    uint64_t t20 = t4 ^ t16;
    uint64_t t21 = t17 ^ t18;
    uint64_t t22 = t19 ^ t20;
    uint64_t t23 = t21 ^ t22;
    uint64_t t24 = ~t23;

    uint64_t t25 = x0 & x4;
    uint64_t t26 = x2 ^ x0;
    uint64_t t27 = t0 ^ t1;
    uint64_t t28 = t3 ^ t15;
    uint64_t t29 = t4 ^ t25;
    uint64_t t30 = t16 ^ t5;
    uint64_t t31 = t26 ^ t27;
    uint64_t t32 = t28 ^ t29;
    uint64_t t33 = t30 ^ t31;
    uint64_t t34 = t32 ^ t33;

    uint64_t t35 = x0 & x3;
    uint64_t t36 = x4 ^ x3;
    uint64_t t37 = x1 ^ x0;
    uint64_t t38 = t4 ^ t35;
    uint64_t t39 = t16 ^ t5;
    uint64_t t40 = t36 ^ t37;
    uint64_t t41 = t38 ^ t39;
    uint64_t t42 = t40 ^ t41;
    uint64_t t43 = ~t42;

    uint64_t t44 = x4 ^ x2;
    uint64_t t45 = x1 ^ x0;
    uint64_t t46 = t3 ^ t15;
    uint64_t t47 = t4 ^ t25;
    uint64_t t48 = t5 ^ t44;
    uint64_t t49 = t45 ^ t46;
    uint64_t t50 = t47 ^ t48;
    uint64_t t51 = t49 ^ t50;

    state[col][0] = t14;
    state[col][1] = t24;
    state[col][2] = t34;
    state[col][3] = t43;
    state[col][4] = t51;
  }
}

static inline void permutation1920_l30(state1920_t state) {
  uint64_t in[STATE1920_WORDS];
  uint64_t out[STATE1920_WORDS];

  for (unsigned int x = 0; x < STATE1920_COLS; ++x) {
    for (unsigned int y = 0; y < STATE1920_ROWS; ++y) {
      in[STATE1920_INDEX(x, y)] = state[x][y];
    }
  }

  for (unsigned int i = 0; i < STATE1920_WORDS; ++i) {
    uint64_t t0 = in[i] ^ in[(i + 3U) % STATE1920_WORDS];

    uint64_t t1 =
        in[(i + 10U) % STATE1920_WORDS] ^ in[(i + 13U) % STATE1920_WORDS];

    uint64_t t2 =
        in[(i + 14U) % STATE1920_WORDS] ^ in[(i + 15U) % STATE1920_WORDS];

    uint64_t t3 =
        in[(i + 16U) % STATE1920_WORDS] ^ in[(i + 18U) % STATE1920_WORDS];

    uint64_t t4 =
        in[(i + 21U) % STATE1920_WORDS] ^ in[(i + 26U) % STATE1920_WORDS];

    uint64_t u0 = t0 ^ t1;
    uint64_t u1 = t2 ^ t3;
    uint64_t u2 = t4 ^ in[(i + 28U) % STATE1920_WORDS];

    out[i] = (u0 ^ u1) ^ u2;
  }

  for (unsigned int x = 0; x < STATE1920_COLS; ++x) {
    for (unsigned int y = 0; y < STATE1920_ROWS; ++y) {
      state[x][y] = out[STATE1920_INDEX(x, y)];
    }
  }
}

static inline void permutation1920_l64(state1920_t state) {
  for (unsigned int x = 0; x < STATE1920_COLS; ++x) {
    for (unsigned int y = 0; y < STATE1920_ROWS; ++y) {
      uint64_t word = state[x][y];
      unsigned int shift = x * STATE1920_ROWS + y;

      state[x][y] = rotl64(word, shift) ^ rotl64(word, shift + 21U) ^
                    rotl64(word, shift + 43U);
    }
  }
}

static inline void permutation1920_round(state1920_t state,
                                         uint64_t round_const) {
    /* One permutation round: SC, MS, AC, ML. */
  permutation1920_sb(state);
  permutation1920_l30(state);
  state[0][0] ^= round_const;
  permutation1920_l64(state);
}

static void words_to_state1920(state1920_t state,
                               const uint64_t words[STATE1920_WORDS]) {
  for (unsigned int x = 0; x < STATE1920_COLS; ++x) {
    for (unsigned int y = 0; y < STATE1920_ROWS; ++y) {
      state[x][y] = words[STATE1920_INDEX(x, y)];
    }
  }
}

static void state1920_to_words(uint64_t words[STATE1920_WORDS],
                               state1920_t state) {
  for (unsigned int x = 0; x < STATE1920_COLS; ++x) {
    for (unsigned int y = 0; y < STATE1920_ROWS; ++y) {
      words[STATE1920_INDEX(x, y)] = state[x][y];
    }
  }
}

static void permutation1920(uint64_t state_words[WORDS_perm],
                            unsigned int flag) {
  state1920_t state;
  unsigned int base = (flag == 2U) ? ROUNDS : 0U;

    /* f1 and f2 share the same round function but use disjoint round constants. */
  words_to_state1920(state, state_words);
  for (unsigned int round = 0; round < ROUNDS; ++round) {
    permutation1920_round(state, E[base + round]);
  }
  state1920_to_words(state_words, state);
}

static void f1_permutation(uint64_t state[WORDS_perm]) {
  permutation1920(state, 1U);
}

static void f2_permutation(uint64_t state[WORDS_perm]) {
  permutation1920(state, 2U);
}


static uint64_t load64_be(const unsigned char in[8]) {
  return ((uint64_t)in[0] << 56) | ((uint64_t)in[1] << 48) |
         ((uint64_t)in[2] << 40) | ((uint64_t)in[3] << 32) |
         ((uint64_t)in[4] << 24) | ((uint64_t)in[5] << 16) |
         ((uint64_t)in[6] << 8) | (uint64_t)in[7];
}

static void store64_be(unsigned char out[8], uint64_t x) {
  out[0] = (unsigned char)(x >> 56);
  out[1] = (unsigned char)(x >> 48);
  out[2] = (unsigned char)(x >> 40);
  out[3] = (unsigned char)(x >> 32);
  out[4] = (unsigned char)(x >> 24);
  out[5] = (unsigned char)(x >> 16);
  out[6] = (unsigned char)(x >> 8);
  out[7] = (unsigned char)x;
}

static void bytes_to_words(uint64_t *out, const uint8_t *in,
                           unsigned int words) {
  for (unsigned int i = 0; i < words; ++i) {
    out[i] = load64_be(in + 8U * i);
  }
}

static void words_to_bytes(uint8_t *out, const uint64_t *in,
                           unsigned int words) {
  for (unsigned int i = 0; i < words; ++i) {
    store64_be(out + 8U * i, in[i]);
  }
}

static void sigma_permutation(uint64_t block[WORDS_r]) {
  const uint64_t w0 = block[0];
  const uint64_t w1 = block[1];
  const uint64_t w2 = block[2];
  const uint64_t w3 = block[3];
  const uint64_t w4 = block[4];
  const uint64_t w5 = block[5];
  const uint64_t w6 = block[6];
  const uint64_t w7 = block[7];
  const uint64_t w8 = block[8];
  const uint64_t w9 = block[9];
  const uint64_t w10 = block[10];
  const uint64_t w11 = block[11];
  const uint64_t w12 = block[12];
  const uint64_t w13 = block[13];
  const uint64_t w14 = block[14];
  const uint64_t w15 = block[15];
  const uint64_t w16 = block[16];
  const uint64_t w17 = block[17];

  block[0] = w9;
  block[1] = w2;
  block[2] = w15;
  block[3] = w0;
  block[4] = w6;
  block[5] = w12;
  block[6] = w4;
  block[7] = w16;
  block[8] = w1;
  block[9] = w10;
  block[10] = w7;
  block[11] = w14;
  block[12] = w3;
  block[13] = w11;
  block[14] = w5;
  block[15] = w13;
  block[16] = w8;
}


static void pad_message_block(uint64_t block[WORDS_r], const uint8_t *msg,
                              uint64_t msg_len_bits) {
  uint8_t buf[R_BYTES];
  uint64_t full_bytes = msg_len_bits / 8U;
  uint32_t rem_bits = (uint32_t)(msg_len_bits % 8U);

  memset(buf, 0, sizeof(buf));
  /* Build the final block as M || 1 || 0^k, preserving partial-byte input bits. */

  if (full_bytes > 0U) {
    memcpy(buf, msg, (size_t)full_bytes);
  }

  if (rem_bits == 0U) {
    buf[full_bytes] = 0x80U;
  } else {
    buf[full_bytes] = msg[full_bytes];
    buf[full_bytes] &= (uint8_t)(0xffU << (8U - rem_bits));
    buf[full_bytes] |= (uint8_t)(0x80U >> rem_bits);
  }

  bytes_to_words(block, buf, WORDS_r);
}

static void xor_words(uint64_t *out, const uint64_t *a, const uint64_t *b,
                      unsigned int n) {
  for (unsigned int i = 0; i < n; ++i) {
    out[i] = a[i] ^ b[i];
  }
}

static void copy_words(uint64_t *out, const uint64_t *in, unsigned int n) {
  for (unsigned int i = 0; i < n; ++i) {
    out[i] = in[i];
  }
}

static void apply_f1(uint64_t rate_out[WORDS_r], uint64_t C_out[WORDS_c],
                     const uint64_t rate_in[WORDS_r],
                     const uint64_t C_in[WORDS_c]) {
  uint64_t perm[WORDS_perm];
  uint64_t D[WORDS_c];

  copy_words(perm, rate_in, WORDS_r);
  copy_words(perm + WORDS_r, C_in, WORDS_c);
  f1_permutation(perm);
  copy_words(rate_out, perm, WORDS_r);
  copy_words(D, perm + WORDS_r, WORDS_c);
  xor_words(C_out, D, C_in, WORDS_c);
}

static void apply_f2(uint64_t rate_out[WORDS_r], uint64_t C_out[WORDS_c],
                     const uint64_t rate_in[WORDS_r],
                     const uint64_t C_in[WORDS_c]) {
  uint64_t perm[WORDS_perm];
  uint64_t D[WORDS_c];

  copy_words(perm, rate_in, WORDS_r);
  copy_words(perm + WORDS_r, C_in, WORDS_c);
  f2_permutation(perm);
  copy_words(rate_out, perm, WORDS_r);
  copy_words(D, perm + WORDS_r, WORDS_c);
  xor_words(C_out, D, C_in, WORDS_c);
}

static void absorb_block(uint64_t A[WORDS_r], uint64_t B[WORDS_r],
                         uint64_t C[WORDS_c], const uint64_t Mi[WORDS_r]) {
  uint64_t SigmaMi[WORDS_r];
  uint64_t rate_in[WORDS_r];
  uint64_t A1[WORDS_r];
  uint64_t B1[WORDS_r];
  uint64_t C1[WORDS_c];
  uint64_t C2[WORDS_c];

  copy_words(SigmaMi, Mi, WORDS_r);
  sigma_permutation(SigmaMi);
  /* First branch absorbs Mi into A and updates the capacity. */
  xor_words(rate_in, A, Mi, WORDS_r);
  apply_f1(A1, C1, rate_in, C);
  /* Second branch absorbs sigma(Mi) into B using the updated capacity. */
  xor_words(rate_in, B, SigmaMi, WORDS_r);
  apply_f2(B1, C2, rate_in, C1);

  copy_words(A, A1, WORDS_r);
  copy_words(B, B1, WORDS_r);
  copy_words(C, C2, WORDS_c);
}

static void squeeze_copy_capacity(uint8_t *digest, int *out_offset,
                                  int *remaining_digest_bits,
                                  const uint64_t C[WORDS_c]) {
  uint8_t c_bytes[C_BYTES];
  int take_bits;
  int take_bytes;

  words_to_bytes(c_bytes, C, WORDS_c);

  take_bits = *remaining_digest_bits > c ? c : *remaining_digest_bits;
  take_bytes = (take_bits + 7) / 8;
  memcpy(digest + *out_offset, c_bytes, (size_t)take_bytes);

  *out_offset += take_bytes;
  *remaining_digest_bits -= take_bits;
}

static void clear_unused_digest_bits(uint8_t *digest, int digest_len_bits) {
  unsigned int rem_bits = (unsigned int)digest_len_bits & 7U;

  if (rem_bits != 0U) {
    digest[digest_len_bits / 8] &= (uint8_t)(0xffU << (8U - rem_bits));
  }
}

int CryptHash(int digest_len_bits, const unsigned char *msg,
              unsigned long long msg_len_bits, unsigned char *digest) {
  uint64_t A[WORDS_r] = {0};
  uint64_t B[WORDS_r] = {0};
  uint64_t C[WORDS_c] = {0};
  uint64_t Mi[WORDS_r];

  unsigned long long full_blocks = msg_len_bits / r;
  unsigned long long rem_bits = msg_len_bits % r;
  if (digest_len_bits <= 0 || digest_len_bits > 8192) {
    return -1;
  }
  if (digest == NULL) {
    return -2;
  }
  if (msg == NULL && msg_len_bits != 0) {
    return -3;
  }

  /* Absorb all complete message blocks before the final padding block. */
  for (unsigned long long blk = 0; blk < full_blocks; ++blk) {
    const uint8_t *p = msg + blk * R_BYTES;

    bytes_to_words(Mi, p, WORDS_r);
    absorb_block(A, B, C, Mi);
  }

  /* Always absorb the final padding block M || 1 || 0^k. */
  {
    static const uint8_t zero_block[R_BYTES] = {0};
    const uint8_t *p = msg == NULL ? zero_block : msg + full_blocks * R_BYTES;

    pad_message_block(Mi, p, rem_bits);
    absorb_block(A, B, C, Mi);
  }
  {
    int remaining_digest_bits = digest_len_bits;
    int out_offset = 0;
    /* Output starts from the finalized capacity, then alternates f1 and f2 as needed. */
    squeeze_copy_capacity(digest, &out_offset, &remaining_digest_bits, C);

    while (remaining_digest_bits > 0) {
      uint64_t A_new[WORDS_r];
      uint64_t B_new[WORDS_r];
      uint64_t C_new[WORDS_c];

      apply_f1(A_new, C_new, A, C);
      copy_words(A, A_new, WORDS_r);
      copy_words(C, C_new, WORDS_c);

      squeeze_copy_capacity(digest, &out_offset, &remaining_digest_bits, C);
      if (remaining_digest_bits == 0) {
        break;
      }

      apply_f2(B_new, C_new, B, C);
      copy_words(B, B_new, WORDS_r);
      copy_words(C, C_new, WORDS_c);

      squeeze_copy_capacity(digest, &out_offset, &remaining_digest_bits, C);
    }

    clear_unused_digest_bits(digest, digest_len_bits);
  }

  return 0;
}
