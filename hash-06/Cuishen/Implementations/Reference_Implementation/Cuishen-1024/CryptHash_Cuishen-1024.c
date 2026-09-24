/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.
*/

#include "CryptHash_Cuishen-1024.h"
#include <stdint.h>
#include <string.h>
enum {
  ROUNDS = 64,
  ROUND_KEY_WORDS = ROUNDS * 8,
  MESSAGE_WORDS = 16,                            // Message block size: 16 words
  MESSAGE_BLOCK_BYTES = 128,                     // Message block size: 128 bytes
  MESSAGE_BLOCK_BITS = 1024,                     // Message block size: 1024 bits
  LINK_WORDS = 8,                                // Chaining value size: 8 words, 512 bits
  MASTER_KEY_WORDS = MESSAGE_WORDS + LINK_WORDS, // Master key size: 24 words, 1536 bits

  // Rotations used in the block cipher round function
  A = 18,
  B = 33,
  C = 47,
  D = 60,
  E = 49,
  F = 20,
  G = 16,
  H = 21,

  // Rotations used by the block cipher key schedule for message part m
  MA = 14,
  MB = 43,
  MC = 28,
  MD = 19,
  ME = 29,
  MF = 63,
  MG = 12,
  MH = 6,

  // Rotation used by the block cipher key schedule for chaining value b
  BA = 47

};

static const uint64_t IV_1[8] = {0xc3578c15393dbe7b, 0x1e039f40ee65e7f5,
                                 0x857b7bee690d3012, 0xa29bf2defe493534,
                                 0xcdf34e803fd487d1, 0x5b89092b8fbef3e8,
                                 0xa0c06a13c70b322b, 0xc9cda6892035228a};

static const uint64_t IV_2[8] = {0xf281f2397b1d4610, 0x77c9c2114e14fd92,
                                 0xb91bf663f039c764, 0x066560954a8e8129,
                                 0x39479381ecbce703, 0x7830769755fe0b0a,
                                 0xc2b2b7559233f645, 0x0c2d3b4be1707aba};

static const uint64_t E_CONST[64] = {
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
    0xd04c324ef10de513, 0xd3f5114b8b5d374d, 0x93cb8879c7d52ffd,
    0x72ba0aae7277da7b, 0xa1b4af1488d8e836, 0xaf14865e6c37ab68,
    0x76fe690b57112138, 0x2af341afe94f77bc, 0xf06c83b8ff5675f0,
    0x979074ad9a787bc5, 0xb9bd4b0c5937d3ed, 0xe4c3a79396215eda,
    0xb1f57d0b5a7db461, 0xdd8f3c75540d0012, 0x1fd56e95f8c731e9,
    0xc4d7221bbed0c62b, 0xb5a87804b679a0ca, 0xa41d802a4604c311,
    0xb71de3e5c6b400e0, 0x24a6668ccf2e2de8, 0x6876e4f5c50000f0,
    0xa93b3aa7e6342b30, 0x2a0a47373b25f73e, 0x3b26d569fe2291ad,
    0x36d6a147d1060b87, 0x1a2801f978376408, 0x2ff592d9140db1e9,
    0x399df4b0e14ca8e8,

};

