#include <stdint.h>
#include "params.h"
#include "poly.h"
#include "ntt.h"
#include "reduce.h"
#include "cbd.h"
#include "symmetric.h"
#include "verify.h"

/*************************************************
* Name:        poly_compress
*
* Description: Compression and subsequent serialization of a polynomial
*
* Arguments:   - uint8_t *r: pointer to output byte array
*                            (of length COMPASS_KEM_POLYCOMPRESSEDBYTES)
*              - const poly *a: pointer to input polynomial
**************************************************/

void poly_compress(uint8_t *r, const poly *a, int8_t d) {
  unsigned int i, j;
  int16_t u;       // Must be signed: arithmetic right-shift produces 0xFFFF (-1)
  uint16_t d_val;  // Used for subsequent unsigned shift packing

#if (COMPASS_KEM_Q == 3329)
  // --- 128/256-bit security level (q=3329) ---
  if (d == 2) {
    // Compress to 10 bits
    uint16_t t[4];
    for(i=0; i<COMPASS_KEM_N/4; i++) {
      for(j=0; j<4; j++) {
        u = a->coeffs[4*i+j];
        // Arithmetic right shift: if negative, u>>15 = 0xFFFF, AND with q adds q
        u += (u >> 15) & COMPASS_KEM_Q;
        d_val = u;
        d_val -= (d_val != 0);
        t[j] = d_val >> 2; 
      }
      r[0] = t[0] & 0xFF;
      r[1] = (t[0] >> 8) | ((t[1] & 0x3F) << 2);
      r[2] = (t[1] >> 6) | ((t[2] & 0x0F) << 4);
      r[3] = (t[2] >> 4) | ((t[3] & 0x03) << 6);
      r[4] = (t[3] >> 2);
      r += 5;
    }
  } 
  else if (d == 6) {
    // Compress to 6 bits
    uint8_t t[4];
    for(i=0; i<COMPASS_KEM_N/4; i++) {
      for(j=0; j<4; j++) {
        u = a->coeffs[4*i+j];
        u += (u >> 15) & COMPASS_KEM_Q;
        d_val = u;
        d_val -= (d_val != 0);
        t[j] = d_val >> 6;
      }
      r[0] = t[0] | (t[1] << 6);
      r[1] = (t[1] >> 2) | (t[2] << 4);
      r[2] = (t[2] >> 4) | (t[3] << 2);
      r += 3;
    }
  }
  else if (d == 8) {
    // [Mod]: Drop 8 bits, keep 4 bits (12 - 8 = 4)
    uint8_t t[2];
    for(i=0; i<COMPASS_KEM_N/2; i++) {
      for(j=0; j<2; j++) {
        u = a->coeffs[2*i+j];
        u += (u >> 15) & COMPASS_KEM_Q;
        d_val = u;
        d_val -= (d_val != 0);
        t[j] = d_val >> 8; // Shift right 8 bits, keep 4 bits
      }
      // Two 4-bit coefficients packed into 1 byte
      r[i] = t[0] | (t[1] << 4); 
    }
  }

#elif (COMPASS_KEM_Q == 7681)
  // --- 384/512-bit security level (q=7681) ---
  if (d == 2) {
    // Compress to 11 bits
    uint16_t t[8];
    for(i=0; i<COMPASS_KEM_N/8; i++) {
      for(j=0; j<8; j++) {
        u = a->coeffs[8*i+j];
        u += (u >> 15) & COMPASS_KEM_Q;
        d_val = u;
        d_val -= (d_val != 0);
        t[j] = d_val >> 2;
      }
      r[0] = t[0] & 0xFF;
      r[1] = (t[0] >> 8) | ((t[1] & 0x1F) << 3);
      r[2] = (t[1] >> 5) | ((t[2] & 0x03) << 6);
      r[3] = (t[2] >> 2) & 0xFF;
      r[4] = (t[2] >> 10) | ((t[3] & 0x7F) << 1);
      r[5] = (t[3] >> 7) | ((t[4] & 0x0F) << 4);
      r[6] = (t[4] >> 4) | ((t[5] & 0x01) << 7);
      r[7] = (t[5] >> 1) & 0xFF;
      r[8] = (t[5] >> 9) | ((t[6] & 0x3F) << 2);
      r[9] = (t[6] >> 6) | ((t[7] & 0x07) << 5);
      r[10] = (t[7] >> 3);
      r += 11;
    }
  } 
  else if (d == 6) {
    // Compress to 7 bits
    uint8_t t[8];
    for(i=0; i<COMPASS_KEM_N/8; i++) {
      for(j=0; j<8; j++) {
        u = a->coeffs[8*i+j];
        u += (u >> 15) & COMPASS_KEM_Q;
        d_val = u;
        d_val -= (d_val != 0);
        t[j] = d_val >> 6;
      }
      r[0] = t[0] | (t[1] << 7);
      r[1] = (t[1] >> 1) | (t[2] << 6);
      r[2] = (t[2] >> 2) | (t[3] << 5);
      r[3] = (t[3] >> 3) | (t[4] << 4);
      r[4] = (t[4] >> 4) | (t[5] << 3);
      r[5] = (t[5] >> 5) | (t[6] << 2);
      r[6] = (t[6] >> 6) | (t[7] << 1);
      r += 7;
    }
  }
  else if (d == 8) {
    // [Mod]: Drop 8 bits, keep 5 bits (13 - 8 = 5)
    uint8_t t[8];
    for(i=0; i<COMPASS_KEM_N/8; i++) {
      for(j=0; j<8; j++) {
        u = a->coeffs[8*i+j];
        u += (u >> 15) & COMPASS_KEM_Q;
        d_val = u;
        d_val -= (d_val != 0);
        t[j] = d_val >> 8; // Shift right 8 bits, keep 5 bits
      }
      // Pack 8 x 5-bit coefficients (40 bits total) into 5 bytes
      r[0] = t[0] | (t[1] << 5);
      r[1] = (t[1] >> 3) | (t[2] << 2) | (t[3] << 7);
      r[2] = (t[3] >> 1) | (t[4] << 4);
      r[3] = (t[4] >> 4) | (t[5] << 1) | (t[6] << 6);
      r[4] = (t[6] >> 2) | (t[7] << 3);
      r += 5;
    }
  }
#endif
}

