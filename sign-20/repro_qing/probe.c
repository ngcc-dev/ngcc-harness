/* Instrumented probe: counts xof_squeeze calls in the UNMODIFIED rsdp_gen_secret_exp (the routine
 * sign.c calls on every signature to re-expand eta from the secret Seed_e). Prints one row per key. */
#define _POSIX_C_SOURCE 199309L
#include <stdlib.h>
#include "api.h"
#include "params.h"
#include "rsdp.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
extern unsigned long g_sq;
static double now(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec+1e-9*t.tv_nsec;}
int main(int argc,char**argv){
  int keys=argc>1?atoi(argv[1]):200;
  static unsigned char pk[CRYPTO_PUBLICKEYBYTES], sk[CRYPTO_SECRETKEYBYTES];
  uint8_t eta[PARAM_N];
  printf("key,draws_eta,rejections,sum_eta,count0,draws_eta_repeat,us_gen_exp\n");
  for(int k=0;k<keys;k++){
    crypto_sign_keypair(pk,sk);
    uint8_t se[PARAM_KEYSEED_BYTES], sp[PARAM_KEYSEED_BYTES];
    rsdp_keypair_seeds(se,sp,sk);
    g_sq=0; double t0=now(); rsdp_gen_secret_exp(eta,se); double t1=now(); unsigned long d=g_sq;
    g_sq=0; rsdp_gen_secret_exp(eta,se); unsigned long d2=g_sq;   /* re-expansion, as on every signature */
    long s=0,c0=0; for(int i=0;i<PARAM_N;i++){s+=eta[i];c0+=eta[i]==0;}
    printf("%d,%lu,%lu,%ld,%ld,%lu,%.1f\n",k,d,d-PARAM_N,s,c0,d2,1e6*(t1-t0));
  }
}

