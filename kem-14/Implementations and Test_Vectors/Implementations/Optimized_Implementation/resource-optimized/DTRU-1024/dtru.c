#include "params.h"
#include "poly.h"
#include "pack.h"

int pke_keygen(unsigned char pk[DTRU_PKE_PUBLICKEYBYTES],
                      unsigned char sk[DTRU_PKE_SECRETKEYBYTES],
                      const unsigned char coins[DTRU_COINBYTES_KEYGEN])
{
  int r;
  poly f, g, h, finv;

  poly_sample_keygen_f(&f, coins);
  poly_sample_keygen_g(&g, coins + DTRU_CBD2_BYTES);
  poly_multi_p(&f, &f);
  f.coeffs[0] += 1;
  pack_sk(sk, &f);

  poly_ntt(&f);
  poly_ntt(&g);
  r = poly_baseinv(&finv, &f);
  poly_basemul(&h, &g, &finv);
  poly_invntt(&h);
  poly_freeze(&h);

  pack_pk(pk, &h);

  return r;
}

void pke_enc(unsigned char ct[DTRU_PKE_CIPHERTEXTBYTES],
                    const unsigned char pk[DTRU_PKE_PUBLICKEYBYTES],
                    const unsigned char m[DTRU_MSGBYTES],
                    const unsigned char coins[DTRU_COINBYTES_ENC])
{
  poly r, e, hr, c, hhat;

  unpack_pk(&hhat, pk);
  poly_sample_enc_r(&r, coins);
  poly_sample_enc_e(&e, coins + DTRU_CBD3_BYTES);

  poly_ntt(&hhat);
  poly_ntt(&r);
  poly_basemul(&hr, &hhat, &r);
  poly_invntt(&hr);
  poly_add(&hr, &hr, &e);
  poly_freeze(&hr);

  poly_encode_compress(&c, &hr, m);
  pack_ct(ct, &c);
}

void pke_dec(unsigned char m[DTRU_MSGBYTES],
                    const unsigned char ct[DTRU_PKE_CIPHERTEXTBYTES],
                    const unsigned char sk[DTRU_PKE_SECRETKEYBYTES])
{
  poly cf, c, fhat;

  unpack_decompress_ct(&c, ct);
  unpack_sk(&fhat, sk);

  poly_ntt(&fhat);
  poly_ntt(&c);
  poly_basemul(&cf, &c, &fhat);
  poly_invntt(&cf);
  poly_freeze(&cf);

  poly_decode(m, &cf);
}
