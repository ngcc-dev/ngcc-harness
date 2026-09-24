/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.
*/

#include "CryptHash_Cuishen-512.h"
#include <stdint.h>
#include <string.h>
enum {
  ROUNDS = 64,
  ROUND_KEY_WORDS = ROUNDS * 4,
  MESSAGE_WORDS = 16,                            // Message block size: 16 words
  MESSAGE_BLOCK_BYTES = 128,                     // Message block size: 128 bytes
  MESSAGE_BLOCK_BITS = 1024,                     // Message block size: 1024 bits
  LINK_WORDS = 4,                                // Chaining value size: 4 words, 256 bits
  MASTER_KEY_WORDS = MESSAGE_WORDS + LINK_WORDS, // Master key size: 20 words, 1280 bits

  // Rotations used in the block cipher round function
  A = 7,
  B = 31,
  C = 56,
  D = 20,

  // Rotations used by the block cipher key schedule for message part m
  MA = 14,
  MB = 43,
  MC = 28,
  MD = 19,
  ME = 29,
  MF = 63,
  MG = 12,
  MH = 6,

  // Rotations used by the block cipher key schedule for chaining value b
  BA = 27

};

static const uint64_t IV_1[4] = {0x0cc4a61194f81760, 0x5815a7be0543c11c,
                                 0x70b7ed67fc9b5c42, 0xa1513c69681ad6d4};

static const uint64_t IV_2[4] = {0x44f9363580e83d02, 0x720dcdfd9dba5b44,
                                 0xb467369e08efd70e, 0xca320b75e2b634f9};

static const uint64_t PI[ROUNDS] = {
    0x243f6a8885a308d3, 0x13198a2e03707344, 0xa4093822299f31d0,
    0x082efa98ec4e6c89, 0x452821e638d01377, 0xbe5466cf34e90c6c,
    0xc0ac29b7c97c50dd, 0x3f84d5b5b5470917, 0x9216d5d98979fb1b,
    0xd1310ba698dfb5ac, 0x2ffd72dbd01adfb7, 0xb8e1afed6a267e96,
    0xba7c9045f12c7f99, 0x24a19947b3916cf7, 0x0801f2e2858efc16,
    0x636920d871574e69, 0xa458fea3f4933d7e, 0x0d95748f728eb658,
    0x718bcd5882154aee, 0x7b54a41dc25a59b5, 0x9c30d5392af26013,
    0xc5d1b023286085f0, 0xca417918b8db38ef, 0x8e79dcb0603a180e,
    0x6c9e0e8bb01e8a3e, 0xd71577c1bd314b27, 0x78af2fda55605c60,
    0xe65525f3aa55ab94, 0x5748986263e81440, 0x55ca396a2aab10b6,
    0xb4cc5c341141e8ce, 0xa15486af7c72e993, 0xb3ee1411636fbc2a,
    0x2ba9c55d741831f6, 0xce5c3e169b87931e, 0xafd6ba336c24cf5c,
    0x7a32538128958677, 0x3b8f48986b4bb9af, 0xc4bfe81b66282193,
    0x61d809ccfb21a991, 0x487cac605dec8032, 0xef845d5de98575b1,
    0xdc262302eb651b88, 0x23893e81d396acc5, 0x0f6d6ff383f44239,
    0x2e0b4482a4842004, 0x69c8f04a9e1f9b5e, 0x21c66842f6e96c9a,
    0x670c9c61abd388f0, 0x6a51a0d2d8542f68, 0x960fa728ab5133a3,
    0x6eef0b6c137a3be4, 0xba3bf0507efb2a98, 0xa1f1651d39af0176,
    0x66ca593e82430e88, 0x8cee8619456f9fb4, 0x7d84a5c33b8b5ebe,
    0xe06f75d885c12073, 0x401a449f56c16aa6, 0x4ed3aa62363f7706,
    0x1bfedf72429b023d, 0x37d0d724d00a1248, 0xdb0fead349f1c09b,
    0x075372c980991b7b};

static inline uint64_t rotl64(uint64_t x, unsigned int n) {
  return (x << n) | (x >> (64U - n));
}

