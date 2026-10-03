/* HARE (NGCC kem-16): end-to-end check of everything in the same-key DS-DOOM attack EXCEPT the
 * decoding search itself, on the unmodified vendor reference core.
 *
 * Victim : official crypto_kem_keypair / crypto_kem_enc / crypto_kem_dec.
 * ORACLE : (r1,r2) = the output a successful DS-DOOM decoder returns for one target. It is
 *          re-derived from the encapsulation randomness (marked ORACLE below); this stands in for
 *          the 2^{lambda-eps} search, which is priced separately by the official estimator.
 * Attacker: reads ONLY pk, ct and (r1,r2) -- never sk -- and must output the session key K.
 * SCORING: K from crypto_kem_dec(sk) is used only to score the attacker's K.
 *
 * Checks per trial:
 *  [api]  replayed encapsulation == official crypto_kem_enc output byte-for-byte
 *  [syn]  u == r1 + h*r2 mod X^n-1, h fixed by pk  -> one SD instance [2n, n, 2wr] per ciphertext
 *  [wt ]  |r1| = |r2| = wr
 *  [rot]  for random j: X^j*u == X^j*r1 + h*(X^j*r2), weights kept -> n solutions per target (rot = n)
 *  [K  ]  m = Decode(Decompress(v) - Trunc(s*r2)), K' = G(H(pk)||m||salt) equals the real K
 *  [neg]  a wrong decoder output (one bit of r2 moved) fails the public syndrome test */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "api.h"
#include "parameters.h"
#include "hqc.h"
#include "parsing.h"
#include "vector.h"
#include "gf2x.h"
#include "code.h"
#include "compression.h"
#include "symmetric.h"
#include "data_structures.h"
#include <stdlib.h>
#include "drng.h"
DRNG_ctx drng_algorithm;

static int wt(const uint64_t *v){int w=0;for(int i=0;i<VEC_N_SIZE_64;i++)w+=__builtin_popcountll(v[i]);return w;}

