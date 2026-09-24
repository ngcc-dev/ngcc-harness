#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "params.h"
#include "poly.h"
#include "msgenc.h"
#if defined(WEAVER_USE_AVX_COMPRESS)
#include "poly_compress_avx.h"
#endif
#include "reduce.h"
#include "bch.h"

#if WEAVER_MODE == 1 || WEAVER_MODE == 3 || WEAVER_MODE == 5

/*************************************************
* Name:        flipabs
*
* Description: Computes |(x mod+ q/2) - q/4|
*
* Arguments:   uint16_t x: input coefficient
*
* Returns |(x mod+ q/2) - q/4|
**************************************************/
static uint16_t flipabs_ex(int16_t x)
{
    int16_t r, m;
    r = barrett_reduce_ex(x);

    r = r - WEAVER_Q / 4;
    m = r >> 15;
    return (r + m) ^ m; // turn to positive
}

// Algorithm 3: MsgEncode
/*************************************************
* Name:        poly_frommsg
* Description: Convert message to polynomial using WEAVER multi-level coding
**************************************************/
void poly_frommsg(poly *r, const uint8_t msg[WEAVER_INDCPA_MSGBYTES])
{
  unsigned int i, j;
  int16_t mask;
  uint8_t mu_tilde[WEAVER_N / 8] = { 0 };
  uint8_t mu_ddot_buf[LOW_CODEWORD_BYTES] = { 0 };

  for (i = 0; i < WEAVER_N; i++) {
      r->coeffs[i] = 0;
  }

  // ==========================================================
  // Step 1: Encode to Higher bits (MSB-first)
  // ==========================================================
#if WEAVER_MODE == 1 || WEAVER_MODE == 5
  memcpy(mu_tilde, msg, ELL_BAR_BYTES);
  encode_bch_high(msg, ELL_BAR_BYTES, mu_tilde + ELL_BAR_BYTES);

#elif WEAVER_MODE == 3
  /* mu_tilde = msg[0] || msg[1] || ... ||(msg[27] = 1111xxxx) */
  memcpy(mu_tilde, msg, 28);
  mu_tilde[27] &= 0xF0; // leave 4 bits empty (0)
  encode_bch_high_nibbles(mu_tilde, ELL_BAR_NIBBLES, mu_tilde + ELL_BAR_BYTES); // just fit in 32 Bytes

#endif

  for(i = 0; i < WEAVER_N/8; i++) {
    for(j = 0; j < 8; j++) {
      mask = -(int16_t)((mu_tilde[i] >> (7 - j)) & 1); // MSB-first
      //mask = -(int16_t)((mu_tilde[i] >> j) & 1);
      r->coeffs[8*i+j] = mask & WEAVER_HALFQ;
    }
  }

  // ==========================================================
  // Step 2: Encode to Lower bits
  // ==========================================================
#if WEAVER_MODE == 1 || WEAVER_MODE == 5
  memcpy(mu_ddot_buf, msg + ELL_BAR_BYTES, ELL_DDOT_BYTES);
  encode_bch_low(mu_ddot_buf, ELL_DDOT_BYTES, mu_ddot_buf + ELL_DDOT_BYTES);

#elif WEAVER_MODE == 3
  memcpy(mu_ddot_buf, msg + 28, 4);
  mu_ddot_buf[4] = (msg[27] << 4);
  encode_bch_low_nibbles(mu_ddot_buf, ELL_DDOT_NIBBLES, mu_ddot_buf + ELL_DDOT_BYTES);

#endif
  // D4 encoding
  for (i = 0; i < LOW_CODEWORD_BYTES; i++) {
      for (j = 0; j < 8; j++) {
          mask = -(int16_t)((mu_ddot_buf[i] >> (7 - j)) & 1);
          r->coeffs[8 * i + j + 0] = r->coeffs[8 * i + j + 0] + (mask & (WEAVER_Q / 4));
          r->coeffs[8 * i + j + D4_STEP_LEN] = r->coeffs[8 * i + j + D4_STEP_LEN] + (mask & (WEAVER_Q / 4));
          r->coeffs[8 * i + j + 2 * D4_STEP_LEN] = r->coeffs[8 * i + j + 2 * D4_STEP_LEN] + (mask & (WEAVER_Q / 4));
          r->coeffs[8 * i + j + 3 * D4_STEP_LEN] = r->coeffs[8 * i + j + 3 * D4_STEP_LEN] + (mask & (WEAVER_Q / 4));
      }
  }
}

