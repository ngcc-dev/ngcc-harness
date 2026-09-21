#include <stdint.h>
#include "params.h"
#include "poly.h"
#include "polyvec.h"

#ifdef PK_COMPRESS
/*************************************************
* Name:        polyvec_compress_pk
*
* Description: Compress and serialize vector of polynomials
*
* Arguments:   - uint8_t *r: pointer to output byte array
*                            (needs space for WEAVER_POLYVECCOMPRESSEDBYTES)
*              - polyvec *a: pointer to input vector of polynomials
**************************************************/
void polyvec_compress_pk(uint8_t r[WEAVER_PK_POLYVECBYTES], const polyvec *a)
{
    unsigned int i, j, k;
    int16_t u;

#if (WEAVER_PK_POLYVECBYTES == (WEAVER_K * WEAVER_N * 11 / 8))
  uint16_t t[8];
  for(i=0;i<WEAVER_K;i++) {
    for(j=0;j<WEAVER_N/8;j++) {
      for(k=0;k<8;k++) {
        u = a->vec[i].coeffs[8*j+k];
        u += ((int16_t)u >> 15) & WEAVER_Q;
        t[k] = ((((uint32_t)u << 11) + WEAVER_Q/2)/WEAVER_Q) & 0x7ff;
      }

      r[ 0] = (t[0] >>  0);
      r[ 1] = (t[0] >>  8) | (t[1] << 3);
      r[ 2] = (t[1] >>  5) | (t[2] << 6);
      r[ 3] = (t[2] >>  2);
      r[ 4] = (t[2] >> 10) | (t[3] << 1);
      r[ 5] = (t[3] >>  7) | (t[4] << 4);
      r[ 6] = (t[4] >>  4) | (t[5] << 7);
      r[ 7] = (t[5] >>  1);
      r[ 8] = (t[5] >>  9) | (t[6] << 2);
      r[ 9] = (t[6] >>  6) | (t[7] << 5);
      r[10] = (t[7] >>  3);
      r += 11;
    }
  }
#elif (WEAVER_PK_POLYVECBYTES == (WEAVER_K * WEAVER_N * 10 / 8))
  uint16_t t[4];
  for(i=0;i<WEAVER_K;i++) {
    for(j=0;j<WEAVER_N/4;j++) {
      for(k=0;k<4;k++){
        u = a->vec[i].coeffs[4*j+k];
        u += ((int16_t)u >> 15) & WEAVER_Q;
        t[k] = ((((uint32_t)u << 10) + WEAVER_Q/2)/ WEAVER_Q) & 0x3ff;
      }

      r[0] = (t[0] >> 0);
      r[1] = (t[0] >> 8) | (t[1] << 2);
      r[2] = (t[1] >> 6) | (t[2] << 4);
      r[3] = (t[2] >> 4) | (t[3] << 6);
      r[4] = (t[3] >> 2);
      r += 5;
    }
  }
#elif (WEAVER_PK_POLYVECBYTES == (WEAVER_K * WEAVER_N * 9 / 8))
  uint16_t t[8];
  for(i=0;i<WEAVER_K;i++) {
    for(j=0;j<WEAVER_N/8;j++) {
      for(k=0;k<8;k++) {
        u = a->vec[i].coeffs[8*j+k];
        u += ((int16_t)u >> 15) & WEAVER_Q;
        t[k] = ((((uint32_t)u << 9) + WEAVER_Q/2)/WEAVER_Q) & 0x1ff;
      }

      r[0] = (t[0] >> 0);
      r[1] = (t[0] >> 8) | (t[1] << 1);
      r[2] = (t[1] >> 7) | (t[2] << 2);
      r[3] = (t[2] >> 6) | (t[3] << 3);
      r[4] = (t[3] >> 5) | (t[4] << 4);
      r[5] = (t[4] >> 4) | (t[5] << 5);
      r[6] = (t[5] >> 3) | (t[6] << 6);
      r[7] = (t[6] >> 2) | (t[7] << 7);
      r[8] = (t[7] >> 1);
      r += 9;
    }
  }
#elif (WEAVER_PK_POLYVECBYTES == (WEAVER_K * WEAVER_N * 8 / 8))
  for(i=0;i<WEAVER_K;i++) {
    for(j=0;j<WEAVER_N;j++) {
      u = a->vec[i].coeffs[j];
      u += ((int16_t)u >> 15) & WEAVER_Q;
      *r++ = ((((uint32_t)u << 8) + WEAVER_Q/2) / WEAVER_Q) & 0xff;
    }
  }
#else
#error "WEAVER_PK_POLYVECBYTES needs to be K*N*8/8, K*N*9/8, K*N*10/8, or K*N*11/8"
#endif
}