int main(int argc,char**argv){
  int trials = argc>1? atoi(argv[1]) : 20;
  static uint8_t pk[PUBLIC_KEY_BYTES], sk[SECRET_KEY_BYTES], ct[CIPHERTEXT_BYTES], ct2[CIPHERTEXT_BYTES];
  uint8_t K[SHARED_SECRET_BYTES], Kdec[SHARED_SECRET_BYTES], Katt[SHARED_SECRET_BYTES];
  uint8_t ent[48]; for(int i=0;i<48;i++) ent[i]=(uint8_t)(i*7+1);
  int ok_api=0, ok_syn=0, ok_w=0, ok_rot=0, ok_K=0, ok_neg=0;
  for(int t=0;t<trials;t++){
    ent[0]=(uint8_t)t; ent[1]=(uint8_t)(t>>8);
    /* --- victim, official API --- */
    init_random_number(&drng_algorithm,ent,48); crypto_kem_keypair(pk,sk);
    crypto_kem_enc(ct,K,pk);
    crypto_kem_dec(Kdec,ct,sk);                      /* SCORING only */
    /* --- ORACLE: replay encaps with the same DRNG to obtain the ground-truth (r1,r2) --- */
    init_random_number(&drng_algorithm,ent,48); crypto_kem_keypair(pk,sk);
    uint8_t m[PARAM_SECURITY_BYTES], salt[SALT_BYTES], hek[SEED_BYTES], Kt[SHARED_SECRET_BYTES+SEED_BYTES];
    prng_get_bytes(m,PARAM_SECURITY_BYTES); prng_get_bytes(salt,SALT_BYTES);
    hash_h(hek,pk); hash_g(Kt,hek,m,salt);
    ciphertext_kem_t ck={0};
    hqc_pke_encrypt(&ck.c_pke,pk,m,Kt+SHARED_SECRET_BYTES); memcpy(ck.salt,salt,SALT_BYTES);
    hqc_c_kem_to_string(ct2,&ck);
    if(!memcmp(ct,ct2,CIPHERTEXT_BYTES)&&!memcmp(K,Kt,SHARED_SECRET_BYTES)&&!memcmp(K,Kdec,SHARED_SECRET_BYTES)) ok_api++;
    uint64_t r1[VEC_N_SIZE_64]={0}, r2[VEC_N_SIZE_64]={0}, e[VEC_N_SIZE_64]={0};
    shake256_xof_ctx x; xof_init(&x,Kt+SHARED_SECRET_BYTES,SEED_BYTES);
    vect_sample_fixed_weight2(&x,r2,PARAM_OMEGA_R);
    vect_sample_fixed_weight2(&x,e,PARAM_OMEGA_E);
    vect_sample_fixed_weight2(&x,r1,PARAM_OMEGA_R);

    /* ===== attacker: sees pk, ct, decoder output (r1,r2) only ===== */
    uint64_t h[VEC_N_SIZE_64]={0}, s[VEC_N_SIZE_64]={0}, u2[VEC_N_SIZE_64]={0};
    hqc_ek_pke_from_string(h,s,pk);
    ciphertext_pke_t cp; uint8_t salt_a[SALT_BYTES];
    memset(&cp,0,sizeof cp); hqc_c_kem_from_string(&cp,salt_a,ct);
    /* (1) DS-DOOM instance: u == r1 + h*r2 with |r1|=|r2|=wr, h fixed by pk */
    vect_mul(u2,r2,h); vect_add(u2,r1,u2,VEC_N_SIZE_64);
    if(!memcmp(u2,cp.u,VEC_N_SIZE_BYTES)) ok_syn++;
    if(wt(r1)==PARAM_OMEGA_R && wt(r2)==PARAM_OMEGA_R) ok_w++;
    /* rotation: X^j * (r1,r2) solves the target X^j * u (quasi-cyclic, mod X^n-1) */
    { uint64_t xj[VEC_N_SIZE_64]={0}, a[VEC_N_SIZE_64]={0}, b[VEC_N_SIZE_64]={0}, ur[VEC_N_SIZE_64]={0}, t2[VEC_N_SIZE_64]={0};
      uint32_t j = (uint32_t)(((uint64_t)t*2654435761u + 12345u) % PARAM_N);
      xj[j/64] = 1ULL << (j%64);
      vect_mul(a,r1,xj); vect_mul(b,r2,xj); vect_mul(ur,cp.u,xj);
      vect_mul(t2,b,h); vect_add(t2,a,t2,VEC_N_SIZE_64);
      if(!memcmp(t2,ur,VEC_N_SIZE_BYTES) && wt(a)==PARAM_OMEGA_R && wt(b)==PARAM_OMEGA_R) ok_rot++; }
    /* (2) m from v - Trunc(s*r2) = C(m) + Trunc(e) (+ KR distortion) */
    uint64_t v[VEC_N_SIZE_64]={0}, tmp[VEC_N_SIZE_64]={0};
    ciphertext_decompress(v,cp.m_tilde,cp.v2);
    vect_mul(tmp,r2,s); vect_truncate(tmp); vect_add(v,v,tmp,VEC_N1N2_SIZE_64);
    uint8_t ma[PARAM_SECURITY_BYTES]={0}; code_decode(ma,v);
    uint8_t ha[SEED_BYTES], Ka[SHARED_SECRET_BYTES+SEED_BYTES];
    hash_h(ha,pk); hash_g(Ka,ha,ma,salt_a); memcpy(Katt,Ka,SHARED_SECRET_BYTES);
    if(!memcmp(Katt,K,SHARED_SECRET_BYTES)) ok_K++;
    /* (3) negative control: a wrong decoder output (one bit of r2 moved) must not verify */
    r2[0]^=1; vect_mul(u2,r2,h); vect_add(u2,r1,u2,VEC_N_SIZE_64);
    if(memcmp(u2,cp.u,VEC_N_SIZE_BYTES)) ok_neg++;
  }
  printf("n=%d wr=%d we=%d trials=%d | [api] %d  [syn] u==r1+h*r2 %d  [wt] %d  [rot] %d  [K] recovered w/o sk %d  [neg] wrong output rejected %d\n",
    PARAM_N,PARAM_OMEGA_R,PARAM_OMEGA_E,trials,ok_api,ok_syn,ok_w,ok_rot,ok_K,ok_neg);
  int fail = !(ok_api==trials&&ok_syn==trials&&ok_w==trials&&ok_rot==trials&&ok_K==trials&&ok_neg==trials);
  if (!fail) puts("ATTACK kem-16-2 CONFIRMED: decoder output recovers the encapsulated key from public inputs");
  return fail;
}