/*************************************************
* Name:        poly_decompress
*
* Description: De-serialization and subsequent decompression of a polynomial;
*              approximate inverse of poly_compress
*
* Arguments:   - poly *r: pointer to output polynomial
*              - const uint8_t *a: pointer to input byte array
*                                  (of length COMPASS_KEM_POLYCOMPRESSEDBYTES bytes)
**************************************************/
void poly_decompress(poly *r, const uint8_t *a, int8_t d) {
  unsigned int i;

#if (COMPASS_KEM_Q == 3329)
  if (d == 2) {
    // Decompress 10 bits: x = round(x * 4 + 2)
    for(i=0; i<COMPASS_KEM_N/4; i++) {
      r->coeffs[4*i+0] = ( (((a[0] >> 0) | ((uint16_t)a[1] << 8)) & 0x3FF) << 2 ) | 2;
      r->coeffs[4*i+1] = ( (((a[1] >> 2) | ((uint16_t)a[2] << 6)) & 0x3FF) << 2 ) | 2;
      r->coeffs[4*i+2] = ( (((a[2] >> 4) | ((uint16_t)a[3] << 4)) & 0x3FF) << 2 ) | 2;
      r->coeffs[4*i+3] = ( (((a[3] >> 6) | ((uint16_t)a[4] << 2)) & 0x3FF) << 2 ) | 2;
      a += 5;
    }
  } 
  else if (d == 6) {
    // Decompress 6 bits: x = round(x * 64 + 32)
    for(i=0; i<COMPASS_KEM_N/4; i++) {
      r->coeffs[4*i+0] = ( (((a[0]      )                 ) & 0x3F) << 6 ) | 32;
      r->coeffs[4*i+1] = ( (((a[0] >> 6) | ((uint16_t)a[1] << 2)) & 0x3F) << 6 ) | 32;
      r->coeffs[4*i+2] = ( (((a[1] >> 4) | ((uint16_t)a[2] << 4)) & 0x3F) << 6 ) | 32;
      r->coeffs[4*i+3] = ( (((a[2] >> 2)                  ) & 0x3F) << 6 ) | 32;
      a += 3;
    }
  }
  else if (d == 8) {
    // [Mod]: Decompress 4 bits: x = round(x * 256 + 128)
    for(i=0; i<COMPASS_KEM_N/2; i++) {
      r->coeffs[2*i+0] = (((uint16_t)(a[i] & 0x0F)) << 8) | 128;
      r->coeffs[2*i+1] = (((uint16_t)(a[i] >> 4)) << 8) | 128;
    }
  }

#elif (COMPASS_KEM_Q == 7681)
  if (d == 2) {
    // Decompress 11 bits: x = round(x * 4 + 2)
    for(i=0; i<COMPASS_KEM_N/8; i++) {
      r->coeffs[8*i+0] = ( (((a[0] >> 0) | ((uint16_t)a[1] << 8)) & 0x7FF) << 2 ) | 2;
      r->coeffs[8*i+1] = ( (((a[1] >> 3) | ((uint16_t)a[2] << 5)) & 0x7FF) << 2 ) | 2;
      r->coeffs[8*i+2] = ( (((a[2] >> 6) | ((uint16_t)a[3] << 2) | ((uint16_t)a[4] << 10)) & 0x7FF) << 2 ) | 2;
      r->coeffs[8*i+3] = ( (((a[4] >> 1) | ((uint16_t)a[5] << 7)) & 0x7FF) << 2 ) | 2;
      r->coeffs[8*i+4] = ( (((a[5] >> 4) | ((uint16_t)a[6] << 4)) & 0x7FF) << 2 ) | 2;
      r->coeffs[8*i+5] = ( (((a[6] >> 7) | ((uint16_t)a[7] << 1) | ((uint16_t)a[8] << 9)) & 0x7FF) << 2 ) | 2;
      r->coeffs[8*i+6] = ( (((a[8] >> 2) | ((uint16_t)a[9] << 6)) & 0x7FF) << 2 ) | 2;
      r->coeffs[8*i+7] = ( (((a[9] >> 5) | ((uint16_t)a[10] << 3)) & 0x7FF) << 2 ) | 2;
      a += 11;
    }
  } 
  else if (d == 6) {
    // Decompress 7 bits: x = round(x * 64 + 32)
    for(i=0; i<COMPASS_KEM_N/8; i++) {
      r->coeffs[8*i+0] = ( (((a[0]      )                 ) & 0x7F) << 6 ) | 32;
      r->coeffs[8*i+1] = ( (((a[0] >> 7) | ((uint16_t)a[1] << 1)) & 0x7F) << 6 ) | 32;
      r->coeffs[8*i+2] = ( (((a[1] >> 6) | ((uint16_t)a[2] << 2)) & 0x7F) << 6 ) | 32;
      r->coeffs[8*i+3] = ( (((a[2] >> 5) | ((uint16_t)a[3] << 3)) & 0x7F) << 6 ) | 32;
      r->coeffs[8*i+4] = ( (((a[3] >> 4) | ((uint16_t)a[4] << 4)) & 0x7F) << 6 ) | 32;
      r->coeffs[8*i+5] = ( (((a[4] >> 3) | ((uint16_t)a[5] << 5)) & 0x7F) << 6 ) | 32;
      r->coeffs[8*i+6] = ( (((a[5] >> 2) | ((uint16_t)a[6] << 6)) & 0x7F) << 6 ) | 32;
      r->coeffs[8*i+7] = ( (((a[6] >> 1)                  ) & 0x7F) << 6 ) | 32;
      a += 7;
    }
  }
  else if (d == 8) {
    // [Mod]: Decompress 5 bits: x = round(x * 256 + 128)
    for(i=0; i<COMPASS_KEM_N/8; i++) {
      r->coeffs[8*i+0] = ((((a[0]      )       ) & 0x1F) << 8) | 128;
      r->coeffs[8*i+1] = ((((a[0] >> 5) | ((uint16_t)a[1] << 3)) & 0x1F) << 8) | 128;
      r->coeffs[8*i+2] = ((((a[1] >> 2)       ) & 0x1F) << 8) | 128;
      r->coeffs[8*i+3] = ((((a[1] >> 7) | ((uint16_t)a[2] << 1)) & 0x1F) << 8) | 128;
      r->coeffs[8*i+4] = ((((a[2] >> 4) | ((uint16_t)a[3] << 4)) & 0x1F) << 8) | 128;
      r->coeffs[8*i+5] = ((((a[3] >> 1)       ) & 0x1F) << 8) | 128;
      r->coeffs[8*i+6] = ((((a[3] >> 6) | ((uint16_t)a[4] << 2)) & 0x1F) << 8) | 128;
      r->coeffs[8*i+7] = ((((a[4] >> 3)       ) & 0x1F) << 8) | 128;
      a += 5;
    }
  }
#endif
}

