#include "util.h"
#include <stdio.h>

extern inline unsigned int count_trailing_zeros(uint64_t x);
extern inline uint8_t expand_bit_to_byte(unsigned long x, unsigned int i);
extern inline void expand_bits_to_bytes(uint8_t* output, size_t num_bits, size_t x);

void printHex(const char* s, const uint8_t* data, size_t len) {
  printf("%s: ", s);
  for (size_t i = 0; i < len; i++) {
    printf("%02X", data[i]);
  }
  printf("\n");
}