/**
 * Combines eight 8-bit words into one 64-bit word in big-endian order.
 */
static uint64_t load64_be(const unsigned char in[8]) {
  return ((uint64_t)in[0] << 56) | ((uint64_t)in[1] << 48) |
         ((uint64_t)in[2] << 40) | ((uint64_t)in[3] << 32) |
         ((uint64_t)in[4] << 24) | ((uint64_t)in[5] << 16) |
         ((uint64_t)in[6] << 8) | (uint64_t)in[7];
}

/**
 * Splits one 64-bit word into eight 8-bit words in big-endian order.
 */
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

static void load1024_be(const unsigned char in[MESSAGE_BLOCK_BYTES],
                        uint64_t out[MESSAGE_WORDS]) {
  int i;

  for (i = 0; i < MESSAGE_WORDS; ++i) {
    out[i] = load64_be(in + 8 * i);
  }
}

static void store512_be(unsigned char out[64], const uint64_t in[8]) {
  int i;
  for (i = 0; i < 8; ++i) {
    store64_be(out + 8 * i, in[i]);
  }
}


static void copy_partial_bits(unsigned char *dst, const unsigned char *src,
                              unsigned long long bit_count) {
  const unsigned long long full_bytes = bit_count / 8ULL;
  const unsigned int tail_bits = (unsigned int)(bit_count % 8ULL);

  if (full_bytes > 0ULL) {
    memcpy(dst, src, (size_t)full_bytes);
  }

  if (tail_bits != 0U) {
    dst[full_bytes] =
        (unsigned char)(src[full_bytes] &
                        (unsigned char)(0xFFU << (8U - tail_bits)));
  }
}


static void load1280_be(const unsigned char in[MASTER_KEY_WORDS * 8],
                        uint64_t out[MASTER_KEY_WORDS]) {
  int i;

  for (i = 0; i < MASTER_KEY_WORDS; ++i) {
    out[i] = load64_be(in + 8 * i);
  }
}

static void copy_words_256(uint64_t dst[4], const uint64_t src[4]) {
  memcpy(dst, src, sizeof(uint64_t) * 4);
}

/* Build the 1280-bit master key from the message block and chaining value. */
static void make_round_key(const uint64_t m[MESSAGE_WORDS],
                           const uint64_t b[LINK_WORDS],
                           uint64_t key[MASTER_KEY_WORDS]) {

  memcpy(key, m, sizeof(uint64_t) * MESSAGE_WORDS);
  memcpy(key + MESSAGE_WORDS, b, sizeof(uint64_t) * LINK_WORDS);
}

/* Expand the master key into per-round keys. */
static void key_schedule(const uint64_t master_key[MASTER_KEY_WORDS],
                         uint64_t round_keys[ROUND_KEY_WORDS],
                         const uint64_t counter[2]) {
  uint64_t m[MESSAGE_WORDS];
  uint64_t m_next[MESSAGE_WORDS];
  uint64_t b[LINK_WORDS];
  uint64_t b_next[LINK_WORDS];

  uint64_t cnt[2];
  uint64_t cnt_next[2];
  int round;

  memcpy(m, master_key, sizeof(m));
  memcpy(b, master_key + MESSAGE_WORDS, sizeof(b));
  memcpy(cnt, counter, sizeof(cnt));

  for (round = 0; round < ROUNDS; ++round) {
    /* Mix message words, chaining words, and the block counter. */
    round_keys[4 * round + 0] = m[0] ^ b[0] ^ cnt[0];
    round_keys[4 * round + 1] = m[1] ^ b[1] ^ cnt[1];
    round_keys[4 * round + 2] = m[8] ^ b[2];
    round_keys[4 * round + 3] = m[9] ^ b[3];

    m_next[0] = m[6];
    m_next[1] = m[7];
    m_next[2] = rotl64(m[8], ME) + rotl64(m[10] ^ m[11], MH);
    m_next[3] = rotl64(m[9], MF) + rotl64(m[10] ^ m[11], MG);
    m_next[4] = m[10];
    m_next[5] = m[11];
    m_next[6] = m[12];
    m_next[7] = m[13];
    m_next[8] = m[14];
    m_next[9] = m[15];
    m_next[10] = rotl64(m[0], MA) + rotl64(m[2] ^ m[3], MD);
    m_next[11] = rotl64(m[1], MB) + rotl64(m[2] ^ m[3], MC);
    m_next[12] = m[2];
    m_next[13] = m[3];
    m_next[14] = m[4];
    m_next[15] = m[5];

    b_next[0] = b[1];
    b_next[1] = b[2];
    b_next[2] = b[3];
    b_next[3] = rotl64(b[0], BA) ^ b[1];

    cnt_next[0] = cnt[1];
    cnt_next[1] = rotl64(cnt[0], 2);

    memcpy(m, m_next, sizeof(m));
    memcpy(b, b_next, sizeof(b));
    memcpy(cnt, cnt_next, sizeof(cnt));
  }
}

