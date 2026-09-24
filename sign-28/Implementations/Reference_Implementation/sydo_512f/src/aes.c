/*
 *  SPDX-License-Identifier: MIT
 */

#if defined(HAVE_CONFIG_H)
#include <config.h>
#endif

#if defined(_WIN32) && !defined(HAVE_OPENSSL)
#if !defined(_WIN32_WINNT)
#define _WIN32_WINNT 0x0601
#elif _WIN32_WINNT < 0x0601
#error "At least Windows 7 is required."
#endif
#endif

#include "aes.h"

#include "fields.h"
#include "compat.h"
#include "utils.h"

#if defined(HAVE_OPENSSL)
#include <openssl/evp.h>
#elif defined(_WIN32)
#include <windows.h>
#endif
#include <string.h>

#define KEY_WORDS_128 4
#define KEY_WORDS_192 6
#define KEY_WORDS_256 8

#define AES_BLOCK_WORDS 4
#define RIJNDAEL_BLOCK_WORDS_192 6
#define RIJNDAEL_BLOCK_WORDS_256 8

static const bf8_t round_constants[30] = {
    0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36, 0x6c, 0xd8, 0xab, 0x4d, 0x9a,
    0x2f, 0x5e, 0xbc, 0x63, 0xc6, 0x97, 0x35, 0x6a, 0xd4, 0xb3, 0x7d, 0xfa, 0xef, 0xc5, 0x91,
};

static const uint8_t aes_sbox[256] = {
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab,
    0x76, 0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4,
    0x72, 0xc0, 0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71,
    0xd8, 0x31, 0x15, 0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2,
    0xeb, 0x27, 0xb2, 0x75, 0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6,
    0xb3, 0x29, 0xe3, 0x2f, 0x84, 0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb,
    0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf, 0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45,
    0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8, 0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5,
    0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2, 0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44,
    0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73, 0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a,
    0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb, 0xe0, 0x32, 0x3a, 0x0a, 0x49,
    0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79, 0xe7, 0xc8, 0x37, 0x6d,
    0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08, 0xba, 0x78, 0x25,
    0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a, 0x70, 0x3e,
    0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e, 0xe1,
    0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb,
    0x16};

static bf8_t compute_sbox(bf8_t in) {
  return aes_sbox[in];
}

#if defined(SYDO_TESTS)
void aes_increment_iv(uint8_t* iv)
#else
static inline void aes_increment_iv(uint8_t* iv)
#endif
{
  uint32_t iv0;
  memcpy(&iv0, iv, sizeof(uint32_t));
  iv0 = htole32(le32toh(iv0) + 1);
  memcpy(iv, &iv0, sizeof(uint32_t));
}

// ## AES ##
// Round Functions
static inline bf8_t aes_xtime(bf8_t x) {
  const uint8_t mask = (uint8_t)-(uint8_t)(x >> 7);
  return (bf8_t)((uint8_t)(x << 1) ^ (mask & UINT8_C(0x1b)));
}

static uint32_t aes_te[4][256];
static unsigned int aes_te_initialized = 0;

static uint32_t pack_column(bf8_t b0, bf8_t b1, bf8_t b2, bf8_t b3) {
  return (uint32_t)b0 | ((uint32_t)b1 << 8) | ((uint32_t)b2 << 16) |
         ((uint32_t)b3 << 24);
}

static uint32_t pack_word(const aes_word_t word) {
  return pack_column(word[0], word[1], word[2], word[3]);
}

static void unpack_column(aes_word_t dst, uint32_t v) {
  dst[0] = (bf8_t)v;
  dst[1] = (bf8_t)(v >> 8);
  dst[2] = (bf8_t)(v >> 16);
  dst[3] = (bf8_t)(v >> 24);
}

static uint32_t load_word_le(const uint8_t* src) {
  return (uint32_t)src[0] | ((uint32_t)src[1] << 8) | ((uint32_t)src[2] << 16) |
         ((uint32_t)src[3] << 24);
}

static void store_word_le(uint8_t* dst, uint32_t v) {
  dst[0] = (uint8_t)v;
  dst[1] = (uint8_t)(v >> 8);
  dst[2] = (uint8_t)(v >> 16);
  dst[3] = (uint8_t)(v >> 24);
}

