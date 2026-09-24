#include <stdint.h>
#include "params.h"
#include "cbd.h"

static uint32_t load32_littleendian(const uint8_t x[4])
{
  uint32_t r;
  r  = (uint32_t)x[0];
  r |= (uint32_t)x[1] << 8;
  r |= (uint32_t)x[2] << 16;
  r |= (uint32_t)x[3] << 24;
  return r;
}

static uint32_t load24_littleendian(const uint8_t x[3])
{
  uint32_t r;
  r  = (uint32_t)x[0];
  r |= (uint32_t)x[1] << 8;
  r |= (uint32_t)x[2] << 16;
  return r;
}

static unsigned int popcount_u32(uint32_t x)
{
  unsigned int r = 0;
  while(x) {
    r += x & 1u;
    x >>= 1;
  }
  return r;
}

#if defined(__GNUC__)
#define WEAVER_UNUSED __attribute__((unused))
#else
#define WEAVER_UNUSED
#endif

static WEAVER_UNUSED void cbd1(poly *r, const uint8_t buf[1*WEAVER_N/4])
{
  unsigned int i,j;
  uint32_t t;
  int16_t a,b;

  for(i=0;i<WEAVER_N/16;i++) {
    t = load32_littleendian(buf + 4*i);
    for(j=0;j<16;j++) {
      a = (t >> (2*j+0)) & 0x1;
      b = (t >> (2*j+1)) & 0x1;
      r->coeffs[16*i+j] = a - b;
    }
  }
}

static WEAVER_UNUSED void cbd2(poly *r, const uint8_t buf[2*WEAVER_N/4])
{
  unsigned int i,j;
  uint32_t t,d;
  int16_t a,b;

  for(i=0;i<WEAVER_N/8;i++) {
    t  = load32_littleendian(buf+4*i);
    d  = t & 0x55555555;
    d += (t>>1) & 0x55555555;

    for(j=0;j<8;j++) {
      a = (d >> (4*j+0)) & 0x3;
      b = (d >> (4*j+2)) & 0x3;
      r->coeffs[8*i+j] = a - b;
    }
  }
}

static WEAVER_UNUSED void cbd3(poly *r, const uint8_t buf[3*WEAVER_N/4])
{
  unsigned int i,j;
  uint32_t t,d;
  int16_t a,b;

  for(i=0;i<WEAVER_N/4;i++) {
    t  = load24_littleendian(buf+3*i);
    d  = t & 0x00249249;
    d += (t>>1) & 0x00249249;
    d += (t>>2) & 0x00249249;

    for(j=0;j<4;j++) {
      a = (d >> (6*j+0)) & 0x7;
      b = (d >> (6*j+3)) & 0x7;
      r->coeffs[4*i+j] = a - b;
    }
  }
}

static WEAVER_UNUSED void cbd4(poly *r, const uint8_t buf[4*WEAVER_N/4])
{
  unsigned int i,j;
  uint32_t t,d;
  int16_t a,b;

  for(i=0;i<WEAVER_N/4;i++) {
    t  = load32_littleendian(buf+4*i);
    d  = t & 0x11111111;
    d += (t>>1) & 0x11111111;
    d += (t>>2) & 0x11111111;
    d += (t>>3) & 0x11111111;

    for(j=0;j<4;j++) {
      a = (d >> (8*j+0)) & 0xf;
      b = (d >> (8*j+4)) & 0xf;
      r->coeffs[4*i+j] = a - b;
    }
  }
}

static WEAVER_UNUSED void cbd5(poly *r, const uint8_t buf[5*WEAVER_N/4])
{
  unsigned int i, j;

  for(i = 0; i < WEAVER_N/4; i++) {
    uint64_t t = (uint64_t)buf[5*i + 0]
               | ((uint64_t)buf[5*i + 1] << 8)
               | ((uint64_t)buf[5*i + 2] << 16)
               | ((uint64_t)buf[5*i + 3] << 24)
               | ((uint64_t)buf[5*i + 4] << 32);

    for(j = 0; j < 4; j++) {
      uint32_t v = (uint32_t)((t >> (10*j)) & 0x3FF);
      int16_t a = (int16_t)popcount_u32(v & 0x1F);
      int16_t b = (int16_t)popcount_u32((v >> 5) & 0x1F);
      r->coeffs[4*i + j] = a - b;
    }
  }
}

static WEAVER_UNUSED void cbd6(poly *r, const uint8_t buf[6*WEAVER_N/4])
{
  unsigned int i, j;

  for(i = 0; i < WEAVER_N/2; i++) {
    uint32_t t = (uint32_t)buf[3*i + 0]
               | ((uint32_t)buf[3*i + 1] << 8)
               | ((uint32_t)buf[3*i + 2] << 16);

    for(j = 0; j < 2; j++) {
      uint32_t v = (t >> (12*j)) & 0xFFF;
      int16_t a = (int16_t)popcount_u32(v & 0x3F);
      int16_t b = (int16_t)popcount_u32((v >> 6) & 0x3F);
      r->coeffs[2*i + j] = a - b;
    }
  }
}

