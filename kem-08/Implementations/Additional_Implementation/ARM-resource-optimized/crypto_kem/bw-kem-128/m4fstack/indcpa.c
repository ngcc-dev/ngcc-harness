#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "params.h"
#include "indcpa.h"
#include "polyvec.h"
#include "poly.h"
#include "ntt.h"
#include "matacc.h"
#include "symmetric.h"
#include "randombytes.h"

void indcpa_keypair_derand(uint8_t pk[KYBER_INDCPA_PUBLICKEYBYTES],
                           uint8_t sk[KYBER_INDCPA_SECRETKEYBYTES],
                           const uint8_t coins[KYBER_SYMBYTES])
{
  unsigned int i;
  uint8_t buf[2*KYBER_SYMBYTES];
  const uint8_t *publicseed = buf;
  const uint8_t *noiseseed  = buf+KYBER_SYMBYTES;
  uint8_t nonce = 0;
  polyvec skpv;
  poly pkp, ei;

  memcpy(buf, coins, KYBER_SYMBYTES);
  buf[KYBER_SYMBYTES] = KYBER_K;
  hash_g(buf, buf, KYBER_SYMBYTES+1);

  for(i=0;i<KYBER_K;i++)
    poly_getnoise_eta1(&skpv.vec[i], noiseseed, nonce++);
  polyvec_ntt(&skpv);

  for(i=0;i<KYBER_K;i++) {
    matacc(&pkp, &skpv, i, publicseed, 0);
    poly_invntt(&pkp);
    poly_getnoise_eta1(&ei, noiseseed, nonce++);
    poly_add(&pkp, &pkp, &ei);
    poly_ntt(&pkp);
    poly_tobytes(pk + i*KYBER_POLYBYTES, &pkp);
  }

  polyvec_tobytes(sk, &skpv);
  memcpy(pk + KYBER_POLYVECBYTES, publicseed, KYBER_SYMBYTES);
}

void indcpa_enc(uint8_t c[KYBER_INDCPA_BYTES],
                const uint8_t m[KYBER_INDCPA_MSGBYTES],
                const uint8_t pk[KYBER_INDCPA_PUBLICKEYBYTES],
                const uint8_t coins[KYBER_SYMBYTES])
{
  unsigned int i;
  uint8_t seed[KYBER_SYMBYTES];
  uint8_t nonce = 0;
  polyvec sp;

  memcpy(seed, pk + KYBER_POLYVECBYTES, KYBER_SYMBYTES);

  for(i=0;i<KYBER_K;i++)
    poly_getnoise_eta1(sp.vec+i, coins, nonce++);
  polyvec_ntt(&sp);

  {
    poly bp, ei;
    for(i=0;i<KYBER_K;i++) {
      matacc(&bp, &sp, i, seed, 1);
      poly_invntt(&bp);
      poly_getnoise_eta1(&ei, coins, nonce++);
      poly_add(&bp, &bp, &ei);
      poly_reduce(&bp);
      polyvec_compress_one(c, &bp, i);
    }
  }

  {
    poly v;
    {
      poly t;
      poly_frombytes(&t, pk);
      poly_basemul_opt_16_16_noprime(&v, &sp.vec[0], &t);
      for(i=1;i<KYBER_K;i++) {
        poly_frombytes(&t, pk + i*KYBER_POLYBYTES);
        poly_basemul_acc_opt_16_16_noprime(&v, &sp.vec[i], &t);
      }
    }

    poly_reduce_centered(&v);
    poly_invntt(&v);

    {
      poly epp, k;
      poly_getnoise_eta2(&epp, coins, nonce++);
      poly_frommsg(&k, m);
      poly_add(&v, &v, &epp);
      poly_add(&v, &v, &k);
    }
    poly_reduce(&v);

    poly_compress(c + KYBER_POLYVECCOMPRESSEDBYTES, &v);
  }
}

unsigned char indcpa_enc_cmp(const uint8_t c[KYBER_INDCPA_BYTES],
                             const uint8_t m[KYBER_INDCPA_MSGBYTES],
                             const uint8_t pk[KYBER_INDCPA_PUBLICKEYBYTES],
                             const uint8_t coins[KYBER_SYMBYTES])
{
  unsigned int i;
  uint8_t seed[KYBER_SYMBYTES];
  uint8_t nonce = 0;
  uint8_t rc = 0;
  polyvec sp;

  memcpy(seed, pk + KYBER_POLYVECBYTES, KYBER_SYMBYTES);

  for(i=0;i<KYBER_K;i++)
    poly_getnoise_eta1(sp.vec+i, coins, nonce++);
  polyvec_ntt(&sp);

  {
    poly bp, ei;
    for(i=0;i<KYBER_K;i++) {
      matacc(&bp, &sp, i, seed, 1);
      poly_invntt(&bp);
      poly_getnoise_eta1(&ei, coins, nonce++);
      poly_add(&bp, &bp, &ei);
      poly_reduce(&bp);
      rc |= cmp_polyvec_compress_one(c, &bp, i);
    }
  }

  {
    poly v;
    {
      poly t;
      poly_frombytes(&t, pk);
      poly_basemul_opt_16_16_noprime(&v, &sp.vec[0], &t);
      for(i=1;i<KYBER_K;i++) {
        poly_frombytes(&t, pk + i*KYBER_POLYBYTES);
        poly_basemul_acc_opt_16_16_noprime(&v, &sp.vec[i], &t);
      }
    }

    poly_reduce_centered(&v);
    poly_invntt(&v);

    {
      poly epp, k;
      poly_getnoise_eta2(&epp, coins, nonce++);
      poly_frommsg(&k, m);
      poly_add(&v, &v, &epp);
      poly_add(&v, &v, &k);
    }
    poly_reduce(&v);

    rc |= cmp_poly_compress(c + KYBER_POLYVECCOMPRESSEDBYTES, &v);
  }

  return (unsigned char)((-(uint64_t)rc) >> 63);
}

void indcpa_dec(uint8_t m[KYBER_INDCPA_MSGBYTES],
                const uint8_t c[KYBER_INDCPA_BYTES],
                const uint8_t sk[KYBER_INDCPA_SECRETKEYBYTES])
{
  poly bp, mp, v;
  unsigned int i;

  polyvec_decompress_one(&bp, c, 0);
  poly_ntt(&bp);
  poly_frombytes_mul_16_16(&mp, &bp, sk);
  for(i = 1; i < KYBER_K; i++) {
    polyvec_decompress_one(&bp, c, i);
    poly_ntt(&bp);
    poly_frombytes_mul_acc_16_16(&mp, &bp, sk + i*KYBER_POLYBYTES);
  }

  poly_reduce_centered(&mp);
  poly_invntt(&mp);

  /* w0 = v - s^T*b */
  poly_decompress(&v, c + KYBER_POLYVECCOMPRESSEDBYTES);
  poly_sub(&mp, &v, &mp);
  poly_reduce(&mp);

  poly_tomsg(m, &mp);
}
