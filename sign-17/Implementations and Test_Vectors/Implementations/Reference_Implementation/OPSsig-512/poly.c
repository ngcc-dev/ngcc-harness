#include <stdint.h>
#include <string.h>
#include "auxfunc.h"
#include "params.h"
#include "reduce.h"
#include "rounding.h"
#include "ntt.h"
#include "poly.h"


/* Wrapper around the ICCS auxiliary XOF with byte-length arguments. */
static void xof_bytes(uint8_t *out, unsigned long long outlen, const uint8_t *in, unsigned long long inlen)
{
  pseudoXOF(outlen * 8ULL, in, inlen * 8ULL, out);
}

/* Initial XOF lengths derived from the current rejection probabilities. */
#define POLY_UNIFORM_BUFLEN \
  (4U * (unsigned int)((((uint64_t)N << 26) + Q - 1) / Q))

#if ETA == 2
#define POLY_UNIFORM_ETA_BUFLEN ((8U * N + 14U) / 15U) 
#elif ETA == 3
#define POLY_UNIFORM_ETA_BUFLEN ((8U * N + 13U) / 14U)
#endif

/* Reduce all coefficients of a polynomial modulo Q. */
void poly_reduce(poly *a) {
  for(unsigned i = 0; i < N; ++i) {
    a->coeffs[i] = reduce32(a->coeffs[i]);
  }
}

/* Conditionally add Q to move coefficients into the standard nonnegative range. */
void poly_caddq(poly *a) {
  for(unsigned i = 0; i < N; ++i) {
    a->coeffs[i] = caddq(a->coeffs[i]);
  }
}

/* Add two polynomials coefficient-wise. */
void poly_add(poly *c, const poly *a, const poly *b) {
  for(unsigned i = 0; i < N; ++i)
    c->coeffs[i] = a->coeffs[i] + b->coeffs[i]; 
}

/* Subtract two polynomials coefficient-wise. */
void poly_sub(poly *c, const poly *a, const poly *b) {
  for(unsigned i = 0; i < N; ++i)
    c->coeffs[i] = a->coeffs[i] - b->coeffs[i]; 
}

/* Left-shift coefficients by D bits as required by the scheme. */
void poly_shiftl(poly *a) {
  for(unsigned i = 0; i < N; ++i)
    a->coeffs[i] = a->coeffs[i] << D; 
}

/* Map a polynomial into the NTT domain. */
void poly_ntt(poly *a) {
  ntt(a->coeffs);
}

/* Map a polynomial back from the NTT domain. */
void poly_invntt_tomont(poly *a) {
  invntt_tomont(a->coeffs);
}

/* Pointwise multiply two NTT-domain polynomials. */
void poly_pointwise_montgomery(poly *c, const poly *a, const poly *b) {
  for(unsigned i = 0; i < N; ++i)
    c->coeffs[i] = montgomery_reduce((int64_t)a->coeffs[i] * b->coeffs[i]);
}

/* Split coefficients into high and low parts for the public key. */
void poly_power2round(poly *a1, poly *a0, const poly *a) {
  for(unsigned i = 0; i < N; ++i)
    a1->coeffs[i] = power2round(&a0->coeffs[i], a->coeffs[i]);
}

/* Decompose coefficients into hint-related high and low parts. */
void poly_decompose(poly *a1, poly *a0, const poly *a) {
  for(unsigned i = 0; i < N; ++i)
    a1->coeffs[i] = decompose(&a0->coeffs[i], a->coeffs[i]);
}

/* Generate the hint polynomial and return its Hamming weight. */
unsigned int poly_make_hint(poly *h, const poly *a0, const poly *a1) {
  unsigned int s = 0;
  for(unsigned i = 0; i < N; ++i) {
    h->coeffs[i] = (int32_t)make_hint(a0->coeffs[i], a1->coeffs[i]);
    s += (unsigned)h->coeffs[i];
  }
  return s;
}

/* Apply hint bits to reconstruct the high bits of a polynomial. */
void poly_use_hint(poly *b, const poly *a, const poly *h) {
  for(unsigned i = 0; i < N; ++i)
    b->coeffs[i] = use_hint(a->coeffs[i], (unsigned int)h->coeffs[i]);
}

/* Check whether any centered coefficient exceeds the requested bound. */
int poly_chknorm(const poly *a, int32_t B) {
  unsigned int i;
  int32_t t;

  if(B > (Q-1)/8)
    return 1;

  for(i = 0; i < N; ++i) {
    t = a->coeffs[i] >> 31;
    t = a->coeffs[i] - (t & 2*a->coeffs[i]);
    if(t >= B)
      return 1;
  }
  return 0;
}

/* Rejection-sample coefficients uniformly modulo Q from an XOF buffer. */
static unsigned int rej_uniform(int32_t *a, unsigned int len, const uint8_t *buf, unsigned int buflen) {
  unsigned int ctr = 0;
  unsigned int pos = 0;
  while(ctr < len && pos + 4 <= buflen) {
    uint32_t t = (uint32_t)buf[pos++];
    t |= (uint32_t)buf[pos++] << 8;
    t |= (uint32_t)buf[pos++] << 16;
    t |= (uint32_t)buf[pos++] << 24;
    t &= ((1u << 26) - 1u);
    if(t < Q)
      a[ctr++] = (int32_t)t;
  }
  return ctr;
}

