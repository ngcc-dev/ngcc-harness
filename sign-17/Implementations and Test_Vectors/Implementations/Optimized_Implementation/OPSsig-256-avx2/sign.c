#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "align.h"
#include "auxfunc.h"
#include "params.h"
#include "sign.h"
#include "packing.h"
#include "polyvec.h"
#include "poly.h"
#include "drng.h"

extern DRNG_ctx drng_algorithm;

int randombytes(unsigned char *buf, unsigned long long len)
{
  return get_random_number(&drng_algorithm, buf, len * 8ULL);
}

/* Byte-oriented wrapper around the ICCS auxiliary XOF. */
static void crh(uint8_t *out, size_t outlen, const uint8_t *in, size_t inlen)
{
  pseudohash((unsigned long long)outlen * 8ULL,
             in,
             (unsigned long long)inlen * 8ULL,
             out);
}

static inline void polyvec_matrix_expand_row(polyvecl **row,
                                             polyvecl buf[2],
                                             const uint8_t rho[SEEDBYTES],
                                             unsigned int i)
{
  switch(i) {
    case 0:
      polyvec_matrix_expand_row0(buf, buf + 1, rho);
      *row = buf;
      break;
    case 1:
      polyvec_matrix_expand_row1(buf + 1, buf, rho);
      *row = buf + 1;
      break;
    case 2:
      polyvec_matrix_expand_row2(buf, buf + 1, rho);
      *row = buf;
      break;
    default:
      polyvec_matrix_expand_row3(buf + 1, buf, rho);
      *row = buf + 1;
      break;
  }
}

int crypto_sign_keypair(uint8_t *pk, uint8_t *sk)
{
  unsigned int i;
  uint8_t rho[SEEDBYTES];
  uint8_t rhoprime[CRHBYTES];
  uint8_t tr[CRHBYTES];
  polyvecl rowbuf[2];
  polyvecl s1, s1hat, *row = rowbuf;
  polyveck s2, t1, t0;

  randombytes(rho, SEEDBYTES);
  randombytes(rhoprime, CRHBYTES);

  poly_uniform_eta_4x(&s1.vec[0], &s1.vec[1], &s1.vec[2], &s1.vec[3],
                      rhoprime, 0, 1, 2, 3);
  poly_uniform_eta_4x(&s2.vec[0], &s2.vec[1], &s2.vec[2], &s2.vec[3],
                      rhoprime, 4, 5, 6, 7);

  s1hat = s1;
  polyvecl_ntt(&s1hat);

  for(i = 0; i < K; ++i) {
    polyvec_matrix_expand_row(&row, rowbuf, rho, i);
    polyvecl_pointwise_acc_montgomery(&t1.vec[i], row, &s1hat);
    poly_reduce(&t1.vec[i]);
    poly_invntt_tomont(&t1.vec[i]);
    poly_add(&t1.vec[i], &t1.vec[i], &s2.vec[i]);
    poly_reduce(&t1.vec[i]);
    poly_caddq(&t1.vec[i]);
    poly_power2round(&t1.vec[i], &t0.vec[i], &t1.vec[i]);
  }

  pack_pk(pk, rho, &t1);
  crh(tr, CRHBYTES, pk, CRYPTO_PUBLICKEYBYTES);
  pack_sk(sk, rho, tr, &t0, &s1, &s2);

  return 0;
}

static int crypto_sign_signature_internal(uint8_t *sig,
                                          size_t *siglen,
                                          const uint8_t *m,
                                          size_t mlen,
                                          const uint8_t *pre,
                                          size_t prelen,
                                          const uint8_t *sk)
{
  unsigned int i, n;
  uint8_t rho[SEEDBYTES];
  uint8_t tr[CRHBYTES];
  uint8_t mu[CRHBYTES];
  uint8_t rhoprime[CRHBYTES];
  uint8_t ctmp[CRHBYTES + K*POLYW1_PACKEDBYTES];
  uint16_t nonce = 0;
  polyvecl mat[K], s1, z;
  polyveck t0, s2, w1, h;
  poly c, tmp;
  union {
    polyvecl y;
    polyveck w0;
  } tmpv;

  unpack_sk(rho, tr, &t0, &s1, &s2, sk);

  {
    unsigned long long inlen = CRHBYTES + prelen + mlen;
    uint8_t *buf = (uint8_t *)malloc(inlen);
    unsigned long long off = 0;

    if(buf == NULL)
      return -1;

    memcpy(buf + off, tr, CRHBYTES);
    off += CRHBYTES;
    memcpy(buf + off, pre, prelen);
    off += prelen;
    memcpy(buf + off, m, mlen);
    crh(mu, CRHBYTES, buf, inlen);
    free(buf);
  }

  randombytes(rhoprime, CRHBYTES);

  polyvec_matrix_expand(mat, rho);
  polyvecl_ntt(&s1);
  polyveck_ntt(&s2);
  polyveck_ntt(&t0);

rej:
  poly_uniform_gamma1_4x(&z.vec[0], &z.vec[1], &z.vec[2], &z.vec[3],
                         rhoprime,
                         (uint16_t)(L*nonce),
                         (uint16_t)(L*nonce + 1),
                         (uint16_t)(L*nonce + 2),
                         (uint16_t)(L*nonce + 3));
  nonce += 1;

  tmpv.y = z;
  polyvecl_ntt(&tmpv.y);
  polyvec_matrix_pointwise_montgomery(&w1, mat, &tmpv.y);
  polyveck_reduce(&w1);
  polyveck_invntt_tomont(&w1);

  polyveck_caddq(&w1);
  polyveck_decompose(&w1, &tmpv.w0, &w1);
  polyveck_pack_w1(sig, &w1);

  memcpy(ctmp, mu, CRHBYTES);
  memcpy(ctmp + CRHBYTES, sig, K*POLYW1_PACKEDBYTES);
  pseudohash((unsigned long long)CTILDEBYTES * 8ULL,
             ctmp,
             (unsigned long long)sizeof(ctmp) * 8ULL,
             sig);
  poly_challenge(&c, sig);
  poly_ntt(&c);

  for(i = 0; i < K; ++i) {
    poly_pointwise_montgomery(&tmp, &c, &s2.vec[i]);
    poly_invntt_tomont(&tmp);
    poly_sub(&tmpv.w0.vec[i], &tmpv.w0.vec[i], &tmp);
    poly_reduce(&tmpv.w0.vec[i]);
    if(poly_chknorm(&tmpv.w0.vec[i], GAMMA2 - BETA))
      goto rej;
  }

  for(i = 0; i < L; ++i) {
    poly_pointwise_montgomery(&tmp, &c, &s1.vec[i]);
    poly_invntt_tomont(&tmp);
    poly_add(&z.vec[i], &z.vec[i], &tmp);
    poly_reduce(&z.vec[i]);
    if(poly_chknorm(&z.vec[i], GAMMA1 - BETA))
      goto rej;
  }

  for(i = 0; i < K; ++i) {
    poly_pointwise_montgomery(&tmp, &c, &t0.vec[i]);
    poly_invntt_tomont(&tmp);
    poly_reduce(&tmp);
    if(poly_chknorm(&tmp, GAMMA2))
      goto rej;

    poly_add(&tmpv.w0.vec[i], &tmpv.w0.vec[i], &tmp);
  }

  n = polyveck_make_hint(&h, &tmpv.w0, &w1);
  if(n > OMEGA)
    goto rej;

  pack_sig(sig, sig, &z, &h);
  *siglen = CRYPTO_BYTES;
  return 0;
}

