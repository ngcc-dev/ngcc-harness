#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "params.h"
#include "BWcoding.h"
#include "drng.h"

DRNG_ctx drng_algorithm;

#define decode_bw32 ref_decode_bw32
#define encode_bw32 ref_encode_bw32
#include "../../Reference_Implementation/BW_KEM_C256/BWcoding.c"
#undef decode_bw32
#undef encode_bw32

static uint64_t rng_state = 0x6a09e667f3bcc909ULL;

static uint32_t next_u32(void)
{
  rng_state ^= rng_state << 7;
  rng_state ^= rng_state >> 9;
  rng_state ^= rng_state << 8;
  return (uint32_t)(rng_state >> 16);
}

static int check_encode(uint32_t m, unsigned int iter)
{
  uint64_t got = encode_bw32(m);
  uint64_t want = ref_encode_bw32(m);

  if(got != want) {
    fprintf(stderr,
            "encode mismatch iter=%u m=0x%08x got=0x%016llx want=0x%016llx\n",
            iter,
            m,
            (unsigned long long)got,
            (unsigned long long)want);
    return 0;
  }

  return 1;
}

static int check_decode(const int16_t input[32], unsigned int iter)
{
  int16_t got_vec[32];
  int16_t want_vec[32];
  uint32_t got;
  uint32_t want;

  memcpy(got_vec, input, sizeof(got_vec));
  memcpy(want_vec, input, sizeof(want_vec));

  got = decode_bw32(got_vec);
  want = ref_decode_bw32(want_vec);

  if(got != want) {
    unsigned int i;

    fprintf(stderr,
            "decode mismatch iter=%u got=0x%08x want=0x%08x\n",
            iter,
            got,
            want);
    for(i = 0; i < 32; i++) {
      fprintf(stderr, " input[%u]=%d\n", i, (int)input[i]);
    }
    return 0;
  }

  return 1;
}

static int run_encode_patterns(void)
{
  static const uint32_t patterns[] = {
    0x00000000u, 0xffffffffu, 0xaaaaaaaau, 0x55555555u,
    0x01234567u, 0x89abcdefu, 0x80000000u, 0x00000001u
  };
  unsigned int i;

  for(i = 0; i < sizeof(patterns) / sizeof(patterns[0]); i++) {
    if(!check_encode(patterns[i], i)) {
      return 0;
    }
  }

  for(i = 0; i < 32; i++) {
    if(!check_encode(1u << i, 100u + i)) {
      return 0;
    }
  }

  return 1;
}

static int run_encode_random(unsigned int iterations)
{
  unsigned int i;

  for(i = 0; i < iterations; i++) {
    if(!check_encode(next_u32(), i)) {
      return 0;
    }
  }

  return 1;
}

static int run_decode_patterns(void)
{
  int16_t vec[32];
  unsigned int i;

  memset(vec, 0, sizeof(vec));
  if(!check_decode(vec, 0u)) return 0;

  for(i = 0; i < 32; i++) vec[i] = 0x0fff;
  if(!check_decode(vec, 1u)) return 0;

  for(i = 0; i < 32; i++) vec[i] = 0x0800;
  if(!check_decode(vec, 2u)) return 0;

  for(i = 0; i < 32; i++) vec[i] = (int16_t)((i & 1u) ? 0x0fff : 0);
  if(!check_decode(vec, 3u)) return 0;

  for(i = 0; i < 32; i++) vec[i] = (int16_t)((97u * i) & 0x0fffu);
  if(!check_decode(vec, 4u)) return 0;

  for(i = 0; i < 32; i++) {
    memset(vec, 0, sizeof(vec));
    vec[i] = 0x0800;
    if(!check_decode(vec, 100u + i)) return 0;
  }

  for(i = 0; i < 32; i++) {
    vec[i] = (int16_t)((i % 4u == 0u) ? 0x03ff :
                       (i % 4u == 1u) ? 0x0400 :
                       (i % 4u == 2u) ? 0x07ff : 0x0800);
  }
  if(!check_decode(vec, 200u)) return 0;

  return 1;
}

static int run_decode_random(unsigned int iterations)
{
  int16_t vec[32];
  unsigned int i;
  unsigned int j;

  for(i = 0; i < iterations; i++) {
    for(j = 0; j < 32; j++) {
      vec[j] = (int16_t)(next_u32() & 0x0fffu);
    }
    if(!check_decode(vec, i)) {
      return 0;
    }
  }

  return 1;
}

int main(void)
{
  if(!run_encode_patterns()) return 1;
  if(!run_encode_random(100000u)) return 1;
  if(!run_decode_patterns()) return 1;
  if(!run_decode_random(100000u)) return 1;

  puts("BW32 encode/decode tests passed.");
  return 0;
}
