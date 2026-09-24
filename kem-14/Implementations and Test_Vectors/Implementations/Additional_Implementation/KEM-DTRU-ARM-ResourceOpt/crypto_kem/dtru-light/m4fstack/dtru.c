#include "params.h"
#include "poly.h"
#include "pack.h"

int pke_keygen(unsigned char pk[DTRU_PKE_PUBLICKEYBYTES],
                      unsigned char sk[DTRU_PKE_SECRETKEYBYTES],
                      const unsigned char coins[DTRU_COINBYTES_KEYGEN])
{
  int r;
  poly fp, f, g;

  poly_sample_keygen_f(&fp, coins);
  poly_sample_keygen_g(&g, coins + DTRU_CBD1_BYTES);
  poly_multi_p(&f, &fp);
  f.coeffs[0] += 1;
  pack_sk(sk, &f);

  poly_ntt(&f);
  poly_ntt(&g);
  r = poly_baseinv(&fp, &f);
  poly_basemul(&f, &g, &fp);
  poly_invntt(&f);
  poly_freeze(&f);

  pack_pk(pk, &f);

  return r;
}

void pke_enc(unsigned char ct[DTRU_PKE_CIPHERTEXTBYTES],
                    const unsigned char pk[DTRU_PKE_PUBLICKEYBYTES],
                    const unsigned char m[DTRU_MSGBYTES],
                    const unsigned char coins[DTRU_COINBYTES_ENC])
{
  poly r, e, hhat;

  unpack_pk(&hhat, pk);
  poly_sample_enc_r(&r, coins);
  poly_sample_enc_e(&e, coins + DTRU_CBD1_BYTES);

  poly_ntt(&hhat);
  poly_ntt(&r);
  poly_basemul(&hhat, &hhat, &r);
  poly_invntt(&hhat);
  poly_add(&hhat, &hhat, &e);
  poly_freeze(&hhat);

  poly_encode_compress(&hhat, &hhat, m);
  pack_ct(ct, &hhat);
}

void pke_dec(unsigned char m[DTRU_MSGBYTES],
                    const unsigned char ct[DTRU_PKE_CIPHERTEXTBYTES],
                    const unsigned char sk[DTRU_PKE_SECRETKEYBYTES])
{
  poly c, fhat;

  unpack_decompress_ct(&c, ct);
  unpack_sk(&fhat, sk);

  poly_ntt(&fhat);
  poly_ntt(&c);
  poly_basemul(&c, &c, &fhat);
  poly_invntt(&c);
  poly_freeze(&c);

  poly_decode(m, &c);
}
