#include "utils.h"

#include <string.h>

uint32_t rotl32_u(uint32_t x, uint8_t n) {
  return (x << n) | (x >> (32 - n));
}