static uint32_t sub_word_u32(uint32_t w) {
  return (uint32_t)aes_sbox[(uint8_t)w] |
         ((uint32_t)aes_sbox[(uint8_t)(w >> 8)] << 8) |
         ((uint32_t)aes_sbox[(uint8_t)(w >> 16)] << 16) |
         ((uint32_t)aes_sbox[(uint8_t)(w >> 24)] << 24);
}

static uint32_t rot_word_u32(uint32_t w) {
  return (w >> 8) | (w << 24);
}

static void aes_te_init(void) {
  if (aes_te_initialized) {
    return;
  }
  for (unsigned int x = 0; x != 256u; ++x) {
    const bf8_t s = compute_sbox((bf8_t)x);
    const bf8_t x2 = aes_xtime(s);
    const bf8_t x3 = (bf8_t)(x2 ^ s);
    aes_te[0][x] = pack_column(x2, s, s, x3);
    aes_te[1][x] = pack_column(x3, x2, s, s);
    aes_te[2][x] = pack_column(s, x3, x2, s);
    aes_te[3][x] = pack_column(s, s, x3, x2);
  }
  aes_te_initialized = 1;
}

static void expand_key_words(uint32_t round_key_words[AES_MAX_ROUNDS + 1][8],
                             const uint8_t* key, unsigned int key_words,
                             unsigned int block_words, unsigned int num_rounds) {
  uint32_t words[(AES_MAX_ROUNDS + 1) * 8];
  const unsigned int total_words = block_words * (num_rounds + 1u);

  for (unsigned int k = 0; k < key_words; k++) {
    words[k] = load_word_le(&key[4u * k]);
  }

  for (unsigned int k = key_words; k < total_words; ++k) {
    uint32_t tmp = words[k - 1u];
    if (k % key_words == 0) {
      tmp = sub_word_u32(rot_word_u32(tmp)) ^ (uint32_t)round_constants[(k / key_words) - 1u];
    }

    if (key_words > 6 && (k % key_words) == 4) {
      tmp = sub_word_u32(tmp);
    }

    words[k] = words[k - key_words] ^ tmp;
  }

  for (unsigned int k = 0; k < total_words; ++k) {
    round_key_words[k / block_words][k % block_words] = words[k];
  }
}

void expand_key(aes_round_keys_t* round_keys, const uint8_t* key, unsigned int key_words,
                unsigned int block_words, unsigned int num_rounds) {
  const unsigned int total_words = block_words * (num_rounds + 1u);

  expand_key_words(round_keys->round_key_words, key, key_words, block_words, num_rounds);

  for (unsigned int k = 0; k < total_words; ++k) {
    const unsigned int round = k / block_words;
    const unsigned int c = k % block_words;
    unpack_column(round_keys->round_keys[round][c], round_keys->round_key_words[round][c]);
  }
}

// Calling Functions

void aes128_init_round_keys(aes_round_keys_t* round_key, const uint8_t* key) {
  expand_key(round_key, key, KEY_WORDS_128, AES_BLOCK_WORDS, AES_ROUNDS_128);
}

void aes192_init_round_keys(aes_round_keys_t* round_key, const uint8_t* key) {
  expand_key(round_key, key, KEY_WORDS_192, AES_BLOCK_WORDS, AES_ROUNDS_192);
}

void aes256_init_round_keys(aes_round_keys_t* round_key, const uint8_t* key) {
  expand_key(round_key, key, KEY_WORDS_256, AES_BLOCK_WORDS, AES_ROUNDS_256);
}

void rijndael192_init_round_keys(aes_round_keys_t* round_key, const uint8_t* key) {
  expand_key(round_key, key, KEY_WORDS_192, RIJNDAEL_BLOCK_WORDS_192, AES_ROUNDS_192);
}

void rijndael256_init_round_keys(aes_round_keys_t* round_key, const uint8_t* key) {
  expand_key(round_key, key, KEY_WORDS_256, RIJNDAEL_BLOCK_WORDS_256, AES_ROUNDS_256);
}