/*************************************************
* Name:        poly_tobytes
*
* Description: Serialization of a polynomial
*
* Arguments:   - uint8_t *r: pointer to output byte array
*                            (needs space for COMPASS_KEM_POLYBYTES bytes)
*              - const poly *a: pointer to input polynomial
**************************************************/
void poly_tobytes(uint8_t r[COMPASS_KEM_POLYBYTES], const poly *a)
{
  unsigned int i;
  
#if (COMPASS_KEM_Q == 3329)
  uint16_t t0, t1;
  for(i=0;i<COMPASS_KEM_N/2;i++) {
    // map to positive standard representatives
    t0  = a->coeffs[2*i];
    t0 += ((int16_t)t0 >> 15) & COMPASS_KEM_Q;
    t1 = a->coeffs[2*i+1];
    t1 += ((int16_t)t1 >> 15) & COMPASS_KEM_Q;
    r[3*i+0] = (t0 >> 0);
    r[3*i+1] = (t0 >> 8) | (t1 << 4);
    r[3*i+2] = (t1 >> 4);
  }
#elif (COMPASS_KEM_Q == 7681)
  // New 13-bit logic: 8 coefficients -> 13 bytes
  uint16_t t[8];
  for(i=0; i<COMPASS_KEM_N/8; i++) {
    for(int j=0; j<8; j++) {
      t[j] = a->coeffs[8*i+j];
      t[j] += ((int16_t)t[j] >> 15) & COMPASS_KEM_Q;
    }
    r[0]  = (t[0] >> 0);
    r[1]  = (t[0] >> 8) | (t[1] << 5);
    r[2]  = (t[1] >> 3);
    r[3]  = (t[1] >> 11) | (t[2] << 2);
    r[4]  = (t[2] >> 6) | (t[3] << 7);
    r[5]  = (t[3] >> 1);
    r[6]  = (t[3] >> 9) | (t[4] << 4);
    r[7]  = (t[4] >> 4);
    r[8]  = (t[4] >> 12) | (t[5] << 1);
    r[9]  = (t[5] >> 7) | (t[6] << 6);
    r[10] = (t[6] >> 2);
    r[11] = (t[6] >> 10) | (t[7] << 3);
    r[12] = (t[7] >> 5);
    r += 13;
  }
#endif
}

