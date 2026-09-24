#include "poly.h"
#include "randombytes.h"
#include "error_correction.h"
#include "fips202.h"
#include <string.h>

static void secure_zero(void *p, unsigned long long n)
{
  volatile unsigned char *v = (volatile unsigned char *)p;
  while(n--)
    *v++ = 0;
}

static void poly_pack_bits(unsigned char *out, const poly *p, unsigned int bits)
{
  unsigned int acc = 0;
  unsigned int accbits = 0;
  unsigned int outpos = 0;
  unsigned int mask = (1u << bits) - 1u;
  unsigned int i;

  for(i=0;i<NIKE_PACKEDPOLYBYTES(bits);i++)
    out[i] = 0;

  for(i=0;i<PARAM_N;i++)
  {
    acc |= ((unsigned int)p->coeffs[i] & mask) << accbits;
    accbits += bits;
    while(accbits >= 8)
    {
      out[outpos++] = (unsigned char)(acc & 0xff);
      acc >>= 8;
      accbits -= 8;
    }
  }

  if(accbits)
    out[outpos] = (unsigned char)(acc & 0xff);
}

static void poly_unpack_bits(poly *p, const unsigned char *in, unsigned int bits)
{
  unsigned int acc = 0;
  unsigned int accbits = 0;
  unsigned int inpos = 0;
  unsigned int mask = (1u << bits) - 1u;
  unsigned int i;

  for(i=0;i<PARAM_N;i++)
  {
    while(accbits < bits)
    {
      acc |= ((unsigned int)in[inpos++]) << accbits;
      accbits += 8;
    }
    p->coeffs[i] = (uint16_t)(acc & mask);
    acc >>= bits;
    accbits -= bits;
  }
}

static void encode_a(unsigned char *r, const poly *pk, const unsigned char *seed)
{
  int i;
  poly_pack_bits(r, pk, PARAM_T_PK);
  for(i=0;i<NIKE_SEEDBYTES;i++)
    r[NIKE_PKPOLYBYTES+i] = seed[i];
}

static void decode_a(poly *pk, unsigned char *seed, const unsigned char *r)
{
  int i;
  poly_unpack_bits(pk, r, PARAM_T_PK);
  for(i=0;i<NIKE_SEEDBYTES;i++)
    seed[i] = r[NIKE_PKPOLYBYTES+i];
}

static void encode_b(unsigned char *r, const unsigned char *mu, const poly *b, const poly *c)
{
  int i;
  for(i=0;i<NIKE_SEEDBYTES;i++)
    r[i] = mu[i];
  poly_pack_bits(r + NIKE_SEEDBYTES, b, PARAM_T_U);
  for(i=0;i<NIKE_RECBYTES;i++)
    r[NIKE_SEEDBYTES + NIKE_UPOLYBYTES+i] = c->coeffs[4*i] | (c->coeffs[4*i+1] << 2) | (c->coeffs[4*i+2] << 4) | (c->coeffs[4*i+3] << 6);
}

static void decode_b(unsigned char *mu, poly *b, poly *c, const unsigned char *r)
{
  int i;
  for(i=0;i<NIKE_SEEDBYTES;i++)
    mu[i] = r[i];
  poly_unpack_bits(b, r + NIKE_SEEDBYTES, PARAM_T_U);
  for(i=0;i<NIKE_RECBYTES;i++)
  {
    c->coeffs[4*i+0] =  r[NIKE_SEEDBYTES + NIKE_UPOLYBYTES+i]       & 0x03;
    c->coeffs[4*i+1] = (r[NIKE_SEEDBYTES + NIKE_UPOLYBYTES+i] >> 2) & 0x03;
    c->coeffs[4*i+2] = (r[NIKE_SEEDBYTES + NIKE_UPOLYBYTES+i] >> 4) & 0x03;
    c->coeffs[4*i+3] = (r[NIKE_SEEDBYTES + NIKE_UPOLYBYTES+i] >> 6);
  }
}

static void gen_a(poly *a, const unsigned char *seed)
{
    poly_uniform(a,seed);
}