static void load_state(aes_block_t state, const uint8_t* src, unsigned int block_words) {
  for (unsigned int i = 0; i != block_words * 4; ++i) {
    state[i / 4][i % 4] = bf8_load(&src[i]);
  }
}

static uint8_t* store_state(uint8_t* dst, aes_block_t state, unsigned int block_words) {
  for (unsigned int i = 0; i != block_words * 4; ++i, ++dst) {
    bf8_store(dst, state[i / 4][i % 4]);
  }
  return dst;
}

static void aes_encrypt_words(const uint32_t round_key_words[AES_MAX_ROUNDS + 1][8],
                              uint32_t state_words[8], unsigned int block_words,
                              unsigned int num_rounds) {
  const unsigned int row2 = block_words == RIJNDAEL_BLOCK_WORDS_256 ? 3u : 2u;
  const unsigned int row3 = block_words == RIJNDAEL_BLOCK_WORDS_256 ? 4u : 3u;

  aes_te_init();

  for (unsigned int c = 0; c != block_words; ++c) {
    state_words[c] ^= round_key_words[0][c];
  }

  for (unsigned int round = 1; round < num_rounds; ++round) {
    uint32_t next[8];
    for (unsigned int c = 0; c != block_words; ++c) {
      const uint32_t w0 = state_words[c];
      const uint32_t w1 = state_words[(c + 1u) % block_words];
      const uint32_t w2 = state_words[(c + row2) % block_words];
      const uint32_t w3 = state_words[(c + row3) % block_words];
      next[c] = aes_te[0][(uint8_t)w0] ^ aes_te[1][(uint8_t)(w1 >> 8)] ^
                aes_te[2][(uint8_t)(w2 >> 16)] ^ aes_te[3][(uint8_t)(w3 >> 24)] ^
                round_key_words[round][c];
    }
    memcpy(state_words, next, sizeof(uint32_t) * block_words);
  }

  {
    uint32_t next[8];
    for (unsigned int c = 0; c != block_words; ++c) {
      const uint32_t w0 = state_words[c];
      const uint32_t w1 = state_words[(c + 1u) % block_words];
      const uint32_t w2 = state_words[(c + row2) % block_words];
      const uint32_t w3 = state_words[(c + row3) % block_words];
      next[c] = ((uint32_t)aes_sbox[(uint8_t)w0]) |
                ((uint32_t)aes_sbox[(uint8_t)(w1 >> 8)] << 8) |
                ((uint32_t)aes_sbox[(uint8_t)(w2 >> 16)] << 16) |
                ((uint32_t)aes_sbox[(uint8_t)(w3 >> 24)] << 24);
      next[c] ^= round_key_words[num_rounds][c];
    }
    memcpy(state_words, next, sizeof(uint32_t) * block_words);
  }
}

static void aes_encrypt(const aes_round_keys_t* keys, aes_block_t state, unsigned int block_words,
                        unsigned int num_rounds) {
  uint32_t state_words[8];

  for (unsigned int c = 0; c != block_words; ++c) {
    state_words[c] = pack_word(state[c]);
  }

  aes_encrypt_words(keys->round_key_words, state_words, block_words, num_rounds);

  for (unsigned int c = 0; c != block_words; ++c) {
    unpack_column(state[c], state_words[c]);
  }
}

void aes128_encrypt_block(const aes_round_keys_t* key, const uint8_t* plaintext,
                          uint8_t* ciphertext) {
  aes_block_t state;
  load_state(state, plaintext, AES_BLOCK_WORDS);
  aes_encrypt(key, state, AES_BLOCK_WORDS, AES_ROUNDS_128);
  store_state(ciphertext, state, AES_BLOCK_WORDS);
}

void aes192_encrypt_block(const aes_round_keys_t* key, const uint8_t* plaintext,
                          uint8_t* ciphertext) {
  aes_block_t state;
  load_state(state, plaintext, AES_BLOCK_WORDS);
  aes_encrypt(key, state, AES_BLOCK_WORDS, AES_ROUNDS_192);
  store_state(ciphertext, state, AES_BLOCK_WORDS);
}