/*************************************************
* Name:        poly_frombytes
*
* Description: De-serialization of a polynomial;
*              inverse of poly_tobytes
*
* Arguments:   - poly *r: pointer to output polynomial
*              - const uint8_t *a: pointer to input byte array
*                                  (of COMPASS_KEM_POLYBYTES bytes)
**************************************************/
void poly_frombytes(poly *r, const uint8_t a[COMPASS_KEM_POLYBYTES])
{
  unsigned int i;
#if (COMPASS_KEM_Q == 3329)
  for(i=0;i<COMPASS_KEM_N/2;i++) {
    r->coeffs[2*i]   = ((a[3*i+0] >> 0) | ((uint16_t)a[3*i+1] << 8)) & 0xFFF;
    r->coeffs[2*i+1] = ((a[3*i+1] >> 4) | ((uint16_t)a[3*i+2] << 4)) & 0xFFF;
  }
#elif (COMPASS_KEM_Q == 7681)
  // --- 13-bit logic (unpack 8 coefficients from 13 bytes) ---
  // Mask is 0x1FFF (i.e., 2^13 - 1)
  for(i=0; i<COMPASS_KEM_N/8; i++) {
    r->coeffs[8*i+0] = ( (uint16_t)a[13*i+0]       | ((uint16_t)a[13*i+1] << 8)) & 0x1FFF;
    r->coeffs[8*i+1] = ( (uint16_t)a[13*i+1] >> 5  | ((uint16_t)a[13*i+2] << 3) | ((uint16_t)a[13*i+3] << 11)) & 0x1FFF;
    r->coeffs[8*i+2] = ( (uint16_t)a[13*i+3] >> 2  | ((uint16_t)a[13*i+4] << 6)) & 0x1FFF;
    r->coeffs[8*i+3] = ( (uint16_t)a[13*i+4] >> 7  | ((uint16_t)a[13*i+5] << 1) | ((uint16_t)a[13*i+6] << 9)) & 0x1FFF;
    r->coeffs[8*i+4] = ( (uint16_t)a[13*i+6] >> 4  | ((uint16_t)a[13*i+7] << 4) | ((uint16_t)a[13*i+8] << 12)) & 0x1FFF;
    r->coeffs[8*i+5] = ( (uint16_t)a[13*i+8] >> 1  | ((uint16_t)a[13*i+9] << 7)) & 0x1FFF;
    r->coeffs[8*i+6] = ( (uint16_t)a[13*i+9] >> 6  | ((uint16_t)a[13*i+10] << 2) | ((uint16_t)a[13*i+11] << 10)) & 0x1FFF;
    r->coeffs[8*i+7] = ( (uint16_t)a[13*i+11] >> 3 | ((uint16_t)a[13*i+12] << 5)) & 0x1FFF;
  }
#endif
}

