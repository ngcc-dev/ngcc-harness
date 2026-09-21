#include <stdint.h>
#include "params.h"
#include "cbd.h"

/*************************************************
* Name:        load32_littleendian
*
* Description: load 4 bytes into a 32-bit integer
*              in little-endian order
*
* Arguments:   - const uint8_t *x: pointer to input byte array
*
* Returns 32-bit unsigned integer loaded from x
**************************************************/
#if WEAVER_ETA1 == 1 || WEAVER_ETA1 == 2 || WEAVER_ETA1 == 4 || WEAVER_ETA1 == 8 || \
    WEAVER_ETA2 == 2 || WEAVER_ETA2 == 4 || WEAVER_ETA2 == 8
static uint32_t load32_littleendian(const uint8_t x[4])
{
  uint32_t r;
  r  = (uint32_t)x[0];
  r |= (uint32_t)x[1] << 8;
  r |= (uint32_t)x[2] << 16;
  r |= (uint32_t)x[3] << 24;
  return r;
}
#endif

/*************************************************
* Name:        load24_littleendian
*
* Description: load 3 bytes into a 32-bit integer
*              in little-endian order
*              This function is only needed for Kyber-512
*
* Arguments:   - const uint8_t *x: pointer to input byte array
*
* Returns 32-bit unsigned integer loaded from x (most significant byte is zero)
**************************************************/
#if WEAVER_ETA1 == 3 || WEAVER_ETA2 == 3
static uint32_t load24_littleendian(const uint8_t x[3])
{
  uint32_t r;
  r  = (uint32_t)x[0];
  r |= (uint32_t)x[1] << 8;
  r |= (uint32_t)x[2] << 16;
  return r;
}
#endif


/*************************************************
* Name:        cbd2
*
* Description: Given an array of uniformly random bytes, compute
*              polynomial with coefficients distributed according to
*              a centered binomial distribution with parameter eta=2
*
* Arguments:   - poly *r:            pointer to output polynomial
*              - const uint8_t *buf: pointer to input byte array
**************************************************/
#if WEAVER_ETA1 == 2 || WEAVER_ETA2 == 2
static void cbd2(poly *r, const uint8_t buf[2*WEAVER_N/4])
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
#endif

/*************************************************
* Name:        cbd3
*
* Description: Given an array of uniformly random bytes, compute
*              polynomial with coefficients distributed according to
*              a centered binomial distribution with parameter eta=3
*              This function is only needed for Kyber-512
*
* Arguments:   - poly *r:            pointer to output polynomial
*              - const uint8_t *buf: pointer to input byte array
**************************************************/
#if WEAVER_ETA1 == 3 || WEAVER_ETA2 == 3
static void cbd3(poly *r, const uint8_t buf[3*WEAVER_N/4])
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
#endif

/*************************************************
* Name:        cbd4
*
* Description: Given an array of uniformly random bytes, compute
*              polynomial with coefficients distributed according to
*              a centered binomial distribution with parameter eta=4
*              This function is needed for WEAVER-512
*
* Arguments:   - poly *r:            pointer to output polynomial
*              - const uint8_t *buf: pointer to input byte array
**************************************************/
#if WEAVER_ETA1 == 4 || WEAVER_ETA2 == 4
static void cbd4(poly *r, const uint8_t buf[4*WEAVER_N/4])
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
#endif

/* cbd1: eta=1, 1 bit per sample, 2 bits per coefficient */
#if WEAVER_ETA1 == 1
static void cbd1(poly *r, const uint8_t buf[1*WEAVER_N/4])
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
#endif

#if WEAVER_ETA1 == 5 || WEAVER_ETA1 == 10 || WEAVER_ETA2 == 5 || WEAVER_ETA2 == 10
static unsigned int popcount5(uint16_t x)
{
  x &= 0x1F;
  return (x & 1u) + ((x >> 1) & 1u) + ((x >> 2) & 1u) + ((x >> 3) & 1u) + ((x >> 4) & 1u);
}
#endif