/* Expand a seed and nonce into a uniformly random polynomial. */
void poly_uniform(poly *a, const uint8_t seed[SEEDBYTES], uint16_t nonce) {
  uint8_t in[SEEDBYTES + 2 + 4];
  uint8_t buf[POLY_UNIFORM_BUFLEN];
  unsigned int ctr = 0;
  uint32_t block = 0;

  memcpy(in, seed, SEEDBYTES);
  in[SEEDBYTES + 0] = (uint8_t)nonce;
  in[SEEDBYTES + 1] = (uint8_t)(nonce >> 8);
  in[SEEDBYTES + 2] = 0;
  in[SEEDBYTES + 3] = 0;
  in[SEEDBYTES + 4] = 0;
  in[SEEDBYTES + 5] = 0;

  xof_bytes(buf, sizeof(buf), in, sizeof(in));
  ctr = rej_uniform(a->coeffs, N, buf, sizeof(buf));

  while(ctr < N) {
    block++;
    in[SEEDBYTES + 2] = (uint8_t)block;
    in[SEEDBYTES + 3] = (uint8_t)(block >> 8);
    in[SEEDBYTES + 4] = (uint8_t)(block >> 16);
    in[SEEDBYTES + 5] = (uint8_t)(block >> 24);
    xof_bytes(buf, sizeof(buf), in, sizeof(in));
    ctr += rej_uniform(a->coeffs + ctr, N - ctr, buf, sizeof(buf));
  }
}

/* Rejection-sample coefficients in the small ETA range. */
static unsigned int rej_eta(int32_t *a, unsigned int len, const uint8_t *buf, unsigned int buflen) {
  unsigned int ctr = 0;
  unsigned int pos = 0;
  while(ctr < len && pos < buflen) {
    uint32_t t0 = buf[pos] & 0x0F;
    uint32_t t1 = buf[pos++] >> 4;

#if ETA == 2
    if(t0 < 15) {
      t0 = t0 - (205*t0 >> 10)*5;
      a[ctr++] = 2 - (int32_t)t0;
    }
    if(t1 < 15 && ctr < len) {
      t1 = t1 - (205*t1 >> 10)*5;
      a[ctr++] = 2 - (int32_t)t1;
    }
#elif ETA == 3
    if(t0 < 14) {
      t0 = t0 - (147*t0 >> 10)*7;
      a[ctr++] = 3 - (int32_t)t0;
    }
    if(t1 < 14 && ctr < len) {
      t1 = t1 - (147*t1 >> 10)*7;
      a[ctr++] = 3 - (int32_t)t1;
    }
#endif
  }
  return ctr;
}

/* Expand a seed and nonce into a small-noise polynomial. */
void poly_uniform_eta(poly *a, const uint8_t seed[CRHBYTES], uint16_t nonce) {
  uint8_t in[CRHBYTES + 2 + 4];
  uint8_t buf[POLY_UNIFORM_ETA_BUFLEN];
  unsigned int ctr = 0;
  uint32_t block = 0;

  memcpy(in, seed, CRHBYTES);
  in[CRHBYTES + 0] = (uint8_t)nonce; 
  in[CRHBYTES + 1] = (uint8_t)(nonce >> 8);
  in[CRHBYTES + 2] = 0;
  in[CRHBYTES + 3] = 0;
  in[CRHBYTES + 4] = 0;
  in[CRHBYTES + 5] = 0;

  xof_bytes(buf, sizeof(buf), in, sizeof(in));
  ctr = rej_eta(a->coeffs, N, buf, sizeof(buf));

  while(ctr < N) {
    block++;
    in[CRHBYTES + 2] = (uint8_t)block;
    in[CRHBYTES + 3] = (uint8_t)(block >> 8);
    in[CRHBYTES + 4] = (uint8_t)(block >> 16);
    in[CRHBYTES + 5] = (uint8_t)(block >> 24);
    xof_bytes(buf, sizeof(buf), in, sizeof(in));
    ctr += rej_eta(a->coeffs + ctr, N - ctr, buf, sizeof(buf));
  }
}

/* Expand a seed and nonce into the signing mask polynomial y. */
void poly_uniform_gamma1(poly *a, const uint8_t seed[CRHBYTES], uint16_t nonce) {
  uint8_t in[CRHBYTES + 2];
  uint8_t buf[POLYZ_PACKEDBYTES];

  memcpy(in, seed, CRHBYTES);
  in[CRHBYTES + 0] = (uint8_t)nonce;
  in[CRHBYTES + 1] = (uint8_t)(nonce >> 8);
  xof_bytes(buf, sizeof(buf), in, sizeof(in));
  polyz_unpack(a, buf);
}