static void poly_dither(poly *r, const unsigned char *seed, unsigned char domain, unsigned int delta)
{
  unsigned char extseed[NIKE_SEEDBYTES + 1];
  unsigned char buf[SHAKE128_RATE];
  uint64_t state[25];
  unsigned int mask = delta - 1;
  unsigned int pos = SHAKE128_RATE;
  int i;

  extseed[0] = domain;
  for(i=0;i<NIKE_SEEDBYTES;i++)
    extseed[i+1] = seed[i];

  shake128_absorb(state, extseed, sizeof(extseed));
  for(i=0;i<PARAM_N;i++)
  {
    if(pos > SHAKE128_RATE - 2)
    {
      shake128_squeezeblocks(buf, 1, state);
      pos = 0;
    }
    r->coeffs[i] = (uint16_t)((buf[pos] | ((uint16_t)buf[pos+1] << 8)) & mask);
    pos += 2;
  }
}

static void gen_public(poly *a, poly *dpk, const unsigned char *seed)
{
  gen_a(a, seed);
  poly_dither(dpk, seed, 0xA0, PARAM_DELTA_PK);
}

static void gen_dither(poly *du, poly *dv, const unsigned char *mu)
{
  poly_dither(du, mu, 0xB0, PARAM_DELTA_U);
  poly_dither(dv, mu, 0xB1, PARAM_DELTA_V);
}

static void poly_quantize(poly *r, const poly *a, const poly *d, unsigned int h, unsigned int p)
{
  unsigned int round = h ? (1u << (h - 1)) : 0;
  unsigned int mask = p - 1;
  int i;

  for(i=0;i<PARAM_N;i++)
    r->coeffs[i] = (uint16_t)((((uint32_t)a->coeffs[i] + d->coeffs[i] + round) >> h) & mask);
}

static void poly_dequantize(poly *r, const poly *label, const poly *d, unsigned int h)
{
  int i;

  for(i=0;i<PARAM_N;i++)
    r->coeffs[i] = (uint16_t)((((uint32_t)label->coeffs[i] << h) - d->coeffs[i]) & (PARAM_Q - 1));
}

#ifndef STATISTICAL_TEST
static void nike_kdf(unsigned char *out, const unsigned char *nu, const unsigned char *pk, const unsigned char *m1)
{
  static const unsigned char domain[NIKE_KDF_DOMAINBYTES] = {
    'M','A','M','B','A','-','N','I','K','E','-','K','D','F','2','0'
  };
  unsigned char input[NIKE_KDF_INPUTBYTES];
  unsigned int off = 0;
  int i;

  for(i=0;i<NIKE_KDF_DOMAINBYTES;i++)
    input[off++] = domain[i];
  for(i=0;i<NIKE_KEYBYTES;i++)
    input[off++] = nu[i];
  for(i=0;i<NIKE_SENDABYTES;i++)
    input[off++] = pk[i];
  for(i=0;i<NIKE_SENDBBYTES;i++)
    input[off++] = m1[i];

  shake256(out, NIKE_SSBYTES, input, off);
  secure_zero(input, sizeof(input));
}
#endif


// API FUNCTIONS 

int nike_keygen(unsigned char *send, poly *sk)
{
  poly a, dpk, r, pk;
  unsigned char seed[NIKE_SEEDBYTES];
  unsigned char noiseseed[32];

  if(send == 0 || sk == 0)
    return -1;
  if(nike_randombytes(seed, NIKE_SEEDBYTES) != 0)
    return -1;
  if(nike_randombytes(noiseseed, 32) != 0) {
    secure_zero(seed, sizeof(seed));
    return -1;
  }

  gen_public(&a, &dpk, seed);

  poly_getnoise(sk,noiseseed,0);
  poly_ntt(sk);

  poly_mul_small(&r, &a, sk);
  poly_quantize(&pk, &r, &dpk, PARAM_H_PK, PARAM_P_PK);

  encode_a(send, &pk, seed);
  secure_zero(seed, sizeof(seed));
  secure_zero(noiseseed, sizeof(noiseseed));
  secure_zero(&a, sizeof(a));
  secure_zero(&dpk, sizeof(dpk));
  secure_zero(&r, sizeof(r));
  secure_zero(&pk, sizeof(pk));
  return 0;
}


