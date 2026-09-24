/* MAMBA-Viper implementation and implementation support layer where applicable. */
#include "../api.h"
#include "../rng.h"
#include "../viper.h"
#include "../viper_arith.h"
#include "cpucycles.h"
#include <stdio.h>
#include <string.h>

#define LOOPS 20
static unsigned long long diff(unsigned long long a, unsigned long long b){return b-a;}
typedef struct { int level; int pk; int ct; int sk; int ss; } expected_size_row;
static const expected_size_row expected_sizes[] = {
  {128, 608, 736, 1424, 16},
  {192, 992, 1088, 2200, 24},
  {256, 1312, 1472, 2912, 32},
  {384, 2496, 2656, 5488, 48},
  {512, 3200, 3456, 7040, 64},
};
static const expected_size_row *active_expected_sizes(void){
  for(size_t i=0;i<sizeof(expected_sizes)/sizeof(expected_sizes[0]);i++) if(expected_sizes[i].level==VIPER_LEVEL) return &expected_sizes[i];
  return 0;
}
int main(void){
  unsigned char pk[CRYPTO_PUBLICKEYBYTES], sk[CRYPTO_SECRETKEYBYTES], ct[CRYPTO_CIPHERTEXTBYTES], ss[CRYPTO_BYTES], m[VIPER_MSGBYTES]={0}, mu[VIPER_MU_BYTES]={3}, omega[VIPER_FALLBACK_KEY_BYTES + VIPER_MU_BYTES]={1}, rho[32]={2};
  vpoly A[VIPER_K][VIPER_K]; uint16_t dpk[VIPER_K][VIPER_N], du[VIPER_K][VIPER_N], dv[VIPER_N], vals[VIPER_N]; vpolyvec s; unsigned char packed[VIPER_PACKED_PK_POLYBYTES];
  /* Match viper_gen_public_shake(): A uses VIPER_QLOG bits per coefficient. */
  unsigned char pubbuf[(VIPER_K * VIPER_K * VIPER_N * VIPER_QLOG / 8u) + (VIPER_K * VIPER_N * 2u / 8u)];
  unsigned long long t0,t1,total;
#define BENCH(label, stmt) do{ total=0; for(int i=0;i<LOOPS;i++){ t0=cpucycles(); stmt; t1=cpucycles(); total += diff(t0,t1);} printf("%-24s %llu cycles\n", label, total/LOOPS);}while(0)
  const expected_size_row *sizes = active_expected_sizes();
  if(!sizes) return 1;
  printf("%s benchmark (current polynomial-multiplication backend)\n", CRYPTO_ALGNAME);
  printf("expected sizes: pk=%d ct=%d sk=%d ss=%d\n", sizes->pk, sizes->ct, sizes->sk, sizes->ss);
  BENCH("PKE KeyGen", viper_pke_keypair(pk,sk,rho,rho));
  viper_pke_keypair(pk,sk,rho,rho);
  BENCH("PKE Enc", viper_pke_enc(ct,pk,m,omega));
  BENCH("PKE Dec", viper_pke_dec(m,sk,ct));
  BENCH("KEM KeyGen", crypto_kem_keypair(pk,sk));
  crypto_kem_keypair(pk,sk);
  BENCH("KEM Encaps", crypto_kem_enc(ct,ss,pk));
  crypto_kem_enc(ct,ss,pk);
  BENCH("KEM Decaps", crypto_kem_dec(ss,ct,sk));
  BENCH("GenPublic total", viper_gen_public(A,dpk,rho));
  BENCH("GenPublic SHAKE", viper_gen_public_shake(pubbuf,rho));
  viper_gen_public_shake(pubbuf,rho);
  BENCH("GenPublic parse A", viper_gen_public_parse_A(A,pubbuf));
  BENCH("GenPublic parse d_pk", viper_gen_public_parse_dpk(dpk,pubbuf));
  BENCH("GenPublic store zero", { memset(A,0,sizeof(A)); memset(dpk,0,sizeof(dpk)); });
  /* viper_gen_dither consumes VIPER_MU_BYTES; VIPER_MSGBYTES is smaller for some benchmark levels. */
  BENCH("GenDither", viper_gen_dither(du,dv,mu));
  BENCH("sample secret", viper_sample_secret(s,rho,VIPER_ETA_S));
  viper_gen_public(A,dpk,rho); viper_sample_secret(s,rho,VIPER_ETA_S);
  BENCH("A*s", ({vpolyvec out; viper_matvec(out,A,s);}));
  BENCH("A^T*r", ({vpolyvec out; viper_matTvec(out,A,s);}));
  BENCH("b_hat^T*r", ({vpoly acc; viper_dot(acc,s,s);}));
  BENCH("s^T*u", ({vpoly acc; viper_dot(acc,s,s);}));
  BENCH("quantize/reconstruct", for(size_t i=0;i<VIPER_N;i++){ vals[i]=viper_reconstruct(viper_quantize((uint16_t)i,3,VIPER_T_PK),3,VIPER_T_PK); });
  BENCH("pack/unpack", {viper_pack_bits(packed, vals, VIPER_N, VIPER_T_PK); viper_unpack_bits(vals, packed, VIPER_N, VIPER_T_PK);});
  BENCH("FO re-encryption", viper_reencrypt_check(ct,pk,m,omega));
  return 0;
}