void aes256_encrypt_block(const aes_round_keys_t* key, const uint8_t* plaintext,
                          uint8_t* ciphertext) {
  aes_block_t state;
  load_state(state, plaintext, AES_BLOCK_WORDS);
  aes_encrypt(key, state, AES_BLOCK_WORDS, AES_ROUNDS_256);
  store_state(ciphertext, state, AES_BLOCK_WORDS);
}

void rijndael192_encrypt_block(const aes_round_keys_t* key, const uint8_t* plaintext,
                               uint8_t* ciphertext) {
  aes_block_t state;
  load_state(state, plaintext, RIJNDAEL_BLOCK_WORDS_192);
  aes_encrypt(key, state, RIJNDAEL_BLOCK_WORDS_192, AES_ROUNDS_192);
  store_state(ciphertext, state, RIJNDAEL_BLOCK_WORDS_192);
}

void rijndael256_encrypt_block(const aes_round_keys_t* key, const uint8_t* plaintext,
                               uint8_t* ciphertext) {
  aes_block_t state;
  load_state(state, plaintext, RIJNDAEL_BLOCK_WORDS_256);
  aes_encrypt(key, state, RIJNDAEL_BLOCK_WORDS_256, AES_ROUNDS_256);
  store_state(ciphertext, state, RIJNDAEL_BLOCK_WORDS_256);
}

static void load_block_words(uint32_t state_words[8], const uint8_t* src,
                             unsigned int block_words) {
  for (unsigned int c = 0; c != block_words; ++c) {
    state_words[c] = load_word_le(src + 4u * c);
  }
}

static void store_block_words(uint8_t* dst, const uint32_t state_words[8],
                              unsigned int block_words) {
  for (unsigned int c = 0; c != block_words; ++c) {
    store_word_le(dst + 4u * c, state_words[c]);
  }
}

void rijndael256_encrypt_2_blocks_from_key(const uint8_t* key, const uint8_t* plaintext0,
                                           const uint8_t* plaintext1, uint8_t* ciphertext0,
                                           uint8_t* ciphertext1) {
  uint32_t round_key_words[AES_MAX_ROUNDS + 1][8];
  uint32_t state0[8];
  uint32_t state1[8];

  expand_key_words(round_key_words, key, KEY_WORDS_256, RIJNDAEL_BLOCK_WORDS_256,
                   AES_ROUNDS_256);
  load_block_words(state0, plaintext0, RIJNDAEL_BLOCK_WORDS_256);
  load_block_words(state1, plaintext1, RIJNDAEL_BLOCK_WORDS_256);
  aes_encrypt_words((const uint32_t(*)[8])round_key_words, state0, RIJNDAEL_BLOCK_WORDS_256,
                    AES_ROUNDS_256);
  aes_encrypt_words((const uint32_t(*)[8])round_key_words, state1, RIJNDAEL_BLOCK_WORDS_256,
                    AES_ROUNDS_256);
  store_block_words(ciphertext0, state0, RIJNDAEL_BLOCK_WORDS_256);
  store_block_words(ciphertext1, state1, RIJNDAEL_BLOCK_WORDS_256);
}

void rijndael192_encrypt_2_blocks_from_key(const uint8_t* key, const uint8_t* plaintext0,
                                           const uint8_t* plaintext1, uint8_t* ciphertext0,
                                           uint8_t* ciphertext1) {
  uint32_t round_key_words[AES_MAX_ROUNDS + 1][8];
  uint32_t state0[8];
  uint32_t state1[8];

  expand_key_words(round_key_words, key, KEY_WORDS_192, RIJNDAEL_BLOCK_WORDS_192,
                   AES_ROUNDS_192);
  load_block_words(state0, plaintext0, RIJNDAEL_BLOCK_WORDS_192);
  load_block_words(state1, plaintext1, RIJNDAEL_BLOCK_WORDS_192);
  aes_encrypt_words((const uint32_t(*)[8])round_key_words, state0, RIJNDAEL_BLOCK_WORDS_192,
                    AES_ROUNDS_192);
  aes_encrypt_words((const uint32_t(*)[8])round_key_words, state1, RIJNDAEL_BLOCK_WORDS_192,
                    AES_ROUNDS_192);
  store_block_words(ciphertext0, state0, RIJNDAEL_BLOCK_WORDS_192);
  store_block_words(ciphertext1, state1, RIJNDAEL_BLOCK_WORDS_192);
}

