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
  poly_sample_keygen_g(&g, coins + DTRU_CBD1_BYTES);
  poly_multi_p(&f, &fp);
  f.coeffs[0] += 1;
  // pack_sk(sk, &f);
  pack_sk_avx(sk, &f);

  poly_ntt_avx2(&f);
  poly_ntt_avx2(&g);
  r = poly_baseinv_avx2(&finv, &f);
  poly_basemul_avx2(&h, &g, &finv);
  poly_invntt_avx2(&h);
  // poly_freeze_avx(&h);
  poly_freeze_avx2(&h);

  // poly_ntt(&f);
  // poly_ntt(&g);
  // r = poly_baseinv(&finv, &f);
  // poly_basemul(&h, &g, &finv);
  // poly_invntt(&h);
  // poly_freeze(&h);

  // pack_pk(pk, &h);
  pack_pk_avx(pk, &h);

  return r;
}

void pke_enc(unsigned char ct[DTRU_PKE_CIPHERTEXTBYTES],
                    const unsigned char pk[DTRU_PKE_PUBLICKEYBYTES],
                    const unsigned char m[DTRU_MSGBYTES],
                    const unsigned char coins[DTRU_COINBYTES_ENC])
{
  poly r, e, hr, sigma, c, hhat;

  // unpack_pk(&hhat, pk);
  unpack_pk_avx(&hhat, pk);
  poly_sample_enc_r(&r, coins);
  poly_sample_enc_e(&e, coins + DTRU_CBD1_BYTES);

  poly_ntt_avx(&hhat);
  poly_ntt_avx(&r);
  poly_basemul_avx(&hr, &hhat, &r);
  poly_invntt_avx(&hr);
  poly_add_avx(&sigma, &hr, &e);
  // poly_freeze_avx(&sigma);
  poly_freeze_avx2(&sigma);

  // poly_ntt(&hhat);
  // poly_ntt(&r);
  // poly_basemul(&hr, &hhat, &r);
  // poly_invntt(&hr);
  // poly_add(&sigma, &hr, &e);
  // poly_freeze(&sigma);

  poly_encode_compress(&c, &sigma, m);
  // pack_ct(ct, &c);
  pack_ct_avx(ct, &c);
}

void pke_dec(unsigned char m[DTRU_MSGBYTES],
                    const unsigned char ct[DTRU_PKE_CIPHERTEXTBYTES],
                    const unsigned char sk[DTRU_PKE_SECRETKEYBYTES])
{
  poly cf, c, fhat;

  // unpack_decompress_ct(&c, ct);
  unpack_decompress_ct_avx(&c, ct);
  // unpack_sk(&fhat, sk);
  unpack_sk_avx(&fhat, sk);

  poly_ntt_avx(&fhat);
  poly_ntt_avx(&c);
  poly_basemul_avx(&cf, &c, &fhat);
  poly_invntt_avx(&cf);
  // poly_freeze_avx(&cf);
  poly_freeze_avx2(&cf);

  // poly_ntt(&fhat);
  // poly_ntt(&c);
  // poly_basemul(&cf, &c, &fhat);
  // poly_invntt(&cf);
  // poly_freeze(&cf);

  // poly_decode(m, &cf);
  poly_decode_avx(m, &cf);
}

#include <stdio.h>
#include "cpucycles.h"

void pke_dec_profile(unsigned char m[DTRU_MSGBYTES],
                     const unsigned char ct[DTRU_PKE_CIPHERTEXTBYTES],
                     const unsigned char sk[DTRU_PKE_SECRETKEYBYTES])
{
  poly cf, c, fhat;
  uint64_t start, end;
  uint64_t time_unpack_ct, time_unpack_sk;
  uint64_t time_ntt_f, time_ntt_c;
  uint64_t time_basemul;
  uint64_t time_invntt;
  uint64_t time_freeze;
  uint64_t time_decode;
  uint64_t total;

  printf("\n=========== pke_dec Profiling ===========\n");

  // 1. Unpack Ciphertext
  start = cpucycles();
  unpack_decompress_ct_avx(&c, ct);
  end = cpucycles();
  time_unpack_ct = end - start;

  // 2. Unpack Secret Key
  start = cpucycles();
  unpack_sk_avx(&fhat, sk);
  end = cpucycles();
  time_unpack_sk = end - start;

  // 3. NTT on Secret Key Poly
  start = cpucycles();
  poly_ntt_avx2(&fhat);
  end = cpucycles();
  time_ntt_f = end - start;

  // 4. NTT on Ciphertext Poly
  start = cpucycles();
  poly_ntt_avx2(&c);
  end = cpucycles();
  time_ntt_c = end - start;

  // 5. Base Multiplication
  start = cpucycles();
  poly_basemul_avx2(&cf, &c, &fhat);
  end = cpucycles();
  time_basemul = end - start;

  // 6. Inverse NTT
  start = cpucycles();
  poly_invntt_avx2(&cf);
  end = cpucycles();
  time_invntt = end - start;

  // 7. Freeze (Reduction)
  start = cpucycles();
  poly_freeze_avx2(&cf);
  end = cpucycles();
  time_freeze = end - start;

  // 8. Decode to message
  start = cpucycles();
  poly_decode(m, &cf);
  end = cpucycles();
  time_decode = end - start;

  total = time_unpack_ct + time_unpack_sk + time_ntt_f + 
          time_ntt_c + time_basemul + time_invntt + 
          time_freeze + time_decode;

  printf("Unpack CT:    %10lu cycles (%5.2f%%)\n", time_unpack_ct, (double)time_unpack_ct / total * 100.0);
  printf("Unpack SK:    %10lu cycles (%5.2f%%)\n", time_unpack_sk, (double)time_unpack_sk / total * 100.0);
  printf("NTT (SK):     %10lu cycles (%5.2f%%)\n", time_ntt_f,     (double)time_ntt_f / total * 100.0);
  printf("NTT (CT):     %10lu cycles (%5.2f%%)\n", time_ntt_c,     (double)time_ntt_c / total * 100.0);
  printf("Basemul:      %10lu cycles (%5.2f%%)\n", time_basemul,   (double)time_basemul / total * 100.0);
  printf("InvNTT:       %10lu cycles (%5.2f%%)\n", time_invntt,    (double)time_invntt / total * 100.0);
  printf("Freeze:       %10lu cycles (%5.2f%%)\n", time_freeze,    (double)time_freeze / total * 100.0);
  printf("Decode:       %10lu cycles (%5.2f%%)\n", time_decode,    (double)time_decode / total * 100.0);
  printf("----------------------------------------\n");
  printf("Sum Total:    %10lu cycles\n", total);
  printf("========================================\n\n");
}