#if WEAVER_ETA1 == 5 || WEAVER_ETA2 == 5
static void cbd5(poly *r, const uint8_t *buf, unsigned int n)
{
  unsigned int i, j;

  for(i = 0; i < n/4; i++) {
    uint64_t t = (uint64_t)buf[5*i + 0]
               | ((uint64_t)buf[5*i + 1] << 8)
               | ((uint64_t)buf[5*i + 2] << 16)
               | ((uint64_t)buf[5*i + 3] << 24)
               | ((uint64_t)buf[5*i + 4] << 32);

    for(j = 0; j < 4; j++) {
      uint16_t v = (t >> (10*j)) & 0x3FF;
      int16_t a = (int16_t)popcount5(v);
      int16_t b = (int16_t)popcount5(v >> 5);
      r->coeffs[4*i + j] = a - b;
    }
  }
}
#endif

#if WEAVER_ETA1 == 8 || WEAVER_ETA2 == 8
static void cbd8(poly *r, const uint8_t *buf, unsigned int n)
{
  unsigned int i, j;
  uint32_t t, d;
  int16_t a, b;

  for(i = 0; i < n/2; i++) {
    t  = load32_littleendian(buf + 4*i);
    d  = t & 0x01010101;
    d += (t >> 1) & 0x01010101;
    d += (t >> 2) & 0x01010101;
    d += (t >> 3) & 0x01010101;
    d += (t >> 4) & 0x01010101;
    d += (t >> 5) & 0x01010101;
    d += (t >> 6) & 0x01010101;
    d += (t >> 7) & 0x01010101;

    for(j = 0; j < 2; j++) {
      a = (d >> (16*j + 0)) & 0xFF;
      b = (d >> (16*j + 8)) & 0xFF;
      r->coeffs[2*i + j] = a - b;
    }
  }
}
#endif

#if WEAVER_ETA1 == 10 || WEAVER_ETA2 == 10
static void cbd10(poly *r, const uint8_t *buf, unsigned int n)
{
  unsigned int i, j;

  for(i = 0; i < n/2; i++) {
    uint64_t t = (uint64_t)buf[5*i + 0]
               | ((uint64_t)buf[5*i + 1] << 8)
               | ((uint64_t)buf[5*i + 2] << 16)
               | ((uint64_t)buf[5*i + 3] << 24)
               | ((uint64_t)buf[5*i + 4] << 32);

    for(j = 0; j < 2; j++) {
      uint32_t v = (uint32_t)((t >> (20*j)) & 0xFFFFF);
      int16_t a = (int16_t)popcount5(v & 0x1F)
                + (int16_t)popcount5((v >> 5) & 0x1F);
      int16_t b = (int16_t)popcount5((v >> 10) & 0x1F)
                + (int16_t)popcount5((v >> 15) & 0x1F);
      r->coeffs[2*i + j] = a - b;
    }
  }
}
#endif

#if WEAVER_ETA1 == 5
static void cbd5_eta1(poly *r, const uint8_t buf[5*WEAVER_N/4])
{
  cbd5(r, buf, WEAVER_N);
}
#endif

void cbd_eta1(poly *r, const uint8_t buf[WEAVER_ETA1*WEAVER_N/4])
{
#if WEAVER_ETA1 == 1
  cbd1(r, buf);
#elif WEAVER_ETA1 == 2
  cbd2(r, buf);
#elif WEAVER_ETA1 == 3
  cbd3(r, buf);
#elif WEAVER_ETA1 == 4
  cbd4(r, buf);
#elif WEAVER_ETA1 == 5
  cbd5_eta1(r, buf);
#elif WEAVER_ETA1 == 8
  cbd8(r, buf, WEAVER_N);
#elif WEAVER_ETA1 == 10
  cbd10(r, buf, WEAVER_N);
#else
#error "This implementation requires eta1 in {1,2,3,4,5,8,10}"
#endif
}

void cbd_eta2(poly *r, const uint8_t buf[WEAVER_ETA2*WEAVER_N/4])
{
#if WEAVER_ETA2 == 2
  cbd2(r, buf);
#elif WEAVER_ETA2 == 3
  cbd3(r, buf);
#elif WEAVER_ETA2 == 4
  cbd4(r, buf);
#elif WEAVER_ETA2 == 5
  cbd5(r, buf, WEAVER_N);
#elif WEAVER_ETA2 == 8
  cbd8(r, buf, WEAVER_N);
#elif WEAVER_ETA2 == 10
  cbd10(r, buf, WEAVER_N);
#else
#error "This implementation requires eta2 in {2,3,4,5,8,10}"
#endif
}