/*************************************************
* Name:        poly_frommsg
*
* Description: Convert 32-byte message to polynomial
*
* Arguments:   - poly *r: pointer to output polynomial
*              - const uint8_t *msg: pointer to input message
**************************************************/
void poly_frommsg(poly *r, const uint8_t msg[COMPASS_KEM_INDCPA_MSGBYTES])
{
  unsigned int i, j;
  int16_t mask;

  // 1. Only process first 256 coefficients (corresponding to 32-byte message)
  for(i=0; i<COMPASS_KEM_INDCPA_MSGBYTES; i++) {
    for(j=0; j<8; j++) {
      // Extract bit. If 1, mask = 0xFFFF; otherwise 0
      mask = -(int16_t)((msg[i] >> j) & 1);
      
      // Set to 0 or (q-1)/2 directly
      r->coeffs[8*i+j] = mask & ((COMPASS_KEM_Q - 1) / 2);
    }
  }

  // 2. MLWR heterogeneous protection: if N > 256 (e.g., 512), zero-clear remaining high coefficients
  for(i = COMPASS_KEM_INDCPA_MSGBYTES * 8; i < COMPASS_KEM_N; i++) {
    r->coeffs[i] = 0;
  }
}
// void poly_frommsg(poly *r, const uint8_t msg[COMPASS_KEM_INDCPA_MSGBYTES])
// {
//   unsigned int i, j;
//   int16_t mask;

//   // Zero all coefficients first (critical: prevents uninitialized data when n=512)
//   for(i=0; i<COMPASS_KEM_N; i++) {
//     r->coeffs[i] = 0;
//   }