/*************************************************
* Name:        polyvec_decompress_pk
*
* Description: De-serialize and decompress vector of polynomials;
*              approximate inverse of polyvec_compress
*
* Arguments:   - polyvec *r:       pointer to output vector of polynomials
*              - const uint8_t *a: pointer to input byte array
*                                  (of length WEAVER_POLYVECCOMPRESSEDBYTES)
**************************************************/
void polyvec_decompress_pk(polyvec *r, const uint8_t a[WEAVER_PK_POLYVECBYTES])
{
    unsigned int i, j, k;

#if (WEAVER_PK_POLYVECBYTES == (WEAVER_K * WEAVER_N * 11 / 8))
  uint16_t t[8];
  for(i=0;i<WEAVER_K;i++) {
    for(j=0;j<WEAVER_N/8;j++) {
      t[0] = (a[0] >> 0) | ((uint16_t)a[ 1] << 8);
      t[1] = (a[1] >> 3) | ((uint16_t)a[ 2] << 5);
      t[2] = (a[2] >> 6) | ((uint16_t)a[ 3] << 2) | ((uint16_t)a[4] << 10);
      t[3] = (a[4] >> 1) | ((uint16_t)a[ 5] << 7);
      t[4] = (a[5] >> 4) | ((uint16_t)a[ 6] << 4);
      t[5] = (a[6] >> 7) | ((uint16_t)a[ 7] << 1) | ((uint16_t)a[8] << 9);
      t[6] = (a[8] >> 2) | ((uint16_t)a[ 9] << 6);
      t[7] = (a[9] >> 5) | ((uint16_t)a[10] << 3);
      a += 11;

      for(k=0;k<8;k++)
        r->vec[i].coeffs[8*j+k] = ((uint32_t)(t[k] & 0x7FF)*WEAVER_Q + 1024) >> 11;
    }
  }
#elif (WEAVER_PK_POLYVECBYTES == (WEAVER_K * WEAVER_N * 10 / 8))
  uint16_t t[4];
  for(i=0;i<WEAVER_K;i++) {
    for(j=0;j<WEAVER_N/4;j++) {
      t[0] = (a[0] >> 0) | ((uint16_t)a[1] << 8);
      t[1] = (a[1] >> 2) | ((uint16_t)a[2] << 6);
      t[2] = (a[2] >> 4) | ((uint16_t)a[3] << 4);
      t[3] = (a[3] >> 6) | ((uint16_t)a[4] << 2);
      a += 5;

      for(k=0;k<4;k++)
        r->vec[i].coeffs[4*j+k] = ((uint32_t)(t[k] & 0x3FF)*WEAVER_Q + 512) >> 10;
    }
  }
#elif (WEAVER_PK_POLYVECBYTES == (WEAVER_K * WEAVER_N * 9 / 8))
  uint16_t t[8];
  for(i=0;i<WEAVER_K;i++) {
    for(j=0;j<WEAVER_N/8;j++) {
      t[0] = (a[0] >> 0) | ((uint16_t)a[1] << 8);
      t[1] = (a[1] >> 1) | ((uint16_t)a[2] << 7);
      t[2] = (a[2] >> 2) | ((uint16_t)a[3] << 6);
      t[3] = (a[3] >> 3) | ((uint16_t)a[4] << 5);
      t[4] = (a[4] >> 4) | ((uint16_t)a[5] << 4);
      t[5] = (a[5] >> 5) | ((uint16_t)a[6] << 3);
      t[6] = (a[6] >> 6) | ((uint16_t)a[7] << 2);
      t[7] = (a[7] >> 7) | ((uint16_t)a[8] << 1);
      a += 9;

      for(k=0;k<8;k++)
        r->vec[i].coeffs[8*j+k] = ((uint32_t)(t[k] & 0x1FF)*WEAVER_Q + 256) >> 9;
    }
  }
#elif (WEAVER_PK_POLYVECBYTES == (WEAVER_K * WEAVER_N * 8 / 8))
  for(i=0;i<WEAVER_K;i++) {
    for(j=0;j<WEAVER_N;j++) {
      r->vec[i].coeffs[j] = ((uint32_t)(*a++)*WEAVER_Q + 128) >> 8;
    }
  }
#else
#error "WEAVER_PK_POLYVECBYTES needs to be K*N*8/8, K*N*9/8, K*N*10/8, or K*N*11/8"
#endif
}