int nike_sharedb(unsigned char *sharedkey, unsigned char *send, const unsigned char *received)
{
  poly sp, v, vlabel, a, dpk, pka, pkahat, c, bp, du, dv;
  unsigned char seed[NIKE_SEEDBYTES];
  unsigned char mu[NIKE_SEEDBYTES];
  unsigned char noiseseed[32];
#ifndef STATISTICAL_TEST
  unsigned char nu[NIKE_KEYBYTES];
#endif

  if(sharedkey == 0 || send == 0 || received == 0)
    return -1;
  
  if(nike_randombytes(mu, NIKE_SEEDBYTES) != 0)
    return -1;
  if(nike_randombytes(noiseseed, 32) != 0) {
    secure_zero(mu, sizeof(mu));
    return -1;
  }

  decode_a(&pka, seed, received);
  gen_public(&a, &dpk, seed);
  gen_dither(&du, &dv, mu);
  poly_dequantize(&pkahat, &pka, &dpk, PARAM_H_PK);

  poly_getnoise(&sp,noiseseed,0);
  poly_ntt(&sp);

  poly_mul_small(&bp, &a, &sp);
  poly_quantize(&bp, &bp, &du, PARAM_H_U, PARAM_P_U);
  
  poly_mul_small(&v, &pkahat, &sp);
  poly_invntt(&v);
  poly_quantize(&vlabel, &v, &dv, PARAM_H_V, PARAM_P_V);
  poly_dequantize(&v, &vlabel, &dv, PARAM_H_V);

  helprec(&c, &v, noiseseed, 3);

  encode_b(send, mu, &bp, &c);
  
#ifdef STATISTICAL_TEST
  rec(sharedkey, &v, &c);
#else
  rec(nu, &v, &c);
  nike_kdf(sharedkey, nu, received, send);
  secure_zero(nu, sizeof(nu));
#endif
  secure_zero(seed, sizeof(seed));
  secure_zero(mu, sizeof(mu));
  secure_zero(noiseseed, sizeof(noiseseed));
  secure_zero(&sp, sizeof(sp));
  secure_zero(&v, sizeof(v));
  secure_zero(&vlabel, sizeof(vlabel));
  secure_zero(&a, sizeof(a));
  secure_zero(&dpk, sizeof(dpk));
  secure_zero(&pka, sizeof(pka));
  secure_zero(&pkahat, sizeof(pkahat));
  secure_zero(&c, sizeof(c));
  secure_zero(&bp, sizeof(bp));
  secure_zero(&du, sizeof(du));
  secure_zero(&dv, sizeof(dv));
  return 0;
}


int nike_shareda(unsigned char *sharedkey, const poly *sk, const unsigned char *pk, const unsigned char *received)
{
  poly v,bp, c, du, dv;
  unsigned char mu[NIKE_SEEDBYTES];
#ifndef STATISTICAL_TEST
  unsigned char nu[NIKE_KEYBYTES];
#endif

  if(sharedkey == 0 || sk == 0 || pk == 0 || received == 0)
    return -1;

  decode_b(mu, &bp, &c, received);
  gen_dither(&du, &dv, mu);
  (void)dv;
  poly_dequantize(&bp, &bp, &du, PARAM_H_U);

  poly_mul_small(&v, &bp, sk);
  poly_invntt(&v);
 
#ifdef STATISTICAL_TEST
  (void)pk;
  rec(sharedkey, &v, &c);
#else
  rec(nu, &v, &c);
  nike_kdf(sharedkey, nu, pk, received);
  secure_zero(nu, sizeof(nu));
#endif
  secure_zero(&v, sizeof(v));
  secure_zero(&bp, sizeof(bp));
  secure_zero(&c, sizeof(c));
  secure_zero(&du, sizeof(du));
  secure_zero(&dv, sizeof(dv));
  secure_zero(mu, sizeof(mu));
  return 0;
}
