#include "params.h"
#include "poly.h"
#include "pack.h"

int pke_keygen(unsigned char pk[DTRU_PKE_PUBLICKEYBYTES],
                      unsigned char sk[DTRU_PKE_SECRETKEYBYTES],
                      const unsigned char coins[DTRU_COINBYTES_KEYGEN])
{
  int r;
  poly fp, f, g, h;
  poly_simd f_simd, g_simd, finv_simd, h_simd;

  poly_sample_keygen_f(&fp, coins);
  poly_sample_keygen_g(&g, coins + DTRU_CBD2_BYTES);
  poly_multi_p_avx2(&f, &fp);
  f.coeffs[0] += 1;
  pack_sk_avx2(sk, &f);

  poly_ntt_simd_v2(&f_simd, &f);
  poly_ntt_simd_v2(&g_simd, &g);
  r = poly_baseinv_simd(&finv_simd, &f_simd);
  poly_basemul_simd(&h_simd, &g_simd, &finv_simd);
  poly_invntt_simd(&h, &h_simd);
  poly_freeze_avx2(&h);

  pack_pk(pk, &h);
  return r;
}

void pke_enc(unsigned char ct[DTRU_PKE_CIPHERTEXTBYTES],
                    const unsigned char pk[DTRU_PKE_PUBLICKEYBYTES],
                    const unsigned char m[DTRU_MSGBYTES],
                    const unsigned char coins[DTRU_COINBYTES_ENC])
{
  poly r, e, hr, sigma, c, hhat;
  poly_simd hhat_simd, r_simd, hr_simd;

  unpack_pk(&hhat, pk);
  poly_sample_enc_r(&r, coins);
  poly_sample_enc_e(&e, coins + DTRU_CBD2_BYTES);

  poly_ntt_simd_v2(&hhat_simd, &hhat);
  poly_ntt_simd_v2(&r_simd, &r);
  poly_basemul_simd(&hr_simd, &hhat_simd, &r_simd);
  poly_invntt_simd(&hr, &hr_simd);
  poly_add_avx2(&sigma, &hr, &e);
  poly_freeze_avx2(&sigma);

  poly_encode_compress(&c, &sigma, m);
  pack_ct(ct, &c);
}

void pke_dec(unsigned char m[DTRU_MSGBYTES],
                    const unsigned char ct[DTRU_PKE_CIPHERTEXTBYTES],
                    const unsigned char sk[DTRU_PKE_SECRETKEYBYTES])
{
  poly cf, c, fhat;
  poly_simd fhat_simd, c_simd, cf_simd;

  unpack_decompress_ct(&c, ct);
  unpack_sk_avx2(&fhat, sk);

  poly_ntt_simd_v2(&fhat_simd, &fhat);
  poly_ntt_simd_v2(&c_simd, &c);
  poly_basemul_simd(&cf_simd, &c_simd, &fhat_simd);
  poly_invntt_simd(&cf, &cf_simd);
  poly_freeze_avx2(&cf);

  poly_decode(m, &cf);
}