/*************************************************
* Name:        polyvec_fromcompressed_pk
*
* Description: Load compressed public key bytes
*              into polyvec coefficients WITHOUT
*              decompression (no multiply-by-Q).
*              Output range is [0, 2^d_t - 1],
*              suitable as Inv_q bucket indices.
*
* Arguments:   - polyvec *r:       pointer to output polyvec
*              - const uint8_t *a: pointer to input compressed PK bytes
**************************************************/
void polyvec_fromcompressed_pk(polyvec *r,
                               const uint8_t a[WEAVER_PK_POLYVECBYTES])
{
    unsigned int i, j, k;

#if (WEAVER_PK_POLYVECBYTES == (WEAVER_K * WEAVER_N * 11 / 8))
  uint16_t t[8];
  for(i=0;i<WEAVER_K;i++) {
    for(j=0;j<WEAVER_N/8;j++) {
      t[0] = (a[0] >> 0) | ((uint16_t)a[ 1] << 8);
      t[1] = (a[1] >> 3) | ((uint16_t)a[ 2] << 5);
      t[2] = (a[2] >> 6) | ((uint16_t)a[ 3] << 2) | ((uint16_t)a[4] << 10);
      t[3] = (a[4] >> 1) | ((uint16_t)a[ 5] << 7);
      t[4] = (a[5] >> 4) | ((uint16_t)a[ 6] << 4);
      t[5] = (a[6] >> 7) | ((uint16_t)a[ 7] << 1) | ((uint16_t)a[8] << 9);
      t[6] = (a[8] >> 2) | ((uint16_t)a[ 9] << 6);
      t[7] = (a[9] >> 5) | ((uint16_t)a[10] << 3);
      a += 11;
      for(k=0;k<8;k++)
        r->vec[i].coeffs[8*j+k] = (int16_t)(t[k] & 0x7FF);
    }
  }
#elif (WEAVER_PK_POLYVECBYTES == (WEAVER_K * WEAVER_N * 10 / 8))
  uint16_t t[4];
  for(i=0;i<WEAVER_K;i++) {
    for(j=0;j<WEAVER_N/4;j++) {
      t[0] = (a[0] >> 0) | ((uint16_t)a[1] << 8);
      t[1] = (a[1] >> 2) | ((uint16_t)a[2] << 6);
      t[2] = (a[2] >> 4) | ((uint16_t)a[3] << 4);
      t[3] = (a[3] >> 6) | ((uint16_t)a[4] << 2);
      a += 5;
      for(k=0;k<4;k++)
        r->vec[i].coeffs[4*j+k] = (int16_t)(t[k] & 0x3FF);   /* 不乘Q！ */
    }
  }
#elif (WEAVER_PK_POLYVECBYTES == (WEAVER_K * WEAVER_N * 9 / 8))
  uint16_t t[8];
  for(i=0;i<WEAVER_K;i++) {
    for(j=0;j<WEAVER_N/8;j++) {
      t[0] = (a[0] >> 0) | ((uint16_t)a[1] << 8);
      t[1] = (a[1] >> 1) | ((uint16_t)a[2] << 7);
      t[2] = (a[2] >> 2) | ((uint16_t)a[3] << 6);
      t[3] = (a[3] >> 3) | ((uint16_t)a[4] << 5);
      t[4] = (a[4] >> 4) | ((uint16_t)a[5] << 4);
      t[5] = (a[5] >> 5) | ((uint16_t)a[6] << 3);
      t[6] = (a[6] >> 6) | ((uint16_t)a[7] << 2);
      t[7] = (a[7] >> 7) | ((uint16_t)a[8] << 1);
      a += 9;
      for(k=0;k<8;k++)
        r->vec[i].coeffs[8*j+k] = (int16_t)(t[k] & 0x1FF);   /* 不乘Q！ */
    }
  }
#elif (WEAVER_PK_POLYVECBYTES == (WEAVER_K * WEAVER_N * 8 / 8))
  for(i=0;i<WEAVER_K;i++) {
    for(j=0;j<WEAVER_N;j++) {
      r->vec[i].coeffs[j] = (int16_t)(*a++);                  /* 纯加载，零运算 */
    }
  }
#else
#error "WEAVER_PK_POLYVECBYTES needs to be K*N*8/8, K*N*9/8, K*N*10/8, or K*N*11/8"
#endif
}
#endif // PK_COMPRESS

