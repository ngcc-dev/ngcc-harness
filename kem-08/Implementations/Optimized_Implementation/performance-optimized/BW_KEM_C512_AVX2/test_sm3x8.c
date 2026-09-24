#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "auxfunc.h"
#include "sm3x8_avx2.h"

static void fill_bytes(uint8_t *buf, size_t len, uint8_t seed)
{
  size_t i;

  for(i = 0; i < len; i++)
    buf[i] = (uint8_t)(seed + 13u * (uint8_t)i + (uint8_t)(i >> 2));
}

static void assert_equal(const char *name, const uint8_t *got, const uint8_t *want, size_t len)
{
  size_t i;

  for(i = 0; i < len; i++) {
    if(got[i] != want[i]) {
      fprintf(stderr, "%s mismatch at %zu: got %02x want %02x\n", name, i, got[i], want[i]);
      exit(1);
    }
  }
}

int main(void)
{
  static const size_t lens[] = {0, 1, 21, 22, 33, 55, 56, 64, 96};
  uint8_t data[8 * 96];
  uint8_t got[8][32];
  uint8_t ref[32];
  size_t i;
  int lane;

  for(i = 0; i < sizeof(lens) / sizeof(lens[0]); i++) {
    fill_bytes(data, 8 * lens[i], (uint8_t)(0x80 + i));
    sm3_x8_digest(data, lens[i], got);
    for(lane = 0; lane < 8; lane++) {
      if(sm3hash(256, data + (size_t)lane * lens[i], (unsigned long long)lens[i] * 8ULL, ref) != 0) {
        fprintf(stderr, "sm3hash failed\n");
        exit(1);
      }
      {
        char name[64];
        snprintf(name, sizeof(name), "sm3_x8_digest len=%zu lane=%d", lens[i], lane);
        assert_equal(name, got[lane], ref, sizeof(ref));
      }
    }
  }

  puts("test_sm3x8: ok");
  return 0;
}
