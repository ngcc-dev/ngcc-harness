/* MAMBA-Viper implementation and implementation support layer where applicable. */
#include "../api.h"
#include "../rng.h"
#include "../viper.h"
#include "../viper_arith.h"
#include "../viper_message_codec.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int fail(const char *name) { printf("FAIL %s\n", name); return 1; }

static void oracle_matvec(vpolyvec out, vpoly A[VIPER_K][VIPER_K], const vpolyvec s, int transpose) {
  vpoly t;
  memset(out, 0, sizeof(vpolyvec));
  for (size_t i = 0; i < VIPER_K; i++) {
    for (size_t j = 0; j < VIPER_K; j++) {
      viper_poly_mul_schoolbook_oracle(t, transpose ? A[j][i] : A[i][j], s[j]);
      for (size_t k = 0; k < VIPER_N; k++) out[i][k] = (uint16_t)((out[i][k] + t[k]) & VIPER_Q_MASK);
    }
  }
}

static void oracle_dot(vpoly out, const vpolyvec a, const vpolyvec b) {
  vpoly t;
  memset(out, 0, sizeof(vpoly));
  for (size_t i = 0; i < VIPER_K; i++) {
    viper_poly_mul_schoolbook_oracle(t, a[i], b[i]);
    for (size_t k = 0; k < VIPER_N; k++) out[k] = (uint16_t)((out[k] + t[k]) & VIPER_Q_MASK);
  }
}

static void fill(uint16_t *a, size_t n, unsigned bits) { for (size_t i=0;i<n;i++) a[i]=(uint16_t)((i*73u+19u)&((1u<<bits)-1u)); }

typedef struct { int level; int pk; int ct; int sk; int ss; } expected_size_row;
static const expected_size_row expected_sizes[] = {
  {128, 608, 736, 1424, 16},
  {192, 992, 1088, 2200, 24},
  {256, 1312, 1472, 2912, 32},
  {384, 2496, 2656, 5488, 48},
  {512, 3200, 3456, 7040, 64},
};

static int check_expected_sizes(void) {
  for (size_t i = 0; i < sizeof(expected_sizes) / sizeof(expected_sizes[0]); i++) {
    if (expected_sizes[i].level == VIPER_LEVEL) {
      return CRYPTO_PUBLICKEYBYTES == expected_sizes[i].pk &&
             CRYPTO_CIPHERTEXTBYTES == expected_sizes[i].ct &&
             CRYPTO_SECRETKEYBYTES == expected_sizes[i].sk &&
             CRYPTO_BYTES == expected_sizes[i].ss;
    }
  }
  return 0;
}