/* One round of the internal block cipher. */
static void round_function(const uint64_t in[4], uint64_t out[4],
                           const uint64_t round_key[4], int round) {

  const uint64_t x0 = in[0] ^ round_key[0] ^ PI[round];
  const uint64_t x1 = in[1] ^ round_key[1];
  const uint64_t x2 = in[2];
  const uint64_t x3 = in[3];
  const uint64_t t = x2 ^ x3;

  out[0] = x2;
  out[1] = x3;
  out[2] = rotl64(x0, A) + rotl64((t ^ round_key[3]), D);
  out[3] = rotl64(x1, B) + rotl64((t ^ round_key[2]), C);
}

/* Encrypt one 256-bit block with the derived round keys. */
static void octarx_encrypt(const uint64_t in[4],
                           const uint64_t master_key[MASTER_KEY_WORDS],
                           const uint64_t counter[2], uint64_t out[4]) {
  uint64_t state[4];
  uint64_t next[4];
  uint64_t round_keys[ROUND_KEY_WORDS];
  int round;

  key_schedule(master_key, round_keys, counter);
  copy_words_256(state, in);

  for (round = 0; round < ROUNDS; ++round) {
    const uint64_t *round_key = round_keys + round * 4;
    round_function(state, next, round_key, round);
    copy_words_256(state, next);
  }

  copy_words_256(out, state);
}

/* Separate the two compression paths with one bit. */
static void lsb_separate(const uint64_t t[4], uint64_t out[4], int bit) {
  copy_words_256(out, t);

  out[3] ^= (uint64_t)(bit & 1);
}

/* Compress one full message block into the two chaining values. */
static void compress_one_block(const uint64_t t_prev[4],
                               const uint64_t b_prev[4],
                               const uint64_t m[MESSAGE_WORDS],
                               unsigned long long msg_bits_processed,
                               uint64_t t_next[4], uint64_t b_next[4]) {
  uint64_t key[MASTER_KEY_WORDS];
  uint64_t counter[2];
  uint64_t top_input[4];
  uint64_t bottom_input[4];

  make_round_key(m, b_prev, key);
  /* The counter binds each block to its message position. */
  counter[0] = 0;
  counter[1] = (uint64_t)msg_bits_processed;


  /* Run the top and bottom paths independently. */
  lsb_separate(t_prev, top_input, 0);
  lsb_separate(t_prev, bottom_input, 1);
  octarx_encrypt(top_input, key, counter, t_next);
  /* Feed-forward keeps the input tied to the new chaining value. */
  for (int i = 0; i < 4; ++i) {
    t_next[i] ^= top_input[i];
  }
  octarx_encrypt(bottom_input, key, counter, b_next);
  /* Feed-forward for the bottom path. */
  for (int i = 0; i < 4; ++i) {
    b_next[i] ^= bottom_input[i];
  }
}

static void make_const256(uint64_t out[4], uint64_t c) {
  memset(out, 0, sizeof(uint64_t) * 4);
  out[4 - 1] = c;
}