// Algorithm 5: MsgDecode
void poly_tomsg(uint8_t msg[WEAVER_INDCPA_MSGBYTES], const poly *a)
{
  unsigned int i, j;
  int16_t w_bar[WEAVER_N];
  uint8_t mu_tilde[WEAVER_N / 8] = { 0 };
  uint8_t mu_ddot_noisy[LOW_CODEWORD_BYTES] = { 0 };
  uint8_t mu_ddot_clean[LOW_CODEWORD_BYTES] = { 0 };

  memset(msg, 0, WEAVER_INDCPA_MSGBYTES);

  for(i = 0; i < WEAVER_N; i++) { 
      int16_t t = a->coeffs[i];
      w_bar[i] = t + ( (t >> 15) & WEAVER_Q );
  }

  // ==========================================================
  // Phase 1: Decode Lower bits
  // ==========================================================
  for(i = 0; i < 8 * LOW_CODEWORD_BYTES; i++) {
    uint16_t ee = 0;
    ee =  flipabs_ex(w_bar[i + 0  ]);
    ee += flipabs_ex(w_bar[i + D4_STEP_LEN]);
    ee += flipabs_ex(w_bar[i + 2 * D4_STEP_LEN]);
    ee += flipabs_ex(w_bar[i + 3 * D4_STEP_LEN]);
    ee = (ee - WEAVER_HALFQ);
    ee >>= 15;
    mu_ddot_noisy[i>>3] |= ee << (7 - (i&7)); /* Here: we need bits to be packed continuously w/o interleaving 0s */
  }

  // ==========================================================
  // Phase 2: Cancel Interference from Lower bits
  // ==========================================================
#if WEAVER_MODE == 1 || WEAVER_MODE == 5
  decode_bch_low(mu_ddot_noisy, ELL_DDOT_BYTES, mu_ddot_noisy + ELL_DDOT_BYTES);
  memcpy(mu_ddot_clean, mu_ddot_noisy, ELL_DDOT_BYTES);
  encode_bch_low(mu_ddot_clean, ELL_DDOT_BYTES, mu_ddot_clean + ELL_DDOT_BYTES);

#elif WEAVER_MODE == 3
  decode_bch_low_nibbles(mu_ddot_noisy, ELL_DDOT_NIBBLES, mu_ddot_noisy + ELL_DDOT_BYTES);
  memcpy(mu_ddot_clean, mu_ddot_noisy, ELL_DDOT_BYTES);
  encode_bch_low_nibbles(mu_ddot_clean, ELL_DDOT_NIBBLES, mu_ddot_clean + ELL_DDOT_BYTES);

#endif

  for(i = 0; i < LOW_CODEWORD_BYTES; i++) {
    for(j = 0; j < 8; j++) {
        int16_t mask = -((mu_ddot_clean[i] >> (7 - j)) & 1); 
        w_bar[8*i + j + 0  ] -= (mask & (WEAVER_Q/4));
        w_bar[8*i + j + D4_STEP_LEN] -= (mask & (WEAVER_Q/4));
        w_bar[8*i + j + 2 * D4_STEP_LEN] -= (mask & (WEAVER_Q/4));
        w_bar[8*i + j + 3 * D4_STEP_LEN] -= (mask & (WEAVER_Q/4));
    }
  }

  // ==========================================================
  // Phase 3: Decode Higher bits
  // ==========================================================
  for(i = 0; i < WEAVER_N/8; i++) {
    for(j=0;j<8;j++) {
      int16_t t = w_bar[8*i+j];
      t += ((int16_t)t >> 15) & WEAVER_Q;  // map to positive
      t = ((((uint32_t)t << 1) + WEAVER_Q/2) / WEAVER_Q) & 1;
      mu_tilde[i] |= t << (7 - j); 
      //mu_tilde[i] |= t << j;
    }
  }

#if WEAVER_MODE == 1 || WEAVER_MODE == 5
  decode_bch_high(mu_tilde, ELL_BAR_BYTES, mu_tilde + ELL_BAR_BYTES);
  memcpy(msg, mu_tilde, ELL_BAR_BYTES);
  memcpy(msg + ELL_BAR_BYTES, mu_ddot_clean, ELL_DDOT_BYTES);

#elif WEAVER_MODE == 3
  // all high bits and 4 of low bits
  memcpy(msg, mu_tilde, 27);
  msg[27] = (mu_tilde[27] & 0xF0) | ((mu_ddot_clean[4] >> 4) & 0xF);
  // low bits
  memcpy(msg + ELL_BAR_BYTES, mu_ddot_clean, ELL_DDOT_BYTES - 1); /* mu_ddot_clean[0:3] */

#endif
}

