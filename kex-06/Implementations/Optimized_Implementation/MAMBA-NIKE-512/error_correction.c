#include "error_correction.h"

//See paper for details on the error reconciliation

static int32_t i32_abs(int32_t v)
{
  int32_t mask = v >> 31;
  return (v ^ mask) - mask;
}


static int32_t f(int32_t *v0, int32_t *v1, int32_t x)
{
  int32_t xit, t, r;

  /* For power-of-2 q: t = x / q = x >> LOG2Q (exact for x >= 0) */
  t = x >> LOG2Q;

  r = t & 1;
  xit = (t>>1);
  *v0 = xit+r; // v0 = round(x/(2*PARAM_Q))

  t -= 1;
  r = t & 1;
  *v1 = (t>>1)+r;

  return i32_abs(x-((*v0)*2*PARAM_Q));
}

static int32_t g(int32_t x)
{
  int32_t t,c;

  /* For power-of-2 q: t = x / (4*q) = x >> (LOG2Q + 2) */
  t = x >> (LOG2Q + 2);

  c = t & 1;
  t = (t >> 1) + c; // t = round(x/(8*PARAM_Q))

  t *= 8*PARAM_Q;

  return i32_abs(t - x);
}


static int16_t LDDecode(int32_t xi0, int32_t xi1, int32_t xi2, int32_t xi3)
{
  int32_t t;

  t  = g(xi0);
  t += g(xi1);
  t += g(xi2);
  t += g(xi3);

  t -= 8*PARAM_Q;
  t >>= 31;
  return t&1;
}


void helprec(poly *c, const poly *v, const unsigned char *seed, unsigned char nonce)
{
  int32_t v0[4], v1[4], v_tmp[4], k;
  unsigned char rbit;
  unsigned char rand[NIKE_KEYBYTES];
  unsigned char n[8];
  int i;
  int group = PARAM_N / 4;  /* quarter size; only the first NIKE_RECGROUPS groups are reconciled */

  for(i=0;i<7;i++)
    n[i] = 0;
  n[7] = nonce;

  crypto_stream_chacha20(rand, NIKE_KEYBYTES, n, seed);
 
  for(i=0; i<NIKE_RECGROUPS; i++)
  {
    rbit = (rand[i>>3] >> (i&7)) & 1;

    k  = f(v0+0, v1+0, 8*v->coeffs[0*group + i] + 4*rbit);
    k += f(v0+1, v1+1, 8*v->coeffs[1*group + i] + 4*rbit);
    k += f(v0+2, v1+2, 8*v->coeffs[2*group + i] + 4*rbit);
    k += f(v0+3, v1+3, 8*v->coeffs[3*group + i] + 4*rbit);

    k = (2*PARAM_Q-1-k) >> 31;

    v_tmp[0] = ((~k) & v0[0]) ^ (k & v1[0]);
    v_tmp[1] = ((~k) & v0[1]) ^ (k & v1[1]);
    v_tmp[2] = ((~k) & v0[2]) ^ (k & v1[2]);
    v_tmp[3] = ((~k) & v0[3]) ^ (k & v1[3]);

    c->coeffs[4*i + 0] = (v_tmp[0] -   v_tmp[3]) & 3;
    c->coeffs[4*i + 1] = (v_tmp[1] -   v_tmp[3]) & 3;
    c->coeffs[4*i + 2] = (v_tmp[2] -   v_tmp[3]) & 3;
    c->coeffs[4*i + 3] = (   -k    + 2*v_tmp[3]) & 3;
  }

  for(i=0;i<NIKE_KEYBYTES;i++)
    rand[i] = 0;
}


void rec(unsigned char *key, const poly *v, const poly *c)
{
  int i;
  int32_t tmp[4];
  int group = PARAM_N / 4;  /* quarter size; only the first NIKE_RECGROUPS groups are reconciled */

  for(i=0;i<NIKE_KEYBYTES;i++)
    key[i] = 0;

  for(i=0; i<NIKE_RECGROUPS; i++)
  {
    tmp[0] = 16*PARAM_Q + 8*(int32_t)v->coeffs[0*group + i] - PARAM_Q * (2*c->coeffs[4*i + 0]+c->coeffs[4*i + 3]);
    tmp[1] = 16*PARAM_Q + 8*(int32_t)v->coeffs[1*group + i] - PARAM_Q * (2*c->coeffs[4*i + 1]+c->coeffs[4*i + 3]);
    tmp[2] = 16*PARAM_Q + 8*(int32_t)v->coeffs[2*group + i] - PARAM_Q * (2*c->coeffs[4*i + 2]+c->coeffs[4*i + 3]);
    tmp[3] = 16*PARAM_Q + 8*(int32_t)v->coeffs[3*group + i] - PARAM_Q * (                    c->coeffs[4*i + 3]);

    key[i>>3] |= LDDecode(tmp[0], tmp[1], tmp[2], tmp[3]) << (i & 7);
  }
}