/* Derive the sparse challenge polynomial from the challenge seed. */
void poly_challenge(poly *c, const uint8_t seed[CTILDEBYTES]) {
  uint8_t buf[CTILDEBYTES];
  uint64_t signs_1, signs_2;
  unsigned int pos = 0;

  xof_bytes(buf, CTILDEBYTES, seed, CTILDEBYTES);
  signs_1 = signs_2 = 0;
  for(unsigned i = 0; i < 8; i++) {
    signs_1 |= (uint64_t)buf[i] << (8*i);
    signs_2 |= (uint64_t)buf[i + 8] << (8*i);
  }
  pos = 16;

  for(unsigned i = 0; i < N; i++)
    c->coeffs[i] = 0;

  for(unsigned i = N - TAU; i < N - TAU + 64; ++i) {
    uint16_t b;
    do {
      if(pos + 2 > sizeof(buf)) {
        xof_bytes(buf, CTILDEBYTES, buf, CTILDEBYTES);
        pos = 0;
      }
      b = (uint16_t)buf[pos] | ((uint16_t)buf[pos + 1] << 8);
      b &= 0x03FF;
      pos += 2;
    } while(b > i);

    c->coeffs[i] = c->coeffs[b];
    c->coeffs[b] = (int32_t)(1 - 2*(signs_1 & 1));
    signs_1 >>= 1;
  }

  for(unsigned i = N - TAU + 64; i < N; ++i) {
    uint16_t b;
    do {
      if(pos + 2 > sizeof(buf)) {
        xof_bytes(buf, CTILDEBYTES, buf, CTILDEBYTES);
        pos = 0;
      }
      b = (uint16_t)buf[pos] | ((uint16_t)buf[pos + 1] << 8);
      b &= 0x03FF;
      pos += 2;
    } while(b > i);

    c->coeffs[i] = c->coeffs[b];
    c->coeffs[b] = (int32_t)(1 - 2*(signs_2 & 1));
    signs_2 >>= 1;
  }
}

/* Pack a polynomial with coefficients in [-ETA, ETA]. */
void polyeta_pack(uint8_t *r, const poly *a) {
  for(unsigned i = 0; i < N/8; ++i) {
    uint8_t t[8];
    t[0] = (uint8_t)(ETA - a->coeffs[8*i+0]);
    t[1] = (uint8_t)(ETA - a->coeffs[8*i+1]);
    t[2] = (uint8_t)(ETA - a->coeffs[8*i+2]);
    t[3] = (uint8_t)(ETA - a->coeffs[8*i+3]);
    t[4] = (uint8_t)(ETA - a->coeffs[8*i+4]);
    t[5] = (uint8_t)(ETA - a->coeffs[8*i+5]);
    t[6] = (uint8_t)(ETA - a->coeffs[8*i+6]);
    t[7] = (uint8_t)(ETA - a->coeffs[8*i+7]);

    r[3*i+0]  = (uint8_t)((t[0] >> 0) | (t[1] << 3) | (t[2] << 6));
    r[3*i+1]  = (uint8_t)((t[2] >> 2) | (t[3] << 1) | (t[4] << 4) | (t[5] << 7));
    r[3*i+2]  = (uint8_t)((t[5] >> 1) | (t[6] << 2) | (t[7] << 5));
  }
}

/* Unpack a polynomial with coefficients in [-ETA, ETA]. */
void polyeta_unpack(poly *r, const uint8_t *a) {
  for(unsigned i = 0; i < N/8; ++i) {
    r->coeffs[8*i+0] =  (a[3*i+0] >> 0) & 7;
    r->coeffs[8*i+1] =  (a[3*i+0] >> 3) & 7;
    r->coeffs[8*i+2] = ((a[3*i+0] >> 6) | ((uint32_t)a[3*i+1] << 2)) & 7;
    r->coeffs[8*i+3] =  (a[3*i+1] >> 1) & 7;
    r->coeffs[8*i+4] =  (a[3*i+1] >> 4) & 7;
    r->coeffs[8*i+5] = ((a[3*i+1] >> 7) | ((uint32_t)a[3*i+2] << 1)) & 7;
    r->coeffs[8*i+6] =  (a[3*i+2] >> 2) & 7;
    r->coeffs[8*i+7] =  (a[3*i+2] >> 5) & 7;

    r->coeffs[8*i+0] = ETA - r->coeffs[8*i+0];
    r->coeffs[8*i+1] = ETA - r->coeffs[8*i+1];
    r->coeffs[8*i+2] = ETA - r->coeffs[8*i+2];
    r->coeffs[8*i+3] = ETA - r->coeffs[8*i+3];
    r->coeffs[8*i+4] = ETA - r->coeffs[8*i+4];
    r->coeffs[8*i+5] = ETA - r->coeffs[8*i+5];
    r->coeffs[8*i+6] = ETA - r->coeffs[8*i+6];
    r->coeffs[8*i+7] = ETA - r->coeffs[8*i+7];
  }
}

/* Pack the high bits t1 of the public key polynomial. */
void polyt1_pack(uint8_t *r, const poly *a) {
  for(unsigned i = 0; i < N/8; ++i) {
    uint32_t t[8];
    for(unsigned j = 0; j < 8; j++)
      t[j] = (uint32_t)a->coeffs[8*i+j] & 0x1FFFu; 

    r[13*i+ 0]  =  t[0];
    r[13*i+ 1]  =  t[0] >>  8;
    r[13*i+ 1] |=  t[1] <<  5;
    r[13*i+ 2]  =  t[1] >>  3;
    r[13*i+ 3]  =  t[1] >> 11;
    r[13*i+ 3] |=  t[2] <<  2;
    r[13*i+ 4]  =  t[2] >>  6;
    r[13*i+ 4] |=  t[3] <<  7;
    r[13*i+ 5]  =  t[3] >>  1;
    r[13*i+ 6]  =  t[3] >>  9;
    r[13*i+ 6] |=  t[4] <<  4;
    r[13*i+ 7]  =  t[4] >>  4;
    r[13*i+ 8]  =  t[4] >> 12;
    r[13*i+ 8] |=  t[5] <<  1;
    r[13*i+ 9]  =  t[5] >>  7;
    r[13*i+ 9] |=  t[6] <<  6;
    r[13*i+10]  =  t[6] >>  2;
    r[13*i+11]  =  t[6] >> 10;
    r[13*i+11] |=  t[7] <<  3;
    r[13*i+12]  =  t[7] >>  5;
  }
}

