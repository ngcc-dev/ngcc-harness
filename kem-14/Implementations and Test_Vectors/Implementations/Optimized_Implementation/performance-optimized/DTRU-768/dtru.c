#include "params.h"
#include "poly.h"
#include "pack.h"

int pke_keygen(unsigned char pk[DTRU_PKE_PUBLICKEYBYTES],
                      unsigned char sk[DTRU_PKE_SECRETKEYBYTES],
                      const unsigned char coins[DTRU_COINBYTES_KEYGEN])
{
  int r;
  poly fp, f, g, h, finv;

  poly_sample_keygen_f(&fp, coins);
  poly_sample_keygen_g(&g, coins + DTRU_CBD3_BYTES);
  poly_multi_p_avx(&f, &fp);
  // poly_multi_p(&f, &fp);
  f.coeffs[0] += 1;
  pack_sk(sk, &f);

  // AVX2 implementation
  poly_ntt_avx(&f, &f);
  poly_ntt_avx(&g, &g);
  r = poly_baseinv_avx(&finv, &f);
  poly_basemul_avx(&h, &g, &finv);
  poly_invntt_avx(&h, &h);
  poly_freeze_avx(&h);


  // poly_ntt(&f);
  // poly_ntt(&g);
  // r = poly_baseinv(&finv, &f);
  // poly_basemul(&h, &g, &finv);
  // poly_invntt(&h);
  // poly_freeze(&h);

  pack_pk(pk, &h);

  return r;
}

void pke_enc(unsigned char ct[DTRU_PKE_CIPHERTEXTBYTES],
                    const unsigned char pk[DTRU_PKE_PUBLICKEYBYTES],
                    const unsigned char m[DTRU_MSGBYTES],
                    const unsigned char coins[DTRU_COINBYTES_ENC])
{
  poly r, e, hr, sigma, c, hhat;

  unpack_pk(&hhat, pk);
  poly_sample_enc_r(&r, coins);
  poly_sample_enc_e(&e, coins + DTRU_CBD4_BYTES);

  // AVX2 implementation
  poly_ntt_avx(&hhat, &hhat);
  poly_ntt_avx(&r, &r);
  poly_basemul_avx(&hr, &hhat, &r);
  poly_invntt_avx(&hr, &hr);
  poly_add_avx(&sigma, &hr, &e);
  poly_freeze_avx(&sigma);

  // poly_ntt(&hhat);
  // poly_ntt(&r);
  // poly_basemul(&hr, &hhat, &r);
  // poly_invntt(&hr);
  // poly_add(&sigma, &hr, &e);
  // poly_freeze(&sigma);

  poly_encode_compress(&c, &sigma, m);
  pack_ct(ct, &c);
}

void pke_dec(unsigned char m[DTRU_MSGBYTES],
                    const unsigned char ct[DTRU_PKE_CIPHERTEXTBYTES],
                    const unsigned char sk[DTRU_PKE_SECRETKEYBYTES])
{
  poly cf, c, fhat;

  unpack_decompress_ct(&c, ct);
  unpack_sk(&fhat, sk);

  // AVX2 implementation
  poly_ntt_avx(&fhat, &fhat);
  poly_ntt_avx(&c, &c);
  poly_basemul_avx(&cf, &c, &fhat);
  poly_invntt_avx(&cf, &cf);
  poly_freeze_avx(&cf);

  // poly_ntt(&fhat);
  // poly_ntt(&c);
  // poly_basemul(&cf, &c, &fhat);
  // poly_invntt(&cf);
  // poly_freeze(&cf);

  poly_decode(m, &cf);
}