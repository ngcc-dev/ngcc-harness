#include "utils.h"

#include <string.h>

uint32_t rotl32_u(uint32_t x, uint8_t n) {
  return (x << n) | (x >> (32 - n));
}

bool decode_all_chall_3(const params_t* params, uint16_t* decoded_chall, const uint8_t* chall) {
  const unsigned int tau = params->tau;
  const unsigned int tau1 = params->tau1;
  const unsigned int k = params->k;
  const size_t chall_bytes = params->lambda_bytes;

  for (unsigned int i = 0; i != tau; ++i) {
    unsigned int lo;
    unsigned int len;
    if (i < tau1) {
      lo = i * k;
      len = k;
    } else {
      const unsigned int t = i - tau1;
      lo = (tau1 * k) + (t * (k - 1));
      len = k - 1;
    }

    const size_t byte_off = lo >> 3;
    const unsigned int bit_off = lo & 7u;
    uint32_t word = 0;
    if (byte_off < chall_bytes) {
      word |= (uint32_t)chall[byte_off];
    }
    if (byte_off + 1u < chall_bytes) {
      word |= (uint32_t)chall[byte_off + 1u] << 8;
    }
    if (byte_off + 2u < chall_bytes) {
      word |= (uint32_t)chall[byte_off + 2u] << 16;
    }

    const uint16_t mask = (uint16_t)((1u << len) - 1u);
    decoded_chall[i] = (uint16_t)((word >> bit_off) & mask);
  }
  return true;
}