static void add_to_upper_word(uint8_t* iv, uint32_t tweak) {
  uint32_t iv3;
  memcpy(&iv3, iv + IV_SIZE - sizeof(uint32_t), sizeof(uint32_t));
  iv3 = htole32(le32toh(iv3) + tweak);
  memcpy(iv + IV_SIZE - sizeof(uint32_t), &iv3, sizeof(uint32_t));
}

static void generic_prg(const uint8_t* key, uint8_t* internal_iv, uint8_t* out, unsigned int seclvl,
                        size_t outlen) {
  generic_aes_ecb_t ctx;
  int ret = generic_aes_ecb_new(&ctx, key, seclvl);
  assert(ret == 0);
  (void)ret;

  for (size_t idx = 0; idx < outlen / IV_SIZE; idx += 1, out += IV_SIZE) {
    ret = generic_aes_ecb_encrypt(&ctx, out, internal_iv, 1);
    assert(ret == 0);
    (void)ret;
    aes_increment_iv(internal_iv);
  }

  if (outlen % IV_SIZE) {
    uint8_t last_block[IV_SIZE];
    ret = generic_aes_ecb_encrypt(&ctx, last_block, internal_iv, 1);
    assert(ret == 0);
    (void)ret;
    memcpy(out, last_block, outlen % IV_SIZE);
  }

  generic_aes_ecb_free(&ctx);
}

void prg(const uint8_t* key, const uint8_t* iv, uint32_t tweak, uint8_t* out, unsigned int seclvl,
         size_t outlen) {
  uint8_t internal_iv[IV_SIZE];
  memcpy(internal_iv, iv, IV_SIZE);
  add_to_upper_word(internal_iv, tweak);


  generic_prg(key, internal_iv, out, seclvl, outlen);
}

void prg_2_lambda(const uint8_t* key, const uint8_t* iv, uint32_t tweak, uint8_t* out,
                  unsigned int seclvl) {
  uint8_t internal_iv[IV_SIZE];
  memcpy(internal_iv, iv, IV_SIZE);
  add_to_upper_word(internal_iv, tweak);


  generic_prg(key, internal_iv, out, seclvl, seclvl * 2 / 8);
}

void prg_4_lambda(const uint8_t* key, const uint8_t* iv, uint32_t tweak, uint8_t* out,
                  unsigned int seclvl) {
  uint8_t internal_iv[IV_SIZE];
  memcpy(internal_iv, iv, IV_SIZE);
  add_to_upper_word(internal_iv, tweak);


  generic_prg(key, internal_iv, out, seclvl, seclvl * 4 / 8);
}

#if defined(HAVE_OPENSSL)
int generic_aes_ecb_new(generic_aes_ecb_t* context, const uint8_t* key, unsigned int seclvl) {
  const EVP_CIPHER* cipher = NULL;
  switch (seclvl) {
  case 256:
    cipher = EVP_aes_256_ecb();
    break;
  case 192:
    cipher = EVP_aes_192_ecb();
    break;
  default:
    cipher = EVP_aes_128_ecb();
    break;
  }

  EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
  if (!ctx) {
    return -1;
  }

  EVP_EncryptInit_ex(ctx, cipher, NULL, key, NULL);
  context->ctx = ctx;
  return 0;
}

int generic_aes_ecb_encrypt(generic_aes_ecb_t* ctx, uint8_t* ciphertext, const uint8_t* plaintext,
                            size_t blocks) {
  int len = 0;
  int ret = EVP_EncryptUpdate(ctx->ctx, ciphertext, &len, plaintext, blocks * IV_SIZE);
  if (ret != 1 || len != (int)blocks * IV_SIZE) {
    return -1;
  }
  return 0;
}