int crypto_sign_signature(uint8_t *sig,
                          size_t *siglen,
                          const uint8_t *m,
                          size_t mlen,
                          const uint8_t *ctx,
                          size_t ctxlen,
                          const uint8_t *sk)
{
  uint8_t pre[257];

  if(ctxlen > 255)
    return -1;

  pre[0] = 0;
  pre[1] = (uint8_t)ctxlen;
  memcpy(&pre[2], ctx, ctxlen);

  return crypto_sign_signature_internal(sig, siglen, m, mlen, pre, 2 + ctxlen, sk);
}

static int crypto_sign_verify_internal(const uint8_t *sig,
                                       size_t siglen,
                                       const uint8_t *m,
                                       size_t mlen,
                                       const uint8_t *pre,
                                       size_t prelen,
                                       const uint8_t *pk)
{
  ALIGNED_UINT8(K*POLYW1_PACKEDBYTES + 14) buf;
  unsigned int i;
  uint8_t rho[SEEDBYTES];
  uint8_t tr[CRHBYTES];
  uint8_t mu[CRHBYTES];
  uint8_t cseed[CTILDEBYTES];
  polyvecl mat[K];
  polyvecl z;
  polyveck t1, w1, h;
  poly c;

  if(siglen != CRYPTO_BYTES)
    return -1;

  unpack_pk(rho, &t1, pk);
  if(unpack_sig(cseed, &z, &h, sig))
    return -1;
  if(polyvecl_chknorm(&z, GAMMA1 - BETA))
    return -1;

  crh(tr, CRHBYTES, pk, CRYPTO_PUBLICKEYBYTES);
  {
    unsigned long long inlen = CRHBYTES + prelen + mlen;
    uint8_t *msgbuf = (uint8_t *)malloc(inlen);
    unsigned long long off = 0;

    if(msgbuf == NULL)
      return -1;

    memcpy(msgbuf + off, tr, CRHBYTES);
    off += CRHBYTES;
    memcpy(msgbuf + off, pre, prelen);
    off += prelen;
    memcpy(msgbuf + off, m, mlen);
    crh(mu, CRHBYTES, msgbuf, inlen);
    free(msgbuf);
  }

  poly_challenge(&c, cseed);
  poly_ntt(&c);

  polyvec_matrix_expand(mat, rho);
  polyvecl_ntt(&z);
  polyvec_matrix_pointwise_montgomery(&w1, mat, &z);
  polyveck_verify_batch(&w1, &t1, &c, &h);
  polyveck_pack_w1(buf.coeffs, &w1);

  {
    uint8_t ccheck[CTILDEBYTES];
    uint8_t ctmp[CRHBYTES + K*POLYW1_PACKEDBYTES];

    memcpy(ctmp, mu, CRHBYTES);
    memcpy(ctmp + CRHBYTES, buf.coeffs, K*POLYW1_PACKEDBYTES);
    pseudohash((unsigned long long)CTILDEBYTES * 8ULL,
               ctmp,
               (unsigned long long)sizeof(ctmp) * 8ULL,
               ccheck);

    for(i = 0; i < CTILDEBYTES; ++i)
      if(ccheck[i] != cseed[i])
        return -1;
  }

  return 0;
}

int crypto_sign_verify(uint8_t *sig,
                       size_t siglen,
                       const uint8_t *m,
                       size_t mlen,
                       const uint8_t *ctx,
                       size_t ctxlen,
                       const uint8_t *pk)
{
  uint8_t pre[257];

  if(ctxlen > 255)
    return -1;

  pre[0] = 0;
  pre[1] = (uint8_t)ctxlen;
  memcpy(&pre[2], ctx, ctxlen);

  return crypto_sign_verify_internal(sig, siglen, m, mlen, pre, 2 + ctxlen, pk);
}