/* Unpack the high bits t1 of the public key polynomial. */
void polyt1_unpack(poly *r, const uint8_t *a) {
  for(unsigned i = 0; i < N/8; ++i) {
    r->coeffs[8*i+0]  = a[13*i+0];
    r->coeffs[8*i+0] |= (uint32_t)a[13*i+1] << 8;
    r->coeffs[8*i+0] &= 0x1FFF;

    r->coeffs[8*i+1]  = a[13*i+1] >> 5;
    r->coeffs[8*i+1] |= (uint32_t)a[13*i+2] << 3;
    r->coeffs[8*i+1] |= (uint32_t)a[13*i+3] << 11;
    r->coeffs[8*i+1] &= 0x1FFF;

    r->coeffs[8*i+2]  = a[13*i+3] >> 2;
    r->coeffs[8*i+2] |= (uint32_t)a[13*i+4] << 6;
    r->coeffs[8*i+2] &= 0x1FFF;

    r->coeffs[8*i+3]  = a[13*i+4] >> 7;
    r->coeffs[8*i+3] |= (uint32_t)a[13*i+5] << 1;
    r->coeffs[8*i+3] |= (uint32_t)a[13*i+6] << 9;
    r->coeffs[8*i+3] &= 0x1FFF;

    r->coeffs[8*i+4]  = a[13*i+6] >> 4;
    r->coeffs[8*i+4] |= (uint32_t)a[13*i+7] << 4;
    r->coeffs[8*i+4] |= (uint32_t)a[13*i+8] << 12;
    r->coeffs[8*i+4] &= 0x1FFF;

    r->coeffs[8*i+5]  = a[13*i+8] >> 1;
    r->coeffs[8*i+5] |= (uint32_t)a[13*i+9] << 7;
    r->coeffs[8*i+5] &= 0x1FFF;

    r->coeffs[8*i+6]  = a[13*i+9] >> 6;
    r->coeffs[8*i+6] |= (uint32_t)a[13*i+10] << 2;
    r->coeffs[8*i+6] |= (uint32_t)a[13*i+11] << 10;
    r->coeffs[8*i+6] &= 0x1FFF;

    r->coeffs[8*i+7]  = a[13*i+11] >> 3;
    r->coeffs[8*i+7] |= (uint32_t)a[13*i+12] << 5;
    r->coeffs[8*i+7] &= 0x1FFF;
  }
}

/* Pack the low bits t0 of the secret key polynomial. */
void polyt0_pack(uint8_t *r, const poly *a) {
  for(unsigned i = 0; i < N/8; ++i) {
    uint32_t t[8];
    for(unsigned j = 0; j < 8; j++)
      t[j] = (uint32_t)((1 << (D-1)) - a->coeffs[8*i+j]);

    r[13*i+ 0]  =  t[0];
    r[13*i+ 1]  =  t[0] >>  8;
    r[13*i+ 1] |=  t[1] <<  5;
    r[13*i+ 2]  =  t[1] >>  3;
    r[13*i+ 3]  =  t[1] >> 11;
    r[13*i+ 3] |=  t[2] <<  2;
    r[13*i+ 4]  =  t[2] >>  6;
    r[13*i+ 4] |=  t[3] <<  7;
    r[13*i+ 5]  =  t[3] >>  1;
    r[13*i+ 6]  =  t[3] >>  9;
    r[13*i+ 6] |=  t[4] <<  4;
    r[13*i+ 7]  =  t[4] >>  4;
    r[13*i+ 8]  =  t[4] >> 12;
    r[13*i+ 8] |=  t[5] <<  1;
    r[13*i+ 9]  =  t[5] >>  7;
    r[13*i+ 9] |=  t[6] <<  6;
    r[13*i+10]  =  t[6] >>  2;
    r[13*i+11]  =  t[6] >> 10;
    r[13*i+11] |=  t[7] <<  3;
    r[13*i+12]  =  t[7] >>  5;
  }
}