#endif /* WEAVER_MODE == 1 || 3 || 5 */

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
    unsigned int i, j;
    int16_t u;
#if (WEAVER_DV <= 8)
    uint8_t t[8];
#else
    uint16_t t[8];
#endif

#if (WEAVER_DV == 5)
    for (i = 0; i < WEAVER_N / 8; i++) {
        for (j = 0; j < 8; j++) {
            // map to positive standard representatives
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
#elif (WEAVER_DV == 6)
    for (i = 0; i < WEAVER_N / 4; i++) {
        for (j = 0; j < 4; j++) {
            // map to positive standard representatives
            u = a->coeffs[4 * i + j];
            u += (u >> 15) & WEAVER_Q;
            t[j] = ((((uint32_t)u << 6) + WEAVER_Q / 2) / WEAVER_Q) & 63;
        }
        r[0] = (t[0] >> 0) | (t[1] << 6);
        r[1] = (t[1] >> 2) | (t[2] << 4);
        r[2] = (t[2] >> 4) | (t[3] << 2);
        r += 3;
    }
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
#elif (WEAVER_DV == 9)
#if defined(WEAVER_USE_AVX_COMPRESS)
  poly_compress_d9_avx(r, a);
#else
  for (i = 0; i < WEAVER_N / 8; i++) {
        for (j = 0; j < 8; j++) {
            u = a->coeffs[8 * i + j];
            u += (u >> 15) & WEAVER_Q;
            t[j] = ((((uint32_t)u << 9) + WEAVER_Q / 2) / WEAVER_Q) & 0x1ff;
        }

        r[0] = (uint8_t)(t[0] >> 0);
        r[1] = (uint8_t)((t[0] >> 8) | (t[1] << 1));
        r[2] = (uint8_t)((t[1] >> 7) | (t[2] << 2));
        r[3] = (uint8_t)((t[2] >> 6) | (t[3] << 3));
        r[4] = (uint8_t)((t[3] >> 5) | (t[4] << 4));
        r[5] = (uint8_t)((t[4] >> 4) | (t[5] << 5));
        r[6] = (uint8_t)((t[5] >> 3) | (t[6] << 6));
        r[7] = (uint8_t)((t[6] >> 2) | (t[7] << 7));
        r[8] = (uint8_t)(t[7] >> 1);
        r += 9;
    }
#endif
#else
#error "WEAVER_DV needs to be 5, 6, 8, or 9"
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

#if (WEAVER_DV == 5)
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
#elif (WEAVER_DV == 6)
    unsigned int j;
    uint8_t t[4];
    for (i = 0; i < WEAVER_N / 4; i++) {
        t[0] = (a[0] >> 0);
        t[1] = (a[0] >> 6) | (a[1] << 2);
        t[2] = (a[1] >> 4) | (a[2] << 4);
        t[3] = (a[2] >> 2);
        a += 3;

        for (j = 0; j < 4; j++)
            r->coeffs[4 * i + j] = ((uint32_t)(t[j] & 63)*WEAVER_Q + 32) >> 6;
    }
#elif (WEAVER_DV == 8)
#if defined(WEAVER_USE_AVX_COMPRESS)
  poly_decompress_d8_avx(r, a);
#else
    for (i = 0; i < WEAVER_N; i++)
        r->coeffs[i] = ((uint32_t)(*a++)*WEAVER_Q + 128) >> 8;
#endif
#elif (WEAVER_DV == 9)
#if defined(WEAVER_USE_AVX_COMPRESS)
  poly_decompress_d9_avx(r, a);
#else
    unsigned int j;
    uint16_t t[8];
    for (i = 0; i < WEAVER_N / 8; i++) {
        t[0] = (a[0] >> 0) | ((uint16_t)a[1] << 8);
        t[1] = (a[1] >> 1) | ((uint16_t)a[2] << 7);
        t[2] = (a[2] >> 2) | ((uint16_t)a[3] << 6);
        t[3] = (a[3] >> 3) | ((uint16_t)a[4] << 5);
        t[4] = (a[4] >> 4) | ((uint16_t)a[5] << 4);
        t[5] = (a[5] >> 5) | ((uint16_t)a[6] << 3);
        t[6] = (a[6] >> 6) | ((uint16_t)a[7] << 2);
        t[7] = (a[7] >> 7) | ((uint16_t)a[8] << 1);
        a += 9;

        for (j = 0; j < 8; j++)
            r->coeffs[8 * i + j] = ((uint32_t)(t[j] & 0x1ff)* WEAVER_Q + 256) >> 9;
    }
#endif
#else
#error "WEAVER_DV needs to be 5, 6, 8, or 9"
#endif
}