//   // Only process 32 bytes (256 bits)
//   for(i=0; i<COMPASS_KEM_INDCPA_MSGBYTES; i++) {
//     for(j=0; j<8; j++) {
//       // Extract corresponding bit from msg. If 1, mask = 0xFFFF (-1); otherwise 0
//       mask = -(int16_t)((msg[i] >> j) & 1);
//       // If bit is 1, assign (q-1)/2; otherwise 0
//       r->coeffs[8*i+j] = mask & ((COMPASS_KEM_Q - 1) / 2);
//     }
//   }
// }
/*************************************************
* Name:        poly_tomsg
*
* Description: Convert polynomial to 32-byte message
*
* Arguments:   - uint8_t *msg: pointer to output message
*              - const poly *a: pointer to input polynomial
**************************************************/
void poly_tomsg(uint8_t msg[COMPASS_KEM_INDCPA_MSGBYTES], const poly *a)
{
  unsigned int i, j;
  uint16_t t;
  
  // Pre-compute constants to avoid repeated division in the loop
  const uint16_t lower_bound = (COMPASS_KEM_Q - 1) / 4;
  const uint16_t width = (COMPASS_KEM_Q - 1) / 2; // upper_bound - lower_bound

  for(i=0; i<COMPASS_KEM_INDCPA_MSGBYTES; i++) {
    msg[i] = 0;
    for(j=0; j<8; j++) {
      t = a->coeffs[8*i+j];
      
      // 1. Map negative values to standard positive domain [0, q-1]
      t += ((int16_t)t >> 15) & COMPASS_KEM_Q;

      // 2. Constant-time branchless threshold decision
      // Uses unsigned underflow: if t < lower_bound, t - lower_bound becomes a very large positive number
      uint16_t bit = (uint16_t)(t - lower_bound) < width;
      
      // 3. Write byte
      msg[i] |= (bit << j);
    }
  }
}
// void poly_tomsg(uint8_t msg[COMPASS_KEM_INDCPA_MSGBYTES], const poly *a)
// {
//   unsigned int i, j;
//   uint16_t t;

//   for(i=0; i<COMPASS_KEM_INDCPA_MSGBYTES; i++) {
//     msg[i] = 0;
//     for(j=0; j<8; j++) {
//       t = a->coeffs[8*i+j];
      
//       // 1. Map negative values to standard positive domain [0, q-1]
//       t += ((int16_t)t >> 15) & COMPASS_KEM_Q;

//       // 2. MLWR exact threshold decision logic
//       int bit = 0;
//       uint16_t lower_bound = (COMPASS_KEM_Q - 1) / 4;
//       uint16_t upper_bound = 3 * (COMPASS_KEM_Q - 1) / 4;
      
//       if (t >= lower_bound && t < upper_bound) {
//         bit = 1;
//       }
      
//       // 3. Write byte
//       msg[i] |= (bit << j);
//     }
//   }
// }

/*************************************************
* Name:        poly_getnoise_eta1
*
* Description: Sample a polynomial deterministically from a seed and a nonce,
*              with output polynomial close to centered binomial distribution
*              with parameter COMPASS_KEM_ETA1
*
* Arguments:   - poly *r: pointer to output polynomial
*              - const uint8_t *seed: pointer to input seed
*                                     (of length COMPASS_KEM_SYMBYTES bytes)
*              - uint8_t nonce: one-byte input nonce
**************************************************/
void poly_getnoise_eta1(poly *r, const uint8_t seed[COMPASS_KEM_SYMBYTES], uint8_t nonce)
{
  uint8_t buf[COMPASS_KEM_ETA1*COMPASS_KEM_N/4];
  prf(buf, sizeof(buf), seed, nonce);
  poly_cbd_eta1(r, buf);
}

/*************************************************
* Name:        poly_getnoise_eta2
*
* Description: Sample a polynomial deterministically from a seed and a nonce,
*              with output polynomial close to centered binomial distribution
*              with parameter COMPASS_KEM_ETA2
*
* Arguments:   - poly *r: pointer to output polynomial
*              - const uint8_t *seed: pointer to input seed
*                                     (of length COMPASS_KEM_SYMBYTES bytes)
*              - uint8_t nonce: one-byte input nonce
**************************************************/
void poly_getnoise_eta2(poly *r, const uint8_t seed[COMPASS_KEM_SYMBYTES], uint8_t nonce)
{
  uint8_t buf[COMPASS_KEM_ETA2*COMPASS_KEM_N/4];
  prf(buf, sizeof(buf), seed, nonce);
  poly_cbd_eta2(r, buf);
}