/*************************************************
* Name:        polyvec_compress
*
* Description: Compress and serialize vector of polynomials
*
* Arguments:   - uint8_t *r: pointer to output byte array
*                            (needs space for WEAVER_POLYVECCOMPRESSEDBYTES)
*              - const polyvec *a: pointer to input vector of polynomials
**************************************************/
void polyvec_compress(uint8_t r[WEAVER_POLYVECCOMPRESSEDBYTES], const polyvec *a)
{
  unsigned int i,j,k;
  int16_t u;

#if (WEAVER_POLYVECCOMPRESSEDBYTES == (WEAVER_K * WEAVER_N * 11 / 8))
  uint16_t t[8];
  for(i=0;i<WEAVER_K;i++) {
    for(j=0;j<WEAVER_N/8;j++) {
      for(k=0;k<8;k++) {
        u = a->vec[i].coeffs[8*j+k];
        u += ((int16_t)u >> 15) & WEAVER_Q;
        t[k] = ((((uint32_t)u << 11) + WEAVER_Q/2)/WEAVER_Q) & 0x7ff;
      }

      r[ 0] = (t[0] >>  0);
      r[ 1] = (t[0] >>  8) | (t[1] << 3);
      r[ 2] = (t[1] >>  5) | (t[2] << 6);
      r[ 3] = (t[2] >>  2);
      r[ 4] = (t[2] >> 10) | (t[3] << 1);
      r[ 5] = (t[3] >>  7) | (t[4] << 4);
      r[ 6] = (t[4] >>  4) | (t[5] << 7);
      r[ 7] = (t[5] >>  1);
      r[ 8] = (t[5] >>  9) | (t[6] << 2);
      r[ 9] = (t[6] >>  6) | (t[7] << 5);
      r[10] = (t[7] >>  3);
      r += 11;
    }
  }
#elif (WEAVER_POLYVECCOMPRESSEDBYTES == (WEAVER_K * WEAVER_N * 10 / 8))
  uint16_t t[4];
  for(i=0;i<WEAVER_K;i++) {
    for(j=0;j<WEAVER_N/4;j++) {
      for(k=0;k<4;k++) {
        u = a->vec[i].coeffs[4*j+k];
        u += ((int16_t)u >> 15) & WEAVER_Q;
        t[k] = ((((uint32_t)u << 10) + WEAVER_Q/2)/ WEAVER_Q) & 0x3ff;
      }

      r[0] = (t[0] >> 0);
      r[1] = (t[0] >> 8) | (t[1] << 2);
      r[2] = (t[1] >> 6) | (t[2] << 4);
      r[3] = (t[2] >> 4) | (t[3] << 6);
      r[4] = (t[3] >> 2);
      r += 5;
    }
  }
#elif (WEAVER_POLYVECCOMPRESSEDBYTES == (WEAVER_K * WEAVER_N * 9 / 8))
  uint16_t t[8];
  for(i=0;i<WEAVER_K;i++) {
    for(j=0;j<WEAVER_N/8;j++) {
      for(k=0;k<8;k++) {
        u = a->vec[i].coeffs[8*j+k];
        u += ((int16_t)u >> 15) & WEAVER_Q;
        t[k] = ((((uint32_t)u << 9) + WEAVER_Q/2) /WEAVER_Q) & 0x1ff;
      }

      r[0] = (t[0] >> 0);
      r[1] = (t[0] >> 8) | (t[1] << 1);
      r[2] = (t[1] >> 7) | (t[2] << 2);
      r[3] = (t[2] >> 6) | (t[3] << 3);
      r[4] = (t[3] >> 5) | (t[4] << 4);
      r[5] = (t[4] >> 4) | (t[5] << 5);
      r[6] = (t[5] >> 3) | (t[6] << 6);
      r[7] = (t[6] >> 2) | (t[7] << 7);
      r[8] = (t[7] >> 1);
      r += 9;
    }
  }
#elif (WEAVER_POLYVECCOMPRESSEDBYTES == (WEAVER_K * WEAVER_N * 8 / 8))
  for(i=0;i<WEAVER_K;i++) {
    for(j=0;j<WEAVER_N;j++) {
      u = a->vec[i].coeffs[j];
      u += ((int16_t)u >> 15) & WEAVER_Q;
      *r++ = ((((uint32_t)u << 8) + WEAVER_Q/2) / WEAVER_Q) & 0xff;
    }
  }
#else
#error "WEAVER_POLYVECCOMPRESSEDBYTES needs to be K*N*8/8, K*N*9/8, K*N*10/8, or K*N*11/8"
#endif
}

