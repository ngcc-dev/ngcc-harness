/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.
*/

#include "CryptHash_Duet-512.h"

#include <stdint.h>
#include <string.h>

enum {
  r = 384,
  c = 576,

  WORD_BITS = 64,

  WORDS_r = r / WORD_BITS,
  WORDS_c = c / WORD_BITS,
  WORDS_perm = WORDS_r + WORDS_c,

  R_BYTES = WORDS_r * 8,
  C_BYTES = WORDS_c * 8,
  PERM_BYTES = WORDS_perm * 8,

  ROUNDS = 12
};

enum {
  STATE960_COLS = 3,
  STATE960_ROWS = 5,
  STATE960_WORDS = STATE960_COLS * STATE960_ROWS
};

typedef uint64_t state960_t[3][5];

static const uint64_t PI[2 * ROUNDS] = {
    0x243f6a8885a308d3, 0x13198a2e03707344, 0xa4093822299f31d0,
    0x082efa98ec4e6c89, 0x452821e638d01377, 0xbe5466cf34e90c6c,
    0xc0ac29b7c97c50dd, 0x3f84d5b5b5470917, 0x9216d5d98979fb1b,
    0xd1310ba698dfb5ac, 0x2ffd72dbd01adfb7, 0xb8e1afed6a267e96,
    0xba7c9045f12c7f99, 0x24a19947b3916cf7, 0x0801f2e2858efc16,
    0x636920d871574e69, 0xa458fea3f4933d7e, 0x0d95748f728eb658,
    0x718bcd5882154aee, 0x7b54a41dc25a59b5, 0x9c30d5392af26013,
    0xc5d1b023286085f0, 0xca417918b8db38ef, 0x8e79dcb0603a180e,
};

#define STATE960_INDEX(x, y) ((x) * STATE960_ROWS + (y))

static inline uint64_t rotl64(uint64_t x, unsigned int n) {
  n &= 63U;
  return (x << n) | (x >> ((64U - n) & 63U));
}

static inline void permutation960_sb(state960_t state) {
  /* Apply the 5-bit S-box to each column in bitsliced form. */
  for (unsigned int col = 0; col < STATE960_COLS; ++col) {
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

static inline void permutation960_l15(state960_t state) {
  uint64_t in[STATE960_WORDS];
  uint64_t out[STATE960_WORDS];

  for (unsigned int x = 0; x < STATE960_COLS; ++x) {
    for (unsigned int y = 0; y < STATE960_ROWS; ++y) {
      in[STATE960_INDEX(x, y)] = state[x][y];
    }
  }

  for (unsigned int i = 0; i < STATE960_WORDS; ++i) {
    uint64_t t0 = in[i] ^ in[(i + 3U) % STATE960_WORDS];

    uint64_t t1 = in[(i + 4U) % STATE960_WORDS] ^ in[(i + 5U) % STATE960_WORDS];

    uint64_t t2 = in[(i + 7U) % STATE960_WORDS] ^ in[(i + 8U) % STATE960_WORDS];

    uint64_t t3 = t0 ^ t1;
    uint64_t t4 = t2 ^ in[(i + 13U) % STATE960_WORDS];

    out[i] = t3 ^ t4;
  }

  for (unsigned int x = 0; x < STATE960_COLS; ++x) {
    for (unsigned int y = 0; y < STATE960_ROWS; ++y) {
      state[x][y] = out[STATE960_INDEX(x, y)];
    }
  }
}

static inline void permutation960_l64(state960_t state) {
  for (unsigned int x = 0; x < STATE960_COLS; ++x) {
    for (unsigned int y = 0; y < STATE960_ROWS; ++y) {
      uint64_t word = state[x][y];
      unsigned int shift = x * STATE960_ROWS + y;
      state[x][y] = rotl64(word, shift) ^ rotl64(word, shift + 21U) ^
                    rotl64(word, shift + 43U);
    }
  }
}

static inline void permutation960_round(state960_t state,
                                        uint64_t round_const) {
  /* One permutation round: SC, MS, AC, ML. */
  permutation960_sb(state);
  permutation960_l15(state);
  state[0][0] ^= round_const;
  permutation960_l64(state);
}

static void words_to_state960(state960_t state,
                              const uint64_t words[STATE960_WORDS]) {
  for (unsigned int x = 0; x < STATE960_COLS; ++x) {
    for (unsigned int y = 0; y < STATE960_ROWS; ++y) {
      state[x][y] = words[STATE960_INDEX(x, y)];
    }
  }
}

static void state960_to_words(uint64_t words[STATE960_WORDS],
                              state960_t state) {
  for (unsigned int x = 0; x < STATE960_COLS; ++x) {
    for (unsigned int y = 0; y < STATE960_ROWS; ++y) {
      words[STATE960_INDEX(x, y)] = state[x][y];
    }
  }
}

static void permutation960(uint64_t state_words[WORDS_perm],
                           unsigned int flag) {
  state960_t state;
  unsigned int base = (flag == 2U) ? ROUNDS : 0U;

  /* f1 and f2 share the same round function but use disjoint round constants. */
  words_to_state960(state, state_words);
  for (unsigned int round = 0; round < ROUNDS; ++round) {
    permutation960_round(state, PI[base + round]);
  }
  state960_to_words(state_words, state);
}



static void f1_permutation(uint64_t state[WORDS_perm]) {
  permutation960(state, 1U);
}

static void f2_permutation(uint64_t state[WORDS_perm]) {
  permutation960(state, 2U);
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

  block[0] = w3;
  block[1] = w0;
  block[2] = w5;
  block[3] = w1;
  block[4] = w4;
  block[5] = w2;
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

  for (unsigned long long blk = 0; blk < full_blocks; ++blk) {
    const uint8_t *p = msg + blk * R_BYTES;

    bytes_to_words(Mi, p, WORDS_r);
    absorb_block(A, B, C, Mi);
  }

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
