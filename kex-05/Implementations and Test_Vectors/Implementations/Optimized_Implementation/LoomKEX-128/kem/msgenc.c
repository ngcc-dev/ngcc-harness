#include <stdint.h>
#include "params.h"
#include "poly.h"
#include "msgenc.h"
#if defined(WEAVER_USE_AVX_COMPRESS)
#include "poly_compress_avx.h"
#endif

/*************************************************
* Name:        poly_frommsg
*
* Description: Convert message bytes to polynomial.
*
* Arguments:   - poly *r:            pointer to output polynomial
*              - const uint8_t *msg: pointer to input message
**************************************************/
void poly_frommsg(poly *r, const uint8_t msg[WEAVER_INDCPA_MSGBYTES])
{
  unsigned int i,j;
  int16_t mask;

#if (WEAVER_INDCPA_MSGBYTES > WEAVER_N/8)
#error "WEAVER_INDCPA_MSGBYTES must be less than WEAVER_N/8 bytes!"
#endif

  for(i=0;i<WEAVER_N/8;i++) {
    for(j=0;j<8;j++) {
      mask = -(int16_t)((msg[i] >> j)&1);
      r->coeffs[8*i+j] = mask & ((WEAVER_Q+1)/2);
    }
  }
}

/*************************************************
* Name:        poly_tomsg
*
* Description: Convert polynomial to message bytes.
*
* Arguments:   - uint8_t *msg:       pointer to output message
*              - const poly *a:      pointer to input polynomial
**************************************************/
void poly_tomsg(uint8_t msg[WEAVER_INDCPA_MSGBYTES], const poly *a)
{
  unsigned int i,j;
  uint16_t t;

  for(i=0;i<WEAVER_N/8;i++) {
    msg[i] = 0;
    for(j=0;j<8;j++) {
      t  = a->coeffs[8*i+j];
      t += ((int16_t)t >> 15) & WEAVER_Q;
      t  = (((t << 1) + WEAVER_Q/2)/WEAVER_Q) & 1;
      msg[i] |= t << j;
    }
  }
}

/*************************************************
* Name:        poly_compress
*
* Description: Compression and subsequent serialization of a polynomial
*
* Arguments:   - uint8_t *r: pointer to output byte array
*                            (of length WEAVER_POLYCOMPRESSEDBYTES)
*              - const poly *a: pointer to input polynomial
**************************************************/
void poly_compress(uint8_t r[WEAVER_POLYCOMPRESSEDBYTES], const poly *a)
{
    unsigned int i;
    int16_t u;

#if (WEAVER_DV == 4)
#if defined(WEAVER_USE_AVX_COMPRESS)
    poly_compress_d4_avx(r, a);
#else
    uint8_t t0, t1;
    for (i = 0; i < WEAVER_N / 2; i++) {
        u = a->coeffs[2 * i];
        u += (u >> 15) & WEAVER_Q;
        t0 = ((((uint32_t)u << 4) + WEAVER_Q / 2) / WEAVER_Q) & 15;

        u = a->coeffs[2 * i + 1];
        u += (u >> 15) & WEAVER_Q;
        t1 = ((((uint32_t)u << 4) + WEAVER_Q / 2) / WEAVER_Q) & 15;

        r[i] = t0 | (t1 << 4);
    }
#endif
#elif (WEAVER_DV == 5)
#if defined(WEAVER_USE_AVX_COMPRESS)
    poly_compress_d5_avx(r, a);
#else
    {
        unsigned int j;
        uint8_t t[8];
        for (i = 0; i < WEAVER_N / 8; i++) {
            for (j = 0; j < 8; j++) {
                u = a->coeffs[8 * i + j];
                u += (u >> 15) & WEAVER_Q;
                t[j] = ((((uint32_t)u << 5) + WEAVER_Q / 2) / WEAVER_Q) & 31;
            }

            r[0] = (t[0] >> 0) | (t[1] << 5);
            r[1] = (t[1] >> 3) | (t[2] << 2) | (t[3] << 7);
            r[2] = (t[3] >> 1) | (t[4] << 4);
            r[3] = (t[4] >> 4) | (t[5] << 1) | (t[6] << 6);
            r[4] = (t[6] >> 2) | (t[7] << 3);
            r += 5;
        }
    }
#endif
#elif (WEAVER_DV == 6)
#if defined(WEAVER_USE_AVX_COMPRESS)
    poly_compress_d6_avx(r, a);
#else
    {
        unsigned int j;
        uint8_t t[4];
        for (i = 0; i < WEAVER_N / 4; i++) {
            for (j = 0; j < 4; j++) {
                u = a->coeffs[4 * i + j];
                u += (u >> 15) & WEAVER_Q;
                t[j] = ((((uint32_t)u << 6) + WEAVER_Q / 2) / WEAVER_Q) & 63;
            }
            r[0] = (t[0] >> 0) | (t[1] << 6);
            r[1] = (t[1] >> 2) | (t[2] << 4);
            r[2] = (t[2] >> 4) | (t[3] << 2);
            r += 3;
        }
    }
#endif
#elif (WEAVER_DV == 8)
#if defined(WEAVER_USE_AVX_COMPRESS)
    poly_compress_d8_avx(r, a);
#else
    for (i = 0; i < WEAVER_N; i++) {
        u = a->coeffs[i];
        u += (u >> 15) & WEAVER_Q;
        *r++ = ((((uint32_t)u << 8) + WEAVER_Q / 2) / WEAVER_Q) & 0xff;
    }
#endif
#else
#error "LOOM_KEM_DV must be 4, 5, 6, or 8"
#endif
}