/* Unpack the low bits t0 of the secret key polynomial. */
void polyt0_unpack(poly *r, const uint8_t *a) {
  for(unsigned i = 0; i < N/8; ++i) {
    r->coeffs[8*i+0]  = a[13*i+0];
    r->coeffs[8*i+0] |= (uint32_t)a[13*i+1] << 8;
    r->coeffs[8*i+0] &= 0x1FFF;

    r->coeffs[8*i+1]  = a[13*i+1] >> 5;
    r->coeffs[8*i+1] |= (uint32_t)a[13*i+2] << 3;
    r->coeffs[8*i+1] |= (uint32_t)a[13*i+3] << 11;
    r->coeffs[8*i+1] &= 0x1FFF;

    r->coeffs[8*i+2]  = a[13*i+3] >> 2;
    r->coeffs[8*i+2] |= (uint32_t)a[13*i+4] << 6;
    r->coeffs[8*i+2] &= 0x1FFF;

    r->coeffs[8*i+3]  = a[13*i+4] >> 7;
    r->coeffs[8*i+3] |= (uint32_t)a[13*i+5] << 1;
    r->coeffs[8*i+3] |= (uint32_t)a[13*i+6] << 9;
    r->coeffs[8*i+3] &= 0x1FFF;

    r->coeffs[8*i+4]  = a[13*i+6] >> 4;
    r->coeffs[8*i+4] |= (uint32_t)a[13*i+7] << 4;
    r->coeffs[8*i+4] |= (uint32_t)a[13*i+8] << 12;
    r->coeffs[8*i+4] &= 0x1FFF;

    r->coeffs[8*i+5]  = a[13*i+8] >> 1;
    r->coeffs[8*i+5] |= (uint32_t)a[13*i+9] << 7;
    r->coeffs[8*i+5] &= 0x1FFF;

    r->coeffs[8*i+6]  = a[13*i+9] >> 6;
    r->coeffs[8*i+6] |= (uint32_t)a[13*i+10] << 2;
    r->coeffs[8*i+6] |= (uint32_t)a[13*i+11] << 10;
    r->coeffs[8*i+6] &= 0x1FFF;

    r->coeffs[8*i+7]  = a[13*i+11] >> 3;
    r->coeffs[8*i+7] |= (uint32_t)a[13*i+12] << 5;
    r->coeffs[8*i+7] &= 0x1FFF;

    r->coeffs[8*i+0] = (1 << (D-1)) - r->coeffs[8*i+0];
    r->coeffs[8*i+1] = (1 << (D-1)) - r->coeffs[8*i+1];
    r->coeffs[8*i+2] = (1 << (D-1)) - r->coeffs[8*i+2];
    r->coeffs[8*i+3] = (1 << (D-1)) - r->coeffs[8*i+3];
    r->coeffs[8*i+4] = (1 << (D-1)) - r->coeffs[8*i+4];
    r->coeffs[8*i+5] = (1 << (D-1)) - r->coeffs[8*i+5];
    r->coeffs[8*i+6] = (1 << (D-1)) - r->coeffs[8*i+6];
    r->coeffs[8*i+7] = (1 << (D-1)) - r->coeffs[8*i+7];
  }
}

