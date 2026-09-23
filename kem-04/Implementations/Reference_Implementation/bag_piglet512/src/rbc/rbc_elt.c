#include "randombytes.h"
#include "rbc_elt.h"

#if defined(__PCLMUL__) && (defined(__x86_64__) || defined(__i386__))
#include <wmmintrin.h>
#define RBC_USE_PCLMUL 1
#else
#define RBC_USE_PCLMUL 0
#endif

static void rbc_elt_mask(rbc_elt o) {
  const uint32_t used_bits = RBC_FIELD_M % 64;
  if(used_bits != 0) {
    o[RBC_ELT_UINT64 - 1] &= ((uint64_t)1 << used_bits) - 1;
  }
}

static uint32_t rbc_modulus_exponents[RBC_FIELD_M];
static uint32_t rbc_modulus_exponents_count = 0;

static void rbc_ur_xor_shifted(rbc_elt_ur o, const uint64_t *input,
                               uint32_t input_words, uint32_t shift) {
  const uint32_t word_shift = shift / 64;
  const uint32_t bit_shift = shift % 64;

  for(uint32_t i = 0; i < input_words; ++i) {
    const uint64_t word = input[i];
    const uint32_t out = word_shift + i;
    if(out < RBC_ELT_UR_UINT64) {
      o[out] ^= word << bit_shift;
    }
    if(bit_shift != 0 && out + 1 < RBC_ELT_UR_UINT64) {
      o[out + 1] ^= word >> (64 - bit_shift);
    }
  }
}

void rbc_field_init(void) {
  if(rbc_modulus_exponents_count != 0) {
    return;
  }

  for(uint32_t exponent = 0; exponent < RBC_FIELD_M; ++exponent) {
    if((RBC_ELT_MODULUS[exponent / 64] >> (exponent % 64)) & 1U) {
      rbc_modulus_exponents[rbc_modulus_exponents_count++] = exponent;
    }
  }
}

void rbc_elt_set_zero(rbc_elt o) {
  memset(o, 0, sizeof(uint64_t) * RBC_ELT_UINT64);
}

void rbc_elt_set_one(rbc_elt o) {
  rbc_elt_set_zero(o);
  o[0] = 1;
}

void rbc_elt_set(rbc_elt o, const rbc_elt e) {
  memcpy(o, e, sizeof(uint64_t) * RBC_ELT_UINT64);
}

void rbc_elt_set_random(rbc_elt o, DRNG_ctx* ctx) {
  uint8_t random[RBC_FIELD_BYTES];
  rbc_elt_set_zero(o);
  get_random_number(ctx, random, 8ULL * RBC_FIELD_BYTES);
  memcpy(o, random, RBC_FIELD_BYTES);
  rbc_elt_mask(o);
}

void rbc_elt_set_random2(rbc_elt o) {
  uint8_t random[RBC_FIELD_BYTES];
  rbc_elt_set_zero(o);
  CryptoRandomBytes(random, RBC_FIELD_BYTES);
  memcpy(o, random, RBC_FIELD_BYTES);
  rbc_elt_mask(o);
}

uint8_t rbc_elt_is_zero(const rbc_elt e) {
  for(uint32_t i = 0; i < RBC_ELT_UINT64; ++i) {
    if(e[i] != 0) {
      return 0;
    }
  }

  return 1;
}

uint8_t rbc_elt_is_equal_to(const rbc_elt e1, const rbc_elt e2) {
  for(uint32_t i = 0; i < RBC_ELT_UINT64; ++i) {
    if(e1[i] != e2[i]) {
      return 0;
    }
  }

  return 1;
}

uint8_t rbc_elt_get_coefficient(const rbc_elt e, uint32_t index) {
  if(index >= RBC_FIELD_M) {
    return 0;
  }

  return (e[index / 64] >> (index % 64)) & 1;
}

void rbc_elt_set_coefficient(rbc_elt o, uint32_t index, uint8_t bit) {
  if(index >= RBC_FIELD_M) {
    return;
  }

  if(bit & 1) {
    o[index / 64] |= (uint64_t)1 << (index % 64);
  }
  else {
    o[index / 64] &= ~((uint64_t)1 << (index % 64));
  }
}

void rbc_elt_add(rbc_elt o, const rbc_elt e1, const rbc_elt e2) {
  for(uint32_t i = 0; i < RBC_ELT_UINT64; ++i) {
    o[i] = e1[i] ^ e2[i];
  }
}

void rbc_elt_reduce(rbc_elt o, const rbc_elt_ur e) {
  rbc_elt_ur reduced;
  rbc_elt high;
  const uint32_t source_word = RBC_FIELD_M / 64;
  const uint32_t source_bit = RBC_FIELD_M % 64;
  const uint64_t top_mask = source_bit == 0
    ? UINT64_MAX : (((uint64_t) 1 << source_bit) - 1);

  memcpy(reduced, e, sizeof(reduced));
  for(;;) {
    uint64_t high_nonzero = 0;
    for(uint32_t i = 0; i < RBC_ELT_UINT64; ++i) {
      const uint32_t source = source_word + i;
      uint64_t value = source < RBC_ELT_UR_UINT64
        ? reduced[source] >> source_bit : 0;
      if(source_bit != 0 && source + 1 < RBC_ELT_UR_UINT64) {
        value ^= reduced[source + 1] << (64 - source_bit);
      }
      high[i] = value;
      high_nonzero |= value;
    }

    if(high_nonzero == 0) {
      break;
    }

    reduced[RBC_ELT_UINT64 - 1] &= top_mask;
    for(uint32_t i = RBC_ELT_UINT64; i < RBC_ELT_UR_UINT64; ++i) {
      reduced[i] = 0;
    }
    for(uint32_t i = 0; i < rbc_modulus_exponents_count; ++i) {
      rbc_ur_xor_shifted(reduced, high, RBC_ELT_UINT64,
                         rbc_modulus_exponents[i]);
    }
  }

  for(uint32_t i = 0; i < RBC_ELT_UINT64; ++i) {
    o[i] = reduced[i];
  }

  rbc_elt_mask(o);
}

