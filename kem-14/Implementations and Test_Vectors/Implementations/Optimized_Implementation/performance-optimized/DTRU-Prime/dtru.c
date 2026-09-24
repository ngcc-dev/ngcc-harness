#include <stdio.h>
#include "params.h"
#include "poly.h"
#include "pack.h"
#include "radix_ntt_n1087.h"
#include "avx2_poly.h"
#include "avx2_ntt.h"

void pke_keygen(unsigned char pk[DTRU_PKE_PUBLICKEYBYTES],
                unsigned char sk[DTRU_PKE_SECRETKEYBYTES],
                const unsigned char coins[DTRU_COINBYTES_KEYGEN])
{
  poly fp, f, g, h, finv;

  poly_sample_keygen_f(&fp, coins);
  poly_sample_keygen_g(&g, coins + DTRU_CBD1_BYTES);
  poly_multi_p(&f, &fp);
  f.coeffs[0] += 1;
  pack_sk(sk, &f);

  poly_inverse(&finv, &f);
  poly_radix_ntt_n1087(&h, &finv, &g);
  poly_fqcsubq(&h);

  pack_pk(pk, &h);
}

void pke_enc(unsigned char ct[DTRU_PKE_CIPHERTEXTBYTES],
                    const unsigned char pk[DTRU_PKE_PUBLICKEYBYTES],
                    const unsigned char m[DTRU_MSGBYTES],
                    const unsigned char coins[DTRU_COINBYTES_ENC])
{
  poly r, e, hr, sigma, c, hhat;

  unpack_pk(&hhat, pk);

  poly_sample_enc_r(&r, coins);
  poly_sample_enc_e(&e, coins + DTRU_CBD2_BYTES);
  
  poly_radix_ntt_n1087(&hr, &hhat, &r);
  poly_add(&sigma, &hr, &e);
  poly_fqcsubq(&sigma);

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

  poly_radix_ntt_n1087(&cf, &c, &fhat);
  poly_fqcsubq(&cf);

  poly_decode(m, &cf);
}

void pke_keygen_avx2(unsigned char pk[DTRU_PKE_PUBLICKEYBYTES],
                unsigned char sk[DTRU_PKE_SECRETKEYBYTES],
                const unsigned char coins[DTRU_COINBYTES_KEYGEN])
{
  poly fp, f, g, h, finv;

  poly_sample_keygen_f_avx2(&fp, coins);
  poly_sample_keygen_g_avx2(&g, coins + DTRU_CBD1_BYTES);
  poly_multi_p_avx2(&f, &fp);
  f.coeffs[0] += 1;
  pack_sk(sk, &f);

  poly_inverse_avx2(&finv, &f);
  poly_radix_ntt_n1087_q1_intrinsic(&h, &finv, &g);
  poly_fqcsubq(&h);

  pack_pk(pk, &h);
}

void pke_enc_avx2(unsigned char ct[DTRU_PKE_CIPHERTEXTBYTES],
                    const unsigned char pk[DTRU_PKE_PUBLICKEYBYTES],
                    const unsigned char m[DTRU_MSGBYTES],
                    const unsigned char coins[DTRU_COINBYTES_ENC])
{
  poly r, e, hr, sigma, c, hhat;

  unpack_pk(&hhat, pk);

  poly_sample_enc_r_avx2(&r, coins);
  poly_sample_enc_e_avx2(&e, coins + DTRU_CBD2_BYTES);
  
  poly_radix_ntt_n1087_q1_intrinsic(&hr, &hhat, &r);
  poly_add_avx2(&sigma, &hr, &e);
  poly_fqcsubq_avx2(&sigma);

  poly_encode_compress(&c, &sigma, m);
  pack_ct(ct, &c);
}

void pke_dec_avx2(unsigned char m[DTRU_MSGBYTES],
                    const unsigned char ct[DTRU_PKE_CIPHERTEXTBYTES],
                    const unsigned char sk[DTRU_PKE_SECRETKEYBYTES])
{
  poly cf, c, fhat;

  unpack_decompress_ct(&c, ct);
  unpack_sk(&fhat, sk);

  poly_radix_ntt_n1087_q1_intrinsic(&cf, &c, &fhat);
  poly_fqcsubq_avx2(&cf);

  poly_decode(m, &cf);
}