/* Pack the response polynomial z in the signature. */
void polyz_pack(uint8_t *r, const poly *a) {

#if GAMMA1 == (1 << 20)
  for(unsigned i = 0; i < N/8; ++i) {
    uint32_t t[8];
    t[0] = (uint32_t)(GAMMA1 - a->coeffs[8*i+0]) & 0x1FFFFFu;
    t[1] = (uint32_t)(GAMMA1 - a->coeffs[8*i+1]) & 0x1FFFFFu;
    t[2] = (uint32_t)(GAMMA1 - a->coeffs[8*i+2]) & 0x1FFFFFu;
    t[3] = (uint32_t)(GAMMA1 - a->coeffs[8*i+3]) & 0x1FFFFFu;
    t[4] = (uint32_t)(GAMMA1 - a->coeffs[8*i+4]) & 0x1FFFFFu;
    t[5] = (uint32_t)(GAMMA1 - a->coeffs[8*i+5]) & 0x1FFFFFu;
    t[6] = (uint32_t)(GAMMA1 - a->coeffs[8*i+6]) & 0x1FFFFFu;
    t[7] = (uint32_t)(GAMMA1 - a->coeffs[8*i+7]) & 0x1FFFFFu;

    r[21*i+ 0]  =  t[0];
    r[21*i+ 1]  =  t[0] >>  8;
    r[21*i+ 2]  =  t[0] >> 16;
    r[21*i+ 2] |=  t[1] <<  5;
    r[21*i+ 3]  =  t[1] >>  3;
    r[21*i+ 4]  =  t[1] >> 11;
    r[21*i+ 5]  =  t[1] >> 19;
    r[21*i+ 5] |=  t[2] <<  2;
    r[21*i+ 6]  =  t[2] >>  6;
    r[21*i+ 7]  =  t[2] >> 14;
    r[21*i+ 7] |=  t[3] <<  7;
    r[21*i+ 8]  =  t[3] >>  1;
    r[21*i+ 9]  =  t[3] >>  9;
    r[21*i+10]  =  t[3] >> 17;
    r[21*i+10] |=  t[4] <<  4;
    r[21*i+11]  =  t[4] >>  4;
    r[21*i+12]  =  t[4] >> 12;
    r[21*i+13]  =  t[4] >> 20;
    r[21*i+13] |=  t[5] <<  1;
    r[21*i+14]  =  t[5] >>  7;
    r[21*i+15]  =  t[5] >> 15;
    r[21*i+15] |=  t[6] <<  6;
    r[21*i+16]  =  t[6] >>  2;
    r[21*i+17]  =  t[6] >> 10;
    r[21*i+18]  =  t[6] >> 18;
    r[21*i+18] |=  t[7] <<  3;
    r[21*i+19]  =  t[7] >>  5;
    r[21*i+20]  =  t[7] >> 13;
  }
#elif GAMMA1 == (1 << 21)
  for(unsigned i = 0; i < N/4; ++i) {
    uint32_t t[4];
    t[0] = (uint32_t)(GAMMA1 - a->coeffs[4*i+0]) & 0x3FFFFFu;
    t[1] = (uint32_t)(GAMMA1 - a->coeffs[4*i+1]) & 0x3FFFFFu;
    t[2] = (uint32_t)(GAMMA1 - a->coeffs[4*i+2]) & 0x3FFFFFu;
    t[3] = (uint32_t)(GAMMA1 - a->coeffs[4*i+3]) & 0x3FFFFFu;

    r[11*i+ 0]  =  t[0];
    r[11*i+ 1]  =  t[0] >>  8;
    r[11*i+ 2]  =  t[0] >> 16;
    r[11*i+ 2] |=  t[1] <<  6;
    r[11*i+ 3]  =  t[1] >>  2;
    r[11*i+ 4]  =  t[1] >> 10;
    r[11*i+ 5]  =  t[1] >> 18;
    r[11*i+ 5] |=  t[2] <<  4;
    r[11*i+ 6]  =  t[2] >>  4;
    r[11*i+ 7]  =  t[2] >> 12;
    r[11*i+ 8]  =  t[2] >> 20;
    r[11*i+ 8] |=  t[3] <<  2;
    r[11*i+ 9]  =  t[3] >>  6;
    r[11*i+10]  =  t[3] >> 14;
  }
#elif GAMMA1 == (1 << 22)
  for(unsigned i = 0; i < N/8; ++i) {
    uint32_t t[8];
    t[0] = (uint32_t)(GAMMA1 - a->coeffs[8*i+0]) & 0x7FFFFFu;
    t[1] = (uint32_t)(GAMMA1 - a->coeffs[8*i+1]) & 0x7FFFFFu;
    t[2] = (uint32_t)(GAMMA1 - a->coeffs[8*i+2]) & 0x7FFFFFu;
    t[3] = (uint32_t)(GAMMA1 - a->coeffs[8*i+3]) & 0x7FFFFFu;
    t[4] = (uint32_t)(GAMMA1 - a->coeffs[8*i+4]) & 0x7FFFFFu;
    t[5] = (uint32_t)(GAMMA1 - a->coeffs[8*i+5]) & 0x7FFFFFu;
    t[6] = (uint32_t)(GAMMA1 - a->coeffs[8*i+6]) & 0x7FFFFFu;
    t[7] = (uint32_t)(GAMMA1 - a->coeffs[8*i+7]) & 0x7FFFFFu;

    r[23*i+ 0]  =  t[0];
    r[23*i+ 1]  =  t[0] >>  8;
    r[23*i+ 2]  =  t[0] >> 16;
    r[23*i+ 2] |=  t[1] <<  7;
    r[23*i+ 3]  =  t[1] >>  1;
    r[23*i+ 4]  =  t[1] >>  9;
    r[23*i+ 5]  =  t[1] >> 17;
    r[23*i+ 5] |=  t[2] <<  6;
    r[23*i+ 6]  =  t[2] >>  2;
    r[23*i+ 7]  =  t[2] >> 10;
    r[23*i+ 8]  =  t[2] >> 18;
    r[23*i+ 8] |=  t[3] <<  5;
    r[23*i+ 9]  =  t[3] >>  3;
    r[23*i+10]  =  t[3] >> 11;
    r[23*i+11]  =  t[3] >> 19;
    r[23*i+11] |=  t[4] <<  4;
    r[23*i+12]  =  t[4] >>  4;
    r[23*i+13]  =  t[4] >> 12;
    r[23*i+14]  =  t[4] >> 20;
    r[23*i+14] |=  t[5] <<  3;
    r[23*i+15]  =  t[5] >>  5;
    r[23*i+16]  =  t[5] >> 13;
    r[23*i+17]  =  t[5] >> 21;
    r[23*i+17] |=  t[6] <<  2;
    r[23*i+18]  =  t[6] >>  6;
    r[23*i+19]  =  t[6] >> 14;
    r[23*i+20]  =  t[6] >> 22;
    r[23*i+20] |=  t[7] <<  1;
    r[23*i+21]  =  t[7] >>  7;
    r[23*i+22]  =  t[7] >> 15;
  }
#endif
}