static WEAVER_UNUSED void cbd7(poly *r, const uint8_t buf[7*WEAVER_N/4])
{
  unsigned int i, j;

  for(i = 0; i < WEAVER_N/4; i++) {
    uint64_t t = (uint64_t)buf[7*i + 0]
               | ((uint64_t)buf[7*i + 1] << 8)
               | ((uint64_t)buf[7*i + 2] << 16)
               | ((uint64_t)buf[7*i + 3] << 24)
               | ((uint64_t)buf[7*i + 4] << 32)
               | ((uint64_t)buf[7*i + 5] << 40)
               | ((uint64_t)buf[7*i + 6] << 48);

    for(j = 0; j < 4; j++) {
      uint32_t v = (uint32_t)((t >> (14*j)) & 0x3FFF);
      int16_t a = (int16_t)popcount_u32(v & 0x7F);
      int16_t b = (int16_t)popcount_u32((v >> 7) & 0x7F);
      r->coeffs[4*i + j] = a - b;
    }
  }
}

static WEAVER_UNUSED void cbd9(poly *r, const uint8_t buf[9*WEAVER_N/4])
{
  unsigned int i, j;

  for(i = 0; i < WEAVER_N/4; i++) {
    uint64_t t0 = (uint64_t)buf[9*i + 0]
                | ((uint64_t)buf[9*i + 1] << 8)
                | ((uint64_t)buf[9*i + 2] << 16)
                | ((uint64_t)buf[9*i + 3] << 24)
                | ((uint64_t)buf[9*i + 4] << 32)
                | ((uint64_t)buf[9*i + 5] << 40)
                | ((uint64_t)buf[9*i + 6] << 48)
                | ((uint64_t)buf[9*i + 7] << 56);
    uint32_t t1 = (uint32_t)buf[9*i + 8];

    for(j = 0; j < 4; j++) {
      uint32_t lo_shift = 18*j;
      uint32_t v;
      if(lo_shift <= 14) {
        v = (uint32_t)((t0 >> lo_shift) & 0x3FFFF);
      } else {
        uint32_t hi_shift = lo_shift - 18;
        v = (uint32_t)((t0 >> lo_shift) | ((uint64_t)t1 << (64 - lo_shift)));
        v &= 0x3FFFF;
        (void)hi_shift;
      }
      {
        int16_t a = (int16_t)popcount_u32(v & 0x1FF);
        int16_t b = (int16_t)popcount_u32((v >> 9) & 0x1FF);
        r->coeffs[4*i + j] = a - b;
      }
    }
  }
}

void cbd_eta1(poly *r, const uint8_t buf[WEAVER_ETA1*WEAVER_N/4])
{
#if defined(WEAVER_USE_AVX_CBD)
  cbd_eta1_avx(r, buf);
#else
#if WEAVER_ETA1 == 1
  cbd1(r, buf);
#elif WEAVER_ETA1 == 2
  cbd2(r, buf);
#elif WEAVER_ETA1 == 3
  cbd3(r, buf);
#elif WEAVER_ETA1 == 4
  cbd4(r, buf);
#elif WEAVER_ETA1 == 5
  cbd5(r, buf);
#elif WEAVER_ETA1 == 6
  cbd6(r, buf);
#elif WEAVER_ETA1 == 7
  cbd7(r, buf);
#elif WEAVER_ETA1 == 9
  cbd9(r, buf);
#else
#error "This implementation requires eta1 in {1,2,3,4,5,6,7,9}"
#endif
#endif
}

void cbd_eta2(poly *r, const uint8_t buf[WEAVER_ETA2*WEAVER_N/4])
{
#if defined(WEAVER_USE_AVX_CBD)
  cbd_eta2_avx(r, buf);
#else
#if WEAVER_ETA2 == 1
  cbd1(r, buf);
#elif WEAVER_ETA2 == 2
  cbd2(r, buf);
#elif WEAVER_ETA2 == 3
  cbd3(r, buf);
#elif WEAVER_ETA2 == 4
  cbd4(r, buf);
#elif WEAVER_ETA2 == 5
  cbd5(r, buf);
#elif WEAVER_ETA2 == 6
  cbd6(r, buf);
#elif WEAVER_ETA2 == 7
  cbd7(r, buf);
#elif WEAVER_ETA2 == 9
  cbd9(r, buf);
#else
#error "This implementation requires eta2 in {1,2,3,4,5,6,7,9}"
#endif
#endif
}
