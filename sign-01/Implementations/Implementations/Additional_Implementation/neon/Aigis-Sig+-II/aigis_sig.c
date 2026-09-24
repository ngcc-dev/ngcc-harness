#include <stdio.h>
#include "api.h"
#include "randombytes.h"
#include "speed.h"
#include "cpucycles.h"
#include "polyvec.h"
#include "sample.h"

#define MLEN 59

//#define NTESTS 10000

#define NTESTS 30000

int main(void)
{
  unsigned int i;
  int ret;
  int32_t mlen=MLEN, smlen;
  unsigned char m[64];
  unsigned char sm[SIG_BYTES + 32];
  unsigned char pk[SIG_PUBLICKEYBYTES +32];
  unsigned char sk[SIG_SECRETKEYBYTES +32];
  unsigned long long t0[NTESTS], t1[NTESTS], t2[NTESTS];
  unsigned char seed[20] = { 0 };
  unsigned long long sl[NTESTS];

  uint16_t nonce = 0;
  poly     c, chat;
  polyvecl mat[PARAM_K], s1, y, yhat,z;
  polyveck s2,  w, w1;
  polyveck h, wcs2, tmp;
  unsigned char buf [SEEDBYTES + CRHBYTES + MLEN];
  polyvecl mat1[PARAM_K];

  int rett = 1;
  msig_keygen(pk, sk);
  msig_sign(sk,m,mlen,sm,&smlen);
  rett = msig_verf(pk,sm,smlen,m,mlen);
  


  printf("\n**********************************\n");
  printf("PK Sizes : %d\n",SIG_PUBLICKEYBYTES);
  printf("SK Sizes : %d\n",SIG_SECRETKEYBYTES);
  printf("SIG MAX Sizes: %d\n",SIG_BYTES);
  printf("SIG MIN Sizes: %d\n",SIG_MIN_SIZE_PACKED);
  printf("Com. Sizes: %d\n", SIG_PUBLICKEYBYTES + SIG_BYTES);
  printf("**********************************\n\n");


  for(i = 0; i < NTESTS; ++i) 
  {
    randombytes(m, MLEN);

    t0[i] = cpucycles();
    msig_keygen(pk, sk);
    t0[i] = cpucycles() - t0[i];
	
    t1[i] = cpucycles();
    msig_sign(sk,m,mlen,sm,&smlen);
    t1[i] = cpucycles() - t1[i];
    sl[i] = smlen;
    
    t2[i] = cpucycles();
    ret=msig_verf(pk,sm,smlen,m,mlen);
    t2[i] = cpucycles() - t2[i];

    
    if(ret!=0) {
      printf("Verification failed %d\n",i);
	  break;
    }
  }

  print_results("keygen:", t0, NTESTS);
  print_results("sign: ", t1, NTESTS);
  print_results("verify: ", t2, NTESTS);


  return 0;
}