/* Unpack the response polynomial z in the signature. */
void polyz_unpack(poly *r, const uint8_t *a) {

#if GAMMA1 == (1 << 20)
  for(unsigned i = 0; i < N/8; ++i) {
    r->coeffs[8*i+0]  = (uint32_t)a[21*i+0];
    r->coeffs[8*i+0] |= (uint32_t)a[21*i+1] << 8;
    r->coeffs[8*i+0] |= (uint32_t)a[21*i+2] << 16;
    r->coeffs[8*i+0] &= 0x1FFFFF;

    r->coeffs[8*i+1]  = (uint32_t)a[21*i+2] >> 5;
    r->coeffs[8*i+1] |= (uint32_t)a[21*i+3] << 3;
    r->coeffs[8*i+1] |= (uint32_t)a[21*i+4] << 11;
    r->coeffs[8*i+1] |= (uint32_t)a[21*i+5] << 19;
    r->coeffs[8*i+1] &= 0x1FFFFF;

    r->coeffs[8*i+2]  = (uint32_t)a[21*i+5] >> 2;
    r->coeffs[8*i+2] |= (uint32_t)a[21*i+6] << 6;
    r->coeffs[8*i+2] |= (uint32_t)a[21*i+7] << 14;
    r->coeffs[8*i+2] &= 0x1FFFFF;

    r->coeffs[8*i+3]  = (uint32_t)a[21*i+7] >> 7;
    r->coeffs[8*i+3] |= (uint32_t)a[21*i+8] << 1;
    r->coeffs[8*i+3] |= (uint32_t)a[21*i+9] << 9;
    r->coeffs[8*i+3] |= (uint32_t)a[21*i+10] << 17;
    r->coeffs[8*i+3] &= 0x1FFFFF;

    r->coeffs[8*i+4]  = (uint32_t)a[21*i+10] >> 4;
    r->coeffs[8*i+4] |= (uint32_t)a[21*i+11] << 4;
    r->coeffs[8*i+4] |= (uint32_t)a[21*i+12] << 12;
    r->coeffs[8*i+4] |= (uint32_t)a[21*i+13] << 20;
    r->coeffs[8*i+4] &= 0x1FFFFF;

    r->coeffs[8*i+5]  = (uint32_t)a[21*i+13] >> 1;
    r->coeffs[8*i+5] |= (uint32_t)a[21*i+14] << 7;
    r->coeffs[8*i+5] |= (uint32_t)a[21*i+15] << 15;
    r->coeffs[8*i+5] &= 0x1FFFFF;

    r->coeffs[8*i+6]  = (uint32_t)a[21*i+15] >> 6;
    r->coeffs[8*i+6] |= (uint32_t)a[21*i+16] << 2;
    r->coeffs[8*i+6] |= (uint32_t)a[21*i+17] << 10;
    r->coeffs[8*i+6] |= (uint32_t)a[21*i+18] << 18;
    r->coeffs[8*i+6] &= 0x1FFFFF;

    r->coeffs[8*i+7]  = (uint32_t)a[21*i+18] >> 3;
    r->coeffs[8*i+7] |= (uint32_t)a[21*i+19] << 5;
    r->coeffs[8*i+7] |= (uint32_t)a[21*i+20] << 13;
    r->coeffs[8*i+7] &= 0x1FFFFF;

    r->coeffs[8*i+0] = (int32_t)GAMMA1 - r->coeffs[8*i+0];
    r->coeffs[8*i+1] = (int32_t)GAMMA1 - r->coeffs[8*i+1];
    r->coeffs[8*i+2] = (int32_t)GAMMA1 - r->coeffs[8*i+2];
    r->coeffs[8*i+3] = (int32_t)GAMMA1 - r->coeffs[8*i+3];
    r->coeffs[8*i+4] = (int32_t)GAMMA1 - r->coeffs[8*i+4];
    r->coeffs[8*i+5] = (int32_t)GAMMA1 - r->coeffs[8*i+5];
    r->coeffs[8*i+6] = (int32_t)GAMMA1 - r->coeffs[8*i+6];
    r->coeffs[8*i+7] = (int32_t)GAMMA1 - r->coeffs[8*i+7];
  }

#elif GAMMA1 == (1 << 21)
  for(unsigned i = 0; i < N/4; ++i) {
    r->coeffs[4*i+0]  = a[11*i+0];
    r->coeffs[4*i+0] |= (uint32_t)a[11*i+1] << 8;
    r->coeffs[4*i+0] |= (uint32_t)a[11*i+2] << 16;
    r->coeffs[4*i+0] &= 0x3FFFFF;

    r->coeffs[4*i+1]  = a[11*i+2] >> 6;
    r->coeffs[4*i+1] |= (uint32_t)a[11*i+3] << 2;
    r->coeffs[4*i+1] |= (uint32_t)a[11*i+4] << 10;
    r->coeffs[4*i+1] |= (uint32_t)a[11*i+5] << 18;
    r->coeffs[4*i+1] &= 0x3FFFFF;

    r->coeffs[4*i+2]  = a[11*i+5] >> 4;
    r->coeffs[4*i+2] |= (uint32_t)a[11*i+6] << 4;
    r->coeffs[4*i+2] |= (uint32_t)a[11*i+7] << 12;
    r->coeffs[4*i+2] |= (uint32_t)a[11*i+8] << 20;
    r->coeffs[4*i+2] &= 0x3FFFFF;

    r->coeffs[4*i+3]  = a[11*i+8] >> 2;
    r->coeffs[4*i+3] |= (uint32_t)a[11*i+9] << 6;
    r->coeffs[4*i+3] |= (uint32_t)a[11*i+10] << 14;
    r->coeffs[4*i+3] &= 0x3FFFFF;

    r->coeffs[4*i+0] = (int32_t)GAMMA1 - r->coeffs[4*i+0];
    r->coeffs[4*i+1] = (int32_t)GAMMA1 - r->coeffs[4*i+1];
    r->coeffs[4*i+2] = (int32_t)GAMMA1 - r->coeffs[4*i+2];
    r->coeffs[4*i+3] = (int32_t)GAMMA1 - r->coeffs[4*i+3];
  }
#elif GAMMA1 == (1 << 22)
  for(unsigned i = 0; i < N/8; ++i) {
    r->coeffs[8*i+0]  = (uint32_t)a[23*i+0];
    r->coeffs[8*i+0] |= (uint32_t)a[23*i+1] << 8;
    r->coeffs[8*i+0] |= (uint32_t)a[23*i+2] << 16;
    r->coeffs[8*i+0] &= 0x7FFFFF;

    r->coeffs[8*i+1]  = (uint32_t)a[23*i+2] >> 7;
    r->coeffs[8*i+1] |= (uint32_t)a[23*i+3] << 1;
    r->coeffs[8*i+1] |= (uint32_t)a[23*i+4] << 9;
    r->coeffs[8*i+1] |= (uint32_t)a[23*i+5] << 17;
    r->coeffs[8*i+1] &= 0x7FFFFF;

    r->coeffs[8*i+2]  = (uint32_t)a[23*i+5] >> 6;
    r->coeffs[8*i+2] |= (uint32_t)a[23*i+6] << 2;
    r->coeffs[8*i+2] |= (uint32_t)a[23*i+7] << 10;
    r->coeffs[8*i+2] |= (uint32_t)a[23*i+8] << 18;
    r->coeffs[8*i+2] &= 0x7FFFFF;

    r->coeffs[8*i+3]  = (uint32_t)a[23*i+8] >> 5;
    r->coeffs[8*i+3] |= (uint32_t)a[23*i+9] << 3;
    r->coeffs[8*i+3] |= (uint32_t)a[23*i+10] << 11;
    r->coeffs[8*i+3] |= (uint32_t)a[23*i+11] << 19;
    r->coeffs[8*i+3] &= 0x7FFFFF;

    r->coeffs[8*i+4]  = (uint32_t)a[23*i+11] >> 4;
    r->coeffs[8*i+4] |= (uint32_t)a[23*i+12] << 4;
    r->coeffs[8*i+4] |= (uint32_t)a[23*i+13] << 12;
    r->coeffs[8*i+4] |= (uint32_t)a[23*i+14] << 20;
    r->coeffs[8*i+4] &= 0x7FFFFF;

    r->coeffs[8*i+5]  = (uint32_t)a[23*i+14] >> 3;
    r->coeffs[8*i+5] |= (uint32_t)a[23*i+15] << 5;
    r->coeffs[8*i+5] |= (uint32_t)a[23*i+16] << 13;
    r->coeffs[8*i+5] |= (uint32_t)a[23*i+17] << 21;
    r->coeffs[8*i+5] &= 0x7FFFFF;

    r->coeffs[8*i+6]  = (uint32_t)a[23*i+17] >> 2;
    r->coeffs[8*i+6] |= (uint32_t)a[23*i+18] << 6;
    r->coeffs[8*i+6] |= (uint32_t)a[23*i+19] << 14;
    r->coeffs[8*i+6] |= (uint32_t)a[23*i+20] << 22;
    r->coeffs[8*i+6] &= 0x7FFFFF;

    r->coeffs[8*i+7]  = (uint32_t)a[23*i+20] >> 1;
    r->coeffs[8*i+7] |= (uint32_t)a[23*i+21] << 7;
    r->coeffs[8*i+7] |= (uint32_t)a[23*i+22] << 15;
    r->coeffs[8*i+7] &= 0x7FFFFF;

    r->coeffs[8*i+0] = (int32_t)GAMMA1 - r->coeffs[8*i+0];
    r->coeffs[8*i+1] = (int32_t)GAMMA1 - r->coeffs[8*i+1];
    r->coeffs[8*i+2] = (int32_t)GAMMA1 - r->coeffs[8*i+2];
    r->coeffs[8*i+3] = (int32_t)GAMMA1 - r->coeffs[8*i+3];
    r->coeffs[8*i+4] = (int32_t)GAMMA1 - r->coeffs[8*i+4];
    r->coeffs[8*i+5] = (int32_t)GAMMA1 - r->coeffs[8*i+5];
    r->coeffs[8*i+6] = (int32_t)GAMMA1 - r->coeffs[8*i+6];
    r->coeffs[8*i+7] = (int32_t)GAMMA1 - r->coeffs[8*i+7];
  }

#endif
}