/* Finalize the hash by encrypting two constants and concatenating them. */
static void finalization(const uint64_t t[4], const uint64_t b[4],
                         const unsigned char *tail_msg,
                         unsigned long long tail_msg_bits,
                         unsigned long long counter_bits,
                         uint64_t digest_words[8]) {
  uint64_t counter[2];
  uint64_t key[MASTER_KEY_WORDS];
  unsigned char key_bytes[MASTER_KEY_WORDS * 8];
  uint64_t c2[4];
  uint64_t c3[4];
  uint64_t y2[4];
  uint64_t y3[4];
  int i;

  counter[0] = 0;
  counter[1] = (uint64_t)counter_bits;

  /* Final key layout: t || b || tail || zero padding. */
  memset(key_bytes, 0, sizeof(key_bytes));
  for (i = 0; i < 4; ++i) {
    store64_be(key_bytes + 8 * i, t[i]);
    store64_be(key_bytes + 32 + 8 * i, b[i]);
  }
  if (tail_msg_bits != 0ULL) {
    copy_partial_bits(key_bytes + 64, tail_msg, tail_msg_bits);
  }
  load1280_be(key_bytes, key);

  make_const256(c2, 2);
  make_const256(c3, 3);
  octarx_encrypt(c2, key, counter, y2);
  octarx_encrypt(c3, key, counter, y3);
  memcpy(digest_words, y2, sizeof(uint64_t) * 4);
  memcpy(digest_words + 4, y3, sizeof(uint64_t) * 4);
}

/* Copy a short tail into a zero-padded 1024-bit block. */
static void prepare_padded_block(const unsigned char *msg_tail,
                                 unsigned long long tail_bits,
                                 unsigned char block[MESSAGE_BLOCK_BYTES]) {
  memset(block, 0, MESSAGE_BLOCK_BYTES);

  if (tail_bits != 0ULL) {
    copy_partial_bits(block, msg_tail, tail_bits);
  }
}

int CryptHash(int digest_len_bits, const unsigned char *msg,
              unsigned long long msg_len_bits, unsigned char *digest) {
  uint64_t t[4];
  uint64_t b[4];
  uint64_t m[MESSAGE_WORDS];
  uint64_t t_next[4];
  uint64_t b_next[4];
  uint64_t digest_words[8];
  unsigned char padded_block[MESSAGE_BLOCK_BYTES];
  unsigned long long full_blocks;
  unsigned long long rem_bits;
  unsigned long long block;
  unsigned long long final_counter_bits;
  const unsigned char *tail_msg;
  unsigned long long tail_msg_bits;

  if (digest_len_bits != DIGEST_BIT_LENGTH || digest == NULL) {
    return -1;
  }

  if (msg_len_bits != 0ULL && msg == NULL) {
    return -1;
  }

  copy_words_256(t, IV_1);
  copy_words_256(b, IV_2);

  /* First absorb all complete 1024-bit blocks. */
  full_blocks = msg_len_bits / MESSAGE_BLOCK_BITS;
  rem_bits = msg_len_bits % MESSAGE_BLOCK_BITS;
  final_counter_bits = msg_len_bits;
  tail_msg = (msg == NULL) ? NULL : (msg + full_blocks * MESSAGE_BLOCK_BYTES);
  tail_msg_bits = rem_bits;
  for (block = 0ULL; block < full_blocks; ++block) {
    load1024_be(msg + block * MESSAGE_BLOCK_BYTES, m);
    compress_one_block(t, b, m, (block + 1ULL) * MESSAGE_BLOCK_BITS, t_next,
                       b_next);
    copy_words_256(t, t_next);
    copy_words_256(b, b_next);
  }


  /* Long tails are compressed once before finalization. */
  if (rem_bits > 768ULL) {
    prepare_padded_block(tail_msg, rem_bits, padded_block);
    load1024_be(padded_block, m);
    compress_one_block(t, b, m, msg_len_bits, t_next, b_next);
    copy_words_256(t, t_next);
    copy_words_256(b, b_next);
    tail_msg_bits = 0ULL;
  }

  /* Convert the final chaining state into the 512-bit digest. */
  finalization(t, b, tail_msg, tail_msg_bits, final_counter_bits, digest_words);
  store512_be(digest, digest_words);

  return 0;
}
