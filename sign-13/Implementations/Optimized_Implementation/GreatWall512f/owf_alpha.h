#ifndef OWF_ALPHA_H
#define OWF_ALPHA_H

#include <stdbool.h>
#include <stdint.h>

#include "hash.h"
#include "owf_proof.h"

static inline void owf_alpha_store(uint8_t* packed, const owf_block* value)
{
#if SECURITY_PARAM == 128
  block137_store_from_block(packed, value);
#elif SECURITY_PARAM == 192
  block197_store_from_block(packed, value);
#elif SECURITY_PARAM == 256
  block263_store_from_block(packed, value);
#elif SECURITY_PARAM == 512
  block521_store_from_block(packed, value);
#endif
}

static inline void owf_alpha_load(owf_block* value, const uint8_t* packed)
{
#if SECURITY_PARAM == 128
  block137_load_to_block(value, packed);
#elif SECURITY_PARAM == 192
  block197_load_to_block(value, packed);
#elif SECURITY_PARAM == 256
  block263_load_to_block(value, packed);
#elif SECURITY_PARAM == 512
  block521_load_to_block(value, packed);
#endif
}

static inline bool owf_alpha_is_zero(const owf_block* value)
{
  uint64_t combined = 0;
  for (size_t word = 0; word < sizeof(value->data) / sizeof(value->data[0]); ++word)
    combined |= value->data[word];
  return combined == 0;
}

static inline bool derive_owf_alpha(
    owf_block* alpha, const owf_block* owf_iv, uint8_t selector)
{
  uint8_t iv_bytes[OWF_BLOCK_SIZE];
  uint8_t alpha_bytes[OWF_BLOCK_SIZE];
  owf_alpha_store(iv_bytes, owf_iv);

  hash_state hasher;
  if (hash_init(&hasher) != 0 ||
      hash_update(&hasher, iv_bytes, sizeof(iv_bytes)) != 0 ||
      hash_update_byte(&hasher, selector) != 0 ||
      hash_final(&hasher, alpha_bytes, sizeof(alpha_bytes)) != 0)
    return false;

#if SECURITY_PARAM == 128 || SECURITY_PARAM == 512
  alpha_bytes[OWF_BLOCK_SIZE - 1] &= 0x01;
#elif SECURITY_PARAM == 192
  alpha_bytes[OWF_BLOCK_SIZE - 1] &= 0x1F;
#elif SECURITY_PARAM == 256
  alpha_bytes[OWF_BLOCK_SIZE - 1] &= 0x7F;
#endif

  owf_alpha_load(alpha, alpha_bytes);
  return true;
}

static inline bool owf_alpha_dot_is_one(
    const owf_block* alpha, const owf_block* sbox_output)
{
  unsigned int parity = 0;
  for (size_t word = 0; word < sizeof(alpha->data) / sizeof(alpha->data[0]); ++word)
    parity ^= (unsigned int)__builtin_parityll(alpha->data[word] & sbox_output->data[word]);
  return (parity & 1U) != 0;
}

#endif