/*************************************************
* Name:        poly_decompress
*
* Description: De-serialization and subsequent decompression of a polynomial;
*              approximate inverse of poly_compress
*
* Arguments:   - poly *r:          pointer to output polynomial
*              - const uint8_t *a: pointer to input byte array
*                                  (of length WEAVER_POLYCOMPRESSEDBYTES bytes)
**************************************************/
void poly_decompress(poly *r, const uint8_t a[WEAVER_POLYCOMPRESSEDBYTES])
{
    unsigned int i;

#if (WEAVER_DV == 4)
#if defined(WEAVER_USE_AVX_COMPRESS)
    poly_decompress_d4_avx(r, a);
#else
    for (i = 0; i < WEAVER_N; i++)
        r->coeffs[i] = 0;

    for (i = 0; i < WEAVER_N / 2; i++) {
        r->coeffs[2 * i]     = (((uint32_t)(a[i] & 15) * WEAVER_Q + 8) >> 4);
        r->coeffs[2 * i + 1] = (((uint32_t)(a[i] >> 4) * WEAVER_Q + 8) >> 4);
    }
#endif
#elif (WEAVER_DV == 5)
#if defined(WEAVER_USE_AVX_COMPRESS)
    poly_decompress_d5_avx(r, a);
#else
    unsigned int j;
    uint8_t t[8];
    for (i = 0; i < WEAVER_N / 8; i++) {
        t[0] = (a[0] >> 0);
        t[1] = (a[0] >> 5) | (a[1] << 3);
        t[2] = (a[1] >> 2);
        t[3] = (a[1] >> 7) | (a[2] << 1);
        t[4] = (a[2] >> 4) | (a[3] << 4);
        t[5] = (a[3] >> 1);
        t[6] = (a[3] >> 6) | (a[4] << 2);
        t[7] = (a[4] >> 3);
        a += 5;

        for (j = 0; j < 8; j++)
            r->coeffs[8 * i + j] = ((uint32_t)(t[j] & 31)*WEAVER_Q + 16) >> 5;
    }
    for (i = WEAVER_N / 8 * 8; i < WEAVER_N; i++)
        r->coeffs[i] = 0;
#endif
#elif (WEAVER_DV == 6)
#if defined(WEAVER_USE_AVX_COMPRESS)
    poly_decompress_d6_avx(r, a);
#else
    {
        unsigned int j;
        uint8_t t[4];
        for (i = 0; i < WEAVER_N / 4; i++) {
            t[0] = (a[0] >> 0);
            t[1] = (a[0] >> 6) | (a[1] << 2);
            t[2] = (a[1] >> 4) | (a[2] << 4);
            t[3] = (a[2] >> 2);
            a += 3;

            for (j = 0; j < 4; j++)
                r->coeffs[4 * i + j] = ((uint32_t)(t[j] & 63) * WEAVER_Q + 32) >> 6;
        }
    }
#endif
#elif (WEAVER_DV == 8)
#if defined(WEAVER_USE_AVX_COMPRESS)
    poly_decompress_d8_avx(r, a);
#else
    for (i = 0; i < WEAVER_N; i++)
        r->coeffs[i] = ((uint32_t)(*a++) * WEAVER_Q + 128) >> 8;
#endif
#else
#error "LOOM_KEM_DV must be 4, 5, 6, or 8"
#endif
}
