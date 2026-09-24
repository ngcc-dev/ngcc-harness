#include <stddef.h>
#include <stdint.h>
#include "params.h"
#include "cbd.h"

/*************************************************
* Name:        get_bit
*
* Description: Read one bit from a packed little-endian bitstream.
*
* Arguments:   - const uint8_t *buf: pointer to input byte array
*              - size_t bit_index: bit position
*
* Returns 0 or 1
**************************************************/
static unsigned int get_bit(const uint8_t *buf, size_t bit_index)
{
  return (unsigned int)((buf[bit_index >> 3] >> (bit_index & 7u)) & 1u);
}

/*************************************************
* Name:        cbd_eta
*
* Description: Given an array of uniformly random bytes, compute
*              polynomial with coefficients distributed according to
*              a centered binomial distribution with parameter eta.
*
* Arguments:   - poly *r: pointer to output polynomial
*              - const uint8_t *buf: pointer to input byte array
*              - unsigned int eta: centered binomial parameter
**************************************************/
static void cbd_eta(poly *r, const uint8_t *buf, unsigned int eta)
{
  unsigned int i;
  unsigned int j;
  size_t bit_offset;
  int16_t a;
  int16_t b;

  bit_offset = 0;
  for(i = 0; i < KYBER_N; i++) {
    a = 0;
    b = 0;

    for(j = 0; j < eta; j++) {
      a += (int16_t)get_bit(buf, bit_offset + j);
      b += (int16_t)get_bit(buf, bit_offset + eta + j);
    }

    r->coeffs[i] = a - b;
    bit_offset += (size_t)(2u * eta);
  }
}

void poly_cbd_eta1(poly *r, const uint8_t buf[KYBER_ETA1*KYBER_N/4])
{
  cbd_eta(r, buf, KYBER_ETA1);
}

void poly_cbd_eta2(poly *r, const uint8_t buf[KYBER_ETA2*KYBER_N/4])
{
  cbd_eta(r, buf, KYBER_ETA2);
}