void generic_aes_ecb_free(generic_aes_ecb_t* ctx) {
  EVP_CIPHER_CTX_free(ctx->ctx);
}
#elif defined(_WIN32)
int generic_aes_ecb_new(generic_aes_ecb_t* ctx, const uint8_t* key, unsigned int seclvl) {
  BCRYPT_ALG_HANDLE aes_handle = NULL;
  BCRYPT_KEY_HANDLE key_handle = NULL;

  NTSTATUS ret = BCryptOpenAlgorithmProvider(&aes_handle, BCRYPT_AES_ALGORITHM, NULL, 0);
  if (!BCRYPT_SUCCESS(ret)) {
    return -1;
  }
  ret = BCryptSetProperty(aes_handle, BCRYPT_CHAINING_MODE, (PUCHAR)BCRYPT_CHAIN_MODE_ECB,
                          (ULONG)((wcslen(BCRYPT_CHAIN_MODE_ECB) + 1) * sizeof(wchar_t)), 0);
  if (!BCRYPT_SUCCESS(ret)) {
    BCryptCloseAlgorithmProvider(aes_handle, 0);
    return -1;
  }

  ret = BCryptGenerateSymmetricKey(aes_handle, &key_handle, NULL, 0, (PUCHAR)key, seclvl / 8, 0);
  if (!BCRYPT_SUCCESS(ret)) {
    BCryptCloseAlgorithmProvider(aes_handle, 0);
    return -1;
  }

  ctx->aes_handle = aes_handle;
  ctx->key_handle = key_handle;
  return 0;
}

int generic_aes_ecb_encrypt(generic_aes_ecb_t* ctx, uint8_t* ciphertext, const uint8_t* plaintext,
                            size_t blocks) {
  ULONG len          = 0;
  const ULONG length = blocks * IV_SIZE;

  NTSTATUS ret =
      BCryptEncrypt(ctx->key_handle, plaintext, length, NULL, NULL, 0, ciphertext, length, &len, 0);
  if (!BCRYPT_SUCCESS(ret) || len != length) {
    return -1;
  }

  return 0;
}

void generic_aes_ecb_free(generic_aes_ecb_t* ctx) {
  BCryptDestroyKey(ctx->key_handle);
  BCryptCloseAlgorithmProvider(ctx->aes_handle, 0);
}
#else
int generic_aes_ecb_new(generic_aes_ecb_t* ctx, const uint8_t* key, unsigned int seclvl) {
  switch (seclvl) {
  case 256:
    aes256_init_round_keys(&ctx->round_keys, key);
    break;
  case 192:
    aes192_init_round_keys(&ctx->round_keys, key);
    break;
  default:
    aes128_init_round_keys(&ctx->round_keys, key);
    break;
  }

  ctx->seclvl = seclvl;
  return 0;
}

int generic_aes_ecb_encrypt(generic_aes_ecb_t* ctx, uint8_t* ciphertext, const uint8_t* plaintext,
                            size_t blocks) {
  switch (ctx->seclvl) {
  case 256: {
    for (; blocks; --blocks, plaintext += IV_SIZE, ciphertext += IV_SIZE) {
      aes_block_t state;
      load_state(state, plaintext, AES_BLOCK_WORDS);
      aes_encrypt(&ctx->round_keys, state, AES_BLOCK_WORDS, AES_ROUNDS_256);
      store_state(ciphertext, state, AES_BLOCK_WORDS);
    }
  }
  case 192: {
    for (; blocks; --blocks, plaintext += IV_SIZE, ciphertext += IV_SIZE) {
      aes_block_t state;
      load_state(state, plaintext, AES_BLOCK_WORDS);
      aes_encrypt(&ctx->round_keys, state, AES_BLOCK_WORDS, AES_ROUNDS_192);
      store_state(ciphertext, state, AES_BLOCK_WORDS);
    }
  }
  default: {
    for (; blocks; --blocks, plaintext += IV_SIZE, ciphertext += IV_SIZE) {
      aes_block_t state;
      load_state(state, plaintext, AES_BLOCK_WORDS);
      aes_encrypt(&ctx->round_keys, state, AES_BLOCK_WORDS, AES_ROUNDS_128);
      store_state(ciphertext, state, AES_BLOCK_WORDS);
    }
  }
  }

  return 0;
}

void generic_aes_ecb_free(generic_aes_ecb_t* ctx) {
  sydo_explicit_bzero(&ctx->round_keys, sizeof(ctx->round_keys));
}
#endif
