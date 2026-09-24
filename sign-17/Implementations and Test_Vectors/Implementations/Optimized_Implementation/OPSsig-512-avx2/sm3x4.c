#include <immintrin.h>
#include <stdint.h>
#include <string.h>

#include "sm3x4.h"

#define FF1_X4(x, y, z) _mm256_xor_si256(_mm256_xor_si256((x), (y)), (z))
#define FF2_X4(x, y, z) _mm256_or_si256(_mm256_and_si256((x), (y)), \
                                        _mm256_or_si256(_mm256_and_si256((x), (z)), \
                                                        _mm256_and_si256((y), (z))))
#define GG1_X4(x, y, z) FF1_X4((x), (y), (z))
#define GG2_X4(x, y, z) _mm256_xor_si256(_mm256_and_si256(_mm256_xor_si256((y), (z)), (x)), (z))

static inline uint32_t load32_be(const unsigned char *x) {
  return ((uint32_t)x[0] << 24)
       | ((uint32_t)x[1] << 16)
       | ((uint32_t)x[2] << 8)
       | (uint32_t)x[3];
}

static inline void store32_be(unsigned char *x, uint32_t u) {
  x[0] = (unsigned char)(u >> 24);
  x[1] = (unsigned char)(u >> 16);
  x[2] = (unsigned char)(u >> 8);
  x[3] = (unsigned char)u;
}

static inline uint32_t rol32_scalar(uint32_t x, unsigned int n) {
  if((n & 31U) == 0)
    return x;
  return (x << (n & 31U)) | (x >> ((32U - n) & 31U));
}

static inline __m256i rol32_x4(__m256i x, unsigned int n) {
  if((n & 31U) == 0)
    return x;
  return _mm256_or_si256(_mm256_sllv_epi32(x, _mm256_set1_epi32((int)(n & 31U))),
                         _mm256_srlv_epi32(x, _mm256_set1_epi32((int)((32U - n) & 31U))));
}

static inline __m256i p0_x4(__m256i x) {
  return _mm256_xor_si256(x,
                          _mm256_xor_si256(rol32_x4(x, 9),
                                           rol32_x4(x, 17)));
}

static inline __m256i p1_x4(__m256i x) {
  return _mm256_xor_si256(x,
                          _mm256_xor_si256(rol32_x4(x, 15),
                                           rol32_x4(x, 23)));
}

static inline __m256i load32_be_x4(const unsigned char *in0,
                                   const unsigned char *in1,
                                   const unsigned char *in2,
                                   const unsigned char *in3) {
  return _mm256_set_epi32(0, 0, 0, 0,
                          (int32_t)load32_be(in3),
                          (int32_t)load32_be(in2),
                          (int32_t)load32_be(in1),
                          (int32_t)load32_be(in0));
}

static void sm3_bit_init_x4(__m256i *digest) {
  static const uint32_t iv[8] = {
    0x7380166FU, 0x4914B2B9U, 0x172442D7U, 0xDA8A0600U,
    0xA96F30BCU, 0x163138AAU, 0xE38DEE4DU, 0xB0FB0E4EU
  };

  for(unsigned int i = 0; i < 8; ++i)
    digest[i] = _mm256_set1_epi32((int32_t)iv[i]);
}