static inline uint64_t rotl64(uint64_t x, unsigned int n) {
  return (x << n) | (x >> (64U - n));
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

static void load1024_be(const unsigned char in[MESSAGE_BLOCK_BYTES],
                        uint64_t out[MESSAGE_WORDS]) {
  int i;

  for (i = 0; i < MESSAGE_WORDS; ++i) {
    out[i] = load64_be(in + 8 * i);
  }
}

static void store1024_be(unsigned char out[128], const uint64_t in[16]) {
  int i;

  for (i = 0; i < 16; ++i) {
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

static void load1536_be(const unsigned char in[MASTER_KEY_WORDS * 8],
                        uint64_t out[MASTER_KEY_WORDS]) {
  int i;

  for (i = 0; i < MASTER_KEY_WORDS; ++i) {
    out[i] = load64_be(in + 8 * i);
  }
}

static void copy_words_512(uint64_t dst[8], const uint64_t src[8]) {
  memcpy(dst, src, sizeof(uint64_t) * 8);
}

/* Build the 1536-bit master key from the message block and chaining value. */
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
    round_keys[8 * round + 0] = m[0] ^ b[0] ^ cnt[0];
    round_keys[8 * round + 1] = m[1] ^ b[1] ^ cnt[1];
    round_keys[8 * round + 2] = m[8] ^ b[2];
    round_keys[8 * round + 3] = m[9] ^ b[3];
    round_keys[8 * round + 4] = b[4];
    round_keys[8 * round + 5] = b[5];
    round_keys[8 * round + 6] = b[6];
    round_keys[8 * round + 7] = b[7];

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
    b_next[3] = b[4];
    b_next[4] = b[5];
    b_next[5] = b[6];
    b_next[6] = b[7];
    b_next[7] = rotl64(b[0], BA) ^ b[1];

    cnt_next[0] = cnt[1];
    cnt_next[1] = rotl64(cnt[0], 2);

    memcpy(m, m_next, sizeof(m));
    memcpy(b, b_next, sizeof(b));
    memcpy(cnt, cnt_next, sizeof(cnt));
  }
}

/* One round of the internal block cipher. */
static void round_function(const uint64_t in[8], uint64_t out[8],
                           const uint64_t round_key[8], int round) {
  const uint64_t x0 = in[0] ^ round_key[0] ^ E_CONST[round];
  const uint64_t x1 = in[1] ^ round_key[1];
  const uint64_t x2 = in[2] ^ round_key[2];
  const uint64_t x3 = in[3] ^ round_key[3];
  const uint64_t x4 = in[4];
  const uint64_t x5 = in[5];
  const uint64_t x6 = in[6];
  const uint64_t x7 = in[7];

  const uint64_t t = x4 ^ x5 ^ x6 ^ x7;

  out[0] = x4;
  out[1] = x5;
  out[2] = x6;
  out[3] = x7;
  out[4] = rotl64(x0, A) + rotl64((t ^ round_key[7]), H);
  out[5] = rotl64(x1, B) + rotl64((t ^ round_key[6]), G);
  out[6] = rotl64(x2, C) + rotl64((t ^ round_key[5]), F);
  out[7] = rotl64(x3, D) + rotl64((t ^ round_key[4]), E);
}

/* Encrypt one 512-bit block with the derived round keys. */
static void octarx_encrypt(const uint64_t in[8],
                           const uint64_t master_key[MASTER_KEY_WORDS],
                           const uint64_t counter[2], uint64_t out[8]) {
  uint64_t state[8];
  uint64_t next[8];
  uint64_t round_keys[ROUND_KEY_WORDS];
  int round;

  key_schedule(master_key, round_keys, counter);
  copy_words_512(state, in);

  for (round = 0; round < ROUNDS; ++round) {
    const uint64_t *round_key = round_keys + round * 8;
    round_function(state, next, round_key, round);
    copy_words_512(state, next);
  }

  copy_words_512(out, state);
}

/* Separate the two compression paths with one bit. */
static void lsb_separate(const uint64_t t[8], uint64_t out[8], int bit) {
  copy_words_512(out, t);

  out[7] ^= (uint64_t)(bit & 1);
}

/* Compress one full message block into the two chaining values. */
static void compress_one_block(const uint64_t t_prev[8],
                               const uint64_t b_prev[8],
                               const uint64_t m[MESSAGE_WORDS],
                               unsigned long long msg_bits_processed,
                               uint64_t t_next[8], uint64_t b_next[8]) {
  uint64_t key[MASTER_KEY_WORDS];
  uint64_t counter[2];
  uint64_t top_input[8];
  uint64_t bottom_input[8];

  make_round_key(m, b_prev, key);
  /* The counter binds each block to its message position. */
  counter[0] = 0;
  counter[1] = (uint64_t)msg_bits_processed;
  /* Run the top and bottom paths independently. */
  lsb_separate(t_prev, top_input, 0);
  lsb_separate(t_prev, bottom_input, 1);
  octarx_encrypt(top_input, key, counter, t_next);
  /* Feed-forward keeps the input tied to the new chaining value. */
  for (int i = 0; i < 8; ++i) {
    t_next[i] ^= top_input[i];
  }
  octarx_encrypt(bottom_input, key, counter, b_next);
  /* Feed-forward for the bottom path. */
  for (int i = 0; i < 8; ++i) {
    b_next[i] ^= bottom_input[i];
  }
}

static void make_const512(uint64_t out[8], uint64_t c) {
  memset(out, 0, sizeof(uint64_t) * 8);
  out[8 - 1] = c;
}

/* Finalize the hash by encrypting two constants and concatenating them. */
static void finalization(const uint64_t t[8], const uint64_t b[8],
                         const unsigned char *tail_msg,
                         unsigned long long tail_msg_bits,
                         unsigned long long counter_bits,
                         uint64_t digest_words[16]) {
  uint64_t counter[2];
  uint64_t key[MASTER_KEY_WORDS];
  unsigned char key_bytes[MASTER_KEY_WORDS * 8];
  uint64_t c2[8];
  uint64_t c3[8];
  uint64_t y2[8];
  uint64_t y3[8];
  int i;

  counter[0] = 0;
  counter[1] = (uint64_t)counter_bits;

  /* Final key layout: t || b || tail || zero padding. */
  memset(key_bytes, 0, sizeof(key_bytes));
  for (i = 0; i < 8; ++i) {
    store64_be(key_bytes + 8 * i, t[i]);
    store64_be(key_bytes + 64 + 8 * i, b[i]);
  }
  if (tail_msg_bits != 0ULL) {
    copy_partial_bits(key_bytes + 128, tail_msg, tail_msg_bits);
  }
  load1536_be(key_bytes, key);
  make_const512(c2, 2);
  make_const512(c3, 3);
  octarx_encrypt(c2, key, counter, y2);
  octarx_encrypt(c3, key, counter, y3);
  memcpy(digest_words, y2, sizeof(uint64_t) * 8);
  memcpy(digest_words + 8, y3, sizeof(uint64_t) * 8);
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
  uint64_t t[8];
  uint64_t b[8];
  uint64_t m[MESSAGE_WORDS];
  uint64_t t_next[8];
  uint64_t b_next[8];
  uint64_t digest_words[16];
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

  copy_words_512(t, IV_1);
  copy_words_512(b, IV_2);
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
    copy_words_512(t, t_next);
    copy_words_512(b, b_next);
  }

  /* Long tails are compressed once before finalization. */
  if (rem_bits > 512ULL) {
    prepare_padded_block(tail_msg, rem_bits, padded_block);
    load1024_be(padded_block, m);
    compress_one_block(t, b, m, msg_len_bits, t_next, b_next);
    copy_words_512(t, t_next);
    copy_words_512(b, b_next);
    tail_msg_bits = 0ULL;
  }

  /* Convert the final chaining state into the 1024-bit digest. */
  finalization(t, b, tail_msg, tail_msg_bits, final_counter_bits, digest_words);
  store1024_be(digest, digest_words);

  return 0;
}