/*************************************************
* Name:        poly_ntt
*
* Description: Computes negacyclic number-theoretic transform (NTT) of
*              a polynomial in place;
*              inputs assumed to be in normal order, output in bitreversed order
*
* Arguments:   - uint16_t *r: pointer to in/output polynomial
**************************************************/
void poly_ntt(poly *r)
{
  ntt(r->coeffs);
  poly_reduce(r);
}

/*************************************************
* Name:        poly_invntt_tomont
*
* Description: Computes inverse of negacyclic number-theoretic transform (NTT)
*              of a polynomial in place;
*              inputs assumed to be in bitreversed order, output in normal order
*
* Arguments:   - uint16_t *a: pointer to in/output polynomial
**************************************************/
void poly_invntt_tomont(poly *r)
{
  invntt(r->coeffs);
}

/*************************************************
* Name:        poly_basemul_montgomery
*
* Description: Multiplication of two polynomials in NTT domain
*
* Arguments:   - poly *r: pointer to output polynomial
*              - const poly *a: pointer to first input polynomial
*              - const poly *b: pointer to second input polynomial
**************************************************/
void poly_basemul_montgomery(poly *r, const poly *a, const poly *b)
{
  unsigned int i;
  for(i=0;i<(COMPASS_KEM_N>>2);i++) {
    basemul(&r->coeffs[4*i], &a->coeffs[4*i], &b->coeffs[4*i], zetas[(COMPASS_KEM_N>>2)+i]);
    basemul(&r->coeffs[4*i+2], &a->coeffs[4*i+2], &b->coeffs[4*i+2], -zetas[(COMPASS_KEM_N>>2)+i]);
  }
}

/*************************************************
* Name:        poly_tomont
*
* Description: Inplace conversion of all coefficients of a polynomial
*              from normal domain to Montgomery domain
*
* Arguments:   - poly *r: pointer to input/output polynomial
**************************************************/
void poly_tomont(poly *r)
{
  unsigned int i;
  const int16_t f = (1ULL << 32) % COMPASS_KEM_Q;
  for(i=0;i<COMPASS_KEM_N;i++)
    r->coeffs[i] = montgomery_reduce((int32_t)r->coeffs[i]*f);
}

/*************************************************
* Name:        poly_reduce
*
* Description: Applies Barrett reduction to all coefficients of a polynomial
*              for details of the Barrett reduction see comments in reduce.c
*
* Arguments:   - poly *r: pointer to input/output polynomial
**************************************************/
void poly_reduce(poly *r)
{
  unsigned int i;
  for(i=0;i<COMPASS_KEM_N;i++)
    r->coeffs[i] = barrett_reduce(r->coeffs[i]);
}

/*************************************************
* Name:        poly_add
*
* Description: Add two polynomials; no modular reduction is performed
*
* Arguments: - poly *r: pointer to output polynomial
*            - const poly *a: pointer to first input polynomial
*            - const poly *b: pointer to second input polynomial
**************************************************/
void poly_add(poly *r, const poly *a, const poly *b)
{
  unsigned int i;
  for(i=0;i<COMPASS_KEM_N;i++)
    r->coeffs[i] = a->coeffs[i] + b->coeffs[i];
}

/*************************************************
* Name:        poly_sub
*
* Description: Subtract two polynomials; no modular reduction is performed
*
* Arguments: - poly *r:       pointer to output polynomial
*            - const poly *a: pointer to first input polynomial
*            - const poly *b: pointer to second input polynomial
**************************************************/
void poly_sub(poly *r, const poly *a, const poly *b)
{
  unsigned int i;
  for(i=0;i<COMPASS_KEM_N;i++)
    r->coeffs[i] = a->coeffs[i] - b->coeffs[i];
}