/*************************************************
* Name:        polyvec_decompress
*
* Description: De-serialize and decompress vector of polynomials;
*              approximate inverse of polyvec_compress
*
* Arguments:   - polyvec *r:       pointer to output vector of polynomials
*              - const uint8_t *a: pointer to input byte array
*                                  (of length WEAVER_POLYVECCOMPRESSEDBYTES)
**************************************************/
void polyvec_decompress(polyvec *r, const uint8_t a[WEAVER_POLYVECCOMPRESSEDBYTES])
{
  unsigned int i,j,k;

#if (WEAVER_POLYVECCOMPRESSEDBYTES == (WEAVER_K * WEAVER_N * 11 / 8))
  uint16_t t[8];
  for(i=0;i<WEAVER_K;i++) {
    for(j=0;j<WEAVER_N/8;j++) {
      t[0] = (a[0] >> 0) | ((uint16_t)a[ 1] << 8);
      t[1] = (a[1] >> 3) | ((uint16_t)a[ 2] << 5);
      t[2] = (a[2] >> 6) | ((uint16_t)a[ 3] << 2) | ((uint16_t)a[4] << 10);
      t[3] = (a[4] >> 1) | ((uint16_t)a[ 5] << 7);
      t[4] = (a[5] >> 4) | ((uint16_t)a[ 6] << 4);
      t[5] = (a[6] >> 7) | ((uint16_t)a[ 7] << 1) | ((uint16_t)a[8] << 9);
      t[6] = (a[8] >> 2) | ((uint16_t)a[ 9] << 6);
      t[7] = (a[9] >> 5) | ((uint16_t)a[10] << 3);
      a += 11;

      for(k=0;k<8;k++)
        r->vec[i].coeffs[8*j+k] = ((uint32_t)(t[k] & 0x7FF)*WEAVER_Q + 1024) >> 11;
    }
  }
#elif (WEAVER_POLYVECCOMPRESSEDBYTES == (WEAVER_K * WEAVER_N * 10 / 8))
  uint16_t t[4];
  for(i=0;i<WEAVER_K;i++) {
    for(j=0;j<WEAVER_N/4;j++) {
      t[0] = (a[0] >> 0) | ((uint16_t)a[1] << 8);
      t[1] = (a[1] >> 2) | ((uint16_t)a[2] << 6);
      t[2] = (a[2] >> 4) | ((uint16_t)a[3] << 4);
      t[3] = (a[3] >> 6) | ((uint16_t)a[4] << 2);
      a += 5;

      for(k=0;k<4;k++)
        r->vec[i].coeffs[4*j+k] = ((uint32_t)(t[k] & 0x3FF)*WEAVER_Q + 512) >> 10;
    }
  }
#elif (WEAVER_POLYVECCOMPRESSEDBYTES == (WEAVER_K * WEAVER_N * 9 / 8))
  uint16_t t[8];
  for(i=0;i<WEAVER_K;i++) {
    for(j=0;j<WEAVER_N/8;j++) {
      t[0] = (a[0] >> 0) | ((uint16_t)a[1] << 8);
      t[1] = (a[1] >> 1) | ((uint16_t)a[2] << 7);
      t[2] = (a[2] >> 2) | ((uint16_t)a[3] << 6);
      t[3] = (a[3] >> 3) | ((uint16_t)a[4] << 5);
      t[4] = (a[4] >> 4) | ((uint16_t)a[5] << 4);
      t[5] = (a[5] >> 5) | ((uint16_t)a[6] << 3);
      t[6] = (a[6] >> 6) | ((uint16_t)a[7] << 2);
      t[7] = (a[7] >> 7) | ((uint16_t)a[8] << 1);
      a += 9;

      for(k=0;k<8;k++)
        r->vec[i].coeffs[8*j+k] = ((uint32_t)(t[k] & 0x1FF)*WEAVER_Q + 256) >> 9;
    }
  }
#elif (WEAVER_POLYVECCOMPRESSEDBYTES == (WEAVER_K * WEAVER_N * 8 / 8))
  for(i=0;i<WEAVER_K;i++) {
    for(j=0;j<WEAVER_N;j++) {
      r->vec[i].coeffs[j] = ((uint32_t)(*a++)*WEAVER_Q + 128) >> 8;
    }
  }
#else
#error "WEAVER_POLYVECCOMPRESSEDBYTES needs to be K*N*8/8, K*N*9/8, K*N*10/8, or K*N*11/8"
#endif
}