void rbc_elt_mul(rbc_elt o, const rbc_elt e1, const rbc_elt e2) {
  rbc_elt_ur tmp;
  memset(tmp, 0, sizeof(tmp));

#if RBC_USE_PCLMUL
  for(uint32_t i = 0; i < RBC_ELT_UINT64; ++i) {
    const __m128i left = _mm_set_epi64x(0, (long long) e1[i]);
    for(uint32_t j = 0; j < RBC_ELT_UINT64; ++j) {
      const __m128i right = _mm_set_epi64x(0, (long long) e2[j]);
      const __m128i product = _mm_clmulepi64_si128(left, right, 0x00);
      uint64_t words[2];
      _mm_storeu_si128((__m128i *) words, product);
      if(i + j < RBC_ELT_UR_UINT64) {
        tmp[i + j] ^= words[0];
      }
      if(i + j + 1 < RBC_ELT_UR_UINT64) {
        tmp[i + j + 1] ^= words[1];
      }
    }
  }
#else
  for(uint32_t i = 0; i < RBC_FIELD_M; ++i) {
    if((e1[i / 64] >> (i % 64)) & 1U) {
      const uint32_t word_shift = i / 64;
      const uint32_t bit_shift = i % 64;
      for(uint32_t j = 0; j < RBC_ELT_UINT64; ++j) {
        const uint64_t word = e2[j];
        const uint32_t out = word_shift + j;
        if(out < RBC_ELT_UR_UINT64) {
          tmp[out] ^= word << bit_shift;
        }
        if(bit_shift != 0 && out + 1 < RBC_ELT_UR_UINT64) {
          tmp[out + 1] ^= word >> (64 - bit_shift);
        }
      }
    }
  }
#endif

  rbc_elt_reduce(o, tmp);
}

static uint16_t rbc_square_byte(uint8_t value) {
  uint16_t expanded = value;
  expanded = (uint16_t) ((expanded | (expanded << 4)) & 0x0f0fU);
  expanded = (uint16_t) ((expanded | (expanded << 2)) & 0x3333U);
  expanded = (uint16_t) ((expanded | (expanded << 1)) & 0x5555U);
  return expanded;
}

void rbc_elt_sqr(rbc_elt o, const rbc_elt e) {
  rbc_elt_ur tmp;
  memset(tmp, 0, sizeof(tmp));

  for(uint32_t word = 0; word < RBC_ELT_UINT64; ++word) {
    for(uint32_t byte = 0; byte < 8; ++byte) {
      const uint8_t value = (uint8_t) (e[word] >> (8 * byte));
      const uint32_t bit_position = 2 * (64 * word + 8 * byte);
      const uint32_t out = bit_position / 64;
      if(out < RBC_ELT_UR_UINT64) {
        tmp[out] ^= (uint64_t) rbc_square_byte(value) <<
          (bit_position % 64);
      }
    }
  }

  rbc_elt_reduce(o, tmp);
}

void rbc_elt_inv(rbc_elt o, const rbc_elt e) {
  if(rbc_elt_is_zero(e)) {
    rbc_elt_set_zero(o);
    return;
  }

  const uint32_t target = RBC_FIELD_M - 1;
  uint32_t top_bit = 0;
  for(uint32_t value = target; value > 1; value >>= 1) {
    ++top_bit;
  }

  rbc_elt base;
  rbc_elt result;
  rbc_elt powered;
  rbc_elt_set(base, e);
  rbc_elt_set(result, e);

  uint32_t span = 1;
  for(int32_t bit = (int32_t) top_bit - 1; bit >= 0; --bit) {
    rbc_elt_set(powered, result);
    for(uint32_t i = 0; i < span; ++i) {
      rbc_elt_sqr(powered, powered);
    }
    rbc_elt_mul(result, powered, result);
    span *= 2;

    if((target >> (uint32_t) bit) & 1U) {
      rbc_elt_sqr(result, result);
      rbc_elt_mul(result, result, base);
      ++span;
    }
  }

  rbc_elt_sqr(o, result);
}

void rbc_elt_from_string(rbc_elt o, const uint8_t* str) {
  rbc_elt_set_zero(o);
  memcpy(o, str, RBC_FIELD_BYTES);
  rbc_elt_mask(o);
}

void rbc_elt_to_string(uint8_t* str, const rbc_elt e) {
  memset(str, 0, RBC_FIELD_BYTES);
  memcpy(str, e, RBC_FIELD_BYTES);
}

void rbc_elt_print(const rbc_elt e) {
  printf("0x");
  for(int32_t i = RBC_ELT_UINT64 - 1; i >= 0; --i) {
    printf("%016" PRIx64, e[i]);
  }
}
