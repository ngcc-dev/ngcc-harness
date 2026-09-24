#include <stdio.h>
#include "params.h"
#include "poly.h"
#include "pack.h"
#include "radix_ntt_n1087.h"

void pke_keygen(unsigned char pk[DTRU_PKE_PUBLICKEYBYTES],
                unsigned char sk[DTRU_PKE_SECRETKEYBYTES],
                const unsigned char coins[DTRU_COINBYTES_KEYGEN])
{
  poly f, g, finv;

  poly_sample_keygen_f(&f, coins);
  poly_sample_keygen_g(&g, coins + DTRU_CBD1_BYTES);
  poly_multi_p(&f, &f);
  f.coeffs[0] += 1;
  pack_sk(sk, &f);

  poly_inverse(&finv, &f);
  poly_radix_ntt_n1087(&f, &finv, &g);
  poly_fqcsubq(&f);

  pack_pk(pk, &f);
}

void pke_enc(unsigned char ct[DTRU_PKE_CIPHERTEXTBYTES],
                    const unsigned char pk[DTRU_PKE_PUBLICKEYBYTES],
                    const unsigned char m[DTRU_MSGBYTES],
                    const unsigned char coins[DTRU_COINBYTES_ENC])
{
  poly r, e, hhat;

  unpack_pk(&hhat, pk);

  poly_sample_enc_r(&r, coins);
  poly_sample_enc_e(&e, coins + DTRU_CBD2_BYTES);
  
  poly_radix_ntt_n1087(&hhat, &hhat, &r);
  poly_add(&hhat, &hhat, &e);
  poly_fqcsubq(&hhat);

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

  poly_radix_ntt_n1087(&c, &c, &fhat);
  poly_fqcsubq(&c);

  poly_decode(m, &c);
}