/*************************************************
* Name:        polyvec_tobytes
*
* Description: Serialize vector of polynomials
*
* Arguments:   - uint8_t *r: pointer to output byte array
*                            (needs space for WEAVER_POLYVECBYTES)
*              - const polyvec *a: pointer to input vector of polynomials
**************************************************/
void polyvec_tobytes(uint8_t r[WEAVER_POLYVECBYTES], const polyvec *a)
{
  unsigned int i;
  for(i=0;i<WEAVER_K;i++)
    poly_tobytes(r+i*WEAVER_POLYBYTES, &a->vec[i]);
}

/*************************************************
* Name:        polyvec_frombytes
*
* Description: De-serialize vector of polynomials;
*              inverse of polyvec_tobytes
*
* Arguments:   - uint8_t *r:       pointer to output byte array
*              - const polyvec *a: pointer to input vector of polynomials
*                                  (of length WEAVER_POLYVECBYTES)
**************************************************/
void polyvec_frombytes(polyvec *r, const uint8_t a[WEAVER_POLYVECBYTES])
{
  unsigned int i;
  for(i=0;i<WEAVER_K;i++)
    poly_frombytes(&r->vec[i], a+i*WEAVER_POLYBYTES);
}

/*************************************************
* Name:        polyvec_ntt
*
* Description: Apply forward NTT to all elements of a vector of polynomials
*
* Arguments:   - polyvec *r: pointer to in/output vector of polynomials
**************************************************/
void polyvec_ntt(polyvec *r)
{
  unsigned int i;
  for(i=0;i<WEAVER_K;i++)
    poly_ntt(&r->vec[i]);
}

/*************************************************
* Name:        polyvec_invntt_tomont
*
* Description: Apply inverse NTT to all elements of a vector of polynomials
*              and multiply by Montgomery factor 2^16
*
* Arguments:   - polyvec *r: pointer to in/output vector of polynomials
**************************************************/
void polyvec_invntt_tomont(polyvec *r)
{
  unsigned int i;
  for(i=0;i<WEAVER_K;i++)
    poly_invntt_tomont(&r->vec[i]);
}

/*************************************************
* Name:        polyvec_basemul_acc_montgomery
*
* Description: Multiply elements of a and b in NTT domain, accumulate into r,
*              and multiply by 2^-16.
*
* Arguments: - poly *r: pointer to output polynomial
*            - const polyvec *a: pointer to first input vector of polynomials
*            - const polyvec *b: pointer to second input vector of polynomials
**************************************************/
void polyvec_basemul_acc_montgomery(poly *r, const polyvec *a, const polyvec *b)
{
  unsigned int i;
  poly t;

  poly_basemul_montgomery(r, &a->vec[0], &b->vec[0]);
  for(i=1;i<WEAVER_K;i++) {
    poly_basemul_montgomery(&t, &a->vec[i], &b->vec[i]);
    poly_add(r, r, &t);
  }

  poly_reduce(r);
}

/*************************************************
* Name:        polyvec_reduce
*
* Description: Applies Barrett reduction to each coefficient
*              of each element of a vector of polynomials;
*              for details of the Barrett reduction see comments in reduce.c
*
* Arguments:   - polyvec *r: pointer to input/output polynomial
**************************************************/
void polyvec_reduce(polyvec *r)
{
  unsigned int i;
  for(i=0;i<WEAVER_K;i++)
    poly_reduce(&r->vec[i]);
}

/*************************************************
* Name:        polyvec_add
*
* Description: Add vectors of polynomials
*
* Arguments: - polyvec *r:       pointer to output vector of polynomials
*            - const polyvec *a: pointer to first input vector of polynomials
*            - const polyvec *b: pointer to second input vector of polynomials
**************************************************/
void polyvec_add(polyvec *r, const polyvec *a, const polyvec *b)
{
  unsigned int i;
  for(i=0;i<WEAVER_K;i++)
    poly_add(&r->vec[i], &a->vec[i], &b->vec[i]);
}