int main(int argc, char **argv) {
  int trials = argc > 1 ? atoi(argv[1]) : 100;
  uint16_t a[VIPER_N], b[VIPER_N], c[VIPER_N], d[VIPER_N];
  unsigned char buf[512];
  if (CRYPTO_PUBLICKEYBYTES != VIPER_PUBLICKEYBYTES || CRYPTO_CIPHERTEXTBYTES != VIPER_CIPHERTEXTBYTES || CRYPTO_SECRETKEYBYTES != VIPER_SECRETKEYBYTES || CRYPTO_BYTES != VIPER_SSBYTES) return fail("size constants");
  if (!check_expected_sizes()) return fail("expected size table");
  unsigned widths[] = {3,4,5,9,10,12};
  for (size_t w=0; w<sizeof(widths)/sizeof(widths[0]); w++) {
    fill(a, VIPER_N, widths[w]);
    viper_pack_bits(buf, a, VIPER_N, widths[w]);
    memset(b,0,sizeof(b)); viper_unpack_bits(b, buf, VIPER_N, widths[w]);
    if (memcmp(a,b,sizeof(a))) return fail("pack/unpack");
  }

  unsigned char fast[512], slow[512];
  uint16_t dither[VIPER_N], recon_fast[VIPER_N], recon_slow[VIPER_N];
  fill(dither, VIPER_N, 9);
  fill(a, VIPER_N, 12);
#define CHECK_QP(T, BITS) do { \
    for (size_t i=0;i<VIPER_N;i++) b[i]=viper_quantize(a[i], dither[i], (BITS)); \
    viper_pack_bits(slow, b, VIPER_N, (BITS)); \
    viper_quantize_pack_t##T##_array(fast, a, dither); \
    if (memcmp(fast, slow, VIPER_N * (BITS) / 8)) return fail("quantize-pack-t" #T); \
    viper_unpack_reconstruct_t##T##_array(recon_fast, fast, dither); \
    viper_unpack_bits(b, slow, VIPER_N, (BITS)); \
    for (size_t i=0;i<VIPER_N;i++) recon_slow[i]=viper_reconstruct(b[i], dither[i], (BITS)); \
    if (memcmp(recon_fast, recon_slow, sizeof(recon_fast))) return fail("unpack-reconstruct-t" #T); \
  } while (0)
  if (VIPER_QLOG == 12) {
    CHECK_QP(10, 10);
    CHECK_QP(9, 9);
    CHECK_QP(4, 4);
    CHECK_QP(3, 3);
  }
  viper_quantize_pack_array(fast, a, dither, 5);
  for (size_t i=0;i<VIPER_N;i++) b[i]=viper_quantize(a[i], dither[i], 5);
  viper_pack_bits(slow, b, VIPER_N, 5);
  if (memcmp(fast, slow, VIPER_N * 5 / 8)) return fail("quantize-pack-t5");
  viper_unpack_reconstruct_array(recon_fast, fast, dither, 5);
  viper_unpack_bits(b, slow, VIPER_N, 5);
  for (size_t i=0;i<VIPER_N;i++) recon_slow[i]=viper_reconstruct(b[i], dither[i], 5);
  if (memcmp(recon_fast, recon_slow, sizeof(recon_fast))) return fail("unpack-reconstruct-t5");
#undef CHECK_QP
  unsigned char mu[VIPER_MU_BYTES]={1}, mu2[VIPER_MU_BYTES]={1};
  uint16_t du1[VIPER_K][VIPER_N], du2[VIPER_K][VIPER_N], dv1[VIPER_N], dv2[VIPER_N];
  viper_gen_dither(du1,dv1,mu); viper_gen_dither(du2,dv2,mu2);
  if (memcmp(du1,du2,sizeof(du1)) || memcmp(dv1,dv2,sizeof(dv1))) return fail("dither determinism");
  for (unsigned t=3;t<=VIPER_QLOG;t++) for (unsigned x=0;x<VIPER_Q;x+=37) { uint16_t q=viper_quantize(x,3,t); (void)viper_reconstruct(q,3,t); }
  unsigned char m[VIPER_MSGBYTES], m2[VIPER_MSGBYTES]; for (int i=0;i<VIPER_MSGBYTES;i++) m[i]=(unsigned char)(i*7+3);
  viper_encode(a,m); viper_decode(m2,a); if (memcmp(m,m2,VIPER_MSGBYTES)) return fail("encode/decode");
  fill(a,VIPER_N,12); fill(b,VIPER_N,12);
#if defined(VIPER_EXPERIMENTAL_TCHES2021_NTT) && (VIPER_EXPERIMENTAL_TCHES2021_NTT == 1)
  for (size_t i=0;i<VIPER_N;i++) b[i]=(uint16_t)((int)(i % (2 * VIPER_ETA_S + 1)) - VIPER_ETA_S) & VIPER_Q_MASK;
#endif
  viper_poly_mul(c,a,b); viper_poly_mul_schoolbook_oracle(d,a,b); if (memcmp(c,d,sizeof(c))) return fail("basebackend polymul oracle");
  vpoly A[VIPER_K][VIPER_K], odot, sdot; vpolyvec sv, mv, omv;
  unsigned char seed[32] = {7}; uint16_t dpk[VIPER_K][VIPER_N];
  viper_gen_public(A, dpk, seed); viper_sample_secret(sv, seed, VIPER_ETA_S);
  viper_matvec(mv, A, sv); oracle_matvec(omv, A, sv, 0); if (memcmp(mv, omv, sizeof(mv))) return fail("A*s matrix-vector");
  viper_genpublic_matvec_fused_experiment(omv, dpk, seed, sv, 0); if (memcmp(mv, omv, sizeof(mv))) return fail("fused A*s experiment");
  viper_matTvec(mv, A, sv); oracle_matvec(omv, A, sv, 1); if (memcmp(mv, omv, sizeof(mv))) return fail("A^T*r matrix-vector");
  viper_genpublic_matvec_fused_experiment(omv, dpk, seed, sv, 1); if (memcmp(mv, omv, sizeof(mv))) return fail("fused A^T*r experiment");
  viper_dot(sdot, sv, sv); oracle_dot(odot, sv, sv); if (memcmp(sdot, odot, sizeof(sdot))) return fail("dot products");
  unsigned char pk[CRYPTO_PUBLICKEYBYTES], sk[CRYPTO_SECRETKEYBYTES], ct[CRYPTO_CIPHERTEXTBYTES], ss1[CRYPTO_BYTES], ss2[CRYPTO_BYTES];
  for (int i=0;i<trials;i++) {
    crypto_kem_keypair(pk,sk); crypto_kem_enc(ct,ss1,pk); crypto_kem_dec(ss2,ct,sk);
    if (memcmp(ss1,ss2,CRYPTO_BYTES)) return fail("KEM correctness");
    unsigned char dm[VIPER_MSGBYTES], omega[VIPER_FALLBACK_KEY_BYTES + VIPER_MU_BYTES]; memset(dm,(unsigned char)i,VIPER_MSGBYTES); memset(omega,(unsigned char)(i+1),sizeof(omega));
    viper_pke_enc(ct,pk,dm,omega); viper_pke_dec(m2,sk,ct);
    if (memcmp(dm,m2,VIPER_MSGBYTES)) return fail("PKE correctness");
    if (!viper_reencrypt_check(ct,pk,dm,omega)) return fail("FO re-encryption");
  }
  crypto_kem_keypair(pk,sk); crypto_kem_enc(ct,ss1,pk); crypto_kem_dec(ss2,ct,sk);
  printf("%s validation ok: trials=%d\nVIPER_LEVEL=%d VIPER_Q=%d VIPER_QLOG=%d VIPER_K=%d VIPER_ETA_S=%d VIPER_ETA_R=%d VIPER_T_PK=%d VIPER_T_U=%d VIPER_T_V=%d VIPER_MSGBYTES=%d VIPER_SSBYTES=%d CRYPTO_PUBLICKEYBYTES=%d CRYPTO_CIPHERTEXTBYTES=%d CRYPTO_SECRETKEYBYTES=%d CRYPTO_BYTES=%d VIPER_USE_E8_CODEC=%d VIPER_E8_RATE=%d VIPER_E8_ALPHA=%d VIPER_E8_ACTIVE_BLOCKS=%d codec=%s\n", CRYPTO_ALGNAME, trials, VIPER_LEVEL, VIPER_Q, VIPER_QLOG, VIPER_K, VIPER_ETA_S, VIPER_ETA_R, VIPER_T_PK, VIPER_T_U, VIPER_T_V, VIPER_MSGBYTES, VIPER_SSBYTES, CRYPTO_PUBLICKEYBYTES, CRYPTO_CIPHERTEXTBYTES, CRYPTO_SECRETKEYBYTES, CRYPTO_BYTES, VIPER_USE_E8_CODEC, VIPER_E8_RATE, VIPER_E8_ALPHA, VIPER_E8_ACTIVE_BLOCKS, viper_message_codec_name());
  printf("fixed-seed-style dump first bytes: pk=%02x sk=%02x ct=%02x ss_enc=%02x ss_dec=%02x\n", pk[0], sk[0], ct[0], ss1[0], ss2[0]);
  return 0;
}
