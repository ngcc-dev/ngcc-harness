#include "vistrutith_witness.h"

#include "../fields.h"
#include "../utils.h"

#include <assert.h>
#include <string.h>

static const uint8_t vistrutith_zero_round_key[VISTRUTAH_TRACE_ROUND_BYTES] = {0};

static bf8_t vistrutith_bf_exp_238(bf8_t x) {
  // 238 == 0b11101110
  bf8_t y = bf8_square(x); // x^2
  x       = bf8_square(y); // x^4
  y       = bf8_mul(x, y);
  x       = bf8_square(x); // x^8
  y       = bf8_mul(x, y);
  x       = bf8_square(x); // x^16
  x       = bf8_square(x); // x^32
  y       = bf8_mul(x, y);
  x       = bf8_square(x); // x^64
  y       = bf8_mul(x, y);
  x       = bf8_square(x); // x^128
  return bf8_mul(x, y);
}

uint8_t vistrutith_inv_norm(uint8_t in) {
  if (in == 0) {
    return 0;
  }

  // Instead of in^{-17}, compute in^{238} in GF(2^8)^*.
  in = vistrutith_bf_exp_238(in);
  return (uint8_t)(set_bit(get_bit(in, 0), 0) ^ set_bit(get_bit(in, 6), 1) ^
                   set_bit(get_bit(in, 7), 2) ^ set_bit(get_bit(in, 2), 3));
}

void vistrutith_extend_witness(const params_t* params, uint8_t* w, const uint8_t* key,
                               const uint8_t* in) {

  const size_t witness_bytes = (size_t)(params->ell / 8u);
  const unsigned int steps = (unsigned int)(VISTRUTAH_512_ROUNDS_LONG_512KEY / ROUNDS_PER_STEP);
  // FK
  uint8_t fixed_key[64];
  // RK
  uint8_t round_key[64];
  // 4 state include 4 AES 128 bits, so 64 states
  uint8_t state[64];
  uint8_t* p = w;

  if (witness_bytes != VISTRUTAH_WITNESS_TOTAL_BYTES) {
    return;
  }

  // :5 
  memcpy(p, key, VISTRUTAH_WITNESS_KEY_BYTES);
  p += VISTRUTAH_WITNESS_KEY_BYTES;

  // :2-4
  memcpy(fixed_key, key, 64);
  vistrutah_shuffle_key(fixed_key);
  memcpy(round_key + 0, fixed_key + 16, 16);
  memcpy(round_key + 16, fixed_key + 0, 16);
  memcpy(round_key + 32, fixed_key + 48, 16);
  memcpy(round_key + 48, fixed_key + 32, 16);

  // :7-10
  for (unsigned int i = 0; i < 64; ++i) {
    state[i] = (uint8_t)(in[i] ^ round_key[i]);
  }

  // :13-17
  for (unsigned int i = 0; i < VISTRUTAH_WITNESS_PACKED_ROUND_BYTES; ++i) {
    p[i] = (uint8_t)((uint8_t)(vistrutith_inv_norm(state[2u * i + 1u]) << 4) |
                     (uint8_t)(vistrutith_inv_norm(state[2u * i]) & 0x0Fu));
  }
  p += VISTRUTAH_WITNESS_PACKED_ROUND_BYTES;

  // :19-22
  vistrutah_aes_round_128(state + 0, fixed_key + 0);
  vistrutah_aes_round_128(state + 16, fixed_key + 16);
  vistrutah_aes_round_128(state + 32, fixed_key + 32);
  vistrutah_aes_round_128(state + 48, fixed_key + 48);

  // :24
  for (unsigned int i = 1; i < steps; ++i) {

    // :25-28
    vistrutah_aes_round_128(state + 0, vistrutith_zero_round_key + 0);
    vistrutah_aes_round_128(state + 16, vistrutith_zero_round_key + 16);
    vistrutah_aes_round_128(state + 32, vistrutith_zero_round_key + 32);
    vistrutah_aes_round_128(state + 48, vistrutith_zero_round_key + 48);

    // :29
    vistrutah_mixing_layer_512(state);

    // :30-33
    vistrutah_rotate_bytes(round_key + 0, 5, 16);
    vistrutah_rotate_bytes(round_key + 16, 10, 16);
    vistrutah_rotate_bytes(round_key + 32, 5, 16);
    vistrutah_rotate_bytes(round_key + 48, 10, 16);

    // :34-37
    xor_u8_array(state, round_key, state, 64);
    // :34
    for (unsigned int j = 0; j < 16; ++j) {
      state[j] ^= ROUND_CONSTANTS[16u * (i - 1u) + j];
    }

    // Add Mi to w
    memcpy(p, state, VISTRUTAH_TRACE_ROUND_BYTES);
    p += VISTRUTAH_TRACE_ROUND_BYTES;

    // Add Ni to w
    for (unsigned int j = 0; j < VISTRUTAH_WITNESS_PACKED_ROUND_BYTES; ++j) {
      p[j] = (uint8_t)((uint8_t)(vistrutith_inv_norm(state[2u * j + 1u]) << 4) |
                       (uint8_t)(vistrutith_inv_norm(state[2u * j]) & 0x0Fu));
    }
    p += VISTRUTAH_WITNESS_PACKED_ROUND_BYTES;

    // :45-48
    vistrutah_aes_round_128(state + 0, fixed_key + 0);
    vistrutah_aes_round_128(state + 16, fixed_key + 16);
    vistrutah_aes_round_128(state + 32, fixed_key + 32);
    vistrutah_aes_round_128(state + 48, fixed_key + 48);
  }

  // the final output is not included in the witness, so we don't need to add it

  assert((size_t)(p - w) == witness_bytes);
}