/* Pack the decomposed high bits w1 used in the challenge hash. */
void polyw1_pack(uint8_t *r, const poly *a) {

#if GAMMA2 == (Q-1)/32
  for(unsigned i = 0; i < N/2; ++i)
    r[i] = (uint8_t)(a->coeffs[2*i+0] | (a->coeffs[2*i+1] << 4));

#elif GAMMA2 == (Q-1)/48
  for(unsigned i = 0; i < N/8; ++i) {
    r[5*i+0] = a->coeffs[8*i+0] | (a->coeffs[8*i+1] << 5);
    r[5*i+1] = (a->coeffs[8*i+1] >> 3) | (a->coeffs[8*i+2] << 2) | (a->coeffs[8*i+3] << 7);
    r[5*i+2] = (a->coeffs[8*i+3] >> 1) | (a->coeffs[8*i+4] << 4);
    r[5*i+3] = (a->coeffs[8*i+4] >> 4) | (a->coeffs[8*i+5] << 1) | (a->coeffs[8*i+6] << 6);
    r[5*i+4] = (a->coeffs[8*i+6] >> 2) | (a->coeffs[8*i+7] << 3);
  }
    
#elif GAMMA2 == (Q-1)/96
  for(unsigned i = 0; i < N/4; ++i) {
    r[3*i+0] = a->coeffs[4*i+0] | (a->coeffs[4*i+1] << 6);
    r[3*i+1] = (a->coeffs[4*i+1] >> 2) | (a->coeffs[4*i+2] << 4);
    r[3*i+2] = (a->coeffs[4*i+2] >> 4) | (a->coeffs[4*i+3] << 2);
  }
#endif
}