static void sm3_bit_compress_4x(__m256i digest[8],
                                const unsigned char *msg0,
                                const unsigned char *msg1,
                                const unsigned char *msg2,
                                const unsigned char *msg3,
                                unsigned long long blocks) {
  while(blocks--) {
    __m256i w[68];
    __m256i w_prime[64];
    __m256i a, b, c, d, e, f, g, h;

    for(unsigned int i = 0; i < 16; ++i) {
      w[i] = load32_be_x4(msg0 + 4*i, msg1 + 4*i, msg2 + 4*i, msg3 + 4*i);
    }

    for(unsigned int i = 16; i < 68; ++i) {
      __m256i x = _mm256_xor_si256(w[i - 16], w[i - 9]);
      x = _mm256_xor_si256(x, rol32_x4(w[i - 3], 15));
      x = p1_x4(x);
      x = _mm256_xor_si256(x, rol32_x4(w[i - 13], 7));
      w[i] = _mm256_xor_si256(x, w[i - 6]);
    }

    for(unsigned int i = 0; i < 64; ++i)
      w_prime[i] = _mm256_xor_si256(w[i], w[i + 4]);

    a = digest[0];
    b = digest[1];
    c = digest[2];
    d = digest[3];
    e = digest[4];
    f = digest[5];
    g = digest[6];
    h = digest[7];

    for(unsigned int i = 0; i < 64; ++i) {
      const uint32_t t = (i < 16) ? 0x79CC4519U : 0x7A879D8AU;
      __m256i ss1, ss2, tt1, tt2;
      __m256i tj = _mm256_set1_epi32((int32_t)rol32_scalar(t, i));

      ss1 = _mm256_add_epi32(rol32_x4(a, 12), e);
      ss1 = _mm256_add_epi32(ss1, tj);
      ss1 = rol32_x4(ss1, 7);
      ss2 = _mm256_xor_si256(ss1, rol32_x4(a, 12));

      if(i < 16) {
        tt1 = FF1_X4(a, b, c);
        tt2 = GG1_X4(e, f, g);
      } else {
        tt1 = FF2_X4(a, b, c);
        tt2 = GG2_X4(e, f, g);
      }

      tt1 = _mm256_add_epi32(_mm256_add_epi32(tt1, d),
                             _mm256_add_epi32(ss2, w_prime[i]));
      tt2 = _mm256_add_epi32(_mm256_add_epi32(tt2, h),
                             _mm256_add_epi32(ss1, w[i]));

      d = c;
      c = rol32_x4(b, 9);
      b = a;
      a = tt1;
      h = g;
      g = rol32_x4(f, 19);
      f = e;
      e = p0_x4(tt2);
    }

    digest[0] = _mm256_xor_si256(digest[0], a);
    digest[1] = _mm256_xor_si256(digest[1], b);
    digest[2] = _mm256_xor_si256(digest[2], c);
    digest[3] = _mm256_xor_si256(digest[3], d);
    digest[4] = _mm256_xor_si256(digest[4], e);
    digest[5] = _mm256_xor_si256(digest[5], f);
    digest[6] = _mm256_xor_si256(digest[6], g);
    digest[7] = _mm256_xor_si256(digest[7], h);

    msg0 += 64;
    msg1 += 64;
    msg2 += 64;
    msg3 += 64;
  }
}

void sm3_bit_4x(const unsigned char *msg0,
                const unsigned char *msg1,
                const unsigned char *msg2,
                const unsigned char *msg3,
                unsigned long long msg_bitlen,
                unsigned char *dgst0,
                unsigned char *dgst1,
                unsigned char *dgst2,
                unsigned char *dgst3) {
  __m256i digest[8];
  unsigned long long block_num = msg_bitlen / 512;
  unsigned long long remain = msg_bitlen & 0x1FFULL;
  unsigned char block[4][128];
  unsigned int lanes[8];
  unsigned int pad_blocks = (remain <= 447) ? 1U : 2U;
  uint64_t total = block_num * 512ULL + remain;
  const unsigned char *msgs[4] = {msg0, msg1, msg2, msg3};
  unsigned char *dgsts[4] = {dgst0, dgst1, dgst2, dgst3};

  sm3_bit_init_x4(digest);

  if(block_num != 0)
    sm3_bit_compress_4x(digest, msg0, msg1, msg2, msg3, block_num);

  memset(block, 0, sizeof(block));
  for(unsigned int lane = 0; lane < 4; ++lane) {
    unsigned long long remain_bytes = (remain + 7) >> 3;
    const unsigned char *tail = msgs[lane] + block_num * 64;

    memcpy(block[lane], tail, remain_bytes);
    block[lane][remain >> 3] &= (unsigned char)((0xFF00U >> (remain & 0x7ULL)) & 0xFFU);
    block[lane][remain >> 3] |= (unsigned char)(1U << (7U - (remain & 0x7ULL)));
    store32_be(block[lane] + pad_blocks * 64 - 8, (uint32_t)(total >> 32));
    store32_be(block[lane] + pad_blocks * 64 - 4, (uint32_t)total);
  }

  sm3_bit_compress_4x(digest, block[0], block[1], block[2], block[3], pad_blocks);

  for(unsigned int i = 0; i < 8; ++i) {
    _mm256_storeu_si256((__m256i *)lanes, digest[i]);
    store32_be(dgsts[0] + 4*i, lanes[0]);
    store32_be(dgsts[1] + 4*i, lanes[1]);
    store32_be(dgsts[2] + 4*i, lanes[2]);
    store32_be(dgsts[3] + 4*i, lanes[3]);
  }
}
